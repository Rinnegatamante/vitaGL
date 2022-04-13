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
#define TEXTURE_IMAGE_UNITS_NUM 16 // Available texture image units
#ifdef HAVE_HIGH_FFP_TEXUNITS
#define TEXTURE_COORDS_NUM 3 // Available texture coords sets for multitexturing with ffp
#else
#define TEXTURE_COORDS_NUM 2 // Available texture coords sets for multitexturing with ffp
#endif
#define COMBINED_TEXTURE_IMAGE_UNITS_NUM 16 // Available combined texture image units
#define VERTEX_ATTRIBS_NUM 16 // Available vertex attributes
#define MODELVIEW_STACK_DEPTH 32 // Depth of modelview matrix stack
#define GENERIC_STACK_DEPTH 2 // Depth of generic matrix stack
#define DISPLAY_WIDTH_DEF 960 // Default display width in pixels
#define DISPLAY_HEIGHT_DEF 544 // Default display height in pixels
#define DISPLAY_MAX_BUFFER_COUNT 3 // Maximum amount of display buffers to use
#define GXM_TEX_MAX_SIZE 4096 // Maximum width/height in pixels per texture
#define FRAME_PURGE_LIST_SIZE 16384 // Number of elements a single frame can hold
#define FRAME_PURGE_RENDERTARGETS_LIST_SIZE 128 // Number of rendertargets a single frame can hold
#define FRAME_PURGE_FREQ 5 // Frequency in frames for garbage collection
#define BUFFERS_NUM 256 // Maximum amount of framebuffers objects usable
#ifdef HAVE_HIGH_FFP_TEXUNITS
#define FFP_VERTEX_ATTRIBS_NUM 9 // Number of attributes used in ffp shaders
#else
#define FFP_VERTEX_ATTRIBS_NUM 8 // Number of attributes used in ffp shaders
#endif
#define MEM_ALIGNMENT 16 // Memory alignment
#define MAX_CLIP_PLANES_NUM 7 // Maximum number of allowed user defined clip planes for ffp
#define LEGACY_VERTEX_STRIDE 24 // Vertex stride for GL1 immediate draw pipeline
#define LEGACY_MT_VERTEX_STRIDE 26 // Vertex stride for GL1 immediate draw pipeline with multitexturing
#define LEGACY_NT_VERTEX_STRIDE 22 // Vertex stride for GL1 immediate draw pipeline without texturing
#define MAX_LIGHTS_NUM 8 // Maximum number of allowed light sources for ffp

// Internal constants set in bootup phase
extern int DISPLAY_WIDTH; // Display width in pixels
extern int DISPLAY_HEIGHT; // Display height in pixels
extern int DISPLAY_STRIDE; // Display stride in pixels
extern float DISPLAY_WIDTH_FLOAT; // Display width in pixels (float)
extern float DISPLAY_HEIGHT_FLOAT; // Display height in pixels (float)

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/appmgr.h>
#include <psp2/common_dialog.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/dmac.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/razor_capture.h>
#include <psp2/razor_hud.h>
#include <psp2/rtc.h>
#include <psp2/sharedfb.h>
#include <psp2/sysmodule.h>

#include "vitaGL.h"

#include "utils/atitc_utils.h"
#include "utils/gpu_utils.h"
#include "utils/gxm_utils.h"
#include "utils/math_utils.h"
#include "utils/mem_utils.h"

#include "texture_callbacks.h"

// Debug flags
//#define DEBUG_MEMCPY // Enable this to use newlib memcpy in order to have proper trace info in coredumps

// Debugging tool
char *get_gxm_error_literal(uint32_t code);
#ifdef FILE_LOG
void vgl_file_log(const char *format, ...);
#define vgl_log vgl_file_log
#elif defined(LOG_ERRORS)
#define vgl_log sceClibPrintf
#else
#define vgl_log(...)
#endif
#ifdef DEBUG_MEMCPY
#define vgl_fast_memcpy memcpy
#else
#define vgl_fast_memcpy sceClibMemcpy
#endif

extern GLboolean prim_is_non_native; // Flag for when a primitive not supported natively by sceGxm is used

