#include <vitasdk.h>
#include <vitaGL.h>
#include <libtoloader.h>

int main() {
	// Initializing graphics device
	vglInitExtended(0, 960, 544, 4 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);

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
	glViewport(0, 0, 960, 544);
	
	// Loading 3D model
	to_model bunny;
	to_loadObj("app0:bunny.obj", &bunny);
	
	// Create a fullscreen fbo with an attached texture to draw on
	GLuint fbo_tex, fbo;
	glGenTextures(1, &fbo_tex);
	glBindTexture(GL_TEXTURE_2D, fbo_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 960, 544, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo_tex, 0);
			
	// Main loop
	for (;;) {
		// Clear color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Drawing our model with vertex arrays
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, bunny.pos);
		glRotatef(1.0f, 0.0f, 0.0f, 1.0f); // Rotating model at each frame by 1 on axis x and axis w
		glRotatef(0.5f, 0.0f, 1.0f, 0.0f); // Rotating model at each frame by 0.5 on axis x and 1.0 on axis z
		glDrawArrays(GL_TRIANGLES, 0, bunny.num_vertices);
		glDisableClientState(GL_VERTEX_ARRAY);
		
		// Blit part of the framebuffer on screen stretched to fullscreen
		glBlitNamedFramebuffer(fbo, 0, 200, 50, 760, 494, 0, 0, 960, 544, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
}
