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

#include "texture_swizzler.h"

#ifdef HAVE_TEX_CACHE
#define XXH_STATIC_LINKING_ONLY
#define XXH_NAMESPACE VITAGL_
#include "xxhash_utils.h"
#endif

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

#ifndef MAX
#define MAX(a, b) (((a) < (b)) ? (b) : (a))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifdef HAVE_TEX_CACHE
uint32_t vgl_tex_cache_freq = 3600; // Number of frames prior a texture becomes cacheable if not used
#endif

// VRAM usage setting
vglMemType VGL_MEM_MAIN = VGL_MEM_VRAM;
uint8_t use_vram_for_usse = GL_FALSE;

// Newlib mempool usage setting
GLboolean use_extra_mem = GL_TRUE;

// Taken from here: https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static inline __attribute__((always_inline)) uint32_t nearest_po2(uint32_t val) {
	val--;
	val |= val >> 1;
	val |= val >> 2;
	val |= val >> 4;
	val |= val >> 8;
	val |= val >> 16;
	val++;

	return val;
}

static inline __attribute__((always_inline)) uint64_t morton_1(uint64_t x) {
	x = x & 0x5555555555555555;
	x = (x | (x >> 1)) & 0x3333333333333333;
	x = (x | (x >> 2)) & 0x0F0F0F0F0F0F0F0F;
	x = (x | (x >> 4)) & 0x00FF00FF00FF00FF;
	x = (x | (x >> 8)) & 0x0000FFFF0000FFFF;
	x = (x | (x >> 16)) & 0xFFFFFFFFFFFFFFFF;
	return x;
}

static inline __attribute__((always_inline)) void d2xy_morton(uint64_t d, uint64_t *x, uint64_t *y) {
	*x = morton_1(d);
	*y = morton_1(d >> 1);
}

