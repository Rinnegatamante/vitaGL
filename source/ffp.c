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
#include "shaders/texture_combiners/modulate.h"
#include "shaders/texture_combiners/replace.h"
#include "shaders/texture_combiners/decal.h"
#include "shaders/texture_combiners/blend.h"
#include "shaders/texture_combiners/add.h"
#ifndef DISABLE_TEXTURE_COMBINER
#include "shaders/texture_combiners/combine.h"
#endif
#include "shared.h"

#define SHADER_CACHE_SIZE 256
#ifndef DISABLE_ADVANCED_SHADER_CACHE
#define SHADER_CACHE_MAGIC 0 // This must be increased whenever ffp shader sources or shader mask/combiner mask changes
//#define DUMP_SHADER_SOURCES // Enable this flag to dump shader sources inside shader cache
#endif

#define VERTEX_UNIFORMS_NUM 11
#define FRAGMENT_UNIFORMS_NUM 7

static uint32_t vertex_count = 0; // Vertex counter for vertex list
static SceGxmPrimitiveType prim; // Current in use primitive for rendering
GLboolean prim_is_non_native = GL_FALSE; // Flag for when a primitive not supported natively by sceGxm is used

int8_t client_texture_unit = 0; // Current in use client side texture unit

// Lighting
GLboolean lighting_state = GL_FALSE; // Current lighting processor state
GLboolean lights_aligned; // Are clip planes in a contiguous range
uint8_t light_range[2]; // The highest and lowest enabled lights
uint8_t light_mask; // Bitmask of enabled lights
vector4f lights_ambients[MAX_LIGHTS_NUM];
vector4f lights_diffuses[MAX_LIGHTS_NUM];
vector4f lights_speculars[MAX_LIGHTS_NUM];
vector4f lights_positions[MAX_LIGHTS_NUM];
vector3f lights_attenuations[MAX_LIGHTS_NUM];

// Fogging
GLboolean fogging = GL_FALSE; // Current fogging processor state
GLint fog_mode = GL_EXP; // Current fogging mode (openGL)
fogType internal_fog_mode = DISABLED; // Current fogging mode (sceGxm)
GLfloat fog_density = 1.0f; // Current fogging density
GLfloat fog_near = 0.0f; // Current fogging near distance
GLfloat fog_far = 1.0f; // Current fogging far distance
vector4f fog_color = {0.0f, 0.0f, 0.0f, 0.0f}; // Current fogging color

// Clipping Planes
GLboolean clip_planes_aligned = GL_TRUE; // Are clip planes in a contiguous range?
uint8_t clip_plane_range[2] = {0}; // The hightest enabled clip plane
uint8_t clip_planes_mask = 0; // Bitmask of enabled clip planes
vector4f clip_planes_eq[MAX_CLIP_PLANES_NUM]; // Current equation for user clip planes

