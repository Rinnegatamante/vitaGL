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
 * custom_shaders.c:
 * Implementation for custom shaders feature
 */

#include "shared.h"

#define MAX_CUSTOM_SHADERS 2048 // Maximum number of linkable custom shaders
#define MAX_CUSTOM_PROGRAMS 1024 // Maximum number of linkable custom programs

#define DISABLED_ATTRIBS_POOL_SIZE (256 * 1024) // Disabled attributes circular pool size in bytes

#define disableDrawAttrib(i) \
	orig_stride[i] = streams[i].stride; \
	orig_fmt[i] = attributes[i].format; \
	orig_size[i] = attributes[i].componentCount; \
	streams[i].stride = 0; \
	attributes[i].offset = 0; \
	attributes[i].componentCount = vertex_attrib_size[real_i[i]]; \
	attributes[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;

// Internal stuffs
GLboolean use_shark = GL_TRUE; // Flag to check if vitaShaRK should be initialized at vitaGL boot
GLboolean is_shark_online = GL_FALSE; // Current vitaShaRK status
SceGxmVertexAttribute vertex_attrib_config[VERTEX_ATTRIBS_NUM];
static SceGxmVertexStream vertex_stream_config[VERTEX_ATTRIBS_NUM];
static float *vertex_attrib_value[VERTEX_ATTRIBS_NUM];
static float *vertex_attrib_pool;
static float *vertex_attrib_pool_ptr;
static float *vertex_attrib_pool_limit;
static uint8_t vertex_attrib_size[VERTEX_ATTRIBS_NUM] = {4, 4, 4, 4, 4, 4, 4, 4};
static uint32_t vertex_attrib_offsets[VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint32_t vertex_attrib_vbo[VERTEX_ATTRIBS_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t vertex_attrib_state = 0;
static SceGxmVertexAttribute temp_attributes[VERTEX_ATTRIBS_NUM];
static SceGxmVertexStream temp_streams[VERTEX_ATTRIBS_NUM];
static unsigned short orig_stride[VERTEX_ATTRIBS_NUM];
static SceGxmAttributeFormat orig_fmt[VERTEX_ATTRIBS_NUM];
static unsigned char orig_size[VERTEX_ATTRIBS_NUM];

// Internal runtime shader compiler settings
int32_t compiler_fastmath = GL_TRUE;
int32_t compiler_fastprecision = GL_FALSE;
int32_t compiler_fastint = GL_TRUE;
shark_opt compiler_opts = SHARK_OPT_FAST;

GLuint cur_program = 0; // Current in use custom program (0 = No custom program)

// Uniform struct
typedef struct uniform {
	const SceGxmProgramParameter *ptr;
	void *chain;
	float *data;
	uint32_t size;
	GLboolean is_alias;
} uniform;

// Generic shader struct
typedef struct shader {
	GLenum type;
	GLboolean valid;
	SceGxmShaderPatcherId id;
	const SceGxmProgram *prog;
	uint32_t size;
	char *source;
	char *log;
} shader;

// Program status enum
typedef enum prog_status {
	PROG_INVALID,
	PROG_UNLINKED,
	PROG_LINKED
} prog_status;

// Program struct holding vertex/fragment shader info
typedef struct program {
	shader *vshader;
	shader *fshader;
	uint8_t status;
	GLboolean texunits[TEXTURE_IMAGE_UNITS_NUM];
	SceGxmVertexAttribute attr[VERTEX_ATTRIBS_NUM];
	SceGxmVertexStream stream[VERTEX_ATTRIBS_NUM];
	SceGxmVertexProgram *vprog;
	SceGxmFragmentProgram *fprog;
	blend_config blend_info;
	GLuint attr_num;
	GLuint attr_idx;
	GLuint stream_num;
	const SceGxmProgramParameter *wvp;
	uniform *vert_uniforms;
	uniform *frag_uniforms;
	GLuint attr_highest_idx;
	GLboolean has_unaligned_attrs;
} program;

// Internal shaders and array
static shader shaders[MAX_CUSTOM_SHADERS];
static program progs[MAX_CUSTOM_PROGRAMS];

float *reserve_attrib_pool(uint8_t count) {
	float *res = vertex_attrib_pool_ptr;
	vertex_attrib_pool_ptr += count;
	if (vertex_attrib_pool_ptr > vertex_attrib_pool_limit) {
		vertex_attrib_pool_ptr = vertex_attrib_pool;
		return vertex_attrib_pool_ptr;
	}
	return res;
}

GLenum gxm_vd_fmt_to_gl(SceGxmAttributeFormat fmt) {
	switch (fmt) {
	case SCE_GXM_ATTRIBUTE_FORMAT_F16:
		return GL_HALF_FLOAT;
	case SCE_GXM_ATTRIBUTE_FORMAT_F32:
		return GL_FLOAT;
	case SCE_GXM_ATTRIBUTE_FORMAT_S16:
	case SCE_GXM_ATTRIBUTE_FORMAT_S16N:
		return GL_SHORT;
	case SCE_GXM_ATTRIBUTE_FORMAT_U16:
	case SCE_GXM_ATTRIBUTE_FORMAT_U16N:
		return GL_UNSIGNED_SHORT;
	case SCE_GXM_ATTRIBUTE_FORMAT_S8:
	case SCE_GXM_ATTRIBUTE_FORMAT_S8N:
		return GL_BYTE;
	case SCE_GXM_ATTRIBUTE_FORMAT_U8:
	case SCE_GXM_ATTRIBUTE_FORMAT_U8N:
		return GL_UNSIGNED_BYTE;
	default:
		break;
	}
}

GLenum gxm_attr_type_to_gl(uint8_t size, uint8_t num) {
	switch (size * num) {
	case 1:
		return GL_FLOAT;
	case 2:
		return GL_FLOAT_VEC2;
	case 3:
		return GL_FLOAT_VEC3;
	case 4:
		return num == 1 ? GL_FLOAT_VEC4 : GL_FLOAT_MAT2;
	case 9:
		return GL_FLOAT_MAT3;
	case 16:
		return GL_FLOAT_MAT4;
	default:
		break;
	}
}

GLenum gxm_unif_type_to_gl(SceGxmParameterType type, uint8_t count) {
	switch (type) {
	case SCE_GXM_PARAMETER_TYPE_F32:
	case SCE_GXM_PARAMETER_TYPE_F16:
	case SCE_GXM_PARAMETER_TYPE_C10:
		switch (count) {
		case 1:
			return GL_FLOAT;
		case 2:
			return GL_FLOAT_VEC2;
		case 3:
			return GL_FLOAT_VEC3;
		case 4:
			return GL_FLOAT_VEC4;
		default:
			break;
		}
	case SCE_GXM_PARAMETER_TYPE_U32:
	case SCE_GXM_PARAMETER_TYPE_S32:
		switch (count) {
		case 1:
			return GL_INT;
		case 2:
			return GL_INT_VEC2;
		case 3:
			return GL_INT_VEC3;
		case 4:
			return GL_INT_VEC4;
		default:
			break;
		}
	default:
		break;
	}
}

void resetCustomShaders(void) {
	// Init custom shaders
	int i;
	for (i = 0; i < MAX_CUSTOM_SHADERS; i++) {
		shaders[i].valid = GL_FALSE;
		shaders[i].log = NULL;
		shaders[i].source = NULL;
	}

	// Init custom programs
	for (i = 0; i < MAX_CUSTOM_PROGRAMS; i++) {
		progs[i].status = PROG_INVALID;
	}

	vertex_attrib_pool = (float *)gpu_alloc_mapped(DISABLED_ATTRIBS_POOL_SIZE, VGL_MEM_RAM);
	vertex_attrib_pool_ptr = vertex_attrib_pool;
	vertex_attrib_pool_limit = (float *)((uint8_t *)vertex_attrib_pool + DISABLED_ATTRIBS_POOL_SIZE);

	// Init generic vertex attrib arrays
	for (i = 0; i < VERTEX_ATTRIBS_NUM; i++) {
		vertex_attrib_value[i] = reserve_attrib_pool(4);
		vertex_attrib_config[i].componentCount = 4;
		vertex_attrib_config[i].offset = 0;
		vertex_attrib_config[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		vertex_attrib_config[i].regIndex = i;
		vertex_attrib_config[i].streamIndex = i;
		vertex_stream_config[i].stride = 0;
		vertex_stream_config[i].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	}
}

GLboolean _glDrawArrays_CustomShadersIMPL(GLsizei count) {
	program *p = &progs[cur_program - 1];

	// Check if a blend info rebuild is required and upload fragment program
	if (p->blend_info.raw != blend_info.raw) {
		p->blend_info.raw = blend_info.raw;
		rebuild_frag_shader(p->fshader->id, &p->fprog);
	}
	sceGxmSetFragmentProgram(gxm_context, p->fprog);

	// Uploading textures on relative texture units
	int i;
	for (i = 0; i < TEXTURE_IMAGE_UNITS_NUM; i++) {
		if (p->texunits[i]) {
			texture_unit *tex_unit = &texture_units[client_texture_unit + i];
#ifndef SKIP_ERROR_HANDLING
			int r = sceGxmTextureValidate(&texture_slots[tex_unit->tex_id].gxm_tex);
			if (r) {
				vgl_log("glDrawArrays: Texture on TEXUNIT%d is invalid (%s), draw will be skipped.\n", i, get_gxm_error_literal(r));
				return GL_FALSE;
			}
#endif
			sceGxmSetFragmentTexture(gxm_context, i, &texture_slots[tex_unit->tex_id].gxm_tex);
		}
	}

	// Aligning attributes
	SceGxmVertexAttribute *attributes;
	SceGxmVertexStream *streams;
	uint8_t real_i[VERTEX_ATTRIBS_NUM] = {0, 1, 2, 3, 4, 5, 6, 7};
	if (p->has_unaligned_attrs) {
		attributes = temp_attributes;
		streams = temp_streams;
		int j = 0;
		for (i = 0; i < p->attr_highest_idx; i++) {
			if (p->attr[i].regIndex != 0xDEAD) {
				sceClibMemcpy(&temp_attributes[j], &vertex_attrib_config[i], sizeof(SceGxmVertexAttribute));
				sceClibMemcpy(&temp_streams[j], &vertex_stream_config[i], sizeof(SceGxmVertexStream));
				attributes[j].streamIndex = j;
				real_i[j] = attributes[j].regIndex;
				j++;
			}
		}
	} else {
		attributes = vertex_attrib_config;
		streams = vertex_stream_config;
	}

	void *ptrs[VERTEX_ATTRIBS_NUM];
#ifndef DRAW_SPEEDHACK
	GLboolean is_packed = p->attr_num > 1;
	if (is_packed) {
		for (i = 0; i < p->attr_num; i++) {
			if (vertex_attrib_vbo[real_i[i]]) {
				is_packed = GL_FALSE;
				break;
			}
		}
		if (is_packed && (!(vertex_attrib_offsets[real_i[0]] + streams[0].stride > vertex_attrib_offsets[real_i[1]] && vertex_attrib_offsets[real_i[1]] > vertex_attrib_offsets[real_i[0]])))
			is_packed = GL_FALSE;
	}

	// Gathering real attribute data pointers
	if (is_packed) {
		ptrs[0] = gpu_alloc_mapped_temp(count * streams[0].stride);
		sceClibMemcpy(ptrs[0], (void *)vertex_attrib_offsets[real_i[0]], count * streams[0].stride);
		for (i = 0; i < p->attr_num; i++) {
			attributes[i].regIndex = p->attr[real_i[i]].regIndex;
			if (vertex_attrib_state & (1 << real_i[i])) {
				attributes[i].offset = vertex_attrib_offsets[real_i[i]] - vertex_attrib_offsets[real_i[0]];
			} else {
				disableDrawAttrib(i)
			}
		}
	} else 
#endif
	{
		for (i = 0; i < p->attr_num; i++) {
			attributes[i].regIndex = p->attr[real_i[i]].regIndex;
			if (vertex_attrib_state & (1 << real_i[i])) {
				if (vertex_attrib_vbo[real_i[i]]) {
					gpubuffer *gpu_buf = (gpubuffer *)vertex_attrib_vbo[real_i[i]];
					ptrs[i] = (uint8_t *)gpu_buf->ptr + vertex_attrib_offsets[real_i[i]];
					gpu_buf->used = GL_TRUE;
					attributes[i].offset = 0;
				} else {
#ifdef DRAW_SPEEDHACK
					ptrs[i] = (void *)vertex_attrib_offsets[real_i[i]];
#else
					ptrs[i] = gpu_alloc_mapped_temp(count * streams[i].stride);
					sceClibMemcpy(ptrs[i], (void *)vertex_attrib_offsets[real_i[i]], count * streams[i].stride);
#endif
					attributes[i].offset = 0;
				}
			} else {
				disableDrawAttrib(i)
			}
		}
	}

	// Uploading new vertex program
	patchVertexProgram(gxm_shader_patcher, p->vshader->id, attributes, p->attr_num, streams, p->attr_num, &p->vprog);
	sceGxmSetVertexProgram(gxm_context, p->vprog);

	// Uploading both fragment and vertex uniforms data
	void *buffer;
	if (p->vert_uniforms) {
		sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &buffer);
		uniform *u = p->vert_uniforms;
		while (u) {
			sceGxmSetUniformDataF(buffer, u->ptr, 0, u->size, u->data);
			u = (uniform *)u->chain;
		}
	}
	if (p->frag_uniforms) {
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &buffer);
		uniform *u = p->frag_uniforms;
		while (u) {
#ifdef HAVE_SAMPLERS_AS_UNIFORMS
			if (u->size)
#endif
				sceGxmSetUniformDataF(buffer, u->ptr, 0, u->size, u->data);
			u = (uniform *)u->chain;
		}
	}

	// Uploading vertex streams
	for (i = 0; i < p->attr_num; i++) {
		GLboolean is_active = vertex_attrib_state & (1 << real_i[i]);
		if (is_active) {
#ifdef DRAW_SPEEDHACK
			sceGxmSetVertexStream(gxm_context, i, ptrs[i]);
#else
			sceGxmSetVertexStream(gxm_context, i, is_packed ? ptrs[0] : ptrs[i]);
#endif
		} else {
			sceGxmSetVertexStream(gxm_context, i, vertex_attrib_value[real_i[i]]);
		}
		if (!p->has_unaligned_attrs) {
			attributes[i].regIndex = i;
			if (!is_active) {
				streams[i].stride = orig_stride[i];
				attributes[i].componentCount = orig_size[i];
				attributes[i].format = orig_fmt[i];
			}
		}
	}

	return GL_TRUE;
}

GLboolean _glDrawElements_CustomShadersIMPL(uint16_t *idx_buf, GLsizei count) {
	program *p = &progs[cur_program - 1];

	// Check if a blend info rebuild is required and upload fragment program
	if (p->blend_info.raw != blend_info.raw) {
		p->blend_info.raw = blend_info.raw;
		rebuild_frag_shader(p->fshader->id, &p->fprog);
	}
	sceGxmSetFragmentProgram(gxm_context, p->fprog);

	// Uploading textures on relative texture units
	int i;
	for (i = 0; i < TEXTURE_IMAGE_UNITS_NUM; i++) {
		if (p->texunits[i]) {
			texture_unit *tex_unit = &texture_units[client_texture_unit + i];
#ifndef SKIP_ERROR_HANDLING
			int r = sceGxmTextureValidate(&texture_slots[tex_unit->tex_id].gxm_tex);
			if (r) {
				vgl_log("glDrawElements: Texture on TEXUNIT%d is invalid (%s), draw will be skipped.\n", i, get_gxm_error_literal(r));
				return GL_FALSE;
			}
#endif
			sceGxmSetFragmentTexture(gxm_context, i, &texture_slots[tex_unit->tex_id].gxm_tex);
		}
	}

	// Aligning attributes
	SceGxmVertexAttribute *attributes;
	SceGxmVertexStream *streams;
	uint8_t real_i[VERTEX_ATTRIBS_NUM] = {0, 1, 2, 3, 4, 5, 6, 7};
	if (p->has_unaligned_attrs) {
		attributes = temp_attributes;
		streams = temp_streams;
		int j = 0;
		for (i = 0; i < p->attr_highest_idx; i++) {
			if (p->attr[i].regIndex != 0xDEAD) {
				sceClibMemcpy(&temp_attributes[j], &vertex_attrib_config[i], sizeof(SceGxmVertexAttribute));
				sceClibMemcpy(&temp_streams[j], &vertex_stream_config[i], sizeof(SceGxmVertexStream));
				attributes[j].streamIndex = j;
				real_i[j] = attributes[j].regIndex;
				j++;
			}
		}
	} else {
		attributes = vertex_attrib_config;
		streams = vertex_stream_config;
	}

	void *ptrs[VERTEX_ATTRIBS_NUM];
#ifndef DRAW_SPEEDHACK	
	GLboolean is_packed = p->attr_num > 1;
	GLboolean is_full_vbo = GL_TRUE;
	if (is_packed) {
		for (i = 0; i < p->attr_num; i++) {
			if (vertex_attrib_vbo[real_i[i]]) {
				is_packed = GL_FALSE;
			} else {
				is_full_vbo = GL_FALSE;
			}
		}
		if (is_packed && (!(vertex_attrib_offsets[real_i[0]] + streams[0].stride > vertex_attrib_offsets[real_i[1]] && vertex_attrib_offsets[real_i[1]] > vertex_attrib_offsets[real_i[0]])))
			is_packed = GL_FALSE;
	} else if (!vertex_attrib_vbo[real_i[0]])
		is_full_vbo = GL_FALSE;

	// Detecting highest index value
	uint32_t top_idx = 0;
	if (!is_full_vbo) {
		for (i = 0; i < count; i++) {
			if (idx_buf[i] > top_idx)
				top_idx = idx_buf[i];
		}
		top_idx++;
	}

	// Gathering real attribute data pointers
	if (is_packed) {
		ptrs[0] = gpu_alloc_mapped_temp(top_idx * streams[0].stride);
		sceClibMemcpy(ptrs[0], (void *)vertex_attrib_offsets[real_i[0]], top_idx * streams[0].stride);
		for (i = 0; i < p->attr_num; i++) {
			attributes[i].regIndex = p->attr[real_i[i]].regIndex;
			if (vertex_attrib_state & (1 << real_i[i])) {
				attributes[i].offset = vertex_attrib_offsets[real_i[i]] - vertex_attrib_offsets[real_i[0]];
			} else {
				disableDrawAttrib(i)
			}
		}
	} else
#endif
	{
		for (i = 0; i < p->attr_num; i++) {
			attributes[i].regIndex = p->attr[real_i[i]].regIndex;
			if (vertex_attrib_state & (1 << real_i[i])) {
				if (vertex_attrib_vbo[real_i[i]]) {
					gpubuffer *gpu_buf = (gpubuffer *)vertex_attrib_vbo[real_i[i]];
					ptrs[i] = (uint8_t *)gpu_buf->ptr + vertex_attrib_offsets[real_i[i]];
					gpu_buf->used = GL_TRUE;
					attributes[i].offset = 0;
				} else {
#ifdef DRAW_SPEEDHACK
					ptrs[i] = (void *)vertex_attrib_offsets[real_i[i]];
#else
					ptrs[i] = gpu_alloc_mapped_temp(top_idx * streams[i].stride);
					sceClibMemcpy(ptrs[i], (void *)vertex_attrib_offsets[real_i[i]], top_idx * streams[i].stride);
#endif
					attributes[i].offset = 0;
				}
			} else {
				disableDrawAttrib(i)
			}
		}
	}

	// Uploading new vertex program
	patchVertexProgram(gxm_shader_patcher, p->vshader->id, attributes, p->attr_num, streams, p->attr_num, &p->vprog);
	sceGxmSetVertexProgram(gxm_context, p->vprog);

	// Uploading both fragment and vertex uniforms data
	void *buffer;
	if (p->vert_uniforms) {
		sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &buffer);
		uniform *u = p->vert_uniforms;
		while (u) {
			sceGxmSetUniformDataF(buffer, u->ptr, 0, u->size, u->data);
			u = (uniform *)u->chain;
		}
	}
	if (p->frag_uniforms) {
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &buffer);
		uniform *u = p->frag_uniforms;
		while (u) {
#ifdef HAVE_SAMPLERS_AS_UNIFORMS
			if (u->size)
#endif
				sceGxmSetUniformDataF(buffer, u->ptr, 0, u->size, u->data);
			u = (uniform *)u->chain;
		}
	}

	// Uploading vertex streams
	for (i = 0; i < p->attr_num; i++) {
		GLboolean is_active = vertex_attrib_state & (1 << real_i[i]);
		if (is_active) {
#ifdef DRAW_SPEEDHACK
			sceGxmSetVertexStream(gxm_context, i, ptrs[i]);
#else
			sceGxmSetVertexStream(gxm_context, i, is_packed ? ptrs[0] : ptrs[i]);
#endif
		} else {
			sceGxmSetVertexStream(gxm_context, i, vertex_attrib_value[real_i[i]]);
		}
		if (!p->has_unaligned_attrs) {
			attributes[i].regIndex = i;
			if (!is_active) {
				streams[i].stride = orig_stride[i];
				attributes[i].componentCount = orig_size[i];
				attributes[i].format = orig_fmt[i];
			}
		}
	}

	return GL_TRUE;
}

