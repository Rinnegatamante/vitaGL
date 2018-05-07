/* 
 * gxm.c:
 * Implementation for setup and cleanup for sceGxm specific stuffs
 */

#include "shared.h" 

static void *vdm_ring_buffer_addr; // VDM ring buffer memblock starting address
static void *vertex_ring_buffer_addr; // vertex ring buffer memblock starting address
static void *fragment_ring_buffer_addr; // fragment ring buffer memblock starting address
static void *fragment_usse_ring_buffer_addr; // fragment USSE ring buffer memblock starting address

static SceGxmRenderTarget *gxm_render_target; // Display render target
static SceGxmColorSurface gxm_color_surfaces[DISPLAY_BUFFER_COUNT]; // Display color surfaces
static void* gxm_color_surfaces_addr[DISPLAY_BUFFER_COUNT]; // Display color surfaces memblock starting addresses
static SceGxmSyncObject *gxm_sync_objects[DISPLAY_BUFFER_COUNT]; // Display sync objects
static unsigned int gxm_front_buffer_index; // Display front buffer id
static unsigned int gxm_back_buffer_index; // Display back buffer id

static void *gxm_shader_patcher_buffer_addr; // Shader PAtcher buffer memblock starting address
static void *gxm_shader_patcher_vertex_usse_addr; // Shader Patcher vertex USSE memblock starting address
static void *gxm_shader_patcher_fragment_usse_addr; // Shader Patcher fragment USSE memblock starting address

static void *gxm_depth_surface_addr; // Depth surface memblock starting address
static void *gxm_stencil_surface_addr; // Stencil surface memblock starting address
static SceGxmDepthStencilSurface gxm_depth_stencil_surface; // Depth/Stencil surfaces setup for sceGxm

static uint8_t heap_mapped = GL_FALSE; // Check for heap memory mapping into sceGxm

SceGxmContext *gxm_context; // sceGxm context instance
GLenum error = GL_NO_ERROR; // Error returned by glGetError
SceGxmShaderPatcher *gxm_shader_patcher; // sceGxmShaderPatcher shader patcher instance

matrix4x4 mvp_matrix; // ModelViewProjection Matrix
matrix4x4 projection_matrix; // Projection Matrix
matrix4x4 modelview_matrix; // ModelView Matrix

int DISPLAY_WIDTH;            // Display width in pixels
int DISPLAY_HEIGHT;           // Display height in pixels
int DISPLAY_STRIDE;           // Display stride in pixels
float DISPLAY_WIDTH_FLOAT;   // Display width in pixels (float)
float DISPLAY_HEIGHT_FLOAT;  // Display height in pixels (float)

extern int _newlib_heap_memblock;  // Newlib Heap memblock
extern unsigned _newlib_heap_size; // Newlib Heap size

// sceDisplay callback data
struct display_queue_callback_data {
	void *addr;
};

// sceGxmShaderPatcher custom allocator
static void *shader_patcher_host_alloc_cb(void *user_data, unsigned int size){
	return malloc(size);
}

// sceGxmShaderPatcher custom deallocator
static void shader_patcher_host_free_cb(void *user_data, void *mem){
	return free(mem);
}

// sceDisplay callback
static void display_queue_callback(const void *callbackData){
	
	// Populating sceDisplay framebuffer parameters
	SceDisplayFrameBuf display_fb;
	const struct display_queue_callback_data *cb_data = callbackData;
	memset(&display_fb, 0, sizeof(SceDisplayFrameBuf));
	display_fb.size = sizeof(SceDisplayFrameBuf);
	display_fb.base = cb_data->addr;
	display_fb.pitch = DISPLAY_STRIDE;
	display_fb.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	display_fb.width = DISPLAY_WIDTH;
	display_fb.height = DISPLAY_HEIGHT;

	// Setting sceDisplay framebuffer
	sceDisplaySetFrameBuf(&display_fb, SCE_DISPLAY_SETBUF_NEXTFRAME);
	
	// Performing VSync if enabled
	if (vblank) sceDisplayWaitVblankStart();
	
}

