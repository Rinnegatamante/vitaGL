#include <vitasdk.h>
#include <vitaGL.h>
#include <tiniest_obj_loader.h>

int main() {
	// Initializing graphics device
	vglInit(0x800000);

	// Setting screen clear color
	glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

	// Enabling depth test
	glEnable(GL_DEPTH_TEST);
	
	// Initializing mvp matrix with a perspective full screen matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0f, 960.f/544.0f, 0.01f, 10000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(0.3f, 0.3f, 0.3f);
	glTranslatef(0.0f, 0.0f, -10.0f);
	
	to_model bunny;
	to_loadObj("app0:bunny.obj", &bunny);
	
	// Main loop
	for (;;){
		// Clear color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Drawing our cube with vertex arrays
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, bunny.pos);
		glRotatef(1.0f, 0.0f, 0.0f, 1.0f); // Rotating model at each frame by 1 on axis x and axis w
		glRotatef(0.5f, 0.0f, 1.0f, 0.0f); // Rotating model at each frame by 0.5 on axis x and 1.0 on axis z
		glDrawArrays(GL_TRIANGLES, 0, bunny.num_vertices);
		glDisableClientState(GL_VERTEX_ARRAY);
		
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
}
