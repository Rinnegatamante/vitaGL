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
 * framebuffers.c:
 * Implementation for framebuffers related functions
 */

#include "shared.h"

extern void *gxm_color_surfaces_addr[DISPLAY_MAX_BUFFER_COUNT]; // Display color surfaces memblock starting addresses
extern unsigned int gxm_back_buffer_index; // Display back buffer id

static framebuffer framebuffers[BUFFERS_NUM]; // Framebuffers array
static renderbuffer renderbuffers[BUFFERS_NUM]; // Renderbuffers array

framebuffer *active_read_fb = NULL; // Current readback framebuffer in use
framebuffer *active_write_fb = NULL; // Current write framebuffer in use
renderbuffer *active_rb = NULL; // Current renderbuffer in use

uint32_t get_color_from_texture(uint32_t type) {
	uint32_t res = 0;
	switch (type) {
	case GL_RGB:
		res = SCE_GXM_COLOR_FORMAT_U8U8U8_BGR;
		break;
	case GL_RGBA:
		res = SCE_GXM_COLOR_FORMAT_U8U8U8U8_ABGR;
		break;
	case GL_LUMINANCE:
		res = SCE_GXM_COLOR_FORMAT_U8_R;
		break;
	case GL_LUMINANCE_ALPHA:
		res = SCE_GXM_COLOR_FORMAT_U8U8_GR;
		break;
	case GL_INTENSITY:
		res = SCE_GXM_COLOR_FORMAT_U8_R;
		break;
	case GL_ALPHA:
		res = SCE_GXM_COLOR_FORMAT_U8_A;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, 0)
	}
	return res;
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glGenFramebuffers(GLsizei n, GLuint *ids) {
	int i = 0, j = 0;
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	for (i = 0; i < BUFFERS_NUM; i++) {
		if (!framebuffers[i].active) {
			ids[j++] = (GLuint)&framebuffers[i];
			framebuffers[i].active = GL_TRUE;
			framebuffers[i].depth_buffer_addr = NULL;
			framebuffers[i].depthbuffer_ptr = NULL;
			framebuffers[i].stencil_buffer_addr = NULL;
			framebuffers[i].target = NULL;
			framebuffers[i].tex = NULL;
		}
		if (j >= n)
			break;
	}
}

void glGenRenderbuffers(GLsizei n, GLuint *ids) {
	int i = 0, j = 0;
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	for (i = 0; i < BUFFERS_NUM; i++) {
		if (!renderbuffers[i].active) {
			ids[j++] = (GLuint)&renderbuffers[i];
			renderbuffers[i].active = GL_TRUE;
			renderbuffers[i].depth_buffer_addr = NULL;
			renderbuffers[i].stencil_buffer_addr = NULL;
		}
		if (j >= n)
			break;
	}
}

void glDeleteFramebuffers(GLsizei n, const GLuint *ids) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	while (n > 0) {
		framebuffer *fb = (framebuffer *)ids[--n];
		if (fb) {
			// Check if the framebuffer is currently bound
			if (fb == active_read_fb)
				active_read_fb = NULL;
			if (fb == active_write_fb)
				active_write_fb = NULL;

			fb->active = GL_FALSE;
			if (fb->tex) {
				fb->tex->ref_counter--;
				if (fb->tex->dirty && fb->tex->ref_counter == 0) {
					gpu_free_texture(fb->tex);
				}
				fb->tex = NULL;
			}
			if (fb->target)
				markRtAsDirty(fb->target);
			if (fb->depth_buffer_addr) {
				markAsDirty(fb->depth_buffer_addr);
				markAsDirty(fb->stencil_buffer_addr);
			}
		}
	}
}

void glDeleteRenderbuffers(GLsizei n, const GLuint *ids) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	while (n > 0) {
		renderbuffer *fb = (renderbuffer *)ids[--n];
		if (fb) {
			// Check if the framebuffer is currently bound
			if (active_read_fb && active_read_fb->depthbuffer_ptr == &fb->depthbuffer)
				active_read_fb->depthbuffer_ptr = &active_read_fb->depthbuffer;
			if (active_write_fb && active_write_fb->depthbuffer_ptr == &fb->depthbuffer)
				active_write_fb->depthbuffer_ptr = &active_write_fb->depthbuffer;
			if (active_rb == fb)
				active_rb = NULL;

			fb->active = GL_FALSE;
			if (fb->depth_buffer_addr) {
				markAsDirty(fb->depth_buffer_addr);
				if (fb->stencil_buffer_addr)
					markAsDirty(fb->stencil_buffer_addr);
			}
		}
	}
}

