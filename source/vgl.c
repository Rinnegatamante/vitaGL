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
 * vgl.c:
 * Implementation for custom vitaGL functions
 */
#include "shared.h"
#include "texture_callbacks.h"
#include "vitaGL.h"

static GLboolean vgl_inited = GL_FALSE;

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
#ifdef HAVE_VITA3K_SUPPORT // Vita3K can't use ShaccCg extensions due to lack of taiHEN support, so we need to provide precompiled glClear shaders
#include "shaders/precompiled_clear_f.h"
#include "shaders/precompiled_clear_v.h"
#else
const char clear_f[] = " \
	float4 main(uniform float4 u_clear_color) : COLOR \
	{ \
		return u_clear_color; \
	}";
const char clear_v[] = " \
	float4 main(unsigned int idx : INDEX, uniform float4 position, uniform float u_clear_depth) : POSITION \
	{ \
		float x = (idx == 1 || idx == 2) ? position[1] : position[0]; \
		float y = (idx == 2 || idx == 3) ? position[3] : position[2]; \
		return float4(x, y, u_clear_depth, 1.f); \
	}";
#endif

SceGxmShaderPatcherId clear_vertex_id;
SceGxmShaderPatcherId clear_fragment_id;
const SceGxmProgramParameter *clear_position;
const SceGxmProgramParameter *clear_depth;
const SceGxmProgramParameter *clear_color;
SceGxmVertexProgram *clear_vertex_program_patched;
SceGxmFragmentProgram *clear_fragment_program_patched;
SceGxmFragmentProgram *clear_fragment_program_float_patched;
vector4f *clear_vertices = NULL; // Memblock starting address for clear screen vertices
vector3f *depth_vertices = NULL; // Memblock starting address for depth clear screen vertices

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

// Disable color buffer shader
uint16_t *depth_clear_indices = NULL; // Memblock starting address for clear screen indices

// Internal stuffs
SceGxmMultisampleMode msaa_mode = SCE_GXM_MULTISAMPLE_NONE;
extern GLboolean use_vram_for_usse;

uint16_t *default_idx_ptr; // sceGxm mapped progressive indices buffer
uint16_t *default_quads_idx_ptr; // sceGxm mapped progressive indices buffer for quads
uint16_t *default_line_strips_idx_ptr; // sceGxm mapped progressive indices buffer for line strips

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

