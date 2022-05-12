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

#ifndef DISABLE_TEXTURE_COMBINER
#define NUM_EXTENSIONS 27 // Number of supported extensions
#else
#define NUM_EXTENSIONS 26 // Number of supported extensions
#endif
#define COMPRESSED_TEXTURE_FORMATS_NUM 24 // The number of supported texture formats

static GLubyte *extensions[NUM_EXTENSIONS] = {
	"GL_OES_vertex_half_float",
	"GL_OES_texture_npot",
	"GL_OES_rgb8_rgba8",
	"GL_OES_depth_texture",
	"GL_OES_framebuffer_object",
	"GL_OES_compressed_paletted_texture",
	"GL_EXT_texture_format_BGRA8888",
	"GL_EXT_read_format_bgra",
	"GL_EXT_abgr",
	"GL_EXT_texture_compression_dxt1",
	"GL_EXT_texture_compression_dxt3",
	"GL_EXT_texture_compression_dxt5",
	"GL_EXT_texture_compression_s3tc",
	"GL_IMG_texture_compression_pvrtc",
	"GL_OES_compressed_ETC1_RGB8_texture",
	"GL_EXT_texture_env_add",
	"GL_WIN_phong_shading",
	"GL_AMD_compressed_ATC_texture",
	"GL_ARB_multitexture",
	"GL_EXT_map_buffer_range",
	"GL_OES_mapbuffer",
	"GL_OES_depth24",
	"GL_OES_packed_depth_stencil",
	"GL_NVX_gpu_memory_info",
	"GL_EXT_color_buffer_half_float",
	"GL_OES_texture_half_float",
#ifndef DISABLE_TEXTURE_COMBINER
	"GL_EXT_texture_env_combine"
#endif
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
		return "OpenGL ES 2.0 VitaGL";
	case GL_EXTENSIONS: // Supported extensions
		if (!extension) {
			int i, size = 0;
			for (i = 0; i < NUM_EXTENSIONS; i++) {
				size += strlen(extensions[i]) + 1;
			}
			extension = vglMalloc(size + 1);
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
	case GL_SHADER_COMPILER:
		*params = GL_TRUE;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
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
	case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
		*data = 1.0f;
		break;
	case GL_DEPTH_BITS:
		*data = 32;
		break;
	case GL_STENCIL_BITS:
		*data = 8;
		break;
	case GL_PACK_ALIGNMENT:
		*data = 1;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

void glGetIntegerv(GLenum pname, GLint *data) {
	// Aliasing to make code more readable
	texture_unit *server_tex_unit = &texture_units[server_texture_unit];

	switch (pname) {
	case GL_BLEND_DST_RGB:
		*data = gxm_blend_to_gl(blend_dfactor_rgb);
		break;
	case GL_BLEND_SRC_RGB:
		*data = gxm_blend_to_gl(blend_sfactor_rgb);
		break;
	case GL_BLEND_DST_ALPHA:
		*data = gxm_blend_to_gl(blend_dfactor_a);
		break;
	case GL_BLEND_SRC_ALPHA:
		*data = gxm_blend_to_gl(blend_sfactor_a);
		break;
	case GL_CURRENT_PROGRAM:
		*data = cur_program;
		break;
	case GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX:
		*data = vgl_mem_get_total_space(VGL_MEM_VRAM) / 1024;
		break;
	case GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX:
		*data = vgl_mem_get_total_space(VGL_MEM_ALL) / 1024;
		break;
	case GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX:
		*data = vgl_mem_get_free_space(VGL_MEM_VRAM) / 1024;
		break;
	case GL_CULL_FACE:
		*data = cull_face_state;
		break;
	case GL_PROGRAM_ERROR_POSITION_ARB:
		*data = -1;
		break;
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
		*data = 32;
		break;
	case GL_STENCIL_BITS:
		*data = 8;
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
		*data = COMPRESSED_TEXTURE_FORMATS_NUM;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		data[0] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		data[1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		data[2] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		data[3] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		data[4] = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
		data[5] = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
		data[6] = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		data[7] = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
		data[8] = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
		data[9] = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
		data[10] = GL_ATC_RGB_AMD;
		data[11] = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
		data[12] = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
		data[13] = GL_PALETTE4_RGB8_OES;
		data[14] = GL_PALETTE4_RGBA8_OES;
		data[15] = GL_PALETTE4_R5_G6_B5_OES;
		data[16] = GL_PALETTE4_RGBA4_OES;
		data[17] = GL_PALETTE4_RGB5_A1_OES;
		data[18] = GL_PALETTE8_RGB8_OES;
		data[19] = GL_PALETTE8_RGBA8_OES;
		data[20] = GL_PALETTE8_R5_G6_B5_OES;
		data[21] = GL_PALETTE8_RGBA4_OES;
		data[22] = GL_PALETTE8_RGB5_A1_OES;
		data[23] = GL_ETC1_RGB8_OES;
		break;
	case GL_NUM_SHADER_BINARY_FORMATS:
		*data = 0;
		break;
	case GL_SHADER_BINARY_FORMATS:
		break;
	case GL_FRAMEBUFFER_BINDING:
		*data = (GLint)active_write_fb;
		break;
	case GL_RENDERBUFFER_BINDING:
		*data = (GLint)active_rb;
		break;
	case GL_READ_FRAMEBUFFER_BINDING:
		*data = (GLint)active_read_fb;
		break;
	case GL_MAX_VERTEX_ATTRIBS:
		*data = VERTEX_ATTRIBS_NUM;
		break;
	case GL_MAX_VERTEX_UNIFORM_VECTORS:
		*data = 128;
		break;
	case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
		*data = 16;
		break;
	case GL_MAX_VARYING_VECTORS:
		*data = 8;
		break;
	case GL_MAJOR_VERSION:
		*data = 2;
		break;
	case GL_MINOR_VERSION:
		*data = 0;
		break;
	case GL_NUM_EXTENSIONS:
		*data = NUM_EXTENSIONS;
		break;
	case GL_MAX_TEXTURE_IMAGE_UNITS:
		*data = TEXTURE_IMAGE_UNITS_NUM;
		break;
	case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
		*data = COMBINED_TEXTURE_IMAGE_UNITS_NUM;
		break;
	case GL_MAX_TEXTURE_COORDS:
		*data = TEXTURE_COORDS_NUM;
		break;
	case GL_MAX_TEXTURE_UNITS:
		*data = TEXTURE_COORDS_NUM;
		break;
	case GL_PACK_ALIGNMENT:
		*data = 1;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
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
