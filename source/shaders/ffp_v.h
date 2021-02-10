const char *ffp_vert_src =
	R"(#define clip_planes_num %d
#define has_texture %d
#define has_colors %d

void main(
	float3 position,
#if has_texture == 1
	float2 texcoord,
#endif
#if has_colors == 1
	float4 color,
#endif
#if has_texture == 1
	float2 out vTexcoord : TEXCOORD0,
#endif
	float4 out vPosition : POSITION,
#if has_colors == 1
	float4 out vColor : COLOR,
#endif
#if clip_planes_num > 0
	float out vClip[clip_planes_num] : CLP0,
	uniform float4 clip_planes_eq[clip_planes_num],
#endif
	uniform float4x4 modelview,
	uniform float4x4 wvp,
	uniform float4x4 texmat
) {
	float4 pos4 = float4(position, 1.f);
	
	// User clip planes
#if clip_planes_num > 0
	float4 modelpos = mul(modelview, pos4);
	for (int i = 0; i < clip_planes_num; i++) {
		vClip[i] = dot(modelpos, clip_planes_eq[i]);
	}
#endif
	vPosition = mul(wvp, pos4);
#if has_texture == 1
	vTexcoord = mul(texmat, float4(texcoord, 0.f, 1.f)).xy;
#endif
#if has_colors == 1
	vColor = color;
#endif
}
)";