void vector4f_convert_to_local_space(vector4f *out, int x, int y, int width, int height) {
	float target_w, target_h;
	if (is_rendering_display) {
		target_w = DISPLAY_WIDTH_FLOAT;
		target_h = DISPLAY_HEIGHT_FLOAT;
	} else {
		target_w = in_use_framebuffer->width;
		target_h = in_use_framebuffer->height;
	}
	out->x = (float)(2 * x) / target_w - 1.0f;
	out->y = (float)(2 * (x + width)) / target_w - 1.0f;
	out->z = 1.0f - (float)(2 * y) / target_h;
	out->w = 1.0f - (float)(2 * (y + height)) / target_h;
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void vglUseVram(GLboolean usage) {
	use_vram = usage;
}

void vglUseVramForUSSE(GLboolean usage) {
	use_vram_for_usse = usage;
}

GLboolean vglInitWithCustomSizes(int pool_size, int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, int cdlg_pool_size, SceGxmMultisampleMode msaa) {
	// Check if vitaGL has been already inited
	if (vgl_inited) {
		vgl_log("%s:%d: Suppressed an attempt at initing vitaGL while it's already inited.\n", __FILE__, __LINE__);
		return GL_FALSE;
	}

#ifndef DISABLE_ADVANCED_SHADER_CACHE
	sceIoMkdir("ux0:data/shader_cache", 0777);
	char fname[256];
	sprintf(fname, "ux0:data/shader_cache/v%d", SHADER_CACHE_MAGIC);
	sceIoMkdir(fname, 0777);
#endif
	// Check if framebuffer size is valid
	GLboolean res_fallback = GL_FALSE;
	int max_w = width, max_h = height;
	sceDisplayGetMaximumFrameBufResolution(&max_w, &max_h);
	if (width > max_w || height > max_h) {
		width = max_w;
		height = max_h;
		res_fallback = GL_TRUE;
	}

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
	vgl_mem_init(ram_pool_size, cdram_pool_size, phycont_pool_size, cdlg_pool_size);

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

#ifdef HAVE_VITA3K_SUPPORT // Vita3K can't use ShaccCg extensions due to lack of taiHEN support, so we need to provide precompiled glClear shaders
	{
		SceGxmProgram *gxm_program_clear_v = (SceGxmProgram *)&clear_v;
		SceGxmProgram *gxm_program_clear_f = (SceGxmProgram *)&clear_f;
#else
	// Compile clear shaders only if shader compiler is up
	if (is_shark_online) {
		uint32_t size = strlen(clear_v);
		SceGxmProgram *p = shark_compile_shader_extended(clear_v, &size, SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		SceGxmProgram *gxm_program_clear_v = (SceGxmProgram *)vglMalloc(size);
		vgl_fast_memcpy((void *)gxm_program_clear_v, (void *)p, size);
		shark_clear_output();
		size = strlen(clear_f);
		p = shark_compile_shader_extended(clear_f, &size, SHARK_FRAGMENT_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		SceGxmProgram *gxm_program_clear_f = (SceGxmProgram *)vglMalloc(size);
		vgl_fast_memcpy((void *)gxm_program_clear_f, (void *)p, size);
		shark_clear_output();
#endif
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
		{
			patchFragmentProgram(gxm_shader_patcher,
				clear_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
				msaa_mode, NULL, NULL,
				&clear_fragment_program_patched);
		}
		{
			patchFragmentProgram(gxm_shader_patcher,
				clear_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_HALF4,
				msaa_mode, NULL, NULL,
				&clear_fragment_program_float_patched);
		}
	}

	sceGxmSetTwoSidedEnable(gxm_context, SCE_GXM_TWO_SIDED_ENABLED);

	// Scissor Test shader register
	sceGxmShaderPatcherCreateMaskUpdateFragmentProgram(gxm_shader_patcher, &scissor_test_fragment_program);

	scissor_test_vertices = gpu_alloc_mapped(1 * sizeof(vector4f), VGL_MEM_RAM);

	// Init texture units
	int i, j;
	for (i = 0; i < COMBINED_TEXTURE_IMAGE_UNITS_NUM; i++) {
		sceClibMemset(&texture_units[i].env_color.r, 0, sizeof(vector4f));
		texture_units[i].env_mode = MODULATE;
		texture_units[i].tex_id[0] = 0;
		texture_units[i].tex_id[1] = 0;
		texture_units[i].tex_id[2] = 0;
		texture_units[i].state = 0;
		texture_units[i].texture_stack_counter = 0;
#ifndef DISABLE_TEXTURE_COMBINER
		texture_units[i].combiner.rgb_func = MODULATE;
		texture_units[i].combiner.a_func = MODULATE;
		texture_units[i].combiner.op_rgb_0 = TEXTURE;
		texture_units[i].combiner.op_rgb_1 = PREVIOUS;
		texture_units[i].combiner.op_rgb_2 = CONSTANT;
		texture_units[i].combiner.op_a_0 = TEXTURE;
		texture_units[i].combiner.op_a_1 = PREVIOUS;
		texture_units[i].combiner.op_a_2 = CONSTANT;
		texture_units[i].combiner.op_mode_rgb_0 = SRC_COLOR;
		texture_units[i].combiner.op_mode_rgb_1 = SRC_COLOR;
		texture_units[i].combiner.op_mode_rgb_2 = SRC_ALPHA;
		texture_units[i].combiner.op_mode_a_0 = SRC_ALPHA;
		texture_units[i].combiner.op_mode_a_1 = SRC_ALPHA;
		texture_units[i].combiner.op_mode_a_2 = SRC_ALPHA;
#endif
		texture_units[i].rgb_scale = 1.0f;
		texture_units[i].a_scale = 1.0f;
	}

	// Init custom shaders
	resetCustomShaders();

	// Init default vao
	resetVao(cur_vao);

#ifdef HAVE_CIRCULAR_VERTEX_POOL
	vertex_data_pool = gpu_alloc_mapped(vertex_data_pool_size, VGL_MEM_RAM);
	vertex_data_pool_ptr = vertex_data_pool;
	vertex_data_pool_limit = (uint8_t *)vertex_data_pool + vertex_data_pool_size;
#endif

	// Init constant index buffers
	default_idx_ptr = (uint16_t *)vglMalloc(MAX_IDX_NUMBER * sizeof(uint16_t));
	default_quads_idx_ptr = (uint16_t *)vglMalloc(MAX_IDX_NUMBER * sizeof(uint16_t));
	default_line_strips_idx_ptr = (uint16_t *)vglMalloc(MAX_IDX_NUMBER * sizeof(uint16_t));
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

	// Init default vertex attributes configurations
	for (i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		// Textureless variant
		if (i < FFP_VERTEX_ATTRIBS_NUM - 2) {
			legacy_nt_vertex_attrib_config[i].streamIndex = i;
			legacy_nt_vertex_attrib_config[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			legacy_nt_vertex_stream_config[i].stride = sizeof(float) * LEGACY_NT_VERTEX_STRIDE;
			legacy_nt_vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		}

		// Single Texture variant
		if (i < FFP_VERTEX_ATTRIBS_NUM - 1) {
			legacy_vertex_attrib_config[i].streamIndex = i;
			legacy_vertex_attrib_config[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			legacy_vertex_stream_config[i].stride = sizeof(float) * LEGACY_VERTEX_STRIDE;
			legacy_vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		}

		// Multi Texture variant
		legacy_mt_vertex_attrib_config[i].streamIndex = i;
		legacy_mt_vertex_attrib_config[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		legacy_mt_vertex_stream_config[i].stride = sizeof(float) * LEGACY_MT_VERTEX_STRIDE;
		legacy_mt_vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		// Non-immediate mode variant
		ffp_vertex_attrib_config[i].streamIndex = i;
		ffp_vertex_attrib_config[i].offset = 0;
		ffp_vertex_attrib_config[i].componentCount = 4;
		ffp_vertex_attrib_config[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		ffp_vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	}

	// Textureless Variant
	legacy_nt_vertex_attrib_config[0].offset = 0; // Position
	legacy_nt_vertex_attrib_config[1].offset = sizeof(float) * 3; // Color/Ambient
	legacy_nt_vertex_attrib_config[2].offset = sizeof(float) * 7; // Diffuse
	legacy_nt_vertex_attrib_config[3].offset = sizeof(float) * 11; // Specular
	legacy_nt_vertex_attrib_config[4].offset = sizeof(float) * 15; // Emission
	legacy_nt_vertex_attrib_config[5].offset = sizeof(float) * 19; // Normals
	legacy_nt_vertex_attrib_config[0].componentCount = 3;
	legacy_nt_vertex_attrib_config[1].componentCount = 4;
	legacy_nt_vertex_attrib_config[2].componentCount = 4;
	legacy_nt_vertex_attrib_config[3].componentCount = 4;
	legacy_nt_vertex_attrib_config[4].componentCount = 4;
	legacy_nt_vertex_attrib_config[5].componentCount = 3;

	// Single Texture Variant
	legacy_vertex_attrib_config[0].offset = 0; // Position
	legacy_vertex_attrib_config[1].offset = sizeof(float) * 3; // Texcoord (UNIT0)
	legacy_vertex_attrib_config[2].offset = sizeof(float) * 5; // Color/Ambient
	legacy_vertex_attrib_config[3].offset = sizeof(float) * 9; // Diffuse
	legacy_vertex_attrib_config[4].offset = sizeof(float) * 13; // Specular
	legacy_vertex_attrib_config[5].offset = sizeof(float) * 17; // Emission
	legacy_vertex_attrib_config[6].offset = sizeof(float) * 21; // Normals
	legacy_vertex_attrib_config[0].componentCount = 3;
	legacy_vertex_attrib_config[1].componentCount = 2;
	legacy_vertex_attrib_config[2].componentCount = 4;
	legacy_vertex_attrib_config[3].componentCount = 4;
	legacy_vertex_attrib_config[4].componentCount = 4;
	legacy_vertex_attrib_config[5].componentCount = 4;
	legacy_vertex_attrib_config[6].componentCount = 3;

	// Multi Texture Variant
	legacy_mt_vertex_attrib_config[0].offset = 0; // Position
	legacy_mt_vertex_attrib_config[1].offset = sizeof(float) * 3; // Texcoord (UNIT0)
	legacy_mt_vertex_attrib_config[2].offset = sizeof(float) * 5; // Texcoord (UNIT1)
	legacy_mt_vertex_attrib_config[3].offset = sizeof(float) * 7; // Color/Ambient
	legacy_mt_vertex_attrib_config[4].offset = sizeof(float) * 11; // Diffuse
	legacy_mt_vertex_attrib_config[5].offset = sizeof(float) * 15; // Specular
	legacy_mt_vertex_attrib_config[6].offset = sizeof(float) * 19; // Emission
	legacy_mt_vertex_attrib_config[7].offset = sizeof(float) * 23; // Normals
	legacy_mt_vertex_attrib_config[0].componentCount = 3;
	legacy_mt_vertex_attrib_config[1].componentCount = 2;
	legacy_mt_vertex_attrib_config[2].componentCount = 2;
	legacy_mt_vertex_attrib_config[3].componentCount = 4;
	legacy_mt_vertex_attrib_config[4].componentCount = 4;
	legacy_mt_vertex_attrib_config[5].componentCount = 4;
	legacy_mt_vertex_attrib_config[6].componentCount = 4;
	legacy_mt_vertex_attrib_config[7].componentCount = 3;

	// Init vertex pool for immediate mode support
	legacy_pool_size = pool_size;

	// Initializing lights configs
	for (i = 0; i < MAX_LIGHTS_NUM; i++) {
		float data[4] = {0.0f, 0.0f, 0.0f, 1.0f};
		vgl_fast_memcpy(&lights_ambients[i].r, &data[0], sizeof(float) * 4);
		data[3] = 1.0f;
		data[4] = 0.0f;
		vgl_fast_memcpy(&lights_positions[i].r, &data[0], sizeof(float) * 4);
		lights_attenuations[i].r = 1.0f;
		lights_attenuations[i].g = 0.0f;
		lights_attenuations[i].b = 0.0f;
		if (i == 0) {
			float data2[4] = {1.0f, 1.0f, 1.0f, 1.0f};
			vgl_fast_memcpy(&lights_diffuses[i].r, &data2[0], sizeof(float) * 4);
			vgl_fast_memcpy(&lights_speculars[i].r, &data2[0], sizeof(float) * 4);
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
		texture_slots[i].palette_data = NULL;
	}

	// Init texture matrices to identity
	for (i = 0; i < TEXTURE_COORDS_NUM; i++) {
		matrix4x4_identity(texture_matrix[i]);
	}

#ifdef HAVE_RAZOR_INTERFACE
	vgl_debugger_init();
#endif

	vgl_inited = GL_TRUE;
	return res_fallback;
}

GLboolean vglInitWithCustomThreshold(int pool_size, int width, int height, int ram_threshold, int cdram_threshold, int phycont_threshold, int cdlg_threshold, SceGxmMultisampleMode msaa) {
	// Initializing sceGxm
	initGxm();

	// Getting max allocatable CDRAM and RAM memory
	if (system_app_mode) {
		SceAppMgrBudgetInfo info;
		info.size = sizeof(SceAppMgrBudgetInfo);
		sceAppMgrGetBudgetInfo(&info);
		return vglInitWithCustomSizes(pool_size, width, height, info.free_user_rw > ram_threshold ? info.free_user_rw - ram_threshold : info.free_user_rw, 0, 0, 0, msaa);
	} else {
		SceKernelFreeMemorySizeInfo info;
		info.size = sizeof(SceKernelFreeMemorySizeInfo);
		sceKernelGetFreeMemorySize(&info);
		return vglInitWithCustomSizes(pool_size, width, height,
			info.size_user > ram_threshold ? info.size_user - ram_threshold : 0,
			info.size_cdram > cdram_threshold ? info.size_cdram - cdram_threshold : 0,
			info.size_phycont > phycont_threshold ? info.size_phycont - phycont_threshold : 0,
			SCE_KERNEL_MAX_MAIN_CDIALOG_MEM_SIZE > cdlg_threshold ? SCE_KERNEL_MAX_MAIN_CDIALOG_MEM_SIZE - cdlg_threshold : 0, msaa);
	}
}

GLboolean vglInitExtended(int pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa) {
	return vglInitWithCustomThreshold(pool_size, width, height, ram_threshold, 0, 0, SCE_KERNEL_MAX_MAIN_CDIALOG_MEM_SIZE, msaa);
}

GLboolean vglInit(int pool_size) {
	return vglInitExtended(pool_size, DISPLAY_WIDTH_DEF, DISPLAY_HEIGHT_DEF, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
}

void vglEnd(void) {
	// Wait for rendering to be finished
	waitRenderingDone();

	// Deallocating default vertices buffers
	vglFree(clear_vertices);
	vglFree(depth_vertices);
	vglFree(depth_clear_indices);
	vglFree(scissor_test_vertices);

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

	vgl_inited = GL_FALSE;
}

void vglWaitVblankStart(GLboolean enable) {
	vsync_interval = enable ? 1 : 0;
}

size_t vglMemFree(vglMemType type) {
#ifndef SKIP_ERROR_HANDLING
	if (type >= VGL_MEM_ALL)
		return 0;
#endif
	return vgl_mem_get_free_space(type);
}

size_t vglMemTotal(vglMemType type) {
#ifndef SKIP_ERROR_HANDLING
	if (type >= VGL_MEM_ALL)
		return 0;
#endif
	return vgl_mem_get_total_space(type);
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

void *vglMalloc(uint32_t size) {
	// First we try to use newlib mem
	void *res = vgl_malloc(size, VGL_MEM_EXTERNAL);
	if (res)
		return res;

	// If it fails, we try with standard RAM mem pool
	res = vgl_malloc(size, VGL_MEM_RAM);
	if (res)
		return res;

	// If it fails, we try with physically contiguous RAM
	res = vgl_malloc(size, VGL_MEM_SLOW);
	if (res)
		return res;

	// If it fails, we try with common dialog mem
	res = vgl_malloc(size, VGL_MEM_BUDGET);
	if (res)
		return res;

	// If it fails, as last resort, we try VRAM
	return vgl_malloc(size, VGL_MEM_VRAM);
}

size_t vglMallocUsableSize(void *ptr) {
	return vgl_malloc_usable_size(ptr);
}

void *vglMemalign(uint32_t alignment, uint32_t size) {
	// First we try to use newlib mem
	void *res = vgl_memalign(alignment, size, VGL_MEM_EXTERNAL);
	if (res)
		return res;

	// If it fails, we try with standard RAM mem pool
	res = vgl_memalign(alignment, size, VGL_MEM_RAM);
	if (res)
		return res;

	// If it fails, we try with physically contiguous RAM
	res = vgl_memalign(alignment, size, VGL_MEM_SLOW);
	if (res)
		return res;

	// If it fails, we try with common dialog mem
	res = vgl_memalign(alignment, size, VGL_MEM_BUDGET);
	if (res)
		return res;

	// If it fails, as last resort, we try VRAM
	return vgl_memalign(alignment, size, VGL_MEM_VRAM);
}

void *vglCalloc(uint32_t nmember, uint32_t size) {
	// First we try to use newlib mem
	void *res = vgl_calloc(nmember, size, VGL_MEM_EXTERNAL);
	if (res)
		return res;

	// If it fails, we try with standard RAM mem pool
	res = vgl_calloc(nmember, size, VGL_MEM_RAM);
	if (res)
		return res;

	// If it fails, we try with physically contiguous RAM
	res = vgl_calloc(nmember, size, VGL_MEM_SLOW);
	if (res)
		return res;

	// If it fails, we try with common dialog mem
	res = vgl_calloc(nmember, size, VGL_MEM_BUDGET);
	if (res)
		return res;

	// If it fails, as last resort, we try VRAM
	return vgl_calloc(nmember, size, VGL_MEM_VRAM);
}

void *vglRealloc(void *ptr, uint32_t size) {
	if (!ptr)
		return vglMalloc(size);

	void *res = vgl_realloc(ptr, size);
	if (res)
		return res;

	res = vglMalloc(size);
	if (res) {
		vgl_fast_memcpy(res, ptr, vgl_malloc_usable_size(ptr));
		vglFree(ptr);
	}

	return res;
}

void vglFree(void *addr) {
	if (!addr)
		return;

	vgl_free(addr);
}

void vglUseExtraMem(GLboolean use) {
	use_extra_mem = use;
}

void vglSetVertexPoolSize(uint32_t size) {
#ifdef HAVE_CIRCULAR_VERTEX_POOL
	vertex_data_pool_size = size;
#endif
}

void vglUseCachedMem(GLboolean use) {
	has_cached_mem = use;
}