static inline __attribute__((always_inline)) void extract_block(const uint8_t *src, int width, uint8_t *block) {
	for (int j = 0; j < 4; j++) {
		vgl_fast_memcpy(&block[j * 4 * 4], src, 16);
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

static int unsafe_allocator_counter = 0;
void *gpu_alloc_mapped_aligned_unsafe(size_t alignment, size_t size, vglMemType type) {
	// Performing a garbage collection cycle prior to attempting to allocate the memory again
	unsafe_allocator_counter++;
	sceGxmFinish(gxm_context);
#if defined(HAVE_SINGLE_THREADED_GC) && !defined(HAVE_PTHREAD)
	garbage_collector(0, NULL);
#else
	sceKernelWaitSema(gc_mutex[1], 1, NULL);
	sceKernelSignalSema(gc_mutex[0], 1);
	sceKernelDelayThread(1000000);
#endif

	// Allocating requested memblock
	void *res = vgl_memalign(alignment, size, type);
	if (res)
		return res;

	// Requested memory type finished, using other one
	res = vgl_memalign(alignment, size, type == VGL_MEM_VRAM ? VGL_MEM_RAM : VGL_MEM_VRAM);
	if (res)
		return res;

	// Even the other one failed, trying with physically contiguous RAM
	res = vgl_memalign(alignment, size, VGL_MEM_SLOW);
	if (res)
		return res;

	// Even this failed, attempting with game common dialog RAM
	res = vgl_memalign(alignment, size, VGL_MEM_BUDGET);
	if (res)
		return res;

	// Internal mempools finished, using newlib mem
	if (use_extra_mem)
		res = vgl_memalign(alignment, size, VGL_MEM_EXTERNAL);

	// Iterating for as many as possible max pending garbage collector cycles
	if (!res && unsafe_allocator_counter < FRAME_PURGE_FREQ)
		res = gpu_alloc_mapped_aligned_unsafe(alignment, size, type);

	return res;
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

	// Even the other one failed, trying with physically contiguous RAM
	res = vgl_memalign(alignment, size, VGL_MEM_SLOW);
	if (res)
		return res;

	// Even this failed, attempting with game common dialog RAM
	res = vgl_memalign(alignment, size, VGL_MEM_BUDGET);
	if (res)
		return res;

	// Internal mempools finished, using newlib mem
	if (use_extra_mem)
		res = vgl_memalign(alignment, size, VGL_MEM_EXTERNAL);
	
#ifdef HAVE_TEX_CACHE
	// Cache any unused texture to free enough space
	uint32_t cached_elements = 0;
	uint32_t cached_bytes = 0;
	while (vgl_uncached_tex_head && cached_bytes < size) {
		texture *tex = vgl_uncached_tex_head; // Just some readability sugar
		if ((tex->last_frame == OBJ_NOT_USED && tex->upload_frame != vgl_framecount) || (vgl_framecount - tex->last_frame > vgl_tex_cache_freq && tex->last_frame < OBJ_CACHED)) {
			if (tex->next)
				tex->next->prev = NULL;
			tex->last_frame = OBJ_CACHED;
			SceGxmTextureFormat tex_format = sceGxmTextureGetFormat(&tex->gxm_tex);
			uint8_t bpp = tex_format_to_bytespp(tex_format);
			uint32_t orig_w = sceGxmTextureGetWidth(&tex->gxm_tex);
			uint32_t orig_h = sceGxmTextureGetHeight(&tex->gxm_tex);
			uint32_t size = VGL_ALIGN(orig_w, 8) * bpp * orig_h;
			char fname[256], hash_str[24];
			uint64_t hash = XXH3_64bits(tex->data, size);
			sprintf(hash_str, "%llX", hash);
			sprintf(fname, "%s/%c%c", vgl_file_cache_path, hash_str[0], hash_str[1]);
			sceIoMkdir(fname, 0777);
			sprintf(fname, "%s/%c%c/%s.raw", vgl_file_cache_path, hash_str[0], hash_str[1], hash_str);
			SceUID f = sceIoOpen(fname, SCE_O_CREAT | SCE_O_TRUNC | SCE_O_WRONLY, 0777);
			sceIoWrite(f, tex->data, size);
			sceIoClose(f);
			vgl_free(tex->data);
			tex->hash = hash;
			tex->prev = NULL;
			tex->next = NULL;
			cached_elements++;
			cached_bytes += size;
		} else {
			break;
		}
	}
	if (cached_elements) {
		vgl_log("%s:%d gpu_alloc_mapped_aligned failed with a requested size of %u bytes, cached %d textures to recover %d bytes.\n", __FILE__, __LINE__, size, cached_elements, cached_bytes);
		if (cached_bytes >= size)
			return gpu_alloc_mapped_aligned(alignment, size, type);
	}
#endif

	if (!res) {
		// Attempt to force garbage collector in order to try to free some mem in an unsafe way
#ifdef LOG_ERRORS
		vgl_log("%s:%d gpu_alloc_mapped_aligned failed with a requested size of %u bytes, attempting to forcefully free required memory.\n", __FILE__, __LINE__, size);
#endif
		unsafe_allocator_counter = 0;
		res = gpu_alloc_mapped_aligned_unsafe(alignment, size, type);

#ifdef LOG_ERRORS
		if (!res)
			vgl_log("%s:%d gpu_alloc_mapped_aligned_unsafe failed with a requested size of %u bytes.\n", __FILE__, __LINE__, size);
		else
			vgl_log("%s:%d gpu_alloc_mapped_aligned_unsafe successfully allocated the requested memory after forcing %d garbage collection cycles.\n", __FILE__, __LINE__, unsafe_allocator_counter);
#endif
	}

	return res;
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

static inline __attribute__((always_inline)) SceGxmTransferFormat tex_format_to_transfer(SceGxmTextureFormat format) {
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

static inline __attribute__((always_inline)) int tex_format_to_alignment(SceGxmTextureFormat format) {
	switch (format & 0x9F000000) {
	case SCE_GXM_TEXTURE_BASE_FORMAT_UBC2:
	case SCE_GXM_TEXTURE_BASE_FORMAT_UBC3:
		return 16;
	default:
		return 8;
	}
}

void *gpu_alloc_palette(const void *data, uint32_t w, uint32_t bpe) {
	// Allocating palette data buffer
	void *texture_palette = gpu_alloc_mapped_aligned(SCE_GXM_PALETTE_ALIGNMENT, 256 * sizeof(uint32_t), VGL_MEM_MAIN);

	// Initializing palette
	if (data == NULL)
		vgl_memset(texture_palette, 0, 256 * sizeof(uint32_t));
	else if (bpe == 4)
		vgl_fast_memcpy(texture_palette, data, w * sizeof(uint32_t));

	// Returning palette
	return texture_palette;
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
	const int face_size = VGL_ALIGN(w, 8) * h * bpp;
	void *base_texture_data = tex->faces_counter == 1 ? gpu_alloc_mapped(face_size * 6, VGL_MEM_MAIN) : tex->data;

	if (base_texture_data != NULL) {
		// Calculating face texture data pointer
		uint8_t *texture_data = (uint8_t *)base_texture_data + face_size * index;

		if (data != NULL) {
			const int tex_size = w * h * bpp;
			void *mapped_data = gpu_alloc_mapped_temp(tex_size);
			vgl_fast_memcpy(mapped_data, data, tex_size);
			SceGxmTransferFormat dst_fmt = tex_format_to_transfer(format);
			sceGxmTransferCopy(
				w, h, 0, 0, SCE_GXM_TRANSFER_COLORKEY_NONE,
				src_format, SCE_GXM_TRANSFER_LINEAR,
				mapped_data, 0, 0, w * src_bpp,
				dst_fmt, SCE_GXM_TRANSFER_SWIZZLED,
				texture_data, 0, 0, VGL_ALIGN(w, 8) * bpp,
				NULL, 0, NULL);
		} else
			vgl_memset(texture_data, 0, face_size);

		// Initializing texture and validating it
		tex->mip_count = 0;
		vglInitCubeTexture(&tex->gxm_tex, base_texture_data, format, w, h, tex->mip_count);
		tex->palette_data = NULL;
		tex->status = TEX_VALID;
		tex->data = base_texture_data;
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = OBJ_NOT_USED;
#endif
#ifdef HAVE_TEX_CACHE
		markAsCacheable(tex)
#endif
	}
}

void gpu_alloc_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *), void (*write_cb)(void *, uint32_t), GLboolean fast_store) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID)
		gpu_free_texture_data(tex);

	// Getting texture format bpp
	uint8_t bpp = tex_format_to_bytespp(format);

	// Allocating texture data buffer
	const int aligned_w = VGL_ALIGN(w, 8);
	const int tex_size = aligned_w * h * bpp;
	void *texture_data = gpu_alloc_mapped(tex_size, VGL_MEM_MAIN);

	if (texture_data != NULL) {
		// Initializing texture data buffer
		if (data != NULL) {
			uint8_t *src = (uint8_t *)data;
			uint8_t *dst;
			if (fast_store) { // Internal Format and Data Format are the same, we can just use vgl_fast_memcpy for better performance
				if (aligned_w == w) // Texture size is already aligned, we can use a single vgl_fast_memcpy for better performance
					vgl_fast_memcpy(texture_data, src, tex_size);
				else {
					uint32_t line_size = w * bpp;
					for (int i = 0; i < h; i++) {
						dst = ((uint8_t *)texture_data) + (aligned_w * bpp) * i;
						vgl_fast_memcpy(dst, src, line_size);
						src += line_size;
					}
				}
			} else { // Different internal and data formats, we need to go with slower callbacks system
				for (int i = 0; i < h; i++) {
					dst = ((uint8_t *)texture_data) + (aligned_w * bpp) * i;
					for (int j = 0; j < w; j++) {
						uint32_t clr = read_cb(src);
						write_cb(dst, clr);
						src += src_bpp;
						dst += bpp;
					}
				}
			}
		} else
			vgl_memset(texture_data, 0, tex_size);

		// Initializing texture and validating it
		tex->mip_count = 1;
		vglInitLinearTexture(&tex->gxm_tex, texture_data, format, w, h, tex->mip_count);
		if ((format & 0x9F000000U) == SCE_GXM_TEXTURE_BASE_FORMAT_P8)
			tex->palette_data = color_table;
		else
			tex->palette_data = NULL;
		tex->status = TEX_VALID;
		tex->data = texture_data;
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = OBJ_NOT_USED;
#endif
#ifdef HAVE_TEX_CACHE
		markAsCacheable(tex)
#endif
	}
}

