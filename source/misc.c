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
 * misc.c:
 * Implementation for miscellaneous functions
 */

#include "shared.h"

#define ATTRIBS_STACK_DEPTH 16 // Depth of attributes stack

enum {
	COLOR_BUFFER_BIT,
	DEPTH_BUFFER_BIT,
	ENABLE_BIT,
	FOG_BIT,
	HINT_BIT,
	LINE_BIT,
	POINT_BIT,
	POLYGON_BIT,
	SCISSOR_BIT,
	STENCIL_BUFFER_BIT,
	TRANSFORM_BIT,
	VIEWPORT_BIT
};

typedef struct {
	uint16_t enabled_bits;
	// GL_COLOR_BUFFER_BIT
	GLenum alpha_func;
	GLfloat alpha_ref;
	uint8_t blend_color_mask;
	uint8_t blend_func_rgb;
	uint8_t blend_func_a;
	uint8_t blend_sfactor_rgb;
	uint8_t blend_dfactor_rgb;
	uint8_t blend_sfactor_a;
	uint8_t blend_dfactor_a;
	// GL_DEPTH_BUFFER_BIT
	uint32_t depth_func;
	GLdouble depth_value;
	GLboolean depth_mask_state;
	// GL_ENABLE_BIT
	GLboolean alpha_test_state;
	GLboolean blend_state;
	GLboolean depth_test_state;
	GLboolean lighting_state;
	GLboolean stencil_test_state;
	GLboolean scissor_test_state;
	GLboolean cull_face_state;
	GLboolean pol_offset_fill;
	GLboolean pol_offset_line;
	GLboolean pol_offset_point;
	GLboolean fogging;
	uint8_t clip_planes_mask;
	uint8_t light_mask;
	// GL_FOG_BIT
	GLfloat fog_density;
	vector4f fog_color;
	GLfloat fog_far;
	GLfloat fog_near;
	GLint fog_mode;
	// GL_HINT_BIT
	GLboolean fast_texture_compression;
	GLboolean recompress_non_native;
	// GL_LINE_BIT
	GLfloat line_width;
	// GL_POINT_BIT
	GLfloat point_size;
	// GL_POLYGON_BIT
	GLenum gl_cull_mode;
	GLenum gl_front_face;
	GLfloat pol_factor;
	GLfloat pol_units;
	// GL_SCISSOR_BIT
	scissor_region region;
	// GL_STENCIL_BUFFER_BIT
	uint8_t stencil_mask_back_write;
	uint8_t stencil_mask_front_write;
	uint8_t stencil_mask_back;
	uint8_t stencil_mask_front;
	uint8_t stencil_ref_front;
	uint8_t stencil_ref_back;
	SceGxmStencilOp stencil_fail_front;
	SceGxmStencilOp depth_fail_front;
	SceGxmStencilOp depth_pass_front;
	SceGxmStencilOp stencil_fail_back;
	SceGxmStencilOp depth_fail_back;
	SceGxmStencilOp depth_pass_back;
	SceGxmStencilFunc stencil_func_front;
	SceGxmStencilFunc stencil_func_back;
	GLint stencil_value;
	// GL_TRANSFORM_BIT
	vector4f clip_planes_eq[MAX_CLIP_PLANES_NUM];
	matrix4x4 *matrix;
	// GL_VIEWPORT_BIT
	viewport gl_viewport;
	float z_port;
	float z_scale;
} attrib_state;

attrib_state attrib_stack[ATTRIBS_STACK_DEPTH];
uint8_t attrib_stack_counter = 0;

GLfloat line_width = 1.0f;
GLfloat point_size = 1.0f;
GLboolean fast_texture_compression = GL_FALSE; // Hints for texture compression
GLboolean recompress_non_native = GL_FALSE;
vector4f clear_rgba_val; // Current clear color for glClear

// Polygon Mode
GLfloat pol_factor = 0.0f; // Current factor for glPolygonOffset
GLfloat pol_units = 0.0f; // Current units for glPolygonOffset

// Cullling
GLboolean cull_face_state = GL_FALSE; // Current state for GL_CULL_FACE
GLenum gl_cull_mode = GL_BACK; // Current in use openGL cull mode
GLenum gl_front_face = GL_CCW; // Current in use openGL setting for front facing primitives
GLboolean no_polygons_mode = GL_FALSE; // GL_TRUE when cull mode is set to GL_FRONT_AND_BACK

