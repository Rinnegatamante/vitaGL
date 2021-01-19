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
#include "shaders/disable_color_buffer_f.h"
#include "shaders/rgba_f.h"
#include "shaders/rgba_v.h"
#include "shaders/texture2d_f.h"
#include "shaders/texture2d_rgba_f.h"
#include "shaders/texture2d_rgba_v.h"
#include "shaders/texture2d_v.h"
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
#include "shaders/ffp_v.h"
#include "shaders/ffp_f.h"
#endif

#define MAX_IDX_NUMBER 8096 // Maximum allowed number of indices per draw call

#ifdef HAVE_SOFTFP_ABI
__attribute__((naked)) void sceGxmSetViewport_sfp(SceGxmContext *context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale) {
  asm volatile (
    "vmov s0, r1\n"
    "vmov s1, r2\n"
    "vmov s2, r3\n"
    "ldr r1, [sp]\n"
    "ldr r2, [sp, #4]\n"
    "ldr r3, [sp, #8]\n"
    "vmov s3, r1\n"
    "vmov s4, r2\n"
    "vmov s5, r2\n"
    "b sceGxmSetViewport\n"
  );
}
#endif

typedef enum {
	TEX2D_WVP_UNIF,
	TEX2D_ALPHA_CUT_UNIF,
	TEX2D_ALPHA_MODE_UNIF,
	TEX2D_TEX_ENV_MODE_UNIF,
	TEX2D_CLIP_PLANE0_UNIF,
	TEX2D_CLIP_PLANEO_EQUATION_UNIF,
	TEX2D_MODELVIEW_UNIF,
	TEX2D_FOG_MODE_UNIF,
	TEX2D_FOG_NEAR_UNIF,
	TEX2D_FOG_FAR_UNIF,
	TEX2D_FOG_DENSITY_UNIF,
	TEX2D_FOG_COLOR_UNIF,
	TEX2D_TEX_ENV_COLOR_UNIF,
	TEX2D_TEXMAT_UNIF
} TEX2D_UNIFS;

// Disable color buffer shader
SceGxmShaderPatcherId disable_color_buffer_fragment_id;
const SceGxmProgramParameter *disable_color_buffer_position;
SceGxmFragmentProgram *disable_color_buffer_fragment_program_patched;
const SceGxmProgramParameter *clear_depth;

// Clear shader
SceGxmShaderPatcherId clear_vertex_id;
SceGxmShaderPatcherId clear_fragment_id;
const SceGxmProgramParameter *clear_position;
const SceGxmProgramParameter *clear_color;
SceGxmVertexProgram *clear_vertex_program_patched;
SceGxmFragmentProgram *clear_fragment_program_patched;

// Color (RGBA/RGB) shader
SceGxmShaderPatcherId rgba_vertex_id;
SceGxmShaderPatcherId rgba_fragment_id;
const SceGxmProgramParameter *rgba_wvp;
SceGxmVertexProgram *rgba_vertex_program_patched;
SceGxmVertexProgram *rgba_u8n_vertex_program_patched;
SceGxmVertexProgram *rgb_vertex_program_patched;
SceGxmVertexProgram *rgb_u8n_vertex_program_patched;
SceGxmFragmentProgram *rgba_fragment_program_patched;
const SceGxmProgram *rgba_fragment_program;
blend_config rgba_blend_cfg;

// Texture2D shader
SceGxmShaderPatcherId texture2d_vertex_id;
SceGxmShaderPatcherId texture2d_fragment_id;
const SceGxmProgramParameter *texture2d_generic_unifs[TEX2D_UNIFS_NUM];
const SceGxmProgramParameter *texture2d_tint_color;
SceGxmVertexProgram *texture2d_vertex_program_patched;
SceGxmFragmentProgram *texture2d_fragment_program_patched;
const SceGxmProgram *texture2d_fragment_program;
blend_config texture2d_blend_cfg;

// Texture2D+RGBA shader
SceGxmShaderPatcherId texture2d_rgba_vertex_id;
SceGxmShaderPatcherId texture2d_rgba_fragment_id;
const SceGxmProgramParameter *texture2d_rgba_generic_unifs[TEX2D_UNIFS_NUM];
SceGxmVertexProgram *texture2d_rgba_vertex_program_patched;
SceGxmVertexProgram *texture2d_rgba_u8n_vertex_program_patched;
SceGxmFragmentProgram *texture2d_rgba_fragment_program_patched;
const SceGxmProgram *texture2d_rgba_fragment_program;
blend_config texture2d_rgba_blend_cfg;

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

GLboolean vblank = GL_TRUE; // Current setting for VSync

extern int _newlib_heap_memblock; // Newlib Heap memblock
extern unsigned _newlib_heap_size; // Newlib Heap size

static const SceGxmProgram *const gxm_program_disable_color_buffer_f = (SceGxmProgram *)&disable_color_buffer_f;
static const SceGxmProgram *const gxm_program_clear_v = (SceGxmProgram *)&clear_v;
static const SceGxmProgram *const gxm_program_clear_f = (SceGxmProgram *)&clear_f;
static const SceGxmProgram *const gxm_program_rgba_v = (SceGxmProgram *)&rgba_v;
static const SceGxmProgram *const gxm_program_rgba_f = (SceGxmProgram *)&rgba_f;
static const SceGxmProgram *const gxm_program_texture2d_v = (SceGxmProgram *)&texture2d_v;
static const SceGxmProgram *const gxm_program_texture2d_f = (SceGxmProgram *)&texture2d_f;
static const SceGxmProgram *const gxm_program_texture2d_rgba_v = (SceGxmProgram *)&texture2d_rgba_v;
static const SceGxmProgram *const gxm_program_texture2d_rgba_f = (SceGxmProgram *)&texture2d_rgba_f;

// Disable color buffer shader
uint16_t *depth_clear_indices = NULL; // Memblock starting address for clear screen indices

// Clear shaders
SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
vector4f *clear_vertices = NULL; // Memblock starting address for clear screen vertices
vector3f *depth_vertices = NULL; // Memblock starting address for depth clear screen vertices

// Internal stuffs
blend_config blend_info; // Current blend info mode
SceGxmMultisampleMode msaa_mode = SCE_GXM_MULTISAMPLE_NONE;

extern GLboolean use_vram;
extern GLboolean use_vram_for_usse;

gpubuffer gpu_buffers[BUFFERS_NUM]; // VBOs array
static SceGxmColorMask blend_color_mask = SCE_GXM_COLOR_MASK_ALL; // Current in-use color mask (glColorMask)
static SceGxmBlendFunc blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD; // Current in-use RGB blend func
static SceGxmBlendFunc blend_func_a = SCE_GXM_BLEND_FUNC_ADD; // Current in-use A blend func
uint32_t vertex_array_unit = 0; // Current in-use vertex array buffer unit
static uint32_t index_array_unit = 0; // Current in-use element array buffer unit
static uint16_t *default_idx_ptr; // sceGxm mapped progressive indices buffer

vector4f texenv_color = { 0.0f, 0.0f, 0.0f, 0.0f }; // Current in use texture environment color

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
	FILE *log = fopen("ux0:/data/vitaGL.log", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
}
#endif

void upload_tex2d_uniforms(const SceGxmProgramParameter *unifs[]) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	void *fbuffer, *vbuffer;
	
	float alpha_operation = (float)alpha_op;
	float env_mode = (float)tex_unit->env_mode;
	float fogmode = (float)internal_fog_mode;
	float clipplane0 = (float)clip_plane0;
	
	// Uploading fragment shader uniforms
	sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &fbuffer);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_ALPHA_CUT_UNIF], 0, 1, &alpha_ref);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_ALPHA_MODE_UNIF], 0, 1, &alpha_operation);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_TEX_ENV_MODE_UNIF], 0, 1, &env_mode);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_FOG_MODE_UNIF], 0, 1, &fogmode);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_FOG_COLOR_UNIF], 0, 4, &fog_color.r);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_TEX_ENV_COLOR_UNIF], 0, 4, &texenv_color.r);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_FOG_NEAR_UNIF], 0, 1, (const float *)&fog_near);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_FOG_FAR_UNIF], 0, 1, (const float *)&fog_far);
	sceGxmSetUniformDataF(fbuffer, unifs[TEX2D_FOG_DENSITY_UNIF], 0, 1, (const float *)&fog_density);
	if (unifs == texture2d_generic_unifs) sceGxmSetUniformDataF(fbuffer, texture2d_tint_color, 0, 4, &current_color.r);
	
	// Uploading vertex shader uniforms
	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
	sceGxmSetUniformDataF(vbuffer, unifs[TEX2D_WVP_UNIF], 0, 16, (const float *)mvp_matrix);					
	sceGxmSetUniformDataF(vbuffer, unifs[TEX2D_CLIP_PLANE0_UNIF], 0, 1, &clipplane0);
	sceGxmSetUniformDataF(vbuffer, unifs[TEX2D_CLIP_PLANEO_EQUATION_UNIF], 0, 4, &clip_plane0_eq.x);
	sceGxmSetUniformDataF(vbuffer, unifs[TEX2D_MODELVIEW_UNIF], 0, 16, (const float *)modelview_matrix);
	sceGxmSetUniformDataF(vbuffer, unifs[TEX2D_TEXMAT_UNIF], 0, 16, (const float *)texture_matrix);
}

