uniform samplerCUBE skybox;

float4 main(
	float3 texcoords : TEXCOORD0
) {
	return texCUBE(skybox, texcoords);
}