// Polygon Offset
GLboolean pol_offset_fill = GL_FALSE; // Current state for GL_POLYGON_OFFSET_FILL
GLboolean pol_offset_line = GL_FALSE; // Current state for GL_POLYGON_OFFSET_LINE
GLboolean pol_offset_point = GL_FALSE; // Current state for GL_POLYGON_OFFSET_POINT
SceGxmPolygonMode polygon_mode_front = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL; // Current in use polygon mode for front
SceGxmPolygonMode polygon_mode_back = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL; // Current in use polygon mode for back
GLenum gl_polygon_mode_front = GL_FILL; // Current in use polygon mode for front
GLenum gl_polygon_mode_back = GL_FILL; // Current in use polygon mode for back

viewport gl_viewport; // Current viewport state

static void update_polygon_offset() {
	switch (polygon_mode_front) {
	case SCE_GXM_POLYGON_MODE_TRIANGLE_LINE:
		if (pol_offset_line)
			sceGxmSetFrontDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
		else
			sceGxmSetFrontDepthBias(gxm_context, 0, 0);
		break;
	case SCE_GXM_POLYGON_MODE_TRIANGLE_POINT:
		if (pol_offset_point)
			sceGxmSetFrontDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
		else
			sceGxmSetFrontDepthBias(gxm_context, 0, 0);
		break;
	case SCE_GXM_POLYGON_MODE_TRIANGLE_FILL:
		if (pol_offset_fill)
			sceGxmSetFrontDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
		else
			sceGxmSetFrontDepthBias(gxm_context, 0, 0);
		break;
	}
	switch (polygon_mode_back) {
	case SCE_GXM_POLYGON_MODE_TRIANGLE_LINE:
		if (pol_offset_line)
			sceGxmSetBackDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
		else
			sceGxmSetBackDepthBias(gxm_context, 0, 0);
		break;
	case SCE_GXM_POLYGON_MODE_TRIANGLE_POINT:
		if (pol_offset_point)
			sceGxmSetBackDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
		else
			sceGxmSetBackDepthBias(gxm_context, 0, 0);
		break;
	case SCE_GXM_POLYGON_MODE_TRIANGLE_FILL:
		if (pol_offset_fill)
			sceGxmSetBackDepthBias(gxm_context, (int)pol_factor, (int)pol_units);
		else
			sceGxmSetBackDepthBias(gxm_context, 0, 0);
		break;
	}
}

void change_cull_mode() {
	// Setting proper cull mode in sceGxm depending to current openGL machine state
	if (cull_face_state) {
#ifdef HAVE_UNFLIPPED_FBOS
		if ((gl_front_face == GL_CW) && (gl_cull_mode == GL_BACK))
			sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CCW);
		else if ((gl_front_face == GL_CCW) && (gl_cull_mode == GL_BACK))
			sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CW);
		else if ((gl_front_face == GL_CCW) && (gl_cull_mode == GL_FRONT))
			sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CCW);
		else if ((gl_front_face == GL_CW) && (gl_cull_mode == GL_FRONT))
			sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_CW);
#else
		if ((gl_front_face == GL_CW) && (gl_cull_mode == GL_BACK))
			sceGxmSetCullMode(gxm_context, is_rendering_display ? SCE_GXM_CULL_CCW : SCE_GXM_CULL_CW);
		else if ((gl_front_face == GL_CCW) && (gl_cull_mode == GL_BACK))
			sceGxmSetCullMode(gxm_context, is_rendering_display ? SCE_GXM_CULL_CW : SCE_GXM_CULL_CCW);
		else if ((gl_front_face == GL_CCW) && (gl_cull_mode == GL_FRONT))
			sceGxmSetCullMode(gxm_context, is_rendering_display ? SCE_GXM_CULL_CCW : SCE_GXM_CULL_CW);
		else if ((gl_front_face == GL_CW) && (gl_cull_mode == GL_FRONT))
			sceGxmSetCullMode(gxm_context, is_rendering_display ? SCE_GXM_CULL_CW : SCE_GXM_CULL_CCW);