// Translates a GL primitive enum to its sceGxm equivalent
#ifndef SKIP_ERROR_HANDLING
#define gl_primitive_to_gxm(x, p, c) \
	if (c <= 0) \
		return; \
	prim_is_non_native = GL_FALSE; \
	switch (x) { \
	case GL_POINTS: \
		p = SCE_GXM_PRIMITIVE_POINTS; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_POINT_01UV); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_POINT_01UV); \
		break; \
	case GL_LINES: \
		if (c % 2) \
			return; \
		p = SCE_GXM_PRIMITIVE_LINES; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		break; \
	case GL_LINE_STRIP: \
		if (c < 2) \
			return; \
		p = SCE_GXM_PRIMITIVE_LINES; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		prim_is_non_native = GL_TRUE; \
		break; \
	case GL_LINE_LOOP: \
		if (c < 2) \
			return; \
		p = SCE_GXM_PRIMITIVE_LINES; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		prim_is_non_native = GL_TRUE; \
		break; \
	case GL_TRIANGLES: \
		if (c % 3 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		break; \
	case GL_TRIANGLE_STRIP: \
		if (c < 3 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP; \
		break; \
	case GL_TRIANGLE_FAN: \
		if (c < 3 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN; \
		break; \
	case GL_QUADS: \
		if (c % 4 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		prim_is_non_native = GL_TRUE; \
		break; \
	default: \
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, x) \
	}
#else
#define gl_primitive_to_gxm(x, p, c) \
	prim_is_non_native = GL_FALSE; \
	switch (x) { \
	case GL_POINTS: \
		p = SCE_GXM_PRIMITIVE_POINTS; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_POINT_01UV); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_POINT_01UV); \
		break; \
	case GL_LINES: \
		p = SCE_GXM_PRIMITIVE_LINES; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		break; \
	case GL_LINE_STRIP: \
		p = SCE_GXM_PRIMITIVE_LINES; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		prim_is_non_native = GL_TRUE; \
		break; \
	case GL_LINE_LOOP: \
		p = SCE_GXM_PRIMITIVE_LINES; \
		sceGxmSetFrontPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		sceGxmSetBackPolygonMode(gxm_context, SCE_GXM_POLYGON_MODE_LINE); \
		prim_is_non_native = GL_TRUE; \
		break; \
	case GL_TRIANGLES: \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		break; \
	case GL_TRIANGLE_STRIP: \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP; \
		break; \
	case GL_TRIANGLE_FAN: \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN; \
		break; \
	case GL_QUADS: \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		prim_is_non_native = GL_TRUE; \
		break; \
	default: \
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, x) \
	}
#endif

// Restore Polygon mode after a draw call
#define restore_polygon_mode(p) \
	if (p == SCE_GXM_PRIMITIVE_LINES || p == SCE_GXM_PRIMITIVE_POINTS) { \
		sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front); \
		sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back); \
	}

