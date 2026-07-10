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

GLboolean has_cached_mem = GL_FALSE; // Flag for whether to use cached memory for mempools or not

#ifndef HAVE_CUSTOM_HEAP
static void *mempool_mspace[VGL_MEM_ALL - 1] = {}; // mspace creations
#endif
static void *mempool_addr[VGL_MEM_ALL] = {}; // addresses of heap memblocks
static void *mempool_end[VGL_MEM_ALL] = {}; // addresses of heap memblocks end
static size_t mempool_size[VGL_MEM_ALL] = {}; // sizes of heap memblocks

GLboolean vgl_has_cdlg_support = GL_TRUE;

#ifdef HAVE_WRAPPED_ALLOCATORS
void *__real_calloc(uint32_t nmember, uint32_t size);
void __real_free(void *addr);
void *__real_malloc(uint32_t size);
void *__real_memalign(uint32_t alignment, uint32_t size);
void *__real_realloc(void *ptr, uint32_t size);
#endif

#ifdef HAVE_CUSTOM_HEAP
SceKernelLwMutexWork tm_mutexes[VGL_MEM_ALL - 1] = {};

typedef struct tm_block_s {
	struct tm_block_s *next; // next block in list (either free or allocated)
	struct tm_block_s *prev; // prev block in list (used only during free)
	int32_t type; // one of vglMemType
	uintptr_t base; // block start address
	uint32_t offset; // offset for USSE stuff (unused)
	uint32_t size; // block size
} tm_block_t;

static tm_block_t *tm_alloclist[VGL_MEM_ALL - 1]; // list of allocated blocks
static tm_block_t *tm_freelist[VGL_MEM_ALL - 1]; // list of free blocks
static uint32_t tm_free[VGL_MEM_ALL - 1]; // see enum vglMemType

// get new block header
static inline tm_block_t *heap_blk_new(void) {
	return calloc(1, sizeof(tm_block_t));
}

// release block header
static inline void heap_blk_release(tm_block_t *block) {
	free(block);
}

// determine if two blocks can be merged into one
// blocks of different types can't be merged,
// blocks of same type can only be merged if they're next to each other
// in memory and have matching offsets
static inline int heap_blk_mergeable(tm_block_t *a, tm_block_t *b) {
	return a->type == b->type
		&& a->base + a->size == b->base
		&& a->offset + a->size == b->offset;
}

// inserts a block into the free list and merges with neighboring
// free blocks if possible
static void heap_blk_insert_free(tm_block_t *block) {
	tm_block_t *curblk = tm_freelist[block->type];
	tm_block_t *prevblk = NULL;
	while (curblk && curblk->base < block->base) {
		prevblk = curblk;
		curblk = curblk->next;
	}

	if (prevblk)
		prevblk->next = block;
	else
		tm_freelist[block->type] = block;

	block->next = curblk;
	tm_free[block->type] += block->size;

	if (curblk && heap_blk_mergeable(block, curblk)) {
		block->size += curblk->size;
		block->next = curblk->next;
		heap_blk_release(curblk);
	}

	if (prevblk && heap_blk_mergeable(prevblk, block)) {
		prevblk->size += block->size;
		prevblk->next = block->next;
		heap_blk_release(block);
	}
}

