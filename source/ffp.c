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
 * ffp.c:
 * Implementation for fixed function pipeline (GL1)
 */

#include "shaders/ffp_f.h"
#include "shaders/ffp_v.h"
#include "shared.h"

#define SHADER_CACHE_SIZE 256

#define VERTEX_UNIFORMS_NUM 10
#define FRAGMENT_UNIFORMS_NUM 7

static uint32_t vertex_count = 0; // Vertex counter for vertex list
static SceGxmPrimitiveType prim; // Current in use primitive for rendering
GLboolean prim_is_quad = GL_FALSE; // Flag for when GL_QUADS primitive is used

typedef struct {
	vector2f uv;
	vector4f clr;
	vector4f amb;
	vector4f diff;
	vector4f spec;
	vector4f emiss;
	vector3f nor;
} legacy_vtx_attachment;
legacy_vtx_attachment current_vtx = {
	.uv = {0.0f, 0.0f},
	.clr = {1.0f, 1.0f, 1.0f, 1.0f},
	.amb = {0.2f, 0.2f, 0.2f, 1.0f},
	.diff = {0.8f, 0.8f, 0.8f, 1.0f},
	.spec = {0.0f, 0.0f, 0.0f, 1.0f},
	.emiss = {0.0f, 0.0f, 0.0f, 1.0f},
	.nor = {0.0f, 0.0f, 1.0f}};

SceGxmVertexAttribute ffp_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream ffp_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexAttribute legacy_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream legacy_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];
static uint32_t ffp_vertex_attrib_offsets[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0};
static uint32_t ffp_vertex_attrib_vbo[GL_MAX_VERTEX_ATTRIBS] = {0, 0, 0};
uint8_t ffp_vertex_attrib_state = 0;
static unsigned short orig_stride[GL_MAX_VERTEX_ATTRIBS];
static SceGxmAttributeFormat orig_fmt[GL_MAX_VERTEX_ATTRIBS];
static unsigned char orig_size[GL_MAX_VERTEX_ATTRIBS];

