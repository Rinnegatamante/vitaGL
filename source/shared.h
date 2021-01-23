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
 * shared.h:
 * All functions/definitions that shouldn't be exposed to
 * end users but are used in multiple source files must be here
 */

#ifndef _SHARED_H_
#define _SHARED_H_

// Internal constants
#define TEXTURES_NUM 16384 // Available textures
#define COMPRESSED_TEXTURE_FORMATS_NUM 9 // The number of supported texture formats
#define MODELVIEW_STACK_DEPTH 32 // Depth of modelview matrix stack
#define GENERIC_STACK_DEPTH 2 // Depth of generic matrix stack
#define DISPLAY_WIDTH_DEF 960 // Default display width in pixels
#define DISPLAY_HEIGHT_DEF 544 // Default display height in pixels
#define DISPLAY_MAX_BUFFER_COUNT 3 // Maximum amount of display buffers to use
#define GXM_TEX_MAX_SIZE 4096 // Maximum width/height in pixels per texture
#define BUFFERS_NUM 384 // Maximum number of allocatable buffers
#define FRAME_PURGE_LIST_SIZE 16384 // Number of elements a single frame can hold

// Internal constants set in bootup phase
extern int DISPLAY_WIDTH; // Display width in pixels
extern int DISPLAY_HEIGHT; // Display height in pixels
extern int DISPLAY_STRIDE; // Display stride in pixels
extern float DISPLAY_WIDTH_FLOAT; // Display width in pixels (float)
extern float DISPLAY_HEIGHT_FLOAT; // Display height in pixels (float)

