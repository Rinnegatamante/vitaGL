/*
 * This file is part of vitaGL
 * Copyright 2017, 2018, 2019, 2020 Rinnegatamante
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *
 */
#include "vitaGL.h"
#include "shared.h"
#include "texture_callbacks.h"

// Shaders
#include "shaders/clear_f.h"
#include "shaders/clear_v.h"

#define MAX_IDX_NUMBER 12288 // Maximum allowed number of indices per draw call

#ifdef HAVE_SOFTFP_ABI
__attribute__((naked)) void sceGxmSetViewport_sfp(SceGxmContext *context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale) {
	asm volatile(
		"vmov s0, r1\n"
		"vmov s1, r2\n"
		"vmov s2, r3\n"
		"ldr r1, [sp]\n"
		"ldr r2, [sp, #4]\n"
		"ldr r3, [sp, #8]\n"
		"vmov s3, r1\n"
		"vmov s4, r2\n"
		"vmov s5, r2\n"
		"b sceGxmSetViewport\n");
}
#endif

// Clear shader
SceGxmShaderPatcherId clear_vertex_id;
SceGxmShaderPatcherId clear_fragment_id;
const SceGxmProgramParameter *clear_position;
const SceGxmProgramParameter *clear_depth;
const SceGxmProgramParameter *clear_color;
SceGxmVertexProgram *clear_vertex_program_patched;
SceGxmFragmentProgram *clear_fragment_program_patched;

// sceGxm viewport setup (NOTE: origin is on center screen)
float x_port = 480.0f;
float y_port = 272.0f;
float z_port = 0.5f;
float x_scale = 480.0f;
float y_scale = -272.0f;
float z_scale = 0.5f;

// Fullscreen sceGxm viewport (NOTE: origin is on center screen)
float fullscreen_x_port = 480.0f;
float fullscreen_y_port = 272.0f;
float fullscreen_z_port = 0.5f;
float fullscreen_x_scale = 480.0f;
float fullscreen_y_scale = -272.0f;
float fullscreen_z_scale = 0.5f;

uint32_t vsync_interval = 1; // Current setting for VSync

static const SceGxmProgram *const gxm_program_clear_v = (SceGxmProgram *)&clear_v;
static const SceGxmProgram *const gxm_program_clear_f = (SceGxmProgram *)&clear_f;

// Disable color buffer shader
uint16_t *depth_clear_indices = NULL; // Memblock starting address for clear screen indices

// Clear shaders
SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
vector4f *clear_vertices = NULL; // Memblock starting address for clear screen vertices
vector3f *depth_vertices = NULL; // Memblock starting address for depth clear screen vertices

// Internal stuffs
blend_config blend_info; // Current blend info mode
SceGxmMultisampleMode msaa_mode = SCE_GXM_MULTISAMPLE_NONE;
int legacy_pool_size = 0; // Mempool size for GL1 immediate draw pipeline

extern GLboolean use_vram_for_usse;

static SceGxmColorMask blend_color_mask = SCE_GXM_COLOR_MASK_ALL; // Current in-use color mask (glColorMask)
static SceGxmBlendFunc blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD; // Current in-use RGB blend func
static SceGxmBlendFunc blend_func_a = SCE_GXM_BLEND_FUNC_ADD; // Current in-use A blend func
uint16_t *default_idx_ptr; // sceGxm mapped progressive indices buffer
uint16_t *default_quads_idx_ptr; // sceGxm mapped progressive indices buffer for quads
uint16_t *default_line_strips_idx_ptr; // sceGxm mapped progressive indices buffer for line strips

vector4f texenv_color = {0.0f, 0.0f, 0.0f, 0.0f}; // Current in use texture environment color

// Internal functions
#ifdef HAVE_CIRCULAR_VERTEX_POOL
#define CIRCULAR_VERTEX_POOL_SIZE_DEF (32 * 1024 * 1024) // Default size in bytes for the circular vertex pool
static uint8_t *vertex_data_pool;
static uint8_t *vertex_data_pool_ptr;
static uint8_t *vertex_data_pool_limit;
static uint32_t vertex_data_pool_size = CIRCULAR_VERTEX_POOL_SIZE_DEF;
uint8_t *reserve_data_pool(uint32_t size) {
	uint8_t *res = vertex_data_pool_ptr;
	vertex_data_pool_ptr += size;
	if (vertex_data_pool_ptr > vertex_data_pool_limit) {
		vertex_data_pool_ptr = vertex_data_pool;
		return vertex_data_pool_ptr;
	}
	return res;
}
#endif

