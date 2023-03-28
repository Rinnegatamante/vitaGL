const char *glsl_hdr =
R"(#define GL_ES 1
inline float4x4 vglMul(float4x4 M, float v) { return v * M; }
inline float3x3 vglMul(float3x3 M, float v) { return v * M; }
inline float2x2 vglMul(float2x2 M, float v) { return v * M; }
inline float4 vglMul(float4x4 M, float4 v) { return mul(v, M); }
inline float3 vglMul(float3x3 M, float3 v) { return mul(v, M); }
inline float2 vglMul(float2x2 M, float2 v) { return mul(v, M); }
inline float4 vglMul(float4 M, float4x4 v) { return mul(v, M); }
inline float3 vglMul(float3 M, float3x3 v) { return mul(v, M); }
inline float2 vglMul(float2 M, float2x2 v) { return mul(v, M); }
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
#define greaterThanEqual(x, y) (x >= y)
#define greaterThan(x, y) (x > y)
#define lessThanEqual(x, y) (x <= y)
#define lessThan(x, y) (x < y)
#define equal(x, y) (x == y)
#define notEqual(x, y) (x != y)
#define texture2DProj(x,y) tex2Dproj(x,y)
#define inversesqrt rsqrt
#define samplerCube samplerCUBE
inline float4 glslTexture2D(sampler2D x, float2 s) { return tex2D(x,s); }
inline float4 glslTexture2D(sampler2D x, float3 s) { return tex2D(x,s); }
inline float4 glslTexture2D(sampler2D x, float2 s, float b) { return tex2Dbias(x,float4(s,1,b)); }
inline float4 textureCube(samplerCUBE x, float3 s) { return texCUBE(x,s); }
inline float4 textureCube(samplerCUBE x, float4 s) { return texCUBE(x,s); }
inline float4 textureCube(samplerCUBE x, float3 s, float b) { return texCUBEbias(x,float4(s,b)); }
#define texture2D glslTexture2D
#define lowp
#define mediump
#define highp
#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mix(a,b,c) lerp(a,b,c)
#define atan(x,y) atan2(x,y)
#define ivec2 int2
#define ivec3 int3
#define ivec4 int4
#define fract(x) frac(x)
#define mod(x,y) fmod(x,y)
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#define vgl varying
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

const char *glsl_precision_hdr =
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