// allocates a block from the heap
static tm_block_t *heap_blk_alloc(int32_t type, uint32_t size, uint32_t alignment) {
	tm_block_t *curblk = tm_freelist[type];
	tm_block_t *prevblk = NULL;

	while (curblk) {
		const uint32_t skip = VGL_ALIGN(curblk->base + 4, alignment) - 4 - curblk->base;

		if (skip + size <= curblk->size) {
			tm_block_t *skipblk = NULL;
			tm_block_t *unusedblk = NULL;

			if (skip != 0) {
				skipblk = heap_blk_new();
				if (!skipblk)
					return NULL;
			}

			if (skip + size != curblk->size) {
				unusedblk = heap_blk_new();
				if (!unusedblk) {
					if (skipblk)
						heap_blk_release(skipblk);
					return NULL;
				}
			}

			if (skip != 0) {
				if (prevblk)
					prevblk->next = skipblk;
				else
					tm_freelist[type] = skipblk;

				skipblk->next = curblk;
				skipblk->type = curblk->type;
				skipblk->base = curblk->base;
				skipblk->offset = curblk->offset;
				skipblk->size = skip;

				curblk->base += skip;
				curblk->offset += skip;
				curblk->size -= skip;

				prevblk = skipblk;
			}

			if (size != curblk->size) {
				unusedblk->next = curblk->next;
				curblk->next = unusedblk;
				unusedblk->type = curblk->type;
				unusedblk->base = curblk->base + size;
				unusedblk->offset = curblk->offset + size;
				unusedblk->size = curblk->size - size;
				curblk->size = size;
			}

			if (prevblk)
				prevblk->next = curblk->next;
			else
				tm_freelist[type] = curblk->next;

			curblk->next = tm_alloclist[type];
			if (tm_alloclist[type])
				tm_alloclist[type]->prev = curblk;
			tm_alloclist[type] = curblk;
			tm_free[type] -= size;
			return curblk;
		}

		prevblk = curblk;
		curblk = curblk->next;
	}

	return NULL;
}

// frees a previously allocated heap block
static void heap_blk_free(uintptr_t base, vglMemType type) {
	tm_block_t **slot = (tm_block_t **)(base - 4);
	tm_block_t *block = *slot;
	
	sceKernelLockLwMutex(&tm_mutexes[type], 1, NULL);

#ifndef SKIP_ERROR_HANDLING
	if (!block || block->base + 4 != base) {
		sceKernelUnlockLwMutex(&tm_mutexes[type], 1);
		vgl_log("%s:%d A heap overflow or double free was detected on pointer: 0x%08X!\n", __FILE__, __LINE__, base);
		return;
	}
#endif
	*slot = NULL;
	
	if (block->prev)
		block->prev->next = block->next;
	else
		tm_alloclist[type] = block->next;
	if (block->next)
		block->next->prev = block->prev;

	block->next = NULL;
	block->prev = NULL;

	heap_blk_insert_free(block);
	sceKernelUnlockLwMutex(&tm_mutexes[type], 1);
}

// initializes heap variables and blockpool
static void heap_init(void) {
	for (int i = 0; i < VGL_MEM_ALL - 1; i++) {
		tm_alloclist[i] = NULL;
		tm_freelist[i] = NULL;
		tm_free[i] = 0;
	}
}

// adds a memblock to the heap
static void heap_extend(int32_t type, void *base, uint32_t size) {
	tm_block_t *block = heap_blk_new();
	block->next = NULL;
	block->type = type;
	block->base = (uintptr_t)base;
	block->offset = 0;
	block->size = size;
	heap_blk_insert_free(block);
}

// allocates memory from the heap (basically malloc())
static void *heap_alloc(int32_t type, uint32_t size, uint32_t alignment) {
	sceKernelLockLwMutex(&tm_mutexes[type], 1, NULL);
	tm_block_t *block = heap_blk_alloc(type, size + 4, alignment);
	sceKernelUnlockLwMutex(&tm_mutexes[type], 1);

	if (!block)
		return NULL;

	*(tm_block_t **)block->base = block;
	return (void *)(block->base + 4);
}
#endif

#ifdef PHYCONT_ON_DEMAND
void *vgl_alloc_phycont_block(uint32_t size) {
	size = VGL_ALIGN(size, 1024 * 1024);
	SceUID blk = sceKernelAllocMemBlock("phycont_blk", has_cached_mem ? SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW : SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, size, NULL);

	if (blk < 0)
		return NULL;

	void *res;
	sceKernelGetMemBlockBase(blk, &res);
	sceGxmMapMemory(res, size, SCE_GXM_MEMORY_ATTRIB_RW);

	return res;
}
#endif

