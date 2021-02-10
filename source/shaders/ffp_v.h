const char *ffp_vert_src =
	R"(#define clip_planes_num %d
#define has_texture %d
#define has_colors %d
#define lights_num %d

static float4 Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
static float4 Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

#if lights_num > 0
uniform float4 lights_ambients[lights_num];
uniform float4 lights_diffuses[lights_num];
uniform float4 lights_speculars[lights_num];
uniform float4 lights_positions[lights_num];
uniform float4 lights_attenuations[lights_num];
#endif

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

void calculate_light(int i, float4 ecPosition, float3 N) {
	float3 ecPosition3 = ecPosition.xyz / ecPosition.w;
	float3 eye = float3(0.0f, 0.0f, 1.0f);
	
	if (lights_positions[i].w == 1.0f)
		point_light(i, N, eye, ecPosition3);
	else
		directional_light(i, N);
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
	for (int i = 0; i < lights_num; i++) {
		calculate_light(i, modelpos, normal);
	}
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
