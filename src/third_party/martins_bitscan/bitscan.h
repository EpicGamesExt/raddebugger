#pragma once

#include <stddef.h>
#include <stdint.h>

// returns index of bit that is equal to "bit" value in [lo,hi) interval starting from lo position (lsb->msb)
// returns "hi" value if none found, or interval is empty
// "bit" value must be 0 or 1
// "bits" array should be large enough to allow indexing it with [hi/32] or [hi/64] index

static inline size_t bitscan_lsb_index32(const uint32_t* bits, size_t lo, size_t hi, int bit);
static inline size_t bitscan_lsb_index64(const uint64_t* bits, size_t lo, size_t hi, int bit);

// returns index of bit that is equal to "bit" value in [lo,hi) interval starting from hi position (msb->lsb)
// returns "hi" value if none found, or interval is empty
// "bit" value must be 0 or 1
// "bits" array should be large enough to allow indexing it with [hi/32] or [hi/64] index

static inline size_t bitscan_msb_index32(const uint32_t* bits, size_t lo, size_t hi, int bit);
static inline size_t bitscan_msb_index64(const uint64_t* bits, size_t lo, size_t hi, int bit);

//
// implementation
//

// to run tests:
// cl.exe -O2 -fsanitize=address -DBITSCAN_TEST=1 -TC bitscan.h && bitscan.exe
// clang.exe -O2 -fsanitize=address,undefined -DBITSCAN_TEST=1 -x c bitscan.h && a.exe

#if !(defined(__GNUC__) || defined(__clang__))
#   include <intrin.h> // _BitScanForward/Reverse
#endif

#if defined(_M_AMD64) || defined(__x86_64__)
#   include <emmintrin.h> // SSE2
#endif

// count of zero bits, either starting from lsb (trailing), or from msb (leading)
// input value must be non-zero
// bit index is from 0 (lsb) to 31/63 (msb)

static inline size_t bitscan__ctz32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(x);
#else
    unsigned long index;
    _BitScanForward(&index, x);
    return index;
#endif
}

static inline size_t bitscan__ctz64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(x);
#else
    unsigned long index;
    _BitScanForward64(&index, x);
    return index;
#endif
}

static inline size_t bitscan__clz32(uint32_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clz(x);
#else
    unsigned long index;
    _BitScanReverse(&index, x);
    return 31 - index;
#endif
}

static inline size_t bitscan__clz64(uint64_t x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_clzll(x);
#else
    unsigned long index;
    _BitScanReverse64(&index, x);
    return 63 - index;
#endif
}

// returns index of first bit "1" in x when scanning from msb downwards
// bit index is from 0 (lsb) to 31/63 (msb)

static inline size_t bitscan__lindex32(uint32_t x)
{
    return 31 - bitscan__clz32(x);
}
static inline size_t bitscan__lindex64(uint64_t x)
{
    return 63 - bitscan__clz64(x);
}

size_t bitscan_lsb_index32(const uint32_t* bits, size_t lo, size_t hi, int bit)
{
#if 0

    // reference implementation
    for (size_t i = lo; i<hi; i++)
    {
        uint32_t word = bits[i/32];
        if (((word >> (i%32)) & 1) == bit)
        {
            return i;
        }
    }

#else

    if (lo >= hi)
    {
        return hi;
    }

    // to be able to use "ctz" for "find first zero bit"
    // do xor with this mask, it will flip all bit values
    // thus changing operation to "find first set bit" - which allows to use "ctz"
    const uint32_t mask = bit ? 0U : ~0U;

    size_t count = hi - lo;
    size_t offset = lo / 32;
    size_t first = (unsigned)(-(int)lo) % 32; // 0 if lo%32 == 0, otherwise (32 - lo%32) % 32

    if (first) // first < 32, how many max bits to use in top of first word
    {
        uint32_t word = bits[offset] ^ mask;

        // first = lo%32 = 18
        // count = 10
        // in cases count > first, clamp it to first
        //
        //   3         2         1         0
        //  10987654321098765432109876543210 bit index
        //  ....xxxxxxxxxx..................
        // ^   ^         ^
        // |   |         |
        // |   |         +------------------ lo%32 = 18
        // |   |         |
        // |   +- count -+                   count = 10 bits to process
        // |             |
        // +--- first ---+                   first = 32 - lo%32 = 14

        size_t n = (count < first ? count : first);
        word &= (~0U >> (32 - n)) << (lo % 32);

        if (word)
        {
            return offset * 32 + bitscan__ctz32(word);
        }
        offset += 1;
        count -= n;
    }

#if defined(_M_AMD64) || defined(__x86_64__)
    while (count >= 128)
    {
        __m128i words = _mm_loadu_si128((const __m128i*)&bits[offset]);
        __m128i cmp = _mm_cmpeq_epi32(words, _mm_set1_epi32(mask));

        // if all 16-bytes of "words" are same as mask, then m will be 0
        uint16_t m = 1 + (uint16_t)_mm_movemask_epi8(cmp);
        if (m) 
        {
            // find word index to use [0,4)
            size_t n = bitscan__ctz32(m) / 4;

            uint32_t word = bits[offset + n] ^ mask;
            return offset * 32 + n * 32 + bitscan__ctz32(word);
        }

        offset += 4;
        count -= 128;
    }
#endif

    while (count >= 32)
    {
        uint32_t word = bits[offset] ^ mask;
        if (word)
        {
            return offset * 32 + bitscan__ctz32(word);
        }
        offset += 1;
        count -= 32;
    }

    if (count) // now count < 32, how many bits to process in bottom of last word
    {
        uint32_t word = bits[offset] ^ mask;

        // use first count bits, rest of bits are masked out to 1
        word |= ~0U << count;

        if (word)
        {
            return offset * 32 + bitscan__ctz32(word);
        }
    }

#endif

    return hi;
}