void rebuild_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog) {
	patchFragmentProgram(gxm_shader_patcher,
		pid, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		msaa_mode, &blend_info.info, NULL, prog);
}

void change_blend_factor() {
	blend_info.info.colorMask = blend_color_mask;
	blend_info.info.colorFunc = blend_func_rgb;
	blend_info.info.alphaFunc = blend_func_a;
	blend_info.info.colorSrc = blend_sfactor_rgb;
	blend_info.info.colorDst = blend_dfactor_rgb;
	blend_info.info.alphaSrc = blend_sfactor_a;
	blend_info.info.alphaDst = blend_dfactor_a;
}

void change_blend_mask() {
	blend_info.info.colorMask = blend_color_mask;
	blend_info.info.colorFunc = SCE_GXM_BLEND_FUNC_NONE;
	blend_info.info.alphaFunc = SCE_GXM_BLEND_FUNC_NONE;
	blend_info.info.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
	blend_info.info.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_info.info.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
	blend_info.info.alphaDst = SCE_GXM_BLEND_FACTOR_ZERO;
}

void vector4f_convert_to_local_space(vector4f *out, int x, int y, int width, int height) {
	if (is_rendering_display) {
		out->x = (float)(2 * x) / DISPLAY_WIDTH_FLOAT - 1.0f;
		out->y = (float)(2 * (x + width)) / DISPLAY_WIDTH_FLOAT - 1.0f;
		out->z = 1.0f - (float)(2 * y) / DISPLAY_HEIGHT_FLOAT;
		out->w = 1.0f - (float)(2 * (y + height)) / DISPLAY_HEIGHT_FLOAT;
	} else {
		out->x = (float)(2 * x) / (float)in_use_framebuffer->width - 1.0f;
		out->y = (float)(2 * (x + width)) / (float)in_use_framebuffer->width - 1.0f;
		out->z = 1.0f - (float)(2 * y) / (float)in_use_framebuffer->height;
		out->w = 1.0f - (float)(2 * (y + height)) / (float)in_use_framebuffer->height;
	}
}

// vitaGL specific functions

void vglUseVram(GLboolean usage) {
	use_vram = usage;
}

void vglUseVramForUSSE(GLboolean usage) {
	use_vram_for_usse = usage;
}

