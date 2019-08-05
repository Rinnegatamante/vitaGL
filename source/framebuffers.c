/* 
 * framebuffers.c:
 * Implementation for framebuffers related functions
 */
 
#include "shared.h"

static framebuffer framebuffers[BUFFERS_NUM]; // Framebuffers array

framebuffer *active_read_fb = NULL; // Current readback framebuffer in use
framebuffer *active_write_fb = NULL; // Current write framebuffer in use

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */
 
 void glGenFramebuffers(GLsizei n, GLuint *ids) {
	int i = 0, j = 0;
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		error = GL_INVALID_VALUE;
		return;
	}
#endif
	for (i=0; i < BUFFERS_NUM; i++) {
		if (!framebuffers[i].active){
			ids[j++] = (GLuint)&framebuffers[i];
			framebuffers[i].active = 1;
			framebuffers[i].depthbuffer = NULL;
			framebuffers[i].colorbuffer = NULL;
			sceGxmSyncObjectCreate(&framebuffers[i].sync_object);
		}
		if (j >= n) break;
	}
}

void glDeleteFramebuffers(GLsizei n, GLuint *framebuffers) {
#ifndef SKIP_ERROR_HANDLING
	if (n < 0) {
		error = GL_INVALID_VALUE;
		return;
	}
#endif
	while (n > 0) {
		framebuffer *fb = (framebuffer*)framebuffers[n--];
		fb->active = 0;
		if (fb->colorbuffer) free(fb->colorbuffer);
		if (fb->target) sceGxmDestroyRenderTarget(fb->target);
		sceGxmSyncObjectDestroy(fb->sync_object);
	}
}

void glBindFramebuffer(GLenum target, GLuint fb) {
	switch (target) {
	case GL_DRAW_FRAMEBUFFER:
		active_write_fb = (framebuffer*)fb;
		break;
	case GL_READ_FRAMEBUFFER:
		active_read_fb = (framebuffer*)fb;
		break;
	case GL_FRAMEBUFFER:
		active_write_fb = active_read_fb = (framebuffer*)fb;
		break;
	default:
		error = GL_INVALID_ENUM;
		break;
	}
}

void glFramebufferTexture(GLenum target, GLenum attachment, GLuint tex_id, GLint level) {
	
	// Detecting requested framebuffer
	framebuffer *fb = NULL;
	switch (target) {
	case GL_DRAW_FRAMEBUFFER:
	case GL_FRAMEBUFFER:
		fb = active_write_fb;
		break;
	case GL_READ_FRAMEBUFFER:
		fb = active_read_fb;
		break;
	default:
		error = GL_INVALID_ENUM;
		break;
	}
	
	// Aliasing to make code more readable
	texture_unit *tex_unit = &texture_units[server_texture_unit];
	texture *tex = &tex_unit->textures[tex_id];
	
	// Detecting requested attachment
	switch (attachment) {
	case GL_COLOR_ATTACHMENT0:
		fb->colorbuffer = (SceGxmColorSurface*)malloc(sizeof(SceGxmColorSurface));
		sceGxmColorSurfaceInit(
			fb->colorbuffer,
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			msaa_mode == SCE_GXM_MULTISAMPLE_NONE ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			sceGxmTextureGetWidth(&tex->gxm_tex),
			sceGxmTextureGetHeight(&tex->gxm_tex),
			sceGxmTextureGetWidth(&tex->gxm_tex),
			sceGxmTextureGetData(&tex->gxm_tex)
		);
		SceGxmRenderTargetParams renderTargetParams;
		memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
		renderTargetParams.flags = 0;
		renderTargetParams.width = sceGxmTextureGetWidth(&tex->gxm_tex);
		renderTargetParams.height = sceGxmTextureGetHeight(&tex->gxm_tex);
		renderTargetParams.scenesPerFrame = 1;
		renderTargetParams.multisampleMode = msaa_mode;
		renderTargetParams.multisampleLocations = 0;
		renderTargetParams.driverMemBlock = -1;
		sceGxmCreateRenderTarget(&renderTargetParams, &fb->target);
		break;
	default:
		error = GL_INVALID_ENUM;
		break;	
	}
	
}