#endif
		else if (gl_cull_mode == GL_FRONT_AND_BACK)
			no_polygons_mode = GL_TRUE;
	} else
		sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_NONE);
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glPolygonMode(GLenum face, GLenum mode) {
	SceGxmPolygonMode new_mode;
	switch (mode) {
	case GL_POINT:
		new_mode = SCE_GXM_POLYGON_MODE_TRIANGLE_POINT;
		break;
	case GL_LINE:
		new_mode = SCE_GXM_POLYGON_MODE_TRIANGLE_LINE;
		break;
	case GL_FILL:
		new_mode = SCE_GXM_POLYGON_MODE_TRIANGLE_FILL;
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, mode)
	}
	switch (face) {
	case GL_FRONT:
		polygon_mode_front = new_mode;
		gl_polygon_mode_front = mode;
		sceGxmSetFrontPolygonMode(gxm_context, new_mode);
		break;
	case GL_BACK:
		polygon_mode_back = new_mode;
		gl_polygon_mode_back = mode;
		sceGxmSetBackPolygonMode(gxm_context, new_mode);
		break;
	case GL_FRONT_AND_BACK:
		polygon_mode_front = polygon_mode_back = new_mode;
		gl_polygon_mode_front = gl_polygon_mode_back = mode;
		sceGxmSetFrontPolygonMode(gxm_context, new_mode);
		sceGxmSetBackPolygonMode(gxm_context, new_mode);
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, face)
	}
	update_polygon_offset();
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
	pol_factor = factor;
	pol_units = units;
	update_polygon_offset();
}

void glPolygonOffsetx(GLfixed factor, GLfixed units) {
	pol_factor = (float)factor / 65536.0f;
	pol_units = (float)units / 65536.0f;
	update_polygon_offset();
}

void glCullFace(GLenum mode) {
	gl_cull_mode = mode;
	if (cull_face_state)
		change_cull_mode();
}

void glFrontFace(GLenum mode) {
	gl_front_face = mode;
	if (cull_face_state)
		change_cull_mode();
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
#ifndef SKIP_ERROR_HANDLING
	if ((width < 0) || (height < 0)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	x_scale = width >> 1;
	x_port = x + x_scale;
	y_scale = -(height >> 1);
	y_port = (is_rendering_display ? DISPLAY_HEIGHT : in_use_framebuffer->height) - y + y_scale;
#ifndef HAVE_UNFLIPPED_FBOS
	if (!is_rendering_display) {
		y_port = in_use_framebuffer->height - y_port;
		y_scale = -y_scale;
	}
#endif

	setViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
	gl_viewport.x = x;
	gl_viewport.y = y;
	gl_viewport.w = width;
	gl_viewport.h = height;
}

void glDepthRange(GLdouble nearVal, GLdouble farVal) {
	z_port = (farVal + nearVal) / 2.0f;
	z_scale = (farVal - nearVal) / 2.0f;
	setViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
}

void glDepthRangef(GLfloat nearVal, GLfloat farVal) {
	z_port = (farVal + nearVal) / 2.0f;
	z_scale = (farVal - nearVal) / 2.0f;
	setViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
}

void glDepthRangex(GLfixed _nearVal, GLfixed _farVal) {
	GLfloat nearVal = (float)_nearVal / 65536.0f;
	GLfloat farVal = (float)_farVal / 65536.0f;
	z_port = (farVal + nearVal) / 2.0f;
	z_scale = (farVal - nearVal) / 2.0f;
	setViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
}

void glEnable(GLenum cap) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glEnable, "U", cap))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	switch (cap) {
	case GL_LIGHTING:
		ffp_dirty_vert = GL_TRUE;
		lighting_state = GL_TRUE;
		break;
	case GL_DEPTH_TEST:
		depth_test_state = GL_TRUE;
		change_depth_func();
		break;
	case GL_STENCIL_TEST:
		stencil_test_state = GL_TRUE;
		change_stencil_settings();
		break;
	case GL_BLEND:
		if (!blend_state)
			change_blend_factor();
		blend_state = GL_TRUE;
		break;
	case GL_COLOR_MATERIAL:
		color_material_state = GL_TRUE;
		break;
	case GL_SCISSOR_TEST:
		scissor_test_state = GL_TRUE;
		sceneReset();
		update_scissor_test();
		break;
	case GL_CULL_FACE:
		cull_face_state = GL_TRUE;
		change_cull_mode();
		break;
	case GL_POLYGON_OFFSET_FILL:
		pol_offset_fill = GL_TRUE;
		update_polygon_offset();
		break;
	case GL_POLYGON_OFFSET_LINE:
		pol_offset_line = GL_TRUE;
		update_polygon_offset();
		break;
	case GL_POLYGON_OFFSET_POINT:
		pol_offset_point = GL_TRUE;
		update_polygon_offset();
		break;
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
		ffp_dirty_vert = GL_TRUE;
		ffp_dirty_frag = GL_TRUE;
		texture_units[server_texture_unit].enabled = GL_TRUE;
		break;
	case GL_ALPHA_TEST:
		alpha_test_state = GL_TRUE;
		update_alpha_test_settings();
		break;
	case GL_NORMALIZE:
		ffp_dirty_vert = GL_TRUE;
		normalize = GL_TRUE;
		break;
	case GL_FOG:
		fogging = GL_TRUE;
		update_fogging_state();
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
	case GL_CLIP_PLANE6:
		ffp_dirty_vert = GL_TRUE;
		clip_planes_mask |= (1 << cap - GL_CLIP_PLANE0);

		clip_plane_range[0] = clip_planes_mask ? __builtin_ctz(clip_planes_mask) : 0; // Get the lowest enabled clip plane
		clip_plane_range[1] = clip_planes_mask ? 8 - (__builtin_clz(clip_planes_mask) - 24) : 0; // Get the highest enabled clip plane
		clip_planes_aligned = GL_TRUE;
		for (int i = clip_plane_range[0]; i < clip_plane_range[1]; i++) {
			if (!(clip_planes_mask & (1 << i)) && clip_planes_aligned) {
				clip_planes_aligned = GL_FALSE;
				break;
			}
		}
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		ffp_dirty_vert = GL_TRUE;
		light_mask |= (1 << cap - GL_LIGHT0);

		light_range[0] = light_mask ? __builtin_ctz(light_mask) : 0; // Get the lowest enabled light
		light_range[1] = light_mask ? 8 - (__builtin_clz(light_mask) - 24) : 0; // Get the highest enabled light
		lights_aligned = GL_TRUE;
		for (int i = light_range[0]; i < light_range[1]; i++) {
			if (!(light_mask & (1 << i)) && lights_aligned) {
				lights_aligned = GL_FALSE;
				break;
			}
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, cap)
	}
}

