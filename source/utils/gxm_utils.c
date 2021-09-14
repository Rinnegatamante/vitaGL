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

static void *frag_buf = NULL;
static void *vert_buf = NULL;
static uint8_t *unif_pool = NULL;
static uint32_t unif_idx = 0;

void vglSetupUniformCircularPool() {
	unif_pool = gpu_alloc_mapped(UNIFORM_CIRCULAR_POOL_SIZE, VGL_MEM_RAM);
}

void *vglReserveUniformCircularPoolBuffer(uint32_t size) {
	void *r;
	if (unif_idx + size > UNIFORM_CIRCULAR_POOL_SIZE) {
		r = unif_pool;
		unif_idx = size;
	} else {
		r = (unif_pool + unif_idx);
		unif_idx += size;
	}
	return r;
}

void vglRestoreFragmentUniformBuffer(void) {
	if (frag_buf)
		sceGxmSetFragmentDefaultUniformBuffer(gxm_context, frag_buf);
}

void vglRestoreVertexUniformBuffer(void) {
	if (vert_buf)
		sceGxmSetVertexDefaultUniformBuffer(gxm_context, vert_buf);
}

uint32_t vglReserveFragmentUniformBuffer(const SceGxmProgram *p, void **uniformBuffer) {
	uint32_t size = sceGxmProgramGetDefaultUniformBufferSize(p);
	if (size) {
		frag_buf = vglReserveUniformCircularPoolBuffer(size);
		sceGxmSetFragmentDefaultUniformBuffer(gxm_context, frag_buf);
		*uniformBuffer = frag_buf;
	}
	return size;
}

uint32_t vglReserveVertexUniformBuffer(const SceGxmProgram *p, void **uniformBuffer) {
	uint32_t size = sceGxmProgramGetDefaultUniformBufferSize(p);
	if (size) {
		vert_buf = vglReserveUniformCircularPoolBuffer(size);
		sceGxmSetVertexDefaultUniformBuffer(gxm_context, vert_buf);
		*uniformBuffer = vert_buf;
	}
	return size;
}

#ifndef PARANOID
typedef struct {
	uint32_t control_words[4];
} SceGxmTextureInternal;

void vglSetTexUMode(SceGxmTexture *texture, SceGxmTextureAddrMode addrMode) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((addrMode << 6) & 0x1C0) | tex->control_words[0] & 0xFFFFFE3F;
}

void vglSetTexVMode(SceGxmTexture *texture, SceGxmTextureAddrMode addrMode) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((addrMode << 3) & 0x38) | tex->control_words[0] & 0xFFFFFFC7;
}

void vglSetTexMinFilter(SceGxmTexture *texture, SceGxmTextureFilter minFilter) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((minFilter << 10) & 0xC00) | tex->control_words[0] & 0xFFFFF3FF;
}

void vglSetTexMagFilter(SceGxmTexture *texture, SceGxmTextureFilter magFilter) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((magFilter << 12) & 0x3000) | tex->control_words[0];
}

void vglSetTexMipFilter(SceGxmTexture *texture, SceGxmTextureMipFilter mipFilter) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = mipFilter & 0x200 | tex->control_words[0] & 0xFFFFFDFF;
}

void vglSetTexLodBias(SceGxmTexture *texture, uint32_t bias) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = tex->control_words[0] & 0xF81FFFFF | (bias << 21);
}

void vglSetTexMipmapCount(SceGxmTexture *texture, uint32_t count) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = tex->control_words[0] & 0xFFE1FFFF | (((count - 1) & 0xF) << 17);
}

void vglSetTexGammaMode(SceGxmTexture *texture, SceGxmTextureGammaMode mode) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = mode & 0x18000000 | tex->control_words[0] & 0xE7FFFFFF;
}

void vglSetTexPalette(SceGxmTexture *texture, void *data) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[3] = tex->control_words[3] & 0xFC000000 | (uint32_t)data >> 6;
}

void vglInitLinearTexture(SceGxmTexture *texture, const void *data, SceGxmTextureFormat texFormat, unsigned int width, unsigned int height, unsigned int mipCount) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((mipCount - 1) & 0xF) << 17 | 0x3E00090 | texFormat & 0x80000000;
	tex->control_words[1] = (height - 1) | 0x60000000 | ((width - 1) << 12) | texFormat & 0x1F000000;
	tex->control_words[2] = (uint32_t)data & 0xFFFFFFFC;
	tex->control_words[3] = ((texFormat & 0x7000) << 16) | 0x80000000;
}

void vglInitCubeTexture(SceGxmTexture *texture, const void *data, SceGxmTextureFormat texFormat, unsigned int width, unsigned int height, unsigned int mipCount) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((mipCount - 1) & 0xF) << 17 | 0x3E00090 | texFormat & 0x80000000;
	tex->control_words[1] = (31 - __builtin_clz(height)) | 0x40000000 | ((31 - __builtin_clz(width)) << 16) | texFormat & 0x1F000000;
	tex->control_words[2] = (uint32_t)data & 0xFFFFFFFC;
	tex->control_words[3] = ((texFormat & 0x7000) << 16) | 0x80000000;
}

void vglInitSwizzledTexture(SceGxmTexture *texture, const void *data, SceGxmTextureFormat texFormat, unsigned int width, unsigned int height, unsigned int mipCount) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((mipCount - 1) & 0xF) << 17 | 0x3E00090 | texFormat & 0x80000000;
	tex->control_words[1] = (height - 1) | 0xA0000000 | ((width - 1) << 12) | texFormat & 0x1F000000;
	tex->control_words[2] = (uint32_t)data & 0xFFFFFFFC;
	tex->control_words[3] = ((texFormat & 0x7000) << 16) | 0x80000000;
}
#endif
