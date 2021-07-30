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
 * vertex_buffers.c:
 * Implementation for vertex buffers related functions
 */

#include "shared.h"

uint32_t vertex_array_unit = 0; // Current in-use vertex array buffer unit
uint32_t index_array_unit = 0; // Current in-use element array buffer unit

void *vertex_object; // Vertex object address for vgl* draw pipeline
void *color_object; // Color object address for vgl* draw pipeline
void *texture_object; // Texture object address for vgl* draw pipeline
void *index_object; // Index object address for vgl* draw pipeline

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glGenBuffers(GLsizei n, GLuint *res) {
	int i;
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	for (i = 0; i < n; i++) {
		res[i] = (GLuint)(vgl_malloc(sizeof(gpubuffer), VGL_MEM_EXTERNAL));
#ifdef LOG_ERRORS
		if (!res[i])
			vgl_log("glGenBuffers failed to alloc a buffer (%d/%lu).\n", i, n);
#endif
		sceClibMemset((void *)res[i], 0, sizeof(gpubuffer));
	}
}

void glBindBuffer(GLenum target, GLuint buffer) {
	switch (target) {
	case GL_ARRAY_BUFFER:
		vertex_array_unit = buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		index_array_unit = buffer;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
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
			gpubuffer *gpu_buf = (gpubuffer *)gl_buffers[j];
			if (gpu_buf->ptr) {
				if (gpu_buf->used)
					markAsDirty(gpu_buf->ptr);
				else
					vgl_free(gpu_buf->ptr);
			}
			vgl_free(gpu_buf);
		}
	}
}

void glBufferData(GLenum target, GLsizei size, const GLvoid *data, GLenum usage) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#ifndef SKIP_ERROR_HANDLING
	if (size < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	} else if (!gpu_buf) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	if (!gpu_buf) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
	switch (usage) {
	case GL_DYNAMIC_DRAW:
	case GL_DYNAMIC_READ:
	case GL_DYNAMIC_COPY:
		gpu_buf->type = VGL_MEM_RAM;
		break;
	default:
		gpu_buf->type = VGL_MEM_VRAM;
		break;
	}

	// Marking previous content for deletion or deleting it straight if unused
	if (gpu_buf->ptr) {
		if (gpu_buf->used)
			markAsDirty(gpu_buf->ptr);
		else
			vgl_free(gpu_buf->ptr);
	}

	// Allocating a new buffer
	gpu_buf->ptr = gpu_alloc_mapped(size, gpu_buf->type);

#ifndef SKIP_ERROR_HANDLING
	if (!gpu_buf->ptr) {
		SET_GL_ERROR(GL_OUT_OF_MEMORY)
	}
#endif

	gpu_buf->size = size;
	gpu_buf->used = GL_FALSE;

	if (data)
		sceClibMemcpy(gpu_buf->ptr, data, size);
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#ifndef SKIP_ERROR_HANDLING
	if ((size < 0) || (offset < 0) || ((offset + size) > gpu_buf->size)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	} else if (!gpu_buf) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	// Allocating a new buffer
	uint8_t *ptr = gpu_alloc_mapped(gpu_buf->size, gpu_buf->type);

#ifdef LOG_ERRORS
	if (!ptr) {
		vgl_log("glBufferSubData failed to alloc a buffer of %ld bytes. Buffer content won't be updated.\n", gpu_buf->size);
		return;
	}
#endif

	// Copying up previous data combined to modified data
	if (offset > 0)
		vgl_memcpy(ptr, gpu_buf->ptr, offset);
	vgl_memcpy(ptr + offset, data, size);
	if (gpu_buf->size - size - offset > 0)
		vgl_memcpy(ptr + offset + size, (uint8_t *)gpu_buf->ptr + offset + size, gpu_buf->size - size - offset);

	// Marking previous content for deletion
	if (gpu_buf->used)
		markAsDirty(gpu_buf->ptr);
	else
		vgl_free(gpu_buf->ptr);

	gpu_buf->ptr = ptr;
	gpu_buf->used = GL_FALSE;
}

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#ifndef SKIP_ERROR_HANDLING
	if (!gpu_buf) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	switch (pname) {
	case GL_BUFFER_SIZE:
		*params = gpu_buf->size;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

// VGL_EXT_gpu_objects_array extension implementation

void vglVertexPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[0];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[0];

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
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}

	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;

	vertex_object = gpu_alloc_mapped_temp(count * streams->stride);
	sceClibMemcpy(vertex_object, pointer, count * streams->stride);
}

void vglColorPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 3) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
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

	color_object = gpu_alloc_mapped_temp(count * streams->stride);
	sceClibMemcpy(color_object, pointer, count * streams->stride);
}

void vglTexCoordPointer(GLint size, GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if ((stride < 0) || (size < 2) || (size > 4)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[1];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[1];

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
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}

	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;

	texture_object = gpu_alloc_mapped_temp(count * streams->stride);
	sceClibMemcpy(texture_object, pointer, count * streams->stride);
}

void vglIndexPointer(GLenum type, GLsizei stride, GLuint count, const GLvoid *pointer) {
#ifndef SKIP_ERROR_HANDLING
	if (stride < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	int bpe;
	switch (type) {
	case GL_SHORT:
		bpe = sizeof(GLshort);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	index_object = gpu_alloc_mapped_temp(count * bpe);
	if (stride == 0)
		sceClibMemcpy(index_object, pointer, count * bpe);
	else {
		int i;
		uint8_t *dst = (uint8_t *)index_object;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			sceClibMemcpy(dst, src, bpe);
			dst += bpe;
			src += stride;
		}
	}
}

void vglVertexPointerMapped(const GLvoid *pointer) {
	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[0];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[0];

	attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attributes->componentCount = 3;
	streams->stride = 12;

	vertex_object = (GLvoid *)pointer;
}

void vglColorPointerMapped(GLenum type, const GLvoid *pointer) {
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

	attributes->componentCount = 4;
	streams->stride = 4 * bpe;

	color_object = (GLvoid *)pointer;
}

void vglTexCoordPointerMapped(const GLvoid *pointer) {
	SceGxmVertexAttribute *attributes = &ffp_vertex_attrib_config[1];
	SceGxmVertexStream *streams = &ffp_vertex_stream_config[1];

	attributes->format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	attributes->componentCount = 2;
	streams->stride = 8;

	texture_object = (GLvoid *)pointer;
}

void vglIndexPointerMapped(const GLvoid *pointer) {
	index_object = (GLvoid *)pointer;
}
