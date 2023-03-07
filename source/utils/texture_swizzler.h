#ifndef TEXTURE_SWIZZLER_H_
#define TEXTURE_SWIZZLER_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Generic Formats
#if defined(SUPPORT_SMALL_FMT)
void SwizzleTexData8Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize);
void SwizzleTexData16Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize);
void SwizzleTexData32Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize);
#endif
void SwizzleTexData64Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize);
void SwizzleTexData128Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize);

// Specialized Swizzles
void SwizzleTexDataETC1(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_SWIZZLER_H_ */