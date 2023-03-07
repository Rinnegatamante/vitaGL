#include "texture_swizzler.h"

#include <arm_neon.h>

#define likely(expr) __builtin_expect(expr, 1)

/**
 * For a better understanding of what this code does, please see:
 * https://fgiesen.wordpress.com/2011/01/17/texture-tiling-and-swizzling/
 */

/**
 * Taken from:
 * https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
 */
// "Insert" a 0 bit after each of the 16 low bits of x
static inline uint32_t Part1By1(uint32_t x)
{
    x &= 0x0000ffff;                 // x = ---- ---- ---- ---- fedc ba98 7654 3210
    x = (x ^ (x << 8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x << 4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x << 2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x << 1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    return x;
}
// Inverse of Part1By1 - "delete" all odd-indexed bits
static inline uint32_t Compact1By1(uint32_t x)
{
    x &= 0x55555555;                 // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    x = (x ^ (x >> 1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x >> 2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x >> 4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x >> 8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
    return x;
}

typedef void (*CopyPixel)(uint8_t *dst, uint8_t *src);
/**
 * Type for tile sized copy functions. Size of tile varies by element size.
 * 8 bpe   - 8x8
 * 16 bpe  - 4x4
 * 32 bpe  - 4x4
 * 64 bpe  - 2x2
 * 128 bpe - 2x2
 * 
 * Since *src[] is automatically converted into **src, we
 */
typedef void (*CopyTile)(uint8_t *dst, uint8_t *const *src, const bool loadAligned);

static inline void CopyPixel8Bpp(uint8_t *dst, uint8_t *src)
{
    *dst = *src;
}
static inline void CopyPixel16Bpp(uint8_t *dst, uint8_t *src)
{
    *(uint16_t *)dst = *(uint16_t *)src;
}
static inline void CopyPixel32Bpp(uint8_t *dst, uint8_t *src)
{
    *(uint32_t *)dst = *(uint32_t *)src;
}
static inline void CopyPixel64Bpp(uint8_t *dst, uint8_t *src)
{
    if (likely(((uintptr_t)src & 0x7) == 0))
        vst1_u64((uint64_t *)dst, vld1_u64((uint64_t *)src));
    else
        vst1_u64((uint64_t *)dst, vreinterpret_u64_u8(vld1_u8(src)));
}
static inline void CopyPixel128Bpp(uint8_t *dst, uint8_t *src)
{
    if (likely(((uintptr_t)src & 0x7) == 0))
        vst1q_u64((uint64_t *)dst, vld1q_u64((uint64_t *)src));
    else
        vst1q_u64((uint64_t *)dst, vreinterpretq_u64_u8(vld1q_u8(src)));
}
static void CopyETCBlock(uint8_t *dst, uint8_t *src)
{
    ((uint32_t *)dst)[0] = __builtin_bswap32(((uint32_t *)src)[0]);
    ((uint32_t *)dst)[1] = __builtin_bswap32(((uint32_t *)src)[1]);
}

static inline void CopyTile8Bpp(uint8_t *dst, uint8_t *const *src, const bool loadAligned)
{
    union
    {
        uint8x16x2_t u8_16x2[2];
        uint32x4x2_t u32_4x2[2];
    } tileBuffer;

    /**
     * Unlike the other tile copy functions we don't prefetch here.
     * There's little to no benefit.
     */
    if (likely(loadAligned))
    {
        tileBuffer.u8_16x2[0].val[0] = vreinterpretq_u8_u64(vcombine_u64(vld1_u64((uint64_t *)src[0]), vld1_u64((uint64_t *)src[2])));
        tileBuffer.u8_16x2[0].val[1] = vreinterpretq_u8_u64(vcombine_u64(vld1_u64((uint64_t *)src[1]), vld1_u64((uint64_t *)src[3])));
        tileBuffer.u8_16x2[1].val[0] = vreinterpretq_u8_u64(vcombine_u64(vld1_u64((uint64_t *)src[4]), vld1_u64((uint64_t *)src[6])));
        tileBuffer.u8_16x2[1].val[1] = vreinterpretq_u8_u64(vcombine_u64(vld1_u64((uint64_t *)src[5]), vld1_u64((uint64_t *)src[7])));
    }
    else
    {
        tileBuffer.u8_16x2[0].val[0] = vreinterpretq_u8_u32(vcombine_u32(vld1_u32((uint32_t *)src[0]), vld1_u32((uint32_t *)src[2])));
        tileBuffer.u8_16x2[0].val[1] = vreinterpretq_u8_u32(vcombine_u32(vld1_u32((uint32_t *)src[1]), vld1_u32((uint32_t *)src[3])));
        tileBuffer.u8_16x2[1].val[0] = vreinterpretq_u8_u32(vcombine_u32(vld1_u32((uint32_t *)src[4]), vld1_u32((uint32_t *)src[6])));
        tileBuffer.u8_16x2[1].val[1] = vreinterpretq_u8_u32(vcombine_u32(vld1_u32((uint32_t *)src[5]), vld1_u32((uint32_t *)src[7])));
    }

    tileBuffer.u8_16x2[0] = vzipq_u8(tileBuffer.u8_16x2[0].val[0], tileBuffer.u8_16x2[0].val[1]);
    tileBuffer.u8_16x2[1] = vzipq_u8(tileBuffer.u8_16x2[1].val[0], tileBuffer.u8_16x2[1].val[1]);

    tileBuffer.u32_4x2[0] = vzipq_u32(tileBuffer.u32_4x2[0].val[0], tileBuffer.u32_4x2[0].val[1]);
    tileBuffer.u32_4x2[1] = vzipq_u32(tileBuffer.u32_4x2[1].val[0], tileBuffer.u32_4x2[1].val[1]);

    vst1q_u64((uint64_t *)(dst + (0 * 1)), vreinterpretq_u64_u8(tileBuffer.u8_16x2[0].val[0]));
    vst1q_u64((uint64_t *)(dst + (16 * 1)), vreinterpretq_u64_u8(tileBuffer.u8_16x2[1].val[0]));
    vst1q_u64((uint64_t *)(dst + (32 * 1)), vreinterpretq_u64_u8(tileBuffer.u8_16x2[0].val[1]));
    vst1q_u64((uint64_t *)(dst + (48 * 1)), vreinterpretq_u64_u8(tileBuffer.u8_16x2[1].val[1]));
}

static inline void CopyTile16Bpp(uint8_t *dst, uint8_t *const *src, const bool loadAligned)
{
    union
    {
        uint16x4x2_t u16_4x2[2];
    } tileBuffer;

    __builtin_prefetch(src[0] + 0x30);
    __builtin_prefetch(src[1] + 0x30);
    __builtin_prefetch(src[2] + 0x30);
    __builtin_prefetch(src[3] + 0x30);
    if (likely(loadAligned))
    {
        tileBuffer.u16_4x2[0].val[0] = vreinterpret_u16_u64(vld1_u64((uint64_t *)src[0]));
        tileBuffer.u16_4x2[0].val[1] = vreinterpret_u16_u64(vld1_u64((uint64_t *)src[1]));
        tileBuffer.u16_4x2[1].val[0] = vreinterpret_u16_u64(vld1_u64((uint64_t *)src[2]));
        tileBuffer.u16_4x2[1].val[1] = vreinterpret_u16_u64(vld1_u64((uint64_t *)src[3]));
    }
    else
    {
        tileBuffer.u16_4x2[0].val[0] = vld1_u16((uint16_t *)src[0]);
        tileBuffer.u16_4x2[0].val[1] = vld1_u16((uint16_t *)src[1]);
        tileBuffer.u16_4x2[1].val[0] = vld1_u16((uint16_t *)src[2]);
        tileBuffer.u16_4x2[1].val[1] = vld1_u16((uint16_t *)src[3]);
    }

    tileBuffer.u16_4x2[0] = vzip_u16(tileBuffer.u16_4x2[0].val[0], tileBuffer.u16_4x2[0].val[1]);
    tileBuffer.u16_4x2[1] = vzip_u16(tileBuffer.u16_4x2[1].val[0], tileBuffer.u16_4x2[1].val[1]);

    vst1_u64((uint64_t *)(dst + (0 * 2)), vreinterpret_u64_u16(tileBuffer.u16_4x2[0].val[0]));
    vst1_u64((uint64_t *)(dst + (4 * 2)), vreinterpret_u64_u16(tileBuffer.u16_4x2[1].val[0]));
    vst1_u64((uint64_t *)(dst + (8 * 2)), vreinterpret_u64_u16(tileBuffer.u16_4x2[0].val[1]));
    vst1_u64((uint64_t *)(dst + (12 * 2)), vreinterpret_u64_u16(tileBuffer.u16_4x2[1].val[1]));
}

static inline void CopyTile32Bpp(uint8_t *dst, uint8_t *const *src, const bool loadAligned)
{
    union
    {
        uint32x4x2_t u32_4x2[2];
    } tileBuffer;

    __builtin_prefetch(src[0] + 0x60);
    __builtin_prefetch(src[1] + 0x60);
    __builtin_prefetch(src[2] + 0x60);
    __builtin_prefetch(src[3] + 0x60);
    if (likely(loadAligned))
    {
        tileBuffer.u32_4x2[0].val[0] = vreinterpretq_u32_u64(vld1q_u64((uint64_t *)src[0]));
        tileBuffer.u32_4x2[0].val[1] = vreinterpretq_u32_u64(vld1q_u64((uint64_t *)src[1]));
        tileBuffer.u32_4x2[1].val[0] = vreinterpretq_u32_u64(vld1q_u64((uint64_t *)src[2]));
        tileBuffer.u32_4x2[1].val[1] = vreinterpretq_u32_u64(vld1q_u64((uint64_t *)src[3]));
    }
    else
    {
        tileBuffer.u32_4x2[0].val[0] = vld1q_u32((uint32_t *)src[0]);
        tileBuffer.u32_4x2[0].val[1] = vld1q_u32((uint32_t *)src[1]);
        tileBuffer.u32_4x2[1].val[0] = vld1q_u32((uint32_t *)src[2]);
        tileBuffer.u32_4x2[1].val[1] = vld1q_u32((uint32_t *)src[3]);
    }

    tileBuffer.u32_4x2[0] = vzipq_u32(tileBuffer.u32_4x2[0].val[0], tileBuffer.u32_4x2[0].val[1]);
    tileBuffer.u32_4x2[1] = vzipq_u32(tileBuffer.u32_4x2[1].val[0], tileBuffer.u32_4x2[1].val[1]);

    vst1q_u64((uint64_t *)(dst + (0 * 4)), vreinterpretq_u64_u32(tileBuffer.u32_4x2[0].val[0]));
    vst1q_u64((uint64_t *)(dst + (4 * 4)), vreinterpretq_u64_u32(tileBuffer.u32_4x2[1].val[0]));
    vst1q_u64((uint64_t *)(dst + (8 * 4)), vreinterpretq_u64_u32(tileBuffer.u32_4x2[0].val[1]));
    vst1q_u64((uint64_t *)(dst + (12 * 4)), vreinterpretq_u64_u32(tileBuffer.u32_4x2[1].val[1]));
}

static inline void CopyTile64Bpp(uint8_t *dst, uint8_t *const *src, const bool loadAligned)
{
    union
    {
        uint64x2x2_t u64_2x2;
    } tileBuffer;

    __builtin_prefetch(src[0] + 0x60);
    __builtin_prefetch(src[1] + 0x60);
    if (likely(loadAligned))
    {
        tileBuffer.u64_2x2.val[0] = vld1q_u64((uint64_t *)src[0]);
        tileBuffer.u64_2x2.val[1] = vld1q_u64((uint64_t *)src[1]);
    }
    else
    {
        tileBuffer.u64_2x2.val[0] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)src[0]));
        tileBuffer.u64_2x2.val[1] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)src[1]));
    }

    vst1q_lane_u64((uint64_t *)(dst + (0 * 8)), tileBuffer.u64_2x2.val[0], 0);
    vst1q_lane_u64((uint64_t *)(dst + (1 * 8)), tileBuffer.u64_2x2.val[1], 0);
    vst1q_lane_u64((uint64_t *)(dst + (2 * 8)), tileBuffer.u64_2x2.val[0], 1);
    vst1q_lane_u64((uint64_t *)(dst + (3 * 8)), tileBuffer.u64_2x2.val[1], 1);
}

