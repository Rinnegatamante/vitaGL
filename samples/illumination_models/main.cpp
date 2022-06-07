#include <vector>
#include <stdio.h>
#include <vitasdk.h>
#include <vitaGL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <libtoloader.h>

// Number of shader sets available
#define SHADERS_NUM 4

// Macro to check if a button has been pressed
#define CHECK_BTN(x) ((pad.buttons & x) && (!(old_buttons & x)))

// Position of our point light source
glm::vec3 lightPos0 = glm::vec3(5.0f, 10.0f, 10.0f);

// Setup for our point light source
GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};
GLfloat specularColor[] = {1.0f,1.0f,1.0f};
GLfloat ambientColor[] = {0.1f,0.1f,0.1f};

// Weights for the diffusive, specular and ambient components
GLfloat Kd = 0.5f;
GLfloat Ks = 0.4f;
GLfloat Ka = 0.1f;

// Shininess coefficient for Phong and Blinn-Phong illumination models
GLfloat shininess = 25.0f;

// Roughness index for FDG illumination model
GLfloat roughness = 0.2f;

// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// Available illumination models
enum {
	LAMBERTIAN,
	PHONG,
	BLINN_PHONG,
	FDG
};

// Shaders and programs
GLuint vshaders[SHADERS_NUM];
GLuint fshaders[SHADERS_NUM];
GLuint programs[SHADERS_NUM];

// Uniforms locations
GLint modelMatrixLoc[SHADERS_NUM];
GLint viewMatrixLoc[SHADERS_NUM];
GLint projectionMatrixLoc[SHADERS_NUM];
GLint normalMatrixLoc[SHADERS_NUM];
GLint pointLightPositionLoc[SHADERS_NUM];
GLint shininessLoc[SHADERS_NUM];
GLint KaLoc[SHADERS_NUM];
GLint KdLoc[SHADERS_NUM];
GLint KsLoc[SHADERS_NUM];
GLint ambientColorLoc[SHADERS_NUM];
GLint diffuseColorLoc[SHADERS_NUM];
GLint specularColorLoc[SHADERS_NUM];
GLint alphaLoc[SHADERS_NUM];
GLint F0Loc[SHADERS_NUM];

// Initialize sceMsgDialog widget with a given message text
int init_msg_dialog(const char *msg) {
	SceMsgDialogUserMessageParam msg_param;
	memset(&msg_param, 0, sizeof(msg_param));
	msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
	msg_param.msg = (SceChar8 *)msg;

	SceMsgDialogParam param;
	sceMsgDialogParamInit(&param);
	_sceCommonDialogSetMagicNumber(&param.commonParam);
	param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
	param.userMsgParam = &msg_param;

	return sceMsgDialogInit(&param);
}

