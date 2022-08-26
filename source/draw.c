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
 * draw.c:
 * Implementation for draw call functions
 */
#include "shared.h"
#include "vitaGL.h"

GLboolean prim_is_non_native = GL_FALSE; // Flag for when a primitive not supported natively by sceGxm is used

#define setup_elements_indices(type_t) \
	type_t *ptr; \
	if (gpu_buf != NULL && !prim_is_non_native) { \
		ptr = (type_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices); \
		gpu_buf->used = GL_TRUE; \
	} else { \
		switch (mode) { \
		case GL_QUADS: \
			ptr = gpu_alloc_mapped_temp(count * 3 * sizeof(type_t)); \
			for (int i = 0; i < count / 4; i++) { \
				ptr[i * 6] = src[i * 4]; \
				ptr[i * 6 + 1] = src[i * 4 + 1]; \
				ptr[i * 6 + 2] = src[i * 4 + 3]; \
				ptr[i * 6 + 3] = src[i * 4 + 1]; \
				ptr[i * 6 + 4] = src[i * 4 + 2]; \
				ptr[i * 6 + 5] = src[i * 4 + 3]; \
			} \
			count = (count / 2) * 3; \
			break; \
		case GL_LINE_STRIP: \
			ptr = gpu_alloc_mapped_temp((count - 1) * 2 * sizeof(type_t)); \
			for (int i = 0; i < count - 1; i++) { \
				ptr[i * 2] = src[i]; \
				ptr[i * 2 + 1] = src[i + 1]; \
			} \
			count = (count - 1) * 2; \
			break; \
		case GL_LINE_LOOP: \
			ptr = gpu_alloc_mapped_temp(count * 2 * sizeof(type_t)); \
			for (int i = 0; i < count - 1; i++) { \
				ptr[i * 2] = src[i]; \
				ptr[i * 2 + 1] = src[i + 1]; \
			} \
			ptr[(count - 1) * 2] = src[count - 1]; \
			ptr[(count - 1) * 2 + 1] = src[0]; \
			count = count * 2; \
			break; \
		default: \
			ptr = gpu_alloc_mapped_temp(count * sizeof(type_t)); \
			vgl_fast_memcpy(ptr, src, count * sizeof(type_t)); \
			break; \
		} \
	}
	
#define setup_elements_indices_with_base(type_t) \
	type_t *ptr; \
	switch (mode) { \
	case GL_QUADS: \
		ptr = gpu_alloc_mapped_temp(count * 3 * sizeof(type_t)); \
		for (int i = 0; i < count / 4; i++) { \
			ptr[i * 6] = src[i * 4] + baseVertex; \
			ptr[i * 6 + 1] = src[i * 4 + 1] + baseVertex; \
			ptr[i * 6 + 2] = src[i * 4 + 3] + baseVertex; \
			ptr[i * 6 + 3] = src[i * 4 + 1] + baseVertex; \
			ptr[i * 6 + 4] = src[i * 4 + 2] + baseVertex; \
			ptr[i * 6 + 5] = src[i * 4 + 3] + baseVertex; \
		} \
		count = (count / 2) * 3; \
		break; \
	case GL_LINE_STRIP: \
		ptr = gpu_alloc_mapped_temp((count - 1) * 2 * sizeof(type_t)); \
		for (int i = 0; i < count - 1; i++) { \
			ptr[i * 2] = src[i] + baseVertex; \
			ptr[i * 2 + 1] = src[i + 1] + baseVertex; \
		} \
		count = (count - 1) * 2; \
		break; \
	case GL_LINE_LOOP: \
		ptr = gpu_alloc_mapped_temp(count * 2 * sizeof(type_t)); \
		for (int i = 0; i < count - 1; i++) { \
			ptr[i * 2] = src[i] + baseVertex; \
			ptr[i * 2 + 1] = src[i + 1] + baseVertex; \
		} \
		ptr[(count - 1) * 2] = src[count - 1] + baseVertex; \
		ptr[(count - 1) * 2 + 1] = src[0] + baseVertex; \
		count = count * 2; \
		break; \
	default: \
		ptr = gpu_alloc_mapped_temp(count * sizeof(type_t)); \
		for (int i = 0; i < count; i++) { \
			ptr[i] = src[i] + baseVertex; \
		} \
		break; \
	}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glDrawArrays, "UII", mode, first, count))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	}
#endif
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
			vgl_fast_memcpy(ptr, default_line_strips_idx_ptr + first * 2, (count - 1) * 2 * sizeof(uint16_t));
			ptr[(count - 1) * 2] = first + count - 1;
			ptr[(count - 1) * 2 + 1] = first;
			count *= 2;
			break;
		default:
			ptr = default_idx_ptr + first;
			break;
		}

#ifndef SKIP_ERROR_HANDLING
		if (first + count > MAX_IDX_NUMBER) {
			vgl_log("%s:%d Attempting to draw a model with glDrawArrays which is too big! Consider increasing MAX_IDX_NUMBER value...\n", __FILE__, __LINE__);
		}	
#endif
		sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
	}
	restore_polygon_mode(gxm_p);
}

void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	}
#endif
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
			vgl_fast_memcpy(ptr, default_line_strips_idx_ptr + first * 2, (count - 1) * 2 * sizeof(uint16_t));
			ptr[(count - 1) * 2] = first + count - 1;
			ptr[(count - 1) * 2 + 1] = first;
			count *= 2;
			break;
		default:
			ptr = default_idx_ptr + first;
			break;
		}

