#include <stdlib.h>
#include "vitaGL.h"
#include "math_utils.h"
#include "gpu_utils.h"

// Shaders
#include "clear_f.h"
#include "clear_v.h"
#include "color_f.h"
#include "color_v.h"
#include "disable_color_buffer_f.h"
#include "disable_color_buffer_v.h"
#include "texture2d_f.h"
#include "texture2d_v.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define TEXTURES_NUM          32   // Available texture units number
#define MODELVIEW_STACK_DEPTH 32   // Depth of modelview matrix stack
#define GENERIC_STACK_DEPTH   2    // Depth of generic matrix stack
#define DISPLAY_WIDTH         960  // Display width in pixels
#define DISPLAY_HEIGHT        544  // Display height in pixels
#define DISPLAY_STRIDE        1024 // Display stride in pixels
#define DISPLAY_BUFFER_COUNT  2    // Display buffers to use
#define GXM_TEX_MAX_SIZE      4096 // MAximum width/height in pixels per texture

const GLubyte* vendor = "Rinnegatamante";
const GLubyte* renderer = "SGX543MP4+";
const GLubyte* version = "VitaGL 1.0";
const GLubyte* extensions = "";

typedef struct clear_vertex{
	vector2f position;
} clear_vertex;

typedef struct position_vertex{
	vector3f position;
} position_vertex;

typedef struct color_vertex{
	vector3f position;
	vector4f color;
} color_vertex;

typedef struct texture2d_vertex{
	vector3f position;
	vector2f texcoord;
} texture2d_vertex;

typedef struct texture{
	SceGxmTexture gxm_tex;
	SceUID data_UID;
	SceUID palette_UID;
} texture;

// Non native primitives implemented
typedef enum SceGxmPrimitiveTypeExtra{
	SCE_GXM_PRIMITIVE_NONE = 0,
	SCE_GXM_PRIMITIVE_QUADS = 1
} SceGxmPrimitiveTypeExtra;

// Vertex list struct
typedef struct vertexList{
	texture2d_vertex v;
	color_vertex v2;
	void* next;
} vertexList;

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
static const SceGxmProgram *const gxm_program_color_v = (SceGxmProgram*)&color_v;
static const SceGxmProgram *const gxm_program_color_f = (SceGxmProgram*)&color_f;
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

// Color shader
static SceGxmShaderPatcherId color_vertex_id;
static SceGxmShaderPatcherId color_fragment_id;
static const SceGxmProgramParameter* color_position;
static const SceGxmProgramParameter* color_color;
static const SceGxmProgramParameter* color_wvp;
static SceGxmVertexProgram* color_vertex_program_patched;
static SceGxmFragmentProgram* color_fragment_program_patched;
static const SceGxmProgram* color_fragment_program;

// Texture2D shader
static SceGxmShaderPatcherId texture2d_vertex_id;
static SceGxmShaderPatcherId texture2d_fragment_id;
static const SceGxmProgramParameter* texture2d_position;
static const SceGxmProgramParameter* texture2d_texcoord;
static const SceGxmProgramParameter* texture2d_wvp;
static SceGxmVertexProgram* texture2d_vertex_program_patched;
static SceGxmFragmentProgram* texture2d_fragment_program_patched;
static const SceGxmProgram* texture2d_fragment_program;

// Internal stuffs
static SceGxmPrimitiveType prim;
static SceGxmPrimitiveTypeExtra prim_extra = SCE_GXM_PRIMITIVE_NONE;
static vertexList* model = NULL;
static vertexList* last = NULL;
static glPhase phase = NONE;
static uint8_t texture_init = 1; 
static uint64_t vertex_count = 0;
static uint8_t drawing = 0;
static uint8_t using_texture = 0;
static matrix4x4 projection_matrix;
static matrix4x4 modelview_matrix;
static GLboolean vblank = GL_TRUE;

static GLenum error = GL_NO_ERROR; // Error global returned by glGetError
static GLuint textures[TEXTURES_NUM]; // Textures array
static texture* gpu_textures[TEXTURES_NUM]; // Textures array

