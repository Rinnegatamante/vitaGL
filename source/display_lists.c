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

#ifdef HAVE_DLISTS
#define NUM_DISPLAY_LISTS 512
#else
#define NUM_DISPLAY_LISTS 1 // Save on memory usage if display lists are disabled
#endif

#define call_full_list(t) \
	for (i = 0; i < n; i++) { \
		t *l = (t *)lists; \
		glCallList(l[i]); \
	}

//#define DEBUG_DLISTS // Uncomment this to debug display lists

display_list *curr_display_list = NULL;
GLboolean display_list_execute;
display_list display_lists[NUM_DISPLAY_LISTS];
static uint32_t dlist_offs = 0;

void resetDlists() {
	sceClibMemset(&display_lists[0], 0, sizeof(display_list) * NUM_DISPLAY_LISTS);
}

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
	new_tail->next = NULL;

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
		if (!strcmp(type, "U"))
			new_tail->type = DLIST_FUNC_U32;
		// 2 arguments
		else if (!strcmp(type, "II"))
			new_tail->type = DLIST_FUNC_I32_I32;
		else if (!strcmp(type, "UU"))
			new_tail->type = DLIST_FUNC_U32_U32;
		else if (!strcmp(type, "UI"))
			new_tail->type = DLIST_FUNC_U32_I32;
		else if (!strcmp(type, "FF"))
			new_tail->type = DLIST_FUNC_F32_F32;
		else if (!strcmp(type, "UF"))
			new_tail->type = DLIST_FUNC_U32_F32;
		// 3 arguments
		else if (!strcmp(type, "UII"))
			new_tail->type = DLIST_FUNC_U32_I32_I32;
		else if (!strcmp(type, "UIU"))
			new_tail->type = DLIST_FUNC_U32_I32_U32;
		else if (!strcmp(type, "UUI"))
			new_tail->type = DLIST_FUNC_U32_U32_I32;
		else if (!strcmp(type, "UUU"))
			new_tail->type = DLIST_FUNC_U32_U32_U32;
		else if (!strcmp(type, "III"))
			new_tail->type = DLIST_FUNC_I32_I32_I32;
		else if (!strcmp(type, "UFF"))
			new_tail->type = DLIST_FUNC_U32_F32_F32;
		else if (!strcmp(type, "UUF"))
			new_tail->type = DLIST_FUNC_U32_U32_F32;
		else if (!strcmp(type, "FFF"))
			new_tail->type = DLIST_FUNC_F32_F32_F32;
		else if (!strcmp(type, "XXX"))
			new_tail->type = DLIST_FUNC_U8_U8_U8;
		else if (!strcmp(type, "SSS"))
			new_tail->type = DLIST_FUNC_I16_I16_I16;
		// 4 arguments
		else if (!strcmp(type, "UUUU"))
			new_tail->type = DLIST_FUNC_U32_U32_U32_U32;
		else if (!strcmp(type, "UIUU"))
			new_tail->type = DLIST_FUNC_U32_I32_U32_U32;
		else if (!strcmp(type, "IIII"))
			new_tail->type = DLIST_FUNC_I32_I32_I32_I32;
		else if (!strcmp(type, "IUIU"))
			new_tail->type = DLIST_FUNC_I32_U32_I32_U32;
		else if (!strcmp(type, "FFFF"))
			new_tail->type = DLIST_FUNC_F32_F32_F32_F32;
		else if (!strcmp(type, "XXXX"))
			new_tail->type = DLIST_FUNC_U8_U8_U8_U8;
	} else
		new_tail->type = DLIST_FUNC_VOID;

	return !display_list_execute;
}

void glListBase(GLuint base) {
	dlist_offs = base;
}

void glCallList(GLuint list) {
	list_chain *l = display_lists[list + dlist_offs - 1].head;
#ifdef DEBUG_DLISTS
	vgl_log("%s:%d %s: Executing display list %d (Offset: %d)\n", __FILE__, __LINE__, __func__, list + dlist_offs, dlist_offs);
#endif
	// Function prototypes
	void (*f)(); // DLIST_FUNC_VOID
	void (*f1)(uint32_t); // DLIST_FUNC_U32
	void (*f2)(uint32_t, uint32_t); // DLIST_FUNC_U32_U32
	void (*f3)(uint32_t, int32_t); // DLIST_FUNC_U32_I32
	void (*f4)(int32_t, int32_t); // DLIST_FUNC_I32_I32
	void (*f5)(uint32_t, float); // DLIST_FUNC_U32_F32
	void (*f6)(float, float); // DLIST_FUNC_F32_F32
	void (*f7)(int32_t, int32_t, int32_t); // DLIST_FUNC_I32_I32_I32
	void (*f8)(uint32_t, int32_t, int32_t); // DLIST_FUNC_U32_I32_I32
	void (*f9)(uint32_t, int32_t, uint32_t); // DLIST_FUNC_U32_I32_U32
	void (*f10)(uint32_t, uint32_t, uint32_t); // DLIST_FUNC_U32_U32_U32
	void (*f11)(uint32_t, uint32_t, int32_t); // DLIST_FUNC_U32_U32_I32
	void (*f12)(uint8_t, uint8_t, uint8_t); // DLIST_FUNC_U8_U8_U8
	void (*f13)(int16_t, int16_t, int16_t); // DLIST_FUNC_I16_I16_I16
	void (*f14)(uint32_t, float, float); // DLIST_FUNC_U32_F32_F32
	void (*f15)(uint32_t, uint32_t, float); // DLIST_FUNC_U32_U32_F32
	void (*f16)(float, float, float); // DLIST_FUNC_F32_F32_F32
	void (*f17)(uint32_t, uint32_t, uint32_t, uint32_t); // DLIST_FUNC_U32_U32_U32_U32
	void (*f18)(int32_t, int32_t, int32_t, int32_t); // DLIST_FUNC_I32_I32_I32_I32
	void (*f19)(int32_t, uint32_t, int32_t, uint32_t); // DLIST_FUNC_I32_U32_I32_U32
	void (*f20)(uint32_t, int32_t, uint32_t, uint32_t); // DLIST_FUNC_U32_I32_U32_U32
	void (*f21)(float, float, float, float); // DLIST_FUNC_F32_F32_F32_F32
	void (*f22)(uint8_t, uint8_t, uint8_t, uint8_t); // DLIST_FUNC_U8_U8_U8_U8
	
	while (l) {
		switch (l->type) {
		// No arguments
		case DLIST_FUNC_VOID:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s()\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func));
#endif
			f = l->func;
			f();
			break;
		// 1 argument
		case DLIST_FUNC_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args));
