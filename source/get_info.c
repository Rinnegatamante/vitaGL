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

#define COMPRESSED_TEXTURE_FORMATS_NUM 25 // The number of supported texture formats

#define NUM_EXTENSIONS (sizeof(extensions) / sizeof(GLubyte *))
static GLubyte *extensions[] = {
	"GL_AMD_compressed_ATC_texture",
	"GL_ARB_fragment_shader",
	"GL_ARB_framebuffer_object",
	"GL_ARB_get_program_binary",
	"GL_ARB_multitexture",
	"GL_ARB_sampler_objects",
#ifdef HAVE_GLSL_TRANSLATOR
	"GL_ARB_shading_language_100",
#endif
	"GL_ARB_texture_compression",
	"GL_ARB_vertex_buffer_object",
	"GL_EXT_abgr",
	"GL_EXT_bgra",
	"GL_EXT_color_buffer_half_float",
	"GL_EXT_debug_marker",
	"GL_EXT_direct_state_access",
	"GL_EXT_draw_instanced",
	"GL_EXT_framebuffer_object",
	"GL_EXT_map_buffer_range",
	"GL_EXT_packed_depth_stencil",
	"GL_EXT_packed_float",
	"GL_EXT_read_format_bgra",
	"GL_EXT_texture_compression_dxt1",
	"GL_EXT_texture_compression_dxt3",
	"GL_EXT_texture_compression_dxt5",
	"GL_EXT_texture_compression_s3tc",
	"GL_EXT_texture_env_add",
#ifndef DISABLE_TEXTURE_COMBINER
	"GL_EXT_texture_env_combine",
#endif
	"GL_EXT_texture_format_BGRA8888",
	"GL_IMG_texture_compression_pvrtc",
	"GL_IMG_user_clip_plane",
	"GL_NVX_gpu_memory_info",
	"GL_NV_fbo_color_attachments",
	"GL_OES_compressed_ETC1_RGB8_texture",
	"GL_OES_compressed_paletted_texture",
	"GL_OES_depth24",
	"GL_OES_framebuffer_object",
	"GL_OES_get_program_binary",
	"GL_OES_mapbuffer",
	"GL_OES_packed_depth_stencil",
	"GL_OES_rgb8_rgba8",
	"GL_OES_texture_float",
	"GL_OES_texture_half_float",
	"GL_OES_texture_half_float_linear",
	"GL_OES_texture_npot",
	"GL_OES_vertex_array_object",
	"GL_OES_vertex_half_float",
	"GL_WIN_phong_shading",
};
static GLubyte *extension = NULL;

GLint gxm_vtx_fmt_to_gl(SceGxmAttributeFormat attr) {
	switch (attr) {
	case SCE_GXM_ATTRIBUTE_FORMAT_F32:
		return GL_FLOAT;
	case SCE_GXM_ATTRIBUTE_FORMAT_S16N:
	case SCE_GXM_ATTRIBUTE_FORMAT_S16:
		return GL_SHORT;
	case SCE_GXM_ATTRIBUTE_FORMAT_S8N:
	case SCE_GXM_ATTRIBUTE_FORMAT_S8:
		return GL_BYTE;
	case SCE_GXM_ATTRIBUTE_FORMAT_U16N:
	case SCE_GXM_ATTRIBUTE_FORMAT_U16:
		return GL_UNSIGNED_SHORT;
	case SCE_GXM_ATTRIBUTE_FORMAT_U8N:
	case SCE_GXM_ATTRIBUTE_FORMAT_U8:
		return GL_UNSIGNED_BYTE;
	default:
		return 0;
	}
}

GLint gxm_depth_func_to_gl(SceGxmDepthFunc func) {
	// Properly translating openGL function to sceGxm one
	switch (func) {
	case SCE_GXM_DEPTH_FUNC_NEVER:
		return GL_NEVER;
	case SCE_GXM_DEPTH_FUNC_LESS:
		return GL_LESS;
	case SCE_GXM_DEPTH_FUNC_EQUAL:
		return GL_EQUAL;
	case SCE_GXM_DEPTH_FUNC_LESS_EQUAL:
		return GL_LEQUAL;
	case SCE_GXM_DEPTH_FUNC_GREATER:
		return GL_GREATER;
	case SCE_GXM_DEPTH_FUNC_NOT_EQUAL:
		return GL_NOTEQUAL;
	case SCE_GXM_DEPTH_FUNC_GREATER_EQUAL:
		return GL_GEQUAL;
	case SCE_GXM_DEPTH_FUNC_ALWAYS:
		return GL_ALWAYS;
	default:
		return 0;
	}
}