#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
#define SHADER_CACHE_SIZE 64

typedef union shader_mask {
	struct {
		uint32_t texenv_mode : 2;
		uint32_t alpha_test_mode : 3;
		uint32_t has_texture : 1;
		uint32_t has_colors : 1;
		uint32_t fog_mode : 2;
		uint32_t UNUSED : 23;
	};
	uint32_t raw;
} shader_mask;

typedef struct {
	SceGxmProgram *frag;
	SceGxmProgram *vert;
	SceGxmShaderPatcherId frag_id;
	SceGxmShaderPatcherId vert_id;
	shader_mask mask;
} cached_shader;
cached_shader shader_cache[SHADER_CACHE_SIZE];
uint8_t shader_cache_size = 0;
int shader_cache_idx = -1;

#define VERTEX_UNIFORMS_NUM 4
#define FRAGMENT_UNIFORMS_NUM 7

typedef enum {
	CLIP_PLANE_EQUATION_UNIF,
	MODELVIEW_MATRIX_UNIF,
	WVP_MATRIX_UNIF,
	TEX_MATRIX_UNIF
} vert_uniform_type;

typedef enum {
	ALPHA_CUT_UNIF,
	FOG_COLOR_UNIF,
	TEX_ENV_COLOR_UNIF,
	TINT_COLOR_UNIF,
	FOG_NEAR_UNIF,
	FOG_FAR_UNIF,
	FOG_DENSITY_UNIF
} frag_uniform_type;

uint8_t ffp_vertex_num_params = 1;
const SceGxmProgramParameter *ffp_vertex_params[VERTEX_UNIFORMS_NUM];
const SceGxmProgramParameter *ffp_fragment_params[FRAGMENT_UNIFORMS_NUM];
SceGxmShaderPatcherId ffp_vertex_program_id;
SceGxmShaderPatcherId ffp_fragment_program_id;
SceGxmProgram *ffp_fragment_program = NULL;
SceGxmProgram *ffp_vertex_program = NULL;
SceGxmVertexProgram *ffp_vertex_program_patched; // Patched vertex program for the fixed function pipeline implementation
SceGxmFragmentProgram *ffp_fragment_program_patched; // Patched fragment program for the fixed function pipeline implementation
GLboolean ffp_dirty_frag = GL_TRUE;
GLboolean ffp_dirty_vert = GL_TRUE;
GLboolean ffp_dirty_vert_stream = GL_TRUE;
blend_config ffp_blend_info;
shader_mask ffp_mask = {.raw = 0};

