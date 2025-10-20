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
#   define SHA256_GET32BE(ptr) _byteswap_ulong( *((const __unaligned uint32_t*)(ptr)) )
#   define SHA256_SET32BE(ptr,x) *((__unaligned uint32_t*)(ptr)) = _byteswap_ulong(x)
#   define SHA256_SET64BE(ptr,x) *((__unaligned uint64_t*)(ptr)) = _byteswap_uint64(x)
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

static const uint32_t SHA256_K[64] =
{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

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
    // similar way how sha1 works in with shani

    // first 16 rounds loads message schedule dwords as 32-bit big endian values

    // for next rounds message schedule is prepared as:
    // w[i] = SSig1(w[i-2]) + w[i-7] + SSig0(w[i-15]) + w[i-16]

    // unrolled by 4:
    // w[i+0] = SSig1(w[i-2]) + w[i-7] + SSig0(w[i-15]) + w[i-16]
    // w[i+1] = SSig1(w[i-1]) + w[i-6] + SSig0(w[i-14]) + w[i-15]
    // w[i+2] = SSig1(w[i+0]) + w[i-5] + SSig0(w[i-13]) + w[i-14]
    // w[i+3] = SSig1(w[i+1]) + w[i-4] + SSig0(w[i-12]) + w[i-13]

    // there is tricky dependency for lanes 2 and 3 on result of lanes 0 and 1, but sha256msg2 op takes care of that

    // by storing W[i] word in 128-bit simd register, the message schedule becomes:
    // W(i) = SSig1(r0) + r1 + SSig0(r2) + r3
    // where + is 32-bit lane addition

    //         [3]      [2]      [1]      [0]      // lanes
    // r0 = [ special, special, w[i-1],  w[i-2]  ]
    // r1 = [ w[i-4],  w[i-5],  w[i-6],  w[i-7]  ]
    // r2 = [ w[i-12], w[i-13], w[i-14], w[i-15] ]
    // r3 = [ w[i-13], w[i-14], w[i-15], w[i-16] ]

    // rN's can be calculated from previous W(..) values:
    // r0 from W(i)
    // r1 from _mm_alignr_epi8(W(i), W(i-1), 4)
    // r2 from W(i-1) and W(i)
    // r3 from W(i-1)

    // rounds i>2: W(i-3) = _mm_sha256msg2_epu32(_mm_add_epi32( W(i-3), _mm_alignr_epi8(W(i), W(i-1), 4) ), W(i))
    // rounds i>0: W(i-1) = _mm_sha256msg1_epu32(W(i-1), W(i))

    // round functions are done with _mm_sha256rnds2_epu32 which performs it for 2 rounds
    // thus repeat it two times, as input use W(i) + K(i) - message schedule added with sha256 constants

    #define W(i) w[(i)%4]

    // 4 wide round calculations
    #define QROUND(i) do {                                                                                                  \
        /* first 4 rounds load input block */                                                                               \
        if (i < 4) W(i) = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[i]), bswap);                                             \
        /* update message schedule */                                                                                       \
        if (i > 2 && i < 15) W(i-3) = _mm_sha256msg2_epu32(_mm_add_epi32(W(i-3), _mm_alignr_epi8(W(i), W(i-1), 4)), W(i));  \
        if (i > 0 && i < 13) W(i-1) = _mm_sha256msg1_epu32(W(i-1), W(i));                                                   \
        /* add round constants */                                                                                           \
        __m128i tmp = _mm_add_epi32(W(i), _mm_loadu_si128((const __m128i*)&SHA256_K[4*i]));                                 \
        /* 4 round functions */                                                                                             \
        state1 = _mm_sha256rnds2_epu32(state1, state0, tmp);                                                                \
        state0 = _mm_sha256rnds2_epu32(state0, state1, _mm_shuffle_epi32(tmp, _MM_SHUFFLE(0,0,3,2)));                       \
    } while(0)

    const __m128i* buffer = (const __m128i*)block;

    // to byteswap when doing big-ending load for message dwords
    const __m128i bswap = _mm_setr_epi8(3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12);
       
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

        __m128i w[4];

        QROUND( 0);
        QROUND( 1);
        QROUND( 2);
        QROUND( 3);
        QROUND( 4);
        QROUND( 5);
        QROUND( 6);
        QROUND( 7);
        QROUND( 8);
        QROUND( 9);
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

#if defined(__aarch64__) || defined(_M_ARM64)

