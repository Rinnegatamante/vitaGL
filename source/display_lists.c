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

GLboolean _vgl_enqueue_list_func(void (*func)(), dlistFuncType type, ...) {
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
	new_tail->type = type;

	// Recording function arguments
	if (type) {
		int i = 0;
		va_list arglist;
		va_start(arglist, type);
		while (type) {
			uint8_t arg_type = (uint8_t)type;
			uint32_t uarg;
			int32_t iarg;
			float farg;
			uint8_t suarg;
			int16_t sarg;
			switch (arg_type) {
			case DLIST_ARG_U32:
				uarg = va_arg(arglist, uint32_t);
				sceClibMemcpy(&new_tail->args[i], &uarg, sizeof(uarg));
				i += sizeof(uarg);
				break;
			case DLIST_ARG_I32:
				iarg = va_arg(arglist, int32_t);
				sceClibMemcpy(&new_tail->args[i], &iarg, sizeof(iarg));
				i += sizeof(iarg);
				break;
			case DLIST_ARG_F32:
				farg = (float)va_arg(arglist, double);
				sceClibMemcpy(&new_tail->args[i], &farg, sizeof(farg));
				i += sizeof(farg);
				break;
			case DLIST_ARG_I16:
				sarg = (int16_t)va_arg(arglist, int);
				sceClibMemcpy(&new_tail->args[i], &sarg, sizeof(sarg));
				i += sizeof(sarg);
				break;
			case DLIST_ARG_U8:
				suarg = (uint8_t)va_arg(arglist, int);
				sceClibMemcpy(&new_tail->args[i], &suarg, sizeof(suarg));
				i += sizeof(suarg);
				break;
			case DLIST_ARG_VOID:
			default:
				break;
			}
			type >>= 8;
		}
		va_end(arglist);
	}

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
	void (*f_void)(); // DLIST_FUNC_VOID
	void (*f_u32)(uint32_t); // DLIST_FUNC_U32
	void (*f_u32_u32)(uint32_t, uint32_t); // DLIST_FUNC_U32_U32
	void (*f_u32_i32)(uint32_t, int32_t); // DLIST_FUNC_U32_I32
	void (*f_i32_i32)(int32_t, int32_t); // DLIST_FUNC_I32_I32
	void (*f_u32_f32)(uint32_t, float); // DLIST_FUNC_U32_F32
	void (*f_f32_f32)(float, float); // DLIST_FUNC_F32_F32
	void (*f_i32_i32_i32)(int32_t, int32_t, int32_t); // DLIST_FUNC_I32_I32_I32
	void (*f_u32_i32_i32)(uint32_t, int32_t, int32_t); // DLIST_FUNC_U32_I32_I32
	void (*f_u32_i32_u32)(uint32_t, int32_t, uint32_t); // DLIST_FUNC_U32_I32_U32
	void (*f_u32_u32_u32)(uint32_t, uint32_t, uint32_t); // DLIST_FUNC_U32_U32_U32
	void (*f_u32_u32_i32)(uint32_t, uint32_t, int32_t); // DLIST_FUNC_U32_U32_I32
	void (*f_u8_u8_u8)(uint8_t, uint8_t, uint8_t); // DLIST_FUNC_U8_U8_U8
	void (*f_i16_i16_i16)(int16_t, int16_t, int16_t); // DLIST_FUNC_I16_I16_I16
	void (*f_u32_f32_f32)(uint32_t, float, float); // DLIST_FUNC_U32_F32_F32
	void (*f_u32_u32_f32)(uint32_t, uint32_t, float); // DLIST_FUNC_U32_U32_F32
	void (*f_f32_f32_f32)(float, float, float); // DLIST_FUNC_F32_F32_F32
	void (*f_u32_u32_u32_u32)(uint32_t, uint32_t, uint32_t, uint32_t); // DLIST_FUNC_U32_U32_U32_U32
	void (*f_i32_i32_i32_i32)(int32_t, int32_t, int32_t, int32_t); // DLIST_FUNC_I32_I32_I32_I32
	void (*f_i32_u32_i32_u32)(int32_t, uint32_t, int32_t, uint32_t); // DLIST_FUNC_I32_U32_I32_U32
	void (*f_u32_i32_u32_u32)(uint32_t, int32_t, uint32_t, uint32_t); // DLIST_FUNC_U32_I32_U32_U32
	void (*f_f32_f32_f32_f32)(float, float, float, float); // DLIST_FUNC_F32_F32_F32_F32
	void (*f_u8_u8_u8_u8)(uint8_t, uint8_t, uint8_t, uint8_t); // DLIST_FUNC_U8_U8_U8_U8
	
	while (l) {
		switch (l->type) {
		// No arguments
		case DLIST_FUNC_VOID:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s()\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func));
#endif
			f_void = l->func;
			f_void();
			break;
		// 1 argument
		case DLIST_FUNC_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args));