GLenum gxm_stencil_func_to_gl(SceGxmStencilFunc func) {
	switch (func) {
	case SCE_GXM_STENCIL_FUNC_NEVER:
		return GL_NEVER;
	case SCE_GXM_STENCIL_FUNC_LESS:
		return GL_LESS;
	case SCE_GXM_STENCIL_FUNC_LESS_EQUAL:
		return GL_LEQUAL;
	case SCE_GXM_STENCIL_FUNC_GREATER:
		return GL_GREATER;
	case SCE_GXM_STENCIL_FUNC_GREATER_EQUAL:
		return GL_GEQUAL;
	case SCE_GXM_STENCIL_FUNC_EQUAL:
		return GL_EQUAL;
	case SCE_GXM_STENCIL_FUNC_NOT_EQUAL:
		return GL_NOTEQUAL;
	case SCE_GXM_STENCIL_FUNC_ALWAYS:
		return GL_ALWAYS;
	default:
		return 0;
	}
}

GLenum gxm_stencil_op_to_gl(SceGxmStencilOp op) {
	switch (op) {
	case SCE_GXM_STENCIL_OP_KEEP:
		return GL_KEEP;
	case SCE_GXM_STENCIL_OP_ZERO:
		return GL_ZERO;
	case SCE_GXM_STENCIL_OP_REPLACE:
		return GL_REPLACE;
	case SCE_GXM_STENCIL_OP_INCR:
		return GL_INCR;
	case SCE_GXM_STENCIL_OP_INCR_WRAP:
		return GL_INCR_WRAP;
	case SCE_GXM_STENCIL_OP_DECR:
		return GL_DECR;
	case SCE_GXM_STENCIL_OP_DECR_WRAP:
		return GL_DECR_WRAP;
	case SCE_GXM_STENCIL_OP_INVERT:
		return GL_INVERT;
	default:
		return 0;
	}
}

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
				strcat(extension, extensions[i]);
				if (i != (NUM_EXTENSIONS - 1))
					strcat(extension, " ");
			}
		}
		return extension;
	case GL_SHADING_LANGUAGE_VERSION: // Supported shading language version
#ifdef HAVE_GLSL_TRANSLATOR
		return "1.00 ES";
#else
		return "2.00 NVIDIA via Cg compiler";
#endif
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
	case GL_COLOR_WRITEMASK:
		params[0] = (blend_color_mask & SCE_GXM_COLOR_MASK_R) ? GL_TRUE : GL_FALSE;
		params[1] = (blend_color_mask & SCE_GXM_COLOR_MASK_G) ? GL_TRUE : GL_FALSE;
		params[2] = (blend_color_mask & SCE_GXM_COLOR_MASK_B) ? GL_TRUE : GL_FALSE;
		params[3] = (blend_color_mask & SCE_GXM_COLOR_MASK_A) ? GL_TRUE : GL_FALSE;
		break;
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
	case GL_ALPHA_TEST_REF:
		*data = vgl_alpha_ref;
		break;
	case GL_DEPTH_CLEAR_VALUE:
		data[0] = depth_value;
		break;
	case GL_DEPTH_RANGE:
		data[0] = z_port - z_scale;
		data[1] = z_port + z_scale;
		break;
	case GL_COLOR_CLEAR_VALUE:
		sceClibMemcpy(data, &clear_rgba_val.r, 4 * sizeof(float));
		break;
	case GL_CURRENT_COLOR:
		sceClibMemcpy(data, &current_vtx.clr.r, 4 * sizeof(float));
		break;
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
				data[i * 4 + j] = texture_matrix[server_texture_unit][j][i];
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