typedef union shader_mask {
	struct {
		uint32_t texenv_mode : 3;
		uint32_t alpha_test_mode : 3;
		uint32_t has_texture : 1;
		uint32_t has_colors : 1;
		uint32_t fog_mode : 2;
		uint32_t clip_planes_num : 3;
		uint32_t lights_num : 4;
		uint32_t UNUSED : 15;
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

typedef enum {
	CLIP_PLANES_EQUATION_UNIF,
	MODELVIEW_MATRIX_UNIF,
	WVP_MATRIX_UNIF,
	TEX_MATRIX_UNIF,
	LIGHTS_AMBIENTS_UNIF,
	LIGHTS_DIFFUSES_UNIF,
	LIGHTS_SPECULARS_UNIF,
	LIGHTS_POSITIONS_UNIF,
	LIGHTS_ATTENUATIONS_UNIF,
	NORMAL_MATRIX_UNIF
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
blend_config ffp_blend_info;
shader_mask ffp_mask = {.raw = 0};

SceGxmVertexAttribute ffp_vertex_attribute[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream ffp_vertex_stream[FFP_VERTEX_ATTRIBS_NUM];

void reload_vertex_uniforms() {
	ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "clip_planes_eq");
	ffp_vertex_params[MODELVIEW_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "modelview");
	ffp_vertex_params[WVP_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "wvp");
	ffp_vertex_params[TEX_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "texmat");
	ffp_vertex_params[NORMAL_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "normal_mat");
	ffp_vertex_params[LIGHTS_AMBIENTS_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "lights_ambients");
	if (ffp_vertex_params[LIGHTS_AMBIENTS_UNIF]) {
		ffp_vertex_params[LIGHTS_DIFFUSES_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "lights_diffuses");
		ffp_vertex_params[LIGHTS_SPECULARS_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "lights_speculars");
		ffp_vertex_params[LIGHTS_POSITIONS_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "lights_positions");
		ffp_vertex_params[LIGHTS_ATTENUATIONS_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "lights_attenuations");
	}
}

void reload_fragment_uniforms() {
	ffp_fragment_params[ALPHA_CUT_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "alphaCut");
	ffp_fragment_params[FOG_COLOR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fogColor");
	ffp_fragment_params[TEX_ENV_COLOR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "texEnvColor");
	ffp_fragment_params[TINT_COLOR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "tintColor");
	ffp_fragment_params[FOG_NEAR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fog_near");
	ffp_fragment_params[FOG_FAR_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fog_far");
	ffp_fragment_params[FOG_DENSITY_UNIF] = sceGxmProgramFindParameterByName(ffp_fragment_program, "fog_density");
}

void reload_ffp_shaders(SceGxmVertexAttribute *attrs, SceGxmVertexStream * streams) {
	// Checking if mask changed
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	GLboolean ffp_dirty_frag_blend = ffp_blend_info.raw != blend_info.raw;
	shader_mask mask = {.raw = 0};
	mask.texenv_mode = tex_unit->env_mode;
	mask.alpha_test_mode = alpha_op;
	mask.has_texture = (ffp_vertex_attrib_state & (1 << 1)) ? GL_TRUE : GL_FALSE;
	mask.has_colors = (ffp_vertex_attrib_state & (1 << 2)) ? GL_TRUE : GL_FALSE;
	mask.fog_mode = internal_fog_mode;
	mask.lights_num = lighting_state ? lights_num : 0;

	vector4f *clip_planes;
	vector4f temp_clip_planes[MAX_CLIP_PLANES_NUM];
	if (clip_planes_aligned) {
		clip_planes = &clip_planes_eq[clip_plane_range[0]];
		mask.clip_planes_num = clip_plane_range[1] - clip_plane_range[0];
	} else {
		clip_planes = &temp_clip_planes[0];
		for (int i = clip_plane_range[0]; i < clip_plane_range[1]; i++) {
			if (clip_planes_mask & (1 << i)) {
				sceClibMemcpy(&clip_planes[mask.clip_planes_num], &clip_planes_eq[i], sizeof(vector4f));
				mask.clip_planes_num++;
			}
		}
	}

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
				ffp_dirty_frag_blend = GL_TRUE;
				
				if (ffp_dirty_vert)
					reload_vertex_uniforms();
				
				if (ffp_dirty_frag)
					reload_fragment_uniforms();
				
				ffp_dirty_vert = GL_FALSE;
				ffp_dirty_frag = GL_FALSE;
				break;
			}
		}
		ffp_mask.raw = mask.raw;
	}

	GLboolean new_shader_flag = ffp_dirty_vert || ffp_dirty_frag;

	// Checking if vertex shader requires a recompilation
	if (ffp_dirty_vert) {
		// Compiling the new shader
		char vshader[8192];
		sprintf(vshader, ffp_vert_src, mask.clip_planes_num, mask.has_texture, mask.has_colors, mask.lights_num);
		uint32_t size = strlen(vshader);
		SceGxmProgram *t = shark_compile_shader_extended(vshader, &size, SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		ffp_vertex_program = (SceGxmProgram *)malloc(size);
		sceClibMemcpy((void *)ffp_vertex_program, (void *)t, size);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_vertex_program, &ffp_vertex_program_id);
		shark_clear_output();

		// Checking for existing uniforms in the shader
		reload_vertex_uniforms();
		
		// Clearing dirty flags
		ffp_dirty_vert = GL_FALSE;
	}

	// Not going for the vertex config setup if we have aligned datas
	if (!attrs && ffp_vertex_attrib_state != 0x05) {
		attrs = ffp_vertex_attrib_config;
		streams = ffp_vertex_stream_config;
	}

	ffp_vertex_num_params = 1;
	if (attrs) {
		// Vertex positions
		const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(ffp_vertex_program, "position");
		attrs[0].regIndex = sceGxmProgramParameterGetResourceIndex(param);

		// Vertex texture coordinates
		if (mask.has_texture) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "texcoord");
			attrs[1].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_num_params++;
		}

		// Vertex colors
		if (mask.has_colors) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "color");
			attrs[ffp_vertex_num_params].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_num_params++;
		}
		
		// Lighting data
		if (mask.lights_num > 0) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "diff");
			attrs[ffp_vertex_num_params++].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "spec");
			attrs[ffp_vertex_num_params++].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "emission");
			attrs[ffp_vertex_num_params++].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "normals");
			attrs[ffp_vertex_num_params++].regIndex = sceGxmProgramParameterGetResourceIndex(param);
		}
	} else {
		// Vertex positions
		const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(ffp_vertex_program, "position");
		sceClibMemcpy(&ffp_vertex_attribute[0], &ffp_vertex_attrib_config[0], sizeof(SceGxmVertexAttribute));
		ffp_vertex_attribute[0].streamIndex = 0;
		ffp_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(param);
		ffp_vertex_stream[0].stride = ffp_vertex_stream_config[0].stride;
		ffp_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		// Vertex texture coordinates
		if (mask.has_texture) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "texcoord");
			sceClibMemcpy(&ffp_vertex_attribute[1], &ffp_vertex_attrib_config[1], sizeof(SceGxmVertexAttribute));
			ffp_vertex_attribute[1].streamIndex = 1;
			ffp_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_stream[1].stride = ffp_vertex_stream_config[1].stride;
			ffp_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
			ffp_vertex_num_params++;
		}

		// Vertex colors
		if (mask.has_colors) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "color");
			sceClibMemcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[2], sizeof(SceGxmVertexAttribute));
			ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
			ffp_vertex_attribute[ffp_vertex_num_params].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[2].stride;
			ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
			ffp_vertex_num_params++;
		}

		streams = ffp_vertex_stream;
		attrs = ffp_vertex_attribute;
	}

	// Creating patched vertex shader
	sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher, ffp_vertex_program_id, attrs, ffp_vertex_num_params, streams, ffp_vertex_num_params, &ffp_vertex_program_patched);

	// Checking if fragment shader requires a recompilation
	if (ffp_dirty_frag) {
		// Compiling the new shader
		char fshader[8192];
		sprintf(fshader, ffp_frag_src, alpha_op, mask.has_texture, mask.has_colors, mask.fog_mode, mask.texenv_mode);
		uint32_t size = strlen(fshader);
		SceGxmProgram *t = shark_compile_shader_extended(fshader, &size, SHARK_FRAGMENT_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		ffp_fragment_program = (SceGxmProgram *)malloc(size);
		sceClibMemcpy((void *)ffp_fragment_program, (void *)t, size);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_fragment_program, &ffp_fragment_program_id);
		shark_clear_output();

		// Checking for existing uniforms in the shader
		reload_fragment_uniforms();

		// Clearing dirty flags
		ffp_dirty_frag = GL_FALSE;
		ffp_dirty_frag_blend = GL_TRUE;
	}

	// Checking if fragment shader requires a blend settings change
	if (ffp_dirty_frag_blend) {
		rebuild_frag_shader(ffp_fragment_program_id, &ffp_fragment_program_patched);

		// Updating current fixed function pipeline blend config
		ffp_blend_info.raw = blend_info.raw;
	}

	if (new_shader_flag) {
		shader_cache_idx = (shader_cache_idx + 1) % SHADER_CACHE_SIZE;
		if (shader_cache_size < SHADER_CACHE_SIZE)
			shader_cache_size++;
		else {
			sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, shader_cache[shader_cache_idx].vert_id);
			sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, shader_cache[shader_cache_idx].frag_id);
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

	// Recalculating MVP matrix if necessary
	if (mvp_modified) {
		matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
		
		// Recalculating normal matrix if necessary (TODO: This should be recalculated only when MV changes)
		if (mask.lights_num > 0) {
			matrix4x4 inverted;
			matrix4x4_invert(inverted, modelview_matrix);
			matrix4x4_transpose(normal_matrix, inverted);
		}
		
		mvp_modified = GL_FALSE;
	}

	// Uploading fragment shader uniforms
	void *buffer;
	sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &buffer);
	if (ffp_fragment_params[ALPHA_CUT_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[ALPHA_CUT_UNIF], 0, 1, &alpha_ref);
	if (ffp_fragment_params[FOG_COLOR_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_COLOR_UNIF], 0, 4, &fog_color.r);
	if (ffp_fragment_params[TEX_ENV_COLOR_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[TEX_ENV_COLOR_UNIF], 0, 4, &texenv_color.r);
	if (ffp_fragment_params[TINT_COLOR_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[TINT_COLOR_UNIF], 0, 4, &current_vtx.clr.r);
	if (ffp_fragment_params[FOG_NEAR_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_NEAR_UNIF], 0, 1, (const float *)&fog_near);
	if (ffp_fragment_params[FOG_FAR_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_FAR_UNIF], 0, 1, (const float *)&fog_far);
	if (ffp_fragment_params[FOG_DENSITY_UNIF])
		sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_DENSITY_UNIF], 0, 1, (const float *)&fog_density);

	// Uploading vertex shader uniforms
	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &buffer);
	if (ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF]) sceGxmSetUniformDataF(buffer, ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF], 0, 4 * mask.clip_planes_num, &clip_planes[0].x);
	if (ffp_vertex_params[MODELVIEW_MATRIX_UNIF]) sceGxmSetUniformDataF(buffer, ffp_vertex_params[MODELVIEW_MATRIX_UNIF], 0, 16, (const float *)modelview_matrix);
	if (ffp_vertex_params[WVP_MATRIX_UNIF]) sceGxmSetUniformDataF(buffer, ffp_vertex_params[WVP_MATRIX_UNIF], 0, 16, (const float *)mvp_matrix);
	if (ffp_vertex_params[TEX_MATRIX_UNIF]) sceGxmSetUniformDataF(buffer, ffp_vertex_params[TEX_MATRIX_UNIF], 0, 16, (const float *)texture_matrix);
	if (ffp_vertex_params[NORMAL_MATRIX_UNIF]) sceGxmSetUniformDataF(buffer, ffp_vertex_params[NORMAL_MATRIX_UNIF], 0, 16, (const float *)normal_matrix);
	if (ffp_vertex_params[LIGHTS_AMBIENTS_UNIF]) {
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_AMBIENTS_UNIF], 0, 4 * mask.lights_num, (const float *)lights_ambients);
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_DIFFUSES_UNIF], 0, 4 * mask.lights_num, (const float *)lights_diffuses);
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_SPECULARS_UNIF], 0, 4 * mask.lights_num, (const float *)lights_speculars);
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_POSITIONS_UNIF], 0, 4 * mask.lights_num, (const float *)lights_positions);
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_ATTENUATIONS_UNIF], 0, 3 * mask.lights_num, (const float *)lights_attenuations);
	}
}