#endif		
			f_u32 = l->func;
			f_u32(*(uint32_t *)(l->args));
			break;
		// 2 arguments
		case DLIST_FUNC_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]));
#endif	
			f_u32_u32 = l->func;
			f_u32_u32(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]));
#endif
			f_u32_i32 = l->func;
			f_u32_i32(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%i, %i)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(int32_t *)(&l->args[4]));
#endif
			f_i32_i32 = l->func;
			f_i32_i32(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(float *)(&l->args[4]));
#endif
			f_u32_f32 = l->func;
			f_u32_f32(*(uint32_t *)(l->args), *(float *)(&l->args[4]));
			break;
		case DLIST_FUNC_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(float *)(l->args), *(float *)(&l->args[4]));
#endif
			f_f32_f32 = l->func;
			f_f32_f32(*(float *)(l->args), *(float *)(&l->args[4]));
			break;
		// 3 arguments
		case DLIST_FUNC_I32_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%d, %d, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
#endif
			f_i32_i32_i32 = l->func;
			f_i32_i32_i32(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
#endif
			f_u32_i32_i32 = l->func;
			f_u32_i32_i32(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_I32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
#endif
			f_u32_i32_u32 = l->func;
			f_u32_i32_u32(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
#endif
			f_u32_u32_u32 = l->func;
			f_u32_u32_u32(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
#endif
			f_u32_u32_i32 = l->func;
			f_u32_u32_i32(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]));
			break;
		case DLIST_FUNC_U8_U8_U8:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%hhu, %hhu, %hhu)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]));
#endif
			f_u8_u8_u8 = l->func;
			f_u8_u8_u8(*(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]));
			break;
		case DLIST_FUNC_I16_I16_I16:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%hd, %hd, %hd)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int16_t *)(l->args), *(int16_t *)(&l->args[2]), *(int16_t *)(&l->args[4]));
#endif
			f_i16_i16_i16 = l->func;
			f_i16_i16_i16(*(int16_t *)(l->args), *(int16_t *)(&l->args[2]), *(int16_t *)(&l->args[4]));
			break;
		case DLIST_FUNC_U32_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
#endif
			f_u32_f32_f32 = l->func;
			f_u32_f32_f32(*(uint32_t *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		case DLIST_FUNC_U32_U32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(float *)(&l->args[8]));
#endif
			f_u32_u32_f32 = l->func;
			f_u32_u32_f32(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		case DLIST_FUNC_F32_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%f, %f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
#endif
			f_f32_f32_f32 = l->func;
			f_f32_f32_f32(*(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]));
			break;
		// 4 arguments
		case DLIST_FUNC_U32_U32_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %u, %u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
#endif
			f_u32_u32_u32_u32 = l->func;
			f_u32_u32_u32_u32(*(uint32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_I32_I32_I32_I32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%d, %d, %d, %d)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(int32_t *)(&l->args[12]));
#endif
			f_i32_i32_i32_i32 = l->func;
			f_i32_i32_i32_i32(*(int32_t *)(l->args), *(int32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(int32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_I32_U32_I32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%d, %u, %d, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(int32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
#endif
			f_i32_u32_i32_u32 = l->func;
			f_i32_u32_i32_u32(*(int32_t *)(l->args), *(uint32_t *)(&l->args[4]), *(int32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_U32_I32_U32_U32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%u, %d, %u, %u)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
#endif
			f_u32_i32_u32_u32 = l->func;
			f_u32_i32_u32_u32(*(uint32_t *)(l->args), *(int32_t *)(&l->args[4]), *(uint32_t *)(&l->args[8]), *(uint32_t *)(&l->args[12]));
			break;
		case DLIST_FUNC_F32_F32_F32_F32:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%f, %f, %f, %f)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]), *(float *)(&l->args[12]));
#endif
			f_f32_f32_f32_f32 = l->func;
			f_f32_f32_f32_f32(*(float *)(l->args), *(float *)(&l->args[4]), *(float *)(&l->args[8]), *(float *)(&l->args[12]));
			break;
		case DLIST_FUNC_U8_U8_U8_U8:
#ifdef DEBUG_DLISTS
			vgl_log("%s:%d %s: %s(%hhu, %hhu, %hhu, %hhu)\n", __FILE__, __LINE__, __func__, vglGetFuncName(l->func), *(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]), *(uint8_t *)(&l->args[3]));
#endif
			f_u8_u8_u8_u8 = l->func;
			f_u8_u8_u8_u8(*(uint8_t *)(l->args), *(uint8_t *)(&l->args[1]), *(uint8_t *)(&l->args[2]), *(uint8_t *)(&l->args[3]));
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
