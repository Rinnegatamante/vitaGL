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
 * matrices.c:
 * Implementation for matrices related functions
 */

#include "shared.h"

matrix4x4 *matrix = NULL; // Current in-use matrix mode
static matrix4x4 modelview_matrix_stack[MODELVIEW_STACK_DEPTH]; // Modelview matrices stack
static uint8_t modelview_stack_counter = 0; // Modelview matrices stack counter
static matrix4x4 projection_matrix_stack[GENERIC_STACK_DEPTH]; // Projection matrices stack
static uint8_t projection_stack_counter = 0; // Projection matrices stack counter
GLboolean mvp_modified = GL_TRUE; // Check if ModelViewProjection matrix needs to be recreated

matrix4x4 mvp_matrix; // ModelViewProjection Matrix
matrix4x4 projection_matrix; // Projection Matrix
matrix4x4 modelview_matrix; // ModelView Matrix
matrix4x4 normal_matrix; // Normal Matrix
matrix4x4 texture_matrix[TEXTURE_COORDS_NUM]; // Texture Matrix

GLint get_gl_matrix_mode() {
	if (matrix == &texture_matrix[server_texture_unit]) {
		return GL_TEXTURE;
	}

	if (matrix == &projection_matrix) {
		return GL_PROJECTION;
	}

	return GL_MODELVIEW;
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glMatrixMode(GLenum mode) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glMatrixMode, "U", mode))
		return;
#endif
	// Changing current in use matrix
	switch (mode) {
	case GL_MODELVIEW: // Modelview matrix
		matrix = &modelview_matrix;
		break;
	case GL_PROJECTION: // Projection matrix
		matrix = &projection_matrix;
		break;
	case GL_TEXTURE: // Texture matrix
		matrix = &texture_matrix[server_texture_unit];
		break;
	default:
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, mode)
	}
}

void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if ((left == right) || (bottom == top) || (nearVal == farVal)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

#ifdef MATH_SPEEDHACK
	// Initializing ortho matrix with requested parameters
	matrix4x4_init_orthographic(*matrix, left, right, bottom, top, nearVal, farVal);
#else
	matrix4x4 res, ortho_matrix;
	matrix4x4_init_orthographic(ortho_matrix, left, right, bottom, top, nearVal, farVal);
	matrix4x4_multiply(res, *matrix, ortho_matrix);
	matrix4x4_copy(*matrix, res);
#endif

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal) {
	glOrthof(left, right, bottom, top, nearVal, farVal);
}

void glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal) {
	glOrthof((float)left / 65536.0f, (float)right / 65536.0f, (float)bottom / 65536.0f, (float)top / 65536.0f, (float)nearVal / 65536.0f, (float)farVal / 65536.0f);
}

void glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if ((left == right) || (bottom == top) || (nearVal < 0) || (farVal < 0)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

#ifdef MATH_SPEEDHACK
	// Initializing frustum matrix with requested parameters
	matrix4x4_init_frustum(*matrix, left, right, bottom, top, nearVal, farVal);
#else
	matrix4x4 res, frustum_matrix;
	matrix4x4_init_frustum(frustum_matrix, left, right, bottom, top, nearVal, farVal);
	matrix4x4_multiply(res, *matrix, frustum_matrix);
	matrix4x4_copy(*matrix, res);
#endif

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glFrustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed nearVal, GLfixed farVal) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if ((left == right) || (bottom == top) || (nearVal < 0) || (farVal < 0)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

#ifdef MATH_SPEEDHACK
	// Initializing frustum matrix with requested parameters
	matrix4x4_init_frustum(*matrix, (float)left / 65536.0f, (float)right / 65536.0f, (float)bottom / 65536.0f, (float)top / 65536.0f, (float)nearVal / 65536.0f, (float)farVal / 65536.0f);
#else
	matrix4x4 res, frustum_matrix;
	matrix4x4_init_frustum(frustum_matrix, (float)left / 65536.0f, (float)right / 65536.0f, (float)bottom / 65536.0f, (float)top / 65536.0f, (float)nearVal / 65536.0f, (float)farVal / 65536.0f);
	matrix4x4_multiply(res, *matrix, frustum_matrix);
	matrix4x4_copy(*matrix, res);
#endif

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal) {
	glFrustumf(left, right, bottom, top, nearVal, farVal);
}

