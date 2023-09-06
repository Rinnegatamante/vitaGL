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

#define HEAP_COOKIE 0x13371337

GLboolean has_cached_mem = GL_FALSE; // Flag for wether to use cached memory for mempools or not

#ifndef HAVE_CUSTOM_HEAP
static void *mempool_mspace[VGL_MEM_ALL] = {NULL, NULL, NULL, NULL, NULL}; // mspace creations (VRAM, RAM, PHYCONT RAM, CDLG, EXTERNAL)
#endif
static void *mempool_addr[VGL_MEM_ALL] = {NULL, NULL, NULL, NULL, NULL}; // addresses of heap memblocks (VRAM, RAM, PHYCONT RAM, CDLG, EXTERNAL)
static SceUID mempool_id[VGL_MEM_ALL] = {0, 0, 0, 0, 0}; // UIDs of heap memblocks (VRAM, RAM, PHYCONT RAM, EXTERNAL)
static size_t mempool_size[VGL_MEM_ALL] = {0, 0, 0, 0, 0}; // sizes of heap memlbocks (VRAM, RAM, PHYCONT RAM, EXTERNAL)

static int mempool_initialized = GL_FALSE;

#ifdef HAVE_WRAPPED_ALLOCATORS
void *__real_calloc(uint32_t nmember, uint32_t size);
void __real_free(void *addr);
void *__real_malloc(uint32_t size);
void *__real_memalign(uint32_t alignment, uint32_t size);
void *__real_realloc(void *ptr, uint32_t size);
#endif

#ifdef HAVE_CUSTOM_HEAP
typedef struct tm_block_s {
	struct tm_block_s *next; // next block in list (either free or allocated)
	int32_t type; // one of vglMemType
	uintptr_t base; // block start address
	uint32_t offset; // offset for USSE stuff (unused)
	uint32_t size; // block size
#ifndef SKIP_ERROR_HANDLING
	uint32_t real_size; // real alloc size for putting a heap cookie
#endif
} tm_block_t;

static tm_block_t *tm_alloclist; // list of allocated blocks
static tm_block_t *tm_freelist; // list of free blocks

static uint32_t tm_free[VGL_MEM_ALL]; // see enum vglMemType

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
	tm_block_t *curblk = tm_freelist;
	tm_block_t *prevblk = NULL;
	while (curblk && curblk->base < block->base) {
		prevblk = curblk;
		curblk = curblk->next;
	}

	if (prevblk)
		prevblk->next = block;
	else
		tm_freelist = block;

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
// (removes it from free list and adds to alloc list)
static tm_block_t *heap_blk_alloc(int32_t type, uint32_t size, uint32_t alignment) {
	tm_block_t *curblk = tm_freelist;
	tm_block_t *prevblk = NULL;

	while (curblk) {
		const uint32_t skip = VGL_ALIGN(curblk->base, alignment) - curblk->base;

		if (curblk->type == type && skip + size <= curblk->size) {
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
					tm_freelist = skipblk;

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
				tm_freelist = curblk->next;

			curblk->next = tm_alloclist;
#ifndef SKIP_ERROR_HANDLING
			curblk->real_size = size;
#endif
			tm_alloclist = curblk;
			tm_free[type] -= size;
			return curblk;
		}

		prevblk = curblk;
		curblk = curblk->next;
	}

	return NULL;
}

// frees a previously allocated heap block
// (removes from alloc list and inserts into free list)
static void heap_blk_free(uintptr_t base) {
	tm_block_t *curblk = tm_alloclist;
	tm_block_t *prevblk = NULL;

	while (curblk && curblk->base != base) {
		prevblk = curblk;
		curblk = curblk->next;
	}

	if (!curblk) {
#ifndef SKIP_ERROR_HANDLING
		vgl_log("%s:%d An internal free failed (possible double free call) on pointer: 0x%08X!\n", __FILE__, __LINE__, base);
#endif
		return;
	}

#ifndef SKIP_ERROR_HANDLING
	if (*(uint32_t *)(curblk->base + curblk->real_size - 4) != HEAP_COOKIE) {
		vgl_log("%s:%d A heap overflow was detected on pointer: 0x%08X!\n", __FILE__, __LINE__, base);
	}
#endif

	if (prevblk)
		prevblk->next = curblk->next;
	else
		tm_alloclist = curblk->next;

	curblk->next = NULL;

	heap_blk_insert_free(curblk);
}

// initializes heap variables and blockpool
static void heap_init(void) {
	tm_alloclist = NULL;
	tm_freelist = NULL;

	for (int i = 0; i < VGL_MEM_ALL; i++)
		tm_free[i] = 0;
}

