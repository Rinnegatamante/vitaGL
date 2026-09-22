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
#include <stdio.h>

#define XXH_INLINE_ALL
#define XXH_memcpy vgl_fast_memcpy
#define XXH_memset sceClibMemset
#include "xxhash_utils.h"

#define UNIFORM_CIRCULAR_POOL_SIZE (2 * 1024 * 1024)

void *vgl_def_frag_buf = NULL;
void *vgl_def_vert_buf = NULL;
static uint8_t *unif_pool = NULL;
static uint32_t unif_idx = 0;

#define VERTEX_PROGRAM_CACHE_WAYS 4
#define VERTEX_PROGRAM_CACHE_SETS 64

typedef struct {
	uint64_t key;
	SceGxmShaderPatcherId id;
	SceGxmVertexProgram *prog;
} vertex_program_cache_entry;

typedef struct {
	SceGxmShaderPatcherId id;
	SceGxmVertexProgram *prog;
	uint8_t attr_num;
	uint8_t stream_num;
	SceGxmVertexAttribute attr[SCE_GXM_MAX_VERTEX_ATTRIBUTES];
	SceGxmVertexStream stream[SCE_GXM_MAX_VERTEX_STREAMS];
} vertex_program_cache_mru_entry;

typedef struct {
	uint64_t key;
	SceGxmShaderPatcherId id;
} vertex_program_cache_last_entry;

typedef struct {
	uint32_t id;
	uint32_t attr_num;
	uint32_t stream_num;
	uint8_t data[SCE_GXM_MAX_VERTEX_ATTRIBUTES * sizeof(SceGxmVertexAttribute) + SCE_GXM_MAX_VERTEX_STREAMS * sizeof(SceGxmVertexStream)];
} vertex_program_cache_key;

static vertex_program_cache_entry vertex_program_cache[VERTEX_PROGRAM_CACHE_SETS][VERTEX_PROGRAM_CACHE_WAYS] = {};
static vertex_program_cache_mru_entry vertex_program_cache_mru = {};
static vertex_program_cache_last_entry vertex_program_cache_last = {};
static GLboolean vertex_program_cache_mru_valid = GL_FALSE;
static uint8_t vertex_program_cache_next[VERTEX_PROGRAM_CACHE_SETS] = {};

#ifdef HAVE_PROFILING
vgl_vertex_program_cache_stats vgl_vcache_stats;

static inline __attribute__((always_inline)) void vertex_program_cache_profile_end(uint32_t start, uint32_t *bucket) {
	uint32_t elapsed = sceKernelGetProcessTimeLow() - start;
	vgl_vcache_stats.total_us += elapsed;
	*bucket += elapsed;
}
#endif

static inline __attribute__((always_inline)) GLboolean vertex_program_is_mru(const vertex_program_cache_mru_entry *entry, SceGxmShaderPatcherId id, const SceGxmVertexAttribute *attr, uint32_t attr_num, const SceGxmVertexStream *stream, uint32_t stream_num) {
	if (entry->id != id || entry->attr_num != attr_num || entry->stream_num != stream_num) {
		return GL_FALSE;
	} else if (attr_num && sceClibMemcmp(entry->attr, attr, attr_num * sizeof(SceGxmVertexAttribute))) {
		return GL_FALSE;
	} else if (stream_num && sceClibMemcmp(entry->stream, stream, stream_num * sizeof(SceGxmVertexStream))) {
		return GL_FALSE;
	}
	return GL_TRUE;
}

