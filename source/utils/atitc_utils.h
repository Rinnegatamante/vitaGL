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
 * atitc_utils.c:
 * Utilities for ATI texture compression support
 */
 
typedef enum {
    ATC_RGB = 1,
    ATC_EXPLICIT_ALPHA = 3,
    ATC_INTERPOLATED_ALPHA = 5,
} ATITCDecodeFlag;

//Decode ATITC encode data to RGBA32
void atitc_decode(uint8_t *encodeData, uint8_t *decodeData, const int pixelsWidth, const int pixelsHeight, ATITCDecodeFlag decodeFlag);