static void upload_ffp_uniforms() {
	void *fbuffer, *vbuffer;
	
	// Uploading fragment shader uniforms
	sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &fbuffer);
	if (ffp_fragment_params[ALPHA_CUT_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[ALPHA_CUT_UNIF], 0, 1, &alpha_ref);
	if (ffp_fragment_params[FOG_COLOR_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[FOG_COLOR_UNIF], 0, 4, &fog_color.r);
	if (ffp_fragment_params[TEX_ENV_COLOR_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[TEX_ENV_COLOR_UNIF], 0, 4, &texenv_color.r);
	if (ffp_fragment_params[TINT_COLOR_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[TINT_COLOR_UNIF], 0, 4, &current_color.r);
	if (ffp_fragment_params[FOG_NEAR_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[FOG_NEAR_UNIF], 0, 1, (const float *)&fog_near);
	if (ffp_fragment_params[FOG_FAR_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[FOG_FAR_UNIF], 0, 1, (const float *)&fog_far);
	if (ffp_fragment_params[FOG_DENSITY_UNIF]) sceGxmSetUniformDataF(fbuffer, ffp_fragment_params[FOG_DENSITY_UNIF], 0, 1, (const float *)&fog_density);
	
	// Uploading vertex shader uniforms
	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
	if (ffp_vertex_params[CLIP_PLANE_EQUATION_UNIF]) sceGxmSetUniformDataF(vbuffer, ffp_vertex_params[CLIP_PLANE_EQUATION_UNIF], 0, 4, &clip_plane0_eq.x);
	if (ffp_vertex_params[MODELVIEW_MATRIX_UNIF]) sceGxmSetUniformDataF(vbuffer, ffp_vertex_params[MODELVIEW_MATRIX_UNIF], 0, 16, (const float *)modelview_matrix);
	if (ffp_vertex_params[WVP_MATRIX_UNIF]) sceGxmSetUniformDataF(vbuffer, ffp_vertex_params[WVP_MATRIX_UNIF], 0, 16, (const float *)mvp_matrix);
	if (ffp_vertex_params[TEX_MATRIX_UNIF]) sceGxmSetUniformDataF(vbuffer, ffp_vertex_params[TEX_MATRIX_UNIF], 0, 16, (const float *)texture_matrix);
}

static void reload_ffp_shaders() {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	
	// Checking if mask changed
	uint8_t ffp_dirty_frag_blend = GL_FALSE;
	shader_mask mask = {.raw = 0};
	mask.texenv_mode = tex_unit->env_mode;
	mask.alpha_test_mode = alpha_op;
	mask.has_texture = tex_unit->texture_array_state;
	mask.has_colors = tex_unit->color_array_state;
	mask.fog_mode = internal_fog_mode;
	if (ffp_mask.raw == mask.raw) { // Fixed function pipeline config didn't change
		ffp_dirty_vert = GL_FALSE;
		ffp_dirty_frag = GL_FALSE;
	} else {
		int i;
		for (i = 0; i < shader_cache_size; i++) {
			if (shader_cache[i].mask.raw == mask.raw) {
				ffp_vertex_program = shader_cache[i].vert;
				ffp_fragment_program = shader_cache[i].frag;
				ffp_vertex_program_id = shader_cache[i].vert_id;
				ffp_fragment_program_id = shader_cache[i].frag_id;
				ffp_dirty_vert_stream = GL_TRUE;
				ffp_dirty_frag_blend = GL_TRUE;
				ffp_dirty_vert = GL_FALSE;
				ffp_dirty_frag = GL_FALSE;
				break;
			}
		}
		ffp_mask.raw = mask.raw;
	}
	
	uint8_t new_shader_flag = ffp_dirty_vert || ffp_dirty_frag;

	// Checking if vertex shader requires a recompilation
	if (ffp_dirty_vert) {
		
		// Compiling the new shader
		char vshader[8192];
		sprintf(vshader, ffp_vert_src, clip_plane0, tex_unit->texture_array_state, tex_unit->color_array_state);
		uint32_t size = strlen(vshader);
		SceGxmProgram *t = shark_compile_shader_extended(vshader, &size, SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		ffp_vertex_program = (SceGxmProgram*)malloc(size);
		memcpy_neon((void *)ffp_vertex_program, (void *)t, size);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_vertex_program, &ffp_vertex_program_id);
		shark_clear_output();
		
		// Checking for existing uniforms in the shader
		ffp_vertex_params[CLIP_PLANE_EQUATION_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "clip_plane0_eq");
		ffp_vertex_params[MODELVIEW_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "modelview");
		ffp_vertex_params[WVP_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "wvp");
		ffp_vertex_params[TEX_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "texmat");
		
		// Clearing dirty flags
		ffp_dirty_vert = GL_FALSE;
		ffp_dirty_vert_stream = GL_TRUE;
	}
	
	// Checking if vertex shader requires stream info update
	if (ffp_dirty_vert_stream) {

		// Setting up shader stream info
		ffp_vertex_num_params = 1;
		SceGxmVertexAttribute ffp_vertex_attribute[3];
		SceGxmVertexStream ffp_vertex_stream[3];
		
		// Vertex positions
		const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(ffp_vertex_program, "position");
		ffp_vertex_attribute[0].streamIndex = 0;
		ffp_vertex_attribute[0].offset = 0;
		ffp_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		ffp_vertex_attribute[0].componentCount = 3;
		ffp_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(param);
		ffp_vertex_stream[0].stride = sizeof(vector3f);
		ffp_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		
		// Vertex texture coordinates
		param = sceGxmProgramFindParameterByName(ffp_vertex_program, "texcoord");
		if (param) {
			ffp_vertex_attribute[1].streamIndex = 1;
			ffp_vertex_attribute[1].offset = 0;
			ffp_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
			ffp_vertex_attribute[1].componentCount = 2;
			ffp_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_stream[1].stride = sizeof(vector2f);
			ffp_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
			ffp_vertex_num_params++;
		}
		
		// Vertex colors
		param = sceGxmProgramFindParameterByName(ffp_vertex_program, "color");
		if (param) {
			ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
			ffp_vertex_attribute[ffp_vertex_num_params].offset = 0;
			ffp_vertex_attribute[ffp_vertex_num_params].format = tex_unit->color_object_type == GL_FLOAT ? SCE_GXM_ATTRIBUTE_FORMAT_F32 : SCE_GXM_ATTRIBUTE_FORMAT_U8N;
			int componentCount = tex_unit->color_array.num > 0 ? tex_unit->color_array.num : 4; // TODO: This is ugly and probably wrong
			ffp_vertex_attribute[ffp_vertex_num_params].componentCount = componentCount;
			ffp_vertex_attribute[ffp_vertex_num_params].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_stream[ffp_vertex_num_params].stride = componentCount * (tex_unit->color_object_type == GL_FLOAT ? sizeof(float) : sizeof(uint8_t));
			ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
			ffp_vertex_num_params++;
		}
		
		// Creating patched vertex shader
		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			ffp_vertex_program_id, ffp_vertex_attribute,
			ffp_vertex_num_params, ffp_vertex_stream, ffp_vertex_num_params, &ffp_vertex_program_patched);
		
		// Clearing dirty flags
		ffp_dirty_vert_stream = GL_FALSE;
	}
	
	// Checking if fragment shader requires a recompilation
	if (ffp_dirty_frag) {
		
		// Compiling the new shader
		char fshader[8192];
		sprintf(fshader, ffp_frag_src, alpha_op, tex_unit->texture_array_state, tex_unit->color_array_state, internal_fog_mode, tex_unit->env_mode);
		uint32_t size = strlen(fshader);
		SceGxmProgram *t = shark_compile_shader_extended(fshader, &size, SHARK_FRAGMENT_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		ffp_fragment_program = (SceGxmProgram*)malloc(size);
		memcpy_neon((void *)ffp_fragment_program, (void *)t, size);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_fragment_program, &ffp_fragment_program_id);
		shark_clear_output();
		
		// Checking for existing uniforms in the shader
		ffp_fragment_params[ALPHA_CUT_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "alphaCut");
		ffp_fragment_params[FOG_COLOR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fogColor");
		ffp_fragment_params[TEX_ENV_COLOR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "texEnvColor");
		ffp_fragment_params[TINT_COLOR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "tintColor");
		ffp_fragment_params[FOG_NEAR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fog_near");
		ffp_fragment_params[FOG_FAR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fog_far");
		ffp_fragment_params[FOG_DENSITY_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fog_density");

		// Clearing dirty flags
		ffp_dirty_frag = GL_FALSE;
		ffp_dirty_frag_blend = GL_TRUE;
	}
	
	// Checking if fragment shader requires a blend settings change
	if (ffp_dirty_frag_blend || (ffp_blend_info.raw != blend_info.raw)) {
		rebuild_frag_shader(ffp_fragment_program_id, &ffp_fragment_program_patched, NULL);
			
		// Updating current fixed function pipeline blend config
		ffp_blend_info.raw = blend_info.raw;
	}
	
	if (new_shader_flag) {
		shader_cache_idx = (shader_cache_idx + 1) % SHADER_CACHE_SIZE;
		if (shader_cache_size < SHADER_CACHE_SIZE)
			shader_cache_size++;
		else {
			sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, ffp_vertex_program_id);
			sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, ffp_fragment_program_id);
			free(shader_cache[shader_cache_idx].frag);
			free(shader_cache[shader_cache_idx].vert);
		}
		shader_cache[shader_cache_idx].mask.raw = mask.raw;
		shader_cache[shader_cache_idx].frag = ffp_fragment_program;
		shader_cache[shader_cache_idx].vert = ffp_vertex_program;
		shader_cache[shader_cache_idx].frag_id = ffp_fragment_program_id;
		shader_cache[shader_cache_idx].vert_id = ffp_vertex_program_id;
	}
	
	sceGxmSetVertexProgram(gxm_context, ffp_vertex_program_patched);
	sceGxmSetFragmentProgram(gxm_context, ffp_fragment_program_patched);
}
#endif

void rebuild_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog, const SceGxmProgram *vert) {
	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		pid, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		msaa_mode,
		&blend_info.info,
		vert, prog);
}

void update_precompiled_ffp_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog, blend_config *cfg) {
	// Check if a blend config change happened
	if (cfg[0].raw != blend_info.raw) {
		rebuild_frag_shader(pid, prog, NULL);
		cfg[0].raw = blend_info.raw;
	}
	
	sceGxmSetFragmentProgram(gxm_context, *prog);
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
	out[0].x = (float)(2 * x) / DISPLAY_WIDTH_FLOAT - 1.0f;
	out[0].y = (float)(2 * (x + width)) / DISPLAY_WIDTH_FLOAT - 1.0f;
	out[0].z = 1.0f - (float)(2 * y) / DISPLAY_HEIGHT_FLOAT;
	out[0].w = 1.0f - (float)(2 * (y + height)) / DISPLAY_HEIGHT_FLOAT;
}

// vitaGL specific functions

void vglUseVram(GLboolean usage) {
	use_vram = usage;
}

void vglUseVramForUSSE(GLboolean usage) {
	use_vram_for_usse = usage;
}

void vglInitWithCustomSizes(uint32_t gpu_pool_size, int width, int height, int ram_pool_size, int cdram_pool_size, int phycont_pool_size, SceGxmMultisampleMode msaa) {
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

	// Disable color buffer shader register
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_disable_color_buffer_f,
		&disable_color_buffer_fragment_id);

	const SceGxmProgram *disable_color_buffer_fragment_program = sceGxmShaderPatcherGetProgramFromId(disable_color_buffer_fragment_id);

	clear_depth = sceGxmProgramFindParameterByName(
		disable_color_buffer_fragment_program, "depth_clear");

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
		msaa,
		&disable_color_buffer_blend_info, NULL,
		&disable_color_buffer_fragment_program_patched);

	vglMemType type = VGL_MEM_RAM;
	clear_vertices = gpu_alloc_mapped(1 * sizeof(vector4f), &type);
	depth_clear_indices = gpu_alloc_mapped(4 * sizeof(unsigned short), &type);

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
	clear_color = sceGxmProgramFindParameterByName(clear_fragment_program, "u_clear_color");

	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
		clear_vertex_id, NULL, 0, NULL, 0, &clear_vertex_program_patched);

	sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
		clear_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		msaa, NULL, NULL,
		&clear_fragment_program_patched);
	
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
	if (!is_shark_online) {
#endif
	
		// Color shader register
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_rgba_v,
			&rgba_vertex_id);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_rgba_f,
			&rgba_fragment_id);

		const SceGxmProgram *rgba_vertex_program = sceGxmShaderPatcherGetProgramFromId(rgba_vertex_id);
		rgba_fragment_program = sceGxmShaderPatcherGetProgramFromId(rgba_fragment_id);

		const SceGxmProgramParameter *rgba_position = sceGxmProgramFindParameterByName(rgba_vertex_program, "aPosition");
		const SceGxmProgramParameter *rgba_color = sceGxmProgramFindParameterByName(rgba_vertex_program, "aColor");

		SceGxmVertexAttribute rgba_vertex_attribute[2];
		SceGxmVertexStream rgba_vertex_stream[2];
		rgba_vertex_attribute[0].streamIndex = 0;
		rgba_vertex_attribute[0].offset = 0;
		rgba_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		rgba_vertex_attribute[0].componentCount = 3;
		rgba_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(rgba_position);
		rgba_vertex_attribute[1].streamIndex = 1;
		rgba_vertex_attribute[1].offset = 0;
		rgba_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		rgba_vertex_attribute[1].componentCount = 4;
		rgba_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(rgba_color);
		rgba_vertex_stream[0].stride = sizeof(vector3f);
		rgba_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		rgba_vertex_stream[1].stride = sizeof(vector4f);
		rgba_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			rgba_vertex_id, rgba_vertex_attribute,
			2, rgba_vertex_stream, 2, &rgba_vertex_program_patched);

		rgba_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		rgba_vertex_stream[1].stride = sizeof(uint8_t) * 4;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			rgba_vertex_id, rgba_vertex_attribute,
			2, rgba_vertex_stream, 2, &rgba_u8n_vertex_program_patched);

		SceGxmVertexAttribute rgb_vertex_attribute[2];
		SceGxmVertexStream rgb_vertex_stream[2];
		rgb_vertex_attribute[0].streamIndex = 0;
		rgb_vertex_attribute[0].offset = 0;
		rgb_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		rgb_vertex_attribute[0].componentCount = 3;
		rgb_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(rgba_position);
		rgb_vertex_attribute[1].streamIndex = 1;
		rgb_vertex_attribute[1].offset = 0;
		rgb_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		rgb_vertex_attribute[1].componentCount = 3;
		rgb_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(rgba_color);
		rgb_vertex_stream[0].stride = sizeof(vector3f);
		rgb_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		rgb_vertex_stream[1].stride = sizeof(vector3f);
		rgb_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			rgba_vertex_id, rgb_vertex_attribute,
			2, rgb_vertex_stream, 2, &rgb_vertex_program_patched);

		rgb_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		rgb_vertex_stream[1].stride = sizeof(uint8_t) * 3;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			rgba_vertex_id, rgb_vertex_attribute,
			2, rgb_vertex_stream, 2, &rgb_u8n_vertex_program_patched);

		sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
			rgba_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			msaa, NULL, NULL,
			&rgba_fragment_program_patched);

		rgba_wvp = sceGxmProgramFindParameterByName(rgba_vertex_program, "wvp");

		// Texture2D shader register
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_v,
			&texture2d_vertex_id);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_f,
			&texture2d_fragment_id);

		const SceGxmProgram *texture2d_vertex_program = sceGxmShaderPatcherGetProgramFromId(texture2d_vertex_id);
		texture2d_fragment_program = sceGxmShaderPatcherGetProgramFromId(texture2d_fragment_id);

		const SceGxmProgramParameter *texture2d_position = sceGxmProgramFindParameterByName(texture2d_vertex_program, "position");
		const SceGxmProgramParameter *texture2d_texcoord = sceGxmProgramFindParameterByName(texture2d_vertex_program, "texcoord");

		texture2d_generic_unifs[TEX2D_ALPHA_CUT_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "alphaCut");
		texture2d_generic_unifs[TEX2D_ALPHA_MODE_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "alphaOp");
		texture2d_tint_color = sceGxmProgramFindParameterByName(texture2d_fragment_program, "tintColor");
		texture2d_generic_unifs[TEX2D_TEX_ENV_MODE_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "texEnv");
		texture2d_generic_unifs[TEX2D_FOG_MODE_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "fog_mode");
		texture2d_generic_unifs[TEX2D_FOG_COLOR_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "fogColor");
		texture2d_generic_unifs[TEX2D_CLIP_PLANE0_UNIF] = sceGxmProgramFindParameterByName(texture2d_vertex_program, "clip_plane0");
		texture2d_generic_unifs[TEX2D_CLIP_PLANEO_EQUATION_UNIF] = sceGxmProgramFindParameterByName(texture2d_vertex_program, "clip_plane0_eq");
		texture2d_generic_unifs[TEX2D_MODELVIEW_UNIF] = sceGxmProgramFindParameterByName(texture2d_vertex_program, "modelview");
		texture2d_generic_unifs[TEX2D_FOG_NEAR_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "fog_near");
		texture2d_generic_unifs[TEX2D_FOG_FAR_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "fog_far");
		texture2d_generic_unifs[TEX2D_FOG_DENSITY_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "fog_density");
		texture2d_generic_unifs[TEX2D_TEX_ENV_COLOR_UNIF] = sceGxmProgramFindParameterByName(texture2d_fragment_program, "texEnvColor");	
		texture2d_generic_unifs[TEX2D_WVP_UNIF] = sceGxmProgramFindParameterByName(texture2d_vertex_program, "wvp");
		texture2d_generic_unifs[TEX2D_TEXMAT_UNIF] = sceGxmProgramFindParameterByName(texture2d_vertex_program, "texmat");

		SceGxmVertexAttribute texture2d_vertex_attribute[2];
		SceGxmVertexStream texture2d_vertex_stream[2];
		texture2d_vertex_attribute[0].streamIndex = 0;
		texture2d_vertex_attribute[0].offset = 0;
		texture2d_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		texture2d_vertex_attribute[0].componentCount = 3;
		texture2d_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(texture2d_position);
		texture2d_vertex_attribute[1].streamIndex = 1;
		texture2d_vertex_attribute[1].offset = 0;
		texture2d_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		texture2d_vertex_attribute[1].componentCount = 2;
		texture2d_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(texture2d_texcoord);
		texture2d_vertex_stream[0].stride = sizeof(vector3f);
		texture2d_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		texture2d_vertex_stream[1].stride = sizeof(vector2f);
		texture2d_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			texture2d_vertex_id, texture2d_vertex_attribute,
			2, texture2d_vertex_stream, 2, &texture2d_vertex_program_patched);

		sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
			texture2d_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			msaa, NULL, NULL,
			&texture2d_fragment_program_patched);

		// Texture2D+RGBA shader register
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_rgba_v,
			&texture2d_rgba_vertex_id);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, gxm_program_texture2d_rgba_f,
			&texture2d_rgba_fragment_id);

		const SceGxmProgram *texture2d_rgba_vertex_program = sceGxmShaderPatcherGetProgramFromId(texture2d_rgba_vertex_id);
		texture2d_rgba_fragment_program = sceGxmShaderPatcherGetProgramFromId(texture2d_rgba_fragment_id);

		const SceGxmProgramParameter *texture2d_rgba_position = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "position");
		const SceGxmProgramParameter *texture2d_rgba_texcoord = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "texcoord");
		const SceGxmProgramParameter *texture2d_rgba_color = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "color");

		texture2d_rgba_generic_unifs[TEX2D_ALPHA_CUT_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "alphaCut");
		texture2d_rgba_generic_unifs[TEX2D_ALPHA_MODE_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "alphaOp");
		texture2d_rgba_generic_unifs[TEX2D_TEX_ENV_MODE_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "texEnv");
		texture2d_rgba_generic_unifs[TEX2D_FOG_MODE_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "fog_mode");
		texture2d_rgba_generic_unifs[TEX2D_FOG_COLOR_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "fogColor");
		texture2d_rgba_generic_unifs[TEX2D_CLIP_PLANE0_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "clip_plane0");
		texture2d_rgba_generic_unifs[TEX2D_CLIP_PLANEO_EQUATION_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "clip_plane0_eq");
		texture2d_rgba_generic_unifs[TEX2D_MODELVIEW_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "modelview");
		texture2d_rgba_generic_unifs[TEX2D_FOG_NEAR_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "fog_near");
		texture2d_rgba_generic_unifs[TEX2D_FOG_FAR_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "fog_far");
		texture2d_rgba_generic_unifs[TEX2D_FOG_DENSITY_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "fog_density");
		texture2d_rgba_generic_unifs[TEX2D_TEX_ENV_COLOR_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_fragment_program, "texEnvColor");	
		texture2d_rgba_generic_unifs[TEX2D_WVP_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "wvp");
		texture2d_rgba_generic_unifs[TEX2D_TEXMAT_UNIF] = sceGxmProgramFindParameterByName(texture2d_rgba_vertex_program, "texmat");

		SceGxmVertexAttribute texture2d_rgba_vertex_attribute[3];
		SceGxmVertexStream texture2d_rgba_vertex_stream[3];
		texture2d_rgba_vertex_attribute[0].streamIndex = 0;
		texture2d_rgba_vertex_attribute[0].offset = 0;
		texture2d_rgba_vertex_attribute[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		texture2d_rgba_vertex_attribute[0].componentCount = 3;
		texture2d_rgba_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(texture2d_rgba_position);
		texture2d_rgba_vertex_attribute[1].streamIndex = 1;
		texture2d_rgba_vertex_attribute[1].offset = 0;
		texture2d_rgba_vertex_attribute[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		texture2d_rgba_vertex_attribute[1].componentCount = 2;
		texture2d_rgba_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(texture2d_rgba_texcoord);
		texture2d_rgba_vertex_attribute[2].streamIndex = 2;
		texture2d_rgba_vertex_attribute[2].offset = 0;
		texture2d_rgba_vertex_attribute[2].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		texture2d_rgba_vertex_attribute[2].componentCount = 4;
		texture2d_rgba_vertex_attribute[2].regIndex = sceGxmProgramParameterGetResourceIndex(texture2d_rgba_color);
		texture2d_rgba_vertex_stream[0].stride = sizeof(vector3f);
		texture2d_rgba_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		texture2d_rgba_vertex_stream[1].stride = sizeof(vector2f);
		texture2d_rgba_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
		texture2d_rgba_vertex_stream[2].stride = sizeof(vector4f);
		texture2d_rgba_vertex_stream[2].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			texture2d_rgba_vertex_id, texture2d_rgba_vertex_attribute,
			3, texture2d_rgba_vertex_stream, 3, &texture2d_rgba_vertex_program_patched);

		texture2d_rgba_vertex_attribute[2].format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		texture2d_rgba_vertex_stream[2].stride = sizeof(uint8_t) * 4;

		sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher,
			texture2d_rgba_vertex_id, texture2d_rgba_vertex_attribute,
			3, texture2d_rgba_vertex_stream, 3, &texture2d_rgba_u8n_vertex_program_patched);

		sceGxmShaderPatcherCreateFragmentProgram(gxm_shader_patcher,
			texture2d_rgba_fragment_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			msaa, NULL, NULL,
			&texture2d_rgba_fragment_program_patched);
		
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
	}