void gpu_alloc_paletted_texture(int32_t level, uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *)) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID)
		gpu_free_texture_data(tex);

	// Check if the texture is P8
	uint8_t is_p8 = tex_format_to_bytespp(format);
	uint32_t orig_w = w;
	uint32_t orig_h = h;

	// Calculating texture data buffer size
	uint32_t tex_size = 0;
	for (int j = 0; j <= level; j++) {
		tex_size += is_p8 ? (w * h) : (w * h / 2);
		w /= 2;
		h /= 2;
	}

	// Allocating texture and palette data buffers
	int num_entries = is_p8 ? 256 : 16;
	tex->palette_data = gpu_alloc_mapped_aligned(SCE_GXM_PALETTE_ALIGNMENT, num_entries * sizeof(uint32_t), VGL_MEM_MAIN);
	tex->data = gpu_alloc_mapped(tex_size, VGL_MEM_MAIN);

	// Populating palette data
	uint32_t *palette_data = (uint32_t *)tex->palette_data;
	uint8_t *src = (uint8_t *)data;
	for (int i = 0; i < num_entries; i++) {
		palette_data[i] = read_cb(src);
		src += src_bpp;
	}

	// Populating texture data
	if (is_p8)
		vgl_fast_memcpy(tex->data, src, tex_size);
	else {
		uint8_t *dst = (uint8_t *)tex->data;
		for (int i = 0; i < tex_size; i++) {
			dst[i] = ((src[i] & 0x0F) << 4) | (src[i] >> 4);
		}
	}

	// Initializing texture and validating it
	tex->mip_count = level + 1;
	vglInitLinearTexture(&tex->gxm_tex, tex->data, format, orig_w, orig_h, tex->mip_count);
	tex->status = TEX_VALID;
