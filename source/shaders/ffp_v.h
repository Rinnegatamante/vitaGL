const char *ffp_vert_src =
	R"(#define clip_planes_num %d
#define has_texture %d
#define has_colors %d
#define lights_num %d

struct light_params {
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 position;
	float const_attenuation;
	float linear_attenuation;
	float quad_attenuation;
};

static float4 Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

void point_light(light_params src, float3 normal, float3 eye, float3 position) {
	float3 VP = src.position.xyz - position;
	float d = length(VP);
	VP = normalize(VP);
	float attenuation = 1.0f / (src.const_attenuation +
		src.linear_attenuation * d +
		src.quad_attenuation * d * d);
	float3 halfVector = normalize(VP + eye);
	float nDotVP = max(0.0f, dot(normal, VP));
	float nDotHV = max(0.0f, dot(normal, halfVector));
	float pf = 0.0f;
	if (nDotVP != 0.0f)
		pf = 1.0f;
	Ambient += src.ambient * attenuation;
	Diffuse += src.diffuse * nDotVP * attenuation;
	Specular += src.specular * pf * attenuation;
}

void directional_light(light_params src, float3 normal) {
	float nDotVP = max(0.0f, dot(normal, normalize(src.position.xyz)));
	float nDotHV = max(0.0f, dot(normal, normalize(normalize(src.position.xyz) + float3(0.0f, 0.0f, 1.0f))));
	
	float pf = 0.0f;
	if (nDotVP != 0.0f)
		pf = 1.0f;
		
	Ambient += src.ambient;
	Diffuse += src.diffuse * nDotVP;
	Specular += src.specular * pf;
}

void calculate_light(light_params src, float4 ecPosition, float3 N) {
	float3 ecPosition3 = ecPosition.xyz / ecPosition.w;
	float3 eye = float3(0.0f, 0.0f, 1.0f);
	
	if (src.position.w == 1.0f)
		point_light(src, N, eye, ecPosition3);
	else
		directional_light(src, N);
}

void main(
	float3 position,
#if has_texture == 1
	float2 texcoord,
#endif
#if has_colors == 1
	float4 color, // We re-use this for ambient values when lighting is on
#endif
#if lights_num > 0
	float4 diff,
	float4 spec,
	float4 emission,
	float3 normals,
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
#if lights_num > 0
	uniform light_params lights_config[lights_num],
#endif
	uniform float4x4 modelview,
	uniform float4x4 wvp,
	uniform float4x4 texmat,
	uniform float4x4 normal_mat
) {
	float4 pos4 = float4(position, 1.f);
	float4 modelpos = mul(modelview, pos4);
	
	// User clip planes
#if clip_planes_num > 0
	for (int i = 0; i < clip_planes_num; i++) {
		vClip[i] = dot(modelpos, clip_planes_eq[i]);
	}
#endif
	vPosition = mul(wvp, pos4);
	
	// Lighting
#if lights_num > 0
	float3 normal = normalize(mul(float3x3(modelview), normals));
	calculate_light(lights_config[0], modelpos, normal);
#if lights_num > 1
	calculate_light(lights_config[1], modelpos, normal);
#if lights_num > 2
	calculate_light(lights_config[2], modelpos, normal);
#if lights_num > 3
	calculate_light(lights_config[3], modelpos, normal);
#if lights_num > 4
	calculate_light(lights_config[4], modelpos, normal);
#if lights_num > 5
	calculate_light(lights_config[5], modelpos, normal);
#if lights_num > 6
	calculate_light(lights_config[6], modelpos, normal);
#if lights_num > 7
	calculate_light(lights_config[7], modelpos, normal);
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#if has_texture == 1
	vTexcoord = mul(texmat, float4(texcoord, 0.f, 1.f)).xy;
#endif
#if has_colors == 1
#if lights_num > 0
	vColor = emission + color * float4(0.2f, 0.2f, 0.2f, 1.0f); // TODO: glLightAmbient impl
	vColor += Ambient * color + Diffuse * diff + Specular * spec;
	vColor = clamp(vColor, 0.0f, 1.0f);
#else
	vColor = color;
#endif
#endif
}
)";
