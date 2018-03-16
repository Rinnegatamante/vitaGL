/* 
 * mem_utils.h:
 * Header file for the memory management utilities exposed by mem_utils.c
 */
 
#ifndef _MEM_UTILS_H_
#define _MEM_UTILS_H_

int mem_init(size_t size_ram, size_t size_cdram); // Initialize both CDRAM and RAM mempools
void mem_term(void); // Terminate both CDRAM and RAM mempools
size_t mempool_get_free_space(VGLmemtype type); // Return free space in bytes for a mempool
void* mempool_alloc(size_t size, VGLmemtype type); // Allocate a memory block on a mempool
void mempool_free(void* ptr, VGLmemtype type); // Free a memory block on a mempool

#endif