static inline void CopyTile128Bpp(uint8_t *dst, uint8_t *const *src, const bool loadAligned)
{
    union
    {
        uint64x2x4_t u64_2x4[2];
    } tileBuffer;

    __builtin_prefetch(src[0] + 0xC0);
    __builtin_prefetch(src[1] + 0xC0);
    if (likely(loadAligned))
    {
        tileBuffer.u64_2x4[0].val[0] = vld1q_u64((uint64_t *)src[0]);
        tileBuffer.u64_2x4[0].val[1] = vld1q_u64((uint64_t *)(src[0] + 16));
        tileBuffer.u64_2x4[0].val[2] = vld1q_u64((uint64_t *)src[1]);
        tileBuffer.u64_2x4[0].val[3] = vld1q_u64((uint64_t *)(src[1]  + 16));
    }
    else
    {
        tileBuffer.u64_2x4[0].val[0] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)src[0]));
        tileBuffer.u64_2x4[0].val[1] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)(src[0] + 16)));
        tileBuffer.u64_2x4[0].val[2] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)src[1]));
        tileBuffer.u64_2x4[0].val[3] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)(src[1] + 16)));
    }
    vst1q_u64((uint64_t *)(dst + (0 * 16)), tileBuffer.u64_2x4[0].val[0]);
    vst1q_u64((uint64_t *)(dst + (1 * 16)), tileBuffer.u64_2x4[0].val[2]);
    vst1q_u64((uint64_t *)(dst + (2 * 16)), tileBuffer.u64_2x4[0].val[1]);
    vst1q_u64((uint64_t *)(dst + (3 * 16)), tileBuffer.u64_2x4[0].val[3]);
}
static inline void CopyETC1Tile(uint8_t *dst, uint8_t *const *src, const bool loadAligned)
{
    union
    {
        uint64x2x2_t u64_2x2;
    } tileBuffer;

    __builtin_prefetch(src[0] + 0x60);
    __builtin_prefetch(src[1] + 0x60);
    if (likely(loadAligned))
    {
        tileBuffer.u64_2x2.val[0] = vld1q_u64((uint64_t *)src[0]);
        tileBuffer.u64_2x2.val[1] = vld1q_u64((uint64_t *)src[1]);
    }
    else
    {
        tileBuffer.u64_2x2.val[0] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)src[0]));
        tileBuffer.u64_2x2.val[1] = vreinterpretq_u64_u32(vld1q_u32((uint32_t *)src[1]));
    }

    tileBuffer.u64_2x2.val[0] = vreinterpretq_u64_u8(vrev32q_u8(vreinterpretq_u8_u64(tileBuffer.u64_2x2.val[0])));
    tileBuffer.u64_2x2.val[1] = vreinterpretq_u64_u8(vrev32q_u8(vreinterpretq_u8_u64(tileBuffer.u64_2x2.val[1])));

    vst1q_lane_u64((uint64_t *)(dst + (0 * 8)), tileBuffer.u64_2x2.val[0], 0);
    vst1q_lane_u64((uint64_t *)(dst + (1 * 8)), tileBuffer.u64_2x2.val[1], 0);
    vst1q_lane_u64((uint64_t *)(dst + (2 * 8)), tileBuffer.u64_2x2.val[0], 1);
    vst1q_lane_u64((uint64_t *)(dst + (3 * 8)), tileBuffer.u64_2x2.val[1], 1);
}

