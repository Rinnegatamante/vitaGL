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
#ifdef HAVE_HIGH_FFP_TEXUNITS
#include "shaders/ffp_ext_f.h"
#include "shaders/ffp_ext_v.h"
#else
#include "shaders/ffp_f.h"
#include "shaders/ffp_v.h"
#endif
#include "shaders/texture_combiners/add.h"
#include "shaders/texture_combiners/blend.h"
#include "shaders/texture_combiners/decal.h"
#include "shaders/texture_combiners/modulate.h"
#include "shaders/texture_combiners/replace.h"
#ifndef DISABLE_TEXTURE_COMBINER
#include "shaders/texture_combiners/combine.h"
#endif
#include "shared.h"

#define setup_lighting_attributes(type, type2, attr) \
	if (mask.has_colors && color_material_state && (color_material_mode == type || color_material_mode == type2)) { \
		vgl_fast_memcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[FFP_ATTRIB_COLOR], sizeof(SceGxmVertexAttribute)); \
		ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params; \
		ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[attr]; \
		ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride; \
	} else { \
		ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params; \
		ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[attr]; \
		ffp_vertex_attribute[ffp_vertex_num_params].format = SCE_GXM_ATTRIBUTE_FORMAT_F32; \
		ffp_vertex_attribute[ffp_vertex_num_params].offset = 0; \
		ffp_vertex_attribute[ffp_vertex_num_params].componentCount = 4; \
		ffp_vertex_stream[ffp_vertex_num_params].stride = 0; \
	} \
	ffp_vertex_stream[ffp_vertex_num_params++].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

#define SHADER_CACHE_SIZE 256

#ifdef HAVE_WVP_ON_GPU
#define WVP_ON_GPU 1
#else
#define WVP_ON_GPU 0
#endif

#define set_interleaved_comp(fmt, size, _stride, offs, attrib) \
	ffp_vertex_attrib_offsets[attrib] = (uint32_t)pointer + offs; \
	ffp_vertex_attrib_vbo[attrib] = vertex_array_unit; \
	attributes = &ffp_vertex_attrib_config[attrib]; \
	streams = &ffp_vertex_stream_config[attrib]; \
	attributes->format = fmt; \
	attributes->componentCount = size; \
	streams->stride = stride ? stride : _stride; \
	ffp_dirty_vert_attr |= (1 << attrib);

uint8_t ffp_texcoord_binds[3] = {FFP_ATTRIB_TEX0, FFP_ATTRIB_TEX1, FFP_ATTRIB_TEX2};

// Internal stuffs
static uint32_t vertex_count = 0; // Vertex counter for vertex list
static SceGxmPrimitiveType prim; // Current in use primitive for rendering

#ifdef HAVE_UNPURE_TEXCOORDS
static uint8_t base_texture_id = 0; // First enabled texture to use during draws
#else
#define base_texture_id (0)
#endif

// Lighting
GLboolean lighting_state = GL_FALSE; // Current lighting processor state
GLboolean lights_aligned; // Are clip planes in a contiguous range
uint8_t light_range[2]; // The highest and lowest enabled lights
uint8_t light_mask = 0; // Bitmask of enabled lights
vector4f lights_ambients[MAX_LIGHTS_NUM];
vector4f lights_diffuses[MAX_LIGHTS_NUM];
vector4f lights_speculars[MAX_LIGHTS_NUM];
vector4f lights_positions[MAX_LIGHTS_NUM];
vector3f lights_attenuations[MAX_LIGHTS_NUM];
vector4f light_global_ambient = {0.2f, 0.2f, 0.2f, 1.0f};
shad_mode shading_mode = SMOOTH;
GLboolean normalize = GL_FALSE;
float current_shininess = 0.0f; // Current GL_SHININESS value (FIXME: This should be a vertex stream for immediate mode)

// Fogging
GLboolean fogging = GL_FALSE; // Current fogging processor state
GLint fog_mode = GL_EXP; // Current fogging mode (openGL)
fog_type internal_fog_mode = DISABLED; // Current fogging mode (sceGxm)
GLfloat fog_density = 1.0f; // Current fogging density
GLfloat fog_near = 0.0f; // Current fogging near distance
GLfloat fog_far = 1.0f; // Current fogging far distance
GLfloat fog_range = 1.0f; // Current fogging range (fog far - fog near)
vector4f fog_color = {0.0f, 0.0f, 0.0f, 0.0f}; // Current fogging color

// Clipping Planes
GLboolean clip_planes_aligned = GL_TRUE; // Are clip planes in a contiguous range?
uint8_t clip_plane_range[2] = {0}; // The hightest enabled clip plane
uint8_t clip_planes_mask = 0; // Bitmask of enabled clip planes
vector4f clip_planes_eq[MAX_CLIP_PLANES_NUM]; // Current equation for user clip planes

// Miscellaneous
gl_phase phase = NONE; // Current drawing phase for legacy openGL
int legacy_pool_size = 0; // Mempool size for GL1 immediate draw pipeline
int8_t client_texture_unit = 0; // Current in use client side texture unit
GLboolean srgb_mode = GL_FALSE; // SRGB mode for color output

legacy_vtx_attachment current_vtx = {
	.uv = {0.0f, 0.0f},
	.clr = {1.0f, 1.0f, 1.0f, 1.0f},
	.amb = {0.2f, 0.2f, 0.2f, 1.0f},
	.diff = {0.8f, 0.8f, 0.8f, 1.0f},
	.spec = {0.0f, 0.0f, 0.0f, 1.0f},
	.emiss = {0.0f, 0.0f, 0.0f, 1.0f},
	.nor = {0.0f, 0.0f, 1.0f},
	.uv2 = {0.0f, 0.0f}
};

static float *lighting_attr_ptr[FFP_COEFF_NUM] = {
	&current_vtx.amb.x,
	&current_vtx.diff.x,
	&current_vtx.spec.x,
	&current_vtx.emiss.x
};

// Non-Immediate Mode
SceGxmVertexAttribute ffp_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream ffp_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];

// Immediate Mode with Texturing
SceGxmVertexAttribute legacy_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM - 1];
SceGxmVertexStream legacy_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM - 1];

// Immediate Mode with Multitexturing
SceGxmVertexAttribute legacy_mt_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream legacy_mt_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];

// Immediate Mode without Texturing
SceGxmVertexAttribute legacy_nt_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM - 2];
SceGxmVertexStream legacy_nt_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM - 2];

