/* 
	Fixed Masks:
	0x01 = Normal
	0x02 = Tex0
	0x04 = Tex1
	0x08 = Tex2
*/

const char *ffp_vert_src =
R"(#define clip_planes_num %d
#define num_textures %d
#define has_colors %d
#define lights_num %d
#define shading_mode %d
#define normalization %d
#define fixed_mode_mask %d
#define fixed_mode_pos %d
#define calculate_wvp %d
#define interp %d

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

#define GLFixedToFloat(fx) (float(bit_cast<short2>(fx).y + (bit_cast<unsigned short2>(fx).x * (1.0f / 65536.0f))))
#define GLFixed2ToFloat2(fx2) (float2(GLFixedToFloat(fx2.x), GLFixedToFloat(fx2.y)))
#define GLFixed3ToFloat3(fx3) (float3(GLFixedToFloat(fx3.x), GLFixedToFloat(fx3.y), GLFixedToFloat(fx3.z)))
#define GLFixed4ToFloat4(fx4) (float4(GLFixedToFloat(fx4.x), GLFixedToFloat(fx4.y), GLFixedToFloat(fx4.z), GLFixedToFloat(fx4.w)))

#if lights_num > 0 && shading_mode < 1 // GL_SMOOTH/GL_FLAT
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

void main(
	float4 position,
#if num_textures > 0
	float2 texcoord0,
#if num_textures > 1
	float2 texcoord1,
#if num_textures > 2
	float2 texcoord2,
#endif
#endif
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
#if num_textures > 0
	float2 out vTexcoord : TEXCOORD0,
#if num_textures > 1
	float2 out vTexcoord2 : TEXCOORD1,
#if num_textures > 2
	float2 out vTexcoord3 : TEXCOORD2,
#endif
#endif
#endif
#if lights_num > 0 && shading_mode == 1 //GL_PHONG_WIN
	float3 out vNormal : TEXCOORD3,
	float3 out vEcPosition : TEXCOORD4,
	float4 out vDiffuse : TEXCOORD5,
	float4 out vSpecular : TEXCOORD6,
	float4 out vEmission : TEXCOORD7,
#endif
	float4 out vPosition : POSITION,
#if has_colors == 1 || lights_num > 0
	float4 out vColor : COLOR,
#endif
	float out psize : PSIZE,
#if clip_planes_num > 0
	float out vClip[clip_planes_num] : CLP0,
	uniform float4 clip_planes_eq[clip_planes_num],
#endif
#if has_colors == 0 && lights_num > 0
	uniform float4 ambient,
#endif
#if clip_planes_num > 0 || lights_num > 0 || calculate_wvp == 1
	uniform float4x4 modelview,
#endif
	uniform float4x4 wvp,
#if num_textures > 0
	uniform float4x4 texmat[num_textures],
#endif
	uniform float point_size,
	uniform float3x3 normal_mat
) {
#if fixed_mode_pos == 1
	position.xy = GLFixed2ToFloat2(position.xy);
#endif
#if fixed_mode_pos == 2
	position.xyz = GLFixed3ToFloat3(position.xyz);
#endif
#if fixed_mode_pos == 3
	position = GLFixed4ToFloat4(position);
#endif
#if calculate_wvp == 1
	wvp = mul(wvp, modelview); // wvp is actually the proj matrix
#endif
#if clip_planes_num > 0 || lights_num > 0
	float4 modelpos = mul(modelview, position);
#endif
	// User clip planes
#if clip_planes_num > 0
	for (short i = 0; i < clip_planes_num; i++) {
		vClip[i] = dot(modelpos, clip_planes_eq[i]);
	}
#endif
	vPosition = mul(wvp, position);
	
	// Lighting
#if lights_num > 0
#if (fixed_mode_mask & 0x01) == 0x01
	normals = GLFixed3ToFloat3(normals);
#endif
#if normalization == 1
	float3 normal = normalize(mul(normal_mat, normals));
#else
	float3 normal = mul(normal_mat, normals);
#endif
	float3 ecPosition = modelpos.xyz / modelpos.w;
#if shading_mode < 1 // GL_SMOOTH/GL_FLAT
	float4 Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
	for (short i = 0; i < lights_num; i++) {
		calculate_light(i, ecPosition, normal, Ambient, Diffuse, Specular);
	}
#endif
#endif

#if num_textures > 0
#if (fixed_mode_mask & 0x02) == 0x02
	texcoord0 = GLFixed2ToFloat2(texcoord0);
#endif
	vTexcoord = mul(texmat[0], float4(texcoord0, 0.f, 1.f)).xy;
#if num_textures > 1
#if (fixed_mode_mask & 0x04) == 0x04
	texcoord1 = GLFixed2ToFloat2(texcoord1);
#endif
	vTexcoord2 = mul(texmat[1], float4(texcoord1, 0.f, 1.f)).xy;
#if num_textures > 2
#if (fixed_mode_mask & 0x08) == 0x08
	texcoord2 = GLFixed2ToFloat2(texcoord2);
#endif
	vTexcoord3 = mul(texmat[2], float4(texcoord2, 0.f, 1.f)).xy;
#endif
#endif
#endif
#if lights_num > 0
#if has_colors == 0
	float4 color = ambient;
#endif
#if shading_mode < 1 // GL_SMOOTH/GL_FLAT
	vColor = emission + color * light_global_ambient;
	vColor += Ambient * color + Diffuse * diff + Specular * spec;
	vColor = clamp(vColor, 0.0f, 1.0f);
#endif
#if shading_mode == 1 // GL_PHONG_WIN
	vColor = color;
	vNormal = normal;
	vEcPosition = ecPosition;
	vDiffuse = diff;
	vSpecular = spec;
	vEmission = emission;
#endif
#elif has_colors == 1
	vColor = color;
#endif
	psize = point_size;
}
)";
