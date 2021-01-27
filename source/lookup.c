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
 * lookup.c:
 * A GL GetProcAddress implementation
 */

#include <stdlib.h>
#include <string.h>

#include "vitaGL.h"

static const struct {
	const char *name;
	void *proc;
} vgl_proctable[] = {
	// *gl
	{ "glActiveTexture", (void *)glActiveTexture },
	{ "glAlphaFunc", (void *)glAlphaFunc },
	{ "glArrayElement", (void *)glArrayElement },
	{ "glAttachShader", (void *)glAttachShader },
	{ "glBegin", (void *)glBegin },
	{ "glBindAttribLocation", (void *)glBindAttribLocation },
	{ "glBindBuffer", (void *)glBindBuffer },
	{ "glBindFramebuffer", (void *)glBindFramebuffer },
	{ "glBindTexture", (void *)glBindTexture },
	{ "glBlendEquation", (void *)glBlendEquation },
	{ "glBlendEquationSeparate", (void *)glBlendEquationSeparate },
	{ "glBlendFunc", (void *)glBlendFunc },
	{ "glBlendFuncSeparate", (void *)glBlendFuncSeparate },
	{ "glBufferData", (void *)glBufferData },
	{ "glBufferSubData", (void *)glBufferSubData },
	{ "glCheckFramebufferStatus", (void *)glCheckFramebufferStatus },
	{ "glClear", (void *)glClear },
	{ "glClearColor", (void *)glClearColor },
	{ "glClearDepth", (void *)glClearDepth },
	{ "glClearDepthf", (void *)glClearDepthf },
	{ "glClearStencil", (void *)glClearStencil },
	{ "glClientActiveTexture", (void *)glClientActiveTexture },
	{ "glClipPlane", (void *)glClipPlane },
	{ "glColor3f", (void *)glColor3f },
	{ "glColor3fv", (void *)glColor3fv },
	{ "glColor3ub", (void *)glColor3ub },
	{ "glColor3ubv", (void *)glColor3ubv },
	{ "glColor4f", (void *)glColor4f },
	{ "glColor4fv", (void *)glColor4fv },
	{ "glColor4ub", (void *)glColor4ub },
	{ "glColor4ubv", (void *)glColor4ubv },
	{ "glColorMask", (void *)glColorMask },
	{ "glColorPointer", (void *)glColorPointer },
	{ "glColorTable", (void *)glColorTable },
	{ "glCompileShader", (void *)glCompileShader },
	{ "glCompressedTexImage2D", (void *)glCompressedTexImage2D },
	{ "glCreateProgram", (void *)glCreateProgram },
	{ "glCreateShader", (void *)glCreateShader },
	{ "glCullFace", (void *)glCullFace },
	{ "glDeleteBuffers", (void *)glDeleteBuffers },
	{ "glDeleteFramebuffers", (void *)glDeleteFramebuffers },
	{ "glDeleteProgram", (void *)glDeleteProgram },
	{ "glDeleteShader", (void *)glDeleteShader },
	{ "glDeleteTextures", (void *)glDeleteTextures },
	{ "glDepthFunc", (void *)glDepthFunc },
	{ "glDepthMask", (void *)glDepthMask },
	{ "glDepthRange", (void *)glDepthRange },
	{ "glDepthRangef", (void *)glDepthRangef },
	{ "glDisable", (void *)glDisable },
	{ "glDisableClientState", (void *)glDisableClientState },
	{ "glDisableVertexAttribArray", (void *)glDisableVertexAttribArray },
	{ "glDrawArrays", (void *)glDrawArrays },
	{ "glDrawElements", (void *)glDrawElements },
	{ "glEnable", (void *)glEnable },
	{ "glEnableClientState", (void *)glEnableClientState },
	{ "glEnableVertexAttribArray", (void *)glEnableVertexAttribArray },
	{ "glEnd", (void *)glEnd },
	{ "glFinish", (void *)glFinish },
	{ "glFogf", (void *)glFogf },
	{ "glFogfv", (void *)glFogfv },
	{ "glFogi", (void *)glFogi },
	{ "glFramebufferTexture", (void *)glFramebufferTexture },
	{ "glFramebufferTexture2D", (void *)glFramebufferTexture2D },
	{ "glFrontFace", (void *)glFrontFace },
	{ "glFrustum", (void *)glFrustum },
	{ "glGenBuffers", (void *)glGenBuffers },
	{ "glGenerateMipmap", (void *)glGenerateMipmap },
	{ "glGenFramebuffers", (void *)glGenFramebuffers },
	{ "glGenTextures", (void *)glGenTextures },
	{ "glGetAttribLocation", (void *)glGetAttribLocation },
	{ "glGetBooleanv", (void *)glGetBooleanv },
	{ "glGetFloatv", (void *)glGetFloatv },
	{ "glGetError", (void *)glGetError },
	{ "glGetIntegerv", (void *)glGetIntegerv },
	{ "glGetProgramInfoLog", (void *)glGetProgramInfoLog },
	{ "glGetProgramiv", (void *)glGetProgramiv },
	{ "glGetShaderInfoLog", (void *)glGetShaderInfoLog },
	{ "glGetShaderiv", (void *)glGetShaderiv },
	{ "glGetString", (void *)glGetString },
	{ "glGetUniformLocation", (void *)glGetUniformLocation },
	{ "glIsEnabled", (void *)glIsEnabled },
	{ "glLineWidth", (void *)glLineWidth },
	{ "glLinkProgram", (void *)glLinkProgram },
	{ "glLoadIdentity", (void *)glLoadIdentity },
	{ "glLoadMatrixf", (void *)glLoadMatrixf },
	{ "glMatrixMode", (void *)glMatrixMode },
	{ "glMultMatrixf", (void *)glMultMatrixf },
	{ "glOrtho", (void *)glOrtho },
	{ "glPointSize", (void *)glPointSize },
	{ "glPolygonMode", (void *)glPolygonMode },
	{ "glPolygonOffset", (void *)glPolygonOffset },
	{ "glPopMatrix", (void *)glPopMatrix },
	{ "glPushMatrix", (void *)glPushMatrix },
	{ "glReadPixels", (void *)glReadPixels },
	{ "glRotatef", (void *)glRotatef },
	{ "glScalef", (void *)glScalef },
	{ "glScissor", (void *)glScissor },
	{ "glShaderBinary", (void *)glShaderBinary },
	{ "glShaderSource", (void *)glShaderSource },
	{ "glStencilFunc", (void *)glStencilFunc },
	{ "glStencilFuncSeparate", (void *)glStencilFuncSeparate },
	{ "glStencilMask", (void *)glStencilMask },
	{ "glStencilMaskSeparate", (void *)glStencilMaskSeparate },
	{ "glStencilOp", (void *)glStencilOp },
	{ "glStencilOpSeparate", (void *)glStencilOpSeparate },
	{ "glTexCoord2f", (void *)glTexCoord2f },
	{ "glTexCoord2fv", (void *)glTexCoord2fv },
	{ "glTexCoord2i", (void *)glTexCoord2i },
	{ "glTexCoord2iv", (void *)glTexCoord2i },
	{ "glTexCoordPointer", (void *)glTexCoordPointer },
	{ "glTexEnvf", (void *)glTexEnvf },
	{ "glTexEnvfv", (void *)glTexEnvfv },
	{ "glTexEnvi", (void *)glTexEnvi },
	{ "glTexImage2D", (void *)glTexImage2D },
	{ "glTexParameterf", (void *)glTexParameterf },
	{ "glTexParameteri", (void *)glTexParameteri },
	{ "glTexSubImage2D", (void *)glTexSubImage2D },
	{ "glTranslatef", (void *)glTranslatef },
	{ "glUniform1f", (void *)glUniform1f },
	{ "glUniform1fv", (void *)glUniform1fv },
	{ "glUniform1i", (void *)glUniform1i },
	{ "glUniform2f", (void *)glUniform2f },
	{ "glUniform2i", (void *)glUniform2i },
	{ "glUniform2fv", (void *)glUniform2fv },
	{ "glUniform3f", (void *)glUniform3f },
	{ "glUniform3fv", (void *)glUniform3fv },
	{ "glUniform4f", (void *)glUniform4f },
	{ "glUniform4fv", (void *)glUniform4fv },
	{ "glUniformMatrix3fv", (void *)glUniformMatrix3fv },
	{ "glUniformMatrix4fv", (void *)glUniformMatrix4fv },
	{ "glUseProgram", (void *)glUseProgram },
	{ "glVertex2f", (void *)glVertex2f },
	{ "glVertex3f", (void *)glVertex3f },
	{ "glVertex3fv", (void *)glVertex3fv },
	{ "glVertexAttrib1f", (void *)glVertexAttrib1f },
	{ "glVertexAttrib1fv", (void *)glVertexAttrib1fv },
	{ "glVertexAttrib2f", (void *)glVertexAttrib2f },
	{ "glVertexAttrib2fv", (void *)glVertexAttrib2fv },
	{ "glVertexAttrib3f", (void *)glVertexAttrib3f },
	{ "glVertexAttrib3fv", (void *)glVertexAttrib3fv },
	{ "glVertexAttrib4f", (void *)glVertexAttrib4f },
	{ "glVertexAttrib4fv", (void *)glVertexAttrib4fv },
	{ "glVertexAttribPointer", (void *)glVertexAttribPointer },
	{ "glVertexPointer", (void *)glVertexPointer },
	{ "glViewport", (void *)glViewport },
	// *glu
	{ "gluPerspective", (void *)gluPerspective },
	// *vgl
	{ "vglColorPointer", (void *)vglColorPointer },
	{ "vglColorPointerMapped", (void *)vglColorPointerMapped },
	{ "vglDrawObjects", (void *)vglDrawObjects },
	{ "vglIndexPointer", (void *)vglIndexPointer },
	{ "vglIndexPointerMapped", (void *)vglIndexPointerMapped },
	{ "vglTexCoordPointer", (void *)vglTexCoordPointer },
	{ "vglTexCoordPointerMapped", (void *)vglTexCoordPointerMapped },
	{ "vglVertexPointer", (void *)vglVertexPointer },
	{ "vglVertexPointerMapped", (void *)vglVertexPointerMapped },
	{ "vglBindAttribLocation", (void *)vglBindAttribLocation },
	{ "vglBindPackedAttribLocation", (void *)vglBindPackedAttribLocation },
	{ "vglVertexAttribPointer", (void *)vglVertexAttribPointer },
	{ "vglVertexAttribPointerMapped", (void *)vglVertexAttribPointerMapped },
	{ "vglAlloc", (void *)vglAlloc },
	{ "vglEnableRuntimeShaderCompiler", (void *)vglEnableRuntimeShaderCompiler },
	{ "vglEnd", (void *)vglEnd },
	{ "vglForceAlloc", (void *)vglForceAlloc },
	{ "vglFree", (void *)vglFree },
	{ "vglGetGxmTexture", (void *)vglGetGxmTexture },
	{ "vglGetTexDataPointer", (void *)vglGetTexDataPointer },
	{ "vglHasRuntimeShaderCompiler", (void *)vglHasRuntimeShaderCompiler },
	{ "vglInit", (void *)vglInit },
	{ "vglInitExtended", (void *)vglInitExtended },
	{ "vglInitWithCustomSizes", (void *)vglInitWithCustomSizes },
	{ "vglMemFree", (void *)vglMemFree },
	{ "vglSetParamBufferSize", (void *)vglSetParamBufferSize },
	{ "vglSetupRuntimeShaderCompiler", (void *)vglSetupRuntimeShaderCompiler },
	{ "vglSwapBuffers", (void *)vglSwapBuffers },
	{ "vglTexImageDepthBuffer", (void *)vglTexImageDepthBuffer },
	{ "vglUseTripleBuffering", (void *)vglUseTripleBuffering },
	{ "vglUseVram", (void *)vglUseVram },
	{ "vglUseVramForUSSE", (void *)vglUseVramForUSSE },
	{ "vglUseExtraMem", (void *)vglUseExtraMem },
	{ "vglWaitVblankStart", (void *)vglWaitVblankStart },
	// *egl
	{ "eglSwapInterval", (void *)eglSwapInterval },
	{ "eglSwapBuffers", (void *)eglSwapBuffers },
};

static const size_t vgl_numproc = sizeof(vgl_proctable) / sizeof(*vgl_proctable);

void *vglGetProcAddress(const char *name) {
	if (!name || !*name) {
		return NULL;
	}

	// strip any extension markers
	const int len = strlen(name);
	char tmpname[len + 1];
	memcpy(tmpname, name, len + 1);
	if (!strcmp(tmpname + len - 3, "EXT") || !strcmp(tmpname + len - 3, "ARB")) {
		tmpname[len - 3] = 0;
	}

	// search for stripped name
	for (size_t i = 0; i < vgl_numproc; ++i) {
		if (!strcmp(tmpname, vgl_proctable[i].name)) {
			return vgl_proctable[i].proc;
		}
	}

	return NULL;
}