static uint32_t ffp_vertex_attrib_offsets[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint32_t ffp_vertex_attrib_vbo[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
static GLenum ffp_mode;
uint16_t ffp_vertex_attrib_state = 0;
uint8_t ffp_vertex_attrib_fixed_mask = 0;
uint8_t ffp_vertex_attrib_fixed_pos_mask = 0;

#ifdef HAVE_HIGH_FFP_TEXUNITS
typedef union {
	struct {
		uint64_t alpha_test_mode : 3; // Frag
		uint64_t num_textures : 2; // Frag/Vert
		uint64_t has_colors : 1; // Frag/Vert
		uint64_t fog_mode : 2; // Frag
		uint64_t clip_planes_num : 3; // Vert
		uint64_t lights_num : 4; // Frag/Vert
		uint64_t tex_env_mode_pass0 : 3; // Frag
		uint64_t tex_env_mode_pass1 : 3; // Frag
		uint64_t shading_mode : 1; // Frag/Vert
		uint64_t normalize : 1; // Vert
		uint64_t tex_env_mode_pass2 : 3; // Frag
		uint64_t fixed_mask : 4; // Vert
		uint64_t pos_fixed_mask : 2; // Vert
		uint64_t point_sprite : 1; // Frag
		uint64_t fast_perspective_correction : 1; // Frag/Vert
		uint64_t srgb_mode : 1; // Frag
		uint64_t UNUSED : 29;
	};
	uint64_t raw;
} shader_mask;
#define VERTEX_SHADER_MASK   (0b0000000000000000000000000000001011111100011000000111111100111000)
#define FRAGMENT_SHADER_MASK (0b0000000000000000000000000000011100000011101111111111100011111111)
#else
typedef union {
	struct {
		uint32_t alpha_test_mode : 3; // Frag
		uint32_t num_textures : 2; // Frag/Vert
		uint32_t has_colors : 1; // Frag/Vert
		uint32_t fog_mode : 2; // Frag
		uint32_t clip_planes_num : 3; // Vert
		uint32_t lights_num : 4; // Frag/Vert
		uint32_t tex_env_mode_pass0 : 3; // Frag
		uint32_t tex_env_mode_pass1 : 3; // Frag
		uint32_t shading_mode : 1; // Frag/Vert
		uint32_t normalize : 1; // Vert
		uint32_t fixed_mask : 3; // Vert
		uint32_t pos_fixed_mask : 2; // Vert
		uint32_t point_sprite : 1; // Frag
		uint32_t fast_perspective_correction : 1; // Frag/Vert
		uint32_t srgb_mode : 1; // Frag
	};
	uint32_t raw;
} shader_mask;
#define VERTEX_SHADER_MASK   (0b0101111111000000111111100111000)
#define FRAGMENT_SHADER_MASK (0b1110000001111111111100011111111)
#endif
#ifndef DISABLE_TEXTURE_COMBINER
typedef union {
	struct {
		combiner_state pass0;
		combiner_state pass1;
#ifdef HAVE_HIGH_FFP_TEXUNITS
		combiner_state pass2;
#endif
	};
#ifdef HAVE_HIGH_FFP_TEXUNITS
	struct {
		uint64_t raw_high;
		uint32_t raw_low;
	};
#else
	uint64_t raw;
#endif
} combiner_mask;
#endif

typedef struct {
	SceGxmProgram *prog;
	uint32_t unif_buf_size;
	uint8_t *unif_buf;
	int frag_unifs[FRAGMENT_UNIFORMS_NUM];
	SceGxmShaderPatcherId id;
	shader_mask mask;
#ifndef DISABLE_TEXTURE_COMBINER
	combiner_mask cmb_mask;
#endif
} cached_fragment_shader;
typedef struct {
	SceGxmProgram *prog;
	uint32_t unif_buf_size;
	uint8_t *unif_buf;
	int vert_unifs[VERTEX_UNIFORMS_NUM];
	int attributes[FFP_ATTRIBS_NUM];
	SceGxmShaderPatcherId id;
	shader_mask mask;
} cached_vertex_shader;
cached_fragment_shader frag_shader_cache[SHADER_CACHE_SIZE];
cached_vertex_shader vert_shader_cache[SHADER_CACHE_SIZE];
uint8_t frag_shader_cache_size = 0;
uint8_t vert_shader_cache_size = 0;
int frag_shader_cache_idx = -1;
int vert_shader_cache_idx = -1;
uint8_t ffp_vertex_num_params = 1;
uint32_t ffp_vertex_unif_buf_size;
uint8_t *ffp_vertex_unif_buf;
uint32_t ffp_fragment_unif_buf_size;
uint8_t *ffp_fragment_unif_buf;
int *ffp_vertex_attribs;
int *ffp_vertex_params;
int *ffp_fragment_params;
SceGxmShaderPatcherId ffp_vertex_program_id;
SceGxmShaderPatcherId ffp_fragment_program_id;
SceGxmProgram *ffp_fragment_program = NULL;
SceGxmProgram *ffp_vertex_program = NULL;
SceGxmVertexProgram *ffp_vertex_program_patched; // Patched vertex program for the fixed function pipeline implementation
SceGxmFragmentProgram *ffp_fragment_program_patched; // Patched fragment program for the fixed function pipeline implementation
GLboolean ffp_dirty_frag = GL_TRUE;
GLboolean ffp_dirty_vert = GL_TRUE;
uint16_t ffp_dirty_vert_attr = 0xFFFF;
uint16_t dirty_vert_unifs = 0xFFFF;
uint32_t dirty_frag_unifs = 0xFFFFFFFF;
blend_config ffp_blend_info;
shader_mask ffp_mask = {.raw = 0xFFFFFFFF};
GLenum color_material_mode = GL_AMBIENT_AND_DIFFUSE;
GLboolean color_material_state = GL_FALSE;
#ifndef DISABLE_TEXTURE_COMBINER
#ifdef HAVE_HIGH_FFP_TEXUNITS
combiner_mask ffp_combiner_mask = {.raw_high = 0, .raw_low = 0};
#else
combiner_mask ffp_combiner_mask = {.raw = 0};
#endif
#endif

SceGxmVertexAttribute ffp_vertex_attribute[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream ffp_vertex_stream[FFP_VERTEX_ATTRIBS_NUM];
SceGxmVertexStream *ffp_lighting_streams = NULL;

void adjust_color_material_state() {
	if (color_material_state) {
		if (color_material_mode == GL_AMBIENT || color_material_mode == GL_AMBIENT_AND_DIFFUSE)
			lighting_attr_ptr[FFP_AMBIENT_COEFF] = &current_vtx.clr.x;
		else
			lighting_attr_ptr[FFP_AMBIENT_COEFF] = &current_vtx.amb.x;
		if (color_material_mode == GL_DIFFUSE || color_material_mode == GL_AMBIENT_AND_DIFFUSE)
			lighting_attr_ptr[FFP_DIFFUSE_COEFF] = &current_vtx.clr.x;
		else
			lighting_attr_ptr[FFP_DIFFUSE_COEFF] = &current_vtx.diff.x;
		if (color_material_mode == GL_SPECULAR)
			lighting_attr_ptr[FFP_SPECULAR_COEFF] = &current_vtx.clr.x;
		else
			lighting_attr_ptr[FFP_SPECULAR_COEFF] = &current_vtx.spec.x;
		if (color_material_mode == GL_EMISSION)
			lighting_attr_ptr[FFP_EMISSION_COEFF] = &current_vtx.clr.x;
		else
			lighting_attr_ptr[FFP_EMISSION_COEFF] = &current_vtx.emiss.x;
	} else {
		lighting_attr_ptr[FFP_AMBIENT_COEFF] = &current_vtx.amb.x;
		lighting_attr_ptr[FFP_DIFFUSE_COEFF] = &current_vtx.diff.x;
		lighting_attr_ptr[FFP_SPECULAR_COEFF] = &current_vtx.spec.x;
		lighting_attr_ptr[FFP_EMISSION_COEFF] = &current_vtx.emiss.x;
	}
}

void reload_vertex_uniforms_and_attributes(int *vertex_params, int *vertex_attributes) {
	sceClibMemset(vertex_params, -1, VERTEX_UNIFORMS_NUM * sizeof(int));
	int cnt = sceGxmProgramGetParameterCount(ffp_vertex_program);
	uint32_t *ptr = vglProgramGetParameterBase(ffp_vertex_program);
	for (int i = 0; i < cnt; i++) {
		SceGxmProgramParameter *p = (SceGxmProgramParameter *)ptr;
		SceGxmParameterCategory cat = sceGxmProgramParameterGetCategory(p);
		switch (cat) {
		case SCE_GXM_PARAMETER_CATEGORY_UNIFORM:
			vertex_params[sceGxmProgramParameterGetName(p)[0] - 'A'] = sceGxmProgramParameterGetResourceIndex(p) * 4;
			break;
		case SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE:
			vertex_attributes[sceGxmProgramParameterGetName(p)[0] - 'N'] = sceGxmProgramParameterGetResourceIndex(p);
			break;
		default:
			break;
		}
		ptr += 4;
	}
	ffp_vertex_params = vertex_params;
	ffp_vertex_attribs = vertex_attributes;
}

void reload_fragment_uniforms(int *fragment_params) {
	sceClibMemset(fragment_params, -1, FRAGMENT_UNIFORMS_NUM * sizeof(int));
	int cnt = sceGxmProgramGetParameterCount(ffp_fragment_program);
	uint32_t *ptr = vglProgramGetParameterBase(ffp_fragment_program);
	for (int i = 0; i < cnt; i++) {
		SceGxmProgramParameter *p = (SceGxmProgramParameter *)ptr;
		if (sceGxmProgramParameterGetCategory(p) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM) {
			fragment_params[sceGxmProgramParameterGetName(p)[0] - 'A'] = sceGxmProgramParameterGetResourceIndex(p) * 4;
		}
		ptr += 4;
	}
	ffp_fragment_params = fragment_params;
}

#ifndef DISABLE_TEXTURE_COMBINER
void setup_combiner_pass(int i, char *dst) {
	char tmp[2048];
	char arg0_rgb[32], arg1_rgb[32], arg2_rgb[32];
	char arg0_a[32], arg1_a[32], arg2_a[32];
	char *args[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	int extra_args_count;
	texture_unit *tex_unit = &texture_units[base_texture_id + i];

	// Note: arg0_rgb is implicit cause it's always used
	// Note: We append arg0_a at the end of RGB pass since always used
	if (tex_unit->combiner.rgb_func == INTERPOLATE) { // Arg0, Arg1, Arg2
		sprintf(arg2_rgb, op_modes[tex_unit->combiner.op_mode_rgb_2], operands[tex_unit->combiner.op_rgb_2]);
		args[0] = arg2_rgb;
		args[1] = arg1_rgb;
		args[2] = arg2_rgb;
		args[3] = arg0_a;
		extra_args_count = 4;
	}
	if (tex_unit->combiner.rgb_func != REPLACE) { // Arg0, Arg1
		sprintf(arg1_rgb, op_modes[tex_unit->combiner.op_mode_rgb_1], operands[tex_unit->combiner.op_rgb_1]);
		if (tex_unit->combiner.rgb_func != INTERPOLATE) {
			args[0] = arg1_rgb;
			args[1] = arg0_a;
			extra_args_count = 2;
		}
	} else { // Arg0
		args[0] = arg0_a;
		extra_args_count = 1;
	}
	if (tex_unit->combiner.a_func == INTERPOLATE) { // Arg0, Arg1, Arg2
		sprintf(arg2_a, op_modes[tex_unit->combiner.op_mode_a_2], operands[tex_unit->combiner.op_a_2]);
		args[extra_args_count++] = arg2_a;
		args[extra_args_count++] = arg1_a;
		args[extra_args_count++] = arg2_a;
	}
	if (tex_unit->combiner.a_func != REPLACE) { // Arg0, Arg1
		sprintf(arg1_a, op_modes[tex_unit->combiner.op_mode_a_1], operands[tex_unit->combiner.op_a_1]);
		if (tex_unit->combiner.a_func != INTERPOLATE) {
			args[extra_args_count++] = arg1_a;
		}
	}
	// Common arguments
	sprintf(arg0_rgb, op_modes[tex_unit->combiner.op_mode_rgb_0], operands[tex_unit->combiner.op_rgb_0]);
	sprintf(arg0_a, op_modes[tex_unit->combiner.op_mode_a_0], operands[tex_unit->combiner.op_a_0]);

	sprintf(tmp, combine_src, i, calc_funcs[tex_unit->combiner.rgb_func], 'O' + i, i, calc_funcs[tex_unit->combiner.a_func], 'O' + i, i);
	switch (extra_args_count) {
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

uint8_t reload_ffp_shaders(SceGxmVertexAttribute *attrs, SceGxmVertexStream *streams, SceGxmIndexSource index_type) {
#ifdef HAVE_PROFILING
	uint32_t reload_ffp_shaders_start = sceKernelGetProcessTimeLow();
#endif
	// Checking if mask changed
	GLboolean ffp_dirty_frag_blend = ffp_blend_info.raw != blend_info.raw;
	shader_mask mask = {.raw = 0};
#ifndef DISABLE_TEXTURE_COMBINER
#ifdef HAVE_HIGH_FFP_TEXUNITS
	combiner_mask cmb_mask = {.raw_low = 0, .raw_high = 0};
#else
	combiner_mask cmb_mask = {.raw = 0};
#endif
#endif
	mask.alpha_test_mode = alpha_op;
	mask.has_colors = (ffp_vertex_attrib_state & (1 << FFP_ATTRIB_COLOR)) ? GL_TRUE : GL_FALSE;
	mask.fog_mode = internal_fog_mode;
	mask.shading_mode = shading_mode;
	mask.point_sprite = point_sprite_state;
	mask.normalize = normalize;
	mask.fixed_mask = ffp_vertex_attrib_fixed_mask;
	mask.pos_fixed_mask = ffp_vertex_attrib_fixed_pos_mask;
	mask.fast_perspective_correction = fast_perspective_correction_hint;
	mask.srgb_mode = srgb_mode;
	uint16_t draw_mask_state = ffp_vertex_attrib_state;

	// Counting number of enabled texture units
	mask.num_textures = 0;
#ifdef HAVE_UNPURE_TEXCOORDS
	base_texture_id = 0;
#endif
#ifdef DISABLE_FFP_MULTITEXTURE
	for (int i = 0; i < 1; i++) {
#else
	for (int i = 0; i < TEXTURE_COORDS_NUM; i++) {
#endif
		if (texture_units[i].state && (ffp_vertex_attrib_state & (1 << FFP_ATTRIB_TEX(i)))) {
#ifdef HAVE_UNPURE_TEXCOORDS
			if (i != mask.num_textures) {
				if (base_texture_id == 0)
					base_texture_id = i;
				draw_mask_state &= ~(1 << FFP_ATTRIB_TEX(i));
				draw_mask_state |= (1 << FFP_ATTRIB_TEX(i - base_texture_id));
			}
#endif
			mask.num_textures++;
			switch (i - base_texture_id) {
			case 0:
				mask.tex_env_mode_pass0 = texture_units[base_texture_id].env_mode;
#ifndef DISABLE_TEXTURE_COMBINER
				if (mask.tex_env_mode_pass0 == COMBINE)
					cmb_mask.pass0.raw = texture_units[base_texture_id].combiner.raw;
#endif
				break;
			case 1:
				mask.tex_env_mode_pass1 = texture_units[base_texture_id + 1].env_mode;
#ifndef DISABLE_TEXTURE_COMBINER
				if (mask.tex_env_mode_pass1 == COMBINE)
					cmb_mask.pass1.raw = texture_units[base_texture_id + 1].combiner.raw;
#endif
				break;
#ifdef HAVE_HIGH_FFP_TEXUNITS
			case 2:
				mask.tex_env_mode_pass2 = texture_units[base_texture_id + 2].env_mode;
#ifndef DISABLE_TEXTURE_COMBINER
				if (mask.tex_env_mode_pass2 == COMBINE)
					cmb_mask.pass2.raw = texture_units[base_texture_id + 2].combiner.raw;
#endif
				break;
#endif
			default:
				break;
			}
		} else {
			draw_mask_state &= ~(1 << FFP_ATTRIB_TEX(i));
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
				vgl_fast_memcpy(&clip_planes[mask.clip_planes_num], &clip_planes_eq[i], sizeof(vector4f));
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
		// Force enabling lights related streams
		if (mask.lights_num > 0) {
			draw_mask_state |= (1 << FFP_ATTRIB_COLOR);
			draw_mask_state |= (1 << FFP_ATTRIB_DIFFUSE);
			draw_mask_state |= (1 << FFP_ATTRIB_SPECULAR);
			draw_mask_state |= (1 << FFP_ATTRIB_EMISSION);
			draw_mask_state |= (1 << FFP_ATTRIB_NORMAL);
		}
	}
	
	// Force disabling lights state
	if (mask.lights_num == 0) {
		draw_mask_state &= ~(1 << FFP_ATTRIB_DIFFUSE);
		draw_mask_state &= ~(1 << FFP_ATTRIB_SPECULAR);
		draw_mask_state &= ~(1 << FFP_ATTRIB_EMISSION);
		draw_mask_state &= ~(1 << FFP_ATTRIB_NORMAL);
	}
	
	uint32_t vert_shader_mask = mask.raw & VERTEX_SHADER_MASK;
	uint32_t frag_shader_mask = mask.raw & FRAGMENT_SHADER_MASK;
	
#ifdef DISABLE_TEXTURE_COMBINER
	#define is_ffp_mask_matching(src_mask, dst_mask) \
		((src_mask) == (dst_mask))
	#define is_cached_mask_matching(src_mask, dst_mask, src_cmb_mask, dst_cmb_mask) \
		((src_mask) == (dst_mask))	
#elif defined(HAVE_HIGH_FFP_TEXUNITS)
	GLboolean is_combiner_matching = ffp_combiner_mask.raw_high == cmb_mask.raw_high && ffp_combiner_mask.raw_low == cmb_mask.raw_low;
	#define is_ffp_mask_matching(src_mask, dst_mask) \
		(is_combiner_matching && (src_mask) == (dst_mask))
	#define is_cached_mask_matching(src_mask, dst_mask, src_cmb_mask, dst_cmb_mask) \
		((src_mask) == (dst_mask) && src_cmb_mask.raw_high == dst_cmb_mask.raw_high && src_cmb_mask.raw_low == dst_cmb_mask.raw_low)
#else
	GLboolean is_combiner_matching = ffp_combiner_mask.raw == cmb_mask.raw;
	#define is_ffp_mask_matching(src_mask, dst_mask) \
		(is_combiner_matching && (src_mask) == (dst_mask))
	#define is_cached_mask_matching(src_mask, dst_mask, src_cmb_mask, dst_cmb_mask) \
		((src_mask) == (dst_mask) && src_cmb_mask.raw == dst_cmb_mask.raw)
#endif

	if (is_ffp_mask_matching(ffp_mask.raw, mask.raw)) { // Fixed function pipeline config didn't change
		ffp_dirty_vert = GL_FALSE;
		ffp_dirty_frag = GL_FALSE;
	} else {
		ffp_dirty_frag_blend = GL_TRUE; // We need to relink fragment with vertex shader if mask changed

		if ((ffp_mask.raw & VERTEX_SHADER_MASK) == vert_shader_mask) {
			ffp_dirty_vert = GL_FALSE;
		} else {
			for (int i = 0; i < vert_shader_cache_size; i++) {
				if (vert_shader_cache[i].mask.raw == vert_shader_mask) {
					ffp_vertex_program = vert_shader_cache[i].prog;
					ffp_vertex_program_id = vert_shader_cache[i].id;
					ffp_vertex_unif_buf_size = vert_shader_cache[i].unif_buf_size;
					ffp_vertex_unif_buf = vert_shader_cache[i].unif_buf;
					ffp_vertex_params = vert_shader_cache[i].vert_unifs;
					ffp_vertex_attribs = vert_shader_cache[i].attributes;
					ffp_dirty_vert_attr = 0xFFFF;
					ffp_dirty_vert = GL_FALSE;
					break;
				}
			}
			dirty_vert_unifs = 0xFFFF;
		}
		if (is_ffp_mask_matching(ffp_mask.raw & FRAGMENT_SHADER_MASK, frag_shader_mask)) {
			ffp_dirty_frag = GL_FALSE;
		} else {
			for (int i = 0; i < frag_shader_cache_size; i++) {
				if (is_cached_mask_matching(frag_shader_cache[i].mask.raw, frag_shader_mask, frag_shader_cache[i].cmb_mask, cmb_mask)) {
					ffp_fragment_program = frag_shader_cache[i].prog;
					ffp_fragment_program_id = frag_shader_cache[i].id;
					ffp_fragment_unif_buf_size = frag_shader_cache[i].unif_buf_size;
					ffp_fragment_unif_buf = frag_shader_cache[i].unif_buf;
					ffp_fragment_params = frag_shader_cache[i].frag_unifs;
					ffp_dirty_frag = GL_FALSE;
					break;
				}
			}
			dirty_frag_unifs = 0xFFFFFFFF;
		}

		ffp_mask.raw = mask.raw;
#ifndef DISABLE_TEXTURE_COMBINER
#ifdef HAVE_HIGH_FFP_TEXUNITS
		ffp_combiner_mask.raw_high = cmb_mask.raw_high;
		ffp_combiner_mask.raw_low = cmb_mask.raw_low;
#else
		ffp_combiner_mask.raw = cmb_mask.raw;
#endif
#endif
	}

	// Checking if vertex shader requires a recompilation
	if (ffp_dirty_vert) {
		char fname[256];
#ifdef HAVE_HIGH_FFP_TEXUNITS
		sprintf(fname, "ux0:data/shader_cache/v%d/v/%016llX-%d.gxp", SHADER_CACHE_MAGIC, vert_shader_mask, WVP_ON_GPU);
#else
		sprintf(fname, "ux0:data/shader_cache/v%d/v/%08X-%d.gxp", SHADER_CACHE_MAGIC, vert_shader_mask, WVP_ON_GPU);
#endif
		SceUID f = sceIoOpen(fname, SCE_O_RDONLY, 0777);
		if (f >= 0) {
			// Gathering the precompiled shader from cache
			uint32_t size = sceIoLseek(f, 0, SCE_SEEK_END);
			sceIoLseek(f, 0, SCE_SEEK_SET);
			ffp_vertex_program = (SceGxmProgram *)vglMalloc(size);
			sceIoRead(f, ffp_vertex_program, size);
			sceIoClose(f);
		} else {
			// Restarting vitaShaRK if we released it before
			if (!is_shark_online)
				start_shader_compiler();

			// Compiling the new shader
			char vshader[8192];
			sprintf(vshader, ffp_vert_src, mask.clip_planes_num, mask.num_textures, mask.has_colors, mask.lights_num, mask.shading_mode, mask.normalize, mask.fixed_mask, mask.pos_fixed_mask, WVP_ON_GPU, mask.fast_perspective_correction);
			uint32_t size = strlen(vshader);
			SceGxmProgram *t = shark_compile_shader_extended(vshader, &size, SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
#ifdef DUMP_SHADER_SOURCES
			if (t) {
#endif
				ffp_vertex_program = (SceGxmProgram *)vglMalloc(size);
				vgl_fast_memcpy((void *)ffp_vertex_program, (void *)t, size);
				shark_clear_output();
				
				// Saving compiled shader in filesystem cache
				f = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_TRUNC | SCE_O_CREAT, 0777);
				sceIoWrite(f, ffp_vertex_program, size);
				sceIoClose(f);
#ifdef DUMP_SHADER_SOURCES
			}
#ifdef HAVE_HIGH_FFP_TEXUNITS
			sprintf(fname, "ux0:data/shader_cache/v%d/v/%016llX-%d.cg", SHADER_CACHE_MAGIC, vert_shader_mask, WVP_ON_GPU);
#else
			sprintf(fname, "ux0:data/shader_cache/v%d/v/%08X-%d.cg", SHADER_CACHE_MAGIC, vert_shader_mask, WVP_ON_GPU);
#endif
			// Saving shader source in filesystem cache
			f = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
			sceIoWrite(f, vshader, strlen(vshader));
			sceIoClose(f);
#endif
		}
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_vertex_program, &ffp_vertex_program_id);
		ffp_vertex_unif_buf_size = sceGxmProgramGetDefaultUniformBufferSize(ffp_vertex_program);
		ffp_vertex_unif_buf = vglMalloc(ffp_vertex_unif_buf_size);

		// Adding new shader to RAM cache
		vert_shader_cache_idx = (vert_shader_cache_idx + 1) % SHADER_CACHE_SIZE;
		if (vert_shader_cache_size < SHADER_CACHE_SIZE) {
			vert_shader_cache_size++;
		} else {
			sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, vert_shader_cache[vert_shader_cache_idx].id);
			vgl_free(vert_shader_cache[vert_shader_cache_idx].prog);
			vgl_free(vert_shader_cache[vert_shader_cache_idx].unif_buf);
		}
		vert_shader_cache[vert_shader_cache_idx].mask.raw = vert_shader_mask;
		vert_shader_cache[vert_shader_cache_idx].prog = ffp_vertex_program;
		vert_shader_cache[vert_shader_cache_idx].id = ffp_vertex_program_id;
		vert_shader_cache[vert_shader_cache_idx].unif_buf_size = ffp_vertex_unif_buf_size;
		vert_shader_cache[vert_shader_cache_idx].unif_buf = ffp_vertex_unif_buf;

		// Reload existing uniform references
		reload_vertex_uniforms_and_attributes(vert_shader_cache[vert_shader_cache_idx].vert_unifs, vert_shader_cache[vert_shader_cache_idx].attributes);

		// Clearing dirty flags
		ffp_dirty_vert = GL_FALSE;
		ffp_dirty_vert_attr = 0xFFFF;
	}

	if (ffp_dirty_vert_attr) {
		// Not going for the vertex config setup if we have aligned datas
		if (!attrs && mask.num_textures == 1 && mask.lights_num == 0) {
			attrs = ffp_vertex_attrib_config;
			streams = ffp_vertex_stream_config;
		}
		
		if (attrs && base_texture_id == 0) { // Immediate mode and non-immediate only when #textures == 1 and no lights
			if (ffp_dirty_vert_attr == 0xFFFF) { // Fill register indices only when we have a brand new shader since those are immutable once set up once
				ffp_vertex_num_params = 1;
				
				// Vertex positions
				attrs[0].regIndex = ffp_vertex_attribs[FFP_ATTRIB_POSITION];

				if (mask.num_textures > 0) {
					// Vertex texture coordinates (First Pass)
					attrs[1].regIndex = ffp_vertex_attribs[FFP_ATTRIB_TEX0];
					ffp_vertex_num_params++;
			
					// Vertex texture coordinates (Second Pass)
					if (mask.num_textures > 1) {
						attrs[2].regIndex = ffp_vertex_attribs[FFP_ATTRIB_TEX1];
						ffp_vertex_num_params++;
					}
				}

				// Vertex colors
				if (mask.has_colors) {
					attrs[ffp_vertex_num_params++].regIndex = ffp_vertex_attribs[FFP_ATTRIB_COLOR];
				}

				// Lighting data
				if (mask.lights_num > 0) {
					ffp_lighting_streams = &attrs[ffp_vertex_num_params];
					attrs[ffp_vertex_num_params++].regIndex = ffp_vertex_attribs[FFP_ATTRIB_DIFFUSE];
					attrs[ffp_vertex_num_params++].regIndex = ffp_vertex_attribs[FFP_ATTRIB_SPECULAR];
					attrs[ffp_vertex_num_params++].regIndex = ffp_vertex_attribs[FFP_ATTRIB_EMISSION];
					attrs[ffp_vertex_num_params++].regIndex = ffp_vertex_attribs[FFP_ATTRIB_NORMAL];
				} else {
					ffp_lighting_streams = NULL;
				}
			}
		} else { // Non immediate mode
			ffp_vertex_num_params = 1;
			
			// Vertex positions
			if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_POSITION)) {
				vgl_fast_memcpy(&ffp_vertex_attribute[0], &ffp_vertex_attrib_config[FFP_ATTRIB_POSITION], sizeof(SceGxmVertexAttribute));
				ffp_vertex_attribute[0].streamIndex = 0;
				ffp_vertex_attribute[0].regIndex = ffp_vertex_attribs[FFP_ATTRIB_POSITION];
				ffp_vertex_stream[0].stride = ffp_vertex_stream_config[FFP_ATTRIB_POSITION].stride;
				ffp_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
			}

			// Vertex texture coordinates (First pass)
			if (mask.num_textures > 0) {
				if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_TEX0)) {
					vgl_fast_memcpy(&ffp_vertex_attribute[1], &ffp_vertex_attrib_config[FFP_ATTRIB_TEX(base_texture_id)], sizeof(SceGxmVertexAttribute));
					ffp_vertex_attribute[1].streamIndex = 1;
					ffp_vertex_attribute[1].regIndex = ffp_vertex_attribs[FFP_ATTRIB_TEX0];
					ffp_vertex_stream[1].stride = ffp_vertex_stream_config[FFP_ATTRIB_TEX(base_texture_id)].stride;
					ffp_vertex_stream[1].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
				}
				ffp_vertex_num_params++;
			}

			if (mask.lights_num > 0) {
				ffp_lighting_streams = &ffp_vertex_stream[ffp_vertex_num_params];

				// Lighting equation attributes
				if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_COLOR)) {
					setup_lighting_attributes(GL_AMBIENT, GL_AMBIENT_AND_DIFFUSE, FFP_ATTRIB_COLOR);
					setup_lighting_attributes(GL_DIFFUSE, GL_AMBIENT_AND_DIFFUSE, FFP_ATTRIB_DIFFUSE);
					setup_lighting_attributes(GL_SPECULAR, GL_SPECULAR, FFP_ATTRIB_SPECULAR);
					setup_lighting_attributes(GL_EMISSION, GL_EMISSION, FFP_ATTRIB_EMISSION);
				} else {
					ffp_vertex_num_params += 4;
				}
			
				if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_NORMAL)) {
					if (ffp_vertex_attrib_state & (1 << FFP_ATTRIB_NORMAL)) {
						vgl_fast_memcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[FFP_ATTRIB_NORMAL], sizeof(SceGxmVertexAttribute));
						ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
						ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[FFP_ATTRIB_NORMAL];
						ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
					} else {
						ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
						ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[FFP_ATTRIB_NORMAL];
						ffp_vertex_attribute[ffp_vertex_num_params].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
						ffp_vertex_attribute[ffp_vertex_num_params].offset = 0;
						ffp_vertex_attribute[ffp_vertex_num_params].componentCount = 3;
						ffp_vertex_stream[ffp_vertex_num_params].stride = 0;
					}
					ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
				}
				ffp_vertex_num_params++;
			} else {
				ffp_lighting_streams = NULL;
				if (mask.has_colors) {
					if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_COLOR)) {
						// Vertex colors
						vgl_fast_memcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[FFP_ATTRIB_COLOR], sizeof(SceGxmVertexAttribute));
						ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
						ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[FFP_ATTRIB_COLOR];
						ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
						ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
						ffp_vertex_num_params++;
					}
				}
			}

			// Vertex texture coordinates (Second pass)
			if (mask.num_textures > 1) {
				if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_TEX1)) {
					vgl_fast_memcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[FFP_ATTRIB_TEX(base_texture_id + 1)], sizeof(SceGxmVertexAttribute));
					ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
					ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[FFP_ATTRIB_TEX1];
					ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[FFP_ATTRIB_TEX(base_texture_id + 1)].stride;
					ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
				}
				ffp_vertex_num_params++;
