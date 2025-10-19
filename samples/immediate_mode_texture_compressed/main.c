// Drawing a fullscreen image on screen with glBegin/glEnd
#include <vitasdk.h>
#include <vitaGL.h>
#include <stdlib.h>
#include <stdio.h>

GLenum texture_format = GL_RGB;
GLuint texture = 0;

int main(){
	// Initializing graphics device
	vglInit(0x100000);
	
	// Loading PVR image to use as texture
	uint32_t *ext_data;
	int width, height;
	FILE *f = fopen("app0:texture.pvr", "rb");
	fseek(f, 0, SEEK_END);
	uint32_t size = ftell(f) - 0x34;
	uint32_t metadata_size;
	fseek(f, 0x08, SEEK_SET);
	uint64_t format;
	fread(&format, 1, 8, f);
	fseek(f, 0x18, SEEK_SET);
	fread(&height, 1, 4, f);
	fread(&width, 1, 4, f);
	fseek(f, 0x30, SEEK_SET);
	fread(&metadata_size, 1, 4, f);
	size -= metadata_size;
	ext_data = vglMalloc(size);
	fseek(f, metadata_size, SEEK_CUR);
	fread(ext_data, 1, size, f);
	fclose(f);
	
	// Setting screen clear color
	glClearColor(0.50, 0, 0, 0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 960, 544, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Initializing an openGL texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	// Uploading the loaded image as texture
	switch (format) {
	case 0x00:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG, width, height, 0, size, ext_data);
		break;
	case 0x01:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG, width, height, 0, size, ext_data);
		break;
	case 0x02:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG, width, height, 0, size, ext_data);
		break;
	case 0x03:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, width, height, 0, size, ext_data);
		break;
	case 0x04:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG, width, height, 0, size, ext_data);
		break;
	case 0x05:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG, width, height, 0, size, ext_data);
		break;
	case 0x06:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, width, height, 0, size, ext_data);
		break;
	case 0x07:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, width, height, 0, size, ext_data);
		break;
	case 0x09:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, width, height, 0, size, ext_data);
		break;
	case 0x0B:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width, height, 0, size, ext_data);
		break;
	case 0x17:
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA8_ETC2_EAC, width, height, 0, size, ext_data);
		break;
	default:
		printf("Unsupported externalized texture format (0x%llX).\n", format);
		break;
	}
	
	// Enabling linear filtering on the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Enabling texturing
	glEnable(GL_TEXTURE_2D);
	
	// Main loop
	for (;;) {
		// Clearing screen
		glClear(GL_COLOR_BUFFER_BIT);
		
		// Binding texture to use for the draw
		glBindTexture(GL_TEXTURE_2D, texture);
		
		// Drawing the texture with immediate mode
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex3f(0, 0, 0);
		glTexCoord2i(1, 0);
		glVertex3f(960, 0, 0);
		glTexCoord2i(1, 1);
		glVertex3f(960, 544, 0);
		glTexCoord2i(0, 1);
		glVertex3f(0, 544, 0);
		glEnd();
		
		// Performing buffer swap
		vglSwapBuffers(GL_FALSE);
	}
}