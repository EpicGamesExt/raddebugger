#pragma once

// https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf

#include <stddef.h>
#include <stdint.h>

//
// interface
//

#define SHA1_DIGEST_SIZE    20
#define SHA1_BLOCK_SIZE     64

typedef struct {
    uint8_t buffer[SHA1_BLOCK_SIZE];
    uint64_t count;
    uint32_t state[5];
} sha1_ctx;

static inline void sha1_init(sha1_ctx* ctx);
static inline void sha1_update(sha1_ctx* ctx, const void* data, size_t size);
static inline void sha1_finish(sha1_ctx* ctx, uint8_t digest[SHA1_DIGEST_SIZE]);

//
// implementation
//

#include <string.h> // memcpy, memset

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wcast-align"
#   pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#   pragma clang diagnostic ignored "-Wlanguage-extension-token"
#   pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#elif defined(_MSC_VER)
#   pragma warning (push)
#   pragma warning (disable : 4127)
#endif

#if defined(__clang__)
#   define SHA1_ROL32(x,n) __builtin_rotateleft32(x, n)
#elif defined(_MSC_VER)
#   include <stdlib.h>
#   define SHA1_ROL32(x,n) _rotl(x, n)
#else
#   define SHA1_ROL32(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#endif

#if defined(_MSC_VER)
#   include <stdlib.h>
#   define SHA1_GET32BE(ptr) _byteswap_ulong( *((const _UNALIGNED uint32_t*)(ptr)) )
#   define SHA1_SET32BE(ptr,x) *((_UNALIGNED uint32_t*)(ptr)) = _byteswap_ulong(x)
#   define SHA1_SET64BE(ptr,x) *((_UNALIGNED uint64_t*)(ptr)) = _byteswap_uint64(x)
#else
#   define SHA1_GET32BE(ptr) \
    (                       \
        ((ptr)[0] << 24) |  \
        ((ptr)[1] << 16) |  \
        ((ptr)[2] <<  8) |  \
        ((ptr)[3] <<  0)    \
    )
#   define SHA1_SET32BE(ptr, x) do          \
    {                                       \
        (ptr)[0] = (uint8_t)((x) >> 24);    \
        (ptr)[1] = (uint8_t)((x) >> 16);    \
        (ptr)[2] = (uint8_t)((x) >>  8);    \
        (ptr)[3] = (uint8_t)((x) >>  0);    \
    }                                       \
    while (0)
#   define SHA1_SET64BE(ptr, x) do          \
    {                                       \
        (ptr)[0] = (uint8_t)((x) >> 56);    \
        (ptr)[1] = (uint8_t)((x) >> 48);    \
        (ptr)[2] = (uint8_t)((x) >> 40);    \
        (ptr)[3] = (uint8_t)((x) >> 32);    \
        (ptr)[4] = (uint8_t)((x) >> 24);    \
        (ptr)[5] = (uint8_t)((x) >> 16);    \
        (ptr)[6] = (uint8_t)((x) >>  8);    \
        (ptr)[7] = (uint8_t)((x) >>  0);    \
    }                                       \
    while (0)
#endif

#if defined(__x86_64__) || defined(_M_AMD64)

#include <tmmintrin.h> // SSSE3
#include <immintrin.h> // SHANI

#if defined(__clang__) || defined(__GNUC__)
#   include <cpuid.h>
#   define SHA1_TARGET(str)          __attribute__((target(str)))
#   define SHA1_CPUID(x, info)       __cpuid(x, info[0], info[1], info[2], info[3])
#   define SHA1_CPUID_EX(x, y, info) __cpuid_count(x, y, info[0], info[1], info[2], info[3])
#else
#   include <intrin.h>
#   define SHA1_TARGET(str)
#   define SHA1_CPUID(x, info)       __cpuid(info, x)
#   define SHA1_CPUID_EX(x, y, info) __cpuidex(info, x, y)
#endif

#define SHA1_CPUID_INIT  (1 << 0)
#define SHA1_CPUID_SHANI (1 << 1)

