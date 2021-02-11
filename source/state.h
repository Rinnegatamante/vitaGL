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
 * state.h:
 * Header file managing state of openGL machine
 */

#ifndef _STATE_H_
#define _STATE_H_

#define MAX_CLIP_PLANES_NUM 7 // Maximum number of allowed user defined clip planes
#define LEGACY_VERTEX_STRIDE 24 // Vertex stride for GL1 immediate draw pipeline
#define MAX_LIGHTS_NUM 8 // Maximum number of allowed light sources

// Drawing phases constants for legacy openGL
typedef enum {
	NONE,
	MODEL_CREATION
} glPhase;

// Vertex array attributes struct
typedef struct {
	GLint size;
	GLint num;
	GLsizei stride;
	const GLvoid *pointer;
} vertexArray;

// Scissor test region struct
typedef struct {
	int x;
	int y;
	int w;
	int h;
	int gl_y;
} scissor_region;

// Viewport struct
typedef struct {
	int x;
	int y;
	int w;
	int h;
} viewport;

// Alpha operations for alpha testing
typedef enum {
	GREATER_EQUAL,
	GREATER,
	NOT_EQUAL,
	EQUAL,
	LESS_EQUAL,
	LESS,
	NEVER,
	ALWAYS
} alphaOp;

// Fog modes
typedef enum {
	LINEAR,
	EXP,
	EXP2,
	DISABLED
} fogType;

// Texture unit struct
typedef struct {
	GLboolean enabled;
	matrix4x4 stack[GENERIC_STACK_DEPTH];
	void *vertex_object;
	void *color_object;
	void *texture_object;
	void *index_object;
	int env_mode;
	int tex_id;
} texture_unit;

// Framebuffer struct
typedef struct {
	uint8_t active;
	SceGxmRenderTarget *target;
	SceGxmColorSurface colorbuffer;
	SceGxmDepthStencilSurface depthbuffer;
	void *depth_buffer_addr;
	void *stencil_buffer_addr;
	int width;
	int height;
	int stride;
	void *data;
	uint32_t data_type;
} framebuffer;

// Blending
extern GLboolean blend_state; // Current state for GL_BLEND
extern SceGxmBlendFactor blend_sfactor_rgb; // Current in use RGB source blend factor
extern SceGxmBlendFactor blend_dfactor_rgb; // Current in use RGB dest blend factor
extern SceGxmBlendFactor blend_sfactor_a; // Current in use A source blend factor
extern SceGxmBlendFactor blend_dfactor_a; // Current in use A dest blend factor

// Depth Test
extern GLboolean depth_test_state; // Current state for GL_DEPTH_TEST
extern SceGxmDepthFunc gxm_depth; // Current in-use depth test func
extern GLenum orig_depth_test; // Original depth test state (used for depth test invalidation)
extern GLdouble depth_value; // Current depth test clear value
extern GLboolean depth_mask_state; // Current state for glDepthMask

// Scissor Test
extern scissor_region region; // Current scissor test region setup
extern GLboolean scissor_test_state; // Current state for GL_SCISSOR_TEST

// Stencil Test
extern uint8_t stencil_mask_front; // Current in use mask for stencil test on front
extern uint8_t stencil_mask_back; // Current in use mask for stencil test on back
extern uint8_t stencil_mask_front_write; // Current in use mask for write stencil test on front
extern uint8_t stencil_mask_back_write; // Current in use mask for write stencil test on back
extern uint8_t stencil_ref_front; // Current in use reference for stencil test on front
extern uint8_t stencil_ref_back; // Current in use reference for stencil test on back
extern SceGxmStencilOp stencil_fail_front; // Current in use stencil operation when stencil test fails for front
extern SceGxmStencilOp depth_fail_front; // Current in use stencil operation when depth test fails for front
extern SceGxmStencilOp depth_pass_front; // Current in use stencil operation when depth test passes for front
extern SceGxmStencilOp stencil_fail_back; // Current in use stencil operation when stencil test fails for back
extern SceGxmStencilOp depth_fail_back; // Current in use stencil operation when depth test fails for back
extern SceGxmStencilOp depth_pass_back; // Current in use stencil operation when depth test passes for back
extern SceGxmStencilFunc stencil_func_front; // Current in use stencil function on front
extern SceGxmStencilFunc stencil_func_back; // Current in use stencil function on back
extern GLboolean stencil_test_state; // Current state for GL_STENCIL_TEST
extern GLint stencil_value; // Current stencil test clear value

