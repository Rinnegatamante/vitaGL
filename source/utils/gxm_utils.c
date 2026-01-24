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
 * gxm_utils.c:
 * Utilities for GXM api usage
 */
#include "../shared.h"

#define UNIFORM_CIRCULAR_POOL_SIZE (2 * 1024 * 1024)

void *vgl_def_frag_buf = NULL;
void *vgl_def_vert_buf = NULL;
static uint8_t *unif_pool = NULL;
static uint32_t unif_idx = 0;

void vglSetupUniformCircularPool() {
	unif_pool = gpu_alloc_mapped(UNIFORM_CIRCULAR_POOL_SIZE, VGL_MEM_RAM);
}

void *vglReserveUniformCircularPoolBuffer(uint32_t size) {
	void *r;
	if (unif_idx + size >= UNIFORM_CIRCULAR_POOL_SIZE) {
#ifndef SKIP_ERROR_HANDLING
		static uint32_t last_frame_swap = 0;
		if (last_frame_swap == vgl_framecount) {
			vgl_log("%s:%d Circular Uniform Pool outage detected! Consider increasing its size...\n", __FILE__, __LINE__);
		}
		last_frame_swap = vgl_framecount;
#endif
		r = unif_pool;
		unif_idx = size;
	} else {
		r = (unif_pool + unif_idx);
		unif_idx += size;
	}
	return r;
}
