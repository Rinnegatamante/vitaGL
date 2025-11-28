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
extern GLboolean glsl_is_first_shader;
extern GLboolean glsl_precision_low;
extern GLenum glsl_sema_mode;
extern GLenum prev_shader_type;

void glsl_translate_with_shader_pair(char *text, GLenum type, GLboolean hasFrontFacing);
void glsl_translate_with_global(char *text, GLenum type, GLboolean hasFrontFacing);

void glsl_inject_mul(char *txt, char *out);
void glsl_nuke_comments(char *txt);

void glsl_translator_process(shader *s);
void glsl_translator_set_process(shader *vs, shader *fs);

#endif