static inline int sha1_cpuid(void)
{
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
        int info[4];

        SHA1_CPUID(1, info);
        int has_ssse3 = info[3] & (1 << 9);

        SHA1_CPUID_EX(7, 0, info);
        int has_shani = info[1] & (1 << 29);

        result |= SHA1_CPUID_INIT;
        if (has_ssse3 && has_shani)
        {
            result |= SHA1_CPUID_SHANI;
        }

        cpuid = result;
    }

#if defined(SHA1_CPUID_MASK)
    result &= SHA1_CPUID_MASK;
#endif

    return result;
}

SHA1_TARGET("ssse3,sha")
static void sha1_process_shani(uint32_t* state, const uint8_t* block, size_t count)
{
    const __m128i* buffer = (const __m128i*)block;

    // for performing two operations in one:
    // 1) dwords need to be loaded as big-endian
    // 2) order of dwords need to be reversed for sha instructions: [0,1,2,3] -> [3,2,1,0]
    const __m128i bswap = _mm_setr_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);

    #define W(i) w[(i)%4]

    // 4 wide round calculations
    #define QROUND(i) do {                                                          \
        /* first four rounds loads input message */                                 \
        if (i < 4) W(i) = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[i]), bswap);     \
        /* update previous message dwords for next rounds */                        \
        if (i > 0 && i < 17) W(i-1) = _mm_sha1msg1_epu32(W(i-1), W(i));             \
        if (i > 1 && i < 18) W(i-2) = _mm_xor_si128(W(i-2), W(i));                  \
        if (i > 2 && i < 19) W(i-3) = _mm_sha1msg2_epu32(W(i-3), W(i));             \
        /* calculate E from message dwords */                                       \
        if (i == 0) tmp = _mm_add_epi32(e0, W(i));                                  \
        if (i != 0) tmp = _mm_sha1nexte_epu32(e0, W(i));                            \
        /* round function */                                                        \
        e0 = abcd;                                                                  \
        abcd = _mm_sha1rnds4_epu32(abcd, tmp, (i/5)%4);                             \
    } while(0)

    // load initial state
    __m128i abcd = _mm_loadu_si128((const __m128i*)state); // [d,c,b,a]
    __m128i e0 = _mm_loadu_si32(&state[4]);                // [0,0,0,e]

    // change dword order
    abcd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(0,1,2,3)); // [a,b,c,d] where a is in the top lane
    e0 = _mm_slli_si128(e0, 12);                          // [e,0,0,0] where e is in top lane

    do
    {
        // remember current state
        __m128i last_abcd = abcd;
        __m128i last_e0 = e0;

        __m128i tmp, w[4];

        QROUND(0);
        QROUND(1);
        QROUND(2);
        QROUND(3);
        QROUND(4);
        QROUND(5);
        QROUND(6);
        QROUND(7);
        QROUND(8);
        QROUND(9);
        QROUND(10);
        QROUND(11);
        QROUND(12);
        QROUND(13);
        QROUND(14);
        QROUND(15);
        QROUND(16);
        QROUND(17);
        QROUND(18);
        QROUND(19);

        // update next state
        abcd = _mm_add_epi32(abcd, last_abcd);
        e0 = _mm_sha1nexte_epu32(e0, last_e0);

        buffer += 4;
    }
    while (--count);

    // restore dword order
    abcd = _mm_shuffle_epi32(abcd, _MM_SHUFFLE(0,1,2,3));
    e0   = _mm_shuffle_epi32(e0,   _MM_SHUFFLE(0,1,2,3));

    // save the new state
    _mm_storeu_si128((__m128i*)state, abcd);
    _mm_storeu_si32(&state[4], e0);

    #undef QROUND
    #undef W
}

#endif // defined(__x86_64__) || defined(_M_AMD64)

