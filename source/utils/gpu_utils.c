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
 * gpu_utils.c:
 * Utilities for GPU usage
 */

#include "../shared.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#define CEIL(a) ceil(a)
#endif

// VRAM usage setting
uint8_t use_vram = GL_TRUE;
uint8_t use_vram_for_usse = GL_FALSE;

// Newlib mempool usage setting
GLboolean use_extra_mem = GL_TRUE;

// Taken from here: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
uint32_t nearest_po2(uint32_t val) {
	val--;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	val++;

	return val;
}

uint64_t morton_1(uint64_t x) {
	x = x & 0x5555555555555555;
	x = (x | (x >> 1)) & 0x3333333333333333;
	x = (x | (x >> 2)) & 0x0F0F0F0F0F0F0F0F;
	x = (x | (x >> 4)) & 0x00FF00FF00FF00FF;
	x = (x | (x >> 8)) & 0x0000FFFF0000FFFF;
	x = (x | (x >> 16)) & 0xFFFFFFFFFFFFFFFF;
	return x;
}

void d2xy_morton(uint64_t d, uint64_t *x, uint64_t *y) {
	*x = morton_1(d);
	*y = morton_1(d >> 1);
}

void extract_block(const uint8_t *src, int width, uint8_t *block) {
	int j;
	for (j = 0; j < 4; j++) {
		sceClibMemcpy(&block[j * 4 * 4], src, 16);
		src += width * 4;
	}
}

void dxt_compress(uint8_t *dst, uint8_t *src, int w, int h, int isdxt5) {
	uint8_t block[64];
	int s = MAX(w, h);
	uint32_t num_blocks = (s * s) / 16;
	uint64_t d, offs_x, offs_y;
	for (d = 0; d < num_blocks; d++) {
		d2xy_morton(d, &offs_x, &offs_y);
		if (offs_x * 4 >= h)
			continue;
		if (offs_y * 4 >= w)
			continue;
		extract_block(src + offs_y * 16 + offs_x * w * 16, w, block);
		stb_compress_dxt_block(dst, block, isdxt5, fast_texture_compression ? STB_DXT_NORMAL : STB_DXT_HIGHQUAL);
		dst += isdxt5 ? 16 : 8;
	}
}

void swizzle_compressed_texture_region(void *dst, const void *src, int tex_width, int tex_height, int region_x, int region_y, int region_width, int region_height, int isdxt5, int ispvrt2bpp) {
	const int blocksize = isdxt5 ? 16 : 8;
	const uint32_t blockw = (ispvrt2bpp ? 8 : 4);

	// round sizes up to block size
	tex_width = ALIGN(tex_width, blockw);
	tex_height = ALIGN(tex_height, 4);
	region_width = ALIGN(region_width, blockw);
	region_height = ALIGN(region_height, 4);

	const int s = MAX(tex_width, tex_height);
	const uint32_t num_blocks = (s * s) / (blockw * 4);

	uint64_t d, offs_x, offs_y;
	uint64_t dst_x, dst_y;
	for (d = 0; d < num_blocks; d++) {
		d2xy_morton(d, &offs_x, &offs_y);
		// If the block coords exceed input texture dimensions.
		if ((offs_x * 4 >= region_height + region_y) || (offs_x * 4 < region_y)) {
			// If the block coord is smaller than the Po2 aligned dimension, skip forward one block.
			if (offs_x * 4 < tex_height)
				dst += blocksize;
			continue;
		}

		if ((offs_y * blockw >= region_width + region_x) || (offs_y * blockw < region_x)) {
			if (offs_y * blockw < tex_width)
				dst += blocksize;
			continue;
		}

		dst_x = offs_x - (region_y / 4);
		dst_y = offs_y - (region_x / blockw);

		sceClibMemcpy(dst, src + dst_y * blocksize + dst_x * (region_width / blockw) * blocksize, blocksize);
		dst += blocksize;
	}
}

