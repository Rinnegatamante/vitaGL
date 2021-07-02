/*
 * GL_BLEND
 */
const char *blend_src =
R"(float4 texenv2(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	float4 res = tex2D(tex, texcoord);
	res.rgb = lerp(prepass.rgb, texenvcol.rgb, res.rgb);
	res.a = res.a * prepass.a;
	return res;
}
)";
