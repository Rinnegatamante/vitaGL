const char *ffp_frag_src =
R"(uniform float pass0_rgb_scale;
uniform float pass0_a_scale;
uniform float pass1_rgb_scale;
uniform float pass1_a_scale;

%s

#define alpha_test_mode %d
#define num_textures %d
#define has_colors %d
#define fog_mode %d
#define pass0_func texenv%d
#define pass1_func texenv%d
#define lights_num %d
#define shading_mode %d
#define point_sprite %d
#define interp %d

#if interp == 1
#define TEXCOORD0 TEXCOORD0_HALF
#define TEXCOORD1 TEXCOORD1_HALF
#define TEXCOORD2 TEXCOORD2_HALF
#define TEXCOORD3 TEXCOORD3_HALF
#define TEXCOORD4 TEXCOORD4_HALF
#define TEXCOORD5 TEXCOORD5_HALF
#define TEXCOORD6 TEXCOORD6_HALF
#endif

#if lights_num > 0 && shading_mode == 1 // GL_PHONG_WIN
uniform float4 lights_ambients[lights_num];
uniform float4 lights_diffuses[lights_num];
uniform float4 lights_speculars[lights_num];
uniform float4 lights_positions[lights_num];
uniform float3 lights_attenuations[lights_num];
uniform float4 light_global_ambient;
uniform float shininess;

void point_light(short i, float3 normal, float3 position, float4 inout Ambient, float4 inout Diffuse, float4 inout Specular) {
	float3 VP = lights_positions[i].xyz - position;
	float d = length(VP);
	VP = normalize(VP);
	float attenuation = 1.0f / (lights_attenuations[i].x +
		lights_attenuations[i].y * d +
		lights_attenuations[i].z * d * d);
	float nDotVP = max(0.0f, dot(normal, VP));

	Ambient += lights_ambients[i] * attenuation;
	Diffuse += lights_diffuses[i] * nDotVP * attenuation;
	if (nDotVP != 0.0f) {
		float nDotHV = max(0.0f, dot(normal, normalize(VP + float3(0.0f, 0.0f, 1.0f))));
		Specular += lights_speculars[i] * pow(nDotHV, shininess) * attenuation;
	}
}

void directional_light(short i, float3 normal, float3 position, float4 inout Ambient, float4 inout Diffuse, float4 inout Specular) {
	float nDotVP = max(0.0f, dot(normal, normalize(lights_positions[i].xyz)));
		
	Ambient += lights_ambients[i];
	Diffuse += lights_diffuses[i] * nDotVP;
	if (nDotVP != 0.0f) {
		float nDotHV = max(0.0f, dot(normal, normalize(lights_positions[i].xyz - position)));
		Specular += lights_speculars[i] * pow(nDotHV, shininess);
	}
}

void calculate_light(short i, float3 ecPosition, float3 N, float4 inout Ambient, float4 inout Diffuse, float4 inout Specular) {
	if (lights_positions[i].w == 1.0f)
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
#endif
#endif
#if lights_num > 0 && shading_mode == 1 // GL_PHONG_WIN
	float3 vNormal : TEXCOORD2,
	float3 vEcPosition : TEXCOORD3,
	float4 vDiffuse : TEXCOORD4,
	float4 vSpecular : TEXCOORD5,
	float4 vEmission : TEXCOORD6,
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
	uniform float4 texEnvColor[num_textures],
#endif
	uniform float alphaCut,
	uniform float4 fogColor,
	uniform float4 tintColor,
	uniform float fog_range,
	uniform float fog_far,
	uniform float fog_density
	)
{
#if alpha_test_mode == 6
	discard;
#endif
#if has_colors == 0 && lights_num == 0
	float4 vColor = tintColor;
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
	vColor = vEmission + fragColor * light_global_ambient;
	vColor += Ambient * fragColor + Diffuse * vDiffuse + Specular * vSpecular;
	vColor = clamp(vColor, 0.0f, 1.0f);
#endif
#if num_textures > 0
#if point_sprite > 0
	vTexcoord = point_coords;
#endif
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
	if (texColor.a < alphaCut) {
		discard;
	}
#endif
#if alpha_test_mode == 1
	if (texColor.a <= alphaCut) {
		discard;
	}
#endif
#if alpha_test_mode == 2
	if (texColor.a == alphaCut) {
		discard;
	}
#endif
#if alpha_test_mode == 3
	if (texColor.a != alphaCut) {
		discard;
	}
#endif
#if alpha_test_mode == 4
	if (texColor.a > alphaCut) {
		discard;
	}
#endif
#if alpha_test_mode == 5
	if (texColor.a >= alphaCut) {
		discard;
	}
#endif

	// Fogging
#if fog_mode < 3
	float fog_dist = coords.z / coords.w;
#if fog_mode == 0 // GL_LINEAR
	float vFog = (fog_far - fog_dist) / fog_range;
#else
	const float LOG2E = 1.442695f;
#if fog_mode == 1  // GL_EXP
	float vFog = exp(-fog_density * fog_dist * LOG2E);
#endif
#if fog_mode == 2  // GL_EXP2
	float vFog = exp(-fog_density * fog_density * fog_dist * fog_dist * LOG2E);
#endif
#endif
	vFog = clamp(vFog, 0.0f, 1.0f);
	texColor.rgb = lerp(fogColor.rgb, texColor.rgb, vFog);
#endif

	return texColor;
}
)";
