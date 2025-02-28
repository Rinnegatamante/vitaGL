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
	
	res.rgb = (%s) * pass%d_scale.x;
	res.a = (%s) * pass%d_scale.y;
	
	return clamp(res, 0.0f, 1.0f);
}
)";