#ifdef HAVE_HIGH_FFP_TEXUNITS
				// Vertex texture coordinates (Third pass)
				if (mask.num_textures > 2) {
					if (ffp_dirty_vert_attr & (1 << FFP_ATTRIB_COLOR)) {
						vgl_fast_memcpy(&ffp_vertex_attribute[ffp_vertex_num_params], &ffp_vertex_attrib_config[FFP_ATTRIB_TEX(base_texture_id + 2)], sizeof(SceGxmVertexAttribute));
						ffp_vertex_attribute[ffp_vertex_num_params].streamIndex = ffp_vertex_num_params;
						ffp_vertex_attribute[ffp_vertex_num_params].regIndex = ffp_vertex_attribs[FFP_ATTRIB_TEX1];
						ffp_vertex_stream[ffp_vertex_num_params].stride = ffp_vertex_stream_config[FFP_ATTRIB_TEX(base_texture_id + 2)].stride;
						ffp_vertex_stream[ffp_vertex_num_params].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
					}
					ffp_vertex_num_params++;
				}
#endif
			}
			streams = ffp_vertex_stream;
			attrs = ffp_vertex_attribute;
		}
		ffp_dirty_vert_attr = 0;
		
		// Creating patched vertex shader
		patch_vertex_program(gxm_shader_patcher, ffp_vertex_program_id, attrs, ffp_vertex_num_params, streams, ffp_vertex_num_params, &ffp_vertex_program_patched);
		
#ifndef INDICES_SPEEDHACK
		for (int i = 0; i < ffp_vertex_num_params; i++) {
			streams[i].indexSource = index_type;
		}
#endif
	}

	// Checking if fragment shader requires a recompilation
	if (ffp_dirty_frag) {
		char fname[256];
#ifndef DISABLE_TEXTURE_COMBINER
#ifdef HAVE_HIGH_FFP_TEXUNITS
		sprintf(fname, "ux0:data/shader_cache/v%d/f/%016llX-%016llX-%08X.cg", SHADER_CACHE_MAGIC, frag_shader_mask, cmb_mask.raw_high, cmb_mask.raw_low);
#else
		sprintf(fname, "ux0:data/shader_cache/v%d/f/%08X-%016llX.gxp", SHADER_CACHE_MAGIC, frag_shader_mask, cmb_mask.raw);
#endif
#else
#ifdef HAVE_HIGH_FFP_TEXUNITS
		sprintf(fname, "ux0:data/shader_cache/v%d/f/%016llX-0000000000000000.gxp", SHADER_CACHE_MAGIC, frag_shader_mask);
#else
		sprintf(fname, "ux0:data/shader_cache/v%d/f/%08X-0000000000000000.gxp", SHADER_CACHE_MAGIC, frag_shader_mask);
#endif
#endif
		SceUID f = sceIoOpen(fname, SCE_O_RDONLY, 0777);
		if (f >= 0) {
			// Gathering the precompiled shader from cache
			uint32_t size = sceIoLseek(f, 0, SCE_SEEK_END);
			sceIoLseek(f, 0, SCE_SEEK_SET);
			ffp_fragment_program = (SceGxmProgram *)vglMalloc(size);
			sceIoRead(f, ffp_fragment_program, size);
			sceIoClose(f);
		} else {
			// Restarting vitaShaRK if we released it before
			if (!is_shark_online)
				start_shader_compiler();

			// Compiling the new shader
			char fshader[8192];
			char texenv_shad[8192] = {0};
			GLboolean unused_mode[5] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
			for (int i = 0; i < mask.num_textures; i++) {
#ifndef DISABLE_TEXTURE_COMBINER
				char tmp[1024];
#endif
				switch (texture_units[base_texture_id + i].env_mode) {
				case MODULATE:
					if (unused_mode[MODULATE]) {
						sprintf(texenv_shad, "%s\n%s", texenv_shad, modulate_src);
						unused_mode[MODULATE] = GL_FALSE;
					}
					break;
				case DECAL:
					if (unused_mode[DECAL]) {
						sprintf(texenv_shad, "%s\n%s", texenv_shad, decal_src);
						unused_mode[DECAL] = GL_FALSE;
					}
					break;
				case BLEND:
					if (unused_mode[BLEND]) {
						sprintf(texenv_shad, "%s\n%s", texenv_shad, blend_src);
						unused_mode[BLEND] = GL_FALSE;
					}
					break;
				case ADD:
					if (unused_mode[ADD]) {
						sprintf(texenv_shad, "%s\n%s", texenv_shad, add_src);
						unused_mode[ADD] = GL_FALSE;
					}
					break;
				case REPLACE:
					if (unused_mode[REPLACE]) {
						sprintf(texenv_shad, "%s\n%s", texenv_shad, replace_src);
						unused_mode[REPLACE] = GL_FALSE;
					}
					break;
#ifndef DISABLE_TEXTURE_COMBINER
				case COMBINE:
					setup_combiner_pass(i, tmp);
					sprintf(texenv_shad, "%s\n%s", texenv_shad, tmp);
					break;
#endif
				default:
					break;
				}
			}
#ifdef HAVE_HIGH_FFP_TEXUNITS
			sprintf(fshader, ffp_frag_src, texenv_shad, alpha_op,
				mask.num_textures, mask.has_colors, mask.fog_mode,
				(mask.tex_env_mode_pass0 != COMBINE) ? mask.tex_env_mode_pass0 : 50,
				(mask.tex_env_mode_pass1 != COMBINE) ? mask.tex_env_mode_pass1 : 51,
				(mask.tex_env_mode_pass2 != COMBINE) ? mask.tex_env_mode_pass2 : 52,
				mask.lights_num, mask.shading_mode, mask.point_sprite, mask.fast_perspective_correction, mask.srgb_mode);
#else
			sprintf(fshader, ffp_frag_src, texenv_shad, alpha_op,
				mask.num_textures, mask.has_colors, mask.fog_mode,
				(mask.tex_env_mode_pass0 != COMBINE) ? mask.tex_env_mode_pass0 : 50,
				(mask.tex_env_mode_pass1 != COMBINE) ? mask.tex_env_mode_pass1 : 51,
				mask.lights_num, mask.shading_mode, mask.point_sprite, mask.fast_perspective_correction, mask.srgb_mode);
#endif
			uint32_t size = strlen(fshader);
			SceGxmProgram *t = shark_compile_shader_extended(fshader, &size, SHARK_FRAGMENT_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
#ifdef DUMP_SHADER_SOURCES
			if (t) {
#endif
			ffp_fragment_program = (SceGxmProgram *)vglMalloc(size);
			vgl_fast_memcpy((void *)ffp_fragment_program, (void *)t, size);
			shark_clear_output();

			// Saving compiled shader in filesystem cache
			f = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
			sceIoWrite(f, ffp_fragment_program, size);
			sceIoClose(f);
#ifdef DUMP_SHADER_SOURCES
			}
#ifndef DISABLE_TEXTURE_COMBINER
#ifdef HAVE_HIGH_FFP_TEXUNITS
			sprintf(fname, "ux0:data/shader_cache/v%d/f/%016llX-%016llX-%08X.cg", SHADER_CACHE_MAGIC, frag_shader_mask, cmb_mask.raw_high, cmb_mask.raw_low);
#else
			sprintf(fname, "ux0:data/shader_cache/v%d/f/%08X-%016llX.cg", SHADER_CACHE_MAGIC, frag_shader_mask, cmb_mask.raw);
#endif
#else
#ifdef HAVE_HIGH_FFP_TEXUNITS
			sprintf(fname, "ux0:data/shader_cache/v%d/f/%016llX-0000000000000000.cg", SHADER_CACHE_MAGIC, frag_shader_mask);
#else
			sprintf(fname, "ux0:data/shader_cache/v%d/f/%08X-0000000000000000.cg", SHADER_CACHE_MAGIC, frag_shader_mask);
#endif
#endif
			// Saving shader source in filesystem cache
			f = sceIoOpen(fname, SCE_O_CREAT | SCE_O_TRUNC | SCE_O_WRONLY, 0777);
			sceIoWrite(f, fshader, strlen(fshader));
			sceIoClose(f);
#endif
		}
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, ffp_fragment_program, &ffp_fragment_program_id);
		ffp_fragment_unif_buf_size = sceGxmProgramGetDefaultUniformBufferSize(ffp_fragment_program);
		if (ffp_fragment_unif_buf_size) {
			ffp_fragment_unif_buf = vglMalloc(ffp_fragment_unif_buf_size);
		} else {
			ffp_fragment_unif_buf = NULL;
		}
		
		// Adding new shader to RAM cache
		frag_shader_cache_idx = (frag_shader_cache_idx + 1) % SHADER_CACHE_SIZE;
		if (frag_shader_cache_size < SHADER_CACHE_SIZE) {
			frag_shader_cache_size++;
		} else {
			sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, frag_shader_cache[frag_shader_cache_idx].id);
			vgl_free(frag_shader_cache[frag_shader_cache_idx].prog);
			if (frag_shader_cache[frag_shader_cache_idx].unif_buf) {
				vgl_free(frag_shader_cache[frag_shader_cache_idx].unif_buf);
			}
		}
		frag_shader_cache[frag_shader_cache_idx].mask.raw = frag_shader_mask;
