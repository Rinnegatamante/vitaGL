/*
 * This file is part of vitaGL
 * Copyright 2017, 2018, 2019, 2020, 2021, 2022 Rinnegatamante
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
 * display_lists.c:
 * Implementation for display lists
 */

#include "shared.h"

#define NUM_DISPLAY_LISTS 32

display_list *curr_display_list = NULL;
GLboolean display_list_execute;
display_list display_lists[NUM_DISPLAY_LISTS];

GLboolean _vgl_enqueue_list_func(void (*func)(), const char *type, ...) {
	// Check if we are creating a display list
	if (!curr_display_list)
		return GL_FALSE;

	// Enqueuing function call
	list_chain *new_tail = (list_chain *)vglMalloc(sizeof(list_chain));
	if (curr_display_list->tail)
		curr_display_list->tail->next = new_tail;
	curr_display_list->tail = new_tail;
	if (!curr_display_list->head)
		curr_display_list->head = new_tail;
	new_tail->func = func;

	// Recording function arguments
	if (*type) {
		va_list arglist;
		va_start(arglist, type);
		int i = 0;
		char *p = (char *)type;
		while (*p) {
			switch (*p) {
			case 'I': {
				int32_t arg = va_arg(arglist, int32_t);
				sceClibMemcpy(&new_tail->args[i], &arg, sizeof(arg));
				i += sizeof(arg);
			} break;
			case 'U': {
				uint32_t arg = va_arg(arglist, uint32_t);
				sceClibMemcpy(&new_tail->args[i], &arg, sizeof(arg));
				i += sizeof(arg);
			} break;
			case 'F': {
				float arg = (float)va_arg(arglist, double);
				sceClibMemcpy(&new_tail->args[i], &arg, sizeof(arg));
				i += sizeof(arg);
			} break;
			case 'X': {
				uint8_t arg = (uint8_t)va_arg(arglist, int);
				sceClibMemcpy(&new_tail->args[i], &arg, sizeof(arg));
				i += sizeof(arg);
			} break;
			case 'S': {
				int16_t arg = (int16_t)va_arg(arglist, int);
				sceClibMemcpy(&new_tail->args[i], &arg, sizeof(arg));
				i += sizeof(arg);
			} break;
			default:
				break;
			}
			p++;
		}
		va_end(arglist);

		// Detecting function type
		// 1 argument
		if (strcmp(type, "U"))
			new_tail->type = DLIST_FUNC_U32;
		// 2 arguments
		else if (strcmp(type, "II"))
			new_tail->type = DLIST_FUNC_I32_I32;
		else if (strcmp(type, "UU"))
			new_tail->type = DLIST_FUNC_U32_U32;
		else if (strcmp(type, "UI"))
			new_tail->type = DLIST_FUNC_U32_I32;
		else if (strcmp(type, "FF"))
			new_tail->type = DLIST_FUNC_F32_F32;
		else if (strcmp(type, "UF"))
			new_tail->type = DLIST_FUNC_U32_F32;
		// 3 arguments
		else if (strcmp(type, "UII"))
			new_tail->type = DLIST_FUNC_U32_I32_I32;
		else if (strcmp(type, "UIU"))
			new_tail->type = DLIST_FUNC_U32_I32_U32;
		else if (strcmp(type, "UUI"))
			new_tail->type = DLIST_FUNC_U32_U32_I32;
		else if (strcmp(type, "UUU"))
			new_tail->type = DLIST_FUNC_U32_U32_U32;
		else if (strcmp(type, "III"))
			new_tail->type = DLIST_FUNC_I32_I32_I32;
		else if (strcmp(type, "UFF"))
			new_tail->type = DLIST_FUNC_U32_F32_F32;
		else if (strcmp(type, "UUF"))
			new_tail->type = DLIST_FUNC_U32_U32_F32;
		else if (strcmp(type, "FFF"))
			new_tail->type = DLIST_FUNC_F32_F32_F32;
		else if (strcmp(type, "XXX"))
			new_tail->type = DLIST_FUNC_U8_U8_U8;
		else if (strcmp(type, "SSS"))
			new_tail->type = DLIST_FUNC_I16_I16_I16;
		// 4 arguments
		else if (strcmp(type, "UUUU"))
			new_tail->type = DLIST_FUNC_U32_U32_U32_U32;
		else if (strcmp(type, "UIUU"))
			new_tail->type = DLIST_FUNC_U32_I32_U32_U32;
		else if (strcmp(type, "IIII"))
			new_tail->type = DLIST_FUNC_I32_I32_I32_I32;
		else if (strcmp(type, "IUIU"))
			new_tail->type = DLIST_FUNC_I32_U32_I32_U32;
		else if (strcmp(type, "FFFF"))
			new_tail->type = DLIST_FUNC_F32_F32_F32_F32;
		else if (strcmp(type, "XXXX"))
			new_tail->type = DLIST_FUNC_U8_U8_U8_U8;
	} else
		new_tail->type = DLIST_FUNC_VOID;

	return !display_list_execute;
}