void vglInitWithCustomSizes(int pool_size, int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, SceGxmMultisampleMode msaa) {
#ifndef DISABLE_ADVANCED_SHADER_CACHE
	sceIoMkdir("ux0:data/shader_cache", 0777);
#endif	
	
	// Setting our display size
	msaa_mode = msaa;
	DISPLAY_WIDTH = width;
	DISPLAY_HEIGHT = height;
	DISPLAY_WIDTH_FLOAT = width * 1.0f;
	DISPLAY_HEIGHT_FLOAT = height * 1.0f;
	DISPLAY_STRIDE = ALIGN(DISPLAY_WIDTH, 64);

	// Adjusting default values for internal viewport
	x_port = DISPLAY_WIDTH_FLOAT / 2.0f;
	x_scale = x_port;
	y_scale = -(DISPLAY_HEIGHT_FLOAT / 2.0f);
	y_port = -y_scale;
	fullscreen_x_port = x_port;
	fullscreen_x_scale = x_scale;
	fullscreen_y_port = y_port;
	fullscreen_y_scale = y_scale;

	// Init viewport state
	gl_viewport.x = 0;
	gl_viewport.y = 0;
	gl_viewport.w = DISPLAY_WIDTH;
	gl_viewport.h = DISPLAY_HEIGHT;

	// Initializing sceGxm
	initGxm();

	// Initializing memory heaps for CDRAM and RAM memory (both standard and physically contiguous)
	vgl_mem_init(ram_pool_size, cdram_pool_size, phycont_pool_size);

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

	// Setting up default blending state
	change_blend_mask();

	clear_vertices = gpu_alloc_mapped(1 * sizeof(vector4f), VGL_MEM_RAM);
	depth_clear_indices = gpu_alloc_mapped(4 * sizeof(unsigned short), VGL_MEM_RAM);

	vector4f_convert_to_local_space(clear_vertices, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	depth_clear_indices[0] = 0;
	depth_clear_indices[1] = 1;
	depth_clear_indices[2] = 2;
	depth_clear_indices[3] = 3;

	// Clear shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_clear_v,
		&clear_vertex_id);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_clear_f,
		&clear_fragment_id);

	const SceGxmProgram *clear_vertex_program = sceGxmShaderPatcherGetProgramFromId(clear_vertex_id);
	const SceGxmProgram *clear_fragment_program = sceGxmShaderPatcherGetProgramFromId(clear_fragment_id);

	clear_position = sceGxmProgramFindParameterByName(clear_vertex_program, "position");
	clear_depth = sceGxmProgramFindParameterByName(clear_vertex_program, "u_clear_depth");
	clear_color = sceGxmProgramFindParameterByName(clear_fragment_program, "u_clear_color");

	patchVertexProgram(gxm_shader_patcher,
		clear_vertex_id, NULL, 0, NULL, 0, &clear_vertex_program_patched);

	patchFragmentProgram(gxm_shader_patcher,
		clear_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		msaa_mode, NULL, NULL,
		&clear_fragment_program_patched);

	sceGxmSetTwoSidedEnable(gxm_context, SCE_GXM_TWO_SIDED_ENABLED);

	// Scissor Test shader register
	sceGxmShaderPatcherCreateMaskUpdateFragmentProgram(gxm_shader_patcher, &scissor_test_fragment_program);

	scissor_test_vertices = gpu_alloc_mapped(1 * sizeof(vector4f), VGL_MEM_RAM);

	// Init texture units
	int i, j;
	for (i = 0; i < COMBINED_TEXTURE_IMAGE_UNITS_NUM; i++) {
		sceClibMemset(&texture_units[i].env_color.r, 0, sizeof(vector4f));
		texture_units[i].env_mode = MODULATE;
		texture_units[i].tex_id = 0;
		texture_units[i].enabled = GL_FALSE;
		texture_units[i].texture_stack_counter = 0;
	}

	// Init custom shaders
	resetCustomShaders();

#ifdef HAVE_CIRCULAR_VERTEX_POOL
	vertex_data_pool = gpu_alloc_mapped(vertex_data_pool_size, VGL_MEM_RAM);
	vertex_data_pool_ptr = vertex_data_pool;
	vertex_data_pool_limit = (uint8_t *)vertex_data_pool + vertex_data_pool_size;
