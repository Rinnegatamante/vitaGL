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
		res[i] = (GLuint)(vglMalloc(sizeof(gpubuffer)));
#ifdef LOG_ERRORS
		if (!res[i])
			vgl_log("%s:%d glGenBuffers failed to alloc a buffer (%d/%lu).\n", __FILE__, __LINE__, i, n);
#endif
		sceClibMemset((void *)res[i], 0, sizeof(gpubuffer));
	}
}

void glBindBuffer(GLenum target, GLuint buffer) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glBindBuffer, "UU", target, buffer))
		return;
#endif
	switch (target) {
	case GL_ARRAY_BUFFER:
		vertex_array_unit = buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		index_array_unit = buffer;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
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
					vglFree(gpu_buf->ptr);
			}
			vglFree(gpu_buf);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
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
			vglFree(gpu_buf->ptr);
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
		vgl_fast_memcpy(gpu_buf->ptr, data, size);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
#ifndef SKIP_ERROR_HANDLING
	if (!gpu_buf) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (size < 0 || offset < 0 || offset + size > gpu_buf->size) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Allocating a new buffer
	if (gpu_buf->used) {
		uint8_t *ptr = gpu_alloc_mapped(gpu_buf->size, gpu_buf->type);

#ifdef LOG_ERRORS
		if (!ptr) {
			vgl_log("%s:%d glBufferSubData failed to alloc a buffer of %ld bytes. Buffer content won't be updated.\n", __FILE__, __LINE__, gpu_buf->size);
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
		markAsDirty(gpu_buf->ptr);
		
		gpu_buf->ptr = ptr;
		gpu_buf->used = GL_FALSE;
	} else {
		vgl_memcpy((uint8_t *)gpu_buf->ptr + offset, data, size);
	}	
}

void *glMapBuffer(GLenum target, GLenum access) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL)
	}

#ifndef SKIP_ERROR_HANDLING
	switch (access) {
	case GL_READ_WRITE:
	case GL_READ_ONLY:
	case GL_WRITE_ONLY:
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL);
	}
	
	if (!gpu_buf || gpu_buf->mapped) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_OPERATION, NULL)
	}
#endif

	// TODO: Current implementation doesn't take into account 'used' state
	gpu_buf->mapped = GL_TRUE;
	return gpu_buf->ptr;
}

void *glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL)
	}
	
#ifndef SKIP_ERROR_HANDLING
	if (!gpu_buf || gpu_buf->mapped) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_OPERATION, NULL)
	} else if (offset < 0 || length < 0 || offset + length > gpu_buf->size) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_VALUE, NULL)
	}
#endif
	
	// TODO: Current implementation doesn't take into account 'used' state
	gpu_buf->mapped = GL_TRUE;
	return (void *)((uint8_t *)gpu_buf->ptr + offset);
}

GLboolean glUnmapBuffer(GLenum target) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, GL_TRUE)
	}

#ifndef SKIP_ERROR_HANDLING
	if (!gpu_buf || !gpu_buf->mapped) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_OPERATION, GL_TRUE)
	}
#endif
	
	gpu_buf->used = GL_FALSE;
	gpu_buf->mapped = GL_FALSE;
	return GL_TRUE;
}

void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
	gpubuffer *gpu_buf;
	switch (target) {
	case GL_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		gpu_buf = (gpubuffer *)index_array_unit;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}

#ifndef SKIP_ERROR_HANDLING
	if (!gpu_buf || !gpu_buf->mapped) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (offset < 0 || length < 0 || offset + length > gpu_buf->size) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}

	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;

	vertex_object = gpu_alloc_mapped_temp(count * streams->stride);
	vgl_fast_memcpy(vertex_object, pointer, count * streams->stride);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}

	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;

	color_object = gpu_alloc_mapped_temp(count * streams->stride);
	vgl_fast_memcpy(color_object, pointer, count * streams->stride);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}

	attributes->componentCount = size;
	streams->stride = stride ? stride : bpe * size;

	texture_object = gpu_alloc_mapped_temp(count * streams->stride);
	vgl_fast_memcpy(texture_object, pointer, count * streams->stride);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
	}
	index_object = gpu_alloc_mapped_temp(count * bpe);
	if (stride == 0)
		vgl_fast_memcpy(index_object, pointer, count * bpe);
	else {
		int i;
		uint8_t *dst = (uint8_t *)index_object;
		uint8_t *src = (uint8_t *)pointer;
		for (i = 0; i < count; i++) {
			vgl_fast_memcpy(dst, src, bpe);
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
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
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
