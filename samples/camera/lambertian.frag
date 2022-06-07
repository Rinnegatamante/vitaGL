/*
 * Lambertian illumination model
 * Formula: Ld = kd * Li (l * n)
 */

// Weights for diffuse component
uniform float Kd;

// Diffuse component
uniform float3 diffuseColor; // 'Li' in the formula

float4 main(
	float3 lightDir : TEXCOORD0,
	float3 vNormal : TEXCOORD1,
	float3 vViewPosition : TEXCOORD2
) {
	float3 n = normalize(vNormal);
	float3 l = normalize(lightDir);
	
	float NdotL = max(dot(l, n), 0.0f);
	
	return float4(float3(Kd * diffuseColor * NdotL), 1.0f);
}
