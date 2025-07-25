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
#include <vitasdk.h>
#include "vitaGL.h"

// Undocumented texture format for ETC1 textures (Thanks to Bythos)
#define SCE_GXM_TEXTURE_FORMAT_ETC1_RGB 0x84000000

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
#define UBOS_NUM 14 // Available uniform buffers bindings
#define MODELVIEW_STACK_DEPTH 32 // Depth of modelview matrix stack
#define GENERIC_STACK_DEPTH 2 // Depth of generic matrix stack
#define DISPLAY_WIDTH_DEF 960 // Default display width in pixels
#define DISPLAY_HEIGHT_DEF 544 // Default display height in pixels
#define DISPLAY_MAX_BUFFER_COUNT 3 // Maximum amount of display buffers to use
#define GXM_TEX_MAX_SIZE 4096 // Maximum width/height in pixels per texture
#define FRAME_PURGE_LIST_SIZE 16384 // Number of elements a single frame can hold
#define FRAME_PURGE_RENDERTARGETS_LIST_SIZE 128 // Number of rendertargets a single frame can hold
#define FRAME_PURGE_FREQ 4 // Frequency in frames for garbage collection
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
#define MAX_IDX_NUMBER 0xC000 // Maximum allowed number of indices per draw call for glDrawArrays

#define OBJ_NOT_USED 0xFFFFFFFF // Flag for not yet used objects
#define OBJ_CACHED 0xFFFFFFFE // Flag for file cached objects

#include "utils/mem_utils.h"

#ifdef SAFER_DRAW_SPEEDHACK
#define SAFE_DRAW_SIZE_THRESHOLD (0x8000) // Minimum bytes of vertices data for a draw to be handled with speedhack
#endif

#ifdef HAVE_FAILSAFE_CIRCULAR_VERTEX_POOL
#define CIRCULAR_VERTEX_POOLS_NUM 3
extern uint8_t *vertex_data_pool[CIRCULAR_VERTEX_POOLS_NUM];
extern uint8_t *vertex_data_pool_ptr[CIRCULAR_VERTEX_POOLS_NUM];
extern int vgl_circular_idx;
#endif
#if defined(HAVE_SCRATCH_MEMORY) && defined(HAVE_CIRCULAR_VERTEX_POOL)
extern GLboolean vgl_dynamic_wants_scratch;
extern GLboolean vgl_stream_wants_scratch;
#endif

// Texture object status enum
enum {
	TEX_UNUSED,
	TEX_UNINITIALIZED,
	TEX_VALID
};

// Texture object struct
typedef struct texture {
#ifndef TEXTURES_SPEEDHACK
	uint32_t last_frame;
#endif
#ifdef HAVE_TEX_CACHE
	uint32_t upload_frame;
	uint64_t hash;
	struct texture *next;
	struct texture *prev;
#endif
	uint8_t status;
	uint8_t mip_count;
	uint8_t ref_counter;
	uint8_t faces_counter;
	GLboolean use_mips;
	GLboolean dirty;
	GLboolean overridden;
	SceGxmTexture gxm_tex;
	void *data;
	void *palette_data;
	uint32_t type;
	void (*write_cb)(void *, uint32_t);
	SceGxmTextureFilter min_filter;
	SceGxmTextureFilter mag_filter;
	SceGxmTextureAddrMode u_mode;
	SceGxmTextureAddrMode v_mode;
	SceGxmTextureMipFilter mip_filter;
	uint32_t lod_bias;
#ifdef HAVE_UNPURE_TEXTURES
	int8_t mip_start;
#endif
} texture;

// Memory file cache settings
#ifdef HAVE_TEX_CACHE
extern char vgl_file_cache_path[256];
extern texture *vgl_uncached_tex_head;
extern texture *vgl_uncached_tex_tail;
extern uint32_t vgl_tex_cache_freq; // Number of frames prior a texture becomes cacheable if not used

#define markAsCacheable(tex) \
	tex->upload_frame = vgl_framecount; \
	tex->prev = vgl_uncached_tex_tail; \
	if (tex->prev) \
		tex->prev->next = tex; \
	else \
		vgl_uncached_tex_head = tex; \
	tex->next = NULL; \
	vgl_uncached_tex_tail = tex;

