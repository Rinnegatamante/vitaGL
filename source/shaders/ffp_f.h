const char *ffp_frag_src =
R"(%s
#define alpha_test_mode %d
#define num_textures %d
#define has_colors %d
#define fog_mode %d
#define pass0_func texenv%d
#define pass1_func texenv%d
#define lights_num %d
#define shading_mode %d

#if lights_num > 0 && shading_mode == 2 // GL_PHONG_WIN
static float4 Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

uniform float4 lights_ambients[lights_num];
uniform float4 lights_diffuses[lights_num];
uniform float4 lights_speculars[lights_num];
uniform float4 lights_positions[lights_num];
uniform float3 lights_attenuations[lights_num];

void point_light(int i, float3 normal, float3 eye, float3 position) {
	float3 VP = lights_positions[i].xyz - position;
	float d = length(VP);
	VP = normalize(VP);
	float attenuation = 1.0f / (lights_attenuations[i].x +
		lights_attenuations[i].y * d +
		lights_attenuations[i].z * d * d);
	float3 halfVector = normalize(VP + eye);
	float nDotVP = max(0.0f, dot(normal, VP));
	float nDotHV = max(0.0f, dot(normal, halfVector));
	float pf = 0.0f;
	if (nDotVP != 0.0f)
		pf = 1.0f;
	Ambient += lights_ambients[i] * attenuation;
	Diffuse += lights_diffuses[i] * nDotVP * attenuation;
	Specular += lights_speculars[i] * pf * attenuation;
}

void directional_light(int i, float3 normal) {
	float nDotVP = max(0.0f, dot(normal, normalize(lights_positions[i].xyz)));
	float nDotHV = max(0.0f, dot(normal, normalize(normalize(lights_positions[i].xyz) + float3(0.0f, 0.0f, 1.0f))));
	
	float pf = 0.0f;
	if (nDotVP != 0.0f)
		pf = 1.0f;
		
	Ambient += lights_ambients[i];
	Diffuse += lights_diffuses[i] * nDotVP;
	Specular += lights_speculars[i] * pf;
}

void calculate_light(int i, float3 ecPosition, float3 N) {
	float3 eye = float3(0.0f, 0.0f, 1.0f);
	
	if (lights_positions[i].w == 1.0f)
		point_light(i, N, eye, ecPosition);
	else
		directional_light(i, N);
}
#endif

float4 main(
#if num_textures > 0
	float2 vTexcoord : TEXCOORD0,
#if num_textures > 1
	float2 vTexcoord2 : TEXCOORD1,
#endif
#endif
#if lights_num > 0 && shading_mode == 2 // GL_PHONG_WIN
	float3 vNormal : TEXCOORD2,
	float3 vEcPosition : TEXCOORD3,
	float4 vDiffuse : TEXCOORD4,
	float4 vSpecular : TEXCOORD5,
	float4 vEmission : TEXCOORD6,
#endif
#if has_colors == 1
	float4 vColor : COLOR,
#endif
#if fog_mode < 3
	float4 coords : WPOS,
#endif
#if num_textures > 0
	uniform sampler2D tex[num_textures],
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

	// Lighting
#if lights_num > 0 && shading_mode == 2 // GL_PHONG_WIN
	for (int i = 0; i < lights_num; i++) {
		calculate_light(i, vEcPosition, vNormal);
	}
	float4 fragColor = texColor;
	texColor = vEmission + fragColor * float4(0.2f, 0.2f, 0.2f, 1.0f); // TODO: glLightAmbient impl
	texColor += Ambient * fragColor + Diffuse * vDiffuse + Specular * vSpecular;
	texColor = clamp(texColor, 0.0f, 1.0f);
#endif	

	return texColor;
}
)";