// Alpha Test
extern GLenum alpha_func; // Current in use alpha test mode
extern GLfloat alpha_ref; // Current in use alpha test reference value
extern int alpha_op; // Current in use alpha test operation
extern GLboolean alpha_test_state; // Current state for GL_ALPHA_TEST

// Polygon Mode
extern GLfloat pol_factor; // Current factor for glPolygonOffset
extern GLfloat pol_units; // Current units for glPolygonOffset

// Texture Units
extern texture_unit texture_units[GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS]; // Available texture units
extern texture texture_slots[TEXTURES_NUM]; // Available texture slots
extern int8_t server_texture_unit; // Current in use server side texture unit
extern int8_t client_texture_unit; // Current in use client side texture unit
extern palette *color_table; // Current in-use color table

// Matrices
extern matrix4x4 *matrix; // Current in-use matrix mode

// Miscellaneous
extern glPhase phase; // Current drawing phase for legacy openGL
extern vector4f current_color; // Current in use color
extern vector4f clear_rgba_val; // Current clear color for glClear
extern viewport gl_viewport; // Current viewport state

// Culling
extern GLboolean no_polygons_mode; // GL_TRUE when cull mode is set to GL_FRONT_AND_BACK
extern GLboolean cull_face_state; // Current state for GL_CULL_FACE
extern GLenum gl_cull_mode; // Current in use openGL cull mode
extern GLenum gl_front_face; // Current in use openGL setting for front facing primitives

// Polygon Offset
extern GLboolean pol_offset_fill; // Current state for GL_POLYGON_OFFSET_FILL
extern GLboolean pol_offset_line; // Current state for GL_POLYGON_OFFSET_LINE
extern GLboolean pol_offset_point; // Current state for GL_POLYGON_OFFSET_POINT
extern SceGxmPolygonMode polygon_mode_front; // Current in use polygon mode for front
extern SceGxmPolygonMode polygon_mode_back; // Current in use polygon mode for back
extern GLenum gl_polygon_mode_front; // Current in use polygon mode for front
extern GLenum gl_polygon_mode_back; // Current in use polygon mode for back

// Texture Environment
extern vector4f texenv_color; // Current in use texture environment color

// Lighting
extern GLboolean lighting_state; // Current lighting processor state
extern GLboolean lights_aligned; // Are clip planes in a contiguous range
extern uint8_t light_range[2]; // The highest and lowest enabled lights
extern uint8_t light_mask; // Bitmask of enabled lights
extern vector4f lights_ambients[MAX_LIGHTS_NUM];
extern vector4f lights_diffuses[MAX_LIGHTS_NUM];
extern vector4f lights_speculars[MAX_LIGHTS_NUM];
extern vector4f lights_positions[MAX_LIGHTS_NUM];
extern vector3f lights_attenuations[MAX_LIGHTS_NUM];

// Fogging
extern GLboolean fogging; // Current fogging processor state
extern GLint fog_mode; // Current fogging mode (openGL)
extern fogType internal_fog_mode; // Current fogging mode (sceGxm)
extern GLfloat fog_density; // Current fogging density
extern GLfloat fog_near; // Current fogging near distance
extern GLfloat fog_far; // Current fogging far distance
extern vector4f fog_color; // Current fogging color

// Clipping Planes
extern GLboolean clip_planes_aligned; // Are clip planes in a contiguous range
extern uint8_t clip_plane_range[2]; // The highest and lowest enabled clip planes
extern uint8_t clip_planes_mask; // Bitmask of enabled clip planes
extern vector4f clip_planes_eq[MAX_CLIP_PLANES_NUM]; // Current equation for user clip planes

// Framebuffers
extern framebuffer *active_read_fb; // Current readback framebuffer in use
extern framebuffer *active_write_fb; // Current write framebuffer in use

#endif
