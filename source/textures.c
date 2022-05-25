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
 * textures.c:
 * Implementation for textures related functions
 */

#include "shared.h"

texture_unit texture_units[COMBINED_TEXTURE_IMAGE_UNITS_NUM]; // Available texture units
texture texture_slots[TEXTURES_NUM]; // Available texture slots

void *color_table = NULL; // Current in-use color table
int8_t server_texture_unit = 0; // Current in use server side texture unit

void _glTexImage2D_CubeIMPL(texture *tex, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data, int index) {
	SceGxmTextureFormat tex_format;
	SceGxmTransferFormat src_format;
	uint8_t data_bpp = 0;
	GLboolean fast_store = GL_FALSE;
	GLboolean gamma_correction = GL_FALSE;

	// Detecting proper read callaback and source bpp
	switch (format) {
	case GL_RED:
	case GL_ALPHA:
	case GL_LUMINANCE:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			src_format = SCE_GXM_TRANSFER_FORMAT_U8_R;
			data_bpp = 1;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RG:
	case GL_LUMINANCE_ALPHA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			src_format = SCE_GXM_TRANSFER_FORMAT_U8U8_GR;
			data_bpp = 2;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RGB:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			src_format = SCE_GXM_TRANSFER_FORMAT_U8U8U8_BGR;
			break;
		case GL_UNSIGNED_SHORT_5_6_5:
			data_bpp = 2;
			src_format = SCE_GXM_TRANSFER_FORMAT_U5U6U5_BGR;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RGBA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			src_format = SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR;
			break;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			data_bpp = 2;
			src_format = SCE_GXM_TRANSFER_FORMAT_U1U5U5U5_ABGR;
			break;
		case GL_UNSIGNED_SHORT_4_4_4_4:
			data_bpp = 2;
			src_format = SCE_GXM_TRANSFER_FORMAT_U4U4U4U4_ABGR;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, format)
		break;
	}

	// Detecting  texture format
	switch (internalFormat) {
	case GL_SRGB:
	case GL_SRGB8:
		gamma_correction = GL_TRUE;
	case GL_RGB:
		if (data_bpp == 2)
			tex_format = SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB;
		else
			tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR;
		break;
	case GL_SRGB_ALPHA:
	case GL_SRGB8_ALPHA8:
		gamma_correction = GL_TRUE;
	case GL_RGBA:
		if (data_bpp == 2) {
			if (src_format == SCE_GXM_TRANSFER_FORMAT_U1U5U5U5_ABGR)
				tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA;
			else
				tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA;
		} else
			tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
		break;
	default:
		tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
		break;
	}

	// Allocating texture/mipmaps depending on user call
	tex->type = internalFormat;
	if (level == 0) // FIXME: Add proper mipmaps support
		gpu_alloc_cube_texture(width, height, tex_format, src_format, data, tex, data_bpp, index);

	// Setting texture parameters
	vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
	vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
	vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
	vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
	vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
	if (gamma_correction)
		vglSetTexGammaMode(&tex->gxm_tex, SCE_GXM_TEXTURE_GAMMA_BGR);
}

