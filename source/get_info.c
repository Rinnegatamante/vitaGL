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
	case GL_VENDOR: // Vendor
		return vendor;
		break;
	case GL_RENDERER: // Renderer
		return renderer;
		break;
	case GL_VERSION: // openGL Version
		return version;
		break;
	case GL_EXTENSIONS: // Supported extensions
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
	case GL_BLEND: // Blending feature state
		*params = blend_state;
		break;
	case GL_BLEND_DST_ALPHA: // Blend Alpha Factor for Destination
		*params = (blend_dfactor_a == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_BLEND_DST_RGB: // Blend RGB Factor for Destination
		*params = (blend_dfactor_rgb == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_BLEND_SRC_ALPHA: // Blend Alpha Factor for Source
		*params = (blend_sfactor_a == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_BLEND_SRC_RGB: // Blend RGB Factor for Source
		*params = (blend_sfactor_rgb == SCE_GXM_BLEND_FACTOR_ZERO) ? GL_FALSE : GL_TRUE;
		break;
	case GL_DEPTH_TEST: // Depth test state
		*params = depth_test_state;
		break;
	case GL_ACTIVE_TEXTURE: // Active texture
		*params = GL_FALSE;
		break;
	default:
		error = GL_INVALID_ENUM;
		break;
	}
}

void glGetFloatv(GLenum pname, GLfloat* data){
	switch (pname){
	case GL_POLYGON_OFFSET_FACTOR: // Polygon offset factor
		*data = pol_factor;
		break;
	case GL_POLYGON_OFFSET_UNITS: // Polygon offset units
		*data = pol_units;
		break;
	case GL_MODELVIEW_MATRIX: // Modelview matrix
		memcpy(data, &modelview_matrix, sizeof(matrix4x4));
		break;
	case GL_ACTIVE_TEXTURE: // Active texture
		*data = (1.0f * (server_texture_unit + GL_TEXTURE0));
		break;
	case GL_MAX_MODELVIEW_STACK_DEPTH: // Max modelview stack depth
		*data = MODELVIEW_STACK_DEPTH;
		break;
	case GL_MAX_PROJECTION_STACK_DEPTH: // Max projection stack depth
		*data = GENERIC_STACK_DEPTH;
		break;
	case GL_MAX_TEXTURE_STACK_DEPTH: // Max texture stack depth
		*data = GENERIC_STACK_DEPTH;
		break;
	default:
		error = GL_INVALID_ENUM;
		break;
	}
}