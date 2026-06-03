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
	if (!unif_pool) {
		unif_pool = gpu_alloc_mapped_for_cpu(UNIFORM_CIRCULAR_POOL_SIZE);
	}
}

void *vglReserveUniformCircularPoolBuffer(uint32_t size) {
	void *r;
	if (unif_idx + size >= UNIFORM_CIRCULAR_POOL_SIZE) {
#ifndef SKIP_ERROR_HANDLING
		static uint32_t last_frame_swap = 0;
		if (last_frame_swap == vgl_framecount) {
			vgl_log("%s:%d Circular Uniform Pool outage detected! Consider increasing UNIFORM_CIRCULAR_POOL_SIZE...\n", __FILE__, __LINE__);
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

#define decode_unif_component_simple(dst_stride, src_type, dst_type) \
	const uint32_t align_size = dst_stride; \
	const src_type *src = (const src_type *)sourceData; \
	dst_type *dst = (dst_type *)uniformBuffer; \
	for (int i = 0; i < count; i++) { \
		for (int j = 0; j < componentCount; i++) { \
			dst[j] = src[j]; \
		} \
		src += componentCount; \
		dst += align_size; \
	}

#include <stdio.h>

void vglSetUniformData(uint8_t *uniformBuffer, const SceGxmParameterType t, const int offset, const uint32_t count, const uint32_t componentCount, const void *sourceData, const SceGxmParameterType input_type) {
	if (offset != 0) {
		switch (t) {
			case SCE_GXM_PARAMETER_TYPE_F32:
				uniformBuffer += (componentCount < 3 ? 8 : 16) * offset;
				break;
			case SCE_GXM_PARAMETER_TYPE_U32:
			case SCE_GXM_PARAMETER_TYPE_S32:
				uniformBuffer += componentCount * offset;
				break;
			case SCE_GXM_PARAMETER_TYPE_F16:
				uniformBuffer += 8 * offset;
				break;
			case SCE_GXM_PARAMETER_TYPE_U16:
			case SCE_GXM_PARAMETER_TYPE_S16:
				uniformBuffer += (componentCount < 3 ? 4 : 8) * offset;
				break;
			case SCE_GXM_PARAMETER_TYPE_U8:
			case SCE_GXM_PARAMETER_TYPE_S8:
				uniformBuffer += 4 * offset;
				break;
			default:
				break;
		}
	}
	if (t == input_type) {// Input type is ideal, we can go for faster path
		if (count == 1) {
			switch (t) {
			case SCE_GXM_PARAMETER_TYPE_F32:
			case SCE_GXM_PARAMETER_TYPE_U32:
			case SCE_GXM_PARAMETER_TYPE_S32:
				vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * 4);
				break;
			case SCE_GXM_PARAMETER_TYPE_F16:
			case SCE_GXM_PARAMETER_TYPE_U16:
			case SCE_GXM_PARAMETER_TYPE_S16:
				vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * 2);
				break;
			case SCE_GXM_PARAMETER_TYPE_U8:
			case SCE_GXM_PARAMETER_TYPE_S8:
			default:
				vgl_fast_memcpy(uniformBuffer, sourceData, componentCount);
				break;
			}
		} else {
			switch (t) {
			case SCE_GXM_PARAMETER_TYPE_F32:
				if (componentCount == 2 || componentCount == 4) {
					vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * 4 * count);
				} else {
					uint32_t align_size = componentCount == 1 ? 8 : 16;
					float *src = (float *)sourceData;
					for (int i = 0; i < count; i++) {
						vgl_fast_memcpy(uniformBuffer, src, componentCount * 4);
						src += componentCount;
						uniformBuffer += align_size;
					}
				}
				break;
			case SCE_GXM_PARAMETER_TYPE_F16:
				if (componentCount == 4) {
					vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * 2 * count);
				} else {
					uint16_t *src = (uint16_t *)sourceData;
					for (int i = 0; i < count; i++) {
						vgl_fast_memcpy(uniformBuffer, src, componentCount * 2);
						src += componentCount;
						uniformBuffer += 8;
					}
				}
				break;
			case SCE_GXM_PARAMETER_TYPE_U32:
			case SCE_GXM_PARAMETER_TYPE_S32:
				vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * 4 * count);
				break;
			case SCE_GXM_PARAMETER_TYPE_U16:
			case SCE_GXM_PARAMETER_TYPE_S16:
				if (componentCount == 2 || componentCount == 4) {
					vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * 2 * count);
				} else {
					uint32_t align_size = componentCount == 1 ? 4 : 8;
					uint16_t *src = (uint16_t *)sourceData;
					for (int i = 0; i < count; i++) {
						vgl_fast_memcpy(uniformBuffer, src, componentCount * 2);
						src += componentCount;
						uniformBuffer += align_size;
					}
				}
				break;
			case SCE_GXM_PARAMETER_TYPE_U8:
			case SCE_GXM_PARAMETER_TYPE_S8:
				if (componentCount == 4) {
					vgl_fast_memcpy(uniformBuffer, sourceData, componentCount * count);
				} else {
					
					uint8_t *src = (uint8_t *)sourceData;
					for (int i = 0; i < count; i++) {
						vgl_fast_memcpy(uniformBuffer, src, componentCount);
						src += componentCount;
						uniformBuffer += 4;
					}
				}
				break;
			default:
				break;
			}
		}
	} else { // Slow-path, needs to convert type
		switch (t) {
		case SCE_GXM_PARAMETER_TYPE_F32:
			{
				// We support only S32 and F32 as input, so if we fall in slow path, input is surely S32
				decode_unif_component_simple(componentCount < 3 ? 2 : 4, int32_t, float)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_F16:
			if (input_type == SCE_GXM_PARAMETER_TYPE_F32) {
				decode_unif_component_simple(4, float, __fp16)
			} else {
				decode_unif_component_simple(4, int32_t, __fp16)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_U32:
			if (input_type == SCE_GXM_PARAMETER_TYPE_F32) {
				decode_unif_component_simple(componentCount, float, uint32_t)
			} else {
				decode_unif_component_simple(componentCount, int32_t, uint32_t)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_S32:
			{
				// We support only S32 and F32 as input, so if we fall in slow path, input is surely F32
				decode_unif_component_simple(componentCount, float, int32_t)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_U16:
			if (input_type == SCE_GXM_PARAMETER_TYPE_F32) {
				decode_unif_component_simple(componentCount < 3 ? 2 : 4, float, uint16_t)
			} else {
				decode_unif_component_simple(componentCount < 3 ? 2 : 4, int32_t, uint16_t)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_S16:
			if (input_type == SCE_GXM_PARAMETER_TYPE_F32) {
				decode_unif_component_simple(componentCount < 3 ? 2 : 4, float, int16_t)
			} else {
				decode_unif_component_simple(componentCount < 3 ? 2 : 4, int32_t, int16_t)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_U8:
			if (input_type == SCE_GXM_PARAMETER_TYPE_F32) {
				decode_unif_component_simple(4, float, uint8_t)
			} else {
				decode_unif_component_simple(4, int32_t, uint8_t)
			}
			break;
		case SCE_GXM_PARAMETER_TYPE_S8:
			if (input_type == SCE_GXM_PARAMETER_TYPE_F32) {
				decode_unif_component_simple(4, float, int8_t)
			} else {
				decode_unif_component_simple(4, int32_t, int8_t)
			}
			break;
		default:
			break;
		}
	}
}
