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
 * gpu_utils.h:
 * Header file for the debug utilities.
 */

#ifndef _DEBUG_UTILS_H_
#define _DEBUG_UTILS_H_

// Debugging tool
char *get_gxm_error_literal(uint32_t code);
#ifdef FILE_LOG
void vgl_file_log(const char *format, ...);
#define vgl_log vgl_file_log
#elif defined(LOG_ERRORS)
#define vgl_log sceClibPrintf
#else
#define vgl_log(...)
#endif

#endif
