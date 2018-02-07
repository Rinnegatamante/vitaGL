#include <vitasdk.h>
#include <stdlib.h>
#include "gpu_utils.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

uint8_t use_vram = 0;

// Temporary memory pool
static void *pool_addr = NULL;
static SceUID poolUid;
static unsigned int pool_index = 0;
static unsigned int pool_size = 0;

void* gpu_alloc_map(SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpu_attrib, size_t size, SceUID *uid){
	SceUID memuid;
	void *addr;

	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW)
		size = ALIGN(size, 256 * 1024);
	else
		size = ALIGN(size, 4 * 1024);

	memuid = sceKernelAllocMemBlock("gpumem", type, size, NULL);
	if (memuid < 0)
		return NULL;

	if (sceKernelGetMemBlockBase(memuid, &addr) < 0)
		return NULL;

	if (sceGxmMapMemory(addr, size, gpu_attrib) < 0) {
		sceKernelFreeMemBlock(memuid);
		return NULL;
	}

	if (uid)
		*uid = memuid;

	return addr;
}

void gpu_unmap_free(SceUID uid){
	void *addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
		return;

	sceGxmUnmapMemory(addr);

	sceKernelFreeMemBlock(uid);
}

void* gpu_vertex_usse_alloc_map(size_t size, SceUID *uid, unsigned int *usse_offset){
	SceUID memuid;
	void *addr;

	size = ALIGN(size, 4 * 1024);

	memuid = sceKernelAllocMemBlock("gpu_vertex_usse",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (memuid < 0)
		return NULL;

	if (sceKernelGetMemBlockBase(memuid, &addr) < 0)
		return NULL;

	if (sceGxmMapVertexUsseMemory(addr, size, usse_offset) < 0)
		return NULL;

	return addr;
}

void gpu_vertex_usse_unmap_free(SceUID uid){
	void *addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
		return;

	sceGxmUnmapVertexUsseMemory(addr);

	sceKernelFreeMemBlock(uid);
}

void *gpu_fragment_usse_alloc_map(size_t size, SceUID *uid, unsigned int *usse_offset){
	SceUID memuid;
	void *addr;

	size = ALIGN(size, 4 * 1024);

	memuid = sceKernelAllocMemBlock("gpu_fragment_usse",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (memuid < 0)
		return NULL;

	if (sceKernelGetMemBlockBase(memuid, &addr) < 0)
		return NULL;

	if (sceGxmMapFragmentUsseMemory(addr, size, usse_offset) < 0)
		return NULL;

	return addr;
}

void gpu_fragment_usse_unmap_free(SceUID uid){
	void *addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
		return;

	sceGxmUnmapFragmentUsseMemory(addr);

	sceKernelFreeMemBlock(uid);
}

void* gpu_pool_malloc(unsigned int size){
	if ((pool_index + size) < pool_size) {
		void *addr = (void *)((unsigned int)pool_addr + pool_index);
		pool_index += size;
		return addr;
	}
	return NULL;
}

void* gpu_pool_memalign(unsigned int size, unsigned int alignment){
	unsigned int new_index = (pool_index + alignment - 1) & ~(alignment - 1);
	if ((new_index + size) < pool_size) {
		void *addr = (void *)((unsigned int)pool_addr + new_index);
		pool_index = new_index + size;
		return addr;
	}
	return NULL;
}

unsigned int gpu_pool_free_space(){
	return pool_size - pool_index;
}

void gpu_pool_reset(){
	pool_index = 0;
}

void gpu_pool_init(uint32_t temp_pool_size){
	pool_size = temp_pool_size;
	pool_addr = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
		SCE_GXM_MEMORY_ATTRIB_READ,
		pool_size,
		&poolUid);
}

int tex_format_to_bytespp(SceGxmTextureFormat format){
	switch (format & 0x9f000000U) {
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

palette* gpu_alloc_palette(const void* data, uint32_t w, uint32_t bpe){
	palette* res = (palette*)malloc(sizeof(palette));
	void *texture_palette = gpu_alloc_map(
		(use_vram ? SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW : SCE_KERNEL_MEMBLOCK_TYPE_USER_RW),
		SCE_GXM_MEMORY_ATTRIB_READ,
		256 * sizeof(uint32_t),
		&res->palette_UID);
	if (data == NULL) memset(texture_palette, 0, 256 * sizeof(uint32_t));
	else if(bpe == 4) memcpy(texture_palette, data, w * sizeof(uint32_t));
	res->data = texture_palette;
	return res;
}

void gpu_free_texture(texture* tex){
	if (tex->data_UID != 0) gpu_unmap_free(tex->data_UID);
	tex->data_UID = 0;
	tex->valid = 0;
}

void gpu_alloc_texture(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void* data, texture* tex){
	if (tex->valid) gpu_free_texture(tex);
	const int tex_size = w * h * tex_format_to_bytespp(format);
	void *texture_data = gpu_alloc_map(
		(use_vram ? SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW : SCE_KERNEL_MEMBLOCK_TYPE_USER_RW),
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
		tex_size, &tex->data_UID);
	if (texture_data != NULL){
		sceGxmColorSurfaceInit(&tex->gxm_sfc,
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			w,h,w,texture_data);	
		if (data != NULL) memcpy(texture_data, (uint8_t*)data, tex_size);
		else memset(texture_data, 0, tex_size);
		sceGxmTextureInitLinear(&tex->gxm_tex, texture_data, format, w, h, 0);
	
		if ((format & 0x9f000000U) == SCE_GXM_TEXTURE_BASE_FORMAT_P8) tex->palette_UID = 1;
		else tex->palette_UID = 0;
		tex->valid = 1;
	}
}

void gpu_alloc_mipmaps(uint32_t w, uint32_t h, SceGxmTextureFormat format, const void* data, int level, texture* tex){
	// Discarding passed content and using sceGxmTransfer to create desired level of mipmap
	uint32_t count = sceGxmTextureGetMipmapCount(&tex->gxm_tex);
	uint32_t orig_w = w = sceGxmTextureGetWidth(&tex->gxm_tex);
	uint32_t orig_h = h = sceGxmTextureGetHeight(&tex->gxm_tex);
	uint32_t size = 0;
	if (level > count){
		int j;
		for (j=0;j<level;j++){
			size += max(w, 8) * h * sizeof(uint32_t);
			w /= 2;
			h /= 2;
		}
		SceUID data_UID;
		void *texture_data = gpu_alloc_map(
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
			SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
			size, &data_UID);
		sceGxmColorSurfaceInit(&tex->gxm_sfc,
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			orig_w,orig_h,orig_w,texture_data);
		memcpy(texture_data, sceGxmTextureGetData(&tex->gxm_tex), orig_w * orig_h * tex_format_to_bytespp(format));
		gpu_free_texture(tex);
		sceGxmTextureInitLinear(&tex->gxm_tex, texture_data, format, orig_w, orig_h, level);
		tex->valid = 1;
		tex->data_UID = data_UID;
		uint32_t* curPtr = (uint32_t*)texture_data;
		uint32_t curWidth = orig_w;
		uint32_t curHeight = orig_h;
		for (j=0;j<level-1;j++){
			uint32_t curSrcStride = max(curWidth, 8);
			uint32_t curDstStride = max((curWidth>>1), 8);
			uint32_t* dstPtr = curPtr + (curSrcStride * curHeight);
			sceGxmTransferDownscale(
				tex_format_to_bytespp(format) == 4 ? SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR : SCE_GXM_TRANSFER_FORMAT_U8U8U8_BGR,
				curPtr, 0, 0,
				curWidth, curHeight,
				curSrcStride * sizeof(uint32_t),
				tex_format_to_bytespp(format) == 4 ? SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR : SCE_GXM_TRANSFER_FORMAT_U8U8U8_BGR,
				dstPtr, 0, 0,
				curDstStride * sizeof(uint32_t),
				NULL, SCE_GXM_TRANSFER_FRAGMENT_SYNC, NULL);
			curPtr = dstPtr;
			curWidth /= 2;
			curHeight /= 2;
		}
	}
}

void gpu_prepare_rendertarget(texture* tex){
	if (tex == NULL) return;
	uint32_t w = sceGxmTextureGetWidth(&tex->gxm_tex);
	uint32_t h = sceGxmTextureGetHeight(&tex->gxm_tex);
		
	const uint32_t alignedWidth = ALIGN(w, SCE_GXM_TILE_SIZEX);
	const uint32_t alignedHeight = ALIGN(h, SCE_GXM_TILE_SIZEY);
	uint32_t sampleCount = alignedWidth*alignedHeight;
	uint32_t depthStrideInSamples = alignedWidth;
	
	void *depthBufferData = gpu_alloc_map(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
		SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE,
		4*sampleCount, &tex->depth_UID);
		
	sceGxmDepthStencilSurfaceInit(
		&tex->gxm_sfd,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		depthStrideInSamples,
		depthBufferData, NULL);
	
	SceGxmRenderTarget *tgt = NULL;
	SceGxmRenderTargetParams renderTargetParams;
	memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
	renderTargetParams.flags = 0;
	renderTargetParams.width = w;
	renderTargetParams.height = h;
	renderTargetParams.scenesPerFrame = 1;
	renderTargetParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
	renderTargetParams.multisampleLocations = 0;
	renderTargetParams.driverMemBlock = -1;
	sceGxmCreateRenderTarget(&renderTargetParams, &tgt);
	tex->gxm_rtgt = tgt;
}

void gpu_destroy_rendertarget(texture* tex){
	sceGxmDestroyRenderTarget(tex->gxm_rtgt);
	gpu_unmap_free(tex->depth_UID);
}

void gpu_free_palette(palette* pal){
	if (pal == NULL) return;
	gpu_unmap_free(pal->palette_UID);
	free(pal);
}