const char *ffp_vert_src =
R"(#define clip_planes_num %d
#define has_texture %d
#define has_colors %d
#define lights_num %d

#define CLIP_PLANE_INPUT(n) float out vClip##n : CLP##n
#define CLIP_PLANE_OUTPUT(n) vClip##n

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
#if has_colors == 1 || lights_num > 0
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
#if has_colors == 1 || lights_num > 0
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
	float clip_out[clip_planes_num];
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
	
	// Lighting
#if lights_num > 0
	float3 normal = mul(float3x3(normal_mat), normals);
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
#if has_colors == 1 || lights_num > 0
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
