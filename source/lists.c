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
 *
 */
#include "vitaGL.h"
#include "shared.h"

command_list cmd_list[COMMAND_LISTS_NUM]; // Command lists array
static command_list *current_cmd_list = NULL;

void *cmdlist_alloc_cb(void *args, SceSize requestedSize, SceSize *size) {
	*size = requestedSize;
	return malloc(requestedSize);
}

// openGL implementation

GLuint glGenLists(GLsizei range) {
#ifndef SKIP_ERROR_HANDLING
	if (range < 0) {
		vgl_error = GL_INVALID_VALUE;
		return 0;
	} else if (phase == MODEL_CREATION) {
		vgl_error = GL_INVALID_OPERATION;
		return 0;
	}
#endif

	// Searching for contiguous free command list slots
	int i, first = -1;
	GLboolean found = GL_FALSE;
	for (i = 0; i < COMMAND_LISTS_NUM; i++) {
		if (!cmd_list[i].valid) {
			if (first < 0) first = i;
			else if (i - first == range) {
				found = GL_TRUE;
				break;
			}
		} else first = -1;
	}
	
	// Reserving command list slots
	if (found) {
		for (i = first; i < first + range; i++) {
			cmd_list[i].valid = 1;
		}
		return first;
	}
	
	return 0;
}

void glNewList(GLuint list, GLenum mode) {
#ifndef SKIP_ERROR_HANDLING
	if (!list) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	} else if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (current_cmd_list) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
	
	phase = CMDLIST_CREATION;
#endif

	// Properly initializing command list
	switch (mode) {
	case GL_COMPILE:
		cmd_list[list].execute = 0;
		break;
	case GL_COMPILE_AND_EXECUTE:
		cmd_list[list].execute = 1;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_VALUE)
		break;
	}
	
	// Creating a deferred context for the command list
	SceGxmDeferredContextParams params;
	memset(&params, 0, sizeof(SceGxmDeferredContextParams));
	params.hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
	params.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	params.vdmCallback = &cmdlist_alloc_cb;
	params.vertexCallback = &cmdlist_alloc_cb;
	params.fragmentCallback = &cmdlist_alloc_cb;
	sceGxmCreateDeferredContext(&params, &gxm_context);
	
	// Setting back current viewport if enabled cause sceGxm will reset it at sceGxmEndScene call
	sceGxmSetViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
	sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_OUTSIDE, 0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
	
	// Initializing a new gxm command list
	sceGxmBeginCommandList(gxm_context);
	current_cmd_list = &cmd_list[list];
}

void glEndList(void) {
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (!current_cmd_list) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
	
	phase = NONE;
#endif
	
	// Ending current gxm command list and deferred context
	sceGxmEndCommandList(gxm_context, &current_cmd_list->gxm_list);
	sceGxmDestroyDeferredContext(gxm_context);
	gxm_context = main_gxm_context;
	
	// Executing command list if required
	if (current_cmd_list->execute) sceGxmExecuteCommandList(gxm_context, &current_cmd_list->gxm_list);
	
	current_cmd_list = NULL;
}

void glCallList(GLuint list) {
	sceGxmExecuteCommandList(gxm_context, &cmd_list[list].gxm_list);
}

void glCallLists(GLsizei n, GLenum type, const void *lists) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		SET_GL_ERROR(GL_INVALID_VALUE);
	}
#endif
	int i;
	
	switch (type) {
	case GL_BYTE:
	{
		for (i = 0; i < n; i++) {
			int8_t *l = (int8_t*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	case GL_UNSIGNED_BYTE:
	{
		for (i = 0; i < n; i++) {
			uint8_t *l = (uint8_t*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	case GL_SHORT:
	{
		for (i = 0; i < n; i++) {
			int16_t *l = (int16_t*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	case GL_UNSIGNED_SHORT:
	{
		for (i = 0; i < n; i++) {
			uint16_t *l = (uint16_t*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	case GL_INT:
	{
		for (i = 0; i < n; i++) {
			int32_t *l = (int32_t*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	case GL_UNSIGNED_INT:
	{
		for (i = 0; i < n; i++) {
			uint32_t *l = (uint32_t*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	case GL_FLOAT:
	{
		for (i = 0; i < n; i++) {
			float *l = (float*)lists;
			glCallList(l[0]);
			l++;
		}
	}
	default:
		SET_GL_ERROR(GL_INVALID_ENUM);
		break;
	}
}
