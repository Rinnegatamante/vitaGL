#include <vector>
#include <stdio.h>
#include <vitasdk.h>
#include <vitaGL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <libtoloader.h>
#include <random>

// Number of shader sets available
#define SHADERS_NUM 3

// Analogs deadzone
#define ANALOGS_DEADZONE 30

// Macro to check if a button has been pressed
#define CHECK_BTN(x) ((pad.buttons & x) && (!(old_buttons & x)))

// Setup for our point light source
GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};

// Weights for the diffusive component
GLfloat Kd = 0.5f;

// Global Up and Front vectors
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 front = glm::vec3(0.0f, 1.0f, 0.0f);

// Camera setup
const float camera_sensitivity = 0.01f;
GLboolean can_fly = GL_FALSE;
glm::vec3 camera_pos = glm::vec3(0.0f, 0.0f, 7.0f); // Camera position
glm::vec3 camera_front; // View direction
glm::vec3 camera_up; // Up vector for the camera
glm::vec3 camera_orientation = glm::vec3(-90.0f, 0.0f, 0.0f); // Yaw, Pitch, Roll
glm::vec3 camera_right; // Right vector for the camera

void update_camera() {
	// Based on https://learnopengl.com/#!Getting-started/Camera
	camera_front = glm::normalize(glm::vec3(cos(glm::radians(camera_orientation.x)) * cos(glm::radians(camera_orientation.y)),
		sin(glm::radians(camera_orientation.y)), sin(glm::radians(camera_orientation.x)) * cos(glm::radians(camera_orientation.y))));
	front = glm::vec3(camera_front.x, 0.0f, camera_front.z);
	camera_right = glm::normalize(glm::cross(camera_front, up));
	camera_up = glm::normalize(glm::cross(camera_right, camera_front));
}

// Movement setup
const float movement_speed = 1.0f;

// Render pass stages
enum {
	GEOMETRY,
	SSAO,
	LIGHTING
};

// Shaders and programs
GLuint vshaders[SHADERS_NUM];
GLuint fshaders[SHADERS_NUM];
GLuint programs[SHADERS_NUM];

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
	if (type == GEOMETRY) {
		glBindAttribLocation(programs[type], 0, "position");
		glBindAttribLocation(programs[type], 1, "normal");
	} else {
		glBindAttribLocation(programs[type], 0, "position");
		glBindAttribLocation(programs[type], 1, "texcoord");
	}
	
	// Linking program
	glLinkProgram(programs[type]);
	
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

