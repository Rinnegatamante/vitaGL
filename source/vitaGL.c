/*
 *
 */
#include "shared.h"
#include "vitaGL.h"
#include "texture_callbacks.h"

// Shaders
#include "shaders/clear_f.h"
#include "shaders/clear_v.h"
#include "shaders/rgba_f.h"
#include "shaders/rgba_v.h"
#include "shaders/rgb_v.h"
#include "shaders/disable_color_buffer_f.h"
#include "shaders/disable_color_buffer_v.h"
#include "shaders/texture2d_f.h"
#include "shaders/texture2d_v.h"
#include "shaders/texture2d_rgba_f.h"
#include "shaders/texture2d_rgba_v.h"

typedef struct gpubuffer{
	void* ptr;
} gpubuffer;

// Non native primitives implemented
typedef enum SceGxmPrimitiveTypeExtra{
	SCE_GXM_PRIMITIVE_NONE = 0,
	SCE_GXM_PRIMITIVE_QUADS = 1
} SceGxmPrimitiveTypeExtra;

// Vertex list structs
typedef struct vertexList{
	vector3f v;
	void* next;
} vertexList;

typedef struct rgbaList{
	vector4f v;
	void* next;
} rgbaList;

typedef struct uvList{
	vector2f v;
	void* next;
} uvList;

// Texture environment mode
typedef enum texEnvMode{
	MODULATE = 0,
	DECAL = 1,
	BLEND = 2,
	REPLACE = 3
} texEnvMode;

// sceGxm viewport setup (NOTE: origin is on center screen)
float x_port = 480.0f;
float y_port = -272.0f;
float z_port = 0.5f;
float x_scale = 480.0f;
float y_scale = 272.0f;
float z_scale = 0.5f;

uint8_t viewport_mode = 0; // Current setting for viewport mode
GLboolean vblank = GL_TRUE; // Current setting for VSync

static const SceGxmProgram *const gxm_program_disable_color_buffer_v = (SceGxmProgram*)&disable_color_buffer_v;
static const SceGxmProgram *const gxm_program_disable_color_buffer_f = (SceGxmProgram*)&disable_color_buffer_f;
static const SceGxmProgram *const gxm_program_clear_v = (SceGxmProgram*)&clear_v;
static const SceGxmProgram *const gxm_program_clear_f = (SceGxmProgram*)&clear_f;
static const SceGxmProgram *const gxm_program_rgba_v = (SceGxmProgram*)&rgba_v;
static const SceGxmProgram *const gxm_program_rgba_f = (SceGxmProgram*)&rgba_f;
static const SceGxmProgram *const gxm_program_rgb_v = (SceGxmProgram*)&rgb_v;
static const SceGxmProgram *const gxm_program_texture2d_v = (SceGxmProgram*)&texture2d_v;
static const SceGxmProgram *const gxm_program_texture2d_f = (SceGxmProgram*)&texture2d_f;
static const SceGxmProgram *const gxm_program_texture2d_rgba_v = (SceGxmProgram*)&texture2d_rgba_v;
static const SceGxmProgram *const gxm_program_texture2d_rgba_f = (SceGxmProgram*)&texture2d_rgba_f;

// Disable color buffer shader
static SceGxmShaderPatcherId disable_color_buffer_vertex_id;
static SceGxmShaderPatcherId disable_color_buffer_fragment_id;
static const SceGxmProgramParameter* disable_color_buffer_position;
static const SceGxmProgramParameter* disable_color_buffer_matrix;
static SceGxmVertexProgram* disable_color_buffer_vertex_program_patched;
static SceGxmFragmentProgram* disable_color_buffer_fragment_program_patched;
static position_vertex* depth_vertices = NULL;
uint16_t* depth_clear_indices = NULL; // Memblock starting address for clear screen indices

// Clear shader
static SceGxmShaderPatcherId clear_vertex_id;
static SceGxmShaderPatcherId clear_fragment_id;
static const SceGxmProgramParameter* clear_position;
static const SceGxmProgramParameter* clear_color;
SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
static SceGxmFragmentProgram* clear_fragment_program_patched;
clear_vertex *clear_vertices = NULL; // Memblock starting address for clear screen vertices

// Color (RGBA/RGB) shader
static SceGxmShaderPatcherId rgba_vertex_id;
static SceGxmShaderPatcherId rgb_vertex_id;
static SceGxmShaderPatcherId rgba_fragment_id;
static const SceGxmProgramParameter* rgba_position;
static const SceGxmProgramParameter* rgba_color;
static const SceGxmProgramParameter* rgba_wvp;
static const SceGxmProgramParameter* rgb_position;
static const SceGxmProgramParameter* rgb_color;
static const SceGxmProgramParameter* rgb_wvp;
static SceGxmVertexProgram* rgba_vertex_program_patched;
static SceGxmVertexProgram* rgb_vertex_program_patched;
static SceGxmFragmentProgram* rgba_fragment_program_patched;
static const SceGxmProgram* rgba_fragment_program;

// Texture2D shader
static SceGxmShaderPatcherId texture2d_vertex_id;
static SceGxmShaderPatcherId texture2d_fragment_id;
static const SceGxmProgramParameter* texture2d_position;
static const SceGxmProgramParameter* texture2d_texcoord;
static const SceGxmProgramParameter* texture2d_wvp;
static const SceGxmProgramParameter* texture2d_alpha_cut;
static const SceGxmProgramParameter* texture2d_alpha_op;
static const SceGxmProgramParameter* texture2d_tint_color;
static const SceGxmProgramParameter* texture2d_tex_env;
static const SceGxmProgramParameter* texture2d_tex_env_color;
static SceGxmVertexProgram* texture2d_vertex_program_patched;
static SceGxmFragmentProgram* texture2d_fragment_program_patched;
static const SceGxmProgram* texture2d_fragment_program;

// Texture2D+RGBA shader
static SceGxmShaderPatcherId texture2d_rgba_vertex_id;
static SceGxmShaderPatcherId texture2d_rgba_fragment_id;
static const SceGxmProgramParameter* texture2d_rgba_position;
static const SceGxmProgramParameter* texture2d_rgba_texcoord;
static const SceGxmProgramParameter* texture2d_rgba_wvp;
static const SceGxmProgramParameter* texture2d_rgba_alpha_cut;
static const SceGxmProgramParameter* texture2d_rgba_alpha_op;
static const SceGxmProgramParameter* texture2d_rgba_color;
static const SceGxmProgramParameter* texture2d_rgba_tex_env;
static const SceGxmProgramParameter* texture2d_rgba_tex_env_color;
static SceGxmVertexProgram* texture2d_rgba_vertex_program_patched;
static SceGxmFragmentProgram* texture2d_rgba_fragment_program_patched;
static const SceGxmProgram* texture2d_rgba_fragment_program;

// Internal stuffs
static SceGxmPrimitiveType prim;
static SceGxmPrimitiveTypeExtra prim_extra = SCE_GXM_PRIMITIVE_NONE;
static vertexList* model_vertices = NULL;
static vertexList* last = NULL;
static rgbaList* model_color = NULL;
static rgbaList* last2 = NULL;
static uvList* model_uv = NULL;
static uvList* last3 = NULL;
static uint64_t vertex_count = 0;
static uint8_t drawing = 0;
static uint8_t np = 0xFF;

static SceGxmBlendInfo* cur_blend_info_ptr = NULL;
static int max_texture_unit = 0;
extern uint8_t use_vram;

static GLuint buffers[BUFFERS_NUM]; // Buffers array
static gpubuffer gpu_buffers[BUFFERS_NUM]; // Buffers array
static SceGxmColorMask blend_color_mask = SCE_GXM_COLOR_MASK_ALL; // Current in-use color mask (glColorMask)
static SceGxmBlendFunc blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD; // Current in-use RGB blend func
static SceGxmBlendFunc blend_func_a = SCE_GXM_BLEND_FUNC_ADD; // Current in-use A blend func
static SceGxmPolygonMode polygon_mode_front = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL; // Current in-use polygon mode for front
static SceGxmPolygonMode polygon_mode_back = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL; // Current in-use polygon mode for back
static int vertex_array_unit = -1; // Current in-use vertex array unit
static int index_array_unit = -1; // Current in-use index array unit
static vector4f clear_rgba_val; // Current clear color for glClear
static GLboolean vertex_array_state = GL_FALSE; // Current state for GL_VERTEX_ARRAY
static GLboolean color_array_state = GL_FALSE; // Current state for GL_COLOR_ARRAY
static GLboolean texture_array_state = GL_FALSE; // Current state for GL_TEXTURE_COORD_ARRAY
static GLboolean cull_face_state = GL_FALSE; // Current state for GL_CULL_FACE
static GLboolean pol_offset_fill = GL_FALSE; // Current state for GL_POLYGON_OFFSET_FILL
static GLboolean pol_offset_line = GL_FALSE; // Current state for GL_POLYGON_OFFSET_LINE
static GLboolean pol_offset_point = GL_FALSE; // Current state for GL_POLYGON_OFFSET_POINT
static GLenum gl_cull_mode = GL_BACK; // Current in-use openGL cull mode
static GLenum gl_front_face = GL_CCW; // Current in-use openGL cull mode
static GLboolean no_polygons_mode = GL_FALSE; // GL_TRUE when cull mode to GL_FRONT_AND_BACK is set
static vector4f current_color = {1.0f, 1.0f, 1.0f, 1.0f}; // Current in-use color
static vector4f texenv_color = {0.0f, 0.0f, 0.0f, 0.0f}; // Current in-use texture environment color

// Internal functions

#ifdef ENABLE_LOG
void LOG(const char *format, ...) {
	__gnuc_va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsprintf(msg, format, arg);
	va_end(arg);
	int i;
	sprintf(msg, "%s\n", msg);
	FILE* log = fopen("ux0:/data/vitaGL.log", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
}
#endif

static void _change_blend_factor(SceGxmBlendInfo* blend_info){
	changeCustomShadersBlend(blend_info);
	
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		rgba_fragment_id,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		blend_info,
		rgba_fragment_program,
		&rgba_fragment_program_patched);
		
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		texture2d_fragment_id,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		blend_info,
		texture2d_fragment_program,
		&texture2d_fragment_program_patched);
		
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		texture2d_rgba_fragment_id,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		blend_info,
		texture2d_rgba_fragment_program,
		&texture2d_rgba_fragment_program_patched);

}

static void change_blend_factor(){
	static SceGxmBlendInfo blend_info;
	blend_info.colorMask = blend_color_mask;
	blend_info.colorFunc = blend_func_rgb;
	blend_info.alphaFunc = blend_func_a;
	blend_info.colorSrc = blend_sfactor_rgb;
	blend_info.colorDst = blend_dfactor_rgb;
	blend_info.alphaSrc = blend_sfactor_a;
	blend_info.alphaDst = blend_dfactor_a;
	
	_change_blend_factor(&blend_info);
	cur_blend_info_ptr = &blend_info;
	if (cur_program != 0){
		reloadCustomShader();
	}
}