// Miscellaneous
glPhase phase = NONE; // Current drawing phase for legacy openGL

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
static uint32_t ffp_vertex_attrib_offsets[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint32_t ffp_vertex_attrib_vbo[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
uint16_t ffp_vertex_attrib_state = 0;
static GLenum ffp_mode;
static uint8_t texcoord_idxs[TEXTURE_COORDS_NUM] = {1, FFP_VERTEX_ATTRIBS_NUM - 1};

typedef union shader_mask {
	struct {
		uint32_t alpha_test_mode : 3;
		uint32_t num_textures : 2;
		uint32_t has_colors : 1;
		uint32_t fog_mode : 2;
		uint32_t clip_planes_num : 3;
		uint32_t lights_num : 4;
		uint32_t tex_env_mode_pass0 : 3;
		uint32_t tex_env_mode_pass1 : 3;
		uint32_t UNUSED : 11;
	};
	uint32_t raw;
} shader_mask;
#ifndef DISABLE_TEXTURE_COMBINER
typedef union combiner_mask {
	struct {
		combinerState pass0;
		combinerState pass1;
	};
	uint64_t raw;
} combiner_mask;
#endif

typedef struct {
	SceGxmProgram *frag;
	SceGxmProgram *vert;
	SceGxmShaderPatcherId frag_id;
	SceGxmShaderPatcherId vert_id;
	shader_mask mask;
#ifndef DISABLE_TEXTURE_COMBINER
	combiner_mask cmb_mask;
#endif
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
	NORMAL_MATRIX_UNIF,
	POINT_SIZE_UNIF
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
GLboolean dirty_frag_unifs = GL_TRUE;
GLboolean dirty_vert_unifs = GL_TRUE;
blend_config ffp_blend_info;
shader_mask ffp_mask = {.raw = 0};
#ifndef DISABLE_TEXTURE_COMBINER
combiner_mask ffp_combiner_mask = {.raw = 0};
#endif

SceGxmVertexAttribute ffp_vertex_attribute[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream ffp_vertex_stream[FFP_VERTEX_ATTRIBS_NUM];

void reload_vertex_uniforms() {
	ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "clip_planes_eq");
	ffp_vertex_params[MODELVIEW_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "modelview");
	ffp_vertex_params[WVP_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "wvp");
	ffp_vertex_params[TEX_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "texmat");
	ffp_vertex_params[POINT_SIZE_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "point_size");
	ffp_vertex_params[NORMAL_MATRIX_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "normal_mat");
	if (ffp_vertex_params[NORMAL_MATRIX_UNIF]) {
		ffp_vertex_params[LIGHTS_AMBIENTS_UNIF] = sceGxmProgramFindParameterByName(ffp_vertex_program, "lights_ambients");
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

#ifndef DISABLE_TEXTURE_COMBINER
void setup_combiner_pass(int i, char *dst) {
	char tmp[2048];
	char arg0_rgb[32], arg1_rgb[32], arg2_rgb[32];
	char arg0_a[32], arg1_a[32], arg2_a[32];
	char *args[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	int args_count;
	
	if (texture_units[i].combiner.rgb_func == INTERPOLATE) { // Arg0, Arg2, Arg1, Arg2
		sprintf(arg2_rgb, op_modes[texture_units[i].combiner.op_mode_rgb_2], operands[texture_units[i].combiner.op_rgb_2]);
		args[0] = arg2_rgb;
		args[1] = arg1_rgb;
		args[2] = arg2_rgb;
		args[3] = arg0_a;
		args_count = 4;
	}
	if (texture_units[i].combiner.rgb_func != REPLACE) { // Arg0, Arg1
		sprintf(arg1_rgb, op_modes[texture_units[i].combiner.op_mode_rgb_1], operands[texture_units[i].combiner.op_rgb_1]);
		if (!args[0]) {
			args[0] = arg1_rgb;
			args[1] = arg0_a;
			args_count = 2;
		}
	} else { // Arg0
		args[0] = arg0_a;
		args_count = 1;
	}
	if (texture_units[i].combiner.a_func == INTERPOLATE) { // Arg0, Arg2, Arg1, Arg2
		sprintf(arg2_a, op_modes[texture_units[i].combiner.op_mode_a_2], operands[texture_units[i].combiner.op_a_2]);
		args[args_count++] = arg2_a;
		args[args_count++] = arg1_a;
		args[args_count++] = arg2_a;
	}
	if (texture_units[i].combiner.a_func != REPLACE) { // Arg0, Arg1
		sprintf(arg1_a, op_modes[texture_units[i].combiner.op_mode_a_1], operands[texture_units[i].combiner.op_a_1]);
		args[args_count++] = arg1_a;
	}
	
	// Common arguments
	sprintf(arg0_rgb, op_modes[texture_units[i].combiner.op_mode_rgb_0], operands[texture_units[i].combiner.op_rgb_0]);
	sprintf(arg0_a, op_modes[texture_units[i].combiner.op_mode_a_0], operands[texture_units[i].combiner.op_a_0]);
	
	sprintf(tmp, combine_src, i, calc_funcs[texture_units[i].combiner.rgb_func], calc_funcs[texture_units[i].combiner.a_func]);
	switch (args_count) {
	case 1:
		sprintf(dst, tmp, arg0_rgb, args[0]);
		break;
	case 2:
		sprintf(dst, tmp, arg0_rgb, args[0], args[1]);
		break;
	case 3:
		sprintf(dst, tmp, arg0_rgb, args[0], args[1], args[2]);
		break;
	case 4:
		sprintf(dst, tmp, arg0_rgb, args[0], args[1], args[2], args[3]);
		break;
	case 5:
		sprintf(dst, tmp, arg0_rgb, args[0], args[1], args[2], args[3], args[4]);
		break;
	case 6:
		sprintf(dst, tmp, arg0_rgb, args[0], args[1], args[2], args[3], args[4], args[5]);
		break;
	case 7:
		sprintf(dst, tmp, arg0_rgb, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
		break;
	default:
		break;
	}
	
}
#endif

void reload_ffp_shaders(SceGxmVertexAttribute *attrs, SceGxmVertexStream *streams) {
	// Checking if mask changed
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	GLboolean ffp_dirty_frag_blend = ffp_blend_info.raw != blend_info.raw;
	shader_mask mask = {.raw = 0};
#ifndef DISABLE_TEXTURE_COMBINER
	combiner_mask cmb_mask = {.raw = 0};
#endif
	mask.alpha_test_mode = alpha_op;
	mask.has_colors = (ffp_vertex_attrib_state & (1 << 2)) ? GL_TRUE : GL_FALSE;
	mask.fog_mode = internal_fog_mode;
	
	// Counting number of enabled texture units
	mask.num_textures = 0;
	for (int i = 0; i < TEXTURE_COORDS_NUM; i++) {
		if ((texture_units[i].enabled || attrs == legacy_vertex_attrib_config) && (ffp_vertex_attrib_state & (1 << texcoord_idxs[i]))) {
			mask.num_textures++;
			switch (i) {
			case 0:
				mask.tex_env_mode_pass0 = texture_units[0].env_mode;
#ifndef DISABLE_TEXTURE_COMBINER
				if (mask.tex_env_mode_pass0 == COMBINE)
					cmb_mask.pass0.raw = texture_units[0].combiner.raw;
#endif
				break;
			case 1:
				mask.tex_env_mode_pass1 = texture_units[1].env_mode;
#ifndef DISABLE_TEXTURE_COMBINER
				if (mask.tex_env_mode_pass1 == COMBINE)
					cmb_mask.pass1.raw = texture_units[1].combiner.raw;
#endif
				break;
			default:
				break;
			}
		}
	}

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

	float *light_vars[MAX_LIGHTS_NUM][5];
	if (!lighting_state)
		mask.lights_num = 0;
	else {
		if (lights_aligned) {
			light_vars[0][0] = &lights_ambients[light_range[0]].x;
			light_vars[0][1] = &lights_diffuses[light_range[0]].x;
			light_vars[0][2] = &lights_speculars[light_range[0]].x;
			light_vars[0][3] = &lights_positions[light_range[0]].x;
			light_vars[0][4] = &lights_attenuations[light_range[0]].x;
			mask.lights_num = light_range[1] - light_range[0];
		} else {
			for (int i = light_range[0]; i < light_range[1]; i++) {
				if (light_mask & (1 << i)) {
					light_vars[mask.lights_num][0] = &lights_ambients[i].x;
					light_vars[mask.lights_num][1] = &lights_diffuses[i].x;
					light_vars[mask.lights_num][2] = &lights_speculars[i].x;
					light_vars[mask.lights_num][3] = &lights_positions[i].x;
					light_vars[mask.lights_num][4] = &lights_attenuations[i].x;
					mask.lights_num++;
				}
			}
		}
	}
#ifdef DISABLE_TEXTURE_COMBINER
	if (ffp_mask.raw == mask.raw) { // Fixed function pipeline config didn't change
#else
	if (ffp_mask.raw == mask.raw && ffp_combiner_mask.raw == cmb_mask.raw) { // Fixed function pipeline config didn't change
#endif
		ffp_dirty_vert = GL_FALSE;
		ffp_dirty_frag = GL_FALSE;
	} else {
		int i;
		for (i = 0; i < shader_cache_size; i++) {
#ifdef DISABLE_TEXTURE_COMBINER
			if (shader_cache[i].mask.raw == mask.raw) {
#else
			if (shader_cache[i].mask.raw == mask.raw && shader_cache[i].cmb_mask.raw == cmb_mask.raw) {
#endif
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
		dirty_frag_unifs = GL_TRUE;
		dirty_vert_unifs = GL_TRUE;
		ffp_mask.raw = mask.raw;
#ifndef DISABLE_TEXTURE_COMBINER
		ffp_combiner_mask.raw = cmb_mask.raw;
#endif
	}

	GLboolean new_shader_flag = ffp_dirty_vert || ffp_dirty_frag;

	// Checking if vertex shader requires a recompilation
	if (ffp_dirty_vert) {
#ifndef DISABLE_ADVANCED_SHADER_CACHE
		char fname[256];
#ifndef DISABLE_TEXTURE_COMBINER
		sprintf(fname, "ux0:data/shader_cache/v%d-%08X-%016X_v.gxp", SHADER_CACHE_MAGIC, mask.raw, cmb_mask.raw);
#else
		sprintf(fname, "ux0:data/shader_cache/v%d-%08X-0000000000000000_v.gxp", SHADER_CACHE_MAGIC, mask.raw);
#endif
		FILE *f = fopen(fname, "rb");
		if (f) {
			// Gathering the precompiled shader from cache
			fseek(f, 0, SEEK_END);
			uint32_t size = ftell(f);
			fseek(f, 0, SEEK_SET);
			ffp_vertex_program = (SceGxmProgram *)vgl_malloc(size, VGL_MEM_EXTERNAL);
			fread(ffp_vertex_program, 1, size, f);
			fclose(f);
		} else
#endif
		{
			// Restarting vitaShaRK if we released it before
			if (!is_shark_online)
				startShaderCompiler();
		
			// Compiling the new shader
			char vshader[8192];
			sprintf(vshader, ffp_vert_src, mask.clip_planes_num, mask.num_textures, mask.has_colors, mask.lights_num);
			uint32_t size = strlen(vshader);
			SceGxmProgram *t = shark_compile_shader_extended(vshader, &size, SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
			ffp_vertex_program = (SceGxmProgram *)vgl_malloc(size, VGL_MEM_EXTERNAL);
			sceClibMemcpy((void *)ffp_vertex_program, (void *)t, size);
			shark_clear_output();
#ifndef DISABLE_ADVANCED_SHADER_CACHE
			// Saving compiled shader in filesystem cache
			f = fopen(fname, "wb");
			fwrite(ffp_vertex_program, 1, size, f);
			fclose(f);
#ifdef DUMP_SHADER_SOURCES
#ifndef DISABLE_TEXTURE_COMBINER
			sprintf(fname, "ux0:data/shader_cache/v%d-%08X-%016X_v.cg", SHADER_CACHE_MAGIC, mask.raw, cmb_mask.raw);
#else
			sprintf(fname, "ux0:data/shader_cache/v%d-%08X-0000000000000000_v.cg", SHADER_CACHE_MAGIC, mask.raw);
#endif
			// Saving shader source in filesystem cache
			f = fopen(fname, "wb");
			fwrite(vshader, 1, strlen(vshader), f);
			fclose(f);
#endif
#endif
		}
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_vertex_program, &ffp_vertex_program_id);
		
		// Checking for existing uniforms in the shader
		reload_vertex_uniforms();

		// Clearing dirty flags
		ffp_dirty_vert = GL_FALSE;
	}

	// Not going for the vertex config setup if we have aligned datas
	if (!attrs && mask.num_textures == 1) {
		attrs = ffp_vertex_attrib_config;
		streams = ffp_vertex_stream_config;
	}

	ffp_vertex_num_params = 1;
	if (attrs) { // Immediate mode and non-immediate only when #textures == 1
		// Vertex positions
		const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(ffp_vertex_program, "position");
		attrs[0].regIndex = sceGxmProgramParameterGetResourceIndex(param);

		// Vertex texture coordinates
		param = sceGxmProgramFindParameterByName(ffp_vertex_program, "texcoord0");
		attrs[1].regIndex = sceGxmProgramParameterGetResourceIndex(param);

		// Vertex colors
		if (mask.has_colors) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "color");
			attrs[2].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_num_params += 2;
		} else
			ffp_vertex_num_params++;

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
	} else { // Non immediate mode
		// Vertex positions
		const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(ffp_vertex_program, "position");
		sceClibMemcpy(&ffp_vertex_attribute[0], &ffp_vertex_attrib_config[0], sizeof(SceGxmVertexAttribute));
		ffp_vertex_attribute[0].streamIndex = 0;
		ffp_vertex_attribute[0].regIndex = sceGxmProgramParameterGetResourceIndex(param);
		ffp_vertex_stream[0].stride = ffp_vertex_stream_config[0].stride;
		ffp_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

		// Vertex texture coordinates (First pass)
		if (mask.num_textures > 0) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "texcoord0");
			sceClibMemcpy(&ffp_vertex_attribute[1], &ffp_vertex_attrib_config[texcoord_idxs[0]], sizeof(SceGxmVertexAttribute));
			ffp_vertex_attribute[1].streamIndex = 1;
			ffp_vertex_attribute[1].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_stream[1].stride = ffp_vertex_stream_config[texcoord_idxs[0]].stride;
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
		
		// Vertex texture coordinates (Second pass)
		if (mask.num_textures > 1) {
			param = sceGxmProgramFindParameterByName(ffp_vertex_program, "texcoord1");
			sceClibMemcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[texcoord_idxs[1]], sizeof(SceGxmVertexAttribute));
			ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
			ffp_vertex_attribute[ffp_vertex_num_params].regIndex = sceGxmProgramParameterGetResourceIndex(param);
			ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[texcoord_idxs[1]].stride;
			ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
			ffp_vertex_num_params++;
		}
		
		streams = ffp_vertex_stream;
		attrs = ffp_vertex_attribute;
	}

	// Creating patched vertex shader
	patchVertexProgram(gxm_shader_patcher, ffp_vertex_program_id, attrs, ffp_vertex_num_params, streams, ffp_vertex_num_params, &ffp_vertex_program_patched);

	// Checking if fragment shader requires a recompilation
	if (ffp_dirty_frag) {
#ifndef DISABLE_ADVANCED_SHADER_CACHE
		char fname[256];
#ifndef DISABLE_TEXTURE_COMBINER
		sprintf(fname, "ux0:data/shader_cache/v%d-%08X-%016X_f.gxp", SHADER_CACHE_MAGIC, mask.raw, cmb_mask.raw);
#else
		sprintf(fname, "ux0:data/shader_cache/v%d-%08X-0000000000000000_f.gxp", SHADER_CACHE_MAGIC, mask.raw);
#endif
		FILE *f = fopen(fname, "rb");
		if (f) {
			// Gathering the precompiled shader from cache
			fseek(f, 0, SEEK_END);
			uint32_t size = ftell(f);
			fseek(f, 0, SEEK_SET);
			ffp_fragment_program = (SceGxmProgram *)vgl_malloc(size, VGL_MEM_EXTERNAL);
			fread(ffp_fragment_program, 1, size, f);
			fclose(f);
		} else
#endif
		{
			// Restarting vitaShaRK if we released it before
			if (!is_shark_online)
				startShaderCompiler();
			
			// Compiling the new shader
			char fshader[8192] = {0};
			GLboolean unused_mode[5] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
			for (int i = 0; i < mask.num_textures; i++) {
				char tmp[1024];
				switch (texture_units[i].env_mode) {
				case MODULATE:
					if (unused_mode[MODULATE]) {
						sprintf(fshader, "%s\n%s", fshader, modulate_src);
						unused_mode[MODULATE] = GL_FALSE;
					}
					break;
				case DECAL:
					if (unused_mode[DECAL]) {
						sprintf(fshader, "%s\n%s", fshader, decal_src);
						unused_mode[DECAL] = GL_FALSE;
					}
					break;
				case BLEND:
					if (unused_mode[BLEND]) {
						sprintf(fshader, "%s\n%s", fshader, blend_src);
						unused_mode[BLEND] = GL_FALSE;
					}
					break;
				case ADD:
					if (unused_mode[ADD]) {
						sprintf(fshader, "%s\n%s", fshader, add_src);
						unused_mode[ADD] = GL_FALSE;
					}
					break;
				case REPLACE:
					if (unused_mode[REPLACE]) {
						sprintf(fshader, "%s\n%s", fshader, replace_src);
						unused_mode[REPLACE] = GL_FALSE;
					}
					break;
#ifndef DISABLE_TEXTURE_COMBINER
				case COMBINE:
					setup_combiner_pass(i, tmp);
					sprintf(fshader, "%s\n%s", fshader, tmp);
					break;
#endif
				default:
					break;
				}
			}
			sprintf(fshader, ffp_frag_src, fshader, alpha_op, mask.num_textures, mask.has_colors, mask.fog_mode, mask.tex_env_mode_pass0 != COMBINE ? mask.tex_env_mode_pass0 : 50, mask.tex_env_mode_pass1 != COMBINE ? mask.tex_env_mode_pass1 : 51);
			uint32_t size = strlen(fshader);
			SceGxmProgram *t = shark_compile_shader_extended(fshader, &size, SHARK_FRAGMENT_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
			ffp_fragment_program = (SceGxmProgram *)vgl_malloc(size, VGL_MEM_EXTERNAL);
			sceClibMemcpy((void *)ffp_fragment_program, (void *)t, size);
			shark_clear_output();
#ifndef DISABLE_ADVANCED_SHADER_CACHE
			// Saving compiled shader in filesystem cache
			f = fopen(fname, "wb");
			fwrite(ffp_fragment_program, 1, size, f);
			fclose(f);
#ifdef DUMP_SHADER_SOURCES
#ifndef DISABLE_TEXTURE_COMBINER
			sprintf(fname, "ux0:data/shader_cache/v%d-%08X-%016X_f.cg", SHADER_CACHE_MAGIC, mask.raw, cmb_mask.raw);
#else
			sprintf(fname, "ux0:data/shader_cache/v%d-%08X-0000000000000000_f.cg", SHADER_CACHE_MAGIC, mask.raw);
#endif
			// Saving shader source in filesystem cache
			f = fopen(fname, "wb");
			fwrite(fshader, 1, strlen(fshader), f);
			fclose(f);
#endif
#endif
		}
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_fragment_program, &ffp_fragment_program_id);
		

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
			vgl_free(shader_cache[shader_cache_idx].frag);
			vgl_free(shader_cache[shader_cache_idx].vert);
		}
		shader_cache[shader_cache_idx].mask.raw = mask.raw;
#ifndef DISABLE_TEXTURE_COMBINER
		shader_cache[shader_cache_idx].cmb_mask.raw = cmb_mask.raw;
#endif
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
		dirty_vert_unifs = GL_TRUE;
	}

	// Uploading fragment shader uniforms
	void *buffer;
	if (dirty_frag_unifs) {
		if (vglReserveFragmentUniformBuffer(ffp_fragment_program, &buffer)) {
			if (ffp_fragment_params[ALPHA_CUT_UNIF])
				sceGxmSetUniformDataF(buffer, ffp_fragment_params[ALPHA_CUT_UNIF], 0, 1, &alpha_ref);
			if (ffp_fragment_params[FOG_COLOR_UNIF])
				sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_COLOR_UNIF], 0, 4, &fog_color.r);
			if (ffp_fragment_params[TEX_ENV_COLOR_UNIF]) {
				for (int i = 0; i < mask.num_textures; i++) {
					sceGxmSetUniformDataF(buffer, ffp_fragment_params[TEX_ENV_COLOR_UNIF], 4 * i, 4, (const float *)&texture_units[i].env_color.r);
				}
			}
			if (ffp_fragment_params[TINT_COLOR_UNIF])
				sceGxmSetUniformDataF(buffer, ffp_fragment_params[TINT_COLOR_UNIF], 0, 4, &current_vtx.clr.r);
			if (ffp_fragment_params[FOG_NEAR_UNIF])
				sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_NEAR_UNIF], 0, 1, (const float *)&fog_near);
			if (ffp_fragment_params[FOG_FAR_UNIF])
				sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_FAR_UNIF], 0, 1, (const float *)&fog_far);
			if (ffp_fragment_params[FOG_DENSITY_UNIF])
				sceGxmSetUniformDataF(buffer, ffp_fragment_params[FOG_DENSITY_UNIF], 0, 1, (const float *)&fog_density);
		}
		dirty_frag_unifs = GL_FALSE;
	}

	// Uploading vertex shader uniforms
	if (dirty_vert_unifs) {
		vglReserveVertexUniformBuffer(ffp_vertex_program, &buffer);
		if (ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF])
			sceGxmSetUniformDataF(buffer, ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF], 0, 4 * mask.clip_planes_num, &clip_planes[0].x);
		if (ffp_vertex_params[MODELVIEW_MATRIX_UNIF])
			sceGxmSetUniformDataF(buffer, ffp_vertex_params[MODELVIEW_MATRIX_UNIF], 0, 16, (const float *)modelview_matrix);
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[WVP_MATRIX_UNIF], 0, 16, (const float *)mvp_matrix);
		if (ffp_vertex_params[TEX_MATRIX_UNIF])
			sceGxmSetUniformDataF(buffer, ffp_vertex_params[TEX_MATRIX_UNIF], 0, 16, (const float *)texture_matrix);
		sceGxmSetUniformDataF(buffer, ffp_vertex_params[POINT_SIZE_UNIF], 0, 1, &point_size);
		if (ffp_vertex_params[NORMAL_MATRIX_UNIF]) {
			sceGxmSetUniformDataF(buffer, ffp_vertex_params[NORMAL_MATRIX_UNIF], 0, 16, (const float *)normal_matrix);
			if (lights_aligned) {
				sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_AMBIENTS_UNIF], 0, 4 * mask.lights_num, (const float *)light_vars[0][0]);
				sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_DIFFUSES_UNIF], 0, 4 * mask.lights_num, (const float *)light_vars[0][1]);
				sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_SPECULARS_UNIF], 0, 4 * mask.lights_num, (const float *)light_vars[0][2]);
				sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_POSITIONS_UNIF], 0, 4 * mask.lights_num, (const float *)light_vars[0][3]);
				sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_ATTENUATIONS_UNIF], 0, 3 * mask.lights_num, (const float *)light_vars[0][4]);
			} else {
				for (int i = 0; i < mask.lights_num; i++) {
					sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_AMBIENTS_UNIF], 4 * i, 4, (const float *)light_vars[i][0]);
					sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_DIFFUSES_UNIF], 4 * i, 4, (const float *)light_vars[i][1]);
					sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_SPECULARS_UNIF], 4 * i, 4, (const float *)light_vars[i][2]);
					sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_POSITIONS_UNIF], 4 * i, 4, (const float *)light_vars[i][3]);
					sceGxmSetUniformDataF(buffer, ffp_vertex_params[LIGHTS_ATTENUATIONS_UNIF], 3 * i, 3, (const float *)light_vars[i][4]);
				}
			}
		}
		dirty_vert_unifs = GL_FALSE;
	}
}