#endif

	sceGxmSetTwoSidedEnable(gxm_context, SCE_GXM_TWO_SIDED_ENABLED);

	// Scissor Test shader register
	sceGxmShaderPatcherCreateMaskUpdateFragmentProgram(gxm_shader_patcher, &scissor_test_fragment_program);

	scissor_test_vertices = gpu_alloc_mapped(1 * sizeof(vector4f), &type);

	// Allocate temp pool for non-VBO drawing
	gpu_pool_init(gpu_pool_size);

	// Init texture units
	int i, j;
	for (i = 0; i < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS; i++) {
		texture_units[i].env_mode = MODULATE;
		texture_units[i].tex_id = 0;
		texture_units[i].enabled = GL_FALSE;
	}

	// Init texture slots
	for (j = 0; j < TEXTURES_NUM; j++) {
		texture_slots[j].used = 0;
		texture_slots[j].valid = 0;
	}

	// Init custom shaders
	resetCustomShaders();
	
	// Init constant index buffer
	default_idx_ptr = (uint16_t*)malloc(MAX_IDX_NUMBER * sizeof(uint16_t));
	for (i = 0; i < MAX_IDX_NUMBER; i++) {
		default_idx_ptr[i] = i;
	}

	// Init buffers
	gpu_buffers[0].used = GL_TRUE;
	for (i = 1; i < BUFFERS_NUM; i++) {
		gpu_buffers[i].used = GL_FALSE;
		gpu_buffers[i].ptr = NULL;
	}
	
	// Init purge lists
	for (i = 0; i < DISPLAY_MAX_BUFFER_COUNT; i++) {
		frame_purge_list[i][0] = NULL;
	}

	// Init scissor test state
	resetScissorTestRegion();

	// Getting newlib heap memblock starting address
	void *addr = NULL;
	sceKernelGetMemBlockBase(_newlib_heap_memblock, &addr);

	// Mapping newlib heap into sceGxm
	sceGxmMapMemory(addr, _newlib_heap_size, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);

	// Allocating default texture object
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Set texture matrix to identity
	matrix4x4_identity(texture_matrix);
}