int vglCreateVertexProgram(SceGxmShaderPatcherId id, const SceGxmVertexAttribute *attr, uint32_t attr_num, const SceGxmVertexStream *stream, uint32_t stream_num, SceGxmVertexProgram **prog) {
#ifdef HAVE_PROFILING
	uint32_t profile_start = sceKernelGetProcessTimeLow();
	vgl_vcache_stats.calls_num++;
#endif

	// First check if last used vertex program matches
	if (vertex_program_cache_mru_valid && vertex_program_is_mru(&vertex_program_cache_mru, id, attr, attr_num, stream, stream_num)) {
		*prog = vertex_program_cache_mru.prog;
#ifdef HAVE_PROFILING
		vgl_vcache_stats.hits++;
		vgl_vcache_stats.mru_hits++;
		vertex_program_cache_profile_end(profile_start, &vgl_vcache_stats.mru_us);
#endif
		return 0;
	}
	vertex_program_cache_mru_valid = GL_FALSE;

	// If program effectively changed, first lookup in our fast cache
	vertex_program_cache_key data;
	data.id = (uint32_t)id;
	data.attr_num = attr_num;
	data.stream_num = stream_num;
	uint8_t *dst = data.data;
	if (attr_num) {
		uint32_t size = attr_num * sizeof(SceGxmVertexAttribute);
		vgl_fast_memcpy(dst, attr, size);
		dst += size;
	}
	if (stream_num) {
		uint32_t size = stream_num * sizeof(SceGxmVertexStream);
		vgl_fast_memcpy(dst, stream, size);
		dst += size;
	}
	uint64_t key = XXH3_64bits(&data, offsetof(vertex_program_cache_key, data) + (dst - data.data));
	uint32_t set = (uint32_t)(key ^ (key >> 32)) & (VERTEX_PROGRAM_CACHE_SETS - 1);
	vertex_program_cache_entry *entries = vertex_program_cache[set];
	for (uint32_t i = 0; i < VERTEX_PROGRAM_CACHE_WAYS; i++) {
		if (entries[i].id == id && entries[i].key == key) {
			*prog = entries[i].prog;
			// Enable mru fast-path only if the vertex program gets requested twice in a row,
			// this might sound counter-intuitive but is more efficient from real use case tests
			if (vertex_program_cache_last.id == id && vertex_program_cache_last.key == key) {
				vertex_program_cache_mru.id = id;
				vertex_program_cache_mru.prog = *prog;
				vertex_program_cache_mru.attr_num = attr_num;
				vertex_program_cache_mru.stream_num = stream_num;
				if (attr_num) {
					vgl_fast_memcpy(vertex_program_cache_mru.attr, attr, attr_num * sizeof(SceGxmVertexAttribute));
				}
				if (stream_num) {
					vgl_fast_memcpy(vertex_program_cache_mru.stream, stream, stream_num * sizeof(SceGxmVertexStream));
				}
				vertex_program_cache_mru_valid = GL_TRUE;
			}
			vertex_program_cache_last.id = id;
			vertex_program_cache_last.key = key;
#ifdef HAVE_PROFILING
			vgl_vcache_stats.hits++;
			vgl_vcache_stats.table_hits++;
			vertex_program_cache_profile_end(profile_start, &vgl_vcache_stats.table_us);
#endif
			return 0;
		}
	}

	// If cache missed, go for the normal internal sceGxm cache or effective patched shader gen
#ifdef HAVE_PROFILING
	vgl_vcache_stats.gen_calls++;
#endif
	int r = sceGxmShaderPatcherCreateVertexProgram(gxm_shader_patcher, id, attr, attr_num, stream, stream_num, prog);
	if (!r) {
		uint32_t slot = VERTEX_PROGRAM_CACHE_WAYS;
		for (uint32_t i = 0; i < VERTEX_PROGRAM_CACHE_WAYS; i++) {
			if (!entries[i].id) {
				slot = i;
				break;
			}
		}
		if (slot == VERTEX_PROGRAM_CACHE_WAYS) {
			slot = vertex_program_cache_next[set];
#ifdef HAVE_PROFILING
			vgl_vcache_stats.evictions++;
#endif
		}
		entries[slot].key = key;
		entries[slot].id = id;
		entries[slot].prog = *prog;
		vertex_program_cache_last.id = id;
		vertex_program_cache_last.key = key;
		vertex_program_cache_next[set] = (slot + 1) & (VERTEX_PROGRAM_CACHE_WAYS - 1);
	}
#ifdef HAVE_PROFILING
	vertex_program_cache_profile_end(profile_start, &vgl_vcache_stats.gen_us);
#endif
	return r;
}

void vglVertexProgramCacheReset(void) {
	sceClibMemset(vertex_program_cache, 0, sizeof(vertex_program_cache));
	sceClibMemset(&vertex_program_cache_mru, 0, sizeof(vertex_program_cache_mru));
	sceClibMemset(&vertex_program_cache_last, 0, sizeof(vertex_program_cache_last));
	vertex_program_cache_mru_valid = GL_FALSE;
	sceClibMemset(vertex_program_cache_next, 0, sizeof(vertex_program_cache_next));
}

void vglVertexProgramCacheInvalidate(SceGxmShaderPatcherId id) {
	if (vertex_program_cache_mru.id == id) {
		sceClibMemset(&vertex_program_cache_mru, 0, sizeof(vertex_program_cache_mru));
		vertex_program_cache_mru_valid = GL_FALSE;
	}
	if (vertex_program_cache_last.id == id) {
		sceClibMemset(&vertex_program_cache_last, 0, sizeof(vertex_program_cache_last));
	}
	for (uint32_t set = 0; set < VERTEX_PROGRAM_CACHE_SETS; set++) {
		for (uint32_t way = 0; way < VERTEX_PROGRAM_CACHE_WAYS; way++) {
			if (vertex_program_cache[set][way].id == id) {
				vertex_program_cache[set][way].key = 0;
				vertex_program_cache[set][way].id = NULL;
				vertex_program_cache[set][way].prog = NULL;
			}
		}
	}
	sceGxmShaderPatcherForceUnregisterProgram(gxm_shader_patcher, id);
}

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
		for (int j = 0; j < componentCount; j++) { \
			dst[j] = src[j]; \
		} \
		src += componentCount; \
		dst += align_size; \
	}

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
	if (t == input_type) { // Input type is ideal, we can go for faster path
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
