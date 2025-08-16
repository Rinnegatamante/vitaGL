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
 * gxm.c:
 * Implementation for setup and cleanup for sceGxm specific stuffs
 */

#include "shared.h"

// Flags available for sceGxmVshInitialize
static enum {
	GXM_FLAG_DEFAULT = 0x00,
	GXM_FLAG_SYSAPP = 0x0A,
	GXM_FLAG_TEXFORMAT_EXT = 0x10
} sceGxmVshInitializeFlags;

// FIXME: Since we use our own default uniform buffers circular pool, fragment and vertex buffer rings can likely be reduced in size
static uint32_t gxm_param_buf_size = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE; // Param buffer size for sceGxm
static uint32_t gxm_vdm_buf_size = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE; // VDM ring buffer size for sceGxm
static uint32_t gxm_vertex_buf_size = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE; // Vertex ring buffer size for sceGxm
static uint32_t gxm_fragment_buf_size = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE; // Fragment ring buffer size for sceGxm
static uint32_t gxm_usse_buf_size = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE; // Fragment ring buffer size for sceGxm

// Shader Patcher memory configuration
static unsigned int shader_patcher_buffer_size = 1024 * 1024;
static unsigned int shader_patcher_vertex_usse_size = 1024 * 1024;
static unsigned int shader_patcher_fragment_usse_size = 1024 * 1024;

static void *vdm_ring_buffer_addr; // VDM ring buffer memblock starting address
static void *vertex_ring_buffer_addr; // vertex ring buffer memblock starting address
static void *fragment_ring_buffer_addr; // fragment ring buffer memblock starting address
static void *fragment_usse_ring_buffer_addr; // fragment USSE ring buffer memblock starting address

static SceGxmRenderTarget *gxm_render_target; // Display render target
SceGxmColorSurface gxm_color_surfaces[DISPLAY_MAX_BUFFER_COUNT]; // Display color surfaces
uint8_t gxm_display_buffer_count = DISPLAY_MAX_BUFFER_COUNT; // Display buffers count
void *gxm_color_surfaces_addr[DISPLAY_MAX_BUFFER_COUNT]; // Display color surfaces memblock starting addresses
static SceGxmSyncObject *gxm_sync_objects[DISPLAY_MAX_BUFFER_COUNT]; // Display sync objects
unsigned int gxm_front_buffer_index; // Display front buffer id
unsigned int gxm_back_buffer_index; // Display back buffer id
static void (*vgl_display_cb)(void *framebuf) = NULL; // Additional custom caallback used inside display queue callback

static void *gxm_shader_patcher_buffer_addr; // Shader PAtcher buffer memblock starting address
static void *gxm_shader_patcher_vertex_usse_addr; // Shader Patcher vertex USSE memblock starting address
static void *gxm_shader_patcher_fragment_usse_addr; // Shader Patcher fragment USSE memblock starting address

SceGxmDepthStencilSurface gxm_depth_stencil_surface; // Depth/Stencil surfaces setup for sceGxm

static SceUID shared_fb; // In-use hared framebuffer identifier
static SceSharedFbInfo shared_fb_info; // In-use shared framebuffer info struct
framebuffer *in_use_framebuffer = NULL; // Currently in use framebuffer
framebuffer *old_framebuffer = NULL; // Framebuffer used in last scene
uint8_t dirty_framebuffer = GL_FALSE; // Flag wether current in use framebuffer is invalidated
static GLboolean needs_end_scene = GL_FALSE; // Flag for gxm end scene requirement at scene reset
static GLboolean needs_scene_reset = GL_TRUE; // Flag for when a scene reset is required

SceGxmContext *gxm_context; // sceGxm context instance
GLenum vgl_error = GL_NO_ERROR; // Error returned by glGetError
SceGxmShaderPatcher *gxm_shader_patcher; // sceGxmShaderPatcher shader patcher instance
GLboolean is_fbo_float = GL_FALSE; // Current framebuffer mode

#ifdef HAVE_PROFILING
uint32_t frame_profiler_cnt = 0;
uint32_t ffp_draw_profiler_cnt = 0;
uint32_t ffp_reload_profiler_cnt = 0;
uint32_t shaders_draw_profiler_cnt = 0;
uint32_t ffp_draw_cnt = 0;
uint32_t shaders_draw_cnt = 0;
static uint32_t gpu_stall_cnt = 0;
#endif

int DISPLAY_WIDTH; // Display width in pixels
int DISPLAY_HEIGHT; // Display height in pixels
int DISPLAY_STRIDE; // Display stride in pixels
float DISPLAY_WIDTH_FLOAT; // Display width in pixels (float)
float DISPLAY_HEIGHT_FLOAT; // Display height in pixels (float)

GLboolean system_app_mode = GL_FALSE; // Flag for system app mode usage
static GLboolean gxm_initialized = GL_FALSE; // Current sceGxm state
GLboolean is_rendering_display = GL_TRUE; // Flag for when drawing without fbo is being performed

float *legacy_pool = NULL; // Mempool for GL1 immediate draw pipeline
float *legacy_pool_ptr = NULL; // Current address for vertices population for GL1 immediate draw pipeline
#ifndef SKIP_ERROR_HANDLING
float *legacy_pool_end = NULL; // Address of the end of the GL1 immediate draw pipeline vertex pool
#endif
uint32_t vgl_framecount = 0; // Current frame number since application started

void *frame_purge_list[FRAME_PURGE_FREQ][FRAME_PURGE_LIST_SIZE]; // Purge list for internal elements
void *frame_rt_purge_list[FRAME_PURGE_FREQ][FRAME_PURGE_RENDERTARGETS_LIST_SIZE]; // Purge list for rendertargets
int frame_purge_idx = 0; // Index for currently populatable purge list
int frame_elem_purge_idx = 0; // Index for currently populatable purge list element
int frame_rt_purge_idx = 0; // Index for currently populatable purge list rendetarget
static int frame_purge_clean_idx = 1;
SceUID gc_mutex[2];
static int gc_thread_priority = 0x10000100;
static int gc_thread_affinity = 0;
#ifdef HAVE_PTHREAD
pthread_t gc_thread;
#else
SceUID gc_thread;
#endif