void vglInitExtended(uint32_t gpu_pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa) {
	// Initializing sceGxm
	initGxm();

	// Getting max allocatable CDRAM and RAM memory
	if (system_app_mode) {
		SceAppMgrBudgetInfo info;
		info.size = sizeof(SceAppMgrBudgetInfo);
		sceAppMgrGetBudgetInfo(&info);
		vglInitWithCustomSizes(gpu_pool_size, width, height, info.free_user_rw > ram_threshold ? info.free_user_rw - ram_threshold : info.free_user_rw, 0, 0, msaa);
	} else {
		SceKernelFreeMemorySizeInfo info;
		info.size = sizeof(SceKernelFreeMemorySizeInfo);
		sceKernelGetFreeMemorySize(&info);
		vglInitWithCustomSizes(gpu_pool_size, width, height, info.size_user > ram_threshold ? info.size_user - ram_threshold : info.size_user, info.size_cdram - 256 * 1024, info.size_phycont - 1 * 1024 * 1024, msaa);
	}
}

void vglInit(uint32_t gpu_pool_size) {
	vglInitExtended(gpu_pool_size, DISPLAY_WIDTH_DEF, DISPLAY_HEIGHT_DEF, 0x1000000, SCE_GXM_MULTISAMPLE_NONE);
}

void vglEnd(void) {
	// Wait for rendering to be finished
	waitRenderingDone();

	// Deallocating default vertices buffers
	vgl_mem_free(clear_vertices);
	vgl_mem_free(depth_vertices);
	vgl_mem_free(depth_clear_indices);
	vgl_mem_free(scissor_test_vertices);

	// Releasing shader programs from sceGxmShaderPatcher
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, scissor_test_fragment_program);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, disable_color_buffer_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, clear_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, clear_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, rgba_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, rgba_fragment_program_patched);
	sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, texture2d_vertex_program_patched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, texture2d_fragment_program_patched);

	// Unregistering shader programs from sceGxmShaderPatcher
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, clear_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgba_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, rgba_fragment_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, texture2d_vertex_id);
	sceGxmShaderPatcherUnregisterProgram(gxm_shader_patcher, texture2d_fragment_id);
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

void vglWaitVblankStart(GLboolean enable) {
	vblank = enable;
}

// EGL implementation

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
	vblank = interval;
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
	vglStopRendering();
	vglStartRendering();
}

// openGL implementation

void glGenBuffers(GLsizei n, GLuint *res) {
	int i = 0, j = 0;
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	for (i = 1; i < BUFFERS_NUM; i++) {
		if (!gpu_buffers[i].used) {
			res[j++] = (GLuint)&gpu_buffers[i];
			gpu_buffers[i].used = GL_TRUE;
		}
		if (j >= n)
			break;
	}
}

void glBindBuffer(GLenum target, GLuint buffer) {
	debugPrintf("glBindBuffer(%u, %u);\n", target, buffer);
	switch (target) {
	case GL_ARRAY_BUFFER:
		debugPrintf("glBindBuffer(GL_ARRAY_BUFFER, %u);\n", buffer);
		vertex_array_unit = buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		index_array_unit = buffer;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glDeleteBuffers(GLsizei n, const GLuint *gl_buffers) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
		return;
	}
#endif
	int i, j;
	for (j = 0; j < n; j++) {
		if (gl_buffers[j]) {
			gpubuffer *gpu_buf = (gpubuffer*)gl_buffers[j];
			if (gpu_buf->ptr != NULL) {
				markAsDirty(gpu_buf->ptr);
				gpu_buf->ptr = NULL;
			}
			gpu_buf->used = GL_FALSE;
		}
	}
}

void glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer*)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer*)index_array_unit;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
#ifndef SKIP_ERROR_HANDLING
	if (size < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	
	// Marking previous content for deletion
	if (gpu_buf->ptr)
		markAsDirty(gpu_buf->ptr);
	
	// Allocating a new buffer
	vglMemType type = use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM;
	gpu_buf->ptr = gpu_alloc_mapped(size, &type);
	gpu_buf->size = size;

	memcpy_neon(gpu_buf->ptr, data, size);
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer*)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer*)index_array_unit;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
#ifndef SKIP_ERROR_HANDLING
	if ((size < 0) || (offset < 0) || ((offset + size) > gpu_buf->size)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	
	// Marking previous content for deletion
	frame_purge_list[frame_purge_idx][frame_elem_purge_idx] = gpu_buf->ptr;
	
	// Allocating a new buffer
	vglMemType type = use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM;
	gpu_buf->ptr = gpu_alloc_mapped(size, &type);
	
	// Copying up previous data combined to modified data
	if (offset > 0)
		memcpy_neon(gpu_buf->ptr, frame_purge_list[frame_purge_idx][frame_elem_purge_idx], offset);
	memcpy_neon((uint8_t*)gpu_buf->ptr + offset, data, size);
	if (gpu_buf->size - size - offset > 0)
		memcpy_neon((uint8_t*)gpu_buf->ptr + offset + size, (uint8_t*)frame_purge_list[frame_purge_idx][frame_elem_purge_idx] + offset + size, gpu_buf->size - size - offset);
		
	frame_elem_purge_idx++;
}

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
		break;
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
		break;
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
		break;
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
		break;
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
		break;
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
		break;
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
		break;
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
		break;
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
		break;
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

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	switch (type) {
	case GL_FLOAT:
		tex_unit->vertex_array.size = sizeof(GLfloat);
		break;
	case GL_SHORT:
		tex_unit->vertex_array.size = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}

	tex_unit->vertex_array.num = size;
	tex_unit->vertex_array.stride = stride;
	tex_unit->vertex_array.pointer = pointer;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 3) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	switch (type) {
	case GL_FLOAT:
		tex_unit->color_array.size = sizeof(GLfloat);
		break;
	case GL_SHORT:
		tex_unit->color_array.size = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
	if (tex_unit->color_array.num != size) ffp_dirty_vert_stream = GL_TRUE;
#endif
	tex_unit->color_array.num = size;
	tex_unit->color_array.stride = stride;
	tex_unit->color_array.pointer = pointer;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	switch (type) {
	case GL_FLOAT:
		tex_unit->texture_array.size = sizeof(GLfloat);
		break;
	case GL_SHORT:
		tex_unit->texture_array.size = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}

	tex_unit->texture_array.num = size;
	tex_unit->texture_array.stride = stride;
	tex_unit->texture_array.pointer = pointer;
}

