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

static framebuffer framebuffers[BUFFERS_NUM]; // Framebuffers array
static renderbuffer renderbuffers[BUFFERS_NUM]; // Renderbuffers array

framebuffer *active_read_fb = NULL; // Current readback framebuffer in use
framebuffer *active_write_fb = NULL; // Current write framebuffer in use
renderbuffer *active_rb = NULL; // Current renderbuffer in use

uint32_t get_color_from_texture(SceGxmTextureFormat type) {
	uint32_t res = 0;
	switch (type) {
	case SCE_GXM_TEXTURE_FORMAT_U8_R:
		res = SCE_GXM_COLOR_FORMAT_U8_R;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR:
		res = SCE_GXM_COLOR_FORMAT_U8U8U8_BGR;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB:
		res = SCE_GXM_COLOR_FORMAT_U5U6U5_RGB;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR:
		res = SCE_GXM_COLOR_FORMAT_U8U8U8U8_ABGR;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB:
		res = SCE_GXM_COLOR_FORMAT_U8U8U8U8_ARGB;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_ABGR:
		res = SCE_GXM_COLOR_FORMAT_U4U4U4U4_ABGR;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA:
		res = SCE_GXM_COLOR_FORMAT_U4U4U4U4_RGBA;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U1U5U5U5_ABGR:
		res = SCE_GXM_COLOR_FORMAT_U1U5U5U5_ABGR;
		break;
	case SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA:
		res = SCE_GXM_COLOR_FORMAT_U5U5U5U1_RGBA;
		break;
	case SCE_GXM_TEXTURE_FORMAT_F16F16F16F16_RGBA:
		res = SCE_GXM_COLOR_FORMAT_F16F16F16F16_RGBA;
		break;
	default:
		SET_GL_ERROR_WITH_RET_AND_VALUE(GL_INVALID_ENUM, 0, type)
	}
	return res;
}

uint32_t get_alpha_channel_size(SceGxmColorFormat type) {
	switch (type) {
	case SCE_GXM_COLOR_FORMAT_U8_R:
	case SCE_GXM_COLOR_FORMAT_U8U8U8_BGR:
	case SCE_GXM_COLOR_FORMAT_U5U6U5_RGB:
		return 0;
	case SCE_GXM_COLOR_FORMAT_U4U4U4U4_ABGR:
	case SCE_GXM_COLOR_FORMAT_U4U4U4U4_RGBA:
		return 4;
	case SCE_GXM_COLOR_FORMAT_U1U5U5U5_ABGR:
	case SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA:
		return 1;
	case SCE_GXM_COLOR_FORMAT_F16F16F16F16_RGBA:
		return 16;
	default:
		return 8;
	}
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
			framebuffers[i].is_depth_hidden = GL_FALSE;
			framebuffers[i].depthbuffer_ptr = NULL;
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
			if (fb->depthbuffer_ptr && fb->is_depth_hidden)
				markAsDirty(fb->depthbuffer_ptr->depthData);
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
		renderbuffer *rb = (renderbuffer *)ids[--n];
		if (rb) {
			// Check if the framebuffer is currently bound
			if (active_read_fb && active_read_fb->depthbuffer_ptr == &rb->depthbuffer)
				active_read_fb->depthbuffer_ptr = NULL;
			if (active_write_fb && active_write_fb->depthbuffer_ptr == &rb->depthbuffer)
				active_write_fb->depthbuffer_ptr = NULL;
			if (active_rb == rb)
				active_rb = NULL;

			rb->active = GL_FALSE;
			if (rb->depthbuffer_ptr) {
				markAsDirty(rb->depthbuffer_ptr->depthData);
				if (rb->depthbuffer_ptr->stencilData)
					markAsDirty(rb->depthbuffer_ptr->stencilData);
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
	if (renderbuffertarget != GL_RENDERBUFFER) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, renderbuffertarget)
	}
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

	// Discarding any previously bound hidden depth buffers
	if (fb->depthbuffer_ptr && fb->is_depth_hidden) {
		markAsDirty(fb->depthbuffer_ptr->depthData);
		fb->is_depth_hidden = GL_FALSE;
	}

	switch (attachment) {
	case GL_DEPTH_STENCIL_ATTACHMENT:
	case GL_DEPTH_ATTACHMENT:
	case GL_STENCIL_ATTACHMENT:
		if (active_rb)
			fb->depthbuffer_ptr = active_rb->depthbuffer_ptr;
		else
			fb->depthbuffer_ptr = NULL;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, attachment)
	}
}