void *gpu_alloc_mapped_aligned(size_t alignment, size_t size, vglMemType type) {
	// Allocating requested memblock
	void *res = vgl_memalign(alignment, size, type);
	if (res)
		return res;

	// Requested memory type finished, using other one
	res = vgl_memalign(alignment, size, type == VGL_MEM_VRAM ? VGL_MEM_RAM : VGL_MEM_VRAM);
	if (res)
		return res;

	// Even the other one failed, using our last resort
	res = vgl_memalign(alignment, size, VGL_MEM_SLOW);
	if (res)
		return res;

	// Internal mempool finished, using newlib mem
	if (use_extra_mem)
		res = vgl_memalign(alignment, size, VGL_MEM_EXTERNAL);

#ifdef LOG_ERRORS
	if (!res)
		vgl_log("gpu_alloc_mapped_aligned failed with a requested size of 0x%08X\n", size);
#endif

	return res;
}

void *gpu_alloc_mapped(size_t size, vglMemType type) {
	return gpu_alloc_mapped_aligned(MEM_ALIGNMENT, size, type);
}

void *gpu_vertex_usse_alloc_mapped(size_t size, unsigned int *usse_offset) {
	// Allocating memblock
	void *addr = gpu_alloc_mapped_aligned(4096, size, use_vram_for_usse ? VGL_MEM_VRAM : VGL_MEM_RAM);

	// Mapping memblock into sceGxm as vertex USSE memory
	sceGxmMapVertexUsseMemory(addr, size, usse_offset);

	// Returning memblock starting address
	return addr;
}

void gpu_vertex_usse_free_mapped(void *addr) {
	// Unmapping memblock from sceGxm as vertex USSE memory
	sceGxmUnmapVertexUsseMemory(addr);

	// Deallocating memblock
	vgl_free(addr);
}

void *gpu_fragment_usse_alloc_mapped(size_t size, unsigned int *usse_offset) {
	// Allocating memblock
	void *addr = gpu_alloc_mapped_aligned(4096, size, use_vram_for_usse ? VGL_MEM_VRAM : VGL_MEM_RAM);

	// Mapping memblock into sceGxm as fragment USSE memory
	sceGxmMapFragmentUsseMemory(addr, size, usse_offset);

	// Returning memblock starting address
	return addr;
}

void gpu_fragment_usse_free_mapped(void *addr) {
	// Unmapping memblock from sceGxm as fragment USSE memory
	sceGxmUnmapFragmentUsseMemory(addr);

	// Deallocating memblock
	vgl_free(addr);
}

void *gpu_alloc_mapped_temp(size_t size) {
#ifndef HAVE_CIRCULAR_VERTEX_POOL
	// Allocating memblock and marking it for garbage collection
	void *res = gpu_alloc_mapped(size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);

#ifdef LOG_ERRORS
	if (!res)
		vgl_log("gpu_alloc_mapped_temp failed with a requested size of 0x%08X\n", size);
#endif

	markAsDirty(res);
	return res;
#else
	return reserve_data_pool(size);
#endif
}

int tex_format_to_bytespp(SceGxmTextureFormat format) {
	// Calculating bpp for the requested texture format
	switch (format & 0x9F000000) {
	case SCE_GXM_TEXTURE_BASE_FORMAT_P4:
		return 0;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_P8:
		return 1;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U4U4U4U4:
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U3U3U2:
	case SCE_GXM_TEXTURE_BASE_FORMAT_U1U5U5U5:
	case SCE_GXM_TEXTURE_BASE_FORMAT_U5U6U5:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S5S5U6:
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8:
		return 2;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8S8:
		return 3;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8U8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8S8S8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_F32:
	case SCE_GXM_TEXTURE_BASE_FORMAT_U32:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S32:
	default:
		return 4;
	}
}

SceGxmTransferFormat tex_format_to_transfer(SceGxmTextureFormat format) {
	// Calculating transfer format for the requested texture format
	switch (format & 0x9F000000) {
	case SCE_GXM_TEXTURE_BASE_FORMAT_U1U5U5U5:
		return SCE_GXM_TRANSFER_FORMAT_U1U5U5U5_ABGR;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U5U6U5:
		return SCE_GXM_TRANSFER_FORMAT_U5U6U5_BGR;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U4U4U4U4:
		return SCE_GXM_TRANSFER_FORMAT_U4U4U4U4_ABGR;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8:
		return SCE_GXM_TRANSFER_FORMAT_U8U8U8_BGR;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8U8:
	default:
		return SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR;
	}
}

