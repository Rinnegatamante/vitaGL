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

#define GL_POINTS                                       0x0000
#define GL_LINES                                        0x0001
#define GL_LINE_LOOP                                    0x0002
#define GL_LINE_STRIP                                   0x0003
#define GL_TRIANGLES                                    0x0004
#define GL_TRIANGLE_STRIP                               0x0005
#define GL_TRIANGLE_FAN                                 0x0006
#define GL_QUADS                                        0x0007
#define GL_POLYGON                                      0x0009
#define GL_ADD                                          0x0104
#define GL_NEVER                                        0x0200
#define GL_NEVER                                        0x0200
#define GL_LESS                                         0x0201
#define GL_EQUAL                                        0x0202
#define GL_LEQUAL                                       0x0203
#define GL_GREATER                                      0x0204
#define GL_NOTEQUAL                                     0x0205
#define GL_GEQUAL                                       0x0206
#define GL_ALWAYS                                       0x0207
#define GL_SRC_COLOR                                    0x0300
#define GL_ONE_MINUS_SRC_COLOR                          0x0301
#define GL_SRC_ALPHA                                    0x0302
#define GL_ONE_MINUS_SRC_ALPHA                          0x0303
#define GL_DST_ALPHA                                    0x0304
#define GL_ONE_MINUS_DST_ALPHA                          0x0305
#define GL_DST_COLOR                                    0x0306
#define GL_ONE_MINUS_DST_COLOR                          0x0307
#define GL_SRC_ALPHA_SATURATE                           0x0308
#define GL_FRONT                                        0x0404
#define GL_BACK                                         0x0405
#define GL_FRONT_AND_BACK                               0x0408
#define GL_INVALID_ENUM                                 0x0500
#define GL_INVALID_VALUE                                0x0501
#define GL_INVALID_OPERATION                            0x0502
#define GL_STACK_OVERFLOW                               0x0503
#define GL_STACK_UNDERFLOW                              0x0504
#define GL_OUT_OF_MEMORY                                0x0505
#define GL_EXP                                          0x0800
#define GL_EXP2                                         0x0801
#define GL_CW                                           0x0900
#define GL_CCW                                          0x0901
#define GL_CURRENT_COLOR                                0x0B00
#define GL_POLYGON_MODE                                 0x0B40
#define GL_CULL_FACE                                    0x0B44
#define GL_LIGHTING                                     0x0B50
#define GL_LIGHT_MODEL_AMBIENT                          0x0B53
#define GL_COLOR_MATERIAL                               0x0B57
#define GL_FOG                                          0x0B60
#define GL_FOG_DENSITY                                  0x0B62
#define GL_FOG_START                                    0x0B63
#define GL_FOG_END                                      0x0B64
#define GL_FOG_MODE                                     0x0B65
#define GL_FOG_COLOR                                    0x0B66
#define GL_DEPTH_TEST                                   0x0B71
#define GL_DEPTH_WRITEMASK                              0x0B72
#define GL_STENCIL_TEST                                 0x0B90
#define GL_MATRIX_MODE                                  0x0BA0
#define GL_NORMALIZE                                    0x0BA1
#define GL_VIEWPORT                                     0x0BA2
#define GL_MODELVIEW_MATRIX                             0x0BA6
#define GL_PROJECTION_MATRIX                            0x0BA7
#define GL_TEXTURE_MATRIX                               0x0BA8
#define GL_ALPHA_TEST                                   0x0BC0
#define GL_BLEND_DST                                    0x0BE0
#define GL_BLEND_SRC                                    0x0BE1
#define GL_BLEND                                        0x0BE2
#define GL_SCISSOR_BOX                                  0x0C10
#define GL_SCISSOR_TEST                                 0x0C11
#define GL_DOUBLEBUFFER                                 0x0C32
#define GL_UNPACK_ROW_LENGTH                            0x0CF2
#define GL_UNPACK_ALIGNMENT                             0x0CF5
#define GL_PACK_ALIGNMENT                               0x0D05
#define GL_ALPHA_SCALE                                  0x0D1C
#define GL_MAX_LIGHTS                                   0x0D31
#define GL_MAX_CLIP_PLANES                              0x0D32
#define GL_MAX_TEXTURE_SIZE                             0x0D33
#define GL_MAX_MODELVIEW_STACK_DEPTH                    0x0D36
#define GL_MAX_PROJECTION_STACK_DEPTH                   0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH                      0x0D39
#define GL_MAX_VIEWPORT_DIMS                            0x0D3A
#define GL_RED_BITS                                     0x0D52
#define GL_GREEN_BITS                                   0x0D53
#define GL_BLUE_BITS                                    0x0D54
#define GL_ALPHA_BITS                                   0x0D55
#define GL_DEPTH_BITS                                   0x0D56
#define GL_STENCIL_BITS                                 0x0D57
#define GL_TEXTURE_1D                                   0x0DE0
#define GL_TEXTURE_2D                                   0x0DE1
#define GL_DONT_CARE                                    0x1100
#define GL_FASTEST                                      0x1101
#define GL_NICEST                                       0x1102
#define GL_AMBIENT                                      0x1200
#define GL_DIFFUSE                                      0x1201
#define GL_SPECULAR                                     0x1202
#define GL_POSITION                                     0x1203
#define GL_CONSTANT_ATTENUATION                         0x1207
#define GL_LINEAR_ATTENUATION                           0x1208
#define GL_QUADRATIC_ATTENUATION                        0x1209
#define GL_COMPILE                                      0x1300
#define GL_COMPILE_AND_EXECUTE                          0x1301
#define GL_BYTE                                         0x1400
#define GL_UNSIGNED_BYTE                                0x1401
#define GL_SHORT                                        0x1402
#define GL_UNSIGNED_SHORT                               0x1403
#define GL_INT                                          0x1404
#define GL_UNSIGNED_INT                                 0x1405
#define GL_FLOAT                                        0x1406
#define GL_HALF_FLOAT                                   0x140B
#define GL_FIXED                                        0x140C
#define GL_INVERT                                       0x150A
#define GL_EMISSION                                     0x1600
#define GL_AMBIENT_AND_DIFFUSE                          0x1602
#define GL_MODELVIEW                                    0x1700
#define GL_PROJECTION                                   0x1701
#define GL_TEXTURE                                      0x1702
#define GL_COLOR_INDEX                                  0x1900
#define GL_DEPTH_COMPONENT                              0x1902
#define GL_RED                                          0x1903
#define GL_GREEN                                        0x1904
#define GL_BLUE                                         0x1905
#define GL_ALPHA                                        0x1906
#define GL_RGB                                          0x1907
#define GL_RGBA                                         0x1908
#define GL_LUMINANCE                                    0x1909
#define GL_LUMINANCE_ALPHA                              0x190A
#define GL_POINT                                        0x1B00
#define GL_LINE                                         0x1B01
#define GL_FILL                                         0x1B02
#define GL_FLAT                                         0x1D00
#define GL_SMOOTH                                       0x1D01
#define GL_KEEP                                         0x1E00
#define GL_REPLACE                                      0x1E01
#define GL_INCR                                         0x1E02
#define GL_DECR                                         0x1E03
#define GL_VENDOR                                       0x1F00
#define GL_RENDERER                                     0x1F01
#define GL_VERSION                                      0x1F02
#define GL_EXTENSIONS                                   0x1F03
#define GL_MODULATE                                     0x2100
#define GL_DECAL                                        0x2101
#define GL_TEXTURE_ENV_MODE                             0x2200
#define GL_TEXTURE_ENV_COLOR                            0x2201
#define GL_TEXTURE_ENV                                  0x2300
#define GL_NEAREST                                      0x2600
#define GL_LINEAR                                       0x2601
#define GL_NEAREST_MIPMAP_NEAREST                       0x2700
#define GL_LINEAR_MIPMAP_NEAREST                        0x2701
#define GL_NEAREST_MIPMAP_LINEAR                        0x2702
#define GL_LINEAR_MIPMAP_LINEAR                         0x2703
#define GL_TEXTURE_MAG_FILTER                           0x2800
#define GL_TEXTURE_MIN_FILTER                           0x2801
#define GL_TEXTURE_WRAP_S                               0x2802
#define GL_TEXTURE_WRAP_T                               0x2803
#define GL_CLAMP                                        0x2900
#define GL_REPEAT                                       0x2901
#define GL_POLYGON_OFFSET_UNITS                         0x2A00
#define GL_POLYGON_OFFSET_POINT                         0x2A01
#define GL_POLYGON_OFFSET_LINE                          0x2A02
#define GL_V2F                                          0x2A20
#define GL_V3F                                          0x2A21
#define GL_C4UB_V2F                                     0x2A22
#define GL_C4UB_V3F                                     0x2A23
#define GL_C3F_V3F                                      0x2A24
#define GL_T2F_V3F                                      0x2A27
#define GL_T4F_V4F                                      0x2A28
#define GL_T2F_C4UB_V3F                                 0x2A29
#define GL_T2F_C3F_V3F                                  0x2A2A
#define GL_CLIP_PLANE0                                  0x3000
#define GL_CLIP_PLANE1                                  0x3001
#define GL_CLIP_PLANE2                                  0x3002
#define GL_CLIP_PLANE3                                  0x3003
#define GL_CLIP_PLANE4                                  0x3004
#define GL_CLIP_PLANE5                                  0x3005
#define GL_CLIP_PLANE6                                  0x3006
#define GL_LIGHT0                                       0x4000
#define GL_LIGHT1                                       0x4001
#define GL_LIGHT2                                       0x4002
#define GL_LIGHT3                                       0x4003
#define GL_LIGHT4                                       0x4004
#define GL_LIGHT5                                       0x4005
#define GL_LIGHT6                                       0x4006
#define GL_LIGHT7                                       0x4007
#define GL_ABGR_EXT                                     0x8000
#define GL_FUNC_ADD                                     0x8006
#define GL_MIN                                          0x8007
#define GL_MAX                                          0x8008
#define GL_BLEND_EQUATION                               0x8009
#define GL_FUNC_SUBTRACT                                0x800A
#define GL_FUNC_REVERSE_SUBTRACT                        0x800B
#define GL_UNSIGNED_SHORT_4_4_4_4                       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1                       0x8034
#define GL_UNSIGNED_INT_8_8_8_8                         0x8035
#define GL_POLYGON_OFFSET_FILL                          0x8037
#define GL_POLYGON_OFFSET_FACTOR                        0x8038
#define GL_INTENSITY                                    0x8049
#define GL_RGB8                                         0x8051
#define GL_RGBA8                                        0x8058
#define GL_TEXTURE_BINDING_2D                           0x8069
#define GL_VERTEX_ARRAY                                 0x8074
#define GL_NORMAL_ARRAY                                 0x8075
#define GL_COLOR_ARRAY                                  0x8076
#define GL_TEXTURE_COORD_ARRAY                          0x8078
#define GL_VERTEX_ARRAY_SIZE                            0x807A
#define GL_VERTEX_ARRAY_TYPE                            0x807B
#define GL_VERTEX_ARRAY_STRIDE                          0x807C
#define GL_NORMAL_ARRAY_TYPE                            0x807E
#define GL_NORMAL_ARRAY_STRIDE                          0x807F
#define GL_COLOR_ARRAY_SIZE                             0x8081
#define GL_COLOR_ARRAY_TYPE                             0x8082
#define GL_COLOR_ARRAY_STRIDE                           0x8083
#define GL_TEXTURE_COORD_ARRAY_SIZE                     0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE                     0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE                   0x808A
#define GL_VERTEX_ARRAY_POINTER                         0x808E
#define GL_NORMAL_ARRAY_POINTER                         0x808F
#define GL_COLOR_ARRAY_POINTER                          0x8090
#define GL_TEXTURE_COORD_ARRAY_POINTER                  0x8092
#define GL_BLEND_DST_RGB                                0x80C8
#define GL_BLEND_SRC_RGB                                0x80C9
#define GL_BLEND_DST_ALPHA                              0x80CA
#define GL_BLEND_SRC_ALPHA                              0x80CB
#define GL_COLOR_TABLE                                  0x80D0
#define GL_BGR                                          0x80E0
#define GL_BGRA                                         0x80E1
#define GL_COLOR_INDEX8_EXT                             0x80E5
#define GL_MAX_ELEMENTS_VERTICES                        0x80E8
#define GL_MAX_ELEMENTS_INDICES                         0x80E9
#define GL_PHONG_WIN                                    0x80EA
#define GL_CLAMP_TO_EDGE                                0x812F
#define GL_DEPTH_COMPONENT16                            0x81A5
#define GL_DEPTH_COMPONENT24                            0x81A6
#define GL_DEPTH_COMPONENT32                            0x81A7
#define GL_DEPTH_STENCIL_ATTACHMENT                     0x821A
#define GL_MAJOR_VERSION                                0x821B
#define GL_MINOR_VERSION                                0x821C
#define GL_NUM_EXTENSIONS                               0x821D
#define GL_RG                                           0x8227
#define GL_UNSIGNED_SHORT_5_6_5                         0x8363
#define GL_UNSIGNED_SHORT_1_5_5_5_REV                   0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV                     0x8367
#define GL_MIRRORED_REPEAT                              0x8370
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                0x83F3
#define GL_TEXTURE0                                     0x84C0
#define GL_TEXTURE1                                     0x84C1
#define GL_TEXTURE2                                     0x84C2
#define GL_TEXTURE3                                     0x84C3
#define GL_TEXTURE4                                     0x84C4
#define GL_TEXTURE5                                     0x84C5
#define GL_TEXTURE6                                     0x84C6
#define GL_TEXTURE7                                     0x84C7
#define GL_TEXTURE8                                     0x84C8
#define GL_TEXTURE9                                     0x84C9
#define GL_TEXTURE10                                    0x84CA
#define GL_TEXTURE11                                    0x84CB
#define GL_TEXTURE12                                    0x84CC
#define GL_TEXTURE13                                    0x84CD
#define GL_TEXTURE14                                    0x84CE
#define GL_TEXTURE15                                    0x84CF
#define GL_ACTIVE_TEXTURE                               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE                        0x84E1
#define GL_MAX_TEXTURE_UNITS                            0x84E2
#define GL_SUBTRACT                                     0x84E7
#define GL_MAX_RENDERBUFFER_SIZE                        0x84E8
#define GL_TEXTURE_COMPRESSION_HINT                     0x84EF
#define GL_TEXTURE_MAX_ANISOTROPY_EXT                   0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT               0x84FF
#define GL_TEXTURE_LOD_BIAS                             0x8501
#define GL_INCR_WRAP                                    0x8507
#define GL_DECR_WRAP                                    0x8508
#define GL_TEXTURE_CUBE_MAP                             0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X                  0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X                  0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y                  0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y                  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z                  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z                  0x851A
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE                    0x851C
#define GL_COMBINE                                      0x8570
#define GL_COMBINE_RGB                                  0x8571
#define GL_COMBINE_ALPHA                                0x8572
#define GL_RGB_SCALE                                    0x8573
#define GL_ADD_SIGNED                                   0x8574
#define GL_INTERPOLATE                                  0x8575
#define GL_CONSTANT                                     0x8576
#define GL_PRIMARY_COLOR                                0x8577
#define GL_PREVIOUS                                     0x8578
#define GL_SRC0_RGB                                     0x8580
#define GL_SRC1_RGB                                     0x8581
#define GL_SRC2_RGB                                     0x8582
#define GL_SRC0_ALPHA                                   0x8588
#define GL_SRC1_ALPHA                                   0x8589
#define GL_SRC2_ALPHA                                   0x858A
#define GL_OPERAND0_RGB                                 0x8590
#define GL_OPERAND1_RGB                                 0x8591
#define GL_OPERAND2_RGB                                 0x8592
#define GL_OPERAND0_ALPHA                               0x8598
#define GL_OPERAND1_ALPHA                               0x8599
#define GL_OPERAND2_ALPHA                               0x859A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED                  0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE                     0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE                   0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE                     0x8625
#define GL_CURRENT_VERTEX_ATTRIB                        0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER                  0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB                   0x864B
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS               0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS                   0x86A3
#define GL_PROGRAM_BINARY_LENGTH                        0x8741
#define GL_MIRROR_CLAMP_EXT                             0x8742
#define GL_BUFFER_SIZE                                  0x8764
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD              0x87EE
#define GL_RGBA16F                                      0x881A
#define GL_BLEND_EQUATION_ALPHA                         0x883D
#define GL_MAX_VERTEX_ATTRIBS                           0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED               0x886A
#define GL_MAX_TEXTURE_COORDS                           0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS                      0x8872
#define GL_ARRAY_BUFFER                                 0x8892
#define GL_ELEMENT_ARRAY_BUFFER                         0x8893
#define GL_ARRAY_BUFFER_BINDING                         0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING                 0x8895
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING           0x889F
#define GL_READ_ONLY                                    0x88B8
#define GL_WRITE_ONLY                                   0x88B9
#define GL_READ_WRITE                                   0x88BA
#define GL_STREAM_DRAW                                  0x88E0
#define GL_STREAM_READ                                  0x88E1
#define GL_STREAM_COPY                                  0x88E2
#define GL_STATIC_DRAW                                  0x88E4
#define GL_STATIC_READ                                  0x88E5
#define GL_STATIC_COPY                                  0x88E6
#define GL_DYNAMIC_DRAW                                 0x88E8
#define GL_DYNAMIC_READ                                 0x88E9
#define GL_DYNAMIC_COPY                                 0x88EA
#define GL_DEPTH24_STENCIL8                             0x88F0
#define GL_SAMPLER_BINDING                              0x8919
#define GL_FRAGMENT_SHADER                              0x8B30
#define GL_VERTEX_SHADER                                0x8B31
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS               0x8B4C
#define GL_SHADER_TYPE                                  0x8B4F
#define GL_FLOAT_VEC2                                   0x8B50
#define GL_FLOAT_VEC3                                   0x8B51
#define GL_FLOAT_VEC4                                   0x8B52
#define GL_INT_VEC2                                     0x8B53
#define GL_INT_VEC3                                     0x8B54
#define GL_INT_VEC4                                     0x8B55
#define GL_FLOAT_MAT2                                   0x8B5A
#define GL_FLOAT_MAT3                                   0x8B5B
#define GL_FLOAT_MAT4                                   0x8B5C
#define GL_SAMPLER_2D                                   0x8B5E
#define GL_SAMPLER_CUBE                                 0x8B60
#define GL_DELETE_STATUS                                0x8B80
#define GL_COMPILE_STATUS                               0x8B81
#define GL_LINK_STATUS                                  0x8B82
#define GL_INFO_LOG_LENGTH                              0x8B84
#define GL_ATTACHED_SHADERS                             0x8B85
#define GL_ACTIVE_UNIFORMS                              0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH                    0x8B87
#define GL_SHADER_SOURCE_LENGTH                         0x8B88
#define GL_ACTIVE_ATTRIBUTES                            0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH                  0x8B8A
#define GL_SHADING_LANGUAGE_VERSION                     0x8B8C
#define GL_CURRENT_PROGRAM                              0x8B8D
#define GL_PALETTE4_RGB8_OES                            0x8B90
#define GL_PALETTE4_RGBA8_OES                           0x8B91
#define GL_PALETTE4_R5_G6_B5_OES                        0x8B92
#define GL_PALETTE4_RGBA4_OES                           0x8B93
#define GL_PALETTE4_RGB5_A1_OES                         0x8B94
#define GL_PALETTE8_RGB8_OES                            0x8B95
#define GL_PALETTE8_RGBA8_OES                           0x8B96
#define GL_PALETTE8_R5_G6_B5_OES                        0x8B97
#define GL_PALETTE8_RGBA4_OES                           0x8B98
#define GL_PALETTE8_RGB5_A1_OES                         0x8B99
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG              0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG              0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG             0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG             0x8C03
#define GL_SRGB                                         0x8C40
#define GL_SRGB8                                        0x8C41
#define GL_SRGB_ALPHA                                   0x8C42
#define GL_SRGB8_ALPHA8                                 0x8C43
#define GL_SLUMINANCE_ALPHA                             0x8C44
#define GL_SLUMINANCE8_ALPHA8                           0x8C45
#define GL_SLUMINANCE                                   0x8C46
#define GL_SLUMINANCE8                                  0x8C47
#define GL_COMPRESSED_SRGB                              0x8C48
#define GL_COMPRESSED_SRGB_ALPHA                        0x8C49
#define GL_COMPRESSED_SRGB_S3TC_DXT1                    0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1              0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3              0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5              0x8C4F
#define GL_ATC_RGB_AMD                                  0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD                  0x8C93
#define GL_FRAMEBUFFER_BINDING                          0x8CA6
#define GL_RENDERBUFFER_BINDING                         0x8CA7
#define GL_READ_FRAMEBUFFER                             0x8CA8
#define GL_DRAW_FRAMEBUFFER                             0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING                     0x8CAA
#define GL_COLOR_ATTACHMENT0                            0x8CE0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE           0x8CD0
#define GL_DEPTH_ATTACHMENT                             0x8D00
#define GL_STENCIL_ATTACHMENT                           0x8D20
#define GL_DEPTH_COMPONENT32F                           0x8DAB
#define GL_DEPTH32F_STENCIL8                            0x8DAC
#define GL_FRAMEBUFFER_COMPLETE                         0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT    0x8CD7
#define GL_FRAMEBUFFER                                  0x8D40
#define GL_RENDERBUFFER                                 0x8D41
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS             0x8B4D
#define GL_HALF_FLOAT_OES                               0x8D61
#define GL_ETC1_RGB8_OES                                0x8D64
#define GL_SHADER_BINARY_FORMATS                        0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS                    0x8DF9
#define GL_SHADER_COMPILER                              0x8DFA
#define GL_MAX_VERTEX_UNIFORM_VECTORS                   0x8DFB
#define GL_MAX_VARYING_VECTORS                          0x8DFC
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS                 0x8DFD
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX         0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX   0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG             0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG             0x9138
#define GL_COMPRESSED_RGBA8_ETC2_EAC                    0x9278

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
#define GL_BLEND_EQUATION_RGB GL_BLEND_EQUATION

