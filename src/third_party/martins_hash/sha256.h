#pragma once

// https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
// https://www.rfc-editor.org/rfc/rfc6234

#include <stddef.h>
#include <stdint.h>

//
// interface
//

#define SHA224_DIGEST_SIZE  28
#define SHA256_DIGEST_SIZE  32
#define SHA256_BLOCK_SIZE   64

typedef struct {
    uint8_t buffer[SHA256_BLOCK_SIZE];
    uint64_t count;
    uint32_t state[8];
} sha256_ctx;

typedef sha256_ctx sha224_ctx;

static inline void sha256_init(sha256_ctx* ctx);
static inline void sha256_update(sha256_ctx* ctx, const void* data, size_t size);
static inline void sha256_finish(sha256_ctx* ctx, uint8_t digest[SHA256_DIGEST_SIZE]);

static inline void sha224_init(sha224_ctx* ctx);
static inline void sha224_update(sha224_ctx* ctx, const void* data, size_t size);
static inline void sha224_finish(sha224_ctx* ctx, uint8_t digest[SHA224_DIGEST_SIZE]);

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
#   define SHA256_ROR32(x,n) __builtin_rotateright32(x, n)
#elif defined(_MSC_VER)
#   include <stdlib.h>
#   define SHA256_ROR32(x,n) _rotr(x, n)
#else
#   define SHA256_ROR32(x,n) ( ((x) >> (n)) | ((x) << (32-(n))) )
#endif

#if defined(_MSC_VER)
#   include <stdlib.h>
#   define SHA256_GET32BE(ptr) _byteswap_ulong( *((const _UNALIGNED uint32_t*)(ptr)) )
#   define SHA256_SET32BE(ptr,x) *((_UNALIGNED uint32_t*)(ptr)) = _byteswap_ulong(x)
#   define SHA256_SET64BE(ptr,x) *((_UNALIGNED uint64_t*)(ptr)) = _byteswap_uint64(x)
#else
#   define SHA256_GET32BE(ptr)  \
    (                           \
        ((ptr)[0] << 24) |      \
        ((ptr)[1] << 16) |      \
        ((ptr)[2] <<  8) |      \
        ((ptr)[3] <<  0)        \
    )
#   define SHA256_SET32BE(ptr, x) do        \
    {                                       \
        (ptr)[0] = (uint8_t)((x) >> 24);    \
        (ptr)[1] = (uint8_t)((x) >> 16);    \
        (ptr)[2] = (uint8_t)((x) >>  8);    \
        (ptr)[3] = (uint8_t)((x) >>  0);    \
    }                                       \
    while (0)
#   define SHA256_SET64BE(ptr, x) do        \
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
#   define SHA256_TARGET(str)          __attribute__((target(str)))
#   define SHA256_CPUID(x, info)       __cpuid(x, info[0], info[1], info[2], info[3])
#   define SHA256_CPUID_EX(x, y, info) __cpuid_count(x, y, info[0], info[1], info[2], info[3])
#else
#   include <intrin.h>
#   define SHA256_TARGET(str)
#   define SHA256_CPUID(x, info)       __cpuid(info, x)
#   define SHA256_CPUID_EX(x, y, info) __cpuidex(info, x, y)
#endif

#define SHA256_CPUID_INIT  (1 << 0)
#define SHA256_CPUID_SHANI (1 << 1)

static inline int sha256_cpuid(void)
{
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
        int info[4];

        SHA256_CPUID(1, info);
        int has_ssse3 = info[3] & (1 << 9);

        SHA256_CPUID_EX(7, 0, info);
        int has_shani = info[1] & (1 << 29);

        result |= SHA256_CPUID_INIT;
        if (has_ssse3 && has_shani)
        {
            result |= SHA256_CPUID_SHANI;
        }

        cpuid = result;
    }

#if defined(SHA256_CPUID_MASK)
    result &= SHA256_CPUID_MASK;
#endif

    return result;
}