static SceGxmBlendFunc blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD; // Current in-use RGB blend func
static SceGxmBlendFunc blend_func_a = SCE_GXM_BLEND_FUNC_ADD; // Current in-use A blend func
static SceGxmBlendFactor blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE; // Current in-use RGB source blend factor
static SceGxmBlendFactor blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO; // Current in-use RGB dest blend factor
static SceGxmBlendFactor blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE; // Current in-use A source blend factor
static SceGxmBlendFactor blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO; // Current in-use A dest blend factor
static SceGxmDepthFunc gxm_depth = SCE_GXM_DEPTH_FUNC_ALWAYS; // Current in-use depth test func
static SceGxmStencilOp stencil_fail = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when stencil test fails
static SceGxmStencilOp depth_fail = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when depth test fails
static SceGxmStencilOp depth_pass = SCE_GXM_STENCIL_OP_KEEP; // Current in-use stencil OP when depth test passes
static SceGxmStencilFunc stencil_func = SCE_GXM_STENCIL_FUNC_ALWAYS; // Current in-use stencil function
static uint8_t stencil_mask = 1; // Current in-use mask for stencil test
static uint8_t stencil_mask_front_write = 0xFF; // Current in-use mask for write stencil test on front
static uint8_t stencil_mask_back_write = 0xFF; // Current in-use mask for write stencil test on back
static uint8_t stencil_ref = 0; // Current in-use reference for stencil test
static GLdouble depth_value = 1.0f; // Current depth test value
static int8_t texture_unit = -1; // Current in-use texture unit
static matrix4x4* matrix = NULL; // Current in-use matrix mode
static vector4f current_color; // Current in-use color
static vector4f clear_color_val; // Current clear color for glClear
static GLboolean depth_test_state = GL_FALSE; // Current state for GL_DEPTH_TEST
static GLboolean stencil_test_state = GL_FALSE; // Current state for GL_STENCIL_TEST
static GLboolean vertex_array_state = GL_FALSE; // Current state for GL_VERTEX_ARRAY
static GLboolean color_array_state = GL_FALSE; // Current state for GL_COLOR_ARRAY
static GLboolean texture_array_state = GL_FALSE; // Current state for GL_TEXTURE_COORD_ARRAY
static GLboolean scissor_test_state = GL_FALSE; // Current state for GL_SCISSOR_TEST
static GLboolean cull_face_state = GL_FALSE; // Current state for GL_CULL_FACE
static GLboolean blend_state = GL_FALSE; // Current state for GL_BLEND
static GLboolean depth_mask_state = GL_TRUE; // Current state for glDepthMask
static GLenum gl_cull_mode = GL_BACK; // Current in-use openGL cull mode
static GLenum gl_front_face = GL_CCW; // Current in-use openGL cull mode
static GLboolean no_polygons_mode = GL_FALSE; // GL_TRUE when cull mode to GL_FRONT_AND_BACK is set
static vertexArray vertex_array; // Current in-use vertex array
static vertexArray color_array; // Current in-use color array
static vertexArray texture_array; // Current in-use texture array

static matrix4x4 modelview_matrix_stack[MODELVIEW_STACK_DEPTH];
uint8_t modelview_stack_counter = 0;
static matrix4x4 projection_matrix_stack[GENERIC_STACK_DEPTH];
uint8_t projection_stack_counter = 0;

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

static void _change_blend_factor(SceGxmBlendInfo* blend_info){
	sceGxmFinish(gxm_context);
	
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, color_fragment_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_program_patched);
	
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		color_fragment_id,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		blend_info,
		color_fragment_program,
		&color_fragment_program_patched);
		
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		texture2d_fragment_id,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE,
		blend_info,
		texture2d_fragment_program,
		&texture2d_fragment_program_patched);
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

static void change_stencil_settings(){
	sceGxmSetFrontStencilFunc(gxm_context,
		stencil_func,
		stencil_fail,
		depth_fail,
		depth_pass,
		stencil_mask, stencil_mask_front_write);
}