int tex_format_to_alignment(SceGxmTextureFormat format) {
	switch (format & 0x9F000000) {
	case SCE_GXM_TEXTURE_BASE_FORMAT_UBC2:
	case SCE_GXM_TEXTURE_BASE_FORMAT_UBC3:
		return 16;
	default:
		return 8;
	}
}

palette *gpu_alloc_palette(const void *data, uint32_t w, uint32_t bpe) {
	// Allocating a palette object
	palette *res = (palette *)vgl_malloc(sizeof(palette), VGL_MEM_EXTERNAL);

	// Allocating palette data buffer
	void *texture_palette = gpu_alloc_mapped(256 * sizeof(uint32_t), use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);

	// Initializing palette
	if (data == NULL)
		sceClibMemset(texture_palette, 0, 256 * sizeof(uint32_t));
	else if (bpe == 4)
		sceClibMemcpy(texture_palette, data, w * sizeof(uint32_t));
	res->data = texture_palette;

	// Returning palette
	return res;
}

void gpu_free_texture_data(texture *tex) {
	// Deallocating texture
	if (tex->data != NULL) {
		markAsDirty(tex->data);
		tex->data = NULL;
	}
	if (tex->palette_data != NULL) {
		markAsDirty(tex->palette_data);
		tex->palette_data = NULL;
	}
}

void gpu_free_texture(texture *tex) {
	gpu_free_texture_data(tex);
	tex->status = TEX_UNUSED;
}

void gpu_alloc_cube_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, SceGxmTransferFormat src_format, const void *data, texture *tex, uint8_t src_bpp, int index) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID && tex->faces_counter >= 6) {
		gpu_free_texture_data(tex);
		tex->faces_counter = 1;
	} else
		tex->faces_counter++;

	// Getting texture format bpp
	uint8_t bpp = tex_format_to_bytespp(format);

	// Allocating texture data buffer
	const int face_size = ALIGN(w, 8) * h * bpp;
	void *base_texture_data = tex->faces_counter == 1 ? gpu_alloc_mapped(face_size * 6, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM) : tex->data;

	if (base_texture_data != NULL) {
		// Calculating face texture data pointer
		uint8_t *texture_data = (uint8_t *)base_texture_data + face_size * index;

		if (data != NULL) {
			const int tex_size = w * h * bpp;
			void *mapped_data = gpu_alloc_mapped_temp(tex_size);
			sceClibMemcpy(mapped_data, data, tex_size);
			SceGxmTransferFormat dst_fmt = tex_format_to_transfer(format);
			sceGxmTransferCopy(
				w, h, 0, 0, SCE_GXM_TRANSFER_COLORKEY_NONE,
				src_format, SCE_GXM_TRANSFER_LINEAR,
				mapped_data, 0, 0, w * src_bpp,
				dst_fmt, SCE_GXM_TRANSFER_SWIZZLED,
				texture_data, 0, 0, ALIGN(w, 8) * bpp,
				NULL, 0, NULL);
		} else
			sceClibMemset(texture_data, 0xFF, face_size);

		// Initializing texture and validating it
		tex->mip_count = 0;
		vglInitCubeTexture(&tex->gxm_tex, base_texture_data, format, w, h, tex->mip_count);
		tex->palette_UID = 0;
		tex->status = TEX_VALID;
		tex->data = base_texture_data;
	}
}