void _glTexImage2D_FlatIMPL(texture *tex, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data) {
	SceGxmTextureFormat tex_format;
	uint8_t data_bpp = 0;
	GLboolean fast_store = GL_FALSE;
	GLboolean gamma_correction = GL_FALSE;

	/*
	 * Callbacks are actually used to just perform down/up-sampling
	 * between U8 texture formats. Reads are expected to give as result
	 * a RGBA sample that will be wrote depending on texture format
	 * by the write callback
	 */
	uint32_t (*read_cb)(void *) = NULL;

	// Detecting proper read callaback and source bpp
	switch (format) {
	case GL_RED:
	case GL_ALPHA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			read_cb = readR;
			data_bpp = 1;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_LUMINANCE:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 1;
			if (internalFormat == GL_LUMINANCE || internalFormat == GL_SLUMINANCE || internalFormat == GL_SLUMINANCE8)
				fast_store = GL_TRUE;
			else
				read_cb = readL;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RG:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			read_cb = readRG;
			data_bpp = 2;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_LUMINANCE_ALPHA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 2;
			if (internalFormat == GL_LUMINANCE_ALPHA || internalFormat == GL_SLUMINANCE_ALPHA || internalFormat == GL_SLUMINANCE8_ALPHA8)
				fast_store = GL_TRUE;
			else
				read_cb = readLA;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_BGR:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			if (internalFormat == GL_BGR)
				fast_store = GL_TRUE;
			else
				read_cb = readBGR;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RGB:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			if (internalFormat == GL_RGB)
				fast_store = GL_TRUE;
			else
				read_cb = readRGB;
			break;
		case GL_UNSIGNED_SHORT_5_6_5:
			data_bpp = 2;
			if (internalFormat == GL_RGB)
				fast_store = GL_TRUE;
			else
				read_cb = readRGB565;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_BGRA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			if (internalFormat == GL_BGRA)
				fast_store = GL_TRUE;
			else
				read_cb = readBGRA;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_ABGR_EXT:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			if (internalFormat == GL_ABGR_EXT)
				fast_store = GL_TRUE;
			else
				read_cb = readABGR;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RGBA:
		switch (type) {
		case GL_HALF_FLOAT:
		case GL_HALF_FLOAT_OES:
			data_bpp = 8;
			fast_store = GL_TRUE; // TODO: For now we assume half float textures are always stored with same internalformat
			break;
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			if (internalFormat == GL_RGBA)
				fast_store = GL_TRUE;
			else
				read_cb = readRGBA;
			break;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			data_bpp = 2;
			if (internalFormat == GL_RGBA)
				fast_store = GL_TRUE;
			read_cb = readRGBA5551;
			break;
		case GL_UNSIGNED_SHORT_4_4_4_4:
			data_bpp = 2;
			if (internalFormat == GL_RGBA)
				fast_store = GL_TRUE;
			read_cb = readRGBA4444;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, format)
		break;
	}

	// Detecting proper write callback and texture format
	tex->write_cb = NULL;
	switch (internalFormat) {
	case GL_RGBA16F:
		tex->write_cb = (void *)GL_TRUE; // Avoid to let this case fall in compressed texture case
		tex_format = SCE_GXM_TEXTURE_FORMAT_F16F16F16F16_RGBA;
		break;
	case GL_COMPRESSED_SRGB_S3TC_DXT1:
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1:
	case GL_COMPRESSED_SRGB:
		gamma_correction = GL_TRUE;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		tex_format = SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR;
		break;
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5:
	case GL_COMPRESSED_SRGB_ALPHA:
		gamma_correction = GL_TRUE;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		tex_format = SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR;
		break;
	case GL_SRGB:
	case GL_SRGB8:
		gamma_correction = GL_TRUE;
	case GL_RGB:
		tex->write_cb = writeRGB;
		if (fast_store && data_bpp == 2)
			tex_format = SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB;
		else
			tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR;
		break;
	case GL_BGR:
		tex->write_cb = writeBGR;
		tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8_RGB;
		break;
	case GL_SRGB_ALPHA:
	case GL_SRGB8_ALPHA8:
		gamma_correction = GL_TRUE;
	case GL_RGBA:
		tex->write_cb = writeRGBA;
		if (fast_store && data_bpp == 2) {
			if (read_cb == readRGBA5551)
				tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA;
			else
				tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA;
		} else
			tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
		break;
	case GL_BGRA:
		tex->write_cb = writeBGRA;
		tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
		break;
	case GL_ABGR_EXT:
		tex->write_cb = writeABGR;
		tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA;
		break;
	case GL_INTENSITY:
		tex->write_cb = writeR;
		tex_format = SCE_GXM_TEXTURE_FORMAT_U8_RRRR;
		break;
	case GL_ALPHA:
		tex->write_cb = writeR;
		tex_format = SCE_GXM_TEXTURE_FORMAT_A8;
		break;
	case GL_COLOR_INDEX8_EXT:
		tex->write_cb = writeR; // TODO: This is a hack
		tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
		break;
	case GL_SLUMINANCE:
	case GL_SLUMINANCE8:
		gamma_correction = GL_TRUE;
	case GL_LUMINANCE:
		tex->write_cb = writeR;
		tex_format = SCE_GXM_TEXTURE_FORMAT_L8;
		break;
	case GL_SLUMINANCE_ALPHA:
	case GL_SLUMINANCE8_ALPHA8:
		gamma_correction = GL_TRUE;
	case GL_LUMINANCE_ALPHA:
		tex->write_cb = writeRA;
		tex_format = SCE_GXM_TEXTURE_FORMAT_A8L8;
		break;
	default:
		tex->write_cb = writeRGBA;
		tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR;
		break;
	}

	// Allocating texture/mipmaps depending on user call
	tex->type = internalFormat;
	if (level == 0)
		if (tex->write_cb)
			gpu_alloc_texture(width, height, tex_format, data, tex, data_bpp, read_cb, tex->write_cb, fast_store);
		else
			gpu_alloc_compressed_texture(level, width, height, tex_format, 0, data, tex, data_bpp, read_cb);
	else if (tex->write_cb)
		gpu_alloc_mipmaps(level, tex);
	else
		gpu_alloc_compressed_texture(level, width, height, tex_format, 0, data, tex, data_bpp, read_cb);

	// Setting texture parameters
	vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
	vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
	vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
	vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
	vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
	vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
	vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
	if (gamma_correction)
		vglSetTexGammaMode(&tex->gxm_tex, SCE_GXM_TEXTURE_GAMMA_BGR);

	// Setting palette if the format requests one
	if (tex->palette_data) {
		vglSetTexPalette(&tex->gxm_tex, tex->palette_data);
		tex->palette_data = NULL;
	}
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glGenTextures(GLsizei n, GLuint *res) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Reserving a texture and returning its id if available
	int j = 0;
	for (GLuint i = 1; i < TEXTURES_NUM; i++) {
		if (texture_slots[i].status == TEX_UNUSED) {
			res[j++] = i;
			texture_slots[i].status = TEX_UNINITIALIZED;

			// Resetting texture parameters to their default values
			texture_slots[i].dirty = GL_FALSE;
			texture_slots[i].faces_counter = 0;
			texture_slots[i].ref_counter = 0;
			texture_slots[i].mip_count = 1;
#ifdef HAVE_UNPURE_TEXTURES
			texture_slots[i].mip_start = -1;
#endif
			texture_slots[i].use_mips = GL_FALSE;
			texture_slots[i].min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			texture_slots[i].mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			texture_slots[i].mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
			texture_slots[i].u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
			texture_slots[i].v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
			texture_slots[i].lod_bias = GL_MAX_TEXTURE_LOD_BIAS; // sceGxm range is 0 - (GL_MAX_TEXTURE_LOD_BIAS*2 + 1)
		}
		if (j >= n)
			return;
	}

	vgl_log("%s:%d glGenTextures: Texture slots limit reached (%d textures hadn't been generated).\n", __FILE__, __LINE__, n - j);
}

