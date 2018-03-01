/* 
 * state.h:
 * Header file managing state of openGL machine
 */

#ifndef _STATE_H_
#define _STATE_H_
 
// Blending
extern GLboolean blend_state; // Current state for GL_BLEND
extern SceGxmBlendFactor blend_sfactor_rgb;		// Current in use RGB source blend factor
extern SceGxmBlendFactor blend_dfactor_rgb;	// Current in use RGB dest blend factor
extern SceGxmBlendFactor blend_sfactor_a;		// Current in use A source blend factor
extern SceGxmBlendFactor blend_dfactor_a;		// Current in use A dest blend factor

// Depth Test
extern GLboolean depth_test_state;	// Current state for GL_DEPTH_TEST

// Polygon Mode
extern GLfloat pol_factor;	// Current factor for glPolygonOffset
extern GLfloat pol_units;	// Current units for glPolygonOffset

// Texture Units
extern int8_t server_texture_unit;	// Current in use server side texture unit
extern int8_t client_texture_unit;	// Current in use client side texture unit

#endif