#ifdef HAVE_CPU_TRACER
int sceRazorCpuSync();
#endif

#ifdef HAVE_RAZOR
#define RAZOR_BUF_SIZE (256 * 1024) // Size in bytes for a live metrics data buffer
#define UPDATE_RATIO 30 // Number of frames between two live metrics updates

#ifndef HAVE_DEVKIT
#define RAZOR_CAPTURE_MOD_PATH "ur0:data/librazorcapture_es4.suprx"
SceUID razor_modid;
#endif

typedef union {
	SceRazorGpuLiveEntryJob *job;
	SceRazorGpuLiveEntryParameterBuffer *pbuf;
	SceRazorGpuLiveEntryFrame *frame;
	uintptr_t ptr;
} SceRazorGpuResult;

uint8_t *razor_buf[DISPLAY_MAX_BUFFER_COUNT]; // Buffers used to store live metrics data
uint32_t frame_idx = 0; // Current frame number
razor_results razor_metrics;

GLboolean has_razor_live = GL_FALSE; // Flag for live metrics support with sceRazor
#endif

static inline __attribute__((always_inline)) int setupRenderTarget(SceGxmRenderTarget **rt, int w, int h, int refs) {
	SceGxmRenderTargetParams renderTargetParams;
	vgl_memset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
	renderTargetParams.width = w ? w : 1;
	renderTargetParams.height = h ? h : 1;
	renderTargetParams.scenesPerFrame = refs;
	renderTargetParams.multisampleMode = msaa_mode;
	renderTargetParams.driverMemBlock = -1;
	return sceGxmCreateRenderTarget(&renderTargetParams, rt);
}

#ifdef HAVE_SHARED_RENDERTARGETS
#define MAX_RENDER_TARGETS_NUM 47 // Maximum amount of dedicated render targets usable for fbos
#define MAX_SHARED_RT_SIZE 256 // Maximum  width value in pixels for shared rendertargets usage
#define MAX_SCENES_PER_FRAME 8 // Maximum amount of scenes per frame allowed by sceGxm per render target
render_target rt_list[MAX_RENDER_TARGETS_NUM];

render_target *getFreeRenderTarget(int w, int h) {
	int i;
	for (i = 0; i < MAX_RENDER_TARGETS_NUM; i++) {
		if (rt_list[i].rt != NULL) {
			if (w == rt_list[i].w && h == rt_list[i].h && rt_list[i].ref_count < rt_list[i].max_refs) {
				rt_list[i].ref_count++;
				return &rt_list[i];
			}
		} else {
			rt_list[i].max_refs = w > MAX_SHARED_RT_SIZE ? 1 : MAX_SCENES_PER_FRAME;
			int r = setupRenderTarget(&rt_list[i].rt, w, h, rt_list[i].max_refs);
#ifdef LOG_ERRORS
			if (r)
				vgl_log("%s:%d Failed to create a shared rendertarget of size %dx%d (%s).\n", __FILE__, __LINE__, w, h, get_gxm_error_literal(r));
#endif
			rt_list[i].w = w;
			rt_list[i].h = h;
			rt_list[i].ref_count = 1;
			return &rt_list[i];
		}
	}
#ifdef RECYCLE_RENDERTARGETS
	vgl_log("%s:%d Out of rendertargets handles: Recycling an old rendertarget.\n", __FILE__, __LINE__);
	uint32_t oldest_framecount = 0xFFFFFFFF;
	render_target *r = NULL;
	for (i = 0; i < MAX_RENDER_TARGETS_NUM; i++) {
		if (rt_list[i].last_frame < oldest_framecount) {
			oldest_framecount = rt_list[i].last_frame;
			r = &rt_list[i];
		}
	}
	sceGxmFinish(gxm_context);
	sceGxmDestroyRenderTarget(r->rt);
	r->max_refs = w > MAX_SHARED_RT_SIZE ? 1 : MAX_SCENES_PER_FRAME;
	int res = setupRenderTarget(&r->rt, w, h, r->max_refs);
#ifdef LOG_ERRORS
	if (res)
		vgl_log("%s:%d Failed to create a shared rendertarget of size %dx%d (%s).\n", __FILE__, __LINE__, w, h, get_gxm_error_literal(res));
#endif
	r->w = w;
	r->h = h;
	r->ref_count = 1;
	return r;
#else
#ifdef LOG_ERRORS
	vgl_log("%s:%d Failed to create a shared rendertarget of size %dx%d (Out of rendertargets handles).\n", __FILE__, __LINE__, w, h);
#endif
	return NULL;
#endif
}

void __markRtAsDirty(render_target *rt) {
	rt->ref_count--;
	if (!rt->ref_count) {
		_markRtAsDirty(rt->rt);
		rt->rt = NULL;
	}
}
#endif

// sceDisplay callback data
struct display_queue_callback_data {
	void *addr;
};

// sceGxmShaderPatcher custom allocator
static void *shader_patcher_host_alloc_cb(void *user_data, unsigned int size) {
	return vglMalloc(size);
}

// sceGxmShaderPatcher custom deallocator
static void shader_patcher_host_free_cb(void *user_data, void *mem) {
	vgl_free(mem);
}

// sceDisplay callback
static void display_queue_callback(const void *callbackData) {
	// Populating sceDisplay framebuffer parameters
	SceDisplayFrameBuf display_fb;
	const struct display_queue_callback_data *cb_data = callbackData;
	vgl_memset(&display_fb, 0, sizeof(SceDisplayFrameBuf));
	display_fb.size = sizeof(SceDisplayFrameBuf);
	display_fb.base = cb_data->addr;
	display_fb.pitch = DISPLAY_STRIDE;
	display_fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	display_fb.width = DISPLAY_WIDTH;
	display_fb.height = DISPLAY_HEIGHT;

#if (defined(HAVE_DEBUG_INTERFACE) && !defined(HAVE_RAZOR_INTERFACE)) || defined(HAVE_LIGHT_RAZOR)
	// Drawing lightweighted debugger info
	vgl_debugger_light_draw(cb_data->addr);
#endif

	if (vgl_display_cb)
		vgl_display_cb(cb_data->addr);

	// Setting sceDisplay framebuffer
	sceDisplaySetFrameBuf(&display_fb, SCE_DISPLAY_SETBUF_NEXTFRAME);

	// Performing VSync if enabled
	if (vsync_interval)
		sceDisplayWaitVblankStartMulti(vsync_interval);
}

