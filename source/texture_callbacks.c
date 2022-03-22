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
 * texture_callbacks.c:
 * Implementation for texture data reading/writing callbacks
 */

#include "shared.h"
#include "texture_callbacks.h"
#include "vitaGL.h"

#define convert_u16_to_u32_cspace(color, lshift, rshift, mask) ((((color << lshift) >> rshift) & mask) * 0xFF) / mask

// Read callback for 32bpp unsigned ABGR format
uint32_t readBGRA(void *data) {
	uint8_t *bgra = (uint8_t *)data;
	return ((bgra[3] << 24) | (bgra[0] << 16) | (bgra[1] << 8) | bgra[2]);
}

// Read callback for 32bpp unsigned RGBA format
uint32_t readRGBA(void *data) {
	uint8_t *rgba = (uint8_t *)data;
	return ((rgba[3] << 24) | (rgba[2] << 16) | (rgba[1] << 8) | rgba[0]);
}

// Read callback for 32bpp unsigned ABGR format
uint32_t readABGR(void *data) {
	uint8_t *rgba = (uint8_t *)data;
	return ((rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3]);
}

// Read callback for 16bpp unsigned RGBA5551 format
uint32_t readRGBA5551(void *data) {
	uint16_t clr;
	uint8_t r, g, b, a;
	uint8_t *dst = (uint8_t *)&clr;
	uint8_t *src = (uint8_t *)data;
	dst[0] = src[2];
	dst[1] = src[1];
	r = convert_u16_to_u32_cspace(clr, 0, 11, 0x1F);
	g = convert_u16_to_u32_cspace(clr, 5, 11, 0x1F);
	b = convert_u16_to_u32_cspace(clr, 10, 11, 0x1F);
	a = convert_u16_to_u32_cspace(clr, 15, 15, 0x01);
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

// Read callback for 16bpp unsigned RGBA4444 format
uint32_t readRGBA4444(void *data) {
	uint16_t clr;
	uint8_t r, g, b, a;
	uint8_t *dst = (uint8_t *)&clr;
	uint8_t *src = (uint8_t *)data;
	dst[0] = src[2];
	dst[1] = src[1];
	r = convert_u16_to_u32_cspace(clr, 0, 12, 0x0F);
	g = convert_u16_to_u32_cspace(clr, 4, 12, 0x0F);
	b = convert_u16_to_u32_cspace(clr, 8, 12, 0x0F);
	a = convert_u16_to_u32_cspace(clr, 12, 12, 0x0F);
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

// Read callback for 16bpp unsigned RGB565 format
uint32_t readRGB565(void *data) {
	uint16_t clr;
	uint8_t r, g, b;
	uint8_t *dst = (uint8_t *)&clr;
	uint8_t *src = (uint8_t *)data;
	dst[0] = src[2];
	dst[1] = src[1];
	r = convert_u16_to_u32_cspace(clr, 0, 11, 0x1F);
	g = convert_u16_to_u32_cspace(clr, 5, 10, 0x3F);
	b = convert_u16_to_u32_cspace(clr, 11, 11, 0x1F);
	return ((0xFF << 24) | (b << 16) | (g << 8) | r);
}

// Read callback for 24bpp unsigned BGR format
uint32_t readBGR(void *data) {
	uint8_t *bgr = (uint8_t *)data;
	return ((0xFF << 24) | (bgr[0] << 16) | (bgr[1] << 8) | bgr[2]);
}

// Read callback for 24bpp unsigned RGB format
uint32_t readRGB(void *data) {
	uint8_t *rgb = (uint8_t *)data;
	return ((0xFF << 24) | (rgb[2] << 16) | (rgb[1] << 8) | rgb[0]);
}

// Read callback for 16bpp unsigned RG format
uint32_t readRG(void *data) {
	uint8_t *rg = (uint8_t *)data;
	return ((0xFFFF << 16) | (rg[1] << 8) | rg[0]);
}

// Read callback for 8bpp unsigned R format
uint32_t readR(void *data) {
	uint8_t *r = (uint8_t *)data;
	return ((0xFFFFFF << 8) | r[0]);
}

uint32_t readL(void *data) {
	uint8_t *d = (uint8_t *)data;
	uint8_t lum = d[0];
	return ((0xFF << 24) | (lum << 16) | (lum << 8) | lum);
}

uint32_t readLA(void *data) {
	uint8_t *d = (uint8_t *)data;
	uint8_t lum = d[0];
	uint8_t a = d[1];
	return ((a << 24) | (lum << 16) | (lum << 8) | lum);
}

// Write callback for 32bpp unsigned RGBA format
void writeRGBA(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst[3] = src[3];
}

// Write callback for 32bpp unsigned ABGR format
void writeABGR(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];
}

// Write callback for 32bpp unsigned BGRA format
void writeBGRA(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[2];
	dst[1] = src[1];
	dst[2] = src[0];
	dst[3] = src[3];
}

// Write callback for 24bpp unsigned RGB format
void writeRGB(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}

// Write callback for 24bpp unsigned BGR format
void writeBGR(void *data, uint32_t color) {
	uint8_t *src = (uint8_t *)&color;
	uint8_t *dst = (uint8_t *)data;
	dst[0] = src[2];
	dst[1] = src[1];
	dst[2] = src[0];
}

// Write callback for 16bpp unsigned RG format
void writeRG(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[1];
}

// Write callback for 16bpp unsigned RA format
void writeRA(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[3];
}

// Write callback for 8bpp unsigned R format
void writeR(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
}
