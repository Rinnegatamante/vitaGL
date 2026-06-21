#version 330 core

in vec3 texcoords;

uniform samplerCube skybox;
uniform float uTime;

out vec4 FragColor;

void main() {
	vec3 dir = normalize(texcoords);

	// roundEven: horizontal banding on elevation
	float band = roundEven(dir.y * 5.0 + sin(uTime * 0.35) * 0.5) / 5.0;

	// samplerCube: texture + textureLod
	vec3 sharp = texture(skybox, dir).rgb;
	float lod = mix(0.0, 2.0, abs(dir.y));
	vec3 soft = textureLod(skybox, dir, lod).rgb;

	vec3 color = mix(sharp, soft, 0.18 + abs(band) * 0.12);
	FragColor = vec4(color, 1.0);
}