#if defined(__clang__)
#   define SHA256_TARGET __attribute__((target("sha2")))
#elif defined(__GNUC__)
#   define SHA256_TARGET __attribute__((target("+sha2")))
#elif defined(_MSC_VER)
#   define SHA256_TARGET
#endif

#include <arm_neon.h>

#if defined(_WIN32)
#   include <windows.h>
#elif defined(__linux__)
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
#elif defined(__APPLE__)
#   include <sys/sysctl.h>
#endif

#define SHA256_CPUID_INIT  (1 << 0)
#define SHA256_CPUID_ARM64 (1 << 1)

static inline int sha256_cpuid(void)
{
#if defined(__ARM_FEATURE_CRYPTO) || defined(__ARM_FEATURE_SHA2)
    int result = SHA256_CPUID_ARM64;
#else
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
#if defined(_WIN32)
        int has_arm64 = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
#elif defined(__linux__)
        unsigned long hwcap = getauxval(AT_HWCAP);
        int has_arm64 = hwcap & HWCAP_SHA2;
#elif defined(__APPLE__)
        int value = 0;
        size_t valuelen = sizeof(value);
        int has_arm64 = sysctlbyname("hw.optional.arm.FEAT_SHA256", &value, &valuelen, NULL, 0) == 0 && value != 0;
#else
#error unknown platform
#endif
        result |= SHA256_CPUID_INIT;
        if (has_arm64)
        {
            result |= SHA256_CPUID_ARM64;
        }

        cpuid = result;
    }
#endif

#if defined(SHA256_CPUID_MASK)
    result &= SHA256_CPUID_MASK;
#endif

    return result;
}

SHA256_TARGET
static void sha256_process_arm64(uint32_t* state, const uint8_t* block, size_t count)
{
    // code here is similar to x64 shani implementation

    #define W(i) w[(i)%4]

    #define QROUND(i) do {                                                          \
        /* load 16 round constants */                                               \
        if ((i % 4) == 0) rk = vld1q_u32_x4(&SHA256_K[4*i]);                        \
        /* first 4 rounds reverse byte order in each 32-bit lane of input block */  \
        if (i <  4) W(i) = vreinterpretq_u32_u8(vrev32q_u8(msg.val[i]));            \
        /* update message schedule */                                               \
        if (i >= 4) W(i) = vsha256su0q_u32(W(i), W(i-3));                           \
        if (i >= 4) W(i) = vsha256su1q_u32(W(i), W(i-2), W(i-1));                   \
        /* add round constants */                                                   \
        uint32x4_t tmp = vaddq_u32(W(i), rk.val[i%4]);                              \
        /* 4 round functions */                                                     \
        uint32x4_t x = vstate.val[0];                                               \
        vstate.val[0] = vsha256hq_u32(vstate.val[0], vstate.val[1], tmp);           \
        vstate.val[1] = vsha256h2q_u32(vstate.val[1], x, tmp);                      \
    } while (0)

    // load initial state
    uint32x4x2_t vstate = vld1q_u32_x2(state);

    do
    {
        // remember current state
        uint32x4x2_t vlast = vstate;

        // load 64-byte block
        uint8x16x4_t msg = vld1q_u8_x4(block);

        uint32x4x4_t rk;
        uint32x4_t w[4];

        QROUND( 0);
        QROUND( 1);
        QROUND( 2);
        QROUND( 3);
        QROUND( 4);
        QROUND( 5);
        QROUND( 6);
        QROUND( 7);
        QROUND( 8);
        QROUND( 9);
        QROUND(10);
        QROUND(11);
        QROUND(12);
        QROUND(13);
        QROUND(14);
        QROUND(15);

        // update next state
        vstate.val[0] = vaddq_u32(vstate.val[0], vlast.val[0]);
        vstate.val[1] = vaddq_u32(vstate.val[1], vlast.val[1]);

        block += SHA256_BLOCK_SIZE;
    }
    while (--count);

    // save the new state
    vst1q_u32_x2(state, vstate);

    #undef QROUND
    #undef W
}

#endif // defined(__aarch64__) || defined(_M_ARM64)

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