void glCallList(GLuint list) {
	list_chain *l = curr_display_list->head;
	while (l) {
		switch (l->type) {
		// No arguments
		case DLIST_FUNC_VOID:
			l->func();
			break;
		// 1 argument
		case DLIST_FUNC_U32:
			l->func(*(uint32_t *)(l->args));
			break;
		// 2 arguments
		case DLIST_FUNC_U32_U32:
			l->func(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_I32:
			l->func(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_I32_I32:
			l->func(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_F32:
			l->func(*(uint32_t *)(l->args), *(float *)(&l->args[4]));
			break;
		case DLIST_FUNC_F32_F32:
			l->func(*(float *)(l->args), *(float *)(&l->args[4]));
			break;
		// 3 arguments
		case DLIST_FUNC_I32_I32_I32:
			l->func(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_I32_I32:
			l->func(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_I32_U32:
			l->func(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_U32:
			l->func(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_I32:
			l->func(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U8_U8_U8:
			l->func(*(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]));
			break;
		case DLIST_FUNC_I16_I16_I16:
			l->func(*(int16_t *)(l->args), *(int16_t *)(&l->args[2]), *(int16_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_F32_F32:
			l->func(*(uint32_t *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_F32:
			l->func(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		case DLIST_FUNC_F32_F32_F32:
			l->func(*(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		// 4 arguments
		case DLIST_FUNC_U32_U32_U32_U32:
			l->func(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_I32_I32_I32_I32:
			l->func(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(int32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_I32_U32_I32_U32:
			l->func(*(int32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_U32_I32_U32_U32:
			l->func(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_F32_F32_F32_F32:
			l->func(*(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]), *(float *)(&l->args[12]));
			break;
		case DLIST_FUNC_U8_U8_U8_U8:
			l->func(*(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]), *(uint8_t *)(&l->args[3]));
			break;
		default:
			break;
		}
		l = (list_chain *)l->next;
	}
}

void glNewList(GLuint list, GLenum mode) {
#ifndef SKIP_ERROR_HANDLING
	if (list == 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, list)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	curr_display_list = &display_lists[list];
	display_list_execute = mode == GL_COMPILE ? GL_FALSE : GL_TRUE;
}

void glEndList(void) {
	curr_display_list = NULL;
}

GLuint glGenLists(GLsizei range) {
#ifndef SKIP_ERROR_HANDLING
	if (range < 0) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_VALUE, 0)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_OPERATION, 0)
	}
#endif
	GLsizei r = range;
	GLuint first = 0;
	for (GLuint i = 0; i < NUM_DISPLAY_LISTS; i++) {
		if (!display_lists[i].used) {
			if (!first)
				first = i;
			r--;
		} else {
			first = 0;
			r = range;
		}
		if (!r)
			break;
	}
#ifndef SKIP_ERROR_HANDLING
	if (r) {
		vgl_log("%s:%d glGenLists: Not enough display lists! Consider increasing the display lists maximum number...\n", __FILE__, __LINE__);
		return 0;
	}
#endif
	for (GLuint i = first; i < first + range; i++) {
		display_lists[i].used = GL_TRUE;
		display_lists[i].head = display_lists[i].tail = NULL;
	}
	return first;
}

void glDeleteLists(GLuint list, GLsizei range) {
#ifndef SKIP_ERROR_HANDLING
	if (range < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, range)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	for (GLuint i = list; i < list + range; i++) {
		list_chain *l = display_lists[i].head;
		while (l) {
			list_chain *old = l;
			l = l->next;
			vglFree(old);
		}
		display_lists[i].used = GL_FALSE;
	}
}
