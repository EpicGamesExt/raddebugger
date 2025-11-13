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
#   define SHA1_GET32BE(ptr) _byteswap_ulong( *((const __unaligned uint32_t*)(ptr)) )
#   define SHA1_SET32BE(ptr,x) *((__unaligned uint32_t*)(ptr)) = _byteswap_ulong(x)
#   define SHA1_SET64BE(ptr,x) *((__unaligned uint64_t*)(ptr)) = _byteswap_uint64(x)
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
    // in sha1 each round has two parts:
    // 1) calculate message schedule dwords in w[i]
    // 2) do round functions to update a/b/c/d/e state values using w[i]

    // initial message schedule is loaded as 32-bit big-endian 16 dwords

    // for next rounds it is done as:
    // w[i] = ROL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16])
    // where ROL(x) = 32-bit rotate left by 1

    // this means it is possible to keep just the last 16 of w's
    // and every new w calculated will need to update 1 to 3 previous w's

    // unrolling round calculations by 4 we get:
    // w[i+0] = ROL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16])
    // w[i+1] = ROL(w[i-2] ^ w[i-7] ^ w[i-13] ^ w[i-15])
    // w[i+2] = ROL(w[i-1] ^ w[i-6] ^ w[i-12] ^ w[i-14])
    // w[i+3] = ROL(w[i+0] ^ w[i-5] ^ w[i-11] ^ w[i-13])

    // now use 4 w[..] values in 128-bit SSE register W(i), then
    // W(i) = ROL( r0 ^ r1 ^ r2 ^ r3 )
    // with caveat that r0 lane 3 depends on W(i) lane 0

    //         [3]      [2]      [1]      [0]      // lanes
    // r0 = [ special, w[i-1],  w[i-2],  w[i-3]  ]
    // r1 = [ w[i-5],  w[i-6],  w[i-7],  w[i-8]  ]
    // r2 = [ w[i-11], w[i-12], w[i-13], w[i-14] ]
    // r3 = [ w[i-13], w[i-14], w[i-15], w[i-16] ]

    // in each 4-round i'th step it is possible to incrementally update W's that will be
    // used in later rounds

    // rounds i>0: m3 = r2 ^ r3      = _mm_sha1msg1_epu32(m3, m0)
    // rounds i>1: m2 = m2 ^ r1      = _mm_xor_si128     (m2, m0)
    // rounds i>2: m1 = ROL(m1 ^ r0) = _mm_sha1msg2_epu32(m1, m0)
    // then the new m0 can be used in round function calculations
    // _mm_sha1msg2_epu32 correctly handles r0 lane 3 dependency on lane 0 output

    // to perform round functions on two SIMD registers with state as:
    // s0 = [a,b,c,d]
    // s1 = [e,0,0,0]
    // use _mm_sha1rnds4_epu32 and _mm_sha1nexte_epu32 ops and swap both state variables between rounds

    // 4 wide round calculations
    #define QROUND(i,s0,s1,m0,m1,m2,m3) do {                  \
        /* update message schedule */                         \
        if (i > 0 && i < 17) m3 = _mm_sha1msg1_epu32(m3, m0); \
        if (i > 1 && i < 18) m2 = _mm_xor_si128     (m2, m0); \
        if (i > 2 && i < 19) m1 = _mm_sha1msg2_epu32(m1, m0); \
        /* calculate E plus message schedule */               \
        if (i == 0) tmp = _mm_add_epi32      (s1, m0);        \
        if (i != 0) tmp = _mm_sha1nexte_epu32(s1, m0);        \
        /* 4 round functions */                               \
        s1 = _mm_sha1rnds4_epu32(s0, tmp, i/5);               \
    } while(0)

    const __m128i* buffer = (const __m128i*)block;

    // for performing two operations in one:
    // 1) dwords need to be loaded as big-endian
    // 2) order of dwords need to be reversed for sha1 instructions: [0,1,2,3] -> [3,2,1,0]
    const __m128i bswap = _mm_setr_epi8(15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0);

    // load initial state
    __m128i s0 = _mm_loadu_si128((const __m128i*)state); // [d,c,b,a]
    __m128i s1 = _mm_loadu_si32(&state[4]);              // [0,0,0,e]

    // flip dword order, to what sha1 instructions use
    s0 = _mm_shuffle_epi32(s0, _MM_SHUFFLE(0,1,2,3)); // [a,b,c,d]
    s1 = _mm_shuffle_epi32(s1, _MM_SHUFFLE(0,1,2,3)); // [e,0,0,0]

    do
    {
        // remember current state
        __m128i last0 = s0;
        __m128i last1 = s1;

        // load initial message schedule, 64-byte block
        __m128i w0 = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[0]), bswap);
        __m128i w1 = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[1]), bswap);
        __m128i w2 = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[2]), bswap);
        __m128i w3 = _mm_shuffle_epi8(_mm_loadu_si128(&buffer[3]), bswap);
        buffer += 4;

        __m128i tmp;

        QROUND( 0, s0, s1, w0, w1, w2, w3);
        QROUND( 1, s1, s0, w1, w2, w3, w0);
        QROUND( 2, s0, s1, w2, w3, w0, w1);
        QROUND( 3, s1, s0, w3, w0, w1, w2);
        QROUND( 4, s0, s1, w0, w1, w2, w3);

        QROUND( 5, s1, s0, w1, w2, w3, w0);
        QROUND( 6, s0, s1, w2, w3, w0, w1);
        QROUND( 7, s1, s0, w3, w0, w1, w2);
        QROUND( 8, s0, s1, w0, w1, w2, w3);
        QROUND( 9, s1, s0, w1, w2, w3, w0);

        QROUND(10, s0, s1, w2, w3, w0, w1);
        QROUND(11, s1, s0, w3, w0, w1, w2);
        QROUND(12, s0, s1, w0, w1, w2, w3);
        QROUND(13, s1, s0, w1, w2, w3, w0);
        QROUND(14, s0, s1, w2, w3, w0, w1);

        QROUND(15, s1, s0, w3, w0, w1, w2);
        QROUND(16, s0, s1, w0, w1, w2, w3);
        QROUND(17, s1, s0, w1, w2, w3, w0);
        QROUND(18, s0, s1, w2, w3, w0, w1);
        QROUND(19, s1, s0, w3, w0, w1, w2);

        // update next state
        s0 = _mm_add_epi32      (s0, last0);
        s1 = _mm_sha1nexte_epu32(s1, last1);
    }
    while (--count);

    // restore dword order
    s0 = _mm_shuffle_epi32(s0, _MM_SHUFFLE(0,1,2,3));
    s1 = _mm_shuffle_epi32(s1, _MM_SHUFFLE(0,1,2,3));

    // save the new state
    _mm_storeu_si128((__m128i*)state, s0);
    _mm_storeu_si32(&state[4], s1);

    #undef QROUND
}