#endif

	// Init constant index buffers
	default_idx_ptr = (uint16_t *)vgl_malloc(MAX_IDX_NUMBER * sizeof(uint16_t), VGL_MEM_EXTERNAL);
	default_quads_idx_ptr = (uint16_t *)vgl_malloc(MAX_IDX_NUMBER * sizeof(uint16_t), VGL_MEM_EXTERNAL);
	default_line_strips_idx_ptr = (uint16_t *)vgl_malloc(MAX_IDX_NUMBER * sizeof(uint16_t), VGL_MEM_EXTERNAL);
	for (i = 0; i < MAX_IDX_NUMBER / 6; i++) {
		default_idx_ptr[i * 6] = i * 6;
		default_idx_ptr[i * 6 + 1] = i * 6 + 1;
		default_idx_ptr[i * 6 + 2] = i * 6 + 2;
		default_idx_ptr[i * 6 + 3] = i * 6 + 3;
		default_idx_ptr[i * 6 + 4] = i * 6 + 4;
		default_idx_ptr[i * 6 + 5] = i * 6 + 5;
		default_line_strips_idx_ptr[i * 6] = i * 3;
		default_line_strips_idx_ptr[i * 6 + 1] = i * 3 + 1;
		default_line_strips_idx_ptr[i * 6 + 2] = i * 3 + 1;
		default_line_strips_idx_ptr[i * 6 + 3] = i * 3 + 2;
		default_line_strips_idx_ptr[i * 6 + 4] = i * 3 + 2;
		default_line_strips_idx_ptr[i * 6 + 5] = i * 3 + 3;
		default_quads_idx_ptr[i * 6] = i * 4;
		default_quads_idx_ptr[i * 6 + 1] = i * 4 + 1;
		default_quads_idx_ptr[i * 6 + 2] = i * 4 + 3;
		default_quads_idx_ptr[i * 6 + 3] = i * 4 + 1;
		default_quads_idx_ptr[i * 6 + 4] = i * 4 + 2;
		default_quads_idx_ptr[i * 6 + 5] = i * 4 + 3;
	}

	// Init buffers
	for (i = 0; i < VERTEX_ATTRIBS_NUM; i++) {
		vertex_attrib_config[i].regIndex = i;
	}

	// Init default vertex attributes configurations
	for (i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		ffp_vertex_attrib_config[i].streamIndex = i;
		ffp_vertex_attrib_config[i].offset = 0;
		ffp_vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		legacy_vertex_attrib_config[i].streamIndex = i;
		legacy_vertex_attrib_config[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		legacy_vertex_stream_config[i].stride = sizeof(float) * LEGACY_VERTEX_STRIDE;
		legacy_vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	}
	legacy_vertex_attrib_config[0].offset = 0;
	legacy_vertex_attrib_config[1].offset = sizeof(float) * 3;
	legacy_vertex_attrib_config[2].offset = sizeof(float) * 5;
	legacy_vertex_attrib_config[3].offset = sizeof(float) * 9;
	legacy_vertex_attrib_config[4].offset = sizeof(float) * 13;
	legacy_vertex_attrib_config[5].offset = sizeof(float) * 17;
	legacy_vertex_attrib_config[6].offset = sizeof(float) * 21;
	legacy_vertex_attrib_config[7].offset = sizeof(float) * 24;
	legacy_vertex_attrib_config[0].componentCount = 3;
	legacy_vertex_attrib_config[1].componentCount = 2;
	legacy_vertex_attrib_config[2].componentCount = 4;
	legacy_vertex_attrib_config[3].componentCount = 4;
	legacy_vertex_attrib_config[4].componentCount = 4;
	legacy_vertex_attrib_config[5].componentCount = 4;
	legacy_vertex_attrib_config[6].componentCount = 3;
	legacy_vertex_attrib_config[7].componentCount = 2;
	legacy_pool_size = pool_size;

	// Initializing lights configs
	for (i = 0; i < MAX_LIGHTS_NUM; i++) {
		float data[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		sceClibMemcpy(&lights_ambients[i].r, &data[0], sizeof(float) * 4);
		data[3] = 1.0f;
		data[4] = 0.0f;
		sceClibMemcpy(&lights_positions[i].r, &data[0], sizeof(float) * 4);
		lights_attenuations[i].r = 1.0f;
		lights_attenuations[i].g = 0.0f;
		lights_attenuations[i].b = 0.0f;
		if (i == 0) {
			float data2[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			sceClibMemcpy(&lights_diffuses[i].r, &data2[0], sizeof(float) * 4);
			sceClibMemcpy(&lights_speculars[i].r, &data2[0], sizeof(float) * 4);
		} else {
			sceClibMemset(&lights_diffuses[i].r, 0, sizeof(float) * 4);
			sceClibMemset(&lights_speculars[i].r, 0, sizeof(float) * 4);
		}
	}

	// Init purge lists
	for (i = 0; i < FRAME_PURGE_FREQ; i++) {
		frame_purge_list[i][0] = NULL;
		frame_rt_purge_list[i][0] = NULL;
	}

	// Init scissor test state
	resetScissorTestRegion();

	// Allocating default texture object
	texture_slots[0].mip_count = 1;
	texture_slots[0].use_mips = GL_FALSE;
	texture_slots[0].min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
	texture_slots[0].mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
	texture_slots[0].mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
	texture_slots[0].u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
	texture_slots[0].v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
	texture_slots[0].lod_bias = GL_MAX_TEXTURE_LOD_BIAS;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Defaulting textures into using texture on ID 0 and resetting free textures queue
	for (i = 1; i < TEXTURES_NUM; i++) {
		texture_slots[i].status = TEX_UNUSED;
		texture_slots[i].gxm_tex = texture_slots[0].gxm_tex;
	}

	// Set texture matrix to identity
	matrix4x4_identity(texture_matrix);

#ifdef HAVE_RAZOR_INTERFACE
	vgl_debugger_init();
#endif
}

void vglInitWithCustomThreshold(int pool_size, int width, int height, int ram_threshold, int cdram_threshold, int phycont_threshold, SceGxmMultisampleMode msaa) {
	// Initializing sceGxm
	initGxm();

	// Getting max allocatable CDRAM and RAM memory
	if (system_app_mode) {
		SceAppMgrBudgetInfo info;
		info.size = sizeof(SceAppMgrBudgetInfo);
		sceAppMgrGetBudgetInfo(&info);
		vglInitWithCustomSizes(pool_size, width, height, info.free_user_rw > ram_threshold ? info.free_user_rw - ram_threshold : info.free_user_rw, 0, 0, msaa);
	} else {
		SceKernelFreeMemorySizeInfo info;
		info.size = sizeof(SceKernelFreeMemorySizeInfo);
		sceKernelGetFreeMemorySize(&info);
		vglInitWithCustomSizes(pool_size, width, height, info.size_user > ram_threshold ? info.size_user - ram_threshold : info.size_user, info.size_cdram > cdram_threshold ? info.size_cdram - cdram_threshold : 0, info.size_phycont > phycont_threshold ? info.size_phycont - phycont_threshold : 0, msaa);
	}
}

void vglInitExtended(int pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa) {
	vglInitWithCustomThreshold(pool_size, width, height, ram_threshold, 256 * 1024, 1 * 1024 * 1024, msaa);
}

void vglInit(int pool_size) {
	vglInitExtended(pool_size, DISPLAY_WIDTH_DEF, DISPLAY_HEIGHT_DEF, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
}

void vglEnd(void) {
	// Wait for rendering to be finished
	waitRenderingDone();

	// Deallocating default vertices buffers
	vgl_free(clear_vertices);
	vgl_free(depth_vertices);
	vgl_free(depth_clear_indices);
	vgl_free(scissor_test_vertices);

	// Releasing shader programs from sceGxmShaderPatcher
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, scissor_test_fragment_program);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, clear_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, clear_fragment_program_patched);

	// Unregistering shader programs from sceGxmShaderPatcher
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_fragment_id);

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

#ifdef HAVE_RAZOR
	// Terminating sceRazor debugger
#ifdef HAVE_DEVKIT
	sceSysmoduleUnloadModule(SCE_SYSMODULE_RAZOR_HUD);
	sceSysmoduleUnloadModule(SCE_SYSMODULE_RAZOR_CAPTURE);
#else
	sceKernelStopUnloadModule(razor_modid, 0, NULL, 0, NULL, NULL);
#endif
#endif
}