#if defined(__aarch64__) || defined(_M_ARM64)
    int cpuid = sha256_cpuid();
    if (cpuid & SHA256_CPUID_ARM64)
    {
        sha256_process_arm64(state, block, count);
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

    #define ROUND(i,a,b,c,d,e,f,g,h) do                                             \
    {                                                                               \
        uint32_t w0;                                                                \
        if (i <  16) W(i) = w0 = SHA256_GET32BE(block + i*sizeof(uint32_t));        \
        if (i >= 16) W(i) = w0 = SSig1(W(i-2)) + W(i-7) + SSig0(W(i-15)) + W(i-16); \
                                                                                    \
        uint32_t t1 = h + BSig1(e) + Ch(e,f,g) + SHA256_K[i] + w0;                  \
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

        ROUND( 0, a, b, c, d, e, f, g, h);
        ROUND( 1, h, a, b, c, d, e, f, g);
        ROUND( 2, g, h, a, b, c, d, e, f);
        ROUND( 3, f, g, h, a, b, c, d, e);
        ROUND( 4, e, f, g, h, a, b, c, d);
        ROUND( 5, d, e, f, g, h, a, b, c);
        ROUND( 6, c, d, e, f, g, h, a, b);
        ROUND( 7, b, c, d, e, f, g, h, a);
        ROUND( 8, a, b, c, d, e, f, g, h);
        ROUND( 9, h, a, b, c, d, e, f, g);
        ROUND(10, g, h, a, b, c, d, e, f);
        ROUND(11, f, g, h, a, b, c, d, e);
        ROUND(12, e, f, g, h, a, b, c, d);
        ROUND(13, d, e, f, g, h, a, b, c);
        ROUND(14, c, d, e, f, g, h, a, b);
        ROUND(15, b, c, d, e, f, g, h, a);
        ROUND(16, a, b, c, d, e, f, g, h);
        ROUND(17, h, a, b, c, d, e, f, g);
        ROUND(18, g, h, a, b, c, d, e, f);
        ROUND(19, f, g, h, a, b, c, d, e);
        ROUND(20, e, f, g, h, a, b, c, d);
        ROUND(21, d, e, f, g, h, a, b, c);
        ROUND(22, c, d, e, f, g, h, a, b);
        ROUND(23, b, c, d, e, f, g, h, a);
        ROUND(24, a, b, c, d, e, f, g, h);
        ROUND(25, h, a, b, c, d, e, f, g);
        ROUND(26, g, h, a, b, c, d, e, f);
        ROUND(27, f, g, h, a, b, c, d, e);
        ROUND(28, e, f, g, h, a, b, c, d);
        ROUND(29, d, e, f, g, h, a, b, c);
        ROUND(30, c, d, e, f, g, h, a, b);
        ROUND(31, b, c, d, e, f, g, h, a);
        ROUND(32, a, b, c, d, e, f, g, h);
        ROUND(33, h, a, b, c, d, e, f, g);
        ROUND(34, g, h, a, b, c, d, e, f);
        ROUND(35, f, g, h, a, b, c, d, e);
        ROUND(36, e, f, g, h, a, b, c, d);
        ROUND(37, d, e, f, g, h, a, b, c);
        ROUND(38, c, d, e, f, g, h, a, b);
        ROUND(39, b, c, d, e, f, g, h, a);
        ROUND(40, a, b, c, d, e, f, g, h);
        ROUND(41, h, a, b, c, d, e, f, g);
        ROUND(42, g, h, a, b, c, d, e, f);
        ROUND(43, f, g, h, a, b, c, d, e);
        ROUND(44, e, f, g, h, a, b, c, d);
        ROUND(45, d, e, f, g, h, a, b, c);
        ROUND(46, c, d, e, f, g, h, a, b);
        ROUND(47, b, c, d, e, f, g, h, a);
        ROUND(48, a, b, c, d, e, f, g, h);
        ROUND(49, h, a, b, c, d, e, f, g);
        ROUND(50, g, h, a, b, c, d, e, f);
        ROUND(51, f, g, h, a, b, c, d, e);
        ROUND(52, e, f, g, h, a, b, c, d);
        ROUND(53, d, e, f, g, h, a, b, c);
        ROUND(54, c, d, e, f, g, h, a, b);
        ROUND(55, b, c, d, e, f, g, h, a);
        ROUND(56, a, b, c, d, e, f, g, h);
        ROUND(57, h, a, b, c, d, e, f, g);
        ROUND(58, g, h, a, b, c, d, e, f);
        ROUND(59, f, g, h, a, b, c, d, e);
        ROUND(60, e, f, g, h, a, b, c, d);
        ROUND(61, d, e, f, g, h, a, b, c);
        ROUND(62, c, d, e, f, g, h, a, b);
        ROUND(63, b, c, d, e, f, g, h, a);

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