void gpu_alloc_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *), void (*write_cb)(void *, uint32_t), GLboolean fast_store) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID)
		gpu_free_texture_data(tex);

	// Getting texture format bpp
	uint8_t bpp = tex_format_to_bytespp(format);

	// Allocating texture data buffer
	int aligned_w = ALIGN(w, 8);
	const int tex_size = aligned_w * h * bpp;
	void *texture_data = gpu_alloc_mapped(tex_size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);

	if (texture_data != NULL) {
		// Initializing texture data buffer
		if (data != NULL) {
			int i, j;
			uint8_t *src = (uint8_t *)data;
			uint8_t *dst;
			if (fast_store) { // Internal Format and Data Format are the same, we can just use sceClibMemcpy for better performance
				if (aligned_w == w) // Texture size is already aligned, we can use a single sceClibMemcpy for better performance
					sceClibMemcpy(texture_data, src, tex_size);
				else {
					uint32_t line_size = w * bpp;
					for (i = 0; i < h; i++) {
						dst = ((uint8_t *)texture_data) + (ALIGN(w, 8) * bpp) * i;
						sceClibMemcpy(dst, src, line_size);
						src += line_size;
					}
				}
			} else { // Different internal and data formats, we need to go with slower callbacks system
				for (i = 0; i < h; i++) {
					dst = ((uint8_t *)texture_data) + (ALIGN(w, 8) * bpp) * i;
					for (j = 0; j < w; j++) {
						uint32_t clr = read_cb(src);
						write_cb(dst, clr);
						src += src_bpp;
						dst += bpp;
					}
				}
			}
		} else
			sceClibMemset(texture_data, 0xFF, tex_size);

		// Initializing texture and validating it
		tex->mip_count = 1;
		vglInitLinearTexture(&tex->gxm_tex, texture_data, format, w, h, tex->mip_count);
		if ((format & 0x9f000000U) == SCE_GXM_TEXTURE_BASE_FORMAT_P8)
			tex->palette_UID = 1;
		else
			tex->palette_UID = 0;
		tex->status = TEX_VALID;
		tex->data = texture_data;
	}
}

void gpu_alloc_paletted_texture(int32_t level, uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *)) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID)
		gpu_free_texture_data(tex);
	
	// Check if the texture is P8
	uint8_t is_p8 = tex_format_to_bytespp(format);
	
	// Calculating texture data buffer size
	uint32_t tex_size;
	for (int j = 0; j < level; j++) {
		tex_size += is_p8 ? (w * h) : (w * h / 2);
		w /= 2;
		h /= 2;
	}
	
	// Allocating texture and palette data buffers
	tex->palette_data = gpu_alloc_mapped(256 * sizeof(uint32_t), use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);
	tex->data = gpu_alloc_mapped(tex_size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);
	
	// Populating palette data
	uint32_t *palette_data = (uint32_t *)tex->palette_data;
	uint8_t *src = (uint8_t *)data;
	for (int i = 0; i < 256; i++) {
		palette_data[i] = read_cb(src);
		src += src_bpp;
	}
	
	// Populating texture data
	sceClibMemcpy(tex->data, src, tex_size);
	
	// Initializing texture and validating it
	tex->mip_count = level + 1;
	vglInitLinearTexture(&tex->gxm_tex, tex->data, format, w, h, tex->mip_count);
	tex->status = TEX_VALID;
}

static inline int gpu_get_compressed_mip_size(int level, int width, int height, SceGxmTextureFormat format) {
	switch (format) {
	case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_1BGR:
	case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_ABGR:
		return (MAX(width, 16) * MAX(height, 8) * 2 + 7) / 8;
	case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_1BGR:
	case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_ABGR:
		return (MAX(width, 8) * MAX(height, 8) * 4 + 7) / 8;
	case SCE_GXM_TEXTURE_FORMAT_PVRTII2BPP_ABGR:
		return CEIL(width / 8.0) * CEIL(height / 4.0) * 8.0;
	case SCE_GXM_TEXTURE_FORMAT_PVRTII4BPP_ABGR:
		return CEIL(width / 4.0) * CEIL(height / 4.0) * 8.0;
	case SCE_GXM_TEXTURE_FORMAT_UBC1_1BGR:
	case SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR:
		return CEIL(width / 4.0) * CEIL(height / 4.0) * 8;
	case SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR:
	case SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR:
		return CEIL(width / 4.0) * CEIL(height / 4.0) * 16;
	default:
		return 0;
	}
}

int gpu_get_compressed_mipchain_size(int level, int width, int height, SceGxmTextureFormat format) {
	int size = 0;

	for (int currentLevel = 0; currentLevel <= level; currentLevel++) {
		size += gpu_get_compressed_mip_size(currentLevel, width, height, format);
		if (width > 1)
			width /= 2;
		if (height > 1)
			height /= 2;
	}

	return size;
}

