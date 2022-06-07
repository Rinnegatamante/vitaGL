// Model matrix
uniform float4x4 modelMatrix;

// View matrix
uniform float4x4 viewMatrix;

// Projection matrix
uniform float4x4 projectionMatrix;

// Normal matrix
uniform float3x3 normalMatrix;

// Point light position
uniform float3 pointLightPosition;

void main(
	float3 position,
	float3 normal,
	float3 out lightDir : TEXCOORD0,
	float3 out vNormal : TEXCOORD1,
	float3 out vViewPosition : TEXCOORD2,
	float4 out gl_Position : POSITION
) {
	// Calculating vertex position in modelview coordinate
	float4 mvPosition = mul(mul(float4(position, 1.0f), modelMatrix), viewMatrix);
	
	// View direction
	vViewPosition = -mvPosition.xyz;
	
	// Applying transformations to normals
	vNormal = normalize(mul(normal, normalMatrix));
	
	// Calculating light incidence direction
	float4 lightPos = mul(float4(pointLightPosition, 1.0f), viewMatrix);
	lightDir = lightPos.xyz - mvPosition.xyz;
	
	// Calculating final position in clip space
	gl_Position = mul(mvPosition, projectionMatrix);
}
