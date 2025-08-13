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
 * glsl_utils.c:
 * Implementation for the GLSL to CG translator
 */

#define _GNU_SOURCE
#include <string.h>
#include "../shared.h"
#include "glsl_utils.h"
#include "preprocessor/preprocessor_c.h"

#ifdef HAVE_GLSL_TRANSLATOR
#define MEM_ENLARGER_SIZE (1024 * 1024) // FIXME: Check if this is too big/small

glsl_sema_bind glsl_custom_bindings[MAX_CUSTOM_BINDINGS];
int glsl_custom_bindings_num = 0;
int glsl_current_ref_idx = 0;
char glsl_texcoords_binds[MAX_CG_TEXCOORD_ID][64];
char glsl_colors_binds[MAX_CG_COLOR_ID][64];
GLboolean glsl_texcoords_used[MAX_CG_TEXCOORD_ID];
GLboolean glsl_colors_used[MAX_CG_COLOR_ID];
GLboolean glsl_is_first_shader = GL_TRUE;
GLboolean glsl_precision_low = GL_FALSE;
GLenum glsl_sema_mode = VGL_MODE_GLOBAL;
GLenum prev_shader_type = GL_NONE;

void glsl_translate_with_shader_pair(char *text, GLenum type, GLboolean hasFrontFacing) {
	char newline[128];
	int idx;
	if (type == GL_VERTEX_SHADER) {
		// Manually patching attributes and varyings
		char *str = strstr(text, "attribute");
		while (str && !(str[9] == ' ' || str[9] == '\t')) {
			str = strstr(str + 9, "attribute");
		}
		char *str2 = strstr(text, "varying");
		while (str2 && !(str2[7] == ' ' || str2[7] == '\t')) {
			str2 = strstr(str2 + 7, "varying");
		}
		while (str || str2) {
			char *t;
			if (!str)
				t = str2;
			else if (!str2)
				t = str;
			else
				t = min(str, str2);
			if (t == str) { // Attribute
				// Replace attribute with 'vgl in' that will get extended in a 'varying in' by the preprocessor
				vgl_fast_memcpy(t, "vgl in    ", 10);
				str = strstr(t, "attribute");
				while (str && !(str[9] == ' ' || str[9] == '\t')) {
					str = strstr(str + 9, "attribute");
				}
			} else { // Varying
				char *end = strstr(t, ";");
				GLboolean name_started = GL_FALSE;
				int extra_chars = -1;
				char *start = end;
				while ((*start != ' ' && *start != '\t') || !name_started) {
					if (!name_started && *start != ' ' && *start != '\t' && *start != ';')
						name_started = GL_TRUE;
					if (!name_started) {
						end--;
						extra_chars++;
					}
					start--;
				}
				end++;
				start++;
				end[0] = 0;
				idx = -1;
				vglSemanticType hint_type = VGL_TYPE_TEXCOORD;
				// Check first if the varying has a known binding
				for (int j = 0; j < glsl_custom_bindings_num; j++) {
					if (!strcmp(glsl_custom_bindings[j].name, start))
						idx = j;
				}
				if (idx != -1) {
					switch (glsl_custom_bindings[idx].type) {
					case VGL_TYPE_TEXCOORD:
						{
							if (glsl_custom_bindings[idx].idx != -1) {
								strcpy(glsl_texcoords_binds[glsl_custom_bindings[idx].idx], start);
								glsl_texcoords_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
								sprintf(newline, "VOUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
							} else {
								goto HINT_DETECTION_PAIR;
							}
						}
						break;
					case VGL_TYPE_COLOR:
						if (glsl_custom_bindings[idx].idx != -1) {
							strcpy(glsl_colors_binds[glsl_custom_bindings[idx].idx], start);
							glsl_colors_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
							sprintf(newline, "COUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						} else {
							hint_type = VGL_TYPE_COLOR;
							goto HINT_DETECTION_PAIR;							
						}
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FOUT(%s,%d);", str2 + 8, 0);
						break;
					case VGL_TYPE_CLIP:
						sprintf(newline, "POUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					}
				} else {
HINT_DETECTION_PAIR:
					idx = -1;
					if (glsl_is_first_shader) {
						// Check if varying has been already bound (eg: a varying that changes in size depending on preprocessor if)
						if (hint_type == VGL_TYPE_TEXCOORD) {
							glsl_get_existing_texcoord_bind(idx, start);
						} else if (hint_type == VGL_TYPE_COLOR) {
							glsl_get_existing_color_bind(idx, start);
						}
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								if (hint_type == VGL_TYPE_TEXCOORD) {
									sprintf(newline, "VOUT(%s,\v);", str2 + 8);
								} else if (hint_type == VGL_TYPE_COLOR) {
									sprintf(newline, "COUT(%s,\f);", str2 + 8);
								}
							} else {
								if (hint_type == VGL_TYPE_TEXCOORD) {
									glsl_reserve_texcoord_bind(idx, start);
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_TEXCOORD_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
								} else if (hint_type == VGL_TYPE_COLOR) {
									glsl_reserve_color_bind(idx, start);
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_COLOR_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (COLOR overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									sprintf(newline, "COUT(%s,%d);", str2 + 8, idx);							
								}
							}
						} else {
							if (hint_type == VGL_TYPE_TEXCOORD) {
								sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
							} else if (hint_type == VGL_TYPE_COLOR) {
								sprintf(newline, "COUT(%s,%d);", str2 + 8, idx);
							}
						}
					} else {
						if (hint_type == VGL_TYPE_TEXCOORD) {
							glsl_get_existing_texcoord_bind(idx, start);
						} else if (hint_type == VGL_TYPE_COLOR) {
							glsl_get_existing_color_bind(idx, start);							
						}
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								if (hint_type == VGL_TYPE_TEXCOORD) {
									sprintf(newline, "VOUT(%s,\v);", str2 + 8);
								} else if (hint_type == VGL_TYPE_COLOR) {
									sprintf(newline, "COUT(%s,\f);", str2 + 8);
								}
							} else {
								if (hint_type == VGL_TYPE_TEXCOORD) {
									glsl_reserve_texcoord_bind(idx, start)
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_TEXCOORD_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									vgl_log("%s:%d %s: Unexpected varying (%s), forcing binding to TEXCOORD%d.\n", __FILE__, __LINE__, __func__, start, idx);
									sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
								} else if (hint_type == VGL_TYPE_COLOR) {
									glsl_reserve_color_bind(idx, start)
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_COLOR_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (COLOR overflow).\n", __FILE__, __LINE__, __func__);	
									}
#endif
									vgl_log("%s:%d %s: Unexpected varying (%s), forcing binding to COLOR%d.\n", __FILE__, __LINE__, __func__, start, idx);
									sprintf(newline, "COUT(%s,%d);", str2 + 8, idx);	
								}
							}
						} else {
							if (hint_type == VGL_TYPE_TEXCOORD) {
								sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
							} else if (hint_type == VGL_TYPE_COLOR) {
								sprintf(newline, "COUT(%s,%d);", str2 + 8, idx);
							}
						}
					}
				}
				vgl_fast_memcpy(str2, newline, strlen(newline));
				if (extra_chars) {
					vgl_memset(str2 + strlen(newline), ' ', extra_chars);
				}
				str2 = strstr(t, "varying");
				while (str2 && !(str2[7] == ' ' || str2[7] == '\t')) {
					str2 = strstr(str2 + 7, "varying");
				}
			}
		}
	} else {
		// Manually patching gl_FrontFacing usage
		if (hasFrontFacing) {
			char *str = strstr(text, "gl_FrontFacing");
			while (str) {
				vgl_fast_memcpy(str, "(vgl_Face > 0)", 14);
				str = strstr(str, "gl_FrontFacing");
			}
		}
		// Manually patching varyings and "texture" uniforms
		char *str = strstr(text, "varying");
		while (str && !(str[7] == ' ' || str[7] == '\t')) {
			str = strstr(str + 1, "varying");
		}
		char *str2 = strcasestr(text, "texture");
		while (str2) {
			char *str2_end = str2 + 7;
			if (*(str2 - 1) == ' ' || *(str2 - 1) == '\t' || *(str2 - 1) == '(') {
				while (*str2_end == ' ' || *str2_end == '\t') {
					str2_end++;
				}
				if (*str2_end == ',' || *str2_end == ';')
					break;
			}
			str2 = strcasestr(str2_end, "texture");
		}
		while (str || str2) {
			char *t;
			if (!str)
				t = str2;
			else if (!str2)
				t = str;
			else
				t = min(str, str2);
			if (t == str) { // Varying
				char *end = strstr(str, ";");
				GLboolean name_started = GL_FALSE;
				int extra_chars = -1;
				char *start = end;
				while ((*start != ' ' && *start != '\t') || !name_started) {
					if (!name_started && *start != ' ' && *start != '\t' && *start != ';')
						name_started = GL_TRUE;
					if (!name_started) {
						end--;
						extra_chars++;
					}
					start--;
				}
				end++;
				start++;
				end[0] = 0;
				idx = -1;
				vglSemanticType hint_type = VGL_TYPE_TEXCOORD;
				// Check first if the varying has a known binding
				for (int j = 0; j < glsl_custom_bindings_num; j++) {
					if (!strcmp(glsl_custom_bindings[j].name, start)) {
						idx = j;
					}
				}
				if (idx != -1) {
					switch (glsl_custom_bindings[idx].type) {
					case VGL_TYPE_TEXCOORD:
						if (glsl_custom_bindings[idx].idx != -1) {
							strcpy(glsl_texcoords_binds[glsl_custom_bindings[idx].idx], start);
							glsl_texcoords_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
							sprintf(newline, "VIN(%s, %d);", str + 8, glsl_custom_bindings[idx].idx);
						} else {
							goto HINT_DETECTION_PAIR_2;
						}
						break;
					case VGL_TYPE_COLOR:
						if (glsl_custom_bindings[idx].idx != -1) {
							strcpy(glsl_colors_binds[glsl_custom_bindings[idx].idx], start);
							glsl_colors_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
							sprintf(newline, "CIN(%s, %d);", str + 8, glsl_custom_bindings[idx].idx);
						} else {
							hint_type = VGL_TYPE_COLOR;
							goto HINT_DETECTION_PAIR_2;
						}
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FIN(%s, %d);", str + 8, 0);
						break;
					case VGL_TYPE_CLIP:
						vgl_log("%s:%d %s: Unexpected varying type (VGL_TYPE_CLIP) for %s in fragment shader.\n", __FILE__, __LINE__, __func__, str + 8);
						break;
					}
				} else {
HINT_DETECTION_PAIR_2:
					idx = -1;
					if (glsl_is_first_shader) {
						// Check if varying has been already bound (eg: a varying that changes in size depending on preprocessor if)
						if (hint_type == VGL_TYPE_TEXCOORD) {
							glsl_get_existing_texcoord_bind(idx, start);
						} else {
							glsl_get_existing_color_bind(idx, start);							
						}
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								if (hint_type == VGL_TYPE_TEXCOORD) {
									sprintf(newline, "VIN(%s, \v);", str + 8);
								} else if (hint_type == VGL_TYPE_COLOR) {
									sprintf(newline, "CIN(%s, \f);", str + 8);									
								}
							} else {
								if (hint_type == VGL_TYPE_TEXCOORD) {
									glsl_reserve_texcoord_bind(idx, start);
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_TEXCOORD_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									sprintf(newline, "VIN(%s, %d);", str + 8, idx);
								} else if (hint_type == VGL_TYPE_COLOR) {
									glsl_reserve_color_bind(idx, start);
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_COLOR_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (COLOR overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									sprintf(newline, "CIN(%s, %d);", str + 8, idx);
								}
								
							}
						} else {
							if (hint_type == VGL_TYPE_TEXCOORD) {
								sprintf(newline, "VIN(%s, %d);", str + 8, idx);
							} else if (hint_type == VGL_TYPE_COLOR) {
								sprintf(newline, "CIN(%s, %d);", str + 8, idx);
							}
						}
					} else {
						if (hint_type == VGL_TYPE_TEXCOORD) {
							glsl_get_existing_texcoord_bind(idx, start);
							if (idx == -1) {
								if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
									sprintf(newline, "VIN(%s, \v);", str + 8);
								} else {
									glsl_reserve_texcoord_bind(idx, start)
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_TEXCOORD_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									vgl_log("%s:%d %s: Unexpected varying (%s), forcing binding to TEXCOORD%d.\n", __FILE__, __LINE__, __func__, start, idx);
									sprintf(newline, "VIN(%s, %d);", str + 8, idx);
								}
							} else
								sprintf(newline, "VIN(%s, %d);", str + 8, idx);
						} else if (hint_type == VGL_TYPE_COLOR) {
							glsl_get_existing_color_bind(idx, start);
							if (idx == -1) {
								if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
									sprintf(newline, "CIN(%s, \f);", str + 8);
								} else {
									glsl_reserve_texcoord_bind(idx, start)
#ifndef SKIP_ERROR_HANDLING
									if (idx == -1) {
										idx = MAX_CG_COLOR_ID - 1;
										vgl_log("%s:%d %s: An error occurred during GLSL translation (COLOR overflow).\n", __FILE__, __LINE__, __func__);
									}
#endif
									vgl_log("%s:%d %s: Unexpected varying (%s), forcing binding to COLOR%d.\n", __FILE__, __LINE__, __func__, start, idx);
									sprintf(newline, "CIN(%s, %d);", str + 8, idx);
								}
							} else
								sprintf(newline, "CIN(%s, %d);", str + 8, idx);
						}
					}
				}
				vgl_fast_memcpy(str, newline, strlen(newline));
				if (extra_chars > 0) {
					vgl_memset(str + strlen(newline), ' ', extra_chars);
				}
				str = strstr(str, "varying");
				while (str && !(str[7] == ' ' || str[7] == '\t')) {
					str = strstr(str + 7, "varying");
				}
			} else { // "texture" Uniform
				if (t[0] == 't')
					vgl_fast_memcpy(t, "vgl_tex", 7);
				else
					vgl_fast_memcpy(t, "Vgl_tex", 7);
				str2 = strcasestr(t, "texture");
				while (str2) {
					char *str2_end = str2 + 7;
					if (*(str2 - 1) == ' ' || *(str2 - 1) == '\t' || *(str2 - 1) == '(') {
						while (*str2_end == ' ' || *str2_end == '\t') {
							str2_end++;
						}
						if (*str2_end == ',' || *str2_end == ';')
							break;
					}
					str2 = strcasestr(str2_end, "texture");
				}
			}
		}
	}
}