int gpu_get_compressed_mip_offset(int level, int width, int height, SceGxmTextureFormat format) {
	return gpu_get_compressed_mipchain_size(level - 1, width, height, format);
}

void gpu_alloc_compressed_texture(int32_t mip_level, uint32_t w, uint32_t h, SceGxmTextureFormat format, uint32_t image_size, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *)) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID && !mip_level)
		gpu_free_texture_data(tex);

	// Calculating swizzled compressed texture size on memory
	vglMemType new_mtype = use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM;

	if (!image_size)
		image_size = gpu_get_compressed_mip_size(mip_level, w, h, format);

	const uint32_t blocksize = (format == SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR) ? 16 : 8;
	const uint32_t aligned_width = nearest_po2(w);
	const uint32_t aligned_height = nearest_po2(h);
	uint32_t max_width, max_height, aligned_max_width, aligned_max_height;
	if (!mip_level) {
		max_width = w;
		max_height = h;
		aligned_max_width = aligned_width;
		aligned_max_height = aligned_height;
	} else {
		max_width = vglGetTexWidth(&tex->gxm_tex);
		max_height = vglGetTexHeight(&tex->gxm_tex);
		aligned_max_width = nearest_po2(max_width);
		aligned_max_height = nearest_po2(max_height);
	}

	// Allocating texture data buffer
	const int mip_offset = gpu_get_compressed_mip_offset(mip_level, aligned_max_width, aligned_max_height, format);
	const int tex_size = gpu_get_compressed_mipchain_size(mip_level, aligned_max_width, aligned_max_height, format);
	const int mip_size = tex_size - mip_offset;

	int mip_count, tex_width, tex_height;
	void *texture_data;
	if (mip_level) {
		mip_count = tex->mip_count - 1;
		tex_width = max_width;
		tex_height = max_height;

		if (mip_count >= mip_level)
			texture_data = tex->data;
		else {
			texture_data = vgl_realloc(tex->data, tex_size);
			if (!texture_data) {
				// Reallocation in the same mspace failed, try manually.
				texture_data = gpu_alloc_mapped(tex_size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);
				const int old_data_size = gpu_get_compressed_mipchain_size(mip_count, aligned_max_width, aligned_max_height, format);
				vgl_memcpy(texture_data, tex->data, old_data_size);
				gpu_free_texture_data(tex);
			}

			// Set new mip count.
			mip_count = mip_level;
		}
	} else {
		mip_count = mip_level;
		tex_width = w;
		tex_height = h;
		texture_data = gpu_alloc_mapped(tex_size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);
	}

	void *mip_data = texture_data + mip_offset;

	// Initializing texture data buffer
	if (texture_data != NULL) {
		if (data != NULL) {
			if (read_cb != NULL) {
				void *temp = (void *)data;

				// stb_dxt expects input as RGBA8888, so we convert input texture if necessary
				if (read_cb != readRGBA) {
					temp = vgl_malloc(w * h * 4, VGL_MEM_EXTERNAL);
					uint8_t *src = (uint8_t *)data;
					uint32_t *dst = (uint32_t *)temp;
					int i;
					for (i = 0; i < w * h; i++) {
						uint32_t clr = read_cb(src);
						writeRGBA(dst++, clr);
						src += src_bpp;
					}
				}

				// Performing swizzling and DXT compression
				uint8_t alignment = tex_format_to_alignment(format);
				dxt_compress(mip_data, temp, aligned_width, aligned_height, alignment == 16);

				// Freeing temporary data if necessary
				if (read_cb != readRGBA)
					vgl_free(temp);
			} else {
				// Perform swizzling if necessary.
				switch (format) {
				case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_1BGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_ABGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_1BGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_ABGR:
					sceClibMemcpy(mip_data, data, image_size);
					break;
				case SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR:
				case SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR:
					swizzle_compressed_texture_region(mip_data, (void *)data, aligned_width, aligned_height, 0, 0, w, h, 1, 0);
					break;
				case SCE_GXM_TEXTURE_FORMAT_PVRTII2BPP_ABGR:
					swizzle_compressed_texture_region(mip_data, (void *)data, aligned_width, aligned_height, 0, 0, w, h, 0, 1);
					break;
				default:
					swizzle_compressed_texture_region(mip_data, (void *)data, aligned_width, aligned_height, 0, 0, w, h, 0, 0);
					break;
				}
			}

		} else
			sceClibMemset(mip_data, 0, mip_size);

		// Initializing texture and validating it
		tex->mip_count = mip_count + 1;
		vglInitSwizzledTexture(&tex->gxm_tex, texture_data, format, tex_width, tex_height, tex->use_mips ? tex->mip_count : 0);
		tex->palette_UID = 0;
		tex->status = TEX_VALID;
		tex->data = texture_data;
	}
}

