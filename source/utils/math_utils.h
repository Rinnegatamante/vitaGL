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
 * math_utils.h:
 * Header file for the math utilities exposed by math_utils.c
 */

#ifndef _MATH_UTILS_H_
#define _MATH_UTILS_H_

#include <math.h>
#include <math_neon.h>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD(x) ((x)*M_PI / 180.0)
#endif

// clang-format off
// vector of 2 floats struct
typedef struct {
	float x, y;
} vector2f;

// vector of 3 floats struct
typedef struct {
	union { float x; float r; };
	union { float y; float g; };
	union { float z; float b; };
} vector3f;

// vector of 4 floats struct
typedef struct {
	union { float x; float r; };
	union { float y; float g; };
	union { float z; float b; };
	union { float w; float a; };
} vector4f;
// clang-format on

// Matrix types
typedef float matrix2x2[2][2];
typedef float matrix3x3[3][3];
typedef float matrix4x4[4][4];

// Creates an identity matrix
static inline __attribute__((always_inline)) void matrix4x4_identity(matrix4x4 m) {
	sceClibMemset(m, 0, sizeof(matrix4x4));
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

// Copy a matrix to another one
static inline __attribute__((always_inline)) void matrix4x4_copy(matrix4x4 dst, const matrix4x4 src) {
	vgl_fast_memcpy(dst, src, sizeof(matrix4x4));
}

// Perform a matrix per matrix moltiplication
static inline __attribute__((always_inline)) void matrix4x4_multiply(matrix4x4 dst, const matrix4x4 src1, const matrix4x4 src2) {
	matmul4_neon((float *)src2, (float *)src1, (float *)dst);
}

// Rotate a matrix on x,y,z axis
static inline __attribute__((always_inline)) void matrix4x4_rotate(matrix4x4 m, float rad, float x, float y, float z) {
	float cs[2];
	sincosf_c(rad, cs);

	matrix4x4 m1, m2;
	sceClibMemset(m1, 0, sizeof(matrix4x4));
	const float c = 1 - cs[1];
	float axis[3] = {x, y, z};
	normalize3_neon(axis, axis);
	const float xc = axis[0] * c, yc = axis[1] * c, zc = axis[2] * c;
	m1[0][0] = axis[0] * xc + cs[1];
	m1[1][0] = axis[1] * xc + axis[2] * cs[0];
	m1[2][0] = axis[2] * xc - axis[1] * cs[0];

	m1[0][1] = axis[0] * yc - axis[2] * cs[0];
	m1[1][1] = axis[1] * yc + cs[1];
	m1[2][1] = axis[2] * yc + axis[0] * cs[0];

	m1[0][2] = axis[0] * zc + axis[1] * cs[0];
	m1[1][2] = axis[1] * zc - axis[0] * cs[0];
	m1[2][2] = axis[2] * zc + cs[1];

	m1[3][3] = 1.0f;

	matrix4x4_multiply(m2, m, m1);
	matrix4x4_copy(m, m2);
}

// Translate a matrix
static inline __attribute__((always_inline)) void matrix4x4_translate(matrix4x4 m, float x, float y, float z) {
	matrix4x4 m1, m2;
	matrix4x4_identity(m1);
	m1[0][3] = x;
	m1[1][3] = y;
	m1[2][3] = z;

	matrix4x4_multiply(m2, m, m1);
	matrix4x4_copy(m, m2);
}

// Scale a matrix
static inline __attribute__((always_inline)) void matrix4x4_scale(matrix4x4 m, float x, float y, float z) {
	matrix4x4 m1, m2;
	sceClibMemset(m1, 0, sizeof(matrix4x4));
	m1[0][0] = x;
	m1[1][1] = y;
	m1[2][2] = z;
	m1[3][3] = 1.0f;

	matrix4x4_multiply(m2, m, m1);
	matrix4x4_copy(m, m2);
}

// Transpose a matrix
static inline __attribute__((always_inline)) void matrix2x2_transpose(matrix2x2 out, const matrix2x2 m) {
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++)
			out[i][j] = m[j][i];
	}
}
static inline __attribute__((always_inline)) void matrix3x3_transpose(matrix3x3 out, const matrix3x3 m) {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++)
			out[i][j] = m[j][i];
	}
}
static inline __attribute__((always_inline)) void matrix4x4_transpose(matrix4x4 out, const matrix4x4 m) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++)
			out[i][j] = m[j][i];
	}
}