void _glDrawArrays_SetupVertices(vector3f **verts, vector2f **texcoords, uint8_t **clrs, uint16_t **idxs, GLint first, GLsizei count, GLboolean fake_clrs) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	uint16_t n;
	uint16_t *indices;
	vector3f *vertices;
	uint8_t *colors;
	vector2f *uv_map;
	if (vertex_array_unit > 0) {
		if (verts != NULL) {
			if (tex_unit->vertex_array.stride == 0)
				vertices = (vector3f *)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size)));
			else
				vertices = (vector3f *)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride));
		}
		if (clrs != NULL) {
			if (tex_unit->color_array.stride == 0)
				colors = (uint8_t *)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer) + (first * (tex_unit->color_array.num * tex_unit->color_array.size)));
			else
				colors = (uint8_t *)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer) + (first * tex_unit->color_array.stride));
		}
		if (uv_map != NULL) {
			if (tex_unit->texture_array.stride == 0)
				uv_map = (vector2f *)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer) + (first * (tex_unit->texture_array.num * tex_unit->texture_array.size)));
			else
				uv_map = (vector2f *)(((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer) + (first * tex_unit->texture_array.stride));
		}
		indices = (uint16_t *)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
		for (n = 0; n < count; n++) {
			indices[n] = n;
		}
	} else {
		uint8_t *ptr;
		uint8_t *ptr_tex;
		uint8_t *ptr_clr;
		uint8_t vec_set = GL_FALSE;
		uint8_t tex_set = (texcoords == NULL);
		uint8_t clr_set = (clrs == NULL);
		
		vertices = (vector3f *)gpu_pool_memalign(count * sizeof(vector3f), sizeof(vector3f));
		if (tex_unit->vertex_array.stride == 0) {
			ptr = ((uint8_t *)tex_unit->vertex_array.pointer) + (first * (tex_unit->vertex_array.num * tex_unit->vertex_array.size));
			memcpy_neon(&vertices[0], ptr, count * sizeof(vector3f));
			vec_set = GL_TRUE;
		} else
			ptr = ((uint8_t *)tex_unit->vertex_array.pointer) + (first * tex_unit->vertex_array.stride);
			
		if (!tex_set) {
			uv_map = (vector2f *)gpu_pool_memalign(count * sizeof(vector2f), sizeof(vector2f));
			if (tex_unit->texture_array.stride == 0) {
				ptr_tex = ((uint8_t *)tex_unit->texture_array.pointer) + (first * (tex_unit->texture_array.num * tex_unit->texture_array.size));
				memcpy_neon(&uv_map[0], ptr_tex, count * sizeof(vector2f));
				tex_set = GL_TRUE;
			} else
				ptr_tex = ((uint8_t *)tex_unit->texture_array.pointer) + (first * tex_unit->texture_array.stride);
		}

		if (!clr_set) {
			if (tex_unit->color_array_state || fake_clrs)
				colors = (uint8_t *)gpu_pool_memalign(count * tex_unit->color_array.num * tex_unit->color_array.size, tex_unit->color_array.num * tex_unit->color_array.size);
			if (tex_unit->color_array_state && !fake_clrs) {
				if (tex_unit->color_array.stride == 0) {
					ptr_clr = ((uint8_t *)tex_unit->color_array.pointer) + (first * ((tex_unit->color_array.num * tex_unit->color_array.size)));
					memcpy_neon(&colors[0], ptr_clr, count * tex_unit->color_array.num * tex_unit->color_array.size);
					clr_set = GL_TRUE;
				} else
					ptr_clr = ((uint8_t *)tex_unit->color_array.pointer) + (first * tex_unit->color_array.stride);
			}
		}
		
		vector4f *colors_float = (vector4f*)colors;
		indices = (uint16_t *)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
		for (n = 0; n < count; n++) {
			if (!vec_set) {
				memcpy_neon(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
				ptr += tex_unit->vertex_array.stride;
			}
			if (!tex_set) {
				memcpy_neon(&uv_map[n], ptr_tex, tex_unit->texture_array.size * tex_unit->texture_array.num);
				ptr_tex += tex_unit->texture_array.stride;
			}
			if (!clr_set) {
				if (fake_clrs) {
					memcpy_neon(&colors_float[n], &current_color.r, sizeof(vector4f));
				} else {
					memcpy_neon(&colors[n * tex_unit->color_array.num * tex_unit->color_array.size], ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
					ptr_clr += tex_unit->color_array.stride;
				}
			}
			indices[n] = n;
		}
	}
	
	*verts = vertices;
	if (texcoords) *texcoords = uv_map;
	if (clrs) *clrs = colors;
	*idxs = indices;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	
	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p);
		
	if (cur_program != 0) {
		gpubuffer *gpu_buf = (gpubuffer*)vertex_array_unit;
		if (!gpu_buf) // Drawing without a bound VBO
			_glDrawArrays_CustomShadersIMPL(first + count);
		else // Drawing with a bound VBO
			_glDraw_VBO_CustomShadersIMPL(gpu_buf->ptr);
		gpu_buf = (gpubuffer*)index_array_unit;
		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, gpu_buf ? (uint16_t*)gpu_buf->ptr + first : default_idx_ptr + first, count);
	} else if (tex_unit->vertex_array_state) {
		int texture2d_idx = tex_unit->tex_id;
		
		if (mvp_modified) {
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			mvp_modified = GL_FALSE;
		}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		if (is_shark_online) {
			uint16_t *indices;
			vector3f *vertices = NULL;
			vector2f *uv_map = NULL;
			uint8_t *colors = NULL;
			reload_ffp_shaders();
			if (tex_unit->texture_array_state) {
				if (!(texture_slots[texture2d_idx].valid))
					return;
				sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
				_glDrawArrays_SetupVertices(&vertices, &uv_map, ffp_vertex_num_params > 2 ? &colors : NULL, &indices, first, count, GL_FALSE);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				if (ffp_vertex_num_params > 2) sceGxmSetVertexStream(gxm_context, 2, colors);
			} else if (ffp_vertex_num_params > 1) {
				_glDrawArrays_SetupVertices(&vertices, NULL, &colors, &indices, first, count, GL_FALSE);
				sceGxmSetVertexStream(gxm_context, 1, colors);
			} else {
				_glDrawArrays_SetupVertices(&vertices, NULL, NULL, &indices, first, count, GL_FALSE);
			}
			sceGxmSetVertexStream(gxm_context, 0, vertices);
			upload_ffp_uniforms();
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
		} else {
#endif
			if (tex_unit->texture_array_state) {
				if (!(texture_slots[texture2d_idx].valid))
					return;
				if (tex_unit->color_array_state) {
					sceGxmSetVertexProgram(gxm_context, texture2d_rgba_vertex_program_patched);
					update_precompiled_ffp_frag_shader(texture2d_rgba_fragment_id, &texture2d_rgba_fragment_program_patched, &texture2d_rgba_blend_cfg);
					upload_tex2d_uniforms(texture2d_rgba_generic_unifs);
				} else {
					sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
					update_precompiled_ffp_frag_shader(texture2d_fragment_id, &texture2d_fragment_program_patched, &texture2d_blend_cfg);
					upload_tex2d_uniforms(texture2d_generic_unifs);
				}
				sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
				uint16_t *indices;
				vector3f *vertices = NULL;
				vector2f *uv_map = NULL;
				uint8_t *colors = NULL;
				_glDrawArrays_SetupVertices(&vertices, &uv_map, &colors, &indices, first, count, GL_FALSE);
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				if (tex_unit->color_array_state)
					sceGxmSetVertexStream(gxm_context, 2, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			} else {
				if (tex_unit->color_array_state && (tex_unit->color_array.num == 3))
					sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
				else
					sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
				update_precompiled_ffp_frag_shader(rgba_fragment_id, &rgba_fragment_program_patched, &rgba_blend_cfg);
				vector3f *vertices = NULL;
				uint8_t *colors = NULL;
				uint16_t *indices;
				void *vbuffer;
				sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
				sceGxmSetUniformDataF(vbuffer, rgba_wvp, 0, 16, (const float *)mvp_matrix);
				_glDrawArrays_SetupVertices(&vertices, NULL, &colors, &indices, first, count, tex_unit->color_array_state ? GL_FALSE : GL_TRUE);
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		}
#endif
	}
}

uint64_t _glDrawElements_CountVertices(GLsizei count, uint16_t *ptr_idx) {
	int j = 0;
	uint64_t vertex_count_int = 0;
	while (j < count) {
		if (ptr_idx[j] >= vertex_count_int)
			vertex_count_int = ptr_idx[j] + 1;
		j++;
	}
	return vertex_count_int;
}

void _glDrawElements_SetupVertices(int dim, vector3f **verts, vector2f **texcoords, uint8_t **clrs, GLsizei count, const GLvoid *gl_indices) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	switch (dim) {
		case 1: // Position
			{
				vector3f *vertices;
				if (vertex_array_unit > 0)
					vertices = (vector3f *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
				else {
					uint64_t vertex_count_int =  _glDrawElements_CountVertices(count, (uint16_t*)gl_indices);
					vertices = (vector3f *)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
					if ((!vertex_array_unit) && tex_unit->vertex_array.stride == 0)
						memcpy_neon(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
					if ((!vertex_array_unit) && tex_unit->vertex_array.stride != 0)
						memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
					uint8_t *ptr = ((uint8_t *)tex_unit->vertex_array.pointer);
					int n = 0;
					for (n = 0; n < vertex_count_int; n++) {
						if ((!vertex_array_unit) && tex_unit->vertex_array.stride != 0)
							memcpy_neon(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
						if (!vertex_array_unit)
							ptr += tex_unit->vertex_array.stride;
					}
				}
				*verts = vertices;
			}
			break;
		case 2: // Position + Colors
			{
				vector3f *vertices;
				uint8_t *colors;
				if (vertex_array_unit > 0) {
					colors = (uint8_t *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer);
					vertices = (vector3f *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
				} else {
					uint64_t vertex_count_int =  _glDrawElements_CountVertices(count, (uint16_t*)gl_indices);
					vertices = (vector3f *)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
					colors = (uint8_t *)gpu_pool_memalign(vertex_count_int * tex_unit->color_array.num * tex_unit->color_array.size, tex_unit->color_array.num * tex_unit->color_array.size);
					if (tex_unit->vertex_array.stride == 0)
						memcpy_neon(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
					if (tex_unit->color_array.stride == 0)
						memcpy_neon(colors, tex_unit->color_array.pointer, vertex_count_int * (tex_unit->color_array.size * tex_unit->color_array.num));
					if ((tex_unit->vertex_array.stride != 0) || (tex_unit->color_array.stride != 0)) {
						if (tex_unit->vertex_array.stride != 0)
							memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
						uint8_t *ptr = ((uint8_t *)tex_unit->vertex_array.pointer);
						uint8_t *ptr_clr = ((uint8_t *)tex_unit->color_array.pointer);
						int n;
						for (n = 0; n < vertex_count_int; n++) {
							if (tex_unit->vertex_array.stride != 0)
								memcpy_neon(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							if (tex_unit->color_array.stride != 0)
								memcpy_neon(&colors[n * tex_unit->color_array.num * tex_unit->color_array.size], ptr_clr, tex_unit->color_array.size * tex_unit->color_array.num);
							ptr += tex_unit->vertex_array.stride;
							ptr_clr += tex_unit->color_array.stride;
						}
					}
				}
				*verts = vertices;
				*clrs = colors;
			}
			break;
		case 3: // Position + Texcoords + Colors
			{
				vector3f *vertices;
				uint8_t *colors;
				vector2f *uv_map;
				if (vertex_array_unit > 0) {
					vertices = (vector3f *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
					uv_map = (vector2f *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->texture_array.pointer);
					if (tex_unit->color_array_state)
						colors = (uint8_t *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->color_array.pointer);
				} else {
					uint64_t vertex_count_int =  _glDrawElements_CountVertices(count, (uint16_t*)gl_indices);
					vertices = (vector3f *)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
					uv_map = (vector2f *)gpu_pool_memalign(vertex_count_int * sizeof(vector2f), sizeof(vector2f));
					colors = (uint8_t *)gpu_pool_memalign(vertex_count_int * sizeof(vector4f), sizeof(vector4f));
					if (tex_unit->vertex_array.stride == 0)
						memcpy_neon(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
					if (tex_unit->texture_array.stride == 0)
						memcpy_neon(uv_map, tex_unit->texture_array.pointer, vertex_count_int * (tex_unit->texture_array.size * tex_unit->texture_array.num));
					if (tex_unit->color_array_state && (tex_unit->color_array.stride == 0))
						memcpy_neon(colors, tex_unit->color_array.pointer, vertex_count_int * (tex_unit->color_array.size * tex_unit->color_array.num));
					if ((tex_unit->vertex_array.stride != 0) || (tex_unit->texture_array.stride != 0)) {
						if (tex_unit->vertex_array.stride != 0)
							memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
						uint8_t *ptr = ((uint8_t *)tex_unit->vertex_array.pointer);
						uint8_t *ptr_tex = ((uint8_t *)tex_unit->texture_array.pointer);
						int n = 0;
						for (n = 0; n < vertex_count_int; n++) {
							if (tex_unit->vertex_array.stride != 0)
								memcpy_neon(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
							if (tex_unit->texture_array.stride != 0)
								memcpy_neon(&uv_map[n], ptr_tex, tex_unit->texture_array.size * tex_unit->texture_array.num);
							ptr += tex_unit->vertex_array.stride;
							ptr_tex += tex_unit->texture_array.stride;
						}
					}
				}
				*verts = vertices;
				*clrs = colors;
				*texcoords = uv_map;
			}
			break;
		case 4: // Position + Fake Colors
			{
				vector3f *vertices;
				uint8_t *colors;
				uint64_t vertex_count_int =  _glDrawElements_CountVertices(count, (uint16_t*)gl_indices);
				if (vertex_array_unit > 0)
					vertices = (vector3f *)((uint32_t)gpu_buffers[vertex_array_unit].ptr + (uint32_t)tex_unit->vertex_array.pointer);
				else
					vertices = (vector3f *)gpu_pool_memalign(vertex_count_int * sizeof(vector3f), sizeof(vector3f));
				colors = (uint8_t *)gpu_pool_memalign(vertex_count_int * tex_unit->color_array.num * tex_unit->color_array.size, tex_unit->color_array.num * tex_unit->color_array.size);
				if ((!vertex_array_unit) && tex_unit->vertex_array.stride == 0)
					memcpy_neon(vertices, tex_unit->vertex_array.pointer, vertex_count_int * (tex_unit->vertex_array.size * tex_unit->vertex_array.num));
				if ((!vertex_array_unit) && tex_unit->vertex_array.stride != 0)
					memset(vertices, 0, (vertex_count_int * sizeof(texture2d_vertex)));
				uint8_t *ptr = ((uint8_t *)tex_unit->vertex_array.pointer);
				int n = 0;
				vector4f *colors_float = (vector4f*)colors;
				for (n = 0; n < vertex_count_int; n++) {
					if ((!vertex_array_unit) && tex_unit->vertex_array.stride != 0)
						memcpy_neon(&vertices[n], ptr, tex_unit->vertex_array.size * tex_unit->vertex_array.num);
					memcpy_neon(&colors_float[n], &current_color.r, sizeof(vector4f));
					if (!vertex_array_unit)
						ptr += tex_unit->vertex_array.stride;
				}
				*verts = vertices;
				*clrs = colors;
			}
			break;
	}
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *gl_indices) {
	return;
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	
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
	gl_primitive_to_gxm(mode, gxm_p);
		
	if (cur_program != 0) {
		gpubuffer *gpu_buf = (gpubuffer*)vertex_array_unit;
		if (!gpu_buf) // Drawing without a bound VBO
			_glDrawElements_CustomShadersIMPL(index_array_unit ? (uint16_t*)gpu_buf->ptr + (uint32_t)gl_indices : (uint16_t*)gl_indices, count);
		else // Drawing with a bound VBO
			_glDraw_VBO_CustomShadersIMPL(gpu_buf->ptr);
		gpu_buf = (gpubuffer*)index_array_unit;
		if (!gpu_buf) { // Drawing without an index buffer
			// Allocating a temp buffer for the indices
			vglMemType type = use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM;
			void *ptr = gpu_alloc_mapped(count * sizeof(uint16_t), &type);
			memcpy_neon(ptr, gl_indices, count * sizeof(uint16_t));
			markAsDirty(ptr);
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
		} else { // Dtawing with an index buffer
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, (uint16_t*)gpu_buf->ptr + (uint32_t)gl_indices, count);
		}
	} else if (tex_unit->vertex_array_state) {
		int texture2d_idx = tex_unit->tex_id;
		uint16_t *indices;
		if (index_array_unit >= 0)
			indices = (uint16_t *)((uint32_t)gpu_buffers[index_array_unit].ptr + (uint32_t)gl_indices);
		else {
			indices = (uint16_t *)gpu_pool_memalign(count * sizeof(uint16_t), sizeof(uint16_t));
			memcpy_neon(indices, gl_indices, sizeof(uint16_t) * count);
		}
		if (mvp_modified) {
			matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
			mvp_modified = GL_FALSE;
		}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		if (is_shark_online) {
			vector3f *vertices = NULL;
			vector2f *uv_map = NULL;
			uint8_t *colors = NULL;
			reload_ffp_shaders();
			if (tex_unit->texture_array_state) {
				if (!(texture_slots[texture2d_idx].valid))
					return;
				sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
				_glDrawElements_SetupVertices(3, &vertices, &uv_map, &colors, count, gl_indices);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				if (ffp_vertex_num_params > 2) sceGxmSetVertexStream(gxm_context, 2, colors);
			} else if (ffp_vertex_num_params > 1) {
				_glDrawElements_SetupVertices(2, &vertices, NULL, &colors, count, gl_indices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
			} else {
				_glDrawElements_SetupVertices(1, &vertices, NULL, NULL, count, gl_indices);
			}
			sceGxmSetVertexStream(gxm_context, 0, vertices);
			upload_ffp_uniforms();
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
		} else {
#endif
			if (tex_unit->texture_array_state) {
				if (!(texture_slots[texture2d_idx].valid))
					return;
				if (tex_unit->color_array_state) {
					sceGxmSetVertexProgram(gxm_context, texture2d_rgba_vertex_program_patched);
					update_precompiled_ffp_frag_shader(texture2d_rgba_fragment_id, &texture2d_rgba_fragment_program_patched, &texture2d_rgba_blend_cfg);
					upload_tex2d_uniforms(texture2d_rgba_generic_unifs);
				} else {
					sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
					update_precompiled_ffp_frag_shader(texture2d_fragment_id, &texture2d_fragment_program_patched, &texture2d_blend_cfg);
					upload_tex2d_uniforms(texture2d_generic_unifs);
				}
				sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
				vector3f *vertices = NULL;
				vector2f *uv_map = NULL;
				uint8_t *colors = NULL;
				_glDrawElements_SetupVertices(3, &vertices, &uv_map, &colors, count, gl_indices);
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, uv_map);
				if (tex_unit->color_array_state)
					sceGxmSetVertexStream(gxm_context, 2, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			} else {
				if (tex_unit->color_array_state && (tex_unit->color_array.num == 3))
					sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
				else
					sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
				update_precompiled_ffp_frag_shader(rgba_fragment_id, &rgba_fragment_program_patched, &rgba_blend_cfg);
				void *vbuffer;
				sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
				sceGxmSetUniformDataF(vbuffer, rgba_wvp, 0, 16, (const float *)mvp_matrix);
				vector3f *vertices = NULL;
				uint8_t *colors = NULL;
				_glDrawElements_SetupVertices(tex_unit->color_array_state ? 2 : 4, &vertices, NULL, &colors, count, gl_indices);
				sceGxmSetVertexStream(gxm_context, 0, vertices);
				sceGxmSetVertexStream(gxm_context, 1, colors);
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, indices, count);
			}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		}
#endif
	}
}

void glEnableClientState(GLenum array) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	switch (array) {
	case GL_VERTEX_ARRAY:
		tex_unit->vertex_array_state = GL_TRUE;
		break;
	case GL_COLOR_ARRAY:
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		ffp_dirty_frag = GL_TRUE;
		ffp_dirty_vert = GL_TRUE;
#endif
		tex_unit->color_array_state = GL_TRUE;
		break;
	case GL_TEXTURE_COORD_ARRAY:
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		ffp_dirty_frag = GL_TRUE;
		ffp_dirty_vert = GL_TRUE;
#endif
		tex_unit->texture_array_state = GL_TRUE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glDisableClientState(GLenum array) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	switch (array) {
	case GL_VERTEX_ARRAY:
		tex_unit->vertex_array_state = GL_FALSE;
		break;
	case GL_COLOR_ARRAY:
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		ffp_dirty_frag = GL_TRUE;
		ffp_dirty_vert = GL_TRUE;
#endif
		tex_unit->color_array_state = GL_FALSE;
		break;
	case GL_TEXTURE_COORD_ARRAY:
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
		ffp_dirty_frag = GL_TRUE;
		ffp_dirty_vert = GL_TRUE;
#endif
		tex_unit->texture_array_state = GL_FALSE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glClientActiveTexture(GLenum texture) {
#ifndef SKIP_ERROR_HANDLING
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE31)) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	} else
#endif
		client_texture_unit = texture - GL_TEXTURE0;
}

// VGL_EXT_gpu_objects_array extension implementation

void vglVertexPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type) {
	case GL_FLOAT:
		bpe = sizeof(GLfloat);
		break;
	case GL_SHORT:
		bpe = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	tex_unit->vertex_object = gpu_pool_memalign(count * bpe * size, bpe * size);
	if (stride == 0)
		memcpy_neon(tex_unit->vertex_object, pointer, count * bpe * size);
	else {
		int i;
		uint8_t *dst = (uint8_t *)tex_unit->vertex_object;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			memcpy_neon(dst, src, bpe * size);
			dst += (bpe * size);
			src += stride;
		}
	}
}

void vglColorPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 3) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type) {
	case GL_FLOAT:
		bpe = sizeof(GLfloat);
		break;
	case GL_SHORT:
		bpe = sizeof(GLshort);
		break;
	case GL_UNSIGNED_BYTE:
		bpe = sizeof(uint8_t);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	tex_unit->color_object = gpu_pool_memalign(count * bpe * size, bpe * size);
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
	if (tex_unit->color_object_type != type) ffp_dirty_vert_stream = GL_TRUE;
#endif
	tex_unit->color_object_type = type;
	if (stride == 0)
		memcpy_neon(tex_unit->color_object, pointer, count * bpe * size);
	else {
		int i;
		uint8_t *dst = (uint8_t *)tex_unit->color_object;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			memcpy_neon(dst, src, bpe * size);
			dst += (bpe * size);
			src += stride;
		}
	}
}

void vglTexCoordPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type) {
	case GL_FLOAT:
		bpe = sizeof(GLfloat);
		break;
	case GL_SHORT:
		bpe = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	tex_unit->texture_object = gpu_pool_memalign(count * bpe * size, bpe * size);
	if (stride == 0)
		memcpy_neon(tex_unit->texture_object, pointer, count * bpe * size);
	else {
		int i;
		uint8_t *dst = (uint8_t *)tex_unit->texture_object;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			memcpy_neon(dst, src, bpe * size);
			dst += (bpe * size);
			src += stride;
		}
	}
}

