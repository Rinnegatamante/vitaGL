/* 
 * mem_utils.h:
 * Header file for the memory management utilities exposed by mem_utils.c
 */
 
#ifndef _MEM_UTILS_H_
#define _MEM_UTILS_H_

typedef enum mem_type{
	VRAM_MEMORY = 0,
	RAM_MEMORY = 1
} mem_type;

int mem_init(size_t size_ram, size_t size_cdram); // Initialize both CDRAM and RAM mempools
void mem_term(void); // Terminate both CDRAM and RAM mempools
size_t mempool_get_free_space(mem_type type); // Return free space in bytes for a mempool
void mempool_reset(void); // Resets both CDRAM and RAM mempools to initial state
void* mempool_alloc(size_t size, mem_type type); // Allocate a memory block on a mempool
void mempool_free(void* ptr, mem_type type); // Free a memory block on a mempool

#endif