void glGetDoublev(GLenum pname, GLdouble *data) {
	int i, j;
	switch (pname) {
	case GL_ALPHA_TEST_REF:
		*data = vgl_alpha_ref;
		break;
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
				data[i * 4 + j] = texture_matrix[server_texture_unit][j][i];
			}
		}
		break;
	case GL_ACTIVE_TEXTURE: // Active texture
		*data = (double)(server_texture_unit + GL_TEXTURE0);
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
	case GL_ALPHA_TEST_REF:
		*data = vgl_alpha_ref;
		break;
	case GL_SHADE_MODEL:
		*data = shading_mode == SMOOTH ? GL_SMOOTH : GL_PHONG_WIN;
		break;
	case GL_STENCIL_FAIL:
		*data = gxm_stencil_op_to_gl(stencil_fail_front);
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL:
		*data = gxm_stencil_op_to_gl(depth_fail_front);
		break;
	case GL_STENCIL_PASS_DEPTH_PASS:
		*data = gxm_stencil_op_to_gl(depth_pass_front);
		break;
	case GL_STENCIL_VALUE_MASK:
		*data = stencil_mask_front;
		break;
	case GL_STENCIL_REF:
		*data = stencil_ref_front;
		break;
	case GL_STENCIL_FUNC:
		*data = gxm_stencil_func_to_gl(stencil_func_front);
		break;
	case GL_FRONT_FACE:
		*data = gl_front_face;
		break;
	case GL_CULL_FACE_MODE:
		*data = gl_cull_mode;
		break;
	case GL_STENCIL_WRITEMASK:
		*data = stencil_mask_front_write;
		break;
	case GL_DEPTH_WRITEMASK:
		*data = depth_mask_state;
		break;
	case GL_COLOR_WRITEMASK:
		data[0] = (blend_color_mask & SCE_GXM_COLOR_MASK_R) ? GL_TRUE : GL_FALSE;
		data[1] = (blend_color_mask & SCE_GXM_COLOR_MASK_G) ? GL_TRUE : GL_FALSE;
		data[2] = (blend_color_mask & SCE_GXM_COLOR_MASK_B) ? GL_TRUE : GL_FALSE;
		data[3] = (blend_color_mask & SCE_GXM_COLOR_MASK_A) ? GL_TRUE : GL_FALSE;
		break;
	case GL_STENCIL_CLEAR_VALUE:
		*data = stencil_value;
		break;
	case GL_MAX_VERTEX_UNIFORM_COMPONENTS:
	case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS:
		*data = 2048;
		break;
	case GL_MAX_VARYING_FLOATS:
		*data = 40;
		break;
	case GL_MAX_COLOR_ATTACHMENTS:
		*data = 1;
		break;
	case GL_SAMPLER_BINDING:
		*data = (GLint)samplers[server_texture_unit];
		break;
	case GL_DOUBLEBUFFER:
		*data = GL_TRUE;
		break;
	case GL_ALPHA_BITS:
		*data = active_write_fb ? get_alpha_channel_size(sceGxmColorSurfaceGetFormat(&active_write_fb->colorbuffer)) : 8;
		break;
	case GL_BLEND_EQUATION:
		*data = gxm_blend_eq_to_gl(blend_func_rgb);
		break;
	case GL_BLEND_EQUATION_ALPHA:
		*data = gxm_blend_eq_to_gl(blend_func_a);
		break;
	case GL_MAX_LIGHTS:
		*data = MAX_LIGHTS_NUM;
		break;
	case GL_VERTEX_ARRAY_SIZE:
		*data = ffp_vertex_attrib_config[0].componentCount;
		break;
	case GL_VERTEX_ARRAY_TYPE:
		*data = ffp_vertex_attrib_fixed_pos_mask ? GL_FIXED : gxm_vtx_fmt_to_gl(ffp_vertex_attrib_config[0].format);
		break;
	case GL_VERTEX_ARRAY_STRIDE:
		*data = ffp_vertex_stream_config[0].stride;
		break;
	case GL_NORMAL_ARRAY_TYPE:
		*data = (ffp_vertex_attrib_fixed_mask & (1 << 0)) ? GL_FIXED : gxm_vtx_fmt_to_gl(ffp_vertex_attrib_config[6].format);
		break;
	case GL_NORMAL_ARRAY_STRIDE:
		*data = ffp_vertex_stream_config[6].stride;
		break;
	case GL_COLOR_ARRAY_SIZE:
		*data = ffp_vertex_attrib_config[2].componentCount;
		break;
	case GL_COLOR_ARRAY_TYPE:
		*data = gxm_vtx_fmt_to_gl(ffp_vertex_attrib_config[2].format);
		break;
	case GL_COLOR_ARRAY_STRIDE:
		*data = ffp_vertex_stream_config[2].stride;
		break;
	case GL_TEXTURE_COORD_ARRAY_SIZE:
		*data = ffp_vertex_attrib_config[texcoord_idxs[client_texture_unit]].componentCount;
		break;
	case GL_TEXTURE_COORD_ARRAY_TYPE:
		*data = (ffp_vertex_attrib_fixed_mask & (1 << texcoord_fixed_idxs[client_texture_unit])) ? GL_FIXED : gxm_vtx_fmt_to_gl(ffp_vertex_attrib_config[texcoord_idxs[client_texture_unit]].format);
		break;
	case GL_TEXTURE_COORD_ARRAY_STRIDE:
		*data = ffp_vertex_stream_config[texcoord_idxs[client_texture_unit]].stride;
		break;
	case GL_UNPACK_ROW_LENGTH:
		*data = unpack_row_len;
		break;
	case GL_UNPACK_ALIGNMENT:
		*data = 1;
		break;
	case GL_ARRAY_BUFFER_BINDING:
		*data = vertex_array_unit;
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		*data = cur_vao->index_array_unit;
		break;
	case GL_MAX_ELEMENTS_INDICES:
	case GL_MAX_ELEMENTS_VERTICES:
		*data = 0x7FFFFFFF;
		break;
	case GL_RED_BITS:
	case GL_GREEN_BITS:
	case GL_BLUE_BITS:
		*data = 8;
		break;
	case GL_BLEND_DST:
	case GL_BLEND_DST_RGB:
		*data = gxm_blend_to_gl(blend_dfactor_rgb);
		break;
	case GL_BLEND_SRC:
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
		*data = (GLint)server_tex_unit->tex_id[0];
		break;
	case GL_TEXTURE_BINDING_CUBE_MAP:
		*data = (GLint)server_tex_unit->tex_id[2];
		break;
	case GL_MAX_VIEWPORT_DIMS:
		data[0] = GXM_TEX_MAX_SIZE;
		data[1] = GXM_TEX_MAX_SIZE;
		break;
	case GL_MAX_TEXTURE_SIZE:
	case GL_MAX_RENDERBUFFER_SIZE:
		*data = GXM_TEX_MAX_SIZE;
		break;
	case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
		*data = GXM_TEX_MAX_SIZE / 4;
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
		data[24] = GL_COMPRESSED_RGBA8_ETC2_EAC;
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
	case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
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
	case GL_ACTIVE_TEXTURE:
		*data = GL_TEXTURE0 + server_texture_unit;
		break;
	case GL_CLIENT_ACTIVE_TEXTURE:
		*data = GL_TEXTURE0 + client_texture_unit;
		break;
	case GL_MATRIX_MODE:
		*data = get_gl_matrix_mode();
		break;
	case GL_DEPTH_FUNC:
		*data = gxm_depth_func_to_gl(depth_func);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, pname)
	}
}

