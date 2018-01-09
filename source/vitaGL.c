#include <stdlib.h>
#include "vitaGL.h"
#include "math_utils.h"
#include "gpu_utils.h"

// Shaders
#include "clear_f.h"
#include "clear_v.h"
#include "rgba_f.h"
#include "rgba_v.h"
#include "rgb_v.h"
#include "disable_color_buffer_f.h"
#include "disable_color_buffer_v.h"
#include "texture2d_f.h"
#include "texture2d_v.h"

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define TEXTURES_NUM          1024 // Available textures per texture unit
#define MODELVIEW_STACK_DEPTH 32   // Depth of modelview matrix stack
#define GENERIC_STACK_DEPTH   2    // Depth of generic matrix stack
#define DISPLAY_WIDTH         960  // Display width in pixels
#define DISPLAY_HEIGHT        544  // Display height in pixels
#define DISPLAY_STRIDE        1024 // Display stride in pixels
#define DISPLAY_BUFFER_COUNT  2    // Display buffers to use
#define GXM_TEX_MAX_SIZE      4096 // Maximum width/height in pixels per texture
#define BUFFERS_ADDR        0xA000 // Starting address for buffers indexing
#define BUFFERS_NUM           128  // Maximum number of allocatable buffers
#define MAX_FRAGMENT_PROGS    32   // Maximum number of fragment programs per shader

static const GLubyte* vendor = "Rinnegatamante";
static const GLubyte* renderer = "SGX543MP4+";
static const GLubyte* version = "VitaGL 1.0";
static const GLubyte* extensions = "";

typedef struct clear_vertex{
	vector2f position;
} clear_vertex;

typedef struct position_vertex{
	vector3f position;
} position_vertex;

typedef struct rgba_vertex{
	vector3f position;
	vector4f color;
} rgba_vertex;

typedef struct rgb_vertex{
	vector3f position;
	vector3f color;
} rgb_vertex;

typedef struct texture2d_vertex{
	vector3f position;
	vector2f texcoord;
} texture2d_vertex;

