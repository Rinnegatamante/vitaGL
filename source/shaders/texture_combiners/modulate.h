/*
 * GL_MODULATE
 */
const char *modulate_src =
R"(float4 texenv0(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	return tex2D(tex, texcoord) * prepass;
}
)";
