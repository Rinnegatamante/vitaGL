#include <stdlib.h>
#include "vitaGL.h"
#include "math_utils.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define TEXTURES_NUM 32

typedef enum SceGxmPrimitiveTypeExtra{
	SCE_GXM_PRIMITIVE_NONE = 0,
	SCE_GXM_PRIMITIVE_QUADS = 1
} SceGxmPrimitiveTypeExtra;

typedef struct vertexList{
	vita2d_texture_vertex v;
	void* next;
} vertexList;

typedef enum glPhase{
	NONE = 0,
	MODEL_CREATION = 1
} glPhase;

static matrix4x4 _vita2d_projection_matrix;
static matrix4x4 modelview;
extern SceGxmContext* _vita2d_context;
extern SceGxmVertexProgram* _vita2d_colorVertexProgram;
extern SceGxmFragmentProgram* _vita2d_colorFragmentProgram;
extern SceGxmVertexProgram* _vita2d_textureVertexProgram;
extern SceGxmFragmentProgram* _vita2d_textureFragmentProgram;
extern SceGxmFragmentProgram* _vita2d_textureTintFragmentProgram;
extern const SceGxmProgramParameter* _vita2d_colorWvpParam;
extern const SceGxmProgramParameter* _vita2d_textureWvpParam;
extern SceGxmProgramParameter* _vita2d_textureTintColorParam;

SceGxmPrimitiveType prim;
SceGxmPrimitiveTypeExtra prim_extra = SCE_GXM_PRIMITIVE_NONE;
vertexList* model = NULL;
vertexList* last = NULL;
glPhase phase = NONE;
GLenum error = GL_NO_ERROR;
GLuint textures[TEXTURES_NUM];
vita2d_texture* v2d_textures[TEXTURES_NUM];
uint8_t texture_init = 1;
int8_t texture_unit = -1;
uint64_t vertex_count = 0;
uint8_t v2d_drawing = 0;
matrix4x4* matrix = NULL;

GLenum glGetError(void){
	return error;
}

