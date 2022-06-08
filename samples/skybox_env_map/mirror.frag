// Cube sampler for our skybox
uniform samplerCUBE skybox;

// Camera position
uniform float3 camera_pos;

float4 main(
	float3 wNormal : TEXCOORD0,
	float3 wPosition : TEXCOORD1
) {
	return texCUBE(skybox, reflect(normalize(wPosition - camera_pos), normalize(wNormal)));
}
