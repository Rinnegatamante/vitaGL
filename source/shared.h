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
#define TEXTURES_NUM 4096 // Available textures per texture unit
#define MODELVIEW_STACK_DEPTH 32 // Depth of modelview matrix stack
#define GENERIC_STACK_DEPTH 2 // Depth of generic matrix stack
#define DISPLAY_WIDTH_DEF 960 // Default display width in pixels
#define DISPLAY_HEIGHT_DEF 544 // Default display height in pixels
#define DISPLAY_BUFFER_COUNT 2 // Display buffers to use
#define GXM_TEX_MAX_SIZE 4096 // Maximum width/height in pixels per texture
#define BUFFERS_ADDR 0xA000 // Starting address for buffers indexing
#define BUFFERS_NUM 128 // Maximum number of allocatable buffers

// Internal constants set in bootup phase
extern int DISPLAY_WIDTH; // Display width in pixels
extern int DISPLAY_HEIGHT; // Display height in pixels
extern int DISPLAY_STRIDE; // Display stride in pixels
extern float DISPLAY_WIDTH_FLOAT; // Display width in pixels (float)
extern float DISPLAY_HEIGHT_FLOAT; // Display height in pixels (float)

#include <stdio.h>
#include <stdlib.h>
#include <vitasdk.h>

#include "vitaGL.h"

#include "utils/gpu_utils.h"
#include "utils/math_utils.h"
#include "utils/mem_utils.h"

#include "state.h"
#include "texture_callbacks.h"

// Texture environment mode
typedef enum texEnvMode {
	MODULATE = 0,
	DECAL = 1,
	BLEND = 2,
	ADD = 3,
	REPLACE = 4
} texEnvMode;

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

// Disable color buffer shader
extern SceGxmShaderPatcherId disable_color_buffer_fragment_id;
extern const SceGxmProgramParameter *disable_color_buffer_position;
extern SceGxmFragmentProgram *disable_color_buffer_fragment_program_patched;
extern const SceGxmProgramParameter *clear_depth;

// Clear shader
extern SceGxmShaderPatcherId clear_vertex_id;
extern SceGxmShaderPatcherId clear_fragment_id;
extern const SceGxmProgramParameter *clear_position;
extern const SceGxmProgramParameter *clear_color;
extern SceGxmVertexProgram *clear_vertex_program_patched;
extern SceGxmFragmentProgram *clear_fragment_program_patched;

// Color (RGBA/RGB) shader
extern SceGxmShaderPatcherId rgba_vertex_id;
extern SceGxmShaderPatcherId rgb_vertex_id;
extern SceGxmShaderPatcherId rgba_fragment_id;
extern const SceGxmProgramParameter *rgba_position;
extern const SceGxmProgramParameter *rgba_color;
extern const SceGxmProgramParameter *rgba_wvp;
extern const SceGxmProgramParameter *rgb_position;
extern const SceGxmProgramParameter *rgb_color;
extern const SceGxmProgramParameter *rgb_wvp;
extern SceGxmVertexProgram *rgba_vertex_program_patched;
extern SceGxmVertexProgram *rgba_u8n_vertex_program_patched;
extern SceGxmVertexProgram *rgb_vertex_program_patched;
extern SceGxmVertexProgram *rgb_u8n_vertex_program_patched;
extern SceGxmFragmentProgram *rgba_fragment_program_patched;
extern const SceGxmProgram *rgba_fragment_program;

// Texture2D shader
extern SceGxmShaderPatcherId texture2d_vertex_id;
extern SceGxmShaderPatcherId texture2d_fragment_id;
extern const SceGxmProgramParameter *texture2d_position;
extern const SceGxmProgramParameter *texture2d_texcoord;
extern const SceGxmProgramParameter *texture2d_wvp;
extern const SceGxmProgramParameter *texture2d_alpha_cut;
extern const SceGxmProgramParameter *texture2d_alpha_op;
extern const SceGxmProgramParameter *texture2d_tint_color;
extern const SceGxmProgramParameter *texture2d_tex_env;
extern const SceGxmProgramParameter *texture2d_clip_plane0;
extern const SceGxmProgramParameter *texture2d_clip_plane0_eq;
extern const SceGxmProgramParameter *texture2d_mv;
extern const SceGxmProgramParameter *texture2d_fog_mode;
extern const SceGxmProgramParameter *texture2d_fog_near;
extern const SceGxmProgramParameter *texture2d_fog_far;
extern const SceGxmProgramParameter *texture2d_fog_density;
extern const SceGxmProgramParameter *texture2d_fog_color;
extern const SceGxmProgramParameter *texture2d_tex_env_color;
extern SceGxmVertexProgram *texture2d_vertex_program_patched;
extern SceGxmFragmentProgram *texture2d_fragment_program_patched;
extern const SceGxmProgram *texture2d_fragment_program;