void _vglDrawObjects_CustomShadersIMPL(GLboolean implicit_wvp) {
	program *p = &progs[cur_program - 1];

	// Check if a blend info rebuild is required
	if (p->blend_info.raw != blend_info.raw) {
		p->blend_info.raw = blend_info.raw;
		rebuild_frag_shader(p->fshader->id, &p->fprog);
	}

	// Setting up required shader
	sceGxmSetVertexProgram(gxm_context, p->vprog);
	sceGxmSetFragmentProgram(gxm_context, p->fprog);

	// Uploading both fragment and vertex uniforms data
	void *vbuffer, *fbuffer;
	if (p->vert_uniforms) {
		sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
		uniform *u = p->vert_uniforms;
		while (u) {
			if (u->ptr == p->wvp && implicit_wvp) {
				if (mvp_modified) {
					matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix);
					mvp_modified = GL_FALSE;
				}
				sceGxmSetUniformDataF(vbuffer, p->wvp, 0, 16, (const float *)mvp_matrix);
			} else
				sceGxmSetUniformDataF(vbuffer, u->ptr, 0, u->size, u->data);
			u = (uniform *)u->chain;
		}
	}
	if (p->frag_uniforms) {
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &fbuffer);
		uniform *u = p->frag_uniforms;
		while (u) {
			sceGxmSetUniformDataF(fbuffer, u->ptr, 0, u->size, u->data);
			u = (uniform *)u->chain;
		}
	}

	// Uploading textures on relative texture units
	int i;
	for (i = 0; i < TEXTURE_IMAGE_UNITS_NUM; i++) {
		if (p->texunits[i]) {
			texture_unit *tex_unit = &texture_units[client_texture_unit + i];
			sceGxmSetFragmentTexture(gxm_context, i, &texture_slots[tex_unit->tex_id].gxm_tex);
		}
	}
}

