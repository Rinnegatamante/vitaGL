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
 * gpu_utils.h:
 * Header file for the GPU utilities exposed by gpu_utils.c
 */

#ifndef _GPU_UTILS_H_
#define _GPU_UTILS_H_

#include "debug_utils.h"
#include "mem_utils.h"

extern uint32_t vgl_framecount; // Current frame number since application started

// Align a value to the requested alignment
#define VGL_ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define ALIGNBLOCK(x, a) (((x) + ((a)-1)) / a)

extern uint32_t vgl_tex_cache_freq; // Number of frames prior a texture becomes cacheable if not used

// Texture object status enum
enum {
	TEX_UNUSED,
	TEX_UNINITIALIZED,
	TEX_VALID
};

// Texture object struct
typedef struct {
#ifndef TEXTURES_SPEEDHACK
	uint32_t last_frame;
#endif
#ifdef HAVE_TEX_CACHE
	uint32_t upload_frame;
	uint64_t hash;
#endif
	uint8_t status;
	uint8_t mip_count;
	uint8_t ref_counter;
	uint8_t faces_counter;
	GLboolean use_mips;
	GLboolean dirty;
	GLboolean overridden;
	SceGxmTexture gxm_tex;
	void *data;
	void *palette_data;
	uint32_t type;
	void (*write_cb)(void *, uint32_t);
	SceGxmTextureFilter min_filter;
	SceGxmTextureFilter mag_filter;
	SceGxmTextureAddrMode u_mode;
	SceGxmTextureAddrMode v_mode;
	SceGxmTextureMipFilter mip_filter;
	uint32_t lod_bias;
#ifdef HAVE_UNPURE_TEXTURES
	int8_t mip_start;
#endif
} texture;

// Alloc a generic memblock into sceGxm mapped memory with alignment
void *gpu_alloc_mapped_aligned(size_t alignment, size_t size, vglMemType type);

// Alloc a generic memblock into sceGxm mapped memory
static inline __attribute__((always_inline)) void *gpu_alloc_mapped(size_t size, vglMemType type) {
	return gpu_alloc_mapped_aligned(MEM_ALIGNMENT, size, type);
}

// Alloc a generic memblock into sceGxm mapped memory and marks it for garbage collection
static inline __attribute__((always_inline)) void *gpu_alloc_mapped_temp(size_t size) {
#ifndef HAVE_CIRCULAR_VERTEX_POOL
	// Allocating memblock and marking it for garbage collection
	void *res = gpu_alloc_mapped(size, use_vram ? VGL_MEM_VRAM : VGL_MEM_RAM);

#ifdef LOG_ERRORS
	if (!res)
		vgl_log("%s:%d gpu_alloc_mapped_temp failed with a requested size of 0x%08X\n", __FILE__, __LINE__, size);
#endif

	markAsDirty(res);
	return res;
#else
	return reserve_data_pool(size);
#endif
}

// Alloc into sceGxm mapped memory a vertex USSE memblock
void *gpu_vertex_usse_alloc_mapped(size_t size, unsigned int *usse_offset);

// Dealloc from sceGxm mapped memory a vertex USSE memblock
void gpu_vertex_usse_free_mapped(void *addr);

// Alloc into sceGxm mapped memory a fragment USSE memblock
void *gpu_fragment_usse_alloc_mapped(size_t size, unsigned int *usse_offset);

// Dealloc from sceGxm mapped memory a fragment USSE memblock
void gpu_fragment_usse_free_mapped(void *addr);

// Calculate bpp for a requested texture format
static inline __attribute__((always_inline)) int tex_format_to_bytespp(SceGxmTextureFormat format) {
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
	case SCE_GXM_TEXTURE_BASE_FORMAT_F16F16F16F16:
		return 8;
	case SCE_GXM_TEXTURE_BASE_FORMAT_U8U8U8U8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S8S8S8S8:
	case SCE_GXM_TEXTURE_BASE_FORMAT_F32:
	case SCE_GXM_TEXTURE_BASE_FORMAT_U32:
	case SCE_GXM_TEXTURE_BASE_FORMAT_S32:
	default:
		return 4;
	}
}

// Alloc a texture
void gpu_alloc_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *), void (*write_cb)(void *, uint32_t), GLboolean fast_store);

// Alloc a cube texture
void gpu_alloc_cube_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, SceGxmTransferFormat src_format, const void *data, texture *tex, uint8_t src_bpp, int index);

// Alloc a compresseed texture
void gpu_alloc_compressed_texture(int32_t level, uint32_t w, uint32_t h, SceGxmTextureFormat format, uint32_t image_size, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *));

// Alloc a compressed cube texture
void gpu_alloc_compressed_cube_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, uint32_t image_size, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *), int index);

// Alloc a paletted texture
void gpu_alloc_paletted_texture(int32_t level, uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *));

// Dealloc a texture data
static inline __attribute__((always_inline)) void gpu_free_texture_data(texture *tex) {
	// Deallocating texture
	if (tex->data != NULL) {
#ifdef HAVE_TEX_CACHE
		if (tex->last_frame == OBJ_CACHED) {
			char fname[256], hash[24]; \
			sprintf(hash, "%llX", tex->hash); \
			sprintf(fname, "%s/%c%c/%s.raw", vgl_file_cache_path, hash[0], hash[1], hash); \
			sceIoRemove(fname);
			tex->data = NULL;
			return;
		}
#endif
#ifndef TEXTURES_SPEEDHACK
		if (vgl_framecount - tex->last_frame > FRAME_PURGE_FREQ)
			vgl_free(tex->data);
		else
#endif
			markAsDirty(tex->data);
		tex->data = NULL;
	}
	if (tex->palette_data != NULL) {
#ifndef TEXTURES_SPEEDHACK
		if (vgl_framecount - tex->last_frame > FRAME_PURGE_FREQ)
			vgl_free(tex->palette_data);
		else
#endif
			markAsDirty(tex->palette_data);
		tex->palette_data = NULL;
	}
}

// Dealloc a texture
static inline __attribute__((always_inline)) void gpu_free_texture(texture *tex) {
	gpu_free_texture_data(tex);
	tex->status = TEX_UNUSED;
}

// Alloc a palette
void *gpu_alloc_palette(const void *data, uint32_t w, uint32_t bpe);

// Dealloc a palette
void gpu_free_palette(void *pal);

// Generate mipmaps for a given texture
void gpu_alloc_mipmaps(int level, texture *tex);

#endif
