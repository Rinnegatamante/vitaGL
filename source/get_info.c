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
 * get_info.c:
 * Implementation for functions returning info to end user
 */

#include <string.h>

#include "shared.h"

#define NUM_EXTENSIONS 11 // Number of supported extensions

static GLubyte *extensions[NUM_EXTENSIONS] = {
	"GL_OES_vertex_half_float",
	"GL_OES_texture_npot",
	"GL_OES_rgb8_rgba8",
	"GL_OES_depth_texture",
	"GL_EXT_texture_format_BGRA8888",
	"GL_EXT_read_format_bgra",
	"GL_EXT_texture_compression_dxt1",
	"GL_EXT_texture_compression_dxt3",
	"GL_EXT_texture_compression_dxt5",
	"GL_EXT_texture_compression_s3tc",
	"GL_IMG_texture_compression_pvrtc"
};
static GLubyte *extension = NULL;

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

const GLubyte *glGetString(GLenum name) {
	switch (name) {
	case GL_VENDOR: // Vendor
		return "Rinnegatamante";
	case GL_RENDERER: // Renderer
		return "SGX543MP4+";
	case GL_VERSION: // openGL Version
		return "OpenGL ES 2 VitaGL";
	case GL_EXTENSIONS: // Supported extensions
		if (!extension) {
			int i, size = 0;
			for (i = 0; i < NUM_EXTENSIONS; i++) {
				size += strlen(extensions[i]) + 1;
			}
			extension = vgl_malloc(size + 1, VGL_MEM_EXTERNAL);
			extension[0] = 0;
			for (i = 0; i < NUM_EXTENSIONS; i++) {
				sprintf(extension, "%s%s ", extension, extensions[i]);
			}
			extension[size - 1] = 0;
		}
		return extension;
	case GL_SHADING_LANGUAGE_VERSION: // Supported shading language version
		return "2.00 NVIDIA via Cg compiler";
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL)
	}
}

const GLubyte *glGetStringi(GLenum name, GLuint index) {
	switch (name) {
	case GL_EXTENSIONS:
		return extensions[index];
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, NULL)
	}
}

