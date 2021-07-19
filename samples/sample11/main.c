#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>

static const GLfloat li_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
static const GLfloat li_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat li_position[] = { 0.5f, 1.0f, 1.0f, 0.0f };

inline void Normalize3(GLfloat *v) {
	GLfloat len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= len;
	v[1] /= len;
	v[2] /= len;
}

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

void Sphere(GLfloat p_radius) {
   GLfloat a[] = {1, 0, 0};
   GLfloat b[] = {0, 0, -1};
   GLfloat c[] = {-1, 0, 0};
   GLfloat d[] = {0, 0, 1};
   GLfloat e[] = {0, 1, 0};
   GLfloat f[] = {0, -1, 0};
   
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

#define CHECK_BTN(x) ((pad.buttons & x) && (!(old_buttons & x)))
GLboolean phong = GL_FALSE;

int main(){
	
	// Initializing graphics device
	vglInit(0x100000);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glDepthFunc(GL_LEQUAL);
	
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT1, GL_AMBIENT, li_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, li_diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, li_position);
	glEnable(GL_LIGHT1);
	
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogf(GL_FOG_DENSITY, 0.8f);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, 960.0 / 544.0, 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	
	GLfloat spd_x = 0.5f;
	GLfloat spd_y = 0.5f;
	GLfloat rot_x = 0.f;
	GLfloat rot_y = 0.f;
	GLfloat z = -15.f;
	GLuint filter = 0;
	
	uint32_t old_buttons = 0;
	GLboolean light = GL_TRUE;
	GLboolean fog = GL_FALSE;
	
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
		
		if (CHECK_BTN(SCE_CTRL_SQUARE)) {
			if (fog)
				glDisable(GL_FOG);
			else
				glEnable(GL_FOG);
			fog = !fog;
		}
		
		if (CHECK_BTN(SCE_CTRL_CIRCLE)) {
			if (!phong)
				glShadeModel(GL_PHONG_WIN);
			else
				glShadeModel(GL_SMOOTH);
			phong = !phong;
		}
		
		if (pad.buttons & SCE_CTRL_UP) {
			z -= 0.1f;
		}
		
		if (pad.buttons & SCE_CTRL_DOWN) {
			z += 0.1f;
		}
		
		old_buttons = pad.buttons;
	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	 
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, z);
		glRotatef(rot_x, 1.0f, 0.0f, 0.0f); /* rotate on the X axis */
		glRotatef(rot_y, 0.0f, 1.0f, 0.0f); /* rotate on the Y axis */

		Sphere(3);
		
		/* change the rotation angles */
		rot_x += spd_x;
		rot_y += spd_y;
		
		vglSwapBuffers(GL_FALSE);
	
	}
	vglEnd();
	
}