void vglIndexPointer(GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (stride < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	int bpe;
	switch (type) {
	case GL_FLOAT:
		bpe = sizeof(GLfloat);
		break;
	case GL_SHORT:
		bpe = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	tex_unit->index_object = gpu_pool_memalign(count * bpe, bpe);
	if (stride == 0)
		memcpy_neon(tex_unit->index_object, pointer, count * bpe);
	else {
		int i;
		uint8_t *dst = (uint8_t *)tex_unit->index_object;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			memcpy_neon(dst, src, bpe);
			dst += bpe;
			src += stride;
		}
	}
}

void vglVertexPointerMapped(const GLvoid *pointer) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	tex_unit->vertex_object = (GLvoid *)pointer;
}

void vglColorPointerMapped(GLenum type, const GLvoid *pointer) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	tex_unit->color_object = (GLvoid *)pointer;
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
	if (tex_unit->color_object_type != type) ffp_dirty_vert_stream = GL_TRUE;
#endif
	tex_unit->color_object_type = type;
}

void vglTexCoordPointerMapped(const GLvoid *pointer) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	tex_unit->texture_object = (GLvoid *)pointer;
}

void vglIndexPointerMapped(const GLvoid *pointer) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	tex_unit->index_object = (GLvoid *)pointer;
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
	gl_primitive_to_gxm(mode, gxm_p);

	if (cur_program != 0) {
		_vglDrawObjects_CustomShadersIMPL(implicit_wvp);
		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, texture_units[client_texture_unit].index_object, count);
	} else {
		texture_unit *tex_unit = &texture_units[client_texture_unit];
		int texture2d_idx = tex_unit->tex_id;
		if (tex_unit->vertex_array_state) {
			if (mvp_modified) {
				matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
				mvp_modified = GL_FALSE;
			}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
			if (is_shark_online) {
				reload_ffp_shaders();
				if (tex_unit->texture_array_state) {
					if (!(texture_slots[texture2d_idx].valid))
						return;
					sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
					sceGxmSetVertexStream(gxm_context, 1, tex_unit->texture_object);
					if (ffp_vertex_num_params > 2) sceGxmSetVertexStream(gxm_context, 2, tex_unit->color_object);
				} else if (ffp_vertex_num_params > 1) sceGxmSetVertexStream(gxm_context, 1, tex_unit->color_object);
				sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
				upload_ffp_uniforms();
				sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
			} else {
#endif
				if (tex_unit->texture_array_state) {
					if (!(texture_slots[texture2d_idx].valid))
						return;
					if (tex_unit->color_array_state) {
						if (tex_unit->color_object_type == GL_FLOAT)
							sceGxmSetVertexProgram(gxm_context, texture2d_rgba_vertex_program_patched);
						else
							sceGxmSetVertexProgram(gxm_context, texture2d_rgba_u8n_vertex_program_patched);
						update_precompiled_ffp_frag_shader(texture2d_rgba_fragment_id, &texture2d_rgba_fragment_program_patched, &texture2d_rgba_blend_cfg);
						upload_tex2d_uniforms(texture2d_rgba_generic_unifs);
						sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
						sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
						sceGxmSetVertexStream(gxm_context, 1, tex_unit->texture_object);
						sceGxmSetVertexStream(gxm_context, 2, tex_unit->color_object);
						sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
					} else {
						sceGxmSetVertexProgram(gxm_context, texture2d_vertex_program_patched);
						update_precompiled_ffp_frag_shader(texture2d_fragment_id, &texture2d_fragment_program_patched, &texture2d_blend_cfg);
						upload_tex2d_uniforms(texture2d_generic_unifs);
						sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);
						sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
						sceGxmSetVertexStream(gxm_context, 1, tex_unit->texture_object);
						sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
					}
				} else {
					if (tex_unit->color_array_state && (tex_unit->color_array.num == 3)) {
						if (tex_unit->color_object_type == GL_FLOAT)
							sceGxmSetVertexProgram(gxm_context, rgb_vertex_program_patched);
						else
							sceGxmSetVertexProgram(gxm_context, rgb_u8n_vertex_program_patched);
					} else {
						if (tex_unit->color_object_type == GL_FLOAT)
							sceGxmSetVertexProgram(gxm_context, rgba_vertex_program_patched);
						else
							sceGxmSetVertexProgram(gxm_context, rgba_u8n_vertex_program_patched);
					}
					update_precompiled_ffp_frag_shader(rgba_fragment_id, &rgba_fragment_program_patched, &rgba_blend_cfg);
					void *vbuffer;
					sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
					sceGxmSetUniformDataF(vbuffer, rgba_wvp, 0, 16, (const float *)mvp_matrix);
					sceGxmSetVertexStream(gxm_context, 0, tex_unit->vertex_object);
					if (tex_unit->color_array_state) {
						sceGxmSetVertexStream(gxm_context, 1, tex_unit->color_object);
					} else {
						vector4f *colors = (vector4f *)gpu_pool_memalign(count * sizeof(vector4f), sizeof(vector4f));
						int n;
						for (n = 0; n < count; n++) {
							memcpy_neon(&colors[n], &current_color.r, sizeof(vector4f));
						}
						sceGxmSetVertexStream(gxm_context, 1, colors);
					}
					sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, tex_unit->index_object, count);
				}
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
			}
#endif
		}
	}
}

size_t vglMemFree(vglMemType type) {
#ifndef SKIP_ERROR_HANDLING
	if (type >= VGL_MEM_TYPE_COUNT)
		return 0;
#endif
	return vgl_mem_get_free_space(type);
}

void *vglAlloc(uint32_t size, vglMemType type) {
#ifndef SKIP_ERROR_HANDLING
	if (type >= VGL_MEM_TYPE_COUNT)
		return NULL;
#endif
	return vgl_mem_alloc(size, type);
}

void *vglForceAlloc(uint32_t size) {
	vglMemType mem_type = use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM;
	return gpu_alloc_mapped(size, &mem_type);
}

void vglFree(void *addr) {
	vgl_mem_free(addr);
}

void vglUseExtraMem(GLboolean use) {
	use_extra_mem = use;
}

GLboolean vglHasRuntimeShaderCompiler(void) {
	return is_shark_online;
}