// Garbage collector
#if defined(HAVE_PTHREAD) && !defined(HAVE_SINGLE_THREADED_GC)
void garbage_collector(void *arg) {
#else
int garbage_collector(unsigned int args, void *arg) {
#endif
#ifndef HAVE_SINGLE_THREADED_GC
	for (;;) {
		// Waiting for garbage collection request
		sceKernelWaitSema(gc_mutex[0], 1, NULL);
#endif
		// Purging all elements marked for deletion
		for (int i = 0; i < FRAME_PURGE_LIST_SIZE; i++) {
			if (frame_purge_list[frame_purge_clean_idx][i]) {
				vgl_free(frame_purge_list[frame_purge_clean_idx][i]);
				frame_purge_list[frame_purge_clean_idx][i] = NULL;
			} else
				break;
		}
		for (int i = 0; i < FRAME_PURGE_RENDERTARGETS_LIST_SIZE; i++) {
			if (frame_rt_purge_list[frame_purge_clean_idx][i]) {
				sceGxmDestroyRenderTarget(frame_rt_purge_list[frame_purge_clean_idx][i]);
				frame_rt_purge_list[frame_purge_clean_idx][i] = NULL;
			} else
				break;
		}
		frame_purge_clean_idx = (frame_purge_clean_idx + 1) % FRAME_PURGE_FREQ;
		frame_purge_idx = (frame_purge_idx + 1) % FRAME_PURGE_FREQ;
		frame_elem_purge_idx = 0;
		frame_rt_purge_idx = 0;
#ifndef HAVE_SINGLE_THREADED_GC
		sceKernelSignalSema(gc_mutex[1], 1);
	}
#ifndef HAVE_PTHREAD
	return sceKernelExitDeleteThread(0);
#endif
#endif
}

GLboolean startShaderCompiler(void) {
	shark_set_allocators(vglMalloc, vglFree);
	is_shark_online = shark_init(NULL) >= 0;

	// If standard path failed to init we try to init it with ScePiglet path
	if (!is_shark_online) {
		is_shark_online = shark_init("ur0:data/external/libshacccg.suprx") >= 0;
#ifdef LOG_ERRORS
		if (!is_shark_online)
			vgl_log("%s:%d Fatal error: SceShaccCg not found.\n", __FILE__, __LINE__);
#endif
	}

	return is_shark_online;
}

void initGxm(void) {
	if (gxm_initialized)
		return;

#ifdef HAVE_RAZOR
	// Initializing sceRazor debugger
#ifdef HAVE_DEVKIT
	sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_HUD);
	sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_CAPTURE);
#else
	razor_modid = sceKernelLoadStartModule(RAZOR_CAPTURE_MOD_PATH, 0, NULL, 0, NULL, NULL);
#endif
#ifdef HAVE_DEVKIT
	for (int i = 0; i < DISPLAY_MAX_BUFFER_COUNT; i++) {
		razor_buf[i] = memalign(8, RAZOR_BUF_SIZE);
	}
#endif

	sceRazorGpuCaptureEnableSalvage("ux0:data/vitagl_gpucrash.sgx");
#endif

	// Initializing runtime shader compiler
	if (startShaderCompiler()) {
#if defined(HAVE_SHARK_LOG) || defined(LOG_ERRORS)
		shark_install_log_cb(shark_log_cb);
		shark_set_warnings_level(SHARK_WARN_HIGH);
#endif
	}

#ifndef HAVE_SINGLE_THREADED_GC
	// Initializing garbage collector
	gc_mutex[0] = sceKernelCreateSema("GC Sema Push", 0, 0, FRAME_PURGE_FREQ, NULL);
	gc_mutex[1] = sceKernelCreateSema("GC Sema Pull", 0, FRAME_PURGE_FREQ, FRAME_PURGE_FREQ, NULL);
#ifdef HAVE_PTRHEAD
	pthread_create(&gc_thread, NULL, garbage_collector, NULL);
	pthread_setaffinity_np(gc_thread, 4, &gc_thread_affinity);
#else
	gc_thread = sceKernelCreateThread("Garbage Collector", &garbage_collector, gc_thread_priority, 0x10000, 0, gc_thread_affinity, NULL);
	sceKernelStartThread(gc_thread, 0, NULL);
#endif
#endif

#ifndef HAVE_VITA3K_SUPPORT // Vita3K lacks sceGxmVshInitialize support, so we can't use it for sysapps
	// Checking if the running application is a system one
	SceAppMgrBudgetInfo info;
	info.size = sizeof(SceAppMgrBudgetInfo);
	if (!sceAppMgrGetBudgetInfo(&info)) {
		system_app_mode = GL_TRUE;
		gxm_display_buffer_count = 2; // Forcing double buffering in system app mode
		if (msaa_mode == SCE_GXM_MULTISAMPLE_NONE) // FIXME: For some reasons, disabling MSAA makes the shader patcher not able to compile fragment programs in sysapp mode...
			msaa_mode = SCE_GXM_MULTISAMPLE_2X;
	}
#endif

	// Initializing sceGxm init parameters
	SceGxmInitializeParams gxm_init_params;
	vgl_memset(&gxm_init_params, 0, sizeof(SceGxmInitializeParams));
#ifdef HAVE_VITA3K_SUPPORT // Vita3K lacks sceGxmVshInitialize support, so we use sceGxmInitialize instead and disable a couple of features (HW ETC1 support and sysapp mode support)
	gxm_init_params.flags = GXM_FLAG_DEFAULT;