#define restoreTexCache(tex) \
	if (tex->last_frame == OBJ_CACHED) { \
		char fname[256], hash[24]; \
		sprintf(hash, "%llX", tex->hash); \
		sprintf(fname, "%s/%c%c/%s.raw", vgl_file_cache_path, hash[0], hash[1], hash); \
		SceUID f = sceIoOpen(fname, SCE_O_RDONLY, 0777); \
		size_t sz = sceIoLseek(f, 0, SCE_SEEK_END); \
		sceIoLseek(f, 0, SCE_SEEK_SET); \
		void *texture_data = gpu_alloc_mapped(sz, VGL_MEM_MAIN); \
		sceIoRead(f, texture_data, sz); \
		sceIoClose(f); \
		sceIoRemove(fname); \
		sceGxmTextureSetData(&tex->gxm_tex, texture_data); \
		tex->data = texture_data; \
		tex->last_frame = OBJ_NOT_USED; \
		markAsCacheable(tex) \
	}
#endif

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

#include "utils/atitc_utils.h"
#include "utils/eac_utils.h"
#include "utils/etc1_utils.h"
#include "utils/gpu_utils.h"
#include "utils/gxm_utils.h"
#include "utils/math_utils.h"
#include "utils/mem_utils.h"

#include "texture_callbacks.h"

// Fixed-function pipeline shader cache settings
#ifndef DISABLE_FS_SHADER_CACHE
#define SHADER_CACHE_MAGIC 26 // This must be increased whenever ffp shader sources or shader mask/combiner mask changes
//#define DUMP_SHADER_SOURCES // Enable this flag to dump shader sources inside shader cache
#endif

