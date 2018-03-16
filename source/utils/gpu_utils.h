/* 
 * gpu_utils.h:
 * Header file for the GPU utilities exposed by gpu_utils.c
 */

#ifndef _GPU_UTILS_H_
#define _GPU_UTILS_H_

// Align a value to the requested alignment
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

// Texture object struct
typedef struct texture{
	SceGxmTexture gxm_tex;
	SceUID data_UID;
	SceUID palette_UID;
	SceGxmRenderTarget *gxm_rtgt;
	SceGxmColorSurface gxm_sfc;
	SceGxmDepthStencilSurface gxm_sfd;
	SceUID depth_UID;
	uint8_t used;
	uint8_t valid;
	uint32_t type;
	void *data;
	void (*write_cb)(void*, uint32_t);
} texture;

// Palette object struct
typedef struct palette{
	void *data;
	SceUID palette_UID;
} palette;

// Alloc and map into sceGxm a generic memblock
void* gpu_alloc_map(SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpu_attrib, size_t size, SceUID *uid);

// Dealloc and unmap from sceFxm a generic memblock
void gpu_unmap_free(SceUID uid);

// Alloc and map into sceGxm a vertex USSE memblock
void* gpu_vertex_usse_alloc_map(size_t size, SceUID *uid, unsigned int *usse_offset);

// Dealloc and unmap from sceGxm a vertex USSE memblock
void gpu_vertex_usse_unmap_free(SceUID uid);

// Alloc and map into sceGxm a fragment USSE memblock
void *gpu_fragment_usse_alloc_map(size_t size, SceUID *uid, unsigned int *usse_offset);

// Dealloc and unmap from sceGxm a fragment USSE memblock
void gpu_fragment_usse_unmap_free(SceUID uid);

// Reserve a memory space from vitaGL mempool
void* gpu_pool_malloc(unsigned int size);

// Reserve an aligned memory space from vitaGL mempool
void* gpu_pool_memalign(unsigned int size, unsigned int alignment);

// Returns available free space on vitaGL mempool
unsigned int gpu_pool_free_space();

// Resets vitaGL mempool
void gpu_pool_reset();

// Alloc vitaGL mempool
void gpu_pool_init(uint32_t temp_pool_size);

// Calculate bpp for a requested texture format
int tex_format_to_bytespp(SceGxmTextureFormat format);

// Alloc a texture
void gpu_alloc_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void *data, texture *tex, uint8_t src_bpp, uint32_t (*read_cb)(void*),  void (*write_cb)(void*, uint32_t));

// Dealloc a texture
void gpu_free_texture(texture *tex);

// Alloc a palette
palette* gpu_alloc_palette(const void *data, uint32_t w, uint32_t bpe);

// Dealloc a palette
void gpu_free_palette(palette *pal);

// Generate mipmaps for a given texture
void gpu_alloc_mipmaps(int level, texture *tex);

#endif
