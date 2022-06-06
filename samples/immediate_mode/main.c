// Drawing a colored quad with glBegin/glEnd

#include <vitaGL.h>

int main(){
	
	// Initializing graphics device
	vglInit(0x800000);
	
	// Setting screen clear color
	glClearColor(0.0, 0.0, 0.0, 0.0);
	
	// Initializing mvp matrix with an orthogonal full screen matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Main loop
	for (;;) {
		// Clearing screen
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Drawing a quad with immediate mode
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
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
}
