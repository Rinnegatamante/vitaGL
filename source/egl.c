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
#include "shared.h"
#include "vitaGL.h"

#define DISPLAY_PHYSICAL_WIDTH (110.7f)
#define DISPLAY_PHYSICAL_HEIGHT (62.7f)

// Default bogus values used for default objects
enum {
	EGL_DEFAULT_CTX,
	EGL_DEFAULT_SURF,
	EGL_DEFAULT_DISP,
	EGL_DEFAULT_CFG
};

#ifdef LOG_ERRORS
static char *get_egl_error_literal(uint32_t code) {
	switch (code) {
	case EGL_BAD_PARAMETER:
		return "EGL_BAD_PARAMETER";
	case EGL_BAD_ATTRIBUTE:
		return "EGL_BAD_ATTRIBUTE";
	case EGL_BAD_CONTEXT:
		return "EGL_BAD_CONTEXT";
	case EGL_BAD_SURFACE:
		return "EGL_BAD_SURFACE";
	default:
		return "Unknown Error";
	}
}
#endif

#define SET_EGL_ERROR(x, y) \
	vgl_log("%s:%d: %s set %s\n", __FILE__, __LINE__, __func__, get_egl_error_literal(x)); \
	egl_error = x; \
	return y;
	
#define EGL_RET(y) \
	egl_error = EGL_SUCCESS; \
	return y;

static EGLint egl_error = EGL_SUCCESS;
static EGLenum rend_api = EGL_OPENGL_ES_API;

// EGL implementation

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor) {
	if (major)
		*major = 2;
	if (minor)
		*minor = 2;
	EGL_RET(EGL_TRUE)
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value) {
	switch (attribute) {
	case EGL_CONFIG_ID:
		*value = 0;
		break;
	case EGL_CONTEXT_CLIENT_TYPE:
		*value = rend_api;
		break;
	case EGL_CONTEXT_CLIENT_VERSION:
		*value = 2;
		break;
	case EGL_RENDER_BUFFER:
		*value = EGL_BACK_BUFFER;
		break;
	default:
		SET_EGL_ERROR(EGL_BAD_ATTRIBUTE, EGL_FALSE)
	}
	
	EGL_RET(EGL_TRUE)
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface eglSurface, EGLint attribute, EGLint *value) {
	switch (attribute) {
	case EGL_CONFIG_ID:
		*value = 0;
		break;
	case EGL_WIDTH:
		*value = DISPLAY_WIDTH;
		break;
	case EGL_HEIGHT:
		*value = DISPLAY_HEIGHT;
		break;
	case EGL_TEXTURE_FORMAT:
		*value = EGL_TEXTURE_RGBA;
		break;
	case EGL_TEXTURE_TARGET:
		*value = EGL_TEXTURE_2D;
		break;
	case EGL_SWAP_BEHAVIOR:
		*value = EGL_BUFFER_PRESERVED;
		break;
	case EGL_LARGEST_PBUFFER:
	case EGL_MIPMAP_TEXTURE:
		*value = EGL_FALSE;
		break;
	case EGL_MIPMAP_LEVEL:
		*value = 0;
		break;
	case EGL_MULTISAMPLE_RESOLVE:
		*value = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
		break;
	case EGL_HORIZONTAL_RESOLUTION:
		*value = (EGLint)((DISPLAY_WIDTH_FLOAT / DISPLAY_PHYSICAL_WIDTH) * EGL_DISPLAY_SCALING);
		break;
	case EGL_VERTICAL_RESOLUTION:
		*value = (EGLint)((DISPLAY_HEIGHT_FLOAT / DISPLAY_PHYSICAL_HEIGHT) * EGL_DISPLAY_SCALING);
		break;
	case EGL_PIXEL_ASPECT_RATIO:
		*value = EGL_DISPLAY_SCALING;
		break;
	case EGL_RENDER_BUFFER:
		*value = EGL_BACK_BUFFER;
		break;
	case EGL_VG_COLORSPACE:
		*value = EGL_VG_COLORSPACE_LINEAR;
		break;
	case EGL_VG_ALPHA_FORMAT:
		*value = EGL_VG_ALPHA_FORMAT_NONPRE;
		break;
	case EGL_TIMESTAMPS_ANDROID:
		*value = EGL_FALSE;
		break;
	default:
		SET_EGL_ERROR(EGL_BAD_ATTRIBUTE, EGL_FALSE)
	}

	EGL_RET(EGL_TRUE)
}

EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value) {
	switch (attribute) {
	case EGL_ALPHA_SIZE:
		*value = 8;
		break;
	case EGL_ALPHA_MASK_SIZE:
		*value = 8;
		break;
	case EGL_BIND_TO_TEXTURE_RGB:
		*value = EGL_TRUE;
		break;
	case EGL_BIND_TO_TEXTURE_RGBA:
		*value = EGL_TRUE;
		break;
	case EGL_BLUE_SIZE:
		*value = 8;
		break;
	case EGL_BUFFER_SIZE:
		*value = 32;
		break;
	case EGL_COLOR_BUFFER_TYPE:
		*value = EGL_RGB_BUFFER;
		break;
	case EGL_CONFIG_CAVEAT:
		*value = EGL_NONE;
		break;
	case EGL_CONFIG_ID:
		*value = 0;
		break;
	case EGL_CONFORMANT:
		*value = 0;
		break;
	case EGL_DEPTH_SIZE:
		*value = 32;
		break;
	case EGL_GREEN_SIZE:
		*value = 8;
		break;
	case EGL_LEVEL:
		*value = 0;
		break;
	case EGL_LUMINANCE_SIZE:
		*value = 0;
		break;
	case EGL_MAX_PBUFFER_WIDTH:
		*value = 0;
		break;
	case EGL_MAX_PBUFFER_HEIGHT:
		*value = 0;
		break;
	case EGL_MAX_PBUFFER_PIXELS:
		*value = 0;
		break;
	case EGL_MAX_SWAP_INTERVAL:
		*value = 65535;
		break;
	case EGL_MIN_SWAP_INTERVAL:
		*value = 1;
		break;
	case EGL_NATIVE_RENDERABLE:
		*value = EGL_FALSE;
		break;
	case EGL_NATIVE_VISUAL_ID:
		*value = 0;
		break;
	case EGL_NATIVE_VISUAL_TYPE:
		*value = 0;
		break;
	case EGL_RED_SIZE:
		*value = 8;
		break;
	case EGL_RENDERABLE_TYPE:
		*value = EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENGL_BIT;
		break;
	case EGL_SAMPLE_BUFFERS:
		*value = 0;
		break;
	case EGL_SAMPLES:
		*value = 1;
		break;
	case EGL_STENCIL_SIZE:
		*value = 8;
		break;
	case EGL_SURFACE_TYPE:
		*value = EGL_WINDOW_BIT;
		break;
	case EGL_TRANSPARENT_TYPE:
		*value = EGL_NONE;
		break;
	case EGL_TRANSPARENT_RED_VALUE:
		*value = 0;
		break;
	case EGL_TRANSPARENT_GREEN_VALUE:
		*value = 0;
		break;
	case EGL_TRANSPARENT_BLUE_VALUE:
		*value = 0;
		break;
	default:
		SET_EGL_ERROR(EGL_BAD_ATTRIBUTE, EGL_FALSE)
	}

	EGL_RET(EGL_TRUE)
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
#ifndef SKIP_ERROR_HANDLING
	if (!num_config) {
		SET_EGL_ERROR(EGL_BAD_PARAMETER, EGL_FALSE)
	}
#endif
	*num_config = 1;
	if (configs)
		*configs = (EGLConfig)EGL_DEFAULT_CFG;

	EGL_RET(EGL_TRUE)
}

EGLBoolean eglGetConfigs(EGLDisplay display, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
#ifndef SKIP_ERROR_HANDLING
	if (!num_config) {
		SET_EGL_ERROR(EGL_BAD_PARAMETER, EGL_FALSE)
	}
#endif
	*num_config = 1;
	if (configs && config_size > 0) {
		*configs = (EGLConfig)EGL_DEFAULT_CFG;
	}

	EGL_RET(EGL_TRUE)
}

