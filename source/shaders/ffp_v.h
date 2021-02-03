const char *ffp_vert_src =
R"(#define clip_planes_num %d
#define has_texture %d
#define has_colors %d

#define CLIP_PLANE_INPUT(n) float out vClip##n : CLP##n
#define CLIP_PLANE_OUTPUT(n) vClip##n

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
	CLIP_PLANE_INPUT(0),
#if clip_planes_num > 1
	CLIP_PLANE_INPUT(1),
#if clip_planes_num > 2
	CLIP_PLANE_INPUT(2),
#if clip_planes_num > 3
	CLIP_PLANE_INPUT(3),
#if clip_planes_num > 4
	CLIP_PLANE_INPUT(4),
#if clip_planes_num > 5
	CLIP_PLANE_INPUT(5),
#if clip_planes_num > 6
	CLIP_PLANE_INPUT(6),
#endif
#endif
#endif
#endif
#endif
#endif
	uniform float4 clip_planes_eq[clip_planes_num],
#endif
	uniform float4x4 modelview,
	uniform float4x4 wvp,
	uniform float4x4 texmat
) {
	float4 pos4 = float4(position, 1.f);
	
	// User clip planes
#if clip_planes_num > 0
	float clip_out[clip_planes_num];
	float4 modelpos = mul(modelview, pos4);
	for (int i = 0; i < clip_planes_num; i++) {
		clip_out[i] = dot(modelpos, clip_planes_eq[i]);
	}
	CLIP_PLANE_OUTPUT(0) = clip_out[0];
#if clip_planes_num > 1
	CLIP_PLANE_OUTPUT(1) = clip_out[1];
#if clip_planes_num > 2
	CLIP_PLANE_OUTPUT(2) = clip_out[2];
#if clip_planes_num > 3
	CLIP_PLANE_OUTPUT(3) = clip_out[3];
#if clip_planes_num > 4
	CLIP_PLANE_OUTPUT(4) = clip_out[4];
#if clip_planes_num > 5
	CLIP_PLANE_OUTPUT(5) = clip_out[5];
#if clip_planes_num > 6
	CLIP_PLANE_OUTPUT(6) = clip_out[6];
#endif
#endif
#endif
#endif
#endif
#endif
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