typedef struct gpubuffer{
	void* ptr;
	SceUID data_UID;
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

// Vertex array attributes struct
typedef struct vertexArray{
	GLint size;
	GLint num;
	GLsizei stride;
	const GLvoid* pointer;
} vertexArray;

// Drawing phases for old openGL
typedef enum glPhase{
	NONE = 0,
	MODEL_CREATION = 1
} glPhase;

// Alpha operations for alpha testing
typedef enum alphaOp{
	GREATER_EQUAL = 0,
	GREATER = 1,
	NOT_EQUAL = 2,
	EQUAL = 3,
	LESS_EQUAL = 4,
	LESS = 5,
	NEVER = 6,
	ALWAYS = 7
} alphaOp;

// Texture environment mode
typedef enum texEnvMode{
	MODULATE = 0,
	DECAL = 1,
	BLEND = 2,
	REPLACE = 3
} texEnvMode;

typedef struct texture_unit{
	GLboolean enabled;
	matrix4x4 stack[GENERIC_STACK_DEPTH];
	texture   textures[TEXTURES_NUM];
	vertexArray vertex_array;
	vertexArray color_array;
	vertexArray texture_array;
	texEnvMode env_mode;
} texture_unit;

struct display_queue_callback_data {
	void *addr;
};

// Internal gxm stuffs
static SceGxmContext* gxm_context;
static SceUID vdm_ring_buffer_uid;
static void* vdm_ring_buffer_addr;
static SceUID vertex_ring_buffer_uid;
static void* vertex_ring_buffer_addr;
static SceUID fragment_ring_buffer_uid;
static void* fragment_ring_buffer_addr;
static SceUID fragment_usse_ring_buffer_uid;
static void* fragment_usse_ring_buffer_addr;
static SceGxmRenderTarget* gxm_render_target;
static SceGxmColorSurface gxm_color_surfaces[DISPLAY_BUFFER_COUNT];
static SceUID gxm_color_surfaces_uid[DISPLAY_BUFFER_COUNT];
static void* gxm_color_surfaces_addr[DISPLAY_BUFFER_COUNT];
static SceGxmSyncObject* gxm_sync_objects[DISPLAY_BUFFER_COUNT];
static unsigned int gxm_front_buffer_index;
static unsigned int gxm_back_buffer_index;
static SceUID gxm_depth_stencil_surface_uid;
static void* gxm_depth_stencil_surface_addr;
static SceGxmDepthStencilSurface gxm_depth_stencil_surface;
static SceGxmShaderPatcher* gxm_shader_patcher;
static SceUID gxm_shader_patcher_buffer_uid;
static void* gxm_shader_patcher_buffer_addr;
static SceUID gxm_shader_patcher_vertex_usse_uid;
static void* gxm_shader_patcher_vertex_usse_addr;
static SceUID gxm_shader_patcher_fragment_usse_uid;
static void* gxm_shader_patcher_fragment_usse_addr;

static const SceGxmProgram *const gxm_program_disable_color_buffer_v = (SceGxmProgram*)&disable_color_buffer_v;
static const SceGxmProgram *const gxm_program_disable_color_buffer_f = (SceGxmProgram*)&disable_color_buffer_f;
static const SceGxmProgram *const gxm_program_clear_v = (SceGxmProgram*)&clear_v;
static const SceGxmProgram *const gxm_program_clear_f = (SceGxmProgram*)&clear_f;
static const SceGxmProgram *const gxm_program_rgba_v = (SceGxmProgram*)&rgba_v;
static const SceGxmProgram *const gxm_program_rgba_f = (SceGxmProgram*)&rgba_f;
static const SceGxmProgram *const gxm_program_rgb_v = (SceGxmProgram*)&rgb_v;
static const SceGxmProgram *const gxm_program_texture2d_v = (SceGxmProgram*)&texture2d_v;
static const SceGxmProgram *const gxm_program_texture2d_f = (SceGxmProgram*)&texture2d_f;

// Disable color buffer shader
static SceGxmShaderPatcherId disable_color_buffer_vertex_id;
static SceGxmShaderPatcherId disable_color_buffer_fragment_id;
static const SceGxmProgramParameter* disable_color_buffer_position;
static const SceGxmProgramParameter* disable_color_buffer_matrix;
static SceGxmVertexProgram* disable_color_buffer_vertex_program_patched;
static SceGxmFragmentProgram* disable_color_buffer_fragment_program_patched;
static position_vertex* depth_vertices = NULL;
static uint16_t* depth_indices = NULL;
static SceUID depth_vertices_uid, depth_indices_uid;

// Clear shader
static SceGxmShaderPatcherId clear_vertex_id;
static SceGxmShaderPatcherId clear_fragment_id;
static const SceGxmProgramParameter* clear_position;
static const SceGxmProgramParameter* clear_color;
static SceGxmVertexProgram* clear_vertex_program_patched;
static SceGxmFragmentProgram* clear_fragment_program_patched;
static clear_vertex* clear_vertices = NULL;
static uint16_t* clear_indices = NULL;
static SceUID clear_vertices_uid, clear_indices_uid;

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
static SceGxmFragmentProgram* rgba_fragment_programs[MAX_FRAGMENT_PROGS];

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
static SceGxmVertexProgram* texture2d_vertex_program_patched;
static SceGxmFragmentProgram* texture2d_fragment_program_patched;
static const SceGxmProgram* texture2d_fragment_program;
static SceGxmFragmentProgram* texture2d_fragment_programs[MAX_FRAGMENT_PROGS];

// Internal stuffs
static SceGxmBlendInfo fragment_program_info[MAX_FRAGMENT_PROGS];
static uint16_t release_idx = 0;
static SceGxmPrimitiveType prim;
static SceGxmPrimitiveTypeExtra prim_extra = SCE_GXM_PRIMITIVE_NONE;
static vertexList* model_vertices = NULL;
static vertexList* last = NULL;
static rgbaList* model_color = NULL;
static rgbaList* last2 = NULL;
static uvList* model_uv = NULL;
static uvList* last3 = NULL;
static glPhase phase = NONE;
static uint64_t vertex_count = 0;
static uint8_t drawing = 0;
static matrix4x4 projection_matrix;
static matrix4x4 modelview_matrix;
static GLboolean vblank = GL_TRUE;
static uint8_t np = 0xFF;
static float alpha_op = 7.0f;
static SceGxmBlendInfo cur_blend_info;

static GLenum error = GL_NO_ERROR; // Error global returned by glGetError
static GLuint buffers[BUFFERS_NUM]; // Buffers array
static gpubuffer gpu_buffers[BUFFERS_NUM]; // Buffers array

static texture_unit texture_units[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS];
static SceGxmBlendFunc blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD; // Current in-use RGB blend func
static SceGxmBlendFunc blend_func_a = SCE_GXM_BLEND_FUNC_ADD; // Current in-use A blend func
static SceGxmBlendFactor blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE; // Current in-use RGB source blend factor
static SceGxmBlendFactor blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO; // Current in-use RGB dest blend factor
static SceGxmBlendFactor blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE; // Current in-use A source blend factor
static SceGxmBlendFactor blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO; // Current in-use A dest blend factor
static SceGxmDepthFunc gxm_depth = SCE_GXM_DEPTH_FUNC_ALWAYS; // Current in-use depth test func
static SceGxmStencilOp stencil_fail_front = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when stencil test fails for front
static SceGxmStencilOp depth_fail_front = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when depth test fails for front
static SceGxmStencilOp depth_pass_front = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when depth test passes for front
static SceGxmStencilOp stencil_fail_back = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when stencil test fails for back
static SceGxmStencilOp depth_fail_back = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when depth test fails for back
static SceGxmStencilOp depth_pass_back = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when depth test passes for back
static SceGxmStencilFunc stencil_func_front = SCE_GXM_STENCIL_FUNC_ALWAYS; // Current in-use stencil function on front
static SceGxmStencilFunc stencil_func_back = SCE_GXM_STENCIL_FUNC_ALWAYS; // Current in-use stencil function on back
static SceGxmPolygonMode polygon_mode_front = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL; // Current in-use polygon mode for front
static SceGxmPolygonMode polygon_mode_back = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL; // Current in-use polygon mode for back
static uint8_t stencil_mask_front = 0xFF; // Current in-use mask for stencil test on front
static uint8_t stencil_mask_back = 0xFF; // Current in-use mask for stencil test on back
static uint8_t stencil_mask_front_write = 0xFF; // Current in-use mask for write stencil test on front
static uint8_t stencil_mask_back_write = 0xFF; // Current in-use mask for write stencil test on back
static uint8_t stencil_ref_front = 0; // Current in-use reference for stencil test on front
static uint8_t stencil_ref_back = 0; // Current in-use reference for stencil test on back
static GLdouble depth_value = 1.0f; // Current depth test value
static int8_t server_texture_unit = 0; // Current in-use server side texture unit
static int8_t client_texture_unit = 0; // Current in-use client side texture unit
static int texture2d_idx = 0; // Current in-use texture index for GL_TEXTURE2D
static int vertex_array_unit = -1; // Current in-use vertex array unit
static int index_array_unit = -1; // Current in-use index array unit
static matrix4x4* matrix = NULL; // Current in-use matrix mode
static vector4f clear_rgba_val; // Current clear color for glClear
static GLboolean depth_test_state = GL_FALSE; // Current state for GL_DEPTH_TEST
static GLboolean stencil_test_state = GL_FALSE; // Current state for GL_STENCIL_TEST
static GLboolean vertex_array_state = GL_FALSE; // Current state for GL_VERTEX_ARRAY
static GLboolean color_array_state = GL_FALSE; // Current state for GL_COLOR_ARRAY
static GLboolean texture_array_state = GL_FALSE; // Current state for GL_TEXTURE_COORD_ARRAY
static GLboolean scissor_test_state = GL_FALSE; // Current state for GL_SCISSOR_TEST
static GLboolean alpha_test_state = GL_FALSE; // Current state for GL_ALPHA_TEST
static GLboolean cull_face_state = GL_FALSE; // Current state for GL_CULL_FACE
static GLboolean blend_state = GL_FALSE; // Current state for GL_BLEND
static GLboolean depth_mask_state = GL_TRUE; // Current state for glDepthMask
static GLboolean pol_offset_fill = GL_FALSE; // Current state for GL_POLYGON_OFFSET_FILL
static GLboolean pol_offset_line = GL_FALSE; // Current state for GL_POLYGON_OFFSET_LINE
static GLboolean pol_offset_point = GL_FALSE; // Current state for GL_POLYGON_OFFSET_POINT
static GLenum alpha_func = GL_ALWAYS; // Current in-use alpha test mode
static GLfloat alpha_ref = 0.0f; // Current in-use alpha test reference value
static GLenum gl_cull_mode = GL_BACK; // Current in-use openGL cull mode
static GLenum gl_front_face = GL_CCW; // Current in-use openGL cull mode
static GLboolean no_polygons_mode = GL_FALSE; // GL_TRUE when cull mode to GL_FRONT_AND_BACK is set
static GLfloat pol_factor = 0.0f; // Current factor for glPolygonOffset
static GLfloat pol_units = 0.0f; // Current units for glPolygonOffset
static vector4f current_color = {1.0f, 1.0f, 1.0f, 1.0f}; // Current in-use color
static palette* color_table = NULL; // Current in-use color table
static SceGxmTextureAddrMode u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT; // Current in-use value for GL_TEXTURE_WRAP_S
static SceGxmTextureAddrMode v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT; // Current in-use value for GL_TEXTURE_WRAP_T

static matrix4x4 modelview_matrix_stack[MODELVIEW_STACK_DEPTH];
static uint8_t modelview_stack_counter = 0;
static matrix4x4 projection_matrix_stack[GENERIC_STACK_DEPTH];
static uint8_t projection_stack_counter = 0;

// Internal functions

static void* shader_patcher_host_alloc_cb(void *user_data, unsigned int size){
	return malloc(size);
}

static void shader_patcher_host_free_cb(void *user_data, void *mem){
	return free(mem);
}

static void display_queue_callback(const void *callbackData){
	SceDisplayFrameBuf display_fb;
	const struct display_queue_callback_data *cb_data = callbackData;

	memset(&display_fb, 0, sizeof(SceDisplayFrameBuf));
	display_fb.size = sizeof(SceDisplayFrameBuf);
	display_fb.base = cb_data->addr;
	display_fb.pitch = DISPLAY_STRIDE;
	display_fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	display_fb.width = DISPLAY_WIDTH;
	display_fb.height = DISPLAY_HEIGHT;

	sceDisplaySetFrameBuf(&display_fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
	
	if (vblank) sceDisplayWaitVblankStart();
	
}

static void release_unused_programs(){
	int i;
	if (cur_blend_info.colorMask == SCE_GXM_COLOR_MASK_NONE){
		for (i=0;i < release_idx;i++){
			if (fragment_program_info[i].colorMask != SCE_GXM_COLOR_MASK_NONE){
				sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, rgba_fragment_programs[i]);
				sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_programs[i]);
			}
		}
	}else{
		for (i=0;i < release_idx;i++){
			if (cur_blend_info.colorFunc != fragment_program_info[i].colorFunc ||
				cur_blend_info.alphaFunc != fragment_program_info[i].alphaFunc ||
				cur_blend_info.colorSrc != fragment_program_info[i].colorSrc ||
				cur_blend_info.alphaSrc != fragment_program_info[i].alphaFunc ||
				cur_blend_info.colorDst != fragment_program_info[i].colorDst ||
				cur_blend_info.alphaDst != fragment_program_info[i].alphaDst){
				sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, rgba_fragment_programs[i]);
				sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_programs[i]);
			}
		}
	}
	release_idx = 0;
}

static void _change_blend_factor(SceGxmBlendInfo* blend_info){
	if (release_idx == MAX_FRAGMENT_PROGS) return;
	
	int i;
	if (blend_info != NULL){
		for (i=0;i<release_idx;i++){
			if (blend_info->colorFunc == fragment_program_info[i].colorFunc &&
			blend_info->alphaFunc == fragment_program_info[i].alphaFunc &&
			blend_info->colorSrc == fragment_program_info[i].colorSrc &&
			blend_info->alphaSrc == fragment_program_info[i].alphaFunc &&
			blend_info->colorDst == fragment_program_info[i].colorDst &&
			blend_info->alphaDst == fragment_program_info[i].alphaDst){
				cur_blend_info = fragment_program_info[i];
				rgba_fragment_program_patched = rgba_fragment_programs[i];
				texture2d_fragment_program_patched = texture2d_fragment_programs[i];
				return;
			}
		}
	}else{
		for (i=0;i<release_idx;i++){
			if (fragment_program_info[i].colorMask == SCE_GXM_COLOR_MASK_NONE){
				cur_blend_info = fragment_program_info[i];
				rgba_fragment_program_patched = rgba_fragment_programs[i];
				texture2d_fragment_program_patched = texture2d_fragment_programs[i];
				return;
			}
		}
	}
	
	fragment_program_info[release_idx] = cur_blend_info;
	rgba_fragment_programs[release_idx] = rgba_fragment_program_patched;
	texture2d_fragment_programs[release_idx] = texture2d_fragment_program_patched;
	
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
		
	release_idx++;
}

