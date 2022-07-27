#ifndef __etc1_utils_h__
#define __etc1_utils_h__

#define ETC1_ENCODED_BLOCK_SIZE 8
#define ETC1_DECODED_BLOCK_SIZE 48

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char etc1_byte;
typedef int etc1_bool;
typedef unsigned int etc1_uint32;

// Decode a block of pixels.
//
// pIn is an ETC1 compressed version of the data.
//
// pOut is a pointer to a ETC_DECODED_BLOCK_SIZE array of bytes that represent a
// 4 x 4 square of 3-byte pixels in form R, G, B. Byte (3 * (x + 4 * y) is the R
// value of pixel (x, y).

void etc1_decode_block(const etc1_byte* pIn, etc1_byte* pOut);

// Decode an entire image.
// pIn - pointer to encoded data.
// pOut - pointer to the image data. Will be written such that
//        pixel (x,y) is at pIn + pixelSize * x + stride * y. Must be
//        large enough to store entire image.
// pixelSize can be 2 or 3. 2 is an GL_UNSIGNED_SHORT_5_6_5 image, 3 is a GL_BYTE RGB image.
// returns non-zero if there is an error.

int etc1_decode_image(const etc1_byte* pIn, etc1_byte* pOut,
        etc1_uint32 width, etc1_uint32 height,
        etc1_uint32 pixelSize, etc1_uint32 stride);

#ifdef __cplusplus
}
#endif

#endif