#ifdef HAVE_SHARK_LOG
static char *shark_log = NULL;
void shark_log_cb(const char *msg, shark_log_level msg_level, int line) {
	uint8_t append = shark_log != NULL;
	char newline[1024];
	switch (msg_level) {
	case SHARK_LOG_INFO:
		sprintf(newline, "%sI] %s on line %d", append ? "\n" : "", msg, line);
		break;
	case SHARK_LOG_WARNING:
		sprintf(newline, "%sW] %s on line %d", append ? "\n" : "", msg, line);
		break;
	case SHARK_LOG_ERROR:
		sprintf(newline, "%sE] %s on line %d", append ? "\n" : "", msg, line);
		break;
	}
	uint32_t size = (append ? strlen(shark_log) : 0) + strlen(newline);
	shark_log = realloc(shark_log, size + 1);
	if (append)
		sprintf(shark_log, "%s%s", shark_log, newline);
	else
		strcpy(shark_log, newline);
}
#endif

float *getUniformAliasDataPtr(uniform *u, const char *name, uint32_t size) {
	int i;
	while (u) {
		if (size == u->size) {
			if (!strcmp(name, sceGxmProgramParameterGetName(u->ptr))) {
				return u->data;
			}
		}
		u = u->chain;
	}
	return NULL;
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */
void vglSetupRuntimeShaderCompiler(shark_opt opt_level, int32_t use_fastmath, int32_t use_fastprecision, int32_t use_fastint) {
	compiler_opts = opt_level;
	compiler_fastmath = use_fastmath;
	compiler_fastprecision = use_fastprecision;
	compiler_fastint = use_fastint;
}

void vglEnableRuntimeShaderCompiler(GLboolean usage) {
	use_shark = usage;
}

GLuint glCreateShader(GLenum shaderType) {
	// Looking for a free shader slot
	GLuint i, res = 0;
	for (i = 1; i <= MAX_CUSTOM_SHADERS; i++) {
		if (!(shaders[i - 1].valid)) {
			res = i;
			break;
		}
	}

	// All shader slots are busy, exiting call
	if (res == 0)
		return res;

	// Reserving and initializing shader slot
	switch (shaderType) {
	case GL_FRAGMENT_SHADER:
		shaders[res - 1].type = GL_FRAGMENT_SHADER;
		break;
	case GL_VERTEX_SHADER:
		shaders[res - 1].type = GL_VERTEX_SHADER;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, 0)
	}
	shaders[res - 1].valid = GL_TRUE;

	return res;
}

