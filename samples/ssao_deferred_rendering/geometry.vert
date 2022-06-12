// Useful for when rendering an indoor environment
uniform float invertedNormals;

uniform float4x4 modelMatrix;
uniform float4x4 viewMatrix;
uniform float4x4 projectionMatrix;
uniform float3x3 normalMatrix;

void main(
	float3 position,
	float3 normal,
	float4 out gl_Position : POSITION,
	float3 out vNormal : TEXCOORD0
) {
    gl_Position = mul(mul(mul(float4(position, 1.0f), modelMatrix), viewMatrix), projectionMatrix);
    
	if (invertedNormals == 1.0f)
		vNormal = mul(-normal, normalMatrix);
	else
		vNormal = mul(normal, normalMatrix);
}