GLboolean glIsEnabled(GLenum cap) {
	GLboolean ret = GL_FALSE;
	switch (cap) {
	case GL_ALPHA_TEST:
		ret = alpha_test_state;
		break;
	case GL_TEXTURE_1D:
		ret = texture_units[server_texture_unit].state & (1 << 0);
		break;
	case GL_TEXTURE_2D:
		ret = texture_units[server_texture_unit].state & (1 << 1);
		break;
	case GL_NORMALIZE:
		ret = normalize;
		break;
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
		ret = (clip_planes_mask & (1 << (cap - GL_CLIP_PLANE0))) ? GL_TRUE : GL_FALSE;
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		ret = (light_mask & (1 << (cap - GL_LIGHT0))) ? GL_TRUE : GL_FALSE;
		break;
	case GL_VERTEX_ARRAY:
		ret = (ffp_vertex_attrib_state & (1 << 0)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_NORMAL_ARRAY:
		ret = (ffp_vertex_attrib_state & (1 << 3)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_COLOR_ARRAY:
		ret = (ffp_vertex_attrib_state & (1 << 2)) ? GL_TRUE : GL_FALSE;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		ret = (ffp_vertex_attrib_state & (1 << texcoord_idxs[client_texture_unit])) ? GL_TRUE : GL_FALSE;
		break;
	case GL_FOG:
		ret = fogging;
		break;
	default:
		SET_GL_ERROR_WITH_RET_AND_VALUE(GL_INVALID_ENUM, GL_FALSE, cap)
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