SHA256_TARGET("ssse3,sha")
static void sha256_process_shani(uint32_t* state, const uint8_t* block, size_t count)
{
    const __m128i* buffer = (const __m128i*)block;

    // to byteswap when doing big-ending load for message dwords
    const __m128i bswap = _mm_setr_epi8(3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12);

    static const uint32_t K[16][4] =
    {
        { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5 },
        { 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5 },
        { 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3 },
        { 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174 },
        { 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc },
        { 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da },
        { 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7 },
        { 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967 },
        { 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13 },
        { 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85 },
        { 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3 },
        { 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070 },
        { 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5 },
        { 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3 },
        { 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208 },
        { 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 },
    };

    #define W(i) w[(i)%4]

    // 4 wide round calculations
    #define QROUND(i) do {                                                                                                  \
        /* first four rounds loads input message */                                                                         \
        if (i < 4) W(i) = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[i]), bswap);                                             \
        /* add round constant */                                                                                            \
        tmp = _mm_add_epi32(W(i), _mm_loadu_si128((const __m128i*)K[i]));                                                   \
        /* update previous message dwords for next rounds */                                                                \
        if (i > 2 && i < 15) W(i-3) = _mm_sha256msg2_epu32(_mm_add_epi32(W(i-3), _mm_alignr_epi8(W(i), W(i-1), 4)), W(i));  \
        if (i > 0 && i < 13) W(i-1) = _mm_sha256msg1_epu32(W(i-1), W(i));                                                   \
        /* round functions */                                                                                               \
        state1 = _mm_sha256rnds2_epu32(state1, state0, tmp);                                                                \
        state0 = _mm_sha256rnds2_epu32(state0, state1, _mm_shuffle_epi32(tmp, _MM_SHUFFLE(0,0,3,2)));                       \
    } while(0)
        
    // load initial state 
    __m128i abcd = _mm_shuffle_epi32(_mm_loadu_si128((const __m128i*)&state[0]), _MM_SHUFFLE(0,1,2,3)); // [a,b,c,d]
    __m128i efgh = _mm_shuffle_epi32(_mm_loadu_si128((const __m128i*)&state[4]), _MM_SHUFFLE(0,1,2,3)); // [e,f,g,h]

    // dword order for sha256rnds2 instruction
    __m128i state0 = _mm_unpackhi_epi64(efgh, abcd); // [a,b,e,f]
    __m128i state1 = _mm_unpacklo_epi64(efgh, abcd); // [c,d,g,h]

    do
    {
        // remember current state
        __m128i last0 = state0;
        __m128i last1 = state1;

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

        // update next state
        state0 = _mm_add_epi32(state0, last0);
        state1 = _mm_add_epi32(state1, last1);

        buffer += 4;
    }
    while (--count);

    // restore dword order
    abcd = _mm_unpackhi_epi64(state1, state0);
    efgh = _mm_unpacklo_epi64(state1, state0);

    // save the new state
    _mm_storeu_si128((__m128i*)&state[0], _mm_shuffle_epi32(abcd, _MM_SHUFFLE(0,1,2,3)));
    _mm_storeu_si128((__m128i*)&state[4], _mm_shuffle_epi32(efgh, _MM_SHUFFLE(0,1,2,3)));

    #undef QROUND
    #undef W
}

#endif // defined(__x86_64__) || defined(_M_AMD64)