#else
	gxm_init_params.flags = GXM_FLAG_TEXFORMAT_EXT | (system_app_mode ? GXM_FLAG_SYSAPP : GXM_FLAG_DEFAULT);
#endif
	gxm_init_params.displayQueueMaxPendingCount = gxm_display_buffer_count - 1;
	gxm_init_params.displayQueueCallback = display_queue_callback;
	gxm_init_params.displayQueueCallbackDataSize = sizeof(struct display_queue_callback_data);
	gxm_init_params.parameterBufferSize = gxm_param_buf_size;

	// Initializing sceGxm
#ifdef HAVE_VITA3K_SUPPORT // Vita3K lacks sceGxmVshInitialize support, so we use sceGxmInitialize instead and disable a couple of features (HW ETC1 support and sysapp mode support)
	sceGxmInitialize(&gxm_init_params);
#else
	sceGxmVshInitialize(&gxm_init_params);
#endif
	gxm_initialized = GL_TRUE;

#ifdef HAVE_DEVKIT
	sceRazorGpuLiveSetMetricsGroup(SCE_RAZOR_GPU_LIVE_METRICS_GROUP_PBUFFER_USAGE);
	has_razor_live = !sceRazorGpuLiveStart();
#endif
}

void initGxmContext(void) {
	// Allocating VDM ring buffer
	vdm_ring_buffer_addr = gpu_alloc_mapped_aligned(4096, gxm_vdm_buf_size, VGL_MEM_VRAM);

	// Allocating vertex ring buffer
	vertex_ring_buffer_addr = gpu_alloc_mapped_aligned(4096, gxm_vertex_buf_size, VGL_MEM_VRAM);

	// Allocating fragment ring buffer
	fragment_ring_buffer_addr = gpu_alloc_mapped_aligned(4096, gxm_fragment_buf_size, VGL_MEM_VRAM);

	// Allocating fragment USSE ring buffer
	unsigned int fragment_usse_offset;
	fragment_usse_ring_buffer_addr = gpu_fragment_usse_alloc_mapped(gxm_usse_buf_size, &fragment_usse_offset);

	// Setting sceGxm context parameters
	SceGxmContextParams gxm_context_params;
	vgl_memset(&gxm_context_params, 0, sizeof(SceGxmContextParams));
	gxm_context_params.hostMem = vglMalloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
	gxm_context_params.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	gxm_context_params.vdmRingBufferMem = vdm_ring_buffer_addr;
	gxm_context_params.vdmRingBufferMemSize = gxm_vdm_buf_size;
	gxm_context_params.vertexRingBufferMem = vertex_ring_buffer_addr;
	gxm_context_params.vertexRingBufferMemSize = gxm_vertex_buf_size;
	gxm_context_params.fragmentRingBufferMem = fragment_ring_buffer_addr;
	gxm_context_params.fragmentRingBufferMemSize = gxm_fragment_buf_size;
	gxm_context_params.fragmentUsseRingBufferMem = fragment_usse_ring_buffer_addr;
	gxm_context_params.fragmentUsseRingBufferMemSize = gxm_usse_buf_size;
	gxm_context_params.fragmentUsseRingBufferOffset = fragment_usse_offset;

	// Initializing sceGxm context
	sceGxmCreateContext(&gxm_context_params, &gxm_context);
#ifdef DISABLE_W_CLAMPING
	sceGxmSetWClampEnable(gxm_context, SCE_GXM_WCLAMP_MODE_DISABLED);
#endif

	// Initializing circular pool for uniform buffers
	vglSetupUniformCircularPool();
}

void termGxmContext(void) {
	// Deallocating ring buffers
	vgl_free(vdm_ring_buffer_addr);
	vgl_free(vertex_ring_buffer_addr);
	vgl_free(fragment_ring_buffer_addr);
	gpu_fragment_usse_free_mapped(fragment_usse_ring_buffer_addr);

	// Destroying sceGxm context
	sceGxmDestroyContext(gxm_context);

	if (system_app_mode) {
		sceSharedFbBegin(shared_fb, &shared_fb_info);
		sceGxmUnmapMemory(shared_fb_info.fb_base);
		sceSharedFbEnd(shared_fb);
		sceSharedFbClose(shared_fb);
	}

	// Shutting down runtime shader compiler
	glReleaseShaderCompiler();
}

void createDisplayRenderTarget(void) {
	// Creating render target for the display
	setupRenderTarget(&gxm_render_target, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1);
}

void destroyDisplayRenderTarget(void) {
	// Destroying render target for the display
	sceGxmDestroyRenderTarget(gxm_render_target);
}