static void update_alpha_test_settings(){
	if (alpha_test_state){
		switch (alpha_func){
			case GL_EQUAL:
				alpha_op = EQUAL * 1.0f;
				break;
			case GL_LEQUAL:
				alpha_op = LESS_EQUAL * 1.0f;
				break;
			case GL_GEQUAL:
				alpha_op = GREATER_EQUAL * 1.0f;
				break;
			case GL_LESS:
				alpha_op = LESS * 1.0f;
				break;
			case GL_GREATER:
				alpha_op = GREATER * 1.0f;
				break;
			case GL_NOTEQUAL:
				alpha_op = NOT_EQUAL * 1.0f;
				break;
			case GL_NEVER:
				alpha_op = NEVER * 1.0f;
				break;
			default:
				alpha_op = ALWAYS * 1.0f;
				break;
		}
	}else alpha_op = ALWAYS * 1.0f;
}

static void change_blend_factor(){
	SceGxmBlendInfo blend_info;
	memset(&blend_info, 0, sizeof(SceGxmBlendInfo));
	blend_info.colorMask = SCE_GXM_COLOR_MASK_ALL;
	blend_info.colorFunc = blend_func_rgb;
	blend_info.alphaFunc = blend_func_a;
	blend_info.colorSrc = blend_sfactor_rgb;
	blend_info.colorDst = blend_dfactor_rgb;
	blend_info.alphaSrc = blend_sfactor_a;
	blend_info.alphaDst = blend_dfactor_a;
	
	_change_blend_factor(&blend_info);
}

static void disable_blend(){
	_change_blend_factor(NULL);
	cur_blend_info.colorMask = SCE_GXM_COLOR_MASK_NONE;
}

static void change_depth_func(SceGxmDepthFunc func){
	sceGxmSetFrontDepthFunc(gxm_context, func);
	sceGxmSetBackDepthFunc(gxm_context, func);
}

static void change_depth_write(SceGxmDepthWriteMode mode){
	sceGxmSetFrontDepthWriteEnable(gxm_context, mode);
	sceGxmSetBackDepthWriteEnable(gxm_context, mode);
}

static void change_stencil_settings(){
	sceGxmSetFrontStencilFunc(gxm_context,
		stencil_func_front,
		stencil_fail_front,
		depth_fail_front,
		depth_pass_front,
		stencil_mask_front, stencil_mask_front_write);
	sceGxmSetBackStencilFunc(gxm_context,
		stencil_func_back,
		stencil_fail_back,
		depth_fail_back,
		depth_pass_back,
		stencil_mask_back, stencil_mask_back_write);
	sceGxmSetFrontStencilRef(gxm_context, stencil_ref_front);
	sceGxmSetBackStencilRef(gxm_context, stencil_ref_back);
}

static GLboolean change_stencil_config(SceGxmStencilOp* cfg, GLenum new){
	GLboolean ret = GL_TRUE;
	switch (new){
		case GL_KEEP:
			*cfg = SCE_GXM_STENCIL_OP_KEEP;
			break;
		case GL_ZERO:
			*cfg = SCE_GXM_STENCIL_OP_ZERO;
			break;
		case GL_REPLACE:
			*cfg = SCE_GXM_STENCIL_OP_REPLACE;
			break;
		case GL_INCR:
			*cfg = SCE_GXM_STENCIL_OP_INCR;
			break;
		case GL_INCR_WRAP:
			*cfg = SCE_GXM_STENCIL_OP_INCR_WRAP;
			break;
		case GL_DECR:
			*cfg = SCE_GXM_STENCIL_OP_DECR;
			break;
		case GL_DECR_WRAP:
			*cfg = SCE_GXM_STENCIL_OP_DECR_WRAP;
			break;
		case GL_INVERT:
			*cfg = SCE_GXM_STENCIL_OP_INVERT;
			break;
		default:
			ret = GL_FALSE;
			break;
	}
	return ret;
}

static GLboolean change_stencil_func_config(SceGxmStencilFunc* cfg, GLenum new){
	GLboolean ret = GL_TRUE;
	switch (new){
		case GL_NEVER:
			*cfg = SCE_GXM_STENCIL_FUNC_NEVER;
			break;
		case GL_LESS:
			*cfg = SCE_GXM_STENCIL_FUNC_LESS;
			break;
		case GL_LEQUAL:
			*cfg = SCE_GXM_STENCIL_FUNC_LESS_EQUAL;
			break;
		case GL_GREATER:
			*cfg = SCE_GXM_STENCIL_FUNC_GREATER;
			break;
		case GL_GEQUAL:
			*cfg = SCE_GXM_STENCIL_FUNC_GREATER_EQUAL;
			break;
		case GL_EQUAL:
			*cfg = SCE_GXM_STENCIL_FUNC_EQUAL;
			break;
		case GL_NOTEQUAL:
			*cfg = SCE_GXM_STENCIL_FUNC_NOT_EQUAL;
			break;
		case GL_ALWAYS:
			*cfg = SCE_GXM_STENCIL_FUNC_ALWAYS;
			break;
		default:
			ret = GL_FALSE;
			break;
	}
	return ret;
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
			if (pol_offset_line) sceGxmSetFrontDepthBias(gxm_context, pol_factor, pol_units);
			else sceGxmSetFrontDepthBias(gxm_context, 0.0f, 0.0f);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_POINT:
			if (pol_offset_point) sceGxmSetFrontDepthBias(gxm_context, pol_factor, pol_units);
			else sceGxmSetFrontDepthBias(gxm_context, 0.0f, 0.0f);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_FILL:
			if (pol_offset_fill) sceGxmSetFrontDepthBias(gxm_context, pol_factor, pol_units);
			else sceGxmSetFrontDepthBias(gxm_context, 0.0f, 0.0f);
			break;
	}
	switch (polygon_mode_back){
		case SCE_GXM_POLYGON_MODE_TRIANGLE_LINE:
			if (pol_offset_line) sceGxmSetBackDepthBias(gxm_context, pol_factor, pol_units);
			else sceGxmSetBackDepthBias(gxm_context, 0.0f, 0.0f);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_POINT:
			if (pol_offset_point) sceGxmSetBackDepthBias(gxm_context, pol_factor, pol_units);
			else sceGxmSetBackDepthBias(gxm_context, 0.0f, 0.0f);
			break;
		case SCE_GXM_POLYGON_MODE_TRIANGLE_FILL:
			if (pol_offset_fill) sceGxmSetBackDepthBias(gxm_context, pol_factor, pol_units);
			else sceGxmSetBackDepthBias(gxm_context, 0.0f, 0.0f);
			break;
	}	
}

// vitaGL specific functions

void vglStartRendering(){
	sceGxmBeginScene(
		gxm_context, 0, gxm_render_target,
		NULL, NULL,
		gxm_sync_objects[gxm_back_buffer_index],
		&gxm_color_surfaces[gxm_back_buffer_index],
		&gxm_depth_stencil_surface);
}

void vglStopRendering(){
	sceGxmEndScene(gxm_context, NULL, NULL);
	sceGxmFinish(gxm_context);
	sceGxmPadHeartbeat(&gxm_color_surfaces[gxm_back_buffer_index], gxm_sync_objects[gxm_back_buffer_index]);
	struct display_queue_callback_data queue_cb_data;
	queue_cb_data.addr = gxm_color_surfaces_addr[gxm_back_buffer_index];
	sceGxmDisplayQueueAddEntry(gxm_sync_objects[gxm_front_buffer_index],
	gxm_sync_objects[gxm_back_buffer_index], &queue_cb_data);
	gxm_front_buffer_index = gxm_back_buffer_index;
	gxm_back_buffer_index = (gxm_back_buffer_index + 1) % DISPLAY_BUFFER_COUNT;
	gpu_pool_reset();
	release_unused_programs();
}

