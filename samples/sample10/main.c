// Drawing a fullscreen image on screen with glBegin/glEnd

#include <vitaGL.h>
#include <vita2d.h>
#include <stdlib.h>

// this provides an RGBA texutre, defined as variables
// texture_width, texture_height and texture_rgba[]
#include "texture.h"

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

#define CHECK_BTN(x) ((pad.buttons & x) && (!(old_buttons & x)))

int main(){
	
	// Initializing graphics device
	vglInit(0x100000);
	
	glClearColor(1.0f, 1.0f, 0.0f, 1.f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glDepthFunc(GL_LEQUAL);
	
	glLightfv(GL_LIGHT0, GL_AMBIENT, li_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, li_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, li_position);
	glEnable(GL_LIGHT0);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, 960.0 / 544.0, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	
	GLuint textures[3];
	load_textures(textures);
	
	GLfloat spd_x = 0.5f;
	GLfloat spd_y = 0.5f;
	GLfloat rot_x = 0.f;
	GLfloat rot_y = 0.f;
	GLfloat z = -5.f;
	GLuint filter = 0;
	
	uint32_t old_buttons = 0;
	GLboolean light = GL_TRUE;
	GLboolean blend = GL_FALSE;
	
	for (;;){
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		
		if (CHECK_BTN(SCE_CTRL_CROSS)) {
			if (light)
				glDisable(GL_LIGHTING);
			else
				glEnable(GL_LIGHTING);
			light = !light;
		}
		
		if (CHECK_BTN(SCE_CTRL_CIRCLE)) {
			if (blend)
				glDisable(GL_BLEND);
			else
				glEnable(GL_BLEND);
			blend = !blend;
		}
		
		old_buttons = pad.buttons;
	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	 
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, z);
		glRotatef(rot_x, 1.0f, 0.0f, 0.0f); /* rotate on the X axis */
		glRotatef(rot_y, 0.0f, 1.0f, 0.0f); /* rotate on the Y axis */

		glBindTexture(GL_TEXTURE_2D, textures[filter]);  /* select our texture */

		glColor4f(1.f, 1.f, 1.f, 0.5f);

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
		
		/* change the rotation angles */
		rot_x += spd_x;
		rot_y += spd_y;
		
		vglSwapBuffers(GL_FALSE);
	
	}
	vglEnd();
	
}