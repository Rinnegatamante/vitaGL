/* 
 * get_info.c:
 * Implementation for functions returning info to end user
 */

#include "shared.h"
 
// Constants returned by glGetString
static const GLubyte* vendor = "Rinnegatamante";
static const GLubyte* renderer = "SGX543MP4+";
static const GLubyte* version = "VitaGL 1.0";
static const GLubyte* extensions = "VGL_EXT_gpu_objects_array VGL_EXT_gxp_shaders";

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

const GLubyte* glGetString(GLenum name){
	switch (name){
	case GL_VENDOR:
		return vendor;
		break;
	case GL_RENDERER:
		return renderer;
		break;
	case GL_VERSION:
		return version;
		break;
	case GL_EXTENSIONS:
		return extensions;
		break;
	default:
		error = GL_INVALID_ENUM;
		return NULL;
		break;
	}
}

void glGetBooleanv(GLenum pname, GLboolean* params){
	switch (pname){
	case GL_BLEND:
		*params = blend_state;
		break;
	case GL_BLEND_DST_ALPHA:
		*params = (blend_dfactor_a == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_BLEND_DST_RGB:
		*params = (blend_dfactor_rgb == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_BLEND_SRC_ALPHA:
		*params = (blend_sfactor_a == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_BLEND_SRC_RGB:
		*params = (blend_sfactor_rgb == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_DEPTH_TEST:
		*params = depth_test_state;
		break;
	case GL_ACTIVE_TEXTURE:
		*params = GL_FALSE;
		break;
	default:
		error = GL_INVALID_ENUM;
		break;
	}
}

void glGetFloatv(GLenum pname, GLfloat* data){
	switch (pname){
	case GL_POLYGON_OFFSET_FACTOR:
		*data = pol_factor;
		break;
	case GL_POLYGON_OFFSET_UNITS:
		*data = pol_units;
		break;
	case GL_MODELVIEW_MATRIX:
		memcpy(data, &modelview_matrix, sizeof(matrix4x4));
		break;
	case GL_ACTIVE_TEXTURE:
		*data = (1.0f * (server_texture_unit + GL_TEXTURE0));
		break;
	case GL_MAX_MODELVIEW_STACK_DEPTH:
		*data = MODELVIEW_STACK_DEPTH;
		break;
	case GL_MAX_PROJECTION_STACK_DEPTH:
		*data = GENERIC_STACK_DEPTH;
		break;
	case GL_MAX_TEXTURE_STACK_DEPTH:
		*data = GENERIC_STACK_DEPTH;
		break;
	default:
		error = GL_INVALID_ENUM;
		break;
	}
}