#ifndef TEXTURES_SPEEDHACK
	tex->last_frame = OBJ_NOT_USED;
#endif
#ifdef HAVE_TEX_CACHE
	markAsCacheable(tex)
#endif
}

static inline __attribute__((always_inline)) int gpu_get_compressed_mip_size(int level, int width, int height, SceGxmTextureFormat format) {
	switch (format) {
	case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_1BGR:
	case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_ABGR:
		return (MAX(width, 16) * MAX(height, 8) * 2 + 7) / 8;
	case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_1BGR:
	case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_ABGR:
		return (MAX(width, 8) * MAX(height, 8) * 4 + 7) / 8;
	case SCE_GXM_TEXTURE_FORMAT_PVRTII2BPP_ABGR:
		return ceil(width / 8.0) * ceil(height / 4.0) * 8.0;
	case SCE_GXM_TEXTURE_FORMAT_PVRTII4BPP_ABGR:
		return ceil(width / 4.0) * ceil(height / 4.0) * 8.0;
	case SCE_GXM_TEXTURE_FORMAT_UBC1_1BGR:
	case SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR:
	case SCE_GXM_TEXTURE_FORMAT_ETC1_RGB:
		return ceil(width / 4.0) * ceil(height / 4.0) * 8;
	case SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR:
	case SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR:
		return ceil(width / 4.0) * ceil(height / 4.0) * 16;
	default:
		return 0;
	}
}