void glGetShaderiv(GLuint handle, GLenum pname, GLint *params) {
	// Grabbing passed shader
	shader *s = &shaders[handle - 1];

	switch (pname) {
	case GL_SHADER_TYPE:
		*params = s->type;
		break;
	case GL_COMPILE_STATUS:
		*params = s->prog ? GL_TRUE : GL_FALSE;
		break;
	case GL_INFO_LOG_LENGTH:
		*params = s->log ? strlen(s->log) : 0;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glGetShaderInfoLog(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
#ifndef SKIP_ERROR_HANDLING
	if (maxLength < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed shader
	shader *s = &shaders[handle - 1];

	GLsizei len = 0;
	if (s->log) {
		len = min(strlen(s->log), maxLength);
		sceClibMemcpy(infoLog, s->log, len);
	}
	if (length)
		*length = len;
}

void glShaderSource(GLuint handle, GLsizei count, const GLchar *const *string, const GLint *length) {
#ifndef SKIP_ERROR_HANDLING
	if (count < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed shader
	shader *s = &shaders[handle - 1];

	uint32_t size = 1;
	for (int i = 0; i < count; i++) {
		size += length ? length[i] : strlen(string[i]);
	}
#ifdef SHADER_COMPILER_SPEEDHACK
	if (count == 1)
		s->prog = (SceGxmProgram *)*string;
	else
#endif
	{
		s->source = (char *)vgl_malloc(size, VGL_MEM_EXTERNAL);
		s->source[0] = 0;
		for (int i = 0; i < count; i++) {
			sprintf(s->source, "%s%s", s->source, string[i]);
		}
		
		s->prog = (SceGxmProgram *)s->source;
	}
	s->size = size - 1;
}

void glShaderBinary(GLsizei count, const GLuint *handles, GLenum binaryFormat, const void *binary, GLsizei length) {
	// Grabbing passed shader
	shader *s = &shaders[handles[0] - 1];

	// Allocating compiled shader on RAM and registering it into sceGxmShaderPatcher
	s->prog = (SceGxmProgram *)vgl_malloc(length, VGL_MEM_EXTERNAL);
	sceClibMemcpy((void *)s->prog, binary, length);
	sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, s->prog, &s->id);
	s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
}

void glCompileShader(GLuint handle) {
	// If vitaShaRK is not enabled, we try to initialize it
	if (!is_shark_online && !startShaderCompiler()) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	// Grabbing passed shader
	shader *s = &shaders[handle - 1];

	// Compiling shader source
	s->prog = shark_compile_shader_extended((const char *)s->prog, &s->size, s->type == GL_FRAGMENT_SHADER ? SHARK_FRAGMENT_SHADER : SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
	if (s->prog) {
		if (s->source)
			vgl_free(s->source);
		SceGxmProgram *res = (SceGxmProgram *)vgl_malloc(s->size, VGL_MEM_EXTERNAL);
		sceClibMemcpy((void *)res, (void *)s->prog, s->size);
#ifdef LOG_ERRORS
		int r = sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &s->id);
		if (r)
			vgl_log("glCompileShader: Program failed to register on sceGxm (%s).\n", get_gxm_error_literal(r));
#else
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &s->id);
#endif
		s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
	}
#ifdef HAVE_SHARK_LOG
	s->log = shark_log;
	shark_log = NULL;
#endif
	shark_clear_output();
}

void glDeleteShader(GLuint shad) {
	// Grabbing passed shader
	shader *s = &shaders[shad - 1];

	// Deallocating shader and unregistering it from sceGxmShaderPatcher
	if (s->valid) {
		sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, s->id);
		vgl_free((void *)s->prog);
		if (s->log)
			vgl_free(s->log);
	}
	s->log = NULL;
	s->source = NULL;
	s->valid = GL_FALSE;
}

void glAttachShader(GLuint prog, GLuint shad) {
	// Grabbing passed shader and program
	shader *s = &shaders[shad - 1];
	program *p = &progs[prog - 1];

	// Attaching shader to desired program
	if (p->status == PROG_UNLINKED && s->valid) {
		switch (s->type) {
		case GL_VERTEX_SHADER:
			p->vshader = s;
			break;
		case GL_FRAGMENT_SHADER:
			p->fshader = s;
			break;
		default:
			break;
		}
	} else {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
}

GLuint glCreateProgram(void) {
	// Looking for a free program slot
	GLuint i, j, res = 0;
	for (i = 1; i <= MAX_CUSTOM_PROGRAMS; i++) {
		// Program slot found, reserving and initializing it
		if (!(progs[i - 1].status)) {
			res = i--;
			progs[i].status = PROG_UNLINKED;
			progs[i].attr_num = 0;
			progs[i].stream_num = 0;
			progs[i].attr_idx = 0;
			progs[i].wvp = NULL;
			progs[i].vshader = NULL;
			progs[i].fshader = NULL;
			progs[i].vert_uniforms = NULL;
			progs[i].frag_uniforms = NULL;
			progs[i].attr_highest_idx = 0;
			for (j = 0; j < VERTEX_ATTRIBS_NUM; j++) {
				progs[i].attr[j].regIndex = 0xDEAD;
			}
			break;
		}
	}
	return res;
}

void glDeleteProgram(GLuint prog) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Releasing both vertex and fragment programs from sceGxmShaderPatcher
	if (p->status) {
		unsigned int count, i;
		sceGxmShaderPatcherGetFragmentProgramRefCount(gxm_shader_patcher, p->fprog, &count);
		for (i = 0; i < count; i++) {
			sceGxmShaderPatcherReleaseFragmentProgram(gxm_shader_patcher, p->fprog);
			sceGxmShaderPatcherReleaseVertexProgram(gxm_shader_patcher, p->vprog);
		}
		while (p->vert_uniforms) {
			uniform *old = p->vert_uniforms;
			p->vert_uniforms = (uniform *)p->vert_uniforms->chain;
			if (!old->is_alias)
				vgl_free(old->data);
			vgl_free(old);
		}
		while (p->frag_uniforms) {
			uniform *old = p->frag_uniforms;
			p->frag_uniforms = (uniform *)p->frag_uniforms->chain;
			vgl_free(old->data);
			vgl_free(old);
		}
	}
	p->status = PROG_INVALID;
}

void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
	if (length)
		*length = 0;
}