void glBindFramebuffer(GLenum target, GLuint fb) {
	switch (target) {
	case GL_DRAW_FRAMEBUFFER:
		active_write_fb = (framebuffer *)fb;
		break;
	case GL_READ_FRAMEBUFFER:
		active_read_fb = (framebuffer *)fb;
		break;
	case GL_FRAMEBUFFER:
		active_write_fb = active_read_fb = (framebuffer *)fb;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glBindRenderbuffer(GLenum target, GLuint rb) {
	switch (target) {
	case GL_RENDERBUFFER:
		active_rb = (renderbuffer *)rb;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
#ifndef SKIP_ERROR_HANDLING
	if (renderbuffertarget != GL_RENDERBUFFER)
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, renderbuffertarget)
#endif

	framebuffer *fb;
	switch (target) {
	case GL_FRAMEBUFFER:
	case GL_DRAW_FRAMEBUFFER:
		fb = active_write_fb;
		break;
	case GL_READ_FRAMEBUFFER:
		fb = active_read_fb;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
		break;
	}

	switch (attachment) {
	case GL_DEPTH_STENCIL_ATTACHMENT:
	case GL_DEPTH_ATTACHMENT:
		if (active_rb)
			fb->depthbuffer_ptr = active_rb->depthbuffer_ptr;
		else
			fb->depthbuffer_ptr = &fb->depthbuffer; // Resetting to temporary depth buffer
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, attachment)
	}
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
#ifndef SKIP_ERROR_HANDLING
	if (target != GL_RENDERBUFFER) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
	if (width < 0 || height < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	if (active_rb->depth_buffer_addr) {
		markAsDirty(active_rb->depth_buffer_addr);
		if (active_rb->stencil_buffer_addr)
			markAsDirty(active_rb->stencil_buffer_addr);
	}

	switch (internalformat) {
	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:
		initDepthStencilBuffer(width, height, &active_rb->depthbuffer, &active_rb->depth_buffer_addr, &active_rb->stencil_buffer_addr);
		break;
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_COMPONENT32F:
		initDepthStencilBuffer(width, height, &active_rb->depthbuffer, &active_rb->depth_buffer_addr, NULL);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, internalformat)
		break;
	}

	sceGxmDepthStencilSurfaceSetForceStoreMode(&active_rb->depthbuffer, SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED);
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint tex_id, GLint level) {
	// Detecting requested framebuffer
	framebuffer *fb = NULL;
	switch (target) {
	case GL_DRAW_FRAMEBUFFER:
	case GL_FRAMEBUFFER:
		fb = active_write_fb;
		break;
	case GL_READ_FRAMEBUFFER:
		fb = active_read_fb;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}

#ifndef SKIP_ERROR_HANDLING
	if (!fb) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (textarget != GL_TEXTURE_2D) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, textarget)
	}
#endif

	// Aliasing to make code more readable
	texture *tex = &texture_slots[tex_id];

	// Extracting texture data
	fb->width = sceGxmTextureGetWidth(&tex->gxm_tex);
	fb->height = sceGxmTextureGetHeight(&tex->gxm_tex);
	fb->stride = ALIGN(fb->width, 8) * tex_format_to_bytespp(sceGxmTextureGetFormat(&tex->gxm_tex));
	fb->data = sceGxmTextureGetData(&tex->gxm_tex);
	fb->data_type = tex->type;

	// Detecting requested attachment
	switch (attachment) {
	case GL_COLOR_ATTACHMENT0:
		// Clearing previously attached texture
		if (fb->tex) {
			fb->tex->ref_counter--;
			if (fb->tex->dirty && fb->tex->ref_counter == 0) {
				gpu_free_texture(fb->tex);
			}
		}

		// Detaching attached texture if passed texture ID is 0
		if (tex_id == 0) {
			if (fb->target) {
				markRtAsDirty(fb->target);
				fb->target = NULL;
			}
			fb->tex = NULL;
			return;
		}

		// Increasing texture reference counter
		fb->tex = tex;
		tex->ref_counter++;

		// Allocating colorbuffer
		sceGxmColorSurfaceInit(
			&fb->colorbuffer,
			get_color_from_texture(tex->type),
			SCE_GXM_COLOR_SURFACE_LINEAR,
			msaa_mode == SCE_GXM_MULTISAMPLE_NONE ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			fb->width, fb->height, ALIGN(fb->width, 8), fb->data);

		// Allocating temporary depth and stencil buffers (if necessary) to ensure scissoring works
		if (!fb->depth_buffer_addr) {
			initDepthStencilBuffer(fb->width, fb->height, &fb->depthbuffer, &fb->depth_buffer_addr, &fb->stencil_buffer_addr);
			if (!fb->depthbuffer_ptr)
				fb->depthbuffer_ptr = &fb->depthbuffer;
		}

		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, attachment)
	}
}

