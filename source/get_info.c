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

GLboolean glIsEnabled(GLenum cap){
	GLboolean ret = GL_FALSE;
	switch (cap){
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
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	return ret;
}

void glClear(GLbitfield mask){
	GLenum orig_depth_test = depth_test_state;
	if ((mask & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT){
		invalidate_depth_test();
		change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
		sceGxmSetVertexProgram(gxm_context, clear_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, clear_fragment_program_patched);
		void *color_buffer;
		sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &color_buffer);
		sceGxmSetUniformDataF(color_buffer, clear_color, 0, 4, &clear_rgba_val.r);
		sceGxmSetVertexStream(gxm_context, 0, clear_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);
		validate_depth_test();
		change_depth_write((depth_mask_state && orig_depth_test) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front);
		sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back);
		drawing = 1;
	}
	if ((mask & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT){
		invalidate_depth_test();
		change_depth_write(SCE_GXM_DEPTH_WRITE_ENABLED);
		depth_vertices[0].position.z = depth_value;
		depth_vertices[1].position.z = depth_value;
		depth_vertices[2].position.z = depth_value;
		depth_vertices[3].position.z = depth_value;
		sceGxmSetVertexProgram(gxm_context, disable_color_buffer_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, disable_color_buffer_fragment_program_patched);
		sceGxmSetVertexStream(gxm_context, 0, depth_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);
		validate_depth_test();
		change_depth_write((depth_mask_state && orig_depth_test) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
	}
	if ((mask & GL_STENCIL_BUFFER_BIT) == GL_STENCIL_BUFFER_BIT){
		invalidate_depth_test();
		change_depth_write(SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetVertexProgram(gxm_context, disable_color_buffer_vertex_program_patched);
		sceGxmSetFragmentProgram(gxm_context, disable_color_buffer_fragment_program_patched);
		sceGxmSetFrontStencilFunc(gxm_context,
			SCE_GXM_STENCIL_FUNC_NEVER,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			0, stencil_value);
		sceGxmSetBackStencilFunc(gxm_context,
			SCE_GXM_STENCIL_FUNC_NEVER,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			SCE_GXM_STENCIL_OP_REPLACE,
			0, stencil_value);
		sceGxmSetVertexStream(gxm_context, 0, clear_vertices);
		sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);
		validate_depth_test();
		change_depth_write((depth_mask_state && orig_depth_test) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);
		change_stencil_settings();
	}
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){
	clear_rgba_val.r = red;
	clear_rgba_val.g = green;
	clear_rgba_val.b = blue;
	clear_rgba_val.a = alpha;
}

GLenum glGetError(void){
	GLenum ret = error;
	error = GL_NO_ERROR;
	return ret;
}

void glReadPixels(GLint x,  GLint y,  GLsizei width,  GLsizei height,  GLenum format,  GLenum type,  GLvoid * data){
	SceDisplayFrameBuf pParam;
	pParam.size = sizeof(SceDisplayFrameBuf);
	sceDisplayGetFrameBuf(&pParam, SCE_DISPLAY_SETBUF_NEXTFRAME);
	y = DISPLAY_HEIGHT - (height + y);
	int i,j;
	uint8_t* out8 = (uint8_t*)data;
	uint8_t* in8 = (uint8_t*)pParam.base;
	uint32_t* out32 = (uint32_t*)data;
	uint32_t* in32 = (uint32_t*)pParam.base;
	switch (format){
		case GL_RGBA:
			switch (type){
				case GL_UNSIGNED_BYTE:
					in32 += (x + y * pParam.pitch);
					for (i = 0; i < height; i++){
						for (j = 0; j < width; j++){
							out32[(height-(i+1))*width+j] = in32[j];
						}
						in32 += pParam.pitch;
					}
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		case GL_RGB:
			switch (type){
				case GL_UNSIGNED_BYTE:
					in8 += (x * 4 + y * pParam.pitch * 4);
					for (i = 0; i < height; i++){
						for (j = 0; j < width; j++){
							out8[((height-(i+1))*width+j)*3] = in8[j*4];
							out8[((height-(i+1))*width+j)*3+1] = in8[j*4+1];
							out8[((height-(i+1))*width+j)*3+2] = in8[j*4+2];
						}
						in8 += pParam.pitch * 4;
					}
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}