static inline __attribute__((always_inline)) int gpu_get_compressed_mipchain_size(int level, int width, int height, SceGxmTextureFormat format) {
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

static inline __attribute__((always_inline)) int gpu_get_compressed_mip_offset(int level, int width, int height, SceGxmTextureFormat format) {
	return gpu_get_compressed_mipchain_size(level - 1, width, height, format);
}

void gpu_alloc_compressed_cube_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, uint32_t image_size, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *), int index) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID && tex->faces_counter >= 6) {
		gpu_free_texture_data(tex);
		tex->faces_counter = 1;
	} else
		tex->faces_counter++;

	// Calculating swizzled compressed texture size on memory
	if (!image_size)
		image_size = gpu_get_compressed_mip_size(0, w, h, format);

	const uint32_t aligned_width = nearest_po2(w);
	const uint32_t aligned_height = nearest_po2(h);
	uint32_t max_width, max_height, aligned_max_width, aligned_max_height;
	max_width = w;
	max_height = h;
	aligned_max_width = aligned_width;
	aligned_max_height = aligned_height;

	// Allocating texture data buffer
	const int mip_offset = gpu_get_compressed_mip_offset(0, aligned_max_width, aligned_max_height, format);
	const int face_size = gpu_get_compressed_mipchain_size(0, aligned_max_width, aligned_max_height, format);
	const int mip_size = face_size - mip_offset;
	void *base_texture_data = tex->faces_counter == 1 ? gpu_alloc_mapped(face_size * 6, VGL_MEM_MAIN) : tex->data;
	void *texture_data = (uint8_t *)base_texture_data + face_size * index;

	// Initializing texture data buffer
	if (texture_data != NULL) {
		void *mip_data = (void *)((uint8_t *)texture_data + mip_offset);
		if (data != NULL) {
			if (read_cb != NULL) {
				// Performing swizzling and DXT compression
				uint8_t alignment = tex_format_to_alignment(format);
				dxt_compress(mip_data, (uint8_t *)data, aligned_width, aligned_height, alignment == 16);
			} else {
				// Perform swizzling if necessary.
				switch (format) {
				case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_1BGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_ABGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_1BGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_ABGR:
					vgl_fast_memcpy(mip_data, data, image_size);
					break;
				case SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR:
				case SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR:
					SwizzleTexData128Bpp((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 4), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 4), ALIGNBLOCK(MIN(aligned_width, aligned_height), 4));
					break;
				case SCE_GXM_TEXTURE_FORMAT_PVRTII2BPP_ABGR:
					SwizzleTexData64Bpp((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 8), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 8), MIN(ALIGNBLOCK(aligned_width, 8), ALIGNBLOCK(aligned_height, 4)));
					break;
				case SCE_GXM_TEXTURE_FORMAT_ETC1_RGB:
					SwizzleTexDataETC1((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 4), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 4), ALIGNBLOCK(MIN(aligned_width, aligned_height), 4));
					break;
				default:
					SwizzleTexData64Bpp((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 4), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 4), ALIGNBLOCK(MIN(aligned_width, aligned_height), 4));
					break;
				}
			}
		} else
			vgl_memset(mip_data, 0, mip_size);

		// Initializing texture and validating it
		tex->mip_count = 0;
		vglInitCubeTexture(&tex->gxm_tex, base_texture_data, format, w, h, tex->mip_count);
		tex->palette_data = NULL;
		tex->status = TEX_VALID;
		tex->data = base_texture_data;
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = OBJ_NOT_USED;
#endif
#ifdef HAVE_TEX_CACHE
		markAsCacheable(tex)
#endif
	}
}

