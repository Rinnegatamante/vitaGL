// Drawing a fullscreen image on screen with glBegin/glEnd

#include <vitaGL.h>

GLint nofcolors = 4;
GLenum texture_format = GL_VITA2D_TEXTURE;
GLuint texture = 0;

int main(){
	
	// Initializing graphics device
	vglInit(0x800000);
	
	// Loading image to use as texture
	vita2d_texture* v2d_texture = vita2d_load_PNG_file("app0:texture.png");
	
	glClearColor(0.50, 0, 0, 0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Initializing openGL texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	int w = vita2d_texture_get_width(v2d_texture);
	int h = vita2d_texture_get_height(v2d_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, nofcolors, w, h, 0, texture_format, GL_UNSIGNED_BYTE, vita2d_texture_get_datap(v2d_texture));
	
	for (;;){
		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
		glLoadIdentity();
	}
	
	vglEnd();
	
}