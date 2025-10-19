#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>

// this provides an RGBA texture, defined as variables
// texture_width, texture_height and texture_rgba[]
#include "texture.h"

// Default state for our light 1
static const GLfloat li_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
static const GLfloat li_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat li_position[] = { 0.0f, 0.0f, 2.0f, 0.0f };

static void load_textures(GLuint *texture)
{
	glGenTextures(3, texture); /* create three textures */
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_rgba);
	/* use no filtering */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	/* the second texture */
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_rgba);
	/* use linear filtering */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	/* the third texture */
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_rgba);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Macro to check if a button has been pressed
#define CHECK_BTN(x) ((pad.buttons & x) && (!(old_buttons & x)))

int main(){
	// Initializing graphics device
	vglInit(0x100000);
	
	// Setting clear color
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	
	// Enabling depth test, texturing, culling and lighting
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
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
	
	// Loading textures for our cube
	GLuint textures[3];
	load_textures(textures);
	
	// Default values for speed, rotation speed, depth and texture in use
	GLfloat spd_x = 0.5f;
	GLfloat spd_y = 0.5f;
	GLfloat rot_x = 0.f;
	GLfloat rot_y = 0.f;
	GLfloat z = -5.f;
	GLuint filter = 0;
	
	// Default values for lighting state, fogging state and pressed buttons bitmask
	uint32_t old_buttons = 0;
	GLboolean light = GL_TRUE;
	GLboolean fog = GL_FALSE;
	
	// Main loop
	for (;;) {
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
		
		// Checking for texture changes requests
		if (CHECK_BTN(SCE_CTRL_TRIANGLE)) {
			filter = (filter + 1) % 3;
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

		// Binding texture to use for the draw
		glBindTexture(GL_TEXTURE_2D, textures[filter]);  /* select our texture */

		// Setting default color used for lighting
		glColor4f(1.f, 1.f, 1.f, 0.5f);

		// Drawing a cube with immediate mode
		glBegin(GL_QUADS);
			/* front face */
			glNormal3f(0.0f, 0.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, -1.0f, 1.0f); 
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, -1.0f, 1.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.0f, 1.0f, 1.0f);
			/* back face */
			glNormal3f(0.0f, 0.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(-1.0f, -1.0f, -1.0f); 
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(-1.0f, 1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(1.0f, 1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(1.0f, -1.0f, -1.0f);
			/* right face */
			glNormal3f(1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, -1.0f, -1.0f); 
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, 1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(1.0f, -1.0f, 1.0f);
			/* left face */
			glNormal3f(-1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(-1.0f, -1.0f, 1.0f); 
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(-1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.0f, 1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, -1.0f, -1.0f);
			/* top face */
			glNormal3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, 1.0f, 1.0f); 
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, 1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.0f, 1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, 1.0f, 1.0f);
			/* bottom face */
			glNormal3f(0.0f, -1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3f(1.0f, -1.0f, -1.0f); 
			glTexCoord2f(1.0f, 1.0f);
			glVertex3f(1.0f, -1.0f, 1.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3f(-1.0f, -1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3f(-1.0f, -1.0f, -1.0f);
		glEnd();
		
		// Change the rotation angles
		rot_x += spd_x;
		rot_y += spd_y;
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
}