// Init a matrix with different settings (ortho, frustum, perspective)
static inline __attribute__((always_inline)) void matrix4x4_init_orthographic(matrix4x4 m, float left, float right, float bottom, float top, float near, float far) {
	sceClibMemset(m, 0, sizeof(matrix4x4));
	m[0][0] = 2.0f / (right - left);
	m[0][3] = -(right + left) / (right - left);
	m[1][1] = 2.0f / (top - bottom);
	m[1][3] = -(top + bottom) / (top - bottom);
	m[2][2] = -2.0f / (far - near);
	m[2][3] = -(far + near) / (far - near);
	m[3][3] = 1.0f;
}
static inline __attribute__((always_inline)) void matrix4x4_init_frustum(matrix4x4 m, float left, float right, float bottom, float top, float near, float far) {
	sceClibMemset(m, 0, sizeof(matrix4x4));
	m[0][0] = (2.0f * near) / (right - left);
	m[0][2] = (right + left) / (right - left);
	m[1][1] = (2.0f * near) / (top - bottom);
	m[1][2] = (top + bottom) / (top - bottom);
	m[2][2] = -(far + near) / (far - near);
	m[2][3] = (-2.0f * far * near) / (far - near);
	m[3][2] = -1.0f;
}
static inline __attribute__((always_inline)) void matrix4x4_init_perspective(matrix4x4 m, float fov, float aspect, float near, float far) {
	float half_height = near * tanf_neon(DEG_TO_RAD(fov) * 0.5f);
	float half_width = half_height * aspect;

	matrix4x4_init_frustum(m, -half_width, half_width, -half_height, half_height, near, far);
}