void glBindTexture(GLenum target, GLuint texture) {
	// Setting current in use texture id for the in use server texture unit
	texture_units[server_texture_unit].tex_id = texture;
}

void glDeleteTextures(GLsizei n, const GLuint *gl_textures) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Deallocating given textures and invalidating used texture ids
	for (int j = 0; j < n; j++) {
		GLuint i = gl_textures[j];
		if (i > 0) {
			if (texture_slots[i].status == TEX_VALID) {
				if (texture_slots[i].ref_counter > 0)
					if (texture_slots[i].ref_counter == 1) {
						framebuffer *fb = NULL;
						if (active_read_fb && active_read_fb->tex == &texture_slots[i])
							fb = active_read_fb;
						else if (active_write_fb && active_write_fb->tex == &texture_slots[i])
							fb = active_write_fb;
						if (fb) {
							gpu_free_texture(&texture_slots[i]);
							if (fb->depthbuffer_ptr && fb->is_depth_hidden) {
								markAsDirty(fb->depthbuffer_ptr->depthData);
								fb->depthbuffer_ptr = NULL;
								fb->is_depth_hidden = GL_FALSE;
							}
							if (fb->target) {
								markRtAsDirty(fb->target);
								fb->target = NULL;
							}
							fb->tex = NULL;
						} else
							texture_slots[i].dirty = GL_TRUE;
					} else {
						texture_slots[i].dirty = GL_TRUE;
					}
				else
					gpu_free_texture(&texture_slots[i]);
			}

			for (int k = 0; k < TEXTURE_IMAGE_UNITS_NUM; k++) {
				texture_unit *tex_unit = &texture_units[k];
				if (i == tex_unit->tex_id)
					tex_unit->tex_id = 0;
			}
		}
	}
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