// Custom shaders pipeline shader cache settings
#ifdef HAVE_SHADER_CACHE
extern char vgl_shader_cache_path[256];
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
		if (c < 2) \
			return; \
		c -= c % 2; \
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
		if (c < 3 || no_polygons_mode) { \
			return; \
		} \
		c -= c % 3; \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		break; \
	case GL_TRIANGLE_STRIP: \
		if (c < 3 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP; \
		break; \
	case GL_POLYGON: \
	case GL_TRIANGLE_FAN: \
		if (c < 3 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN; \
		break; \
	case GL_QUADS: \
		if (c < 4 || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		c -= c % 4; \
		prim_is_non_native = GL_TRUE; \
		break; \
	case GL_QUAD_STRIP: \
		if ((c < 4) || (c % 2) || no_polygons_mode) \
			return; \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP; \
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
	case GL_POLYGON: \
	case GL_TRIANGLE_FAN: \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_FAN; \
		break; \
	case GL_QUADS: \
		p = SCE_GXM_PRIMITIVE_TRIANGLES; \
		prim_is_non_native = GL_TRUE; \
		break; \
	case GL_QUAD_STRIP: \
		p = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP; \
		break; \
	default: \
		SET_GL_ERROR_WITH_VALUE(GL_INVALID_ENUM, x) \
	}
#endif

// Restore Polygon mode after a draw call
#ifdef PRIMITIVES_SPEEDHACK
#define restore_polygon_mode(p)
#else
#define restore_polygon_mode(p) \
	if (p == SCE_GXM_PRIMITIVE_LINES || p == SCE_GXM_PRIMITIVE_POINTS) { \
		sceGxmSetFrontPolygonMode(gxm_context, polygon_mode_front); \
		sceGxmSetBackPolygonMode(gxm_context, polygon_mode_back); \
	}
#endif

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
#define SET_GL_ERROR_WITH_RET_AND_VALUE(x, y, z) \
	vgl_log("%s:%d: %s set %s (%s: 0x%X)\n", __FILE__, __LINE__, __func__, #x, #z, z); \
	vgl_error = x; \
	return y;

#ifdef LOG_ERRORS
#define patchVertexProgram(patcher, id, attr, attr_num, stream, stream_num, prog) \
	int __v = sceGxmShaderPatcherCreateVertexProgram(patcher, id, attr, attr_num, stream, stream_num, prog); \
	if (__v) \
		vgl_log("Vertex shader patching failed (%s) on shader 0x%X with %d attributes and %d streams.\n", get_gxm_error_literal(__v), id, attr_num, stream_num);
#define patchFragmentProgram(patcher, id, fmt, msaa_mode, blend_cfg, vertex_link, prog) \
	int __f = sceGxmShaderPatcherCreateFragmentProgram(patcher, id, fmt, msaa_mode, blend_cfg, vertex_link, prog); \
	if (__f) \
		vgl_log("Fragment shader patching failed (%s) on shader 0x%X.\n", get_gxm_error_literal(__f), id);
#else
#define patchVertexProgram sceGxmShaderPatcherCreateVertexProgram
#define patchFragmentProgram sceGxmShaderPatcherCreateFragmentProgram
#endif

#define rebuild_frag_shader(x, y, z, w) patchFragmentProgram(gxm_shader_patcher, x, w, msaa_mode, &blend_info.info, z, y) // Creates a new patched fragment program with proper blend settings

#ifdef HAVE_SOFTFP_ABI
extern __attribute__((naked)) void sceGxmSetViewport_sfp(SceGxmContext *context, float xOffset, float xScale, float yOffset, float yScale, float zOffset, float zScale);
#define setViewport sceGxmSetViewport_sfp
#else
#define setViewport sceGxmSetViewport
#endif

// Struct used for immediate mode vertices
typedef struct {
	vector2f uv;
	vector4f clr;
	vector4f amb;
	vector4f diff;
	vector4f spec;
	vector4f emiss;
	vector3f nor;
	vector2f uv2;
} legacy_vtx_attachment;

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
	int gl_x;
	int gl_y;
	int gl_w;
	int gl_h;
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
	uint8_t state;
	uint8_t texture_stack_counter;
	uint8_t env_mode;
	matrix4x4 texture_matrix_stack[GENERIC_STACK_DEPTH];
	combinerState combiner;
	vector4f env_color;
	float rgb_scale;
	float a_scale;
	GLuint tex_id[3]; // {2D, 1D, CUBE_MAP}
} texture_unit;

// Framebuffer struct
typedef struct {
	GLboolean active;
	GLboolean is_float;
	GLboolean is_depth_hidden;
	SceGxmRenderTarget *target;
	SceGxmColorSurface colorbuffer;
	SceGxmDepthStencilSurface depthbuffer;
	SceGxmDepthStencilSurface *depthbuffer_ptr;
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
} renderbuffer;

// Sampler object struct
typedef struct {
	SceGxmTextureFilter min_filter;
	SceGxmTextureFilter mag_filter;
	SceGxmTextureAddrMode u_mode;
	SceGxmTextureAddrMode v_mode;
	SceGxmTextureMipFilter mip_filter;
	GLboolean use_mips;
	uint32_t lod_bias;
} sampler;

// Texture environment mode
typedef enum {
	MODULATE = 0,
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
	uint32_t last_frame;
#if defined(HAVE_SCRATCH_MEMORY) && defined(HAVE_CIRCULAR_VERTEX_POOL)
	GLboolean scratch;
#endif
	GLboolean mapped;
} gpubuffer;

// VAO struct
typedef struct {
	uint32_t index_array_unit;
	uint8_t vertex_attrib_size[VERTEX_ATTRIBS_NUM];
	uint32_t vertex_attrib_offsets[VERTEX_ATTRIBS_NUM];
	uint32_t vertex_attrib_vbo[VERTEX_ATTRIBS_NUM];
	uint32_t vertex_attrib_state;
	float *vertex_attrib_value[VERTEX_ATTRIBS_NUM];
	SceGxmVertexAttribute vertex_attrib_config[VERTEX_ATTRIBS_NUM];
	SceGxmVertexStream vertex_stream_config[VERTEX_ATTRIBS_NUM];
	float *vertex_attrib_pool;
	float *vertex_attrib_pool_ptr;
	float *vertex_attrib_pool_limit;
} vao;

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

typedef enum {
	DLIST_ARG_VOID = 0x00,
	DLIST_ARG_U32 = 0x01,
	DLIST_ARG_I32 = 0x02,
	DLIST_ARG_F32 = 0x04,
	DLIST_ARG_I16 = 0x08,
	DLIST_ARG_U8 = 0x10
} dlistArgType;

typedef enum {
	// No arguments
	DLIST_FUNC_VOID = DLIST_ARG_VOID,
	// 1 argument
	DLIST_FUNC_U32 = DLIST_ARG_U32,
	// 2 arguments
	DLIST_FUNC_I32_I32 = DLIST_ARG_I32 | (DLIST_ARG_I32 << 8),
	DLIST_FUNC_U32_U32 = DLIST_ARG_U32 | (DLIST_ARG_U32 << 8),
	DLIST_FUNC_U32_I32 = DLIST_ARG_U32 | (DLIST_ARG_I32 << 8),
	DLIST_FUNC_U32_F32 = DLIST_ARG_U32 | (DLIST_ARG_F32 << 8),
	DLIST_FUNC_F32_F32 = DLIST_ARG_F32 | (DLIST_ARG_F32 << 8),
	// 3 arguments
	DLIST_FUNC_I32_I32_I32 = DLIST_ARG_I32 | (DLIST_ARG_I32 << 8) | (DLIST_ARG_I32 << 16),
	DLIST_FUNC_U32_I32_I32 = DLIST_ARG_U32 | (DLIST_ARG_I32 << 8) | (DLIST_ARG_I32 << 16),
	DLIST_FUNC_U32_U32_I32 = DLIST_ARG_U32 | (DLIST_ARG_U32 << 8) | (DLIST_ARG_I32 << 16),
	DLIST_FUNC_U32_I32_U32 = DLIST_ARG_U32 | (DLIST_ARG_I32 << 8) | (DLIST_ARG_U32 << 16),
	DLIST_FUNC_U32_U32_U32 = DLIST_ARG_U32 | (DLIST_ARG_U32 << 8) | (DLIST_ARG_U32 << 16),
	DLIST_FUNC_U32_F32_F32 = DLIST_ARG_U32 | (DLIST_ARG_F32 << 8) | (DLIST_ARG_F32 << 16),
	DLIST_FUNC_U32_U32_F32 = DLIST_ARG_U32 | (DLIST_ARG_U32 << 8) | (DLIST_ARG_F32 << 16),
	DLIST_FUNC_F32_F32_F32 = DLIST_ARG_F32 | (DLIST_ARG_F32 << 8) | (DLIST_ARG_F32 << 16),
	DLIST_FUNC_I16_I16_I16 = DLIST_ARG_I16 | (DLIST_ARG_I16 << 8) | (DLIST_ARG_I16 << 16),
	DLIST_FUNC_U8_U8_U8    = DLIST_ARG_U8  | (DLIST_ARG_U8 << 8)  | (DLIST_ARG_U8 << 16),
	// 4 arguments
	DLIST_FUNC_U32_U32_U32_U32 = DLIST_ARG_U32 | (DLIST_ARG_U32 << 8) | (DLIST_ARG_U32 << 16) | (DLIST_ARG_U32 << 24),
	DLIST_FUNC_I32_I32_I32_I32 = DLIST_ARG_I32 | (DLIST_ARG_I32 << 8) | (DLIST_ARG_I32 << 16) | (DLIST_ARG_I32 << 24),
	DLIST_FUNC_I32_U32_I32_U32 = DLIST_ARG_I32 | (DLIST_ARG_U32 << 8) | (DLIST_ARG_I32 << 16) | (DLIST_ARG_U32 << 24),
	DLIST_FUNC_U32_I32_U32_U32 = DLIST_ARG_U32 | (DLIST_ARG_I32 << 8) | (DLIST_ARG_U32 << 16) | (DLIST_ARG_U32 << 24),
	DLIST_FUNC_F32_F32_F32_F32 = DLIST_ARG_F32 | (DLIST_ARG_F32 << 8) | (DLIST_ARG_F32 << 16) | (DLIST_ARG_F32 << 24),
	DLIST_FUNC_U8_U8_U8_U8     = DLIST_ARG_U8  | (DLIST_ARG_U8 << 8)  | (DLIST_ARG_U8 << 16)  | (DLIST_ARG_U8 << 24),
} dlistFuncType;

// Available ffp shading models
typedef enum {
	//FLAT, // FIXME: Not easy to implement with ShaccCg constraints
	SMOOTH,
	PHONG
} shadingMode;

// Display list function call internal struct
typedef struct {
	void (*func)();
	uint8_t args[16];
	uint32_t type;
	void *next;
} list_chain;

// Display list internal struct
typedef struct {
	GLboolean used;
	list_chain *head;
	list_chain *tail;
} display_list;

// Matrix uniform struct
typedef struct {
	const SceGxmProgramParameter *ptr;
	void *chain;
} matrix_uniform;

// Uniform block struct
typedef struct {
	char name[128];
	uint8_t idx;
	void *chain;
} block_uniform;

// Generic shader struct
typedef struct {
	GLenum type;
	GLboolean valid;
	GLboolean dirty;
	int16_t ref_counter;
	SceGxmShaderPatcherId id;
	const SceGxmProgram *prog;
	uint32_t size;
	char *source;
#ifdef HAVE_GLSL_TRANSLATOR
	char *glsl_source;
#endif
	matrix_uniform *mat;
	block_uniform *unif_blk;
#ifdef HAVE_SHARK_LOG
	char *log;
#endif
} shader;

#include "shaders.h"

// Internal stuffs
extern GLboolean skip_viewport_override;
extern uint8_t texcoord_idxs[TEXTURE_COORDS_NUM];
extern uint8_t texcoord_fixed_idxs[TEXTURE_COORDS_NUM];
extern uint8_t ffp_vertex_attrib_fixed_mask;
extern uint8_t ffp_vertex_attrib_fixed_pos_mask;
extern legacy_vtx_attachment current_vtx;
extern void *frag_uniforms;
extern void *vert_uniforms;
extern SceGxmMultisampleMode msaa_mode;
extern void *gxm_color_surfaces_addr[DISPLAY_MAX_BUFFER_COUNT]; // Display color surfaces memblock starting addresses
extern SceGxmColorSurface gxm_color_surfaces[DISPLAY_MAX_BUFFER_COUNT]; // Display color surfaces
extern unsigned int gxm_back_buffer_index; // Display back buffer id
extern GLboolean use_extra_mem;
extern blend_config blend_info;
extern SceGxmVertexAttribute vertex_attrib_config[VERTEX_ATTRIBS_NUM];
extern GLboolean is_rendering_display; // Flag for when we're rendering without a framebuffer object
extern uint16_t *default_idx_ptr; // sceGxm mapped progressive indices buffer
extern uint16_t *default_quads_idx_ptr; // sceGxm mapped progressive indices buffer for quads
extern uint16_t *default_line_strips_idx_ptr; // sceGxm mapped progressive indices buffer for line strips
#if !defined(HAVE_PTHREAD) && defined(HAVE_SINGLE_THREADED_GC)
extern int garbage_collector(unsigned int args, void *arg); // Garbage collector function
#endif
extern SceUID gc_mutex[2]; // Garbage collector mutex
extern GLboolean has_cached_mem; // Flag for wether to use cached memory for mempools or not
extern uint8_t gxm_display_buffer_count; // Display buffers count

extern int legacy_pool_size; // Mempool size for GL1 immediate draw pipeline
extern float *legacy_pool; // Mempool for GL1 immediate draw pipeline
extern float *legacy_pool_ptr; // Current address for vertices population for GL1 immediate draw pipeline
#ifndef SKIP_ERROR_HANDLING
extern float *legacy_pool_end; // Address of the end of the GL1 immediate draw pipeline vertex pool
#endif
extern uint32_t vgl_framecount; // Current frame number since application started
extern SceGxmVertexAttribute legacy_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM - 1];
extern SceGxmVertexStream legacy_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM - 1];
extern SceGxmVertexAttribute legacy_mt_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
extern SceGxmVertexStream legacy_mt_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];
extern SceGxmVertexAttribute legacy_nt_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM - 2];
extern SceGxmVertexStream legacy_nt_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM - 2];
extern SceGxmVertexAttribute ffp_vertex_attrib_config[FFP_VERTEX_ATTRIBS_NUM];
extern SceGxmVertexStream ffp_vertex_stream_config[FFP_VERTEX_ATTRIBS_NUM];