template<uint32_t pixelSize, CopyPixel copyPixel, CopyTile copyTile>
static void SwizzleTexData8x8(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    const bool loadAligned = ((uintptr_t)src & 0x7) == 0 && ((stride * pixelSize) & 0x7) == 0;
    const uint32_t xTwiddleMask = 0xAAAAAA80 | ~((tileSize * tileSize) - 1), yTwiddleMask = 0x55555540 | ~((tileSize * tileSize) - 1);
    const uint32_t xTwiddleOrigin = Part1By1(x) << 1, yTwiddleOrigin = Part1By1(y);
    uint32_t xTwiddle = xTwiddleOrigin, yTwiddle = yTwiddleOrigin;

    for (uint32_t yPos = 0; yPos < height; yPos += 8)
    {
        uint8_t *rowAddrs[8] = 
        {
            src + ((yPos + 0) * stride * pixelSize), src + ((yPos + 1) * stride * pixelSize),
            src + ((yPos + 2) * stride * pixelSize), src + ((yPos + 3) * stride * pixelSize),
            src + ((yPos + 4) * stride * pixelSize), src + ((yPos + 5) * stride * pixelSize),
            src + ((yPos + 6) * stride * pixelSize), src + ((yPos + 7) * stride * pixelSize),
        };
        for (uint32_t xPos = 0; xPos < width; xPos += 8)
        {
            copyTile(dst + (xTwiddle + yTwiddle) * pixelSize, rowAddrs, loadAligned);
            xTwiddle = (xTwiddle - xTwiddleMask) & xTwiddleMask;
            rowAddrs[0] += (8 * pixelSize);
            rowAddrs[1] += (8 * pixelSize);
            rowAddrs[2] += (8 * pixelSize);
            rowAddrs[3] += (8 * pixelSize);
            rowAddrs[4] += (8 * pixelSize);
            rowAddrs[5] += (8 * pixelSize);
            rowAddrs[6] += (8 * pixelSize);
            rowAddrs[7] += (8 * pixelSize);
        }
        xTwiddle = xTwiddleOrigin;
        yTwiddle = (yTwiddle - yTwiddleMask) & yTwiddleMask;
    }
}