void _glDrawArrays_FixedFunctionIMPL(GLsizei count) {
	reload_ffp_shaders(NULL, NULL);

	// Uploading textures on relative texture units
	for (int i = 0; i < ffp_mask.num_textures; i++) {
		sceGxmSetFragmentTexture(gxm_context, i, &texture_slots[texture_units[i].tex_id].gxm_tex);
	}

	// Uploading vertex streams
	int i, j = 0;
	void *ptrs[FFP_VERTEX_ATTRIBS_NUM];
	for (i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (ffp_vertex_attrib_state & (1 << i)) {
			if (ffp_vertex_attrib_vbo[i]) {
				gpubuffer *gpu_buf = (gpubuffer *)ffp_vertex_attrib_vbo[i];
				gpu_buf->used = GL_TRUE;
				ptrs[i] = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[i];
			} else {
#ifdef DRAW_SPEEDHACK
				ptrs[i] = (void *)ffp_vertex_attrib_offsets[i];
#else
				uint32_t size = count * ffp_vertex_stream_config[i].stride;
				ptrs[i] = gpu_alloc_mapped_temp(size);
				sceClibMemcpy(ptrs[i], (void *)ffp_vertex_attrib_offsets[i], size);
#endif
			}
			sceGxmSetVertexStream(gxm_context, j++, ptrs[i]);
		}
	}
}