void glGetProgramiv(GLuint progr, GLenum pname, GLint *params) {
	// Grabbing passed program
	program *p = &progs[progr - 1];
	int i, cnt;
	const SceGxmProgramParameter *param;
	uniform *u;

	switch (pname) {
	case GL_LINK_STATUS:
		*params = p->status == PROG_LINKED;
		break;
	case GL_INFO_LOG_LENGTH:
		*params = 0;
		break;
	case GL_ATTACHED_SHADERS:
		i = 0;
		if (p->fshader)
			i++;
		if (p->vshader)
			i++;
		*params = i;
		break;
	case GL_ACTIVE_ATTRIBUTES:
		*params = p->attr_num;
		break;
	case GL_ACTIVE_UNIFORM_MAX_LENGTH:
		i = 0;
		u = p->vert_uniforms;
		while (u) {
			int len = strlen(sceGxmProgramParameterGetName(u->ptr)) + 1;
			if (len > i)
				i = len;
			u = u->chain;
		}
		u = p->frag_uniforms;
		while (u) {
			int len = strlen(sceGxmProgramParameterGetName(u->ptr)) + 1;
			if (len > i)
				i = len;
			u = u->chain;
		}
		*params = i;
		break;
	case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
		i = 0;
		cnt = sceGxmProgramGetParameterCount(p->vshader->prog);
		while (cnt--) {
			param = sceGxmProgramGetParameter(p->vshader->prog, cnt);
			if (sceGxmProgramParameterGetCategory(param) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE) {
				int len = strlen(sceGxmProgramParameterGetName(param)) + 1;
				if (len > i)
					i = len;
			}
		}
		*params = i;
		break;
	case GL_ACTIVE_UNIFORMS:
		i = 0;
		u = p->vert_uniforms;
		while (u) {
			i++;
			u = u->chain;
		}
		u = p->frag_uniforms;
		while (u) {
			i++;
			u = u->chain;
		}
		*params = i;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glLinkProgram(GLuint progr) {
	// Grabbing passed program
	program *p = &progs[progr - 1];
#ifndef SKIP_ERROR_HANDLING
	if (!p->fshader->prog || !p->vshader->prog)
		return;
#endif
	p->status = PROG_LINKED;

	// Analyzing fragment shader
	uint32_t i, cnt;
	for (i = 0; i < TEXTURE_IMAGE_UNITS_NUM; i++) {
		p->texunits[i] = GL_FALSE;
	}
	cnt = sceGxmProgramGetParameterCount(p->fshader->prog);
	for (i = 0; i < cnt; i++) {
		const SceGxmProgramParameter *param = sceGxmProgramGetParameter(p->fshader->prog, i);
		SceGxmParameterCategory cat = sceGxmProgramParameterGetCategory(param);
		if (cat == SCE_GXM_PARAMETER_CATEGORY_SAMPLER) {
			p->texunits[sceGxmProgramParameterGetResourceIndex(param)] = GL_TRUE;
#ifdef HAVE_SAMPLERS_AS_UNIFORMS
			uniform *u = (uniform *)vgl_malloc(sizeof(uniform), VGL_MEM_EXTERNAL);
			u->chain = p->frag_uniforms;
			u->ptr = param;
			u->size = 0;
			u->data = (float *)vgl_malloc(sizeof(float), VGL_MEM_EXTERNAL);
			p->frag_uniforms = u;
#endif
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_UNIFORM) {
			uniform *u = (uniform *)vgl_malloc(sizeof(uniform), VGL_MEM_EXTERNAL);
			u->chain = p->frag_uniforms;
			u->ptr = param;
			u->size = sceGxmProgramParameterGetComponentCount(param) * sceGxmProgramParameterGetArraySize(param);
			u->data = (float *)vgl_malloc(u->size * sizeof(float), VGL_MEM_EXTERNAL);
			sceClibMemset(u->data, 0, u->size * sizeof(float));
			p->frag_uniforms = u;
		}
	}

	// Analyzing vertex shader
	p->wvp = sceGxmProgramFindParameterByName(p->vshader->prog, "wvp");
	cnt = sceGxmProgramGetParameterCount(p->vshader->prog);
	for (i = 0; i < cnt; i++) {
		const SceGxmProgramParameter *param = sceGxmProgramGetParameter(p->vshader->prog, i);
		SceGxmParameterCategory cat = sceGxmProgramParameterGetCategory(param);
		if (cat == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE) {
			p->attr_num++;
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_UNIFORM) {
			uniform *u = (uniform *)vgl_malloc(sizeof(uniform), VGL_MEM_EXTERNAL);
			u->chain = p->vert_uniforms;
			u->ptr = param;
			u->size = sceGxmProgramParameterGetComponentCount(param) * sceGxmProgramParameterGetArraySize(param);
			u->data = getUniformAliasDataPtr(p->frag_uniforms, sceGxmProgramParameterGetName(param), u->size);
			if (u->data) {
				u->is_alias = GL_TRUE;
			} else {
				u->is_alias = GL_FALSE;
				u->data = (float *)vgl_malloc(u->size * sizeof(float), VGL_MEM_EXTERNAL);
				sceClibMemset(u->data, 0, u->size * sizeof(float));
			}
			p->vert_uniforms = u;
		}
	}

	// Creating fragment and vertex program via sceGxmShaderPatcher if using vgl* draw pipeline
	if (p->stream_num) {
		if (p->stream_num > 1)
			p->stream_num = p->attr_num;
		patchVertexProgram(gxm_shader_patcher,
			p->vshader->id, p->attr, p->attr_num,
			p->stream, p->stream_num, &p->vprog);
		patchFragmentProgram(gxm_shader_patcher,
			p->fshader->id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
			msaa_mode, &blend_info.info, NULL, &p->fprog);

		// Populating current blend settings
		p->blend_info.raw = blend_info.raw;
	} else {
		// Checking if bound attributes are aligned
		p->has_unaligned_attrs = GL_FALSE;
		for (i = 0; i < p->attr_num; i++) {
			if (p->attr[i].regIndex == 0xDEAD) {
				p->has_unaligned_attrs = GL_TRUE;
				break;
			}
		}
	}
}

void glUseProgram(GLuint prog) {
	// Setting current custom program to passed program
	cur_program = prog;
}

GLint glGetUniformLocation(GLuint prog, const GLchar *name) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Checking if parameter is a vertex or fragment related one
	uniform *j;
	const SceGxmProgramParameter *u = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (u == NULL) {
		u = sceGxmProgramFindParameterByName(p->fshader->prog, name);
		if (u == NULL)
			return -1;
		else
			j = p->frag_uniforms;
	} else
		j = p->vert_uniforms;

	// Getting the desired location
	while (j) {
		if (j->ptr == u)
			return -((GLint)j);
		j = j->chain;
	}

	return -1;
}

void glUniform1i(GLint location, GLint v0) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
}