template<uint32_t pixelSize, CopyPixel copyPixel, CopyTile copyTile>
static void SwizzleTexData4x4(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    const bool loadAligned = ((uintptr_t)src & 0x7) == 0 && ((stride * pixelSize) & 0x7) == 0;
    const uint32_t xTwiddleMask = 0xAAAAAAA0 | ~((tileSize * tileSize) - 1), yTwiddleMask = 0x55555550 | ~((tileSize * tileSize) - 1);
    const uint32_t xTwiddleOrigin = Part1By1(x) << 1, yTwiddleOrigin = Part1By1(y);
    uint32_t xTwiddle = xTwiddleOrigin, yTwiddle = yTwiddleOrigin;

    for (uint32_t yPos = 0; yPos < height; yPos += 4)
    {
        uint8_t *rowAddrs[4] = 
        {
            src + ((yPos + 0) * stride * pixelSize), src + ((yPos + 1) * stride * pixelSize),
            src + ((yPos + 2) * stride * pixelSize), src + ((yPos + 3) * stride * pixelSize),
        };
        for (uint32_t xPos = 0; xPos < width; xPos += 4)
        {
            copyTile(dst + (xTwiddle + yTwiddle) * pixelSize, rowAddrs, loadAligned);
            xTwiddle = (xTwiddle - xTwiddleMask) & xTwiddleMask;
            rowAddrs[0] += (4 * pixelSize);
            rowAddrs[1] += (4 * pixelSize);
            rowAddrs[2] += (4 * pixelSize);
            rowAddrs[3] += (4 * pixelSize);
        }
        xTwiddle = xTwiddleOrigin;
        yTwiddle = (yTwiddle - yTwiddleMask) & yTwiddleMask;
    }
}