// Fixed function pipeline attribute masks
enum {
	FFP_ATTRIB_POSITION = 0,
	FFP_ATTRIB_TEX0 = 1,
	FFP_ATTRIB_COLOR = 2,
	FFP_ATTRIB_DIFFUSE = 3,
	FFP_ATTRIB_SPECULAR = 4,
	FFP_ATTRIB_EMISSION = 5,
	FFP_ATTRIB_NORMAL = 6,
	FFP_ATTRIB_TEX1 = 7,
	FFP_ATTRIB_TEX2 = 8,
	FFP_ATTRIB_MASK_ALL = 0xFFFF
};
enum {
	FFP_AMBIENT_COEFF = 0,
	FFP_DIFFUSE_COEFF,
	FFP_SPECULAR_COEFF,
	FFP_EMISSION_COEFF,
	FFP_COEFF_NUM
};

extern uint8_t ffp_texcoord_binds[3];
#define FFP_ATTRIB_TEX(i) (ffp_texcoord_binds[i])
#define FFP_ATTRIB_IS_TEX(i) (i == FFP_ATTRIB_TEX0 || i == FFP_ATTRIB_TEX1 || i == FFP_ATTRIB_TEX2)
#define FFP_ATTRIB_IS_LIGHT(i) (i >= FFP_ATTRIB_COLOR && i <= FFP_ATTRIB_NORMAL)
#define FFP_ATTRIB_LIGHT_COEFF(i) (i - (FFP_ATTRIB_COLOR - FFP_AMBIENT_COEFF))