void _glDrawElements_FixedFunctionIMPL(uint16_t *idx_buf, GLsizei count) {
	reload_ffp_shaders(NULL, NULL);
	int attr_idxs[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
	int attr_num = 0;
	GLboolean is_full_vbo = GL_TRUE;
	for (int i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (ffp_vertex_attrib_state & (1 << i)) {
			if (!ffp_vertex_attrib_vbo[i])
				is_full_vbo = GL_FALSE;
			attr_idxs[attr_num++] = i;
		}
	}

#ifndef DRAW_SPEEDHACK
	// Detecting highest index value
	uint32_t top_idx = 0;
	if (!is_full_vbo) {
		for (int i = 0; i < count; i++) {
			if (idx_buf[i] > top_idx)
				top_idx = idx_buf[i];
		}
		top_idx++;
	}
#endif

	// Uploading textures on relative texture units
	for (int i = 0; i < ffp_mask.num_textures; i++) {
		sceGxmSetFragmentTexture(gxm_context, i, &texture_slots[texture_units[i].tex_id].gxm_tex);
	}

	// Uploading vertex streams
	void *ptrs[FFP_VERTEX_ATTRIBS_NUM];
	for (int i = 0; i < attr_num; i++) {
		int attr_idx = attr_idxs[i];
		if (ffp_vertex_attrib_vbo[attr_idx]) {
			gpubuffer *gpu_buf = (gpubuffer *)ffp_vertex_attrib_vbo[attr_idx];
			gpu_buf->used = GL_TRUE;
			ptrs[i] = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[attr_idx];
		} else {
#ifdef DRAW_SPEEDHACK
			ptrs[i] = (void *)ffp_vertex_attrib_offsets[attr_idx];
#else
			uint32_t size = top_idx * ffp_vertex_stream_config[attr_idx].stride;
			ptrs[i] = gpu_alloc_mapped_temp(size);
			sceClibMemcpy(ptrs[i], (void *)ffp_vertex_attrib_offsets[attr_idx], size);
#endif
		}
		sceGxmSetVertexStream(gxm_context, i, ptrs[i]);
	}
}

void update_fogging_state() {
	ffp_dirty_frag = GL_TRUE;
	if (fogging) {
		switch (fog_mode) {
		case GL_LINEAR:
			internal_fog_mode = LINEAR;
			break;
		case GL_EXP:
			internal_fog_mode = EXP;
			break;
		default:
			internal_fog_mode = EXP2;
			break;
		}
	} else
		internal_fog_mode = DISABLED;
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glEnableClientState(GLenum array) {
	ffp_dirty_vert = GL_TRUE;
	ffp_dirty_frag = GL_TRUE;
	switch (array) {
	case GL_VERTEX_ARRAY:
		ffp_vertex_attrib_state |= (1 << 0);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ffp_vertex_attrib_state |= (1 << texcoord_idxs[client_texture_unit]);
		break;
	case GL_COLOR_ARRAY:
		ffp_vertex_attrib_state |= (1 << 2);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glDisableClientState(GLenum array) {
	ffp_dirty_vert = GL_TRUE;
	ffp_dirty_frag = GL_TRUE;
	switch (array) {
	case GL_VERTEX_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << 0);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << texcoord_idxs[client_texture_unit]);
		break;
	case GL_COLOR_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << 2);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
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
	
	ffp_vertex_attrib_offsets[texcoord_idxs[client_texture_unit]] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[texcoord_idxs[client_texture_unit]] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[texcoord_idxs[client_texture_unit]];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[texcoord_idxs[client_texture_unit]];

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
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glInterleavedArrays(GLenum format, GLsizei stride, const void *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (stride < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	SceGxmVertexAttribute *attributes;
	SceGxmVertexStream *streams;

	switch (format) {
	case GL_V2F:
		// Vertex2f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 2;
		streams->stride = stride ? stride : 8;
		break;
	case GL_V3F:
		// Vertex3f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 12;
		break;
	case GL_C4UB_V2F:
		// Color4Ub
		ffp_vertex_attrib_offsets[2] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[2] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[2];
		streams = &ffp_vertex_stream_config[2];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		attributes->componentCount = 4;
		streams->stride = stride ? stride : 12;

		// Vertex2f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 4;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 2;
		streams->stride = stride ? stride : 12;
		break;
	case GL_C4UB_V3F:
		// Color4Ub
		ffp_vertex_attrib_offsets[2] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[2] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[2];
		streams = &ffp_vertex_stream_config[2];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		attributes->componentCount = 4;
		streams->stride = stride ? stride : 16;

		// Vertex3f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 4;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 16;
		break;
	case GL_C3F_V3F:
		// Color3f
		ffp_vertex_attrib_offsets[2] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[2] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[2];
		streams = &ffp_vertex_stream_config[2];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 24;

		// Vertex3f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 12;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 24;
		break;
	case GL_T2F_V3F:
		// Texcoord2f
		ffp_vertex_attrib_offsets[1] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[1] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[1];
		streams = &ffp_vertex_stream_config[1];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 2;
		streams->stride = stride ? stride : 20;

		// Vertex3f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 8;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 20;
		break;
	case GL_T4F_V4F:
		// Texcoord4f
		ffp_vertex_attrib_offsets[1] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[1] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[1];
		streams = &ffp_vertex_stream_config[1];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 4;
		streams->stride = stride ? stride : 32;

		// Vertex4f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 16;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 4;
		streams->stride = stride ? stride : 32;
		break;
	case GL_T2F_C4UB_V3F:
		// Texcoord2f
		ffp_vertex_attrib_offsets[1] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[1] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[1];
		streams = &ffp_vertex_stream_config[1];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 2;
		streams->stride = stride ? stride : 24;

		// Color4ub
		ffp_vertex_attrib_offsets[2] = (uint32_t)pointer + 8;
		ffp_vertex_attrib_vbo[2] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[2];
		streams = &ffp_vertex_stream_config[2];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		attributes->componentCount = 4;
		streams->stride = stride ? stride : 24;

		// Vertex3f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 12;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 24;
		break;
	case GL_T2F_C3F_V3F:
		// Texcoord2f
		ffp_vertex_attrib_offsets[1] = (uint32_t)pointer;
		ffp_vertex_attrib_vbo[1] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[1];
		streams = &ffp_vertex_stream_config[1];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 2;
		streams->stride = stride ? stride : 32;

		// Color3f
		ffp_vertex_attrib_offsets[2] = (uint32_t)pointer + 8;
		ffp_vertex_attrib_vbo[2] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[2];
		streams = &ffp_vertex_stream_config[2];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 32;

		// Vertex3f
		ffp_vertex_attrib_offsets[0] = (uint32_t)pointer + 20;
		ffp_vertex_attrib_vbo[0] = vertex_array_unit;
		attributes = &ffp_vertex_attrib_config[0];
		streams = &ffp_vertex_stream_config[0];
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		attributes->componentCount = 3;
		streams->stride = stride ? stride : 32;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
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

void glClientActiveTexture(GLenum texture) {
#ifndef SKIP_ERROR_HANDLING
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE15)) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	} else
#endif
		client_texture_unit = texture - GL_TEXTURE0;
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
	}
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	// Setting current color value
	current_vtx.clr.r = red;
	current_vtx.clr.g = green;
	current_vtx.clr.b = blue;
	current_vtx.clr.a = 1.0f;
	dirty_frag_unifs = GL_TRUE;
}

void glColor3fv(const GLfloat *v) {
	// Setting current color value
	sceClibMemcpy(&current_vtx.clr.r, v, sizeof(vector3f));
	current_vtx.clr.a = 1.0f;
	dirty_frag_unifs = GL_TRUE;
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
	// Setting current color value
	current_vtx.clr.r = (1.0f * red) / 255.0f;
	current_vtx.clr.g = (1.0f * green) / 255.0f;
	current_vtx.clr.b = (1.0f * blue) / 255.0f;
	current_vtx.clr.a = 1.0f;
	dirty_frag_unifs = GL_TRUE;
}

void glColor3ubv(const GLubyte *c) {
	// Setting current color value
	current_vtx.clr.r = (1.0f * c[0]) / 255.0f;
	current_vtx.clr.g = (1.0f * c[1]) / 255.0f;
	current_vtx.clr.b = (1.0f * c[2]) / 255.0f;
	current_vtx.clr.a = 1.0f;
	dirty_frag_unifs = GL_TRUE;
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	// Setting current color value
	current_vtx.clr.r = red;
	current_vtx.clr.g = green;
	current_vtx.clr.b = blue;
	current_vtx.clr.a = alpha;
	dirty_frag_unifs = GL_TRUE;
}

void glColor4fv(const GLfloat *v) {
	// Setting current color value
	sceClibMemcpy(&current_vtx.clr.r, v, sizeof(vector4f));
	dirty_frag_unifs = GL_TRUE;
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	current_vtx.clr.r = (1.0f * red) / 255.0f;
	current_vtx.clr.g = (1.0f * green) / 255.0f;
	current_vtx.clr.b = (1.0f * blue) / 255.0f;
	current_vtx.clr.a = (1.0f * alpha) / 255.0f;
	dirty_frag_unifs = GL_TRUE;
}

void glColor4ubv(const GLubyte *c) {
	// Setting current color value
	current_vtx.clr.r = (1.0f * c[0]) / 255.0f;
	current_vtx.clr.g = (1.0f * c[1]) / 255.0f;
	current_vtx.clr.b = (1.0f * c[2]) / 255.0f;
	current_vtx.clr.a = (1.0f * c[3]) / 255.0f;
	dirty_frag_unifs = GL_TRUE;
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
	dirty_frag_unifs = GL_TRUE;
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

void glNormal3fv(const GLfloat *v) {
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

	// Changing current openGL machine state
	phase = MODEL_CREATION;
#endif

	// Tracking desired primitive
	ffp_mode = mode;

	// Resetting vertex count
	vertex_count = 0;
}

void glEnd(void) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
		return;
	}

	// Changing current openGL machine state
	phase = NONE;
#endif

	// Translating primitive to sceGxm one
	gl_primitive_to_gxm(ffp_mode, prim, vertex_count);

	sceneReset();

	// Invalidating current attributes state settings
	uint8_t orig_state = ffp_vertex_attrib_state;
	ffp_vertex_attrib_state = 0x07;
	ffp_dirty_frag = GL_TRUE;
	ffp_dirty_vert = GL_TRUE;
	reload_ffp_shaders(legacy_vertex_attrib_config, legacy_vertex_stream_config);

	// Uploading texture to use
	sceGxmSetFragmentTexture(gxm_context, 0, &texture_slots[texture_units[0].tex_id].gxm_tex);

	// Restoring original attributes state settings
	ffp_vertex_attrib_state = orig_state;

	// Uploading vertex streams and performing the draw
	int i;
	for (i = 0; i < ffp_vertex_num_params; i++) {
		sceGxmSetVertexStream(gxm_context, i, legacy_pool);
	}

	// Restoring original attributes state settings
	ffp_vertex_attrib_state = orig_state;

	uint16_t *ptr;
	uint32_t index_count;

	// Get the index source
	switch (ffp_mode) {
	case GL_QUADS:
		ptr = default_quads_idx_ptr;
		index_count = (vertex_count / 2) * 3;
		break;
	case GL_LINE_STRIP:
		ptr = default_line_strips_idx_ptr;
		index_count = (vertex_count - 1) * 2;
		break;
	case GL_LINE_LOOP:
		ptr = gpu_alloc_mapped_temp(vertex_count * 2 * sizeof(uint16_t));
		sceClibMemcpy(ptr, default_line_strips_idx_ptr, (vertex_count - 1) * 2 * sizeof(uint16_t));
		ptr[(vertex_count - 1) * 2] = vertex_count - 1;
		ptr[(vertex_count - 1) * 2 + 1] = 0;

		index_count = vertex_count * 2;
		break;
	default:
		ptr = default_idx_ptr;
		index_count = vertex_count;
		break;
	}

	sceGxmDraw(gxm_context, prim, SCE_GXM_INDEX_FORMAT_U16, ptr, index_count);

	// Moving legacy pool address offset
	legacy_pool += vertex_count * LEGACY_VERTEX_STRIDE;

	// Restore polygon mode if a GL_LINES/GL_POINTS has been rendered
	restore_polygon_mode(prim);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];

	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
		case GL_TEXTURE_ENV_MODE:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_MODULATE)
				tex_unit->env_mode = MODULATE;
			else if (param == GL_DECAL)
				tex_unit->env_mode = DECAL;
			else if (param == GL_REPLACE)
				tex_unit->env_mode = REPLACE;
			else if (param == GL_BLEND)
				tex_unit->env_mode = BLEND;
			else if (param == GL_ADD)
				tex_unit->env_mode = ADD;
