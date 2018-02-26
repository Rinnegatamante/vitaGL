#include <stdlib.h>
#include <vitasdk.h>
#include "texture_callbacks.h"

// Read callback for 32bpp unsigned RGBA format
uint32_t readRGBA(void *data){
	uint32_t res;
	memcpy(&res, data, 4);
	return res;
}

// Read callback for 24bpp unsigned RGB format
uint32_t readRGB(void *data){
	uint32_t res = 0xFFFFFFFF;
	memcpy(&res, data, 3);
	return res;
}

// Read callback for 16bpp unsigned RG format
uint32_t readRG(void *data){
	uint32_t res = 0xFFFFFFFF;
	memcpy(&res, data, 2);
	return res;
}

// Read callback for 8bpp unsigned R format
uint32_t readR(void *data){
	uint32_t res = 0xFFFFFFFF;
	memcpy(&res, data, 1);
	return res;
}

// Write callback for 32bpp unsigned RGBA format
void writeRGBA(void *data, uint32_t color){
	memcpy(data, &color, 4);
}

// Write callback for 24bpp unsigned RGB format
void writeRGB(void *data, uint32_t color){
	memcpy(data, &color, 3);
}

// Write callback for 8bpp unsigned R format
void writeR(void *data, uint32_t color){
	memcpy(data, &color, 1);
}