// Error set funcs
#define SET_GL_ERROR(x) \
	vgl_log("%s:%d: %s set %s\n", __FILE__, __LINE__, __func__, #x); \
	vgl_error = x; \
	return;
#define SET_GL_ERROR_WITH_RET(x, y) \
	vgl_log("%s:%d: %s set %s\n", __FILE__, __LINE__, __func__, #x); \
	vgl_error = x; \
	return y;
#define SET_GL_ERROR_WITH_VALUE(x, y) \
	vgl_log("%s:%d: %s set %s (%s: 0x%X)\n", __FILE__, __LINE__, __func__, #x, #y, y); \
	vgl_error = x; \
	return;

#ifdef LOG_ERRORS
#define patchVertexProgram(patcher, id, attr, attr_num, stream, stream_num, prog) \
	int __v = sceGxmShaderPatcherCreateVertexProgram(patcher, id, attr, attr_num, stream, stream_num, prog); \
	if (__v) \
		vgl_log("Vertex shader patching failed (%s) on shader %d with %d attributes and %d streams.\n", get_gxm_error_literal(__v), id, attr_num, stream_num);
#define patchFragmentProgram(patcher, id, fmt, msaa_mode, blend_cfg, vertex_link, prog) \
	int __f = sceGxmShaderPatcherCreateFragmentProgram(patcher, id, fmt, msaa_mode, blend_cfg, vertex_link, prog); \
	if (__f) \
		vgl_log("Fragment shader patching failed (%s) on shader %d.\n", get_gxm_error_literal(__f), id);
#else
#define patchVertexProgram sceGxmShaderPatcherCreateVertexProgram
#define patchFragmentProgram sceGxmShaderPatcherCreateFragmentProgram
#endif

#ifdef HAVE_SOFTFP_ABI
extern __attribute__((naked)) void sceGxmSetViewport_sfp(SceGxmContext *context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale);
#define setViewport sceGxmSetViewport_sfp
#else
#define setViewport sceGxmSetViewport
#endif

// Drawing phases constants for legacy openGL
typedef enum {
	NONE,
	MODEL_CREATION
} glPhase;

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

typedef union combinerState {
	struct {
		uint32_t rgb_func : 3;
		uint32_t a_func : 3;
		uint32_t op_mode_rgb_0 : 2;
		uint32_t op_mode_a_0 : 2;
		uint32_t op_rgb_0 : 2;
		uint32_t op_a_0 : 2; // This can be ideally reduced to 1 bit if necessary
		uint32_t op_mode_rgb_1 : 2;
		uint32_t op_mode_a_1 : 2;
		uint32_t op_rgb_1 : 2;
		uint32_t op_a_1 : 2; // This can be ideally reduced to 1 bit if necessary
		uint32_t op_mode_rgb_2 : 2;
		uint32_t op_mode_a_2 : 2;
		uint32_t op_rgb_2 : 2;
		uint32_t op_a_2 : 2; // This can be ideally reduced to 1 bit if necessary
		uint32_t UNUSED : 2;
	};
	uint32_t raw;
} combinerState;

// Texture unit struct
typedef struct {
	GLboolean enabled;
	matrix4x4 texture_matrix_stack[GENERIC_STACK_DEPTH];
	uint8_t texture_stack_counter;
	int env_mode;
	combinerState combiner;
	vector4f env_color;
	float rgb_scale;
	float a_scale;
	GLuint tex_id;
} texture_unit;

// Framebuffer struct
typedef struct {
	GLboolean active;
	SceGxmRenderTarget *target;
	SceGxmColorSurface colorbuffer;
	SceGxmDepthStencilSurface depthbuffer;
	SceGxmDepthStencilSurface *depthbuffer_ptr;
	void *depth_buffer_addr;
	void *stencil_buffer_addr;
	int width;
	int height;
	int stride;
	void *data;
	uint32_t data_type;
	texture *tex;
} framebuffer;

// Renderbuffer struct
typedef struct {
	GLboolean active;
	SceGxmDepthStencilSurface depthbuffer;
	SceGxmDepthStencilSurface *depthbuffer_ptr;
	void *depth_buffer_addr;
	void *stencil_buffer_addr;
} renderbuffer;

// Texture environment mode
typedef enum {
	MODULATE,
	DECAL,
	BLEND,
	ADD,
	REPLACE,
	SUBTRACT,
	COMBINE,
	ADD_SIGNED = 1,
	INTERPOLATE = 2,
} texEnvMode;

#ifndef DISABLE_TEXTURE_COMBINER
typedef enum {
	TEXTURE,
	CONSTANT,
	PRIMARY_COLOR,
	PREVIOUS
} texEnvOp;

typedef enum {
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA
} texEnvOpMode;
#endif

// VBO struct
typedef struct {
	void *ptr;
	int32_t size;
	vglMemType type;
	GLboolean used;
	GLboolean mapped;
} gpubuffer;

// 3D vertex for position + 4D vertex for RGBA color struct
typedef struct {
	vector3f position;
	vector4f color;
} rgba_vertex;

// 3D vertex for position + 3D vertex for RGB color struct
typedef struct {
	vector3f position;
	vector3f color;
} rgb_vertex;

// 3D vertex for position + 2D vertex for UV map struct
typedef struct {
	vector3f position;
	vector2f texcoord;
} texture2d_vertex;

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
extern SceGxmVertexAttribute vertex_attrib_config[VERTEX_ATTRIBS_NUM];
extern GLboolean is_rendering_display; // Flag for when we're rendering without a framebuffer object
extern uint16_t *default_idx_ptr; // sceGxm mapped progressive indices buffer
extern uint16_t *default_quads_idx_ptr; // sceGxm mapped progressive indices buffer for quads
extern uint16_t *default_line_strips_idx_ptr; // sceGxm mapped progressive indices buffer for line strips

extern int legacy_pool_size; // Mempool size for GL1 immediate draw pipeline
extern float *legacy_pool; // Mempool for GL1 immediate draw pipeline
extern float *legacy_pool_ptr; // Current address for vertices population for GL1 immediate draw pipeline
#ifndef SKIP_ERROR_HANDLING
extern float *legacy_pool_end; // Address of the end of the GL1 immediate draw pipeline vertex pool
extern uint32_t vgl_debugger_framecount; // Current frame number since application started
#endif
extern SceGxmVertexAttribute legacy_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM - 1];
extern SceGxmVertexStream legacy_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM - 1];
extern SceGxmVertexAttribute legacy_mt_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
extern SceGxmVertexStream legacy_mt_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];
extern SceGxmVertexAttribute legacy_nt_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM - 2];
extern SceGxmVertexStream legacy_nt_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM - 2];
extern SceGxmVertexAttribute ffp_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
extern SceGxmVertexStream ffp_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];

