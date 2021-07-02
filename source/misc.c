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
GLfloat point_size = 1.0f;
GLboolean fast_texture_compression = GL_FALSE; // Hints for texture compression

static void update_fogging_state() {
	ffp_dirty_frag = GL_TRUE;
	if (fogging) {
		switch (fog_mode) {
		case GL_LINEAR:
			internal_fog_mode = LINEAR;
			break;
		case GL_EXP:
			internal_fog_mode = EXP;
			break;
		default:
			internal_fog_mode = EXP2;
			break;
		}
	} else
		internal_fog_mode = DISABLED;
}

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
		SET_GL_ERROR(GL_INVALID_ENUM)
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	update_polygon_offset();
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
	pol_factor = factor;
	pol_units = units;
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
	if (!is_rendering_display)
		y_scale = -y_scale;
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

void glEnable(GLenum cap) {
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
	case GL_TEXTURE_2D:
		ffp_dirty_vert = GL_TRUE;
		ffp_dirty_frag = GL_TRUE;
		texture_units[server_texture_unit].enabled = GL_TRUE;
		break;
	case GL_ALPHA_TEST:
		alpha_test_state = GL_TRUE;
		update_alpha_test_settings();
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glDisable(GLenum cap) {
#ifndef SKIP_ERROR_HANDLING
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	switch (cap) {
	case GL_LIGHTING:
		ffp_dirty_vert = GL_TRUE;
		lighting_state = GL_FALSE;
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
	case GL_TEXTURE_2D:
		ffp_dirty_vert = GL_TRUE;
		ffp_dirty_frag = GL_TRUE;
		texture_units[server_texture_unit].enabled = GL_FALSE;
		break;
	case GL_ALPHA_TEST:
		alpha_test_state = GL_FALSE;
		update_alpha_test_settings();
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
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
#ifndef SKIP_ERROR_HANDLING
	if (light < GL_LIGHT0 && light > GL_LIGHT7) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#endif

	switch (pname) {
	case GL_AMBIENT:
		sceClibMemcpy(&lights_ambients[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_DIFFUSE:
		sceClibMemcpy(&lights_diffuses[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_SPECULAR:
		sceClibMemcpy(&lights_speculars[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_POSITION:
		sceClibMemcpy(&lights_positions[light - GL_LIGHT0].r, params, sizeof(float) * 4);
		break;
	case GL_CONSTANT_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].r = params[0];
		break;
	case GL_LINEAR_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].g = params[0];
		break;
	case GL_QUADRATIC_ATTENUATION:
		lights_attenuations[light - GL_LIGHT0].b = params[0];
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
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

	if ((mask & GL_COLOR_BUFFER_BIT) == 0) {
		// Disable fragment program if not clearing color buffer. Depth and stencil clears are unaffected.
		sceGxmSetFrontFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_DISABLED);
		sceGxmSetBackFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_DISABLED);
	}

	if ((mask & GL_STENCIL_BUFFER_BIT) == 0) {
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
	change_depth_write(depth_mask_state ? SCE_GXM_DEPTH_WRITE_ENABLED : SCE_GXM_DEPTH_WRITE_DISABLED);

	change_stencil_settings();

	sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front);
	sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back);

	sceGxmSetFrontFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_ENABLED);
	sceGxmSetBackFragmentProgramEnable(gxm_context, SCE_GXM_FRAGMENT_PROGRAM_ENABLED);

	update_polygon_offset();

	// Restoring viewport and culling
	validate_viewport();
	change_cull_mode();
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	clear_rgba_val.r = red;
	clear_rgba_val.g = green;
	clear_rgba_val.b = blue;
	clear_rgba_val.a = alpha;
}

void glLineWidth(GLfloat width) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (width <= 0.0f) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	uint32_t int_width = width;
	if (int_width > 16)
		int_width = 16;
	else if (int_width < 1)
		int_width = 1;

	// Changing line width as requested
	sceGxmSetFrontPointLineWidth(gxm_context, int_width);
	sceGxmSetBackPointLineWidth(gxm_context, int_width);
}

void glPointSize(GLfloat size) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (size <= 0) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Changing point size as requested
	point_size = size;
}

void glFogf(GLenum pname, GLfloat param) {
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = param;
		break;
	case GL_FOG_START:
		fog_near = param;
		break;
	case GL_FOG_END:
		fog_far = param;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glFogfv(GLenum pname, const GLfloat *params) {
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = params[0];
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = params[0];
		break;
	case GL_FOG_START:
		fog_near = params[0];
		break;
	case GL_FOG_END:
		fog_far = params[0];
		break;
	case GL_FOG_COLOR:
		sceClibMemcpy(&fog_color.r, params, sizeof(vector4f));
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glFogi(GLenum pname, const GLint param) {
	switch (pname) {
	case GL_FOG_MODE:
		fog_mode = param;
		update_fogging_state();
		break;
	case GL_FOG_DENSITY:
		fog_density = param;
		break;
	case GL_FOG_START:
		fog_near = param;
		break;
	case GL_FOG_END:
		fog_far = param;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glClipPlane(GLenum plane, const GLdouble *equation) {
#ifndef SKIP_ERROR_HANDLING
	if (plane < GL_CLIP_PLANE0 || plane > GL_CLIP_PLANE6) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
#endif
	int idx = plane - GL_CLIP_PLANE0;
	clip_planes_eq[idx].x = equation[0];
	clip_planes_eq[idx].y = equation[1];
	clip_planes_eq[idx].z = equation[2];
	clip_planes_eq[idx].w = equation[3];
	matrix4x4 inverted, inverted_transposed;
	matrix4x4_invert(inverted, modelview_matrix);
	matrix4x4_transpose(inverted_transposed, inverted);
	vector4f temp;
	vector4f_matrix4x4_mult(&temp, inverted_transposed, &clip_planes_eq[idx]);
	sceClibMemcpy(&clip_planes_eq[idx].x, &temp.x, sizeof(vector4f));
}

void glHint(GLenum target, GLenum mode) {
	switch (target) {
	case GL_TEXTURE_COMPRESSION_HINT:
		switch (mode) {
		case GL_FASTEST:
			fast_texture_compression = GL_TRUE;
			break;
		default:
			fast_texture_compression = GL_FALSE;
			break;
		}
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}
