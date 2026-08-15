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

/*
 * GL_DECAL
 */
const char *decal_src =
R"(float4 texenv1(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	float4 res = tex2D(tex, texcoord);
	res.rgb = lerp(prepass.rgb, res.rgb, res.a);
	res.a = prepass.a;
	return res;
}
)";

/*
 * GL_MODULATE
 */
const char *modulate_src =
R"(float4 texenv0(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	return tex2D(tex, texcoord) * prepass;
}
)";

/*
 * GL_REPLACE
 */
const char *replace_src =
R"(float4 texenv4(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	return tex2D(tex, texcoord);
}
)";

#ifndef DISABLE_TEXTURE_COMBINER
/*
 * GL_COMBINE
 */
const char *calc_funcs[] = {
	"%s * %s", // GL_MODULATE
	"%s + %s - 0.5f", // GL_ADD_SIGNED
	"(%s * %s + %s * (1 - %s))", // GL_INTERPOLATE
	"%s + %s", // GL_ADD
	"%s", // GL_REPLACE
	"%s - %s" // GL_SUBTRACT
};

const char *operands[] = {
	"texcol", // GL_TEXTURE
	"texenvcol", // GL_CONSTANT
	"fragcol", // GL_PRIMARY_COLOR
	"prepass" // GL_PREVIOUS
};

const char *op_modes[] = {
	"%s.rgb", // GL_SRC_COLOR
	"(1 - %s.rgb)", // GL_ONE_MINUS_SRC_COLOR
	"%s.a", // GL_SRC_ALPHA
	"(1 - %s.a)" // GL_ONE_MINUS_SRC_ALPHA
};

const char *combine_src =
R"(float4 texenv5%d(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	float4 texcol = tex2D(tex, texcoord);
	float4 res;
	
	res.rgb = (%s) * %cpass%d_scale.x;
	res.a = (%s) * %cpass%d_scale.y;
	
	return clamp(res, 0.0f, 1.0f);
}
)";
#endif