static void change_blend_mask(){
	static SceGxmBlendInfo blend_info;
	blend_info.colorMask = blend_color_mask;
	blend_info.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
	blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
	blend_info.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
	blend_info.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_info.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
	blend_info.alphaDst = SCE_GXM_BLEND_FACTOR_ZERO;
	
	_change_blend_factor(&blend_info);
	cur_blend_info_ptr = &blend_info;
	if (cur_program != 0){
		reloadCustomShader();
	}
}

static void disable_blend(){
	if (blend_color_mask == SCE_GXM_COLOR_MASK_ALL){
		_change_blend_factor(NULL);
		cur_blend_info_ptr = NULL;
		if (cur_program != 0){
			reloadCustomShader();
		}
	}else change_blend_mask();
}

static void purge_vertex_list(){
	vertexList* old;
	rgbaList* old2;
	uvList* old3;
	while (model_vertices != NULL){
		old = model_vertices;
		model_vertices = model_vertices->next;
		free(old);
	}
	while (model_color != NULL){
		old2 = model_color;
		model_color = model_color->next;
		free(old2);
	}
	while (model_uv != NULL){
		old3 = model_uv;
		model_uv = model_uv->next;
		free(old3);
	}
}

static void change_cull_mode(){
	if (cull_face_state){
		if ((gl_front_face == GL_CW) && (gl_cull_mode == GL_BACK)) sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CCW);
		if ((gl_front_face == GL_CCW) && (gl_cull_mode == GL_BACK)) sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CW);
		if ((gl_front_face == GL_CCW) && (gl_cull_mode == GL_FRONT)) sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CCW);
		if ((gl_front_face == GL_CW) && (gl_cull_mode == GL_FRONT)) sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CW);
		if (gl_cull_mode == GL_FRONT_AND_BACK) no_polygons_mode = GL_TRUE;
	}else sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_NONE);
}

static void update_polygon_offset(){
	switch (polygon_mode_front){
		case SCE_GXM_POLYGON_MODE_TRIANGLE_LINE:
			if (pol_offset_line) sceGxmSetFrontDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
			else sceGxmSetFrontDepthBias(gxm_context, 0, 0);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_POINT:
			if (pol_offset_point) sceGxmSetFrontDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
			else sceGxmSetFrontDepthBias(gxm_context, 0, 0);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_FILL:
			if (pol_offset_fill) sceGxmSetFrontDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
			else sceGxmSetFrontDepthBias(gxm_context, 0, 0);
			break;
	}
	switch (polygon_mode_back){
		case SCE_GXM_POLYGON_MODE_TRIANGLE_LINE:
			if (pol_offset_line) sceGxmSetBackDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
			else sceGxmSetBackDepthBias(gxm_context, 0, 0);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_POINT:
			if (pol_offset_point) sceGxmSetBackDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
			else sceGxmSetBackDepthBias(gxm_context, 0, 0);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_FILL:
			if (pol_offset_fill) sceGxmSetBackDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
			else sceGxmSetBackDepthBias(gxm_context, 0, 0);
			break;
	}	
}

// vitaGL specific functions

void vglUseVram(GLboolean usage){
	use_vram = usage;
}