void vgl_mem_init(size_t size_ram, size_t size_cdram, size_t size_phycont, size_t size_cdlg) {
	vgl_has_cdlg_support = GL_TRUE;

	if (!has_cached_mem && size_ram > 0xC800000) // Vita has a smaller address mapping for uncached mem
		size_ram = 0xC800000;

	mempool_size[VGL_MEM_VRAM] = VGL_ALIGN(size_cdram, 256 * 1024);
	mempool_size[VGL_MEM_RAM] = VGL_ALIGN(size_ram, 4 * 1024);
#ifdef PHYCONT_ON_DEMAND
	mempool_size[VGL_MEM_SLOW] = 0;
#else
	mempool_size[VGL_MEM_SLOW] = VGL_ALIGN(size_phycont, 1024 * 1024);
#endif
	mempool_size[VGL_MEM_BUDGET] = VGL_ALIGN(size_cdlg, 4 * 1024);

#ifdef HAVE_CUSTOM_HEAP
	// Initialize heap
	heap_init();
#endif

	SceUID mempool_id[VGL_MEM_ALL - 1] = {}; // UIDs of heap memblocks
	if (mempool_size[VGL_MEM_VRAM]) {
		mempool_id[VGL_MEM_VRAM] = sceKernelAllocMemBlock("cdram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, mempool_size[VGL_MEM_VRAM], NULL);
#ifdef HAVE_CUSTOM_HEAP
		sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_VRAM], "cdram mutex alloc", 0, 0, NULL);
#endif
	}
	if (has_cached_mem) {
		if (mempool_size[VGL_MEM_RAM]) {
			mempool_id[VGL_MEM_RAM] = sceKernelAllocMemBlock("ram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, mempool_size[VGL_MEM_RAM], NULL);
#ifdef HAVE_CUSTOM_HEAP
			sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_RAM], "ram mutex alloc", 0, 0, NULL);
#endif
		}
		if (mempool_size[VGL_MEM_SLOW]) {
			mempool_id[VGL_MEM_SLOW] = sceKernelAllocMemBlock("phycont_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW, mempool_size[VGL_MEM_SLOW], NULL);
#ifdef HAVE_CUSTOM_HEAP
			sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_SLOW], "phycont mutex alloc", 0, 0, NULL);
#endif
		}
		if (mempool_size[VGL_MEM_BUDGET]) {
			mempool_id[VGL_MEM_BUDGET] = sceKernelAllocMemBlock("cdlg_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_RW, mempool_size[VGL_MEM_BUDGET], NULL);
			vgl_has_cdlg_support = GL_FALSE;
#ifdef HAVE_CUSTOM_HEAP
			sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_BUDGET], "cdlg mutex alloc", 0, 0, NULL);
#endif
		}
	} else {
		if (mempool_size[VGL_MEM_RAM]) {
			mempool_id[VGL_MEM_RAM] = sceKernelAllocMemBlock("ram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, mempool_size[VGL_MEM_RAM], NULL);
#ifdef HAVE_CUSTOM_HEAP
			sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_RAM], "ram mutex alloc", 0, 0, NULL);
#endif
		}
		if (mempool_size[VGL_MEM_SLOW]) {
			mempool_id[VGL_MEM_SLOW] = sceKernelAllocMemBlock("phycont_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, mempool_size[VGL_MEM_SLOW], NULL);
#ifdef HAVE_CUSTOM_HEAP
			sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_SLOW], "phycont mutex alloc", 0, 0, NULL);
#endif
		}
		if (mempool_size[VGL_MEM_BUDGET]) {
			mempool_id[VGL_MEM_BUDGET] = sceKernelAllocMemBlock("cdlg_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_NC_RW, mempool_size[VGL_MEM_BUDGET], NULL);
			vgl_has_cdlg_support = GL_FALSE;
#ifdef HAVE_CUSTOM_HEAP
			sceKernelCreateLwMutex(&tm_mutexes[VGL_MEM_BUDGET], "cdlg mutex alloc", 0, 0, NULL);
#endif
		}
	}
	for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
		if (mempool_size[i]) {
			mempool_addr[i] = NULL;
			sceKernelGetMemBlockBase(mempool_id[i], &mempool_addr[i]);

			if (mempool_addr[i]) {
				mempool_end[i] = (void *)((uintptr_t)mempool_addr[i] + mempool_size[i]);
				sceGxmMapMemory(mempool_addr[i], mempool_size[i], SCE_GXM_MEMORY_ATTRIB_RW);
#ifndef HAVE_CUSTOM_HEAP
				mempool_mspace[i] = sceClibMspaceCreate(mempool_addr[i], mempool_size[i]);
#else
				heap_extend(i, mempool_addr[i], mempool_size[i]);
#endif
			}
		}
	}

