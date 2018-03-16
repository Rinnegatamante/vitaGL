/* 
 * mem_utils.c:
 * Utilities for memory management
 */
 
#include "../shared.h"

static void *mempool_addr[2] = {NULL, NULL};
static SceUID mempool_id[2] = {0, 0};
static size_t mempool_size[2] = {0, 0};
static size_t mempool_free_size[2] = {0, 0};

// memblock header struct
typedef struct mempool_block_hdr{
	uint8_t used;
	size_t size;
} mempool_block_hdr;

// Allocks a new block on mempool
static void *_mempool_alloc_block(size_t size, mem_type type){
	size_t i = 0;
	mempool_block_hdr* hdr = (mempool_block_hdr*)mempool_addr[type];
	
	// Checking for a big enough free memblock
	while (i < mempool_size[type]){
		if (!hdr->used){
			if (hdr->size >= size){
				
				// Reserving memory
				hdr->used = 1;
				size_t old_size = hdr->size;
				hdr->size = size;
				
				// Splitting blocks
				mempool_block_hdr *new_hdr = (mempool_block_hdr*)(mempool_addr[type] + i + sizeof(mempool_block_hdr) + size);
				new_hdr->used = 0;
				new_hdr->size = old_size - (size + sizeof(mempool_block_hdr));
				
				mempool_free_size[type] -= (sizeof(mempool_block_hdr) + size);
				return (void*)(mempool_addr[type] + i + sizeof(mempool_block_hdr));
			}
		}
		
		// Jumping to next block
		i += (hdr->size + sizeof(mempool_block_hdr));
		hdr = (mempool_block_hdr*)(mempool_addr[type] + i);
		
	}
	return NULL;
}

// Frees a block on mempool
static void _mempool_free_block(void* ptr, mem_type type){
	mempool_block_hdr* hdr = (mempool_block_hdr*)(ptr - sizeof(mempool_block_hdr));
	hdr->used = 0;
	mempool_free_size[type] += hdr->size;
}

// Merge contiguous free blocks in a bigger one
static void _mempool_merge_blocks(mem_type type){
	size_t i = 0;
	mempool_block_hdr* hdr = (mempool_block_hdr*)mempool_addr[type];
	mempool_block_hdr* previousBlock = NULL;
	
	while (i < mempool_size[type]){
		if (!hdr->used){
			if (previousBlock != NULL){
				previousBlock->size += (hdr->size + sizeof(mempool_block_hdr));
				mempool_free_size[type] += sizeof(mempool_block_hdr); 
			}else{
				previousBlock = hdr;
			}
		}else{
			previousBlock = NULL;
		}
		
		// Jumping to next block
		i += hdr->size + sizeof(mempool_block_hdr);
		hdr = (mempool_block_hdr*)(mempool_addr[type] + i);
		
	}
}

void mempool_reset(void){
	int i;
	for (i = 0; i < 2; i++){
		if (mempool_addr[i] != NULL){
			mempool_block_hdr* master_block = (mempool_block_hdr*)mempool_addr[i];
			master_block->used = 0;
			master_block->size = mempool_size[i] - sizeof(mempool_block_hdr);
			mempool_free_size[i] = master_block->size;
		}
	}
}

void mem_term(void){
	if (mempool_addr[0] != NULL){
		sceKernelFreeMemBlock(mempool_id[0]);
		sceKernelFreeMemBlock(mempool_id[1]);
		mempool_addr[0] = NULL;
	}
}

int mem_init(size_t size_ram, size_t size_cdram){
	
	if (mempool_addr[0] != NULL) mem_term();
	
	mempool_id[0] = sceKernelAllocMemBlock("cdram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, ALIGN(size_cdram, 256 * 1024), NULL);
	mempool_id[1] = sceKernelAllocMemBlock("ram_mempool", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, ALIGN(size_ram, 4 * 1024), NULL);
	mempool_size[0] = size_cdram;
	mempool_size[1] = size_ram;
	
	int i;
	for (i = 0; i < 2; i++){
		sceKernelGetMemBlockBase(mempool_id[i], &mempool_addr[i]);
		sceGxmMapMemory(mempool_addr[i], mempool_size[i], SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
	}

	// Initializing mempool as a single block
	mempool_reset();

	return 1;
}

void mempool_free(void* ptr, mem_type type){
	_mempool_free_block(ptr, type);
	_mempool_merge_blocks(type);
}

void *mempool_alloc(size_t size, mem_type type){
	void* res = NULL;
	if (size <= mempool_free_size[type]) res = _mempool_alloc_block(size, type);
	return res;
}

// Returns currently free space on mempool
size_t mempool_get_free_space(mem_type type){
	return mempool_free_size[type];
}