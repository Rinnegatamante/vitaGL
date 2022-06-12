void main(
	float3 position,
	float2 texcoord,
	float2 out vTexcoords : TEXCOORD0,
	float4 out gl_Position : POSITION
) {
	vTexcoords = texcoord;
	gl_Position = float4(position, 1.0f);
}