#ifdef HAVE_PROFILING
extern uint32_t frame_profiler_cnt;
extern uint32_t ffp_draw_profiler_cnt;
extern uint32_t ffp_reload_profiler_cnt;
extern uint32_t shaders_draw_profiler_cnt;
extern uint32_t ffp_draw_cnt;
extern uint32_t shaders_draw_cnt;
#endif

// Logging callback for vitaShaRK
#if defined(HAVE_SHARK_LOG) || defined(LOG_ERRORS)
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
	uint32_t vertex_duration;
	uint32_t fragment_duration;
} scene_metrics;

typedef struct {
	uint32_t vertex_job_count;
	uint64_t vertex_job_time;
	uint32_t fragment_job_count;
	uint64_t fragment_job_time;
	uint32_t firmware_job_count;
	uint64_t firmware_job_time;
	float usse_vertex_processing_percent;
	float usse_fragment_processing_percent;
	float usse_dependent_texture_reads_percent;
	float usse_non_dependent_texture_reads_percent;
	uint32_t vdm_primitives_input_num;
	uint32_t mte_primitives_output_num;
	uint32_t vdm_vertices_input_num;
	uint32_t mte_vertices_output_num;
	uint32_t rasterized_pixels_before_hsr_num;
	uint32_t rasterized_output_pixels_num;
	uint32_t rasterized_output_samples_num;
	uint32_t tiling_accelerated_mem_writes;
	uint32_t isp_parameter_fetches_mem_reads;
	uint32_t peak_usage_value;
	uint8_t partial_render;
	uint8_t vertex_job_paused;
	uint64_t frame_start_time;
	uint32_t frame_duration;
	uint32_t frame_number;
	uint32_t gpu_activity_duration_time;
	uint32_t scene_count;
	scene_metrics scenes[RAZOR_MAX_SCENES_NUM];
} razor_results;