static void sha1_process(uint32_t* state, const uint8_t* block, size_t count)
{
#if defined(__x86_64__) || defined(_M_AMD64)
    int cpuid = sha1_cpuid();
    if (cpuid & SHA1_CPUID_SHANI)
    {
        sha1_process_shani(state, block, count);
        return;
    }
#endif

    #define F1(x,y,z) (0x5a827999 + ((x & (y ^ z)) ^ z))
    #define F2(x,y,z) (0x6ed9eba1 + (x ^ y ^ z))
    #define F3(x,y,z) (0x8f1bbcdc + ((x & y) | (z & (x | y))))
    #define F4(x,y,z) (0xca62c1d6 + (x ^ y ^ z))

    #define W(i) w[(i+16)%16]

    #define ROUND(i,a,b,c,d,e,F) do                                                     \
    {                                                                                   \
        uint32_t w0;                                                                    \
        if (i <  16) W(i) = w0 = SHA1_GET32BE(block + i*sizeof(uint32_t));              \
        if (i >= 16) W(i) = w0 = SHA1_ROL32(W(i-3) ^ W(i-8) ^ W(i-14) ^ W(i-16), 1);    \
                                                                                        \
        e += SHA1_ROL32(a,5) + F(b,c,d) + w0;                                           \
        b = SHA1_ROL32(b,30);                                                           \
    } while (0)

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];

    do
    {
        uint32_t last_a = a;
        uint32_t last_b = b;
        uint32_t last_c = c;
        uint32_t last_d = d;
        uint32_t last_e = e;

        uint32_t w[16];

        ROUND( 0, a, b, c, d, e, F1);
        ROUND( 1, e, a, b, c, d, F1);
        ROUND( 2, d, e, a, b, c, F1);
        ROUND( 3, c, d, e, a, b, F1);
        ROUND( 4, b, c, d, e, a, F1);
        ROUND( 5, a, b, c, d, e, F1);
        ROUND( 6, e, a, b, c, d, F1);
        ROUND( 7, d, e, a, b, c, F1);
        ROUND( 8, c, d, e, a, b, F1);
        ROUND( 9, b, c, d, e, a, F1);
        ROUND(10, a, b, c, d, e, F1);
        ROUND(11, e, a, b, c, d, F1);
        ROUND(12, d, e, a, b, c, F1);
        ROUND(13, c, d, e, a, b, F1);
        ROUND(14, b, c, d, e, a, F1);
        ROUND(15, a, b, c, d, e, F1);
        ROUND(16, e, a, b, c, d, F1);
        ROUND(17, d, e, a, b, c, F1);
        ROUND(18, c, d, e, a, b, F1);
        ROUND(19, b, c, d, e, a, F1);

        ROUND(20, a, b, c, d, e, F2);
        ROUND(21, e, a, b, c, d, F2);
        ROUND(22, d, e, a, b, c, F2);
        ROUND(23, c, d, e, a, b, F2);
        ROUND(24, b, c, d, e, a, F2);
        ROUND(25, a, b, c, d, e, F2);
        ROUND(26, e, a, b, c, d, F2);
        ROUND(27, d, e, a, b, c, F2);
        ROUND(28, c, d, e, a, b, F2);
        ROUND(29, b, c, d, e, a, F2);
        ROUND(30, a, b, c, d, e, F2);
        ROUND(31, e, a, b, c, d, F2);
        ROUND(32, d, e, a, b, c, F2);
        ROUND(33, c, d, e, a, b, F2);
        ROUND(34, b, c, d, e, a, F2);
        ROUND(35, a, b, c, d, e, F2);
        ROUND(36, e, a, b, c, d, F2);
        ROUND(37, d, e, a, b, c, F2);
        ROUND(38, c, d, e, a, b, F2);
        ROUND(39, b, c, d, e, a, F2);

        ROUND(40, a, b, c, d, e, F3);
        ROUND(41, e, a, b, c, d, F3);
        ROUND(42, d, e, a, b, c, F3);
        ROUND(43, c, d, e, a, b, F3);
        ROUND(44, b, c, d, e, a, F3);
        ROUND(45, a, b, c, d, e, F3);
        ROUND(46, e, a, b, c, d, F3);
        ROUND(47, d, e, a, b, c, F3);
        ROUND(48, c, d, e, a, b, F3);
        ROUND(49, b, c, d, e, a, F3);
        ROUND(50, a, b, c, d, e, F3);
        ROUND(51, e, a, b, c, d, F3);
        ROUND(52, d, e, a, b, c, F3);
        ROUND(53, c, d, e, a, b, F3);
        ROUND(54, b, c, d, e, a, F3);
        ROUND(55, a, b, c, d, e, F3);
        ROUND(56, e, a, b, c, d, F3);
        ROUND(57, d, e, a, b, c, F3);
        ROUND(58, c, d, e, a, b, F3);
        ROUND(59, b, c, d, e, a, F3);

        ROUND(60, a, b, c, d, e, F4);
        ROUND(61, e, a, b, c, d, F4);
        ROUND(62, d, e, a, b, c, F4);
        ROUND(63, c, d, e, a, b, F4);
        ROUND(64, b, c, d, e, a, F4);
        ROUND(65, a, b, c, d, e, F4);
        ROUND(66, e, a, b, c, d, F4);
        ROUND(67, d, e, a, b, c, F4);
        ROUND(68, c, d, e, a, b, F4);
        ROUND(69, b, c, d, e, a, F4);
        ROUND(70, a, b, c, d, e, F4);
        ROUND(71, e, a, b, c, d, F4);
        ROUND(72, d, e, a, b, c, F4);
        ROUND(73, c, d, e, a, b, F4);
        ROUND(74, b, c, d, e, a, F4);
        ROUND(75, a, b, c, d, e, F4);
        ROUND(76, e, a, b, c, d, F4);
        ROUND(77, d, e, a, b, c, F4);
        ROUND(78, c, d, e, a, b, F4);
        ROUND(79, b, c, d, e, a, F4);

        a += last_a;
        b += last_b;
        c += last_c;
        d += last_d;
        e += last_e;

        block += SHA1_BLOCK_SIZE;
    }
    while (--count);

    state[0] = a;
    state[1] = b;
    state[2] = c;
    state[3] = d;
    state[4] = e;

    #undef ROUND
    #undef W
    #undef F1
    #undef F2
    #undef F3
    #undef F4
}

