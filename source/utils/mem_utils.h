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

#define SCE_KERNEL_MAX_MAIN_CDIALOG_MEM_SIZE 0x8C6000

// Debug flags
//#define DEBUG_MEMCPY // Enable this to use newlib memcpy in order to have proper trace info in coredumps

#ifdef DEBUG_MEMCPY
#define vgl_fast_memcpy memcpy
#else
#define vgl_fast_memcpy sceClibMemcpy
#endif

extern vglMemType VGL_MEM_MAIN; // Flag for VRAM usage for allocations

// Support for older vitasdk versions for CI based on ancient builds
#ifndef SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_NC_RW
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_NC_RW 0x0CA08060
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_CDIALOG_RW 0x0CA0D060
#endif

// Garbage collector related stuffs
extern void *frame_purge_list[FRAME_PURGE_FREQ][FRAME_PURGE_LIST_SIZE]; // Purge list for internal elements
extern void *frame_rt_purge_list[FRAME_PURGE_FREQ][FRAME_PURGE_RENDERTARGETS_LIST_SIZE]; // Purge list for rendertargets
extern int frame_purge_idx; // Index for currently populatable purge list
extern int frame_elem_purge_idx; // Index for currently populatable purge list element
extern int frame_rt_purge_idx; // Index for currently populatable purge list rendertarget

// Macro to mark a pointer or a rendertarget as dirty for garbage collection
#define markAsDirty(x) frame_purge_list[frame_purge_idx][frame_elem_purge_idx++] = x
#ifdef HAVE_SHARED_RENDERTARGETS
typedef struct {
	SceGxmRenderTarget *rt;
	int w;
	int h;
	int ref_count;
	int max_refs;
#ifdef RECYCLE_RENDERTARGETS
	uint32_t last_frame;
#endif
} render_target;
void __markRtAsDirty(render_target *rt);
#define _markRtAsDirty(x) frame_rt_purge_list[frame_purge_idx][frame_rt_purge_idx++] = x
#define markRtAsDirty(x) __markRtAsDirty((render_target *)x)
#else
#define markRtAsDirty(x) frame_rt_purge_list[frame_purge_idx][frame_rt_purge_idx++] = x
#endif

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
static inline __attribute__((always_inline)) void vgl_memcpy(void *dst, const void *src, size_t size) {
#ifndef DEBUG_MEMCPY
#ifndef DISABLE_DMAC
	if (size >= 0x2000 && (uint32_t)src < 0x81000000 && (uint32_t)dst < 0x81000000)
		sceDmacMemcpy(dst, src, size);
	else
#endif
		vgl_fast_memcpy(dst, src, size);
#else
	memcpy(dst, src, size);
#endif
}

#endif