void glNamedFramebufferRenderbuffer(GLuint target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
#ifndef SKIP_ERROR_HANDLING
	if (renderbuffertarget != GL_RENDERBUFFER) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, renderbuffertarget)
	}
#endif

	framebuffer *fb = (framebuffer *)target;

	// Discarding any previously bound hidden depth buffers
	if (fb->depthbuffer_ptr && fb->is_depth_hidden) {
		markAsDirty(fb->depthbuffer_ptr->depthData);
		fb->is_depth_hidden = GL_FALSE;
	}

	switch (attachment) {
	case GL_DEPTH_STENCIL_ATTACHMENT:
	case GL_DEPTH_ATTACHMENT:
	case GL_STENCIL_ATTACHMENT:
		if (active_rb)
			fb->depthbuffer_ptr = active_rb->depthbuffer_ptr;
		else
			fb->depthbuffer_ptr = NULL;
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

	if (active_rb->depthbuffer_ptr) {
		markAsDirty(active_rb->depthbuffer_ptr->depthData);
		if (active_rb->depthbuffer_ptr->stencilData)
			markAsDirty(active_rb->depthbuffer_ptr->stencilData);
	}

	switch (internalformat) {
	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:
		initDepthStencilBuffer(width, height, &active_rb->depthbuffer, GL_TRUE);
		break;
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_COMPONENT32F:
		initDepthStencilBuffer(width, height, &active_rb->depthbuffer, GL_FALSE);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, internalformat)
		break;
	}

	active_rb->depthbuffer_ptr = &active_rb->depthbuffer;
}

void glNamedRenderbufferStorage(GLuint target, GLenum internalformat, GLsizei width, GLsizei height) {
	renderbuffer *rb = (renderbuffer *)target;
	if (rb->depthbuffer_ptr) {
		markAsDirty(rb->depthbuffer_ptr->depthData);
		if (rb->depthbuffer_ptr->stencilData)
			markAsDirty(rb->depthbuffer_ptr->stencilData);
	}

	switch (internalformat) {
	case GL_DEPTH24_STENCIL8:
	case GL_DEPTH32F_STENCIL8:
		initDepthStencilBuffer(width, height, &rb->depthbuffer, GL_TRUE);
		break;
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	case GL_DEPTH_COMPONENT32F:
		initDepthStencilBuffer(width, height, &rb->depthbuffer, GL_FALSE);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, internalformat)
		break;
	}

	rb->depthbuffer_ptr = &rb->depthbuffer;
}

inline void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint tex_id, GLint level) {
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
	int old_w = fb->width, old_h = fb->height;
	SceGxmTextureFormat fmt = sceGxmTextureGetFormat(&tex->gxm_tex);

	// Detecting requested attachment
	switch (attachment) {
	case GL_COLOR_ATTACHMENT0:
		fb->width = sceGxmTextureGetWidth(&tex->gxm_tex);
		fb->height = sceGxmTextureGetHeight(&tex->gxm_tex);
		fb->stride = VGL_ALIGN(fb->width, 8) * tex_format_to_bytespp(fmt);
		fb->data = sceGxmTextureGetData(&tex->gxm_tex);
		fb->data_type = tex->type;

		// Discarding any previously bound hidden depth buffer
		if (fb->depthbuffer_ptr && fb->is_depth_hidden) {
			markAsDirty(fb->depthbuffer_ptr->depthData);
			fb->depthbuffer_ptr = NULL;
			fb->is_depth_hidden = GL_FALSE;
		}
	
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
		} else if (fb->target && (old_w != fb->width || old_h != fb->height)) {
			markRtAsDirty(fb->target);
			fb->target = NULL;
		}

		// Increasing texture reference counter
		fb->tex = tex;
		tex->ref_counter++;

		// Checking if the framebuffer requires extended register size
		fb->is_float = fmt == SCE_GXM_TEXTURE_FORMAT_F16F16F16F16_RGBA;

		// Allocating colorbuffer
		sceGxmColorSurfaceInit(&fb->colorbuffer, get_color_from_texture(fmt), SCE_GXM_COLOR_SURFACE_LINEAR,
			msaa_mode == SCE_GXM_MULTISAMPLE_NONE ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			fb->is_float ? SCE_GXM_OUTPUT_REGISTER_SIZE_64BIT : SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			fb->width, fb->height, VGL_ALIGN(fb->width, 8), fb->data);
		
		// Invalidating current framebuffer if we update its bound texture to force a scene reset
		if (in_use_framebuffer == active_write_fb) {
			dirty_framebuffer = GL_TRUE;
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, attachment)
	}
}