void glLoadIdentity(void) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glLoadIdentity, ""))
		return;
#endif
	// Set current in use matrix to identity one
	matrix4x4_identity(*matrix);
	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glMultMatrixf(const GLfloat *m) {
	// Properly ordering matrix
	matrix4x4 res, src;
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			src[i][j] = m[j * 4 + i];
		}
	}
	
	// Multiplicating passed matrix with in use one
	matrix4x4_multiply(res, *matrix, src);

	// Copying result to in use matrix
	matrix4x4_copy(*matrix, res);

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glMultTransposeMatrixf(const GLfloat *m) {
	// Properly ordering matrix
	matrix4x4 res, src;
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			src[i][j] = m[i * 4 + j];
		}
	}
	
	// Multiplicating passed matrix with in use one
	matrix4x4_multiply(res, *matrix, src);

	// Copying result to in use matrix
	matrix4x4_copy(*matrix, res);

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glMultMatrixx(const GLfixed *m) {
	// Properly ordering matrix
	matrix4x4 res, src;
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			src[i][j] = (float)m[j * 4 + i] / 65536.0f;
		}
	}
	
	// Multiplicating passed matrix with in use one
	matrix4x4_multiply(res, *matrix, src);

	// Copying result to in use matrix
	matrix4x4_copy(*matrix, res);

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glMultTransposeMatrixx(const GLfixed *m) {
	// Properly ordering matrix
	matrix4x4 res, src;
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			src[i][j] = (float)m[i * 4 + j] / 65536.0f;
		}
	}
	
	// Multiplicating passed matrix with in use one
	matrix4x4_multiply(res, *matrix, src);

	// Copying result to in use matrix
	matrix4x4_copy(*matrix, res);

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glLoadMatrixf(const GLfloat *m) {
	// Properly ordering matrix
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			(*matrix)[i][j] = m[j * 4 + i];
		}
	}

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glLoadTransposeMatrixf(const GLfloat *m) {
	// Properly ordering matrix
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			(*matrix)[i][j] = m[i * 4 + j];
		}
	}

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glLoadMatrixx(const GLfixed *m) {
	// Properly ordering matrix
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			(*matrix)[i][j] = (float)m[j * 4 + i] / 65536.0f;
		}
	}

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glLoadTransposeMatrixx(const GLfixed *m) {
	// Properly ordering matrix
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			(*matrix)[i][j] = (float)m[i * 4 + j] / 65536.0f;
		}
	}

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	// Translating in use matrix
	matrix4x4_translate(*matrix, x, y, z);
	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glTranslatex(GLfixed x, GLfixed y, GLfixed z) {
	// Translating in use matrix
	matrix4x4_translate(*matrix, (float)x / 65536.0f, (float)y / 65536.0f, (float)z / 65536.0f);
	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
	// Scaling in use matrix
	matrix4x4_scale(*matrix, x, y, z);
	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glScalex(GLfixed x, GLfixed y, GLfixed z) {
	// Scaling in use matrix
	matrix4x4_scale(*matrix, (float)x / 65536.0f, (float)y / 65536.0f, (float)z / 65536.0f);
	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	// Performing rotation on in use matrix depending on user call
	float rad = DEG_TO_RAD(angle);
	matrix4x4_rotate(*matrix, rad, x, y, z);
	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	// Performing rotation on in use matrix depending on user call
	float rad = DEG_TO_RAD((float)angle / 65536.0f);
	matrix4x4_rotate(*matrix, rad, (float)x / 65536.0f, (float)y / 65536.0f, (float)z / 65536.0f);

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void glPushMatrix(void) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glPushMatrix, ""))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	if (matrix == &modelview_matrix) {
#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (modelview_stack_counter >= MODELVIEW_STACK_DEPTH) {
			SET_GL_ERROR(GL_STACK_OVERFLOW)
		} else