// Logging callback for vitaShaRK
#ifdef HAVE_SHARK_LOG
void shark_log_cb(const char *msg, shark_log_level msg_level, int line);
#endif

// Depending on SDK, these could be or not defined
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// sceRazor debugger related stuffs
#ifdef HAVE_RAZOR
#define RAZOR_MAX_SCENES_NUM 32

#ifndef HAVE_DEVKIT
extern SceUID razor_modid;
#endif

typedef struct {
	uint32_t vertexDuration;
	uint32_t fragmentDuration;
} scene_metrics;

typedef struct {
	uint32_t vertexJobCount;
	uint64_t vertexJobTime;
	uint32_t fragmentJobCount;
	uint64_t fragmentJobTime;
	uint32_t firmwareJobCount;
	uint64_t firmwareJobTime;
	float usseVertexProcessing;
	float usseFragmentProcessing;
	float usseDependentTextureReadRequest;
	float usseNonDependentTextureReadRequest;
	uint32_t vdmPrimitivesInput;
	uint32_t mtePrimitivesOutput;
	uint32_t vdmVerticesInput;
	uint32_t mteVerticesOutput;
	uint32_t rasterizedPixelsBeforeHsr;
	uint32_t rasterizedOutputPixels;
	uint32_t rasterizedOutputSamples;
	uint32_t bifTaMemoryWrite;
	uint32_t bifIspParameterFetchMemoryRead;
	uint32_t peakUsage;
	uint8_t partialRender;
	uint8_t vertexJobPaused;
	uint64_t frameStartTime;
	uint32_t frameDuration;
	uint32_t frameNumber;
	uint32_t frameGpuActive;
	uint32_t sceneCount;
	scene_metrics scenes[RAZOR_MAX_SCENES_NUM];
} razor_results;

extern uint32_t frame_idx; // Current frame number
extern GLboolean has_razor_live; // Flag for live metrics support with sceRazor
#endif

extern GLboolean use_shark; // Flag to check if vitaShaRK should be initialized at vitaGL boot
extern GLboolean is_shark_online; // Current vitaShaRK status

extern GLboolean dirty_frag_unifs;
extern GLboolean dirty_vert_unifs;

// Internal fixed function pipeline dirty flags and variables
extern GLboolean ffp_dirty_frag;
extern GLboolean ffp_dirty_vert;
#ifdef HAVE_HIGH_FFP_TEXUNITS
extern uint16_t ffp_vertex_attrib_state;
#else
extern uint8_t ffp_vertex_attrib_state;
#endif
extern uint8_t ffp_vertex_num_params;

// Internal runtime shader compiler settings
extern int32_t compiler_fastmath;
extern int32_t compiler_fastprecision;
extern int32_t compiler_fastint;
extern shark_opt compiler_opts;

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
extern void *frame_purge_list[FRAME_PURGE_FREQ][FRAME_PURGE_LIST_SIZE]; // Purge list for internal elements
extern void *frame_rt_purge_list[FRAME_PURGE_FREQ][FRAME_PURGE_RENDERTARGETS_LIST_SIZE]; // Purge list for rendertargets
extern int frame_purge_idx; // Index for currently populatable purge list
extern int frame_elem_purge_idx; // Index for currently populatable purge list element
extern int frame_rt_purge_idx; // Index for currently populatable purge list rendertarget
extern GLboolean use_vram; // Flag for VRAM usage for allocations