size_t bitscan_lsb_index64(const uint64_t* bits, size_t lo, size_t hi, int bit)
{
#if 0

    // reference implementation
    for (size_t i = lo; i < hi; i++)
    {
        uint64_t word = bits[i / 64];
        if (((word >> (i % 64)) & 1) == bit)
        {
            return i;
        }
    }

#else

    if (lo >= hi)
    {
        return hi;
    }

    const uint64_t mask = bit ? 0ULL : ~0ULL;

    size_t count = hi - lo;
    size_t offset = lo / 64;
    size_t first = (unsigned)(-(int)lo) % 64;

    if (first)
    {
        uint64_t word = bits[offset] ^ mask;

        size_t n = (count < first ? count : first);
        word &= (~0ULL >> (64 - n)) << (lo % 64);

        if (word)
        {
            return offset * 64 + bitscan__ctz64(word);
        }
        offset += 1;
        count -= n;
    }

#if defined(_M_AMD64) || defined(__x86_64__)
    while (count >= 128)
    {
        __m128i words = _mm_loadu_si128((const __m128i*)&bits[offset]);
        __m128i cmp = _mm_cmpeq_epi32(words, _mm_set1_epi32((uint32_t)mask));
        uint16_t m = 1 + (uint16_t)_mm_movemask_epi8(cmp);
        if (m)
        {
            size_t n = bitscan__ctz32(m) / 8;

            uint64_t word = bits[offset + n] ^ mask;
            return offset * 64 + n * 64 + bitscan__ctz64(word);
        }

        offset += 2;
        count -= 128;
    }
#endif

    while (count >= 64)
    {
        uint64_t word = bits[offset] ^ mask;
        if (word)
        {
            return offset * 64 + bitscan__ctz64(word);
        }
        offset += 1;
        count -= 64;
    }

    if (count)
    {
        uint64_t word = bits[offset] ^ mask;
        word |= ~0ULL << count;

        if (word)
        {
            return offset * 64 + bitscan__ctz64(word);
        }
    }

#endif

    return hi;
}

size_t bitscan_msb_index32(const uint32_t* bits, size_t lo, size_t hi, int bit)
{
#if 0

    // reference implementation
    for (size_t i = hi; i-- > lo; )
    {
        uint32_t word = bits[i/32];
        if (((word >> (i%32)) & 1) == bit)
        {
            return i;
        }
    }

#else

    if (lo >= hi)
    {
        return hi;
    }

    // to be able to use "clz" for "find last zero bit"
    // do xor with this mask, it will flip all bit values
    // thus changing operation to "find last set bit" - which allows to use "clz"
    const uint32_t mask = bit ? 0U : ~0U;

    size_t count = hi - lo;
    size_t offset = (hi - 1) / 32;
    size_t first = hi % 32;

    if (first) // first < 32, how many max bits to use in bottom of first word
    {
        uint32_t word = bits[offset] ^ mask;

        // first = hi%32 = 14
        // count = 10
        // in cases count > first, clamp it to first
        //
        //   3          2         1        0
        //  10987654321098765432109876543210 bit index
        //  ..................xxxxxxxxxx....
        //                   ^         ^
        //                   |         |
        //                   +- count -+     count = 10 bits to process
        //                   |
        //                   +-------------- first = 14

        size_t n = (count < first ? count : first);
        word &= (1U << first) - (1U << (first - n));

        if (word)
        {
            return offset * 32 + bitscan__lindex32(word);
        }
        offset -= 1;
        count -= n;
    }

#if defined(_M_AMD64) || defined(__x86_64__)
    while (count >= 128)
    {
        __m128i words = _mm_loadu_si128((const __m128i*)&bits[offset - 3]);
        __m128i cmp = _mm_cmpeq_epi32(words, _mm_set1_epi32(mask));

        // if all 16-bytes of "words" are same as mask, then m will be 0xffff
        uint16_t m = (uint16_t)_mm_movemask_epi8(cmp);

        if ((uint16_t)(m + 1))
        {
            // find word index to use [0,4)
            size_t n = bitscan__lindex32((uint16_t)~m) / 4;

            uint32_t word = bits[offset + n - 3] ^ mask;
            return offset * 32 + (n - 3) * 32 + bitscan__lindex32(word);
        }
        offset -= 4;
        count -= 128;
    }
#endif

    while (count >= 32)
    {
        uint32_t word = bits[offset] ^ mask;
        if (word)
        {
            return offset * 32 + bitscan__lindex32(word);
        }
        offset -= 1;
        count -= 32;
    }

    if (count) // now count < 32, how many bits to process in top of last word
    {
        uint32_t word = bits[offset] ^ mask;

        // use last count bits, rest of bits are masked out to 0
        word &= ~0U << (32 - count);

        if (word)
        {
            return offset * 32 + bitscan__lindex32(word);
        }
    }

#endif

    return hi;
}