void vglInit(uint32_t gpu_pool_size){
	
	SceGxmInitializeParams gxm_init_params;
	memset(&gxm_init_params, 0, sizeof(SceGxmInitializeParams));
	
	gxm_init_params.flags = 0;
	gxm_init_params.displayQueueMaxPendingCount = DISPLAY_BUFFER_COUNT - 1;
	gxm_init_params.displayQueueCallback = display_queue_callback;
	gxm_init_params.displayQueueCallbackDataSize = sizeof(struct display_queue_callback_data);
	gxm_init_params.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;
	
	sceGxmInitialize(&gxm_init_params);
	
	vdm_ring_buffer_addr = gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ, SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE,
		&vdm_ring_buffer_uid);

	vertex_ring_buffer_addr = gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ, SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE,
		&vertex_ring_buffer_uid);

	fragment_ring_buffer_addr = gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ, SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE,
		&fragment_ring_buffer_uid);

	unsigned int fragment_usse_offset;
	fragment_usse_ring_buffer_addr = gpu_fragment_usse_alloc_map(
		SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE,
		&fragment_ring_buffer_uid, &fragment_usse_offset);

	SceGxmContextParams gxm_context_params;
	memset(&gxm_context_params, 0, sizeof(SceGxmContextParams));
	gxm_context_params.hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
	gxm_context_params.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	gxm_context_params.vdmRingBufferMem = vdm_ring_buffer_addr;
	gxm_context_params.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
	gxm_context_params.vertexRingBufferMem = vertex_ring_buffer_addr;
	gxm_context_params.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
	gxm_context_params.fragmentRingBufferMem = fragment_ring_buffer_addr;
	gxm_context_params.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
	gxm_context_params.fragmentUsseRingBufferMem = fragment_usse_ring_buffer_addr;
	gxm_context_params.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
	gxm_context_params.fragmentUsseRingBufferOffset = fragment_usse_offset;

	sceGxmCreateContext(&gxm_context_params, &gxm_context);

	SceGxmRenderTargetParams render_target_params;
	memset(&render_target_params, 0, sizeof(SceGxmRenderTargetParams));
	render_target_params.flags = 0;
	render_target_params.width = DISPLAY_WIDTH;
	render_target_params.height = DISPLAY_HEIGHT;
	render_target_params.scenesPerFrame = 1;
	render_target_params.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
	render_target_params.multisampleLocations = 0;
	render_target_params.driverMemBlock = -1;
	
	sceGxmCreateRenderTarget(&render_target_params, &gxm_render_target);
	
	int i;
	for (i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		gxm_color_surfaces_addr[i] = gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
			SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
			ALIGN(4 * DISPLAY_STRIDE * DISPLAY_HEIGHT, 1 * 1024 * 1024),
			&gxm_color_surfaces_uid[i]);

		memset(gxm_color_surfaces_addr[i], 0, DISPLAY_STRIDE * DISPLAY_HEIGHT);

		sceGxmColorSurfaceInit(&gxm_color_surfaces[i],
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			DISPLAY_WIDTH,
			DISPLAY_HEIGHT,
			DISPLAY_STRIDE,
			gxm_color_surfaces_addr[i]);

		sceGxmSyncObjectCreate(&gxm_sync_objects[i]);
	}
	
	unsigned int depth_stencil_width = ALIGN(DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX);
	unsigned int depth_stencil_height = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
	unsigned int depth_stencil_samples = depth_stencil_width * depth_stencil_height;

	gxm_depth_stencil_surface_addr = gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
		4 * depth_stencil_samples, &gxm_depth_stencil_surface_uid);

	sceGxmDepthStencilSurfaceInit(&gxm_depth_stencil_surface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		depth_stencil_width,
		gxm_depth_stencil_surface_addr,
		NULL);
		
	static const unsigned int shader_patcher_buffer_size = 64 * 1024;
	static const unsigned int shader_patcher_vertex_usse_size = 64 * 1024;
	static const unsigned int shader_patcher_fragment_usse_size = 64 * 1024;
	
	gxm_shader_patcher_buffer_addr = gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_READ,
		shader_patcher_buffer_size, &gxm_shader_patcher_buffer_uid);

	unsigned int shader_patcher_vertex_usse_offset;
	gxm_shader_patcher_vertex_usse_addr = gpu_vertex_usse_alloc_map(
		shader_patcher_vertex_usse_size, &gxm_shader_patcher_vertex_usse_uid,
		&shader_patcher_vertex_usse_offset);

	unsigned int shader_patcher_fragment_usse_offset;
	gxm_shader_patcher_fragment_usse_addr = gpu_fragment_usse_alloc_map(
		shader_patcher_fragment_usse_size, &gxm_shader_patcher_fragment_usse_uid,
		&shader_patcher_fragment_usse_offset);

	SceGxmShaderPatcherParams shader_patcher_params;
	memset(&shader_patcher_params, 0, sizeof(SceGxmShaderPatcherParams));
	shader_patcher_params.userData = NULL;
	shader_patcher_params.hostAllocCallback = shader_patcher_host_alloc_cb;
	shader_patcher_params.hostFreeCallback = shader_patcher_host_free_cb;
	shader_patcher_params.bufferAllocCallback = NULL;
	shader_patcher_params.bufferFreeCallback = NULL;
	shader_patcher_params.bufferMem = gxm_shader_patcher_buffer_addr;
	shader_patcher_params.bufferMemSize = shader_patcher_buffer_size;
	shader_patcher_params.vertexUsseAllocCallback = NULL;
	shader_patcher_params.vertexUsseFreeCallback = NULL;
	shader_patcher_params.vertexUsseMem = gxm_shader_patcher_vertex_usse_addr;
	shader_patcher_params.vertexUsseMemSize = shader_patcher_vertex_usse_size;
	shader_patcher_params.vertexUsseOffset = shader_patcher_vertex_usse_offset;
	shader_patcher_params.fragmentUsseAllocCallback = NULL;
	shader_patcher_params.fragmentUsseFreeCallback = NULL;
	shader_patcher_params.fragmentUsseMem = gxm_shader_patcher_fragment_usse_addr;
	shader_patcher_params.fragmentUsseMemSize = shader_patcher_fragment_usse_size;
	shader_patcher_params.fragmentUsseOffset = shader_patcher_fragment_usse_offset;
	
	sceGxmShaderPatcherCreate(&shader_patcher_params, &gxm_shader_patcher);
	
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
		
	depth_vertices = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		4 * sizeof(struct position_vertex), &depth_vertices_uid);

	depth_indices = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		4 * sizeof(unsigned short), &depth_indices_uid);

	depth_vertices[0].position = (vector3f){-1.0f, -1.0f, 1.0f};
	depth_vertices[1].position = (vector3f){ 1.0f, -1.0f, 1.0f};
	depth_vertices[2].position = (vector3f){-1.0f,  1.0f, 1.0f};
	depth_vertices[3].position = (vector3f){ 1.0f,  1.0f, 1.0f};

	depth_indices[0] = 0;
	depth_indices[1] = 1;
	depth_indices[2] = 2;
	depth_indices[3] = 3;
	
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

	clear_vertices = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		4 * sizeof(struct clear_vertex), &clear_vertices_uid);

	clear_indices = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		4 * sizeof(unsigned short), &clear_indices_uid);

	clear_vertices[0].position = (vector2f){-1.0f, -1.0f};
	clear_vertices[1].position = (vector2f){ 1.0f, -1.0f};
	clear_vertices[2].position = (vector2f){-1.0f,  1.0f};
	clear_vertices[3].position = (vector2f){ 1.0f,  1.0f};

	clear_indices[0] = 0;
	clear_indices[1] = 1;
	clear_indices[2] = 2;
	clear_indices[3] = 3;
	
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
	
	sceGxmSetTwoSidedEnable(gxm_context, SCE_GXM_TWO_SIDED_ENABLED);
	
	// Allocate temp pool for non-VBO drawing
	gpu_pool_init(gpu_pool_size);
	
	// Init texture units
	int j;
	for (i=0; i < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++){
		for (j=0; j < TEXTURES_NUM; j++){
			texture_units[i].textures[j].used = 0;
			texture_units[i].textures[j].valid = 0;
		}
		texture_units[i].env_mode = MODULATE;
	}
	
	// Init buffers
	for (i=0; i < BUFFERS_NUM; i++){
		buffers[i] = BUFFERS_ADDR + i;
		gpu_buffers[i].ptr = NULL;
	}
	
	// Setting up current blend info state
	cur_blend_info.colorMask = SCE_GXM_COLOR_MASK_NONE;
	
}

void vglEnd(void){
	sceGxmDisplayQueueFinish();
	sceGxmFinish(gxm_context);
	gpu_unmap_free(clear_vertices_uid);
	gpu_unmap_free(clear_indices_uid);
	gpu_unmap_free(depth_vertices_uid);
	gpu_unmap_free(depth_indices_uid);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, disable_color_buffer_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, disable_color_buffer_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, clear_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, clear_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, rgba_vertex_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, rgb_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, rgba_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, texture2d_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_program_patched);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgb_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgba_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgba_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, texture2d_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, texture2d_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, disable_color_buffer_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, disable_color_buffer_fragment_id);
	sceGxmShaderPatcherDestroy(gxm_shader_patcher);
	gpu_unmap_free(gxm_shader_patcher_buffer_uid);
	gpu_vertex_usse_unmap_free(gxm_shader_patcher_vertex_usse_uid);
	gpu_fragment_usse_unmap_free(gxm_shader_patcher_fragment_usse_uid);
	gpu_unmap_free(gxm_depth_stencil_surface_uid);
	int i;
	for (i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		gpu_unmap_free(gxm_color_surfaces_uid[i]);
		sceGxmSyncObjectDestroy(gxm_sync_objects[i]);
	}
	sceGxmDestroyRenderTarget(gxm_render_target);
	gpu_unmap_free(vdm_ring_buffer_uid);
	gpu_unmap_free(vertex_ring_buffer_uid);
	gpu_unmap_free(fragment_ring_buffer_uid);
	gpu_fragment_usse_unmap_free(fragment_usse_ring_buffer_uid);
	sceGxmDestroyContext(gxm_context);
	sceGxmTerminate();
}

