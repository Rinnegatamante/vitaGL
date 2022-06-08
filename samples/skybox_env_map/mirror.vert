// Model matrix
uniform float4x4 modelMatrix;
uniform float4x4 inverseModelMatrix;

// View matrix
uniform float4x4 viewMatrix;

// Projection matrix
uniform float4x4 projectionMatrix;

void main(
	float3 position,
	float3 normal,
	float3 out wNormal : TEXCOORD0,
	float3 out wPosition : TEXCOORD1,
	float4 out gl_Position : POSITION
) {
	// Calculating vertex position in world coordinates
	wPosition = mul(float4(position, 1.0f), modelMatrix).xyz;
	
	// Calculate normals in world coordinates
	wNormal = mul(normal, float3x3(transpose(inverseModelMatrix)));
	
	// Calculating final position in clip space
	gl_Position = mul(mul(float4(wPosition, 1.0f), viewMatrix), projectionMatrix);
}
