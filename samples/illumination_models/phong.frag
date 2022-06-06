/*
 * Phong illumination model
 * Formula: Lf = Le + kaLa + Li(kdLd(l * n) + ksLs(r * v) ^ a)
 */

// Weights for ambient, diffuse and specular components
uniform float Ka;
uniform float Kd;
uniform float Ks;

// Shininess coefficient
uniform float shininess; // 'a' in the formula

// Ambient, diffuse and specular components
uniform float3 ambientColor; // 'La' in the formula
uniform float3 diffuseColor; // 'Ld' in the formula
uniform float3 specularColor; // 'Ls' in the formula

float4 main(
	float3 lightDir : TEXCOORD0,
	float3 vNormal : TEXCOORD1,
	float3 vViewPosition : TEXCOORD2
) {
	// Weighting ambient component
	float3 color = Ka * ambientColor;
	
	float3 n = normalize(vNormal);
	float3 l = normalize(lightDir);
	float NdotL = max(dot(l, n), 0.0f);
	
	// Calculating specular component if the angle between light vector and fragment normal is positive
	if (NdotL > 0.0f) {
		float3 v = normalize(vViewPosition);
		
		// Calculating reflection vector
		float3 r = reflect(-l, n);
		
		// Calculating specular component
		float VdotR = max(dot(r, v), 0.0f);
		float specular = pow(VdotR, shininess); // (r * v) ^ a
		
		// Adding diffuse and specular component contribution to the final fragment color
		color += float3(Kd * diffuseColor * NdotL + Ks * specularColor * specular);
	}
	
	return float4(color, 1.0f);
}