void initGxm(void){
	
	// Initializing sceGxm init parameters
	SceGxmInitializeParams gxm_init_params;
	memset(&gxm_init_params, 0, sizeof(SceGxmInitializeParams));
	gxm_init_params.flags = 0;
	gxm_init_params.displayQueueMaxPendingCount = DISPLAY_BUFFER_COUNT - 1;
	gxm_init_params.displayQueueCallback = display_queue_callback;
	gxm_init_params.displayQueueCallbackDataSize = sizeof(struct display_queue_callback_data);
	gxm_init_params.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;
	
	// Initializing sceGxm
	sceGxmInitialize(&gxm_init_params);
	
}

void initGxmContext(void){
	
	// Allocating VDM ring buffer
	vdm_ring_buffer_addr = gpu_alloc_mapped(SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE, VGL_MEM_VRAM);

	// Allocating vertex ring buffer
	vertex_ring_buffer_addr = gpu_alloc_mapped(SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE, VGL_MEM_VRAM);

	// Allocating fragment ring buffer
	fragment_ring_buffer_addr = gpu_alloc_mapped(SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE, VGL_MEM_VRAM);

	// Allocating fragment USSE ring buffer
	unsigned int fragment_usse_offset;
	fragment_usse_ring_buffer_addr = gpu_fragment_usse_alloc_mapped(
		SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE, &fragment_usse_offset);

	// Setting sceGxm context parameters
	SceGxmContextParams gxm_context_params;
	memset(&gxm_context_params, 0, sizeof(SceGxmContextParams));
	gxm_context_params.hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
	gxm_context_params.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	gxm_context_params.vdmRingBufferMem = vdm_ring_buffer_addr;
	gxm_context_params.vdmRingBufferMemSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE;
	gxm_context_params.vertexRingBufferMem = vertex_ring_buffer_addr;
	gxm_context_params.vertexRingBufferMemSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE;
	gxm_context_params.fragmentRingBufferMem = fragment_ring_buffer_addr;
	gxm_context_params.fragmentRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE;
	gxm_context_params.fragmentUsseRingBufferMem = fragment_usse_ring_buffer_addr;
	gxm_context_params.fragmentUsseRingBufferMemSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE;
	gxm_context_params.fragmentUsseRingBufferOffset = fragment_usse_offset;
	
	// Initializing sceGxm context
	sceGxmCreateContext(&gxm_context_params, &gxm_context);
	
}

void termGxmContext(void){
	
	// Deallocating ring buffers
	gpu_free_mapped(vdm_ring_buffer_addr, VGL_MEM_VRAM);
	gpu_free_mapped(vertex_ring_buffer_addr, VGL_MEM_VRAM);
	gpu_free_mapped(fragment_ring_buffer_addr, VGL_MEM_VRAM);
	gpu_fragment_usse_free_mapped(fragment_usse_ring_buffer_addr);
	
	// Destroying sceGxm context
	sceGxmDestroyContext(gxm_context);
	
}

void createDisplayRenderTarget(void){
	
	// Populating sceGxmRenderTarget parameters
	SceGxmRenderTargetParams render_target_params;
	memset(&render_target_params, 0, sizeof(SceGxmRenderTargetParams));
	render_target_params.flags = 0;
	render_target_params.width = DISPLAY_WIDTH;
	render_target_params.height = DISPLAY_HEIGHT;
	render_target_params.scenesPerFrame = 1;
	render_target_params.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;
	render_target_params.multisampleLocations = 0;
	render_target_params.driverMemBlock = -1;
	
	// Creating render target for the display
	sceGxmCreateRenderTarget(&render_target_params, &gxm_render_target);
	
}

void destroyDisplayRenderTarget(void){
	
	// Destroying render target for the display
	sceGxmDestroyRenderTarget(gxm_render_target);
	
}

