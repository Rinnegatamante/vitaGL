#version 330 core

in vec3 wNormal;
in vec3 wPosition;

uniform samplerCube skybox;
uniform sampler2D palette;
uniform sampler3D colorVol;

uniform vec3 camera_pos;
uniform float uTime;
uniform int uDemo;

out vec4 FragColor;

vec3 demoPalette(float t) {
	vec2 uv = vec2(clamp(t, 0.0, 1.0), 0.5);
	vec4 base = texture(palette, uv);
	ivec2 off = ivec2(int(roundEven(sin(uTime * 2.0) * 2.0)), 0);
	vec4 shifted = textureOffset(palette, uv, off);
	vec4 lod = textureLod(palette, uv, 1.5);
	vec4 proj = textureProj(palette, vec4(uv * 2.0 - 1.0, 1.0, 1.5));
	return mix(base.rgb, shifted.rgb, 0.35) + lod.rgb * 0.15 + proj.rgb * 0.1;
}

void main() {
	vec3 I = normalize(wPosition - camera_pos);
	vec3 N = normalize(wNormal);
	vec3 R = reflect(I, N);
	float fresnel = clamp(1.0 - dot(N, -I), 0.0, 1.0);

	// samplerCube
	vec3 env = texture(skybox, R).rgb;
	vec3 envBlur = textureLod(skybox, R, mix(0.5, 2.5, fresnel)).rgb;

	// roundEven posterization
	float shade = roundEven(fresnel * 7.0) / 7.0;

	// bitfieldExtract: unpack tint from fresnel bit pattern
	int bits = floatBitsToInt(fresnel);
	int rBand = bitfieldExtract(bits, 16, 8);
	int gBand = bitfieldExtract(bits, 8, 8);
	int bBand = bitfieldExtract(bits, 0, 8);
	vec3 bitTint = vec3(float(rBand & 63) / 63.0,
		float(gBand & 63) / 63.0,
		float(bBand & 63) / 63.0);

	// sampler2D variants
	vec3 paletteTint = demoPalette(fresnel);

	// sampler3D: animated volume lookup (3D atlas bound on texture unit 2)
	vec3 volCoord = fract(wPosition * 0.18 + vec3(uTime * 0.04, uTime * 0.06, 0.0));
	vec3 vol = texture(colorVol, volCoord).rgb;
	vec3 volLod = textureLod(colorVol, volCoord, 1.0).rgb;
	vec3 volOff = textureOffset(colorVol, volCoord, ivec3(1, -1, 0)).rgb;
	vec3 volProj = textureProj(colorVol, vec4(volCoord.xy * 2.0 - 1.0, volCoord.z, 1.0)).rgb;

	vec3 color = mix(env, envBlur, 0.25 + shade * 0.35);
	color = mix(color, paletteTint, 0.22);
	color = mix(color, bitTint, 0.18 * float(uDemo == 1 || uDemo == 3));
	color = mix(color, vol, 0.20 * float(uDemo >= 2));
	color = mix(color, volLod, 0.10 * float(uDemo >= 2));
	color = mix(color, volOff, 0.08 * float(uDemo == 3));
	color = mix(color, volProj, 0.06 * float(uDemo == 3));
	color *= 0.55 + shade * 0.45;

	FragColor = vec4(color, 1.0);
}