#ifndef SKIP_ERROR_HANDLING
	// Checking if texture is too big for sceGxm
	if (width > GXM_TEX_MAX_SIZE || height > GXM_TEX_MAX_SIZE) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

#ifdef HAVE_UNPURE_TEXTURES
	if (tex->mip_start < 0)
		tex->mip_start = level;
	level -= tex->mip_start;
#endif

	// Support for legacy GL1.0 internalFormat
	switch (internalFormat) {
	case 1:
		internalFormat = GL_RED;
		break;
	case 2:
		internalFormat = GL_RG;
		break;
	case 3:
		internalFormat = GL_RGB;
		break;
	case 4:
		internalFormat = GL_RGBA;
		break;
	}

	switch (target) {
	case GL_TEXTURE_2D:
		_glTexImage2D_FlatIMPL(tex, level, internalFormat, width, height, format, type, data);
		break;
	case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
	case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
	case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
		_glTexImage2D_CubeIMPL(tex, level, internalFormat, width, height, format, type, data, target - GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *target_texture = &texture_slots[texture2d_idx];

#ifdef HAVE_UNPURE_TEXTURES
	level -= target_texture->mip_start;
#endif

	// Calculating implicit texture stride and start address of requested texture modification
	uint32_t orig_w = sceGxmTextureGetWidth(&target_texture->gxm_tex);
	uint32_t orig_h = sceGxmTextureGetHeight(&target_texture->gxm_tex);
	SceGxmTextureFormat tex_format = sceGxmTextureGetFormat(&target_texture->gxm_tex);
	uint8_t bpp = tex_format_to_bytespp(tex_format);
	uint32_t stride = ALIGN(orig_w, 8) * bpp;
	uint8_t *ptr = (uint8_t *)target_texture->data + xoffset * bpp + yoffset * stride;
	uint8_t *ptr_line = ptr;
	uint8_t data_bpp = 0;
	int i, j;
	GLboolean fast_store = GL_FALSE;

#ifndef SKIP_ERROR_HANDLING
	if (xoffset + width > orig_w) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	} else if (yoffset + height > orig_h) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Support for legacy GL1.0 format
	switch (format) {
	case 1:
		format = GL_RED;
		break;
	case 2:
		format = GL_RG;
		break;
	case 3:
		format = GL_RGB;
		break;
	case 4:
		format = GL_RGBA;
		break;
	}

	/*
	 * Callbacks are actually used to just perform down/up-sampling
	 * between U8 texture formats. Reads are expected to give as result
	 * a RGBA sample that will be wrote depending on texture format
	 * by the write callback
	 */
	void (*write_cb)(void *, uint32_t) = NULL;
	uint32_t (*read_cb)(void *) = NULL;

	// Detecting proper read callback and source bpp
	switch (format) {
	case GL_RED:
	case GL_ALPHA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			read_cb = readR;
			data_bpp = 1;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RG:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			read_cb = readRG;
			data_bpp = 2;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RGB:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			read_cb = readRGB;
			break;
		case GL_UNSIGNED_SHORT_5_6_5:
			data_bpp = 2;
			read_cb = readRGB565;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_BGR:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 3;
			read_cb = readBGR;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_RGBA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			read_cb = readRGBA;
			break;
		case GL_UNSIGNED_SHORT_5_5_5_1:
			data_bpp = 2;
			read_cb = readRGBA5551;
			break;
		case GL_UNSIGNED_SHORT_4_4_4_4:
			data_bpp = 2;
			read_cb = readRGBA4444;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	case GL_BGRA:
		switch (type) {
		case GL_UNSIGNED_BYTE:
			data_bpp = 4;
			read_cb = readBGRA;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, format)
		break;
	}

	switch (target) {
	case GL_TEXTURE_2D:

		// Detecting proper write callback
		switch (tex_format) {
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR:
			if (read_cb == readRGB)
				fast_store = GL_TRUE;
			else
				write_cb = writeRGB;
			break;
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8_RGB:
			if (read_cb == readBGR)
				fast_store = GL_TRUE;
			else
				write_cb = writeBGR;
			break;
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR:
			if (read_cb == readRGBA)
				fast_store = GL_TRUE;
			else
				write_cb = writeRGBA;
			break;
		case SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB:
			if (read_cb == readBGRA)
				fast_store = GL_TRUE;
			else
				write_cb = writeBGRA;
			break;
		case SCE_GXM_TEXTURE_FORMAT_L8:
		case SCE_GXM_TEXTURE_FORMAT_U8_RRRR:
		case SCE_GXM_TEXTURE_FORMAT_A8:
		case SCE_GXM_TEXTURE_FORMAT_P8_ABGR:
			if (read_cb == readR)
				fast_store = GL_TRUE;
			else
				write_cb = writeR;
			break;
		case SCE_GXM_TEXTURE_FORMAT_A8L8:
			if (read_cb == readRG)
				fast_store = GL_TRUE;
			else
				write_cb = writeRG;
			break;
		// From here, we assume we're always in fast_store trunk (Not 100% accurate)
		case SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB:
		case SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA:
		case SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA:
			fast_store = GL_TRUE;
			break;
		}

		if (fast_store) { // Internal format and input format are the same, we can take advantage of this
			uint8_t *data = (uint8_t *)pixels;
			uint32_t line_size = width * data_bpp;
			for (i = 0; i < height; i++) {
				vgl_fast_memcpy(ptr, data, line_size);
				data += line_size;
				ptr += stride;
			}
		} else { // Executing texture modification via callbacks
			uint8_t *data = (uint8_t *)pixels;
			for (i = 0; i < height; i++) {
				for (j = 0; j < width; j++) {
					uint32_t clr = read_cb((uint8_t *)data);
					write_cb(ptr, clr);
					data += data_bpp;
					ptr += bpp;
				}
				ptr = ptr_line + stride;
				ptr_line = ptr;
			}
		}

		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

#ifdef HAVE_UNPURE_TEXTURES
	if (tex->mip_start < 0)
		tex->mip_start = level;
	level -= tex->mip_start;
#endif

	SceGxmTextureFormat tex_format;
	GLboolean gamma_correction = GL_FALSE;
	GLboolean non_native_format = GL_FALSE;
	GLboolean paletted_format = GL_FALSE;
	void *decompressed_data;
	uint8_t data_bpp;
	uint32_t (*read_cb)(void *) = NULL;

#ifndef SKIP_ERROR_HANDLING
	// Checking if texture is too big for sceGxm
	if (width > GXM_TEX_MAX_SIZE || height > GXM_TEX_MAX_SIZE) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}

	// Checking if texture dimensions are not a power of two
	if (((width & (width - 1)) != 0) || ((height & (height - 1)) != 0)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}

	// Ensure imageSize isn't zero.
	if (imageSize == 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	switch (target) {
	case GL_TEXTURE_2D:
		// Detecting proper write callback and texture format
		switch (internalFormat) {
		case GL_PALETTE4_RGB8_OES:
			read_cb = readRGB;
			data_bpp = 3;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P4_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE4_RGBA8_OES:
			read_cb = readRGBA;
			data_bpp = 4;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P4_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE4_RGBA4_OES:
			read_cb = readRGBA4444;
			data_bpp = 2;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P4_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE4_R5_G6_B5_OES:
			read_cb = readRGB565;
			data_bpp = 2;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P4_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE4_RGB5_A1_OES:
			read_cb = readRGBA5551;
			data_bpp = 2;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P4_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE8_RGB8_OES:
			read_cb = readRGB;
			data_bpp = 3;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE8_RGBA8_OES:
			read_cb = readRGBA;
			data_bpp = 4;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE8_RGBA4_OES:
			read_cb = readRGBA4444;
			data_bpp = 2;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE8_R5_G6_B5_OES:
			read_cb = readRGB565;
			data_bpp = 2;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_PALETTE8_RGB5_A1_OES:
			read_cb = readRGBA5551;
			data_bpp = 2;
			tex_format = SCE_GXM_TEXTURE_FORMAT_P8_ABGR;
			paletted_format = GL_TRUE;
			break;
		case GL_COMPRESSED_SRGB_S3TC_DXT1:
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1:
			gamma_correction = GL_TRUE;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			tex_format = SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			tex_format = SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR;
			break;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5:
			gamma_correction = GL_TRUE;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			tex_format = SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR;
			break;
		case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
			tex_format = SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_1BGR;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
			tex_format = SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_ABGR;
			break;
		case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
			tex_format = SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_1BGR;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
			tex_format = SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_ABGR;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:
			tex_format = SCE_GXM_TEXTURE_FORMAT_PVRTII2BPP_ABGR;
			break;
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:
			tex_format = SCE_GXM_TEXTURE_FORMAT_PVRTII4BPP_ABGR;
			break;
		case GL_ETC1_RGB8_OES:
			non_native_format = GL_TRUE;
			decompressed_data = vglMalloc(width * height * 3);
			etc1_decode_image((etc1_byte *)data, (etc1_byte *)decompressed_data, width, height, 3, width * 3);
			if (recompress_non_native) {
				read_cb = readRGB;
				tex_format = SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR;
			} else
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR;
			data_bpp = 3;
			break;
		case GL_ATC_RGB_AMD:
			non_native_format = GL_TRUE;
			decompressed_data = vglMalloc(width * height * 4);
			atitc_decode((uint8_t *)data, decompressed_data, width, height, ATC_RGB);
			if (recompress_non_native) {
				read_cb = readBGRA;
				tex_format = SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR;
			} else
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
			data_bpp = 4;
			break;
		case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD:
			non_native_format = GL_TRUE;
			decompressed_data = vglMalloc(width * height * 4);
			atitc_decode((uint8_t *)data, decompressed_data, width, height, ATC_EXPLICIT_ALPHA);
			if (recompress_non_native) {
				read_cb = readBGRA;
				tex_format = SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR;
			} else
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
			data_bpp = 4;
			break;
		case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD:
			non_native_format = GL_TRUE;
			decompressed_data = vglMalloc(width * height * 4);
			atitc_decode((uint8_t *)data, decompressed_data, width, height, ATC_INTERPOLATED_ALPHA);
			if (recompress_non_native) {
				read_cb = readBGRA;
				tex_format = SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR;
			} else
				tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
			data_bpp = 4;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, internalFormat)
		}

		// Allocating texture/mipmaps depending on user call
		tex->type = internalFormat;
		if (paletted_format) {
#ifndef SKIP_ERROR_HANDLING
			if (level > 0) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			gpu_alloc_paletted_texture(-level, width, height, tex_format, data, tex, data_bpp, read_cb);
			vglSetTexPalette(&tex->gxm_tex, tex->palette_data);
		} else {
#ifndef SKIP_ERROR_HANDLING
			if (level < 0) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			if (non_native_format) {
				if (level == 0)
					if (read_cb)
						gpu_alloc_compressed_texture(level, width, height, tex_format, 0, decompressed_data, tex, data_bpp, read_cb);
					else
						gpu_alloc_texture(width, height, tex_format, decompressed_data, tex, data_bpp, NULL, NULL, GL_TRUE);
				else if (read_cb)
					gpu_alloc_compressed_texture(level, width, height, tex_format, 0, decompressed_data, tex, data_bpp, read_cb);
				else
					gpu_alloc_mipmaps(level, tex);
				vgl_free(decompressed_data);
			} else
				gpu_alloc_compressed_texture(level, width, height, tex_format, imageSize, data, tex, 0, NULL);
		}
		// Setting texture parameters
		vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
		vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
		vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
		vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
		vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
		vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
		vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
		if (gamma_correction)
			vglSetTexGammaMode(&tex->gxm_tex, SCE_GXM_TEXTURE_GAMMA_BGR);

		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *data) {
	// Checking if a color table is already enabled, if so, deallocating it
	if (color_table != NULL) {
		gpu_free_palette(color_table);
		color_table = NULL;
	}

	// Calculating color table bpp
	uint8_t bpp = 0;
	switch (target) {
	case GL_COLOR_TABLE:
		switch (format) {
		case GL_RGBA:
			bpp = 4;
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, format)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}

	// Allocating and initializing color table
	color_table = gpu_alloc_palette(data, width, bpp);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_CUBE_MAP:
		switch (pname) {
		case GL_TEXTURE_WRAP_S: 
		case GL_TEXTURE_WRAP_T:
#ifndef SKIP_ERROR_HANDLING
			if (param != GL_CLAMP_TO_EDGE && param != GL_CLAMP) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			break;
		case GL_TEXTURE_MIN_FILTER: // Min filter
			switch (param) {
			case GL_NEAREST: // Point
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR: // Linear
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			case GL_NEAREST_MIPMAP_NEAREST:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR_MIPMAP_NEAREST:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			case GL_NEAREST_MIPMAP_LINEAR:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR_MIPMAP_LINEAR:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, param)
			}
			if (tex->status == TEX_VALID) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			}
			break;
		case GL_TEXTURE_MAG_FILTER: // Mag Filter
			switch (param) {
			case GL_NEAREST:
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR:
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, param)
			}
			if (tex->status == TEX_VALID)
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
	case GL_TEXTURE_2D:
		switch (pname) {
		case GL_TEXTURE_MIN_FILTER: // Min filter
			switch (param) {
			case GL_NEAREST: // Point
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR: // Linear
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			case GL_NEAREST_MIPMAP_NEAREST:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR_MIPMAP_NEAREST:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			case GL_NEAREST_MIPMAP_LINEAR:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR_MIPMAP_LINEAR:
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, param)
			}
			if (tex->status == TEX_VALID) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			}
			break;
		case GL_TEXTURE_MAG_FILTER: // Mag Filter
			switch (param) {
			case GL_NEAREST:
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_POINT;
				break;
			case GL_LINEAR:
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, param)
			}
			if (tex->status == TEX_VALID)
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
			break;
		case GL_TEXTURE_WRAP_S: // U Mode
			switch (param) {
			case GL_CLAMP_TO_EDGE: // Clamp
			case GL_CLAMP:
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
				break;
			case GL_REPEAT: // Repeat
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
				break;
			case GL_MIRRORED_REPEAT: // Mirror
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
				break;
			case GL_MIRROR_CLAMP_EXT: // Mirror Clamp
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR_CLAMP;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, param)
			}
			if (tex->status == TEX_VALID)
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
			break;
		case GL_TEXTURE_WRAP_T: // V Mode
			switch (param) {
			case GL_CLAMP_TO_EDGE: // Clamp
			case GL_CLAMP:
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_CLAMP;
				break;
			case GL_REPEAT: // Repeat
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT;
				break;
			case GL_MIRRORED_REPEAT: // Mirror
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR;
				break;
			case GL_MIRROR_CLAMP_EXT: // Mirror Clamp
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR_CLAMP;
				break;
			default:
				SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, param)
			}
			if (tex->status == TEX_VALID)
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
			break;
		case GL_TEXTURE_LOD_BIAS: // Distant LOD bias
			tex->lod_bias = (uint32_t)(param + GL_MAX_TEXTURE_LOD_BIAS);
			if (tex->status == TEX_VALID)
				vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_CUBE_MAP:
		switch (pname) {
		case GL_TEXTURE_MIN_FILTER: // Min Filter
			if (param == GL_NEAREST) {
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			} else if (param == GL_LINEAR) {
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			} else if (param == GL_NEAREST_MIPMAP_NEAREST) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			} else if (param == GL_LINEAR_MIPMAP_NEAREST) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			} else if (param == GL_NEAREST_MIPMAP_LINEAR) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			} else if (param == GL_LINEAR_MIPMAP_LINEAR) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			}
			if (tex->status == TEX_VALID) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			}
			break;
		case GL_TEXTURE_MAG_FILTER: // Mag filter
			if (param == GL_NEAREST)
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			else if (param == GL_LINEAR)
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			if (tex->status == TEX_VALID)
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
	case GL_TEXTURE_2D:
		switch (pname) {
		case GL_TEXTURE_MAX_ANISOTROPY_EXT: // Anisotropic Filter
#ifndef SKIP_ERROR_HANDLING			
			if (param != 1.0f) {
				SET_GL_ERROR(GL_INVALID_VALUE)
			}
#endif
			break;
		case GL_TEXTURE_MIN_FILTER: // Min Filter
			if (param == GL_NEAREST) {
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			} else if (param == GL_LINEAR) {
				tex->use_mips = GL_FALSE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			} else if (param == GL_NEAREST_MIPMAP_NEAREST) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			} else if (param == GL_LINEAR_MIPMAP_NEAREST) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_DISABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			} else if (param == GL_NEAREST_MIPMAP_LINEAR) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			} else if (param == GL_LINEAR_MIPMAP_LINEAR) {
				tex->use_mips = GL_TRUE;
				tex->mip_filter = SCE_GXM_TEXTURE_MIP_FILTER_ENABLED;
				tex->min_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			}
			if (tex->status == TEX_VALID) {
				vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
				vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
				vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);
			}
			break;
		case GL_TEXTURE_MAG_FILTER: // Mag filter
			if (param == GL_NEAREST)
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_POINT;
			else if (param == GL_LINEAR)
				tex->mag_filter = SCE_GXM_TEXTURE_FILTER_LINEAR;
			if (tex->status == TEX_VALID)
				vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
			break;
		case GL_TEXTURE_WRAP_S: // U Mode
			if (param == GL_CLAMP_TO_EDGE || param == GL_CLAMP)
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_CLAMP; // Clamp
			else if (param == GL_REPEAT)
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_REPEAT; // Repeat
			else if (param == GL_MIRRORED_REPEAT)
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR; // Mirror
			else if (param == GL_MIRROR_CLAMP_EXT)
				tex->u_mode = SCE_GXM_TEXTURE_ADDR_MIRROR_CLAMP; // Mirror Clamp
			if (tex->status == TEX_VALID)
				vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
			break;
		case GL_TEXTURE_WRAP_T: // V Mode
			if (param == GL_CLAMP_TO_EDGE || param == GL_CLAMP)
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_CLAMP; // Clamp
			else if (param == GL_REPEAT)
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_REPEAT; // Repeat
			else if (param == GL_MIRRORED_REPEAT)
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR; // Mirror
			else if (param == GL_MIRROR_CLAMP_EXT)
				tex->v_mode = SCE_GXM_TEXTURE_ADDR_MIRROR_CLAMP; // Mirror Clamp
			if (tex->status == TEX_VALID)
				vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
			break;
		case GL_TEXTURE_LOD_BIAS: // Distant LOD bias
			tex->lod_bias = (uint32_t)(param + GL_MAX_TEXTURE_LOD_BIAS);
			if (tex->status == TEX_VALID)
				vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
			break;
		default:
			SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glActiveTexture(GLenum texture) {
	// Changing current in use server texture unit
#ifndef SKIP_ERROR_HANDLING
	if ((texture < GL_TEXTURE0) && (texture > GL_TEXTURE15)) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, texture)
	} else
