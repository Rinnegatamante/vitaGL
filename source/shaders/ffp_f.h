const char *ffp_frag_src =
R"(#define alpha_test_mode %d
#define has_texture %d
#define has_colors %d
#define fog_mode %d
#define tex_env_mode %d

float4 main(
#if has_texture == 1
	float2 vTexcoord : TEXCOORD0,
#endif
#if has_colors == 1
	float4 vColor : COLOR,
#endif
#if fog_mode < 3
	float4 coords : WPOS,
#endif
	uniform sampler2D tex,
	uniform float alphaCut,
	uniform float4 fogColor,
	uniform float4 texEnvColor,
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
#if has_texture == 0
	float4 texColor = vColor;
#else
	float4 texColor = tex2D(tex, vTexcoord);

	// Texture Environment
#if tex_env_mode == 0 // GL_MODULATE
	texColor = texColor * vColor;
#endif
#if tex_env_mode == 1 // GL_DECAL
	texColor.rgb = lerp(vColor.rgb, texColor.rgb, texColor.a);
	texColor.a = vColor.a;
#endif
#if tex_env_mode == 2 // GL_BLEND
	texColor.rgb = lerp(vColor.rgb, texEnvColor.rgb, texColor.rgb);
	texColor.a = texColor.a * vColor.a;
#endif
#if tex_env_mode == 3 // GL_ADD
	texColor.rgb = clamp(texColor.rgb + vColor.rgb, 0.0, 1.0);
	texColor.a = texColor.a * vColor.a;
#endif
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