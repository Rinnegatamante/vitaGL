/*

Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include "../shared.h"

const uint8_t detex_clamp0to255_table[255 + 256 + 256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
	65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
	81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
	97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112,
	113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
	129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
	145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
	161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
	177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
	193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
	209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
	225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
	241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

/* Clamp an integer value in the range -255 to 511 to the the range 0 to 255. */
static DETEX_INLINE_ONLY uint8_t detexClamp0To255(int x) {
	return detex_clamp0to255_table[x + 255];
}

static const int8_t eac_modifier_table[16][8] = {
	{ -3, -6, -9, -15, 2, 5, 8, 14 },
	{ -3, -7, -10, -13, 2, 6, 9, 12 },
	{ -2, -5, -8, -13, 1, 4, 7, 12 },
	{ -2, -4, -6, -13, 1, 3, 5, 12 },
	{ -3, -6, -8, -12, 2, 5, 7, 11 },
	{ -3, -7, -9, -11, 2, 6, 8, 10 },
	{ -4, -7, -8, -11, 3, 6, 7, 10 },
	{ -3, -5, -8, -11, 2, 4, 7, 10 },
	{ -2, -6, -8, -10, 1, 5, 7, 9 },
	{ -2, -5, -8, -10, 1, 4, 7, 9 },
	{ -2, -4, -8, -10, 1, 3, 7, 9 },
	{ -2, -5, -7, -10, 1, 4, 6, 9 },
	{ -3, -4, -7, -10, 2, 3, 6, 9 },
	{ -1, -2, -3, -10, 0, 1, 2, 9 },
	{ -4, -6, -8, -9, 3, 5, 7, 8 },
	{ -3, -5, -7, -9, 2, 4, 6, 8 }
};

static DETEX_INLINE_ONLY int modifier_times_multiplier(int modifier, int multiplier) {
	return modifier * multiplier;
}

