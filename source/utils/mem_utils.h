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
 * mem_utils.h:
 * Header file for the memory management utilities exposed by mem_utils.c
 */

#ifndef _MEM_UTILS_H_
#define _MEM_UTILS_H_

#define SCE_KERNEL_MEMBLOCK_TYPE_GAME_CDLG_NC_RW 0xCA0D060
#define SCE_KERNEL_MAX_GAME_CDLG_MEM_SIZE 0x8C6000

void vgl_mem_init(size_t size_ram, size_t size_cdram, size_t size_phycont, size_t size_cdlg);
void vgl_mem_term(void);
size_t vgl_mem_get_free_space(vglMemType type);
size_t vgl_mem_get_total_space(vglMemType type);

size_t vgl_malloc_usable_size(void *ptr);
void *vgl_malloc(size_t size, vglMemType type);
void *vgl_calloc(size_t num, size_t size, vglMemType type);
void *vgl_memalign(size_t alignment, size_t size, vglMemType type);
void *vgl_realloc(void *ptr, size_t size);
void vgl_free(void *ptr);

// Helper function for fastest memory copy on uncached mem
void vgl_memcpy(void *dst, const void *src, size_t size);

#endif