// Translates a GL primitive enum to its sceGxm equivalent
#define gl_primitive_to_gxm(x, p) \
	switch (x) { \
	case GL_POINTS: \
		p = SCE_GXM_PRIMITIVE_POINTS; \
		break; \
	case GL_LINES: \
		p = SCE_GXM_PRIMITIVE_LINES; \
		break; \
	case GL_TRIANGLES: \
		if (no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		break; \
	case GL_TRIANGLE_STRIP: \
		if (no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP; \
		break; \
	case GL_TRIANGLE_FAN: \
		if (no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN; \
		break; \
	default: \
		SET_GL_ERROR(GL_INVALID_ENUM) \
	}

#include <stdio.h>
#include <stdlib.h>
#include <vitasdk.h>

#include "vitaGL.h"

#include "utils/gpu_utils.h"
#include "utils/math_utils.h"
#include "utils/mem_utils.h"

#include "state.h"
#include "texture_callbacks.h"

#define SET_GL_ERROR(x) \
	vgl_error = x;      \
	return;
	
#ifdef HAVE_SOFTFP_ABI
extern __attribute__((naked)) void sceGxmSetViewport_sfp(SceGxmContext *context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale);
#define setViewport sceGxmSetViewport_sfp
#else
#define setViewport sceGxmSetViewport
#endif

// Texture environment mode
typedef enum texEnvMode {
	MODULATE = 0,
	DECAL = 1,
	BLEND = 2,
	ADD = 3,
	REPLACE = 4
} texEnvMode;

// VBO struct
typedef struct gpubuffer {
	void *ptr;
	int32_t size;
	GLboolean used;
} gpubuffer;

// 3D vertex for position + 4D vertex for RGBA color struct
typedef struct rgba_vertex {
	vector3f position;
	vector4f color;
} rgba_vertex;

// 3D vertex for position + 3D vertex for RGB color struct
typedef struct rgb_vertex {
	vector3f position;
	vector3f color;
} rgb_vertex;

// 3D vertex for position + 2D vertex for UV map struct
typedef struct texture2d_vertex {
	vector3f position;
	vector2f texcoord;
} texture2d_vertex;

// Non native primitives implemented
typedef enum SceGxmPrimitiveTypeExtra {
	SCE_GXM_PRIMITIVE_NONE = 0,
	SCE_GXM_PRIMITIVE_QUADS = 1
} SceGxmPrimitiveTypeExtra;

// Blend info internal struct
typedef union {
	SceGxmBlendInfo info;
	uint32_t raw;
} blend_config;

#include "shaders.h"

// Internal stuffs
extern void *frag_uniforms;
extern void *vert_uniforms;
extern SceGxmMultisampleMode msaa_mode;
extern GLboolean use_extra_mem;
extern blend_config blend_info;
extern SceGxmVertexAttribute vertex_attrib_config[GL_MAX_VERTEX_ATTRIBS];

// Debugging tool
#ifdef ENABLE_LOG
void LOG(const char *format, ...);
#endif

// Logging callback for vitaShaRK
#if defined(HAVE_SHARK) && defined(HAVE_SHARK_LOG)
void shark_log_cb(const char *msg, shark_log_level msg_level, int line);
#endif

// Depending on SDK, these could be or not defined
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

extern GLboolean use_shark; // Flag to check if vitaShaRK should be initialized at vitaGL boot
extern GLboolean is_shark_online; // Current vitaShaRK status

#if defined(HAVE_SHARK) && defined(HAVE_SHARK_FFP)
// Internal fixed function pipeline dirty flags
extern GLboolean ffp_dirty_frag;
extern GLboolean ffp_dirty_vert;
extern GLboolean ffp_dirty_vert_stream;

// Internal runtime shader compiler settings
extern int32_t compiler_fastmath;
extern int32_t compiler_fastprecision;
extern int32_t compiler_fastint;
extern shark_opt compiler_opts;
#endif

// sceGxm viewport setup (NOTE: origin is on center screen)
extern float x_port;
extern float y_port;
extern float z_port;
extern float x_scale;
extern float y_scale;
extern float z_scale;

// Fullscreen sceGxm viewport (NOTE: origin is on center screen)
extern float fullscreen_x_port;
extern float fullscreen_y_port;
extern float fullscreen_z_port;
extern float fullscreen_x_scale;
extern float fullscreen_y_scale;
extern float fullscreen_z_scale;

extern SceGxmContext *gxm_context; // sceGxm context instance
extern GLenum vgl_error; // Error returned by glGetError
extern SceGxmShaderPatcher *gxm_shader_patcher; // sceGxmShaderPatcher shader patcher instance
extern void *gxm_depth_surface_addr; // Depth surface memblock starting address
extern GLboolean system_app_mode; // Flag for system app mode usage
extern void *frame_purge_list[DISPLAY_MAX_BUFFER_COUNT][FRAME_PURGE_LIST_SIZE]; // Purge list for internal elements
extern int frame_purge_idx; // Index for currently populatable purge list
extern int frame_elem_purge_idx; // Index for currently populatable purge list element

// Macro to mark a pointer as dirty for garbage collection
#define markAsDirty(x) frame_purge_list[frame_purge_idx][frame_elem_purge_idx++] = x;

extern matrix4x4 mvp_matrix; // ModelViewProjection Matrix
extern matrix4x4 projection_matrix; // Projection Matrix
extern matrix4x4 modelview_matrix; // ModelView Matrix
extern matrix4x4 texture_matrix; // Texture Matrix
extern GLboolean mvp_modified; // Check if ModelViewProjection matrix needs to be recreated

extern GLuint cur_program; // Current in use custom program (0 = No custom program)
extern GLboolean vblank; // Current setting for VSync
extern uint32_t vertex_array_unit; // Current in-use vertex array buffer unit
extern gpubuffer gpu_buffers[BUFFERS_NUM]; // VBOs array

extern GLenum orig_depth_test; // Original depth test state (used for depth test invalidation)

// Scissor test shaders
extern SceGxmFragmentProgram *scissor_test_fragment_program; // Scissor test fragment program
extern vector4f *scissor_test_vertices; // Scissor test region vertices
extern SceUID scissor_test_vertices_uid; // Scissor test vertices memblock id

extern uint16_t *depth_clear_indices; // Memblock starting address for clear screen indices

// Clear screen shaders
extern SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
extern vector4f *clear_vertices; // Memblock starting address for clear screen vertices

extern GLboolean fast_texture_compression; // Hints for texture compression

/* gxm.c */
void initGxm(void); // Inits sceGxm
void initGxmContext(void); // Inits sceGxm context
void termGxmContext(void); // Terms sceGxm context
void createDisplayRenderTarget(void); // Creates render target for the display
void destroyDisplayRenderTarget(void); // Destroys render target for the display
void initDisplayColorSurfaces(void); // Creates color surfaces for the display
void termDisplayColorSurfaces(void); // Destroys color surfaces for the display
void initDepthStencilBuffer(uint32_t w, uint32_t h, SceGxmDepthStencilSurface *surface, void **depth_buffer, void **stencil_buffer); // Creates depth and stencil surfaces
void initDepthStencilSurfaces(void); // Creates depth and stencil surfaces for the display
void termDepthStencilSurfaces(void); // Destroys depth and stencil surfaces for the display
void startShaderPatcher(void); // Creates a shader patcher instance
void stopShaderPatcher(void); // Destroys a shader patcher instance
void waitRenderingDone(void); // Waits for rendering to be finished

/* tests.c */
void change_depth_write(SceGxmDepthWriteMode mode); // Changes current in use depth write mode
void change_depth_func(void); // Changes current in use depth test function
void invalidate_depth_test(void); // Invalidates depth test state
void validate_depth_test(void); // Resets original depth test state after invalidation
void change_stencil_settings(void); // Changes current in use stencil test parameters
GLboolean change_stencil_config(SceGxmStencilOp *cfg, GLenum new); // Changes current in use stencil test operation value
GLboolean change_stencil_func_config(SceGxmStencilFunc *cfg, GLenum new); // Changes current in use stencil test function value
void update_alpha_test_settings(void); // Changes current in use alpha test operation value
void update_scissor_test(void); // Changes current in use scissor test region
void resetScissorTestRegion(void); // Resets scissor test region to default values
void invalidate_viewport(void); // Invalidates currently set viewport
void validate_viewport(void); // Restores previously invalidated viewport

/* blending.c (TODO) */
void change_blend_factor(void); // Changes current blending settings for all used shaders
void change_blend_mask(void); // Changes color mask when blending is disabled for all used shaders
void rebuild_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog, const SceGxmProgram *vert); // Creates a new patched fragment program with proper blend settings
void update_precompiled_ffp_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog, blend_config *cfg); // Updated current in use fragment program for precompiled ffp implementation

/* custom_shaders.c */
void resetCustomShaders(void); // Resets custom shaders
void _vglDrawObjects_CustomShadersIMPL(GLboolean implicit_wvp); // vglDrawObjects implementation for rendering with custom shaders
void _glDrawElements_CustomShadersIMPL(uint16_t *idx_buf, GLsizei count); // glDrawElements implementation for rendering with custom shaders
void _glDrawArrays_CustomShadersIMPL(GLsizei count); // glDrawArrays implementation for rendering with custom shaders

/* misc functions */
void vector4f_convert_to_local_space(vector4f *out, int x, int y, int width, int height); // Converts screen coords to local space

#endif