extern uint32_t frame_idx; // Current frame number
extern GLboolean has_razor_live; // Flag for live metrics support with sceRazor
#endif

extern GLboolean is_shark_online; // Current vitaShaRK status
extern GLboolean dirty_frag_unifs;
extern GLboolean dirty_vert_unifs;

// Internal fixed function pipeline dirty flags and variables
extern GLboolean ffp_dirty_frag;
extern GLboolean ffp_dirty_vert;
extern uint16_t ffp_vertex_attrib_state;
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
extern SceGxmDepthStencilSurface gxm_depth_stencil_surface; // Depth/Stencil surfaces setup for sceGxm
extern GLboolean system_app_mode; // Flag for system app mode usage

extern sampler *samplers[COMBINED_TEXTURE_IMAGE_UNITS_NUM]; // Sampler objects array

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
extern GLfloat vgl_alpha_ref; // Current in use alpha test reference value
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
extern int unpack_row_len; // Current setting for GL_UNPACK_ROW_LENGTH

// Matrices
extern matrix4x4 *matrix; // Current in-use matrix mode
GLint get_gl_matrix_mode(); // Get current in-use matrix mode (for glGetIntegerv)

// Miscellaneous
extern glPhase phase; // Current drawing phase for legacy openGL
extern vector4f current_color; // Current in use color
extern vector4f clear_rgba_val; // Current clear color for glClear
extern viewport gl_viewport; // Current viewport state
extern GLboolean is_fbo_float; // Current framebuffer mode
extern vao *cur_vao; // Current in-use vertex array object
extern shadingMode shading_mode;

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