// Gets current state for sceMsgDialog running widget
int get_msg_dialog_result(void) {
	if (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
		return 0;
	sceMsgDialogTerm();
	return 1;
}

// Draws an error message on screen and force closes the app after user input
void fatal_error(const char *fmt, ...) {
	va_list list;
	char string[512];

	va_start(list, fmt);
	vsnprintf(string, sizeof(string), fmt, list);
	va_end(list);
	
	init_msg_dialog(string);

	while (!get_msg_dialog_result()) {
		glClear(GL_COLOR_BUFFER_BIT);
		vglSwapBuffers(GL_TRUE);
	}

	sceKernelExitProcess(0);
	while (1);
}

// Loads a shader from filesystem
void loadShader(const char *name, int type) {
	// Load vertex shader from filesystem
	char fname[256];
	sprintf(fname, "app0:%s.vert", name);
	FILE *f = fopen(fname, "r");
	if (!f)
		fatal_error("Cannot open %s", fname);
	fseek(f, 0, SEEK_END);
	int32_t vsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *vshader = (char *)malloc(vsize);
	fread(vshader, 1, vsize, f);
	fclose(f);
	
	// Load fragment shader from filesystem
	sprintf(fname, "app0:%s.frag", name);
	f = fopen(fname, "r");
	if (!f)
		fatal_error("Cannot open %s", fname);
	fseek(f, 0, SEEK_END);
	int32_t fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *fshader = (char *)malloc(fsize);
	fread(fshader, 1, fsize, f);
	fclose(f);
	
	// Create required shaders and program
	vshaders[type] = glCreateShader(GL_VERTEX_SHADER);
	fshaders[type] = glCreateShader(GL_FRAGMENT_SHADER);
	programs[type] = glCreateProgram();
	
	// Compiling vertex shader
	glShaderSource(vshaders[type], 1, &vshader, &vsize);
	glCompileShader(vshaders[type]);
	
	// Compiling fragment shader
	glShaderSource(fshaders[type], 1, &fshader, &fsize);
	glCompileShader(fshaders[type]);
	
	// Attaching shaders to final program
	glAttachShader(programs[type], vshaders[type]);
	glAttachShader(programs[type], fshaders[type]);
	
	// Binding attrib locations for the given shaders
	glBindAttribLocation(programs[type], 0, "position");
	glBindAttribLocation(programs[type], 1, "normal");
	
	// Linking program
	glLinkProgram(programs[type]);
	
	// Getting uniforms locations for the given shaders
	modelMatrixLoc[type] = glGetUniformLocation(programs[type], "modelMatrix");
	viewMatrixLoc[type] = glGetUniformLocation(programs[type], "viewMatrix");
	projectionMatrixLoc[type] = glGetUniformLocation(programs[type], "projectionMatrix");
	normalMatrixLoc[type] = glGetUniformLocation(programs[type], "normalMatrix");
	pointLightPositionLoc[type] = glGetUniformLocation(programs[type], "pointLightPosition");
	shininessLoc[type] = glGetUniformLocation(programs[type], "shininess");
	KaLoc[type] = glGetUniformLocation(programs[type], "Ka");
	KdLoc[type] = glGetUniformLocation(programs[type], "Kd");
	KsLoc[type] = glGetUniformLocation(programs[type], "Ks");
	ambientColorLoc[type] = glGetUniformLocation(programs[type], "ambientColor");
	diffuseColorLoc[type] = glGetUniformLocation(programs[type], "diffuseColor");
	specularColorLoc[type] = glGetUniformLocation(programs[type], "specularColor");
	alphaLoc[type] = glGetUniformLocation(programs[type], "alpha");
	F0Loc[type] = glGetUniformLocation(programs[type], "F0");
	
	// Deleting temporary buffers
	free(fshader);
	free(vshader);
}

// Draws a model
void drawModel(to_model *mdl) {
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, mdl->pos);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, mdl->normals);
	glDrawArrays(GL_TRIANGLES, 0, mdl->num_vertices);
}