template<uint32_t pixelSize, CopyPixel copyPixel, CopyTile copyTile>
static void SwizzleTexData2x2(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    const bool loadAligned = ((uintptr_t)src & 0x7) == 0 && ((stride * pixelSize) & 0x7) == 0;
    const uint32_t xTwiddleMask = 0xAAAAAAA8 | ~((tileSize * tileSize) - 1), yTwiddleMask = 0x55555554 | ~((tileSize * tileSize) - 1);
    const uint32_t xTwiddleOrigin = Part1By1(x) << 1, yTwiddleOrigin = Part1By1(y);
    uint32_t xTwiddle = xTwiddleOrigin, yTwiddle = yTwiddleOrigin;

    for (uint32_t yPos = 0; yPos < height; yPos += 2)
    {
        uint8_t *rowAddrs[2] = 
        {
            src + ((yPos + 0) * stride * pixelSize), 
            src + ((yPos + 1) * stride * pixelSize),
        };
        for (uint32_t xPos = 0; xPos < width; xPos += 2)
        {
            copyTile(dst + (xTwiddle + yTwiddle) * pixelSize, rowAddrs, loadAligned);
            xTwiddle = (xTwiddle - xTwiddleMask) & xTwiddleMask;
            rowAddrs[0] += (2 * pixelSize);
            rowAddrs[1] += (2 * pixelSize);
        }
        xTwiddle = xTwiddleOrigin;
        yTwiddle = (yTwiddle - yTwiddleMask) & yTwiddleMask;
    }
}