inline void glNamedFramebufferTexture2D(GLuint target, GLenum attachment, GLenum textarget, GLuint tex_id, GLint level) {
	framebuffer *fb = (framebuffer *)target;

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
	int old_w = fb->width, old_h = fb->height;
	SceGxmTextureFormat fmt = sceGxmTextureGetFormat(&tex->gxm_tex);

	// Detecting requested attachment
	switch (attachment) {
	case GL_COLOR_ATTACHMENT0:
		fb->width = sceGxmTextureGetWidth(&tex->gxm_tex);
		fb->height = sceGxmTextureGetHeight(&tex->gxm_tex);
		fb->stride = VGL_ALIGN(fb->width, 8) * tex_format_to_bytespp(fmt);
		fb->data = sceGxmTextureGetData(&tex->gxm_tex);
		fb->data_type = tex->type;

		// Discarding any previously bound hidden depth buffer
		if (fb->depthbuffer_ptr && fb->is_depth_hidden) {
			markAsDirty(fb->depthbuffer_ptr->depthData);
			fb->depthbuffer_ptr = NULL;
			fb->is_depth_hidden = GL_FALSE;
		}
	
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
		} else if (fb->target && (old_w != fb->width || old_h != fb->height)) {
			markRtAsDirty(fb->target);
			fb->target = NULL;
		}

		// Increasing texture reference counter
		fb->tex = tex;
		tex->ref_counter++;

		// Checking if the framebuffer requires extended register size
		fb->is_float = fmt == SCE_GXM_TEXTURE_FORMAT_F16F16F16F16_RGBA;

		// Allocating colorbuffer
		sceGxmColorSurfaceInit(&fb->colorbuffer, get_color_from_texture(fmt), SCE_GXM_COLOR_SURFACE_LINEAR,
			msaa_mode == SCE_GXM_MULTISAMPLE_NONE ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			fb->is_float ? SCE_GXM_OUTPUT_REGISTER_SIZE_64BIT : SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			fb->width, fb->height, VGL_ALIGN(fb->width, 8), fb->data);
		
		// Invalidating current framebuffer if we update its bound texture to force a scene reset
		if (in_use_framebuffer == active_write_fb) {
			dirty_framebuffer = GL_TRUE;
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, attachment)
	}
}

void glFramebufferTexture(GLenum target, GLenum attachment, GLuint tex_id, GLint level) {
	glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, tex_id, level);
}

void glNamedFramebufferTexture(GLuint target, GLenum attachment, GLuint tex_id, GLint level) {
	glNamedFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, tex_id, level);
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

	return (!fb || fb->tex) ? GL_FRAMEBUFFER_COMPLETE : GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
}

GLenum glCheckNamedFramebufferStatus(GLuint target, GLenum dummy) {
	framebuffer *fb = (framebuffer *)target;

	return (!fb || fb->tex) ? GL_FRAMEBUFFER_COMPLETE : GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
}