static void disable_blend(){
	_change_blend_factor(NULL);
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

// vitaGL specific functions

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
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_color_v,
		&color_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_color_f,
		&color_fragment_id);

	const SceGxmProgram* color_vertex_program = sceGxmShaderPatcherGetProgramFromId(color_vertex_id);
	color_fragment_program = sceGxmShaderPatcherGetProgramFromId(color_fragment_id);

	color_position = sceGxmProgramFindParameterByName(
		color_vertex_program, "aPosition");

	color_color = sceGxmProgramFindParameterByName(
		color_vertex_program, "aColor");

	SceGxmVertexAttribute color_vertex_attribute[2];
	SceGxmVertexStream color_vertex_stream;
	color_vertex_attribute[0].streamIndex = 0;
	color_vertex_attribute[0].offset = 0;
	color_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	color_vertex_attribute[0].componentCount = 3;
	color_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		color_position);
	color_vertex_attribute[1].streamIndex = 0;
	color_vertex_attribute[1].offset = sizeof(vector3f);
	color_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	color_vertex_attribute[1].componentCount = 4;
	color_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		color_color);
	color_vertex_stream.stride = sizeof(struct color_vertex);
	color_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		color_vertex_id, color_vertex_attribute,
		2, &color_vertex_stream, 1, &color_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		color_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, color_fragment_program,
		&color_fragment_program_patched);
		
	color_wvp = sceGxmProgramFindParameterByName(color_vertex_program, "wvp");
		
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

	SceGxmVertexAttribute texture2d_vertex_attribute[2];
	SceGxmVertexStream texture2d_vertex_stream;
	texture2d_vertex_attribute[0].streamIndex = 0;
	texture2d_vertex_attribute[0].offset = 0;
	texture2d_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_vertex_attribute[0].componentCount = 3;
	texture2d_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_position);
	texture2d_vertex_attribute[1].streamIndex = 0;
	texture2d_vertex_attribute[1].offset = sizeof(vector3f);
	texture2d_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texture2d_vertex_attribute[1].componentCount = 2;
	texture2d_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		texture2d_texcoord);
	texture2d_vertex_stream.stride = sizeof(struct texture2d_vertex);
	texture2d_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		texture2d_vertex_id, texture2d_vertex_attribute,
		2, &texture2d_vertex_stream, 1, &texture2d_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		texture2d_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		SCE_GXM_MULTISAMPLE_NONE, NULL, texture2d_fragment_program,
		&texture2d_fragment_program_patched);
		
	texture2d_wvp = sceGxmProgramFindParameterByName(texture2d_vertex_program, "wvp");	
		
	// Allocate temp pool for non-VBO drawing
	gpu_pool_init(gpu_pool_size);
	
	// Initializing openGL stuffs
	current_color.r = current_color.g = current_color.b = current_color.a = 1.0f;
	
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
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, color_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, color_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, texture2d_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_program_patched);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, color_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, color_fragment_id);
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
		if (drawing){
			sceGxmEndScene(gxm_context, NULL, NULL);
			sceGxmFinish(gxm_context);
			sceGxmPadHeartbeat(&gxm_color_surfaces[gxm_back_buffer_index], gxm_sync_objects[gxm_back_buffer_index]);
			struct display_queue_callback_data queue_cb_data;
			queue_cb_data.addr = gxm_color_surfaces_addr[gxm_back_buffer_index];
			sceGxmDisplayQueueAddEntry(gxm_sync_objects[gxm_front_buffer_index],
				gxm_sync_objects[gxm_back_buffer_index], &queue_cb_data);
			gxm_front_buffer_index = gxm_back_buffer_index;
			gxm_back_buffer_index = (gxm_back_buffer_index + 1) % DISPLAY_BUFFER_COUNT;
		}
		gpu_pool_reset();
		sceGxmBeginScene(
			gxm_context,
			0,
			gxm_render_target,
			NULL,
			NULL,
			gxm_sync_objects[gxm_back_buffer_index],
			&gxm_color_surfaces[gxm_back_buffer_index],
			&gxm_depth_stencil_surface);
		sceGxmSetFrontDepthWriteEnable(gxm_context, SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetFrontDepthFunc(gxm_context, SCE_GXM_DEPTH_FUNC_ALWAYS);
		sceGxmSetVertexProgram(gxm_context, clear_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, clear_fragment_program_patched);
		void *color_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &color_buffer);
		sceGxmSetUniformDataF(color_buffer, clear_color, 0, 4, &clear_color_val.r);
		sceGxmSetVertexStream(gxm_context, 0, clear_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, clear_indices, 4);
		sceGxmSetFrontDepthWriteEnable(gxm_context, depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetFrontDepthFunc(gxm_context, gxm_depth);
		drawing = 1;
	}
	if ((mask & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT){
		sceGxmSetFrontDepthWriteEnable(gxm_context, SCE_GXM_DEPTH_WRITE_ENABLED);
		sceGxmSetFrontDepthFunc(gxm_context, SCE_GXM_DEPTH_FUNC_ALWAYS);
		sceGxmSetFrontStencilFunc(gxm_context,
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
		sceGxmSetFrontDepthWriteEnable(gxm_context, depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetFrontDepthFunc(gxm_context, gxm_depth);
		change_stencil_settings();
	}
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){
	clear_color_val.r = red;
	clear_color_val.g = green;
	clear_color_val.b = blue;
	clear_color_val.a = alpha;
}

void glEnable(GLenum cap){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	switch (cap){
		case GL_DEPTH_TEST:
			sceGxmSetFrontDepthFunc(gxm_context, SCE_GXM_DEPTH_FUNC_LESS);
			depth_test_state = GL_TRUE;
			break;
		case GL_STENCIL_TEST:
			sceGxmSetFrontStencilRef(gxm_context, stencil_ref);
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
			sceGxmSetFrontDepthFunc(gxm_context, SCE_GXM_DEPTH_FUNC_ALWAYS);
			depth_test_state = GL_FALSE;
			break;
		case GL_STENCIL_TEST:
			sceGxmSetFrontStencilFunc(gxm_context,
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
			break;
		case GL_LINES:
			prim = SCE_GXM_PRIMITIVE_LINES;
			break;
		case GL_TRIANGLES:
			prim = SCE_GXM_PRIMITIVE_TRIANGLES;
			break;
		case GL_TRIANGLE_STRIP:
			prim = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
			break;
		case GL_TRIANGLE_FAN:
			prim = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
			break;
		case GL_QUADS:
			prim = SCE_GXM_PRIMITIVE_TRIANGLES;
			prim_extra = SCE_GXM_PRIMITIVE_QUADS;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	vertex_count = 0;
}

void glEnd(void){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	phase = NONE;
	
	if (no_polygons_mode && ((prim != SCE_GXM_PRIMITIVE_TRIANGLES) && (prim >= SCE_GXM_PRIMITIVE_TRIANGLE_STRIP))){
		model = NULL;
		vertex_count = 0;
		using_texture = 0;
		return;
	}
	
	matrix4x4 mvp_matrix;
	matrix4x4 final_mvp_matrix;
	
	matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
	matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
	
	if (texture_unit >= 0){
		sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
	}else{
		sceGxmSetVertexProgram(gxm_context, color_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, color_fragment_program_patched);
	}
	
	void* vertex_wvp_buffer;
	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
	if (using_texture){
		sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
		sceGxmSetFragmentTexture(gxm_context, 0, &gpu_textures[texture_unit]->gxm_tex);
		texture2d_vertex* vertices;
		uint16_t* indices;
		int n = 0, quad_n = 0, v_n = 0;
		vertexList* object = model;
		uint32_t idx_count = vertex_count;
	
		switch (prim_extra){
			case SCE_GXM_PRIMITIVE_NONE:
				vertices = (texture2d_vertex*)gpu_pool_memalign(vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex));
				memset(vertices, 0, (vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex)));
				indices = (uint16_t*)gpu_pool_memalign(vertex_count * sizeof(uint16_t), sizeof(uint16_t));
				while (object != NULL){
					memcpy(&vertices[n], &object->v, sizeof(texture2d_vertex));
					indices[n] = n;
					vertexList* old = object;
					object = object->next;
					free(old);
					n++;
				}
				break;
			case SCE_GXM_PRIMITIVE_QUADS:
				quad_n = vertex_count >> 2;
				idx_count = quad_n * 6;
				vertices = (texture2d_vertex*)gpu_pool_memalign(vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex));
				memset(vertices, 0, (vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex)));
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				int i, j;
				for (i=0; i < quad_n; i++){
					for (j=0; j < 4; j++){
						memcpy(&vertices[i*4+j], &object->v, sizeof(texture2d_vertex));
						vertexList* old = object;
						object = object->next;
						free(old);
					}
					indices[i*6] = i*4;
					indices[i*6+1] = i*4+1;
					indices[i*6+2] = i*4+3;
					indices[i*6+3] = i*4+1;
					indices[i*6+4] = i*4+2;
					indices[i*6+5] = i*4+3;
				}
				break;
		}
		sceGxmSetVertexStream(gxm_context, 0, vertices);
		sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
	}else{
		sceGxmSetUniformDataF(vertex_wvp_buffer, color_wvp, 0, 16, (const float*)final_mvp_matrix);
		color_vertex* vertices;
		uint16_t* indices;
		int n = 0, quad_n = 0, v_n = 0;
		vertexList* object = model;
		uint32_t idx_count = vertex_count;
	
		switch (prim_extra){
			case SCE_GXM_PRIMITIVE_NONE:
				vertices = (color_vertex*)gpu_pool_memalign(vertex_count * sizeof(color_vertex), sizeof(color_vertex));
				memset(vertices, 0, (vertex_count * sizeof(color_vertex), sizeof(color_vertex)));
				indices = (uint16_t*)gpu_pool_memalign(vertex_count * sizeof(uint16_t), sizeof(uint16_t));
				while (object != NULL){
					memcpy(&vertices[n], &object->v2, sizeof(color_vertex));
					indices[n] = n;
					vertexList* old = object;
					object = object->next;
					free(old);
					n++;
				}
				break;
			case SCE_GXM_PRIMITIVE_QUADS:
				quad_n = vertex_count >> 2;
				idx_count = quad_n * 6;
				vertices = (color_vertex*)gpu_pool_memalign(vertex_count * sizeof(color_vertex), sizeof(color_vertex));
				memset(vertices, 0, (vertex_count * sizeof(color_vertex), sizeof(color_vertex)));
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				int i, j;
				for (i=0; i < quad_n; i++){
					for (j=0; j < 4; j++){
						memcpy(&vertices[i*4+j], &object->v2, sizeof(color_vertex));
						vertexList* old = object;
						object = object->next;
						free(old);
					}
					indices[i*6] = i*4;
					indices[i*6+1] = i*4+1;
					indices[i*6+2] = i*4+3;
					indices[i*6+3] = i*4+1;
					indices[i*6+4] = i*4+2;
					indices[i*6+5] = i*4+3;
				}
				break;
		}
		sceGxmSetVertexStream(gxm_context, 0, vertices);
		sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
	}
	
	model = NULL;
	vertex_count = 0;
	using_texture = 0;
	
}