#ifdef PHYCONT_ON_DEMAND
	// Getting total available phycont mem
	uint32_t phycont_size;
	if (system_app_mode) {
		SceAppMgrBudgetInfo info;
		info.size = sizeof(SceAppMgrBudgetInfo);
		sceAppMgrGetBudgetInfo(&info);
		phycont_size = info.total_phycont_mem;
	} else {
		SceKernelFreeMemorySizeInfo info;
		info.size = sizeof(SceKernelFreeMemorySizeInfo);
		sceKernelGetFreeMemorySize(&info);
		phycont_size = info.size_phycont;
	}
	mempool_size[VGL_MEM_SLOW] = phycont_size;
#endif

	// Mapping newlib heap into sceGxm
	void *dummy = malloc(1);
	free(dummy);

	SceKernelMemBlockInfo info;
	info.size = sizeof(SceKernelMemBlockInfo);
	sceKernelGetMemBlockInfoByAddr(dummy, &info);
	sceGxmMapMemory(info.mappedBase, info.mappedSize, SCE_GXM_MEMORY_ATTRIB_RW);

	mempool_size[VGL_MEM_EXTERNAL] = info.mappedSize;
	mempool_addr[VGL_MEM_EXTERNAL] = info.mappedBase;
	mempool_end[VGL_MEM_EXTERNAL] = (void *)((uintptr_t)mempool_addr[VGL_MEM_EXTERNAL] + mempool_size[VGL_MEM_EXTERNAL]);
}

vglMemType vgl_mem_get_type_by_addr(void *addr) {
	if (addr >= mempool_addr[VGL_MEM_VRAM] && addr < mempool_end[VGL_MEM_VRAM])
		return VGL_MEM_VRAM;
	else if (addr >= mempool_addr[VGL_MEM_RAM] && addr < mempool_end[VGL_MEM_RAM])
		return VGL_MEM_RAM;
#ifndef PHYCONT_ON_DEMAND
	else if (addr >= mempool_addr[VGL_MEM_SLOW] && addr < mempool_end[VGL_MEM_SLOW])
		return VGL_MEM_SLOW;
#endif
	else if (addr >= mempool_addr[VGL_MEM_BUDGET] && addr < mempool_end[VGL_MEM_BUDGET])
		return VGL_MEM_BUDGET;
	else if (addr >= mempool_addr[VGL_MEM_EXTERNAL] && addr < mempool_end[VGL_MEM_EXTERNAL])
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
#ifdef PHYCONT_ON_DEMAND
	} else if (type == VGL_MEM_SLOW) {
		if (system_app_mode) {
			SceAppMgrBudgetInfo info;
			info.size = sizeof(SceAppMgrBudgetInfo);
			sceAppMgrGetBudgetInfo(&info);
			return info.free_phycont_mem;
		} else {
			SceKernelFreeMemorySizeInfo info;
			info.size = sizeof(SceKernelFreeMemorySizeInfo);
			sceKernelGetFreeMemorySize(&info);
			return info.size_phycont;
		}
#endif
	} else if (type == VGL_MEM_ALL) {
		size_t size = 0;
		for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
			size += vgl_mem_get_free_space(i);
		}
		return size;
#ifdef HAVE_CUSTOM_HEAP
	} else {
		return tm_free[type];
	}
#else
	} else if (mempool_size[type]) {
		SceClibMspaceStats stats;
		sceClibMspaceMallocStats(mempool_mspace[type], &stats);
		return stats.capacity - stats.current_in_use;
	} else
		return 0;
#endif
}

size_t vgl_mem_get_total_space(vglMemType type) {
	if (type == VGL_MEM_ALL) {
		size_t size = 0;
		for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
			size += vgl_mem_get_total_space(i);
		}
		return size;
	} else {
		return mempool_size[type];
	}
}

size_t vgl_malloc_usable_size(void *ptr) {
	vglMemType type = vgl_mem_get_type_by_addr(ptr);
	if (type == VGL_MEM_EXTERNAL)
		return malloc_usable_size(ptr);
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		SceKernelMemBlockInfo info;
		info.size = sizeof(SceKernelMemBlockInfo);
		sceKernelGetMemBlockInfoByAddr(ptr, &info);
		return info.mappedSize;
	}
