// Drawing a rotating cube with VBO
#include <vitaGL.h>
#include <math.h>

// Helper macro to get offset in a VBO for an element without having compilation warnings
#define BUF_OFFS(i) ((void*)(i))

float colors[] = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0}; // Colors for a face
float vertices_front[] = {-0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f}; // Front Face
float vertices_back[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}; // Back Face
float vertices_left[] = {-0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f}; // Left Face
float vertices_right[] = {0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f}; // Right Face
float vertices_top[] = {-0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f}; // Top Face
float vertices_bottom[] = {-0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}; // Bottom Face

// Buffers used for EBO and VBO
GLuint buffers[2];

uint16_t indices[] = {
	 0, 1, 2, 1, 2, 3, // Front
	 4, 5, 6, 5, 6, 7, // Back
	 8, 9,10, 9,10,11, // Left
	12,13,14,13,14,15, // Right
	16,17,18,17,18,19, // Top
	20,21,22,21,22,23  // Bottom
};

int main(){
	// Initializing graphics device
	vglInit(0x80000);

	// Enabling V-Sync
	vglWaitVblankStart(GL_TRUE);
	
	// Creating VBO data with vertices + colors
	float vbo[12*12];
	memcpy(&vbo[12*0], &vertices_front[0], sizeof(float) * 12);
	memcpy(&vbo[12*1], &vertices_back[0], sizeof(float) * 12);
	memcpy(&vbo[12*2], &vertices_left[0], sizeof(float) * 12);
	memcpy(&vbo[12*3], &vertices_right[0], sizeof(float) * 12);
	memcpy(&vbo[12*4], &vertices_top[0], sizeof(float) * 12);
	memcpy(&vbo[12*5], &vertices_bottom[0], sizeof(float) * 12);
	memcpy(&vbo[12*6], &colors[0], sizeof(float) * 12);
	memcpy(&vbo[12*7], &colors[0], sizeof(float) * 12);
	memcpy(&vbo[12*8], &colors[0], sizeof(float) * 12);
	memcpy(&vbo[12*9], &colors[0], sizeof(float) * 12);
	memcpy(&vbo[12*10], &colors[0], sizeof(float) * 12);
	memcpy(&vbo[12*11], &colors[0], sizeof(float) * 12);
	
	// Creating two buffers for colors, vertices and indices
	glGenBuffers(2, buffers);
	
	// Setting up VBO
	glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12 * 12, vbo, GL_STATIC_DRAW);
	
	// Setting up EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * 6 * 6, indices, GL_STATIC_DRAW);
	
	// Setting clear color
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	
	// Initializing mvp matrix with a perspective full screen matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0f, 960.f/544.0f, 0.01f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -3.0f); // Centering the cube

	// Enabling depth test
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	// Main loop
	for (;;) {
		// Clear color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Drawing our cube with VBO
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, BUF_OFFS(0));
		glColorPointer(3, GL_FLOAT, 0, BUF_OFFS(12*6*sizeof(float)));
		glRotatef(1.0f, 0.0f, 0.0f, 1.0f); // Rotating cube at each frame by 1 on axis x and axis w
		glRotatef(0.5f, 0.0f, 1.0f, 0.0f); // Rotating cube at each frame by 0.5 on axis x and 1.0 on axis z
		glDrawElements(GL_TRIANGLES, 6*6, GL_UNSIGNED_SHORT, BUF_OFFS(0));
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
}