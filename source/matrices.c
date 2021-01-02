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
static matrix4x4 texture_matrix_stack[GENERIC_STACK_DEPTH]; // Texture matrices stack
static uint8_t texture_stack_counter = 0; // Texture matrices stack counter
GLboolean mvp_modified = GL_TRUE; // Check if ModelViewProjection matrix needs to be recreated

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void glMatrixMode(GLenum mode) {
	// Changing current in use matrix
	switch (mode) {
	case GL_MODELVIEW: // Modelview matrix
		matrix = &modelview_matrix;
		break;
	case GL_PROJECTION: // Projection matrix
		matrix = &projection_matrix;
		break;
	case GL_TEXTURE: // Texture matrix
		matrix = &texture_matrix;
		break;
	default:
		SET_GL_ERROR(GL_INVALID_ENUM)
		break;
	}
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if ((left == right) || (bottom == top) || (nearVal == farVal)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Initializing ortho matrix with requested parameters
	matrix4x4_init_orthographic(*matrix, left, right, bottom, top, nearVal, farVal);
	mvp_modified = GL_TRUE;
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal) {
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	} else if ((left == right) || (bottom == top) || (nearVal < 0) || (farVal < 0)) {
		SET_GL_ERROR(GL_INVALID_VALUE)
	}
#endif

	// Initializing frustum matrix with requested parameters
	matrix4x4_init_frustum(*matrix, left, right, bottom, top, nearVal, farVal);
	mvp_modified = GL_TRUE;
}

void glLoadIdentity(void) {
	// Set current in use matrix to identity one
	matrix4x4_identity(*matrix);
	if (matrix != &texture_matrix) {
		mvp_modified = GL_TRUE;
	}
}

void glMultMatrixf(const GLfloat *m) {
	matrix4x4 res;

	// Properly ordering matrix to perform multiplication
	matrix4x4 tmp;
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			tmp[i][j] = m[j * 4 + i];
		}
	}

	// Multiplicating passed matrix with in use one
	matrix4x4_multiply(res, *matrix, tmp);

	// Copying result to in use matrix
	matrix4x4_copy(*matrix, res);

	if (matrix != &texture_matrix) {
		mvp_modified = GL_TRUE;
	}
}

void glLoadMatrixf(const GLfloat *m) {
	// Properly ordering matrix
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			(*matrix)[i][j] = m[j * 4 + i];
		}
	}

	if (matrix != &texture_matrix) {
		mvp_modified = GL_TRUE;
	}
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
	// Translating in use matrix
	matrix4x4_translate(*matrix, x, y, z);
	if (matrix != &texture_matrix) {
		mvp_modified = GL_TRUE;
	}
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
	// Scaling in use matrix
	matrix4x4_scale(*matrix, x, y, z);
	if (matrix != &texture_matrix) {
		mvp_modified = GL_TRUE;
	}
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
	if (x == 1.0f) {
		matrix4x4_rotate_x(*matrix, rad);
	}
	if (y == 1.0f) {
		matrix4x4_rotate_y(*matrix, rad);
	}
	if (z == 1.0f) {
		matrix4x4_rotate_z(*matrix, rad);
	}

	if (matrix != &texture_matrix) {
		mvp_modified = GL_TRUE;
	}
}

void glPushMatrix(void) {
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
	} else if (matrix == &texture_matrix) {
#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (texture_stack_counter >= GENERIC_STACK_DEPTH) {
			SET_GL_ERROR(GL_STACK_OVERFLOW)
		} else
#endif
			// Copying current matrix into the matrix stack and increasing stack counter
			matrix4x4_copy(texture_matrix_stack[texture_stack_counter++], *matrix);
	}
}

void glPopMatrix(void) {
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
		} else
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
		} else
#endif
			// Copying last matrix on stack into current matrix and decreasing stack counter
			matrix4x4_copy(*matrix, projection_matrix_stack[--projection_stack_counter]);
			// MVP matrix will have to be updated
			mvp_modified = GL_TRUE;
	} else if (matrix == &texture_matrix) {
#ifndef SKIP_ERROR_HANDLING
		// Error handling
		if (texture_stack_counter == 0) {
			SET_GL_ERROR(GL_STACK_UNDERFLOW)
		} else
#endif
			// Copying last matrix on stack into current matrix and decreasing stack counter
			matrix4x4_copy(*matrix, texture_matrix_stack[--texture_stack_counter]);
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
	mvp_modified = GL_TRUE;
}
