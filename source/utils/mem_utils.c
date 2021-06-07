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
 * mem_utils.c:
 * Utilities for memory management
 */

#include "../shared.h"

static void *mempool_mspace[4] = {NULL, NULL, NULL, NULL}; // mspace creations (VRAM, RAM, PHYCONT RAM, EXTERNAL)
static void *mempool_addr[4] = {NULL, NULL, NULL, NULL}; // addresses of heap memblocks (VRAM, RAM, PHYCONT RAM, EXTERNAL)
static SceUID mempool_id[4] = {0, 0, 0, 0}; // UIDs of heap memblocks (VRAM, RAM, PHYCONT RAM, EXTERNAL)
static size_t mempool_size[4] = {0, 0, 0, 0}; // sizes of heap memlbocks (VRAM, RAM, PHYCONT RAM, EXTERNAL)

static int mempool_initialized = 0;

#ifdef PHYCONT_ON_DEMAND
void *vgl_alloc_phycont_block(uint32_t size) {
	size = ALIGN(size, 1024 * 1024);
	SceUID blk = sceKernelAllocMemBlock("phycont_blk", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, size, NULL);
	
	if (blk < 0)
		return NULL;
	
	void *res;
	sceKernelGetMemBlockBase(blk, &res);
	sceGxmMapMemory(res, size, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
	
	return res;
}
#endif

void vgl_mem_term(void) {
	if (!mempool_initialized)
		return;

	for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
		sceClibMspaceDestroy(mempool_mspace[i]);
		sceKernelFreeMemBlock(mempool_id[i]);
		mempool_mspace[i] = NULL;
		mempool_addr[i] = NULL;
		mempool_id[i] = 0;
		mempool_size[i] = 0;
	}

	mempool_initialized = 0;
}

