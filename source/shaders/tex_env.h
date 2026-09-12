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
	"%s - %s", // GL_SUBTRACT
	"%s * %s", // COMBINE (placeholder: never used as a function, keeps the indices aligned)
	// GL_DOT3_RGB / GL_DOT3_RGBA: dot product with a -0.5 bias and a scale of 4.
	// The scalar result is broadcast to every channel (Cg does this implicitly).
	// ES 1.1 era Android games use this to pull an alpha mask out of an ETC1
	// atlas, since ETC1 carries no alpha channel of its own.
	"4.0f * dot((%s) - 0.5f, (%s) - 0.5f)", // GL_DOT3_RGB
	"4.0f * dot((%s) - 0.5f, (%s) - 0.5f)"  // GL_DOT3_RGBA
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

// GL_DOT3_RGBA writes the dot product to alpha as well as to RGB, so the alpha
// line does not take a function of its own the way the generic template does.
const char *combine_dot3a_src =
R"(float4 texenv5%d(sampler2D tex, float2 texcoord, float4 prepass, float4 fragcol, float4 texenvcol) {
	float4 texcol = tex2D(tex, texcoord);
	float4 res;
	float d3 = (%s) * %cpass%d_scale.x;
	res.rgb = float3(d3, d3, d3);
	res.a = d3;
	
	return clamp(res, 0.0f, 1.0f);
}
)";
#endif