void vglWaitVblankStart(GLboolean enable){
	vblank = enable;
}

// openGL implementation

GLenum glGetError(void){
	return error;
}

void glClear(GLbitfield mask){
	if ((mask & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT){
		change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
		change_depth_func(SCE_GXM_DEPTH_FUNC_ALWAYS);
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
		sceGxmSetVertexProgram(gxm_context, clear_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, clear_fragment_program_patched);
		void *color_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &color_buffer);
		sceGxmSetUniformDataF(color_buffer, clear_color, 0, 4, &clear_rgba_val.r);
		sceGxmSetVertexStream(gxm_context, 0, clear_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, clear_indices, 4);
		change_depth_write(depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		change_depth_func(gxm_depth);
		sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front);
		sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back);
		drawing = 1;
	}
	if ((mask & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT){
		change_depth_write(SCE_GXM_DEPTH_WRITE_ENABLED);
		change_depth_func(SCE_GXM_DEPTH_FUNC_ALWAYS);
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
		depth_vertices[0].position.z = depth_value;
		depth_vertices[1].position.z = depth_value;
		depth_vertices[2].position.z = depth_value;
		depth_vertices[3].position.z = depth_value;
		sceGxmSetVertexProgram(gxm_context, disable_color_buffer_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, disable_color_buffer_fragment_program_patched);
		sceGxmSetVertexStream(gxm_context, 0, depth_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, depth_indices, 4);
		change_depth_write(depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		change_depth_func(gxm_depth);
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
			change_depth_func(SCE_GXM_DEPTH_FUNC_LESS);
			depth_test_state = GL_TRUE;
			break;
		case GL_STENCIL_TEST:
			change_stencil_settings();
			stencil_test_state = GL_TRUE;
			break;
		case GL_BLEND:
			change_blend_factor();
			blend_state = GL_TRUE;
			break;
		case GL_SCISSOR_TEST:
			scissor_test_state = GL_TRUE;
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
			change_depth_func(SCE_GXM_DEPTH_FUNC_ALWAYS);
			depth_test_state = GL_FALSE;
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
			disable_blend();
			blend_state = GL_FALSE;
			break;
		case GL_SCISSOR_TEST:
			scissor_test_state = GL_FALSE;
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
	
	matrix4x4 mvp_matrix;
	matrix4x4 final_mvp_matrix;
	
	matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
	matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
	
	uint8_t use_texture = 0;
	if ((server_texture_unit >= 0) && (server_texture_unit >= 0) && (texture_units[server_texture_unit].enabled) && (model_uv != NULL) && (texture_units[server_texture_unit].textures[texture2d_idx].valid)){
		switch (texture_units[server_texture_unit].textures[texture2d_idx].type){
			case GL_RGB:
			case GL_RGBA:
			case GL_RG:
			case GL_COLOR_INDEX:
			case GL_VITA2D_TEXTURE:
				sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
				void* alpha_buffer;
				sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
				sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
				sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_op);
				sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
				float tex_env_mode = texture_units[server_texture_unit].env_mode * 1.0f;
				sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &tex_env_mode);
				break;
			default:
				purge_vertex_list();
				vertex_count = 0;
				return;
				break;
		}
		use_texture = 1;
	}else{
		sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
	}
	
	int i, j;
	void* vertex_wvp_buffer;
	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
	if (use_texture){
		sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
		sceGxmTextureSetUAddrMode(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, u_mode);
		sceGxmTextureSetVAddrMode(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, v_mode);
		sceGxmSetFragmentTexture(gxm_context, 0, &texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex);
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
		sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)final_mvp_matrix);
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