void gpu_alloc_mipmaps(int level, texture *tex) {
	// Getting current mipmap count in passed texture
	uint32_t count = tex->mip_count - 1;

	// Checking if we need at least one more new mipmap level
	if ((level > count) || (level < 0)) { // Note: level < 0 means we will use max possible mipmaps level

		// Getting textures info and calculating bpp
		SceGxmTextureFormat format = sceGxmTextureGetFormat(&tex->gxm_tex);
		uint32_t bpp = tex_format_to_bytespp(format);
		uint32_t orig_w = vglGetTexWidth(&tex->gxm_tex);
		uint32_t orig_h = vglGetTexHeight(&tex->gxm_tex);
		uint32_t w = nearest_po2(orig_w);
		uint32_t h = nearest_po2(orig_h);

		// Calculating new texture data buffer size
		uint32_t jumps[10];
		uint32_t size = 0;
		int j;
		if (level > 0) {
			for (j = 0; j < level; j++) {
				jumps[j] = max(w, 8) * h * bpp;
				size += jumps[j];
				w /= 2;
				h /= 2;
			}
		} else {
			level = 0;
			while ((w > 1) && (h > 1)) {
				jumps[level] = max(w, 8) * h * bpp;
				size += jumps[level];
				w /= 2;
				h /= 2;
				level++;
			}
		}

		// Calculating needed sceGxmTransfer format for the downscale process
		SceGxmTransferFormat fmt = tex_format_to_transfer(format);

		// Reallocating texture
		void *texture_data = vgl_realloc(tex->data, size);
		if (!texture_data) {
			// Reallocation in the same mspace failed, try manually.
			texture_data = gpu_alloc_mapped(size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);
			vgl_memcpy(texture_data, tex->data, ALIGN(orig_w, 8) * orig_h * bpp);
			gpu_free_texture_data(tex);
		}

		// Performing a chain downscale process to generate requested mipmaps
		uint8_t *curPtr = (uint8_t *)texture_data;
		uint32_t curWidth = orig_w;
		uint32_t curHeight = orig_h;
		if (curWidth % 2)
			curWidth--;
		if (curHeight % 2)
			curHeight--;
		for (j = 0; j < level - 1; j++) {
			uint32_t curSrcStride = ALIGN(curWidth, 8);
			uint32_t curDstStride = ALIGN(curWidth >> 1, 8);
			uint8_t *dstPtr = curPtr + jumps[j];
			sceGxmTransferDownscale(
				fmt, curPtr, 0, 0,
				curWidth, curHeight,
				curSrcStride * bpp,
				fmt, dstPtr, 0, 0,
				curDstStride * bpp,
				NULL, 0, NULL);
			curPtr = dstPtr;
			curWidth /= 2;
			curHeight /= 2;
		}

		// Initializing texture in sceGxm
		tex->mip_count = level + 1;
		vglInitLinearTexture(&tex->gxm_tex, texture_data, format, orig_w, orig_h, tex->use_mips ? tex->mip_count : 0);
		tex->palette_UID = 0;
		tex->status = TEX_VALID;
		tex->data = texture_data;
	}
}

void gpu_free_palette(palette *pal) {
	// Deallocating palette memblock and object
	if (pal == NULL)
		return;
	vgl_free(pal->data);
	vgl_free(pal);
}