void glDisable(GLenum cap) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glDisable, "U", cap))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	switch (cap) {
	case GL_LIGHTING:
		ffp_dirty_vert = GL_TRUE;
		ffp_dirty_frag = GL_TRUE;
		lighting_state = GL_FALSE;
		break;
	case GL_COLOR_MATERIAL:
		color_material_state = GL_FALSE;
		break;
	case GL_DEPTH_TEST:
		depth_test_state = GL_FALSE;
		change_depth_func();
		break;
	case GL_STENCIL_TEST:
		stencil_test_state = GL_FALSE;
		change_stencil_settings();
		break;
	case GL_BLEND:
		if (blend_state)
			change_blend_mask();
		blend_state = GL_FALSE;
		break;
	case GL_SCISSOR_TEST:
		scissor_test_state = GL_FALSE;
		sceneReset();
		update_scissor_test();
		break;
	case GL_CULL_FACE:
		cull_face_state = GL_FALSE;
		change_cull_mode();
		break;
	case GL_POLYGON_OFFSET_FILL:
		pol_offset_fill = GL_FALSE;
		update_polygon_offset();
		break;
	case GL_POLYGON_OFFSET_LINE:
		pol_offset_line = GL_FALSE;
		update_polygon_offset();
		break;
	case GL_POLYGON_OFFSET_POINT:
		pol_offset_point = GL_FALSE;
		update_polygon_offset();
		break;
	case GL_TEXTURE_1D:
	case GL_TEXTURE_2D:
		ffp_dirty_vert = GL_TRUE;
		ffp_dirty_frag = GL_TRUE;
		texture_units[server_texture_unit].enabled = GL_FALSE;
		break;
	case GL_ALPHA_TEST:
		alpha_test_state = GL_FALSE;
		update_alpha_test_settings();
		break;
	case GL_NORMALIZE:
		ffp_dirty_vert = GL_TRUE;
		normalize = GL_FALSE;
		break;
	case GL_FOG:
		fogging = GL_FALSE;
		update_fogging_state();
		break;
	case GL_CLIP_PLANE0:
	case GL_CLIP_PLANE1:
	case GL_CLIP_PLANE2:
	case GL_CLIP_PLANE3:
	case GL_CLIP_PLANE4:
	case GL_CLIP_PLANE5:
	case GL_CLIP_PLANE6:
		ffp_dirty_vert = GL_TRUE;
		clip_planes_mask &= ~(1 << (cap - GL_CLIP_PLANE0));

		clip_plane_range[0] = clip_planes_mask ? __builtin_ctz(clip_planes_mask) : 0; // Get the lowest enabled clip plane
		clip_plane_range[1] = clip_planes_mask ? 8 - (__builtin_clz(clip_planes_mask) - 24) : 0; // Get the highest enabled clip plane
		clip_planes_aligned = GL_TRUE;
		for (int i = clip_plane_range[0]; i < clip_plane_range[1]; i++) {
			if (!(clip_planes_mask & (1 << i)) && clip_planes_aligned) {
				clip_planes_aligned = GL_FALSE;
				break;
			}
		}
		break;
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
		ffp_dirty_vert = GL_TRUE;
		light_mask &= ~(1 << (cap - GL_LIGHT0));

		light_range[0] = light_mask ? __builtin_ctz(light_mask) : 0; // Get the lowest enabled clip plane
		light_range[1] = light_mask ? 8 - (__builtin_clz(light_mask) - 24) : 0; // Get the highest enabled clip plane
		lights_aligned = GL_TRUE;
		for (int i = light_range[0]; i < light_range[1]; i++) {
			if (!(light_mask & (1 << i)) && lights_aligned) {
				lights_aligned = GL_FALSE;
				break;
			}
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, cap)
	}
}

