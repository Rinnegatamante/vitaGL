const char *ffp_frag_src =
R"(uniform float2 Opass0_scale;
uniform float2 Ppass1_scale;
uniform float2 Qpass2_scale;

%s

#define alpha_test_mode %d
#define num_textures %d
#define has_colors %d
#define fog_mode %d
#define pass0_func texenv%d
#define pass1_func texenv%d
#define pass2_func texenv%d
#define lights_num %d
#define shading_mode %d
#define point_sprite %d
#define interp %d
#define srgb_mode %d

#if interp == 1
#define TEXCOORD0 TEXCOORD0_HALF
#define TEXCOORD1 TEXCOORD1_HALF
#define TEXCOORD2 TEXCOORD2_HALF
#define TEXCOORD3 TEXCOORD3_HALF
#define TEXCOORD4 TEXCOORD4_HALF
#define TEXCOORD5 TEXCOORD5_HALF
#define TEXCOORD6 TEXCOORD6_HALF
#define TEXCOORD7 TEXCOORD7_HALF
#endif

#if lights_num > 0 && shading_mode == 1 // GL_PHONG_WIN
uniform float4 Alights_ambients[lights_num];
uniform float4 Blights_diffuses[lights_num];
uniform float4 Clights_speculars[lights_num];
uniform float4 Dlights_positions[lights_num];
uniform float3 Elights_attenuations[lights_num];
uniform float4 Flight_global_ambient;
uniform float Gshininess;

void point_light(short i, float3 normal, float3 position, float4 inout Ambient, float4 inout Diffuse, float4 inout Specular) {
	float3 VP = Dlights_positions[i].xyz - position;
	float d = length(VP);
	VP = normalize(VP);
	float attenuation = 1.0f / (Elights_attenuations[i].x +
		Elights_attenuations[i].y * d +
		Elights_attenuations[i].z * d * d);
	float nDotVP = max(0.0f, dot(normal, VP));

	Ambient += Alights_ambients[i] * attenuation;
	Diffuse += Blights_diffuses[i] * nDotVP * attenuation;
	if (nDotVP != 0.0f) {
		float nDotHV = max(0.0f, dot(normal, normalize(VP + float3(0.0f, 0.0f, 1.0f))));
		Specular += Clights_speculars[i] * pow(nDotHV, Gshininess) * attenuation;
	}
}

void directional_light(short i, float3 normal, float3 position, float4 inout Ambient, float4 inout Diffuse, float4 inout Specular) {
	float nDotVP = max(0.0f, dot(normal, normalize(Dlights_positions[i].xyz)));
		
	Ambient += Alights_ambients[i];
	Diffuse += Blights_diffuses[i] * nDotVP;
	if (nDotVP != 0.0f) {
		float nDotHV = max(0.0f, dot(normal, normalize(Dlights_positions[i].xyz - position)));
		Specular += Clights_speculars[i] * pow(nDotHV, Gshininess);
	}
}

void calculate_light(short i, float3 ecPosition, float3 N, float4 inout Ambient, float4 inout Diffuse, float4 inout Specular) {
	if (Dlights_positions[i].w == 1.0f)
		point_light(i, N, ecPosition, Ambient, Diffuse, Specular);
	else
		directional_light(i, N, ecPosition, Ambient, Diffuse, Specular);
}
#endif

float4 main(
#if num_textures > 0
	float2 vTexcoord : TEXCOORD0,
#if num_textures > 1
	float2 vTexcoord2 : TEXCOORD1,
#if num_textures > 2
	float2 vTexcoord3 : TEXCOORD2,
#endif
#endif
#endif
#if lights_num > 0 && shading_mode == 1 // GL_PHONG_WIN
	float3 vNormal : TEXCOORD3,
	float3 vEcPosition : TEXCOORD4,
	float4 vDiffuse : TEXCOORD5,
	float4 vSpecular : TEXCOORD6,
	float4 vEmission : TEXCOORD7,
#endif
#if (has_colors == 1 || lights_num > 0)
	float4 vColor : COLOR,
#endif
#if fog_mode < 3
	float4 coords : WPOS,
#endif
#if point_sprite > 0 && num_textures > 0
	float2 point_coords : SPRITECOORD,
#endif
#if num_textures > 0
	uniform sampler2D tex[num_textures],
	uniform float4 ItexEnvColor[num_textures],
#endif
	uniform float JalphaCut,
	uniform float4 KfogColor,
	uniform float4 LtintColor,
	uniform float Mfog_range,
	uniform float Nfog_far,
	uniform float Hfog_density
	)
{
#if alpha_test_mode == 6
	discard;
#endif
#if has_colors == 0 && lights_num == 0
	float4 vColor = LtintColor;
#endif
	// Lighting
#if lights_num > 0 && shading_mode == 1 // GL_PHONG_WIN
	float4 Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
	for (short i = 0; i < lights_num; i++) {
		calculate_light(i, vEcPosition, vNormal, Ambient, Diffuse, Specular);
	}
	float4 fragColor = vColor;
	vColor = vEmission + fragColor * Flight_global_ambient;
	vColor += Ambient * fragColor + Diffuse * vDiffuse + Specular * vSpecular;
	vColor = clamp(vColor, 0.0f, 1.0f);
#endif	
#if num_textures > 0
#if point_sprite > 0
	vTexcoord = point_coords;
#endif
	// Texture Environment
	float4 prevColor = vColor;
	prevColor = pass0_func(tex[0], vTexcoord, prevColor, vColor, ItexEnvColor[0]);
#if num_textures > 1
	prevColor = pass1_func(tex[1], vTexcoord2, prevColor, vColor, ItexEnvColor[1]);
#if num_textures > 2
	prevColor = pass2_func(tex[2], vTexcoord3, prevColor, vColor, ItexEnvColor[2]);
#endif
#endif
	float4 texColor = prevColor;
#else
	float4 texColor = vColor;
#endif
	
	// Alpha Test
#if alpha_test_mode == 0
	if (texColor.a < JalphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 1
	if (texColor.a <= JalphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 2
	if (texColor.a == JalphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 3
	if (texColor.a != JalphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 4
	if (texColor.a > JalphaCut){
		discard;
	}
#endif
#if alpha_test_mode == 5
	if (texColor.a >= JalphaCut){
		discard;
	}
#endif

	// Fogging
#if fog_mode < 3
	float fog_dist = coords.z / coords.w;
#if fog_mode == 0 // GL_LINEAR
	float vFog = (Nfog_far - fog_dist) / Mfog_range;
#else
	const float LOG2E = 1.442695f;
#if fog_mode == 1  // GL_EXP
	float vFog = exp(-Hfog_density * fog_dist * LOG2E);
#endif
#if fog_mode == 2  // GL_EXP2
	float vFog = exp(-Hfog_density * Hfog_density * fog_dist * fog_dist * LOG2E);
#endif
#endif
	vFog = clamp(vFog, 0.0f, 1.0f);
	texColor.rgb = lerp(KfogColor.rgb, texColor.rgb, vFog);
#endif

#if srgb_mode == 1
	float3 cutoff = float3(texColor.r < 0.0031308f ? 1.0f : 0.0f, texColor.g < 0.0031308f ? 1.0f : 0.0f, texColor.b < 0.0031308f ? 1.0f : 0.0f);
	float3 higher = float3(1.055f) * pow(texColor.rgb, float3(1.0f / 2.4f)) - float3(0.055f);
	float3 lower = texColor.rgb * float3(12.92f);
	return float4(lerp(higher, lower, cutoff), texColor.a);
#else
	return texColor;
#endif
}
)";