// Texture2D+RGBA shader
extern SceGxmShaderPatcherId texture2d_rgba_vertex_id;
extern SceGxmShaderPatcherId texture2d_rgba_fragment_id;
extern const SceGxmProgramParameter *texture2d_rgba_position;
extern const SceGxmProgramParameter *texture2d_rgba_texcoord;
extern const SceGxmProgramParameter *texture2d_rgba_wvp;
extern const SceGxmProgramParameter *texture2d_rgba_alpha_cut;
extern const SceGxmProgramParameter *texture2d_rgba_alpha_op;
extern const SceGxmProgramParameter *texture2d_rgba_color;
extern const SceGxmProgramParameter *texture2d_rgba_tex_env;
extern const SceGxmProgramParameter *texture2d_rgba_clip_plane0;
extern const SceGxmProgramParameter *texture2d_rgba_clip_plane0_eq;
extern const SceGxmProgramParameter *texture2d_rgba_mv;
extern const SceGxmProgramParameter *texture2d_rgba_fog_mode;
extern const SceGxmProgramParameter *texture2d_rgba_fog_near;
extern const SceGxmProgramParameter *texture2d_rgba_fog_far;
extern const SceGxmProgramParameter *texture2d_rgba_fog_density;
extern const SceGxmProgramParameter *texture2d_rgba_fog_color;
extern const SceGxmProgramParameter *texture2d_rgba_tex_env_color;
extern SceGxmVertexProgram *texture2d_rgba_vertex_program_patched;
extern SceGxmVertexProgram *texture2d_rgba_u8n_vertex_program_patched;
extern SceGxmFragmentProgram *texture2d_rgba_fragment_program_patched;
extern const SceGxmProgram *texture2d_rgba_fragment_program;
// Internal stuffs
extern void *frag_uniforms;
extern void *vert_uniforms;
extern SceGxmMultisampleMode msaa_mode;
extern GLboolean use_extra_mem;

// Debugging tool
#ifdef ENABLE_LOG
void LOG(const char *format, ...);
#endif

// Depending on SDK, that could be or not defined
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
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

extern matrix4x4 mvp_matrix; // ModelViewProjection Matrix
extern matrix4x4 projection_matrix; // Projection Matrix
extern matrix4x4 modelview_matrix; // ModelView Matrix
extern GLboolean mvp_modified; // Check if ModelViewProjection matrix needs to be recreated

extern GLuint cur_program; // Current in use custom program (0 = No custom program)
extern GLboolean vblank; // Current setting for VSync

extern GLenum orig_depth_test; // Original depth test state (used for depth test invalidation)

// Scissor test shaders
extern SceGxmFragmentProgram *scissor_test_fragment_program; // Scissor test fragment program
extern vector4f *scissor_test_vertices; // Scissor test region vertices
extern SceUID scissor_test_vertices_uid; // Scissor test vertices memblock id

extern uint16_t *depth_clear_indices; // Memblock starting address for clear screen indices

// Clear screen shaders
extern SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
extern vector4f *clear_vertices; // Memblock starting address for clear screen vertices

/* gxm.c */
void initGxm(void); // Inits sceGxm
void initGxmContext(void); // Inits sceGxm context
void termGxmContext(void); // Terms sceGxm context
void createDisplayRenderTarget(void); // Creates render target for the display
void destroyDisplayRenderTarget(void); // Destroys render target for the display
void initDisplayColorSurfaces(void); // Creates color surfaces for the display
void termDisplayColorSurfaces(void); // Destroys color surfaces for the display
void initDepthStencilBuffer(uint32_t w, uint32_t h, SceGxmDepthStencilSurface *surface, void **depth_buffer, void **stencil_buffer, vglMemType *depth_type, vglMemType *stencil_type); // Creates depth and stencil surfaces
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

/* blending.c */
void change_blend_factor(void); // Changes current blending settings for all used shaders
void disable_blend(void); // Disables blending for all used shaders

/* custom_shaders.c */
void resetCustomShaders(void); // Resets custom shaders
void changeCustomShadersBlend(SceGxmBlendInfo *blend_info); // Change SceGxmBlendInfo value to all custom shaders
void reloadCustomShader(void); // Reloads in use custom shader inside sceGxm
void _vglDrawObjects_CustomShadersIMPL(GLenum mode, GLsizei count, GLboolean implicit_wvp); // vglDrawObjects implementation for rendering with custom shaders

/* misc functions */
void vector4f_convert_to_local_space(vector4f *out, int x, int y, int width, int height); // Converts screen coords to local space

#endif
