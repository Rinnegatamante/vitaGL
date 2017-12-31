// Drawing a triangle on screen with vertex array

#include <vitaGL.h>

float vertices[] = {100, 100, 0, 150, 100, 0, 100, 150, 0};

int main(){
	
	// Initializing graphics device
	vita2d_init_advanced(0x800000);
	vita2d_set_vblank_wait(0);
	
	glClearColor (050.0f, 0.0f, 0.0f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	for (;;){
		glClear(GL_COLOR_BUFFER_BIT);
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableClientState(GL_VERTEX_ARRAY);
		
		glLoadIdentity();
	}
	
	vita2d_fini();
	
}