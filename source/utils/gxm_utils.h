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
 * gxm_utils.h:
 * Header file for the GXM utilities exposed by gxm_utils.c
 */

#ifndef _GXM_UTILS_H_
#define _GXM_UTILS_H_

//#define PARANOID // Enable this flag to use original sceGxmTexture functions instead of faster re-implementations

void vglReserveFragmentUniformBuffer(SceGxmProgram *p, void **uniformBuffer);
void vglReserveVertexUniformBuffer(SceGxmProgram *p, void **uniformBuffer);
void vglRestoreFragmentUniformBuffer(void);
void vglRestoreVertexUniformBuffer(void);
void vglSetupUniformCircularPool(void);

#ifndef PARANOID
// Faster variants with stripped error handling
uint32_t vglGetTexWidth(const SceGxmTexture *texture);
uint32_t vglGetTexHeight(const SceGxmTexture *texture);
void vglSetTexUMode(SceGxmTexture *texture, SceGxmTextureAddrMode addrMode);
void vglSetTexVMode(SceGxmTexture *texture, SceGxmTextureAddrMode addrMode);
void vglSetTexMinFilter(SceGxmTexture *texture, SceGxmTextureFilter minFilter);
void vglSetTexMagFilter(SceGxmTexture *texture, SceGxmTextureFilter magFilter);
void vglSetTexMipFilter(SceGxmTexture *texture, SceGxmTextureMipFilter mipFilter);
void vglSetTexLodBias(SceGxmTexture *texture, uint32_t bias);
void vglSetTexMipmapCount(SceGxmTexture *texture, uint32_t count);
void vglSetTexGammaMode(SceGxmTexture *texture, SceGxmTextureGammaMode mode);
void vglSetTexPalette(SceGxmTexture *texture, void *data);
void vglInitLinearTexture(SceGxmTexture *texture, const void *data, SceGxmTextureFormat texFormat, unsigned int width, unsigned int height, unsigned int mipCount);
void vglInitSwizzledTexture(SceGxmTexture *texture, const void *data, SceGxmTextureFormat texFormat, unsigned int width, unsigned int height, unsigned int mipCount);
#else
// Default sceGxm functions
#define vglGetTexWidth sceGxmTextureGetWidth
#define vglGetTexHeight sceGxmTextureGetHeight
#define vglSetTexUMode sceGxmTextureSetUAddrMode
#define vglSetTexVMode sceGxmTextureSetVAddrMode
#define vglSetTexMinFilter sceGxmTextureSetMinFilter
#define vglSetTexMagFilter sceGxmTextureSetMagFilter
#define vglSetTexMipFilter sceGxmTextureSetMipFilter
#define vglSetTexLodBias sceGxmTextureSetLodBias
#define vglSetTexMipmapCount sceGxmTextureSetMipmapCount
#define vglSetTexGammaMode sceGxmTextureSetGammaMode
#define vglSetTexPalette sceGxmTextureSetPalette
#define vglInitLinearTexture sceGxmTextureInitLinear
#define vglInitSwizzledTexture sceGxmTextureInitSwizzledArbitrary
#endif
#endif