void vglInitExtended(uint32_t gpu_pool_size, int width, int height){
	
	// Setting our display size
	DISPLAY_WIDTH = width;
	DISPLAY_HEIGHT = height;
	DISPLAY_WIDTH_FLOAT = width * 1.0f;
	DISPLAY_HEIGHT_FLOAT = height * 1.0f;
	switch (DISPLAY_WIDTH){
	case 480:
		DISPLAY_STRIDE = 512;
		break;
	case 640:
		DISPLAY_STRIDE = 640;
		break;
	case 720:
		DISPLAY_STRIDE = 768;
		break;
	default:
		DISPLAY_STRIDE = 960;
		break;
	}
	
	// Initializing sceGxm
	initGxm();
	
	// Getting max allocatable CDRAM and RAM memory
	SceKernelFreeMemorySizeInfo info;
    info.size = sizeof(SceKernelFreeMemorySizeInfo);
	sceKernelGetFreeMemorySize(&info);
	
	// Initializing memory heap for CDRAM and RAM memory
	mem_init(info.size_user - 16 * 1024 * 1024, info.size_cdram - 1 * 1024 * 1024); // leave some just in case
	
	// Initializing sceGxm context
	initGxmContext();

	// Creating render target for the display
	createDisplayRenderTarget();
	
	// Creating color surfaces for the display
	initDisplayColorSurfaces();
	
	// Creating depth and stencil surfaces for the display
	initDepthStencilSurfaces();
	
	// Starting a sceGxmShaderPatcher instance
	startShaderPatcher();
	
	// Disable color buffer shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_disable_color_buffer_v,
		&disable_color_buffer_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_disable_color_buffer_f,
		&disable_color_buffer_fragment_id);

	const SceGxmProgram* disable_color_buffer_vertex_program = sceGxmShaderPatcherGetProgramFromId(disable_color_buffer_vertex_id);
	const SceGxmProgram* disable_color_buffer_fragment_program = sceGxmShaderPatcherGetProgramFromId(disable_color_buffer_fragment_id);

	disable_color_buffer_position = sceGxmProgramFindParameterByName(
		disable_color_buffer_vertex_program, "position");

	disable_color_buffer_matrix = sceGxmProgramFindParameterByName(
		disable_color_buffer_vertex_program, "u_mvp_matrix");
		
	SceGxmVertexAttribute disable_color_buffer_attributes;
	SceGxmVertexStream disable_color_buffer_stream;
	disable_color_buffer_attributes.streamIndex = 0;
	disable_color_buffer_attributes.offset = 0;
	disable_color_buffer_attributes.format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	disable_color_buffer_attributes.componentCount = 3;
	disable_color_buffer_attributes.regIndex = sceGxmProgramParameterGetResourceIndex(
		disable_color_buffer_position);
	disable_color_buffer_stream.stride = sizeof(struct position_vertex);
	disable_color_buffer_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	
	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		disable_color_buffer_vertex_id, &disable_color_buffer_attributes,
		1, &disable_color_buffer_stream, 1, &disable_color_buffer_vertex_program_patched);

	SceGxmBlendInfo disable_color_buffer_blend_info;
	memset(&disable_color_buffer_blend_info, 0, sizeof(SceGxmBlendInfo));
	disable_color_buffer_blend_info.colorMask = SCE_GXM_COLOR_MASK_NONE;
	disable_color_buffer_blend_info.colorFunc = SCE_GXM_BLEND_FUNC_NONE;
	disable_color_buffer_blend_info.alphaFunc = SCE_GXM_BLEND_FUNC_NONE;
	disable_color_buffer_blend_info.colorSrc = SCE_GXM_BLEND_FACTOR_ZERO;
	disable_color_buffer_blend_info.colorDst = SCE_GXM_BLEND_FACTOR_ZERO;
	disable_color_buffer_blend_info.alphaSrc = SCE_GXM_BLEND_FACTOR_ZERO;
	disable_color_buffer_blend_info.alphaDst = SCE_GXM_BLEND_FACTOR_ZERO;

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		disable_color_buffer_fragment_id,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		&disable_color_buffer_blend_info,
		disable_color_buffer_fragment_program,
		&disable_color_buffer_fragment_program_patched);
		
	depth_vertices = gpu_alloc_mapped(4 * sizeof(struct position_vertex), VGL_MEM_RAM);

	depth_clear_indices = gpu_alloc_mapped(4 * sizeof(unsigned short), VGL_MEM_RAM);

	depth_vertices[0].position = (vector3f){-1.0f, -1.0f, 1.0f};
	depth_vertices[1].position = (vector3f){ 1.0f, -1.0f, 1.0f};
	depth_vertices[2].position = (vector3f){-1.0f,  1.0f, 1.0f};
	depth_vertices[3].position = (vector3f){ 1.0f,  1.0f, 1.0f};

	depth_clear_indices[0] = 0;
	depth_clear_indices[1] = 1;
	depth_clear_indices[2] = 2;
	depth_clear_indices[3] = 3;
	
	// Clear shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_clear_v,
		&clear_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_clear_f,
		&clear_fragment_id);

	const SceGxmProgram* clear_vertex_program = sceGxmShaderPatcherGetProgramFromId(clear_vertex_id);
	const SceGxmProgram* clear_fragment_program = sceGxmShaderPatcherGetProgramFromId(clear_fragment_id);

	clear_position = sceGxmProgramFindParameterByName(
		clear_vertex_program, "position");

	clear_color = sceGxmProgramFindParameterByName(
		clear_fragment_program, "u_clear_color");

	SceGxmVertexAttribute clear_vertex_attribute;
	SceGxmVertexStream clear_vertex_stream;
	clear_vertex_attribute.streamIndex = 0;
	clear_vertex_attribute.offset = 0;
	clear_vertex_attribute.format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	clear_vertex_attribute.componentCount = 2;
	clear_vertex_attribute.regIndex = sceGxmProgramParameterGetResourceIndex(
		clear_position);
	clear_vertex_stream.stride = sizeof(struct clear_vertex);
	clear_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		clear_vertex_id, &clear_vertex_attribute,
		1, &clear_vertex_stream, 1, &clear_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		clear_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, clear_fragment_program,
		&clear_fragment_program_patched);

	clear_vertices = gpu_alloc_mapped(4 * sizeof(struct clear_vertex), VGL_MEM_RAM);

	clear_vertices[0].position = (vector2f){-1.0f, -1.0f};
	clear_vertices[1].position = (vector2f){ 1.0f, -1.0f};
	clear_vertices[2].position = (vector2f){ 1.0f,  1.0f};
	clear_vertices[3].position = (vector2f){-1.0f,  1.0f};
	
	// Color shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_rgba_v,
		&rgba_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_rgb_v,
		&rgb_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_rgba_f,
		&rgba_fragment_id);

	const SceGxmProgram* rgba_vertex_program = sceGxmShaderPatcherGetProgramFromId(rgba_vertex_id);
	const SceGxmProgram* rgb_vertex_program = sceGxmShaderPatcherGetProgramFromId(rgb_vertex_id);
	rgba_fragment_program = sceGxmShaderPatcherGetProgramFromId(rgba_fragment_id);

	rgba_position = sceGxmProgramFindParameterByName(
		rgba_vertex_program, "aPosition");

	rgba_color = sceGxmProgramFindParameterByName(
		rgba_vertex_program, "aColor");
		
	rgb_position = sceGxmProgramFindParameterByName(
		rgba_vertex_program, "aPosition");

	rgb_color = sceGxmProgramFindParameterByName(
		rgba_vertex_program, "aColor");

	SceGxmVertexAttribute rgba_vertex_attribute[2];
	SceGxmVertexStream rgba_vertex_stream[2];
	rgba_vertex_attribute[0].streamIndex = 0;
	rgba_vertex_attribute[0].offset = 0;
	rgba_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	rgba_vertex_attribute[0].componentCount = 3;
	rgba_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		rgba_position);
	rgba_vertex_attribute[1].streamIndex = 1;
	rgba_vertex_attribute[1].offset = 0;
	rgba_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	rgba_vertex_attribute[1].componentCount = 4;
	rgba_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		rgba_color);
	rgba_vertex_stream[0].stride = sizeof(vector3f);
	rgba_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	rgba_vertex_stream[1].stride = sizeof(vector4f);
	rgba_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		rgba_vertex_id, rgba_vertex_attribute,
		2, rgba_vertex_stream, 2, &rgba_vertex_program_patched);
		
	SceGxmVertexAttribute rgb_vertex_attribute[2];
	SceGxmVertexStream rgb_vertex_stream[2];
	rgb_vertex_attribute[0].streamIndex = 0;
	rgb_vertex_attribute[0].offset = 0;
	rgb_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	rgb_vertex_attribute[0].componentCount = 3;
	rgb_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		rgb_position);
	rgb_vertex_attribute[1].streamIndex = 1;
	rgb_vertex_attribute[1].offset = 0;
	rgb_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	rgb_vertex_attribute[1].componentCount = 3;
	rgb_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		rgb_color);
	rgb_vertex_stream[0].stride = sizeof(vector3f);
	rgb_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	rgb_vertex_stream[1].stride = sizeof(vector3f);
	rgb_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		rgb_vertex_id, rgb_vertex_attribute,
		2, rgb_vertex_stream, 2, &rgb_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		rgba_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, rgba_fragment_program,
		&rgba_fragment_program_patched);
		
	rgba_wvp = sceGxmProgramFindParameterByName(rgba_vertex_program, "wvp");
	rgb_wvp = sceGxmProgramFindParameterByName(rgb_vertex_program, "wvp");
		
	// Texture2D shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_v,
		&texture2d_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_f,
		&texture2d_fragment_id);

	const SceGxmProgram* texture2d_vertex_program = sceGxmShaderPatcherGetProgramFromId(texture2d_vertex_id);
	texture2d_fragment_program = sceGxmShaderPatcherGetProgramFromId(texture2d_fragment_id);
	
	texture2d_position = sceGxmProgramFindParameterByName(
		texture2d_vertex_program, "position");

	texture2d_texcoord = sceGxmProgramFindParameterByName(
		texture2d_vertex_program, "texcoord");
		
	texture2d_alpha_cut = sceGxmProgramFindParameterByName(
		texture2d_fragment_program, "alphaCut");
		
	texture2d_alpha_op = sceGxmProgramFindParameterByName(
		texture2d_fragment_program, "alphaOp");
		
	texture2d_tint_color = sceGxmProgramFindParameterByName(
		texture2d_fragment_program, "tintColor");
		
	texture2d_tex_env = sceGxmProgramFindParameterByName(
		texture2d_fragment_program, "texEnv");
		
	texture2d_tex_env_color = sceGxmProgramFindParameterByName(
		texture2d_fragment_program, "texEnvColor");

	SceGxmVertexAttribute texture2d_vertex_attribute[2];
	SceGxmVertexStream texture2d_vertex_stream[2];
	texture2d_vertex_attribute[0].streamIndex = 0;
	texture2d_vertex_attribute[0].offset = 0;
	texture2d_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_vertex_attribute[0].componentCount = 3;
	texture2d_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_position);
	texture2d_vertex_attribute[1].streamIndex = 1;
	texture2d_vertex_attribute[1].offset = 0;
	texture2d_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_vertex_attribute[1].componentCount = 2;
	texture2d_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_texcoord);
	texture2d_vertex_stream[0].stride = sizeof(vector3f);
	texture2d_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	texture2d_vertex_stream[1].stride = sizeof(vector2f);
	texture2d_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	
	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		texture2d_vertex_id, texture2d_vertex_attribute,
		2, texture2d_vertex_stream, 2, &texture2d_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		texture2d_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, texture2d_fragment_program,
		&texture2d_fragment_program_patched);
		
	texture2d_wvp = sceGxmProgramFindParameterByName(texture2d_vertex_program, "wvp");	
	
	// Texture2D+RGBA shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_rgba_v,
		&texture2d_rgba_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_rgba_f,
		&texture2d_rgba_fragment_id);

	const SceGxmProgram* texture2d_rgba_vertex_program = sceGxmShaderPatcherGetProgramFromId(texture2d_rgba_vertex_id);
	texture2d_rgba_fragment_program = sceGxmShaderPatcherGetProgramFromId(texture2d_rgba_fragment_id);
	
	texture2d_rgba_position = sceGxmProgramFindParameterByName(
		texture2d_rgba_vertex_program, "position");

	texture2d_rgba_texcoord = sceGxmProgramFindParameterByName(
		texture2d_rgba_vertex_program, "texcoord");
		
	texture2d_rgba_alpha_cut = sceGxmProgramFindParameterByName(
		texture2d_rgba_fragment_program, "alphaCut");
		
	texture2d_rgba_alpha_op = sceGxmProgramFindParameterByName(
		texture2d_rgba_fragment_program, "alphaOp");
		
	texture2d_rgba_color = sceGxmProgramFindParameterByName(
		texture2d_rgba_vertex_program, "color");
		
	texture2d_rgba_tex_env = sceGxmProgramFindParameterByName(
		texture2d_rgba_fragment_program, "texEnv");
		
	texture2d_rgba_tex_env_color = sceGxmProgramFindParameterByName(
		texture2d_rgba_fragment_program, "texEnvColor");

	SceGxmVertexAttribute texture2d_rgba_vertex_attribute[3];
	SceGxmVertexStream texture2d_rgba_vertex_stream[3];
	texture2d_rgba_vertex_attribute[0].streamIndex = 0;
	texture2d_rgba_vertex_attribute[0].offset = 0;
	texture2d_rgba_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_rgba_vertex_attribute[0].componentCount = 3;
	texture2d_rgba_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_rgba_position);
	texture2d_rgba_vertex_attribute[1].streamIndex = 1;
	texture2d_rgba_vertex_attribute[1].offset = 0;
	texture2d_rgba_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_rgba_vertex_attribute[1].componentCount = 2;
	texture2d_rgba_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_rgba_texcoord);
	texture2d_rgba_vertex_attribute[2].streamIndex = 2;
	texture2d_rgba_vertex_attribute[2].offset = 0;
	texture2d_rgba_vertex_attribute[2].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_rgba_vertex_attribute[2].componentCount = 4;
	texture2d_rgba_vertex_attribute[2].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_rgba_color);
	texture2d_rgba_vertex_stream[0].stride = sizeof(vector3f);
	texture2d_rgba_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	texture2d_rgba_vertex_stream[1].stride = sizeof(vector2f);
	texture2d_rgba_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	texture2d_rgba_vertex_stream[2].stride = sizeof(vector4f);
	texture2d_rgba_vertex_stream[2].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	
	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		texture2d_rgba_vertex_id, texture2d_rgba_vertex_attribute,
		3, texture2d_rgba_vertex_stream, 3, &texture2d_rgba_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		texture2d_rgba_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, texture2d_rgba_fragment_program,
		&texture2d_rgba_fragment_program_patched);
		
	texture2d_rgba_wvp = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "wvp");	
	
	sceGxmSetTwoSidedEnable(gxm_context, SCE_GXM_TWO_SIDED_ENABLED);
	
	// Scissor Test shader register
	sceGxmShaderPatcherCreateMaskUpdateFragmentProgram(gxm_shader_patcher, &scissor_test_fragment_program);
	
	scissor_test_vertices = gpu_alloc_mapped(4 * sizeof(struct clear_vertex), VGL_MEM_RAM);
	
	// Allocate temp pool for non-VBO drawing
	gpu_pool_init(gpu_pool_size);
	
	// Init texture units
	int i, j;
	for (i=0; i < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++){
		for (j=0; j < TEXTURES_NUM; j++){
			texture_units[i].textures[j].used = 0;
			texture_units[i].textures[j].valid = 0;
		}
		texture_units[i].env_mode = MODULATE;
		texture_units[i].tex_id = 0;
		texture_units[i].enabled = 0;
		texture_units[i].min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
		texture_units[i].mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
		texture_units[i].u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
		texture_units[i].v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
	}
	
	// Init custom shaders
	resetCustomShaders();
	
	// Init buffers
	for (i=0; i < BUFFERS_NUM; i++){
		buffers[i] = BUFFERS_ADDR + i;
		gpu_buffers[i].ptr = NULL;
	}
	
	// Init scissor test state
	resetScissorTestRegion();
	
}

void vglInit(uint32_t gpu_pool_size){
	vglInitExtended(gpu_pool_size, DISPLAY_WIDTH_DEF, DISPLAY_HEIGHT_DEF);
}

void vglEnd(void){
	
	// Wait for rendering to be finished
	waitRenderingDone();
	
	// Deallocating default vertices buffers
	gpu_free_mapped(clear_vertices, VGL_MEM_RAM);
	gpu_free_mapped(depth_vertices, VGL_MEM_RAM);
	gpu_free_mapped(depth_clear_indices, VGL_MEM_RAM);
	gpu_free_mapped(scissor_test_vertices, VGL_MEM_RAM);
	
	// Releasing shader programs from sceGxmShaderPatcher
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, scissor_test_fragment_program);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, disable_color_buffer_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, disable_color_buffer_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, clear_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, clear_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, rgba_vertex_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, rgb_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, rgba_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, texture2d_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_program_patched);
	
	// Unregistering shader programs from sceGxmShaderPatcher
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgb_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgba_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgba_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, texture2d_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, texture2d_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, disable_color_buffer_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, disable_color_buffer_fragment_id);
	
	// Terminating shader patcher
	stopShaderPatcher();
	
	// Deallocating depth and stencil surfaces for display
	termDepthStencilSurfaces();
	
	// Terminating display's color surfaces
	termDisplayColorSurfaces();
	
	// Destroing display's render target
	destroyDisplayRenderTarget();
	
	// Terminating sceGxm context
	termGxmContext();
	
	// Terminating sceGxm
	sceGxmTerminate();
	
}

void vglWaitVblankStart(GLboolean enable){
	vblank = enable;
}

// openGL implementation

GLenum glGetError(void){
	GLenum ret = error;
	error = GL_NO_ERROR;
	return ret;
}