void initDisplayColorSurfaces(void){

	int i;
	for (i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		
		// Allocating color surface memblock
		gxm_color_surfaces_addr[i] = gpu_alloc_mapped(
			ALIGN(4 * DISPLAY_STRIDE * DISPLAY_HEIGHT, 1 * 1024 * 1024),
			VGL_MEM_VRAM);
		
		// Initializing allocated color surface
		memset(gxm_color_surfaces_addr[i], 0, DISPLAY_STRIDE * DISPLAY_HEIGHT);
		sceGxmColorSurfaceInit(&gxm_color_surfaces[i],
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			SCE_GXM_COLOR_SURFACE_SCALE_NONE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			DISPLAY_WIDTH,
			DISPLAY_HEIGHT,
			DISPLAY_STRIDE,
			gxm_color_surfaces_addr[i]);
		
		// Creating a display sync object for the allocated color surface
		sceGxmSyncObjectCreate(&gxm_sync_objects[i]);
		
	}
	
}

void termDisplayColorSurfaces(void){
	
	// Deallocating display's color surfaces and destroying sync objects
	int i;
	for (i = 0; i < DISPLAY_BUFFER_COUNT; i++) {
		gpu_free_mapped(gxm_color_surfaces_addr[i], VGL_MEM_VRAM);
		sceGxmSyncObjectDestroy(gxm_sync_objects[i]);
	}
	
}

void initDepthStencilSurfaces(void){
	
	// Calculating sizes for depth and stencil surfaces
	unsigned int depth_stencil_width = ALIGN(DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX);
	unsigned int depth_stencil_height = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
	unsigned int depth_stencil_samples = depth_stencil_width * depth_stencil_height;

	// Allocating depth surface
	gxm_depth_surface_addr = gpu_alloc_mapped(4 * depth_stencil_samples, VGL_MEM_VRAM);
	
	// Allocating stencil surface
	gxm_stencil_surface_addr = gpu_alloc_mapped(1 * depth_stencil_samples, VGL_MEM_VRAM);

	// Initializing depth and stencil surfaces
	sceGxmDepthStencilSurfaceInit(&gxm_depth_stencil_surface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_DF32M_S8,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		depth_stencil_width,
		gxm_depth_surface_addr,
		gxm_stencil_surface_addr);
	
}

void termDepthStencilSurfaces(void){
	
	// Deallocating depth and stencil surfaces memblocks
	gpu_free_mapped(gxm_depth_surface_addr, VGL_MEM_VRAM);
	gpu_free_mapped(gxm_stencil_surface_addr, VGL_MEM_VRAM);
	
}

void startShaderPatcher(void){
	
	// Constants for shader patcher buffers
	static const unsigned int shader_patcher_buffer_size = 1024 * 1024;
	static const unsigned int shader_patcher_vertex_usse_size = 1024 * 1024;
	static const unsigned int shader_patcher_fragment_usse_size = 1024 * 1024;
	
	// Allocating Shader Patcher buffer
	gxm_shader_patcher_buffer_addr = gpu_alloc_mapped(
		shader_patcher_buffer_size, VGL_MEM_VRAM);

	// Allocating Shader Patcher vertex USSE buffer
	unsigned int shader_patcher_vertex_usse_offset;
	gxm_shader_patcher_vertex_usse_addr = gpu_vertex_usse_alloc_mapped(
		shader_patcher_vertex_usse_size, &shader_patcher_vertex_usse_offset);

	// Allocating Shader Patcher fragment USSE buffer
	unsigned int shader_patcher_fragment_usse_offset;
	gxm_shader_patcher_fragment_usse_addr = gpu_fragment_usse_alloc_mapped(
		shader_patcher_fragment_usse_size, &shader_patcher_fragment_usse_offset);

	// Populating shader patcher parameters
	SceGxmShaderPatcherParams shader_patcher_params;
	memset(&shader_patcher_params, 0, sizeof(SceGxmShaderPatcherParams));
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

void stopShaderPatcher(void){
	
	// Destroying shader patcher instance
	sceGxmShaderPatcherDestroy(gxm_shader_patcher);
	
	// Freeing shader patcher buffers
	gpu_free_mapped(gxm_shader_patcher_buffer_addr, VGL_MEM_VRAM);
	gpu_vertex_usse_free_mapped(gxm_shader_patcher_vertex_usse_addr);
	gpu_fragment_usse_free_mapped(gxm_shader_patcher_fragment_usse_addr);

}

void waitRenderingDone(void){
	
	// Wait for rendering to be finished
	sceGxmDisplayQueueFinish();
	sceGxmFinish(gxm_context);
	
}



/*
 * ------------------------------
 * - IMPLEMENTATION STARTS HERE -
 * ------------------------------
 */
 
void vglMapHeapMem(){
	
	if (!heap_mapped) {
	
		// Getting newlib heap memblock starting address
		void *addr = NULL;
		sceKernelGetMemBlockBase(_newlib_heap_memblock, &addr);
	
		// Mapping newlib heap into sceGxm
		sceGxmMapMemory(addr, _newlib_heap_size, SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE);
		heap_mapped = GL_TRUE;
	
	}
	
}
 
 void vglStartRendering(){
	 
	// Starting drawing scene
	sceGxmBeginScene(
		gxm_context, 0, gxm_render_target,
		NULL, NULL,
		gxm_sync_objects[gxm_back_buffer_index],
		&gxm_color_surfaces[gxm_back_buffer_index],
		&gxm_depth_stencil_surface);
		
	// Setting back current viewport if enabled cause sceGxm will reset it at sceGxmEndScene call
    if (scissor_test_state){
        if (viewport_mode) sceGxmSetViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
        sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_OUTSIDE, region.x , region.y, region.x + region.w, region.y + region.h);
    }else if (viewport_mode){
        sceGxmSetViewport(gxm_context, x_port, x_scale, y_port, y_scale, z_port, z_scale);
        sceGxmSetRegionClip(gxm_context, SCE_GXM_REGION_CLIP_OUTSIDE, gl_viewport.x, DISPLAY_HEIGHT - gl_viewport.y - gl_viewport.h, gl_viewport.x + gl_viewport.w, gl_viewport.y + gl_viewport.h);
	}
}
 
 void vglStopRenderingInit(){
	
	// Ending drawing scene
	sceGxmEndScene(gxm_context, NULL, NULL);
	
}