// Macro to mark a pointer or a rendertarget as dirty for garbage collection
#define markAsDirty(x) frame_purge_list[frame_purge_idx][frame_elem_purge_idx++] = x
#ifdef HAVE_SHARED_RENDERTARGETS
typedef struct {
	SceGxmRenderTarget *rt;
	int w;
	int h;
	int ref_count;
	int max_refs;
} render_target;
void __markRtAsDirty(render_target *rt);
#define _markRtAsDirty(x) frame_rt_purge_list[frame_purge_idx][frame_rt_purge_idx++] = x
#define markRtAsDirty(x) __markRtAsDirty((render_target *)x)
#else
#define markRtAsDirty(x) frame_rt_purge_list[frame_purge_idx][frame_rt_purge_idx++] = x
#endif

// Blending
extern GLboolean blend_state; // Current state for GL_BLEND
extern SceGxmBlendFactor blend_sfactor_rgb; // Current in use RGB source blend factor
extern SceGxmBlendFactor blend_dfactor_rgb; // Current in use RGB dest blend factor
extern SceGxmBlendFactor blend_sfactor_a; // Current in use A source blend factor
extern SceGxmBlendFactor blend_dfactor_a; // Current in use A dest blend factor
extern SceGxmColorMask blend_color_mask; // Current in-use color mask (glColorMask)
extern SceGxmBlendFunc blend_func_rgb; // Current in-use RGB blend func
extern SceGxmBlendFunc blend_func_a; // Current in-use A blend func

// Depth Test
extern GLboolean depth_test_state; // Current state for GL_DEPTH_TEST
extern SceGxmDepthFunc depth_func; // Current in-use depth test func
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
extern texture_unit texture_units[COMBINED_TEXTURE_IMAGE_UNITS_NUM]; // Available texture units
extern texture texture_slots[TEXTURES_NUM]; // Available texture slots
extern int8_t server_texture_unit; // Current in use server side texture unit
extern int8_t client_texture_unit; // Current in use client side texture unit
extern void *color_table; // Current in-use color table

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
extern GLboolean normalize;
extern GLboolean color_material_state;

// Fogging
extern GLboolean fogging; // Current fogging processor state
extern GLint fog_mode; // Current fogging mode (openGL)
extern fogType internal_fog_mode; // Current fogging mode (sceGxm)
extern GLfloat fog_density; // Current fogging density
extern GLfloat fog_near; // Current fogging near distance
extern GLfloat fog_far; // Current fogging far distance
extern GLfloat fog_range; // Current fogging range (fog far - fog near)
extern vector4f fog_color; // Current fogging color

// Clipping Planes
extern GLboolean clip_planes_aligned; // Are clip planes in a contiguous range
extern uint8_t clip_plane_range[2]; // The highest and lowest enabled clip planes
extern uint8_t clip_planes_mask; // Bitmask of enabled clip planes
extern vector4f clip_planes_eq[MAX_CLIP_PLANES_NUM]; // Current equation for user clip planes

// Framebuffers
extern framebuffer *active_read_fb; // Current readback framebuffer in use
extern framebuffer *active_write_fb; // Current write framebuffer in use

// vgl* Draw Pipeline
extern void *vertex_object;
extern void *color_object;
extern void *texture_object;
extern void *index_object;

extern matrix4x4 mvp_matrix; // ModelViewProjection Matrix
extern matrix4x4 projection_matrix; // Projection Matrix
extern matrix4x4 modelview_matrix; // ModelView Matrix
extern matrix4x4 texture_matrix; // Texture Matrix
extern matrix4x4 normal_matrix; // Normal Matrix
extern GLboolean mvp_modified; // Check if ModelViewProjection matrix needs to be recreated

extern GLuint cur_program; // Current in use custom program (0 = No custom program)
extern uint32_t vsync_interval; // Current setting for VSync