#ifndef DISABLE_TEXTURE_COMBINER
#ifdef HAVE_HIGH_FFP_TEXUNITS
		frag_shader_cache[frag_shader_cache_idx].cmb_mask.raw_low = cmb_mask.raw_low;
		frag_shader_cache[frag_shader_cache_idx].cmb_mask.raw_high = cmb_mask.raw_high;
#else
		frag_shader_cache[frag_shader_cache_idx].cmb_mask.raw = cmb_mask.raw;
#endif
#endif
		frag_shader_cache[frag_shader_cache_idx].prog = ffp_fragment_program;
		frag_shader_cache[frag_shader_cache_idx].id = ffp_fragment_program_id;
		frag_shader_cache[frag_shader_cache_idx].unif_buf_size = ffp_fragment_unif_buf_size;
		frag_shader_cache[frag_shader_cache_idx].unif_buf = ffp_fragment_unif_buf;

		// Reload existing uniform references
		reload_fragment_uniforms(frag_shader_cache[frag_shader_cache_idx].frag_unifs);

		// Clearing dirty flags
		ffp_dirty_frag = GL_FALSE;
	}

	// Checking if fragment shader requires a blend settings change
	if (ffp_dirty_frag_blend) {
		rebuild_frag_shader(ffp_fragment_program_id, &ffp_fragment_program_patched, ffp_vertex_program, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4);

		// Updating current fixed function pipeline blend config
		ffp_blend_info.raw = blend_info.raw;
	}

	sceGxmSetVertexProgram(gxm_context, ffp_vertex_program_patched);
	sceGxmSetFragmentProgram(gxm_context, ffp_fragment_program_patched);

	// Recalculating MVP matrix if necessary
	if (mvp_modified) {
#ifndef HAVE_WVP_ON_GPU
		matrix4x4_multiply(vgl_mvp_matrix, projection_matrix, modelview_matrix);
#endif
		// Recalculating normal matrix if necessary (TODO: This should be recalculated only when MV changes)
		if (mask.lights_num > 0) {
			recalculate_normal_matrix();
		}

		mvp_modified = GL_FALSE;
	}

	// Uploading fragment shader uniforms
	#define upload_ffp_fragment_unif(x, offs, cnt, size, ptr) \
		if (dirty_frag_unifs & (1 << x)) { \
			vglSetUniformData(ffp_fragment_unif_buf + ffp_fragment_params[x], SCE_GXM_PARAMETER_TYPE_F32, offs, cnt, size, ptr, SCE_GXM_PARAMETER_TYPE_F32); \
		}
	if (dirty_frag_unifs) {
		if (ffp_fragment_unif_buf_size) {
			uint8_t *buffer = vglReserveFragmentUniformBuffer(ffp_fragment_unif_buf_size);
			if (ffp_fragment_params[ALPHA_CUT_UNIF] >= 0) {
				upload_ffp_fragment_unif(ALPHA_CUT_UNIF, 0, 1, 1, &vgl_alpha_ref)
			}
			if (ffp_fragment_params[FOG_COLOR_UNIF] >= 0) {
				upload_ffp_fragment_unif(FOG_COLOR_UNIF, 0, 1, 4, &fog_color.r)
			}
			if (ffp_fragment_params[TEX_ENV_COLOR_UNIF] >= 0) {
				for (int i = 0; i < mask.num_textures; i++) {
					upload_ffp_fragment_unif(TEX_ENV_COLOR_UNIF, i, 1, 4, (const float *)&texture_units[base_texture_id + i].env_color.r)
				}
			}
#ifndef DISABLE_TEXTURE_COMBINER
			if (ffp_fragment_params[SCALE_PASS_0_UNIF] >= 0) {
				upload_ffp_fragment_unif(SCALE_PASS_0_UNIF, 0, 1, 2, &texture_units[base_texture_id].rgb_scale)
			}
			if (ffp_fragment_params[SCALE_PASS_1_UNIF] >= 0) {
				upload_ffp_fragment_unif(SCALE_PASS_1_UNIF, 0, 1, 2, &texture_units[base_texture_id + 1].rgb_scale)
			}
#ifdef HAVE_HIGH_FFP_TEXUNITS
			if (ffp_fragment_params[SCALE_PASS_2_UNIF] >= 0) {
				upload_ffp_fragment_unif(SCALE_PASS_2_UNIF, 0, 1, 2, &texture_units[base_texture_id + 2].rgb_scale)
			}
#endif
#endif
			if (ffp_fragment_params[TINT_COLOR_UNIF] >= 0) {
				upload_ffp_fragment_unif(TINT_COLOR_UNIF, 0, 1, 4, &current_vtx.clr.r)
			}
			if (ffp_fragment_params[FOG_RANGE_UNIF] >= 0) {
				upload_ffp_fragment_unif(FOG_RANGE_UNIF, 0, 1, 1, (const float *)&fog_range)
			}
			if (ffp_fragment_params[FOG_FAR_UNIF] >= 0) {
				upload_ffp_fragment_unif(FOG_FAR_UNIF, 0, 1, 1, (const float *)&fog_far)
			}
			if (ffp_fragment_params[FOG_DENSITY_UNIF] >= 0) {
				upload_ffp_fragment_unif(FOG_DENSITY_UNIF, 0, 1, 1, (const float *)&fog_density)
			}
			if (ffp_fragment_params[LIGHTS_AMBIENTS_F_UNIF] >= 0) {
				upload_ffp_fragment_unif(LIGHT_GLOBAL_AMBIENT_F_UNIF, 0, 1, 4, (const float *)&light_global_ambient.r)
				upload_ffp_fragment_unif(SHININESS_F_UNIF, 0, 1, 1, (const float *)&current_shininess)
				if (lights_aligned) {
					upload_ffp_fragment_unif(LIGHTS_AMBIENTS_F_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][0])
					upload_ffp_fragment_unif(LIGHTS_DIFFUSES_F_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][1])
					upload_ffp_fragment_unif(LIGHTS_SPECULARS_F_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][2])
					upload_ffp_fragment_unif(LIGHTS_POSITIONS_F_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][3])
					upload_ffp_fragment_unif(LIGHTS_ATTENUATIONS_F_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][4])
				} else {
					for (int i = 0; i < mask.lights_num; i++) {
						upload_ffp_fragment_unif(LIGHTS_AMBIENTS_F_UNIF, i, 1, 4, (const float *)light_vars[i][0])
						upload_ffp_fragment_unif(LIGHTS_DIFFUSES_F_UNIF, i, 1, 4, (const float *)light_vars[i][1])
						upload_ffp_fragment_unif(LIGHTS_SPECULARS_F_UNIF, i, 1, 4, (const float *)light_vars[i][2])
						upload_ffp_fragment_unif(LIGHTS_POSITIONS_F_UNIF, i, 1, 4, (const float *)light_vars[i][3])
						upload_ffp_fragment_unif(LIGHTS_ATTENUATIONS_F_UNIF, i, 1, 3, (const float *)light_vars[i][4])
					}
				}
			}
			sceClibMemcpy(buffer, ffp_fragment_unif_buf, ffp_fragment_unif_buf_size);
		}
		dirty_frag_unifs = 0;
	}

	// Uploading vertex shader uniforms
	#define upload_ffp_vertex_unif(x, offs, cnt, size, ptr) \
		if (dirty_vert_unifs & (1 << x)) { \
			vglSetUniformData(ffp_vertex_unif_buf + ffp_vertex_params[x], SCE_GXM_PARAMETER_TYPE_F32, offs, cnt, size, ptr, SCE_GXM_PARAMETER_TYPE_F32); \
		}
	if (dirty_vert_unifs) {
		uint8_t *buffer = vglReserveVertexUniformBuffer(ffp_vertex_unif_buf_size);
		if (ffp_vertex_params[CLIP_PLANES_EQUATION_UNIF] >= 0) {
			upload_ffp_vertex_unif(CLIP_PLANES_EQUATION_UNIF, 0, mask.clip_planes_num, 4, &clip_planes[0].x)
		}
		if (ffp_vertex_params[MODELVIEW_MATRIX_UNIF] >= 0) {
			upload_ffp_vertex_unif(MODELVIEW_MATRIX_UNIF, 0, 4, 4, (const float *)modelview_matrix)
		}
#ifdef HAVE_WVP_ON_GPU
		upload_ffp_vertex_unif(WVP_MATRIX_UNIF, 0, 4, 4, (const float *)projection_matrix)
#else
		upload_ffp_vertex_unif(WVP_MATRIX_UNIF, 0, 4, 4, (const float *)vgl_mvp_matrix)
#endif
		if (ffp_vertex_params[TEX_MATRIX_UNIF] >= 0) {
			upload_ffp_vertex_unif(TEX_MATRIX_UNIF, 0, 4 * mask.num_textures, 4, (const float *)&texture_matrix[base_texture_id])
		}
		upload_ffp_vertex_unif(POINT_SIZE_UNIF, 0, 1, 1, &point_size)
		if (ffp_vertex_params[NORMAL_MATRIX_UNIF] >= 0) {
			upload_ffp_vertex_unif(NORMAL_MATRIX_UNIF, 0, 3, 3, (const float *)normal_matrix)
			if (ffp_vertex_params[LIGHTS_AMBIENTS_V_UNIF] >= 0) {
				upload_ffp_vertex_unif(LIGHT_GLOBAL_AMBIENT_V_UNIF, 0, 1, 4, (const float *)&light_global_ambient.r)
				upload_ffp_vertex_unif(SHININESS_V_UNIF, 0, 1, 1, (const float *)&current_shininess)
				if (lights_aligned) {
					upload_ffp_vertex_unif(LIGHTS_AMBIENTS_V_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][0])
					upload_ffp_vertex_unif(LIGHTS_DIFFUSES_V_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][1])
					upload_ffp_vertex_unif(LIGHTS_SPECULARS_V_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][2])
					upload_ffp_vertex_unif(LIGHTS_POSITIONS_V_UNIF, 0, mask.lights_num, 4, (const float *)light_vars[0][3])
					upload_ffp_vertex_unif(LIGHTS_ATTENUATIONS_V_UNIF, 0, mask.lights_num, 3, (const float *)light_vars[0][4])
				} else {
					for (int i = 0; i < mask.lights_num; i++) {
						upload_ffp_vertex_unif(LIGHTS_AMBIENTS_V_UNIF, i, 1, 4, (const float *)light_vars[i][0])
						upload_ffp_vertex_unif(LIGHTS_DIFFUSES_V_UNIF, i, 1, 4, (const float *)light_vars[i][1])
						upload_ffp_vertex_unif(LIGHTS_SPECULARS_V_UNIF, i, 1, 4, (const float *)light_vars[i][2])
						upload_ffp_vertex_unif(LIGHTS_POSITIONS_V_UNIF, i, 1, 4, (const float *)light_vars[i][3])
						upload_ffp_vertex_unif(LIGHTS_ATTENUATIONS_V_UNIF, i, 1, 3, (const float *)light_vars[i][4])
					}
				}
			}
		}
		sceClibMemcpy(buffer, ffp_vertex_unif_buf, ffp_vertex_unif_buf_size);
		dirty_vert_unifs = 0;
	}
#ifdef HAVE_PROFILING
	ffp_reload_profiler_cnt += sceKernelGetProcessTimeLow() - reload_ffp_shaders_start;
#endif
	return draw_mask_state;
}