void vglStopRenderingTerm(){
	
	// Waiting GPU to complete its work
	sceGxmFinish(gxm_context);
	
	// Properly requesting a display update
	sceGxmPadHeartbeat(&gxm_color_surfaces[gxm_back_buffer_index], gxm_sync_objects[gxm_back_buffer_index]);
	struct display_queue_callback_data queue_cb_data;
	queue_cb_data.addr = gxm_color_surfaces_addr[gxm_back_buffer_index];
	sceGxmDisplayQueueAddEntry(gxm_sync_objects[gxm_front_buffer_index],
		gxm_sync_objects[gxm_back_buffer_index], &queue_cb_data);
	gxm_front_buffer_index = gxm_back_buffer_index;
	gxm_back_buffer_index = (gxm_back_buffer_index + 1) % DISPLAY_BUFFER_COUNT;
	
	// Resetting vitaGL mempool
	gpu_pool_reset();
	
}
 
 void vglStopRendering(){

	// Ending drawing scene
	vglStopRenderingInit();
	
	// Updating display and resetting vitaGL mempool
	vglStopRenderingTerm();
	
}

void vglUpdateCommonDialog(){
	
	// Populating SceCommonDialog parameters
	SceCommonDialogUpdateParam updateParam;
	memset(&updateParam, 0, sizeof(updateParam));
	updateParam.renderTarget.colorFormat    = SCE_GXM_COLOR_FORMAT_A8B8G8R8;
	updateParam.renderTarget.surfaceType    = SCE_GXM_COLOR_SURFACE_LINEAR;
	updateParam.renderTarget.width          = DISPLAY_WIDTH;
	updateParam.renderTarget.height         = DISPLAY_HEIGHT;
	updateParam.renderTarget.strideInPixels = DISPLAY_STRIDE;
	updateParam.renderTarget.colorSurfaceData = gxm_color_surfaces_addr[gxm_back_buffer_index];
	updateParam.renderTarget.depthSurfaceData = gxm_depth_surface_addr;
	updateParam.displaySyncObject = gxm_sync_objects[gxm_back_buffer_index];

	// Updating sceCommonDialog
	sceCommonDialogUpdate(&updateParam);
	
}

void glFinish(void){
	
	// Waiting for GPU to finish drawing jobs
	sceGxmFinish(gxm_context);
	
}