void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
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

	// Detecting requested attachment
	switch (attachment) {
	case GL_COLOR_ATTACHMENT0:
		switch (pname) {
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
			if (!fb || !fb->tex)
				*params = GL_NONE;
			else
				*params = GL_TEXTURE;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_OPERATION, attachment)
	}
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
		case GL_BGRA:
			read_cb = readBGRA;
			src_bpp = 4;
			break;
		default:
			break;
		}
		if (format == active_read_fb->data_type) {
			fast_store = GL_TRUE;
			dst_bpp = src_bpp;
		}
		src = (uint8_t *)active_read_fb->data;
		stride = active_read_fb->stride;
		y = (active_read_fb->height - (height + y)) * stride;
	} else {
		src = (uint8_t *)gxm_color_surfaces_addr[gxm_back_buffer_index];
		stride = DISPLAY_STRIDE * 4;
		y = (DISPLAY_HEIGHT - (height + y)) * stride;
		src_bpp = 4;
		if (format == GL_RGBA) {
			fast_store = GL_TRUE;
			dst_bpp = src_bpp;
		} else
			read_cb = readRGBA;
	}

	if (!fast_store) {
		switch (format) {
		case GL_RGBA:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				write_cb = writeRGBA;
				dst_bpp = 4;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
			}
			break;
		case GL_BGRA:
			switch (type) {
			case GL_UNSIGNED_BYTE:
			case GL_UNSIGNED_INT_8_8_8_8_REV:
				write_cb = writeBGRA;
				dst_bpp = 4;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
			}
			break;
		case GL_RGB:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				write_cb = writeRGB;
				dst_bpp = 3;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
			}
			break;
		case GL_BGR:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				write_cb = writeBGR;
				dst_bpp = 3;
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
	uint8_t *data_u8 = (uint8_t *)data + (width * dst_bpp * (height - 1));
#else
	uint8_t *data_u8 = active_read_fb ? (uint8_t *)data : ((uint8_t *)data + (width * dst_bpp * (height - 1)));
#endif
	if (fast_store) {
		for (int i = 0; i < height; i++) {
			vgl_fast_memcpy(data_u8, &src[y + x * src_bpp], width * src_bpp);
			y += stride;
#ifdef HAVE_UNFLIPPED_FBOS
			data_u8 -= width * src_bpp;
#else
			data_u8 -= (active_read_fb ? -width : width) * src_bpp;
#endif
		}
	} else {
		for (int i = 0; i < height; i++) {
			uint8_t *line_src = &src[y + i * stride + x * src_bpp];
			uint8_t *line_dst = data_u8;
			for (int j = 0; j < width; j++) {
				uint32_t clr = read_cb(line_src);
				write_cb(line_dst, clr);
				line_src += src_bpp;
				line_dst += dst_bpp;
			}
#ifdef HAVE_UNFLIPPED_FBOS
			data_u8 -= width * dst_bpp;
#else
			data_u8 -= (active_read_fb ? -width : width) * dst_bpp;
#endif
		}
	}
}