void _glDrawArrays_FixedFunctionIMPL(GLint first, GLsizei count) {
	uint8_t mask_state = reload_ffp_shaders(NULL, NULL, SCE_GXM_INDEX_SOURCE_INDEX_16BIT);
#ifdef HAVE_PROFILING
	uint32_t draw_start = sceKernelGetProcessTimeLow();
#endif
	// Uploading textures on relative texture units
	for (int i = 0; i < ffp_mask.num_textures; i++) {
		texture *tex = &texture_slots[texture_units[base_texture_id + i].tex_id[texture_units[base_texture_id + i].state > 1 ? 0 : 1]];
#ifdef HAVE_TEX_CACHE
		restore_tex_cache(tex);
#endif
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = vgl_framecount;
#endif
		sampler *smp = samplers[i];
		if (smp) {
			vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
			vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_TRUE;
		} else if (tex->overridden) {
			vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
			vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_FALSE;
		}
		sceGxmSetFragmentTexture(gxm_context, i, &tex->gxm_tex);
	}
	
	// Preparing materials temp buffer if lights are enabled
	float *materials;
	if (ffp_lighting_streams) {
		materials = (float *)gpu_alloc_mapped_temp(19 * sizeof(float));
	}

	// Uploading vertex streams
	int j = 0;
	for (int i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (mask_state & (1 << i)) {
			int id;
#ifdef HAVE_UNPURE_TEXCOORDS
			if (FFP_ATTRIB_IS_TEX(i) && base_texture_id != 0) {
				switch (i) {
				case FFP_ATTRIB_TEX0:
					id = FFP_ATTRIB_TEX1;
					break;
				case FFP_ATTRIB_TEX1:
					id = FFP_ATTRIB_TEX2;
					break;
				default:
					break;
				}		
			} else
#endif
				id = i;
			void *ptr;
			if (ffp_vertex_attrib_vbo[id]) {
				vbo *gpu_buf = (vbo *)ffp_vertex_attrib_vbo[id];
				gpu_buf->last_frame = vgl_framecount;
				ptr = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[id] + first * ffp_vertex_stream_config[id].stride;
			} else {
				if (ffp_lighting_streams && FFP_ATTRIB_IS_LIGHT(id)) {
					if (ffp_lighting_streams[FFP_ATTRIB_LIGHT_COEFF(id)].stride == 0) { // Color array not mapped to this material attribute
						if (id == FFP_ATTRIB_NORMAL) {
							vgl_fast_memcpy(materials, &current_vtx.nor.x, 3 * sizeof(float));
						} else {
 							vgl_fast_memcpy(materials, lighting_attr_ptr[FFP_ATTRIB_LIGHT_COEFF(id)], 4 * sizeof(float));
						}
						ptr = materials;
						materials += 4;
					} else { // Color array mapped to this attribute (FIXME: This could be optimized by re-using color temp mem)
#ifdef DRAW_SPEEDHACK
						if (id != FFP_ATTRIB_NORMAL) {
							ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] + first * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
						} else {
							ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] + first * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
						}
#else
						if (id != FFP_ATTRIB_NORMAL) {
							uint32_t size = count * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
#ifdef SAFER_DRAW_SPEEDHACK
							if (size > SAFE_DRAW_SIZE_THRESHOLD) {
								ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] + first * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
							} else
#endif
							{
								ptr = gpu_alloc_mapped_temp(size);
								vgl_fast_memcpy(ptr, (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] + first * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride, size);
							}
						} else {
							uint32_t size = count * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
#ifdef SAFER_DRAW_SPEEDHACK
							if (size > SAFE_DRAW_SIZE_THRESHOLD) {
								ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] + first * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
							} else
#endif
							{
								ptr = gpu_alloc_mapped_temp(size);
								vgl_fast_memcpy(ptr, (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] + first * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride, size);
							}
						}
#endif
					}
				} else {
#ifdef DRAW_SPEEDHACK
					ptr = (void *)ffp_vertex_attrib_offsets[id] + first * ffp_vertex_stream_config[id].stride;
#else
					uint32_t size = count * ffp_vertex_stream_config[id].stride;
#ifdef SAFER_DRAW_SPEEDHACK
					if (size > SAFE_DRAW_SIZE_THRESHOLD) {
						ptr = (void *)ffp_vertex_attrib_offsets[id] + first * ffp_vertex_stream_config[id].stride;
					} else
#endif
					{
						ptr = gpu_alloc_mapped_temp(size);
						vgl_fast_memcpy(ptr, (void *)ffp_vertex_attrib_offsets[id] + first * ffp_vertex_stream_config[id].stride, size);
					}
#endif
				}
			}
			sceGxmSetVertexStream(gxm_context, j++, ptr);
		}
	}
#ifdef HAVE_PROFILING
	ffp_draw_profiler_cnt += sceKernelGetProcessTimeLow() - draw_start;
	ffp_draw_cnt++;
#endif
}

void _glMultiDrawArrays_FixedFunctionIMPL(SceGxmPrimitiveType gxm_p, uint16_t *idx_ptr, const GLint *first, const GLsizei *count, GLint lowest, GLsizei highest, GLsizei drawcount) {
	uint8_t mask_state = reload_ffp_shaders(NULL, NULL, SCE_GXM_INDEX_SOURCE_INDEX_16BIT);
#ifdef HAVE_PROFILING
	uint32_t draw_start = sceKernelGetProcessTimeLow();
#endif
	// Uploading textures on relative texture units
	for (int i = 0; i < ffp_mask.num_textures; i++) {
		texture *tex = &texture_slots[texture_units[base_texture_id + i].tex_id[texture_units[base_texture_id + i].state > 1 ? 0 : 1]];
#ifdef HAVE_TEX_CACHE
		restore_tex_cache(tex);
#endif
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = vgl_framecount;
#endif
		sampler *smp = samplers[i];
		if (smp) {
			vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
			vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_TRUE;
		} else if (tex->overridden) {
			vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
			vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_FALSE;
		}
		sceGxmSetFragmentTexture(gxm_context, i, &tex->gxm_tex);
	}
	
	// Preparing materials temp buffer if lights are enabled
	float *materials;
	if (ffp_lighting_streams) {
		materials = (float *)gpu_alloc_mapped_temp(19 * sizeof(float));
	}

	// Uploading vertex streams
	int j = 0;
	void *ptrs[FFP_VERTEX_ATTRIBS_NUM];
	uint32_t strides[FFP_VERTEX_ATTRIBS_NUM];
	for (int i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (mask_state & (1 << i)) {
			int id;
#ifdef HAVE_UNPURE_TEXCOORDS
			if (FFP_ATTRIB_IS_TEX(i) && base_texture_id != 0) {
				switch (i) {
				case FFP_ATTRIB_TEX0:
					id = FFP_ATTRIB_TEX1;
					break;
				case FFP_ATTRIB_TEX1:
					id = FFP_ATTRIB_TEX2;
					break;
				default:
					break;
				}		
			} else
#endif
			{
				id = i;
			}
			if (ffp_vertex_attrib_vbo[id]) {
				vbo *gpu_buf = (vbo *)ffp_vertex_attrib_vbo[id];
				gpu_buf->last_frame = vgl_framecount;
				ptrs[j] = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[i] + lowest * ffp_vertex_stream_config[id].stride;
				strides[j] = ffp_vertex_stream_config[id].stride;
			} else {
				if (ffp_lighting_streams && FFP_ATTRIB_IS_LIGHT(id)) {
					if (ffp_lighting_streams[FFP_ATTRIB_LIGHT_COEFF(id)].stride == 0) { // Color array not mapped to this material attribute
						if (id != FFP_ATTRIB_NORMAL) {
							vgl_fast_memcpy(materials, lighting_attr_ptr[FFP_ATTRIB_LIGHT_COEFF(id)], 4 * sizeof(float));
						} else {
							vgl_fast_memcpy(materials, &current_vtx.nor.x, 3 * sizeof(float));
						}
						ptrs[j] = materials;
						strides[j] = 0;
						materials += 4;
					} else { // Color array mapped to this attribute (FIXME: This could be optimized by re-using color temp mem)
#ifdef DRAW_SPEEDHACK
						if (id != FFP_ATTRIB_NORMAL) {
							ptrs[j] = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] + lowest * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
							strides[j] = ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
						} else {
							ptrs[j] = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] + lowest * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
							strides[j] = ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
						}
#else
						if (id != FFP_ATTRIB_NORMAL) {
							uint32_t size = (highest - lowest) * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
#ifdef SAFER_DRAW_SPEEDHACK
							if (size > SAFE_DRAW_SIZE_THRESHOLD) {
								ptrs[j] = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] + lowest * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
							} else
#endif
							{
								ptrs[j] = gpu_alloc_mapped_temp(size);
								vgl_fast_memcpy(ptrs[j], (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] + lowest * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride, size);
							}
							strides[j] = ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
						} else {
							uint32_t size = (highest - lowest) * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
#ifdef SAFER_DRAW_SPEEDHACK
							if (size > SAFE_DRAW_SIZE_THRESHOLD) {
								ptrs[j] = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] + lowest * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
							} else
#endif
							{
								ptrs[j] = gpu_alloc_mapped_temp(size);
								vgl_fast_memcpy(ptrs[j], (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] + lowest * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride, size);
							}
							strides[j] = ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
						}
#endif
					}
				} else {
					strides[j] = ffp_vertex_stream_config[id].stride;
#ifdef DRAW_SPEEDHACK
					ptrs[j] = (void *)ffp_vertex_attrib_offsets[id] + lowest * ffp_vertex_stream_config[id].stride;
#else
					uint32_t size = (highest - lowest) * ffp_vertex_stream_config[id].stride;
#ifdef SAFER_DRAW_SPEEDHACK
					if (size > SAFE_DRAW_SIZE_THRESHOLD) {
						ptrs[j] = (void *)ffp_vertex_attrib_offsets[id] + lowest * ffp_vertex_stream_config[id].stride;
					} else
#endif
					{
						ptrs[j] = gpu_alloc_mapped_temp(size);
						vgl_fast_memcpy(ptrs[j], (void *)ffp_vertex_attrib_offsets[id] + lowest * ffp_vertex_stream_config[id].stride, size);
					}
#endif
				}
			}
			j++;
		}
	}
	
	for (int i = 0; i < drawcount; i++) {
		for (int z = 0; z < j; z++) {
			sceGxmSetVertexStream(gxm_context, z, ptrs[z] + (first[i] - lowest) * strides[z]);
		}
		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, idx_ptr, count[i]);
	}
#ifdef HAVE_PROFILING
	ffp_draw_profiler_cnt += sceKernelGetProcessTimeLow() - draw_start;
	ffp_draw_cnt++;
#endif
}

void _glDrawElements_FixedFunctionIMPL(uint16_t *idx_buf, GLsizei count, uint32_t top_idx, uint32_t base_idx, SceGxmIndexSource index_type) {
	uint8_t mask_state = reload_ffp_shaders(NULL, NULL, index_type);
#ifdef HAVE_PROFILING
	uint32_t draw_start = sceKernelGetProcessTimeLow();
#endif
	int attr_idxs[FFP_VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
	int attr_num = 0;
#ifndef DRAW_SPEEDHACK
	GLboolean is_full_vbo = GL_TRUE;
#endif
	for (int i = 0; i < FFP_VERTEX_ATTRIBS_NUM; i++) {
		if (mask_state & (1 << i)) {
#ifdef HAVE_UNPURE_TEXCOORDS
			if (FFP_ATTRIB_IS_TEX(i) && base_texture_id != 0) {
				int tex_id;
				switch (i) {
				case FFP_ATTRIB_TEX0:
					tex_id = FFP_ATTRIB_TEX1;
					break;
				case FFP_ATTRIB_TEX1:
					tex_id = FFP_ATTRIB_TEX2;
					break;
				default:
					break;
				}
#ifndef DRAW_SPEEDHACK
				if (!ffp_vertex_attrib_vbo[tex_id])
					is_full_vbo = GL_FALSE;
#endif
				attr_idxs[attr_num++] = tex_id;				
			} else
#endif
			{
#ifndef DRAW_SPEEDHACK
				if (!ffp_vertex_attrib_vbo[i])
					is_full_vbo = GL_FALSE;
#endif
				attr_idxs[attr_num++] = i;
			}
		}
	}

#ifndef DRAW_SPEEDHACK
	// Detecting highest index value
	if (!is_full_vbo && !top_idx) {
		if ((index_type & 1) == 0)
		{
			for (int i = 0; i < count; i++) {
				if (idx_buf[i] > top_idx)
					top_idx = idx_buf[i];
			}
		} else {
			uint32_t *_idx_buf = (uint32_t *)idx_buf;
			for (int i = 0; i < count; i++) {
				if (_idx_buf[i] > top_idx)
					top_idx = _idx_buf[i];
			}
		}
		top_idx += base_idx + 1;
	}
#endif

	// Uploading textures on relative texture units
	for (int i = 0; i < ffp_mask.num_textures; i++) {
		texture *tex = &texture_slots[texture_units[base_texture_id + i].tex_id[texture_units[base_texture_id + i].state > 1 ? 0 : 1]];
#ifdef HAVE_TEX_CACHE
		restore_tex_cache(tex);
#endif
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = vgl_framecount;
#endif
		sampler *smp = samplers[i];
		if (smp) {
			vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
			vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_TRUE;
		} else if (tex->overridden) {
			vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
			vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_FALSE;
		}
		sceGxmSetFragmentTexture(gxm_context, i, &tex->gxm_tex);
	}

	// Preparing materials temp buffer if lights are enabled
	float *materials;
	if (ffp_lighting_streams) {
		materials = (float *)gpu_alloc_mapped_temp(19 * sizeof(float));
	}

	// Uploading vertex streams
	for (int i = 0; i < attr_num; i++) {
		void *ptr;
		int attr_idx = attr_idxs[i];
		if (ffp_vertex_attrib_vbo[attr_idx]) {
			vbo *gpu_buf = (vbo *)ffp_vertex_attrib_vbo[attr_idx];
			gpu_buf->last_frame = vgl_framecount;
			ptr = (uint8_t *)gpu_buf->ptr + ffp_vertex_attrib_offsets[attr_idx];
		} else {
			if (ffp_lighting_streams && FFP_ATTRIB_IS_LIGHT(attr_idx)) {
				if (ffp_lighting_streams[FFP_ATTRIB_LIGHT_COEFF(attr_idx)].stride == 0) { // Color array not mapped to this material attribute
					if (attr_idx != FFP_ATTRIB_NORMAL) {
						vgl_fast_memcpy(materials, lighting_attr_ptr[FFP_ATTRIB_LIGHT_COEFF(attr_idx)], 4 * sizeof(float));
					} else {
						vgl_fast_memcpy(materials, &current_vtx.nor.x, 3 * sizeof(float));
					}
					ptr = materials;
					materials += 4;
				} else { // Color array mapped to this attribute (FIXME: This could be optimized by re-using color temp mem)
#ifdef DRAW_SPEEDHACK
					if (attr_idx != FFP_ATTRIB_NORMAL) {
						ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR];
					} else {
						ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL];
					}
#else
					if (attr_idx != FFP_ATTRIB_NORMAL) {
						uint32_t size = top_idx * ffp_vertex_stream_config[FFP_ATTRIB_COLOR].stride;
#ifdef SAFER_DRAW_SPEEDHACK
						if (size > SAFE_DRAW_SIZE_THRESHOLD) {
							ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR];
						} else
#endif
						{
							ptr = gpu_alloc_mapped_temp(size);
							vgl_fast_memcpy(ptr, (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR], size);
						}
					} else {
						uint32_t size = top_idx * ffp_vertex_stream_config[FFP_ATTRIB_NORMAL].stride;
#ifdef SAFER_DRAW_SPEEDHACK
						if (size > SAFE_DRAW_SIZE_THRESHOLD) {
							ptr = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL];
						} else
#endif
						{
							ptr = gpu_alloc_mapped_temp(size);
							vgl_fast_memcpy(ptr, (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL], size);
						}
					}
#endif
				}
			} else {
#ifdef DRAW_SPEEDHACK
				ptr = (void *)ffp_vertex_attrib_offsets[attr_idx];
#else
				uint32_t size = top_idx * ffp_vertex_stream_config[attr_idx].stride;
#ifdef SAFER_DRAW_SPEEDHACK
				if (size > SAFE_DRAW_SIZE_THRESHOLD) {
					ptr = (void *)ffp_vertex_attrib_offsets[attr_idx];
				} else
#endif
				{
					ptr = gpu_alloc_mapped_temp(size);
					vgl_fast_memcpy(ptr, (void *)ffp_vertex_attrib_offsets[attr_idx], size);
				}
#endif
			}
		}
		sceGxmSetVertexStream(gxm_context, i, ptr);
	}
#ifdef HAVE_PROFILING
	ffp_draw_profiler_cnt += sceKernelGetProcessTimeLow() - draw_start;
	ffp_draw_cnt++;
#endif
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
	} else {
		internal_fog_mode = DISABLED;
	}
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glEnableClientState(GLenum array) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glEnableClientState, DLIST_FUNC_U32, array))
		return;