#endif
		server_texture_unit = texture - GL_TEXTURE0;
}

void glGenerateMipmap(GLenum target) {
	// Setting some aliases to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

#ifndef SKIP_ERROR_HANDLING
	// Checking if current texture is valid
	if (tex->status != TEX_VALID)
		return;
	// Checking if current texture is compressed
	else {
		SceGxmTextureFormat fmt = sceGxmTextureGetFormat(&tex->gxm_tex);
		if (fmt >= 0x80000000 && fmt <= 0x87000000) {
			SET_GL_ERROR(GL_INVALID_OPERATION)
		}
	}
#endif

	switch (target) {
	case GL_TEXTURE_2D:
		// Generating mipmaps to the max possible level
		gpu_alloc_mipmaps(-1, tex);

		// Setting texture parameters
		vglSetTexUMode(&tex->gxm_tex, tex->u_mode);
		vglSetTexVMode(&tex->gxm_tex, tex->v_mode);
		vglSetTexMinFilter(&tex->gxm_tex, tex->min_filter);
		vglSetTexMagFilter(&tex->gxm_tex, tex->mag_filter);
		vglSetTexMipFilter(&tex->gxm_tex, tex->mip_filter);
		vglSetTexLodBias(&tex->gxm_tex, tex->lod_bias);
		vglSetTexMipmapCount(&tex->gxm_tex, tex->use_mips ? tex->mip_count : 0);

		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void gluBuild2DMipmaps(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data) {
#ifndef SKIP_ERROR_HANDLING
	if (target != GL_TEXTURE_2D) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
#endif

	glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	glGenerateMipmap(target);
}

void *vglGetTexDataPointer(GLenum target) {
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_2D:
		return tex->data;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL)
	}
}

void vglOverloadTexDataPointer(GLenum target, void *data) {
		// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_2D:
		tex->data = data;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

SceGxmTexture *vglGetGxmTexture(GLenum target) {
	// Aliasing texture unit for cleaner code
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	int texture2d_idx = tex_unit->tex_id;
	texture *tex = &texture_slots[texture2d_idx];

	switch (target) {
	case GL_TEXTURE_2D:
		return &tex->gxm_tex;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL)
	}
}
