#ifndef _GLSL_TRANSLATOR_HDR_H_
#define _GLSL_TRANSLATOR_HDR_H_

static const char *glsl_hdr =
R"(#define GL_ES 1
#define VITAGL
inline float4x4 vglMul(float4x4 M1, float4x4 M2) { return M1 * M2; }
inline float3x3 vglMul(float3x3 M1, float3x3 M2) { return M1 * M2; }
inline float2x2 vglMul(float2x2 M1, float2x2 M2) { return M1 * M2; }
inline float4x4 vglMul(float4x4 M, float v) { return v * M; }
inline float3x3 vglMul(float3x3 M, float v) { return v * M; }
inline float2x2 vglMul(float2x2 M, float v) { return v * M; }
inline float4x4 vglMul(float v, float4x4 M) { return M * v; }
inline float3x3 vglMul(float v, float3x3 M) { return M * v; }
inline float2x2 vglMul(float v, float2x2 M) { return M * v; }
inline float4 vglMul(float4x4 M, float4 v) { return mul(v, M); }
inline float3 vglMul(float3x3 M, float3 v) { return mul(v, M); }
inline float2 vglMul(float2x2 M, float2 v) { return mul(v, M); }
inline float4 vglMul(float4 v, float4x4 M) { return mul(M, v); }
inline float3 vglMul(float3 v, float3x3 M) { return mul(M, v); }
inline float2 vglMul(float2 v, float2x2 M) { return mul(M, v); }
inline float4 vglMul(float v1, float4 v2) { return v1 * v2; }
inline float3 vglMul(float v1, float3 v2) { return v1 * v2; }
inline float2 vglMul(float v1, float2 v2) { return v1 * v2; }
inline float vglMul(float v1, float v2) { return v1 * v2; }
inline float4 vglMul(float4 v1, float4 v2) { return v1 * v2; }
inline float3 vglMul(float3 v1, float3 v2) { return v1 * v2; }
inline float2 vglMul(float2 v1, float2 v2) { return v1 * v2; }
inline float4 vglMul(float4 v1, float v2) { return v1 * v2; }
inline float3 vglMul(float3 v1, float v2) { return v1 * v2; }
inline float2 vglMul(float2 v1, float v2) { return v1 * v2; }
inline int4 vglMul(int4 v1, int4 v2) { return v1 * v2; }
inline int3 vglMul(int3 v1, int3 v2) { return v1 * v2; }
inline int2 vglMul(int2 v1, int2 v2) { return v1 * v2; }
inline int4 vglMul(int v1, int4 v2) { return v1 * v2; }
inline int3 vglMul(int v1, int3 v2) { return v1 * v2; }
inline int2 vglMul(int v1, int2 v2) { return v1 * v2; }
inline int vglMul(int v1, int v2) { return v1 * v2; }
inline int4 vglMul(int4 v1, int v2) { return v1 * v2; }
inline int3 vglMul(int3 v1, int v2) { return v1 * v2; }
inline int2 vglMul(int2 v1, int v2) { return v1 * v2; }
inline int greaterThanEqual(int a, int b) { return (a >= b ? 1 : 0); }
inline int2 greaterThanEqual(int2 a, int2 b) { return int2(a.x >= b.x ? 1 : 0, a.y >= b.y ? 1 : 0); }
inline int3 greaterThanEqual(int3 a, int3 b) { return int3(a.x >= b.x ? 1 : 0, a.y >= b.y ? 1 : 0, a.z >= b.z ? 1 : 0); }
inline int4 greaterThanEqual(int4 a, int4 b) { return int4(a.x >= b.x ? 1 : 0, a.y >= b.y ? 1 : 0, a.z >= b.z ? 1 : 0, a.w >= b.w ? 1 : 0); }
inline int greaterThanEqual(float a, float b) { return (a >= b ? 1 : 0); }
inline int2 greaterThanEqual(float2 a, float2 b) { return int2(a.x >= b.x ? 1 : 0, a.y >= b.y ? 1 : 0); }
inline int3 greaterThanEqual(float3 a, float3 b) { return int3(a.x >= b.x ? 1 : 0, a.y >= b.y ? 1 : 0, a.z >= b.z ? 1 : 0); }
inline int4 greaterThanEqual(float4 a, float4 b) { return int4(a.x >= b.x ? 1 : 0, a.y >= b.y ? 1 : 0, a.z >= b.z ? 1 : 0, a.w >= b.w ? 1 : 0); }
inline int greaterThan(int a, int b) { return (a >= b ? 1 : 0); }
inline int2 greaterThan(int2 a, int2 b) { return int2(a.x > b.x ? 1 : 0, a.y > b.y ? 1 : 0); }
inline int3 greaterThan(int3 a, int3 b) { return int3(a.x > b.x ? 1 : 0, a.y > b.y ? 1 : 0, a.z > b.z ? 1 : 0); }
inline int4 greaterThan(int4 a, int4 b) { return int4(a.x > b.x ? 1 : 0, a.y > b.y ? 1 : 0, a.z > b.z ? 1 : 0, a.w > b.w ? 1 : 0); }
inline int greaterThan(float a, float b) { return (a > b ? 1 : 0); }
inline int2 greaterThan(float2 a, float2 b) { return int2(a.x > b.x ? 1 : 0, a.y > b.y ? 1 : 0); }
inline int3 greaterThan(float3 a, float3 b) { return int3(a.x > b.x ? 1 : 0, a.y > b.y ? 1 : 0, a.z > b.z ? 1 : 0); }
inline int4 greaterThan(float4 a, float4 b) { return int4(a.x > b.x ? 1 : 0, a.y > b.y ? 1 : 0, a.z > b.z ? 1 : 0, a.w > b.w ? 1 : 0); }
#define lessThan(x, y) greaterThanEqual(y, x)
#define lessThanEqual(x, y) greaterThan(y, x)
inline int equal(int a, int b) { return (a == b ? 1 : 0); }
inline int2 equal(int2 a, int2 b) { return int2(a.x == b.x ? 1 : 0, a.y == b.y ? 1 : 0); }
inline int3 equal(int3 a, int3 b) { return int3(a.x == b.x ? 1 : 0, a.y == b.y ? 1 : 0, a.z == b.z ? 1 : 0); }
inline int4 equal(int4 a, int4 b) { return int4(a.x == b.x ? 1 : 0, a.y == b.y ? 1 : 0, a.z == b.z ? 1 : 0, a.w == b.w ? 1 : 0); }
inline int equal(float a, float b) { return (a == b ? 1 : 0); }
inline int2 equal(float2 a, float2 b) { return int2(a.x == b.x ? 1 : 0, a.y == b.y ? 1 : 0); }
inline int3 equal(float3 a, float3 b) { return int3(a.x == b.x ? 1 : 0, a.y == b.y ? 1 : 0, a.z == b.z ? 1 : 0); }
inline int4 equal(float4 a, float4 b) { return int4(a.x == b.x ? 1 : 0, a.y == b.y ? 1 : 0, a.z == b.z ? 1 : 0, a.w == b.w ? 1 : 0); }
inline int notEqual(int a, int b) { return (a != b ? 1 : 0); }
inline int2 notEqual(int2 a, int2 b) { return int2(a.x != b.x ? 1 : 0, a.y != b.y ? 1 : 0); }
inline int3 notEqual(int3 a, int3 b) { return int3(a.x != b.x ? 1 : 0, a.y != b.y ? 1 : 0, a.z != b.z ? 1 : 0); }
inline int4 notEqual(int4 a, int4 b) { return int4(a.x != b.x ? 1 : 0, a.y != b.y ? 1 : 0, a.z != b.z ? 1 : 0, a.w != b.w ? 1 : 0); }
inline int notEqual(float a, float b) { return (a != b ? 1 : 0); }
inline int2 notEqual(float2 a, float2 b) { return int2(a.x != b.x ? 1 : 0, a.y != b.y ? 1 : 0); }
inline int3 notEqual(float3 a, float3 b) { return int3(a.x != b.x ? 1 : 0, a.y != b.y ? 1 : 0, a.z != b.z ? 1 : 0); }
inline int4 notEqual(float4 a, float4 b) { return int4(a.x != b.x ? 1 : 0, a.y != b.y ? 1 : 0, a.z != b.z ? 1 : 0, a.w != b.w ? 1 : 0); }
inline float4 texture2DProj(sampler2D s, float3 c) { return tex2Dproj(s, c); }
inline float4 texture2DProj(sampler2D s, float4 c) { return tex2Dproj(s, c.xyw); }
inline float4 texture2DProj(sampler2D s, float4 c, float b) { return tex2Dbias(s, float4(c.xy / c.w, 1, b)); }
inline float4 texture2DProj(sampler2D s, float3 c, float b) { return tex2Dbias(s, float4(c.xy / c.z, 1, b)); }
#define inversesqrt rsqrt
#define samplerCube samplerCUBE
inline float4 glslTexture2D(sampler2D x, float2 s) { return tex2D(x,s); }
inline float4 glslTexture2D(sampler2D x, float3 s) { return tex2D(x,s); }
inline float4 glslTexture2D(sampler2D x, float2 s, float b) { return tex2Dbias(x,float4(s,1,b)); }
inline float4 textureCube(samplerCUBE x, float3 s) { return texCUBE(x,s); }
inline float4 textureCube(samplerCUBE x, float4 s) { return texCUBE(x,s); }
inline float4 textureCube(samplerCUBE x, float3 s, float b) { return texCUBEbias(x,float4(s,b)); }
inline float4 texture2DLod(sampler2D x, float2 coord, float lod) { return tex2Dlod(x, float4(coord, 0.0f, lod)); }
#define s x
#define t y
#define st xy
#define ts yx
#define ss xx
#define tt yy
#define texture2D glslTexture2D
#define lowp
#define mediump
#define highp
#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mix(a,b,c) lerp(a,b,c)
inline float vgl_atan(float x, float y) { return atan2(x, y); }
inline float2 vgl_atan(float2 x, float2 y) { return atan2(x, y); }
inline float3 vgl_atan(float3 x, float3 y) { return atan2(x, y); }
inline float4 vgl_atan(float4 x, float4 y) { return atan2(x, y); }
inline float vgl_atan(float x) { return atan(x); }
inline float2 vgl_atan(float2 x) { return atan(x); }
inline float3 vgl_atan(float3 x) { return atan(x); }
inline float4 vgl_atan(float4 x) { return atan(x); }
#define atan vgl_atan
#define ivec2 int2
#define ivec3 int3
#define ivec4 int4
#define bvec2 bool2
#define bvec3 bool3
#define bvec4 bool4
#define fract(x) frac(x)
#define mod(x,y) fmod(x,y)
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#define matrix _matrix
#define sampler _sampler
#define vgl varying
#define POUT(x, y) \
	varying out x : CLP##y
