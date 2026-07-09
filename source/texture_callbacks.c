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

#define convert_u16_to_u32_cspace(color, lshift, rshift, mask) (((((color << lshift) >> rshift) & mask) * 0xFF) / mask)

// Read callback for 32bpp unsigned BGRA format
uint32_t read_bgra8888(const void *data) {
	const uint8_t *bgra = (uint8_t *)data;
	return ((bgra[3] << 24) | (bgra[0] << 16) | (bgra[1] << 8) | bgra[2]);
}

// Read callback for 32bpp unsigned RGBA format
uint32_t read_rgba8888(const void *data) {
	const uint8_t *rgba = (uint8_t *)data;
	return ((rgba[3] << 24) | (rgba[2] << 16) | (rgba[1] << 8) | rgba[0]);
}

// Read callback for 32bpp unsigned ABGR format
uint32_t read_abgr8888(const void *data) {
	const uint8_t *abgr = (uint8_t *)data;
	return ((abgr[0] << 24) | (abgr[1] << 16) | (abgr[2] << 8) | abgr[3]);
}

// Read callback for 32bpp unsigned ARGB format
uint32_t read_argb8888(const void *data) {
	const uint8_t *argb = (uint8_t *)data;
	return ((argb[0] << 24) | (argb[3] << 16) | (argb[2] << 8) | argb[1]);
}

// Read callback for 16bpp unsigned RGBA5551 format
uint32_t read_rgba5551(const void *data) {
	const uint16_t clr = *(uint16_t *)data;
	uint8_t r, g, b, a;
	r = convert_u16_to_u32_cspace(clr, 0, 11, 0x1F);
	g = convert_u16_to_u32_cspace(clr, 5, 11, 0x1F);
	b = convert_u16_to_u32_cspace(clr, 10, 11, 0x1F);
	a = convert_u16_to_u32_cspace(clr, 15, 15, 0x01);
	return (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r);
}

// Read callback for 16bpp unsigned BGRA1555 format
uint32_t read_argb1555(const void *data) {
	const uint16_t clr = *(uint16_t *)data;
	uint8_t r, g, b, a;
	b = convert_u16_to_u32_cspace(clr, 11, 11, 0x1F);
	g = convert_u16_to_u32_cspace(clr, 6, 11, 0x1F);
	r = convert_u16_to_u32_cspace(clr, 1, 11, 0x1F);
	a = convert_u16_to_u32_cspace(clr, 0, 15, 0x01);
	return (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r);
}

// Read callback for 16bpp unsigned BGRA1555 format
uint32_t read_abgr1555(const void *data) {
	const uint16_t clr = *(uint16_t *)data;
	uint8_t r, g, b, a;
	r = convert_u16_to_u32_cspace(clr, 11, 11, 0x1F);
	g = convert_u16_to_u32_cspace(clr, 6, 11, 0x1F);
	b = convert_u16_to_u32_cspace(clr, 1, 11, 0x1F);
	a = convert_u16_to_u32_cspace(clr, 0, 15, 0x01);
	return (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r);
}

// Read callback for 16bpp unsigned RGBA4444 format
uint32_t read_rgba4444(const void *data) {
	const uint16_t clr = *(uint16_t *)data;
	uint8_t r, g, b, a;
	r = convert_u16_to_u32_cspace(clr, 0, 12, 0x0F);
	g = convert_u16_to_u32_cspace(clr, 4, 12, 0x0F);
	b = convert_u16_to_u32_cspace(clr, 8, 12, 0x0F);
	a = convert_u16_to_u32_cspace(clr, 12, 12, 0x0F);
	return ((a << 24) | (b << 16) | (g << 8) | r);
}

// Read callback for 16bpp unsigned RGB565 format
uint32_t read_rgb565(const void *data) {
	const uint16_t clr = *(uint16_t *)data;
	uint8_t r, g, b;
	r = convert_u16_to_u32_cspace(clr, 0, 11, 0x1F);
	g = convert_u16_to_u32_cspace(clr, 5, 10, 0x3F);
	b = convert_u16_to_u32_cspace(clr, 11, 11, 0x1F);
	return (((uint32_t)0xFF << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r);
}

// Read callback for 24bpp unsigned BGR format
uint32_t read_bgr888(const void *data) {
	const uint8_t *bgr = (uint8_t *)data;
	return (((uint32_t)0xFF << 24) | ((uint32_t)bgr[0] << 16) | ((uint32_t)bgr[1] << 8) | bgr[2]);
}

// Read callback for 24bpp unsigned RGB format
uint32_t read_rgb888(const void *data) {
	const uint8_t *rgb = (uint8_t *)data;
	return (((uint32_t)0xFF << 24) | ((uint32_t)rgb[2] << 16) | ((uint32_t)rgb[1] << 8) | rgb[0]);
}

// Read callback for 16bpp unsigned RG format
uint32_t read_rg88(const void *data) {
	const uint8_t *rg = (uint8_t *)data;
	return (((uint32_t)0xFFFF << 16) | ((uint32_t)rg[1] << 8) | rg[0]);
}

// Read callback for 8bpp unsigned R format
uint32_t read_r8(const void *data) {
	const uint8_t *r = (uint8_t *)data;
	return ((0xFFFFFF << 8) | r[0]);
}

uint32_t read_l8(const void *data) {
	const uint8_t *d = (uint8_t *)data;
	uint8_t lum = d[0];
	return (((uint32_t)0xFF << 24) | ((uint32_t)lum << 16) | ((uint32_t)lum << 8) | lum);
}

uint32_t read_la88(const void *data) {
	const uint8_t *d = (uint8_t *)data;
	uint8_t lum = d[0];
	uint8_t a = d[1];
	return ((a << 24) | (lum << 16) | (lum << 8) | lum);
}

// Write callback for 32bpp unsigned RGBA format
void write_rgba8888(void *data, uint32_t color) {
	uint32_t *dst = (uint32_t *)data;
	*dst = color;
}

// Write callback for 32bpp unsigned ABGR format
void write_abgr8888(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[3];
	dst[1] = src[2];
	dst[2] = src[1];
	dst[3] = src[0];
}

// Write callback for 32bpp unsigned BGRA format
void write_bgra8888(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[2];
	dst[1] = src[1];
	dst[2] = src[0];
	dst[3] = src[3];
}

// Write callback for 24bpp unsigned RGB format
void write_rgb888(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
}

// Write callback for 24bpp unsigned BGR format
void write_bgr888(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[2];
	dst[1] = src[1];
	dst[2] = src[0];
}

// Write callback for 16bpp unsigned RG format
void write_rg88(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[1];
}

// Write callback for 16bpp unsigned RA format
void write_ra88(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
	dst[1] = src[3];
}

// Write callback for 8bpp unsigned R format
void write_r8(void *data, uint32_t color) {
	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	dst[0] = src[0];
}

// Write callback for 16bpp unsigned RGBA5551 format
void write_rgba5551(void *data, uint32_t color) {
	uint16_t *dst = (uint16_t *)data;
	const uint8_t *src = (uint8_t *)&color;
	uint8_t r, g, b, a;
	r = src[0] >> 3;
    g = src[1] >> 3;
    b = src[2] >> 3;
    a = src[3] >> 7;
	*dst = (r << 11) | (g << 6) | (b << 1) | a;
}