void glClear(GLbitfield mask){
	switch (mask){
		case GL_COLOR_BUFFER_BIT:
			if (v2d_drawing){
				vita2d_end_drawing();
				vita2d_wait_rendering_done();
				vita2d_swap_buffers();
			}
			vita2d_start_drawing();
			vita2d_clear_screen();
			v2d_drawing = 1;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha){
	uint8_t r, g, b, a;
	r = red * 255.0f;
	g = green * 255.0f;
	b = blue * 255.0f;
	a = alpha * 255.0f;
	vita2d_set_clear_color(RGBA8(r, g, b, a));
}

void glEnable(GLenum cap){
	
}

void glBegin(GLenum mode){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	phase = MODEL_CREATION;
	prim_extra = SCE_GXM_PRIMITIVE_NONE;
	switch (mode){
		case GL_POINTS:
			prim = SCE_GXM_PRIMITIVE_POINTS;
			break;
		case GL_LINES:
			prim = SCE_GXM_PRIMITIVE_LINES;
			break;
		case GL_TRIANGLES:
			prim = SCE_GXM_PRIMITIVE_TRIANGLES;
			break;
		case GL_TRIANGLE_STRIP:
			prim = SCE_GXM_PRIMITIVE_TRIANGLE_STRIP;
			break;
		case GL_TRIANGLE_FAN:
			prim = SCE_GXM_PRIMITIVE_TRIANGLE_FAN;
			break;
		case GL_QUADS:
			prim = SCE_GXM_PRIMITIVE_TRIANGLES;
			prim_extra = SCE_GXM_PRIMITIVE_QUADS;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
	vertex_count = 0;
}

void glEnd(void){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	phase = NONE;
	
	matrix4x4 mvp_matrix;
	matrix4x4 final_mvp_matrix;
	
	matrix4x4_multiply(mvp_matrix, _vita2d_projection_matrix, modelview);
	matrix4x4_transpose(final_mvp_matrix,mvp_matrix);
	
	sceGxmSetVertexProgram(_vita2d_context, _vita2d_textureVertexProgram);
	sceGxmSetFragmentProgram(_vita2d_context, _vita2d_textureFragmentProgram);
	
	void* vertex_wvp_buffer;
	sceGxmReserveVertexDefaultUniformBuffer(_vita2d_context, &vertex_wvp_buffer);
	sceGxmSetUniformDataF(vertex_wvp_buffer, _vita2d_textureWvpParam, 0, 16, (const float*)final_mvp_matrix);
	
	sceGxmSetFragmentTexture(_vita2d_context, 0, &v2d_textures[texture_unit]->gxm_tex);
	
	
	vita2d_texture_vertex* vertices;
	uint16_t* indices;
	int n = 0, quad_n = 0, v_n = 0;
	vertexList* object = model;
	uint32_t idx_count = vertex_count;
	
	switch (prim_extra){
		case SCE_GXM_PRIMITIVE_NONE:
			vertices = (vita2d_texture_vertex*)vita2d_pool_memalign(vertex_count * sizeof(vita2d_texture_vertex), sizeof(vita2d_texture_vertex));
			indices = (uint16_t*)vita2d_pool_memalign(vertex_count * sizeof(uint16_t), sizeof(uint16_t));
			while (object != NULL){
				memcpy(&vertices[n], &object->v, sizeof(vita2d_texture_vertex));
				indices[n] = n;
				vertexList* old = object;
				object = object->next;
				free(old);
				n++;
			}
			break;
		case SCE_GXM_PRIMITIVE_QUADS:
			quad_n = vertex_count >> 2;
			idx_count = quad_n * 6;
			vertices = (vita2d_texture_vertex*)vita2d_pool_memalign(vertex_count * sizeof(vita2d_texture_vertex), sizeof(vita2d_texture_vertex));
			indices = (uint16_t*)vita2d_pool_memalign(idx_count * sizeof(uint16_t), sizeof(uint16_t));
			int i, j;
			for (i=0; i < quad_n; i++){
				for (j=0; j < 4; j++){
					memcpy(&vertices[i*4+j], &object->v, sizeof(vita2d_texture_vertex));
					vertexList* old = object;
					object = object->next;
					free(old);
				}
				indices[i*6] = i*4;
				indices[i*6+1] = i*4+1;
				indices[i*6+2] = i*4+3;
				indices[i*6+3] = i*4+1;
				indices[i*6+4] = i*4+2;
				indices[i*6+5] = i*4+3;
			}
			break;
	}
	
	model = NULL;
	vertex_count = 0;
	sceGxmSetVertexStream(_vita2d_context, 0, vertices);
	sceGxmDraw(_vita2d_context, prim, SCE_GXM_INDEX_FORMAT_U16, indices, idx_count);
	
}

void glGenTextures(GLsizei n, GLuint* res){
	int i = 0, j = 0;
	if (n < 0){
		error = GL_INVALID_VALUE;
		return;
	}
	if (texture_init){
		texture_init = 0;
		for (i=0; i < TEXTURES_NUM; i++){
			textures[i] = GL_TEXTURE0 + i;
			v2d_textures[i] = NULL;
		}
	}
	i = 0;
	for (i=0; i < TEXTURES_NUM; i++){
		if (textures[i] != 0x0000){
			res[j++] = textures[i];
			textures[i] = 0x0000;
		}
		if (j >= n) break;
	}
}

void glBindTexture(GLenum target, GLuint texture){
	if ((texture != 0x0000) && (texture > GL_TEXTURE31) && (texture < GL_TEXTURE0)){
		error = GL_INVALID_VALUE;
		return;
	}
	switch (target){
		case GL_TEXTURE_2D:
			texture_unit = texture - GL_TEXTURE0;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data){
	SceGxmTextureFormat tex_format;
	switch (target){
		case GL_TEXTURE_2D:
			if (v2d_textures[texture_unit] != NULL) vita2d_free_texture(v2d_textures[texture_unit]);
			switch (format){
				case GL_RGB:
					switch (type){
						case GL_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_S8S8S8X8_RGB1;
							break;
						case GL_UNSIGNED_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8X8_RGB1;
							break;
						case GL_UNSIGNED_SHORT_4_4_4_4:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4X4_RGB1;
							break;
						case GL_UNSIGNED_SHORT_5_5_5_1:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5X1_RGB1;
							break;
						case GL_UNSIGNED_SHORT_5_6_5:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U5U6U5_RGB;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}
					break;
				case GL_RGBA:
					switch (type){
						case GL_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_S8S8S8S8_RGBA;
							break;
						case GL_UNSIGNED_BYTE:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA;
							break;
						case GL_UNSIGNED_SHORT_4_4_4_4:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U4U4U4U4_RGBA;
							break;
						case GL_UNSIGNED_SHORT_5_5_5_1:
							tex_format = SCE_GXM_TEXTURE_FORMAT_U5U5U5U1_RGBA;
							break;
						default:
							error = GL_INVALID_ENUM;
							break;
					}					
					break;
				case GL_VITA2D_TEXTURE:
					tex_format = SCE_GXM_TEXTURE_FORMAT_A8B8G8R8;
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			if (error == GL_NO_ERROR){
				v2d_textures[texture_unit] = vita2d_create_empty_texture_rendertarget(width, height, tex_format);
				uint8_t* tex = (uint8_t*)vita2d_texture_get_datap(v2d_textures[texture_unit]);
				memcpy(tex, (uint8_t*)data, width*height*internalFormat);
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;	
	}
}

void glTexParameteri(GLenum target, GLenum pname, GLint param){
	switch (target){
		case GL_TEXTURE_2D:
			switch (pname){
				case GL_TEXTURE_MIN_FILTER:
					switch (param){
						case GL_NEAREST:
							vita2d_texture_set_filters(v2d_textures[texture_unit], SCE_GXM_TEXTURE_FILTER_POINT , vita2d_texture_get_mag_filter(v2d_textures[texture_unit]));
							break;
						case GL_LINEAR:
							vita2d_texture_set_filters(v2d_textures[texture_unit], SCE_GXM_TEXTURE_FILTER_LINEAR , vita2d_texture_get_mag_filter(v2d_textures[texture_unit]));
							break;
						case GL_NEAREST_MIPMAP_NEAREST:
							break;
						case GL_LINEAR_MIPMAP_NEAREST:
							break;
						case GL_NEAREST_MIPMAP_LINEAR:
							break;
						case GL_LINEAR_MIPMAP_LINEAR:
							break;
					}
					break;
				case GL_TEXTURE_MAG_FILTER:
					switch (param){
						case GL_NEAREST:
							vita2d_texture_set_filters(v2d_textures[texture_unit], vita2d_texture_get_min_filter(v2d_textures[texture_unit]), SCE_GXM_TEXTURE_FILTER_POINT);
							break;
						case GL_LINEAR:
							vita2d_texture_set_filters(v2d_textures[texture_unit], vita2d_texture_get_min_filter(v2d_textures[texture_unit]), SCE_GXM_TEXTURE_FILTER_LINEAR);
							break;
						case GL_NEAREST_MIPMAP_NEAREST:
							break;
						case GL_LINEAR_MIPMAP_NEAREST:
							break;
						case GL_NEAREST_MIPMAP_LINEAR:
							break;
						case GL_LINEAR_MIPMAP_LINEAR:
							break;
					}
					break;
				default:
					error = GL_INVALID_ENUM;
					break;
			}
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glTexCoord2i(GLint s, GLint t){
	if (phase != MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}
	if (model == NULL){ 
		last = (vertexList*)malloc(sizeof(vertexList));
		model = last;
	}else{
		last->next = (vertexList*)malloc(sizeof(vertexList));
		last = last->next;
	}
	last->v.u = s;
	last->v.v = t;
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z){
	last->v.x = x;
	last->v.y = y;
	last->v.z = z;
	vertex_count++;
}

void glLoadIdentity(void){
	matrix4x4_identity(*matrix);
}

void glMatrixMode(GLenum mode){
	switch (mode){
		case GL_MODELVIEW:
			matrix = &modelview;
			break;
		case GL_PROJECTION:
			matrix = &_vita2d_projection_matrix;
			break;
		default:
			error = GL_INVALID_ENUM;
			break;
	}
}

void glViewport(GLint x,  GLint y,  GLsizei width,  GLsizei height){
	
}

void glOrtho(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}else if ((left == right) || (bottom == top) || (nearVal == farVal)){
		error = GL_INVALID_VALUE;
		return;
	}
	matrix4x4_init_orthographic(*matrix, left, right, bottom, top, nearVal, farVal);
}

void glFrustum(GLdouble left,  GLdouble right,  GLdouble bottom,  GLdouble top,  GLdouble nearVal,  GLdouble farVal){
	if (phase == MODEL_CREATION){
		error = GL_INVALID_OPERATION;
		return;
	}else if ((left == right) || (bottom == top) || (nearVal < 0) || (farVal < 0)){
		error = GL_INVALID_VALUE;
		return;
	}
	matrix4x4_init_frustum(*matrix, left, right, bottom, top, nearVal, farVal);
}

void glMultMatrixf(const GLfloat* m){
	matrix4x4 res;
	matrix4x4 tmp;
	int i,j;
	for (i=0;i<4;i++){
		for (j=0;j<4;j++){
			tmp[i][j] = m[i*4+j];
		}
	}
	matrix4x4_multiply(res, *matrix, tmp);
	matrix4x4_copy(*matrix, res);
}

void glLoadMatrixf(const GLfloat* m){
	matrix4x4 tmp;
	int i,j;
	for (i=0;i<4;i++){
		for (j=0;j<4;j++){
			tmp[i][j] = m[i*4+j];
		}
	}
	matrix4x4_copy(*matrix, tmp);
}

void glTranslatef(GLfloat x,  GLfloat y,  GLfloat z){
	matrix4x4_translate(*matrix, x, y, z);
}

void glScalef (GLfloat x, GLfloat y, GLfloat z){
	matrix4x4_scale(*matrix, x, y, z);
}