#ifndef _GLSL_TRANSLATOR_HDR_H_
#define _GLSL_TRANSLATOR_HDR_H_

static const char *glsl_hdr =
R"(#define GL_ES 1
#define VITAGL
inline float4x4 vglMul(float4x4 M1, float4x4 M2) { return mul(M2, M1); }
inline float3x3 vglMul(float3x3 M1, float3x3 M2) { return mul(M2, M1); }
inline float2x2 vglMul(float2x2 M1, float2x2 M2) { return mul(M2, M1); }
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
inline float vgl_round_even(float x) {
	float t = trunc(x);
	float r = abs(x - t);
	if (r < 0.5) return t;
	if (r > 0.5) return t + sign(x);
	return fmod(abs(t), 2.0) < 0.5 ? t : (t + sign(x));
}
inline float2 vgl_round_even(float2 x) { return float2(vgl_round_even(x.x), vgl_round_even(x.y)); }
inline float3 vgl_round_even(float3 x) { return float3(vgl_round_even(x.x), vgl_round_even(x.y), vgl_round_even(x.z)); }
inline float4 vgl_round_even(float4 x) { return float4(vgl_round_even(x.x), vgl_round_even(x.y), vgl_round_even(x.z), vgl_round_even(x.w)); }
#define roundEven vgl_round_even
inline int vgl_bitfield_extract(int value, int offset, int bits) {
	if (bits == 0) return 0;
	unsigned int u = (unsigned int)value;
	unsigned int mask = (bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
	unsigned int r = (u >> offset) & mask;
	if (bits < 32 && (r & (1u << (bits - 1)))) r |= ~mask;
	return (int)r;
}
inline int2 vgl_bitfield_extract(int2 value, int offset, int bits) {
	return int2(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits));
}
inline int3 vgl_bitfield_extract(int3 value, int offset, int bits) {
	return int3(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits), vgl_bitfield_extract(value.z, offset, bits));
}
inline int4 vgl_bitfield_extract(int4 value, int offset, int bits) {
	return int4(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits), vgl_bitfield_extract(value.z, offset, bits), vgl_bitfield_extract(value.w, offset, bits));
}
inline unsigned int vgl_bitfield_extract(unsigned int value, int offset, int bits) {
	if (bits == 0) return 0u;
	unsigned int mask = (bits >= 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
	return (value >> offset) & mask;
}
inline uint2 vgl_bitfield_extract(uint2 value, int offset, int bits) {
	return uint2(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits));
}
inline uint3 vgl_bitfield_extract(uint3 value, int offset, int bits) {
	return uint3(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits), vgl_bitfield_extract(value.z, offset, bits));
}
inline uint4 vgl_bitfield_extract(uint4 value, int offset, int bits) {
	return uint4(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits), vgl_bitfield_extract(value.z, offset, bits), vgl_bitfield_extract(value.w, offset, bits));
}
inline float vgl_bitfield_extract(float value, int offset, int bits) {
	return intBitsToFloat(vgl_bitfield_extract(floatToIntBits(value), offset, bits));
}
inline float2 vgl_bitfield_extract(float2 value, int offset, int bits) {
	return float2(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits));
}
inline float3 vgl_bitfield_extract(float3 value, int offset, int bits) {
	return float3(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits), vgl_bitfield_extract(value.z, offset, bits));
}
inline float4 vgl_bitfield_extract(float4 value, int offset, int bits) {
	return float4(vgl_bitfield_extract(value.x, offset, bits), vgl_bitfield_extract(value.y, offset, bits), vgl_bitfield_extract(value.z, offset, bits), vgl_bitfield_extract(value.w, offset, bits));
}
#define bitfieldExtract vgl_bitfield_extract
inline float2 vgl_texel_offset2(float2 p, int2 off, sampler2D s) {
	int2 sz = tex2Dsize(s, 0);
	return p + float2(off) / float2(sz);
}
inline float2 vgl_texel_offset2(float2 p, int2 off, isampler2D s) {
	int2 sz = tex2Dsize(s, 0);
	return p + float2(off) / float2(sz);
}
inline float2 vgl_texel_offset2(float2 p, int2 off, usampler2D s) {
	int2 sz = tex2Dsize(s, 0);
	return p + float2(off) / float2(sz);
}
inline float2 vgl_texel_offset2(float2 p, int2 off, sampler2DShadow s) {
	int2 sz = tex2Dsize(s, 0);
	return p + float2(off) / float2(sz);
}
inline float3 vgl_texel_offset3(float3 p, int3 off, sampler3D s) {
	int3 sz = tex3Dsize(s, 0);
	return p + float3(off) / float3(sz);
}
inline float3 vgl_texel_offset3(float3 p, int3 off, isampler3D s) {
	int3 sz = tex3Dsize(s, 0);
	return p + float3(off) / float3(sz);
}
inline float3 vgl_texel_offset3(float3 p, int3 off, usampler3D s) {
	int3 sz = tex3Dsize(s, 0);
	return p + float3(off) / float3(sz);
}
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
inline float4 vgl_texture(sampler2D x, float2 s) { return tex2D(x, s); }
inline float4 vgl_texture(sampler2D x, float3 s) { return tex2D(x, s); }
inline float4 vgl_texture(sampler2D x, float2 s, float b) { return tex2Dbias(x, float4(s, 1, b)); }
inline int4 vgl_texture(isampler2D x, float2 s) { return tex2D(x, s); }
inline int4 vgl_texture(isampler2D x, float2 s, float b) { return tex2Dbias(x, float4(s, 1, b)); }
inline uint4 vgl_texture(usampler2D x, float2 s) { return tex2D(x, s); }
inline uint4 vgl_texture(usampler2D x, float2 s, float b) { return tex2Dbias(x, float4(s, 1, b)); }
inline float4 vgl_texture(sampler3D x, float3 s) { return tex3D(x, s); }
inline float4 vgl_texture(sampler3D x, float3 s, float b) { return tex3Dbias(x, float4(s, b)); }
inline int4 vgl_texture(isampler3D x, float3 s) { return tex3D(x, s); }
inline int4 vgl_texture(isampler3D x, float3 s, float b) { return tex3Dbias(x, float4(s, b)); }
inline uint4 vgl_texture(usampler3D x, float3 s) { return tex3D(x, s); }
inline uint4 vgl_texture(usampler3D x, float3 s, float b) { return tex3Dbias(x, float4(s, b)); }
inline float4 vgl_texture(samplerCUBE x, float3 s) { return texCUBE(x, s); }
inline float4 vgl_texture(samplerCUBE x, float3 s, float b) { return texCUBEbias(x, float4(s, b)); }
inline int4 vgl_texture(isamplerCUBE x, float3 s) { return texCUBE(x, s); }
inline int4 vgl_texture(isamplerCUBE x, float3 s, float b) { return texCUBEbias(x, float4(s, b)); }
inline uint4 vgl_texture(usamplerCUBE x, float3 s) { return texCUBE(x, s); }
inline uint4 vgl_texture(usamplerCUBE x, float3 s, float b) { return texCUBEbias(x, float4(s, b)); }
inline float vgl_texture(sampler2DShadow x, float3 s) { return tex2D(x, s); }
inline float vgl_texture(sampler2DShadow x, float3 s, float b) { return tex2Dcmpbias(x, float4(s.xy, s.z, b)); }
inline float vgl_texture(samplerCUBEShadow x, float4 s) { return texCUBE(x, s); }
inline float vgl_texture(samplerCUBEShadow x, float4 s, float b) { return texCUBE(x, s); }
#define texture vgl_texture
inline float4 vgl_textureLod(sampler2D x, float2 s, float lod) { return tex2Dlod(x, float4(s, 0, lod)); }
inline int4 vgl_textureLod(isampler2D x, float2 s, float lod) { return tex2Dlod(x, float4(s, 0, lod)); }
inline uint4 vgl_textureLod(usampler2D x, float2 s, float lod) { return tex2Dlod(x, float4(s, 0, lod)); }
inline float4 vgl_textureLod(sampler3D x, float3 s, float lod) { return tex3Dlod(x, float4(s, lod)); }
inline int4 vgl_textureLod(isampler3D x, float3 s, float lod) { return tex3Dlod(x, float4(s, lod)); }
inline uint4 vgl_textureLod(usampler3D x, float3 s, float lod) { return tex3Dlod(x, float4(s, lod)); }
inline float4 vgl_textureLod(samplerCUBE x, float3 s, float lod) { return texCUBElod(x, float4(s, lod)); }
inline int4 vgl_textureLod(isamplerCUBE x, float3 s, float lod) { return texCUBElod(x, float4(s, lod)); }
inline uint4 vgl_textureLod(usamplerCUBE x, float3 s, float lod) { return texCUBElod(x, float4(s, lod)); }
inline float vgl_textureLod(sampler2DShadow x, float3 s, float lod) { return tex2Dlod(x, float4(s.xy, s.z, lod)); }
#define textureLod vgl_textureLod
inline float4 vgl_textureProj(sampler2D x, float3 s) { return tex2Dproj(x, s); }
inline float4 vgl_textureProj(sampler2D x, float4 s) { return tex2Dproj(x, s.xyw); }
inline float4 vgl_textureProj(sampler2D x, float3 s, float b) { return tex2Dbias(x, float4(s.xy / s.z, 1, b)); }
inline float4 vgl_textureProj(sampler2D x, float4 s, float b) { return tex2Dbias(x, float4(s.xy / s.w, 1, b)); }
inline int4 vgl_textureProj(isampler2D x, float3 s) { return tex2Dproj(x, s); }
inline int4 vgl_textureProj(isampler2D x, float4 s) { return tex2Dproj(x, s.xyw); }
inline int4 vgl_textureProj(isampler2D x, float3 s, float b) { return tex2Dbias(x, float4(s.xy / s.z, 1, b)); }
inline int4 vgl_textureProj(isampler2D x, float4 s, float b) { return tex2Dbias(x, float4(s.xy / s.w, 1, b)); }
inline uint4 vgl_textureProj(usampler2D x, float3 s) { return tex2Dproj(x, s); }
inline uint4 vgl_textureProj(usampler2D x, float4 s) { return tex2Dproj(x, s.xyw); }
inline uint4 vgl_textureProj(usampler2D x, float3 s, float b) { return tex2Dbias(x, float4(s.xy / s.z, 1, b)); }
inline uint4 vgl_textureProj(usampler2D x, float4 s, float b) { return tex2Dbias(x, float4(s.xy / s.w, 1, b)); }
inline float4 vgl_textureProj(sampler3D x, float4 s) { return tex3Dproj(x, s); }
inline float4 vgl_textureProj(sampler3D x, float4 s, float b) { return tex3Dbias(x, float4(s.xyz / s.w, b)); }
inline int4 vgl_textureProj(isampler3D x, float4 s) { return tex3Dproj(x, s); }
inline int4 vgl_textureProj(isampler3D x, float4 s, float b) { return tex3Dbias(x, float4(s.xyz / s.w, b)); }
inline uint4 vgl_textureProj(usampler3D x, float4 s) { return tex3Dproj(x, s); }
inline uint4 vgl_textureProj(usampler3D x, float4 s, float b) { return tex3Dbias(x, float4(s.xyz / s.w, b)); }
inline float vgl_textureProj(sampler2DShadow x, float4 s) { return tex2Dproj(x, s); }
inline float vgl_textureProj(sampler2DShadow x, float4 s, float b) { return tex2Dcmpbias(x, float4(s.xy / s.w, s.z, b)); }
#define textureProj vgl_textureProj
inline float4 vgl_textureOffset(sampler2D x, float2 s, int2 off) { return tex2D(x, vgl_texel_offset2(s, off, x)); }
inline float4 vgl_textureOffset(sampler2D x, float2 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s, off, x), 1, b)); }
inline int4 vgl_textureOffset(isampler2D x, float2 s, int2 off) { return tex2D(x, vgl_texel_offset2(s, off, x)); }
inline int4 vgl_textureOffset(isampler2D x, float2 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s, off, x), 1, b)); }
inline uint4 vgl_textureOffset(usampler2D x, float2 s, int2 off) { return tex2D(x, vgl_texel_offset2(s, off, x)); }
inline uint4 vgl_textureOffset(usampler2D x, float2 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s, off, x), 1, b)); }
inline float4 vgl_textureOffset(sampler3D x, float3 s, int3 off) { return tex3D(x, vgl_texel_offset3(s, off, x)); }
inline float4 vgl_textureOffset(sampler3D x, float3 s, int3 off, float b) { return tex3Dbias(x, float4(vgl_texel_offset3(s, off, x), b)); }
inline int4 vgl_textureOffset(isampler3D x, float3 s, int3 off) { return tex3D(x, vgl_texel_offset3(s, off, x)); }
inline int4 vgl_textureOffset(isampler3D x, float3 s, int3 off, float b) { return tex3Dbias(x, float4(vgl_texel_offset3(s, off, x), b)); }
inline uint4 vgl_textureOffset(usampler3D x, float3 s, int3 off) { return tex3D(x, vgl_texel_offset3(s, off, x)); }
inline uint4 vgl_textureOffset(usampler3D x, float3 s, int3 off, float b) { return tex3Dbias(x, float4(vgl_texel_offset3(s, off, x), b)); }
inline float vgl_textureOffset(sampler2DShadow x, float3 s, int2 off) { return tex2D(x, float3(vgl_texel_offset2(s.xy, off, x), s.z)); }
inline float vgl_textureOffset(sampler2DShadow x, float3 s, int2 off, float b) { return tex2Dcmpbias(x, float4(vgl_texel_offset2(s.xy, off, x), s.z, b)); }
#define textureOffset vgl_textureOffset
inline float4 vgl_textureProjOffset(sampler2D x, float3 s, int2 off) { return tex2Dproj(x, float3(vgl_texel_offset2(s.xy / s.z, off, x), s.z)); }
inline float4 vgl_textureProjOffset(sampler2D x, float4 s, int2 off) { return tex2Dproj(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), s.zw)); }
inline float4 vgl_textureProjOffset(sampler2D x, float3 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s.xy / s.z, off, x), 1, b)); }
inline float4 vgl_textureProjOffset(sampler2D x, float4 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), 1, b)); }
inline int4 vgl_textureProjOffset(isampler2D x, float3 s, int2 off) { return tex2Dproj(x, float3(vgl_texel_offset2(s.xy / s.z, off, x), s.z)); }
inline int4 vgl_textureProjOffset(isampler2D x, float4 s, int2 off) { return tex2Dproj(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), s.zw)); }
inline int4 vgl_textureProjOffset(isampler2D x, float3 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s.xy / s.z, off, x), 1, b)); }
inline int4 vgl_textureProjOffset(isampler2D x, float4 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), 1, b)); }
inline uint4 vgl_textureProjOffset(usampler2D x, float3 s, int2 off) { return tex2Dproj(x, float3(vgl_texel_offset2(s.xy / s.z, off, x), s.z)); }
inline uint4 vgl_textureProjOffset(usampler2D x, float4 s, int2 off) { return tex2Dproj(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), s.zw)); }
inline uint4 vgl_textureProjOffset(usampler2D x, float3 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s.xy / s.z, off, x), 1, b)); }
inline uint4 vgl_textureProjOffset(usampler2D x, float4 s, int2 off, float b) { return tex2Dbias(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), 1, b)); }
inline float4 vgl_textureProjOffset(sampler3D x, float4 s, int3 off) { return tex3Dproj(x, float4(vgl_texel_offset3(s.xyz / s.w, off, x), s.w)); }
inline float4 vgl_textureProjOffset(sampler3D x, float4 s, int3 off, float b) { return tex3Dbias(x, float4(vgl_texel_offset3(s.xyz / s.w, off, x), b)); }
inline int4 vgl_textureProjOffset(isampler3D x, float4 s, int3 off) { return tex3Dproj(x, float4(vgl_texel_offset3(s.xyz / s.w, off, x), s.w)); }
inline int4 vgl_textureProjOffset(isampler3D x, float4 s, int3 off, float b) { return tex3Dbias(x, float4(vgl_texel_offset3(s.xyz / s.w, off, x), b)); }
inline uint4 vgl_textureProjOffset(usampler3D x, float4 s, int3 off) { return tex3Dproj(x, float4(vgl_texel_offset3(s.xyz / s.w, off, x), s.w)); }
inline uint4 vgl_textureProjOffset(usampler3D x, float4 s, int3 off, float b) { return tex3Dbias(x, float4(vgl_texel_offset3(s.xyz / s.w, off, x), b)); }
inline float vgl_textureProjOffset(sampler2DShadow x, float4 s, int2 off) { return tex2Dproj(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), s.zw)); }
inline float vgl_textureProjOffset(sampler2DShadow x, float4 s, int2 off, float b) { return tex2Dcmpbias(x, float4(vgl_texel_offset2(s.xy / s.w, off, x), s.z, b)); }
#define textureProjOffset vgl_textureProjOffset
inline float4 vgl_textureLodOffset(sampler2D x, float2 s, float lod, int2 off) { return tex2Dlod(x, float4(vgl_texel_offset2(s, off, x), lod)); }
inline int4 vgl_textureLodOffset(isampler2D x, float2 s, float lod, int2 off) { return tex2Dlod(x, float4(vgl_texel_offset2(s, off, x), lod)); }
inline uint4 vgl_textureLodOffset(usampler2D x, float2 s, float lod, int2 off) { return tex2Dlod(x, float4(vgl_texel_offset2(s, off, x), lod)); }
inline float4 vgl_textureLodOffset(sampler3D x, float3 s, float lod, int3 off) { return tex3Dlod(x, float4(vgl_texel_offset3(s, off, x), lod)); }
inline int4 vgl_textureLodOffset(isampler3D x, float3 s, float lod, int3 off) { return tex3Dlod(x, float4(vgl_texel_offset3(s, off, x), lod)); }
inline uint4 vgl_textureLodOffset(usampler3D x, float3 s, float lod, int3 off) { return tex3Dlod(x, float4(vgl_texel_offset3(s, off, x), lod)); }
inline float vgl_textureLodOffset(sampler2DShadow x, float3 s, float lod, int2 off) { return tex2Dlod(x, float4(vgl_texel_offset2(s.xy, off, x), s.z, lod)); }
#define textureLodOffset vgl_textureLodOffset
inline float4 texture2DLod(sampler2D x, float2 coord, float lod) { return tex2Dlod(x, float4(coord, 0.0f, lod)); }
#define dFdx(a) ddx(a)
#define dFdy(a) ddy(a)
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
#define uvec2 uint2
#define uvec3 uint3
#define uvec4 uint4
#define isamplerCube isamplerCUBE
#define usamplerCube usamplerCUBE
#define samplerCubeShadow samplerCUBEShadow
#define bvec2 bool2
#define bvec3 bool3
#define bvec4 bool4
#define fract(x) frac(x)
#define mod(x,y) fmod(x,y)
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#define matrix _matrix
#define Matrix _Matrix
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
)";

#endif
