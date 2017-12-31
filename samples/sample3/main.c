// Drawing a colored quad with glBegin/glEnd

#include <vitaGL.h>

GLint nofcolors = 4;
GLenum texture_format = GL_VITA2D_TEXTURE;
GLuint texture = 0;

int main(){
	
	// Initializing graphics device
	vita2d_init_advanced(0x800000);
	vita2d_set_vblank_wait(0);
	
	
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	for (;;){
		glClear(GL_COLOR_BUFFER_BIT);
		glBegin(GL_QUADS);
		glColor3f(1.0, 0.0, 0.0);
		glVertex3f(400, 0, 0);
		glColor3f(1.0, 1.0, 0.0);
		glVertex3f(800, 0, 0);
		glColor3f(0.0, 1.0, 0.0);
		glVertex3f(800, 400, 0);
		glColor3f(1.0, 0.0, 1.0);
		glVertex3f(400, 400, 0);
		glEnd();
		glLoadIdentity();
	}
	
	vita2d_fini();
	
}