// Derived from https://subscription.packtpub.com/book/game-development/9781782167020/1/ch01lvl1sec18/using-uniform-blocks-and-uniform-buffer-objects

#include <vitasdk.h>
#include <vitaGL.h>

char *frag_shader = " \
	float3 in TexCoord : TEXCOORD0; \
	uniform BlobSettings { \
		float4 InnerColor; \
		float4 OuterColor; \
		float RadiusInner; \
		float RadiusOuter; \
	} BlobSettings : BUFFER[0]; \
	float4 main() : COLOR { \
		float dx = TexCoord.x - 0.5; \
		float dy = TexCoord.y - 0.5; \
		float dist = sqrt(dx * dx + dy * dy); \
		return lerp( \
			BlobSettings.InnerColor, \
			BlobSettings.OuterColor, \
			smoothstep(BlobSettings.RadiusInner, \
			BlobSettings.RadiusOuter, dist)); \
	}";
	
char *vert_shader = " \
	float3 out TexCoord: TEXCOORD0; \
	float4 out gl_Position : POSITION; \
	void main(float3 VertexPosition, float3 VertexTexCoord) { \
		TexCoord = VertexTexCoord; \
		gl_Position = float4(VertexPosition, 1.0); \
	}";	

int main() {
	// Initializing graphics device
	vglInitExtended(0, 960, 544, 4 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);

	// Setting screen clear color
	glViewport(0, 0, 960, 544);
	glClearColor(0.26f, 0.46f, 0.98f, 1.0f);
	
	// Compiling our shaders
	GLuint vshad = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshad, 1, &vert_shader, NULL);
	glCompileShader(vshad);
	GLuint fshad = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshad, 1, &frag_shader, NULL);
	glCompileShader(fshad);
	
	// Creating our program
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vshad);
	glAttachShader(prog, fshad);
	glBindAttribLocation(prog, 0, "VertexPosition");
	glBindAttribLocation(prog, 1, "VertexTexCoord");
	glLinkProgram(prog);
	glUseProgram(prog);
	
	// Setup binding for our uniform block
	GLuint blockIndex = glGetUniformBlockIndex(prog, "BlobSettings");
	glUniformBlockBinding(prog, blockIndex, 0);
	
	// Setting up our UBO
	GLuint uboHandle;
	glGenBuffers(1, &uboHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, uboHandle);
	GLfloat outerColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
	GLfloat innerColor[] = {1.0f, 1.0f, 0.75f, 1.0f};
	GLfloat innerRadius = 0.25f, outerRadius = 0.45f;
	float blockBuffer[10];
	memcpy(blockBuffer, innerColor, 4 * sizeof(GLfloat));
	memcpy(&blockBuffer[4], outerColor, 4 * sizeof(GLfloat));
	memcpy(&blockBuffer[8], &innerRadius, sizeof(GLfloat));
	memcpy(&blockBuffer[9], &outerRadius, sizeof(GLfloat));
	glBufferData(GL_UNIFORM_BUFFER, 10 * sizeof(float), blockBuffer, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboHandle);
	
	float positionData[] = {
		-0.8f, -0.8f, 0.0f,
		0.8f, -0.8f, 0.0f,
		0.8f,  0.8f, 0.0f,
		-0.8f, -0.8f, 0.0f,
		0.8f, 0.8f, 0.0f,
		-0.8f, 0.8f, 0.0f
	 };
	float tcData[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	
	// Create and populate the buffer objects
	GLuint vboHandles[2];
	glGenBuffers(2, vboHandles);
	GLuint positionBufferHandle = vboHandles[0];
	GLuint tcBufferHandle = vboHandles[1];
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), positionData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, tcBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), tcData, GL_STATIC_DRAW);
	
	// Bind our buffer objects
	glEnableVertexAttribArray(0);  // Vertex position
	glEnableVertexAttribArray(1);  // Vertex texture coords
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte *)NULL );
	glBindBuffer(GL_ARRAY_BUFFER, tcBufferHandle);
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte *)NULL );
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	// Main loop
	for (;;) {
		// Clear color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Draw our object
		glDrawArrays(GL_TRIANGLES, 0, 6 );

		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
}