size_t bitscan_msb_index64(const uint64_t* bits, size_t lo, size_t hi, int bit)
{
#if 0

    // reference implementation
    for (size_t i = hi; i-- > lo; )
    {
        uint64_t word = bits[i/64];
        if (((word >> (i%64)) & 1) == bit)
        {
            return i;
        }
    }

#else

    if (lo >= hi)
    {
        return hi;
    }

    const uint64_t mask = bit ? 0ULL : ~0ULL;

    size_t count = hi - lo;
    size_t offset = (hi - 1) / 64;
    size_t first = hi % 64;

    if (first)
    {
        uint64_t word = bits[offset] ^ mask;

        size_t n = (count < first ? count : first);
        word &= (1ULL << first) - (1ULL << (first - n));

        if (word)
        {
            return offset * 64 + bitscan__lindex64(word);
        }
        offset -= 1;
        count -= n;
    }

#if defined(_M_AMD64) || defined(__x86_64__)
    while (count >= 128)
    {
        __m128i words = _mm_loadu_si128((const __m128i*)&bits[offset - 1]);
        __m128i cmp = _mm_cmpeq_epi32(words, _mm_set1_epi32(mask));
        uint16_t m = (uint16_t)_mm_movemask_epi8(cmp);
        if ((uint16_t)(m + 1))
        {
            size_t n = bitscan__lindex32((uint16_t)~m) / 8;

            uint64_t word = bits[offset + n - 1] ^ mask;
            return offset * 64 + (n - 1) * 64 + bitscan__lindex64(word);
        }
        offset -= 2;
        count -= 128;
    }
#endif

    while (count >= 64)
    {
        uint64_t word = bits[offset] ^ mask;
        if (word)
        {
            return offset * 64 + bitscan__lindex64(word);
        }
        offset -= 1;
        count -= 64;
    }

    if (count)
    {
        uint64_t word = bits[offset] ^ mask;
        word &= ~0ULL << (64 - count);

        if (word)
        {
            return offset * 64 + bitscan__lindex64(word);
        }
    }

#endif

    return hi;
}

#if defined(BITSCAN_TEST)

#include <assert.h>
#include <stdio.h>

static uint64_t random64()
{
    static uint64_t x = 0, w = 0;
    x = x*x + (w += 0xb5ad4eceda1ce2a9);
    return x = (x>>32) | (x<<32);
}

#define BIT32_SET(v,i)   v[(i)/32] |=   1U << ((i)%32)
#define BIT32_CLEAR(v,i) v[(i)/32] &= ~(1U << ((i)%32))

#define BIT64_SET(v,i)   v[(i)/64] |=   1ULL << ((i)%64)
#define BIT64_CLEAR(v,i) v[(i)/64] &= ~(1ULL << ((i)%64))

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