void _glDrawArrays_FixedFunctionIMPL(GLsizei count) {
	reload_ffp_shaders(NULL, NULL);

	// Uploading textures on relative texture units
	if (ffp_vertex_attrib_state & (1 << 1)) {
		texture_unit *tex_unit = &texture_units[client_texture_unit];
		sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[tex_unit->tex_id].gxm_tex);
	}

	// Uploading vertex streams
	int i, j = 0;
	void *ptrs[FFP_VERTEX_ATTRIBS_NUM];
	for (i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (ffp_vertex_attrib_state & (1 << i)) {
			if (ffp_vertex_attrib_vbo[i]) {
				gpubuffer *gpu_buf = (gpubuffer *)ffp_vertex_attrib_vbo[i];
				ptrs[i] = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[i];
			} else {
				ptrs[i] = gpu_alloc_mapped_temp(count * ffp_vertex_stream_config[i].stride);
				sceClibMemcpy(ptrs[i], (void *)ffp_vertex_attrib_offsets[i], count * ffp_vertex_stream_config[i].stride);
			}
			sceGxmSetVertexStream(gxm_context, j++, ptrs[i]);
		}
	}
}

void _glDrawElements_FixedFunctionIMPL(uint16_t *idx_buf, GLsizei count) {
	reload_ffp_shaders(NULL, NULL);

	uint32_t top_idx = 0;
	int i;
	for (i = 0; i < count; i++) {
		if (idx_buf[i] > top_idx)
			top_idx = idx_buf[i];
	}
	top_idx++;

	// Uploading textures on relative texture units
	if (ffp_vertex_attrib_state & (1 << 1)) {
		texture_unit *tex_unit = &texture_units[client_texture_unit];
		sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[tex_unit->tex_id].gxm_tex);
	}

	// Uploading vertex streams
	int j = 0;
	void *ptrs[FFP_VERTEX_ATTRIBS_NUM];
	for (i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (ffp_vertex_attrib_state & (1 << i)) {
			if (ffp_vertex_attrib_vbo[i]) {
				gpubuffer *gpu_buf = (gpubuffer *)ffp_vertex_attrib_vbo[i];
				ptrs[i] = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[i];
			} else {
				ptrs[i] = gpu_alloc_mapped_temp(top_idx * ffp_vertex_stream_config[i].stride);
				sceClibMemcpy(ptrs[i], (void *)ffp_vertex_attrib_offsets[i], top_idx * ffp_vertex_stream_config[i].stride);
			}
			sceGxmSetVertexStream(gxm_context, j++, ptrs[i]);
		}
	}
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glEnableClientState(GLenum array) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	ffp_dirty_vert = GL_TRUE;
	ffp_dirty_frag = GL_TRUE;
	switch (array) {
	case GL_VERTEX_ARRAY:
		ffp_vertex_attrib_state |= (1 << 0);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ffp_vertex_attrib_state |= (1 << 1);
		break;
	case GL_COLOR_ARRAY:
		ffp_vertex_attrib_state |= (1 << 2);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glDisableClientState(GLenum array) {
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	ffp_dirty_vert = GL_TRUE;
	ffp_dirty_frag = GL_TRUE;
	switch (array) {
	case GL_VERTEX_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << 0);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << 1);
		break;
	case GL_COLOR_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << 2);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[0] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[0] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[0];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[0];

	unsigned short bpe;
	switch (type) {
	case GL_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16;
		bpe = 2;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 3) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[2] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[2] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[2];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[2];

	unsigned short bpe;
	switch (type) {
	case GL_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;
		bpe = 2;
		break;
	case GL_UNSIGNED_SHORT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U16N;
		bpe = 2;
		break;
	case GL_BYTE:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S8N;
		bpe = 1;
		break;
	case GL_UNSIGNED_BYTE:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		bpe = 1;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 1) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[1] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[1] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[1];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[1];

	unsigned short bpe;
	switch (type) {
	case GL_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16;
		bpe = 2;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	legacy_pool_ptr[0] = x;
	legacy_pool_ptr[1] = y;
	legacy_pool_ptr[2] = z;
	if (lighting_state) {
		sceClibMemcpy(legacy_pool_ptr + 3, &current_vtx.uv.x, sizeof(float) * 2);
		sceClibMemcpy(legacy_pool_ptr + 5, &current_vtx.amb.x, sizeof(float) * 19);
	} else
		sceClibMemcpy(legacy_pool_ptr + 3, &current_vtx.uv.x, sizeof(float) * 6);
	legacy_pool_ptr += LEGACY_VERTEX_STRIDE;
	
	// Increasing vertex counter
	vertex_count++;
}