extern uint32_t vertex_array_unit; // Current in-use vertex array buffer unit
extern uint32_t index_array_unit; // Current in-use element array buffer unit

extern GLenum orig_depth_test; // Original depth test state (used for depth test invalidation)
extern framebuffer *in_use_framebuffer; // Currently in use framebuffer

// Scissor test shaders
extern SceGxmFragmentProgram *scissor_test_fragment_program; // Scissor test fragment program
extern vector4f *scissor_test_vertices; // Scissor test region vertices
extern SceUID scissor_test_vertices_uid; // Scissor test vertices memblock id
extern GLboolean skip_scene_reset;

extern uint16_t *depth_clear_indices; // Memblock starting address for clear screen indices

// Clear screen shaders
extern SceGxmVertexProgram *clear_vertex_program_patched; // Patched vertex program for clearing screen
extern vector4f *clear_vertices; // Memblock starting address for clear screen vertices

extern GLboolean fast_texture_compression; // Hints for texture compression
extern GLboolean recompress_atitc;
extern GLfloat point_size; // Size of points for fixed function pipeline

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
void sceneReset(void); // Resets drawing scene if required
GLboolean startShaderCompiler(void); // Starts a shader compiler instance

/* tests.c */
void change_depth_write(SceGxmDepthWriteMode mode); // Changes current in use depth write mode
void change_depth_func(void); // Changes current in use depth test function
void invalidate_depth_test(void); // Invalidates depth test state
void validate_depth_test(void); // Resets original depth test state after invalidation
void change_stencil_settings(void); // Changes current in use stencil test parameters
GLboolean change_stencil_config(SceGxmStencilOp *cfg, GLenum new_cfg); // Changes current in use stencil test operation value
GLboolean change_stencil_func_config(SceGxmStencilFunc *cfg, GLenum new_cfg); // Changes current in use stencil test function value
void update_alpha_test_settings(void); // Changes current in use alpha test operation value
void update_scissor_test(void); // Changes current in use scissor test region
void resetScissorTestRegion(void); // Resets scissor test region to default values
void invalidate_viewport(void); // Invalidates currently set viewport
void validate_viewport(void); // Restores previously invalidated viewport

/* blending.c (TODO) */
void change_blend_factor(void); // Changes current blending settings for all used shaders
void change_blend_mask(void); // Changes color mask when blending is disabled for all used shaders
void rebuild_frag_shader(SceGxmShaderPatcherId pid, SceGxmFragmentProgram **prog, SceGxmProgram *vprog); // Creates a new patched fragment program with proper blend settings

/* custom_shaders.c */
void resetCustomShaders(void); // Resets custom shaders
void _vglDrawObjects_CustomShadersIMPL(GLboolean implicit_wvp); // vglDrawObjects implementation for rendering with custom shaders
GLboolean _glDrawElements_CustomShadersIMPL(uint16_t *idx_buf, GLsizei count, GLboolean is_short); // glDrawElements implementation for rendering with custom shaders
GLboolean _glDrawArrays_CustomShadersIMPL(GLsizei count); // glDrawArrays implementation for rendering with custom shaders

/* ffp.c */
void _glDrawElements_FixedFunctionIMPL(uint16_t *idx_buf, GLsizei count, GLboolean is_short); // glDrawElements implementation for rendering with ffp
void _glDrawArrays_FixedFunctionIMPL(GLsizei count); // glDrawArrays implementation for rendering with ffp
uint8_t reload_ffp_shaders(SceGxmVertexAttribute *attrs, SceGxmVertexStream *streams); // Reloads current in use ffp shaders
void upload_ffp_uniforms(); // Uploads required uniforms for the in use ffp shaders
void update_fogging_state(); // Updates current setup for fogging

/* misc.c */
void change_cull_mode(void); // Updates current cull mode

/* misc functions */
void vector4f_convert_to_local_space(vector4f *out, int x, int y, int width, int height); // Converts screen coords to local space

/* debug.cpp */
void vgl_debugger_init(); // Inits ImGui debugger context
void vgl_debugger_draw(); // Draws ImGui debugger window
void vgl_debugger_light_draw(uint32_t *fb); // Draws CPU rendered debugger window

/* vitaGL.c */
uint8_t *reserve_data_pool(uint32_t size);

#endif
