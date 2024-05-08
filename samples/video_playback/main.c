// Playing a video using sceAvPlayer
#include <vitasdk.h>
#include <vitaGL.h>
#include <math.h>

#define SCREEN_W 960
#define SCREEN_H 544

#define VIDEO_BUFFERS 5 // Number of consecutive frames to process during video playback, ensure it's at least 4 with triple buffering or 3 with double buffering
#define PHYCONT_MEM_ALIGNMENT (1024 * 1024) // Required memory alignment for physically contiguous memblocks in bytes
#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

// Callbacks required by sceAvPlayer for GPU and non-GPU accessible memory internal usage
void *alloc_for_cpu(void *p, uint32_t align, uint32_t size) {
	return memalign(align, size);
}
void free_for_cpu(void *p, void *ptr) {
	free(ptr);
}
void *alloc_for_gpu(void *p, uint32_t align, uint32_t size) {
	// Aligning size to required phycont requirements
	size = ALIGN_MEM(size, PHYCONT_MEM_ALIGNMENT);
	
	// Allocating a new memblock of the required size
	SceUID blk = sceKernelAllocMemBlock("av_blk", SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW, size, NULL);
	if (blk < 0)
		return NULL;
	
	// Mapping it as read/write for GPU usage
	void *res;
	sceKernelGetMemBlockBase(blk, &res);
	sceGxmMapMemory(res, size, SCE_GXM_MEMORY_ATTRIB_RW);
	
	return res;
}
void free_for_gpu(void *p, void *addr) {
	// Ensuring GPU finished rendering prior deleting GPU mapped memory
	glFinish();
	
	SceUID blk = sceKernelFindMemBlockByAddr(addr, 0);
	sceGxmUnmapMemory(addr);
	sceKernelFreeMemBlock(blk);
}

// State for our video player
enum {
	PLAYER_INACTIVE,
	PLAYER_ACTIVE,
	PLAYER_PAUSED
};
volatile int movie_player_state = PLAYER_INACTIVE;
SceAvPlayerHandle movie_player;

// Audio thread
int audio_thread(SceSize args, void *argp) {
	// Open an audio port for audio playback
	int audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, 1024, 48000, SCE_AUDIO_OUT_MODE_STEREO);
	
	// Loop until video is playing
	while (movie_player_state != PLAYER_INACTIVE) {
		// If there's new data to playback, play it, reschedule thread if not
		if (sceAvPlayerIsActive(movie_player)) {
			SceAvPlayerFrameInfo frame;
			if (sceAvPlayerGetAudioData(movie_player, &frame)) {
				sceAudioOutSetConfig(audio_port, 1024, frame.details.audio.sampleRate, frame.details.audio.channelCount == 1 ? SCE_AUDIO_OUT_MODE_MONO : SCE_AUDIO_OUT_MODE_STEREO);
				sceAudioOutOutput(audio_port, frame.pData);
			} else {
				sceKernelDelayThread(1000);
			}
		} else {
			sceKernelDelayThread(1000);
		}
	}
	
	return sceKernelExitDeleteThread(0);
}

