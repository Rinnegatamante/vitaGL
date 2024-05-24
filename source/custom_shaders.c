/*
 * This file is part of vitaGL
 * Copyright 2017-2023 Rinnegatamante
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

#define _GNU_SOURCE
#include <string.h>
#include "shared.h"
#include "utils/glsl_utils.h"
#include "utils/shacccg_paramquery.h"
#if defined(HAVE_SHADER_CACHE) || defined(HAVE_TEX_CACHE)
#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#define XXH_NAMESPACE VITAGL_
#include "utils/xxhash_utils.h"
#ifdef HAVE_SHADER_CACHE
char vgl_shader_cache_path[256];
#endif
#ifdef HAVE_TEX_CACHE
char vgl_file_cache_path[256];
#endif
#endif

#define MAX_CUSTOM_SHADERS 2048 // Maximum number of linkable custom shaders
#define MAX_CUSTOM_PROGRAMS 1024 // Maximum number of linkable custom programs

#define setDefaultAttribBindings() \
	uint32_t cnt = sceGxmProgramGetParameterCount(p->vshader->prog); \
	for (int i = 0; i < cnt; i++) { \
		const SceGxmProgramParameter *param = sceGxmProgramGetParameter(p->vshader->prog, i); \
		SceGxmParameterCategory cat = sceGxmProgramParameterGetCategory(param); \
		if (cat == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE) { \
			p->attr[p->attr_highest_idx++].regIndex = sceGxmProgramParameterGetResourceIndex(param); \
		} \
	}

#define disableDrawAttrib(i) \
	orig_stride[i] = streams[i].stride; \
	orig_fmt[i] = attributes[i].format; \
	orig_size[i] = attributes[i].componentCount; \
	streams[i].stride = 0; \
	attributes[i].offset = 0; \
	attributes[i].componentCount = cur_vao->vertex_attrib_size[attr_idx]; \
	attributes[i].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;

#define handleUnpackedAttrib(first, count) \
	if (cur_vao->vertex_attrib_state & (1 << attr_idx)) { \
		if (cur_vao->vertex_attrib_vbo[attr_idx]) { \
			gpubuffer *gpu_buf = (gpubuffer *)cur_vao->vertex_attrib_vbo[attr_idx]; \
			ptrs[i] = (uint8_t *)gpu_buf->ptr + cur_vao->vertex_attrib_offsets[attr_idx] + first * streams[i].stride; \
			gpu_buf->last_frame = vgl_framecount; \
			attributes[i].offset = 0; \
		} else { \
			ptrs[i] = gpu_alloc_mapped_temp(count * streams[i].stride); \
			vgl_fast_memcpy(ptrs[i], (void *)cur_vao->vertex_attrib_offsets[attr_idx] + first * streams[i].stride, count * streams[i].stride); \
			attributes[i].offset = 0; \
		} \
	} else { \
		disableDrawAttrib(i) \
	}
	
#define handleSpeedhackAttrib() \
	for (int i = 0; i < p->attr_num; i++) { \
		uint8_t attr_idx = p->attr_map[i]; \
		attributes[i].regIndex = p->attr[attr_idx].regIndex; \
		if (cur_vao->vertex_attrib_state & (1 << attr_idx)) { \
			if (cur_vao->vertex_attrib_vbo[attr_idx]) { \
				gpubuffer *gpu_buf = (gpubuffer *)cur_vao->vertex_attrib_vbo[attr_idx]; \
				ptrs[i] = (uint8_t *)gpu_buf->ptr + cur_vao->vertex_attrib_offsets[attr_idx]; \
				gpu_buf->last_frame = vgl_framecount; \
				attributes[i].offset = 0; \
			} else { \
				ptrs[i] = (void *)cur_vao->vertex_attrib_offsets[attr_idx]; \
				attributes[i].offset = 0; \
			} \
		} else { \
			disableDrawAttrib(i) \
		} \
	}

#define handlePackedAttrib() \
	if (cur_vao->vertex_attrib_state & (1 << attr_idx)) { \
		attributes[i].offset = cur_vao->vertex_attrib_offsets[attr_idx] - cur_vao->vertex_attrib_offsets[p->attr_map[0]]; \
	} else { \
		disableDrawAttrib(i) \
	}
	
#define uploadUniforms() \
	void *buffer; \
	if (p->vert_uniforms && dirty_vert_unifs) { \
		vglReserveVertexUniformBuffer(p->vshader->prog, &buffer); \
		uniform *u = p->vert_uniforms; \
		while (u) { \
			if (u->ptr == p->wvp) { \
				if (mvp_modified) { \
					matrix4x4_multiply(mvp_matrix, projection_matrix, modelview_matrix); \
					mvp_modified = GL_FALSE; \
				} \
				sceGxmSetUniformDataF(buffer, p->wvp, 0, 16, (const float *)mvp_matrix); \
			} else if (u->size > 0 && u->size < 0xFFFFFFFF) \
				sceGxmSetUniformDataF(buffer, u->ptr, 0, u->size, u->data); \
			u = (uniform *)u->chain; \
		} \
		dirty_vert_unifs = GL_FALSE; \
	} \
	if (p->frag_uniforms && dirty_frag_unifs) { \
		vglReserveFragmentUniformBuffer(p->fshader->prog, &buffer); \
		uniform *u = p->frag_uniforms; \
		while (u) { \
			if (u->size > 0 && u->size < 0xFFFFFFFF) \
				sceGxmSetUniformDataF(buffer, u->ptr, 0, u->size, u->data); \
			u = (uniform *)u->chain; \
		} \
		dirty_frag_unifs = GL_FALSE; \
	} \
	if (p->vert_ubos) { \
		ubo *u = p->vert_ubos; \
		while (u) { \
			ubo *b = u->alias ? u->alias : u; \
			sceGxmSetVertexUniformBuffer(gxm_context, b->idx, (uint8_t *)ubo_buf[b->bind]->ptr + ubo_offset[b->bind]); \
			ubo_buf[b->bind]->last_frame = vgl_framecount; \
			u = (ubo *)u->chain; \
		} \
	} \
	if (p->frag_ubos) { \
		ubo *u = p->frag_ubos; \
		while (u) { \
			sceGxmSetFragmentUniformBuffer(gxm_context, u->idx, (uint8_t *)ubo_buf[u->bind]->ptr + ubo_offset[u->bind]); \
			ubo_buf[u->bind]->last_frame = vgl_framecount; \
			u = (ubo *)u->chain; \
		} \
	}
	
#define setupFragProgram() \
	if ((p->blend_info.raw != blend_info.raw) || (is_fbo_float != p->is_fbo_float)) { \
		p->is_fbo_float = is_fbo_float; \
		p->blend_info.raw = blend_info.raw; \
		rebuild_frag_shader(p->fshader->id, &p->fprog, (SceGxmProgram *)p->vshader->prog, is_fbo_float ? SCE_GXM_OUTPUT_REGISTER_FORMAT_HALF4 : SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4); \
	} \
	sceGxmSetFragmentProgram(gxm_context, p->fprog);
	
#define alignAttributes(attributes, streams) \
	if (p->has_unaligned_attrs) { \
		attributes = temp_attributes; \
		streams = temp_streams; \
		for (int i = 0; i < p->attr_num; i++) { \
			uint8_t attr_idx = p->attr_map[i]; \
			temp_attributes[i] = cur_vao->vertex_attrib_config[attr_idx]; \
			temp_streams[i] = cur_vao->vertex_stream_config[attr_idx]; \
			attributes[i].streamIndex = i; \
		} \
	} else { \
		attributes = cur_vao->vertex_attrib_config; \
		streams = cur_vao->vertex_stream_config; \
	}

// Internal stuffs
GLboolean is_shark_online = GL_FALSE; // Current vitaShaRK status
static SceGxmVertexAttribute temp_attributes[VERTEX_ATTRIBS_NUM];
static SceGxmVertexStream temp_streams[VERTEX_ATTRIBS_NUM];
static unsigned short orig_stride[VERTEX_ATTRIBS_NUM];
static SceGxmAttributeFormat orig_fmt[VERTEX_ATTRIBS_NUM];
static unsigned char orig_size[VERTEX_ATTRIBS_NUM];
static gpubuffer *ubo_buf[UBOS_NUM];
static uint32_t ubo_offset[UBOS_NUM];
static uint8_t tex2d_override = 0;

#ifdef HAVE_GLSL_TRANSLATOR
typedef struct {
	GLuint idx;
	char name[64];
} attr_mapping;
#endif

// Internal runtime shader compiler settings
int32_t compiler_fastmath = GL_TRUE;
int32_t compiler_fastprecision = GL_FALSE;
int32_t compiler_fastint = GL_TRUE;
shark_opt compiler_opts = SHARK_OPT_FAST;

GLuint cur_program = 0; // Current in use custom program (0 = No custom program)

// Uniform struct
typedef struct {
	const SceGxmProgramParameter *ptr;
	void *chain;
	float *data;
	uint32_t size;
	GLboolean is_fragment;
	GLboolean is_vertex;
} uniform;

// Program status enum
typedef enum {
	PROG_INVALID,
	PROG_UNLINKED,
	PROG_LINKED
} prog_status;

// Uniform buffer object struct
typedef struct {
	const SceGxmProgramParameter *ptr;
	uint32_t bind;
	uint32_t idx;
	void *alias;
	void *chain;
} ubo;

// Program struct holding vertex/fragment shader info
typedef struct {
	shader *vshader;
	shader *fshader;
	uint8_t status;
	uint8_t max_frag_texunit_idx;
	uint8_t max_vert_texunit_idx;
	uniform *vert_texunits[TEXTURE_IMAGE_UNITS_NUM];
	uniform *frag_texunits[TEXTURE_IMAGE_UNITS_NUM];
	SceGxmVertexAttribute attr[VERTEX_ATTRIBS_NUM];
	SceGxmVertexStream stream[VERTEX_ATTRIBS_NUM];
	uint8_t attr_map[VERTEX_ATTRIBS_NUM];
	SceGxmVertexProgram *vprog;
	SceGxmFragmentProgram *fprog;
	blend_config blend_info;
	GLuint attr_num;
	GLuint attr_idx;
	GLuint stream_num;
	const SceGxmProgramParameter *wvp;
	uniform *vert_uniforms;
	uniform *frag_uniforms;
	ubo *vert_ubos;
	ubo *frag_ubos;
	GLuint attr_highest_idx;
	GLboolean has_unaligned_attrs;
	GLboolean is_fbo_float;
#ifdef HAVE_GLSL_TRANSLATOR
	uint8_t num_glsl_attr;
	attr_mapping *glsl_attr_map;
#endif
} program;

// Internal shaders and array
static shader shaders[MAX_CUSTOM_SHADERS];
static program progs[MAX_CUSTOM_PROGRAMS];

#ifdef HAVE_SHARK_LOG
static char *shark_log = NULL;
#endif

void release_shader(shader *s) {
	// Deallocating shader and unregistering it from sceGxmShaderPatcher
	if (s->valid) {
		sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, s->id);
		vgl_free((void *)s->prog);
		while (s->mat) {
			matrix_uniform *m = (matrix_uniform *)s->mat->chain;
			vgl_free(s->mat);
			s->mat = m;
		}
		while (s->unif_blk) {
			block_uniform *b = (block_uniform *)s->unif_blk->chain;
			vgl_free(s->unif_blk);
			s->unif_blk = b;
		}
#ifdef HAVE_SHARK_LOG
		if (s->log) {
			vgl_free(s->log);
			s->log = NULL;
		}
#endif
	}
	s->source = NULL;
	s->valid = GL_FALSE;
	s->dirty = GL_FALSE;
}

float *reserve_attrib_pool(uint8_t count) {
	float *res = cur_vao->vertex_attrib_pool_ptr;
	cur_vao->vertex_attrib_pool_ptr += count;
	if (cur_vao->vertex_attrib_pool_ptr > cur_vao->vertex_attrib_pool_limit) {
		cur_vao->vertex_attrib_pool_ptr = cur_vao->vertex_attrib_pool;
		return cur_vao->vertex_attrib_pool_ptr;
	}
	return res;
}

static inline __attribute__((always_inline)) GLenum gxm_vd_fmt_to_gl(SceGxmAttributeFormat fmt) {
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
		return GL_FLOAT;
	}
}

static inline __attribute__((always_inline)) GLenum gxm_attr_type_to_gl(uint8_t size, uint8_t num) {
	switch (size * num) {
	case 1:
		return GL_FLOAT;
	case 2:
		return GL_FLOAT_VEC2;
	case 3:
		return GL_FLOAT_VEC3;
	case 4:
		return GL_FLOAT_VEC4;
	case 9:
		return GL_FLOAT_MAT3;
	case 16:
		return GL_FLOAT_MAT4;
	default:
		return GL_FLOAT;
	}
}

static inline __attribute__((always_inline)) GLenum gxm_unif_type_to_gl(SceGxmParameterType type, uint8_t count, int *size) {
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
			return GL_FLOAT;
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
			return GL_INT;
		}
	default:
		return GL_FLOAT;
	}
}

static inline __attribute__((always_inline)) void gxm_unif_to_mat(GLenum *type, int *size) {
	switch (*type) {
	case GL_FLOAT_VEC2:
		*type = GL_FLOAT_MAT2;
		*size = *size / 2;
		break;
	case GL_FLOAT_VEC3:
		*type = GL_FLOAT_MAT3;
		*size = *size / 3;
		break;
	case GL_FLOAT_VEC4:
		*type = GL_FLOAT_MAT4;
		*size = *size / 4;
		break;
	default:
		break;
	}
}

static inline __attribute__((always_inline)) void compile_shader(shader *s, uint8_t skip_cache) {
#ifdef HAVE_SHADER_CACHE
	if (skip_cache)
		goto COMPILE_SHADER;
	char fname[256];
	sprintf(fname, "%s/%llX.gxp", vgl_shader_cache_path, XXH3_64bits(s->prog, s->size));
	FILE *f = fopen(fname, "rb");
	if (f) {
		if (s->source) {
			vgl_free(s->source);
			s->source = NULL;
		}
		fseek(f, 0, SEEK_END);
		s->size = ftell(f);
		fseek(f, 0, SEEK_SET);
		SceGxmProgram *res = (SceGxmProgram *)vglMalloc(s->size);
		fread(res, 1, s->size, f);
		fclose(f);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &s->id);
		s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
	} else {
#endif
COMPILE_SHADER:
		// Compiling shader source
		s->prog = shark_compile_shader_extended((const char *)s->prog, &s->size, s->type == GL_FRAGMENT_SHADER ? SHARK_FRAGMENT_SHADER : SHARK_VERTEX_SHADER, compiler_opts, compiler_fastmath, compiler_fastprecision, compiler_fastint);
		if (s->prog) {
			if (s->source) {
				vgl_free(s->source);
				s->source = NULL;
			}
			SceGxmProgram *res = (SceGxmProgram *)vglMalloc(s->size);
			vgl_fast_memcpy((void *)res, (void *)s->prog, s->size);
			int r = sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &s->id);
#ifdef LOG_ERRORS
			if (r)
				vgl_log("%s:%d %s: Program failed to register on sceGxm (%s).\n", __FILE__, __LINE__, __func__, get_gxm_error_literal(r));
#endif
			s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
			SceShaccCgCompileOutput *cout = (SceShaccCgCompileOutput *)shark_get_internal_compile_output();
			SceShaccCgParameter param = sceShaccCgGetFirstParameter(cout);
			while (param) {
				if (sceShaccCgGetParameterClass(param) == SCE_SHACCCG_PARAMETERCLASS_MATRIX) {
					matrix_uniform *m = (matrix_uniform *)vglMalloc(sizeof(matrix_uniform));
					m->ptr = sceGxmProgramFindParameterByName(s->prog, sceShaccCgGetParameterName(param));
					m->chain = s->mat;
					s->mat = m;
				} else if (sceShaccCgGetParameterClass(param) == SCE_SHACCCG_PARAMETERCLASS_UNIFORMBLOCK) {
					block_uniform *b = (block_uniform *)vglMalloc(sizeof(block_uniform));
					b->idx = sceShaccCgGetParameterBufferIndex(param);
					strcpy(b->name, sceShaccCgGetParameterName(param));
					b->chain = s->unif_blk;
					s->unif_blk = b;
				}
				param = sceShaccCgGetNextParameter(param);
			}
		}
#ifdef HAVE_SHARK_LOG
		if (s->log)
			vgl_free(s->log);
		s->log = shark_log;
		shark_log = NULL;
#endif
		shark_clear_output();
#ifdef HAVE_SHADER_CACHE
		if (!skip_cache) {
			f = fopen(fname, "wb");
			fwrite(s->prog, 1, s->size, f);
			fclose(f);
		}
	}
#endif
}

void resetCustomShaders(void) {
	// Init custom shaders
	for (int i = 0; i < MAX_CUSTOM_SHADERS; i++) {
		shaders[i].valid = GL_FALSE;
#ifdef HAVE_SHARK_LOG
		shaders[i].log = NULL;
#endif
		shaders[i].source = NULL;
	}

	// Init custom programs
	for (int i = 0; i < MAX_CUSTOM_PROGRAMS; i++) {
		progs[i].status = PROG_INVALID;
	}
}

GLboolean _glDrawArrays_CustomShadersIMPL(GLint first, GLsizei count) {
	program *p = &progs[cur_program - 1];

	// Check if a blend info rebuild is required and upload fragment program
	setupFragProgram();

	// Uploading fragment textures on relative texture units
	for (int i = 0; i < p->max_frag_texunit_idx; i++) {
#ifndef SAMPLERS_SPEEDHACK
		if (p->frag_texunits[i]) {
#endif
			texture_unit *tex_unit = &texture_units[(int)p->frag_texunits[i]->data];
			uint8_t tex_type = p->frag_texunits[i]->size ? 2 : tex2d_override;
			texture *tex = &texture_slots[tex_unit->tex_id[tex_type]];
#ifdef HAVE_TEX_CACHE
			restoreTexCache(tex);
#endif
#ifndef SKIP_ERROR_HANDLING
			int r = sceGxmTextureValidate(&tex->gxm_tex);
			if (r) {
				vgl_log("%s:%d glDrawArrays: Fragment %s texture on TEXUNIT%d is invalid (%s), draw will be skipped.\n", __FILE__, __LINE__, tex_type ? "cube" : "2D", i, get_gxm_error_literal(r));
				return GL_FALSE;
			}
#endif
#ifndef TEXTURES_SPEEDHACK
			tex->last_frame = vgl_framecount;
#endif
			sampler *smp = samplers[(int)p->frag_texunits[i]->data];
			if (smp) {
				vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, smp->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
				vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
				vglSetTexLodBias(&tex->gxm_tex, smp->lod_bias);
				tex->overridden = GL_TRUE;
			} else if (tex->overridden) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
				vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
				tex->overridden = GL_FALSE;
			}
			sceGxmSetFragmentTexture(gxm_context, i, &tex->gxm_tex);
#ifndef SAMPLERS_SPEEDHACK
		}
#endif
	}

	// Uploading vertex textures on relative texture units
	for (int i = 0; i < p->max_vert_texunit_idx; i++) {
#ifndef SAMPLERS_SPEEDHACK
		if (p->vert_texunits[i]) {
#endif
			texture_unit *tex_unit = &texture_units[(int)p->vert_texunits[i]->data];
			uint8_t tex_type = p->vert_texunits[i]->size ? 2 : tex2d_override;
			texture *tex = &texture_slots[tex_unit->tex_id[tex_type]];
#ifndef SKIP_ERROR_HANDLING
			int r = sceGxmTextureValidate(&tex->gxm_tex);
			if (r) {
				vgl_log("%s:%d glDrawArrays: Vertex %s texture on TEXUNIT%d is invalid (%s), draw will be skipped.\n", __FILE__, __LINE__, tex_type ? "cube" : "2D", i, get_gxm_error_literal(r));
				return GL_FALSE;
			}
#endif
#ifndef TEXTURES_SPEEDHACK
			tex->last_frame = vgl_framecount;
#endif
			sampler *smp = samplers[(int)p->vert_texunits[i]->data];
			if (smp) {
				vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, smp->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
				vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_TRUE;
			} else if (tex->overridden) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_FALSE;
			}
			sceGxmSetVertexTexture(gxm_context, i, &tex->gxm_tex);
#ifndef SAMPLERS_SPEEDHACK
		}
#endif
	}

	// Aligning attributes
	SceGxmVertexAttribute *attributes;
	SceGxmVertexStream *streams;
	alignAttributes(attributes, streams);

	void *ptrs[VERTEX_ATTRIBS_NUM];
#ifndef DRAW_SPEEDHACK
#ifdef STRICT_DRAW_COMPLIANCE
	GLboolean is_packed[VERTEX_ATTRIBS_NUM];
	sceClibMemset(is_packed, GL_TRUE, p->attr_num * sizeof(GLboolean));
#else
	GLboolean is_packed = p->attr_num > 1;
	if (is_packed) {
#endif
		for (int i = 0; i < p->attr_num; i++) {
			uint8_t attr_idx = p->attr_map[i];
			if (cur_vao->vertex_attrib_vbo[attr_idx]) {
#ifdef STRICT_DRAW_COMPLIANCE
				sceClibMemset(is_packed, 0, p->attr_num * sizeof(GLboolean));
#else
				is_packed = GL_FALSE;
#endif
				break;
#ifdef STRICT_DRAW_COMPLIANCE
			} else {
				if (!(cur_vao->vertex_attrib_offsets[p->attr_map[0]] + streams[0].stride > cur_vao->vertex_attrib_offsets[attr_idx] && cur_vao->vertex_attrib_offsets[attr_idx] >= cur_vao->vertex_attrib_offsets[p->attr_map[0]])) {
					is_packed[attr_idx] = GL_FALSE;
				}
#endif
			}
		}
#ifndef STRICT_DRAW_COMPLIANCE
		if (is_packed && (!(cur_vao->vertex_attrib_offsets[p->attr_map[0]] + streams[0].stride > cur_vao->vertex_attrib_offsets[p->attr_map[1]] && cur_vao->vertex_attrib_offsets[p->attr_map[1]] > cur_vao->vertex_attrib_offsets[p->attr_map[0]])))
			is_packed = GL_FALSE;
	}
#endif
#ifdef STRICT_DRAW_COMPLIANCE
	// Gathering real attribute data pointers
	if (is_packed[0]) {
		ptrs[0] = gpu_alloc_mapped_temp(count * streams[0].stride);
		vgl_fast_memcpy(ptrs[0], (void *)cur_vao->vertex_attrib_offsets[p->attr_map[0]] + first * streams[0].stride, count * streams[0].stride);
	}
	for (int i = 0; i < p->attr_num; i++) {
		uint8_t attr_idx = p->attr_map[i];
		attributes[i].regIndex = p->attr[attr_idx].regIndex;
		if (is_packed[i]) {
			handlePackedAttrib();
		} else {
			handleUnpackedAttrib(first, count);
		}
	}
#else
	// Gathering real attribute data pointers
	if (is_packed) {
		ptrs[0] = gpu_alloc_mapped_temp(count * streams[0].stride);
		vgl_fast_memcpy(ptrs[0], (void *)cur_vao->vertex_attrib_offsets[p->attr_map[0]] + first * streams[0].stride, count * streams[0].stride);
		for (int i = 0; i < p->attr_num; i++) {
			uint8_t attr_idx = p->attr_map[i];
			attributes[i].regIndex = p->attr[attr_idx].regIndex;
			handlePackedAttrib();
		}
	} else {
		for (int i = 0; i < p->attr_num; i++) {
			uint8_t attr_idx = p->attr_map[i];
			attributes[i].regIndex = p->attr[attr_idx].regIndex;
			handleUnpackedAttrib(first, count);
		}
	}
#endif
#else // DRAW_SPEEDHACK
	handleSpeedhackAttrib();
#endif
	// Uploading new vertex program
	patchVertexProgram(gxm_shader_patcher, p->vshader->id, attributes, p->attr_num, streams, p->attr_num, &p->vprog);
	sceGxmSetVertexProgram(gxm_context, p->vprog);

	// Uploading both fragment and vertex uniforms data
	uploadUniforms();

	// Uploading vertex streams
	for (int i = 0; i < p->attr_num; i++) {
		uint8_t attr_idx = p->attr_map[i];
		GLboolean is_active = (cur_vao->vertex_attrib_state & (1 << attr_idx)) ? GL_TRUE : GL_FALSE;
		if (is_active) {
#ifdef DRAW_SPEEDHACK
			sceGxmSetVertexStream(gxm_context, i, ptrs[i]);
#else
#ifdef STRICT_DRAW_COMPLIANCE
			sceGxmSetVertexStream(gxm_context, i, is_packed[i] ? ptrs[0] : ptrs[i]);
#else
			sceGxmSetVertexStream(gxm_context, i, is_packed ? ptrs[0] : ptrs[i]);
#endif
#endif
		} else {
			sceGxmSetVertexStream(gxm_context, i, cur_vao->vertex_attrib_value[attr_idx]);
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

GLboolean _glDrawElements_CustomShadersIMPL(uint16_t *idx_buf, GLsizei count, uint32_t top_idx, GLboolean is_short) {
	program *p = &progs[cur_program - 1];

	// Check if a blend info rebuild is required and upload fragment program
	setupFragProgram();

	// Uploading fragment textures on relative texture units
	for (int i = 0; i < p->max_frag_texunit_idx; i++) {
#ifndef SAMPLERS_SPEEDHACK
		if (p->frag_texunits[i]) {
#endif
			texture_unit *tex_unit = &texture_units[(int)p->frag_texunits[i]->data];
			uint8_t tex_type = p->frag_texunits[i]->size ? 2 : tex2d_override;
			texture *tex = &texture_slots[tex_unit->tex_id[tex_type]];
#ifdef HAVE_TEX_CACHE
			restoreTexCache(tex);
#endif
#ifndef SKIP_ERROR_HANDLING
			int r = sceGxmTextureValidate(&tex->gxm_tex);
			if (r) {
				vgl_log("%s:%d glDrawElements: Fragment %s texture on TEXUNIT%d is invalid (%s), draw will be skipped.\n", __FILE__, __LINE__, tex_type ? "cube" : "2D", i, get_gxm_error_literal(r));
				return GL_FALSE;
			}
#endif
#ifndef TEXTURES_SPEEDHACK
			tex->last_frame = vgl_framecount;
#endif
			sampler *smp = samplers[(int)p->frag_texunits[i]->data];
			if (smp) {
				vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, smp->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
				vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_TRUE;
			} else if (tex->overridden) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_FALSE;
			}
			sceGxmSetFragmentTexture(gxm_context, i, &tex->gxm_tex);
#ifndef SAMPLERS_SPEEDHACK
		}
#endif
	}

	// Uploading vertex textures on relative texture units
	for (int i = 0; i < p->max_vert_texunit_idx; i++) {
#ifndef SAMPLERS_SPEEDHACK
		if (p->vert_texunits[i]) {
#endif
			texture_unit *tex_unit = &texture_units[(int)p->vert_texunits[i]->data];
			uint8_t tex_type = p->vert_texunits[i]->size ? 2 : tex2d_override;
			texture *tex = &texture_slots[tex_unit->tex_id[tex_type]];
#ifndef SKIP_ERROR_HANDLING
			int r = sceGxmTextureValidate(&tex->gxm_tex);
			if (r) {
				vgl_log("%s:%d glDrawElements: Vertex %s texture on TEXUNIT%d is invalid (%s), draw will be skipped.\n", __FILE__, __LINE__, tex_type ? "cube" : "2D", i, get_gxm_error_literal(r));
				return GL_FALSE;
			}
#endif
#ifndef TEXTURES_SPEEDHACK
			tex->last_frame = vgl_framecount;
#endif
			sampler *smp = samplers[(int)p->vert_texunits[i]->data];
			if (smp) {
				vglSetTexMinFilter(&tex->gxm_tex, smp->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, smp->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, smp->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, smp->u_mode);
				vglSetTexVMode(&tex->gxm_tex, smp->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, smp->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_TRUE;
			} else if (tex->overridden) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
				tex->overridden = GL_FALSE;
			}
			sceGxmSetVertexTexture(gxm_context, i, &tex->gxm_tex);
#ifndef SAMPLERS_SPEEDHACK
		}
#endif
	}

	// Aligning attributes
	SceGxmVertexAttribute *attributes;
	SceGxmVertexStream *streams;
	alignAttributes(attributes, streams);

	void *ptrs[VERTEX_ATTRIBS_NUM];
#ifndef DRAW_SPEEDHACK
	GLboolean is_full_vbo = GL_TRUE;
#ifdef STRICT_DRAW_COMPLIANCE
	GLboolean is_packed[VERTEX_ATTRIBS_NUM];
	sceClibMemset(is_packed, GL_TRUE, p->attr_num * sizeof(GLboolean));
#else
	GLboolean is_packed = p->attr_num > 1;
	if (is_packed) {
#endif
		for (int i = 0; i < p->attr_num; i++) {
			uint8_t attr_idx = p->attr_map[i];
			if (cur_vao->vertex_attrib_vbo[attr_idx]) {
#ifdef STRICT_DRAW_COMPLIANCE
				sceClibMemset(is_packed, 0, p->attr_num * sizeof(GLboolean));
#else
				is_packed = GL_FALSE;
#endif
			} else {
#ifdef STRICT_DRAW_COMPLIANCE
				if (!(cur_vao->vertex_attrib_offsets[p->attr_map[0]] + streams[0].stride > cur_vao->vertex_attrib_offsets[attr_idx] && cur_vao->vertex_attrib_offsets[attr_idx] >= cur_vao->vertex_attrib_offsets[p->attr_map[0]])) {
					is_packed[attr_idx] = GL_FALSE;
				}
#endif
				is_full_vbo = GL_FALSE;
			}
		}
#ifndef STRICT_DRAW_COMPLIANCE
		if (is_packed && (!(cur_vao->vertex_attrib_offsets[p->attr_map[0]] + streams[0].stride > cur_vao->vertex_attrib_offsets[p->attr_map[1]] && cur_vao->vertex_attrib_offsets[p->attr_map[1]] > cur_vao->vertex_attrib_offsets[p->attr_map[0]])))
			is_packed = GL_FALSE;
	} else if (!cur_vao->vertex_attrib_vbo[p->attr_map[0]])
		is_full_vbo = GL_FALSE;
#endif

	// Detecting highest index value
	if (!is_full_vbo && !top_idx) {
		if (is_short) {
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
		top_idx++;
	}

#ifdef STRICT_DRAW_COMPLIANCE
	// Gathering real attribute data pointers
	if (is_packed[0]) {
		ptrs[0] = gpu_alloc_mapped_temp(top_idx * streams[0].stride);
		vgl_fast_memcpy(ptrs[0], (void *)cur_vao->vertex_attrib_offsets[p->attr_map[0]], top_idx * streams[0].stride);
	}
	for (int i = 0; i < p->attr_num; i++) {
		uint8_t attr_idx = p->attr_map[i];
		attributes[i].regIndex = p->attr[attr_idx].regIndex;
		if (is_packed[i]) {
			handlePackedAttrib();
		} else {
			handleUnpackedAttrib(0, top_idx);
		}
	}
#else
	// Gathering real attribute data pointers
	if (is_packed) {
		ptrs[0] = gpu_alloc_mapped_temp(top_idx * streams[0].stride);
		vgl_fast_memcpy(ptrs[0], (void *)cur_vao->vertex_attrib_offsets[p->attr_map[0]], top_idx * streams[0].stride);
		for (int i = 0; i < p->attr_num; i++) {
			uint8_t attr_idx = p->attr_map[i];
			attributes[i].regIndex = p->attr[attr_idx].regIndex;
			handlePackedAttrib();
		}
	} else {
		for (int i = 0; i < p->attr_num; i++) {
			uint8_t attr_idx = p->attr_map[i];
			attributes[i].regIndex = p->attr[attr_idx].regIndex;
			handleUnpackedAttrib(0, top_idx);
		}
	}
#endif
#else // DRAW_SPEEDHACK
	handleSpeedhackAttrib();
#endif
	// Uploading new vertex program
	patchVertexProgram(gxm_shader_patcher, p->vshader->id, attributes, p->attr_num, streams, p->attr_num, &p->vprog);
	sceGxmSetVertexProgram(gxm_context, p->vprog);

	// Uploading both fragment and vertex uniforms data
	uploadUniforms();

	// Uploading vertex streams
	for (int i = 0; i < p->attr_num; i++) {
		uint8_t attr_idx = p->attr_map[i];
		GLboolean is_active = (cur_vao->vertex_attrib_state & (1 << attr_idx)) ? GL_TRUE : GL_FALSE;
		if (is_active) {
#ifdef DRAW_SPEEDHACK
			sceGxmSetVertexStream(gxm_context, i, ptrs[i]);
#else
#ifdef STRICT_DRAW_COMPLIANCE
			sceGxmSetVertexStream(gxm_context, i, is_packed[i] ? ptrs[0] : ptrs[i]);
#else
			sceGxmSetVertexStream(gxm_context, i, is_packed ? ptrs[0] : ptrs[i]);
#endif
#endif
		} else {
			sceGxmSetVertexStream(gxm_context, i, cur_vao->vertex_attrib_value[attr_idx]);
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
	setupFragProgram();

	// Setting up required vertex shader
	sceGxmSetVertexProgram(gxm_context, p->vprog);

	// Uploading both fragment and vertex uniforms data
	uploadUniforms();

	// Uploading textures on relative texture units
	for (int i = 0; i < p->max_frag_texunit_idx; i++) {
#ifndef SAMPLERS_SPEEDHACK
		if (p->frag_texunits[i]) {
#endif
			texture_unit *tex_unit = &texture_units[i];
			sceGxmSetFragmentTexture(gxm_context, i, &texture_slots[tex_unit->tex_id[0]].gxm_tex);
#ifndef SAMPLERS_SPEEDHACK
		}
#endif
	}
}

#ifdef HAVE_SHARK_LOG
void shark_log_cb(const char *msg, shark_log_level msg_level, int line) {
	char newline[1024];
	GLboolean is_extra_line = shark_log ? GL_TRUE : GL_FALSE;
	switch (msg_level) {
	case SHARK_LOG_INFO:
		sprintf(newline, "%sI] %s on line %d.", is_extra_line ? "\n" : "", msg, line);
#ifdef LOG_ERRORS
		vgl_log("Shader Compiler: I] %s on line %d.\n", msg, line);
#endif
		break;
	case SHARK_LOG_WARNING:
		sprintf(newline, "%sW] %s on line %d.", is_extra_line ? "\n" : "", msg, line);
#ifdef LOG_ERRORS
		vgl_log("Shader Compiler: W] %s on line %d.\n", msg, line);
#endif
		break;
	case SHARK_LOG_ERROR:
		sprintf(newline, "%sE] %s on line %d.", is_extra_line ? "\n" : "", msg, line);
#ifdef LOG_ERRORS
		vgl_log("Shader Compiler: E] %s on line %d.\n", msg, line);
#endif
		break;
	}
	uint32_t size = (is_extra_line ? strlen(shark_log) : 0) + strlen(newline);
	shark_log = shark_log ? vglRealloc(shark_log, size + 1) : vglMalloc(size + 1);
	if (is_extra_line)
		strcat(shark_log, newline);
	else
		strcpy(shark_log, newline);
}
#elif defined(LOG_ERRORS)
void shark_log_cb(const char *msg, shark_log_level msg_level, int line) {
	switch (msg_level) {
	case SHARK_LOG_INFO:
		vgl_log("Shader Compiler: I] %s on line %d.\n", msg, line);
		break;
	case SHARK_LOG_WARNING:
		vgl_log("Shader Compiler: W] %s on line %d.\n", msg, line);
		break;
	case SHARK_LOG_ERROR:
		vgl_log("Shader Compiler: E] %s on line %d.\n", msg, line);
		break;
	}
}
#endif

float *getUniformAliasDataPtr(uniform *u, const char *name, uint32_t size) {
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

ubo *hasBlockAlias(ubo *u, const char *name) {
	while (u) {
		if (!strcmp(name, ((block_uniform*)u->ptr)->name)) {
			return u;
		}
		u = u->chain;
	}
	return NULL;
}

block_uniform *getBlockDetails(block_uniform *b, uint8_t idx) {
	while (b) {
		if (b->idx == idx)
			return b;
		b = (block_uniform *)b->chain;
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
	if (res == 0) {
		vgl_log("%s:%d %s: Out of shaders handles. Consider increasing MAX_CUSTOM_SHADERS...\n", __FILE__, __LINE__, __func__);
		return res;
	}

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
	shaders[res - 1].mat = NULL;
	shaders[res - 1].unif_blk = NULL;
	shaders[res - 1].valid = GL_TRUE;
#ifdef HAVE_GLSL_TRANSLATOR
	shaders[res - 1].glsl_source = NULL;
#endif
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
	case GL_DELETE_STATUS:
		*params = s->dirty ? GL_TRUE : GL_FALSE;
		break;
	case GL_INFO_LOG_LENGTH:
#ifdef HAVE_SHARK_LOG
		*params = s->log ? (strlen(s->log) + 1) : 0;
#else
		*params = 0;
#endif
		break;
	case GL_SHADER_SOURCE_LENGTH:
		*params = s->source ? (strlen(s->source) + 1) : 0;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glGetShaderInfoLog(GLuint handle, GLsizei maxLength, GLsizei *length, GLchar *infoLog) {
#ifndef SKIP_ERROR_HANDLING
	if (maxLength < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	GLsizei len = 0;
#ifdef HAVE_SHARK_LOG
	shader *s = &shaders[handle - 1];
	if (s->log) {
		len = min(strlen(s->log), maxLength - 1);
		vgl_fast_memcpy(infoLog, s->log, len);
		infoLog[len] = 0;
	}
#endif
	if (length)
		*length = len;
}

void glGetShaderSource(GLuint handle, GLsizei bufSize, GLsizei *length, GLchar *source) {
#ifndef SKIP_ERROR_HANDLING
	if (bufSize < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed shader
	shader *s = &shaders[handle - 1];

	GLsizei size = 0;
	if (s->source) {
		GLsizei src_len = strlen(s->source);
		if (bufSize <= src_len)
			src_len = bufSize - 1;
		strncpy(source, s->source, src_len);
		size = src_len;
	}
	if (length)
		*length = size;
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

#if defined(SHADER_COMPILER_SPEEDHACK) && !defined(HAVE_GLSL_TRANSLATOR)
	if (count == 1)
		s->prog = (SceGxmProgram *)*string;
	else
#endif
	{
		s->source = (char *)vglMalloc(size);
		s->source[0] = 0;

		for (int i = 0; i < count; i++) {
			strncat(s->source, string[i], length ? length[i] : strlen(string[i]));
		}
		s->prog = (SceGxmProgram *)s->source;
	}
	
	s->size = size - 1;
}

void glShaderBinary(GLsizei count, const GLuint *handles, GLenum binaryFormat, const void *binary, GLsizei length) {
	// Grabbing passed shader
	shader *s = &shaders[handles[0] - 1];

	// Allocating compiled shader on RAM and registering it into sceGxmShaderPatcher
	uint8_t *ubin = (uint8_t *)binary;
	if (ubin[0] == 'G' && ubin[1] == 'X' && ubin[2] == 'P') { // Standard GXP file
		s->prog = (SceGxmProgram *)vglMalloc(length);
		vgl_fast_memcpy((void *)s->prog, binary, length);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, s->prog, &s->id);
		s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
	} else { // GXP program with matrix uniforms table
		uint32_t *ubin32 = (uint32_t *)binary;
		GLsizei bin_len = length - (ubin32[0] + 1) * sizeof(uint32_t);
		s->prog = (SceGxmProgram *)vglMalloc(bin_len);
		vgl_fast_memcpy((void *)s->prog, &ubin32[ubin32[0] + 1], bin_len);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, s->prog, &s->id);
		s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
		for (uint32_t i = 1; i <= ubin32[0]; i++) {
			matrix_uniform *m = (matrix_uniform *)vglMalloc(sizeof(matrix_uniform));
			m->ptr = sceGxmProgramGetParameter(s->prog, ubin32[i]);
			m->chain = s->mat;
			s->mat = m;
		}
	}
}

void glCompileShader(GLuint handle) {
	// If vitaShaRK is not enabled, we try to initialize it
	if (!is_shark_online && !startShaderCompiler()) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
	
#ifdef HAVE_GLSL_TRANSLATOR
	// If we use VGL_MODE_POSTPONED, we compile shaders in glLinkProgram
	if (glsl_sema_mode == VGL_MODE_POSTPONED)
		return;
#endif

	// Grabbing passed shader
	shader *s = &shaders[handle - 1];
	
#ifdef HAVE_GLSL_TRANSLATOR
#ifdef HAVE_SHADER_CACHE
	char fname[256];
	sprintf(fname, "%s/%llX.gxp", vgl_shader_cache_path, XXH3_64bits(s->prog, s->size));
	FILE *f = fopen(fname, "rb");
	if (f) {
		if (s->source) {
			vgl_free(s->source);
			s->source = NULL;
		}
		fseek(f, 0, SEEK_END);
		s->size = ftell(f);
		fseek(f, 0, SEEK_SET);
		SceGxmProgram *res = (SceGxmProgram *)vglMalloc(s->size);
		fread(res, 1, s->size, f);
		fclose(f);
		sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &s->id);
		s->prog = sceGxmShaderPatcherGetProgramFromId(s->id);
		return;
	}
#endif
	s->glsl_source = s->source;
	glsl_translator_process(s, 1, &s->glsl_source, NULL);
#endif
	compile_shader(s, GL_FALSE);
#ifdef HAVE_GLSL_TRANSLATOR
	vgl_free(s->glsl_source);
	s->glsl_source = NULL;
#ifdef HAVE_SHADER_CACHE
	f = fopen(fname, "wb");
	fwrite(s->prog, 1, s->size, f);
	fclose(f);
#endif
#endif
}

void glDeleteShader(GLuint shad) {
	// Grabbing passed shader
	shader *s = &shaders[shad - 1];

	// If the shader is attached to any program, we only mark it for deletion
	if (s->ref_counter > 0)
		s->dirty = GL_TRUE;
	else
		release_shader(s);
	
#ifdef HAVE_GLSL_TRANSLATOR
	if (s->glsl_source) {
		vgl_free(s->glsl_source);
	}
#endif
}

void glAttachShader(GLuint prog, GLuint shad) {
	// Grabbing passed shader and program
	shader *s = &shaders[shad - 1];
	program *p = &progs[prog - 1];
	
	// Attaching shader to desired program
	if (p->status == PROG_UNLINKED && s->valid) {
		switch (s->type) {
		case GL_VERTEX_SHADER:
			s->ref_counter++;
			p->vshader = s;
#ifdef HAVE_GLSL_TRANSLATOR
			// If we use VGL_MODE_POSTPONED, we perform attributes binding in glLinkProgram
			if (glsl_sema_mode != VGL_MODE_POSTPONED) {
#endif
				// Setting progressive default attribute bindings
				setDefaultAttribBindings();
#ifdef HAVE_GLSL_TRANSLATOR			
			}
#endif
			break;
		case GL_FRAGMENT_SHADER:
			s->ref_counter++;
			p->fshader = s;
			break;
		default:
			break;
		}
	} else {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
}

void glGetAttachedShaders(GLuint prog, GLsizei maxCount, GLsizei *count, GLuint *shads) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

#ifndef SKIP_ERROR_HANDLING
	if (maxCount < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, maxCount)
	}
#endif

	// Returning attached shaders
	GLuint shad = 1;
	*count = 0;
	if (p->vshader) {
		for (int i = 1; i <= MAX_CUSTOM_SHADERS; i++) {
			if (p->vshader == &shaders[i - 1]) {
				shad = i;
				break;
			}
		}
		shads[0] = shad;
		*count = 1;
	}
	if (p->fshader) {
		for (int i = 1; i <= MAX_CUSTOM_SHADERS; i++) {
			if (p->fshader == &shaders[i - 1]) {
				shad = i;
				break;
			}
		}
		shads[*count] = shad;
		*count = *count + 1;
	}
}

GLuint glCreateProgram(void) {
	// Looking for a free program slot
	GLuint i, j, res = 0xFFFFFFFF;
	for (i = 1; i <= MAX_CUSTOM_PROGRAMS; i++) {
		// Program slot found, reserving and initializing it
		if (!(progs[i - 1].status)) {
			res = i--;
			progs[i].status = PROG_UNLINKED;
			progs[i].attr_num = 0;
			progs[i].stream_num = 0;
			progs[i].attr_idx = 0;
			progs[i].max_frag_texunit_idx = 0;
			progs[i].max_vert_texunit_idx = 0;
			progs[i].wvp = NULL;
			progs[i].vshader = NULL;
			progs[i].fshader = NULL;
			progs[i].vert_uniforms = NULL;
			progs[i].frag_uniforms = NULL;
			progs[i].vert_ubos = NULL;
			progs[i].frag_ubos = NULL;
			progs[i].attr_highest_idx = 0;
#ifdef HAVE_GLSL_TRANSLATOR
			progs[i].num_glsl_attr = 0;
			progs[i].glsl_attr_map = NULL;
#endif
			progs[i].is_fbo_float = 0xFF;
			for (j = 0; j < VERTEX_ATTRIBS_NUM; j++) {
				progs[i].attr[j].regIndex = 0xDEAD;
			}
			break;
		}
	}
#ifndef SKIP_ERROR_HANDLING
	if (res == 0xFFFFFFFF) {
		vgl_log("%s:%d %s: Out of programs handles. Consider increasing MAX_CUSTOM_PROGRAMS...\n", __FILE__, __LINE__, __func__);
		return 0;
	}
#endif
	return res;
}

GLboolean glIsProgram(GLuint i) {
	if (progs[i - 1].status != PROG_INVALID)
		return GL_TRUE;
	return GL_FALSE;
}

void glGetProgramBinary(GLuint prog, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) {
#ifndef SKIP_ERROR_HANDLING
	if (bufSize < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Saving info related to bound attributes locations
	GLuint *b = (GLuint *)binary;
	b[0] = p->attr_highest_idx;
	sceClibMemcpy(&b[1], p->attr, sizeof(SceGxmVertexAttribute) * VERTEX_ATTRIBS_NUM);
	GLsizei size = sizeof(GLuint) + sizeof(SceGxmVertexAttribute) * VERTEX_ATTRIBS_NUM;

	// Dumping vertex binary
	uint32_t matrix_uniforms_num = 0;
	matrix_uniform *m = p->vshader->mat;
	uint32_t *ubin = (uint32_t *)((uint8_t *)binary + size);
	while (m) {
		matrix_uniforms_num++;
		ubin[matrix_uniforms_num + 1] = sceGxmProgramParameterGetIndex(p->vshader->prog, m->ptr);
		m = (matrix_uniform *)m->chain;
	}
	ubin[1] = matrix_uniforms_num;
	GLsizei bin_len = sceGxmProgramGetSize(p->vshader->prog);
	ubin[0] = bin_len + (matrix_uniforms_num + 1) * sizeof(uint32_t);
	vgl_fast_memcpy(&ubin[matrix_uniforms_num + 2], p->vshader->prog, bin_len);
	size += bin_len + sizeof(uint32_t) * (2 + matrix_uniforms_num);
	
	// Dumping fragment binary
	matrix_uniforms_num = 0;
	m = p->fshader->mat;
	ubin = (uint32_t *)((uint8_t *)binary + size);
	while (m) {
		matrix_uniforms_num++;
		ubin[matrix_uniforms_num + 1] = sceGxmProgramParameterGetIndex(p->fshader->prog, m->ptr);
		m = (matrix_uniform *)m->chain;
	}
	ubin[1] = matrix_uniforms_num;
	bin_len = sceGxmProgramGetSize(p->fshader->prog);
	ubin[0] = bin_len + (matrix_uniforms_num + 1) * sizeof(uint32_t);
	vgl_fast_memcpy(&ubin[matrix_uniforms_num + 2], p->fshader->prog, bin_len);
	size += bin_len + sizeof(uint32_t) * (2 + matrix_uniforms_num);

	if (length)
		*length = size;
}

void glProgramBinary(GLuint prog, GLenum binaryFormat, const void *binary, GLsizei length) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Restoring bound attributes info
	GLuint *b = (GLuint *)binary;
	p->attr_highest_idx = b[0];
	sceClibMemcpy(p->attr, &b[1], sizeof(SceGxmVertexAttribute) * VERTEX_ATTRIBS_NUM);
	GLsizei size = sizeof(GLuint) + sizeof(SceGxmVertexAttribute) * VERTEX_ATTRIBS_NUM;

	// Restoring shaders
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	uint32_t *sizeptr = (uint32_t *)((uint8_t *)binary + size);
	glShaderBinary(1, &vs, 0, &sizeptr[1], sizeptr[0]);
	sizeptr = (uint32_t *)((uint8_t *)binary + size + sizeptr[0] + sizeof(uint32_t));
	glShaderBinary(1, &fs, 0, &sizeptr[1], sizeptr[0]);
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);

	// Linking program and marking for deletion temporary shaders
	glLinkProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);
}

void glDeleteProgram(GLuint prog) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Releasing both vertex and fragment programs from sceGxmShaderPatcher
	if (p->status) {
		sceGxmFinish(gxm_context);
		while (p->vert_uniforms) {
			uniform *old = p->vert_uniforms;
			p->vert_uniforms = (uniform *)p->vert_uniforms->chain;
			if (old->size != 0xFFFFFFFF && old->size != 0 && !(old->is_fragment && old->is_vertex))
				vgl_free(old->data);
			vgl_free(old);
		}
		while (p->frag_uniforms) {
			uniform *old = p->frag_uniforms;
			p->frag_uniforms = (uniform *)p->frag_uniforms->chain;
			if (old->size != 0xFFFFFFFF && old->size != 0)
				vgl_free(old->data);
			vgl_free(old);
		}
		while (p->vert_ubos) {
			ubo *old = p->vert_ubos;
			p->vert_ubos = (ubo *)p->vert_ubos->chain;
			vgl_free(old);
		}
		while (p->frag_ubos) {
			ubo *old = p->frag_ubos;
			p->frag_ubos = (ubo *)p->frag_ubos->chain;
			vgl_free(old);
		}

		// Checking if attached shaders are marked for deletion and should be deleted
		if (p->vshader) {
			p->vshader->ref_counter--;
			if (p->vshader->dirty && p->vshader->ref_counter == 0)
				release_shader(p->vshader);
		}
		if (p->fshader) {
			p->fshader->ref_counter--;
			if (p->fshader->dirty && p->fshader->ref_counter == 0)
				release_shader(p->fshader);
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
	uniform *u;
	matrix_uniform *m;
	int matrix_uniform_num = 0;

	switch (pname) {
	case GL_LINK_STATUS:
	case GL_VALIDATE_STATUS:
		*params = p->status == PROG_LINKED;
		break;
	case GL_INFO_LOG_LENGTH:
		*params = 0;
		break;
	case GL_PROGRAM_BINARY_LENGTH:
		m = p->vshader->mat;
		while (m) {
			matrix_uniform_num++;
			m = (matrix_uniform *)m->chain;
		}
		m = p->fshader->mat;
		while (m) {
			matrix_uniform_num++;
			m = (matrix_uniform *)m->chain;
		}
		*params = sceGxmProgramGetSize(p->vshader->prog) + sceGxmProgramGetSize(p->fshader->prog) + sizeof(uint32_t) * 2 + sizeof(SceGxmVertexAttribute) * VERTEX_ATTRIBS_NUM + sizeof(GLuint) + (matrix_uniform_num + 2) * sizeof(GLuint);
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
			const SceGxmProgramParameter *param = sceGxmProgramGetParameter(p->vshader->prog, cnt);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glLinkProgram(GLuint progr) {
	// Grabbing passed program
	program *p = &progs[progr - 1];
#ifndef SKIP_ERROR_HANDLING
	if (!p->fshader->prog || !p->vshader->prog) {
		vgl_log("%s:%d: %s: %s shader is missing.\n", __FILE__, __LINE__, __func__, p->fshader->prog ? "fragment" : "vertex");
		return;
	}
#endif

#ifdef HAVE_GLSL_TRANSLATOR
	// With VGL_MODE_POSTPONED we perform shaders translation+compilation and attributes binding prior actual program linking
	if (glsl_sema_mode == VGL_MODE_POSTPONED) {
		glsl_sema_mode = VGL_MODE_SHADER_PAIR;
		if (!p->vshader->glsl_source)
			p->vshader->glsl_source = p->vshader->source;
#ifdef HAVE_SHADER_CACHE
		char fname[256];
		sprintf(fname, "%s/%llX.gxp", vgl_shader_cache_path, XXH3_64bits(p->vshader->glsl_source, strlen(p->vshader->glsl_source)));
		FILE *f = fopen(fname, "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			p->vshader->size = ftell(f);
			fseek(f, 0, SEEK_SET);
			SceGxmProgram *res = (SceGxmProgram *)vglMalloc(p->vshader->size);
			fread(res, 1, p->vshader->size, f);
			fclose(f);
			sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &p->vshader->id);
			p->vshader->prog = sceGxmShaderPatcherGetProgramFromId(p->vshader->id);
		} else {
#endif
			glsl_translator_process(p->vshader, 1, &p->vshader->glsl_source, NULL);
			compile_shader(p->vshader, GL_TRUE);
#ifdef HAVE_SHADER_CACHE
			f = fopen(fname, "wb");
			fwrite(p->vshader->prog, 1, p->vshader->size, f);
			fclose(f);
		}
#endif
		if (!p->fshader->glsl_source)
			p->fshader->glsl_source = p->fshader->source;
#ifdef HAVE_SHADER_CACHE
		sprintf(fname, "%s/%llX.gxp", vgl_shader_cache_path, XXH3_64bits(p->fshader->glsl_source, strlen(p->fshader->glsl_source)));
		f = fopen(fname, "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			p->fshader->size = ftell(f);
			fseek(f, 0, SEEK_SET);
			SceGxmProgram *res = (SceGxmProgram *)vglMalloc(p->fshader->size);
			fread(res, 1, p->fshader->size, f);
			fclose(f);
			sceGxmShaderPatcherRegisterProgram(gxm_shader_patcher, res, &p->fshader->id);
			p->fshader->prog = sceGxmShaderPatcherGetProgramFromId(p->fshader->id);
		} else {
#endif
			glsl_translator_process(p->fshader, 1, &p->fshader->glsl_source, NULL);
			compile_shader(p->fshader, GL_TRUE);
#ifdef HAVE_SHADER_CACHE
			f = fopen(fname, "wb");
			fwrite(p->fshader->prog, 1, p->fshader->size, f);
			fclose(f);
		}
#endif
		// Setting progressive default attribute bindings
		setDefaultAttribBindings();

		if (p->glsl_attr_map) {
			for (int i = 0; i < p->num_glsl_attr; i++) {
				glBindAttribLocation(progr, p->glsl_attr_map[i].idx, p->glsl_attr_map[i].name);
			}
			vgl_free(p->glsl_attr_map);
		}
		glsl_sema_mode = VGL_MODE_POSTPONED;
	}
#endif

	if (p->status == PROG_LINKED)
		return;
	p->status = PROG_LINKED;

	// Analyzing fragment shader
	uint32_t i, cnt;
	for (i = 0; i < TEXTURE_IMAGE_UNITS_NUM; i++) {
		p->frag_texunits[i] = GL_FALSE;
		p->vert_texunits[i] = GL_FALSE;
	}
	cnt = sceGxmProgramGetParameterCount(p->fshader->prog);
	for (i = 0; i < cnt; i++) {
		const SceGxmProgramParameter *param = sceGxmProgramGetParameter(p->fshader->prog, i);
		SceGxmParameterCategory cat = sceGxmProgramParameterGetCategory(param);
		if (cat == SCE_GXM_PARAMETER_CATEGORY_SAMPLER) {
			uint8_t texunit_idx = sceGxmProgramParameterGetResourceIndex(param) + 1;
			if (p->max_frag_texunit_idx < texunit_idx)
				p->max_frag_texunit_idx = texunit_idx;
			uniform *u = (uniform *)vglMalloc(sizeof(uniform));
			u->chain = p->frag_uniforms;
			u->ptr = param;
			u->size = sceGxmProgramParameterIsSamplerCube(param) ? 0xFFFFFFFF : 0;
			u->data = NULL;
			p->frag_uniforms = u;
			p->frag_texunits[texunit_idx - 1] = u;
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_UNIFORM) {
			if (sceGxmProgramParameterGetContainerIndex(param) == UBOS_NUM) {
				uniform *u = (uniform *)vglMalloc(sizeof(uniform));
				u->chain = p->frag_uniforms;
				u->ptr = param;
				u->is_vertex = GL_FALSE;
				u->is_fragment = GL_TRUE;
				u->size = sceGxmProgramParameterGetComponentCount(param) * sceGxmProgramParameterGetArraySize(param);
				u->data = (float *)vglMalloc(u->size * sizeof(float));
				sceClibMemset(u->data, 0, u->size * sizeof(float));
				p->frag_uniforms = u;
			}
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_UNIFORM_BUFFER) {
			ubo *u = (ubo *)vglMalloc(sizeof(ubo));
			u->chain = p->frag_ubos;
			u->idx = sceGxmProgramParameterGetResourceIndex(param);
			u->ptr = (const SceGxmProgramParameter *)getBlockDetails(p->fshader->unif_blk, u->idx);
			u->bind = 0;
			u->alias = NULL;
			p->frag_ubos = u;
		}
	}

	// Analyzing vertex shader
	p->wvp = sceGxmProgramFindParameterByName(p->vshader->prog, "gl_ModelViewProjectionMatrix");
	cnt = sceGxmProgramGetParameterCount(p->vshader->prog);
	for (i = 0; i < cnt; i++) {
		const SceGxmProgramParameter *param = sceGxmProgramGetParameter(p->vshader->prog, i);
		SceGxmParameterCategory cat = sceGxmProgramParameterGetCategory(param);
		if (cat == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE) {
			p->attr_num++;
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_SAMPLER) {
			uint8_t texunit_idx = sceGxmProgramParameterGetResourceIndex(param) + 1;
			if (p->max_vert_texunit_idx < texunit_idx)
				p->max_vert_texunit_idx = texunit_idx;
			uniform *u = (uniform *)vglMalloc(sizeof(uniform));
			u->chain = p->vert_uniforms;
			u->ptr = param;
			u->size = sceGxmProgramParameterIsSamplerCube(param) ? 0xFFFFFFFF : 0;
			u->data = NULL;
			p->vert_uniforms = u;
			p->vert_texunits[texunit_idx - 1] = u;
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_UNIFORM) {
			if (sceGxmProgramParameterGetContainerIndex(param) == UBOS_NUM) {
				uniform *u = (uniform *)vglMalloc(sizeof(uniform));
				u->chain = p->vert_uniforms;
				u->ptr = param;
				u->is_vertex = GL_TRUE;
				u->size = sceGxmProgramParameterGetComponentCount(param) * sceGxmProgramParameterGetArraySize(param);
				u->data = getUniformAliasDataPtr(p->frag_uniforms, sceGxmProgramParameterGetName(param), u->size);
				if (u->data) {
					u->is_fragment = GL_TRUE;
				} else {
					u->is_fragment = GL_FALSE;
					u->data = (float *)vglMalloc(u->size * sizeof(float));
					sceClibMemset(u->data, 0, u->size * sizeof(float));
				}
				p->vert_uniforms = u;
			}
		} else if (cat == SCE_GXM_PARAMETER_CATEGORY_UNIFORM_BUFFER) {
			ubo *u = (ubo *)vglMalloc(sizeof(ubo));
			u->chain = p->vert_ubos;
			u->idx = sceGxmProgramParameterGetResourceIndex(param);
			u->ptr = (const SceGxmProgramParameter *)getBlockDetails(p->vshader->unif_blk, u->idx);
			u->bind = 0;
			u->alias = hasBlockAlias(p->frag_ubos, ((block_uniform *)u->ptr)->name);
			p->frag_ubos = u;
		}
	}

	// Creating fragment and vertex program via sceGxmShaderPatcher if using vgl* draw pipeline
	if (p->stream_num) {
		if (p->stream_num > 1)
			p->stream_num = p->attr_num;
		patchVertexProgram(gxm_shader_patcher,
			p->vshader->id, p->attr, p->attr_num,
			p->stream, p->stream_num, &p->vprog);
		rebuild_frag_shader(p->fshader->id, &p->fprog, NULL, is_fbo_float ? SCE_GXM_OUTPUT_REGISTER_FORMAT_HALF4 : SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4);
		p->is_fbo_float = is_fbo_float;

		// Populating current blend settings
		p->blend_info.raw = blend_info.raw;
	} else {
		// Checking if bound attributes are aligned
		p->has_unaligned_attrs = GL_FALSE;

		for (i = 0; i < p->attr_num; i++) {
			p->attr_map[i] = i;
			if (p->attr[i].regIndex == 0xDEAD) {
				p->has_unaligned_attrs = GL_TRUE;
				break;
			}
		}

		// Fixing attributes mapping cache if in presence of unaligned attributes
		if (p->has_unaligned_attrs) {
			int j = 0;
			for (i = 0; i < p->attr_highest_idx; i++) {
				if (p->attr[i].regIndex != 0xDEAD) {
					p->attr_map[j] = i;
					j++;
				}
			}
		}
	}
}

void glUseProgram(GLuint prog) {
	// Setting current custom program to passed program
	cur_program = prog;
	dirty_frag_unifs = GL_TRUE;
	dirty_vert_unifs = GL_TRUE;
}

GLuint glGetUniformBlockIndex(GLuint prog, const GLchar *uniformBlockName) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

	// Getting the desired location
	ubo *j = p->vert_ubos;
	while (j) {
		block_uniform *b = (block_uniform *)j->ptr;
		if (!strcmp(b->name, uniformBlockName))
			return j->alias ? (GLuint)j->alias : (GLuint)j;
		j = j->chain;
	}
	j = p->frag_ubos;
	while (j) {
		block_uniform *b = (block_uniform *)j->ptr;
		if (!strcmp(b->name, uniformBlockName))
			return (GLuint)j;
		j = j->chain;
	}

	return 0xFFFFFFFF;
}

void glUniformBlockBinding(GLuint prog, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
	ubo *u = (ubo *)uniformBlockIndex;
	u->bind = uniformBlockBinding;
}

GLint glGetUniformLocation(GLuint prog, const GLchar *name) {
	// Grabbing passed program
	program *p = &progs[prog - 1];

#ifdef HAVE_GLSL_TRANSLATOR
	// texture and matrix are reserved keywords in CG but are not in GLSL
	if (!strcmp(name, "texture"))
		name = "vgl_tex";
	else if (!strcmp(name, "Texture"))
		name = "Vgl_tex";
	else if (!strcmp(name, "matrix"))
		name = "_matrix";
#endif

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
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	if (u->size == 0 || u->size == 0xFFFFFFFF) // Sampler
		u->data = (float *)v0;
	else // Regular Uniform
		u->data[0] = (float)v0;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform1i(GLuint prog, GLint location, GLint v0) {
	glUniform1i(location, v0);
}

void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	if (u->size == 0 || u->size == 0xFFFFFFFF) // Sampler
		u->data = (float *)value[0];
	else {
		// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
		if (u->size != count) {
			vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count, sceGxmProgramParameterGetName(u->ptr));
			SET_GL_ERROR(GL_INVALID_OPERATION)
		}
#endif
		for (int i = 0; i < count; i++) {
			u->data[i] = (float)value[i];
		}
	}

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform1iv(GLuint prog, GLint location, GLsizei count, const GLint *value) {
	glUniform1iv(location, count, value);
}

void glUniform1f(GLint location, GLfloat v0) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform1f(GLuint prog, GLint location, GLfloat v0) {
	glUniform1f(location, v0);
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	vgl_fast_memcpy(u->data, value, count * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform1fv(GLuint prog, GLint location, GLsizei count, const GLfloat *value) {
	glUniform1fv(location, count, value);
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
	u->data[1] = (float)v1;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform2i(GLuint prog, GLint location, GLint v0, GLint v1) {
	glUniform2i(location, v0, v1);
}

void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 2) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 2, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	for (int i = 0; i < count * 2; i++) {
		u->data[i] = (float)value[i];
	}

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform2iv(GLuint prog, GLint location, GLsizei count, const GLint *value) {
	glUniform2iv(location, count, value);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
	u->data[1] = v1;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform2f(GLuint prog, GLint location, GLfloat v0, GLfloat v1) {
	glUniform2f(location, v0, v1);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 2) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 2, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	vgl_fast_memcpy(u->data, value, count * 2 * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform2fv(GLuint prog, GLint location, GLsizei count, const GLfloat *value) {
	glUniform2fv(location, count, value);
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
	u->data[1] = (float)v1;
	u->data[2] = (float)v2;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform3i(GLuint prog, GLint location, GLint v0, GLint v1, GLint v2) {
	glUniform3i(location, v0, v1, v2);
}

void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 3) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 3, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	for (int i = 0; i < count * 3; i++) {
		u->data[i] = (float)value[i];
	}

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform3iv(GLuint prog, GLint location, GLsizei count, const GLint *value) {
	glUniform3iv(location, count, value);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
	u->data[1] = v1;
	u->data[2] = v2;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform3f(GLuint prog, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	glUniform3f(location, v0, v1, v2);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 3) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 3, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	vgl_fast_memcpy(u->data, value, count * 3 * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform3fv(GLuint prog, GLint location, GLsizei count, const GLfloat *value) {
	glUniform3fv(location, count, value);
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = (float)v0;
	u->data[1] = (float)v1;
	u->data[2] = (float)v2;
	u->data[3] = (float)v3;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform4i(GLuint prog, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
	glUniform4i(location, v0, v1, v2, v3);
}

void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 4) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 4, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	for (int i = 0; i < count * 4; i++) {
		u->data[i] = (float)value[i];
	}

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform4iv(GLuint prog, GLint location, GLsizei count, const GLint *value) {
	glUniform4iv(location, count, value);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
	u->data[0] = v0;
	u->data[1] = v1;
	u->data[2] = v2;
	u->data[3] = v3;

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform4f(GLuint prog, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	glUniform4f(location, v0, v1, v2, v3);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 4) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 4, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	vgl_fast_memcpy(u->data, value, count * 4 * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniform4fv(GLuint prog, GLint location, GLsizei count, const GLfloat *value) {
	glUniform4fv(location, count, value);
}

inline void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 4) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 4, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	if (transpose) {
		for (int i = 0; i < count; i++) {
			matrix2x2_transpose(&u->data[i * 4], &value[i * 4]);
		}
	} else
		vgl_fast_memcpy(u->data, value, count * 4 * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniformMatrix2fv(GLuint prog, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	glUniformMatrix2fv(location, count, transpose, value);
}

inline void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0)
		return;

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 9) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 9, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	if (transpose) {
		for (int i = 0; i < count; i++) {
			matrix3x3_transpose(&u->data[i * 9], &value[i * 9]);
		}
	} else
		vgl_fast_memcpy(u->data, value, count * 9 * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniformMatrix3fv(GLuint prog, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	glUniformMatrix3fv(location, count, transpose, value);
}

inline void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	// Checking if the uniform does exist
	if (location == -1 || location == 0) {
		return;
	}

	// Grabbing passed uniform
	uniform *u = (uniform *)-location;

	// Setting passed value to desired uniform
#ifndef SKIP_ERROR_HANDLING
	if (u->size < count * 16) {
		vgl_log("%s:%d: %s: expected %d elements but got %d for uniform %s.\n", __FILE__, __LINE__, __func__, u->size, count * 16, sceGxmProgramParameterGetName(u->ptr));
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	if (transpose) {
		for (int i = 0; i < count; i++) {
			matrix4x4_transpose(&u->data[i * 16], &value[i * 16]);
		}
	} else
		vgl_fast_memcpy(u->data, value, count * 16 * sizeof(float));

	if (u->is_vertex)
		dirty_vert_unifs = GL_TRUE;
	if (u->is_fragment)
		dirty_frag_unifs = GL_TRUE;
}

void glProgramUniformMatrix4fv(GLuint prog, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	glUniformMatrix4fv(location, count, transpose, value);
}

void glEnableVertexAttribArray(GLuint index) {
#ifndef SKIP_ERROR_HANDLING
	if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	cur_vao->vertex_attrib_state |= (1 << index);
}

void glDisableVertexAttribArray(GLuint index) {
#ifndef SKIP_ERROR_HANDLING
	if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	cur_vao->vertex_attrib_state &= ~(1 << index);
}

void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	} else if (index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE);
	}
#endif
	pointer[0] = (void *)cur_vao->vertex_attrib_offsets[index];
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (size < 1 || size > 4 || stride < 0 || index >= VERTEX_ATTRIBS_NUM) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	cur_vao->vertex_attrib_offsets[index] = (uint32_t)pointer;
	cur_vao->vertex_attrib_vbo[index] = vertex_array_unit;

	SceGxmVertexAttribute *attributes = &cur_vao->vertex_attrib_config[index];
	SceGxmVertexStream *streams = &cur_vao->vertex_stream_config[index];

	// Detecting attribute format and size
	unsigned short bpe;
	switch (type) {
	case GL_HALF_FLOAT:
	case GL_HALF_FLOAT_OES:
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
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
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? cur_vao->vertex_attrib_vbo[index] : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? cur_vao->vertex_attrib_config[index].componentCount : cur_vao->vertex_attrib_size[index];
		break;
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? cur_vao->vertex_stream_config[index].stride : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? gxm_vd_fmt_to_gl(cur_vao->vertex_attrib_config[index].format) : GL_FLOAT;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? (cur_vao->vertex_attrib_config[index].format >= SCE_GXM_ATTRIBUTE_FORMAT_U8N && cur_vao->vertex_attrib_config[index].format <= SCE_GXM_ATTRIBUTE_FORMAT_S16N) : GL_FALSE;
		break;
	case GL_CURRENT_VERTEX_ATTRIB:
#ifndef SKIP_ERROR_HANDLING
		if (index == 0) {
			SET_GL_ERROR(GL_INVALID_OPERATION)
		}
#endif
		params[0] = cur_vao->vertex_attrib_value[index][0];
		params[1] = cur_vao->vertex_attrib_size[index] > 1 ? cur_vao->vertex_attrib_value[index][1] : 0;
		params[2] = cur_vao->vertex_attrib_size[index] > 2 ? cur_vao->vertex_attrib_value[index][2] : 0;
		params[3] = cur_vao->vertex_attrib_size[index] > 3 ? cur_vao->vertex_attrib_value[index][3] : 1;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
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
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? cur_vao->vertex_attrib_vbo[index] : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_SIZE:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? cur_vao->vertex_attrib_config[index].componentCount : cur_vao->vertex_attrib_size[index];
		break;
	case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? cur_vao->vertex_stream_config[index].stride : 0;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_TYPE:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? gxm_vd_fmt_to_gl(cur_vao->vertex_attrib_config[index].format) : GL_FLOAT;
		break;
	case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
		params[0] = (cur_vao->vertex_attrib_state & (1 << index)) ? (cur_vao->vertex_attrib_config[index].format >= SCE_GXM_ATTRIBUTE_FORMAT_U8N && cur_vao->vertex_attrib_config[index].format <= SCE_GXM_ATTRIBUTE_FORMAT_S16N) : GL_FALSE;
		break;
	case GL_CURRENT_VERTEX_ATTRIB:
#ifndef SKIP_ERROR_HANDLING
		if (index == 0) {
			SET_GL_ERROR(GL_INVALID_OPERATION)
		}
#endif
		params[0] = cur_vao->vertex_attrib_value[index][0];
		params[1] = cur_vao->vertex_attrib_size[index] > 1 ? cur_vao->vertex_attrib_value[index][1] : 0;
		params[2] = cur_vao->vertex_attrib_size[index] > 2 ? cur_vao->vertex_attrib_value[index][2] : 0;
		params[3] = cur_vao->vertex_attrib_size[index] > 3 ? cur_vao->vertex_attrib_value[index][3] : 1;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glVertexAttrib1f(GLuint index, GLfloat v0) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(1);
	cur_vao->vertex_attrib_size[index] = 1;
	cur_vao->vertex_attrib_value[index][0] = v0;
}

void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(2);
	cur_vao->vertex_attrib_size[index] = 2;
	cur_vao->vertex_attrib_value[index][0] = v0;
	cur_vao->vertex_attrib_value[index][1] = v1;
}

void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(3);
	cur_vao->vertex_attrib_size[index] = 3;
	cur_vao->vertex_attrib_value[index][0] = v0;
	cur_vao->vertex_attrib_value[index][1] = v1;
	cur_vao->vertex_attrib_value[index][2] = v2;
}

void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(4);
	cur_vao->vertex_attrib_size[index] = 4;
	cur_vao->vertex_attrib_value[index][0] = v0;
	cur_vao->vertex_attrib_value[index][1] = v1;
	cur_vao->vertex_attrib_value[index][2] = v2;
	cur_vao->vertex_attrib_value[index][3] = v3;
}

void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(1);
	cur_vao->vertex_attrib_size[index] = 1;
	cur_vao->vertex_attrib_value[index][0] = v[0];
}

void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(2);
	cur_vao->vertex_attrib_size[index] = 2;
	cur_vao->vertex_attrib_value[index][0] = v[0];
	cur_vao->vertex_attrib_value[index][1] = v[1];
}

void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(3);
	cur_vao->vertex_attrib_size[index] = 3;
	cur_vao->vertex_attrib_value[index][0] = v[0];
	cur_vao->vertex_attrib_value[index][1] = v[1];
	cur_vao->vertex_attrib_value[index][2] = v[2];
}

void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
	cur_vao->vertex_attrib_value[index] = reserve_attrib_pool(4);
	cur_vao->vertex_attrib_size[index] = 4;
	cur_vao->vertex_attrib_value[index][0] = v[0];
	cur_vao->vertex_attrib_value[index][1] = v[1];
	cur_vao->vertex_attrib_value[index][2] = v[2];
	cur_vao->vertex_attrib_value[index][3] = v[3];
}

void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
#ifndef SKIP_ERROR_HANDLING
	if (target != GL_UNIFORM_BUFFER) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	} else if (index >= UBOS_NUM) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, index)
	} else if (size <= 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, size)
	}
#endif

	ubo_buf[index] = (gpubuffer *)buffer;
	ubo_offset[index] = offset;
}

void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
#ifndef SKIP_ERROR_HANDLING
	if (target != GL_UNIFORM_BUFFER) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	} else if (index >= UBOS_NUM) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, index)
	}
#endif

	ubo_buf[index] = (gpubuffer *)buffer;
	ubo_offset[index] = 0;
}

void glBindAttribLocation(GLuint prog, GLuint index, const GLchar *name) {
	// Grabbing passed program
	program *p = &progs[prog - 1];
	
#ifdef HAVE_GLSL_TRANSLATOR
	// If we use VGL_MODE_POSTPONED, we perform attributes binding in glLinkProgram
	if (glsl_sema_mode == VGL_MODE_POSTPONED) {
		if (!p->glsl_attr_map)
			p->glsl_attr_map = vglMalloc(sizeof(attr_mapping) * VERTEX_ATTRIBS_NUM);
		p->glsl_attr_map[p->num_glsl_attr].idx = index;
		strcpy(p->glsl_attr_map[p->num_glsl_attr++].name, name);
		return;
	}
#endif

	// Looking for desired parameter in requested program
	const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (param == NULL || sceGxmProgramParameterGetCategory(param) != SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE)
		return;
	uint32_t attrIndex = sceGxmProgramParameterGetResourceIndex(param);
	
	// Swapping any previously made bind to the requested attribute
	for (int i = 0; i < p->attr_highest_idx; i++) {
		if (p->attr[i].regIndex == attrIndex) {
			p->attr[i].regIndex = p->attr[index].regIndex;
			break;
		}
	}
	
	// Set new binding to the requested attribute
	p->attr[index].regIndex = attrIndex;
	if ((p->attr_highest_idx == 0) || (p->attr_highest_idx - 1 < index))
		p->attr_highest_idx = index + 1;
}

GLint glGetAttribLocation(GLuint prog, const GLchar *name) {
	program *p = &progs[prog - 1];
	const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(p->vshader->prog, name);
	if (param == NULL || sceGxmProgramParameterGetCategory(param) != SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE)
		return -1;
	uint32_t index = sceGxmProgramParameterGetResourceIndex(param);

	// Return requested attribute location
	for (int i = 0; i < p->attr_highest_idx; i++) {
		if (p->attr[i].regIndex == index)
			return i;
	}

	return -1;
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

	// Detecting uniform type
	const char *pname = sceGxmProgramParameterGetName(u->ptr);
	if (sceGxmProgramParameterGetCategory(u->ptr) == SCE_GXM_PARAMETER_CATEGORY_SAMPLER) {
		*type = sceGxmProgramParameterIsSamplerCube(u->ptr) ? GL_SAMPLER_CUBE : GL_SAMPLER_2D;
		*size = 1;
	} else {
		*size = sceGxmProgramParameterGetArraySize(u->ptr);
		*type = gxm_unif_type_to_gl(sceGxmProgramParameterGetType(u->ptr), sceGxmProgramParameterGetComponentCount(u->ptr), size);
		if (*type >= GL_FLOAT_VEC2 && *type <= GL_FLOAT_VEC4 && *size > 1) {
			matrix_uniform *m = p->vshader->mat;
			while (m) {
				if (m->ptr == u->ptr) {
					gxm_unif_to_mat(type, size);
					break;
				}
				m = (matrix_uniform *)m->chain;
			}
			if (!m) {
				m = p->fshader->mat;
				while (m) {
					if (m->ptr == u->ptr) {
						gxm_unif_to_mat(type, size);
						break;
					}
					m = (matrix_uniform *)m->chain;
				}
			}
		}
	}
	
	// Copying uniform name
#ifdef HAVE_GLSL_TRANSLATOR
	// texture and matrix are reserved keywords in CG but are not in GLSL
	if (!strcmp(pname, "vgl_tex"))
		pname = "texture";
	else if (!strcmp(pname, "Vgl_tex"))
		pname = "Texture";
	else if (!strcmp(name, "_matrix"))
		name = "matrix";
#endif
	bufSize = min(strlen(pname), bufSize - 1);
	if (length)
		*length = bufSize;
	strncpy(name, pname, bufSize);
	name[bufSize] = 0;
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}

	// Allocating enough memory on vitaGL mempool
	void *ptr = gpu_alloc_mapped_temp(count * bpe * size);

	// Copying passed data to vitaGL mempool
	if (stride == 0)
		vgl_fast_memcpy(ptr, pointer, count * bpe * size); // Faster if stride == 0
	else {
		int i;
		uint8_t *dst = (uint8_t *)ptr;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			vgl_fast_memcpy(dst, src, bpe * size);
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

void vglGetShaderBinary(GLuint handle, GLsizei bufSize, GLsizei *length, void *binary) {
#ifndef SKIP_ERROR_HANDLING
	if (bufSize < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Grabbing passed shader
	shader *s = &shaders[handle - 1];

	GLsizei size = 0;
	if (s->prog) {
		uint32_t matrix_uniforms_num = 0;
		matrix_uniform *m = s->mat;
		uint32_t *ubin = (uint32_t *)binary;
		while (m) {
			matrix_uniforms_num++;
			ubin[matrix_uniforms_num] = sceGxmProgramParameterGetIndex(s->prog, m->ptr);
			m = (matrix_uniform *)m->chain;
		}
		ubin[0] = matrix_uniforms_num;
		
		GLsizei bin_len = sceGxmProgramGetSize(s->prog);
		vgl_fast_memcpy(&ubin[matrix_uniforms_num + 1], s->prog, bin_len);
		size = bin_len + (matrix_uniforms_num + 1) * sizeof(uint32_t);
	}
	if (length)
		*length = size;
}

void vglCgShaderSource(GLuint handle, GLsizei count, const GLchar *const *string, const GLint *length) {
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

#if defined(SHADER_COMPILER_SPEEDHACK)
	if (count == 1)
		s->prog = (SceGxmProgram *)*string;
	else
#endif
	{
		s->source = (char *)vglMalloc(size);
		s->source[0] = 0;
		for (int i = 0; i < count; i++) {
			strncat(s->source, string[i], length ? length[i] : strlen(string[i]));
		}
		s->prog = (SceGxmProgram *)s->source;
	}
	
	s->size = size - 1;
}

void vglAddSemanticBinding(const GLchar *const *varying, GLint index, GLenum type) {
#ifdef HAVE_GLSL_TRANSLATOR
#ifndef SKIP_ERROR_HANDLING
	if (glsl_custom_bindings_num >= MAX_CUSTOM_BINDINGS) {
		vgl_log("%s:%d %s: Too many custom bindings supplied. Consider increasing MAX_CUSTOM_BINDINGS.\n", __FILE__, __LINE__, __func__);
		return;
	}			
#endif
	strcpy(glsl_custom_bindings[glsl_custom_bindings_num].name, varying);
	glsl_custom_bindings[glsl_custom_bindings_num].idx = index;
	glsl_custom_bindings[glsl_custom_bindings_num].type = type;
	glsl_custom_bindings[glsl_custom_bindings_num++].ref_idx = glsl_current_ref_idx;
#endif
}

void vglUseLowPrecision(GLboolean val) {
#ifdef HAVE_GLSL_TRANSLATOR
	glsl_precision_low = val;
#endif
}

void vglSetSemanticBindingMode(GLenum mode) {
#ifdef HAVE_GLSL_TRANSLATOR
	glsl_sema_mode = mode;
#endif
}

void vglOverrideTexFormat(GLenum target) {
#ifdef HAVE_UNPURE_TEXFORMATS
	switch (target) {
	case GL_TEXTURE_1D:
		tex2d_override = 1;
		break;
	default:
		tex2d_override = 0;
		break;
	}
#endif
}