#ifndef SKIP_ERROR_HANDLING
		if (first + count > MAX_IDX_NUMBER) {
			vgl_log("%s:%d Attempting to draw a model with glDrawArrays which is too big! Consider increasing MAX_IDX_NUMBER value...\n", __FILE__, __LINE__);
		}	
#endif
		sceGxmDrawInstanced(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count * primcount, count);
	}
	restore_polygon_mode(gxm_p);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *gl_indices) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glDrawElements, "UIUU", mode, count, type, gl_indices))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	gpubuffer *gpu_buf = (gpubuffer *)cur_vao->index_array_unit;
	uint16_t *src = gpu_buf ? (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices) : (uint16_t *)gl_indices;
	if (cur_program != 0)
		is_draw_legal = _glDrawElements_CustomShadersIMPL(src, count, 0, type == GL_UNSIGNED_SHORT);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawElements_FixedFunctionIMPL(src, count, 0, type == GL_UNSIGNED_SHORT);
	}

#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
#endif
	{
		if (type == GL_UNSIGNED_SHORT) {
			setup_elements_indices(uint16_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
		} else {
			setup_elements_indices(uint32_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U32, ptr, count);
		}
	}
	restore_polygon_mode(gxm_p);
}

void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const GLvoid *gl_indices, GLint baseVertex) {
#ifndef SKIP_ERROR_HANDLING
	if (type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	gpubuffer *gpu_buf = (gpubuffer *)cur_vao->index_array_unit;
	uint16_t *src = gpu_buf ? (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices) : (uint16_t *)gl_indices;
	if (cur_program != 0)
		is_draw_legal = _glDrawElements_CustomShadersIMPL(src, count, 0, type == GL_UNSIGNED_SHORT);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawElements_FixedFunctionIMPL(src, count, 0, type == GL_UNSIGNED_SHORT);
	}

#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
#endif
	{
		if (type == GL_UNSIGNED_SHORT) {
			setup_elements_indices_with_base(uint16_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
		} else {
			setup_elements_indices_with_base(uint32_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U32, ptr, count);
		}
	}
	restore_polygon_mode(gxm_p);
}

void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *gl_indices) {
#ifndef SKIP_ERROR_HANDLING
	if (type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	} else if (end < start) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	gpubuffer *gpu_buf = (gpubuffer *)cur_vao->index_array_unit;
	uint16_t *src = gpu_buf ? (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices) : (uint16_t *)gl_indices;
	if (cur_program != 0)
		is_draw_legal = _glDrawElements_CustomShadersIMPL(src, count, end + 1, type == GL_UNSIGNED_SHORT);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawElements_FixedFunctionIMPL(src, count, end + 1, type == GL_UNSIGNED_SHORT);
	}

#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
#endif
	{
		if (type == GL_UNSIGNED_SHORT) {
			setup_elements_indices(uint16_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
		} else {
			setup_elements_indices(uint32_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U32, ptr, count);
		}
	}
	restore_polygon_mode(gxm_p);
}

void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *gl_indices, GLint baseVertex) {
#ifndef SKIP_ERROR_HANDLING
	if (type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	gpubuffer *gpu_buf = (gpubuffer *)cur_vao->index_array_unit;
	uint16_t *src = gpu_buf ? (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices) : (uint16_t *)gl_indices;
	if (cur_program != 0)
		is_draw_legal = _glDrawElements_CustomShadersIMPL(src, count, end + 1, type == GL_UNSIGNED_SHORT);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawElements_FixedFunctionIMPL(src, count, end + 1, type == GL_UNSIGNED_SHORT);
	}

#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
#endif
	{
		if (type == GL_UNSIGNED_SHORT) {
			setup_elements_indices_with_base(uint16_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count);
		} else {
			setup_elements_indices_with_base(uint32_t)
			sceGxmDraw(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U32, ptr, count);
		}
	}
	restore_polygon_mode(gxm_p);
}

void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *gl_indices, GLsizei primcount) {
#ifndef SKIP_ERROR_HANDLING
	if (type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
	}
#endif

	SceGxmPrimitiveType gxm_p;
	gl_primitive_to_gxm(mode, gxm_p, count);
	sceneReset();
	GLboolean is_draw_legal = GL_TRUE;

	gpubuffer *gpu_buf = (gpubuffer *)cur_vao->index_array_unit;
	uint16_t *src = gpu_buf ? (uint16_t *)((uint8_t *)gpu_buf->ptr + (uint32_t)gl_indices) : (uint16_t *)gl_indices;
	if (cur_program != 0)
		is_draw_legal = _glDrawElements_CustomShadersIMPL(src, count, 0, type == GL_UNSIGNED_SHORT);
	else {
		if (!(ffp_vertex_attrib_state & (1 << 0)))
			return;
		_glDrawElements_FixedFunctionIMPL(src, count, 0, type == GL_UNSIGNED_SHORT);
	}

#ifndef SKIP_ERROR_HANDLING
	if (is_draw_legal)
#endif
	{
		if (type == GL_UNSIGNED_SHORT) {
			setup_elements_indices(uint16_t)
			sceGxmDrawInstanced(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U16, ptr, count * primcount, count);
		} else {
			setup_elements_indices(uint32_t)
			sceGxmDrawInstanced(gxm_context, gxm_p, SCE_GXM_INDEX_FORMAT_U32, ptr, count * primcount, count);
		}
	}
	restore_polygon_mode(gxm_p);
}

void vglDrawObjects(GLenum mode, GLsizei count, GLboolean implicit_wvp) {
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (count < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, count)
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