// Function to draw a fullscreen quad
void DrawQuad() {
	float quadVertices[] = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), quadVertices);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), &quadVertices[3]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int main() {
	// Initializing graphics device
	vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_NONE);
	
	// Enabling sampling for the analogs
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	
	// Setting screen clear color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Projection matrix: FOV angle, aspect ratio, near and far planes
	glm::mat4 projection = glm::perspective(45.0f, 960.0f / 544.0f, 0.1f, 50.0f);
	
	// Initializing model and normal matrices for our objects to identity
	glm::mat4 sphereModelMatrix = glm::mat4(1.0f);
	glm::mat3 sphereNormalMatrix = glm::mat3(1.0f);
	glm::mat4 cubeModelMatrix = glm::mat4(1.0f);
	glm::mat3 cubeNormalMatrix = glm::mat3(1.0f);
	glm::mat4 bunnyModelMatrix = glm::mat4(1.0f);
	glm::mat3 bunnyNormalMatrix = glm::mat3(1.0f);
	
	// Light configuration
	glm::vec3 lightPos = glm::vec3(2.0f, 4.0f, -2.0f);
	glm::vec3 lightColor = glm::vec3(0.2f, 0.8f, 0.2f);
	GLfloat linearAttenuation = 0.09f;
	GLfloat quadraticAttenuation = 0.032f;
	
	// Default values for spinning state, wireframe mode and pressed buttons bitmask
	uint32_t old_buttons = 0;
	GLboolean spinning = GL_TRUE;
	
	// Default values for timing calculations
	GLfloat deltaTime = 0.0f;
	GLfloat lastFrame = 0.0f;
	
	// Initial rotation angle on Y axis
	GLfloat orientationY = 0.0f;
	
	// Rotation speed on Y axis
	GLfloat spin_speed = 30.0f;
	
	// Setting up our shaders
	loadShader("geometry", GEOMETRY);
	loadShader("ssao", SSAO);
	loadShader("lighting", LIGHTING);
	
	// Available ambient occlusion modes
	enum {
		NO_SSAO,
		SSAO,
		SSAO_MODES_NUM
	};
	GLint ssao_mode = SSAO;
	
	// Create a full white texture to simulate absence of ambient occlusion
	GLuint gWhiteTex;
	glGenTextures(1, &gWhiteTex);
	glBindTexture(GL_TEXTURE_2D, gWhiteTex);
	uint32_t white = 0xFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white);
	
	// Creating the textures for the G-Buffer framebuffer (gNormal)
	GLuint gNormal;
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 960, 544, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // Float texture to ensure values are not clamped in [0, 1] range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Creating the G-Buffer framebuffer and binding the previously created texture to it
	GLuint gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);

	// Create and attach a depth buffer to our framebuffer
	GLuint depthBuffer;
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 960, 544);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	// Bind our depth buffer to a GL texture
	GLuint gDepthMap;
	glGenTextures(1, &gDepthMap);
	glBindTexture(GL_TEXTURE_2D, gDepthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // First we initialize a bogus texture
	vglFree(vglGetTexDataPointer(GL_TEXTURE_2D)); // We free then its texture data
	vglTexImageDepthBuffer(GL_TEXTURE_2D); // Last we replace the texture data with a pointer to the depth buffer

	// Creating the texture for the framebuffer holding SSAO processing stage
	GLuint SSAOColorBuffer;
	glGenTextures(1, &SSAOColorBuffer);
	glBindTexture(GL_TEXTURE_2D, SSAOColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 960, 544, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Create the framebuffer for SSAO processing stage and binding previously created texture as color buffer
	GLuint SSAOfbo;
	glGenFramebuffers(1, &SSAOfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOColorBuffer, 0);

	// Generate the sample kernel required for SSAO processing
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	std::vector<glm::vec3> SSAOKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / float(64);

		// Scale samples so that they're more aligned to center of kernel
		scale = (scale * scale) * 0.9f + 0.1f;
		sample *= scale;
		SSAOKernel.push_back(sample);
	}
	
	// Generate a noise texture required for SSAO processing
	__fp16 noise_quad[16 * 4];
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
		noise_quad[i*4] = noise.x;
		noise_quad[i*4+1] = noise.y;
		noise_quad[i*4+2] = noise.z;
		noise_quad[i*4+3] = 1.0f;
	}
	GLuint noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise_quad);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	// Loading our models
	to_model cube, bunny, sphere;
	if (to_loadObj("app0:cube.obj", &cube))
		fatal_error("Cannot open app0:cube.obj");
	if (to_loadObj("app0:bunny.obj", &bunny))
		fatal_error("Cannot open app0:bunny.obj");
	if (to_loadObj("app0:sphere.obj", &sphere))
		fatal_error("Cannot open app0:sphere.obj");
	
	// Getting uniforms locations for the used shaders and setting unmutable ones to their desired values
	// Geometry pass
	GLint geo_model = glGetUniformLocation(programs[GEOMETRY], "modelMatrix");
	GLint geo_view = glGetUniformLocation(programs[GEOMETRY], "viewMatrix");
	GLint geo_norm = glGetUniformLocation(programs[GEOMETRY], "normalMatrix");
	GLint geo_invert = glGetUniformLocation(programs[GEOMETRY], "invertedNormals");
	glUniformMatrix4fv(glGetUniformLocation(programs[GEOMETRY], "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	// SSAO pass
	glUniformMatrix4fv(glGetUniformLocation(programs[SSAO], "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(programs[SSAO], "invProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	glUniform3fv(glGetUniformLocation(programs[SSAO], "kernel"), 64, glm::value_ptr(SSAOKernel.front()));
	glUniform1i(glGetUniformLocation(programs[SSAO], "gDepthMap"), 0);
	glUniform1i(glGetUniformLocation(programs[SSAO], "gNormal"), 1);
	glUniform1i(glGetUniformLocation(programs[SSAO], "noiseTexture"), 2);
	// Lighting pass
	GLint light_pos = glGetUniformLocation(programs[LIGHTING], "lightPosition");
	glUniformMatrix4fv(glGetUniformLocation(programs[LIGHTING], "invProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(glm::inverse(projection)));
	glUniform1i(glGetUniformLocation(programs[LIGHTING], "gDepthMap"), 0);
	glUniform1i(glGetUniformLocation(programs[LIGHTING], "gNormal"), 1);
	glUniform1i(glGetUniformLocation(programs[LIGHTING], "SSAO"), 2);
	glUniform3fv(glGetUniformLocation(programs[LIGHTING], "lightColor"), 1, glm::value_ptr(lightColor));
	glUniform1f(glGetUniformLocation(programs[LIGHTING], "linearAttenuation"), linearAttenuation);
	glUniform1f(glGetUniformLocation(programs[LIGHTING], "quadraticAttenuation"), quadraticAttenuation);
	
	// Setting up camera
	update_camera();
	
	// Main loop
	for (;;) {
		// Reading inputs
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		
		// Calculating delta time in seconds
		GLfloat currentFrame = (float)sceKernelGetProcessTimeWide() / 1000000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		// Clearing color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Checking for spinning mode changes requests
		if (CHECK_BTN(SCE_CTRL_CIRCLE)) {
			spinning = !spinning;
		}
		
		// Checking for flying mode changes requests
		if (CHECK_BTN(SCE_CTRL_TRIANGLE)) {
			can_fly = !can_fly;
		}
		
		// Checking for flying mode changes requests
		if (CHECK_BTN(SCE_CTRL_SQUARE)) {
			ssao_mode = ssao_mode == SSAO ? NO_SSAO : SSAO;
		}
		
		// Dealing with camera orientation changes
		GLboolean needs_camera_update = GL_FALSE;
		int rx = pad.rx - 127, ry = pad.ry - 127;
		if (rx < -ANALOGS_DEADZONE) {
			camera_orientation.x += rx * camera_sensitivity;
			needs_camera_update = GL_TRUE;
		} else if (rx > ANALOGS_DEADZONE) {
			camera_orientation.x += rx * camera_sensitivity;
			needs_camera_update = GL_TRUE;
		}
		if (ry < -ANALOGS_DEADZONE) {
			camera_orientation.y -= ry * camera_sensitivity;
			needs_camera_update = GL_TRUE;
		} else if (ry > ANALOGS_DEADZONE) {
			camera_orientation.y -= ry * camera_sensitivity;
			needs_camera_update = GL_TRUE;
		}
		
		// Dealing with movements
		int lx = pad.lx - 127, ly = pad.ly - 127;
		if (lx < -ANALOGS_DEADZONE) {
			camera_pos -= camera_right * (movement_speed * deltaTime);
		} else if (lx > ANALOGS_DEADZONE) {
			camera_pos += camera_right * (movement_speed * deltaTime);
		}
		if (ly < -ANALOGS_DEADZONE) {
			camera_pos += (can_fly ? camera_front : front) * (movement_speed * deltaTime);
		} else if (ly > ANALOGS_DEADZONE) {
			camera_pos -= (can_fly ? camera_front : front) * (movement_speed * deltaTime);
		}
		
		// Performing camera setup update if required
		if (needs_camera_update) {
			// Preventing pitch to get on the "back"
			if (camera_orientation.y < -89.0f)
				camera_orientation.y = -89.0f;
			else if (camera_orientation.y > 89.0f)
				camera_orientation.y = 89.0f;
			update_camera();
		}
		
		// Properly altering rotation angle if spinning mode is enabled
		if (spinning) {
			orientationY += deltaTime * spin_speed;
		}
		
		// STEP 1 - GEOMETRY PASS
		// Render the full scene data into our auxiliary G Buffer
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, 960, 544);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(programs[GEOMETRY]);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 view = glm::lookAt(camera_pos, camera_pos + camera_front, camera_up);
		glUniformMatrix4fv(geo_view, 1, GL_FALSE, glm::value_ptr(view));
		// Drawing room
		glUniform1f(geo_invert, 1.0f); // Invert normals for our room model
		cubeModelMatrix = glm::mat4(1.0f);
		cubeNormalMatrix = glm::mat3(1.0f);
		cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 7.0f, 0.0f));
		cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(7.5f, 7.5f, 7.5f));
		cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
		glUniformMatrix4fv(geo_model, 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
		glUniformMatrix3fv(geo_norm, 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));
		drawModel(&cube);
		// Drawing sphere
		glUniform1i(geo_invert, 0); // Back to standard normals calculation
		sphereModelMatrix = glm::mat4(1.0f);
		sphereNormalMatrix = glm::mat3(1.0f);
		sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(-3.0f, 0.3f, 0.0f));
		sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
		glUniformMatrix4fv(geo_model, 1, GL_FALSE, glm::value_ptr(sphereModelMatrix));
		glUniformMatrix3fv(geo_norm, 1, GL_FALSE, glm::value_ptr(sphereNormalMatrix));
		drawModel(&sphere);
		// Drawing cube
		cubeModelMatrix = glm::mat4(1.0f);
		cubeNormalMatrix = glm::mat3(1.0f);
		cubeModelMatrix = glm::translate(cubeModelMatrix, glm::vec3(0.0f, 0.3f, 0.0f));
		cubeModelMatrix = glm::rotate(cubeModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		cubeModelMatrix = glm::scale(cubeModelMatrix, glm::vec3(0.8f, 0.8f, 0.8f));
		cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
		glUniformMatrix4fv(geo_model, 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
		glUniformMatrix3fv(geo_norm, 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));
		drawModel(&cube);
		// Drawing bunny
		bunnyModelMatrix = glm::mat4(1.0f);
		bunnyNormalMatrix = glm::mat3(1.0f);
		bunnyModelMatrix = glm::translate(bunnyModelMatrix, glm::vec3(3.0f, 0.3f, 0.0f));
		bunnyModelMatrix = glm::rotate(bunnyModelMatrix, glm::radians(orientationY), glm::vec3(0.0f, 1.0f, 0.0f));
		bunnyModelMatrix = glm::scale(bunnyModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
		bunnyNormalMatrix = glm::inverseTranspose(glm::mat3(view * bunnyModelMatrix));
		glUniformMatrix4fv(geo_model, 1, GL_FALSE, glm::value_ptr(bunnyModelMatrix));
		glUniformMatrix3fv(geo_norm, 1, GL_FALSE, glm::value_ptr(bunnyNormalMatrix));
		drawModel(&bunny);

		if (ssao_mode != NO_SSAO) {
			// STEP 2 - SSAO Texture generation
			glDisable(GL_DEPTH_TEST);
			glBindFramebuffer(GL_FRAMEBUFFER, SSAOfbo);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(programs[SSAO]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gDepthMap);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			DrawQuad();
		}
			
		// STEP 3 - Deferred rendering for lighting with added SSAO
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(programs[LIGHTING]);
		glm::vec3 lightPosView = view * glm::vec4(lightPos, 1.0);
		glUniform3fv(light_pos, 1, glm::value_ptr(lightPosView));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gDepthMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ssao_mode == NO_SSAO ? gWhiteTex : SSAOColorBuffer);
		DrawQuad();
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
		old_buttons = pad.buttons;
	}
}