void gpu_alloc_compressed_texture(int32_t mip_level, uint32_t w, uint32_t h, SceGxmTextureFormat format, uint32_t image_size, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void *)) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID && !mip_level)
		gpu_free_texture_data(tex);

	// Calculating swizzled compressed texture size on memory
	if (!image_size)
		image_size = gpu_get_compressed_mip_size(mip_level, w, h, format);

	const uint32_t aligned_width = nearest_po2(w);
	const uint32_t aligned_height = nearest_po2(h);
	uint32_t max_width, max_height, aligned_max_width, aligned_max_height;
	if (!mip_level) {
		max_width = w;
		max_height = h;
		aligned_max_width = aligned_width;
		aligned_max_height = aligned_height;
	} else {
		max_width = sceGxmTextureGetWidth(&tex->gxm_tex);
		max_height = sceGxmTextureGetHeight(&tex->gxm_tex);
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
				texture_data = gpu_alloc_mapped(tex_size, VGL_MEM_MAIN);
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
		texture_data = gpu_alloc_mapped(tex_size, VGL_MEM_MAIN);
	}

	// Initializing texture data buffer
	if (texture_data != NULL) {
		void *mip_data = (void *)((uint8_t *)texture_data + mip_offset);
		if (data != NULL) {
			if (read_cb != NULL) {
				// Performing swizzling and DXT compression
				uint8_t alignment = tex_format_to_alignment(format);
				dxt_compress(mip_data, (uint8_t *)data, aligned_width, aligned_height, alignment == 16);
			} else {
				// Perform swizzling if necessary.
				switch (format) {
				case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_1BGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT2BPP_ABGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_1BGR:
				case SCE_GXM_TEXTURE_FORMAT_PVRT4BPP_ABGR:
					vgl_fast_memcpy(mip_data, data, image_size);
					break;
				case SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR:
				case SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR:
					SwizzleTexData128Bpp((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 4), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 4), ALIGNBLOCK(MIN(aligned_width, aligned_height), 4));
					break;
				case SCE_GXM_TEXTURE_FORMAT_PVRTII2BPP_ABGR:
					SwizzleTexData64Bpp((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 8), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 8), MIN(ALIGNBLOCK(aligned_width, 8), ALIGNBLOCK(aligned_height, 4)));
					break;
				case SCE_GXM_TEXTURE_FORMAT_ETC1_RGB:
					SwizzleTexDataETC1((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 4), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 4), ALIGNBLOCK(MIN(aligned_width, aligned_height), 4));
					break;
				default:
					SwizzleTexData64Bpp((uint8_t *)mip_data, (uint8_t *)data, 0, 0, ALIGNBLOCK(w, 4), ALIGNBLOCK(h, 4), ALIGNBLOCK(w, 4), ALIGNBLOCK(MIN(aligned_width, aligned_height), 4));
					break;
				}
			}

		} else
			vgl_memset(mip_data, 0, mip_size);

		// Initializing texture and validating it
		tex->mip_count = mip_count + 1;
		vglInitSwizzledTexture(&tex->gxm_tex, texture_data, format, tex_width, tex_height, tex->use_mips ? tex->mip_count : 0);
		tex->palette_data = NULL;
		tex->status = TEX_VALID;
		tex->data = texture_data;
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = OBJ_NOT_USED;
#endif
#ifdef HAVE_TEX_CACHE
		markAsCacheable(tex)
#endif
	}
}