#endif // defined(__x86_64__) || defined(_M_AMD64)


#if defined(__aarch64__) || defined(_M_ARM64)

#if defined(__clang__)
#   define SHA1_TARGET __attribute__((target("sha2")))
#elif defined(__GNUC__)
#   define SHA1_TARGET __attribute__((target("+sha2")))
#elif defined(_MSC_VER)
#   define SHA1_TARGET
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

#define SHA1_CPUID_INIT  (1 << 0)
#define SHA1_CPUID_ARM64 (1 << 1)

static inline int sha1_cpuid(void)
{
#if defined(__ARM_FEATURE_CRYPTO) || defined(__ARM_FEATURE_SHA2)
    int result = SHA1_CPUID_ARM64;
#else
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
#if defined(_WIN32)
        int has_arm64 = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
#elif defined(__linux__)
        unsigned long hwcap = getauxval(AT_HWCAP);
        int has_arm64 = hwcap & HWCAP_SHA1;
#elif defined(__APPLE__)
        int value = 0;
        size_t valuelen = sizeof(value);
        int has_arm64 = sysctlbyname("hw.optional.arm.FEAT_SHA1", &value, &valuelen, NULL, 0) == 0 && value != 0;
#else
#error unknown platform
#endif
        result |= SHA1_CPUID_INIT;
        if (has_arm64)
        {
            result |= SHA1_CPUID_ARM64;
        }

        cpuid = result;
    }
#endif

#if defined(SHA1_CPUID_MASK)
    result &= SHA1_CPUID_MASK;
#endif

    return result;
}