#endif
	ffp_dirty_vert = GL_TRUE;
	ffp_dirty_frag = GL_TRUE;
	switch (array) {
	case GL_VERTEX_ARRAY:
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_POSITION);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_TEX(client_texture_unit));
		break;
	case GL_COLOR_ARRAY:
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_COLOR);
		break;
	case GL_NORMAL_ARRAY:
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_DIFFUSE);
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_SPECULAR);
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_EMISSION);
		ffp_vertex_attrib_state |= (1 << FFP_ATTRIB_NORMAL);
		ffp_dirty_vert_attr |= (1 << FFP_ATTRIB_NORMAL);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, array)
	}
}

void glDisableClientState(GLenum array) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glEnableClientState, DLIST_FUNC_U32, array))
		return;
#endif
	ffp_dirty_vert = GL_TRUE;
	ffp_dirty_frag = GL_TRUE;
	switch (array) {
	case GL_VERTEX_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_POSITION);
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_TEX(client_texture_unit));
		break;
	case GL_COLOR_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_COLOR);
		break;
	case GL_NORMAL_ARRAY:
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_DIFFUSE);
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_SPECULAR);
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_EMISSION);
		ffp_vertex_attrib_state &= ~(1 << FFP_ATTRIB_NORMAL);
		ffp_dirty_vert_attr |= (1 << FFP_ATTRIB_NORMAL);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, array)
	}
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glVertexPointer, DLIST_FUNC_I32_U32_I32_U32, size, type, stride, pointer))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[FFP_ATTRIB_POSITION] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[FFP_ATTRIB_POSITION] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[FFP_ATTRIB_POSITION];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[FFP_ATTRIB_POSITION];
	ffp_dirty_vert_attr |= (1 << FFP_ATTRIB_POSITION);

	unsigned short bpe;
	switch (type) {
	case GL_FLOAT:
		ffp_vertex_attrib_fixed_pos_mask = 0;
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		ffp_vertex_attrib_fixed_pos_mask = 0;
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16;
		bpe = 2;
		break;
	case GL_FIXED:
		ffp_vertex_attrib_fixed_pos_mask = size - 1;
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColorPointer, DLIST_FUNC_I32_U32_I32_U32, size, type, stride, pointer))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 3) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[FFP_ATTRIB_COLOR] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[FFP_ATTRIB_COLOR];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[FFP_ATTRIB_COLOR];
	ffp_dirty_vert_attr |= (1 << FFP_ATTRIB_COLOR);

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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glNormalPointer(GLenum type, GLsizei stride, const void *pointer) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glNormalPointer, DLIST_FUNC_U32_I32_U32, type, stride, pointer))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (stride < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[FFP_ATTRIB_NORMAL] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[FFP_ATTRIB_NORMAL];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[FFP_ATTRIB_NORMAL];
	ffp_dirty_vert_attr |= (1 << FFP_ATTRIB_NORMAL);

	unsigned short bpe;
	switch (type) {
	case GL_FLOAT:
		ffp_vertex_attrib_fixed_mask &= ~(1 << 0);
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		ffp_vertex_attrib_fixed_mask &= ~(1 << 0);
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;
		bpe = 2;
		break;
	case GL_BYTE:
		ffp_vertex_attrib_fixed_mask &= ~(1 << 0);
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S8N;
		bpe = 1;
		break;
	case GL_FIXED:
		ffp_vertex_attrib_fixed_mask |= (1 << 0);
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}
	attributes->componentCount = 3;
	streams->stride = stride ? stride : bpe * 3;
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glTexCoordPointer, DLIST_FUNC_I32_U32_I32_U32, size, type, stride, pointer))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 1) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	ffp_vertex_attrib_offsets[FFP_ATTRIB_TEX(client_texture_unit)] = (uint32_t)pointer;
	ffp_vertex_attrib_vbo[FFP_ATTRIB_TEX(client_texture_unit)] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[FFP_ATTRIB_TEX(client_texture_unit)];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[FFP_ATTRIB_TEX(client_texture_unit)];
	ffp_dirty_vert_attr |= (1 << FFP_ATTRIB_TEX(client_texture_unit));

	unsigned short bpe;
	switch (type) {
	case GL_FLOAT:
		ffp_vertex_attrib_fixed_mask &= ~(1 << (client_texture_unit + 1));
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		ffp_vertex_attrib_fixed_mask &= ~(1 << (client_texture_unit + 1));
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16;
		bpe = 2;
		break;
	case GL_FIXED:
		ffp_vertex_attrib_fixed_mask |= (1 << (client_texture_unit + 1));
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glInterleavedArrays(GLenum format, GLsizei stride, const void *pointer) {
	THREAD_SAFE()

#ifndef SKIP_ERROR_HANDLING
	if (stride < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	SceGxmVertexAttribute *attributes;
	SceGxmVertexStream *streams;

	switch (format) {
	case GL_V2F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 2, 8, 0, FFP_ATTRIB_POSITION) // Vertex2f
		break;
	case GL_V3F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 12, 0, FFP_ATTRIB_POSITION) // Vertex3f
		break;
	case GL_C4UB_V2F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_U8N, 4, 12, 0, FFP_ATTRIB_COLOR) // Color4Ub
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 2, 12, 4, FFP_ATTRIB_POSITION) // Vertex2f
		break;
	case GL_C4UB_V3F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_U8N, 4, 16, 0, FFP_ATTRIB_COLOR) // Color4Ub
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 16, 4, FFP_ATTRIB_POSITION) // Vertex3f
		break;
	case GL_C3F_V3F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 24, 0, FFP_ATTRIB_COLOR) // Color3f
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 24, 12, FFP_ATTRIB_POSITION) // Vertex3f
		break;
	case GL_T2F_V3F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 2, 20, 0, FFP_ATTRIB_TEX0) // Texcoord2f
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 20, 8, FFP_ATTRIB_POSITION) // Vertex3f
		break;
	case GL_T4F_V4F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 32, 0, FFP_ATTRIB_TEX0) // Texcoord4f
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 4, 32, 16, FFP_ATTRIB_POSITION) // Vertex4f
		break;
	case GL_T2F_C4UB_V3F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 2, 24, 0, FFP_ATTRIB_TEX0) // Texcoord2f
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_U8N, 4, 24, 8, FFP_ATTRIB_COLOR) // Color4ub
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 24, 12, FFP_ATTRIB_POSITION) // Vertex3f
		break;
	case GL_T2F_C3F_V3F:
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 2, 32, 0, FFP_ATTRIB_TEX0) // Texcoord2f
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 32, 8, FFP_ATTRIB_COLOR) // Color3f
		set_interleaved_comp(SCE_GXM_ATTRIBUTE_FORMAT_F32, 3, 32, 20, FFP_ATTRIB_POSITION) // Vertex3f
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, format)
	}
}

inline void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glVertex3f, DLIST_FUNC_F32_F32_F32, x, y, z))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	legacy_pool_ptr[0] = x;
	legacy_pool_ptr[1] = y;
	legacy_pool_ptr[2] = z;
	if (texture_units[1].state) { // Multitexturing enabled
		vgl_fast_memcpy(legacy_pool_ptr + 3, &current_vtx.uv.x, sizeof(float) * 2);
		vgl_fast_memcpy(legacy_pool_ptr + 5, &current_vtx.uv2.x, sizeof(float) * 2);
		if (lighting_state) {
			vgl_fast_memcpy(legacy_pool_ptr + 7, &current_vtx.amb.x, sizeof(float) * 19);
		} else {
			vgl_fast_memcpy(legacy_pool_ptr + 7, &current_vtx.clr.x, sizeof(float) * 4);
		}
		legacy_pool_ptr += LEGACY_MT_VERTEX_STRIDE;
	} else if (texture_units[0].state) { // Texturing enabled
		if (lighting_state) {
			vgl_fast_memcpy(legacy_pool_ptr + 3, &current_vtx.uv.x, sizeof(float) * 2);
			vgl_fast_memcpy(legacy_pool_ptr + 5, &current_vtx.amb.x, sizeof(float) * 19);
		} else {
			vgl_fast_memcpy(legacy_pool_ptr + 3, &current_vtx.uv.x, sizeof(float) * 6);
		}
		legacy_pool_ptr += LEGACY_VERTEX_STRIDE;
	} else { // Texturing disabled
		if (lighting_state) {
			vgl_fast_memcpy(legacy_pool_ptr + 3, &current_vtx.amb.x, sizeof(float) * 19);
		} else {
			vgl_fast_memcpy(legacy_pool_ptr + 3, &current_vtx.clr.x, sizeof(float) * 4);
		}
		legacy_pool_ptr += LEGACY_NT_VERTEX_STRIDE;
	}

	// Increasing vertex counter
	vertex_count++;
}

void glClientActiveTexture(GLenum texture) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glClientActiveTexture, DLIST_FUNC_U32, texture))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE15)) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, texture)
	}
	if (texture - GL_TEXTURE0 > TEXTURE_COORDS_NUM) {
		vgl_log("%s:%d Attempting to use a too high client texture unit (GL_TEXTURE%d).\n", __FILE__, __LINE__, texture - GL_TEXTURE0);
	}
#endif
	client_texture_unit = texture - GL_TEXTURE0;
}

void glVertex3fv(const GLfloat *v) {
	glVertex3f(v[0], v[1], v[2]);
}

void glVertex3dv(const GLdouble *v) {
	glVertex3f(v[0], v[1], v[2]);
}

void glVertex3i(GLint x, GLint y, GLint z) {
	glVertex3f(x, y, z);
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
	glVertex3f(x, y, z);
}

void glVertex2fv(const GLfloat *v) {
	glVertex2f(v[0], v[1]);
}

void glVertex2dv(const GLdouble *v) {
	glVertex2f(v[0], v[1]);
}

inline void glVertex2f(GLfloat x, GLfloat y) {
	glVertex3f(x, y, 0.0f);
}

inline void glVertex2d(GLdouble x, GLdouble y) {
	glVertex3d(x, y, 0.0f);
}

inline void glVertex2i(GLint x, GLint y) {
	glVertex3f(x, y, 0.0f);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glMaterialfv, DLIST_FUNC_U32_U32_U32, face, pname, params))
		return;
#endif
	switch (pname) {
	case GL_AMBIENT:
		vgl_fast_memcpy(&current_vtx.amb.x, params, sizeof(float) * 4);
		break;
	case GL_DIFFUSE:
		vgl_fast_memcpy(&current_vtx.diff.x, params, sizeof(float) * 4);
		break;
	case GL_SPECULAR:
		vgl_fast_memcpy(&current_vtx.spec.x, params, sizeof(float) * 4);
		break;
	case GL_EMISSION:
		vgl_fast_memcpy(&current_vtx.emiss.x, params, sizeof(float) * 4);
		break;
	case GL_AMBIENT_AND_DIFFUSE:
		vgl_fast_memcpy(&current_vtx.amb.x, params, sizeof(float) * 4);
		vgl_fast_memcpy(&current_vtx.diff.x, params, sizeof(float) * 4);
		break;
	case GL_SHININESS:
		current_shininess = params[0];
		flag_dirty_ffp_unif(shading_mode == SMOOTH, SHININESS_V_UNIF, SHININESS_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glMaterialf, DLIST_FUNC_U32_U32_F32, face, pname, param))
		return;
#endif
	switch (pname) {
	case GL_SHININESS:
		current_shininess = param;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, SHININESS_V_UNIF, SHININESS_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glMateriali(GLenum face, GLenum pname, GLint param) {
	glMaterialf(face, pname, (GLfloat)param);
}

void glMaterialxv(GLenum face, GLenum pname, const GLfixed *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glMaterialxv, DLIST_FUNC_U32_U32_U32, face, pname, params))
		return;
#endif
	switch (pname) {
	case GL_AMBIENT:
		current_vtx.amb.x = (float)params[0] / 65536.0f;
		current_vtx.amb.y = (float)params[1] / 65536.0f;
		current_vtx.amb.z = (float)params[2] / 65536.0f;
		current_vtx.amb.w = (float)params[3] / 65536.0f;
		break;
	case GL_DIFFUSE:
		current_vtx.diff.x = (float)params[0] / 65536.0f;
		current_vtx.diff.y = (float)params[1] / 65536.0f;
		current_vtx.diff.z = (float)params[2] / 65536.0f;
		current_vtx.diff.w = (float)params[3] / 65536.0f;
		break;
	case GL_SPECULAR:
		current_vtx.spec.x = (float)params[0] / 65536.0f;
		current_vtx.spec.y = (float)params[1] / 65536.0f;
		current_vtx.spec.z = (float)params[2] / 65536.0f;
		current_vtx.spec.w = (float)params[3] / 65536.0f;
		break;
	case GL_EMISSION:
		current_vtx.emiss.x = (float)params[0] / 65536.0f;
		current_vtx.emiss.y = (float)params[1] / 65536.0f;
		current_vtx.emiss.z = (float)params[2] / 65536.0f;
		current_vtx.emiss.w = (float)params[3] / 65536.0f;
		break;
	case GL_AMBIENT_AND_DIFFUSE:
		current_vtx.amb.x = (float)params[0] / 65536.0f;
		current_vtx.amb.y = (float)params[1] / 65536.0f;
		current_vtx.amb.z = (float)params[2] / 65536.0f;
		current_vtx.amb.w = (float)params[3] / 65536.0f;
		vgl_fast_memcpy(&current_vtx.diff.x, &current_vtx.amb.x, sizeof(float) * 4);
		break;
	case GL_SHININESS:
		current_shininess = (float)params[0] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, SHININESS_V_UNIF, SHININESS_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glMaterialx(GLenum face, GLenum pname, const GLfixed param) {
	glMaterialf(face, pname, (float)param / 65536.0f);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor3f, DLIST_FUNC_F32_F32_F32, red, green, blue))
		return;
#endif
	// Setting current color value
	current_vtx.clr.r = red;
	current_vtx.clr.g = green;
	current_vtx.clr.b = blue;
	current_vtx.clr.a = 1.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor3fv(const GLfloat *v) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor3fv, DLIST_FUNC_U32, v))
		return;
#endif
	// Setting current color value
	vgl_fast_memcpy(&current_vtx.clr.r, v, sizeof(vector3f));
	current_vtx.clr.a = 1.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor3us(GLushort red, GLushort green, GLushort blue) {
	THREAD_SAFE()

	current_vtx.clr.r = (float)red / 65535.0f;
	current_vtx.clr.g = (float)green / 65535.0f;
	current_vtx.clr.b = (float)blue / 65535.0f;
	current_vtx.clr.a = 1.0f;
	
	flag_dirty_frag_unif(TINT_COLOR_UNIF)
};

void glColor3usv(const GLushort *c) {
	THREAD_SAFE()

	current_vtx.clr.r = (float)c[0] / 65535.0f;
	current_vtx.clr.g = (float)c[1] / 65535.0f;
	current_vtx.clr.b = (float)c[2] / 65535.0f;
	current_vtx.clr.a = 1.0f;
	
	flag_dirty_frag_unif(TINT_COLOR_UNIF)
};

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor3ub, DLIST_FUNC_U8_U8_U8, red, green, blue))
		return;
#endif
	// Setting current color value
	current_vtx.clr.r = (float)red / 255.0f;
	current_vtx.clr.g = (float)green / 255.0f;
	current_vtx.clr.b = (float)blue / 255.0f;
	current_vtx.clr.a = 1.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor3ubv(const GLubyte *c) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor3ubv, DLIST_FUNC_U32, c))
		return;