void glGenTextures(GLsizei n, GLuint* res){
	int i = 0, j = 0;
	if (n < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	if (texture_init){
		texture_init = 0;
		for (i=0; i < TEXTURES_NUM; i++){
			textures[i] = GL_TEXTURE0 + i;
			gpu_textures[i] = NULL;
		}
	}
	i = 0;
	for (i=0; i < TEXTURES_NUM; i++){
		if (textures[i] != 0x0000){
			res[j++] = textures[i];
			textures[i] = 0x0000;
		}
		if (j >= n) break;
	}
}

void glBindTexture(GLenum target, GLuint texture){
	if ((texture != 0x0000) && (texture > GL_TEXTURE31) && (texture < GL_TEXTURE0)){
		error = GL_INVALID_VALUE;
		return;
	}
	switch (target){
		case GL_TEXTURE_2D:
			texture_unit = texture - GL_TEXTURE0;
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
	int i, j;
	for (j=0; j<n; j++){
		if (gl_textures[j] >= GL_TEXTURE0 && gl_textures[j] <= GL_TEXTURE31){
			uint8_t idx = gl_textures[j] - GL_TEXTURE0;
			textures[idx] = gl_textures[j];
			if (gpu_textures[idx] != NULL){
				gpu_unmap_free(gpu_textures[idx]->data_UID);
				free(gpu_textures[idx]);
				gpu_textures[idx] = NULL;
			}
		}
	}
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data){
	SceGxmTextureFormat tex_format;
	switch (target){
		case GL_TEXTURE_2D:
			if (gpu_textures[texture_unit] != NULL){
				gpu_unmap_free(gpu_textures[texture_unit]->data_UID);
				free(gpu_textures[texture_unit]);
			}
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
			if (error == GL_NO_ERROR){
				if (width > GXM_TEX_MAX_SIZE || height > GXM_TEX_MAX_SIZE){
					error = GL_INVALID_VALUE;
					return;
				}
				gpu_textures[texture_unit] = (texture*)malloc(sizeof(texture));
				const int tex_size = width * height * tex_format_to_bytespp(tex_format);
				void *texture_data = gpu_alloc_map(
					SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
					SCE_GXM_MEMORY_ATTRIB_READ,
					tex_size,
					&gpu_textures[texture_unit]->data_UID);
				memcpy(texture_data, (uint8_t*)data, width*height*internalFormat);
				sceGxmTextureInitLinear(&gpu_textures[texture_unit]->gxm_tex, texture_data, tex_format, width, height, 0);
				gpu_textures[texture_unit]->palette_UID = 0;
			}
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
							sceGxmTextureSetMinFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
							break;
						case GL_LINEAR:
							sceGxmTextureSetMinFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
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
							sceGxmTextureSetMagFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
							break;
						case GL_LINEAR:
							sceGxmTextureSetMagFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
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
					if (param == GL_NEAREST) sceGxmTextureSetMinFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
					else if (param == GL_LINEAR) sceGxmTextureSetMinFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
					break;
				case GL_TEXTURE_MAG_FILTER:
					if (param == GL_NEAREST) sceGxmTextureSetMagFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
					else if (param == GL_LINEAR) sceGxmTextureSetMagFilter(&gpu_textures[texture_unit]->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);	
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
	using_texture = 1;
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model == NULL){ 
		last = (vertexList*)malloc(sizeof(vertexList));
		model = last;
	}else{
		last->next = (vertexList*)malloc(sizeof(vertexList));
		last = last->next;
	}
	last->v.texcoord.x = s;
	last->v.texcoord.y = t;
}

void glActiveTexture(GLenum texture){
	if (texture < GL_TEXTURE0 && texture > GL_TEXTURE0 + TEXTURES_NUM) error = GL_INVALID_ENUM;
	else texture_unit = texture - GL_TEXTURE0;
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (!using_texture){
		if (model == NULL){ 
			last = (vertexList*)malloc(sizeof(vertexList));
			model = last;
		}else{
			last->next = (vertexList*)malloc(sizeof(vertexList));
			last = last->next;
		}
		last->v2.color = current_color;
		last->v2.position.x = x;
		last->v2.position.y = y;
		last->v2.position.z = z;
	}else{
		last->v.position.x = x;
		last->v.position.y = y;
		last->v.position.z = z;
	}
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
	if (vertex_array_state){
		float coords[3];
		uint8_t* ptr = ((uint8_t*)vertex_array.pointer) + (i * ((vertex_array.num * vertex_array.size) + vertex_array.stride));
		memcpy(coords, ptr, vertex_array.size * vertex_array.num);
		if (texture_array_state){
			float tex[2];
			uint8_t* ptr_tex = ((uint8_t*)texture_array.pointer) + (i * ((texture_array.num * texture_array.size) + texture_array.stride));
			if (model == NULL){ 
				last = (vertexList*)malloc(sizeof(vertexList));
				model = last;
			}else{
				last->next = (vertexList*)malloc(sizeof(vertexList));
				last = last->next;
			}
			memcpy(tex, ptr_tex, vertex_array.size * 2);
			last->v.texcoord.x = tex[0];
			last->v.texcoord.y = tex[1];
			last->v.position.x = coords[0];
			last->v.position.y = coords[1];
			last->v.position.z = coords[2];
		}else if (color_array_state){
			uint8_t* ptr_clr = ((uint8_t*)color_array.pointer) + (i * ((color_array.num * color_array.size) + color_array.stride));	
			if (model == NULL){ 
				last = (vertexList*)malloc(sizeof(vertexList));
				model = last;
			}else{
				last->next = (vertexList*)malloc(sizeof(vertexList));
				last = last->next;
			}
			last->v2.color.a = 1.0f;
			memcpy(&last->v2.color, ptr_clr, color_array.size * color_array.num);
			last->v2.position.x = coords[0];
			last->v2.position.y = coords[1];
			last->v2.position.z = coords[2];
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

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue){
	current_color.r = (1.0f * red) / 255.0f;
	current_color.g = (1.0f * green) / 255.0f;
	current_color.b = (1.0f * blue) / 255.0f;
	current_color.a = 1.0f;
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){
	current_color.r = red;
	current_color.g = green;
	current_color.b = blue;
	current_color.a = alpha;
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha){
	current_color.r = (1.0f * red) / 255.0f;
	current_color.g = (1.0f * green) / 255.0f;
	current_color.b = (1.0f * blue) / 255.0f;
	current_color.a = (1.0f * alpha) / 255.0f;
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
	sceGxmSetFrontDepthFunc(gxm_context, gxm_depth);
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
	sceGxmSetFrontDepthWriteEnable(gxm_context, depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
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

void glStencilOp(GLenum sfail,  GLenum dpfail,  GLenum dppass){
	switch (sfail){
		case GL_KEEP:
			stencil_fail = SCE_GXM_STENCIL_OP_KEEP;
			break;
		case GL_ZERO:
			stencil_fail = SCE_GXM_STENCIL_OP_ZERO;
			break;
		case GL_REPLACE:
			stencil_fail = SCE_GXM_STENCIL_OP_REPLACE;
			break;
		case GL_INCR:
			stencil_fail = SCE_GXM_STENCIL_OP_INCR;
			break;
		case GL_INCR_WRAP:
			stencil_fail = SCE_GXM_STENCIL_OP_INCR_WRAP;
			break;
		case GL_DECR:
			stencil_fail = SCE_GXM_STENCIL_OP_DECR;
			break;
		case GL_DECR_WRAP:
			stencil_fail = SCE_GXM_STENCIL_OP_DECR_WRAP;
			break;
		case GL_INVERT:
			stencil_fail = SCE_GXM_STENCIL_OP_INVERT;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (dpfail){
		case GL_KEEP:
			depth_fail = SCE_GXM_STENCIL_OP_KEEP;
			break;
		case GL_ZERO:
			depth_fail = SCE_GXM_STENCIL_OP_ZERO;
			break;
		case GL_REPLACE:
			depth_fail = SCE_GXM_STENCIL_OP_REPLACE;
			break;
		case GL_INCR:
			depth_fail = SCE_GXM_STENCIL_OP_INCR;
			break;
		case GL_INCR_WRAP:
			depth_fail = SCE_GXM_STENCIL_OP_INCR_WRAP;
			break;
		case GL_DECR:
			depth_fail = SCE_GXM_STENCIL_OP_DECR;
			break;
		case GL_DECR_WRAP:
			depth_fail = SCE_GXM_STENCIL_OP_DECR_WRAP;
			break;
		case GL_INVERT:
			depth_fail = SCE_GXM_STENCIL_OP_INVERT;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	switch (dppass){
		case GL_KEEP:
			depth_pass = SCE_GXM_STENCIL_OP_KEEP;
			break;
		case GL_ZERO:
			depth_pass = SCE_GXM_STENCIL_OP_ZERO;
			break;
		case GL_REPLACE:
			depth_pass = SCE_GXM_STENCIL_OP_REPLACE;
			break;
		case GL_INCR:
			depth_pass = SCE_GXM_STENCIL_OP_INCR;
			break;
		case GL_INCR_WRAP:
			depth_pass = SCE_GXM_STENCIL_OP_INCR_WRAP;
			break;
		case GL_DECR:
			depth_pass = SCE_GXM_STENCIL_OP_DECR;
			break;
		case GL_DECR_WRAP:
			depth_pass = SCE_GXM_STENCIL_OP_DECR_WRAP;
			break;
		case GL_INVERT:
			depth_pass = SCE_GXM_STENCIL_OP_INVERT;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	change_stencil_settings();
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask){
	switch (func){
		case GL_NEVER:
			stencil_func = SCE_GXM_STENCIL_FUNC_NEVER;
			break;
		case GL_LESS:
			stencil_func = SCE_GXM_STENCIL_FUNC_LESS;
			break;
		case GL_LEQUAL:
			stencil_func = SCE_GXM_STENCIL_FUNC_LESS_EQUAL;
			break;
		case GL_GREATER:
			stencil_func = SCE_GXM_STENCIL_FUNC_GREATER;
			break;
		case GL_GEQUAL:
			stencil_func = SCE_GXM_STENCIL_FUNC_GREATER_EQUAL;
			break;
		case GL_EQUAL:
			stencil_func = SCE_GXM_STENCIL_FUNC_EQUAL;
			break;
		case GL_NOTEQUAL:
			stencil_func = SCE_GXM_STENCIL_FUNC_NOT_EQUAL;
			break;
		case GL_ALWAYS:
			stencil_func = SCE_GXM_STENCIL_FUNC_ALWAYS;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	stencil_mask = mask;
	stencil_ref = ref;
	sceGxmSetFrontStencilRef(gxm_context, ref);
	change_stencil_settings();
}

void glStencilMaskSeparate(GLenum face, GLuint mask){
	switch (face){
		case GL_FRONT:
			stencil_mask_front_write = mask;
			break;
		case GL_BACK:
			stencil_mask_back_write = mask;
			break
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

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer){
	if ((stride < 0) || ((size < 2) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	switch (type){
		case GL_FLOAT:
			vertex_array.size = sizeof(GLfloat);
			break;
		case GL_SHORT:
			vertex_array.size = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	vertex_array.num = size;
	vertex_array.stride = stride;
	vertex_array.pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer){
	if ((stride < 0) || ((size < 3) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	switch (type){
		case GL_FLOAT:
			color_array.size = sizeof(GLfloat);
			break;
		case GL_SHORT:
			color_array.size = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	color_array.num = size;
	color_array.stride = stride;
	color_array.pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer){
	if ((stride < 0) || ((size < 2) && (size > 4))){
		error = GL_INVALID_VALUE;
		return;
	}
	switch (type){
		case GL_FLOAT:
			texture_array.size = sizeof(GLfloat);
			break;
		case GL_SHORT:
			texture_array.size = sizeof(GLshort);
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
	texture_array.num = size;
	texture_array.stride = stride;
	texture_array.pointer = pointer;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count){
	SceGxmPrimitiveType gxm_p;
	SceGxmPrimitiveTypeExtra gxm_ep = SCE_GXM_PRIMITIVE_NONE;
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
			case GL_QUADS:
				gxm_p = SCE_GXM_PRIMITIVE_TRIANGLES;
				gxm_ep = SCE_GXM_PRIMITIVE_QUADS;
				if (no_polygons_mode) skip_draw = GL_TRUE;
				break;
			default:
				error = GL_INVALID_ENUM;
				break;
		}
		if ((error == GL_NO_ERROR) && (!skip_draw)){
			matrix4x4 mvp_matrix;
			matrix4x4 final_mvp_matrix;
	
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
			
			if (texture_array_state){
				sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
			}else{
				sceGxmSetVertexProgram(gxm_context, color_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, color_fragment_program_patched);
			}
			
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
			if (texture_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
				sceGxmSetFragmentTexture(gxm_context, 0, &gpu_textures[texture_unit]->gxm_tex);
				texture2d_vertex* vertices;
				uint16_t* indices;
				int n = 0, quad_n = 0, v_n = 0;
				uint32_t idx_count = vertex_count = count;
				uint8_t* ptr = ((uint8_t*)vertex_array.pointer) + (first * ((vertex_array.num * vertex_array.size) + vertex_array.stride));
				uint8_t* ptr_tex = ((uint8_t*)texture_array.pointer) + (first * ((texture_array.num * texture_array.size) + texture_array.stride));
				
				switch (gxm_ep){
					case SCE_GXM_PRIMITIVE_NONE:
						vertices = (texture2d_vertex*)gpu_pool_memalign(vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex));
						memset(vertices, 0, (vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex)));
						indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
						for (n=0; n<count; n++){
							memcpy(&vertices[n], ptr, vertex_array.size * vertex_array.num);
							memcpy(&vertices[n].texcoord, ptr_tex, vertex_array.size * 2);
							indices[n] = n;
							ptr += ((vertex_array.num * vertex_array.size) + vertex_array.stride);
							ptr_tex += ((texture_array.num * texture_array.size) + texture_array.stride);
						}
						break;
					case SCE_GXM_PRIMITIVE_QUADS:
						quad_n = vertex_count >> 2;
						idx_count = quad_n * 6;
						vertices = (texture2d_vertex*)gpu_pool_memalign(vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex));
						indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
						int i, j;
						for (i=0; i < quad_n; i++){
							for (j=0; j < 4; j++){
								memcpy(&vertices[i*4+j], ptr, vertex_array.size * vertex_array.num);
								memcpy(&vertices[n].texcoord, ptr_tex, vertex_array.size * 2);
								ptr += ((texture_array.num * texture_array.size) + texture_array.stride);
							}
							indices[i*6] = i*4;
							indices[i*6+1] = i*4+1;
							indices[i*6+2] = i*4+3;
							indices[i*6+3] = i*4+1;
							indices[i*6+4] = i*4+2;
							indices[i*6+5] = i*4+3;
						}
						break;
					default:
						break;
				}
				
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
				
			}else if (color_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, color_wvp, 0, 16, (const float*)final_mvp_matrix);
				color_vertex* vertices;
				uint16_t* indices;
				int n = 0, quad_n = 0, v_n = 0;
				uint32_t idx_count = vertex_count = count;
				uint8_t* ptr = ((uint8_t*)vertex_array.pointer) + (first * ((vertex_array.num * vertex_array.size) + vertex_array.stride));
				uint8_t* ptr_clr = ((uint8_t*)color_array.pointer) + (first * ((color_array.num * color_array.size) + color_array.stride));
				switch (gxm_ep){
					case SCE_GXM_PRIMITIVE_NONE:
						vertices = (color_vertex*)gpu_pool_memalign(vertex_count * sizeof(color_vertex), sizeof(color_vertex));
						memset(vertices, 0, (vertex_count * sizeof(color_vertex), sizeof(color_vertex)));
						indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
						for (n=0; n<count; n++){
							memcpy(&vertices[n], ptr, vertex_array.size * vertex_array.num);
							memcpy(&vertices[n].color, ptr_clr, color_array.size * color_array.num);
							if (color_array.num == 3) vertices[n].color.a = 1.0f;
							indices[n] = n;
							ptr += ((vertex_array.num * vertex_array.size) + vertex_array.stride);
							ptr_clr += ((color_array.num * color_array.size) + color_array.stride);
						}
						break;
					case SCE_GXM_PRIMITIVE_QUADS:
						quad_n = vertex_count >> 2;
						idx_count = quad_n * 6;
						vertices = (color_vertex*)gpu_pool_memalign(vertex_count * sizeof(color_vertex), sizeof(color_vertex));
						indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
						int i, j;
						for (i=0; i < quad_n; i++){
							for (j=0; j < 4; j++){
								memcpy(&vertices[i*4+j], ptr, vertex_array.size * vertex_array.num);
								memcpy(&vertices[n].color, ptr_clr, color_array.size * color_array.num);
								if (color_array.num == 3) vertices[n].color.a = 1.0f;
								ptr += ((vertex_array.num * vertex_array.size) + vertex_array.stride);
								ptr_clr += ((color_array.num * color_array.size) + color_array.stride);
							}
							indices[i*6] = i*4;
							indices[i*6+1] = i*4+1;
							indices[i*6+2] = i*4+3;
							indices[i*6+3] = i*4+1;
							indices[i*6+4] = i*4+2;
							indices[i*6+5] = i*4+3;
						}
						break;
					default:
						break;
				}
				
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
				
			}
		}
	}
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* gl_indices){
	SceGxmPrimitiveType gxm_p;
	SceGxmPrimitiveTypeExtra gxm_ep = SCE_GXM_PRIMITIVE_NONE;
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
		if ((error == GL_NO_ERROR) && (!skip_draw)){
			matrix4x4 mvp_matrix;
			matrix4x4 final_mvp_matrix;
	
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
			
			if (texture_array_state){
				sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, texture2d_fragment_program_patched);
			}else{
				sceGxmSetVertexProgram(gxm_context, color_vertex_program_patched);
				sceGxmSetFragmentProgram(gxm_context, color_fragment_program_patched);
			}
			
			void* vertex_wvp_buffer;
			sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vertex_wvp_buffer);
	
			if (texture_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, texture2d_wvp, 0, 16, (const float*)final_mvp_matrix);
				sceGxmSetFragmentTexture(gxm_context, 0, &gpu_textures[texture_unit]->gxm_tex);
				texture2d_vertex* vertices;
				uint16_t* indices;
				int n = 0, j = 0;
				uint32_t vertex_count = 0;
				uint32_t idx_count = count;
				uint16_t* ptr_idx = (uint16_t*)gl_indices;
				while (j < idx_count){
					if (ptr_idx[j] >= vertex_count) vertex_count = ptr_idx[j] + 1;
					j++;
				}
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				memcpy(indices, gl_indices, sizeof(uint16_t) * idx_count);
				uint8_t* ptr = ((uint8_t*)vertex_array.pointer);
				uint8_t* ptr_tex = ((uint8_t*)texture_array.pointer);
				vertices = (texture2d_vertex*)gpu_pool_memalign(vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex));
				memset(vertices, 0, (vertex_count * sizeof(texture2d_vertex), sizeof(texture2d_vertex)));
				for (n=0; n<vertex_count; n++){
					memcpy(&vertices[n], ptr, vertex_array.size * vertex_array.num);
					memcpy(&vertices[n].texcoord, ptr_tex, vertex_array.size * 2);
					ptr += ((vertex_array.num * vertex_array.size) + vertex_array.stride);
					ptr_tex += ((texture_array.num * texture_array.size) + texture_array.stride);
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
			}else if (color_array_state){
				sceGxmSetUniformDataF(vertex_wvp_buffer, color_wvp, 0, 16, (const float*)final_mvp_matrix);
				color_vertex* vertices;
				uint16_t* indices;
				int n = 0, j = 0;
				uint32_t vertex_count = 0;
				uint32_t idx_count = count;
				uint16_t* ptr_idx = (uint16_t*)gl_indices;
				while (j < idx_count){
					if (ptr_idx[j] >= vertex_count) vertex_count = ptr_idx[j] + 1;
					j++;
				}
				indices = (uint16_t*)gpu_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
				memcpy(indices, gl_indices, sizeof(uint16_t) * idx_count);
				uint8_t* ptr = ((uint8_t*)vertex_array.pointer);
				uint8_t* ptr_clr = ((uint8_t*)color_array.pointer);
				vertices = (color_vertex*)gpu_pool_memalign(vertex_count * sizeof(color_vertex), sizeof(color_vertex));
				memset(vertices, 0, (vertex_count * sizeof(color_vertex), sizeof(color_vertex)));
				for (n=0; n<vertex_count; n++){
					memcpy(&vertices[n], ptr, vertex_array.size * vertex_array.num);
					memcpy(&vertices[n].color, ptr_clr, color_array.size * color_array.num);
					if (color_array.num == 3) vertices[n].color.a = 1.0f;
					ptr += ((vertex_array.num * vertex_array.size) + vertex_array.stride);
					ptr_clr += ((color_array.num * color_array.size) + color_array.stride);
				}
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
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
	glActiveTexture(texture);
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
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}