EGLBoolean eglSwapInterval(EGLDisplay display, EGLint interval) {
	vsync_interval = interval;
	EGL_RET(EGL_TRUE)
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
	vglSwapBuffers(GL_FALSE);
	EGL_RET(EGL_TRUE)
}

EGLBoolean eglBindAPI(EGLenum api) {
	switch (api) {
	case EGL_OPENGL_API:
	case EGL_OPENGL_ES_API:
		rend_api = api;
		egl_error = EGL_SUCCESS;
		break;
	default:
		SET_EGL_ERROR(EGL_BAD_PARAMETER, EGL_FALSE);
	}
	
	EGL_RET(EGL_TRUE)
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
	EGL_RET((EGLContext)EGL_DEFAULT_CTX)
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
#ifndef SKIP_ERROR_HANDLING
	if (ctx != (EGLContext)EGL_DEFAULT_CTX) {
		SET_EGL_ERROR(EGL_BAD_CONTEXT, EGL_FALSE);
	}
#endif
	EGL_RET(EGL_TRUE)
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, void * win, const EGLint *attrib_list) {
	EGL_RET((EGLSurface)EGL_DEFAULT_SURF)
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
#ifndef SKIP_ERROR_HANDLING
	if (surface != (EGLSurface)EGL_DEFAULT_SURF) {
		SET_EGL_ERROR(EGL_BAD_SURFACE, EGL_FALSE);
	}
#endif
	EGL_RET(EGL_TRUE)
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
	EGL_RET(EGL_TRUE)
}

EGLenum eglQueryAPI(void) {
	EGL_RET(rend_api)
}

EGLBoolean eglTerminate(EGLDisplay dpy) {
	EGL_RET(EGL_TRUE)
}

EGLContext eglGetCurrentContext(void) {
	EGL_RET((EGLContext)EGL_DEFAULT_CTX)
}

EGLint eglGetError(void) {
	EGLint ret = egl_error;
	EGL_RET(ret)
}

char const * eglQueryString(EGLDisplay display, EGLint name) {
	switch (name) {
	case EGL_CLIENT_APIS:
		EGL_RET("OpenGL OpenGL_ES");
	case EGL_VENDOR:
		EGL_RET("Rinnegatamante");
	case EGL_VERSION:
		EGL_RET("2.2 VitaGL");
	case EGL_EXTENSIONS:
		EGL_RET("EGL_KHR_image "
			"EGL_KHR_image_base "
			"EGL_KHR_image_pixmap "
			"EGL_KHR_gl_texture_2D_image "
			"EGL_KHR_gl_texture_cubemap_image "
			"EGL_KHR_gl_renderbuffer_image "
			"EGL_KHR_fence_sync "
			"EGL_NV_system_time "
			"EGL_ANDROID_image_native_buffer ");
	default:
		SET_EGL_ERROR(EGL_BAD_PARAMETER, NULL);
	}
}

void (*eglGetProcAddress(char const *procname))(void) {
	EGL_RET(vglGetProcAddress(procname));
}

EGLDisplay eglGetDisplay(NativeDisplayType native_display) {
	if (native_display == EGL_DEFAULT_DISPLAY) {
		EGL_RET((EGLDisplay)EGL_DEFAULT_DISP);
	} else {
		EGL_RET(EGL_NO_DISPLAY)
	}
}

EGLSurface eglGetCurrentSurface(EGLint readdraw) {
	EGL_RET((EGLSurface)EGL_DEFAULT_SURF)
}

EGLuint64 eglGetSystemTimeFrequencyNV(void) {
	EGL_RET((EGLuint64)sceRtcGetTickResolution())
}

EGLuint64 eglGetSystemTimeNV(void) {
	SceRtcTick t;
	sceRtcGetCurrentTick(&t);
	EGL_RET(t.tick)
}
