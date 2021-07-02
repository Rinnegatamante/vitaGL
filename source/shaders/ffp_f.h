const char *ffp_frag_src =
R"(%s
#define alpha_test_mode %d
#define num_textures %d
#define has_colors %d
#define fog_mode %d
#define pass0_func texenv%d
#define pass1_func texenv%d

float4 main(
#if num_textures > 0
	float2 vTexcoord : TEXCOORD0,
#if num_textures > 1
	float2 vTexcoord2 : TEXCOORD1,
#endif
#endif
#if has_colors == 1
	float4 vColor : COLOR,
#endif
#if fog_mode < 3
	float4 coords : WPOS,
#endif
#if num_textures > 0
	uniform sampler2D tex[num_textures],
	uniform float tex_env_mode[num_textures],
	uniform float4 texEnvColor[num_textures],
#endif
	uniform float alphaCut,
	uniform float4 fogColor,
	uniform float4 tintColor,
	uniform float fog_near,
	uniform float fog_far,
	uniform float fog_density
	)
{
#if alpha_test_mode == 6
	discard;
#endif
#if has_colors == 0
	float4 vColor = tintColor;
#endif
#if num_textures > 0
	// Texture Environment
	float4 prevColor = vColor;
	prevColor = pass0_func(tex[0], vTexcoord, prevColor, vColor, texEnvColor[0]);
#if num_textures > 1
	prevColor = pass1_func(tex[1], vTexcoord2, prevColor, vColor, texEnvColor[1]);
#endif
	float4 texColor = prevColor;
#else
	float4 texColor = vColor;
#endif
	
	// Alpha Test
#if alpha_test_mode == 0
	if (texColor.a < alphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 1
	if (texColor.a <= alphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 2
	if (texColor.a == alphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 3
	if (texColor.a != alphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 4
	if (texColor.a > alphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 5
	if (texColor.a >= alphaCut){
		discard;
	}
#endif
	
	// Fogging
#if fog_mode < 3
#if fog_mode == 0 // GL_LINEAR
	float vFog = (fog_far - coords.z) / (fog_far - fog_near);
#endif
#if fog_mode == 1  // GL_EXP
	float vFog = exp(-fog_density * coords.z);
#endif
#if fog_mode == 2  // GL_EXP2
	const float LOG2 = -1.442695;
	float d = fog_density * coords.z;
	float vFog = exp(d * d * LOG2);
#endif
	vFog = clamp(vFog, 0.0, 1.0);
	texColor.rgb = lerp(fogColor.rgb, texColor.rgb, vFog);
#endif

	return texColor;
}
)";
