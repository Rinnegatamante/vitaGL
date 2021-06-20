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

#ifndef PARANOID
typedef struct {
	uint32_t control_words[4];
} SceGxmTextureInternal;

uint32_t vglGetTexWidth(const SceGxmTexture *texture) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	return ((tex->control_words[1] >> 12) & 0xFFF) + 1;
}

uint32_t vglGetTexHeight(const SceGxmTexture *texture) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	return (tex->control_words[1] & 0xFFF) + 1;
}

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

void vglInitSwizzledTexture(SceGxmTexture *texture, const void *data, SceGxmTextureFormat texFormat, unsigned int width, unsigned int height, unsigned int mipCount) {
	SceGxmTextureInternal *tex = (SceGxmTextureInternal *)texture;
	tex->control_words[0] = ((mipCount - 1) & 0xF) << 17 | 0x3E00090 | texFormat & 0x80000000;
	tex->control_words[1] = (height - 1) | 0xA0000000 | ((width - 1) << 12) | texFormat & 0x1F000000;
	tex->control_words[2] = (uint32_t)data & 0xFFFFFFFC;
	tex->control_words[3] = ((texFormat & 0x7000) << 16) | 0x80000000;
}
#endif