SHA1_TARGET
static void sha1_process_arm64(uint32_t* state, const uint8_t* block, size_t count)
{
    // code here is similar to x64 shani implementation

    #define QROUND(i,m0,m1,m2,m3,k,F) do {          \
        /* update message schedule */               \
        if (i >= 4) m0 = vsha1su0q_u32(m0, m1, m2); \
        if (i >= 4) m0 = vsha1su1q_u32(m0, m3);     \
        /* add round constant */                    \
        uint32x4_t tmp = vaddq_u32(m0, k);          \
        /* 4 round functions */                     \
        uint32_t e = vgetq_lane_u32(s0, 0);         \
        s0 = F(s0, s1, tmp);                        \
        s1 = vsha1h_u32(e);                         \
    } while (0)

    const uint32x4_t k0 = vdupq_n_u32(0x5a827999);
    const uint32x4_t k1 = vdupq_n_u32(0x6ed9eba1);
    const uint32x4_t k2 = vdupq_n_u32(0x8f1bbcdc);
    const uint32x4_t k3 = vdupq_n_u32(0xca62c1d6);

    // load initial state
    uint32x4_t s0 = vld1q_u32(state); // [d,c,b,a]
    uint32_t   s1 = state[4];         // e

    do
    {
        // remember current state
        uint32x4_t last0 = s0;
        uint32_t   last1 = s1;

        // load initial message schedule, 64-byte block
        uint8x16x4_t msg = vld1q_u8_x4(block);
        block += SHA1_BLOCK_SIZE;

        // reverse the byte order in each 32-bit lane
        uint32x4_t w0 = vreinterpretq_u32_u8(vrev32q_u8(msg.val[0]));
        uint32x4_t w1 = vreinterpretq_u32_u8(vrev32q_u8(msg.val[1]));
        uint32x4_t w2 = vreinterpretq_u32_u8(vrev32q_u8(msg.val[2]));
        uint32x4_t w3 = vreinterpretq_u32_u8(vrev32q_u8(msg.val[3]));

        QROUND( 0, w0, w1, w2, w3, k0, vsha1cq_u32);
        QROUND( 1, w1, w2, w3, w0, k0, vsha1cq_u32);
        QROUND( 2, w2, w3, w0, w1, k0, vsha1cq_u32);
        QROUND( 3, w3, w0, w1, w2, k0, vsha1cq_u32);
        QROUND( 4, w0, w1, w2, w3, k0, vsha1cq_u32);

        QROUND( 5, w1, w2, w3, w0, k1, vsha1pq_u32);
        QROUND( 6, w2, w3, w0, w1, k1, vsha1pq_u32);
        QROUND( 7, w3, w0, w1, w2, k1, vsha1pq_u32);
        QROUND( 8, w0, w1, w2, w3, k1, vsha1pq_u32);
        QROUND( 9, w1, w2, w3, w0, k1, vsha1pq_u32);

        QROUND(10, w2, w3, w0, w1, k2, vsha1mq_u32);
        QROUND(11, w3, w0, w1, w2, k2, vsha1mq_u32);
        QROUND(12, w0, w1, w2, w3, k2, vsha1mq_u32);
        QROUND(13, w1, w2, w3, w0, k2, vsha1mq_u32);
        QROUND(14, w2, w3, w0, w1, k2, vsha1mq_u32);

        QROUND(15, w3, w0, w1, w2, k3, vsha1pq_u32);
        QROUND(16, w0, w1, w2, w3, k3, vsha1pq_u32);
        QROUND(17, w1, w2, w3, w0, k3, vsha1pq_u32);
        QROUND(18, w2, w3, w0, w1, k3, vsha1pq_u32);
        QROUND(19, w3, w0, w1, w2, k3, vsha1pq_u32);

        // update next state
        s0 = vaddq_u32(s0, last0);
        s1 += last1;
    }
    while (--count);

    // save state
    vst1q_u32(state, s0);
    state[4] = s1;

    #undef QROUND
}

#endif // defined(__aarch64__) || defined(_M_ARM64)

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

#if defined(__aarch64__) || defined(_M_ARM64)
    int cpuid = sha1_cpuid();
    if (cpuid & SHA1_CPUID_ARM64)
    {
        sha1_process_arm64(state, block, count);
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
