/*
 * This file is part of vitaGL
 * Copyright 2017-2023 Rinnegatamante
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
 * Header file for the GLSL translator utilities exposed by glsl_utils.c
 */

#ifndef _GLSL_UTILS_H_
#define _GLSL_UTILS_H_
#ifdef HAVE_GLSL_TRANSLATOR
#include "../shaders/glsl_translator_hdr.h"
#endif

//#define DEBUG_GLSL_TRANSLATOR // Define this to enable logging of GLSL translator output prior compilation
#define MAX_CG_TEXCOORD_ID 10 // Maximum number of bindable TEXCOORD semantic
#define MAX_CG_COLOR_ID 2 // Maximum number of bindable COLOR semantic
#define MAX_CUSTOM_BINDINGS 64 // Maximum number of custom semantic bindings usable with vglAddSemanticBinding

typedef struct {
	char name[64];
	int idx;
	int ref_idx;
	GLenum type;
} glsl_sema_bind;

extern glsl_sema_bind glsl_custom_bindings[MAX_CUSTOM_BINDINGS];
extern int glsl_custom_bindings_num;
extern int glsl_current_ref_idx;
extern char glsl_texcoords_binds[MAX_CG_TEXCOORD_ID][64];
extern char glsl_colors_binds[MAX_CG_COLOR_ID][64];
extern GLboolean glsl_texcoords_used[MAX_CG_TEXCOORD_ID];
extern GLboolean glsl_colors_used[MAX_CG_COLOR_ID];
extern GLboolean glsl_is_first_shader;
extern GLboolean glsl_precision_low;
extern GLenum glsl_sema_mode;
extern GLenum prev_shader_type;

#define glsl_get_existing_texcoord_bind(idx, s) \
	for (int j = 0; j < MAX_CG_TEXCOORD_ID; j++) { \
		if (glsl_texcoords_used[j] && !strcmp(glsl_texcoords_binds[j], s)) { \
			idx = j; \
			break; \
		} \
	}
	
#define glsl_get_existing_color_bind(idx, s) \
	for (int j = 0; j < MAX_CG_COLOR_ID; j++) { \
		if (glsl_colors_used[j] && !strcmp(glsl_colors_binds[j], s)) { \
			idx = j; \
			break; \
		} \
	}

#define glsl_reserve_texcoord_bind(idx, s) \
	for (int j = 0; j < MAX_CG_TEXCOORD_ID; j++) { \
		if (!glsl_texcoords_used[j]) { \
			glsl_texcoords_used[j] = GL_TRUE; \
			strcpy(glsl_texcoords_binds[j], s); \
			idx = j; \
			break; \
		} \
	}
	
#define glsl_reserve_color_bind(idx, s) \
	for (int j = 0; j < MAX_CG_COLOR_ID; j++) { \
		if (!glsl_colors_used[j]) { \
			glsl_colors_used[j] = GL_TRUE; \
			strcpy(glsl_colors_binds[j], s); \
			idx = j; \
			break; \
		} \
	}

#define glsl_replace_marker(m, r) \
	type = strstr(txt + strlen(glsl_hdr), m); \
	while (type) { \
		char *res = (char *)vglMalloc(1024 * 1024); \
		type[0] = 0; \
		strcpy(res, txt); \
		strcat(res, r); \
		strcat(res, type + 1); \
		strcpy(out, res); \
		vgl_free(res); \
		txt = out; \
		type = strstr(txt + strlen(glsl_hdr), m); \
	}

void glsl_translate_with_shader_pair(char *text, GLenum type, GLboolean hasFrontFacing);
void glsl_translate_with_global(char *text, GLenum type, GLboolean hasFrontFacing);

void glsl_handle_globals(char *txt, char *out);
void glsl_inject_mul(char *txt, char *out);
void glsl_nuke_comments(char *txt);

void glsl_translator_process(shader *s, GLsizei count, const GLchar *const *string, const GLint *length);

#endif