// resets heap state and frees allocated block headers
static void heap_destroy(void) {
	tm_block_t *n;

	tm_block_t *p = tm_alloclist;
	while (p) {
		n = p->next;
		heap_blk_release(p);
		p = n;
	}

	p = tm_freelist;
	while (p) {
		n = p->next;
		heap_blk_release(p);
		p = n;
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
#ifndef SKIP_ERROR_HANDLING
	size += 4;
#endif
	tm_block_t *block = heap_blk_alloc(type, size, alignment);

	if (!block)
		return NULL;

#ifndef SKIP_ERROR_HANDLING
	*(uint32_t *)(block->base + block->real_size - 4) = HEAP_COOKIE;
#endif
	return (void *)block->base;
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

void vgl_mem_term(void) {
	if (!mempool_initialized)
		return;

#ifdef HAVE_CUSTOM_HEAP
	heap_destroy();
#endif

	for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
#ifndef HAVE_CUSTOM_HEAP
		sceClibMspaceDestroy(mempool_mspace[i]);
		mempool_mspace[i] = NULL;
#endif
		sceKernelFreeMemBlock(mempool_id[i]);
		mempool_addr[i] = NULL;
		mempool_id[i] = 0;
		mempool_size[i] = 0;
	}

	mempool_initialized = 0;
}

void vgl_mem_init(size_t size_ram, size_t size_cdram, size_t size_phycont, size_t size_cdlg) {
	if (mempool_initialized)
		vgl_mem_term();

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

	if (mempool_size[VGL_MEM_VRAM])
		mempool_id[VGL_MEM_VRAM] = sceKernelAllocMemBlock("cdram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, mempool_size[VGL_MEM_VRAM], NULL);
	if (has_cached_mem) {
		if (mempool_size[VGL_MEM_RAM])
			mempool_id[VGL_MEM_RAM] = sceKernelAllocMemBlock("ram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, mempool_size[VGL_MEM_RAM], NULL);
		if (mempool_size[VGL_MEM_SLOW])
			mempool_id[VGL_MEM_SLOW] = sceKernelAllocMemBlock("phycont_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW, mempool_size[VGL_MEM_SLOW], NULL);
		if (mempool_size[VGL_MEM_BUDGET])
			mempool_id[VGL_MEM_BUDGET] = sceKernelAllocMemBlock("cdlg_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_RW, mempool_size[VGL_MEM_BUDGET], NULL);
	} else {
		if (mempool_size[VGL_MEM_RAM])
			mempool_id[VGL_MEM_RAM] = sceKernelAllocMemBlock("ram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, mempool_size[VGL_MEM_RAM], NULL);
		if (mempool_size[VGL_MEM_SLOW])
			mempool_id[VGL_MEM_SLOW] = sceKernelAllocMemBlock("phycont_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, mempool_size[VGL_MEM_SLOW], NULL);
		if (mempool_size[VGL_MEM_BUDGET])
			mempool_id[VGL_MEM_BUDGET] = sceKernelAllocMemBlock("cdlg_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_NC_RW, mempool_size[VGL_MEM_BUDGET], NULL);
	}
	for (int i = 0; i < VGL_MEM_EXTERNAL; i++) {
		if (mempool_size[i]) {
			mempool_addr[i] = NULL;
			sceKernelGetMemBlockBase(mempool_id[i], &mempool_addr[i]);

			if (mempool_addr[i]) {
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

	mempool_initialized = 1;
}

vglMemType vgl_mem_get_type_by_addr(void *addr) {
#if !defined(PHYCONT_ON_DEMAND) && defined(HAVE_CUSTOM_HEAP)
	if (addr >= mempool_addr[VGL_MEM_EXTERNAL] && (addr < mempool_addr[VGL_MEM_EXTERNAL] + mempool_size[VGL_MEM_EXTERNAL]))
		return VGL_MEM_EXTERNAL;
	return -1;
#else
	if (addr >= mempool_addr[VGL_MEM_VRAM] && (addr < (void *)((uint8_t *)mempool_addr[VGL_MEM_VRAM] + mempool_size[VGL_MEM_VRAM])))
		return VGL_MEM_VRAM;
	else if (addr >= mempool_addr[VGL_MEM_RAM] && (addr < (void *)((uint8_t *)mempool_addr[VGL_MEM_RAM] + mempool_size[VGL_MEM_RAM])))
		return VGL_MEM_RAM;
#ifndef PHYCONT_ON_DEMAND
	else if (addr >= mempool_addr[VGL_MEM_SLOW] && (addr < (void *)((uint8_t *)mempool_addr[VGL_MEM_SLOW] + mempool_size[VGL_MEM_SLOW])))
		return VGL_MEM_SLOW;
#endif
	else if (addr >= mempool_addr[VGL_MEM_BUDGET] && (addr < (void *)((uint8_t *)mempool_addr[VGL_MEM_BUDGET] + mempool_size[VGL_MEM_BUDGET])))
		return VGL_MEM_BUDGET;
	else if (addr >= mempool_addr[VGL_MEM_EXTERNAL] && (addr < (void *)((uint8_t *)mempool_addr[VGL_MEM_EXTERNAL] + mempool_size[VGL_MEM_EXTERNAL])))
		return VGL_MEM_EXTERNAL;
#endif
#ifdef PHYCONT_ON_DEMAND
	return VGL_MEM_SLOW;
#else
	return -1;
#endif
}

size_t vgl_mem_get_free_space(vglMemType type) {
	if (type == VGL_MEM_EXTERNAL) {
		return 0;
#if defined(PHYCONT_ON_DEMAND)
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
	else
		return sceClibMspaceMallocUsableSize(ptr);
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
		heap_blk_free(ptr);
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
	else if (type == VGL_MEM_SLOW)
		return vgl_alloc_phycont_block(num * size);
#endif
#ifdef HAVE_CUSTOM_HEAP
	else if (num * size <= tm_free[type])
		return heap_alloc(type, num * size, MEM_ALIGNMENT);
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
