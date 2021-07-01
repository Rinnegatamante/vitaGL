const char *ffp_frag_src =
R"(#define alpha_test_mode %d
#define num_textures %d
#define has_colors %d
#define fog_mode %d
#define tex_env_mode %d

float4 main(
#if num_textures > 0
	float2 vTexcoord : TEXCOORD0,
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
	for (int i = 0; i < num_textures; i++) {
		float4 currColor = tex2D(tex[i], vTexcoord);
		if (tex_env_mode[i] == 0.0f) { // GL_MODULATE
			currColor = currColor * vColor;
		} else if (tex_env_mode[i] == 1.0f) { // GL_DECAL
			currColor.rgb = lerp(vColor.rgb, currColor.rgb, currColor.a);
			currColor.a = vColor.a;
		} else if (tex_env_mode[i] == 2.0f) { // GL_BLEND
			currColor.rgb = lerp(vColor.rgb, texEnvColor.rgb, currColor.rgb);
			currColor.a = currColor.a * vColor.a;
		} else if (tex_env_mode[i] == 3.0f) { // GL_ADD
			currColor.rgb = clamp(currColor.rgb + vColor.rgb, 0.0, 1.0);
			currColor.a = currColor.a * vColor.a;
		}
		vColor = currColor;
	}
#endif
	float4 texColor = vColor;
	
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