void glClear(GLbitfield mask){
	GLenum orig_depth_test = depth_test_state;
	if ((mask & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT){
		change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
		invalidate_depth_test();
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
		sceGxmSetVertexProgram(gxm_context, clear_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, clear_fragment_program_patched);
		void *color_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &color_buffer);
		sceGxmSetUniformDataF(color_buffer, clear_color, 0, 4, &clear_rgba_val.r);
		sceGxmSetVertexStream(gxm_context, 0, clear_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);
		change_depth_write(depth_mask_state && orig_depth_test ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		validate_depth_test();
		sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front);
		sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back);
		drawing = 1;
	}
	if ((mask & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT){
		change_depth_write(SCE_GXM_DEPTH_WRITE_ENABLED);
		invalidate_depth_test();
		depth_vertices[0].position.z = depth_value;
		depth_vertices[1].position.z = depth_value;
		depth_vertices[2].position.z = depth_value;
		depth_vertices[3].position.z = depth_value;
		sceGxmSetVertexProgram(gxm_context, disable_color_buffer_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, disable_color_buffer_fragment_program_patched);
		sceGxmSetVertexStream(gxm_context, 0, depth_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);
		change_depth_write(depth_mask_state && orig_depth_test ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		validate_depth_test();
	}
	if ((mask & GL_STENCIL_BUFFER_BIT) == GL_STENCIL_BUFFER_BIT){
		change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
		invalidate_depth_test();
		sceGxmSetVertexProgram(gxm_context, disable_color_buffer_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, disable_color_buffer_fragment_program_patched);
		sceGxmSetFrontStencilFunc(gxm_context,
			SCE_GXM_STENCIL_FUNC_NEVER,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			0, stencil_value);
		sceGxmSetBackStencilFunc(gxm_context,
			SCE_GXM_STENCIL_FUNC_NEVER,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			0, stencil_value);
		sceGxmSetVertexStream(gxm_context, 0, clear_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);
		change_depth_write(depth_mask_state && orig_depth_test ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		validate_depth_test();
		change_stencil_settings();
	}
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){
	clear_rgba_val.r = red;
	clear_rgba_val.g = green;
	clear_rgba_val.b = blue;
	clear_rgba_val.a = alpha;
}

void glEnable(GLenum cap){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	switch (cap){
		case GL_DEPTH_TEST:
			depth_test_state = GL_TRUE;
			change_depth_func();
			change_depth_write(depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
			break;
		case GL_STENCIL_TEST:
			change_stencil_settings();
			stencil_test_state = GL_TRUE;
			break;
		case GL_BLEND:
			if (!blend_state) change_blend_factor();
			blend_state = GL_TRUE;
			break;
		case GL_SCISSOR_TEST:
			scissor_test_state = GL_TRUE;
			update_scissor_test();
			break;
		case GL_CULL_FACE:
			cull_face_state = GL_TRUE;
			change_cull_mode();
			break;
		case GL_POLYGON_OFFSET_FILL:
			pol_offset_fill = GL_TRUE;
			update_polygon_offset();
			break;
		case GL_POLYGON_OFFSET_LINE:
			pol_offset_line = GL_TRUE;
			update_polygon_offset();
			break;
		case GL_POLYGON_OFFSET_POINT:
			pol_offset_point = GL_TRUE;
			update_polygon_offset();
			break;
		case GL_TEXTURE_2D:
			texture_units[server_texture_unit].enabled = GL_TRUE;
			if (server_texture_unit > max_texture_unit) max_texture_unit = server_texture_unit;
			break;
		case GL_ALPHA_TEST:
			alpha_test_state = GL_TRUE;
			update_alpha_test_settings();
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glDisable(GLenum cap){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	switch (cap){
		case GL_DEPTH_TEST:
			depth_test_state = GL_FALSE;
			change_depth_func();
			change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
			break;
		case GL_STENCIL_TEST:
			sceGxmSetFrontStencilFunc(gxm_context,
				SCE_GXM_STENCIL_FUNC_ALWAYS,
				SCE_GXM_STENCIL_OP_KEEP,
				SCE_GXM_STENCIL_OP_KEEP,
				SCE_GXM_STENCIL_OP_KEEP,
				0, 0);
			sceGxmSetBackStencilFunc(gxm_context,
				SCE_GXM_STENCIL_FUNC_ALWAYS,
				SCE_GXM_STENCIL_OP_KEEP,
				SCE_GXM_STENCIL_OP_KEEP,
				SCE_GXM_STENCIL_OP_KEEP,
				0, 0);
			stencil_test_state = GL_FALSE;
			break;
		case GL_BLEND:
			if (blend_state) disable_blend();
			blend_state = GL_FALSE;
			break;
		case GL_SCISSOR_TEST:
			scissor_test_state = GL_FALSE;
			update_scissor_test();
			break;
		case GL_CULL_FACE:
			cull_face_state = GL_FALSE;
			change_cull_mode();
			break;
		case GL_POLYGON_OFFSET_FILL:
			pol_offset_fill = GL_FALSE;
			update_polygon_offset();
			break;
		case GL_POLYGON_OFFSET_LINE:
			pol_offset_line = GL_FALSE;
			update_polygon_offset();
			break;
		case GL_POLYGON_OFFSET_POINT:
			pol_offset_point = GL_FALSE;
			update_polygon_offset();
			break;
		case GL_TEXTURE_2D:
			texture_units[server_texture_unit].enabled = GL_FALSE;
			if (server_texture_unit == max_texture_unit) max_texture_unit--;
			break;
		case GL_ALPHA_TEST:
			alpha_test_state = GL_FALSE;
			update_alpha_test_settings();
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glBegin(GLenum mode){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	phase = MODEL_CREATION;
	prim_extra = SCE_GXM_PRIMITIVE_NONE;
	switch (mode){
		case GL_POINTS:
			prim = SCE_GXM_PRIMITIVE_POINTS;
			np = 1;
			break;
		case GL_LINES:
			prim = SCE_GXM_PRIMITIVE_LINES;
			np = 2;
			break;
		case GL_TRIANGLES:
			prim = SCE_GXM_PRIMITIVE_TRIANGLES;
			np = 3;
			break;
		case GL_TRIANGLE_STRIP:
			prim = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
			np = 1;
			break;
		case GL_TRIANGLE_FAN:
			prim = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
			np = 1;
			break;
		case GL_QUADS:
			prim = SCE_GXM_PRIMITIVE_TRIANGLES;
			prim_extra = SCE_GXM_PRIMITIVE_QUADS;
			np = 4;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	vertex_count = 0;
}

void glEnd(void){
	if (vertex_count == 0 || ((vertex_count % np) != 0)) return;
	
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	phase = NONE;
	
	if (no_polygons_mode && ((prim == SCE_GXM_PRIMITIVE_TRIANGLES) || (prim >= SCE_GXM_PRIMITIVE_TRIANGLE_STRIP))){
		purge_vertex_list();
		vertex_count = 0;
		return;
	}
	
	texture_unit* tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	
	matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
	
	uint8_t use_texture = 0;
	if ((server_texture_unit >= 0) && (tex_unit->enabled) && (model_uv != NULL) && (tex_unit->textures[texture2d_idx].valid)){
		sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
		void* alpha_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
		sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
		float alpha_operation = (float)alpha_op;
		sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_operation);
		sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
		float tex_env = (float)tex_unit->env_mode;
		sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &tex_env);
		sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env_color, 0, 4, &texenv_color.r);
		use_texture = 1;
	}else{
		sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
	}
	
	int i, j;
	void* vertex_wvp_buffer;
	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
	if (use_texture){
		sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)mvp_matrix);
		sceGxmSetFragmentTexture(gxm_context, 0, &tex_unit->textures[texture2d_idx].gxm_tex);
		vector3f* vertices;
		vector2f* uv_map;
		uint16_t* indices;
		int n = 0, quad_n = 0;
		vertexList* object = model_vertices;
		uvList* object_uv = model_uv;
		uint64_t idx_count = vertex_count;
		switch (prim_extra){
			case SCE_GXM_PRIMITIVE_NONE:
				vertices = (vector3f*)gpu_pool_memalign(vertex_count * sizeof(vector3f), sizeof(vector3f));
				uv_map = (vector2f*)gpu_pool_memalign(vertex_count * sizeof(vector2f), sizeof(vector2f));
				memset(vertices, 0, (vertex_count * sizeof(vector3f)));
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				for (i=0; i < vertex_count; i++){
					memcpy(&vertices[n], &object->v, sizeof(vector3f));
					memcpy(&uv_map[n], &object_uv->v, sizeof(vector2f));
					indices[n] = n;
					object = object->next;
					object_uv = object_uv->next;
					n++;
				}
				break;
			case SCE_GXM_PRIMITIVE_QUADS:
				quad_n = vertex_count >> 2;
				idx_count = quad_n * 6;
				vertices = (vector3f*)gpu_pool_memalign(vertex_count * sizeof(vector3f), sizeof(vector3f));
				uv_map = (vector2f*)gpu_pool_memalign(vertex_count * sizeof(vector2f), sizeof(vector2f));
				memset(vertices, 0, (vertex_count * sizeof(vector3f)));
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				for (i=0; i < quad_n; i++){
					indices[i*6] = i*4;
					indices[i*6+1] = i*4+1;
					indices[i*6+2] = i*4+3;
					indices[i*6+3] = i*4+1;
					indices[i*6+4] = i*4+2;
					indices[i*6+5] = i*4+3;
				}
				for (j=0; j < vertex_count; j++){
					memcpy(&vertices[j], &object->v, sizeof(vector3f));
					memcpy(&uv_map[j], &object_uv->v, sizeof(vector2f));
					object = object->next;
					object_uv = object_uv->next;
				}
				break;
		}
		sceGxmSetVertexStream(gxm_context, 0, vertices);
		sceGxmSetVertexStream(gxm_context, 1, uv_map);
		sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
	}else{
		sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
		vector3f* vertices;
		vector4f* colors;
		uint16_t* indices;
		int n = 0, quad_n = 0;
		vertexList* object = model_vertices;
		rgbaList* object_clr = model_color;
		uint64_t idx_count = vertex_count;
	
		switch (prim_extra){
			case SCE_GXM_PRIMITIVE_NONE:
				vertices = (vector3f*)gpu_pool_memalign(vertex_count * sizeof(vector3f), sizeof(vector3f));
				colors = (vector4f*)gpu_pool_memalign(vertex_count * sizeof(vector4f), sizeof(vector4f));
				memset(vertices, 0, (vertex_count * sizeof(vector3f)));
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				for (i=0; i < vertex_count; i++){
					memcpy(&vertices[n], &object->v, sizeof(vector3f));
					memcpy(&colors[n], &object_clr->v, sizeof(vector4f));
					indices[n] = n;
					object = object->next;
					object_clr = object_clr->next;
					n++;
				}
				break;
			case SCE_GXM_PRIMITIVE_QUADS:
				quad_n = vertex_count >> 2;
				idx_count = quad_n * 6;
				vertices = (vector3f*)gpu_pool_memalign(vertex_count * sizeof(vector3f), sizeof(vector3f));
				colors = (vector4f*)gpu_pool_memalign(vertex_count * sizeof(vector4f), sizeof(vector4f));
				memset(vertices, 0, (vertex_count * sizeof(vector3f)));
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				int i, j;
				for (i=0; i < quad_n; i++){
					indices[i*6] = i*4;
					indices[i*6+1] = i*4+1;
					indices[i*6+2] = i*4+3;
					indices[i*6+3] = i*4+1;
					indices[i*6+4] = i*4+2;
					indices[i*6+5] = i*4+3;
				}
				for (j=0; j < vertex_count; j++){
					memcpy(&vertices[j], &object->v, sizeof(vector3f));
					memcpy(&colors[j], &object_clr->v, sizeof(vector4f));
					object = object->next;
					object_clr = object_clr->next;
				}
				break;
		}
		sceGxmSetVertexStream(gxm_context, 0, vertices);
		sceGxmSetVertexStream(gxm_context, 1, colors);
		sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
	}
	
	purge_vertex_list();
	vertex_count = 0;
	
}

void glGenBuffers(GLsizei n, GLuint* res){
	int i = 0, j = 0;
	if (n < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	i = 0;
	for (i=0; i < BUFFERS_NUM; i++){
		if (buffers[i] != 0x0000){
			res[j++] = buffers[i];
			buffers[i] = 0x0000;
		}
		if (j >= n) break;
	}
}

void glBindBuffer(GLenum target, GLuint buffer){
	if ((buffer != 0x0000) && ((buffer >= BUFFERS_ADDR + BUFFERS_NUM) || (buffer < BUFFERS_ADDR))){
		error = GL_INVALID_VALUE;
		return;
	}
	switch (target){
		case GL_ARRAY_BUFFER:
			vertex_array_unit = buffer - BUFFERS_ADDR;
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			index_array_unit = buffer - BUFFERS_ADDR;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glDeleteBuffers(GLsizei n, const GLuint* gl_buffers){
	if (n < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	int i, j;
	for (j=0; j<n; j++){
		if (gl_buffers[j] >= BUFFERS_ADDR && gl_buffers[j] < (BUFFERS_ADDR + BUFFERS_NUM)){
			uint8_t idx = gl_buffers[j] - BUFFERS_ADDR;
			buffers[idx] = gl_buffers[j];
			if (gpu_buffers[idx].ptr != NULL){
				gpu_free_mapped(gpu_buffers[idx].ptr, VGL_MEM_VRAM);
				gpu_buffers[idx].ptr = NULL;
			}
		}
	}
}

void glBufferData(GLenum target, GLsizei size, const GLvoid* data, GLenum usage){
	if (size < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	int idx = 0;
	switch (target){
		case GL_ARRAY_BUFFER:
			idx = vertex_array_unit;
			break;
		case GL_ELEMENT_ARRAY_BUFFER:
			idx = index_array_unit;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	gpu_buffers[idx].ptr = gpu_alloc_mapped(size, VGL_MEM_VRAM);
	memcpy(gpu_buffers[idx].ptr, data, size);
}

void glTexCoord2i(GLint s, GLint t){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model_uv == NULL){ 
		last3 = (uvList*)malloc(sizeof(uvList));
		model_uv = last3;
	}else{
		last3->next = (uvList*)malloc(sizeof(uvList));
		last3 = last3->next;
	}
	last3->v.x = s;
	last3->v.y = t;
}

void glTexCoord2f(GLfloat s, GLfloat t){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model_uv == NULL){ 
		last3 = (uvList*)malloc(sizeof(uvList));
		model_uv = last3;
	}else{
		last3->next = (uvList*)malloc(sizeof(uvList));
		last3 = last3->next;
	}
	last3->v.x = s;
	last3->v.y = t;
}

void glTexCoord2fv(GLfloat *f){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model_uv == NULL){ 
		last3 = (uvList*)malloc(sizeof(uvList));
		model_uv = last3;
	}else{
		last3->next = (uvList*)malloc(sizeof(uvList));
		last3 = last3->next;
	}
	last3->v.x = f[0];
	last3->v.y = f[1];
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model_vertices == NULL){ 
		last = (vertexList*)malloc(sizeof(vertexList));
		last2 = (rgbaList*)malloc(sizeof(rgbaList));
		model_vertices = last;
		model_color = last2;
	}else{
		last->next = (vertexList*)malloc(sizeof(vertexList));
		last2->next = (rgbaList*)malloc(sizeof(rgbaList));
		last = last->next;
		last2 = last2->next;
	}
	last->v.x = x;
	last->v.y = y;
	last->v.z = z;
	memcpy(&last2->v, &current_color.r, sizeof(vector4f));
	vertex_count++;
}

void glVertex3fv(const GLfloat* v){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model_vertices == NULL){ 
		last = (vertexList*)malloc(sizeof(vertexList));
		last2 = (rgbaList*)malloc(sizeof(rgbaList));
		model_vertices = last;
		model_color = last2;
	}else{
		last->next = (vertexList*)malloc(sizeof(vertexList));
		last2->next = (rgbaList*)malloc(sizeof(rgbaList));
		last = last->next;
		last2 = last2->next;
	}
	memcpy(&last->v, v, sizeof(vector3f));
	memcpy(&last2->v, &current_color.r, sizeof(vector4f));
	vertex_count++;
}

void glVertex2f(GLfloat x,  GLfloat y){
	glVertex3f(x, y, 0.5f);
}

void glArrayElement(GLint i){
	if (i < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	if (vertex_array_state){
		uint8_t* ptr;
		if (tex_unit->vertex_array.stride == 0) ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (i * (tex_unit->vertex_array.num * tex_unit->vertex_array.size));
		else ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (i * tex_unit->vertex_array.stride);
		if (model_vertices == NULL){ 
			last = (vertexList*)malloc(sizeof(vertexList));
			last2 = (rgbaList*)malloc(sizeof(rgbaList));
			model_color = last2;
			model_vertices = last;
		}else{
			last->next = (vertexList*)malloc(sizeof(vertexList));
			last2->next = (rgbaList*)malloc(sizeof(rgbaList));
			last2 = last2->next;
			last = last->next;
		}
		memcpy(&last->v, ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
		if (texture_array_state){
			uint8_t* ptr_tex;
			if (tex_unit->texture_array.stride == 0) ptr_tex = ((uint8_t*)tex_unit->texture_array.pointer) + (i * (tex_unit->texture_array.num * tex_unit->texture_array.size));
			else ptr_tex = ((uint8_t*)tex_unit->texture_array.pointer) + (i * tex_unit->texture_array.stride);
			if (model_uv == NULL){
				last3 = (uvList*)malloc(sizeof(uvList));
				model_uv = last3;
			}else{
				last3->next = (uvList*)malloc(sizeof(uvList));
				last3 = last3->next;
			}
			memcpy(&last3->v, ptr_tex, tex_unit->vertex_array.size * 2);
		}else if (color_array_state){
			uint8_t* ptr_clr;
			if (tex_unit->color_array.stride == 0) ptr_clr = ((uint8_t*)tex_unit->color_array.pointer) + (i * (tex_unit->color_array.num * tex_unit->color_array.size));
			else ptr_clr = ((uint8_t*)tex_unit->color_array.pointer) + (i * tex_unit->color_array.stride);
			last2->v.a = 1.0f;
			memcpy(&last2->v, ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
		}else{
			memcpy(&last2->v, &current_color.r, sizeof(vector4f));
		}
	}
}

void glViewport(GLint x,  GLint y,  GLsizei width,  GLsizei height){
	if ((width < 0) || (height < 0)){
		error = GL_INVALID_VALUE;
		return;
	}
	x_scale = width>>1;
	x_port = x + x_scale;
	y_scale = -(height>>1);
	y_port = DISPLAY_HEIGHT - y + y_scale;
	sceGxmSetViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
	viewport_mode = 1;
}

void glDepthRange(GLdouble nearVal, GLdouble farVal){
	z_port = (farVal + nearVal) / 2.0f;
	z_scale = (farVal - nearVal) / 2.0f;
	sceGxmSetViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
}

void glDepthRangef(GLfloat nearVal, GLfloat farVal){
	z_port = (farVal + nearVal) / 2.0f;
	z_scale = (farVal - nearVal) / 2.0f;
	sceGxmSetViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue){
	current_color.r = red;
	current_color.g = green;
	current_color.b = blue;
	current_color.a = 1.0f;
}

void glColor3fv(const GLfloat* v){
	memcpy(&current_color.r, v, sizeof(vector3f));
	current_color.a = 1.0f;
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue){
	current_color.r = (1.0f * red) / 255.0f;
	current_color.g = (1.0f * green) / 255.0f;
	current_color.b = (1.0f * blue) / 255.0f;
	current_color.a = 1.0f;
}

void glColor3ubv(const GLubyte* c){
	current_color.r = (1.0f * c[0]) / 255.0f;
	current_color.g = (1.0f * c[1]) / 255.0f;
	current_color.b = (1.0f * c[2]) / 255.0f;
	current_color.a = 1.0f;
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){
	current_color.r = red;
	current_color.g = green;
	current_color.b = blue;
	current_color.a = alpha;
}

void glColor4fv(const GLfloat* v){
	memcpy(&current_color.r, v, sizeof(vector4f));
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha){
	current_color.r = (1.0f * red) / 255.0f;
	current_color.g = (1.0f * green) / 255.0f;
	current_color.b = (1.0f * blue) / 255.0f;
	current_color.a = (1.0f * alpha) / 255.0f;
}

void glColor4ubv(const GLubyte* c){
	current_color.r = (1.0f * c[0]) / 255.0f;
	current_color.g = (1.0f * c[1]) / 255.0f;
	current_color.b = (1.0f * c[2]) / 255.0f;
	current_color.a = (1.0f * c[3]) / 255.0f;
}

void glBlendFunc(GLenum sfactor, GLenum dfactor){
	switch (sfactor){
		case GL_ZERO:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
			break;
		case GL_ONE:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
			break;
		case GL_SRC_COLOR:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GL_SRC_ALPHA:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (dfactor){
		case GL_ZERO:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
			break;
		case GL_ONE:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
			break;
		case GL_SRC_COLOR:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GL_SRC_ALPHA:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	if (blend_state) change_blend_factor();
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha){
	switch (srcRGB){
		case GL_ZERO:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO;
			break;
		case GL_ONE:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE;
			break;
		case GL_SRC_COLOR:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GL_SRC_ALPHA:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (dstRGB){
		case GL_ZERO:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO;
			break;
		case GL_ONE:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE;
			break;
		case GL_SRC_COLOR:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GL_SRC_ALPHA:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (srcAlpha){
		case GL_ZERO:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
			break;
		case GL_ONE:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
			break;
		case GL_SRC_COLOR:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GL_SRC_ALPHA:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (dstAlpha){
		case GL_ZERO:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
			break;
		case GL_ONE:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
			break;
		case GL_SRC_COLOR:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			break;
		case GL_DST_COLOR:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
			break;
		case GL_ONE_MINUS_DST_COLOR:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			break;
		case GL_SRC_ALPHA:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			break;
		case GL_DST_ALPHA:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
			break;
		case GL_ONE_MINUS_DST_ALPHA:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			break;
		case GL_SRC_ALPHA_SATURATE:
			blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	if (blend_state) change_blend_factor();
}

void glBlendEquation(GLenum mode){
	switch (mode){
		case GL_FUNC_ADD:
			blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_ADD;
			break;
		case GL_FUNC_SUBTRACT:
			blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_SUBTRACT;
			break;
		case GL_FUNC_REVERSE_SUBTRACT:
			blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;
			break;
		case GL_MIN:
			blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_MIN;
			break;
		case GL_MAX:
			blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_MAX;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	if (blend_state) change_blend_factor();
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha){
	switch (modeRGB){
		case GL_FUNC_ADD:
			blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD;
			break;
		case GL_FUNC_SUBTRACT:
			blend_func_rgb = SCE_GXM_BLEND_FUNC_SUBTRACT;
			break;
		case GL_FUNC_REVERSE_SUBTRACT:
			blend_func_rgb = SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;
			break;
		case GL_MIN:
			blend_func_rgb = SCE_GXM_BLEND_FUNC_MIN;
			break;
		case GL_MAX:
			blend_func_rgb = SCE_GXM_BLEND_FUNC_MAX;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (modeAlpha){
		case GL_FUNC_ADD:
			blend_func_a = SCE_GXM_BLEND_FUNC_ADD;
			break;
		case GL_FUNC_SUBTRACT:
			blend_func_a = SCE_GXM_BLEND_FUNC_SUBTRACT;
			break;
		case GL_FUNC_REVERSE_SUBTRACT:
			blend_func_a = SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;
			break;
		case GL_MIN:
			blend_func_a = SCE_GXM_BLEND_FUNC_MIN;
			break;
		case GL_MAX:
			blend_func_a = SCE_GXM_BLEND_FUNC_MAX;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	if (blend_state) change_blend_factor();
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){
	blend_color_mask = SCE_GXM_COLOR_MASK_NONE;
	if (red) blend_color_mask += SCE_GXM_COLOR_MASK_R;
	if (green) blend_color_mask += SCE_GXM_COLOR_MASK_G;
	if (blue) blend_color_mask += SCE_GXM_COLOR_MASK_B;
	if (alpha) blend_color_mask += SCE_GXM_COLOR_MASK_A;
	if (blend_state) change_blend_factor();
	else change_blend_mask();
}

void glCullFace(GLenum mode){
	gl_cull_mode = mode;
	if (cull_face_state) change_cull_mode();
}

void glFrontFace(GLenum mode){
	gl_front_face = mode;
	if (cull_face_state) change_cull_mode();
}

GLboolean glIsEnabled(GLenum cap){
	GLboolean ret = GL_FALSE;
	switch (cap){
		case GL_DEPTH_TEST:
			ret = depth_test_state;
			break;
		case GL_STENCIL_TEST:
			ret = stencil_test_state;
			break;
		case GL_BLEND:
			ret = blend_state;
			break;
		case GL_SCISSOR_TEST:
			ret = scissor_test_state;
			break;
		case GL_CULL_FACE:
			ret = cull_face_state;
			break;
		case GL_POLYGON_OFFSET_FILL:
			ret = pol_offset_fill;
			break;
		case GL_POLYGON_OFFSET_LINE:
			ret = pol_offset_line;
			break;
		case GL_POLYGON_OFFSET_POINT:
			ret = pol_offset_point;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	return ret;
}

void glPolygonMode(GLenum face,  GLenum mode){
	SceGxmPolygonMode new_mode;
	switch (mode){
		case GL_POINT:
			new_mode = SCE_GXM_POLYGON_MODE_TRIANGLE_POINT;
			break;
		case GL_LINE:
			new_mode = SCE_GXM_POLYGON_MODE_TRIANGLE_LINE;
			break;
		case GL_FILL:
			new_mode = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (face){
		case GL_FRONT:
			polygon_mode_front = new_mode;
			sceGxmSetFrontPolygonMode(gxm_context, new_mode);
			break;
		case GL_BACK:
			polygon_mode_back = new_mode;
			sceGxmSetBackPolygonMode(gxm_context, new_mode);
			break;
		case GL_FRONT_AND_BACK:
			polygon_mode_front = polygon_mode_back = new_mode;
			sceGxmSetFrontPolygonMode(gxm_context, new_mode);
			sceGxmSetBackPolygonMode(gxm_context, new_mode);
			break;
		default:
			error = GL_INVALID_ENUM;
			return;
	}
	update_polygon_offset();
}

void glPolygonOffset(GLfloat factor, GLfloat units){
	pol_factor = factor;
	pol_units = units;
	update_polygon_offset();
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer){
	if ((stride < 0) || ((size < 2) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	switch (type){
		case GL_FLOAT:
			tex_unit->vertex_array.size = sizeof(GLfloat);
			break;
		case GL_SHORT:
			tex_unit->vertex_array.size = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	tex_unit->vertex_array.num = size;
	tex_unit->vertex_array.stride = stride;
	tex_unit->vertex_array.pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer){
	if ((stride < 0) || ((size < 3) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	switch (type){
		case GL_FLOAT:
			tex_unit->color_array.size = sizeof(GLfloat);
			break;
		case GL_SHORT:
			tex_unit->color_array.size = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	tex_unit->color_array.num = size;
	tex_unit->color_array.stride = stride;
	tex_unit->color_array.pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer){
	if ((stride < 0) || ((size < 2) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	switch (type){
		case GL_FLOAT:
			tex_unit->texture_array.size = sizeof(GLfloat);
			break;
		case GL_SHORT:
			tex_unit->texture_array.size = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	tex_unit->texture_array.num = size;
	tex_unit->texture_array.stride = stride;
	tex_unit->texture_array.pointer = pointer;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count){
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	SceGxmPrimitiveType gxm_p;
	GLboolean skip_draw = GL_FALSE;
	if (vertex_array_state){
		switch (mode){
			case GL_POINTS:
				gxm_p = SCE_GXM_PRIMITIVE_POINTS;
				break;
			case GL_LINES:
				gxm_p = SCE_GXM_PRIMITIVE_LINES;
				if ((count % 2) != 0) skip_draw = GL_TRUE;
				break;
			case GL_TRIANGLES:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLES;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				else if ((count % 3) != 0) skip_draw = GL_TRUE;
				break;
			case GL_TRIANGLE_STRIP:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				break;
			case GL_TRIANGLE_FAN:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				break;
			default:
				error = GL_INVALID_ENUM;
				break;
		}
		if (!skip_draw){
	
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			
			if (texture_array_state){
				if (!(tex_unit->textures[texture2d_idx].valid)) return;
				if (color_array_state){
					sceGxmSetVertexProgram(gxm_context, texture2d_rgba_vertex_program_patched);
					sceGxmSetFragmentProgram(gxm_context, texture2d_rgba_fragment_program_patched);
					void* alpha_buffer;
					sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_alpha_cut, 0, 1, &alpha_ref);
					float alpha_operation = (float)alpha_op;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_alpha_op, 0, 1, &alpha_operation);
					float env_mode = (float)tex_unit->env_mode;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_tex_env, 0, 1, &env_mode);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_tex_env_color, 0, 4, &texenv_color.r);
				}else{
					sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
					sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
					void* alpha_buffer;
					sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
					float alpha_operation = (float)alpha_op;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_operation);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
					float env_mode = (float)tex_unit->env_mode;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &env_mode);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env_color, 0, 4, &texenv_color.r);
				}
			}else if (color_array_state && (tex_unit->color_array.num == 3)){
				sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}else{
				sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}
			
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
			if (texture_array_state){
				if (color_array_state) sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_rgba_wvp, 0, 16, (const float*)mvp_matrix);
				else sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)mvp_matrix);
				sceGxmSetFragmentTexture(gxm_context, 0, &tex_unit->textures[texture2d_idx].gxm_tex);
				vector3f* vertices = NULL;
				vector2f* uv_map = NULL;
				vector4f* colors = NULL;
				uint16_t* indices;
				uint16_t n;
				if (vertex_array_unit >= 0){
					if (tex_unit->vertex_array.stride == 0) vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
					else vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride));
					if (tex_unit->texture_array.stride == 0) uv_map = (vector2f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer) + (first * (tex_unit->texture_array.num * tex_unit->texture_array.size)));
					else uv_map = (vector2f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer) + (first * tex_unit->texture_array.stride));
					if (color_array_state){
						if (tex_unit->color_array.stride == 0) colors = (vector4f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer) + (first * (tex_unit->color_array.num * tex_unit->color_array.size)));
						else colors = (vector4f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer) + (first * tex_unit->color_array.stride));
					}
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0; n<count; n++){
						indices[n] = n;
					}
				}else{
					uint8_t* ptr;
					uint8_t* ptr_tex;
					uint8_t* ptr_clr;
					vertices = (vector3f*)gpu_pool_memalign(count * sizeof(vector3f), sizeof(vector3f));
					uv_map = (vector2f*)gpu_pool_memalign(count * sizeof(vector2f), sizeof(vector2f));
					if (color_array_state) colors = (vector4f*)gpu_pool_memalign(count * sizeof(vector4f), sizeof(vector4f));
					memset(vertices, 0, (count * sizeof(vector3f)));
					uint8_t vec_set = 0, tex_set = 0, clr_set = 0;
					if (tex_unit->vertex_array.stride == 0){
						ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size));
						memcpy(&vertices[n], ptr, count * sizeof(vector3f));
						vec_set = 1;
					}else ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride);	
					if (tex_unit->texture_array.stride == 0){
						ptr_tex = ((uint8_t*)tex_unit->texture_array.pointer) + (first * (tex_unit->texture_array.num * tex_unit->texture_array.size));
						memcpy(&uv_map[n], ptr_tex, count * sizeof(vector2f));
						tex_set = 1;
					}else ptr_tex = ((uint8_t*)tex_unit->texture_array.pointer) + (first * tex_unit->texture_array.stride);
					if (color_array_state){
						if (tex_unit->color_array.stride == 0){
							ptr_clr = ((uint8_t*)tex_unit->color_array.pointer) + (first * sizeof(vector4f));
							memcpy(&colors[n], ptr_clr, count * sizeof(vector4f));
							clr_set = 1;
						}else ptr_clr = ((uint8_t*)tex_unit->color_array.pointer) + (first * tex_unit->color_array.stride);
					}
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0;n<count;n++){
						if (!vec_set){
							memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							ptr += tex_unit->vertex_array.stride;
						}
						if (!tex_set){
							memcpy(&uv_map[n], ptr_tex, tex_unit->texture_array.size * tex_unit->texture_array.num);
							ptr_tex += tex_unit->texture_array.stride;
						}
						if (color_array_state && (!clr_set)){
							memcpy(&colors[n], ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
							ptr_clr += tex_unit->color_array.stride;
						}
						indices[n] = n;
					}
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				if (color_array_state) sceGxmSetVertexStream(gxm_context, 2, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}else if (color_array_state){
				if (tex_unit->color_array.num == 3) sceGxmSetUniformDataF(vertex_wvp_buffer, rgb_wvp, 0, 16, (const float*)mvp_matrix);
				else  sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
				vector3f* vertices = NULL;
				uint8_t* colors = NULL;
				uint16_t* indices;
				uint16_t n = 0;
				if (vertex_array_unit >= 0){
					if (tex_unit->vertex_array.stride == 0) vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
					else vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride));
					if (tex_unit->color_array.stride == 0) colors = (uint8_t*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer) + (first * (tex_unit->color_array.num * tex_unit->color_array.size)));
					else colors = (uint8_t*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer) + (first * tex_unit->color_array.stride));
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0; n<count; n++){
						indices[n] = n;
					}
				}else{
					uint8_t* ptr;
					uint8_t* ptr_clr;
					vertices = (vector3f*)gpu_pool_memalign(count * sizeof(vector3f), sizeof(vector3f));
					colors = (uint8_t*)gpu_pool_memalign(count * tex_unit->color_array.num * tex_unit->color_array.size, tex_unit->color_array.num * tex_unit->color_array.size);
					memset(vertices, 0, (count * sizeof(vector3f)));
					uint8_t vec_set = 0, clr_set = 0;
					if (tex_unit->vertex_array.stride == 0){
						ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (first * ((tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
						memcpy(&vertices[n], ptr, count * sizeof(vector3f));
						vec_set = 1;
					}else ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.stride));
					if (tex_unit->color_array.stride == 0){
						ptr_clr = ((uint8_t*)tex_unit->color_array.pointer) + (first * ((tex_unit->color_array.num * tex_unit->color_array.size)));
						memcpy(&colors[n], ptr_clr, count * tex_unit->color_array.num * tex_unit->color_array.size);
						clr_set = 1;
					}else ptr_clr = ((uint8_t*)tex_unit->color_array.pointer) + (first * tex_unit->color_array.size);
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0; n<count; n++){
						if (!vec_set){
							memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							ptr += tex_unit->vertex_array.stride;
						}
						if (!clr_set){
							memcpy(&colors[n * tex_unit->color_array.num * tex_unit->color_array.size], ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
							ptr_clr += tex_unit->color_array.stride;
						}
						indices[n] = n;
					}
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);	
			}else{
				sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
				vector3f* vertices = NULL;
				vector4f* colors = NULL;
				uint16_t* indices;
				uint16_t n = 0;
				if (vertex_array_unit >= 0){
					if (tex_unit->vertex_array.stride == 0) vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
					else vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride));
					colors = (vector4f*)gpu_pool_memalign(count * sizeof(vector4f), sizeof(vector4f));
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0; n<count; n++){
						memcpy(&colors[n], &current_color.r, sizeof(vector4f));
						indices[n] = n;
					}
				}else{
					uint8_t* ptr;
					vertices = (vector3f*)gpu_pool_memalign(count * sizeof(vector3f), sizeof(vector3f));
					colors = (vector4f*)gpu_pool_memalign(count * sizeof(vector4f), sizeof(vector4f));
					memset(vertices, 0, (count * sizeof(vector3f)));
					uint8_t vec_set = 0;
					if (tex_unit->vertex_array.stride == 0){
						ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (first * ((tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
						memcpy(&vertices[n], ptr, count * sizeof(vector3f));
						vec_set = 1;
					}else ptr = ((uint8_t*)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.stride));
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0; n<count; n++){
						if (!vec_set){
							memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							ptr += tex_unit->vertex_array.stride;
						}
						memcpy(&colors[n], &current_color.r, sizeof(vector4f));
						indices[n] = n;
					}
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}
		}
	}
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* gl_indices){
	SceGxmPrimitiveType gxm_p;
	SceGxmPrimitiveTypeExtra gxm_ep = SCE_GXM_PRIMITIVE_NONE;
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	GLboolean skip_draw = GL_FALSE;
	if (vertex_array_state){
		switch (mode){
			case GL_POINTS:
				gxm_p = SCE_GXM_PRIMITIVE_POINTS;
				break;
			case GL_LINES:
				gxm_p = SCE_GXM_PRIMITIVE_LINES;
				break;
			case GL_TRIANGLES:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLES;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				break;
			case GL_TRIANGLE_STRIP:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				break;
			case GL_TRIANGLE_FAN:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				break;
			default:
				error = GL_INVALID_ENUM;
				break;
		}
		if (type != GL_UNSIGNED_SHORT) error = GL_INVALID_ENUM;
		else if (phase == MODEL_CREATION) error = GL_INVALID_OPERATION;
		else if (count < 0) error = GL_INVALID_VALUE;
		if (!skip_draw){
	
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			
			if (texture_array_state){
				if (!(tex_unit->textures[texture2d_idx].valid)) return;
				if (color_array_state){
					sceGxmSetVertexProgram(gxm_context, texture2d_rgba_vertex_program_patched);
					sceGxmSetFragmentProgram(gxm_context, texture2d_rgba_fragment_program_patched);
					void* alpha_buffer;
					sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_alpha_cut, 0, 1, &alpha_ref);
					float alpha_operation = (float)alpha_op;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_alpha_op, 0, 1, &alpha_operation);
					float env_mode = (float)tex_unit->env_mode;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_tex_env, 0, 1, &env_mode);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_tex_env_color, 0, 4, &texenv_color.r);
				}else{
					sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
					sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
					void* alpha_buffer;
					sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
					float alpha_operation = (float)alpha_op;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_operation);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
					float env_mode = (float)tex_unit->env_mode;
					sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &env_mode);
					sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env_color, 0, 4, &texenv_color.r);
				}
			}else if (color_array_state && (tex_unit->color_array.num == 3)){
				sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}else{
				sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}
			
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
			if (texture_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)mvp_matrix);
				sceGxmSetFragmentTexture(gxm_context, 0, &texture_units[client_texture_unit].textures[texture2d_idx].gxm_tex);
				vector3f* vertices = NULL;
				vector2f* uv_map = NULL;
				vector4f* colors = NULL;
				uint16_t* indices;
				if (index_array_unit >= 0) indices = (uint16_t*)((uint32_t)gpu_buffers[index_array_unit].ptr + (uint32_t)gl_indices);
				else{
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					memcpy(indices, gl_indices, sizeof(uint16_t) * count);
				}
				if (vertex_array_unit >= 0){
					vertices = (vector3f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
					uv_map = (vector2f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer);	
					if (color_array_state) colors = (vector4f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer);	
				}else{
					int n = 0, j = 0;
					uint64_t vertex_count_int = 0;
					uint16_t* ptr_idx = (uint16_t*)gl_indices;
					while (j < count){
						if (ptr_idx[j] >= vertex_count_int) vertex_count_int = ptr_idx[j] + 1;
						j++;
					}
					vertices = (vector3f*)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
					uv_map = (vector2f*)gpu_pool_memalign(vertex_count_int * sizeof(vector2f), sizeof(vector2f));
					colors = (vector4f*)gpu_pool_memalign(vertex_count_int * sizeof(vector4f), sizeof(vector4f));
					if (tex_unit->vertex_array.stride == 0) memcpy(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
					if (tex_unit->texture_array.stride == 0) memcpy(uv_map, tex_unit->texture_array.pointer, vertex_count_int * (tex_unit->texture_array.size * tex_unit->texture_array.num));
					if (color_array_state && (tex_unit->color_array.stride == 0)) memcpy(colors, tex_unit->color_array.pointer, vertex_count_int * (tex_unit->color_array.size * tex_unit->color_array.num));
					if ((tex_unit->vertex_array.stride != 0) || (tex_unit->texture_array.stride != 0)){
						if (tex_unit->vertex_array.stride != 0) memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
						uint8_t* ptr = ((uint8_t*)tex_unit->vertex_array.pointer);
						uint8_t* ptr_tex = ((uint8_t*)tex_unit->texture_array.pointer);
						for (n=0; n<vertex_count_int; n++){
							if (tex_unit->vertex_array.stride != 0)  memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							if (tex_unit->texture_array.stride != 0)  memcpy(&uv_map[n], ptr_tex, tex_unit->texture_array.size * tex_unit->texture_array.num);
							ptr += tex_unit->vertex_array.stride;
							ptr_tex += tex_unit->texture_array.stride;
						}
					}
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				if (color_array_state) sceGxmSetVertexStream(gxm_context, 2, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}else if (color_array_state){
				if (tex_unit->color_array.num == 3) sceGxmSetUniformDataF(vertex_wvp_buffer, rgb_wvp, 0, 16, (const float*)mvp_matrix);
				else sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
				vector3f* vertices = NULL;
				uint8_t* colors = NULL;
				uint16_t* indices;
				if (index_array_unit >= 0) indices = (uint16_t*)((uint32_t)gpu_buffers[index_array_unit].ptr + (uint32_t)gl_indices);
				else{
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					memcpy(indices, gl_indices, sizeof(uint16_t) * count);
				}
				if (vertex_array_unit >= 0){ 
					colors = (uint8_t*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer);
					vertices = (vector3f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
				}else{
					int n = 0, j = 0;
					uint64_t vertex_count_int = 0;
					uint16_t* ptr_idx = (uint16_t*)gl_indices;
					while (j < count){
						if (ptr_idx[j] >= vertex_count_int) vertex_count_int = ptr_idx[j] + 1;
						j++;
					}
					vertices = (vector3f*)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
					colors = (uint8_t*)gpu_pool_memalign(vertex_count_int * tex_unit->color_array.num * tex_unit->color_array.size, tex_unit->color_array.num * tex_unit->color_array.size);
					if (tex_unit->vertex_array.stride == 0) memcpy(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
					if (tex_unit->color_array.stride == 0) memcpy(colors, tex_unit->color_array.pointer, vertex_count_int * (tex_unit->color_array.size * tex_unit->color_array.num));
					if ((tex_unit->vertex_array.stride != 0) || (tex_unit->color_array.stride != 0)){
						if (tex_unit->vertex_array.stride != 0) memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
						uint8_t* ptr = ((uint8_t*)tex_unit->vertex_array.pointer);
						uint8_t* ptr_clr = ((uint8_t*)tex_unit->color_array.pointer);
						for (n=0; n<vertex_count_int; n++){
							if (tex_unit->vertex_array.stride != 0)  memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							if (tex_unit->color_array.stride != 0)  memcpy(&colors[n * tex_unit->color_array.num * tex_unit->color_array.size], ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
							ptr += tex_unit->vertex_array.stride;
							ptr_clr += tex_unit->color_array.stride;
						}
					}
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}else{
				sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
				vector3f* vertices = NULL;
				vector4f* colors = NULL;
				uint16_t* indices;
				if (index_array_unit >= 0) indices = (uint16_t*)((uint32_t)gpu_buffers[index_array_unit].ptr + (uint32_t)gl_indices);
				else{
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					memcpy(indices, gl_indices, sizeof(uint16_t) * count);
				}
				int n = 0, j = 0;
				uint64_t vertex_count_int = 0;
				uint16_t* ptr_idx = (uint16_t*)gl_indices;
				while (j < count){
					if (ptr_idx[j] >= vertex_count_int) vertex_count_int = ptr_idx[j] + 1;
					j++;
				}
				if (vertex_array_unit >= 0) vertices = (vector3f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
				else vertices = (vector3f*)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
				colors = (vector4f*)gpu_pool_memalign(vertex_count_int * tex_unit->color_array.num * tex_unit->color_array.size, tex_unit->color_array.num * tex_unit->color_array.size);
				if ((!vertex_array_unit) && tex_unit->vertex_array.stride == 0) memcpy(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
				if ((!vertex_array_unit) && tex_unit->vertex_array.stride != 0) memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
				uint8_t* ptr = ((uint8_t*)tex_unit->vertex_array.pointer);
				for (n=0; n<vertex_count_int; n++){
					if ((!vertex_array_unit) && tex_unit->vertex_array.stride != 0)  memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
					memcpy(&colors[n], &current_color.r, sizeof(vector4f));
					if (!vertex_array_unit) ptr += tex_unit->vertex_array.stride;
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}
		}
	}
}

void glEnableClientState(GLenum array){
	switch (array){
		case GL_VERTEX_ARRAY:
			vertex_array_state = GL_TRUE;
			break;
		case GL_COLOR_ARRAY:
			color_array_state = GL_TRUE;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			texture_array_state = GL_TRUE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glDisableClientState(GLenum array){
	switch (array){
		case GL_VERTEX_ARRAY:
			vertex_array_state = GL_FALSE;
			break;
		case GL_COLOR_ARRAY:
			color_array_state = GL_FALSE;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			texture_array_state = GL_FALSE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glClientActiveTexture(GLenum texture){
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE31)) error = GL_INVALID_ENUM;
	else client_texture_unit = texture - GL_TEXTURE0;
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param){
	texture_unit* tex_unit = &texture_units[server_texture_unit];
	switch (target){
		case GL_TEXTURE_ENV:
			switch (pname){
				case GL_TEXTURE_ENV_MODE:
					if (param == GL_MODULATE) tex_unit->env_mode = MODULATE;
					else if (param == GL_DECAL) tex_unit->env_mode = DECAL;
					else if (param == GL_REPLACE) tex_unit->env_mode = REPLACE;
					else if (param == GL_BLEND) tex_unit->env_mode = BLEND;
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
	}
}

void glTexEnvi(GLenum target,  GLenum pname,  GLint param){
	texture_unit* tex_unit = &texture_units[server_texture_unit];
	switch (target){
		case GL_TEXTURE_ENV:
			switch (pname){
				case GL_TEXTURE_ENV_MODE:
					switch (param){
						case GL_MODULATE:
							tex_unit->env_mode = MODULATE;
							break;
						case GL_DECAL: 
							tex_unit->env_mode = DECAL;
							break;
						case GL_REPLACE:
							tex_unit->env_mode = REPLACE;
							break;
						case GL_BLEND:
							tex_unit->env_mode = BLEND;
							break;
					}
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
	}
}

void glReadPixels(GLint x,  GLint y,  GLsizei width,  GLsizei height,  GLenum format,  GLenum type,  GLvoid * data){
	SceDisplayFrameBuf pParam;
	pParam.size = sizeof(SceDisplayFrameBuf);
	sceDisplayGetFrameBuf(&pParam, SCE_DISPLAY_SETBUF_NEXTFRAME);
	y = DISPLAY_HEIGHT - (height + y);
	int i,j;
	uint8_t* out8 = (uint8_t*)data;
	uint8_t* in8 = (uint8_t*)pParam.base;
	uint32_t* out32 = (uint32_t*)data;
	uint32_t* in32 = (uint32_t*)pParam.base;
	switch (format){
		case GL_RGBA:
			switch (type){
				case GL_UNSIGNED_BYTE:
					in32 += (x + y * pParam.pitch);
					for (i = 0; i < height; i++){
						for (j = 0; j < width; j++){
							out32[(height-(i+1))*width+j] = in32[j];
						}
						in32 += pParam.pitch;
					}
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		case GL_RGB:
			switch (type){
				case GL_UNSIGNED_BYTE:
					in8 += (x * 4 + y * pParam.pitch * 4);
					for (i = 0; i < height; i++){
						for (j = 0; j < width; j++){
							out8[((height-(i+1))*width+j)*3] = in8[j*4];
							out8[((height-(i+1))*width+j)*3+1] = in8[j*4+1];
							out8[((height-(i+1))*width+j)*3+2] = in8[j*4+2];
						}
						in8 += pParam.pitch * 4;
					}
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

// VGL_EXT_gpu_objects_array extension implementation

void vglVertexPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid* pointer){
	if ((stride < 0) || ((size < 2) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type){
		case GL_FLOAT:
			bpe = sizeof(GLfloat);
			break;
		case GL_SHORT:
			bpe = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	tex_unit->vertex_object = gpu_pool_memalign(count * bpe * size, bpe * size);
	if (stride == 0) memcpy(tex_unit->vertex_object, pointer, count * bpe * size);
	else{
		int i;
		uint8_t* dst = (uint8_t*)tex_unit->vertex_object;
		uint8_t* src = (uint8_t*)pointer;
		for (i=0;i<count;i++){
			memcpy(dst, src, bpe * size);
			dst += (bpe*size);
			src += stride;
		}
	}
}

void vglColorPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid* pointer){
	if ((stride < 0) || ((size < 3) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type){
		case GL_FLOAT:
			bpe = sizeof(GLfloat);
			break;
		case GL_SHORT:
			bpe = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	tex_unit->color_object = gpu_pool_memalign(count * bpe * size, bpe * size);
	if (stride == 0) memcpy(tex_unit->color_object, pointer, count * bpe * size);
	else{
		int i;
		uint8_t* dst = (uint8_t*)tex_unit->color_object;
		uint8_t* src = (uint8_t*)pointer;
		for (i=0;i<count;i++){
			memcpy(dst, src, bpe * size);
			dst += (bpe*size);
			src += stride;
		}
	}
}

void vglTexCoordPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid* pointer){
	if ((stride < 0) || ((size < 2) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type){
		case GL_FLOAT:
			bpe = sizeof(GLfloat);
			break;
		case GL_SHORT:
			bpe = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	tex_unit->texture_object = gpu_pool_memalign(count * bpe * size, bpe * size);
	if (stride == 0) memcpy(tex_unit->texture_object, pointer, count * bpe * size);
	else{
		int i;
		uint8_t* dst = (uint8_t*)tex_unit->texture_object;
		uint8_t* src = (uint8_t*)pointer;
		for (i=0;i<count;i++){
			memcpy(dst, src, bpe * size);
			dst += (bpe*size);
			src += stride;
		}
	}
}

void vglIndexPointer(GLenum type, GLsizei stride, GLuint count, const GLvoid* pointer){
	if (stride < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type){
		case GL_FLOAT:
			bpe = sizeof(GLfloat);
			break;
		case GL_SHORT:
			bpe = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	tex_unit->index_object = gpu_pool_memalign(count * bpe, bpe);
	if (stride == 0) memcpy(tex_unit->index_object, pointer, count * bpe);
	else{
		int i;
		uint8_t* dst = (uint8_t*)tex_unit->index_object;
		uint8_t* src = (uint8_t*)pointer;
		for (i=0;i<count;i++){
			memcpy(dst, src, bpe);
			dst += bpe;
			src += stride;
		}
	}
}

void vglDrawObjects(GLenum mode, GLsizei count, GLboolean implicit_wvp){
	SceGxmPrimitiveType gxm_p;
	texture_unit* tex_unit = &texture_units[client_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	GLboolean skip_draw = GL_FALSE;
	switch (mode){
		case GL_POINTS:
			gxm_p = SCE_GXM_PRIMITIVE_POINTS;
			break;
		case GL_LINES:
			gxm_p = SCE_GXM_PRIMITIVE_LINES;
			break;
		case GL_TRIANGLES:
			gxm_p = SCE_GXM_PRIMITIVE_TRIANGLES;
			if (no_polygons_mode) skip_draw = GL_TRUE;
			break;
		case GL_TRIANGLE_STRIP:
			gxm_p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
			if (no_polygons_mode) skip_draw = GL_TRUE;
			break;
		case GL_TRIANGLE_FAN:
			gxm_p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
			if (no_polygons_mode) skip_draw = GL_TRUE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	if (phase == MODEL_CREATION) error = GL_INVALID_OPERATION;
	else if (count < 0) error = GL_INVALID_VALUE;
	if (!skip_draw){
		if (cur_program != 0){
			_vglDrawObjects_CustomShadersIMPL(mode, count, implicit_wvp);
			sceGxmSetFragmentTexture(gxm_context, 0, &tex_unit->textures[texture2d_idx].gxm_tex);
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
			vert_uniforms = NULL;
			frag_uniforms = NULL;
		}else{
			if (vertex_array_state){
				matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
				if (texture_array_state){
					if (!(tex_unit->textures[texture2d_idx].valid)) return;
					if (color_array_state){
						sceGxmSetVertexProgram(gxm_context, texture2d_rgba_vertex_program_patched);
						sceGxmSetFragmentProgram(gxm_context, texture2d_rgba_fragment_program_patched);
						void* alpha_buffer;
						sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_alpha_cut, 0, 1, &alpha_ref);
						float alpha_operation = (float)alpha_op;
						sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_alpha_op, 0, 1, &alpha_operation);
						float env_mode = (float)tex_unit->env_mode;
						sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_tex_env, 0, 1, &env_mode);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_rgba_tex_env_color, 0, 4, &texenv_color.r);
					}else{
						sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
						sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
						void* alpha_buffer;
						sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
						float alpha_operation = (float)alpha_op;
						sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_operation);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
						float env_mode = (float)tex_unit->env_mode;
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &env_mode);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env_color, 0, 4, &texenv_color.r);
					}
				}else if (color_array_state && (tex_unit->color_array.num == 3)){
					sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
					sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
				}else{
					sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
					sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
				}			
				void* vertex_wvp_buffer;
				sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
				if (texture_array_state){
					sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)mvp_matrix);
					sceGxmSetFragmentTexture(gxm_context, 0, &tex_unit->textures[texture2d_idx].gxm_tex);
					sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
					sceGxmSetVertexStream(gxm_context, 1, tex_unit->texture_object);
					if (color_array_state) sceGxmSetVertexStream(gxm_context, 2, tex_unit->color_object);
					sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
				}else if (color_array_state){
					if (tex_unit->color_array.num == 3) sceGxmSetUniformDataF(vertex_wvp_buffer, rgb_wvp, 0, 16, (const float*)mvp_matrix);
					else sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
					sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
					sceGxmSetVertexStream(gxm_context, 1, tex_unit->color_object);
					sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
				}else{
					sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)mvp_matrix);
					vector4f* colors = (vector4f*)gpu_pool_memalign(count * sizeof(vector4f), sizeof(vector4f));
					int n;
					for (n=0; n<count; n++){
						memcpy(&colors[n], &current_color.r, sizeof(vector4f));
					}
					sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
					sceGxmSetVertexStream(gxm_context, 1, colors);
					sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
				}
			}
		}
	}
}

size_t vglMemFree(VGLmemtype type) {
	if (type >= VGL_MEM_TYPE_COUNT) return 0;
	return mempool_get_free_space(type);
}
