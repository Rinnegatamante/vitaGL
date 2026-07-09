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
 * texture_callbacks.h:
 * Header file for texture data reading/writing callbacks exposed by texture_callbacks.c
 */

#ifndef _TEXTURE_CALLBACKS_H_
#define _TEXTURE_CALLBACKS_H_

// Read callbacks
uint32_t read_r8(const void *data);
uint32_t read_rg88(const void *data);
uint32_t read_rgb888(const void *data);
uint32_t read_bgr888(const void *data);
uint32_t read_rgb565(const void *data);
uint32_t read_rgba8888(const void *data);
uint32_t read_abgr8888(const void *data);
uint32_t read_bgra8888(const void *data);
uint32_t read_argb8888(const void *data);
uint32_t read_rgba5551(const void *data);
uint32_t read_argb1555(const void *data);
uint32_t read_abgr1555(const void *data);
uint32_t read_rgba4444(const void *data);
uint32_t read_l8(const void *data);
uint32_t read_la88(const void *data);

// Write callbacks
void write_r8(void *data, uint32_t color);
void write_rg88(void *data, uint32_t color);
void write_ra88(void *data, uint32_t color);
void write_rgb888(void *data, uint32_t color);
void write_bgr888(void *data, uint32_t color);
void write_rgba8888(void *data, uint32_t color);
void write_abgr8888(void *data, uint32_t color);
void write_bgra8888(void *data, uint32_t color);
void write_rgba5551(void *data, uint32_t color);

// Inlined variants
static inline __attribute__((always_inline)) void write_rgba8888_inlined(void *data, uint32_t color) {
	uint32_t *dst = (uint32_t *)data;
	*dst = color;
}


#endif