#ifndef DISABLE_TEXTURE_COMBINER
			else if (param == GL_COMBINE)
				tex_unit->env_mode = COMBINE;
#endif
			break;
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_COMBINE_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_REPLACE)
				tex_unit->combiner.rgb_func = REPLACE;
			else if (param == GL_MODULATE)
				tex_unit->combiner.rgb_func = MODULATE;
			else if (param == GL_ADD)
				tex_unit->combiner.rgb_func = ADD;
			else if (param == GL_ADD_SIGNED)
				tex_unit->combiner.rgb_func = ADD_SIGNED;
			else if (param == GL_INTERPOLATE)
				tex_unit->combiner.rgb_func = INTERPOLATE;
			else if (param == GL_SUBTRACT)
				tex_unit->combiner.rgb_func = SUBTRACT;
			break;
		case GL_COMBINE_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_REPLACE)
				tex_unit->combiner.a_func = REPLACE;
			else if (param == GL_MODULATE)
				tex_unit->combiner.a_func = MODULATE;
			else if (param == GL_ADD)
				tex_unit->combiner.a_func = ADD;
			else if (param == GL_ADD_SIGNED)
				tex_unit->combiner.a_func = ADD_SIGNED;
			else if (param == GL_INTERPOLATE)
				tex_unit->combiner.a_func = INTERPOLATE;
			else if (param == GL_SUBTRACT)
				tex_unit->combiner.a_func = SUBTRACT;
			break;
		case GL_SRC0_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_TEXTURE)
				tex_unit->combiner.op_rgb_0 = TEXTURE;
			else if (param == GL_CONSTANT)
				tex_unit->combiner.op_rgb_0 = CONSTANT;
			else if (param == GL_PRIMARY_COLOR)
				tex_unit->combiner.op_rgb_0 = PRIMARY_COLOR;
			else if (param == GL_PREVIOUS)
				tex_unit->combiner.op_rgb_0 = PREVIOUS;
			break;
		case GL_SRC1_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_TEXTURE)
				tex_unit->combiner.op_rgb_1 = TEXTURE;
			else if (param == GL_CONSTANT)
				tex_unit->combiner.op_rgb_1 = CONSTANT;
			else if (param == GL_PRIMARY_COLOR)
				tex_unit->combiner.op_rgb_1 = PRIMARY_COLOR;
			else if (param == GL_PREVIOUS)
				tex_unit->combiner.op_rgb_1 = PREVIOUS;
			break;
		case GL_SRC2_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_TEXTURE)
				tex_unit->combiner.op_rgb_2 = TEXTURE;
			else if (param == GL_CONSTANT)
				tex_unit->combiner.op_rgb_2 = CONSTANT;
			else if (param == GL_PRIMARY_COLOR)
				tex_unit->combiner.op_rgb_2 = PRIMARY_COLOR;
			else if (param == GL_PREVIOUS)
				tex_unit->combiner.op_rgb_2 = PREVIOUS;
			break;
		case GL_SRC0_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_TEXTURE)
				tex_unit->combiner.op_a_0 = TEXTURE;
			else if (param == GL_CONSTANT)
				tex_unit->combiner.op_a_0 = CONSTANT;
			else if (param == GL_PRIMARY_COLOR)
				tex_unit->combiner.op_a_0 = PRIMARY_COLOR;
			else if (param == GL_PREVIOUS)
				tex_unit->combiner.op_a_0 = PREVIOUS;
			break;
		case GL_SRC1_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_TEXTURE)
				tex_unit->combiner.op_a_1 = TEXTURE;
			else if (param == GL_CONSTANT)
				tex_unit->combiner.op_a_1 = CONSTANT;
			else if (param == GL_PRIMARY_COLOR)
				tex_unit->combiner.op_a_1 = PRIMARY_COLOR;
			else if (param == GL_PREVIOUS)
				tex_unit->combiner.op_a_1 = PREVIOUS;
			break;
		case GL_SRC2_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_TEXTURE)
				tex_unit->combiner.op_a_2 = TEXTURE;
			else if (param == GL_CONSTANT)
				tex_unit->combiner.op_a_2 = CONSTANT;
			else if (param == GL_PRIMARY_COLOR)
				tex_unit->combiner.op_a_2 = PRIMARY_COLOR;
			else if (param == GL_PREVIOUS)
				tex_unit->combiner.op_a_2 = PREVIOUS;
			break;
		case GL_OPERAND0_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_SRC_COLOR)
				tex_unit->combiner.op_mode_rgb_0 = SRC_COLOR;
			else if (param == GL_ONE_MINUS_SRC_COLOR)
				tex_unit->combiner.op_mode_rgb_0 = ONE_MINUS_SRC_COLOR;
			else if (param == GL_SRC_ALPHA)
				tex_unit->combiner.op_mode_rgb_0 = SRC_ALPHA;
			else if (param == GL_ONE_MINUS_SRC_ALPHA)
				tex_unit->combiner.op_mode_rgb_0 = ONE_MINUS_SRC_ALPHA;
			break;
		case GL_OPERAND1_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_SRC_COLOR)
				tex_unit->combiner.op_mode_rgb_1 = SRC_COLOR;
			else if (param == GL_ONE_MINUS_SRC_COLOR)
				tex_unit->combiner.op_mode_rgb_1 = ONE_MINUS_SRC_COLOR;
			else if (param == GL_SRC_ALPHA)
				tex_unit->combiner.op_mode_rgb_1 = SRC_ALPHA;
			else if (param == GL_ONE_MINUS_SRC_ALPHA)
				tex_unit->combiner.op_mode_rgb_1 = ONE_MINUS_SRC_ALPHA;
			break;
		case GL_OPERAND2_RGB:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_SRC_COLOR)
				tex_unit->combiner.op_mode_rgb_2 = SRC_COLOR;
			else if (param == GL_ONE_MINUS_SRC_COLOR)
				tex_unit->combiner.op_mode_rgb_2 = ONE_MINUS_SRC_COLOR;
			else if (param == GL_SRC_ALPHA)
				tex_unit->combiner.op_mode_rgb_2 = SRC_ALPHA;
			else if (param == GL_ONE_MINUS_SRC_ALPHA)
				tex_unit->combiner.op_mode_rgb_2 = ONE_MINUS_SRC_ALPHA;
			break;
		case GL_OPERAND0_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_SRC_COLOR)
				tex_unit->combiner.op_mode_a_0 = SRC_COLOR;
			else if (param == GL_ONE_MINUS_SRC_COLOR)
				tex_unit->combiner.op_mode_a_0 = ONE_MINUS_SRC_COLOR;
			else if (param == GL_SRC_ALPHA)
				tex_unit->combiner.op_mode_a_0 = SRC_ALPHA;
			else if (param == GL_ONE_MINUS_SRC_ALPHA)
				tex_unit->combiner.op_mode_a_0 = ONE_MINUS_SRC_ALPHA;
			break;
		case GL_OPERAND1_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_SRC_COLOR)
				tex_unit->combiner.op_mode_a_1 = SRC_COLOR;
			else if (param == GL_ONE_MINUS_SRC_COLOR)
				tex_unit->combiner.op_mode_a_1 = ONE_MINUS_SRC_COLOR;
			else if (param == GL_SRC_ALPHA)
				tex_unit->combiner.op_mode_a_1 = SRC_ALPHA;
			else if (param == GL_ONE_MINUS_SRC_ALPHA)
				tex_unit->combiner.op_mode_a_1 = ONE_MINUS_SRC_ALPHA;
			break;
		case GL_OPERAND2_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			if (param == GL_SRC_COLOR)
				tex_unit->combiner.op_mode_a_2 = SRC_COLOR;
			else if (param == GL_ONE_MINUS_SRC_COLOR)
				tex_unit->combiner.op_mode_a_2 = ONE_MINUS_SRC_COLOR;
			else if (param == GL_SRC_ALPHA)
				tex_unit->combiner.op_mode_a_2 = SRC_ALPHA;
			else if (param == GL_ONE_MINUS_SRC_ALPHA)
				tex_unit->combiner.op_mode_a_2 = ONE_MINUS_SRC_ALPHA;
			break;
