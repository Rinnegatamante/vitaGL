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
uint32_t readR(const void *data);
uint32_t readRG(const void *data);
uint32_t readRGB(const void *data);
uint32_t readBGR(const void *data);
uint32_t readRGB565(const void *data);
uint32_t readRGBA(const void *data);
uint32_t readABGR(const void *data);
uint32_t readBGRA(const void *data);
uint32_t readARGB(const void *data);
uint32_t readRGBA5551(const void *data);
uint32_t readARGB1555(const void *data);
uint32_t readABGR1555(const void *data);
uint32_t readRGBA4444(const void *data);
uint32_t readL(const void *data);
uint32_t readLA(const void *data);

// Write callbacks
void writeR(void *data, uint32_t color);
void writeRG(void *data, uint32_t color);
void writeRA(void *data, uint32_t color);
void writeRGB(void *data, uint32_t color);
void writeBGR(void *data, uint32_t color);
void writeRGBA(void *data, uint32_t color);
void writeABGR(void *data, uint32_t color);
void writeBGRA(void *data, uint32_t color);

#endif