void vglWaitVblankStart(GLboolean enable) {
	vsync_interval = enable ? 1 : 0;
}

// openGL implementation

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
	switch (sfactor) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (dfactor) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
	switch (srcRGB) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (dstRGB) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (srcAlpha) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (dstAlpha) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glBlendEquation(GLenum mode) {
	switch (mode) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
	switch (modeRGB) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (modeAlpha) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	blend_color_mask = SCE_GXM_COLOR_MASK_NONE;
	if (red)
		blend_color_mask += SCE_GXM_COLOR_MASK_R;
	if (green)
		blend_color_mask += SCE_GXM_COLOR_MASK_G;
	if (blue)
		blend_color_mask += SCE_GXM_COLOR_MASK_B;
	if (alpha)
		blend_color_mask += SCE_GXM_COLOR_MASK_A;
	if (blend_state)
		change_blend_factor();
	else
		change_blend_mask();
}

extern GLboolean skip_this_draw;

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	if (cur_program != 0)
		is_draw_legal = _glDrawArrays_CustomShadersIMPL(first + count);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawArrays_FixedFunctionIMPL(first + count);
	}

#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
#endif
	{
		uint16_t *ptr;
		switch (mode) {
		case GL_QUADS:
			ptr = default_quads_idx_ptr + (first / 2) * 3;
			count = (count / 2) * 3;
			break;
		case GL_LINE_STRIP:
			ptr = default_line_strips_idx_ptr + first * 2;
			count = (count - 1) * 2;
			break;
		case GL_LINE_LOOP:
			ptr = gpu_alloc_mapped_temp(count * 2 * sizeof(uint16_t));
			sceClibMemcpy(ptr, default_line_strips_idx_ptr + first * 2, (count - 1) * 2 * sizeof(uint16_t));
			ptr[(count - 1) * 2] = first + count - 1;
			ptr[(count - 1) * 2 + 1] = first;

			count *= 2;
			break;
		default:
			ptr = default_idx_ptr + first;
			break;
		}

		sceClibPrintf("Draw on drawArrays: %X\n", sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count));
	}
	restore_polygon_mode(gxm_p);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *gl_indices) {
#ifndef SKIP_ERROR_HANDLING
	if (type != GL_UNSIGNED_SHORT) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	gpubuffer *gpu_buf = (gpubuffer *)index_array_unit;
	uint16_t *src = gpu_buf ? (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices) : (uint16_t *)gl_indices;
	if (cur_program != 0)
		is_draw_legal = _glDrawElements_CustomShadersIMPL(src, count);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawElements_FixedFunctionIMPL(src, count);
	}