void glClear(GLbitfield mask) {
#ifndef SKIP_ERROR_HANDLING
	if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
		SET_GL_ERROR(GL_INVALID_VALUE);
	}
#endif

	sceneReset();

	// Invalidating viewport and culling
	invalidate_viewport();
	sceGxmSetCullMode(gxm_context, SCE_GXM_CULL_NONE);

	void *fbuffer, *vbuffer;

	GLenum orig_depth_test = depth_test_state;

	const GLfloat clear_depth_value = depth_value * 2 - 1;

	invalidate_depth_test();
	// Enable disable depth write if both depth mask is true and the depth buffer bit is active.
	change_depth_write(depth_mask_state && (mask & GL_DEPTH_BUFFER_BIT) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);

	sceGxmSetFrontDepthBias(gxm_context, 0, 0);
	sceGxmSetBackDepthBias(gxm_context, 0, 0);

	sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
	sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);

	sceGxmSetVertexProgram(gxm_context, clear_vertex_program_patched);
	if (is_fbo_float)
		sceGxmSetFragmentProgram(gxm_context, clear_fragment_program_float_patched);
	else
		sceGxmSetFragmentProgram(gxm_context, clear_fragment_program_patched);

	sceGxmReserveVertexDefaultUniformBuffer(gxm_context, &vbuffer);
	sceGxmSetUniformDataF(vbuffer, clear_position, 0, 4, &clear_vertices->x);
	sceGxmSetUniformDataF(vbuffer, clear_depth, 0, 1, &clear_depth_value);

	sceGxmReserveFragmentDefaultUniformBuffer(gxm_context, &fbuffer);
	sceGxmSetUniformDataF(fbuffer, clear_color, 0, 4, &clear_rgba_val.r);

	sceGxmSetFrontStencilFunc(gxm_context,
		SCE_GXM_STENCIL_FUNC_ALWAYS,
		SCE_GXM_STENCIL_OP_REPLACE,
		SCE_GXM_STENCIL_OP_REPLACE,
		SCE_GXM_STENCIL_OP_REPLACE,
		0XFF, stencil_mask_front_write & 0xFF);
	sceGxmSetFrontStencilRef(gxm_context, stencil_value & 0xFF);

	sceGxmSetBackStencilFunc(gxm_context,
		SCE_GXM_STENCIL_FUNC_ALWAYS,
		SCE_GXM_STENCIL_OP_REPLACE,
		SCE_GXM_STENCIL_OP_REPLACE,
		SCE_GXM_STENCIL_OP_REPLACE,
		0xFF, stencil_mask_back_write & 0xFF);
	sceGxmSetBackStencilRef(gxm_context, stencil_value & 0xFF);

	if (!(mask & GL_COLOR_BUFFER_BIT)) {
		// Disable fragment program if not clearing color buffer. Depth and stencil clears are unaffected.
		sceGxmSetFrontFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_DISABLED);
		sceGxmSetBackFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_DISABLED);
	}

	if (!(mask & GL_STENCIL_BUFFER_BIT)) {
		// Set stencil functions to KEEP if not clearing stencil buffer.
		sceGxmSetFrontStencilFunc(gxm_context,
			SCE_GXM_STENCIL_FUNC_ALWAYS,
			SCE_GXM_STENCIL_OP_KEEP,
			SCE_GXM_STENCIL_OP_KEEP,
			SCE_GXM_STENCIL_OP_KEEP,
			0XFF, 0xFF);
		sceGxmSetBackStencilFunc(gxm_context,
			SCE_GXM_STENCIL_FUNC_ALWAYS,
			SCE_GXM_STENCIL_OP_KEEP,
			SCE_GXM_STENCIL_OP_KEEP,
			SCE_GXM_STENCIL_OP_KEEP,
			0xFF, 0xFF);
	}

	sceGxmDraw(gxm_context, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, SCE_GXM_INDEX_FORMAT_U16, depth_clear_indices, 4);

	validate_depth_test();
	change_depth_write((depth_mask_state && depth_test_state) ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);

	change_stencil_settings();

	sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front);
	sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back);

	sceGxmSetFrontFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_ENABLED);
	sceGxmSetBackFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_ENABLED);

	update_polygon_offset();

	// Restoring viewport and culling
	validate_viewport();
	change_cull_mode();

	vglRestoreFragmentUniformBuffer();
	vglRestoreVertexUniformBuffer();
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	clear_rgba_val.r = red;
	clear_rgba_val.g = green;
	clear_rgba_val.b = blue;
	clear_rgba_val.a = alpha;
}

void glClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) {
	clear_rgba_val.r = (float)red / 65536.0f;
	clear_rgba_val.g = (float)green / 65536.0f;
	clear_rgba_val.b = (float)blue / 65536.0f;
	clear_rgba_val.a = (float)alpha / 65536.0f;
}

void glLineWidth(GLfloat width) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (width <= 0.0f) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	line_width = width;
	uint32_t int_width = width;
	if (int_width > 16)
		int_width = 16;
	else if (int_width < 1)
		int_width = 1;

	// Changing line width as requested
	sceGxmSetFrontPointLineWidth(gxm_context, int_width);
	sceGxmSetBackPointLineWidth(gxm_context, int_width);
}

void glLineWidthx(GLfixed width) {
	glLineWidth((float)width / 65536.0f);
}

void glPointSize(GLfloat size) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (size <= 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif
	dirty_vert_unifs = GL_TRUE;

	// Changing point size as requested
	point_size = size;
}

void glPointSizex(GLfixed size) {
	glPointSize((float)size / 65536.0f);
}

void glHint(GLenum target, GLenum mode) {
	switch (target) {
	case GL_TEXTURE_COMPRESSION_HINT:
		switch (mode) {
		case GL_FASTEST:
			fast_texture_compression = GL_TRUE;
			recompress_non_native = GL_FALSE;
			break;
		case GL_DONT_CARE:
			fast_texture_compression = GL_FALSE;
			recompress_non_native = GL_FALSE;
			break;
		default:
			recompress_non_native = GL_TRUE;
			fast_texture_compression = GL_FALSE;
			break;
		}
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, target)
	}
}

void glPushAttrib(GLbitfield mask) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (attrib_stack_counter >= ATTRIBS_STACK_DEPTH) {
		SET_GL_ERROR(GL_STACK_OVERFLOW)
	}