#define FOUT(x, y) \
	varying out x : FOGC
#define COUT(x, y) \
	varying out x : COLOR##y
#define VOUT(x, y) \
	varying out x : TEXCOORD##y
#define FIN(x, y) \
	varying in x : FOGC
#define CIN(x, y) \
	varying in x : COLOR##y
#define VIN(x, y) \
	varying in x : TEXCOORD##y
#ifdef VGL_IS_VERTEX_SHADER
varying out float4 gl_Position : POSITION;
#else
varying out float4 gl_FragColor : COLOR;
#endif
)";

static const char *glsl_precision_hdr =
R"(#define float half
#define float2 half2
#define float3 half3
#define float4 half4
#define float2x2 half2x2
#define float3x3 half3x3
#define float4x4 half4x4
#define int short
#define int2 short2
#define int3 short3
#define int4 short4
)";

static const char *glsl_ffp_hdr =
R"(#ifdef VGL_HAS_MVP
uniform float4x4 gl_ModelViewProjectionMatrix;
#endif
#ifdef VGL_HAS_MV
uniform float4x4 gl_ModelViewMatrix;
#endif
#ifdef VGL_HAS_NM
uniform float3x3 gl_NormalMatrix;
#endif
#ifdef VGL_HAS_FOG
struct vgl_Fog {
	float density;
	float4 color;
};
uniform vgl_Fog gl_Fog;
#endif
)";

#endif