void glGetBooleanv(GLenum pname, GLboolean *params) {
	switch (pname) {
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
	case GL_STENCIL_TEST:
		*params = stencil_test_state;
		break;
	case GL_SCISSOR_TEST:
		*params = scissor_test_state;
		break;
	case GL_CULL_FACE:
		*params = cull_face_state;
		break;
	case GL_POLYGON_OFFSET_FILL:
		*params = pol_offset_fill;
		break;
	case GL_POLYGON_OFFSET_LINE:
		*params = pol_offset_line;
		break;
	case GL_POLYGON_OFFSET_POINT:
		*params = pol_offset_point;
		break;
	case GL_DEPTH_WRITEMASK:
		*params = depth_mask_state;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glGetFloatv(GLenum pname, GLfloat *data) {
	int i, j;
	switch (pname) {
	case GL_POLYGON_OFFSET_FACTOR: // Polygon offset factor
		*data = pol_factor;
		break;
	case GL_POLYGON_OFFSET_UNITS: // Polygon offset units
		*data = pol_units;
		break;
	case GL_MODELVIEW_MATRIX: // Modelview matrix
		// Since we use column-major matrices internally, wee need to transpose it before returning it to the application
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				data[i * 4 + j] = modelview_matrix[j][i];
			}
		}
		break;
	case GL_PROJECTION_MATRIX: // Projection matrix
		// Since we use column-major matrices internally, wee need to transpose it before returning it to the application
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				data[i * 4 + j] = projection_matrix[j][i];
			}
		}
		break;
	case GL_TEXTURE_MATRIX: // Texture matrix
		// Since we use column-major matrices internally, wee need to transpose it before returning it to the application
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				data[i * 4 + j] = texture_matrix[j][i];
			}
		}
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
	case GL_DEPTH_BITS:
		*data = 32;
		break;
	case GL_STENCIL_BITS:
		*data = 8;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glGetIntegerv(GLenum pname, GLint *data) {
	// Aliasing to make code more readable
	texture_unit *server_tex_unit = &texture_units[server_texture_unit];

	switch (pname) {
	case GL_POLYGON_MODE:
		data[0] = gl_polygon_mode_front;
		data[1] = gl_polygon_mode_back;
		break;
	case GL_SCISSOR_BOX:
		data[0] = region.x;
		data[1] = region.y;
		data[2] = region.w;
		data[3] = region.h;
		break;
	case GL_TEXTURE_BINDING_2D:
		*data = server_tex_unit->tex_id;
		break;
	case GL_MAX_TEXTURE_SIZE:
		*data = GXM_TEX_MAX_SIZE;
		break;
	case GL_MAX_CLIP_PLANES:
		*data = MAX_CLIP_PLANES_NUM;
		break;
	case GL_VIEWPORT:
		data[0] = gl_viewport.x;
		data[1] = gl_viewport.y;
		data[2] = gl_viewport.w;
		data[3] = gl_viewport.h;
		break;
	case GL_DEPTH_BITS:
		data[0] = 32;
		break;
	case GL_STENCIL_BITS:
		data[0] = 8;
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
		*data = COMPRESSED_TEXTURE_FORMATS_NUM;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		data[0] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		data[1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		data[2] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		data[3] = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
		data[4] = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
		data[5] = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		data[6] = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
		data[7] = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
		data[8] = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
		break;
	case GL_FRAMEBUFFER_BINDING:
		data[0] = (GLint)active_write_fb;
		break;
	case GL_READ_FRAMEBUFFER_BINDING:
		data[0] = (GLint)active_read_fb;
		break;
	case GL_MAX_VERTEX_UNIFORM_VECTORS:
		data[0] = 256;
		break;
	case GL_MAJOR_VERSION:	
		data[0] = 2;
		break;
	case GL_MINOR_VERSION:
		data[0] = 0;
		break;
	case GL_NUM_EXTENSIONS:
		data[0] = NUM_EXTENSIONS;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

GLboolean glIsEnabled(GLenum cap) {
	GLboolean ret = GL_FALSE;
	switch (cap) {
	case GL_LIGHTING:
		ret = lighting_state;
		break;
	case GL_DEPTH_TEST:
		ret = depth_test_state;
		break;
	case GL_STENCIL_TEST:
		ret = stencil_test_state;
		break;
	case GL_BLEND:
		ret = blend_state;
		break;
	case GL_SCISSOR_TEST:
		ret = scissor_test_state;
		break;
	case GL_CULL_FACE:
		ret = cull_face_state;
		break;
	case GL_POLYGON_OFFSET_FILL:
		ret = pol_offset_fill;
		break;
	case GL_POLYGON_OFFSET_LINE:
		ret = pol_offset_line;
		break;
	case GL_POLYGON_OFFSET_POINT:
		ret = pol_offset_point;
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
	case GL_CLIP_PLANE6:
		ret = clip_planes_mask & (1 << (cap - GL_CLIP_PLANE0)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		ret = light_mask & (1 << (cap - GL_LIGHT0)) ? GL_TRUE : GL_FALSE;
		break;
	default:
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, GL_FALSE)
	}
	return ret;
}

GLenum glGetError(void) {
	GLenum ret = vgl_error;
	vgl_error = GL_NO_ERROR;
	return ret;
}

GLboolean glIsTexture(GLuint i) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR_WITH_RET(GL_INVALID_ENUM, GL_FALSE)
	}
#endif

	return (i < TEXTURES_NUM && texture_slots[i].status != TEX_UNUSED);
}

GLboolean glIsFramebuffer(GLuint fb) {
	framebuffer *p = (framebuffer *)fb;
	return (p && p->active);
}