static DETEX_INLINE_ONLY void ProcessPixelEAC(uint8_t i, uint64_t pixels,
const int8_t * DETEX_RESTRICT modifier_table, int base_codeword, int multiplier,
uint8_t * DETEX_RESTRICT pixel_buffer) {
	int modifier = modifier_table[(pixels >> (45 - i * 3)) & 7];
	pixel_buffer[((i & 3) * 4 + ((i & 12) >> 2)) * 4 + DETEX_PIXEL32_ALPHA_BYTE_OFFSET] =
		detexClamp0To255(base_codeword + modifier_times_multiplier(modifier, multiplier));
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the ETC2_EAC */
/* format. */
bool detexDecompressBlockETC2_EAC(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	bool r = detexDecompressBlockETC2(&bitstring[8], mode_mask, flags, pixel_buffer);
	if (!r)
		return false;
	// Decode the alpha part.
	int base_codeword = bitstring[0];
	const int8_t *modifier_table = eac_modifier_table[(bitstring[1] & 0x0F)];
	int multiplier = (bitstring[1] & 0xF0) >> 4;
	if (multiplier == 0 && (flags & DETEX_DECOMPRESS_FLAG_ENCODE))
		// Not allowed in encoding. Decoder should handle it.
		return false;
	uint64_t pixels = ((uint64_t)bitstring[2] << 40) | ((uint64_t)bitstring[3] << 32) |
		((uint64_t)bitstring[4] << 24)
		| ((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	ProcessPixelEAC(0, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(1, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(2, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(3, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(4, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(5, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(6, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(7, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(8, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(9, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(10, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(11, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(12, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(13, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(14, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	ProcessPixelEAC(15, pixels, modifier_table, base_codeword, multiplier, pixel_buffer);
	return true;
}

/* Return the internal mode of a ETC2_EAC block. */
uint32_t detexGetModeETC2_EAC(const uint8_t *bitstring) {
	return detexGetModeETC2(&bitstring[8]);
}

void detexSetModeETC2_EAC(uint8_t *bitstring, uint32_t mode, uint32_t flags,
uint32_t *colors) {
	detexSetModeETC2(&bitstring[8], mode, flags, colors);
}

static DETEX_INLINE_ONLY int Clamp0To2047(int x) {
	if (x < 0)
		return 0;
	if (x > 2047)
		return 2047;
	return x;
}

// For each pixel, decode an 11-bit integer and store as follows:
// If shift and offset are zero, store each value in consecutive 16 bit values in pixel_buffer.
// If shift is one, store each value in consecutive 32-bit words in pixel_buffer; if offset
// is zero, store it in the first 16 bits, if offset is one store it in the last 16 bits of each
// 32-bit word.
static DETEX_INLINE_ONLY void DecodeBlockEAC11Bit(uint64_t qword, int shift, int offset,
uint8_t * DETEX_RESTRICT pixel_buffer) {
	int base_codeword_times_8_plus_4 = ((qword & 0xFF00000000000000) >> (56 - 3)) | 0x4;
	int modifier_index = (qword & 0x000F000000000000) >> 48;
	const int8_t *modifier_table = eac_modifier_table[modifier_index];
	int multiplier_times_8 = (qword & 0x00F0000000000000) >> (52 - 3);
	if (multiplier_times_8 == 0)
		multiplier_times_8 = 1;
	uint16_t *buffer = (uint16_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int pixel_index = (qword & (0x0000E00000000000 >> (i * 3))) >> (45 - i * 3);
		int modifier = modifier_table[pixel_index];
		uint32_t value = Clamp0To2047(base_codeword_times_8_plus_4 +
			modifier * multiplier_times_8);
		buffer[(((i & 3) * 4 + ((i & 12) >> 2)) << shift) + offset] =
			(value << 5) | (value >> 6);	// Replicate bits to 16-bit.
	}
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* EAC_R11 format. */
bool detexDecompressBlockEAC_R11(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	uint64_t qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) |
		((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	DecodeBlockEAC11Bit(qword, 0, 0, pixel_buffer);
	return true;
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* EAC_RG11 format. */
bool detexDecompressBlockEAC_RG11(const uint8_t * DETEX_RESTRICT bitstring, uint32_t mode_mask,
uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	uint64_t red_qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) |
		((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	DecodeBlockEAC11Bit(red_qword, 1, 0, pixel_buffer);
	uint64_t green_qword = ((uint64_t)bitstring[8] << 56) | ((uint64_t)bitstring[9] << 48) |
		((uint64_t)bitstring[10] << 40) |
		((uint64_t)bitstring[11] << 32) | ((uint64_t)bitstring[12] << 24) |
		((uint64_t)bitstring[13] << 16) | ((uint64_t)bitstring[14] << 8) | bitstring[15];
	DecodeBlockEAC11Bit(green_qword, 1, 1, pixel_buffer);
	return true;
}

static DETEX_INLINE_ONLY int ClampMinus1023To1023(int x) {
	if (x < - 1023)
		return - 1023;
	if (x > 1023)
		return 1023;
	return x;
}

static DETEX_INLINE_ONLY uint32_t ReplicateSigned11BitsTo16Bits(int value) {
	if (value >= 0)
		return (value << 5) | (value >> 5);
	value = - value;
	value = (value << 5) | (value >> 5);
	return - value;
}

// For each pixel, decode an 11-bit signed integer and store as follows:
// If shift and offset are zero, store each value in consecutive 16 bit values in pixel_buffer.
// If shift is one, store each value in consecutive 32-bit words in pixel_buffer; if offset
// is zero, store it in the first 16 bits, if offset is one store it in the last 16 bits of each
// 32-bit word.
static DETEX_INLINE_ONLY bool DecodeBlockEACSigned11Bit(uint64_t qword, int shift, int offset,
uint8_t *pixel_buffer) {
	int base_codeword = (int8_t)((qword & 0xFF00000000000000) >> 56);	// Signed 8 bits.
	if (base_codeword == - 128)
		// Not allowed in encoding. Decoder should handle it but we don't do that yet.
		return false;
	int base_codeword_times_8 = base_codeword << 3;				// Arithmetic shift.
	int modifier_index = (qword & 0x000F000000000000) >> 48;
	const int8_t *modifier_table = eac_modifier_table[modifier_index];
	int multiplier_times_8 = (qword & 0x00F0000000000000) >> (52 - 3);
	if (multiplier_times_8 == 0)
		multiplier_times_8 = 1;
	uint16_t *buffer = (uint16_t *)pixel_buffer;
	for (int i = 0; i < 16; i++) {
		int pixel_index = (qword & (0x0000E00000000000 >> (i * 3))) >> (45 - i * 3);
		int modifier = modifier_table[pixel_index];
		int value = ClampMinus1023To1023(base_codeword_times_8 +
			modifier * multiplier_times_8);
		uint32_t bits = ReplicateSigned11BitsTo16Bits(value);
		buffer[(((i & 3) * 4 + ((i & 12) >> 2)) << shift) + offset] = bits;
	}
	return true;
}

/* Decompress a 64-bit 4x4 pixel texture block compressed using the */
/* EAC_SIGNED_R11 format. */
bool detexDecompressBlockEAC_SIGNED_R11(const uint8_t * DETEX_RESTRICT bitstring,
uint32_t mode_mask, uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	uint64_t qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) |
		((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	return DecodeBlockEACSigned11Bit(qword, 0, 0, pixel_buffer);
}

/* Decompress a 128-bit 4x4 pixel texture block compressed using the */
/* EAC_SIGNED_RG11 format. */
bool detexDecompressBlockEAC_SIGNED_RG11(const uint8_t * DETEX_RESTRICT bitstring,
uint32_t mode_mask, uint32_t flags, uint8_t * DETEX_RESTRICT pixel_buffer) {
	uint64_t red_qword = ((uint64_t)bitstring[0] << 56) | ((uint64_t)bitstring[1] << 48) |
		((uint64_t)bitstring[2] << 40) |
		((uint64_t)bitstring[3] << 32) | ((uint64_t)bitstring[4] << 24) |
		((uint64_t)bitstring[5] << 16) | ((uint64_t)bitstring[6] << 8) | bitstring[7];
	int r = DecodeBlockEACSigned11Bit(red_qword, 1, 0, pixel_buffer);
	if (!r)
		return false;
	uint64_t green_qword = ((uint64_t)bitstring[8] << 56) | ((uint64_t)bitstring[9] << 48) |
		((uint64_t)bitstring[10] << 40) |
		((uint64_t)bitstring[11] << 32) | ((uint64_t)bitstring[12] << 24) |
		((uint64_t)bitstring[13] << 16) | ((uint64_t)bitstring[14] << 8) | bitstring[15];
	return DecodeBlockEACSigned11Bit(green_qword, 1, 1, pixel_buffer);
}

//Decode ETC2 EAC encoded data to RGBA32
void eac_decode(uint8_t *encodeData,             //in_data
                 uint8_t *decodeData,              //out_data
                 const int pixelsWidth,
                 const int pixelsHeight,
                 EACDecodeFlag decodeFlag)
{
    uint32_t *decodeBlockData = (uint32_t *)decodeData;

    for (int block_y = 0; block_y < pixelsHeight / 4; ++block_y)   //stride = 3*width
    {
		uint32_t y = block_y * 4;
        for (int block_x = 0; block_x < pixelsWidth / 4; ++block_x)            //skip 4 pixels
        {
			uint32_t x = block_x * 4;
			uint32_t blockData[16];
			
            switch (decodeFlag)
            {
                case EAC_ETC2:
                {
					
					detexDecompressBlockETC2_EAC(encodeData, DETEX_MODE_MASK_ALL, 0, blockData);
					sceClibMemcpy(&decodeBlockData[y * pixelsWidth + x], blockData, 4 * sizeof(uint32_t));
					sceClibMemcpy(&decodeBlockData[(y + 1) * pixelsWidth + x], &blockData[4], 4 * sizeof(uint32_t));
					sceClibMemcpy(&decodeBlockData[(y + 2) * pixelsWidth + x], &blockData[8], 4 * sizeof(uint32_t));
					sceClibMemcpy(&decodeBlockData[(y + 3) * pixelsWidth + x], &blockData[12], 4 * sizeof(uint32_t));
                    encodeData += 16;
                }
                    break;
                default:
                    break;
            }//switch
        }//for block_x
    }//for block_y
}