void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	int i;
	for (i = 0; i < count; i++) {
		u->data[i] = (float)value[i];
	}
}

void glUniform1f(GLint location, GLfloat v0) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * sizeof(float));
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
	u->data[1] = (float)v1;
}

void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	int i;
	for (i = 0; i < count * 2; i++) {
		u->data[i] = (float)value[i];
	}
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
	u->data[1] = v1;
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * 2 * sizeof(float));
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
	u->data[1] = (float)v1;
	u->data[2] = (float)v2;
}

void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	int i;
	for (i = 0; i < count * 3; i++) {
		u->data[i] = (float)value[i];
	}
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
	u->data[1] = v1;
	u->data[2] = v2;
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * 3 * sizeof(float));
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
	u->data[1] = (float)v1;
	u->data[2] = (float)v2;
	u->data[3] = (float)v3;
}

void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	int i;
	for (i = 0; i < count * 4; i++) {
		u->data[i] = (float)value[i];
	}
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
	u->data[1] = v1;
	u->data[2] = v2;
	u->data[3] = v3;
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * 4 * sizeof(float));
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * 4 * sizeof(float));
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * 9 * sizeof(float));
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	sceClibMemcpy(u->data, value, count * 16 * sizeof(float));
}

void glEnableVertexAttribArray(GLuint index) {
#ifndef SKIP_ERROR_HANDLING
	if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	vertex_attrib_state |= (1 << index);
}

void glDisableVertexAttribArray(GLuint index) {
#ifndef SKIP_ERROR_HANDLING
	if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	vertex_attrib_state &= ~(1 << index);
}

void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	} else if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE);
	}
