/* 
 * state.h:
 * Header file managing state of openGL machine
 */

#ifndef _STATE_H_
#define _STATE_H_

// Drawing phases constants for legacy openGL
typedef enum glPhase{
	NONE = 0,
	MODEL_CREATION = 1
} glPhase;

// Vertex array attributes struct
typedef struct vertexArray{
	GLint size;
	GLint num;
	GLsizei stride;
	const GLvoid* pointer;
} vertexArray;

// Texture unit struct
typedef struct texture_unit{
	GLboolean enabled;
	matrix4x4 stack[GENERIC_STACK_DEPTH];
	texture   textures[TEXTURES_NUM];
	vertexArray vertex_array;
	vertexArray color_array;
	vertexArray texture_array;
	void* vertex_object;
	void* color_object;
	void* texture_object;
	void* index_object;
	int env_mode;
	int tex_id;
	SceGxmTextureFilter min_filter;
	SceGxmTextureFilter mag_filter;
	SceGxmTextureAddrMode u_mode;
	SceGxmTextureAddrMode v_mode;
} texture_unit;

// Blending
extern GLboolean blend_state; // Current state for GL_BLEND
extern SceGxmBlendFactor blend_sfactor_rgb;		// Current in use RGB source blend factor
extern SceGxmBlendFactor blend_dfactor_rgb;		// Current in use RGB dest blend factor
extern SceGxmBlendFactor blend_sfactor_a;		// Current in use A source blend factor
extern SceGxmBlendFactor blend_dfactor_a;		// Current in use A dest blend factor

// Depth Test
extern GLboolean depth_test_state;	// Current state for GL_DEPTH_TEST

// Polygon Mode
extern GLfloat pol_factor;	// Current factor for glPolygonOffset
extern GLfloat pol_units;	// Current units for glPolygonOffset

// Texture Units
extern texture_unit texture_units[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS]; // Available texture units
extern int8_t server_texture_unit;	// Current in use server side texture unit
extern int8_t client_texture_unit;	// Current in use client side texture unit
extern palette *color_table; // Current in-use color table

// Matrices
extern matrix4x4 *matrix; // Current in-use matrix mode

// Miscellaneous
extern glPhase phase; // Current drawing phase for legacy openGL

#endif