#endif
		default:
			SET_GL_ERROR(GL_INVALID_ENUM)
		}
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glTexEnvfv(GLenum target, GLenum pname, GLfloat *param) {
	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
		case GL_TEXTURE_ENV_COLOR:
			sceClibMemcpy(&texture_units[server_texture_unit].env_color.r, param, sizeof(GLfloat) * 4);
			break;
		default:
			SET_GL_ERROR(GL_INVALID_ENUM)
		}
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	dirty_frag_unifs = GL_TRUE;
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];

	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
		case GL_TEXTURE_ENV_MODE:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
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
			case GL_ADD:
				tex_unit->env_mode = ADD;
				break;
#ifndef DISABLE_TEXTURE_COMBINER
			case GL_COMBINE:
				tex_unit->env_mode = COMBINE;
				break;
#endif
			}
			break;
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_COMBINE_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_REPLACE:
				tex_unit->combiner.rgb_func = REPLACE;
				break;
			case GL_MODULATE:
				tex_unit->combiner.rgb_func = MODULATE;
				break;
			case GL_ADD:
				tex_unit->combiner.rgb_func = ADD;
				break;
			case GL_ADD_SIGNED:
				tex_unit->combiner.rgb_func = ADD_SIGNED;
				break;
			case GL_INTERPOLATE:
				tex_unit->combiner.rgb_func = INTERPOLATE;
				break;
			case GL_SUBTRACT:
				tex_unit->combiner.rgb_func = SUBTRACT;
				break;
			}
			break;
		case GL_COMBINE_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_REPLACE:
				tex_unit->combiner.a_func = REPLACE;
				break;
			case GL_MODULATE:
				tex_unit->combiner.a_func = MODULATE;
				break;
			case GL_ADD:
				tex_unit->combiner.a_func = ADD;
				break;
			case GL_ADD_SIGNED:
				tex_unit->combiner.a_func = ADD_SIGNED;
				break;
			case GL_INTERPOLATE:
				tex_unit->combiner.a_func = INTERPOLATE;
				break;
			case GL_SUBTRACT:
				tex_unit->combiner.a_func = SUBTRACT;
				break;
			}
			break;
		case GL_SRC0_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_TEXTURE:
				tex_unit->combiner.op_rgb_0 = TEXTURE;
				break;
			case GL_CONSTANT:
				tex_unit->combiner.op_rgb_0 = CONSTANT;
				break;
			case GL_PRIMARY_COLOR:
				tex_unit->combiner.op_rgb_0 = PRIMARY_COLOR;
				break;
			case GL_PREVIOUS:
				tex_unit->combiner.op_rgb_0 = PREVIOUS;
				break;
			}
			break;
		case GL_SRC1_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_TEXTURE:
				tex_unit->combiner.op_rgb_1 = TEXTURE;
				break;
			case GL_CONSTANT:
				tex_unit->combiner.op_rgb_1 = CONSTANT;
				break;
			case GL_PRIMARY_COLOR:
				tex_unit->combiner.op_rgb_1 = PRIMARY_COLOR;
				break;
			case GL_PREVIOUS:
				tex_unit->combiner.op_rgb_1 = PREVIOUS;
				break;
			}
			break;
		case GL_SRC2_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_TEXTURE:
				tex_unit->combiner.op_rgb_2 = TEXTURE;
				break;
			case GL_CONSTANT:
				tex_unit->combiner.op_rgb_2 = CONSTANT;
				break;
			case GL_PRIMARY_COLOR:
				tex_unit->combiner.op_rgb_2 = PRIMARY_COLOR;
				break;
			case GL_PREVIOUS:
				tex_unit->combiner.op_rgb_2 = PREVIOUS;
				break;
			}
			break;
		case GL_SRC0_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_TEXTURE:
				tex_unit->combiner.op_a_0 = TEXTURE;
				break;
			case GL_CONSTANT:
				tex_unit->combiner.op_a_0 = CONSTANT;
				break;
			case GL_PRIMARY_COLOR:
				tex_unit->combiner.op_a_0 = PRIMARY_COLOR;
				break;
			case GL_PREVIOUS:
				tex_unit->combiner.op_a_0 = PREVIOUS;
				break;
			}
			break;
		case GL_SRC1_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_TEXTURE:
				tex_unit->combiner.op_a_1 = TEXTURE;
				break;
			case GL_CONSTANT:
				tex_unit->combiner.op_a_1 = CONSTANT;
				break;
			case GL_PRIMARY_COLOR:
				tex_unit->combiner.op_a_1 = PRIMARY_COLOR;
				break;
			case GL_PREVIOUS:
				tex_unit->combiner.op_a_1 = PREVIOUS;
				break;
			}
			break;
		case GL_SRC2_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_TEXTURE:
				tex_unit->combiner.op_a_2 = TEXTURE;
				break;
			case GL_CONSTANT:
				tex_unit->combiner.op_a_2 = CONSTANT;
				break;
			case GL_PRIMARY_COLOR:
				tex_unit->combiner.op_a_2 = PRIMARY_COLOR;
				break;
			case GL_PREVIOUS:
				tex_unit->combiner.op_a_2 = PREVIOUS;
				break;
			}
			break;
		case GL_OPERAND0_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_SRC_COLOR:
				tex_unit->combiner.op_mode_rgb_0 = SRC_COLOR;
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				tex_unit->combiner.op_mode_rgb_0 = ONE_MINUS_SRC_COLOR;
				break;
			case GL_SRC_ALPHA:
				tex_unit->combiner.op_mode_rgb_0 = SRC_ALPHA;
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				tex_unit->combiner.op_mode_rgb_0 = ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND1_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_SRC_COLOR:
				tex_unit->combiner.op_mode_rgb_1 = SRC_COLOR;
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				tex_unit->combiner.op_mode_rgb_1 = ONE_MINUS_SRC_COLOR;
				break;
			case GL_SRC_ALPHA:
				tex_unit->combiner.op_mode_rgb_1 = SRC_ALPHA;
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				tex_unit->combiner.op_mode_rgb_1 = ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND2_RGB:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_SRC_COLOR:
				tex_unit->combiner.op_mode_rgb_2 = SRC_COLOR;
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				tex_unit->combiner.op_mode_rgb_2 = ONE_MINUS_SRC_COLOR;
				break;
			case GL_SRC_ALPHA:
				tex_unit->combiner.op_mode_rgb_2 = SRC_ALPHA;
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				tex_unit->combiner.op_mode_rgb_2 = ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND0_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_SRC_COLOR:
				tex_unit->combiner.op_mode_a_0 = SRC_COLOR;
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				tex_unit->combiner.op_mode_a_0 = ONE_MINUS_SRC_COLOR;
				break;
			case GL_SRC_ALPHA:
				tex_unit->combiner.op_mode_a_0 = SRC_ALPHA;
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				tex_unit->combiner.op_mode_a_0 = ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND1_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_SRC_COLOR:
				tex_unit->combiner.op_mode_a_1 = SRC_COLOR;
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				tex_unit->combiner.op_mode_a_1 = ONE_MINUS_SRC_COLOR;
				break;
			case GL_SRC_ALPHA:
				tex_unit->combiner.op_mode_a_1 = SRC_ALPHA;
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				tex_unit->combiner.op_mode_a_1 = ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND2_ALPHA:
			ffp_dirty_frag = GL_TRUE;
			switch (param) {
			case GL_SRC_COLOR:
				tex_unit->combiner.op_mode_a_2 = SRC_COLOR;
				break;
			case GL_ONE_MINUS_SRC_COLOR:
				tex_unit->combiner.op_mode_a_2 = ONE_MINUS_SRC_COLOR;
				break;
			case GL_SRC_ALPHA:
				tex_unit->combiner.op_mode_a_2 = SRC_ALPHA;
				break;
			case GL_ONE_MINUS_SRC_ALPHA:
				tex_unit->combiner.op_mode_a_2 = ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
#endif
		default:
			SET_GL_ERROR(GL_INVALID_ENUM)
		}
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
#ifndef SKIP_ERROR_HANDLING
	if (light < GL_LIGHT0 && light > GL_LIGHT7) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#endif

	switch (pname) {
	case GL_AMBIENT:
		sceClibMemcpy(&lights_ambients[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_DIFFUSE:
		sceClibMemcpy(&lights_diffuses[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_SPECULAR:
		sceClibMemcpy(&lights_speculars[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_POSITION:
		sceClibMemcpy(&lights_positions[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_CONSTANT_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].r = params[0];
		break;
	case GL_LINEAR_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].g = params[0];
		break;
	case GL_QUADRATIC_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].b = params[0];
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	
	dirty_vert_unifs = GL_TRUE;
}

void glFogf(GLenum pname, GLfloat param) {
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = param;
		break;
	case GL_FOG_START:
		fog_near = param;
		break;
	case GL_FOG_END:
		fog_far = param;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	dirty_frag_unifs = GL_TRUE;
}

void glFogfv(GLenum pname, const GLfloat *params) {
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = params[0];
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = params[0];
		break;
	case GL_FOG_START:
		fog_near = params[0];
		break;
	case GL_FOG_END:
		fog_far = params[0];
		break;
	case GL_FOG_COLOR:
		sceClibMemcpy(&fog_color.r, params, sizeof(vector4f));
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	dirty_frag_unifs = GL_TRUE;
}

void glFogi(GLenum pname, const GLint param) {
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = param;
		break;
	case GL_FOG_START:
		fog_near = param;
		break;
	case GL_FOG_END:
		fog_far = param;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	dirty_frag_unifs = GL_TRUE;
}

void glClipPlane(GLenum plane, const GLdouble *equation) {
#ifndef SKIP_ERROR_HANDLING
	if (plane < GL_CLIP_PLANE0 || plane > GL_CLIP_PLANE6) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#endif
	int idx = plane - GL_CLIP_PLANE0;
	clip_planes_eq[idx].x = equation[0];
	clip_planes_eq[idx].y = equation[1];
	clip_planes_eq[idx].z = equation[2];
	clip_planes_eq[idx].w = equation[3];
	matrix4x4 inverted, inverted_transposed;
	matrix4x4_invert(inverted, modelview_matrix);
	matrix4x4_transpose(inverted_transposed, inverted);
	vector4f temp;
	vector4f_matrix4x4_mult(&temp, inverted_transposed, &clip_planes_eq[idx]);
	sceClibMemcpy(&clip_planes_eq[idx].x, &temp.x, sizeof(vector4f));
	dirty_vert_unifs = GL_TRUE;
}
