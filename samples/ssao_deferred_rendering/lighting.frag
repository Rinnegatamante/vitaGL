uniform sampler2D gDepthMap : TEXUNIT0;
uniform sampler2D gNormal : TEXUNIT1;
uniform sampler2D SSAO : TEXUNIT2;

uniform float4x4 invProjectionMatrix;

uniform float3 lightPosition;
uniform float3 lightColor;
uniform float linearAttenuation;
uniform float quadraticAttenuation;

float3 CalcViewPos(float2 texcoords)
{
	float4 clip_space_pos = float4(texcoords, tex2D<float>(gDepthMap, texcoords), 1.0f);
	clip_space_pos = clip_space_pos * 2.0f - float4(1.0f, 1.0f, 1.0f, 1.0f);
	float4 view_pos = mul(clip_space_pos, invProjectionMatrix);
	return view_pos.xyz / view_pos.w;
}

float4 main(
	float2 vTexcoords : TEXCOORD0
) {	 
	// Reconstruct view position from depth buffer
	float3 FragPos = CalcViewPos(vTexcoords);
	
	// retrieve data from gbuffer
	float3 Normal = tex2D(gNormal, vTexcoords).xyz;
	float3 Diffuse = float3(0.95f, 0.95f, 0.95f);
	float AmbientOcclusion = tex2D<float>(SSAO, vTexcoords);
	
	// Ambient coefficient
	float3 ambient = float3(0.3f * Diffuse * AmbientOcclusion);
	float3 lighting  = ambient; 
	float3 viewDir  = normalize(-FragPos);
	
	// Diffuse coefficient
	float3 lightDir = normalize(lightPosition - FragPos);
	float3 diffuse = max(dot(Normal, lightDir), 0.0f) * Diffuse * lightColor;
	
	// Specular coefficient
	float3 halfwayDir = normalize(lightDir + viewDir);  
	float spec = pow(max(dot(Normal, halfwayDir), 0.0f), 8.0f);
	float3 specular = lightColor * spec;
	
	// Attenuation (Linear and Quadratic)
	float dist = length(lightPosition - FragPos);
	float attenuation = 1.0f / (1.0f + linearAttenuation * dist + quadraticAttenuation * dist * dist);
	diffuse *= attenuation;
	specular *= attenuation;
	lighting += diffuse + specular;

	return float4(lighting, 1.0f);
}
