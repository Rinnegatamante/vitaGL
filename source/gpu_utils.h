typedef struct texture{
	SceGxmTexture gxm_tex;
	SceUID data_UID;
	SceUID palette_UID;
	SceGxmRenderTarget* gxm_rtgt;
	SceGxmColorSurface gxm_sfc;
	SceGxmDepthStencilSurface gxm_sfd;
	SceUID depth_UID;
	uint8_t used;
	uint8_t valid;
	uint32_t type;
} texture;

typedef struct palette{
	void* data;
	SceUID palette_UID;
} palette;

void* gpu_alloc_map(SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpu_attrib, size_t size, SceUID *uid);
void gpu_unmap_free(SceUID uid);
void* gpu_vertex_usse_alloc_map(size_t size, SceUID *uid, unsigned int *usse_offset);
void gpu_vertex_usse_unmap_free(SceUID uid);
void *gpu_fragment_usse_alloc_map(size_t size, SceUID *uid, unsigned int *usse_offset);
void gpu_fragment_usse_unmap_free(SceUID uid);
void* gpu_pool_malloc(unsigned int size);
void* gpu_pool_memalign(unsigned int size, unsigned int alignment);
unsigned int gpu_pool_free_space();
void gpu_pool_reset();
void gpu_pool_init(uint32_t temp_pool_size);
int tex_format_to_bytespp(SceGxmTextureFormat format);
void gpu_alloc_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void* data, texture* tex);
void gpu_free_texture(texture* tex);
palette* gpu_alloc_palette(const void* data, uint32_t w, uint32_t bpe);
void gpu_free_palette(palette* pal);
void gpu_prepare_rendertarget(texture* tex);
void gpu_destroy_rendertarget(texture* tex);
void gpu_alloc_mipmaps(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void* data, int level, texture* tex);