template <uint32_t pixelSize, CopyPixel copyPixel>
static void SwizzleTexData1x1(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    const uint32_t xTwiddleMask = 0xAAAAAAAA | ~((tileSize * tileSize) - 1), yTwiddleMask = 0x55555555 | ~((tileSize * tileSize) - 1);
    const uint32_t xTwiddleOrigin = Part1By1(x) << 1, yTwiddleOrigin = Part1By1(y);
    uint32_t xTwiddle = xTwiddleOrigin, yTwiddle = yTwiddleOrigin;

    for (uint32_t yPos = 0; yPos < height; yPos++)
    {
        uint8_t *rowAddr = src + (yPos * stride * pixelSize);
        for (uint32_t xPos = 0; xPos < width; xPos++)
        {
            copyPixel(dst + (xTwiddle + yTwiddle) * pixelSize, rowAddr + (xPos * pixelSize));
            xTwiddle = (xTwiddle - xTwiddleMask) & xTwiddleMask;
        }
        xTwiddle = xTwiddleOrigin;
        yTwiddle = (yTwiddle - yTwiddleMask) & yTwiddleMask;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
template<uint32_t pixelSize, CopyPixel copyPixel, CopyTile copyTile>
static void SwizzleTexData(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    if (copyTile != nullptr)
    {
        if (((x | y | width | height) & 0x7) == 0 && (pixelSize == 1))
            SwizzleTexData8x8<pixelSize, copyPixel, copyTile>(dst, src, x, y, width, height, stride, tileSize);
        else if (((x | y | width | height) & 0x3) == 0 && (pixelSize == 2 || pixelSize == 4))
            SwizzleTexData4x4<pixelSize, copyPixel, copyTile>(dst, src, x, y, width, height, stride, tileSize);
        else if (((x | y | width | height) & 0x1) == 0 && (pixelSize == 8 || pixelSize == 16))
            SwizzleTexData2x2<pixelSize, copyPixel, copyTile>(dst, src, x, y, width, height, stride, tileSize);
        else
            SwizzleTexData1x1<pixelSize, copyPixel>(dst, src, x, y, width, height, stride, tileSize);
    }
    else
        SwizzleTexData1x1<pixelSize, copyPixel>(dst, src, x, y, width, height, stride, tileSize);
}
#pragma GCC diagnostic pop

#if defined(SUPPORT_SMALL_FMT)
void SwizzleTexData8Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    SwizzleTexData<1, CopyPixel8Bpp, CopyTile8Bpp>(dst, src, x, y, width, height, stride, tileSize);
}
void SwizzleTexData16Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    SwizzleTexData<2, CopyPixel16Bpp, CopyTile16Bpp>(dst, src, x, y, width, height, stride, tileSize);
}
void SwizzleTexData32Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    SwizzleTexData<4, CopyPixel32Bpp, CopyTile32Bpp>(dst, src, x, y, width, height, stride, tileSize);
}
#endif
#include <psp2/kernel/clib.h>

void SwizzleTexData64Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    SwizzleTexData<8, CopyPixel64Bpp, CopyTile64Bpp>(dst, src, x, y, width, height, stride, tileSize);
}
void SwizzleTexData128Bpp(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    SwizzleTexData<16, CopyPixel128Bpp, CopyTile128Bpp>(dst, src, x, y, width, height, stride, tileSize);
}
void SwizzleTexDataETC1(uint8_t *dst, uint8_t *src, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t stride, uint32_t tileSize)
{
    SwizzleTexData<8, CopyETCBlock, CopyETC1Tile>(dst, src, x, y, width, height, stride, tileSize);
}