void glVertex3fv(const GLfloat *v) {
	glVertex3f(v[0], v[1], v[2]);
}

void glVertex2f(GLfloat x, GLfloat y) {
	glVertex3f(x, y, 0.0f);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
	switch (pname) {
	case GL_AMBIENT:
		sceClibMemcpy(&current_vtx.amb.x, params, sizeof(float) * 4);
		break;
	case GL_DIFFUSE:
		sceClibMemcpy(&current_vtx.diff.x, params, sizeof(float) * 4);
		break;
	case GL_SPECULAR:
		sceClibMemcpy(&current_vtx.spec.x, params, sizeof(float) * 4);
		break;
	case GL_EMISSION:
		sceClibMemcpy(&current_vtx.emiss.x, params, sizeof(float) * 4);
		break;
	case GL_AMBIENT_AND_DIFFUSE:
		sceClibMemcpy(&current_vtx.amb.x, params, sizeof(float) * 4);
		sceClibMemcpy(&current_vtx.diff.x, params, sizeof(float) * 4);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	// Setting current color value
	current_vtx.clr.r = red;
	current_vtx.clr.g = green;
	current_vtx.clr.b = blue;
	current_vtx.clr.a = 1.0f;
}

void glColor3fv(const GLfloat *v) {
	// Setting current color value
	sceClibMemcpy(&current_vtx.clr.r, v, sizeof(vector3f));
	current_vtx.clr.a = 1.0f;
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
	// Setting current color value
	current_vtx.clr.r = (1.0f * red) / 255.0f;
	current_vtx.clr.g = (1.0f * green) / 255.0f;
	current_vtx.clr.b = (1.0f * blue) / 255.0f;
	current_vtx.clr.a = 1.0f;
}

void glColor3ubv(const GLubyte *c) {
	// Setting current color value
	current_vtx.clr.r = (1.0f * c[0]) / 255.0f;
	current_vtx.clr.g = (1.0f * c[1]) / 255.0f;
	current_vtx.clr.b = (1.0f * c[2]) / 255.0f;
	current_vtx.clr.a = 1.0f;
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	// Setting current color value
	current_vtx.clr.r = red;
	current_vtx.clr.g = green;
	current_vtx.clr.b = blue;
	current_vtx.clr.a = alpha;
}

void glColor4fv(const GLfloat *v) {
	// Setting current color value
	sceClibMemcpy(&current_vtx.clr.r, v, sizeof(vector4f));
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	current_vtx.clr.r = (1.0f * red) / 255.0f;
	current_vtx.clr.g = (1.0f * green) / 255.0f;
	current_vtx.clr.b = (1.0f * blue) / 255.0f;
	current_vtx.clr.a = (1.0f * alpha) / 255.0f;
}

void glColor4ubv(const GLubyte *c) {
	// Setting current color value
	current_vtx.clr.r = (1.0f * c[0]) / 255.0f;
	current_vtx.clr.g = (1.0f * c[1]) / 255.0f;
	current_vtx.clr.b = (1.0f * c[2]) / 255.0f;
	current_vtx.clr.a = (1.0f * c[3]) / 255.0f;
}

void glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	// Setting current color value
	current_vtx.clr.r = (1.0f * red) / 65536.0f;
	current_vtx.clr.g = (1.0f * green) / 65536.0f;
	current_vtx.clr.b = (1.0f * blue) / 65536.0f;
	current_vtx.clr.a = (1.0f * alpha) / 65536.0f;
}