#endif
	attrib_state *setup = &attrib_stack[attrib_stack_counter++];
	setup->enabled_bits = 0;

	if (mask & GL_COLOR_BUFFER_BIT) {
		setup->enabled_bits += (1 << COLOR_BUFFER_BIT);
		setup->alpha_test_state = alpha_test_state;
		setup->alpha_func = alpha_func;
		setup->alpha_ref = alpha_ref;
		setup->blend_state = blend_state;
		setup->blend_color_mask = blend_color_mask;
		setup->blend_func_rgb = blend_func_rgb;
		setup->blend_func_a = blend_func_a;
		setup->blend_sfactor_rgb = blend_sfactor_rgb;
		setup->blend_sfactor_a = blend_sfactor_a;
		setup->blend_dfactor_rgb = blend_dfactor_rgb;
		setup->blend_dfactor_a = blend_dfactor_a;
	}
	if (mask & GL_DEPTH_BUFFER_BIT) {
		setup->enabled_bits += (1 << DEPTH_BUFFER_BIT);
		setup->depth_test_state = depth_test_state;
		setup->depth_func = depth_func;
		setup->depth_value = depth_value;
		setup->depth_mask_state = depth_mask_state;
	}
	if (mask & GL_ENABLE_BIT) {
		setup->enabled_bits += (1 << ENABLE_BIT);
		setup->alpha_test_state = alpha_test_state;
		setup->blend_state = blend_state;
		setup->depth_test_state = depth_test_state;
		setup->lighting_state = lighting_state;
		setup->stencil_test_state = stencil_test_state;
		setup->scissor_test_state = scissor_test_state;
		setup->cull_face_state = cull_face_state;
		setup->pol_offset_fill = pol_offset_fill;
		setup->pol_offset_line = pol_offset_line;
		setup->pol_offset_point = pol_offset_point;
		setup->fogging = fogging;
		setup->clip_planes_mask = clip_planes_mask;
		setup->light_mask = light_mask;
	}
	if (mask & GL_FOG_BIT) {
		setup->enabled_bits += (1 << FOG_BIT);
		setup->fogging = fogging;
		setup->fog_density = fog_density;
		setup->fog_color = fog_color;
		setup->fog_far = fog_far;
		setup->fog_near = fog_near;
		setup->fog_mode = fog_mode;
	}
	if (mask & GL_HINT_BIT) {
		setup->enabled_bits += (1 << HINT_BIT);
		setup->fast_texture_compression = fast_texture_compression;
		setup->recompress_non_native = recompress_non_native;
	}
	if (mask & GL_LINE_BIT) {
		setup->enabled_bits += (1 << LINE_BIT);
		setup->line_width = line_width;
	}
	if (mask & GL_POINT_BIT) {
		setup->enabled_bits += (1 << POINT_BIT);
		setup->point_size = point_size;
	}
	if (mask & GL_POLYGON_BIT) {
		setup->enabled_bits += (1 << POLYGON_BIT);
		setup->cull_face_state = cull_face_state;
		setup->gl_cull_mode = gl_cull_mode;
		setup->gl_front_face = gl_front_face;
		setup->pol_offset_fill = pol_offset_fill;
		setup->pol_offset_line = pol_offset_line;
		setup->pol_offset_point = pol_offset_point;
		setup->pol_factor = pol_factor;
		setup->pol_units = pol_units;
	}
	if (mask & GL_SCISSOR_BIT) {
		setup->enabled_bits += (1 << SCISSOR_BIT);
		setup->scissor_test_state = scissor_test_state;
		setup->region = region;
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
		setup->enabled_bits += (1 << STENCIL_BUFFER_BIT);
		setup->stencil_test_state = stencil_test_state;
		setup->stencil_mask_back_write = stencil_mask_back_write;
		setup->stencil_mask_front_write = stencil_mask_front_write;
		setup->stencil_mask_back = stencil_mask_back;
		setup->stencil_mask_front = stencil_mask_front;
		setup->stencil_ref_front = stencil_ref_front;
		setup->stencil_ref_back = stencil_ref_back;
		setup->stencil_fail_front = stencil_fail_front;
		setup->depth_fail_front = depth_fail_front;
		setup->depth_pass_front = depth_pass_front;
		setup->stencil_fail_back = stencil_fail_back;
		setup->depth_fail_back = depth_fail_back;
		setup->depth_pass_back = depth_pass_back;
		setup->stencil_func_front = stencil_func_front;
		setup->stencil_func_back = stencil_func_back;
		setup->stencil_value = stencil_value;
	}
	if (mask & GL_TRANSFORM_BIT) {
		setup->enabled_bits += (1 << TRANSFORM_BIT);
		setup->clip_planes_mask = clip_planes_mask;
		setup->matrix = matrix;
		for (int i = 0; i < MAX_CLIP_PLANES_NUM; i++) {
			setup->clip_planes_eq[i] = clip_planes_eq[i];
		}
	}
	if (mask & GL_VIEWPORT_BIT) {
		setup->enabled_bits += (1 << VIEWPORT_BIT);
		setup->gl_viewport = gl_viewport;
		setup->z_port = z_port;
		setup->z_scale = z_scale;
	}
}

void glPopAttrib(void) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if (attrib_stack_counter == 0) {
		SET_GL_ERROR(GL_STACK_UNDERFLOW)
	}