void glsl_translate_with_global(char *text, GLenum type, GLboolean hasFrontFacing) {
	char newline[128];
	int idx;
	if (type == GL_VERTEX_SHADER) {
		// Manually patching attributes and varyings
		char *str = strstr(text, "attribute");
		while (str && !(str[9] == ' ' || str[9] == '\t')) {
			str = strstr(str + 9, "attribute");
		}
		char *str2 = strstr(text, "varying");
		while (str2 && !(str2[7] == ' ' || str2[7] == '\t')) {
			str2 = strstr(str2 + 7, "varying");
		}
		while (str || str2) {
			char *t;
			if (!str)
				t = str2;
			else if (!str2)
				t = str;
			else
				t = min(str, str2);
			if (t == str) { // Attribute
				// Replace attribute with 'vgl in' that will get extended in a 'varying in' by the preprocessor
				vgl_fast_memcpy(t, "vgl in    ", 10);
				str = strstr(t, "attribute");
				while (str && !(str[9] == ' ' || str[9] == '\t')) {
					str = strstr(str + 9, "attribute");
				}
			} else { // Varying
				char *end = strstr(t, ";");
				GLboolean name_started = GL_FALSE;
				int extra_chars = -1;
				char *start = end;
				while ((*start != ' ' && *start != '\t') || !name_started) {
					if (!name_started && *start != ' ' && *start != '\t' && *start != ';')
						name_started = GL_TRUE;
					if (!name_started) {
						end--;
						extra_chars++;
					}
					start--;
				}
				end++;
				start++;
				end[0] = 0;
				idx = -1;
				// Check first if the varying has a known binding
				for (int j = 0; j < glsl_custom_bindings_num; j++) {
					if (!strcmp(glsl_custom_bindings[j].name, start)) {
						glsl_custom_bindings[j].ref_idx = glsl_current_ref_idx;
						idx = j;
					}
				}
				if (idx != -1) {
					switch (glsl_custom_bindings[idx].type) {
					case VGL_TYPE_TEXCOORD:
						{
							if (glsl_custom_bindings[idx].idx != -1) {
								strcpy(glsl_texcoords_binds[glsl_custom_bindings[idx].idx], start);
								glsl_texcoords_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
								sprintf(newline, "VOUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
							} else {
								sprintf(newline, "VOUT(%s,\v);", str2 + 8);
							}
						}
						break;
					case VGL_TYPE_COLOR:
						{
							if (glsl_custom_bindings[idx].idx != -1) {
								strcpy(glsl_colors_binds[glsl_custom_bindings[idx].idx], start);
								glsl_colors_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
								sprintf(newline, "COUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
							} else {
								sprintf(newline, "COUT(%s,\f);", str2 + 8);
							}
						}
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FOUT(%s,%d);", str2 + 8, 0);
						break;
					case VGL_TYPE_CLIP:
						sprintf(newline, "POUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					}
				} else {
					sprintf(newline, "VOUT(%s,\v);", str2 + 8);
				}
				vgl_fast_memcpy(str2, newline, strlen(newline));
				if (extra_chars > 0) {
					vgl_memset(str2 + strlen(newline), ' ', extra_chars);
				}
				str2 = strstr(t, "varying");
				while (str2 && !(str2[7] == ' ' || str2[7] == '\t')) {
					str2 = strstr(str2 + 7, "varying");
				}
			}
		}
	} else {
		// Manually patching gl_FrontFacing usage
		if (hasFrontFacing) {
			char *str = strstr(text, "gl_FrontFacing");
			while (str) {
				vgl_fast_memcpy(str, "(vgl_Face > 0)", 14);
				str = strstr(str, "gl_FrontFacing");
			}
		}
		// Manually patching varyings and "texture" uniforms
		char *str = strstr(text, "varying");
		while (str && !(str[7] == ' ' || str[7] == '\t')) {
			str = strstr(str + 1, "varying");
		}
		char *str2 = strcasestr(text, "texture");
		while (str2) {
			char *str2_end = str2 + 7;
			if (*(str2 - 1) == ' ' || *(str2 - 1) == '\t' || *(str2 - 1) == '(') {
				while (*str2_end == ' ' || *str2_end == '\t') {
					str2_end++;
				}
				if (*str2_end == ',' || *str2_end == ';')
					break;
			}
			str2 = strcasestr(str2_end, "texture");
		}
		while (str || str2) {
			char *t;
			if (!str)
				t = str2;
			else if (!str2)
				t = str;
			else
				t = min(str, str2);
			if (t == str) { // Varying
				char *end = strstr(str, ";");
				GLboolean name_started = GL_FALSE;
				int extra_chars = -1;
				char *start = end;
				while ((*start != ' ' && *start != '\t') || !name_started) {
					if (!name_started && *start != ' ' && *start != '\t' && *start != ';')
						name_started = GL_TRUE;
					if (!name_started) {
						end--;
						extra_chars++;
					}
					start--;
				}
				end++;
				start++;
				end[0] = 0;
				idx = -1;
				// Check first if the varying has a known binding
				for (int j = 0; j < glsl_custom_bindings_num; j++) {
					if (!strcmp(glsl_custom_bindings[j].name, start)) {
						glsl_custom_bindings[j].ref_idx = glsl_current_ref_idx;
						idx = j;
					}
				}
				if (idx != -1) {
					switch (glsl_custom_bindings[idx].type) {
					case VGL_TYPE_TEXCOORD:
						{
							if (glsl_custom_bindings[idx].idx != -1) {
								strcpy(glsl_texcoords_binds[glsl_custom_bindings[idx].idx], start);
								glsl_texcoords_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
								sprintf(newline, "VIN(%s, %d);", str + 8, glsl_custom_bindings[idx].idx);
							} else {
								sprintf(newline, "VIN(%s, \v);", str + 8);
							}
						}
						break;
					case VGL_TYPE_COLOR:
						{
							if (glsl_custom_bindings[idx].idx != -1) {
								strcpy(glsl_colors_binds[glsl_custom_bindings[idx].idx], start);
								glsl_colors_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
								sprintf(newline, "CIN(%s, %d);", str + 8, glsl_custom_bindings[idx].idx);
							} else {
								sprintf(newline, "CIN(%s, \f);", str + 8);
							}
						}
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FIN(%s, %d);", str + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_CLIP:
						vgl_log("%s:%d %s: Unexpected varying type (VGL_TYPE_CLIP) for %s in fragment shader.\n", __FILE__, __LINE__, __func__, str + 8);
						break;
					}
				} else {
					sprintf(newline, "VIN(%s, \v);", str + 8);
				}
				vgl_fast_memcpy(str, newline, strlen(newline));
				if (extra_chars > 0) {
					vgl_memset(str + strlen(newline), ' ', extra_chars);
				}
				str = strstr(str, "varying");
				while (str && !(str[7] == ' ' || str[7] == '\t')) {
					str = strstr(str + 7, "varying");
				}
			} else { // "texture" Uniform
				if (t[0] == 't')
					vgl_fast_memcpy(t, "vgl_tex", 7);
				else
					vgl_fast_memcpy(t, "Vgl_tex", 7);
				str2 = strcasestr(t, "texture");
				while (str2) {
					char *str2_end = str2 + 7;
					if (*(str2 - 1) == ' ' || *(str2 - 1) == '\t' || *(str2 - 1) == '(') {
						while (*str2_end == ' ' || *str2_end == '\t') {
							str2_end++;
						}
						if (*str2_end == ',' || *str2_end == ';')
							break;
					}
					str2 = strcasestr(str2_end, "texture");
				}
			}
		}
	}
}

/* 
 * Experimental function to add static keyword to all global variables:
 * The idea behind this is to check if a variable falls outside of a function and, if so,
 * add to it static keyword only if not uniform. This is required cause CG handles
 * global variables by default as uniforms.
 */
void glsl_handle_globals(char *txt, char *out) {
	char *src = txt;
	out[0] = 0;
	char *type = txt + strlen(glsl_hdr);
	char *last_func_start = strstr(txt + strlen(glsl_hdr), "{");
	char *last_func_end = strstr(last_func_start, "}");
	// First pass: marking all global variables
	while (type) {
		while (*type == ' ' || *type == '\t' || *type == '\r' || *type == '\n') {
			type++;
		}
		if (*type == 0)
			break;
		if (!strncmp(type, "float", 5) ||
		    !strncmp(type, "int", 3) || 
		    !strncmp(type, "vec", 3) ||
		    !strncmp(type, "ivec", 4) ||
		    !strncmp(type, "mat", 3) ||
		    !strncmp(type, "const", 5)
		) {
			char *var_end = strstr(type, ";");
HANDLE_VAR:
			if (last_func_start && last_func_end && type > last_func_start && var_end < last_func_end) { // Var is inside a function, skipping
				type = strstr(type, "}");
				if (type)
					type++;
			} else if (last_func_end && type > last_func_end) { // Var is after last function, need to update last function
				last_func_start = strstr(last_func_start + 1, "{");
				last_func_end = strstr(last_func_end + 1, "}");
				goto HANDLE_VAR;
			} else if (var_end < last_func_start || !last_func_start) { // Var is prior a function, handling it
				type[0] = '\v';
				type = var_end + 1;
			} else { // Var is a function, skipping
				type = var_end + 1;
			}
		} else {
			type = strstr(type, ";");
			if (type)
				type++;
		}
	}
	// Second pass: replacing all marked variables
	glsl_replace_marker("\vloat", "static f");
	glsl_replace_marker("\vnt", "static i");
	glsl_replace_marker("\vec", "static v");
	glsl_replace_marker("\vvec", "static i");
	glsl_replace_marker("\vat", "static m");
	glsl_replace_marker("\vonst", "static c");
	
	if (strlen(out) == 0)
		strcpy(out, src);
}

/* 
 * Experimental function to replace all multiplication operators in a GLSL shader code:
 * The idea behind this is to replace all operators with a function call (vglMul)
 * which is an overloaded inlined function properly adding support for matrix * vector
 * and vector * matrix operations. This implementation is very likely non exhaustive
 * since, for a proper implementation, ideally we'd want a proper GLSL parser.
 */
void glsl_inject_mul(char *txt, char *out) {
	char *star = strstr(txt + strlen(glsl_hdr), "*");
	while (star) {
		if (star[1] == '=') // FIXME: *= still not handled
			star = strstr(star + 1, "*");
		else
			break;
	}
	if (!star) {
		strcpy(out, txt);
		return;
	}
	char *left;
LOOP_START:
	left = star - 1;
	int para_left = 0;
	int quad_para_left = 0;
	int found = 0;
	while (left != txt) {
		switch (*left) {
		case 'n':
			if (!strncmp(left - 5, "return", 6)) {
				if (left[1] == ' ' || left[1] == '\t')
					left++;
				found = 1;
			}
			break;
		case ' ':
		case '\t':
			break;
		case '[':
			quad_para_left--;
			if (quad_para_left < 0)
				found = 2;
			break;
		case ']':
			quad_para_left++;
			break;
		case ')':
			para_left++;
			break;
		case '(':
			para_left--;
			if (para_left < 0)
				found = 1;
			break;
		case '!':
		case '=':
			found = 1;
			break;
		case '>':
		case '<':
		case ',':
		case '+':
		case '?':
			if (para_left == 0 && quad_para_left == 0)
				found = 1;
			break;
		case '-':
			if ((*(left - 1) == 'E' || *(left - 1) == 'e') && *(left - 2) >= '0' && *(left - 2) <= '9')
				break;
			if (para_left == 0 && quad_para_left == 0)
				found = 1;
			break;
		default:
			break;
		}
		if (found) {
			left++;
			break;
		} else
			left--;
	}
	found = 0;
	char *right = star + 1;
	para_left = 0;
	int literal = 0;
	while (*right) {
		switch (*right) {
		case ' ':
		case '\t':
			break;
		case ']':
			quad_para_left--;
			if (quad_para_left < 0)
				found = 2;
			break;
		case '[':
			quad_para_left++;
			break;
		case '(':
			para_left++;
			break;
		case ')':
			para_left--;
			if (para_left < 0)
				found = 1;
			break;
		case '>':
		case '<':
		case ',':
		case '+':
		case ':':
		case '*':
			if (para_left == 0 && quad_para_left == 0)
				found = 1;
			break;
		case '-':
			if ((*(right - 1) == 'E' || *(right - 1) == 'e') && *(right - 2) >= '0' && *(right - 2) <= '9')
				break;
			if (para_left == 0 && quad_para_left == 0 && literal)
				found = 1;
			break;
		case ';':
			found = 1;
			break;
		default:
			literal = 1;
			break;
		}
		if (found)
			break;
		else
			right++;
	}
	char *res = (char *)vglMalloc(MEM_ENLARGER_SIZE);
	if (found < 2) { // Standard match
		char tmp = *left;
		left[0] = 0;
		strcpy(res, txt);
		left[0] = tmp;
		strcat(res, " vglMul(");
		tmp = *right;
		right[0] = 0;
		*star = ',';
		strcat(res, left);
		strcat(res, ")");
		right[0] = tmp;
		strcat(res, right);
		strcpy(out, res);
		vgl_free(res);
		txt = out;
		star = strstr(txt + strlen(glsl_hdr), "*");
	} else { // [ bracket match, we assume a matrix is not involved
		uint32_t jump = right - txt;
		strcpy(res, txt);
		strcpy(out, res);
		vgl_free(res);
		txt = out;
		star = strstr(txt + jump, "*");
	}
	while (star) {
		if (star[1] == '=') // FIXME: *= still not handled
			star = strstr(star + 1, "*");
		else
			goto LOOP_START;
	}
}

void glsl_nuke_comments(char *txt) {
	// Nuke C++ and C styled comments
	char *cpp_s = strstr(txt, "/*");
	char *c_s = strstr(txt, "//");
	while (cpp_s || c_s) {
		char *next;
		if (cpp_s) {
			next = (c_s && cpp_s > c_s) ? c_s : cpp_s;
		} else {
			next = c_s;
		}
		if (next == c_s) {
			// Nuke C styled comment
			char *end = strstr(next, "\n");
			if (!end)
				end = txt + strlen(txt);
			vgl_memset(next, ' ', end - next);
		} else {
			// Nuke C++ styled comment
			char *end = strstr(next, "*/") + 2;
			vgl_memset(next, ' ', end - next);
		}
		if (c_s)
			c_s = strstr(next, "//");
		if (cpp_s)
			cpp_s = strstr(next, "/*");
	}
}

void glsl_translator_process(shader *s, GLsizei count, const GLchar *const *string, const GLint *length) {
	uint32_t source_size = 1;
	uint32_t size = 1;
	GLboolean hasFragCoord = GL_FALSE, hasInstanceID = GL_FALSE, hasVertexID = GL_FALSE, hasPointCoord = GL_FALSE;
	GLboolean hasPointSize = GL_FALSE, hasFragDepth = GL_FALSE, hasFrontFacing = GL_FALSE;
	size += strlen(glsl_hdr);
	if (glsl_precision_low)
		size += strlen(glsl_precision_hdr);
#ifndef SKIP_ERROR_HANDLING
	if (glsl_sema_mode == VGL_MODE_SHADER_PAIR) {
		if (prev_shader_type == s->type) {
			vgl_log("%s:%d %s: Unexpected shader type, translation may be imperfect.\n", __FILE__, __LINE__, __func__);
			glsl_is_first_shader = GL_TRUE;
			vgl_memset(glsl_texcoords_used, 0, sizeof(GLboolean) * MAX_CG_TEXCOORD_ID);
			vgl_memset(glsl_colors_used, 0, sizeof(GLboolean) * MAX_CG_COLOR_ID);
		}
	} else
		glsl_current_ref_idx++;
#endif
	if (s->type == GL_VERTEX_SHADER)
		size += strlen("#define VGL_IS_VERTEX_SHADER\n");
	
	for (int i = 0; i < count; i++) {
		source_size += length ? length[i] : strlen(string[i]);
	}
	char *input = vglMalloc(source_size);
	input[0] = 0;
	for (int i = 0; i < count; i++) {
		strncat(input, string[i], length ? length[i] : strlen(string[i]));
	}
	
	// Nukeing version directive
	char *str = strstr(input, "#version");
	if (str) {
		str[0] = str[1] = '/';
	}

#if defined(DEBUG_GLSL_PREPROCESSOR) || defined(DEBUG_GLSL_TRANSLATOR)
	vgl_log("%s:%d %s: GLSL translation input:\n\n%s\n\n", __FILE__, __LINE__, __func__, input);
#endif

#ifdef HAVE_GLSL_PREPROCESSOR
	char *out = vglMalloc(strlen(input));
	glsl_preprocess("full", input, out);
	vglFree(input);
#ifdef DEBUG_GLSL_PREPROCESSOR
	vgl_log("%s:%d %s: GLSL preprocessor output:\n\n%s\n\n", __FILE__, __LINE__, __func__, out);
#endif
	size += strlen(out);
#else
	char *out = input;
	// Nukeing comments
	glsl_nuke_comments(out);
	size+= strlen(out);
#endif
	
	// Nukeing precision directives
	str = strstr(out, "precision ");
	while (str) {
		str[0] = ' ';
		str++;
		if (str[0] == ';') {
			str[0] = ' ';
			str = strstr(str, "precision ");
		}
	}
	
	// Replacing any gl_FragData[0] reference to gl_FragColor
	str = strstr(out, "gl_FragData[0]");
	while (str) {
		strcpy(str, "gl_FragColor");
		str[12] = str[13] = ' ';
		str = strstr(str, "gl_FragData[0]");
	}	
	
	if (s->type == GL_VERTEX_SHADER) {
		// Checking if shader requires gl_PointSize
		if (!hasPointSize)
			hasPointSize = strstr(out, "gl_PointSize") ? GL_TRUE : GL_FALSE;
		// Checking if shader requires gl_InstanceID
		if (!hasInstanceID)
			hasInstanceID = strstr(out, "gl_InstanceID") ? GL_TRUE : GL_FALSE;
		// Checking if shader requires gl_VertexID
		if (!hasVertexID)
			hasVertexID = strstr(out, "gl_VertexID") ? GL_TRUE : GL_FALSE;
	} else {
		// Checking if shader requires gl_PointCoord
		if (!hasPointCoord)
			hasPointCoord = strstr(out, "gl_PointCoord") ? GL_TRUE : GL_FALSE;
		// Checking if shader requires gl_FrontFacing
		if (!hasFrontFacing)
			hasFrontFacing = strstr(out, "gl_FrontFacing") ? GL_TRUE : GL_FALSE;
		// Checking if shader requires gl_FragCoord
		if (!hasFragCoord)
			hasFragCoord = strstr(out, "gl_FragCoord") ? GL_TRUE : GL_FALSE;
		// Checking if shader requires gl_FragDepth
		if (!hasFragDepth)
			hasFragDepth = strstr(out, "gl_FragDepth") ? GL_TRUE : GL_FALSE;
	}
	
	if (hasPointSize)
		size += strlen("varying out float gl_PointSize : PSIZE;\n");
	if (hasFrontFacing)
		size += strlen("varying in float vgl_Face : FACE;\n");
	if (hasFragCoord)
		size += strlen("varying in float4 gl_FragCoord : WPOS;\n");
	if (hasInstanceID)
		size += strlen("varying in int gl_InstanceID : INSTANCE;\n");
	if (hasVertexID)
		size += strlen("varying in int gl_VertexID : INDEX;\n");
	if (hasFragDepth)
		size += strlen("varying out float gl_FragDepth : DEPTH;\n");
	if (hasPointCoord)
		size += strlen("varying in float2 gl_PointCoord : SPRITECOORD;\n");
	
	s->source = (char *)vglMalloc(size);
	s->source[0] = 0;
	
	// Injecting GLSL to CG header
	if (s->type == GL_VERTEX_SHADER) {
		strcat(s->source, "#define VGL_IS_VERTEX_SHADER\n");
		if (hasPointSize)
			strcat(s->source, "varying out float gl_PointSize : PSIZE;\n");
		if (hasInstanceID)
			strcat(s->source, "varying in int gl_InstanceID : INSTANCE;\n");
		if (hasVertexID)
			strcat(s->source, "varying in int gl_VertexID : INDEX;\n");
	} else {
		if (hasFrontFacing)
			strcat(s->source, "varying in float vgl_Face : FACE;\n");
		if (hasFragCoord)
			strcat(s->source, "varying in float4 gl_FragCoord : WPOS;\n");
		if (hasFragDepth)
			strcat(s->source, "varying out float gl_FragDepth : DEPTH;\n");
		if (hasPointCoord)
			strcat(s->source, "varying in float2 gl_PointCoord : SPRITECOORD;\n");
	}
	strcat(s->source, glsl_hdr);
	if (glsl_precision_low)
		strcat(s->source, glsl_precision_hdr);
	
	char *text = s->source + strlen(s->source);
	strcat(s->source, out);
	vglFree(out);

	switch (glsl_sema_mode) {
		case VGL_MODE_SHADER_PAIR:
			glsl_translate_with_shader_pair(text, s->type, hasFrontFacing);
			break;
		case VGL_MODE_GLOBAL:
			glsl_translate_with_global(text, s->type, hasFrontFacing);
			break;
		default:
			vgl_log("%s:%d %s: Invalid semantic binding resolution mode supplied.\n", __FILE__, __LINE__, __func__);
			break;
	}
	
	// Replacing all marked varying with actual bindings if custom bindings are used
	if (glsl_custom_bindings_num > 0 || glsl_sema_mode == VGL_MODE_GLOBAL) {
		// Texcoords
		char *str = strstr(s->source, "\v");
		while (str) {
			char *start = str;
			while (*start != ',') {
				start--;
			}
			char *end = start;
			while (*start != ' ' && *start != '\t') {
				start--;
			}
			start++;
			int idx = -1;
			*end = 0;
			if (glsl_sema_mode == VGL_MODE_GLOBAL) {
				for (int j = 0; j < MAX_CG_TEXCOORD_ID; j++) {
					idx = j;
					for (int i = 0; i < glsl_custom_bindings_num; i++) {
						// Check if amongst the currently known bindings, used in the shader, there's one mapped to the attempted index
						if (glsl_custom_bindings[i].type == VGL_TYPE_TEXCOORD && glsl_custom_bindings[i].idx == j && glsl_custom_bindings[i].ref_idx == glsl_current_ref_idx) {
							idx = -1;
							break;
						}
					}
					if (idx != -1)
						break;
				}
				if (idx != -1)
					vglAddSemanticBinding(start, idx, VGL_TYPE_TEXCOORD);
			} else {
				glsl_reserve_texcoord_bind(idx, start);
			}
			*end = ',';
#ifndef SKIP_ERROR_HANDLING
			if (idx == -1) {
				idx = MAX_CG_TEXCOORD_ID - 1;
				vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
			}
#endif
			*str = '0' + idx;
			str = strstr(str, "\v");
		}
		// Colors
		str = strstr(s->source, "\f");
		while (str) {
			char *start = str;
			while (*start != ',') {
				start--;
			}
			char *end = start;
			while (*start != ' ' && *start != '\t') {
				start--;
			}
			start++;
			int idx = -1;
			*end = 0;
			if (glsl_sema_mode == VGL_MODE_GLOBAL) {
				for (int j = 0; j < MAX_CG_COLOR_ID; j++) {
					idx = j;
					for (int i = 0; i < glsl_custom_bindings_num; i++) {
						// Check if amongst the currently known bindings, used in the shader, there's one mapped to the attempted index
						if (glsl_custom_bindings[i].type == VGL_TYPE_COLOR && glsl_custom_bindings[i].idx == j && glsl_custom_bindings[i].ref_idx == glsl_current_ref_idx) {
							idx = -1;
							break;
						}
					}
					if (idx != -1)
						break;
				}
				if (idx != -1)
					vglAddSemanticBinding(start, idx, VGL_TYPE_COLOR);
			} else {
				glsl_reserve_color_bind(idx, start);
			}
			*end = ',';
#ifndef SKIP_ERROR_HANDLING
			if (idx == -1) {
				idx = MAX_CG_COLOR_ID - 1;
				vgl_log("%s:%d %s: An error occurred during GLSL translation (COLOR overflow).\n", __FILE__, __LINE__, __func__);
			}
#endif
			*str = '0' + idx;
			str = strstr(str, "\f");
		}
	}
	// Manually handle * operator replacements for vector * matrix and matrix * vector operations support
	char *dst = vglMalloc(size + MEM_ENLARGER_SIZE); // FIXME: This is just an estimation, check if 1MB is enough/too big
	glsl_inject_mul(s->source, dst);
	vgl_free(s->source);
	// Manually handle global variables, adding "static" to them
	char *dst2 = vglMalloc(strlen(dst) + MEM_ENLARGER_SIZE); // FIXME: This is just an estimation, check if 1MB is enough/too big
	glsl_handle_globals(dst, dst2);
	vgl_free(dst);
	// Keeping on mem only the strict minimum necessary for the translated shader
	char *final = vglMalloc(strlen(dst2) + 1);
	vgl_fast_memcpy(final, dst2, strlen(dst2) + 1);
	vgl_free(dst2);
	s->source = final;
#ifdef DEBUG_GLSL_TRANSLATOR
	vgl_log("%s:%d %s: GLSL translation output (%s shader):\n\n%s\n\n", __FILE__, __LINE__, __func__, glsl_is_first_shader ? "first" : "second", s->source);
#endif
	s->prog = (SceGxmProgram *)s->source;

	if (glsl_sema_mode == VGL_MODE_SHADER_PAIR) {
		glsl_is_first_shader = !glsl_is_first_shader;
		if (glsl_is_first_shader)
			vgl_memset(glsl_texcoords_used, 0, sizeof(GLboolean) * MAX_CG_TEXCOORD_ID);
	}
	s->size = strlen(s->source);
}
#endif