#endif
	// Setting current color value
	current_vtx.clr.r = (float)c[0] / 255.0f;
	current_vtx.clr.g = (float)c[1] / 255.0f;
	current_vtx.clr.b = (float)c[2] / 255.0f;
	current_vtx.clr.a = 1.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor4f, DLIST_FUNC_F32_F32_F32_F32, red, green, blue, alpha))
		return;
#endif
	// Setting current color value
	current_vtx.clr.r = red;
	current_vtx.clr.g = green;
	current_vtx.clr.b = blue;
	current_vtx.clr.a = alpha;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor4fv(const GLfloat *v) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor4fv, DLIST_FUNC_U32, v))
		return;
#endif
	// Setting current color value
	vgl_fast_memcpy(&current_vtx.clr.r, v, sizeof(vector4f));

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor4ub, DLIST_FUNC_U8_U8_U8_U8, red, green, blue, alpha))
		return;
#endif
	current_vtx.clr.r = (float)red / 255.0f;
	current_vtx.clr.g = (float)green / 255.0f;
	current_vtx.clr.b = (float)blue / 255.0f;
	current_vtx.clr.a = (float)alpha / 255.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha) {
	THREAD_SAFE()

	current_vtx.clr.r = (float)red / 65535.0f;
	current_vtx.clr.g = (float)green / 65535.0f;
	current_vtx.clr.b = (float)blue / 65535.0f;
	current_vtx.clr.a = (float)alpha / 65535.0f;
	
	flag_dirty_frag_unif(TINT_COLOR_UNIF)
};

void glColor4usv(const GLushort *c) {
	THREAD_SAFE()

	current_vtx.clr.r = (float)c[0] / 65535.0f;
	current_vtx.clr.g = (float)c[1] / 65535.0f;
	current_vtx.clr.b = (float)c[2] / 65535.0f;
	current_vtx.clr.a = (float)c[3] / 65535.0f;
	
	flag_dirty_frag_unif(TINT_COLOR_UNIF)
};

void glColor4ubv(const GLubyte *c) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor4ubv, DLIST_FUNC_U32, c))
		return;
#endif
	// Setting current color value
	current_vtx.clr.r = (float)c[0] / 255.0f;
	current_vtx.clr.g = (float)c[1] / 255.0f;
	current_vtx.clr.b = (float)c[2] / 255.0f;
	current_vtx.clr.a = (float)c[3] / 255.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColor4x, DLIST_FUNC_I32_I32_I32_I32, red, green, blue, alpha))
		return;
#endif

	// Setting current color value
	current_vtx.clr.r = (float)red / 65536.0f;
	current_vtx.clr.g = (float)green / 65536.0f;
	current_vtx.clr.b = (float)blue / 65536.0f;
	current_vtx.clr.a = (float)alpha / 65536.0f;

	flag_dirty_frag_unif(TINT_COLOR_UNIF)
}

void glNormal3f(GLfloat x, GLfloat y, GLfloat z) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glNormal3f, DLIST_FUNC_F32_F32_F32, x, y, z))
		return;
#endif
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

void glNormal3s(GLshort x, GLshort y, GLshort z) {
	glNormal3f(x, y, z);
}

void glNormal3x(GLfixed x, GLfixed y, GLfixed z) {
	glNormal3f((float)x / 65536.0f, (float)y / 65536.0f, (float)z / 65536.0f);
}

void glNormal3fv(const GLfloat *v) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glNormal3fv, DLIST_FUNC_U32, v))
		return;
#endif
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
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glTexCoord2f, DLIST_FUNC_F32_F32, s, t))
		return;
#endif
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

void glTexCoord2s(GLshort s, GLshort t) {
	glTexCoord2f(s, t);
}

void glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glMultiTexCoord2f, DLIST_FUNC_U32_F32_F32, target, s, t))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	switch (target) {
	case GL_TEXTURE0:
		current_vtx.uv.x = s;
		current_vtx.uv.y = t;
		break;
	case GL_TEXTURE1:
		current_vtx.uv2.x = s;
		current_vtx.uv2.y = t;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glMultiTexCoord2fv(GLenum target, GLfloat *f) {
	glMultiTexCoord2f(target, f[0], f[1]);
}

void glMultiTexCoord2i(GLenum target, GLint s, GLint t) {
	glMultiTexCoord2f(target, s, t);
}

void glBegin(GLenum mode) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glBegin, DLIST_FUNC_U32, mode))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	// Changing current openGL machine state
	phase = MODEL_CREATION;
#endif

	// Performing a scene reset if necessary
	scene_reset();

	// Tracking desired primitive
	ffp_mode = mode;

	// Resetting vertex count
	vertex_count = 0;
}

void glEnd(void) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glEnd, DLIST_FUNC_VOID))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase != MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	// Changing current openGL machine state
	phase = NONE;
#endif

	// Translating primitive to sceGxm one
	gl_primitive_to_gxm(ffp_mode, prim, vertex_count);

	// Invalidating current attributes state settings
	uint16_t orig_state = ffp_vertex_attrib_state;

	ffp_dirty_frag = GL_TRUE;
	ffp_dirty_vert = GL_TRUE;
	if (texture_units[1].state) { // Multitexture usage
		ffp_vertex_attrib_state = FFP_ATTRIB_MASK_ALL;
		reload_ffp_shaders(legacy_mt_vertex_attrib_config, legacy_mt_vertex_stream_config, SCE_GXM_INDEX_SOURCE_INDEX_16BIT);
		for (int i = 0; i < 2; i++) {
			texture *tex = &texture_slots[texture_units[i].tex_id[texture_units[i].state > 1 ? 0 : 1]];
#ifdef HAVE_TEX_CACHE
			restore_tex_cache(tex);
#endif
#ifndef TEXTURES_SPEEDHACK
			tex->last_frame = vgl_framecount;
#endif
			sampler *smp = samplers[i];
			if (smp) {
				vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
				vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
				vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_TRUE;
			} else if (tex->overridden) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_FALSE;
			}
			sceGxmSetFragmentTexture(gxm_context, i, &tex->gxm_tex);
		}
	} else if (texture_units[0].state) { // Texturing usage
		ffp_vertex_attrib_state = (1 << FFP_ATTRIB_POSITION) | (1 << FFP_ATTRIB_TEX0) | (1 << FFP_ATTRIB_COLOR);
		reload_ffp_shaders(legacy_vertex_attrib_config, legacy_vertex_stream_config, SCE_GXM_INDEX_SOURCE_INDEX_16BIT);
		texture *tex = &texture_slots[texture_units[0].tex_id[texture_units[0].state > 1 ? 0 : 1]];
#ifdef HAVE_TEX_CACHE
		restore_tex_cache(tex);
#endif
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = vgl_framecount;
#endif
		sampler *smp = samplers[0];
		if (smp) {
			vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
			vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_TRUE;
		} else if (tex->overridden) {
			vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
			vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
			vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
			vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
			vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			tex->overridden = GL_FALSE;
		}
		sceGxmSetFragmentTexture(gxm_context, 0, &tex->gxm_tex);
	} else { // No texturing usage
		ffp_vertex_attrib_state = (1 << FFP_ATTRIB_POSITION) | (1 << FFP_ATTRIB_COLOR);
		reload_ffp_shaders(legacy_nt_vertex_attrib_config, legacy_nt_vertex_stream_config, SCE_GXM_INDEX_SOURCE_INDEX_16BIT);
	}

	// Restoring original attributes state settings
	ffp_vertex_attrib_state = orig_state;

	// Uploading vertex streams and performing the draw
	for (int i = 0; i < ffp_vertex_num_params; i++) {
		sceGxmSetVertexStream(gxm_context, i, legacy_pool);
	}

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
		vgl_fast_memcpy(ptr, default_line_strips_idx_ptr, (vertex_count - 1) * 2 * sizeof(uint16_t));
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
	if (texture_units[1].state)
		legacy_pool += vertex_count * LEGACY_MT_VERTEX_STRIDE;
	else if (texture_units[0].state)
		legacy_pool += vertex_count * LEGACY_VERTEX_STRIDE;
	else
		legacy_pool += vertex_count * LEGACY_NT_VERTEX_STRIDE;

#ifndef SKIP_ERROR_HANDLING
	// Checking for out of bounds of the immediate mode vertex pool
	if (legacy_pool >= legacy_pool_end) {
		vgl_log("%s:%d %s: Legacy pool outbounded by %d bytes! Consider increasing its size...\n", __FILE__, __LINE__, __func__, legacy_pool - legacy_pool_end);
	}
#endif

	// Restore polygon mode if a GL_LINES/GL_POINTS has been rendered
	restore_polygon_mode(prim);
}

void glTexEnvfv(GLenum target, GLenum pname, GLfloat *param) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glTexEnvf, DLIST_FUNC_U32_U32_U32, target, pname, param))
		return;
#endif
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];

	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
		case GL_TEXTURE_ENV_COLOR:
			vgl_fast_memcpy(&tex_unit->env_color.r, param, sizeof(GLfloat) * 4);
			flag_dirty_frag_unif(TEX_ENV_COLOR_UNIF)
			break;
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_RGB_SCALE:
#ifndef SKIP_ERROR_HANDLING
			if (*param != 1.0f && *param != 2.0f && *param != 4.0f) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			tex_unit->rgb_scale = *param;
			flag_dirty_frag_unif(SCALE_PASS_0_UNIF + server_texture_unit)
			break;
		case GL_ALPHA_SCALE:
#ifndef SKIP_ERROR_HANDLING
			if (*param != 1.0f && *param != 2.0f && *param != 4.0f) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			tex_unit->a_scale = *param;
			flag_dirty_frag_unif(SCALE_PASS_0_UNIF + server_texture_unit)
			break;
#endif
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glTexEnvxv(GLenum target, GLenum pname, GLfixed *param) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glTexEnvxv, DLIST_FUNC_U32_U32_U32, target, pname, param))
		return;
#endif
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];

	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
		case GL_TEXTURE_ENV_COLOR:
			tex_unit->env_color.r = (float)param[0] / 65536.0f;
			tex_unit->env_color.g = (float)param[1] / 65536.0f;
			tex_unit->env_color.b = (float)param[2] / 65536.0f;
			tex_unit->env_color.a = (float)param[3] / 65536.0f;
			flag_dirty_frag_unif(TEX_ENV_COLOR_UNIF)
			break;
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_RGB_SCALE:
#ifndef SKIP_ERROR_HANDLING
			if (*param != 65536 && *param != 131072 && *param != 262144) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			tex_unit->rgb_scale = (float)param[0] / 65536.0f;
			flag_dirty_frag_unif(SCALE_PASS_0_UNIF + server_texture_unit)
			break;
		case GL_ALPHA_SCALE:
#ifndef SKIP_ERROR_HANDLING
			if (*param != 65536 && *param != 131072 && *param != 262144) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			tex_unit->a_scale = (float)param[0] / 65536.0f;
			flag_dirty_frag_unif(SCALE_PASS_0_UNIF + server_texture_unit)
			break;
#endif
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

inline void glTexEnvi(GLenum target, GLenum pname, GLint param) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glTexEnvf, DLIST_FUNC_U32_U32_I32, target, pname, param))
		return;
#endif
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];

	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_RGB_SCALE:
#ifndef SKIP_ERROR_HANDLING
			if (param != 1 && param != 2 && param != 4) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			tex_unit->rgb_scale = param;
			break;
		case GL_ALPHA_SCALE:
#ifndef SKIP_ERROR_HANDLING
			if (param != 1 && param != 2 && param != 4) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			tex_unit->a_scale = param;
			break;