void glFramebufferTexture(GLenum target, GLenum attachment, GLuint tex_id, GLint level) {
	glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, tex_id, level);
}

GLenum glCheckFramebufferStatus(GLenum target) {
	// Detecting requested framebuffer
	framebuffer *fb = NULL;
	switch (target) {
	case GL_DRAW_FRAMEBUFFER:
	case GL_FRAMEBUFFER:
		fb = active_write_fb;
		break;
	case GL_READ_FRAMEBUFFER:
		fb = active_read_fb;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, GL_FRAMEBUFFER_COMPLETE)
	}

	if (!fb)
		return GL_FRAMEBUFFER_COMPLETE;
	else
		return fb->tex ? GL_FRAMEBUFFER_COMPLETE : GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *data) {
	/*
	 * Callbacks are actually used to just perform down/up-sampling
	 * between U8 texture formats. Reads are expected to give as result
	 * a RGBA sample that will be wrote depending on texture format
	 * by the write callback
	 */
	void (*write_cb)(void *, uint32_t) = NULL;
	uint32_t (*read_cb)(void *) = NULL;

	GLboolean fast_store = GL_FALSE;
	uint8_t *src;
	int stride, src_bpp, dst_bpp;
	if (active_read_fb) {
		switch (active_read_fb->data_type) {
		case GL_RGBA:
			read_cb = readRGBA;
			src_bpp = 4;
			break;
		case GL_RGB:
			read_cb = readRGB;
			src_bpp = 3;
			break;
		default:
			break;
		}
		if (format == active_read_fb->data_type)
			fast_store = GL_TRUE;
		src = (uint8_t *)active_read_fb->data;
		stride = active_read_fb->stride;
		y = (active_read_fb->height - (height + y)) * stride;
	} else {
		src = (uint8_t *)gxm_color_surfaces_addr[gxm_back_buffer_index];
		stride = DISPLAY_STRIDE * 4;
		y = (DISPLAY_HEIGHT - (height + y)) * stride;
		src_bpp = 4;
		if (format == GL_RGBA)
			fast_store = GL_TRUE;
		else
			read_cb = readRGBA;
	}

	if (!fast_store) {
		switch (format) {
		case GL_RGBA:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				write_cb = writeRGBA;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
			}
			break;
		case GL_RGB:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				write_cb = writeRGB;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
			}
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, format)
		}
	}

#ifdef HAVE_UNFLIPPED_FBOS
	uint8_t *data_u8 = data + (width * src_bpp * (height - 1));
#else
	uint8_t *data_u8 = active_read_fb ? data : (data + (width * src_bpp * (height - 1)));
#endif
	int i;
	if (fast_store) {
		for (i = 0; i < height; i++) {
			sceClibMemcpy(data_u8, &src[y + x * src_bpp], width * src_bpp);
			y += stride;
#ifdef HAVE_UNFLIPPED_FBOS
			data_u8 -= width * src_bpp;
#else
			data_u8 -= (active_read_fb ? -width : width) * src_bpp;
#endif
		}
	} else {
		int j;
		for (i = 0; i < height; i++) {
			src = &src[y + i * stride + x * src_bpp];
			uint8_t *line_u8 = data_u8;
			for (j = 0; j < width; j++) {
				uint32_t clr = read_cb(src);
				write_cb(line_u8, clr);
				src += src_bpp;
				line_u8 += dst_bpp;
			}
#ifdef HAVE_UNFLIPPED_FBOS
			data_u8 -= width * src_bpp;
#else
			data_u8 -= (active_read_fb ? -width : width) * src_bpp;
#endif
		}
	}
}

/* vgl* */

void vglTexImageDepthBuffer(GLenum target) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_2D: {
		if (active_read_fb)
			sceGxmTextureInitLinear(&tex->gxm_tex, active_read_fb->depth_buffer_addr, SCE_GXM_TEXTURE_FORMAT_DF32M, active_read_fb->width, active_read_fb->height, 0);
		else
			sceGxmTextureInitLinear(&tex->gxm_tex, gxm_depth_surface_addr, SCE_GXM_TEXTURE_FORMAT_DF32M, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
		tex->status = TEX_VALID;
	} break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}
