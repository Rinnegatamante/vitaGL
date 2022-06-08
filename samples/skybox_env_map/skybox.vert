uniform float4x4 projectionMatrix;
uniform float4x4 viewMatrix;

void main (
	float3 position,
	float3 out texcoords : TEXCOORD0,
	float4 out gl_Position : POSITION
) {
	texcoords = position;
	gl_Position = mul(mul(float4(position, 1.0f), viewMatrix), projectionMatrix).xyww;
}