void sha1_init(sha1_ctx* ctx)
{
    ctx->count = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xc3d2e1f0;
}

void sha1_update(sha1_ctx* ctx, const void* data, size_t size)
{
    const uint8_t* buffer = (const uint8_t*)data;

    size_t pending = ctx->count % SHA1_BLOCK_SIZE;
    ctx->count += size;

    size_t available = SHA1_BLOCK_SIZE - pending;
    if (pending && size >= available)
    {
        memcpy(ctx->buffer + pending, buffer, available);
        sha1_process(ctx->state, ctx->buffer, 1);
        buffer += available;
        size -= available;
        pending = 0;
    }

    size_t count = size / SHA1_BLOCK_SIZE;
    if (count)
    {
        sha1_process(ctx->state, buffer, count);
        buffer += count * SHA1_BLOCK_SIZE;
        size -= count * SHA1_BLOCK_SIZE;
    }

    memcpy(ctx->buffer + pending, buffer, size);
}

void sha1_finish(sha1_ctx* ctx, uint8_t digest[SHA1_DIGEST_SIZE])
{
    uint64_t count = ctx->count;
    uint64_t bitcount = count * 8;

    size_t pending = count % SHA1_BLOCK_SIZE;
    size_t blocks = pending < SHA1_BLOCK_SIZE - sizeof(bitcount) ? 1 : 2;

    ctx->buffer[pending++] = 0x80;

    uint8_t padding[2 * SHA1_BLOCK_SIZE];
    memcpy(padding, ctx->buffer, SHA1_BLOCK_SIZE);
    memset(padding + pending, 0, SHA1_BLOCK_SIZE);
    SHA1_SET64BE(padding + blocks * SHA1_BLOCK_SIZE - sizeof(bitcount), bitcount);

    sha1_process(ctx->state, padding, blocks);

    for (size_t i=0; i<5; i++)
    {
        SHA1_SET32BE(digest + i*sizeof(uint32_t), ctx->state[i]);
    }
}

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(_MSC_VER)
#   pragma warning (pop)
#endif
