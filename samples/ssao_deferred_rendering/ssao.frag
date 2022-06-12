uniform sampler2D gDepthMap : TEXUNIT0;
uniform sampler2D gNormal : TEXUNIT1;
uniform sampler2D noiseTexture : TEXUNIT2; 

uniform float3 kernel[64];

// Tile noise texture over screen based on screen dimensions divided by noise size
static float2 noiseScale = float2(960.0/4.0, 544.0/4.0); 

uniform float4x4 projectionMatrix;
uniform float4x4 invProjectionMatrix;

float3 CalcViewPos(float2 texcoords)
{
	float4 clip_space_pos = float4(texcoords, tex2D<float>(gDepthMap, texcoords), 1.0f);
	clip_space_pos = clip_space_pos * 2.0f - float4(1.0f, 1.0f, 1.0f, 1.0f);
	float4 view_pos = mul(clip_space_pos, invProjectionMatrix);
	return view_pos.xyz / view_pos.w;
}

float main(
	float2 vTexcoords : TEXCOORD0
) {
	// get input for SSAO algorithm
	float3 fragPos = CalcViewPos(vTexcoords);
	float3 normal = normalize(tex2D(gNormal, vTexcoords).xyz);
	float3 randomVec = normalize(tex2D(noiseTexture, vTexcoords * noiseScale).xyz);
	
	// Create TBN change-of-basis matrix: from tangent-space to view-space
	float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, normal);
	
	// Iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0f;
	for (int i = 0; i < 64; ++i) {
		// Get sample position
		float3 samplePos = mul(kernel[i], TBN); // Get the sample to camera space
		samplePos = fragPos + samplePos * 0.5f; 
		
		// Project sample position (to sample texture) (to get position on screen/texture)
		float4 offset = float4(samplePos, 1.0f);
		offset = mul(offset, projectionMatrix); // Get the offset to clip space
		offset.xyz /= offset.w; // Normalize the value
		offset.xyz = offset.xyz * 0.5f + 0.5f; // Get it in [0, 1] range
		
		// Get sample depth
		float sampleDepth = CalcViewPos(offset.xy).z;
		
		// Range check & accumulate
		float rangeCheck = smoothstep(0.0f, 1.0f, 0.5f / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + 0.025f ? 1.0f : 0.0f) * rangeCheck;		   
	}
	occlusion = 1.0f - (occlusion / 64);
	
	return occlusion;
}