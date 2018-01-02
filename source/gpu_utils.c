#include <vitasdk.h>
#include "gpu_utils.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

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