void gpu_alloc_mipmaps(int level, texture *tex) {
	// Getting current mipmap count in passed texture
	int count = tex->mip_count - 1;

	// Checking if we need at least one more new mipmap level
	if ((level > count) || (level < 0)) { // Note: level < 0 means we will use max possible mipmaps level

		// Getting textures info and calculating bpp
		SceGxmTextureFormat format = sceGxmTextureGetFormat(&tex->gxm_tex);
		uint32_t bpp = tex_format_to_bytespp(format);
		uint32_t orig_w = sceGxmTextureGetWidth(&tex->gxm_tex);
		uint32_t orig_h = sceGxmTextureGetHeight(&tex->gxm_tex);
		uint32_t w = nearest_po2(orig_w);
		uint32_t h = nearest_po2(orig_h);

		// Calculating new texture data buffer size
		uint32_t jumps[16];
		uint32_t size = 0;
		int j;
		if (level < 0 || count <= 0) {
			int mips = 0;
			while ((w > 1) && (h > 1)) {
				jumps[mips] = MAX(w, 8) * h * bpp;
				size += jumps[mips];
				w /= 2;
				h /= 2;
				mips++;
			}
			jumps[mips] = MAX(w, 8) * h * bpp;
			size += jumps[mips];
			if (level < 0)
				level = mips;
			else
				level++;
		} else {
			for (j = 0; j < level; j++) {
				jumps[j] = MAX(w, 8) * h * bpp;
				w /= 2;
				h /= 2;
			}
			level++;
		}

		// Calculating needed sceGxmTransfer format for the downscale process
		SceGxmTransferFormat fmt = tex_format_to_transfer(format);

		// Reallocating texture with full mipchain size
		void *texture_data = count <= 0 ? vgl_realloc(tex->data, size) : tex->data;
		if (count <= 0 && !texture_data) {
			// Reallocation in the same mspace failed, try manually.
			texture_data = gpu_alloc_mapped(size, VGL_MEM_MAIN);
			vgl_memcpy(texture_data, tex->data, VGL_ALIGN(orig_w, 8) * orig_h * bpp);
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
			if (curWidth <= 1 || curHeight <= 1)
				break;
			uint32_t curSrcStride = VGL_ALIGN(curWidth, 8);
			uint32_t curDstStride = VGL_ALIGN(curWidth >> 1, 8);
			uint8_t *dstPtr = curPtr + jumps[j];
			if (curWidth <= 1024 && curHeight <= 1024) {
				sceGxmTransferDownscale(
					fmt, curPtr, 0, 0,
					curWidth, curHeight,
					curSrcStride * bpp,
					fmt, dstPtr, 0, 0,
					curDstStride * bpp,
					NULL, 0, NULL);
			} else { // sceGxmTransferDownscale doesn't support higher sizes, so we go for CPU downscaling
				for (int y = 0, y2 = 0; y < curHeight; y += 2, y2++) {
					uint8_t *srcLine = curPtr + curSrcStride * bpp * y;
					uint8_t *dstLine = dstPtr + curDstStride * bpp * y2;
					for (int x = 0, x2 = 0; x < curWidth; x += 2, x2++) {
						sceClibMemcpy(dstLine + x2 * bpp, srcLine + x * bpp, bpp);
					}
				}
			}
			curPtr = dstPtr;
			curWidth /= 2;
			curHeight /= 2;
		}

		// Initializing texture in sceGxm
		tex->mip_count = level;
		vglInitLinearTexture(&tex->gxm_tex, texture_data, format, orig_w, orig_h, tex->use_mips ? tex->mip_count : 0);
		tex->palette_data = NULL;
		tex->status = TEX_VALID;
		tex->data = texture_data;
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = OBJ_NOT_USED;
#endif
#ifdef HAVE_TEX_CACHE
		markAsCacheable(tex)
#endif
	}
}

void gpu_free_palette(void *pal) {
	// Deallocating palette memblock and object
	if (pal != NULL)
		vgl_free(pal);
}