void initDisplayColorSurfaces(void) {
	// Getting access to the shared framebuffer on system app mode
	while (system_app_mode) {
		shared_fb = sceSharedFbOpen(1);
		vgl_memset(&shared_fb_info, 0, sizeof(SceSharedFbInfo));
		sceSharedFbGetInfo(shared_fb, &shared_fb_info);
		if (shared_fb_info.index == 1)
			sceSharedFbClose(shared_fb);
		else {
			sceGxmMapMemory(shared_fb_info.fb_base, shared_fb_info.fb_size, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
			gxm_color_surfaces_addr[0] = shared_fb_info.fb_base;
			gxm_color_surfaces_addr[1] = shared_fb_info.fb_base2;
			break;
		}
	}

	int i;
	for (i = 0; i < gxm_display_buffer_count; i++) {
		// Allocating color surface memblock
		if (!system_app_mode) {
			gxm_color_surfaces_addr[i] = gpu_alloc_mapped_aligned(4096, VGL_ALIGN(4 * DISPLAY_STRIDE * DISPLAY_HEIGHT, 1 * 1024 * 1024), VGL_MEM_VRAM);
			vgl_memset(gxm_color_surfaces_addr[i], 0, 4 * DISPLAY_STRIDE * DISPLAY_HEIGHT);
		}

		// Initializing allocated color surface
		sceGxmColorSurfaceInit(&gxm_color_surfaces[i],
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			msaa_mode == SCE_GXM_MULTISAMPLE_NONE ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			DISPLAY_WIDTH,
			DISPLAY_HEIGHT,
			DISPLAY_STRIDE,
			gxm_color_surfaces_addr[i]);

		// Creating a display sync object for the allocated color surface
		sceGxmSyncObjectCreate(&gxm_sync_objects[i]);
	}
}

void termDisplayColorSurfaces(void) {
	// Deallocating display's color surfaces and destroying sync objects
	int i;
	for (i = 0; i < gxm_display_buffer_count; i++) {
		if (!system_app_mode)
			vgl_free(gxm_color_surfaces_addr[i]);
		sceGxmSyncObjectDestroy(gxm_sync_objects[i]);
	}
}

void initDepthStencilBuffer(uint32_t w, uint32_t h, SceGxmDepthStencilSurface *surface, GLboolean has_stencil) {
	// Calculating sizes for depth and stencil surfaces
	unsigned int depth_stencil_width = VGL_ALIGN(w, SCE_GXM_TILE_SIZEX);
	unsigned int depth_stencil_height = VGL_ALIGN(h, SCE_GXM_TILE_SIZEY);
	unsigned int depth_stencil_samples = depth_stencil_width * depth_stencil_height;
	if (msaa_mode == SCE_GXM_MULTISAMPLE_2X)
		depth_stencil_samples *= 2;
	else if (msaa_mode == SCE_GXM_MULTISAMPLE_4X)
		depth_stencil_samples *= 4;

	// Allocating depth surface
	void *depth_buffer = gpu_alloc_mapped_aligned(SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT, 4 * depth_stencil_samples, VGL_MEM_VRAM);

#ifdef STORE_DEPTH_STENCIL
	// Initializing mask update bit to 1
	vgl_memset(depth_buffer, 0x80, 4 * depth_stencil_samples);
#endif

	// Allocating stencil surface
	void *stencil_buffer = NULL;
	if (has_stencil)
		stencil_buffer = gpu_alloc_mapped_aligned(SCE_GXM_DEPTHSTENCIL_SURFACE_ALIGNMENT, depth_stencil_samples, VGL_MEM_VRAM);

	// Initializing depth and stencil surfaces
	sceGxmDepthStencilSurfaceInit(surface,
		stencil_buffer ? SCE_GXM_DEPTH_STENCIL_FORMAT_DF32M_S8 : SCE_GXM_DEPTH_STENCIL_FORMAT_DF32M,
		SCE_GXM_DEPTH_STENCIL_SURFACE_LINEAR,
		msaa_mode == SCE_GXM_MULTISAMPLE_4X ? depth_stencil_width * 2 : depth_stencil_width,
		depth_buffer, stencil_buffer);

#ifdef STORE_DEPTH_STENCIL
	sceGxmDepthStencilSurfaceSetForceLoadMode(surface, SCE_GXM_DEPTH_STENCIL_FORCE_LOAD_ENABLED);
	sceGxmDepthStencilSurfaceSetForceStoreMode(surface, SCE_GXM_DEPTH_STENCIL_FORCE_STORE_ENABLED);
#endif
}

void initDepthStencilSurfaces(void) {
	initDepthStencilBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT, &gxm_depth_stencil_surface, GL_TRUE);
}

void termDepthStencilSurfaces(void) {
	// Deallocating depth and stencil surfaces memblocks
	vgl_free(gxm_depth_stencil_surface.depthData);
	vgl_free(gxm_depth_stencil_surface.stencilData);
}

void startShaderPatcher(void) {
	// Allocating Shader Patcher buffer
	gxm_shader_patcher_buffer_addr = gpu_alloc_mapped_aligned(4, shader_patcher_buffer_size, VGL_MEM_VRAM);

	// Allocating Shader Patcher vertex USSE buffer
	unsigned int shader_patcher_vertex_usse_offset;
	gxm_shader_patcher_vertex_usse_addr = gpu_vertex_usse_alloc_mapped(shader_patcher_vertex_usse_size, &shader_patcher_vertex_usse_offset);

	// Allocating Shader Patcher fragment USSE buffer
	unsigned int shader_patcher_fragment_usse_offset;
	gxm_shader_patcher_fragment_usse_addr = gpu_fragment_usse_alloc_mapped(shader_patcher_fragment_usse_size, &shader_patcher_fragment_usse_offset);

	// Populating shader patcher parameters
	SceGxmShaderPatcherParams shader_patcher_params;
	vgl_memset(&shader_patcher_params, 0, sizeof(SceGxmShaderPatcherParams));
	shader_patcher_params.userData = NULL;
	shader_patcher_params.hostAllocCallback = shader_patcher_host_alloc_cb;
	shader_patcher_params.hostFreeCallback = shader_patcher_host_free_cb;
	shader_patcher_params.bufferAllocCallback = NULL;
	shader_patcher_params.bufferFreeCallback = NULL;
	shader_patcher_params.bufferMem = gxm_shader_patcher_buffer_addr;
	shader_patcher_params.bufferMemSize = shader_patcher_buffer_size;
	shader_patcher_params.vertexUsseAllocCallback = NULL;
	shader_patcher_params.vertexUsseFreeCallback = NULL;
	shader_patcher_params.vertexUsseMem = gxm_shader_patcher_vertex_usse_addr;
	shader_patcher_params.vertexUsseMemSize = shader_patcher_vertex_usse_size;
	shader_patcher_params.vertexUsseOffset = shader_patcher_vertex_usse_offset;
	shader_patcher_params.fragmentUsseAllocCallback = NULL;
	shader_patcher_params.fragmentUsseFreeCallback = NULL;
	shader_patcher_params.fragmentUsseMem = gxm_shader_patcher_fragment_usse_addr;
	shader_patcher_params.fragmentUsseMemSize = shader_patcher_fragment_usse_size;
	shader_patcher_params.fragmentUsseOffset = shader_patcher_fragment_usse_offset;

	// Creating shader patcher instance
	sceGxmShaderPatcherCreate(&shader_patcher_params, &gxm_shader_patcher);
}

