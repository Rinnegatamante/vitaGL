/* 
 * state.h:
 * Header file managing state of openGL machine
 */

 #ifndef _STATE_H_
#define _STATE_H_
 
// Blending
GLboolean blend_state = GL_FALSE;										// Current state for GL_BLEND
SceGxmBlendFactor blend_sfactor_rgb = SCE_GXM_BLEND_FACTOR_ONE;		// Current in use RGB source blend factor
SceGxmBlendFactor blend_dfactor_rgb = SCE_GXM_BLEND_FACTOR_ZERO;	// Current in use RGB dest blend factor
SceGxmBlendFactor blend_sfactor_a = SCE_GXM_BLEND_FACTOR_ONE;		// Current in use A source blend factor
SceGxmBlendFactor blend_dfactor_a = SCE_GXM_BLEND_FACTOR_ZERO;		// Current in use A dest blend factor

// Depth Test
GLboolean depth_test_state = GL_FALSE;	// Current state for GL_DEPTH_TEST

// Polygon Mode
GLfloat pol_factor = 0.0f;	// Current factor for glPolygonOffset
GLfloat pol_units = 0.0f;	// Current units for glPolygonOffset

// Texture Units
int8_t server_texture_unit = 0;	// Current in use server side texture unit
int8_t client_texture_unit = 0;	// Current in use client side texture unit

#endif