// Point Sprite
extern GLboolean point_sprite_state; // Current state for GL_POINT_SPRITE

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
extern renderbuffer *active_rb; // Current renderbuffer in use
extern GLboolean srgb_mode; // SRGB mode for color output

// Display Lists
extern display_list *curr_display_list; // Current display list being generated
extern GLboolean display_list_execute; // Flag to check if compiled function should be executed as well
extern GLboolean _vgl_enqueue_list_func(void (*func)(), dlistFuncType type, ...);

// vgl* Draw Pipeline
extern void *vertex_object;
extern void *color_object;
extern void *texture_object;
extern void *index_object;

extern matrix4x4 mvp_matrix; // ModelViewProjection Matrix
extern matrix4x4 projection_matrix; // Projection Matrix
extern matrix4x4 modelview_matrix; // ModelView Matrix
extern matrix4x4 texture_matrix[TEXTURE_COORDS_NUM]; // Texture Matrix
extern matrix4x4 normal_matrix; // Normal Matrix
extern matrix4x4 modelview_matrix_stack[MODELVIEW_STACK_DEPTH]; // Modelview matrices stack
extern matrix4x4 projection_matrix_stack[GENERIC_STACK_DEPTH]; // Projection matrices stack
extern GLboolean mvp_modified; // Check if ModelViewProjection matrix needs to be recreated

extern GLuint cur_program; // Current in use custom program (0 = No custom program)
extern uint32_t vsync_interval; // Current setting for VSync

extern uint32_t vertex_array_unit; // Current in-use vertex array buffer unit
extern uint32_t uniform_array_unit; // Current in-use uniform buffer unit

extern GLenum orig_depth_test; // Original depth test state (used for depth test invalidation)
extern framebuffer *in_use_framebuffer; // Currently in use framebuffer
extern uint8_t dirty_framebuffer; // Flag wether current in use framebuffer is invalidated

// Scissor test shaders
extern SceGxmFragmentProgram *scissor_test_fragment_program; // Scissor test fragment program
extern vector4f *scissor_test_vertices; // Scissor test region vertices
extern SceUID scissor_test_vertices_uid; // Scissor test vertices memblock id
extern GLboolean skip_scene_reset;

extern uint16_t *depth_clear_indices; // Memblock starting address for clear screen indices

// Clear screen shaders
extern vector4f *clear_vertices; // Memblock starting address for clear screen vertices

extern GLboolean fast_texture_compression; // Hints for texture compression
extern GLboolean recompress_non_native;
extern GLboolean fast_perspective_correction_hint;
extern GLfloat point_size; // Size of points for fixed function pipeline

