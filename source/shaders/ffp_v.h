const char *ffp_vert_src =
R"(#define has_clip_plane %d
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
#if has_clip_plane == 1
	float out vClip : CLP0,
#endif
	uniform float4 clip_plane0_eq,
	uniform float4x4 modelview,
	uniform float4x4 wvp
) {
	float4 pos4 = float4(position, 1.f);

#if has_clip_plane == 1
	// User clip planes
	float4 modelpos = mul(modelview, pos4);
	vClip = dot(modelpos, clip_plane0_eq);
#endif

	vPosition = mul(wvp, pos4);
#if has_texture == 1
	vTexcoord = texcoord;
#endif
#if has_colors == 1
	vColor = color;
#endif
}
)";
