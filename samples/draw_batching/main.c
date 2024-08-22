#include <vitaGL.h>
#include <string.h>

typedef struct {
	int elements_count;
	float *array;
} fan_s;


GLuint vbo_id;
#define VBO_SIZE 4096

static void setup_vbo(void) {
	glGenBuffers(1, &vbo_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, VBO_SIZE, NULL, GL_DYNAMIC_DRAW);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);
}

static void draw_fans(const fan_s *fans, int num_fans) {
#define MAX_FANS 1024
	GLint fans_starts[MAX_FANS];
	GLsizei fans_sizes[MAX_FANS];

	float *mapping = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	int i;
	int num_elements_total = 0;
	for(i = 0; i != num_fans; ++i) {
		memcpy(mapping, fans[i].array, fans[i].elements_count * 3 * sizeof(float));
		mapping += fans[i].elements_count * 3;

		fans_starts[i] = num_elements_total;
		fans_sizes[i] = fans[i].elements_count;

		num_elements_total += fans[i].elements_count;
	}

	glUnmapBuffer(GL_ARRAY_BUFFER);

	glMultiDrawArrays(GL_TRIANGLE_FAN, fans_starts, fans_sizes, num_fans);
}

static void display(void) {
	glClear(GL_COLOR_BUFFER_BIT);

	float fan0_data[] = {
		0.0, 0.0, 0.0,
		0.0, 100.0, 0.0,
		100.0, 100.0, 0.0,
		100.0, 0.0, 0.0
	};
	float fan1_data[] = {
		200.0, 0.0, 0.0,
		200.0, 100.0, 0.0,
		300.0, 100.0, 0.0,
		300.0, 0.0, 0.0
	};
	fan_s fans[] = {
		4, fan0_data,
		4, fan1_data
	};
	draw_fans(fans, 2);

	vglSwapBuffers(GL_FALSE);
}

static void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int main(int argc, char **argv) {
	// Initializing graphics device
	vglInit(0x800000);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	
	reshape(960, 544);

	setup_vbo();

	for (;;) {
		display();
	}

	return 0;
}