/* gxm.c */
void initGxm(void); // Inits sceGxm
void initGxmContext(void); // Inits sceGxm context
void termGxmContext(void); // Terms sceGxm context
void createDisplayRenderTarget(void); // Creates render target for the display
void destroyDisplayRenderTarget(void); // Destroys render target for the display
void initDisplayColorSurfaces(void); // Creates color surfaces for the display
void termDisplayColorSurfaces(void); // Destroys color surfaces for the display
void initDepthStencilBuffer(uint32_t w, uint32_t h, SceGxmDepthStencilSurface *surface, GLboolean has_stencil); // Creates depth and stencil surfaces
void initDepthStencilSurfaces(void); // Creates depth and stencil surfaces for the display
void termDepthStencilSurfaces(void); // Destroys depth and stencil surfaces for the display
void startShaderPatcher(void); // Creates a shader patcher instance
void stopShaderPatcher(void); // Destroys a shader patcher instance
void waitRenderingDone(void); // Waits for rendering to be finished
void sceneReset(void); // Resets drawing scene if required
GLboolean startShaderCompiler(void); // Starts a shader compiler instance

/* framebuffers.c */
uint32_t get_alpha_channel_size(SceGxmColorFormat type); // Get alpha channel size in bits

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

/* blending.c */
void change_blend_factor(void); // Changes current blending settings for all used shaders
void change_blend_mask(void); // Changes color mask when blending is disabled for all used shaders
GLenum gxm_blend_to_gl(SceGxmBlendFactor factor); // Converts SceGxmBlendFactor to GL blend mode equivalent
GLenum gxm_blend_eq_to_gl(SceGxmBlendFunc factor); // Converts SceGxmBlendFunc to GL blend func equivalent

/* custom_shaders.c */
void resetCustomShaders(void); // Resets custom shaders
float *reserve_attrib_pool(uint8_t count);
void _vglDrawObjects_CustomShadersIMPL(GLboolean implicit_wvp); // vglDrawObjects implementation for rendering with custom shaders
GLboolean _glDrawElements_CustomShadersIMPL(uint16_t *idx_buf, GLsizei count, uint32_t top_idx, GLboolean is_short); // glDrawElements implementation for rendering with custom shaders
GLboolean _glDrawArrays_CustomShadersIMPL(GLint first, GLsizei count); // glDrawArrays implementation for rendering with custom shaders
void _glMultiDrawArrays_CustomShadersIMPL(SceGxmPrimitiveType gxm_p, uint16_t *idx_buf, const GLint *first, const GLsizei *count, GLint lowest, GLsizei highest, GLsizei drawcount); // glMultiDrawArrays implementation for rendering with custom shaders

/* ffp.c */
void _glDrawElements_FixedFunctionIMPL(uint16_t *idx_buf, GLsizei count, uint32_t top_idx, GLboolean is_short); // glDrawElements implementation for rendering with ffp
void _glDrawArrays_FixedFunctionIMPL(GLint first, GLsizei count); // glDrawArrays implementation for rendering with ffp
void _glMultiDrawArrays_FixedFunctionIMPL(SceGxmPrimitiveType gxm_p, uint16_t *idx_buf, const GLint *first, const GLsizei *count, GLint lowest, GLsizei highest, GLsizei drawcount); // glMultiDrawArrays implementation for rendering with ffp
uint8_t reload_ffp_shaders(SceGxmVertexAttribute *attrs, SceGxmVertexStream *streams, GLboolean is_short); // Reloads current in use ffp shaders
void upload_ffp_uniforms(); // Uploads required uniforms for the in use ffp shaders
void update_fogging_state(); // Updates current setup for fogging
void adjust_color_material_state(); // Updates internal settings for GL_COLOR_MATERIAL

/* buffers.c */
void resetVao(vao *v); // Reset vao state

/* display_lists.c */
void resetDlists(); // Reset display lists state

/* misc.c */
void change_cull_mode(void); // Updates current cull mode
void update_polygon_offset(); // Updates current polygon offset mode

/* misc functions */
void vector4f_convert_to_local_space(vector4f *out, int x, int y, int width, int height); // Converts screen coords to local space

/* debug.cpp */
void vgl_debugger_init(); // Inits ImGui debugger context
void vgl_debugger_draw(); // Draws ImGui debugger window
void vgl_debugger_light_draw(uint32_t *fb); // Draws CPU rendered debugger window

/* vitaGL.c */
uint8_t *vgl_reserve_data_pool(uint32_t size);

#endif
