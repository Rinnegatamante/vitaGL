// Drawing a quad on screen with glDrawElements

#include <vitaGL.h>

float colors[] = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0};
float vertices[] = {100, 100, 0, 150, 100, 0, 100, 150, 0, 150, 150, 0};
uint16_t indices[] = {0, 1, 2, 1, 2, 3};

int main(){
	
	// Initializing graphics device
	vglInit(0x800000);
	
	// Setting screen clear color
	glClearColor (0.50f, 0.0f, 0.0f, 1.0f);
	
	// Initializing mvp matrix with an orthogonal full screen matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Main loop
	for (;;){
		// Clearing screen
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Enabling vertex and color arrays
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		// Loading vertex and color arrays
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glColorPointer(3, GL_FLOAT, 0, colors);
		
		// Performing drawing
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
		
		// Disabling vertex and color arrays
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
}