#endif
#ifdef HAVE_CUSTOM_HEAP
	tm_block_t **hdr = (tm_block_t **)((uintptr_t)ptr - 4);
	tm_block_t *block = *hdr;
	return block->size - 4;
#else
	return sceClibMspaceMallocUsableSize(ptr);
#endif
}

void vgl_free(void *ptr) {
	vglMemType type = vgl_mem_get_type_by_addr(ptr);
	if (type == VGL_MEM_EXTERNAL)
#ifdef HAVE_WRAPPED_ALLOCATORS
		__real_free(ptr);
#else
		free(ptr);
#endif
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		sceGxmUnmapMemory(ptr);
		sceKernelFreeMemBlock(sceKernelFindMemBlockByAddr(ptr, 0));
	}
#endif
#ifdef HAVE_CUSTOM_HEAP
	else
		heap_blk_free((uintptr_t)ptr, type);
#else
	else if (mempool_mspace[type])
		sceClibMspaceFree(mempool_mspace[type], ptr);
#endif
}

void *vgl_malloc(size_t size, vglMemType type) {
	if (type == VGL_MEM_EXTERNAL)
#ifdef HAVE_WRAPPED_ALLOCATORS
		return __real_malloc(size);
#else
		return malloc(size);
#endif
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW)
		return vgl_alloc_phycont_block(size);
#endif
#ifdef HAVE_CUSTOM_HEAP
	else if (size <= tm_free[type])
		return heap_alloc(type, size, MEM_ALIGNMENT);
#else
	else if (mempool_mspace[type])
		return sceClibMspaceMalloc(mempool_mspace[type], size);
#endif
	return NULL;
}

void *vgl_calloc(size_t num, size_t size, vglMemType type) {
	if (type == VGL_MEM_EXTERNAL)
#ifdef HAVE_WRAPPED_ALLOCATORS
		return __real_calloc(num, size);
#else
		return calloc(num, size);
#endif
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		void *ret = vgl_alloc_phycont_block(num * size);
		if (ret) {
			sceClibMemset(ret, 0, num * size);
		}
		return ret;
	}
#endif
#ifdef HAVE_CUSTOM_HEAP
	else if (num * size <= tm_free[type]) {
		void *ret = heap_alloc(type, num * size, MEM_ALIGNMENT);
		if (ret) {
			sceClibMemset(ret, 0, num * size);
		}
		return ret;
	}
#else
	else if (mempool_mspace[type])
		return sceClibMspaceCalloc(mempool_mspace[type], num, size);
#endif
	return NULL;
}

void *vgl_memalign(size_t alignment, size_t size, vglMemType type) {
	if (type == VGL_MEM_EXTERNAL)
#ifdef HAVE_WRAPPED_ALLOCATORS
		return __real_memalign(alignment, size);
#else
		return memalign(alignment, size);
#endif
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW)
		return vgl_alloc_phycont_block(size);
#endif
#ifdef HAVE_CUSTOM_HEAP
	else if (size <= tm_free[type])
		return heap_alloc(type, size, alignment);
#else
	else if (mempool_mspace[type])
		return sceClibMspaceMemalign(mempool_mspace[type], alignment, size);
#endif
	return NULL;
}

void *vgl_realloc(void *ptr, size_t size) {
	vglMemType type = vgl_mem_get_type_by_addr(ptr);
	if (type == VGL_MEM_EXTERNAL)
#ifdef HAVE_WRAPPED_ALLOCATORS
		return __real_realloc(ptr, size);
#else
		return realloc(ptr, size);
#endif
#ifdef PHYCONT_ON_DEMAND
	else if (type == VGL_MEM_SLOW) {
		size_t old_size = vgl_malloc_usable_size(ptr);
		if (old_size >= size)
			return ptr;
		void *res = vgl_alloc_phycont_block(size);
		if (res) {
			vgl_fast_memcpy(res, ptr, old_size);
			vgl_free(ptr);
			return res;
		}
	}
#endif
#ifndef HAVE_CUSTOM_HEAP
	else if (mempool_mspace[type])
		return sceClibMspaceRealloc(mempool_mspace[type], ptr, size);
#endif
	return NULL;
}