static void sha256_process(uint32_t* state, const uint8_t* block, size_t count)
{
#if defined(__x86_64__) || defined(_M_AMD64)
    int cpuid = sha256_cpuid();
    if (cpuid & SHA256_CPUID_SHANI)
    {
        sha256_process_shani(state, block, count);
        return;
    }
#endif

    #define Ch(x,y,z)  ((x & (y ^ z)) ^ z)
    #define Maj(x,y,z) ((x & y) | (z & (x | y)))

    #define BSig0(x) (SHA256_ROR32(x,  2) ^ SHA256_ROR32(x, 13) ^ SHA256_ROR32(x, 22))
    #define BSig1(x) (SHA256_ROR32(x,  6) ^ SHA256_ROR32(x, 11) ^ SHA256_ROR32(x, 25))
    #define SSig0(x) (SHA256_ROR32(x,  7) ^ SHA256_ROR32(x, 18) ^ (x >> 3))
    #define SSig1(x) (SHA256_ROR32(x, 17) ^ SHA256_ROR32(x, 19) ^ (x >> 10))

    #define W(i) w[(i+16)%16]

    #define ROUND(i,a,b,c,d,e,f,g,h,K) do                                           \
    {                                                                               \
        uint32_t w0;                                                                \
        if (i <  16) W(i) = w0 = SHA256_GET32BE(block + i*sizeof(uint32_t));        \
        if (i >= 16) W(i) = w0 = SSig1(W(i-2)) + W(i-7) + SSig0(W(i-15)) + W(i-16); \
                                                                                    \
        uint32_t t1 = h + BSig1(e) + Ch(e,f,g) + K + w0;                            \
        uint32_t t2 = BSig0(a) + Maj(a,b,c);                                        \
        d += t1;                                                                    \
        h = t1 + t2;                                                                \
    } while (0)

    do
    {
        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        uint32_t w[16];

        ROUND( 0, a, b, c, d, e, f, g, h, 0x428a2f98);
        ROUND( 1, h, a, b, c, d, e, f, g, 0x71374491);
        ROUND( 2, g, h, a, b, c, d, e, f, 0xb5c0fbcf);
        ROUND( 3, f, g, h, a, b, c, d, e, 0xe9b5dba5);
        ROUND( 4, e, f, g, h, a, b, c, d, 0x3956c25b);
        ROUND( 5, d, e, f, g, h, a, b, c, 0x59f111f1);
        ROUND( 6, c, d, e, f, g, h, a, b, 0x923f82a4);
        ROUND( 7, b, c, d, e, f, g, h, a, 0xab1c5ed5);
        ROUND( 8, a, b, c, d, e, f, g, h, 0xd807aa98);
        ROUND( 9, h, a, b, c, d, e, f, g, 0x12835b01);
        ROUND(10, g, h, a, b, c, d, e, f, 0x243185be);
        ROUND(11, f, g, h, a, b, c, d, e, 0x550c7dc3);
        ROUND(12, e, f, g, h, a, b, c, d, 0x72be5d74);
        ROUND(13, d, e, f, g, h, a, b, c, 0x80deb1fe);
        ROUND(14, c, d, e, f, g, h, a, b, 0x9bdc06a7);
        ROUND(15, b, c, d, e, f, g, h, a, 0xc19bf174);
        ROUND(16, a, b, c, d, e, f, g, h, 0xe49b69c1);
        ROUND(17, h, a, b, c, d, e, f, g, 0xefbe4786);
        ROUND(18, g, h, a, b, c, d, e, f, 0x0fc19dc6);
        ROUND(19, f, g, h, a, b, c, d, e, 0x240ca1cc);
        ROUND(20, e, f, g, h, a, b, c, d, 0x2de92c6f);
        ROUND(21, d, e, f, g, h, a, b, c, 0x4a7484aa);
        ROUND(22, c, d, e, f, g, h, a, b, 0x5cb0a9dc);
        ROUND(23, b, c, d, e, f, g, h, a, 0x76f988da);
        ROUND(24, a, b, c, d, e, f, g, h, 0x983e5152);
        ROUND(25, h, a, b, c, d, e, f, g, 0xa831c66d);
        ROUND(26, g, h, a, b, c, d, e, f, 0xb00327c8);
        ROUND(27, f, g, h, a, b, c, d, e, 0xbf597fc7);
        ROUND(28, e, f, g, h, a, b, c, d, 0xc6e00bf3);
        ROUND(29, d, e, f, g, h, a, b, c, 0xd5a79147);
        ROUND(30, c, d, e, f, g, h, a, b, 0x06ca6351);
        ROUND(31, b, c, d, e, f, g, h, a, 0x14292967);
        ROUND(32, a, b, c, d, e, f, g, h, 0x27b70a85);
        ROUND(33, h, a, b, c, d, e, f, g, 0x2e1b2138);
        ROUND(34, g, h, a, b, c, d, e, f, 0x4d2c6dfc);
        ROUND(35, f, g, h, a, b, c, d, e, 0x53380d13);
        ROUND(36, e, f, g, h, a, b, c, d, 0x650a7354);
        ROUND(37, d, e, f, g, h, a, b, c, 0x766a0abb);
        ROUND(38, c, d, e, f, g, h, a, b, 0x81c2c92e);
        ROUND(39, b, c, d, e, f, g, h, a, 0x92722c85);
        ROUND(40, a, b, c, d, e, f, g, h, 0xa2bfe8a1);
        ROUND(41, h, a, b, c, d, e, f, g, 0xa81a664b);
        ROUND(42, g, h, a, b, c, d, e, f, 0xc24b8b70);
        ROUND(43, f, g, h, a, b, c, d, e, 0xc76c51a3);
        ROUND(44, e, f, g, h, a, b, c, d, 0xd192e819);
        ROUND(45, d, e, f, g, h, a, b, c, 0xd6990624);
        ROUND(46, c, d, e, f, g, h, a, b, 0xf40e3585);
        ROUND(47, b, c, d, e, f, g, h, a, 0x106aa070);
        ROUND(48, a, b, c, d, e, f, g, h, 0x19a4c116);
        ROUND(49, h, a, b, c, d, e, f, g, 0x1e376c08);
        ROUND(50, g, h, a, b, c, d, e, f, 0x2748774c);
        ROUND(51, f, g, h, a, b, c, d, e, 0x34b0bcb5);
        ROUND(52, e, f, g, h, a, b, c, d, 0x391c0cb3);
        ROUND(53, d, e, f, g, h, a, b, c, 0x4ed8aa4a);
        ROUND(54, c, d, e, f, g, h, a, b, 0x5b9cca4f);
        ROUND(55, b, c, d, e, f, g, h, a, 0x682e6ff3);
        ROUND(56, a, b, c, d, e, f, g, h, 0x748f82ee);
        ROUND(57, h, a, b, c, d, e, f, g, 0x78a5636f);
        ROUND(58, g, h, a, b, c, d, e, f, 0x84c87814);
        ROUND(59, f, g, h, a, b, c, d, e, 0x8cc70208);
        ROUND(60, e, f, g, h, a, b, c, d, 0x90befffa);
        ROUND(61, d, e, f, g, h, a, b, c, 0xa4506ceb);
        ROUND(62, c, d, e, f, g, h, a, b, 0xbef9a3f7);
        ROUND(63, b, c, d, e, f, g, h, a, 0xc67178f2);

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;

        block += SHA256_BLOCK_SIZE;
    }
    while (--count);

    #undef ROUND
    #undef W
    #undef Ch
    #undef Maj
    #undef BSig0
    #undef BSig1
    #undef SSig0
    #undef SSig1
}