#endif		
			f1 = l->func;
			f1(*(uint32_t *)(l->args));
			break;
		// 2 arguments
		case DLIST_FUNC_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]));
#endif	
			f2 = l->func;
			f2(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]));
#endif
			f3 = l->func;
			f3(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%i, %i)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(int32_t *)(&l->args[4]));
#endif
			f4 = l->func;
			f4(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(float *)(&l->args[4]));
#endif
			f5 = l->func;
			f5(*(uint32_t *)(l->args), *(float *)(&l->args[4]));
			break;
		case DLIST_FUNC_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(float *)(l->args), *(float *)(&l->args[4]));
#endif
			f6 = l->func;
			f6(*(float *)(l->args), *(float *)(&l->args[4]));
			break;
		// 3 arguments
		case DLIST_FUNC_I32_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%d, %d, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
#endif
			f7 = l->func;
			f7(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
#endif
			f8 = l->func;
			f8(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_I32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
#endif
			f9 = l->func;
			f9(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
#endif
			f10 = l->func;
			f10(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
#endif
			f11 = l->func;
			f11(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U8_U8_U8:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%hhu, %hhu, %hhu)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]));
#endif
			f12 = l->func;
			f12(*(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]));
			break;
		case DLIST_FUNC_I16_I16_I16:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%hd, %hd, %hd)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int16_t *)(l->args), *(int16_t *)(&l->args[2]), *(int16_t *)(&l->args[4]));
#endif
			f13 = l->func;
			f13(*(int16_t *)(l->args), *(int16_t *)(&l->args[2]), *(int16_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
#endif
			f14 = l->func;
			f14(*(uint32_t *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(float *)(&l->args[8]));
#endif
			f15 = l->func;
			f15(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		case DLIST_FUNC_F32_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%f, %f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
#endif
			f16 = l->func;
			f16(*(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		// 4 arguments
		case DLIST_FUNC_U32_U32_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
#endif
			f17 = l->func;
			f17(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_I32_I32_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%d, %d, %d, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(int32_t *)(&l->args[12]));
#endif
			f18 = l->func;
			f18(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(int32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_I32_U32_I32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%d, %u, %d, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
#endif
			f19 = l->func;
			f19(*(int32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_U32_I32_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d, %u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
#endif
			f20 = l->func;
			f20(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_F32_F32_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%f, %f, %f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]), *(float *)(&l->args[12]));
#endif
			f21 = l->func;
			f21(*(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]), *(float *)(&l->args[12]));
			break;
		case DLIST_FUNC_U8_U8_U8_U8:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%hhu, %hhu, %hhu, %hhu)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]), *(uint8_t *)(&l->args[3]));
#endif
			f22 = l->func;
			f22(*(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]), *(uint8_t *)(&l->args[3]));
			break;
		default:
			break;
		}
		l = (list_chain *)l->next;
	}
}

void glCallLists(GLsizei n, GLenum type, const void *lists) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_VALUE, n)
	}
#endif
	int i;
	switch (type) {
	case GL_BYTE:
		call_full_list(int8_t);
		break;
	case GL_UNSIGNED_BYTE:
		call_full_list(uint8_t);
		break;
	case GL_SHORT:
		call_full_list(int16_t);
		break;
	case GL_UNSIGNED_SHORT:
		call_full_list(uint16_t);
		break;
	case GL_INT:
		call_full_list(int32_t);
		break;
	case GL_UNSIGNED_INT:
		call_full_list(uint32_t);
		break;
	case GL_FLOAT:
		call_full_list(float);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, type)
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
	curr_display_list = &display_lists[list - 1];
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
			if (first == 0)
				first = i + 1;
			r--;
		} else {
			first = i + 1;
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
	for (GLuint i = first - 1; i < first + range; i++) {
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
	for (GLuint i = list - 1; i < list + range; i++) {
		list_chain *l = display_lists[i].head;
		while (l) {
			list_chain *old = l;
			l = l->next;
			vglFree(old);
		}
		display_lists[i].used = GL_FALSE;
	}
}
