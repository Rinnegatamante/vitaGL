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
 * blending.c:
 * Implementation for blending related functions
 */
 
#include "shared.h"

GLboolean blend_state = GL_FALSE; // Current state for GL_BLEND
SceGxmBlendFactor blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE; // Current in use RGB source blend factor
SceGxmBlendFactor blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO; // Current in use RGB dest blend factor
SceGxmBlendFactor blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE; // Current in use A source blend factor
SceGxmBlendFactor blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO; // Current in use A dest blend factor

blend_config blend_info; // Current blend info mode

static SceGxmColorMask blend_color_mask = SCE_GXM_COLOR_MASK_ALL; // Current in-use color mask (glColorMask)
static SceGxmBlendFunc blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD; // Current in-use RGB blend func
static SceGxmBlendFunc blend_func_a = SCE_GXM_BLEND_FUNC_ADD; // Current in-use A blend func

void rebuild_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog, SceGxmProgram *vprog) {
	patchFragmentProgram(gxm_shader_patcher,
		pid, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		msaa_mode, &blend_info.info, vprog, prog);
}

void change_blend_factor() {
	blend_info.info.colorMask = blend_color_mask;
	blend_info.info.colorFunc = blend_func_rgb;
	blend_info.info.alphaFunc = blend_func_a;
	blend_info.info.colorSrc = blend_sfactor_rgb;
	blend_info.info.colorDst = blend_dfactor_rgb;
	blend_info.info.alphaSrc = blend_sfactor_a;
	blend_info.info.alphaDst = blend_dfactor_a;
}

void change_blend_mask() {
	blend_info.info.colorMask = blend_color_mask;
	blend_info.info.colorFunc = SCE_GXM_BLEND_FUNC_NONE;
	blend_info.info.alphaFunc = SCE_GXM_BLEND_FUNC_NONE;
	blend_info.info.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
	blend_info.info.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_info.info.alphaSrc = SCE_GXM_BLEND_FACTOR_ONE;
	blend_info.info.alphaDst = SCE_GXM_BLEND_FACTOR_ZERO;
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
	switch (sfactor) {
	case GL_ZERO:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
		break;
	case GL_ONE:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
		break;
	case GL_SRC_COLOR:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case GL_DST_COLOR:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		blend_sfactor_rgb = blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (dfactor) {
	case GL_ZERO:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
		break;
	case GL_ONE:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
		break;
	case GL_SRC_COLOR:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case GL_DST_COLOR:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		blend_dfactor_rgb = blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
	switch (srcRGB) {
	case GL_ZERO:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO;
		break;
	case GL_ONE:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE;
		break;
	case GL_SRC_COLOR:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case GL_DST_COLOR:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (dstRGB) {
	case GL_ZERO:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO;
		break;
	case GL_ONE:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE;
		break;
	case GL_SRC_COLOR:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case GL_DST_COLOR:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (srcAlpha) {
	case GL_ZERO:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
		break;
	case GL_ONE:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
		break;
	case GL_SRC_COLOR:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case GL_DST_COLOR:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		blend_sfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (dstAlpha) {
	case GL_ZERO:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;
		break;
	case GL_ONE:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE;
		break;
	case GL_SRC_COLOR:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case GL_DST_COLOR:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		blend_dfactor_a = SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glBlendEquation(GLenum mode) {
	switch (mode) {
	case GL_FUNC_ADD:
		blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_ADD;
		break;
	case GL_FUNC_SUBTRACT:
		blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_SUBTRACT;
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;
		break;
	case GL_MIN:
		blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_MIN;
		break;
	case GL_MAX:
		blend_func_rgb = blend_func_a = SCE_GXM_BLEND_FUNC_MAX;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
	switch (modeRGB) {
	case GL_FUNC_ADD:
		blend_func_rgb = SCE_GXM_BLEND_FUNC_ADD;
		break;
	case GL_FUNC_SUBTRACT:
		blend_func_rgb = SCE_GXM_BLEND_FUNC_SUBTRACT;
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		blend_func_rgb = SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;
		break;
	case GL_MIN:
		blend_func_rgb = SCE_GXM_BLEND_FUNC_MIN;
		break;
	case GL_MAX:
		blend_func_rgb = SCE_GXM_BLEND_FUNC_MAX;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	switch (modeAlpha) {
	case GL_FUNC_ADD:
		blend_func_a = SCE_GXM_BLEND_FUNC_ADD;
		break;
	case GL_FUNC_SUBTRACT:
		blend_func_a = SCE_GXM_BLEND_FUNC_SUBTRACT;
		break;
	case GL_FUNC_REVERSE_SUBTRACT:
		blend_func_a = SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT;
		break;
	case GL_MIN:
		blend_func_a = SCE_GXM_BLEND_FUNC_MIN;
		break;
	case GL_MAX:
		blend_func_a = SCE_GXM_BLEND_FUNC_MAX;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
	if (blend_state)
		change_blend_factor();
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	blend_color_mask = SCE_GXM_COLOR_MASK_NONE;
	if (red)
		blend_color_mask += SCE_GXM_COLOR_MASK_R;
	if (green)
		blend_color_mask += SCE_GXM_COLOR_MASK_G;
	if (blue)
		blend_color_mask += SCE_GXM_COLOR_MASK_B;
	if (alpha)
		blend_color_mask += SCE_GXM_COLOR_MASK_A;
	if (blend_state)
		change_blend_factor();
	else
		change_blend_mask();
}
