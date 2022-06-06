#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>

// Default state for our light 1
static const GLfloat li_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
static const GLfloat li_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat li_position[] = { 0.5f, 1.0f, 1.0f, 0.0f };

// Function to normalize a vector of 3 floats
inline void Normalize3(GLfloat *v) {
	GLfloat len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= len;
	v[1] /= len;
	v[2] /= len;
}

// Draws a sphere face
void SphereFace(int p_recurse, GLfloat p_radius, GLfloat *a, GLfloat *b, GLfloat *c) {
	if(p_recurse > 1) {
		// Compute vectors halfway between the passed vectors 
		GLfloat d[3] = {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
		GLfloat e[3] = {b[0] + c[0], b[1] + c[1], b[2] + c[2]};
		GLfloat f[3] = {c[0] + a[0], c[1] + a[1], c[2] + a[2]};

		Normalize3(d);
		Normalize3(e);
		Normalize3(f);

		SphereFace(p_recurse-1, p_radius, a, d, f);
		SphereFace(p_recurse-1, p_radius, d, b, e);
		SphereFace(p_recurse-1, p_radius, f, e, c);
		SphereFace(p_recurse-1, p_radius, f, d, e);
	}

	glBegin(GL_TRIANGLES);
	glNormal3fv(a);
	glVertex3f(a[0] * p_radius, a[1] * p_radius, a[2] * p_radius);
	glNormal3fv(b);
	glVertex3f(b[0] * p_radius, b[1] * p_radius, b[2] * p_radius);
	glNormal3fv(c);
	glVertex3f(c[0] * p_radius, c[1] * p_radius, c[2] * p_radius);
	glEnd();
}

// Draws a sphere
void Sphere(GLfloat p_radius) {
   GLfloat a[] = {1, 0, 0};
   GLfloat b[] = {0, 0, -1};
   GLfloat c[] = {-1, 0, 0};
   GLfloat d[] = {0, 0, 1};
   GLfloat e[] = {0, 1, 0};
   GLfloat f[] = {0, -1, 0};
   
   // Increase this value to make the sphere more rounded but more computationally expensive
   int recurse = 5;

   SphereFace(recurse, p_radius, d, a, e);
   SphereFace(recurse, p_radius, a, b, e);
   SphereFace(recurse, p_radius, b, c, e);
   SphereFace(recurse, p_radius, c, d, e);
   SphereFace(recurse, p_radius, a, d, f);
   SphereFace(recurse, p_radius, b, a, f);
   SphereFace(recurse, p_radius, c, b, f);
   SphereFace(recurse, p_radius, d, c, f);
}

// Macro to check if a button has been pressed
#define CHECK_BTN(x) ((pad.buttons & x) && (!(old_buttons & x)))

// Current state for phong lighting usage
GLboolean phong = GL_FALSE;

int main(){
	// Initializing graphics device
	vglInit(0x100000);
	
	// Setting clear color
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	
	// Enabling depth test, culling and lighting
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glDepthFunc(GL_LEQUAL);
	
	// Enabling light 0 with default values
	glEnable(GL_LIGHT0);
	
	// Enabling light 1 with custom values
	glLightfv(GL_LIGHT1, GL_AMBIENT, li_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, li_diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, li_position);
	glEnable(GL_LIGHT1);
	
	// Configuring initial state for fogging
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogf(GL_FOG_DENSITY, 0.8f);
	
	// Initializing mvp matrix with a perspective full screen matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, 960.0 / 544.0, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	
	// Default values for speed, rotation speed and depth
	GLfloat spd_x = 0.5f;
	GLfloat spd_y = 0.5f;
	GLfloat rot_x = 0.f;
	GLfloat rot_y = 0.f;
	GLfloat z = -15.f;

	// Default values for lighting state, fogging state and pressed buttons bitmask
	uint32_t old_buttons = 0;
	GLboolean light = GL_TRUE;
	GLboolean fog = GL_FALSE;
	
	// Main loop
	for (;;){
		// Reading inputs
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		
		// Checking for lighting state changes requests
		if (CHECK_BTN(SCE_CTRL_CROSS)) {
			if (light)
				glDisable(GL_LIGHTING);
			else
				glEnable(GL_LIGHTING);
			light = !light;
		}
		
		// Checking for fogging state changes requests
		if (CHECK_BTN(SCE_CTRL_SQUARE)) {
			if (fog)
				glDisable(GL_FOG);
			else
				glEnable(GL_FOG);
			fog = !fog;
		}
		
		// Checking for illumination models changes requests
		if (CHECK_BTN(SCE_CTRL_CIRCLE)) {
			if (!phong)
				glShadeModel(GL_PHONG_WIN);
			else
				glShadeModel(GL_SMOOTH);
			phong = !phong;
		}
		
		// Pushing model far
		if (pad.buttons & SCE_CTRL_UP) {
			z -= 0.1f;
		}
		
		// Pulling model near
		if (pad.buttons & SCE_CTRL_DOWN) {
			z += 0.1f;
		}
		
		// Saving current input bitmask state to previous frame one
		old_buttons = pad.buttons;
	
		// Clearing color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	 
		// Rotating and translating accordingly the modelview
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, z);
		glRotatef(rot_x, 1.0f, 0.0f, 0.0f); /* rotate on the X axis */
		glRotatef(rot_y, 0.0f, 1.0f, 0.0f); /* rotate on the Y axis */

		// Drawing a sphere with radius 3
		Sphere(3);
		
		// Change the rotation angles
		rot_x += spd_x;
		rot_y += spd_y;
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
	
	// Terminating graphics device
	vglEnd();
}