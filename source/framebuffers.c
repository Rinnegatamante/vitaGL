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

framebuffer *active_read_fb = NULL; // Current readback framebuffer in use
framebuffer *active_write_fb = NULL; // Current write framebuffer in use

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
		vgl_error = GL_INVALID_ENUM;
		break;
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
			framebuffers[i].active = 1;
			framebuffers[i].depth_buffer_addr = NULL;
			framebuffers[i].stencil_buffer_addr = NULL;
		}
		if (j >= n)
			break;
	}
}

void glDeleteFramebuffers(GLsizei n, GLuint *framebuffers) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	while (n > 0) {
		framebuffer *fb = (framebuffer *)framebuffers[n--];
		if (fb) {
			fb->active = 0;
			if (fb->target) {
				sceGxmDestroyRenderTarget(fb->target);
				fb->target = NULL;
			}
			if (fb->depth_buffer_addr) {
				vgl_mem_free(fb->depth_buffer_addr);
				vgl_mem_free(fb->stencil_buffer_addr);
				fb->depth_buffer_addr = NULL;
				fb->stencil_buffer_addr = NULL;
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
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glFramebufferTexture(GLenum target, GLenum attachment, GLuint tex_id, GLint level) {
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
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
	
#ifndef SKIP_ERROR_HANDLING
	if (!fb) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
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

		// Allocating colorbuffer
		sceGxmColorSurfaceInit(
			&fb->colorbuffer,
			get_color_from_texture(tex->type),
			SCE_GXM_COLOR_SURFACE_LINEAR,
			msaa_mode == SCE_GXM_MULTISAMPLE_NONE ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			fb->width, fb->height, ALIGN(fb->width, 8), fb->data);

		// Allocating depth and stencil buffer (FIXME: This probably shouldn't be here)
		initDepthStencilBuffer(fb->width, fb->height, &fb->depthbuffer, &fb->depth_buffer_addr, &fb->stencil_buffer_addr);

		// Creating rendertarget
		SceGxmRenderTargetParams renderTargetParams;
		memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
		renderTargetParams.flags = 0;
		renderTargetParams.width = fb->width;
		renderTargetParams.height = fb->height;
		renderTargetParams.scenesPerFrame = 1;
		renderTargetParams.multisampleMode = msaa_mode;
		renderTargetParams.multisampleLocations = 0;
		renderTargetParams.driverMemBlock = -1;
		sceGxmCreateRenderTarget(&renderTargetParams, &fb->target);
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
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
			write_cb = writeRGBA;
			src_bpp = 4;
			break;
		case GL_RGB:
			write_cb = writeRGB;
			src_bpp = 3;
			break;
		default:
			break;
		}
		if (format == active_read_fb->data_type)
			fast_store = GL_TRUE;
		src = (uint8_t*)active_read_fb->data;
		y = active_read_fb->height - (height + y);
		stride = active_read_fb->stride;
	} else {
		src = (uint8_t*)gxm_color_surfaces_addr[gxm_back_buffer_index];
		y = DISPLAY_HEIGHT - (height + y);
		stride = DISPLAY_STRIDE * 4;
		src_bpp = 4;
		if (format == GL_RGBA)
			fast_store = GL_TRUE;
	}
	y *= stride;
	
	if (!fast_store) {
		switch (format) {
		case GL_RGBA:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				read_cb = readRGBA;
				break;
			default:
				SET_GL_ERROR(GL_INVALID_ENUM)
				break;
			}
			break;
		case GL_RGB:
			switch (type) {
			case GL_UNSIGNED_BYTE:
				read_cb = readRGB;
				break;
			default:
				SET_GL_ERROR(GL_INVALID_ENUM)
				break;
			}
			break;
		default:
			SET_GL_ERROR(GL_INVALID_ENUM)
			break;
		}
	}
	
	int i = 0;
	if (fast_store) {
		while (i < height) {
			memcpy_neon(data, &src[y + x * src_bpp], width * src_bpp);
			y += stride;
			i++;
		}
	} else {
		int j;
		uint8_t *data_u8;
		for (i = 0; i < height; i++) {
			src = &src[y + i * stride + x * src_bpp];
			for (j = 0; j < width; j++) {
				uint32_t clr = read_cb(src);
				write_cb(data_u8, clr);
				src += src_bpp;
				data_u8 += dst_bpp;
			}
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
	case GL_TEXTURE_2D:
		{
			if (active_read_fb)
				sceGxmTextureInitLinear(&tex->gxm_tex, active_read_fb->depth_buffer_addr, SCE_GXM_TEXTURE_FORMAT_DF32M, active_read_fb->width, active_read_fb->height, 0);
			else
				sceGxmTextureInitLinear(&tex->gxm_tex, gxm_depth_surface_addr, SCE_GXM_TEXTURE_FORMAT_DF32M, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
			tex->valid = 1;
		}
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}
