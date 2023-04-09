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
 
#include "../shared.h"
#include "glsl_utils.h"

#ifdef HAVE_GLSL_TRANSLATOR
glsl_sema_bind glsl_custom_bindings[MAX_CUSTOM_BINDINGS];
int glsl_custom_bindings_num = 0;
int glsl_current_ref_idx = 0;
char glsl_texcoords_binds[MAX_CG_TEXCOORD_ID][64];
GLboolean glsl_texcoords_used[MAX_CG_TEXCOORD_ID];
GLboolean glsl_is_first_shader = GL_TRUE;
GLboolean glsl_precision_low = GL_FALSE;
GLenum glsl_sema_mode = VGL_MODE_SHADER_PAIR;
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
				sceClibMemcpy(t, "vgl in    ", 10);
				str = strstr(t, "attribute");
				while (str && !(str[9] == ' ' || str[9] == '\t')) {
					str = strstr(str + 9, "attribute");
				}
			} else { // Varying
				char *end = strstr(t, ";");
				char *start = end;
				while (*start != ' ' && *start != '\t') {
					start--;
				}
				start++;
				end[0] = 0;
				idx = -1;
				// Check first if the varying has a known binding
				for (int j = 0; j < glsl_custom_bindings_num; j++) {
					if (!strcmp(glsl_custom_bindings[j].name, start)) {
						idx = j;
					}
				}
				if (idx != -1) {
					switch (glsl_custom_bindings[idx].type) {
					case VGL_TYPE_TEXCOORD:
						strcpy(glsl_texcoords_binds[glsl_custom_bindings[idx].idx], start);
						glsl_texcoords_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
						sprintf(newline, "VOUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_COLOR:
						sprintf(newline, "COUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FOUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					}
				} else {
					if (glsl_is_first_shader) {
						// Check if varying has been already bound (eg: a varying that changes in size depending on preprocessor if)
						glsl_get_existing_texcoord_bind(idx, start);
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								sprintf(newline, "VOUT(%s,\v);", str2 + 8);
							} else {
								glsl_reserve_texcoord_bind(idx, start);
#ifndef SKIP_ERROR_HANDLING
								if (idx == -1) {
									idx = 9;
									vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
								}
#endif
								sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
							}
						} else
							sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);							
					} else {
						// FIXME: We rely on the fact shaders are always compiled in couples (fragment+vertex) to ensure proper semantic bindings coherence
						glsl_get_existing_texcoord_bind(idx, start);
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								sprintf(newline, "VOUT(%s,\v);", str2 + 8);
							} else {
								glsl_reserve_texcoord_bind(idx, start)
#ifndef SKIP_ERROR_HANDLING
								if (idx == -1) {
									idx = 9;
									vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
								}
#endif
								vgl_log("%s:%d %s: Unexpected varying (%s), forcing binding to TEXCOORD%d.\n", __FILE__, __LINE__, __func__, start, idx);
								sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
							}
						} else
							sprintf(newline, "VOUT(%s,%d);", str2 + 8, idx);
					}
				}
				sceClibMemcpy(str2, newline, strlen(newline));
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
				sceClibMemcpy(str, "(vgl_Face > 0)", 14);
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
				char *start = end;
				while (*start != ' ' && *start != '\t') {
					start--;
				}
				start++;
				end[0] = 0;
				idx = -1;
				// Check first if the varying has a known binding
				for (int j = 0; j < glsl_custom_bindings_num; j++) {
					if (!strcmp(glsl_custom_bindings[j].name, start)) {
						idx = j;
					}
				}
				if (idx != -1) {
					switch (glsl_custom_bindings[idx].type) {
					case VGL_TYPE_TEXCOORD:
						strcpy(glsl_texcoords_binds[glsl_custom_bindings[idx].idx], start);
						glsl_texcoords_used[glsl_custom_bindings[idx].idx] = GL_TRUE;
						sprintf(newline, "VIN(%s, %d);\n", str + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_COLOR:
						sprintf(newline, "CIN(%s, %d);\n", str + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FIN(%s, %d);\n", str + 8, glsl_custom_bindings[idx].idx);
						break;
					}
				} else {
					if (glsl_is_first_shader) {
						// Check if varying has been already bound (eg: a varying that changes in size depending on preprocessor if)
						glsl_get_existing_texcoord_bind(idx, start);
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								sprintf(newline, "VIN(%s, \v);", str + 8);
							} else {
								glsl_reserve_texcoord_bind(idx, start);
#ifndef SKIP_ERROR_HANDLING
								if (idx == -1) {
									idx = 9;
									vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
								}
#endif
								sprintf(newline, "VIN(%s, %d);\n", str + 8, idx);
							}
						} else
							sprintf(newline, "VIN(%s, %d);\n", str + 8, idx);
					} else {
						// FIXME: We rely on the fact shaders are always compiled in couples (fragment+vertex) to ensure proper semantic bindings coherence
						glsl_get_existing_texcoord_bind(idx, start);
						if (idx == -1) {
							if (glsl_custom_bindings_num > 0) { // To prevent clashing with custom semantic bindings, we need to go for a slower path
								sprintf(newline, "VIN(%s, \v);", str + 8);
							} else {
								glsl_reserve_texcoord_bind(idx, start)
#ifndef SKIP_ERROR_HANDLING
								if (idx == -1) {
									idx = 9;
									vgl_log("%s:%d %s: An error occurred during GLSL translation (TEXCOORD overflow).\n", __FILE__, __LINE__, __func__);
								}
#endif
								vgl_log("%s:%d %s: Unexpected varying (%s), forcing binding to TEXCOORD%d.\n", __FILE__, __LINE__, __func__, start, idx);
								sprintf(newline, "VIN(%s, %d);\n", str + 8, idx);
							}
						} else
							sprintf(newline, "VIN(%s, %d);\n", str + 8, idx);
					}
				}
				sceClibMemcpy(str, newline, strlen(newline));
				str = strstr(str, "varying");
				while (str && !(str[7] == ' ' || str[7] == '\t')) {
					str = strstr(str + 7, "varying");
				}
			} else { // "texture" Uniform
				if (t[0] == 't')
					sceClibMemcpy(t, "vgl_tex", 7);
				else
					sceClibMemcpy(t, "Vgl_tex", 7);
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
				sceClibMemcpy(t, "vgl in    ", 10);
				str = strstr(t, "attribute");
				while (str && !(str[9] == ' ' || str[9] == '\t')) {
					str = strstr(str + 9, "attribute");
				}
			} else { // Varying
				char *end = strstr(t, ";");
				char *start = end;
				while (*start != ' ' && *start != '\t') {
					start--;
				}
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
						sprintf(newline, "VOUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_COLOR:
						sprintf(newline, "COUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FOUT(%s,%d);", str2 + 8, glsl_custom_bindings[idx].idx);
						break;
					}
				} else {
					sprintf(newline, "VOUT(%s,\v);", str2 + 8);
				}
				sceClibMemcpy(str2, newline, strlen(newline));
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
				sceClibMemcpy(str, "(vgl_Face > 0)", 14);
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
				char *start = end;
				while (*start != ' ' && *start != '\t') {
					start--;
				}
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
						sprintf(newline, "VIN(%s, %d);\n", str + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_COLOR:
						sprintf(newline, "CIN(%s, %d);\n", str + 8, glsl_custom_bindings[idx].idx);
						break;
					case VGL_TYPE_FOG:
						sprintf(newline, "FIN(%s, %d);\n", str + 8, glsl_custom_bindings[idx].idx);
						break;
					}
				} else {
					sprintf(newline, "VIN(%s, \v);", str + 8);
				}
				sceClibMemcpy(str, newline, strlen(newline));
				str = strstr(str, "varying");
				while (str && !(str[7] == ' ' || str[7] == '\t')) {
					str = strstr(str + 7, "varying");
				}
			} else { // "texture" Uniform
				if (t[0] == 't')
					sceClibMemcpy(t, "vgl_tex", 7);
				else
					sceClibMemcpy(t, "Vgl_tex", 7);
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
	char *type = strstr(txt + strlen(glsl_hdr), "\n");
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
				type = strstr(type, "\n");
			} else if (last_func_end && type > last_func_end) { // Var is after last function, need to update last function
				last_func_start = strstr(last_func_start + 1, "{");
				last_func_end = strstr(last_func_end + 1, "}");
				goto HANDLE_VAR;
			} else if (var_end < last_func_start || !last_func_start) { // Var is prior a function, handling it
				type[0] = '\v';
				type = strstr(type, "\n");
			} else { // Var is a function, skipping
				type = strstr(type, "\n");
			}
		} else {
			type = strstr(type, "\n");
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
		case '=':
			found = 1;
			break;
		case ',':
		case '+':
			if (para_left == 0 && quad_para_left == 0)
				found = 1;
			break;
		case '-':
			if (*(left - 1) == 'E' && *(left - 2) >= '0' && *(left - 2) <= '9')
				break;
			if (para_left == 0 && quad_para_left == 0)
				found = 1;
			break;
		case '?':
			if (para_left == 0 && quad_para_left == 0) {
				found = 1;
				left++;
			}
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
		case ',':
		case '+':
		case ':':
			if (para_left == 0 && quad_para_left == 0)
				found = 1;
			break;
		case '-':
			if (*(right - 1) == 'E' && *(right - 2) >= '0' && *(right - 2) <= '9')
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
	char *res = (char *)vglMalloc(1024 * 1024);
	if (found < 2) { // Standard match
		char tmp = *left;
		left[0] = 0;
		strcpy(res, txt);
		left[0] = tmp;
		strcat(res, "vglMul(");
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
	// Nuke C++ styled comments
	char *s = strstr(txt, "/*");
	char *s2;
	while (s) {
		s2 = strstr(s, "*/") + 2;
		sceClibMemset(s, ' ', s2 - s);
		s = strstr(s2, "/*");
	}
	
	// Nuke C styled comments
	s = strstr(txt, "//");
	while (s) {
		s2 = strstr(s, "\n");
		if (!s2)
			s2 = txt + strlen(txt);
		sceClibMemset(s, ' ', s2 - s);
		s = strstr(s2, "//");
	}
}
#endif