void stopShaderPatcher(void) {
	// Destroying shader patcher instance
	sceGxmShaderPatcherDestroy(gxm_shader_patcher);

	// Freeing shader patcher buffers
	vgl_free(gxm_shader_patcher_buffer_addr);
	gpu_vertex_usse_free_mapped(gxm_shader_patcher_vertex_usse_addr);
	gpu_fragment_usse_free_mapped(gxm_shader_patcher_fragment_usse_addr);
}

void waitRenderingDone(void) {
	// Wait for rendering to be finished
	sceGxmDisplayQueueFinish();
	sceGxmFinish(gxm_context);
}

void sceneEnd(void) {
	// Ends current gxm scene
	sceGxmEndScene(gxm_context, NULL, NULL);
	if (system_app_mode && vsync_interval)
		sceDisplayWaitVblankStartMulti(vsync_interval);
}

void sceneReset(void) {
	if (in_use_framebuffer != active_write_fb || needs_scene_reset || dirty_framebuffer) {
		dirty_framebuffer = GL_FALSE;
		needs_scene_reset = GL_FALSE;
		in_use_framebuffer = active_write_fb;
		is_fbo_float = in_use_framebuffer ? in_use_framebuffer->is_float : GL_FALSE;

		// Ending drawing scene
		if (needs_end_scene)
			sceneEnd();
		else {
			if (legacy_pool_size) {
				legacy_pool = (float *)gpu_alloc_mapped_temp(legacy_pool_size);
				legacy_pool_ptr = legacy_pool;
#ifndef SKIP_ERROR_HANDLING
				legacy_pool_end = (float *)((uint8_t *)legacy_pool + legacy_pool_size);
#endif
			}
			needs_end_scene = GL_TRUE;
		}

		// Starting drawing scene
		is_rendering_display = !active_write_fb;
		if (is_rendering_display) { // Default framebuffer is used
			if (system_app_mode) {
				sceSharedFbBegin(shared_fb, &shared_fb_info);
				shared_fb_info.vsync = vsync_interval;
				gxm_back_buffer_index = (shared_fb_info.index + 1) % 2;
			}
#ifdef LOG_ERRORS
			int r = sceGxmBeginScene(gxm_context, 0, gxm_render_target,
				NULL, NULL,
				gxm_sync_objects[gxm_back_buffer_index],
				&gxm_color_surfaces[gxm_back_buffer_index],
				&gxm_depth_stencil_surface);
			if (r)
				vgl_log("%s:%d Scene reset failed due to sceGxmBeginScene erroring (%s) on display.\n", __FILE__, __LINE__, get_gxm_error_literal(r));
#else
			sceGxmBeginScene(gxm_context, 0, gxm_render_target,
				NULL, NULL,
				gxm_sync_objects[gxm_back_buffer_index],
				&gxm_color_surfaces[gxm_back_buffer_index],
				&gxm_depth_stencil_surface);
#endif
		} else {
			// If a depthstencil surface is not bound to the in use framebuffer, we get one for it to ensure scissor testing compatibility
			if (!active_write_fb->depthbuffer_ptr) {
				initDepthStencilBuffer(active_write_fb->width, active_write_fb->height, &active_write_fb->depthbuffer, GL_FALSE);
				active_write_fb->depthbuffer_ptr = &active_write_fb->depthbuffer;
				active_write_fb->is_depth_hidden = GL_TRUE;
			}

			// If a rendertarget is not bound to the in use framebuffer, we get one for it
			if (!active_write_fb->target) {
#ifdef HAVE_SHARED_RENDERTARGETS
				active_write_fb->target = (SceGxmRenderTarget *)getFreeRenderTarget(active_write_fb->width, active_write_fb->height);
#else
				int r = setupRenderTarget(&active_write_fb->target, active_write_fb->width, active_write_fb->height, 1);
#ifdef LOG_ERRORS
				if (r)
					vgl_log("%s:%d Failed to create a rendertarget of size %dx%d for framebuffer 0x%08X (%s).\n", __FILE__, __LINE__, active_write_fb->width, active_write_fb->height, active_write_fb, get_gxm_error_literal(r));
#endif
#endif
			}
#ifdef RECYCLE_RENDERTARGETS
			else {
				render_target *fbo_rt = (render_target *)active_write_fb->target;
				if (active_write_fb->width != fbo_rt->w || active_write_fb->height != fbo_rt->h) {
					vgl_log("%s:%d Attempting to use a recycled rendertarget. Re-allocating it.\n", __FILE__, __LINE__);
					active_write_fb->target = (SceGxmRenderTarget *)getFreeRenderTarget(active_write_fb->width, active_write_fb->height);
				}
			}
#endif
#ifdef HAVE_TEX_CACHE
			// FIXME: This may be useful even without texture cache enabled maybe?
			if (sceGxmColorSurfaceGetData(&active_write_fb->colorbuffer) != active_write_fb->tex->data)
				sceGxmColorSurfaceSetData(&active_write_fb->colorbuffer, active_write_fb->tex->data);
#endif
#ifdef HAVE_SHARED_RENDERTARGETS
			render_target *fbo_rt = (render_target *)active_write_fb->target;
#ifdef RECYCLE_RENDERTARGETS
			fbo_rt->last_frame = vgl_framecount;
#endif
			int r = sceGxmBeginScene(gxm_context, 0, fbo_rt->rt,
#else
			int r = sceGxmBeginScene(gxm_context, 0, active_write_fb->target,
#endif
					NULL, NULL, NULL,
					&active_write_fb->colorbuffer,
					active_write_fb->depthbuffer_ptr);
#ifdef LOG_ERRORS
			if (r)
				vgl_log("%s:%d Scene reset failed due to sceGxmBeginScene erroring (%s) on framebuffer 0x%08X.\n", __FILE__, __LINE__, get_gxm_error_literal(r), active_write_fb);
#endif
		}

		// Setting back current viewport if enabled cause sceGxm will reset it at sceGxmEndScene call
		if (old_framebuffer != in_use_framebuffer) {
			old_framebuffer = in_use_framebuffer;
			glViewport(gl_viewport.x, gl_viewport.y, gl_viewport.w, gl_viewport.h);
			skip_scene_reset = GL_TRUE;
			glScissor(region.gl_x, region.gl_y, region.gl_w, region.gl_h);
			skip_scene_reset = GL_FALSE;
#ifndef HAVE_UNFLIPPED_FBOS
			change_cull_mode();
#endif
		} else
			setViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);

#ifndef DISABLE_TILE_CLIPPER
		if (scissor_test_state)
			sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_OUTSIDE, region.x, region.y, region.x + region.w - 1, region.y + region.h - 1);
#endif
	}
}