void glNormal3f(GLfloat x, GLfloat y, GLfloat z) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	current_vtx.nor.x = x;
	current_vtx.nor.y = y;
	current_vtx.nor.z = z;
}

void glNormal3fv(const GLfloat * v) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	current_vtx.nor.x = v[0];
	current_vtx.nor.y = v[1];
	current_vtx.nor.z = v[2];
}

void glTexCoord2f(GLfloat s, GLfloat t) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	current_vtx.uv.x = s;
	current_vtx.uv.y = t;
}

void glTexCoord2fv(GLfloat *f) {
	glTexCoord2f(f[0], f[1]);
}

void glTexCoord2i(GLint s, GLint t) {
	glTexCoord2f(s, t);
}

void glBegin(GLenum mode) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	// Changing current openGL machine state
	phase = MODEL_CREATION;

	// Translating primitive to sceGxm one
	gl_primitive_to_gxm(mode, prim);

	// Resetting vertex count
	vertex_count = 0;
}

void glEnd(void) {
#ifndef SKIP_ERROR_HANDLING
	// Integrity checks
	if (vertex_count == 0)
		return;

	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
		return;
	}
#endif

	// Changing current openGL machine state
	phase = NONE;
	int i, j;

	sceneReset();

	// Invalidating current attributes state settings
	uint8_t orig_state = ffp_vertex_attrib_state;
	ffp_vertex_attrib_state = 0xFF;
	ffp_dirty_frag = GL_TRUE;
	ffp_dirty_vert = GL_TRUE;
	reload_ffp_shaders(legacy_vertex_attrib_config, legacy_vertex_stream_config);

	// Uploading texture to use
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture2d_idx].gxm_tex);

	// Restoring original attributes state settings
	ffp_vertex_attrib_state = orig_state;

	
	// Uploading vertex streams and performing the draw
	int i;
	for (i = 0; i < ffp_vertex_num_params; i++) {
		sceGxmSetVertexStream(gxm_context, i, legacy_pool);
	}
		
	// Restoring original attributes state settings
	ffp_vertex_attrib_state = orig_state;
	
	if (prim_is_quad)
		sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, default_quads_idx_ptr, (vertex_count / 2) * 3);
	else
		sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, default_idx_ptr, vertex_count);

	// Moving legacy pool address offset
	legacy_pool += vertex_count * LEGACY_VERTEX_STRIDE;
}