void glGenTextures(GLsizei n, GLuint* res){
	int i, j=0;
	if (n < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	for (i=0; i < TEXTURES_NUM; i++){
		if (!(texture_units[server_texture_unit].textures[i].used)){
			res[j++] = i;
			texture_units[server_texture_unit].textures[i].used = 1;
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

void glBindTexture(GLenum target, GLuint texture){
	switch (target){
		case GL_TEXTURE_2D:
			texture2d_idx = texture;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glDeleteTextures(GLsizei n, const GLuint* gl_textures){
	if (n < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	int j;
	for (j=0; j<n; j++){
		GLuint i = gl_textures[j];
		texture_units[server_texture_unit].textures[i].used = 0;
		gpu_free_texture(&texture_units[server_texture_unit].textures[i]);
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
				gpu_unmap_free(gpu_buffers[idx].data_UID);
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
	gpu_buffers[idx].ptr = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ,
		size,
		&gpu_buffers[idx].data_UID);
	memcpy(gpu_buffers[idx].ptr, data, size);
}

void glColorTable(GLenum target,  GLenum internalformat,  GLsizei width,  GLenum format,  GLenum type,  const GLvoid * data){
	if (color_table != NULL){
		gpu_free_palette(color_table);
		color_table = NULL;
	}
	uint8_t bpe = 0;
	switch (target){
		case GL_COLOR_TABLE:
			switch (format){
				case GL_RGBA:
					bpe = 4;
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
	color_table = gpu_alloc_palette(data, width, bpe);
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data){
	SceGxmTextureFormat tex_format;
	switch (target){
		case GL_TEXTURE_2D:
			switch (internalFormat){
				case GL_RGB:
					if (format == GL_RGB){
						switch (type){
							case GL_UNSIGNED_BYTE:
								tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8X8_BGR1;
								break;
							case GL_UNSIGNED_SHORT_5_6_5:
								tex_format = SCE_GXM_TEXTURE_FORMAT_U5U6U5_BGR;
								break;
							default:
								error = GL_INVALID_ENUM;
								break;	
						}
					}
					break;
				case GL_RGBA:
					if (format == GL_RGBA){
						switch (type){
							case GL_UNSIGNED_BYTE:
								tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
								break;
							case GL_UNSIGNED_SHORT_4_4_4_4:
								tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_ABGR;
								break;
							default:
								error = GL_INVALID_ENUM;
								break;
						}
					}
					break;
				case GL_LUMINANCE:
					if (format == GL_LUMINANCE){
						switch (type){
							case GL_UNSIGNED_BYTE:
								tex_format = SCE_GXM_TEXTURE_FORMAT_L8;
								break;
							default:
								error = GL_INVALID_ENUM;
								break;
						}
					}
					break;
				case GL_LUMINANCE_ALPHA:
					if (format == GL_LUMINANCE){
						switch (type){
							case GL_UNSIGNED_BYTE:
								tex_format = SCE_GXM_TEXTURE_FORMAT_L8A8;
								break;
							default:
								error = GL_INVALID_ENUM;
								break;
						}
					}
					break;
				case GL_ALPHA:
					if (format == GL_ALPHA){
						switch (type){
							case GL_UNSIGNED_BYTE:
								tex_format = SCE_GXM_TEXTURE_FORMAT_U8_R;
								break;
							default:
								error = GL_INVALID_ENUM;
								break;
						}
					}
				case GL_COLOR_INDEX8_EXT:
					if (format == GL_COLOR_INDEX){
						switch (type){
							case GL_UNSIGNED_BYTE:
								tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
								break;
							default:
								error = GL_INVALID_ENUM;
								break;	
						}
					}
					break;
				default:
					switch (format){
						case GL_RED:
							switch (type){
								case GL_BYTE:
									tex_format = SCE_GXM_TEXTURE_FORMAT_S8_R;
									break;
								case GL_UNSIGNED_BYTE:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U8_R;
									break;
								default:
									error = GL_INVALID_ENUM;
									break;	
							}
							break;
						case GL_RG:
							switch (type){
								default:
									error = GL_INVALID_ENUM;
									break;	
							}
							break;
						case GL_RGB:
							switch (type){
								case GL_BYTE:
									tex_format = SCE_GXM_TEXTURE_FORMAT_S8S8S8X8_BGR1;
									break;
								case GL_UNSIGNED_BYTE:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8X8_BGR1;
									break;
								case GL_UNSIGNED_SHORT_4_4_4_4:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4X4_BGR1;
									break;
								case GL_UNSIGNED_SHORT_5_5_5_1:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5X1_BGR1;
									break;
								case GL_UNSIGNED_SHORT_5_6_5:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U5U6U5_BGR;
									break;
								default:
									error = GL_INVALID_ENUM;
									break;
							}
							break;
						case GL_RGBA:
							switch (type){
								case GL_BYTE:
									tex_format = SCE_GXM_TEXTURE_FORMAT_S8S8S8S8_ABGR;
									break;
								case GL_UNSIGNED_BYTE:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
									break;
								case GL_UNSIGNED_SHORT_4_4_4_4:
									tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_ABGR;
									break;
								default:
									error = GL_INVALID_ENUM;
									break;
							}					
							break;
						case GL_VITA2D_TEXTURE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_A8B8G8R8;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					break;
			}
			if (width > GXM_TEX_MAX_SIZE || height > GXM_TEX_MAX_SIZE){
				error = GL_INVALID_VALUE;
				return;
			}
			texture_units[server_texture_unit].textures[texture2d_idx].type = format;
			if (level == 0) gpu_alloc_texture(width, height, tex_format, data, &texture_units[server_texture_unit].textures[texture2d_idx]);
			else gpu_alloc_mipmaps(width, height, tex_format, data, level, &texture_units[server_texture_unit].textures[texture2d_idx]);
			if (texture_units[server_texture_unit].textures[texture2d_idx].valid && texture_units[server_texture_unit].textures[texture2d_idx].palette_UID) sceGxmTextureSetPalette(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, color_table->data);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;	
	}
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels){
	SceGxmTextureFormat tex_format;
	switch (target){
		case GL_TEXTURE_2D:
			switch (format){
				case GL_RGB:
					switch (type){
						case GL_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_S8S8S8X8_RGB1;
							break;
						case GL_UNSIGNED_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8X8_RGB1;
							break;
						case GL_UNSIGNED_SHORT_4_4_4_4:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4X4_RGB1;
							break;
						case GL_UNSIGNED_SHORT_5_5_5_1:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5X1_RGB1;
							break;
						case GL_UNSIGNED_SHORT_5_6_5:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					break;
				case GL_RGBA:
					switch (type){
						case GL_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_S8S8S8S8_RGBA;
							break;
						case GL_UNSIGNED_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA;
							break;
						case GL_UNSIGNED_SHORT_4_4_4_4:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA;
							break;
						case GL_UNSIGNED_SHORT_5_5_5_1:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}					
					break;
				case GL_VITA2D_TEXTURE:
					tex_format = SCE_GXM_TEXTURE_FORMAT_A8B8G8R8;
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			if (width > GXM_TEX_MAX_SIZE || height > GXM_TEX_MAX_SIZE){
				error = GL_INVALID_VALUE;
				return;
			}
			/*gpu_prepare_rendertarget(&texture_units[server_texture_unit].textures[texture2d_idx]);
			texture temp;
			gpu_alloc_texture(width, height, tex_format, pixels, &temp);
			matrix4x4 view_matrix, identity_matrix;
			matrix4x4_identity(identity_matrix);
			uint32_t tw = sceGxmTextureGetWidth(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex);
			uint32_t th = sceGxmTextureGetHeight(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex);
			matrix4x4_init_orthographic(view_matrix, 0, tw, th, 0, -1, 1);
			sceGxmBeginScene(gxm_context, SCE_GXM_SCENE_FRAGMENT_SET_DEPENDENCY,
				texture_units[server_texture_unit].textures[texture2d_idx].gxm_rtgt,
				NULL, NULL, NULL,
				&texture_units[server_texture_unit].textures[texture2d_idx].gxm_sfc,
				&texture_units[server_texture_unit].textures[texture2d_idx].gxm_sfd);
			sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
			sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
			matrix4x4 mvp_matrix;
			matrix4x4 final_mvp_matrix;
			matrix4x4_multiply(mvp_matrix, view_matrix, identity_matrix);
			matrix4x4_transpose(final_mvp_matrix, mvp_matrix);
			sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
			sceGxmSetFragmentTexture(gxm_context, 0, &temp.gxm_tex);
			vector3f* vertices = (vector3f*)gpu_pool_memalign(4 * sizeof(vector3f), sizeof(vector3f));
			vector2f* uv_map = (vector2f*)gpu_pool_memalign(4 * sizeof(vector2f), sizeof(vector2f));
			uint16_t* indices = (uint16_t*)gpu_pool_memalign(4 * sizeof(uint16_t), sizeof(uint16_t));
			vertices[0] = (vector3f){xoffset,yoffset,0.5f};
			vertices[1] = (vector3f){xoffset+width,yoffset,0.5f};
			vertices[2] = (vector3f){xoffset,yoffset+height,0.5f};
			vertices[3] = (vector3f){xoffset+width,yoffset+height,0.5f};
			uv_map[0] = (vector2f){0,0};
			uv_map[1] = (vector2f){1,0};
			uv_map[2] = (vector2f){0,1};
			uv_map[3] = (vector2f){1,1};
			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			indices[3] = 3;
			sceGxmSetVertexStream(gxm_context, 0, vertices);
			sceGxmSetVertexStream(gxm_context, 1, uv_map);
			sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, indices, 4);
			sceGxmEndScene(gxm_context, NULL, NULL);
			sceGxmFinish(gxm_context);
			gpu_free_texture(&temp);
			gpu_destroy_rendertarget(&texture_units[server_texture_unit].textures[texture2d_idx]);*/
			break;
		default:
			error = GL_INVALID_ENUM;
			break;	
	}
}

void glTexParameteri(GLenum target, GLenum pname, GLint param){
	switch (target){
		case GL_TEXTURE_2D:
			switch (pname){
				case GL_TEXTURE_MIN_FILTER:
					switch (param){
						case GL_NEAREST:
							sceGxmTextureSetMinFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
							break;
						case GL_LINEAR:
							sceGxmTextureSetMinFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
							break;
						case GL_NEAREST_MIPMAP_NEAREST:
							break;
						case GL_LINEAR_MIPMAP_NEAREST:
							break;
						case GL_NEAREST_MIPMAP_LINEAR:
							break;
						case GL_LINEAR_MIPMAP_LINEAR:
							break;
					}
					break;
				case GL_TEXTURE_MAG_FILTER:
					switch (param){
						case GL_NEAREST:
							sceGxmTextureSetMagFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
							break;
						case GL_LINEAR:
							sceGxmTextureSetMagFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
							break;
						case GL_NEAREST_MIPMAP_NEAREST:
							break;
						case GL_LINEAR_MIPMAP_NEAREST:
							break;
						case GL_NEAREST_MIPMAP_LINEAR:
							break;
						case GL_LINEAR_MIPMAP_LINEAR:
							break;
					}
					break;
				case GL_TEXTURE_WRAP_S:
					switch (param){
						case GL_CLAMP_TO_EDGE:
							u_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
							break;
						case GL_REPEAT: 
							u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
							break;
						case GL_MIRRORED_REPEAT:
							u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					break;
				case GL_TEXTURE_WRAP_T:
					switch (param){
						case GL_CLAMP_TO_EDGE:
							v_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
							break;
						case GL_REPEAT: 
							v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
							break;
						case GL_MIRRORED_REPEAT:
							v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
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
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param){
	switch (target){
		case GL_TEXTURE_2D:
			switch (pname){
				case GL_TEXTURE_MIN_FILTER:
					if (param == GL_NEAREST) sceGxmTextureSetMinFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
					else if (param == GL_LINEAR) sceGxmTextureSetMinFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
					break;
				case GL_TEXTURE_MAG_FILTER:
					if (param == GL_NEAREST) sceGxmTextureSetMagFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
					else if (param == GL_LINEAR) sceGxmTextureSetMagFilter(&texture_units[server_texture_unit].textures[texture2d_idx].gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);	
					break;
				case GL_TEXTURE_WRAP_S:
					if (param == GL_CLAMP_TO_EDGE) u_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
					else if (param == GL_REPEAT) u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
					else if (param == GL_MIRRORED_REPEAT) u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
					break;
				case GL_TEXTURE_WRAP_T:
					if (param == GL_CLAMP_TO_EDGE) v_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
					else if (param == GL_REPEAT) v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
					else if (param == GL_MIRRORED_REPEAT) v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
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

void glActiveTexture(GLenum texture){
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE31)) error = GL_INVALID_ENUM;
	else server_texture_unit = texture - GL_TEXTURE0;
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
		}
	}
}

void glLoadIdentity(void){
	matrix4x4_identity(*matrix);
}

void glMatrixMode(GLenum mode){
	switch (mode){
		case GL_MODELVIEW:
			matrix = &modelview_matrix;
			break;
		case GL_PROJECTION:
			matrix = &projection_matrix;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glViewport(GLint x,  GLint y,  GLsizei width,  GLsizei height){
	if ((width < 0) || (height < 0)){
		error = GL_INVALID_VALUE;
		return;
	}
	sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_NONE, x, y, x+width, y+height);
}

void glScissor(GLint x,  GLint y,  GLsizei width,  GLsizei height){
	if (scissor_test_state){
		if ((width < 0) || (height < 0)){
			error = GL_INVALID_VALUE;
			return;
		}
		sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_NONE, x, y, x+width, y+height);
	}
}

void glOrtho(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}else if ((left == right) || (bottom == top) || (nearVal == farVal)){
		error = GL_INVALID_VALUE;
		return;
	}
	matrix4x4_init_orthographic(*matrix, left, right, bottom, top, nearVal, farVal);
}

void glFrustum(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}else if ((left == right) || (bottom == top) || (nearVal < 0) || (farVal < 0)){
		error = GL_INVALID_VALUE;
		return;
	}
	matrix4x4_init_frustum(*matrix, left, right, bottom, top, nearVal, farVal);
}

void glMultMatrixf(const GLfloat* m){
	matrix4x4 res;
	matrix4x4 tmp;
	int i,j;
	for (i=0;i<4;i++){
		for (j=0;j<4;j++){
			tmp[i][j] = m[i*4+j];
		}
	}
	matrix4x4_multiply(res, *matrix, tmp);
	matrix4x4_copy(*matrix, res);
}

void glLoadMatrixf(const GLfloat* m){
	matrix4x4 tmp;
	int i,j;
	for (i=0;i<4;i++){
		for (j=0;j<4;j++){
			tmp[i][j] = m[i*4+j];
		}
	}
	matrix4x4_copy(*matrix, tmp);
}

void glTranslatef(GLfloat x,  GLfloat y,  GLfloat z){
	matrix4x4_translate(*matrix, x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z){
	matrix4x4_scale(*matrix, x, y, z);
}

void glRotatef(GLfloat angle,  GLfloat x,  GLfloat y,  GLfloat z){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	float rad = DEG_TO_RAD(angle);
	if (x == 1.0f){
		matrix4x4_rotate_x(*matrix, rad);
	}
	if (y == 1.0f){
		matrix4x4_rotate_y(*matrix, rad);
	}
	if (z == 1.0f){
		matrix4x4_rotate_z(*matrix, rad);
	}
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

void glDepthFunc(GLenum func){
	switch (func){
		case GL_NEVER:
			gxm_depth = SCE_GXM_DEPTH_FUNC_NEVER;
			break;
		case GL_LESS:
			gxm_depth = SCE_GXM_DEPTH_FUNC_LESS;
			break;
		case GL_EQUAL:
			gxm_depth = SCE_GXM_DEPTH_FUNC_EQUAL;
			break;
		case GL_LEQUAL:
			gxm_depth = SCE_GXM_DEPTH_FUNC_LESS_EQUAL;
			break;
		case GL_GREATER:
			gxm_depth = SCE_GXM_DEPTH_FUNC_GREATER;
			break;
		case GL_NOTEQUAL:
			gxm_depth = SCE_GXM_DEPTH_FUNC_NOT_EQUAL;
			break;
		case GL_GEQUAL:
			gxm_depth = SCE_GXM_DEPTH_FUNC_GREATER_EQUAL;
			break;
		case GL_ALWAYS:
			gxm_depth = SCE_GXM_DEPTH_FUNC_ALWAYS;
			break;
	}
	change_depth_func(gxm_depth);
}

void glClearDepth(GLdouble depth){
	depth_value = depth;
}

void glDepthMask(GLboolean flag){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	depth_mask_state = flag;
	change_depth_write(depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
}

void glAlphaFunc(GLenum func, GLfloat ref){
	alpha_func = func;
	alpha_ref = ref;
	update_alpha_test_settings();
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
		default:
			error = GL_INVALID_ENUM;
			break;
	}
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
		default:
			error = GL_INVALID_ENUM;
			break;
	}
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
}

void glStencilOpSeparate(GLenum face, GLenum sfail,  GLenum dpfail,  GLenum dppass){
	switch (face){
		case GL_FRONT:
			if (!change_stencil_config(&stencil_fail_front, sfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_fail_front, dpfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_pass_front, dppass)) error = GL_INVALID_ENUM;
			break;
		case GL_BACK:
			if (!change_stencil_config(&stencil_fail_back, sfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_fail_back, dpfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_pass_front, dppass)) error = GL_INVALID_ENUM;
			break;
		case GL_FRONT_AND_BACK:
			if (!change_stencil_config(&stencil_fail_front, sfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&stencil_fail_back, sfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_fail_front, dpfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_fail_back, dpfail)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_pass_front, dppass)) error = GL_INVALID_ENUM;
			if (!change_stencil_config(&depth_pass_back, dppass)) error = GL_INVALID_ENUM;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	change_stencil_settings();
}

void glStencilOp(GLenum sfail,  GLenum dpfail,  GLenum dppass){
	glStencilOpSeparate(GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask){
	switch (face){
		case GL_FRONT:
			if (!change_stencil_func_config(&stencil_func_front, func)) error = GL_INVALID_ENUM;
			stencil_mask_front = mask;
			stencil_ref_front = ref;
			break;
		case GL_BACK:
			if (!change_stencil_func_config(&stencil_func_back, func)) error = GL_INVALID_ENUM;
			stencil_mask_back = mask;
			stencil_ref_back = ref;
			break;
		case GL_FRONT_AND_BACK:
			if (!change_stencil_func_config(&stencil_func_front, func)) error = GL_INVALID_ENUM;
			if (!change_stencil_func_config(&stencil_func_back, func)) error = GL_INVALID_ENUM;
			stencil_mask_front = stencil_mask_back = mask;
			stencil_ref_front = stencil_ref_back = ref;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	change_stencil_settings();
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask){
	glStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void glStencilMaskSeparate(GLenum face, GLuint mask){
	switch (face){
		case GL_FRONT:
			stencil_mask_front_write = mask;
			break;
		case GL_BACK:
			stencil_mask_back_write = mask;
			break;
		case GL_FRONT_AND_BACK:
			stencil_mask_front_write = stencil_mask_back_write = mask;
			break;
		default:
			error = GL_INVALID_ENUM;
			return;
	}
	stencil_mask_front_write = mask;
}

void glStencilMask(GLuint mask){
	glStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
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

void glPushMatrix(void){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (matrix == &modelview_matrix){
		if (modelview_stack_counter >= MODELVIEW_STACK_DEPTH){
			error = GL_STACK_OVERFLOW;
		}
		matrix4x4_copy(modelview_matrix_stack[modelview_stack_counter++], *matrix);
	}else if (matrix == &projection_matrix){
		if (projection_stack_counter >= GENERIC_STACK_DEPTH){
			error = GL_STACK_OVERFLOW;
		}
		matrix4x4_copy(projection_matrix_stack[projection_stack_counter++], *matrix);
	}
}

void glPopMatrix(void){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (matrix == &modelview_matrix){
		if (modelview_stack_counter == 0) error = GL_STACK_UNDERFLOW;
		else matrix4x4_copy(*matrix, modelview_matrix_stack[--modelview_stack_counter]);
	}else if (matrix == &projection_matrix){
		if (projection_stack_counter == 0) error = GL_STACK_UNDERFLOW;
		else matrix4x4_copy(*matrix, projection_matrix_stack[--projection_stack_counter]);
	}
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
			matrix4x4 mvp_matrix;
			matrix4x4 final_mvp_matrix;
	
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
			
			if (texture_array_state){
				if (!(tex_unit->textures[texture2d_idx].valid)) return;
				switch (tex_unit->textures[texture2d_idx].type){
					case GL_RGB:
					case GL_RGBA:
					case GL_RG:
					case GL_COLOR_INDEX:
					case GL_VITA2D_TEXTURE:
						sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
						sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
						void* alpha_buffer;
						sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_op);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
						float tex_env_mode = tex_unit->env_mode * 1.0f;
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &tex_env_mode);
						break;
					default:
						return;
						break;
				}
			}else if (tex_unit->color_array.num == 3){
				sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}else{
				sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}
			
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
			if (texture_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
				sceGxmTextureSetUAddrMode(&tex_unit->textures[texture2d_idx].gxm_tex, u_mode);
				sceGxmTextureSetVAddrMode(&tex_unit->textures[texture2d_idx].gxm_tex, v_mode);
				sceGxmSetFragmentTexture(gxm_context, 0, &tex_unit->textures[texture2d_idx].gxm_tex);
				vector3f* vertices = NULL;
				vector2f* uv_map = NULL;
				uint16_t* indices;
				int n;
				if (vertex_array_unit >= 0){
					if (tex_unit->vertex_array.stride == 0) vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
					else vertices = (vector3f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride));
					if (tex_unit->texture_array.stride == 0) uv_map = (vector2f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer) + (first * (tex_unit->texture_array.num * tex_unit->texture_array.size)));
					else uv_map = (vector2f*)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer) + (first * tex_unit->texture_array.stride));
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					for (n=0; n<count; n++){
						indices[n] = n;
					}
				}else{
					uint8_t* ptr;
					uint8_t* ptr_tex;
					vertices = (vector3f*)gpu_pool_memalign(count * sizeof(vector3f), sizeof(vector3f));
					uv_map = (vector2f*)gpu_pool_memalign(count * sizeof(vector2f), sizeof(vector2f));
					memset(vertices, 0, (count * sizeof(vector3f)));
					uint8_t vec_set = 0, tex_set = 0;
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
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					int n;
					for (n=0;n<count;n++){
						if (!vec_set){
							memcpy(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							ptr += tex_unit->vertex_array.stride;
						}
						if (!tex_set){
							memcpy(&uv_map[n], ptr_tex, tex_unit->texture_array.size * tex_unit->texture_array.num);
							ptr_tex += tex_unit->texture_array.stride;
						}
						indices[n] = n;
					}
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}else if (color_array_state){
				if (tex_unit->color_array.num == 3) sceGxmSetUniformDataF(vertex_wvp_buffer, rgb_wvp, 0, 16, (const float*)final_mvp_matrix);
				else  sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)final_mvp_matrix);
				vector3f* vertices = NULL;
				uint8_t* colors = NULL;
				uint16_t* indices;
				int n = 0;
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
							memcpy(&colors[n], ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
							ptr_clr += tex_unit->color_array.stride;
						}
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
			matrix4x4 mvp_matrix;
			matrix4x4 final_mvp_matrix;
	
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
			
			if (texture_array_state){
				if (!(tex_unit->textures[texture2d_idx].valid)) return;
				switch (tex_unit->textures[texture2d_idx].type){
					case GL_RGB:
					case GL_RGBA:
					case GL_RG:
					case GL_COLOR_INDEX:
					case GL_VITA2D_TEXTURE:
						sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
						sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
						void* alpha_buffer;
						sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &alpha_buffer);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_cut, 0, 1, &alpha_ref);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_alpha_op, 0, 1, &alpha_op);
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tint_color, 0, 4, &current_color.r);
						float tex_env_mode = texture_units[client_texture_unit].env_mode * 1.0f;
						sceGxmSetUniformDataF(alpha_buffer, texture2d_tex_env, 0, 1, &tex_env_mode);
						break;
					default:
						return;
						break;
				}
			}else if (texture_units[client_texture_unit].color_array.num == 3){
				sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}else{
				sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, rgba_fragment_program_patched);
			}
			
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
			if (texture_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
				sceGxmTextureSetUAddrMode(&tex_unit->textures[texture2d_idx].gxm_tex, u_mode);
				sceGxmTextureSetVAddrMode(&tex_unit->textures[texture2d_idx].gxm_tex, v_mode);
				sceGxmSetFragmentTexture(gxm_context, 0, &texture_units[client_texture_unit].textures[texture2d_idx].gxm_tex);
				vector3f* vertices = NULL;
				vector2f* uv_map = NULL;
				uint16_t* indices;
				if (index_array_unit >= 0) indices = (uint16_t*)((uint32_t)gpu_buffers[index_array_unit].ptr + (uint32_t)gl_indices);
				else{
					indices = (uint16_t*)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
					memcpy(indices, gl_indices, sizeof(uint16_t) * count);
				}
				if (vertex_array_unit >= 0){
					vertices = (vector3f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
					uv_map = (vector2f*)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer);	
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
					if (tex_unit->vertex_array.stride == 0) memcpy(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
					if (tex_unit->texture_array.stride == 0) memcpy(uv_map, tex_unit->texture_array.pointer, vertex_count_int * (tex_unit->texture_array.size * tex_unit->texture_array.num));
					if ((tex_unit->vertex_array.stride != 0) || (texture_units[client_texture_unit].texture_array.stride != 0)){
						if (tex_unit->vertex_array.stride != 0) memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex), sizeof(texture2d_vertex)));
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
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}else if (color_array_state){
				if (tex_unit->color_array.num == 3) sceGxmSetUniformDataF(vertex_wvp_buffer, rgb_wvp, 0, 16, (const float*)final_mvp_matrix);
				else sceGxmSetUniformDataF(vertex_wvp_buffer, rgba_wvp, 0, 16, (const float*)final_mvp_matrix);
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
						if (tex_unit->vertex_array.stride != 0) memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex), sizeof(texture2d_vertex)));
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

void glFinish(void){
	sceGxmFinish(gxm_context);
}

const GLubyte* glGetString(GLenum name){
	switch (name){
		case GL_VENDOR:
			return vendor;
			break;
		case GL_RENDERER:
			return renderer;
			break;
		case GL_VERSION:
			return version;
			break;
		case GL_EXTENSIONS:
			return extensions;
			break;
		default:
			error = GL_INVALID_ENUM;
			return NULL;
			break;
	}
}

void glGetBooleanv(GLenum pname, GLboolean* params){
	switch (pname){
		case GL_BLEND:
			*params = blend_state;
			break;
		case GL_BLEND_DST_ALPHA:
			*params = (blend_dfactor_a == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
			break;
		case GL_BLEND_DST_RGB:
			*params = (blend_dfactor_rgb == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
			break;
		case GL_BLEND_SRC_ALPHA:
			*params = (blend_sfactor_a == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
			break;
		case GL_BLEND_SRC_RGB:
			*params = (blend_sfactor_rgb == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
			break;
		case GL_DEPTH_TEST:
			*params = depth_test_state;
			break;
		case GL_ACTIVE_TEXTURE:
			*params = GL_FALSE;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glGetFloatv(GLenum pname, GLfloat* data){
	switch (pname){
		case GL_POLYGON_OFFSET_FACTOR:
			*data = pol_factor;
			break;
		case GL_POLYGON_OFFSET_UNITS:
			*data = pol_units;
			break;
		case GL_MODELVIEW_MATRIX:
			memcpy(data, &modelview_matrix, sizeof(matrix4x4));
			break;
		case GL_ACTIVE_TEXTURE:
			*data = (1.0f * (server_texture_unit + GL_TEXTURE0));
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
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

void glGenerateMipmap(GLenum target){
	texture* tex = &texture_units[server_texture_unit].textures[texture2d_idx];
	if (!tex->valid) return;
	SceGxmTransferFormat fmt;
	switch (target){
		case GL_TEXTURE_2D:
			switch (tex->type){
				case GL_RGBA:
					fmt = SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR;
					break;
				case GL_RGB:
					fmt = SCE_GXM_TRANSFER_FORMAT_U8U8U8_BGR;
				default:
					break;
			}
			int j, mipcount = 0;
			uint32_t w, h;
			uint32_t orig_w = w = sceGxmTextureGetWidth(&tex->gxm_tex);
			uint32_t orig_h = h = sceGxmTextureGetHeight(&tex->gxm_tex);
			uint32_t size = 0;
			while ((w > 1) && (h > 1)){
				size += max(w, 8) * h * sizeof(uint32_t);
				w /= 2;
				h /= 2;
				mipcount++;
			}
			SceUID data_UID;
			void *texture_data = gpu_alloc_map(
				SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
				SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
				size, &data_UID);
			sceGxmColorSurfaceInit(&tex->gxm_sfc,
				SCE_GXM_COLOR_FORMAT_A8B8G8R8,
				SCE_GXM_COLOR_SURFACE_LINEAR,
				SCE_GXM_COLOR_SURFACE_SCALE_NONE,
				SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
				orig_w,orig_h,orig_w,texture_data);
			SceGxmTextureFormat format = sceGxmTextureGetFormat(&tex->gxm_tex);
			memcpy(texture_data, sceGxmTextureGetData(&tex->gxm_tex), orig_w * orig_h * tex_format_to_bytespp(format));
			gpu_free_texture(tex);
			sceGxmTextureInitLinear(&tex->gxm_tex, texture_data, format, orig_w, orig_h, mipcount);
			tex->valid = 1;
			tex->data_UID = data_UID;
			uint32_t* curPtr = (uint32_t*)texture_data;
			uint32_t curWidth = orig_w;
			uint32_t curHeight = orig_h;
			for (j=0;j<mipcount-1;j++){
				uint32_t curSrcStride = max(curWidth, 8);
				uint32_t curDstStride = max((curWidth>>1), 8);
				uint32_t* dstPtr = curPtr + (curSrcStride * curHeight);
				sceGxmTransferDownscale(
					fmt, curPtr, 0, 0,
					curWidth, curHeight,
					curSrcStride * sizeof(uint32_t),
					fmt, dstPtr, 0, 0,
					curDstStride * sizeof(uint32_t),
					NULL, SCE_GXM_TRANSFER_FRAGMENT_SYNC, NULL);
				curPtr = dstPtr;
				curWidth /= 2;
				curHeight /= 2;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}