void sha256_init(sha256_ctx* ctx)
{
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

void sha256_update(sha256_ctx* ctx, const void* data, size_t size)
{
    const uint8_t* buffer = (const uint8_t*)data;

    size_t pending = ctx->count % SHA256_BLOCK_SIZE;
    ctx->count += size;

    size_t available = SHA256_BLOCK_SIZE - pending;
    if (pending && size >= available)
    {
        memcpy(ctx->buffer + pending, buffer, available);
        sha256_process(ctx->state, ctx->buffer, 1);
        buffer += available;
        size -= available;
        pending = 0;
    }

    size_t count = size / SHA256_BLOCK_SIZE;
    if (count)
    {
        sha256_process(ctx->state, buffer, count);
        buffer += count * SHA256_BLOCK_SIZE;
        size -= count * SHA256_BLOCK_SIZE;
    }

    memcpy(ctx->buffer + pending, buffer, size);
}

void sha256_finish(sha256_ctx* ctx, uint8_t digest[SHA256_DIGEST_SIZE])
{
    uint64_t count = ctx->count;
    uint64_t bitcount = count * 8;

    size_t pending = count % SHA256_BLOCK_SIZE;
    size_t blocks = pending < SHA256_BLOCK_SIZE - sizeof(bitcount) ? 1 : 2;

    ctx->buffer[pending++] = 0x80;

    uint8_t padding[2 * SHA256_BLOCK_SIZE];
    memcpy(padding, ctx->buffer, SHA256_BLOCK_SIZE);
    memset(padding + pending, 0, SHA256_BLOCK_SIZE);
    SHA256_SET64BE(padding + blocks * SHA256_BLOCK_SIZE - sizeof(bitcount), bitcount);

    sha256_process(ctx->state, padding, blocks);

    for (size_t i=0; i<8; i++)
    {
        SHA256_SET32BE(digest + i*sizeof(uint32_t), ctx->state[i]);
    }
}

void sha224_init(sha224_ctx* ctx)
{
    ctx->count = 0;
    ctx->state[0] = 0xc1059ed8;
    ctx->state[1] = 0x367cd507;
    ctx->state[2] = 0x3070dd17;
    ctx->state[3] = 0xf70e5939;
    ctx->state[4] = 0xffc00b31;
    ctx->state[5] = 0x68581511;
    ctx->state[6] = 0x64f98fa7;
    ctx->state[7] = 0xbefa4fa4;
}

void sha224_update(sha224_ctx* ctx, const void* data, size_t size)
{
    sha256_update(ctx, data, size);
}

void sha224_finish(sha224_ctx* ctx, uint8_t digest[SHA224_DIGEST_SIZE])
{
    uint8_t temp[SHA256_DIGEST_SIZE];
    sha256_finish(ctx, temp);

    memcpy(digest, temp, SHA224_DIGEST_SIZE);
}

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(_MSC_VER)
#   pragma warning (pop)
#endif
