// Drawing a fullscreen image on screen with glBegin/glEnd
#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>

GLenum texture_format = GL_YUV420P_BT601_VGL;
GLuint texture = 0;

int main(){
	
	// Initializing graphics device
	vglInit(0x100000);
	
	// Loading YUV420p image to use as texture
	SceUID fd = sceIoOpen("app0:texture.yuv", SCE_O_RDONLY, 0777);
	uint16_t w = 960, h = 544;
	uint8_t *buffer = (uint8_t*)malloc((w * h * 3) / 2);
	sceIoRead(fd, buffer, (w * h * 3) / 2);
	sceIoClose(fd);
	
	// Setting screen clear color
	glClearColor(0.50, 0, 0, 0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Initializing an openGL texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	// Uploading the loaded image as texture
	glCompressedTexImage2D(GL_TEXTURE_2D, 0, texture_format, w, h, 0, (w * h * 3) / 2, buffer);
	
	// Enabling linear filtering on the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Enabling texturing
	glEnable(GL_TEXTURE_2D);
	
	// Main loop
	for (;;) {
		// Clearing screen
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Binding texture to use for the draw
		glBindTexture(GL_TEXTURE_2D, texture);
		
		// Drawing the texture with immediate mode
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex3f(0, 0, 0);
		glTexCoord2i(1, 0);
		glVertex3f(960, 0, 0);
		glTexCoord2i(1, 1);
		glVertex3f(960, 544, 0);
		glTexCoord2i(0, 1);
		glVertex3f(0, 544, 0);
		glEnd();
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
	
}