int main()
{
    enum { kBitCount = 512 };
    enum { kRngCount = 32 };

    static const size_t offsets[] =
    {
        0, 1, 10, 31, 32, 33, 50, 63, 64, 65, 130, kBitCount-65, kBitCount-64, kBitCount-63, kBitCount-33, kBitCount-32, kBitCount-31, kBitCount-1, kBitCount,
    };
    size_t offset_count = sizeof(offsets) / sizeof(offsets[0]);

    for (size_t n=0; n<=kBitCount; n++)
    {
        printf("."); fflush(stdout);

        // bitscan_lsb_index32

        for (size_t r=0; r<kRngCount; r++)
        {
            uint32_t v[kBitCount/32];
            for (size_t i=0; i<kBitCount/32; i++) v[i] = (uint32_t)random64();

            for (size_t olo=0; olo<offset_count; olo++)
            for (size_t ohi=0; ohi<offset_count; ohi++)
            {
                size_t lo = offsets[olo];
                size_t hi = offsets[ohi];
                if (lo >= n) continue;

                size_t r;
                size_t expected = lo >= hi || hi < n ? hi : n;

                // first n bits are 0, then bit 1 when possible
                for (size_t i=0; i<n; i++) BIT32_CLEAR(v, i);
                if (n < kBitCount)         BIT32_SET(v, n);

                r = bitscan_lsb_index32(v, lo, hi, 1);
                assert(r == expected);

                // first n bits are 1, then bit 0 when possible
                for (size_t i=0; i<n; i++) BIT32_SET(v, i);
                if (n < kBitCount)         BIT32_CLEAR(v, n);

                r = bitscan_lsb_index32(v, lo, hi, 0);
                assert(r == expected);
            }
        }

        // bitscan_msb_index32

        for (size_t r=0; r<kRngCount; r++)
        {
            uint32_t v[kBitCount/32];
            for (size_t i=0; i<kBitCount/32; i++) v[i] = (uint32_t)random64();

            for (size_t olo=0; olo<offset_count; olo++)
            for (size_t ohi=0; ohi<offset_count; ohi++)
            {
                size_t lo = offsets[olo];
                size_t hi = offsets[ohi];
                if (hi < kBitCount - n) continue;

                size_t r;
                size_t expected = lo >= hi || lo >= kBitCount-n ? hi : MIN(hi-1, kBitCount-1-n);

                // last n bits are 0, then bit 1 when possible
                for (size_t i=kBitCount-n; i<kBitCount; i++) BIT32_CLEAR(v, i);
                if (n < kBitCount)                           BIT32_SET(v, kBitCount-n-1);

                r = bitscan_msb_index32(v, lo, hi, 1);
                assert(r == expected);

                // last n bits are 1, then bit 0 when possible
                for (size_t i=kBitCount-n; i<kBitCount; i++) BIT32_SET(v, i);
                if (n < kBitCount)                           BIT32_CLEAR(v, kBitCount-n-1);

                r = bitscan_msb_index32(v, lo, hi, 0);
                assert(r == expected);
            }
        }

        // bitscan_lsb_index64

        for (size_t r = 0; r < kRngCount; r++)
        {
            uint64_t v[kBitCount / 64];
            for (size_t i = 0; i < kBitCount / 64; i++) v[i] = random64();

            for (size_t olo = 0; olo < offset_count; olo++)
                for (size_t ohi = 0; ohi < offset_count; ohi++)
                {
                    size_t lo = offsets[olo];
                    size_t hi = offsets[ohi];
                    if (lo >= n) continue;

                    size_t r;
                    size_t expected = lo >= hi || hi < n ? hi : n;

                    // first n bits are 0, then bit 1 when possible
                    for (size_t i = 0; i < n; i++) BIT64_CLEAR(v, i);
                    if (n < kBitCount)             BIT64_SET(v, n);

                    r = bitscan_lsb_index64(v, lo, hi, 1);
                    assert(r == expected);

                    // first n bits are 1, then bit 0 when possible
                    for (size_t i = 0; i < n; i++) BIT64_SET(v, i);
                    if (n < kBitCount)             BIT64_CLEAR(v, n);

                    r = bitscan_lsb_index64(v, lo, hi, 0);
                    assert(r == expected);
                }
        }

        // bitscan_msb_index64

        for (size_t r = 0; r < kRngCount; r++)
        {
            uint64_t v[kBitCount / 64];
            for (size_t i = 0; i < kBitCount / 64; i++) v[i] = random64();

            for (size_t olo = 0; olo < offset_count; olo++)
                for (size_t ohi = 0; ohi < offset_count; ohi++)
                {
                    size_t lo = offsets[olo];
                    size_t hi = offsets[ohi];
                    if (hi < kBitCount - n) continue;

                    size_t r;
                    size_t expected = lo >= hi || lo >= kBitCount-n ? hi : MIN(hi-1, kBitCount-1-n);

                    // last n bits are 0, then bit 1 when possible
                    for (size_t i = kBitCount - n; i < kBitCount; i++) BIT64_CLEAR(v, i);
                    if (n < kBitCount)                                 BIT64_SET(v, kBitCount-n-1);

                    r = bitscan_msb_index64(v, lo, hi, 1);
                    assert(r == expected);

                    // last n bits are 1, then bit 0 when possible
                    for (size_t i = kBitCount - n; i < kBitCount; i++) BIT64_SET(v, i);
                    if (n < kBitCount)                                 BIT64_CLEAR(v, kBitCount-n-1);

                    r = bitscan_msb_index64(v, lo, hi, 0);
                    assert(r == expected);
                }
        }
    }
    printf(" OK!\n");
}

#endif