// clang-format on

// gl*
void glActiveTexture(GLenum texture);
void glAlphaFunc(GLenum func, GLfloat ref);
void glAlphaFuncx(GLenum func, GLfixed ref);
void glAttachShader(GLuint prog, GLuint shad);
void glBegin(GLenum mode);
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name);
void glBindBuffer(GLenum target, GLuint buffer);
void glBindFramebuffer(GLenum target, GLuint framebuffer);
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
void glBindSampler(GLuint unit, GLuint smp);
void glBindTexture(GLenum target, GLuint texture);
void glBindVertexArray(GLuint array);
void glBlendEquation(GLenum mode);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
void glCallList(GLuint list);
GLenum glCheckFramebufferStatus(GLenum target);
GLenum glCheckNamedFramebufferStatus(GLuint target, GLenum dummy);
void glClear(GLbitfield mask);
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha);
void glClearDepth(GLdouble depth);
void glClearDepthf(GLclampf depth);
void glClearDepthx(GLclampx depth);
void glClearStencil(GLint s);
void glClientActiveTexture(GLenum texture);
void glClipPlane(GLenum plane, const GLdouble *equation);
void glClipPlanef(GLenum plane, const GLfloat *equation);
void glClipPlanex(GLenum plane, const GLfixed *equation);
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
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GLuint glCreateProgram(void);
GLuint glCreateShader(GLenum shaderType);
void glCullFace(GLenum mode);
void glDeleteBuffers(GLsizei n, const GLuint *gl_buffers);
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
void glDeleteLists(GLuint list, GLsizei range);
void glDeleteProgram(GLuint prog);
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
void glDeleteSamplers(GLsizei n, const GLuint *smp);
void glDeleteShader(GLuint shad);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glDeleteVertexArrays(GLsizei n, const GLuint *gl_arrays);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glDepthRange(GLdouble nearVal, GLdouble farVal);
void glDepthRangef(GLfloat nearVal, GLfloat farVal);
void glDepthRangex(GLfixed nearVal, GLfixed farVal);
void glDisable(GLenum cap);
void glDisableClientState(GLenum array);
void glDisableVertexAttribArray(GLuint index);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid *gl_indices, GLint baseVertex);
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *gl_indices, GLsizei primcount);
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *indices, GLint basevertex);
void glEnable(GLenum cap);
void glEnableClientState(GLenum array);
void glEnableVertexAttribArray(GLuint index);
void glEnd(void);
void glEndList(void);
void glFinish(void);
void glFlush(void);
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
void glFogf(GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi(GLenum pname, const GLint param);
void glFogx(GLenum pname, GLfixed param);
void glFogxv(GLenum pname, const GLfixed *params);
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glFrontFace(GLenum mode);
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal);
void glFrustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glGenerateMipmap(GLenum target);
void glGenerateTextureMipmap(GLuint target);
void glGenFramebuffers(GLsizei n, GLuint *framebuffers);
GLuint glGenLists(GLsizei range);
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
void glGenSamplers(GLsizei n, GLuint *samplers);
void glGenTextures(GLsizei n, GLuint *textures);
void glGenVertexArrays(GLsizei n, GLuint *res);
void glGetActiveAttrib(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void glGetActiveUniform(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
void glGetAttachedShaders(GLuint prog, GLsizei maxCount, GLsizei *count, GLuint *shads);
GLint glGetAttribLocation(GLuint prog, const GLchar *name);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params);
void glGetDoublev(GLenum pname, GLdouble *data);
GLenum glGetError(void);
void glGetFloatv(GLenum pname, GLfloat *data);
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params);
void glGetIntegerv(GLenum pname, GLint *data);
void glGetPointerv(GLenum pname, void **params);
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
void glGetShaderInfoLog(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void glGetShaderiv(GLuint handle, GLenum pname, GLint *params);
void glGetShaderSource(GLuint handle, GLsizei bufSize, GLsizei *length, GLchar *source);
const GLubyte *glGetString(GLenum name);
const GLubyte *glGetStringi(GLenum name, GLuint index);
void glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
GLint glGetUniformLocation(GLuint prog, const GLchar *name);
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params);
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params);
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer);
void glHint(GLenum target, GLenum mode);
void glInterleavedArrays(GLenum format, GLsizei stride, const void *pointer);
GLboolean glIsEnabled(GLenum cap);
GLboolean glIsFramebuffer(GLuint fb);
GLboolean glIsProgram(GLuint program);
GLboolean glIsRenderbuffer(GLuint rb);
GLboolean glIsTexture(GLuint texture);
void glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void glLightModelfv(GLenum pname, const GLfloat *params);
void glLightModelxv(GLenum pname, const GLfixed *params);
void glLightxv(GLenum light, GLenum pname, const GLfixed *params);
void glLineWidth(GLfloat width);
void glLineWidthx(GLfixed width);
void glLinkProgram(GLuint progr);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void glLoadMatrixx(const GLfixed *m);
void glLoadTransposeMatrixf(const GLfloat *m);
void glLoadTransposeMatrixx(const GLfixed *m);
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
void glMultTransposeMatrixf(const GLfloat *m);
void glMultTransposeMatrixx(const GLfixed *m);
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
void glNamedFramebufferTexture(GLuint target, GLenum attachment, GLuint texture, GLint level);
void glNamedFramebufferTexture2D(GLuint target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
void glNamedRenderbufferStorage(GLuint target, GLenum internalformat, GLsizei width, GLsizei height);
void glNewList(GLuint list, GLenum mode);
void glNormal3f(GLfloat x, GLfloat y, GLfloat z);
void glNormal3fv(const GLfloat *v);
void glNormal3s(GLshort x, GLshort y, GLshort z);
void glNormal3x(GLfixed x, GLfixed y, GLfixed z);
void glNormalPointer(GLenum type, GLsizei stride, const void *pointer);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal);
void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal);
void glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal);
void glPixelStorei(GLenum pname, GLint param);
void glPointSize(GLfloat size);
void glPointSizex(GLfixed size);
void glPolygonMode(GLenum face, GLenum mode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glPolygonOffsetx(GLfixed factor, GLfixed units);
void glPopAttrib(void);
void glPopMatrix(void);
void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0);
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
void glProgramUniform1i(GLuint program, GLint location, GLint v0);
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value);
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1);
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1);
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value);
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value);
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value);
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value);
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
void glPushAttrib(GLbitfield mask);
void glPushMatrix(void);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data);
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
void glReleaseShaderCompiler(void);
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
void glSamplerParameteri(GLuint target, GLenum pname, GLint param);
void glScaled(GLdouble x, GLdouble y, GLdouble z);
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
void glTexCoord2s(GLshort s, GLshort t);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvfv(GLenum target, GLenum pname, GLfloat *param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexEnvx(GLenum target, GLenum pname, GLfixed param);
void glTexEnvxv(GLenum target, GLenum pname, GLfixed *param);
void glTexImage1D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *data);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexParameteriv(GLenum target, GLenum pname, GLint *param);
void glTexParameterx(GLenum target, GLenum pname, GLfixed param);
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void glTranslated(GLdouble x, GLdouble y, GLdouble z);
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
void glVertex2i(GLint x, GLint y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glVertex3i(GLint x, GLint y, GLint z);
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
void vglEnd(void);
void *vglForceAlloc(uint32_t size);
void vglFree(void *addr);
SceGxmTexture *vglGetGxmTexture(GLenum target);
void *vglGetProcAddress(const char *name);
void *vglGetTexDataPointer(GLenum target);
GLboolean vglInit(int legacy_pool_size);
GLboolean vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa);
GLboolean vglInitWithCustomSizes(int legacy_pool_size, int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, int cdlg_pool_size, SceGxmMultisampleMode msaa);
GLboolean vglInitWithCustomThreshold(int pool_size, int width, int height, int ram_threshold, int cdram_threshold, int phycont_threshold, int cdlg_threshold, SceGxmMultisampleMode msaa);
void *vglMalloc(uint32_t size);
size_t vglMallocUsableSize(void *ptr);
void *vglMemalign(uint32_t alignment, uint32_t size);
size_t vglMemFree(vglMemType type);
size_t vglMemTotal(vglMemType type);
void vglOverloadTexDataPointer(GLenum target, void *data);
void *vglRealloc(void *ptr, uint32_t size);
void vglSetDisplayCallback(void (*cb)(void *framebuf));
void vglSetFragmentBufferSize(uint32_t size);
void vglSetParamBufferSize(uint32_t size);
void vglSetUSSEBufferSize(uint32_t size);
void vglSetVDMBufferSize(uint32_t size);
void vglSetVertexBufferSize(uint32_t size);
void vglSetVertexPoolSize(uint32_t size);
void vglSetupGarbageCollector(int priority, int affinity);
void vglSetupRuntimeShaderCompiler(shark_opt opt_level, int32_t use_fastmath, int32_t use_fastprecision, int32_t use_fastint);
void vglSwapBuffers(GLboolean has_commondialog);
void vglTexImageDepthBuffer(GLenum target);
void vglUseCachedMem(GLboolean use);
void vglUseTripleBuffering(GLboolean usage);
void vglUseVram(GLboolean usage);
void vglUseVramForUSSE(GLboolean usage);
void vglUseExtraMem(GLboolean usage);
void vglWaitVblankStart(GLboolean enable);

#ifdef __cplusplus
}
#endif

#endif
