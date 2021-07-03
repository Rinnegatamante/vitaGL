/*
 * GL_ADD
 */
const char *add_src =
R"(float4 texenv3(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	float4 res = tex2D(tex, texcoord);
	res.rgb = clamp(res.rgb + prepass.rgb, 0.0, 1.0);
	res.a = res.a * prepass.a;
	return res;
}
)";