int main(){
	// Initializing graphics device (Note: we leave physically contiguous memory unused so that we can use it in sceAvPlayer)
	vglInitWithCustomThreshold(0, SCREEN_W, SCREEN_H, 4 * 1024 * 1024, 0, 32 * 1024 * 1024, 0, SCE_GXM_MULTISAMPLE_NONE);
	
	// Initializing sceAvPlayer
	sceSysmoduleLoadModule(SCE_SYSMODULE_AVPLAYER);
	SceAvPlayerInitData playerInit;
	memset(&playerInit, 0, sizeof(SceAvPlayerInitData));
	playerInit.memoryReplacement.allocate = alloc_for_cpu;
	playerInit.memoryReplacement.deallocate = free_for_cpu;
	playerInit.memoryReplacement.allocateTexture = alloc_for_gpu;
	playerInit.memoryReplacement.deallocateTexture = free_for_gpu;
	playerInit.basePriority = 0xA0;
	playerInit.numOutputVideoFrameBuffers = VIDEO_BUFFERS;
	playerInit.autoStart = GL_TRUE;
	movie_player = sceAvPlayerInit(&playerInit);
	movie_player_state = PLAYER_ACTIVE;
	
	// Allocating required textures to handle processed video frames
	SceGxmTexture *movie_frames_gxm_tex[VIDEO_BUFFERS];
	GLuint movie_frames_tex[VIDEO_BUFFERS];
	glGenTextures(VIDEO_BUFFERS, movie_frames_tex);
	for (int i = 0; i < VIDEO_BUFFERS; i++) {
		// Init every frame texture to blank textures
		glBindTexture(GL_TEXTURE_2D, movie_frames_tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		
		// Internally free texture data since we're going to replace the data pointer later with what sceAvPlayer will provide us
		movie_frames_gxm_tex[i] = vglGetGxmTexture(GL_TEXTURE_2D);
		vglFree(vglGetTexDataPointer(GL_TEXTURE_2D));
	}
	
	// Allocating and filling attributes for the video frame draw (NOTE: We use malloc since newlib memory is automatically GPU mapped by vitaGL)
	float *draw_attributes = (float *)malloc(sizeof(float) * 22);
	draw_attributes[0] = 0.0f;
	draw_attributes[1] = 0.0f;
	draw_attributes[2] = 0.0f;
	draw_attributes[3] = 960.0f;
	draw_attributes[4] = 0.0f;
	draw_attributes[5] = 0.0f;
	draw_attributes[6] = 0.0f;
	draw_attributes[7] = 544.0f;
	draw_attributes[8] = 0.0f;
	draw_attributes[9] = 960.0f;
	draw_attributes[10] = 544.0f;
	draw_attributes[11] = 0.0f;
	vglVertexPointerMapped(3, draw_attributes);
	draw_attributes[12] = 0.0f;
	draw_attributes[13] = 0.0f;
	draw_attributes[14] = 1.0f;
	draw_attributes[15] = 0.0f;
	draw_attributes[16] = 0.0f;
	draw_attributes[17] = 1.0f;
	draw_attributes[18] = 1.0f;
	draw_attributes[19] = 1.0f;
	vglTexCoordPointerMapped(&draw_attributes[12]);
	uint16_t *draw_indices = (uint16_t*)&draw_attributes[20];
	draw_indices[0] = 0;
	draw_indices[1] = 1;
	draw_indices[2] = 2;
	draw_indices[3] = 3;
	vglIndexPointerMapped(draw_indices);
	
	// Setting renderer state
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	// Start a thread to handle audio playback
	SceUID audio_thid = sceKernelCreateThread("video audio playback", audio_thread, 0x10000100 - 10, 0x4000, 0, 0, NULL);
	sceKernelStartThread(audio_thid, 0, NULL);
	
	// Adding the video file to our video player instance (this will also trigger the start of the video playback since we set autoStart to GL_TRUE)
	sceAvPlayerAddSource(movie_player, "app0:video.mp4");
	sceAvPlayerSetLooping(movie_player, GL_TRUE);
	
	// Main loop
	int movie_frame_idx = 0;
	int movie_first_frame_decoded = GL_FALSE;
	for (;;) {
		// Resetting wvp matrix
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof(0, SCREEN_W, SCREEN_H, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		// Checking if we want to pause/unpause the video playback
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		static uint32_t oldpad = 0;
		if (pad.buttons & SCE_CTRL_CROSS && !(oldpad & SCE_CTRL_CROSS)) {
			if (movie_player_state != PLAYER_PAUSED) {
				sceAvPlayerPause(movie_player);
				movie_player_state = PLAYER_PAUSED;
			} else {
				sceAvPlayerResume(movie_player);
				movie_player_state = PLAYER_ACTIVE;
			}
		}
		oldpad = pad.buttons;
		
		// Get the frame to draw from sceAvPlayer
		if (movie_player_state == PLAYER_ACTIVE) {
			if (sceAvPlayerIsActive(movie_player)) {
				SceAvPlayerFrameInfo frame;
				// Check if we have new decoded frames
				if (sceAvPlayerGetVideoData(movie_player, &frame)) {
					// Increase current frame index
					movie_frame_idx = (movie_frame_idx + 1) % VIDEO_BUFFERS;
					
					// Init internal sceGxmTexture with data reported by sceAvPlayer
					sceGxmTextureInitLinear(movie_frames_gxm_tex[movie_frame_idx], frame.pData, SCE_GXM_TEXTURE_FORMAT_YVU420P2_CSC1, frame.details.video.width, frame.details.video.height, 0);
					
					// Set up bilinear filtering for better quality
					sceGxmTextureSetMinFilter(movie_frames_gxm_tex[movie_frame_idx], SCE_GXM_TEXTURE_FILTER_LINEAR);
					sceGxmTextureSetMagFilter(movie_frames_gxm_tex[movie_frame_idx], SCE_GXM_TEXTURE_FILTER_LINEAR);
					
					// Report that we decoded at least one frame
					movie_first_frame_decoded = GL_TRUE;
				}
			} else {
				// sceAvPlayer can take some time to actually start the video playback, so we ensure we're not waiting for video playback to actually start
				if (movie_first_frame_decoded) {
					// If sceAvPlayer is not active, we decoded the first frame and the player state is not paused, it means the video playback finished
					if (movie_player_state == PLAYER_ACTIVE) {
						sceAvPlayerStop(movie_player);
						sceAvPlayerClose(movie_player);
						movie_player_state = PLAYER_INACTIVE;
						break;
					}
				}	
			}
		}
		
		// Drawing the last decoded video frame if we decoded at least one frame
		if (movie_first_frame_decoded) {
			glBindTexture(GL_TEXTURE_2D, movie_frames_tex[movie_frame_idx]);
			vglDrawObjects(GL_TRIANGLE_STRIP, 4, GL_TRUE);
		}
		
		// Performing display swap
		vglSwapBuffers(GL_FALSE);
	}
}
