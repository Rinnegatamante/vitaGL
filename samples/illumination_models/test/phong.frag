static float PI = 3.14159265359;

// Weights for ambient, diffuse and specular components
uniform float Ka;
uniform float Kd;
uniform float Ks;

// Shininess coefficient
uniform float shininess;

// Ambient, diffuse and specular components
uniform float3 ambientColor;
uniform float3 diffuseColor;
uniform float3 specularColor;

float4 main(
	float3 lightDir : TEXCOORD0,
	float3 vNormal : TEXCOORD1,
	float3 vViewPosition : TEXCOORD2
) {
	// Weighting ambient component
	float3 color = Ka * ambientColor;
	
	// Calculating lambertian coefficient
	float3 N = normalize(vNormal);
	float3 L = normalize(lightDir);
	float lambertian = max(dot(L, N), 0.0f);
	
	// Calculating specular component if lambertian coefficient is positive
	if (lambertian > 0.0f) {
		float3 V = normalize(vViewPosition);
		
		// Calculating reflection vector
		float3 R = reflect(-L, N);
		
		// Calculating specular component
		float specAngle = max(dot(R, V), 0.0f);
		float specular = pow(specAngle, shininess);
		
		// Adding diffuse and specular component contribution to the final fragment color
		color += float3(Kd * lambertian * diffuseColor + Ks * specular * specularColor);
	}
	
	return float4(color, 1.0f);
}
