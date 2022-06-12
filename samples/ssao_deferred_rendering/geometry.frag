float4 main(
	float3 vNormal : TEXCOORD0
) {
	return float4(normalize(vNormal), 1.0f);
}
