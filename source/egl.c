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

//#define EGL_PEDANTIC // This flag makes eGL error be properly set when a function success

#ifdef LOG_ERRORS
char *get_egl_error_literal(uint32_t code) {
	switch (code) {
	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";
	default:
		return "Unknown Error";
	}
}
#endif

// Error set funcs
#define SET_EGL_ERROR(x) \
	vgl_log("%s:%d: %s set %s\n", __FILE__, __LINE__, __func__, get_egl_error_literal(x)); \
	egl_error = x; \
	return;
#define SET_EGL_ERROR_WITH_RET(x, y) \
	vgl_log("%s:%d: %s set %s\n", __FILE__, __LINE__, __func__, get_egl_error_literal(x)); \
	egl_error = x; \
	return y;

EGLint egl_error = EGL_SUCCESS;
EGLenum rend_api = EGL_OPENGL_ES_API;

// EGL implementation

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
	vsync_interval = interval;
#ifdef EGL_PEDANTIC
	egl_error = EGL_SUCCESS;
#endif
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
	vglSwapBuffers(GL_FALSE);
#ifdef EGL_PEDANTIC
	egl_error = EGL_SUCCESS;
#endif
}

EGLBoolean eglBindAPI(EGLenum api) {
	switch (api) {
	case EGL_OPENGL_API:
	case EGL_OPENGL_ES_API:
		rend_api = api;
#ifdef EGL_PEDANTIC
		egl_error = EGL_SUCCESS;
#endif
		return EGL_TRUE;
		break;
	default:
		SET_EGL_ERROR_WITH_RET(EGL_BAD_PARAMETER, EGL_FALSE);
		break;
	}
}

EGLenum eglQueryAPI(void) {
#ifdef EGL_PEDANTIC
	egl_error = EGL_SUCCESS;
#endif
	return rend_api;
}

EGLint eglGetError(void) {
	EGLint ret = egl_error;
	egl_error = EGL_SUCCESS;
	return ret;
}