void vgl_mem_init(size_t size_ram, size_t size_cdram, size_t size_phycont) {
	if (mempool_initialized)
		vgl_mem_term();

	if (size_ram > 0xC800000) // Vita limits memblocks size to a max of approx. 200 MBs apparently
		size_ram = 0xC800000;

	mempool_size[VGL_MEM_VRAM] = ALIGN(size_cdram, 256 * 1024);
	mempool_size[VGL_MEM_RAM] = ALIGN(size_ram, 4 * 1024);
#ifdef PHYCONT_ON_DEMAND
	mempool_size[VGL_MEM_SLOW] = 0;
#else
	mempool_size[VGL_MEM_SLOW] = ALIGN(size_phycont, 1024 * 1024);
#endif

	if (mempool_size[VGL_MEM_VRAM])
		mempool_id[VGL_MEM_VRAM] = sceKernelAllocMemBlock("cdram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, mempool_size[VGL_MEM_VRAM], NULL);
	if (mempool_size[VGL_MEM_RAM])
		mempool_id[VGL_MEM_RAM] = sceKernelAllocMemBlock("ram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, mempool_size[VGL_MEM_RAM], NULL);
	if (mempool_size[VGL_MEM_SLOW])
		mempool_id[VGL_MEM_SLOW] = sceKernelAllocMemBlock("phycont_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, mempool_size[VGL_MEM_SLOW], NULL);

	for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
		if (mempool_size[i]) {
			mempool_addr[i] = NULL;
			sceKernelGetMemBlockBase(mempool_id[i], &mempool_addr[i]);

			if (mempool_addr[i]) {
				sceGxmMapMemory(mempool_addr[i], mempool_size[i], SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
				mempool_mspace[i] = sceClibMspaceCreate(mempool_addr[i], mempool_size[i]);
			}
		}
	}

	// Mapping newlib heap into sceGxm
	void *dummy = malloc(1);
	free(dummy);

	SceKernelMemBlockInfo info;
	info.size = sizeof(SceKernelMemBlockInfo);
	sceKernelGetMemBlockInfoByAddr(dummy, &info);
	sceGxmMapMemory(info.mappedBase, info.mappedSize, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);

	mempool_size[VGL_MEM_EXTERNAL] = info.mappedSize;
	mempool_addr[VGL_MEM_EXTERNAL] = info.mappedBase;

	mempool_initialized = 1;
}

vglMemType vgl_mem_get_type_by_addr(void *addr) {
	if (addr >= mempool_addr[VGL_MEM_VRAM] && (addr < mempool_addr[VGL_MEM_VRAM] + mempool_size[VGL_MEM_VRAM]))
		return VGL_MEM_VRAM;
	else if (addr >= mempool_addr[VGL_MEM_RAM] && (addr < mempool_addr[VGL_MEM_RAM] + mempool_size[VGL_MEM_RAM]))
		return VGL_MEM_RAM;
#ifndef PHYCONT_ON_DEMAND
	else if (addr >= mempool_addr[VGL_MEM_SLOW] && (addr < mempool_addr[VGL_MEM_SLOW] + mempool_size[VGL_MEM_SLOW]))
		return VGL_MEM_SLOW;
#endif
	else if (addr >= mempool_addr[VGL_MEM_EXTERNAL] && (addr < mempool_addr[VGL_MEM_EXTERNAL] + mempool_size[VGL_MEM_EXTERNAL]))
		return VGL_MEM_EXTERNAL;
#ifdef PHYCONT_ON_DEMAND
	return VGL_MEM_SLOW;
#else
	return -1;
#endif
}

size_t vgl_mem_get_free_space(vglMemType type) {
	if (type == VGL_MEM_EXTERNAL) {
		return 0;
	} else if (type == VGL_MEM_ALL) {
		size_t size = 0;
		for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
			SceClibMspaceStats stats;
			sceClibMspaceMallocStats(mempool_mspace[i], &stats);
			size += stats.capacity - stats.current_in_use;
		}
		return size;
	} else {
		SceClibMspaceStats stats;
		sceClibMspaceMallocStats(mempool_mspace[type], &stats);
		return stats.capacity - stats.current_in_use;
	}
}

size_t vgl_malloc_usable_size(void *ptr) {
	vglMemType type = vgl_mem_get_type_by_addr(ptr);
	if (type == VGL_MEM_EXTERNAL)
		return 0;
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		SceKernelMemBlockInfo info;
		info.size = sizeof(SceKernelMemBlockInfo);
		sceKernelGetMemBlockInfoByAddr(ptr, &info);
		return info.mappedSize;
	}
#endif
	else
		return sceClibMspaceMallocUsableSize(ptr);
}

void vgl_free(void *ptr) {
	vglMemType type = vgl_mem_get_type_by_addr(ptr);
	if (type == VGL_MEM_EXTERNAL)
		free(ptr);
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		sceGxmUnmapMemory(ptr);
		sceKernelFreeMemBlock(sceKernelFindMemBlockByAddr(ptr, 0));
	}
#endif
	else if (mempool_mspace[type])
		sceClibMspaceFree(mempool_mspace[type], ptr);
}

void *vgl_malloc(size_t size, vglMemType type) {
	if (type == VGL_MEM_EXTERNAL)
		return malloc(size);
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW)
		return vgl_alloc_phycont_block(size);
#endif
	else if (mempool_mspace[type])
		return sceClibMspaceMalloc(mempool_mspace[type], size);
	return NULL;
}

void *vgl_calloc(size_t num, size_t size, vglMemType type) {
	if (type == VGL_MEM_EXTERNAL)
		return calloc(num, size);
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW)
		return vgl_alloc_phycont_block(num * size);
#endif
	else if (mempool_mspace[type])
		return sceClibMspaceCalloc(mempool_mspace[type], num, size);
	return NULL;
}

void *vgl_memalign(size_t alignment, size_t size, vglMemType type) {
	if (type == VGL_MEM_EXTERNAL)
		return memalign(alignment, size);
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW)
		return vgl_alloc_phycont_block(size);
#endif
	else if (mempool_mspace[type])
		return sceClibMspaceMemalign(mempool_mspace[type], alignment, size);
	return NULL;
}

void *vgl_realloc(void *ptr, size_t size) {
	vglMemType type = vgl_mem_get_type_by_addr(ptr);
	if (type == VGL_MEM_EXTERNAL)
		return realloc(ptr, size);
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		if (vgl_malloc_usable_size(ptr) >= size)
			return ptr;
		void *res = vgl_alloc_phycont_block(size);
		if (res) {
			sceClibMemcpy(res, ptr, size);
			vgl_free(ptr);
			return res;
		}
	}
#endif
	else if (mempool_mspace[type])
		return sceClibMspaceRealloc(mempool_mspace[type], ptr, size);
	return NULL;
}