// Invert a matrix
static inline __attribute__((always_inline)) int matrix3x3_invert(matrix3x3 out, const matrix3x3 in) {
	float inv[9], det;
	float *invOut = (float *)&out[0][0];
	float *m = (float *)&in[0][0];
	int i;

	inv[ 0] = m[4] * m[8] - m[5] * m[7];
	inv[ 1] = m[2] * m[7] - m[1] * m[8];
	inv[ 2] = m[1] * m[5] - m[2] * m[4];

	det = m[0] * inv[0] + m[3] * inv[1] + m[6] * inv[2];

	if (det == 0)
		return 0;

	inv[ 3] = m[5] * m[6] - m[3] * m[8];
	inv[ 4] = m[0] * m[8] - m[2] * m[6];
	inv[ 5] = m[2] * m[3] - m[0] * m[5];
	inv[ 6] = m[3] * m[7] - m[4] * m[6];
	inv[ 7] = m[1] * m[6] - m[0] * m[7];
	inv[ 8] = m[0] * m[4] - m[1] * m[3];

	det = 1.f / det;

	for(i = 0; i < 9; i++)
		invOut[i] = inv[i] * det;
	
	return 1;
}
static inline __attribute__((always_inline)) int matrix4x4_invert(matrix4x4 out, const matrix4x4 in) {
	float inv[16], det;
	float *invOut = (float *)&out[0][0];
	float *m = (float *)&in[0][0];
	int i;
  
	inv[ 0] =  m[5] * m[10] * m[15] - m[5] * m[14] * m[11] - m[6] * m[9] * m[15] + m[6] * m[13] * m[11] + m[7] * m[9] * m[14] - m[7] * m[13] * m[10];
	inv[ 1] = -m[1] * m[10] * m[15] + m[1] * m[14] * m[11] + m[2] * m[9] * m[15] - m[2] * m[13] * m[11] - m[3] * m[9] * m[14] + m[3] * m[13] * m[10];
	inv[ 2] =  m[1] * m[ 6] * m[15] - m[1] * m[14] * m[ 7] - m[2] * m[5] * m[15] + m[2] * m[13] * m[ 7] + m[3] * m[5] * m[14] - m[3] * m[13] * m[ 6];
	inv[ 3] = -m[1] * m[ 6] * m[11] + m[1] * m[10] * m[ 7] + m[2] * m[5] * m[11] - m[2] * m[ 9] * m[ 7] - m[3] * m[5] * m[10] + m[3] * m[ 9] * m[ 6];
	
	det = m[0] * inv[0] + m[4] * inv[1] + m[8] * inv[2] + m[12] * inv[3];
  
	if(det == 0)
		return 0;
	
	inv[ 4] = -m[4] * m[10] * m[15] + m[4] * m[14] * m[11] + m[6] * m[8] * m[15] - m[6] * m[12] * m[11] - m[7] * m[8] * m[14] + m[7] * m[12] * m[10];
	inv[ 5] =  m[0] * m[10] * m[15] - m[0] * m[14] * m[11] - m[2] * m[8] * m[15] + m[2] * m[12] * m[11] + m[3] * m[8] * m[14] - m[3] * m[12] * m[10];
	inv[ 6] = -m[0] * m[ 6] * m[15] + m[0] * m[14] * m[ 7] + m[2] * m[4] * m[15] - m[2] * m[12] * m[ 7] - m[3] * m[4] * m[14] + m[3] * m[12] * m[ 6];
	inv[ 7] =  m[0] * m[ 6] * m[11] - m[0] * m[10] * m[ 7] - m[2] * m[4] * m[11] + m[2] * m[ 8] * m[ 7] + m[3] * m[4] * m[10] - m[3] * m[ 8] * m[ 6];
	inv[ 8] =  m[4] * m[ 9] * m[15] - m[4] * m[13] * m[11] - m[5] * m[8] * m[15] + m[5] * m[12] * m[11] + m[7] * m[8] * m[13] - m[7] * m[12] * m[ 9];
	inv[ 9] = -m[0] * m[ 9] * m[15] + m[0] * m[13] * m[11] + m[1] * m[8] * m[15] - m[1] * m[12] * m[11] - m[3] * m[8] * m[13] + m[3] * m[12] * m[ 9];
	inv[10] =  m[0] * m[ 5] * m[15] - m[0] * m[13] * m[ 7] - m[1] * m[4] * m[15] + m[1] * m[12] * m[ 7] + m[3] * m[4] * m[13] - m[3] * m[12] * m[ 5];
	inv[11] = -m[0] * m[ 5] * m[11] + m[0] * m[ 9] * m[ 7] + m[1] * m[4] * m[11] - m[1] * m[ 8] * m[ 7] - m[3] * m[4] * m[ 9] + m[3] * m[ 8] * m[ 5];
	inv[12] = -m[4] * m[ 9] * m[14] + m[4] * m[13] * m[10] + m[5] * m[8] * m[14] - m[5] * m[12] * m[10] - m[6] * m[8] * m[13] + m[6] * m[12] * m[ 9];
	inv[13] =  m[0] * m[ 9] * m[14] - m[0] * m[13] * m[10] - m[1] * m[8] * m[14] + m[1] * m[12] * m[10] + m[2] * m[8] * m[13] - m[2] * m[12] * m[ 9];
	inv[14] = -m[0] * m[ 5] * m[14] + m[0] * m[13] * m[ 6] + m[1] * m[4] * m[14] - m[1] * m[12] * m[ 6] - m[2] * m[4] * m[13] + m[2] * m[12] * m[ 5];
	inv[15] =  m[0] * m[ 5] * m[10] - m[0] * m[ 9] * m[ 6] - m[1] * m[4] * m[10] + m[1] * m[ 8] * m[ 6] + m[2] * m[4] * m[ 9] - m[2] * m[ 8] * m[ 5];
  
	det = 1.f / det;
  
	for(i = 0; i < 16; i++)
		invOut[i] = inv[i] * det;
  
	return 1;
}

// Perform a matrix per vector moltiplication
static inline __attribute__((always_inline)) void vector4f_matrix4x4_mult(vector4f *u, const matrix4x4 m, const vector4f *v) {
	u->x = m[0][0] * v->x + m[0][1] * v->y + m[0][2] * v->z + m[0][3] * v->w;
	u->y = m[1][0] * v->x + m[1][1] * v->y + m[1][2] * v->z + m[1][3] * v->w;
	u->z = m[2][0] * v->x + m[2][1] * v->y + m[2][2] * v->z + m[2][3] * v->w;
	u->w = m[3][0] * v->x + m[3][1] * v->y + m[3][2] * v->z + m[3][3] * v->w;
}

// Cross product between two vectors
static inline __attribute__((always_inline)) void vector3f_cross_product(vector3f *r, const vector3f *v1, const vector3f *v2) {
	r->x = v1->y * v2->z - v1->z * v2->y;
	r->y = -v1->x * v2->z + v1->z * v2->x;
	r->z = v1->x * v2->y - v1->y * v2->x;
}

// Normalize a vector
static inline __attribute__((always_inline)) void vector4f_normalize(vector4f *v) {
	normalize4_neon((float *)v, (float *)v);
}

#endif
