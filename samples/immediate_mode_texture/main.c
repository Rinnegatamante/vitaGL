// Drawing a fullscreen image on screen with glBegin/glEnd
#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>

GLenum texture_format = GL_RGB;
GLuint texture = 0;

int main(){
	
	// Initializing graphics device
	vglInit(0x100000);
	
	// Loading BMP image to use as texture
	SceUID fd = sceIoOpen("app0:texture.bmp", SCE_O_RDONLY, 0777);
	uint16_t w, h;
	sceIoLseek(fd, 0x12, SCE_SEEK_SET);
	sceIoRead(fd, &w, sizeof(uint16_t));
	sceIoLseek(fd, 0x16, SCE_SEEK_SET);
	sceIoRead(fd, &h, sizeof(uint16_t));
	sceIoLseek(fd, 0x26, SCE_SEEK_SET);
	uint8_t *buffer = (uint8_t*)malloc(w * h * 3);
	sceIoRead(fd, buffer, w * h * 3);
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
	glTexImage2D(GL_TEXTURE_2D, 0, texture_format, w, h, 0, texture_format, GL_UNSIGNED_BYTE, buffer);
	
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
		
		// Drawing the texture with immediate mode (Note: BMP images are vertically flipped)
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex3f(0, 0, 0);
		glTexCoord2i(1, 1);
		glVertex3f(960, 0, 0);
		glTexCoord2i(1, 0);
		glVertex3f(960, 544, 0);
		glTexCoord2i(0, 0);
		glVertex3f(0, 544, 0);
		glEnd();
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
	
}