#endif
			// Copying current matrix into the matrix stack and increasing stack counter
			matrix4x4_copy(modelview_matrix_stack[modelview_stack_counter++], *matrix);

	} else if (matrix == &projection_matrix) {
#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (projection_stack_counter >= GENERIC_STACK_DEPTH) {
			SET_GL_ERROR(GL_STACK_OVERFLOW)
		} else
#endif
			// Copying current matrix into the matrix stack and increasing stack counter
			matrix4x4_copy(projection_matrix_stack[projection_stack_counter++], *matrix);
	} else if (matrix == &texture_matrix[server_texture_unit]) {
		texture_unit *tex_unit = &texture_units[server_texture_unit];

#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (tex_unit->texture_stack_counter >= GENERIC_STACK_DEPTH) {
			SET_GL_ERROR(GL_STACK_OVERFLOW)
		} else
#endif
			// Copying current matrix into the matrix stack and increasing stack counter
			matrix4x4_copy(tex_unit->texture_matrix_stack[tex_unit->texture_stack_counter++], *matrix);
	}
}

void glPopMatrix(void) {
#ifdef HAVE_DLISTS
	// Enqueueing function to a display list if one is being compiled
	if (_vgl_enqueue_list_func(glPopMatrix, ""))
		return;
#endif
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	if (matrix == &modelview_matrix) {
#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (modelview_stack_counter == 0) {
			SET_GL_ERROR(GL_STACK_UNDERFLOW)
		}
#endif
		// Copying last matrix on stack into current matrix and decreasing stack counter
		matrix4x4_copy(*matrix, modelview_matrix_stack[--modelview_stack_counter]);

		// MVP matrix will have to be updated
		mvp_modified = GL_TRUE;

	} else if (matrix == &projection_matrix) {
#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (projection_stack_counter == 0) {
			SET_GL_ERROR(GL_STACK_UNDERFLOW)
		}
#endif
		// Copying last matrix on stack into current matrix and decreasing stack counter
		matrix4x4_copy(*matrix, projection_matrix_stack[--projection_stack_counter]);

		// MVP matrix will have to be updated
		mvp_modified = GL_TRUE;

	} else if (matrix == &texture_matrix[server_texture_unit]) {
		texture_unit *tex_unit = &texture_units[server_texture_unit];

#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (tex_unit->texture_stack_counter == 0) {
			SET_GL_ERROR(GL_STACK_UNDERFLOW)
		}
#endif
		// Copying last matrix on stack into current matrix and decreasing stack counter
		matrix4x4_copy(*matrix, tex_unit->texture_matrix_stack[--tex_unit->texture_stack_counter]);

		dirty_vert_unifs = GL_TRUE;
	}
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif

	// Initializing perspective matrix with requested parameters
	matrix4x4_init_perspective(*matrix, fovy, aspect, zNear, zFar);

	if (matrix != &texture_matrix[server_texture_unit])
		mvp_modified = GL_TRUE;
	else
		dirty_vert_unifs = GL_TRUE;
}

void gluLookAt(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ) {
	float f[4], up[4];
	f[0] = centerX - eyeX;
	f[1] = centerY - eyeY;
	f[2] = centerZ - eyeZ;
	f[3] = 0.0f;
	vector4f_normalize((vector4f *)f);
	up[0] = upX;
	up[1] = upY;
	up[2] = upZ;
	up[3] = 0.0f;
	vector4f_normalize((vector4f *)up);
	
	matrix4x4 m, res;
	float s[4], u[3];
	vector3f_cross_product((vector3f *)s, (vector3f *)f, (vector3f *)up);
	s[3] = 0.0f;
	m[0][0] = s[0];
	m[0][1] = s[1];
	m[0][2] = s[2];
	m[0][3] = 0.0f;
	
	vector4f_normalize((vector4f *)s);
	vector3f_cross_product((vector3f *)u, (vector3f *)s, (vector3f *)f);
	m[1][0] = u[0];
	m[1][1] = u[1];
	m[1][2] = u[2];
	m[1][3] = 0.0f;
	m[2][0] = -f[0];
	m[2][1] = -f[1];
	m[2][2] = -f[2];
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
	
	matrix4x4_multiply(res, m, *matrix);
	matrix4x4_copy(*matrix, res);
	matrix4x4_translate(*matrix, -eyeX, -eyeY, -eyeZ);
}
