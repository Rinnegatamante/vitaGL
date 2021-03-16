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

#include "vitaGL.h"
#include "shared.h"

#ifdef FILE_LOG
void vgl_file_log(const char *format, ...) {
	__gnuc_va_list arg;
	va_start(arg, format);
	char msg[512];
	vsprintf(msg, format, arg);
	va_end(arg);
	FILE *log = fopen("ux0:/data/vitaGL.log", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
}
#endif

char *get_gl_error_literal(uint32_t code) {
	switch (code) {
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";
	default:
		return "Unknown Error";
	}
}

char *get_gxm_error_literal(uint32_t code) {
	switch (code) {
	case SCE_GXM_ERROR_UNINITIALIZED:
		return "SCE_GXM_ERROR_UNINITIALIZED";
	case SCE_GXM_ERROR_ALREADY_INITIALIZED:
		return "SCE_GXM_ERROR_ALREADY_INITIALIZED";
	case SCE_GXM_ERROR_OUT_OF_MEMORY:
		return "SCE_GXM_ERROR_OUT_OF_MEMORY";
	case SCE_GXM_ERROR_INVALID_VALUE:
		return "SCE_GXM_ERROR_INVALID_VALUE";
	case SCE_GXM_ERROR_INVALID_POINTER:
		return "SCE_GXM_ERROR_INVALID_POINTER";
	case SCE_GXM_ERROR_INVALID_ALIGNMENT:
		return "SCE_GXM_ERROR_INVALID_ALIGNMENT";
	case SCE_GXM_ERROR_NOT_WITHIN_SCENE:
		return "SCE_GXM_ERROR_NOT_WITHIN_SCENE";
	case SCE_GXM_ERROR_WITHIN_SCENE:
		return "SCE_GXM_ERROR_WITHIN_SCENE";
	case SCE_GXM_ERROR_NULL_PROGRAM:
		return "SCE_GXM_ERROR_NULL_PROGRAM";
	case SCE_GXM_ERROR_UNSUPPORTED:
		return "SCE_GXM_ERROR_UNSUPPORTED";
	case SCE_GXM_ERROR_PATCHER_INTERNAL:
		return "SCE_GXM_ERROR_PATCHER_INTERNAL";
	case SCE_GXM_ERROR_RESERVE_FAILED:
		return "SCE_GXM_ERROR_RESERVE_FAILED";
	case SCE_GXM_ERROR_PROGRAM_IN_USE:
		return "SCE_GXM_ERROR_PROGRAM_IN_USE";
	case SCE_GXM_ERROR_INVALID_INDEX_COUNT:
		return "SCE_GXM_ERROR_INVALID_INDEX_COUNT";
	case SCE_GXM_ERROR_INVALID_POLYGON_MODE:
		return "SCE_GXM_ERROR_INVALID_POLYGON_MODE";
	case SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_PRECISION:
		return "SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_PRECISION";
	case SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_COMPONENT_COUNT:
		return "SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_COMPONENT_COUNT";
	case SCE_GXM_ERROR_UNIFORM_BUFFER_NOT_RESERVED:
		return "SCE_GXM_ERROR_UNIFORM_BUFFER_NOT_RESERVED";
	case SCE_GXM_ERROR_INVALID_AUXILIARY_SURFACE:
		return "SCE_GXM_ERROR_INVALID_AUXILIARY_SURFACE";
	case SCE_GXM_ERROR_INVALID_PRECOMPUTED_DRAW:
		return "SCE_GXM_ERROR_INVALID_PRECOMPUTED_DRAW";
	case SCE_GXM_ERROR_INVALID_PRECOMPUTED_VERTEX_STATE:
		return "SCE_GXM_ERROR_INVALID_PRECOMPUTED_VERTEX_STATE";
	case SCE_GXM_ERROR_INVALID_PRECOMPUTED_FRAGMENT_STATE:
		return "SCE_GXM_ERROR_INVALID_PRECOMPUTED_FRAGMENT_STATE";
	case SCE_GXM_ERROR_DRIVER:
		return "SCE_GXM_ERROR_DRIVER";
	case SCE_GXM_ERROR_INVALID_TEXTURE:
		return "SCE_GXM_ERROR_INVALID_TEXTURE";
	case SCE_GXM_ERROR_INVALID_TEXTURE_DATA_POINTER:
		return "SCE_GXM_ERROR_INVALID_TEXTURE_DATA_POINTER";
	case SCE_GXM_ERROR_INVALID_TEXTURE_PALETTE_POINTER:
		return "SCE_GXM_ERROR_INVALID_TEXTURE_PALETTE_POINTER";
	default:
		return "Unknown Error";
	}
}