/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */

void vglSetupGarbageCollector(int priority, int affinity) {
	gc_thread_priority = priority;
	gc_thread_affinity = affinity;
}

void vglSetParamBufferSize(uint32_t size) {
	gxm_param_buf_size = size;
}

void vglSetVDMBufferSize(uint32_t size) {
	gxm_vdm_buf_size = size;
}

void vglSetVertexBufferSize(uint32_t size) {
	gxm_vertex_buf_size = size;
}

void vglSetFragmentBufferSize(uint32_t size) {
	gxm_fragment_buf_size = size;
}

void vglSetUSSEBufferSize(uint32_t size) {
	gxm_usse_buf_size = size;
}

void vglUseTripleBuffering(GLboolean usage) {
	gxm_display_buffer_count = usage ? 3 : 2;
}

void vglSwapBuffers(GLboolean has_commondialog) {
#ifdef HAVE_PROFILING
	// Show profiling results once every 30 frames to not clog CPU
	uint32_t tick = sceKernelGetProcessTimeLow();
	static uint32_t frame_start_profiler_cnt = 0;
	frame_profiler_cnt += tick - frame_start_profiler_cnt;
	if ((vgl_framecount % 30) == 0) {
		vgl_log("-----------------------------------------\n");
		vgl_log("Last 30 frames took %ums to be processed.\n", frame_profiler_cnt / 1000);
		vgl_log("%ums spent processing %u fixed-function pipeline non-immediate draw calls.\n", ffp_draw_profiler_cnt / 1000, ffp_draw_cnt);
		vgl_log("%ums spent setting up fixed-function pipeline states.\n", ffp_reload_profiler_cnt / 1000);
		vgl_log("%ums spent processing %u shaders pipeline draw calls.\n", shaders_draw_profiler_cnt / 1000, shaders_draw_cnt);
		vgl_log("%ums spent waiting for GPU to process frames.\n", gpu_stall_cnt / 1000);
		vgl_log("-----------------------------------------\n");
		frame_profiler_cnt = 0;
		ffp_draw_profiler_cnt = 0;
		ffp_reload_profiler_cnt = 0;
		shaders_draw_profiler_cnt = 0;
		shaders_draw_cnt = 0;
		ffp_draw_cnt = 0;
		gpu_stall_cnt = 0;
	}
	frame_start_profiler_cnt = tick;
#endif

	vgl_framecount++;
#ifdef HAVE_FAILSAFE_CIRCULAR_VERTEX_POOL
	vgl_circular_idx = vgl_framecount % gxm_display_buffer_count;
	vertex_data_pool_ptr[vgl_circular_idx] = vertex_data_pool[vgl_circular_idx];
#endif

	// Marking uniform values as dirty at each frame end just to be safe
	dirty_frag_unifs = GL_TRUE;
	dirty_vert_unifs = GL_TRUE;

#if defined(HAVE_RAZOR_INTERFACE) && !defined(HAVE_LIGHT_RAZOR)
	if (!in_use_framebuffer) {
		vgl_debugger_draw();
	}
#endif

	needs_end_scene = GL_FALSE;

	if (!needs_scene_reset)
		sceneEnd();

	if (has_commondialog) {
		// Populating SceCommonDialog parameters
		SceCommonDialogUpdateParam updateParam;
		vgl_memset(&updateParam, 0, sizeof(updateParam));
		updateParam.renderTarget.colorFormat = SCE_GXM_COLOR_FORMAT_A8B8G8R8;
		updateParam.renderTarget.surfaceType = SCE_GXM_COLOR_SURFACE_LINEAR;
		updateParam.renderTarget.width = DISPLAY_WIDTH;
		updateParam.renderTarget.height = DISPLAY_HEIGHT;
		updateParam.renderTarget.strideInPixels = DISPLAY_STRIDE;
		updateParam.renderTarget.colorSurfaceData = gxm_color_surfaces_addr[gxm_back_buffer_index];
		updateParam.renderTarget.depthSurfaceData = gxm_depth_stencil_surface.depthData;
		updateParam.displaySyncObject = gxm_sync_objects[gxm_back_buffer_index];

		// Updating sceCommonDialog
		sceCommonDialogUpdate(&updateParam);
	}

	if (!in_use_framebuffer) {
		if (system_app_mode)
			sceSharedFbEnd(shared_fb);
		else {
#ifdef HAVE_RAZOR
			sceGxmPadHeartbeat(&gxm_color_surfaces[gxm_back_buffer_index], gxm_sync_objects[gxm_back_buffer_index]);
#ifdef HAVE_DEVKIT
			if (has_razor_live) {
				SceRazorGpuLiveResultInfo razor_res;
				sceRazorGpuLiveSetBuffer(razor_buf[gxm_back_buffer_index], RAZOR_BUF_SIZE, &razor_res);

				if (razor_res.result_data) {
					if ((frame_idx % UPDATE_RATIO) == 1) {
						if (!razor_res.overflow_count) {
							vgl_memset(&razor_metrics, 0, sizeof(razor_results));
							SceUID pid = sceKernelGetProcessId();
							SceRazorGpuResult r;
							r.ptr = (uintptr_t)razor_res.result_data;

							// Analyzing the collected jobs
							for (uint32_t i = 0; i < razor_res.entry_count; i++) {
								switch (r.job->header.entry_type) {
								case SCE_RAZOR_LIVE_TRACE_METRIC_ENTRY_TYPE_JOB:
									if ((pid == r.job->process_id) && (r.job->type != SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FIRMWARE)) {
										if (razor_metrics.scene_count < r.job->scene_index + 1)
											razor_metrics.scene_count = r.job->scene_index + 1;
										switch (r.job->type) {
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX0:
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX1:
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX2:
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX3:
											razor_metrics.vertex_job_count++;
											razor_metrics.vertex_job_time += r.job->end_time - r.job->start_time;
											if (r.job->scene_index < RAZOR_MAX_SCENES_NUM) {
												razor_metrics.scenes[r.job->scene_index].vertex_duration += r.job->end_time - r.job->start_time;
											}
											break;
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT0:
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT1:
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT2:
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT3:
											razor_metrics.fragment_job_count++;
											razor_metrics.fragment_job_time += r.job->end_time - r.job->start_time;
											if (r.job->scene_index < RAZOR_MAX_SCENES_NUM) {
												razor_metrics.scenes[r.job->scene_index].fragment_duration += r.job->end_time - r.job->start_time;
											}
											break;
										}
										switch (r.job->type) {
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX1:
											razor_metrics.usse_vertex_processing_percent += r.job->job_values.vertex_values_type1.usse_vertex_processing_percent;
											break;
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX2:
											razor_metrics.vdm_primitives_input_num += r.job->job_values.vertex_values_type2.vdm_primitives_input_num;
											razor_metrics.mte_primitives_output_num += r.job->job_values.vertex_values_type2.mte_primitives_output_num;
											razor_metrics.vdm_vertices_input_num += r.job->job_values.vertex_values_type2.vdm_vertices_input_num;
											razor_metrics.mte_vertices_output_num += r.job->job_values.vertex_values_type2.mte_vertices_output_num;
											break;
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_VERTEX3:
											razor_metrics.tiling_accelerated_mem_writes += r.job->job_values.vertex_values_type3.tiling_accelerated_mem_writes;
											break;
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT1:
											razor_metrics.usse_fragment_processing_percent += r.job->job_values.fragment_values_type1.usse_fragment_processing_percent;
											razor_metrics.usse_dependent_texture_reads_percent += r.job->job_values.fragment_values_type1.usse_dependent_texture_reads_percent;
											razor_metrics.usse_non_dependent_texture_reads_percent += r.job->job_values.fragment_values_type1.usse_non_dependent_texture_reads_percent;
											break;
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT2:
											razor_metrics.rasterized_pixels_before_hsr_num += r.job->job_values.fragment_values_type2.rasterized_pixels_before_hsr_num;
											razor_metrics.rasterized_output_pixels_num += r.job->job_values.fragment_values_type2.rasterized_output_pixels_num;
											razor_metrics.rasterized_output_samples_num += r.job->job_values.fragment_values_type2.rasterized_output_samples_num;
											break;
										case SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FRAGMENT3:
											razor_metrics.isp_parameter_fetches_mem_reads += r.job->job_values.fragment_values_type3.isp_parameter_fetches_mem_reads;
											break;
										}
									} else if (r.job->type == SCE_RAZOR_LIVE_TRACE_METRIC_JOB_TYPE_FIRMWARE) {
										razor_metrics.firmware_job_count++;
										razor_metrics.firmware_job_time += r.job->end_time - r.job->start_time;
									}
									break;
								case SCE_RAZOR_LIVE_TRACE_METRIC_ENTRY_TYPE_PARAMETER_BUFFER:
									vgl_fast_memcpy(&razor_metrics.peak_usage_value, &r.pbuf->peak_usage_value, 6);
									break;
								case SCE_RAZOR_LIVE_TRACE_METRIC_ENTRY_TYPE_FRAME:
									vgl_fast_memcpy(&razor_metrics.frame_start_time, &r.frame->start_time, 20);
									break;
								default:
									break;
								}
								r.ptr += r.job->header.entry_size;
							}
						}
					}
					frame_idx++;
				}
			}
#endif
#endif
			struct display_queue_callback_data queue_cb_data;
			queue_cb_data.addr = gxm_color_surfaces_addr[gxm_back_buffer_index];
#ifdef HAVE_PROFILING
			tick = sceKernelGetProcessTimeLow();
#endif
			sceGxmDisplayQueueAddEntry(gxm_sync_objects[gxm_front_buffer_index], gxm_sync_objects[gxm_back_buffer_index], &queue_cb_data);
#ifdef HAVE_PROFILING
			gpu_stall_cnt += sceKernelGetProcessTimeLow() - tick;
#endif
#ifdef HAVE_CPU_TRACER
			sceRazorCpuSync();
#endif
			gxm_front_buffer_index = gxm_back_buffer_index;
			gxm_back_buffer_index = (gxm_back_buffer_index + 1) % gxm_display_buffer_count;
		}
	}
	needs_scene_reset = GL_TRUE;

	// Starting garbage collector job
#ifdef HAVE_SINGLE_THREADED_GC
	garbage_collector(0, NULL);
#else
	sceKernelWaitSema(gc_mutex[1], 1, NULL);
	sceKernelSignalSema(gc_mutex[0], 1);
#endif
}

void glFinish(void) {
	// Waiting for GPU to finish drawing jobs
	sceGxmFinish(gxm_context);
}

void glReleaseShaderCompiler(void) {
	if (is_shark_online) {
		shark_end();
		is_shark_online = GL_FALSE;
	}
}

void glFlush(void) {
}

void vglSetDisplayCallback(void (*cb)(void *framebuf)) {
	vgl_display_cb = cb;
}

void vglSetupShaderPatcher(uint32_t buffer_mem_size, uint32_t vertex_usse_mem_size, uint32_t fragment_usse_mem_size) {
	shader_patcher_buffer_size = buffer_mem_size;
	shader_patcher_vertex_usse_size = vertex_usse_mem_size;
	shader_patcher_fragment_usse_size = fragment_usse_mem_size;
}