void gpu_alloc_planar_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex) {
	// If there's already a texture in passed texture object we first dealloc it
	if (tex->status == TEX_VALID)
		gpu_free_texture_data(tex);

	// Allocating texture data buffer
	const int plane_w = MAX(nearest_po2(w), 8);
	const int plane_h = nearest_po2(h);
	const int half_plane_w = MAX(nearest_po2(w / 2), 8);
	const int half_plane_h = MAX(nearest_po2(h / 2), 8);
	const int half_w = w / 2;
	const int half_h = h / 2;
	const int aligned_w = VGL_ALIGN(w, 8);
	const int aligned_half_w = VGL_ALIGN(w / 2, 8);
	const int tex_size = plane_w * plane_h + half_plane_w * half_plane_h * 2;
	void *texture_data = gpu_alloc_mapped(tex_size, VGL_MEM_MAIN);

	if (texture_data != NULL) {
		// Initializing texture data buffer
		if (data != NULL) {
			if (plane_w == w && half_plane_w == half_w) // Texture size is already aligned, we can use a single vgl_fast_memcpy for better performance
				vgl_fast_memcpy(texture_data, data, tex_size);
			else if ((format & 0x9F000000) == SCE_GXM_TEXTURE_BASE_FORMAT_YUV420P3) {
				uint8_t *src_y = (uint8_t *)data;
				uint8_t *src_u = src_y + w * h;
				uint8_t *src_v = src_u + half_w * half_h;
				uint8_t *dst_y = (uint8_t *)texture_data;
				uint8_t *dst_u = dst_y + plane_w * plane_h;
				uint8_t *dst_v = dst_u + half_plane_w * half_plane_h;

				if (aligned_w == w && aligned_half_w == half_w) { // Plane size is already aligned, we can use a single vgl_fast_memcpy for better performance
					vgl_fast_memcpy(dst_y, src_y, w * h);
					vgl_fast_memcpy(dst_u, src_u, half_w * half_h);
					vgl_fast_memcpy(dst_v, src_v, half_w * half_h);
				} else {
					// Copy two Y rows and one UV row at a time
					for (int i = 0; i < half_h; i++) {
						vgl_fast_memcpy(dst_y, src_y, w);
						vgl_fast_memcpy(dst_u, src_u, half_w);
						vgl_fast_memcpy(dst_v, src_v, half_w);
						src_y += w;
						src_u += half_w;
						src_v += half_w;
						dst_y += aligned_w;
						dst_u += aligned_half_w;
						dst_v += aligned_half_w;
						vgl_fast_memcpy(dst_y, src_y, w);
						src_y += w;
						dst_y += aligned_w;
					}
				}
			} else {
				uint8_t  *src_y = (uint8_t *)data;
				uint16_t *src_uv = (uint16_t *)(src_y + w * h);
				uint8_t  *dst_y = (uint8_t *)texture_data;
				uint16_t *dst_uv = (uint16_t *)(dst_y + plane_w * plane_h);

				if (aligned_w == w && aligned_half_w == half_w) { // Plane size is already aligned, we can use a single vgl_fast_memcpy for better performance
					vgl_fast_memcpy(dst_y, src_y, w * h);
					vgl_fast_memcpy(dst_uv, src_uv, half_w * half_h * sizeof(uint16_t));
				} else {
					// Copy two Y rows and one UV row at a time
					for (int i = 0; i < half_h; i++) {
						vgl_fast_memcpy(dst_y, src_y, w);
						vgl_fast_memcpy(dst_uv, src_uv, half_w * sizeof(uint16_t));
						src_y += w;
						src_uv += half_w;
						dst_y += aligned_w;
						dst_uv += aligned_half_w;
						vgl_fast_memcpy(dst_y, src_y, w);
						src_y += w;
						dst_y += aligned_w;
					}
				}
			}
		} else
			vgl_memset(texture_data, 0, tex_size);

		// Initializing texture and validating it
		tex->mip_count = 1;
		vglInitLinearTexture(&tex->gxm_tex, texture_data, format, w, h, tex->mip_count);
		tex->palette_data = NULL;
		tex->status = TEX_VALID;
		tex->data = texture_data;
#ifndef TEXTURES_SPEEDHACK
		tex->last_frame = 0xFFFFFFFF;
#endif
	}
}
