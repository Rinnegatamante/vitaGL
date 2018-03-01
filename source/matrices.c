/* 
 * matrices.c:
 * Implementation for matrices related functions
 */
 
 #include "shared.h"
 
matrix4x4 *matrix = NULL; // Current in-use matrix mode
 
/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */
 
void glMatrixMode(GLenum mode){
	
	// Changing current in use matrix
	switch (mode){
		case GL_MODELVIEW: // Modelview matrix
			matrix = &modelview_matrix;
			break;
		case GL_PROJECTION: // Projection matrix
			matrix = &projection_matrix;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	
}

void glOrtho(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal){
	
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}else if ((left == right) || (bottom == top) || (nearVal == farVal)){
		error = GL_INVALID_VALUE;
		return;
	}
#endif
	
	// Initializing ortho matrix with requested parameters
	matrix4x4_init_orthographic(*matrix, left, right, bottom, top, nearVal, farVal);
	
}

void glFrustum(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal){
	
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}else if ((left == right) || (bottom == top) || (nearVal < 0) || (farVal < 0)){
		error = GL_INVALID_VALUE;
		return;
	}
#endif
	
	// Initializing frustum matrix with requested parameters
	matrix4x4_init_frustum(*matrix, left, right, bottom, top, nearVal, farVal);
	
}

void glLoadIdentity(void){
	
	// Set current in use matrix to identity one
	matrix4x4_identity(*matrix);
	
}

void glMultMatrixf(const GLfloat* m){
	
	// Properly ordering matrix to perform multiplication
	matrix4x4 res;
	matrix4x4 tmp;
	int i,j;
	for (i=0;i<4;i++){
		for (j=0;j<4;j++){
			tmp[i][j] = m[i*4+j];
		}
	}
	
	// Multiplicating passed matrix with in use one
	matrix4x4_multiply(res, *matrix, tmp);
	
	// Copying result to in use matrix
	matrix4x4_copy(*matrix, res);
	
}

void glLoadMatrixf(const GLfloat* m){
	
	// Properly ordering matrix
	matrix4x4 tmp;
	int i,j;
	for (i=0;i<4;i++){
		for (j=0;j<4;j++){
			tmp[i][j] = m[i*4+j];
		}
	}
	
	// Copying passed matrix to in use one
	matrix4x4_copy(*matrix, tmp);
	
}

void glTranslatef(GLfloat x,  GLfloat y,  GLfloat z){
	
	// Translating in use matrix
	matrix4x4_translate(*matrix, x, y, z);
	
}

void glScalef(GLfloat x, GLfloat y, GLfloat z){
	
	// Scaling in use matrix
	matrix4x4_scale(*matrix, x, y, z);
	
}

void glRotatef(GLfloat angle,  GLfloat x,  GLfloat y,  GLfloat z){
	
#ifndef SKIP_ERROR_HANDLING
	// Error handling
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
#endif
	
	// Performing rotation on in use matrix depending on user call
	float rad = DEG_TO_RAD(angle);
	if (x == 1.0f){
		matrix4x4_rotate_x(*matrix, rad);
	}
	if (y == 1.0f){
		matrix4x4_rotate_y(*matrix, rad);
	}
	if (z == 1.0f){
		matrix4x4_rotate_z(*matrix, rad);
	}
	
}