#endif
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
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glTexEnvx(GLenum target, GLenum pname, GLfixed param) {
	THREAD_SAFE()

	glTexEnvi(target, pname, param);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	THREAD_SAFE()

	glTexEnvi(target, pname, (GLint)param);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params) {
	THREAD_SAFE()

	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];

	// Properly changing texture environment settings as per request
	switch (target) {
	case GL_TEXTURE_ENV:
		switch (pname) {
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_RGB_SCALE:
			*params = tex_unit->rgb_scale;
			break;
		case GL_ALPHA_SCALE:
			*params = tex_unit->a_scale;
			break;
#endif
		case GL_TEXTURE_ENV_MODE:
			switch (tex_unit->env_mode) {
			case MODULATE:
				*params = GL_MODULATE;
				break;
			case DECAL:
				*params = GL_DECAL;
				break;
			case REPLACE:
				*params = GL_REPLACE;
				break;
			case BLEND:
				*params = GL_BLEND;
				break;
			case ADD:
				*params = GL_ADD;
				break;
#ifndef DISABLE_TEXTURE_COMBINER
			case COMBINE:
				*params = GL_COMBINE;
				break;
#endif
			}
			break;
#ifndef DISABLE_TEXTURE_COMBINER
		case GL_COMBINE_RGB:
			switch (tex_unit->combiner.rgb_func) {
			case REPLACE:
				*params = GL_REPLACE;
				break;
			case MODULATE:
				*params = GL_MODULATE;
				break;
			case ADD:
				*params = GL_ADD;
				break;
			case ADD_SIGNED:
				*params = GL_ADD_SIGNED;
				break;
			case INTERPOLATE:
				*params = GL_INTERPOLATE;
				break;
			case SUBTRACT:
				*params = GL_SUBTRACT;
				break;
			}
			break;
		case GL_COMBINE_ALPHA:
			switch (tex_unit->combiner.a_func) {
			case REPLACE:
				*params = GL_REPLACE;
				break;
			case MODULATE:
				*params = GL_MODULATE;
				break;
			case ADD:
				*params = GL_ADD;
				break;
			case ADD_SIGNED:
				*params = GL_ADD_SIGNED;
				break;
			case INTERPOLATE:
				*params = GL_INTERPOLATE;
				break;
			case SUBTRACT:
				*params = GL_SUBTRACT;
				break;
			}
			break;
		case GL_SRC0_RGB:
			switch (tex_unit->combiner.op_rgb_0) {
			case TEXTURE:
				*params = GL_TEXTURE;
				break;
			case CONSTANT:
				*params = GL_CONSTANT;
				break;
			case PRIMARY_COLOR:
				*params = GL_PRIMARY_COLOR;
				break;
			case PREVIOUS:
				*params = GL_PREVIOUS;
				break;
			}
			break;
		case GL_SRC1_RGB:
			switch (tex_unit->combiner.op_rgb_1) {
			case TEXTURE:
				*params = GL_TEXTURE;
				break;
			case CONSTANT:
				*params = GL_CONSTANT;
				break;
			case PRIMARY_COLOR:
				*params = GL_PRIMARY_COLOR;
				break;
			case PREVIOUS:
				*params = GL_PREVIOUS;
				break;
			}
			break;
		case GL_SRC2_RGB:
			switch (tex_unit->combiner.op_rgb_2) {
			case TEXTURE:
				*params = GL_TEXTURE;
				break;
			case CONSTANT:
				*params = GL_CONSTANT;
				break;
			case PRIMARY_COLOR:
				*params = GL_PRIMARY_COLOR;
				break;
			case PREVIOUS:
				*params = GL_PREVIOUS;
				break;
			}
			break;
		case GL_SRC0_ALPHA:
			switch (tex_unit->combiner.op_a_0) {
			case TEXTURE:
				*params = GL_TEXTURE;
				break;
			case CONSTANT:
				*params = GL_CONSTANT;
				break;
			case PRIMARY_COLOR:
				*params = GL_PRIMARY_COLOR;
				break;
			case PREVIOUS:
				*params = GL_PREVIOUS;
				break;
			}
			break;
		case GL_SRC1_ALPHA:
			switch (tex_unit->combiner.op_a_1) {
			case TEXTURE:
				*params = GL_TEXTURE;
				break;
			case CONSTANT:
				*params = GL_CONSTANT;
				break;
			case PRIMARY_COLOR:
				*params = GL_PRIMARY_COLOR;
				break;
			case PREVIOUS:
				*params = GL_PREVIOUS;
				break;
			}
			break;
		case GL_SRC2_ALPHA:
			switch (tex_unit->combiner.op_a_2) {
			case TEXTURE:
				*params = GL_TEXTURE;
				break;
			case CONSTANT:
				*params = GL_CONSTANT;
				break;
			case PRIMARY_COLOR:
				*params = GL_PRIMARY_COLOR;
				break;
			case PREVIOUS:
				*params = GL_PREVIOUS;
				break;
			}
			break;
		case GL_OPERAND0_RGB:
			switch (tex_unit->combiner.op_mode_rgb_0) {
			case SRC_COLOR:
				*params = GL_SRC_COLOR;
				break;
			case ONE_MINUS_SRC_COLOR:
				*params = GL_ONE_MINUS_SRC_COLOR;
				break;
			case SRC_ALPHA:
				*params = GL_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_ALPHA:
				*params = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND1_RGB:
			switch (tex_unit->combiner.op_mode_rgb_1) {
			case SRC_COLOR:
				*params = GL_SRC_COLOR;
				break;
			case ONE_MINUS_SRC_COLOR:
				*params = GL_ONE_MINUS_SRC_COLOR;
				break;
			case SRC_ALPHA:
				*params = GL_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_ALPHA:
				*params = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND2_RGB:
			switch (tex_unit->combiner.op_mode_rgb_2) {
			case SRC_COLOR:
				*params = GL_SRC_COLOR;
				break;
			case ONE_MINUS_SRC_COLOR:
				*params = GL_ONE_MINUS_SRC_COLOR;
				break;
			case SRC_ALPHA:
				*params = GL_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_ALPHA:
				*params = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND0_ALPHA:
			switch (tex_unit->combiner.op_mode_a_0) {
			case SRC_ALPHA:
				*params = GL_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_ALPHA:
				*params = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND1_ALPHA:
			switch (tex_unit->combiner.op_mode_a_1) {
			case SRC_ALPHA:
				*params = GL_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_ALPHA:
				*params = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
		case GL_OPERAND2_ALPHA:
			switch (tex_unit->combiner.op_mode_a_2) {
			case SRC_ALPHA:
				*params = GL_SRC_ALPHA;
				break;
			case ONE_MINUS_SRC_ALPHA:
				*params = GL_ONE_MINUS_SRC_ALPHA;
				break;
			}
			break;
#endif
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glLightfv, DLIST_FUNC_U32_U32_U32, light, pname, params))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (light < GL_LIGHT0 && light > GL_LIGHT7) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, light)
	}
#endif

	switch (pname) {
	case GL_AMBIENT:
		vgl_fast_memcpy(&lights_ambients[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_AMBIENTS_V_UNIF, LIGHTS_AMBIENTS_F_UNIF)
		break;
	case GL_DIFFUSE:
		vgl_fast_memcpy(&lights_diffuses[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_DIFFUSES_V_UNIF, LIGHTS_DIFFUSES_F_UNIF)
		break;
	case GL_SPECULAR:
		vgl_fast_memcpy(&lights_speculars[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_SPECULARS_V_UNIF, LIGHTS_SPECULARS_F_UNIF)
		break;
	case GL_POSITION:
		vector4f_matrix4x4_mult(&lights_positions[light - GL_LIGHT0].r, modelview_matrix, params);
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_POSITIONS_V_UNIF, LIGHTS_POSITIONS_F_UNIF)
		break;
	case GL_CONSTANT_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].r = params[0];
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_ATTENUATIONS_V_UNIF, LIGHTS_ATTENUATIONS_F_UNIF)
		break;
	case GL_LINEAR_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].g = params[0];
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_ATTENUATIONS_V_UNIF, LIGHTS_ATTENUATIONS_F_UNIF)
		break;
	case GL_QUADRATIC_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].b = params[0];
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_ATTENUATIONS_V_UNIF, LIGHTS_ATTENUATIONS_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glLightxv(GLenum light, GLenum pname, const GLfixed *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glLightxv, DLIST_FUNC_U32_U32_U32, light, pname, params))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (light < GL_LIGHT0 && light > GL_LIGHT7) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, light)
	}
#endif

	vector4f v;
	switch (pname) {
	case GL_AMBIENT:
		lights_ambients[light - GL_LIGHT0].r = (float)params[0] / 65536.0f;
		lights_ambients[light - GL_LIGHT0].g = (float)params[1] / 65536.0f;
		lights_ambients[light - GL_LIGHT0].b = (float)params[2] / 65536.0f;
		lights_ambients[light - GL_LIGHT0].a = (float)params[3] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_AMBIENTS_V_UNIF, LIGHTS_AMBIENTS_F_UNIF)
		break;
	case GL_DIFFUSE:
		lights_diffuses[light - GL_LIGHT0].r = (float)params[0] / 65536.0f;
		lights_diffuses[light - GL_LIGHT0].g = (float)params[1] / 65536.0f;
		lights_diffuses[light - GL_LIGHT0].b = (float)params[2] / 65536.0f;
		lights_diffuses[light - GL_LIGHT0].a = (float)params[3] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_DIFFUSES_V_UNIF, LIGHTS_DIFFUSES_F_UNIF)
		break;
	case GL_SPECULAR:
		lights_speculars[light - GL_LIGHT0].r = (float)params[0] / 65536.0f;
		lights_speculars[light - GL_LIGHT0].g = (float)params[1] / 65536.0f;
		lights_speculars[light - GL_LIGHT0].b = (float)params[2] / 65536.0f;
		lights_speculars[light - GL_LIGHT0].a = (float)params[3] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_SPECULARS_V_UNIF, LIGHTS_SPECULARS_F_UNIF)
		break;
	case GL_POSITION:
		v.r = (float)params[0] / 65536.0f;
		v.g = (float)params[1] / 65536.0f;
		v.b = (float)params[2] / 65536.0f;
		v.a = (float)params[3] / 65536.0f;
		vector4f_matrix4x4_mult(&lights_positions[light - GL_LIGHT0].r, modelview_matrix, &v);
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_POSITIONS_V_UNIF, LIGHTS_POSITIONS_F_UNIF)
		break;
	case GL_CONSTANT_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].r = (float)params[0] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_ATTENUATIONS_V_UNIF, LIGHTS_ATTENUATIONS_F_UNIF)
		break;
	case GL_LINEAR_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].g = (float)params[0] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_ATTENUATIONS_V_UNIF, LIGHTS_ATTENUATIONS_F_UNIF)
		break;
	case GL_QUADRATIC_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].b = (float)params[0] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHTS_ATTENUATIONS_V_UNIF, LIGHTS_ATTENUATIONS_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glLightModelfv(GLenum pname, const GLfloat *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glLightModelfv, DLIST_FUNC_U32_U32, pname, params))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	switch (pname) {
	case GL_LIGHT_MODEL_AMBIENT:
		vgl_fast_memcpy(&light_global_ambient.r, params, sizeof(float) * 4);
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHT_GLOBAL_AMBIENT_V_UNIF, LIGHT_GLOBAL_AMBIENT_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glLightModelxv(GLenum pname, const GLfixed *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glLightModelxv, DLIST_FUNC_U32_U32, pname, params))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	switch (pname) {
	case GL_LIGHT_MODEL_AMBIENT:
		light_global_ambient.r = (float)params[0] / 65536.0f;
		light_global_ambient.g = (float)params[1] / 65536.0f;
		light_global_ambient.b = (float)params[2] / 65536.0f;
		light_global_ambient.a = (float)params[3] / 65536.0f;
		flag_dirty_ffp_unif(shading_mode == SMOOTH, LIGHT_GLOBAL_AMBIENT_V_UNIF, LIGHT_GLOBAL_AMBIENT_F_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glFogf(GLenum pname, GLfloat param) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glFogf, DLIST_FUNC_U32_F32, pname, param))
		return;
#endif
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = param;
		flag_dirty_frag_unif(FOG_DENSITY_UNIF)
		break;
	case GL_FOG_START:
		fog_near = param;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		break;
	case GL_FOG_END:
		fog_far = param;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		flag_dirty_frag_unif(FOG_FAR_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glFogx(GLenum pname, GLfixed param) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glFogx, DLIST_FUNC_U32_I32, pname, param))
		return;
#endif
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = (float)param / 65536.0f;
		flag_dirty_frag_unif(FOG_DENSITY_UNIF)
		break;
	case GL_FOG_START:
		fog_near = (float)param / 65536.0f;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		break;
	case GL_FOG_END:
		fog_far = (float)param / 65536.0f;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		flag_dirty_frag_unif(FOG_FAR_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glFogfv(GLenum pname, const GLfloat *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glFogfv, DLIST_FUNC_U32_U32, pname, params))
		return;
#endif
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = params[0];
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = params[0];
		flag_dirty_frag_unif(FOG_DENSITY_UNIF)
		break;
	case GL_FOG_START:
		fog_near = params[0];
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		break;
	case GL_FOG_END:
		fog_far = params[0];
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		flag_dirty_frag_unif(FOG_FAR_UNIF)
		break;
	case GL_FOG_COLOR:
		vgl_fast_memcpy(&fog_color.r, params, sizeof(vector4f));
		flag_dirty_frag_unif(FOG_COLOR_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glFogxv(GLenum pname, const GLfixed *params) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glFogxv, DLIST_FUNC_U32_U32, pname, params))
		return;
#endif
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = params[0];
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = (float)params[0] / 65536.0f;
		flag_dirty_frag_unif(FOG_DENSITY_UNIF)
		break;
	case GL_FOG_START:
		fog_near = (float)params[0] / 65536.0f;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		break;
	case GL_FOG_END:
		fog_far = (float)params[0] / 65536.0f;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		flag_dirty_frag_unif(FOG_FAR_UNIF)
		break;
	case GL_FOG_COLOR:
		fog_color.r = (float)params[0] / 65536.0f;
		fog_color.g = (float)params[1] / 65536.0f;
		fog_color.b = (float)params[2] / 65536.0f;
		fog_color.a = (float)params[3] / 65536.0f;
		flag_dirty_frag_unif(FOG_COLOR_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glFogi(GLenum pname, const GLint param) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glFogi, DLIST_FUNC_U32_I32, pname, param))
		return;
#endif
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = param;
		flag_dirty_frag_unif(FOG_DENSITY_UNIF)
		break;
	case GL_FOG_START:
		fog_near = param;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		break;
	case GL_FOG_END:
		fog_far = param;
		fog_range = fog_far - fog_near;
		flag_dirty_frag_unif(FOG_RANGE_UNIF)
		flag_dirty_frag_unif(FOG_FAR_UNIF)
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glClipPlane(GLenum plane, const GLdouble *equation) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glClipPlane, DLIST_FUNC_U32_U32, plane, equation))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (plane < GL_CLIP_PLANE0 || plane > GL_CLIP_PLANE6) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, plane)
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
	vgl_fast_memcpy(&clip_planes_eq[idx].x, &temp.x, sizeof(vector4f));
	flag_dirty_frag_unif(CLIP_PLANES_EQUATION_UNIF)
}

void glClipPlanef(GLenum plane, const GLfloat *equation) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glClipPlanef, DLIST_FUNC_U32_U32, plane, equation))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (plane < GL_CLIP_PLANE0 || plane > GL_CLIP_PLANE6) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, plane)
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
	vgl_fast_memcpy(&clip_planes_eq[idx].x, &temp.x, sizeof(vector4f));
	flag_dirty_frag_unif(CLIP_PLANES_EQUATION_UNIF)
}

void glClipPlanex(GLenum plane, const GLfixed *equation) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glClipPlanex, DLIST_FUNC_U32_U32, plane, equation))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (plane < GL_CLIP_PLANE0 || plane > GL_CLIP_PLANE6) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, plane)
	}
#endif
	int idx = plane - GL_CLIP_PLANE0;
	clip_planes_eq[idx].x = (float)equation[0] / 65536.0f;
	clip_planes_eq[idx].y = (float)equation[1] / 65536.0f;
	clip_planes_eq[idx].z = (float)equation[2] / 65536.0f;
	clip_planes_eq[idx].w = (float)equation[3] / 65536.0f;
	matrix4x4 inverted, inverted_transposed;
	matrix4x4_invert(inverted, modelview_matrix);
	matrix4x4_transpose(inverted_transposed, inverted);
	vector4f temp;
	vector4f_matrix4x4_mult(&temp, inverted_transposed, &clip_planes_eq[idx]);
	vgl_fast_memcpy(&clip_planes_eq[idx].x, &temp.x, sizeof(vector4f));
	flag_dirty_frag_unif(CLIP_PLANES_EQUATION_UNIF)
}

void glShadeModel(GLenum mode) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glShadeModel, DLIST_FUNC_U32, mode))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	switch (mode) {
	case GL_FLAT:
		//shading_mode = FLAT;
		shading_mode = SMOOTH;
		vgl_log("%s:%d %s: GL_FLAT as shading model is not supported. GL_SMOOTH will be used instead.\n", __FILE__, __LINE__, __func__);
		break;
	case GL_SMOOTH:
		shading_mode = SMOOTH;
		break;
	case GL_PHONG_WIN:
		shading_mode = PHONG;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, mode)
	}

	ffp_dirty_frag = GL_TRUE;
	ffp_dirty_vert = GL_TRUE;
}

void glColorMaterial(GLenum face, GLenum mode) {
	THREAD_SAFE()

#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glColorMaterial, DLIST_FUNC_U32_U32, face, mode))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	color_material_mode = mode;
	adjust_color_material_state();
}

void glGetPointerv(GLenum pname, void **params) {
	THREAD_SAFE()

	switch (pname) {
	case GL_VERTEX_ARRAY_POINTER:
		*params = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_POSITION];
		break;
	case GL_TEXTURE_COORD_ARRAY_POINTER:
		*params = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_TEX(client_texture_unit)];
		break;
	case GL_COLOR_ARRAY_POINTER:
		*params = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_COLOR];
		break;
	case GL_NORMAL_ARRAY_POINTER:
		*params = (void *)ffp_vertex_attrib_offsets[FFP_ATTRIB_NORMAL];
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
	THREAD_SAFE()

#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
	THREAD_SAFE()

#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	glBegin(GL_QUADS);
	glVertex2i(x1, y1);
	glVertex2i(x2, y1);
	glVertex2i(x2, y2);
	glVertex2i(x1, y2);
	glEnd();
}