//#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
//#endif
	{
		uint16_t *ptr;
		// Directly use the current IBO if there is no need for a temporary index buffer.
		if (gpu_buf != NULL && !prim_is_non_native) {
			ptr = (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices);
			gpu_buf->used = GL_TRUE;
		} else {
			int i;
			switch (mode) {
			case GL_QUADS:
				ptr = gpu_alloc_mapped_temp(count * 3 * sizeof(uint16_t));
				for (i = 0; i < count / 4; i++) {
					ptr[i * 6] = src[i * 4];
					ptr[i * 6 + 1] = src[i * 4 + 1];
					ptr[i * 6 + 2] = src[i * 4 + 3];
					ptr[i * 6 + 3] = src[i * 4 + 1];
					ptr[i * 6 + 4] = src[i * 4 + 2];
					ptr[i * 6 + 5] = src[i * 4 + 3];
				}
				count = (count / 2) * 3;
				break;
			case GL_LINE_STRIP:
				ptr = gpu_alloc_mapped_temp((count - 1) * 2 * sizeof(uint16_t));
				for (i = 0; i < count - 1; i++) {
					ptr[i * 2] = src[i];
					ptr[i * 2 + 1] = src[i + 1];
				}
				count = (count - 1) * 2;
				break;
			case GL_LINE_LOOP:
				ptr = gpu_alloc_mapped_temp(count * 2 * sizeof(uint16_t));
				for (i = 0; i < count - 1; i++) {
					ptr[i * 2] = src[i];
					ptr[i * 2 + 1] = src[i + 1];
				}
				ptr[i * 2] = src[count - 1];
				ptr[i * 2 + 1] = src[0];
				count = count * 2;
				break;
			default:
				ptr = gpu_alloc_mapped_temp(count * sizeof(uint16_t));
				sceClibMemcpy(ptr, src, count * sizeof(uint16_t));
				break;
			}
		}

		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
	}

	restore_polygon_mode(gxm_p);
}

void vglDrawObjects(GLenum mode, GLsizei count, GLboolean implicit_wvp) {
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();

	texture_unit *tex_unit = &texture_units[0];
	if (cur_program != 0) {
		_vglDrawObjects_CustomShadersIMPL(implicit_wvp);
		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, index_object, count);
	} else if (ffp_vertex_attrib_state & (1 << 0)) {
		reload_ffp_shaders(NULL, NULL);
		if (ffp_vertex_attrib_state & (1 << 1)) {
			if (texture_slots[tex_unit->tex_id].status != TEX_VALID)
				return;
			sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[tex_unit->tex_id].gxm_tex);
			sceGxmSetVertexStream(gxm_context, 1, texture_object);
			if (ffp_vertex_num_params > 2)
				sceGxmSetVertexStream(gxm_context, 2, color_object);
		} else if (ffp_vertex_num_params > 1)
			sceGxmSetVertexStream(gxm_context, 1, color_object);
		sceGxmSetVertexStream(gxm_context, 0, vertex_object);
		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, index_object, count);
	}

	restore_polygon_mode(gxm_p);
}

size_t vglMemFree(vglMemType type) {
#ifndef SKIP_ERROR_HANDLING
	if (type >= VGL_MEM_ALL)
		return 0;
#endif
	return vgl_mem_get_free_space(type);
}

void *vglAlloc(uint32_t size, vglMemType type) {
#ifndef SKIP_ERROR_HANDLING
	if (type >= VGL_MEM_ALL)
		return NULL;
#endif
	return vgl_malloc(size, type);
}

void *vglForceAlloc(uint32_t size) {
	return gpu_alloc_mapped(size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);
}

void vglFree(void *addr) {
	vgl_free(addr);
}

void vglUseExtraMem(GLboolean use) {
	use_extra_mem = use;
}

GLboolean vglHasRuntimeShaderCompiler(void) {
	return is_shark_online;
}

void vglSetVertexPoolSize(uint32_t size) {
#ifdef HAVE_CIRCULAR_VERTEX_POOL
	vertex_data_pool_size = size;
#endif
}