#endif
	pointer[0] = (void *)vertex_attrib_offsets[index];
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (size < 1 || size > 4 || stride < 0 || index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	vertex_attrib_offsets[index] = (uint32_t)pointer;
	vertex_attrib_vbo[index] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &vertex_attrib_config[index];
	SceGxmVertexStream *streams = &vertex_stream_config[index];

	// Detecting attribute format and size
	unsigned short bpe;
	switch (type) {
	case GL_HALF_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F16;
		bpe = 2;
		break;
	case GL_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = 4;
		break;
	case GL_SHORT:
		attributes->format = normalized ? SCE_GXM_ATTRIBUTE_FORMAT_S16N : SCE_GXM_ATTRIBUTE_FORMAT_S16;
		bpe = 2;
		break;
	case GL_UNSIGNED_SHORT:
		attributes->format = normalized ? SCE_GXM_ATTRIBUTE_FORMAT_U16N : SCE_GXM_ATTRIBUTE_FORMAT_U16;
		bpe = 2;
		break;
	case GL_BYTE:
		attributes->format = normalized ? SCE_GXM_ATTRIBUTE_FORMAT_S8N : SCE_GXM_ATTRIBUTE_FORMAT_S8;
		bpe = 1;
		break;
	case GL_UNSIGNED_BYTE:
		attributes->format = normalized ? SCE_GXM_ATTRIBUTE_FORMAT_U8N : SCE_GXM_ATTRIBUTE_FORMAT_U8;
		bpe = 1;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;
}

void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
#ifndef SKIP_ERROR_HANDLING
	if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	switch (pname) {
	case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
		params[0] = (vertex_attrib_state & (1 << index)) ? vertex_attrib_vbo[index] : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
		params[0] = (vertex_attrib_state & (1 << index)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:
		params[0] = (vertex_attrib_state & (1 << index)) ? vertex_attrib_config[index].componentCount : vertex_attrib_size[index];
		break;
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
		params[0] = (vertex_attrib_state & (1 << index)) ? vertex_stream_config[index].stride : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:
		params[0] = (vertex_attrib_state & (1 << index)) ? gxm_vd_fmt_to_gl(vertex_attrib_config[index].format) : GL_FLOAT;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
		params[0] = (vertex_attrib_state & (1 << index)) ? (vertex_attrib_config[index].format >= SCE_GXM_ATTRIBUTE_FORMAT_U8N && vertex_attrib_config[index].format <= SCE_GXM_ATTRIBUTE_FORMAT_S16N) : GL_FALSE;
		break;
	case GL_CURRENT_VERTEX_ATTRIB:
#ifndef SKIP_ERROR_HANDLING
		if (index == 0) {
			SET_GL_ERROR(GL_INVALID_OPERATION)
		}
#endif
		params[0] = vertex_attrib_value[index][0];
		params[1] = vertex_attrib_size[index] > 1 ? vertex_attrib_value[index][1] : 0;
		params[2] = vertex_attrib_size[index] > 2 ? vertex_attrib_value[index][2] : 0;
		params[3] = vertex_attrib_size[index] > 3 ? vertex_attrib_value[index][3] : 1;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
#ifndef SKIP_ERROR_HANDLING
	if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	switch (pname) {
	case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
		params[0] = (vertex_attrib_state & (1 << index)) ? vertex_attrib_vbo[index] : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
		params[0] = (vertex_attrib_state & (1 << index)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:
		params[0] = (vertex_attrib_state & (1 << index)) ? vertex_attrib_config[index].componentCount : vertex_attrib_size[index];
		break;
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
		params[0] = (vertex_attrib_state & (1 << index)) ? vertex_stream_config[index].stride : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:
		params[0] = (vertex_attrib_state & (1 << index)) ? gxm_vd_fmt_to_gl(vertex_attrib_config[index].format) : GL_FLOAT;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
		params[0] = (vertex_attrib_state & (1 << index)) ? (vertex_attrib_config[index].format >= SCE_GXM_ATTRIBUTE_FORMAT_U8N && vertex_attrib_config[index].format <= SCE_GXM_ATTRIBUTE_FORMAT_S16N) : GL_FALSE;
		break;
	case GL_CURRENT_VERTEX_ATTRIB:
#ifndef SKIP_ERROR_HANDLING
		if (index == 0) {
			SET_GL_ERROR(GL_INVALID_OPERATION)
		}
#endif
		params[0] = vertex_attrib_value[index][0];
		params[1] = vertex_attrib_size[index] > 1 ? vertex_attrib_value[index][1] : 0;
		params[2] = vertex_attrib_size[index] > 2 ? vertex_attrib_value[index][2] : 0;
		params[3] = vertex_attrib_size[index] > 3 ? vertex_attrib_value[index][3] : 1;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glVertexAttrib1f(GLuint index, GLfloat v0) {
	vertex_attrib_value[index] = reserve_attrib_pool(1);
	vertex_attrib_size[index] = 1;
	vertex_attrib_value[index][0] = v0;
}

void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1) {
	vertex_attrib_value[index] = reserve_attrib_pool(2);
	vertex_attrib_size[index] = 2;
	vertex_attrib_value[index][0] = v0;
	vertex_attrib_value[index][1] = v1;
}

void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2) {
	vertex_attrib_value[index] = reserve_attrib_pool(3);
	vertex_attrib_size[index] = 3;
	vertex_attrib_value[index][0] = v0;
	vertex_attrib_value[index][1] = v1;
	vertex_attrib_value[index][2] = v2;
}

void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	vertex_attrib_value[index] = reserve_attrib_pool(4);
	vertex_attrib_size[index] = 4;
	vertex_attrib_value[index][0] = v0;
	vertex_attrib_value[index][1] = v1;
	vertex_attrib_value[index][2] = v2;
	vertex_attrib_value[index][3] = v3;
}

void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
	vertex_attrib_value[index] = reserve_attrib_pool(1);
	vertex_attrib_size[index] = 1;
	sceClibMemcpy(vertex_attrib_value[index], v, sizeof(float));
}

void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
	vertex_attrib_value[index] = reserve_attrib_pool(2);
	vertex_attrib_size[index] = 2;
	sceClibMemcpy(vertex_attrib_value[index], v, 2 * sizeof(float));
}

void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
	vertex_attrib_value[index] = reserve_attrib_pool(3);
	vertex_attrib_size[index] = 3;
	sceClibMemcpy(vertex_attrib_value[index], v, 3 * sizeof(float));
}

void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
	vertex_attrib_value[index] = reserve_attrib_pool(4);
	vertex_attrib_size[index] = 4;
	sceClibMemcpy(vertex_attrib_value[index], v, 4 * sizeof(float));
}

void glBindAttribLocation(GLuint prog, GLuint index, const GLchar *name) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Looking for desired parameter in requested program
	const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (param == NULL || sceGxmProgramParameterGetCategory(param) != SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE)
		return;

	p->attr[index].regIndex = sceGxmProgramParameterGetResourceIndex(param);
	if ((p->attr_highest_idx == 0) || (p->attr_highest_idx - 1 < index))
		p->attr_highest_idx = index + 1;
}

