// Copyright 2009 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "../shared.h"

static const int kModifierTable[] = {
/* 0 */2, 8, -2, -8,
/* 1 */5, 17, -5, -17,
/* 2 */9, 29, -9, -29,
/* 3 */13, 42, -13, -42,
/* 4 */18, 60, -18, -60,
/* 5 */24, 80, -24, -80,
/* 6 */33, 106, -33, -106,
/* 7 */47, 183, -47, -183
};

static const int kLookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

static inline etc1_byte clamp(int x) {
    return (etc1_byte) (x >= 0 ? (x < 255 ? x : 255) : 0);
}

static inline int convert4To8(int b) {
    int c = b & 0xf;
    return (c << 4) | c;
}

static inline int convert5To8(int b) {
    int c = b & 0x1f;
    return (c << 3) | (c >> 2);
}

static inline int convert6To8(int b) {
    int c = b & 0x3f;
    return (c << 2) | (c >> 4);
}

static inline int divideBy255(int d) {
    return (d + 128 + (d >> 8)) >> 8;
}

static inline int convert8To4(int b) {
    int c = b & 0xff;
    return divideBy255(c * 15);
}

static inline int convert8To5(int b) {
    int c = b & 0xff;
    return divideBy255(c * 31);
}

static inline int convertDiff(int base, int diff) {
    return convert5To8((0x1f & base) + kLookup[0x7 & diff]);
}

static
void decode_subblock(etc1_byte* pOut, int r, int g, int b, const int* table,
        etc1_uint32 low, int second, int flipped) {
    int baseX = 0;
    int baseY = 0;
    if (second) {
        if (flipped) {
            baseY = 2;
        } else {
            baseX = 2;
        }
    }
    for (int i = 0; i < 8; i++) {
        int x, y;
        if (flipped) {
            x = baseX + (i >> 1);
            y = baseY + (i & 1);
        } else {
            x = baseX + (i >> 2);
            y = baseY + (i & 3);
        }
        int k = y + (x * 4);
        int offset = ((low >> k) & 1) | ((low >> (k + 15)) & 2);
        int delta = table[offset];
        etc1_byte* q = pOut + 3 * (x + 4 * y);
        *q++ = clamp(r + delta);
        *q++ = clamp(g + delta);
        *q++ = clamp(b + delta);
    }
}

// Input is an ETC1 compressed version of the data.
// Output is a 4 x 4 square of 3-byte pixels in form R, G, B
void etc1_decode_block(const etc1_byte* pIn, etc1_byte* pOut) {
    etc1_uint32 high = (pIn[0] << 24) | (pIn[1] << 16) | (pIn[2] << 8) | pIn[3];
    etc1_uint32 low = (pIn[4] << 24) | (pIn[5] << 16) | (pIn[6] << 8) | pIn[7];
    int r1, r2, g1, g2, b1, b2;
    if (high & 2) {
        // differential
        int rBase = high >> 27;
        int gBase = high >> 19;
        int bBase = high >> 11;
        r1 = convert5To8(rBase);
        r2 = convertDiff(rBase, high >> 24);
        g1 = convert5To8(gBase);
        g2 = convertDiff(gBase, high >> 16);
        b1 = convert5To8(bBase);
        b2 = convertDiff(bBase, high >> 8);
    } else {
        // not differential
        r1 = convert4To8(high >> 28);
        r2 = convert4To8(high >> 24);
        g1 = convert4To8(high >> 20);
        g2 = convert4To8(high >> 16);
        b1 = convert4To8(high >> 12);
        b2 = convert4To8(high >> 8);
    }
    int tableIndexA = 7 & (high >> 5);
    int tableIndexB = 7 & (high >> 2);
    const int* tableA = kModifierTable + tableIndexA * 4;
    const int* tableB = kModifierTable + tableIndexB * 4;
    int flipped = (high & 1) != 0;
    decode_subblock(pOut, r1, g1, b1, tableA, low, 0, flipped);
    decode_subblock(pOut, r2, g2, b2, tableB, low, 1, flipped);
}


// Decode an entire image.
// pIn - pointer to encoded data.
// pOut - pointer to the image data. Will be written such that the Red component of
//       pixel (x,y) is at pIn + pixelSize * x + stride * y + redOffset. Must be
//        large enough to store entire image.
int etc1_decode_image(const etc1_byte* pIn, etc1_byte* pOut,
        etc1_uint32 width, etc1_uint32 height,
        etc1_uint32 pixelSize, etc1_uint32 stride) {
    if (pixelSize < 2 || pixelSize > 3) {
        return -1;
    }
    etc1_byte block[ETC1_DECODED_BLOCK_SIZE];

    etc1_uint32 encodedWidth = (width + 3) & ~3;
    etc1_uint32 encodedHeight = (height + 3) & ~3;

    for (etc1_uint32 y = 0; y < encodedHeight; y += 4) {
        etc1_uint32 yEnd = height - y;
        if (yEnd > 4) {
            yEnd = 4;
        }
        for (etc1_uint32 x = 0; x < encodedWidth; x += 4) {
            etc1_uint32 xEnd = width - x;
            if (xEnd > 4) {
                xEnd = 4;
            }
            etc1_decode_block(pIn, block);
            pIn += ETC1_ENCODED_BLOCK_SIZE;
            for (etc1_uint32 cy = 0; cy < yEnd; cy++) {
                const etc1_byte* q = block + (cy * 4) * 3;
                etc1_byte* p = pOut + pixelSize * x + stride * (y + cy);
                if (pixelSize == 3) {
                    memcpy(p, q, xEnd * 3);
                } else {
                    for (etc1_uint32 cx = 0; cx < xEnd; cx++) {
                        etc1_byte r = *q++;
                        etc1_byte g = *q++;
                        etc1_byte b = *q++;
                        etc1_uint32 pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                        *p++ = (etc1_byte) pixel;
                        *p++ = (etc1_byte) (pixel >> 8);
                    }
                }
            }
        }
    }
    return 0;
}