#endif
	attrib_state *setup = &attrib_stack[attrib_stack_counter--];

	if (setup->enabled_bits & (1 << COLOR_BUFFER_BIT)) {
		alpha_test_state = setup->alpha_test_state;
		alpha_func = setup->alpha_func;
		alpha_ref = setup->alpha_ref;
		update_alpha_test_settings();
		blend_state = setup->blend_state;
		blend_color_mask = setup->blend_color_mask;
		blend_func_rgb = setup->blend_func_rgb;
		blend_func_a = setup->blend_func_a;
		blend_sfactor_rgb = setup->blend_sfactor_rgb;
		blend_sfactor_a = setup->blend_sfactor_a;
		blend_dfactor_rgb = setup->blend_dfactor_rgb;
		blend_dfactor_a = setup->blend_dfactor_a;
		if (blend_state)
			change_blend_factor();
		else
			change_blend_mask();
	}
	if (setup->enabled_bits & (1 << DEPTH_BUFFER_BIT)) {
		depth_test_state = setup->depth_test_state;
		depth_func = setup->depth_func;
		depth_value = setup->depth_value;
		depth_mask_state = setup->depth_mask_state;
		change_depth_func();
	}
	if (setup->enabled_bits & (1 << ENABLE_BIT)) {
		alpha_test_state = setup->alpha_test_state;
		update_alpha_test_settings();
		blend_state = setup->blend_state;
		if (blend_state)
			change_blend_factor();
		else
			change_blend_mask();
		depth_test_state = setup->depth_test_state;
		change_depth_func();
		lighting_state = setup->lighting_state;
		stencil_test_state = setup->stencil_test_state;
		change_stencil_settings();
		scissor_test_state = setup->scissor_test_state;
		sceneReset();
		update_scissor_test();
		cull_face_state = setup->cull_face_state;
		change_cull_mode();
		pol_offset_fill = setup->pol_offset_fill;
		pol_offset_line = setup->pol_offset_line;
		pol_offset_point = setup->pol_offset_point;
		update_polygon_offset();
		fogging = setup->fogging;
		update_fogging_state();
		clip_planes_mask = setup->clip_planes_mask;
		light_mask = setup->light_mask;
		ffp_dirty_vert = GL_TRUE;
		ffp_dirty_frag = GL_TRUE;
	}
	if (setup->enabled_bits & (1 << FOG_BIT)) {
		fogging = setup->fogging;
		fog_density = setup->fog_density;
		fog_color = setup->fog_color;
		fog_far = setup->fog_far;
		fog_near = setup->fog_near;
		fog_mode = setup->fog_mode;
		fog_range = fog_far - fog_near;
		update_fogging_state();
	}
	if (setup->enabled_bits & (1 << HINT_BIT)) {
		fast_texture_compression = setup->fast_texture_compression;
		recompress_non_native = setup->recompress_non_native;
	}
	if (setup->enabled_bits & (1 << LINE_BIT)) {
		line_width = setup->line_width;
	}
	if (setup->enabled_bits & (1 << POINT_BIT)) {
		point_size = setup->point_size;
	}
	if (setup->enabled_bits & (1 << POLYGON_BIT)) {
		cull_face_state = setup->cull_face_state;
		gl_cull_mode = setup->gl_cull_mode;
		gl_front_face = setup->gl_front_face;
		pol_offset_fill = setup->pol_offset_fill;
		pol_offset_line = setup->pol_offset_line;
		pol_offset_point = setup->pol_offset_point;
		pol_factor = setup->pol_factor;
		pol_units = setup->pol_units;
		change_cull_mode();
		update_polygon_offset();
	}
	if (setup->enabled_bits & (1 << SCISSOR_BIT)) {
		scissor_test_state = setup->scissor_test_state;
		region = setup->region;
		sceneReset();
		update_scissor_test();
	}
	if (setup->enabled_bits & (1 << STENCIL_BUFFER_BIT)) {
		stencil_test_state = setup->stencil_test_state;
		stencil_mask_back_write = setup->stencil_mask_back_write;
		stencil_mask_front_write = setup->stencil_mask_front_write;
		stencil_mask_back = setup->stencil_mask_back;
		stencil_mask_front = setup->stencil_mask_front;
		stencil_ref_front = setup->stencil_ref_front;
		stencil_ref_back = setup->stencil_ref_back;
		stencil_fail_front = setup->stencil_fail_front;
		depth_fail_front = setup->depth_fail_front;
		depth_pass_front = setup->depth_pass_front;
		stencil_fail_back = setup->stencil_fail_back;
		depth_fail_back = setup->depth_fail_back;
		depth_pass_back = setup->depth_pass_back;
		stencil_func_front = setup->stencil_func_front;
		stencil_func_back = setup->stencil_func_back;
		stencil_value = setup->stencil_value;
		change_stencil_settings();
	}
	if (setup->enabled_bits & (1 << TRANSFORM_BIT)) {
		clip_planes_mask = setup->clip_planes_mask;
		matrix = setup->matrix;
		for (int i = 0; i < MAX_CLIP_PLANES_NUM; i++) {
			clip_planes_eq[i] = setup->clip_planes_eq[i];
		}
		ffp_dirty_vert = GL_TRUE;
	}
	if (setup->enabled_bits & (1 << VIEWPORT_BIT)) {
		gl_viewport = setup->gl_viewport;
		z_port = setup->z_port;
		z_scale = setup->z_scale;
		glViewport(gl_viewport.x, gl_viewport.y, gl_viewport.w, gl_viewport.h);
	}
}