GLint glGetAttribLocation(GLuint prog, const GLchar *name) {
	program *p = &progs[prog - 1];
	const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (param == NULL || sceGxmProgramParameterGetCategory(param) != SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE)
		return -1;
	int index = sceGxmProgramParameterGetResourceIndex(param);

	// If attribute has been already bound, we return its location
	int i;
	for (i = 0; i < p->attr_highest_idx; i++) {
		if (p->attr[i].regIndex == index)
			return i;
	}

	// If attribute is not bound, we bind it and return its location
	for (i = 0; i < p->attr_num; i++) {
		if (p->attr[i].regIndex == 0xDEAD) {
			p->attr[i].regIndex = index;

			if ((p->attr_highest_idx == 0) || (p->attr_highest_idx - 1 < i))
				p->attr_highest_idx = i + 1;

			// Checking back if attributes are aligned
			p->has_unaligned_attrs = GL_FALSE;
			int j;
			for (j = 0; j < p->attr_num; j++) {
				if (p->attr[j].regIndex == 0xDEAD) {
					p->has_unaligned_attrs = GL_TRUE;
					break;
				}
			}
			return i;
		}
	}
}

void glGetActiveAttrib(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
#ifndef SKIP_ERROR_HANDLING
	if (bufSize < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed program
	program *p = &progs[prog - 1];

	int i, cnt = sceGxmProgramGetParameterCount(p->vshader->prog);
	const SceGxmProgramParameter *param;
	for (i = 0; i < cnt; i++) {
		param = sceGxmProgramGetParameter(p->vshader->prog, i);
		if (sceGxmProgramParameterGetCategory(param) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE && (sceGxmProgramParameterGetResourceIndex(param) / 4) == index)
			break;
	}

	// Copying attribute name
	const char *pname = sceGxmProgramParameterGetName(param);
	bufSize = min(strlen(pname), bufSize - 1);
	if (length)
		*length = bufSize;
	strncpy(name, pname, bufSize);
	name[bufSize] = 0;

	*type = gxm_attr_type_to_gl(sceGxmProgramParameterGetComponentCount(param), sceGxmProgramParameterGetArraySize(param));
	*size = 1;
}

void glGetActiveUniform(GLuint prog, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
#ifndef SKIP_ERROR_HANDLING
	if (bufSize < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed program
	program *p = &progs[prog - 1];

	// FIXME: We assume the func is never called by going out of bounds towards the active uniforms
	uniform *u = p->vert_uniforms ? p->vert_uniforms : p->frag_uniforms;
	while (index) {
		u = u->chain;
		if (!u)
			u = p->frag_uniforms;
		index--;
	}

	// Copying attribute name
	const char *pname = sceGxmProgramParameterGetName(u->ptr);
	bufSize = min(strlen(pname), bufSize - 1);
	if (length)
		*length = bufSize;
	strncpy(name, pname, bufSize);
	name[bufSize] = 0;

	*type = gxm_unif_type_to_gl(sceGxmProgramParameterGetType(u->ptr), sceGxmProgramParameterGetComponentCount(u->ptr));
	*size = sceGxmProgramParameterGetArraySize(u->ptr);
}

/*
 * ------------------------------
 * -    VGL_EXT_gxp_shaders     -
 * ------------------------------
 */

// Equivalent of glBindAttribLocation but for sceGxm architecture
void vglBindAttribLocation(GLuint prog, GLuint index, const GLchar *name, const GLuint num, const GLenum type) {
	// Grabbing passed program
	program *p = &progs[prog - 1];
	SceGxmVertexAttribute *attributes = &p->attr[index];
	SceGxmVertexStream *streams = &p->stream[index];

	// Looking for desired parameter in requested program
	const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (param == NULL)
		return;

	// Setting stream index and offset values
	attributes->streamIndex = index;
	attributes->offset = 0;

	// Detecting attribute format and size
	int bpe;
	switch (type) {
	case GL_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = sizeof(float);
		break;
	case GL_SHORT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;
		bpe = sizeof(int16_t);
		break;
	case GL_UNSIGNED_BYTE:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		bpe = sizeof(uint8_t);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}

	// Setting various info about the stream
	attributes->componentCount = num;
	attributes->regIndex = sceGxmProgramParameterGetResourceIndex(param);
	streams->stride = bpe * num;
	streams->indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	p->stream_num = 2;
}

// Equivalent of glBindAttribLocation but for sceGxm architecture when packed attributes are used
GLint vglBindPackedAttribLocation(GLuint prog, const GLchar *name, const GLuint num, const GLenum type, GLuint offset, GLint stride) {
	// Grabbing passed program
	program *p = &progs[prog - 1];
	SceGxmVertexAttribute *attributes = &p->attr[p->attr_idx];
	SceGxmVertexStream *streams = &p->stream[0];

	// Looking for desired parameter in requested program
	const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (param == NULL)
		return GL_FALSE;

	// Setting stream index and offset values
	attributes->streamIndex = 0;
	attributes->offset = offset;

	// Detecting attribute format and size
	int bpe;
	switch (type) {
	case GL_FLOAT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
		bpe = sizeof(float);
		break;
	case GL_SHORT:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;
		bpe = sizeof(int16_t);
		break;
	case GL_UNSIGNED_BYTE:
		attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_U8N;
		bpe = sizeof(uint8_t);
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, GL_FALSE)
	}

	// Setting various info about the stream
	attributes->componentCount = num;
	attributes->regIndex = sceGxmProgramParameterGetResourceIndex(param);
	streams->stride = stride ? stride : bpe * num;
	streams->indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;
	p->stream_num = 1;
	p->attr_idx++;

	return GL_TRUE;
}

// Equivalent of glVertexAttribPointer but for sceGxm architecture
void vglVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (stride < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Detecting type size
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
	}

	// Allocating enough memory on vitaGL mempool
	void *ptr = gpu_alloc_mapped_temp(count * bpe * size);

	// Copying passed data to vitaGL mempool
	if (stride == 0)
		sceClibMemcpy(ptr, pointer, count * bpe * size); // Faster if stride == 0
	else {
		int i;
		uint8_t *dst = (uint8_t *)ptr;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			sceClibMemcpy(dst, src, bpe * size);
			dst += (bpe * size);
			src += stride;
		}
	}

	// Setting vertex stream to passed index in sceGxm
	sceGxmSetVertexStream(gxm_context, index, ptr);
}

void vglVertexAttribPointerMapped(GLuint index, const GLvoid *pointer) {
	// Setting vertex stream to passed index in sceGxm
	sceGxmSetVertexStream(gxm_context, index, pointer);
}