void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	// Invalidate current write framebuffer binding
	framebuffer *real_write_fb = active_write_fb;
	active_write_fb = (framebuffer *)drawFramebuffer;
	
	switch (mask) {
	case GL_COLOR_BUFFER_BIT:
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_OPERATION, mask)
	}

	sceneReset();

	// Invalidating viewport and culling
	invalidate_viewport();
	sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_NONE);

	// Invalidate depth test and depth write
	orig_depth_test = depth_test_state;
	invalidate_depth_test();
	change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
	
	// Force polygon fill mode and no depth bias
	sceGxmSetFrontDepthBias(gxm_context, 0, 0);
	sceGxmSetBackDepthBias(gxm_context, 0, 0);
	sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
	sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);

	// Set framebuffer blit shader
	sceGxmSetVertexProgram(gxm_context, blit_vertex_program_patched);
	if (is_fbo_float)
		sceGxmSetFragmentProgram(gxm_context, blit_fragment_program_float_patched);
	else
		sceGxmSetFragmentProgram(gxm_context, blit_fragment_program_patched);
	
	// Set fragment texture to read framebuffer bound color attachment
	framebuffer *read_fb = (framebuffer *)readFramebuffer;
	SceGxmTexture *tex = readFramebuffer ? &read_fb->tex->gxm_tex : &gxm_color_surfaces[gxm_back_buffer_index].backgroundTex;
	if (filter == GL_LINEAR) {
		vglSetTexMagFilter(tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
		vglSetTexMinFilter(tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
		vglSetTexMipmapCount(tex, 0);
	} else {
		vglSetTexMagFilter(tex, SCE_GXM_TEXTURE_FILTER_POINT);
		vglSetTexMinFilter(tex, SCE_GXM_TEXTURE_FILTER_POINT);
		vglSetTexMipmapCount(tex, 0);
	}
	sceGxmSetFragmentTexture(gxm_context, 0, tex);
	
	// Set stencil func to keep original data
	sceGxmSetFrontStencilFunc(gxm_context,
		SCE_GXM_STENCIL_FUNC_ALWAYS,
		SCE_GXM_STENCIL_OP_KEEP,
		SCE_GXM_STENCIL_OP_KEEP,
		SCE_GXM_STENCIL_OP_KEEP,
		0xFF, 0xFF);
	sceGxmSetBackStencilFunc(gxm_context,
		SCE_GXM_STENCIL_FUNC_ALWAYS,
		SCE_GXM_STENCIL_OP_KEEP,
		SCE_GXM_STENCIL_OP_KEEP,
		SCE_GXM_STENCIL_OP_KEEP,
		0xFF, 0xFF);
	
	// Filling position and texcoord values
	float *vertex_data = (float *)gpu_alloc_mapped_temp(16 * sizeof(float));
	// Position
	vector4f tmp;
	vector4f_convert_to_local_space(&tmp, dstX0, dstY0, dstX1 - dstX0, dstY1 - dstY0);
	vertex_data[0] = vertex_data[2] = tmp.x; // X0
	vertex_data[1] = vertex_data[7] = tmp.z; // Y0
	vertex_data[4] = vertex_data[6] = tmp.y; // X1
	vertex_data[3] = vertex_data[5] = tmp.w; // Y1
	// Texcoords
	vertex_data[8] = vertex_data[10] = (float)srcX0 / (float)read_fb->width; // X0
	vertex_data[9] = vertex_data[15] = (float)srcY0 / (float)read_fb->height; // Y0
	vertex_data[12] = vertex_data[14] = (float)srcX1 / (float)read_fb->width; // X1
	vertex_data[11] = vertex_data[13] = (float)srcY1 / (float)read_fb->height; // Y1
	sceGxmSetVertexStream(gxm_context, 0, vertex_data);
	sceGxmSetVertexStream(gxm_context, 1, &vertex_data[8]);

	// Draw read framebuffer on top of write framebuffer
	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);

	// Restore all invalidated configurations
	if (readFramebuffer) {
		vglSetTexMagFilter(tex, read_fb->tex->mag_filter);
		vglSetTexMinFilter(tex, read_fb->tex->min_filter);
		vglSetTexMipmapCount(tex, read_fb->tex->use_mips ? read_fb->tex->mip_count : 0);
	}
	validate_depth_test();
	change_depth_write((depth_mask_state && depth_test_state) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
	change_stencil_settings();
	sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front);
	sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back);
	update_polygon_offset();
	validate_viewport();
	change_cull_mode();
	
	// Restoring write framebuffer binding
	active_write_fb = real_write_fb;
}

void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	glBlitNamedFramebuffer((GLuint)active_read_fb, (GLuint)active_write_fb, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

/* vgl* */

void vglTexImageDepthBuffer(GLenum target) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id[0];
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_2D: {
		if (active_read_fb) {
			sceGxmDepthStencilSurfaceSetForceStoreMode(active_read_fb->depthbuffer_ptr, SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED);
			sceGxmTextureInitLinear(&tex->gxm_tex, active_read_fb->depthbuffer_ptr->depthData, SCE_GXM_TEXTURE_FORMAT_DF32M, active_read_fb->width, active_read_fb->height, 0);
		} else {
			sceGxmDepthStencilSurfaceSetForceStoreMode(&gxm_depth_stencil_surface, SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED);
			sceGxmTextureInitLinear(&tex->gxm_tex, gxm_depth_stencil_surface.depthData, SCE_GXM_TEXTURE_FORMAT_DF32M, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
		}
		tex->status = TEX_VALID;
	} break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

GLboolean glIsFramebuffer(GLuint fb) {
	framebuffer *p = (framebuffer *)fb;
	return (p && p->active);
}

GLboolean glIsRenderbuffer(GLuint rb) {
	renderbuffer *p = (renderbuffer *)rb;
	return (p && p->active);
}