int main() {
	// Initializing graphics device
	vglInit(0x800000);
	
	// Setting screen clear color
	glClearColor(0.26f, 0.46f, 0.98f, 1.0f);
	
	// Enabling depth test
	glEnable(GL_DEPTH_TEST);
	
	// Projection matrix: FOV angle, aspect ratio, near and far planes
	glm::mat4 projection = glm::perspective(45.0f, 960.0f / 544.0f, 0.1f, 10000.0f);
	
	// View matrix: position, view direction, camera "up" vector
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 7.0f), glm::vec3(0.0f, 0.0f, -7.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	// Initializing model and normal matrices for our objects to identity
	glm::mat4 sphereModelMatrix = glm::mat4(1.0f);
	glm::mat3 sphereNormalMatrix = glm::mat3(1.0f);
	glm::mat4 cubeModelMatrix = glm::mat4(1.0f);
	glm::mat3 cubeNormalMatrix = glm::mat3(1.0f);
	glm::mat4 bunnyModelMatrix = glm::mat4(1.0f);
	glm::mat3 bunnyNormalMatrix = glm::mat3(1.0f);
	glm::mat4 planeModelMatrix = glm::mat4(1.0f);
	glm::mat3 planeNormalMatrix = glm::mat3(1.0f);
	
	// Calculating plane model and normal matrices since static
	planeModelMatrix = glm::mat4(1.0f);
	planeNormalMatrix = glm::mat3(1.0f);
	planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
	planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 1.0f, 10.0f));
	planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
	
	// Default values for spinning state, wireframe mode and pressed buttons bitmask
	uint32_t old_buttons = 0;
	GLboolean spinning = GL_TRUE;
	GLboolean wireframe = GL_FALSE;
	
	// Default values for timing calculations
	GLfloat deltaTime = 0.0f;
	GLfloat lastFrame = 0.0f;
	
	// Initial rotation angle on Y axis
	GLfloat orientationY = 0.0f;
	
	// Rotation speed on Y axis
	GLfloat spin_speed = 30.0f;
	
	// Setting up our shaders
	loadShader("lambertian", LAMBERTIAN);
	loadShader("phong", PHONG);
	loadShader("blinn", BLINN_PHONG);
	loadShader("fdg", FDG);
	
	// Setting constant uniform values
	for (int i = 0; i < SHADERS_NUM; i++) {
		glUniform3fv(diffuseColorLoc[i], 1, diffuseColor);
		if (i == FDG) { // Uniforms required only by FDG illumination model
			glUniform1f(alphaLoc[i], roughness);
			glUniform1f(F0Loc[i], F0);
		} else if (i != LAMBERTIAN) { // Uniforms required only by Phong and Blinn-Phong illumination models
			glUniform3fv(specularColorLoc[i], 1, specularColor);
			glUniform3fv(ambientColorLoc[i], 1, ambientColor);
			glUniform1f(KaLoc[i], Ka);
			glUniform1f(KsLoc[i], Ks);
			glUniform1f(shininessLoc[i], shininess);
		}
		glUniform1f(KdLoc[i], Kd);
		glUniformMatrix4fv(projectionMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(viewMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(pointLightPositionLoc[i], 1, glm::value_ptr(lightPos0));
	}
	
	//Setting default in use shader
	int shader_idx = LAMBERTIAN;
	glUseProgram(programs[shader_idx]);
	
	// Loading our models
	to_model plane, bunny, sphere, cube;
	if (to_loadObj("app0:plane.obj", &plane))
		fatal_error("Cannot open app0:plane.obj");
	if (to_loadObj("app0:bunny.obj", &bunny))
		fatal_error("Cannot open app0:bunny.obj");
	if (to_loadObj("app0:sphere.obj", &sphere))
		fatal_error("Cannot open app0:sphere.obj");
	if (to_loadObj("app0:cube.obj", &cube))
		fatal_error("Cannot open app0:cube.obj");
	
	// Main loop
	for (;;){
		// Reading inputs
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		
		// Calculating delta time in seconds
		GLfloat currentFrame = (float)sceKernelGetProcessTimeWide() / 1000000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		// Clearing color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Checking for wireframe mode changes requests
		if (CHECK_BTN(SCE_CTRL_CROSS)) {
			if (wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			else
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			wireframe = !wireframe;
		}
		
		// Checking for spinning mode changes requests
		if (CHECK_BTN(SCE_CTRL_CIRCLE)) {
			spinning = !spinning;
		}
		
		// Properly altering rotation angle if spinning mode is enabled
		if (spinning) {
			orientationY += deltaTime * spin_speed;
		}
		
		// Checking for illumination models changes requests
		if (CHECK_BTN(SCE_CTRL_LEFT)) {
			shader_idx--;
			if (shader_idx < 0)
				shader_idx = SHADERS_NUM - 1;
		} else if (CHECK_BTN(SCE_CTRL_RIGHT)) {
			shader_idx = (shader_idx + 1) % SHADERS_NUM;
		}
		glUseProgram(programs[shader_idx]);
		
		// Drawing plane
		glUniformMatrix4fv(modelMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
		glUniformMatrix3fv(normalMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));
		drawModel(&plane);
		
		// Drawing cube
		cubeModelMatrix = glm::mat4(1.0f);
		cubeNormalMatrix = glm::mat3(1.0f);
		cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
		cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
		glUniformMatrix4fv(modelMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
		glUniformMatrix3fv(normalMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));
		drawModel(&cube);

		// Drawing sphere
		sphereModelMatrix = glm::mat4(1.0f);
		sphereNormalMatrix = glm::mat3(1.0f);
		sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(-3.0f, 0.0f, 0.0f));
		sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
		glUniformMatrix4fv(modelMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
		glUniformMatrix3fv(normalMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));
		drawModel(&sphere);
		
		// Drawing bunny
		bunnyModelMatrix = glm::mat4(1.0f);
		bunnyNormalMatrix = glm::mat3(1.0f);
		bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(3.0f, 0.0f, 0.0f));
		bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
		bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view*bunnyModelMatrix));
		glUniformMatrix4fv(modelMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
		glUniformMatrix3fv(normalMatrixLoc[shader_idx], 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));
		drawModel(&bunny);
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
		old_buttons = pad.buttons;
	}
	
	// Terminating graphics device
	vglEnd();
}