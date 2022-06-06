/*
 * Physically-based FDG illumination model with GGX microfacets model
 * Formula: Lf = (kd * Ld + (F(v, h) * D(h) * G2(n, l, v)) / (4(n * v)(n * l))) * (n * l)
 */

// Weights for roughness, diffuse and Fresnel reflectance components
uniform float alpha; // 0 = smooth, 1 = rough
uniform float Kd;
uniform float F0;

// Diffuse component
uniform float3 diffuseColor; // 'Ld' in the formula

// Schlick-GGX method for geometry obstruction (from https://github.com/JoeyDeVries/LearnOpenGL/blob/bc41d2c0192220fb146c5eabf05f3d8317851200/src/6.pbr/2.2.1.ibl_specular/2.2.1.pbr.fs)
float GeometrySchlickGGX(float angle, float alpha) { // Note: angle is 'n * v' in the formula
	// G1(n, v) = (n * v) / ((n * v) (1 - k) + k) with k = ((a + 1) ^ 2) / 8
	float r = (alpha + 1.0f);
	float k = (r * r) / 8.0f;

	float num = angle;
	float denom = angle * (1.0f - k) + k;

	return num / denom;
}

float4 main(
	float3 lightDir : TEXCOORD0,
	float3 vNormal : TEXCOORD1,
	float3 vViewPosition : TEXCOORD2
) {
	// Calculating NdotL
	float3 n = normalize(vNormal);
	float3 l = normalize(lightDir);
	float NdotL = max(dot(l, n), 0.0f);
	
	// Calculating diffuse component with lambertian model
	float3 diffuse = (Kd * diffuseColor) / 3.14159265359f; // kd * Ld
	
	// Initializing specular component
	float3 specular = float3(0.0f, 0.0f, 0.0f);
	
	// Calculating specular component contribution
	if (NdotL > 0.0f) {
		// Calculating half vector
		float3 v = normalize(vViewPosition);
		float3 h = normalize(l + v);
		
		float NdotH = max(dot(n, h), 0.0f);
		float NdotV = max(dot(n, v), 0.0f);
		float VdotH = max(dot(v, h), 0.0f);

		// Fresnel reflectance F with Schlick approximation (F(x) = F(0) + (1 - F(0)) * (1 - (v * h)) ^ 5)
		float3 F = F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);

		// Microfacets GGX distribution (D(h) = a^2 / (PI((n * h)^2 * (a^2 - 1) + 1)^2)
		float alpha2 = alpha * alpha;
		float NdotH2 = NdotH * NdotH;
		float D = alpha2;
		float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
		D /= 3.14159265359f * denom * denom;
		
		// Calculating geometric factor G2 with Smith's method (G2(n, l, v) = G1(n, v) * G1(n, l))
		float G2 = GeometrySchlickGGX(NdotV, alpha) * GeometrySchlickGGX(NdotL, alpha);

		// Summing all components
		specular = (F * D * G2) / (4.0f * NdotV * NdotL); // (F(v, h) * D(h) * G2(n, l, v)) / (4(n * v)(n * l))
	}

	return float4((diffuse + specular) * NdotL, 0.0f);
}
