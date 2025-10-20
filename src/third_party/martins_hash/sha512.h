#pragma once

// https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
// https://www.rfc-editor.org/rfc/rfc6234

#include <stddef.h>
#include <stdint.h>

//
// interface
//

#define SHA384_DIGEST_SIZE  48
#define SHA512_DIGEST_SIZE  64
#define SHA512_BLOCK_SIZE   128

typedef struct {
    uint8_t buffer[SHA512_BLOCK_SIZE];
    uint64_t count[2];
    uint64_t state[8];
} sha512_ctx;

typedef sha512_ctx sha384_ctx;

static inline void sha512_init(sha512_ctx* ctx);
static inline void sha512_update(sha512_ctx* ctx, const void* data, size_t size);
static inline void sha512_finish(sha512_ctx* ctx, uint8_t digest[SHA512_DIGEST_SIZE]);

static inline void sha384_init(sha384_ctx* ctx);
static inline void sha384_update(sha384_ctx* ctx, const void* data, size_t size);
static inline void sha384_finish(sha384_ctx* ctx, uint8_t digest[SHA384_DIGEST_SIZE]);

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
#   define SHA512_ROR64(x,n) __builtin_rotateright64(x, n)
#elif defined(_MSC_VER)
#   include <stdlib.h>
#   define SHA512_ROR64(x,n) _rotr64(x, n)
#else
#   define SHA512_ROR64(x,n) ( ((x) >> (n)) | ((x) << (64-(n))) )
#endif

#if defined(_MSC_VER)
#   include <stdlib.h>
#   define SHA512_GET64BE(ptr) _byteswap_uint64( *((const __unaligned uint64_t*)(ptr)) )
#   define SHA512_SET64BE(ptr,x) *((__unaligned uint64_t*)(ptr)) = _byteswap_uint64(x)
#else
#   define SHA512_GET64BE(ptr)              \
    (                                       \
        ((uint64_t)((ptr)[0]) << 56) |      \
        ((uint64_t)((ptr)[1]) << 48) |      \
        ((uint64_t)((ptr)[2]) << 40) |      \
        ((uint64_t)((ptr)[3]) << 32) |      \
        ((uint64_t)((ptr)[4]) << 24) |      \
        ((uint64_t)((ptr)[5]) << 16) |      \
        ((uint64_t)((ptr)[6]) <<  8) |      \
        ((uint64_t)((ptr)[7]) <<  0)        \
    )
#   define SHA512_SET64BE(ptr, x) do        \
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

static const uint64_t SHA512_K[80] =
{
    0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
    0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
    0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
    0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
    0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
    0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
    0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
    0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
    0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
    0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
    0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
    0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
    0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
};

#if defined(__x86_64__) || defined(_M_AMD64)

#include <immintrin.h> // AVX2 + SHA512

#if defined(__clang__) || defined(__GNUC__)
#   include <cpuid.h>
#   define SHA512_TARGET(str)          __attribute__((target(str)))
#   define SHA512_CPUID(x, info)       __cpuid(x, info[0], info[1], info[2], info[3])
#   define SHA512_CPUID_EX(x, y, info) __cpuid_count(x, y, info[0], info[1], info[2], info[3])
#   define SHA512_XGETBV(x)            __builtin_ia32_xgetbv(x)
#else
#   include <intrin.h>
#   define SHA512_TARGET(str)
#   define SHA512_CPUID(x, info)       __cpuid(info, x)
#   define SHA512_CPUID_EX(x, y, info) __cpuidex(info, x, y)
#   define SHA512_XGETBV(x)            _xgetbv(x)
#endif

#define SHA512_CPUID_INIT    (1 << 0)
#define SHA512_CPUID_VSHA512 (1 << 1)

SHA512_TARGET("xsave")
static inline int sha512_cpuid(void)
{
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
        int info[4];

        SHA512_CPUID(1, info);
        int has_xsave = info[2] & (1 << 26);

        int has_ymm = 0;
        if (has_xsave)
        {
            uint64_t xcr0 = SHA512_XGETBV(0);
            has_ymm = xcr0 & (1 << 2);
        }

        SHA512_CPUID_EX(7, 0, info);
        int has_avx2 = info[1] & (1 << 5);

        SHA512_CPUID_EX(7, 1, info);
        int has_sha512 = info[0] & (1 << 0);

        result |= SHA512_CPUID_INIT;
        if (has_ymm && has_avx2 && has_sha512)
        {
            result |= SHA512_CPUID_VSHA512;
        }

        cpuid = result;
    }

#if defined(SHA512_CPUID_MASK)
    result &= SHA512_CPUID_MASK;
#endif

    return result;
}

SHA512_TARGET("avx2,sha512")
static void sha512_process_vsha512(uint64_t* state, const uint8_t* block, size_t count)
{
    // pretty much same way how sha256 works, only with avx2 registers and 64-bit additions
    // state is kept as two 256-bit ymm registers (8 qwords)

    // message qwords are loaded as 64-bit big-endian values

    #define W(i) w[(i)%4]

    // 4 wide round calculations
    #define QROUND(i) do {                                                                                                                                                          \
        /* first 4 rounds load input block */                                                                                                                                       \
        if (i < 4) W(i) = _mm256_shuffle_epi8(_mm256_loadu_si256(&buffer[i]), bswap);                                                                                               \
        /* update message schedule */                                                                                                                                               \
        if (i > 2 && i < 19) W(i-3) = _mm256_sha512msg2_epi64(_mm256_add_epi64(W(i-3), _mm256_permute4x64_epi64(_mm256_blend_epi32(W(i-1), W(i), 3), _MM_SHUFFLE(0,3,2,1))), W(i)); \
        if (i > 0 && i < 17) W(i-1) = _mm256_sha512msg1_epi64(W(i-1), _mm256_castsi256_si128(W(i)));                                                                                \
        /* add round constants */                                                                                                                                                   \
        __m256i tmp = _mm256_add_epi64(W(i), _mm256_loadu_si256((const __m256i*)&SHA512_K[4*i]));                                                                                   \
        /* round functions */                                                                                                                                                       \
        state1 = _mm256_sha512rnds2_epi64(state1, state0, _mm256_castsi256_si128(tmp));                                                                                             \
        state0 = _mm256_sha512rnds2_epi64(state0, state1, _mm256_extracti128_si256(tmp, 1));                                                                                        \
    } while(0)

    const __m256i* buffer = (const __m256i*)block;

    // to byteswap when doing big-ending load for message qwords
    const __m256i bswap = _mm256_broadcastsi128_si256(_mm_setr_epi8(7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8));

    // load initial state 
    __m256i abcd = _mm256_permute4x64_epi64(_mm256_loadu_si256((const __m256i*)&state[0]), _MM_SHUFFLE(0,1,2,3)); // [a,b,c,d]
    __m256i efgh = _mm256_permute4x64_epi64(_mm256_loadu_si256((const __m256i*)&state[4]), _MM_SHUFFLE(0,1,2,3)); // [e,f,g,h]

    // qword order for vsha512rnds2 instruction
    __m256i state0 = _mm256_permute2x128_si256(efgh, abcd, (3 << 4) | 1); // [a,b,e,f]
    __m256i state1 = _mm256_permute2x128_si256(efgh, abcd, (2 << 4) | 0); // [c,d,g,h]

    do
    {
        // remember current state
        __m256i last0 = state0;
        __m256i last1 = state1;

        __m256i w[4];

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
        state0 = _mm256_add_epi64(state0, last0);
        state1 = _mm256_add_epi64(state1, last1);

        buffer += 4;
    }
    while (--count);

    // restore qword order
    abcd = _mm256_permute2x128_si256(state1, state0, (3 << 4) | 1);
    efgh = _mm256_permute2x128_si256(state1, state0, (2 << 4) | 0);

    // save the new state
    _mm256_storeu_si256((__m256i*)&state[0], _mm256_permute4x64_epi64(abcd, _MM_SHUFFLE(0,1,2,3)));
    _mm256_storeu_si256((__m256i*)&state[4], _mm256_permute4x64_epi64(efgh, _MM_SHUFFLE(0,1,2,3)));

    #undef QROUND
    #undef W
}

#endif // defined(__x86_64__) || defined(_M_AMD64)

#if defined(__aarch64__) || defined(_M_ARM64)

#if defined(__clang__)
#   define SHA512_TARGET __attribute__((target("sha3")))
#elif defined(__GNUC__)
#   define SHA512_TARGET __attribute__((target("+sha3")))
#elif defined(_MSC_VER)
#   define SHA512_TARGET
#endif

#include <arm_neon.h>

#if defined(_WIN32)
#   include <windows.h>
#   pragma comment (lib, "advapi32")
#elif defined(__linux__)
#   include <sys/auxv.h>
#   include <asm/hwcap.h>
#elif defined(__APPLE__)
#   include <sys/sysctl.h>
#endif

#define SHA512_CPUID_INIT  (1 << 0)
#define SHA512_CPUID_ARM64 (1 << 1)

#if defined(_WIN32)

#endif

static inline int sha512_cpuid(void)
{
#if defined(__ARM_FEATURE_SHA512)
    int result = SHA512_CPUID_ARM64;
#else
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
#if defined(_WIN32)
        // no sha512 bit in IsProcessorFeaturePresent function :(
        uint64_t bits;
        DWORD bitsize = sizeof(bits);
        RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "CP 4030", RRF_RT_QWORD | RRF_ZEROONFAILURE, NULL, &bits, &bitsize);
        // bits from ID_AA64ISAR0_EL1
        int has_arm64 = ((bits >> 15) & 0xf) == 0x2;
#elif defined(__linux__)
        unsigned long hwcap = getauxval(AT_HWCAP);
        int has_arm64 = hwcap & HWCAP_SHA512;
#elif defined(__APPLE__)
        int value = 0;
        size_t valuelen = sizeof(value);
        int has_arm64 = sysctlbyname("hw.optional.arm.FEAT_SHA512", &value, &valuelen, NULL, 0) == 0 && value != 0;
#else
#error unknown platform
#endif
        result |= SHA512_CPUID_INIT;
        if (has_arm64)
        {
            result |= SHA512_CPUID_ARM64;
        }

        cpuid = result;
    }
#endif

#if defined(SHA512_CPUID_MASK)
    result &= SHA512_CPUID_MASK;
#endif

    return result;
}

SHA512_TARGET
static void sha512_process_arm64(uint64_t* state, const uint8_t* block, size_t count)
{
    #define W(i) w[(i)%8]
    #define S(i) vstate.val[3-(i)%4]

    #define DROUND(i) do {                                                                                          \
        /* load 8 round constants */                                                                                \
        if ((i % 4) == 0) rk = vld1q_u64_x4(&SHA512_K[2*i]);                                                        \
        /* first 8 rounds reverse byte order in each 64-bit lane of input block */                                  \
        if (i <  8) W(i) = vreinterpretq_u64_u8(vrev64q_u8(msg[(i/4)%2].val[i%4]));                                 \
        /* update message schedule for next rounds */                                                               \
        if (i >= 8) W(i) = vsha512su1q_u64(vsha512su0q_u64(W(i), W(i-7)), W(i-1), vextq_u64(W(i-4), W(i-3), 1));    \
        /* add round constants */                                                                                   \
        uint64x2_t tmp = vaddq_u64(W(i), rk.val[i%4]);                                                              \
        /* 2 round functions */                                                                                     \
        uint64x2_t x0 = vaddq_u64(vextq_u64(tmp, tmp, 1), S(i+0));                                                  \
        uint64x2_t x1 = vsha512hq_u64(x0, vextq_u64(S(i+1), S(i+0), 1), vextq_u64(S(i+2), S(i+1), 1));              \
        S(i+0) = vsha512h2q_u64(x1, S(i+2), S(i+3));                                                                \
        S(i+2) = vaddq_u64(S(i+2), x1);                                                                             \
    } while (0)

    // load initial state
    uint64x2x4_t vstate = vld1q_u64_x4(state);

    do
    {
        // remember current state
        uint64x2x4_t vlast = vstate;

        // load 128-byte block
        uint8x16x4_t msg[2] =
        {
            vld1q_u8_x4(block + 0 * 16),
            vld1q_u8_x4(block + 4 * 16),
        };

        uint64x2x4_t rk;
        uint64x2_t w[8];

        DROUND( 0);
        DROUND( 1);
        DROUND( 2);
        DROUND( 3);

        DROUND( 4);
        DROUND( 5);
        DROUND( 6);
        DROUND( 7);

        DROUND( 8);
        DROUND( 9);
        DROUND(10);
        DROUND(11);

        DROUND(12);
        DROUND(13);
        DROUND(14);
        DROUND(15);

        DROUND(16);
        DROUND(17);
        DROUND(18);
        DROUND(19);

        DROUND(20);
        DROUND(21);
        DROUND(22);
        DROUND(23);

        DROUND(24);
        DROUND(25);
        DROUND(26);
        DROUND(27);

        DROUND(28);
        DROUND(29);
        DROUND(30);
        DROUND(31);

        DROUND(32);
        DROUND(33);
        DROUND(34);
        DROUND(35);

        DROUND(36);
        DROUND(37);
        DROUND(38);
        DROUND(39);

        // update next state
        vstate.val[0] = vaddq_u64(vstate.val[0], vlast.val[0]);
        vstate.val[1] = vaddq_u64(vstate.val[1], vlast.val[1]);
        vstate.val[2] = vaddq_u64(vstate.val[2], vlast.val[2]);
        vstate.val[3] = vaddq_u64(vstate.val[3], vlast.val[3]);

        block += SHA512_BLOCK_SIZE;
    }
    while (--count);

    // save the new state
    vst1q_u64_x4(state, vstate);

    #undef DROUND
    #undef S
    #undef W
}

#endif // defined(__aarch64__) || defined(_M_ARM64)

static void sha512_process(uint64_t* state, const uint8_t* block, size_t count)
{
#if defined(__x86_64__) || defined(_M_AMD64)
    int cpuid = sha512_cpuid();
    if (cpuid & SHA512_CPUID_VSHA512)
    {
        sha512_process_vsha512(state, block, count);
        return;
    }
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    int cpuid = sha512_cpuid();
    if (cpuid & SHA512_CPUID_ARM64)
    {
        sha512_process_arm64(state, block, count);
        return;
    }
#endif

    #define Ch(x,y,z)  ((x & (y ^ z)) ^ z)
    #define Maj(x,y,z) ((x & y) | (z & (x | y)))

    #define BSig0(x) (SHA512_ROR64(x, 28) ^ SHA512_ROR64(x, 34) ^ SHA512_ROR64(x, 39))
    #define BSig1(x) (SHA512_ROR64(x, 14) ^ SHA512_ROR64(x, 18) ^ SHA512_ROR64(x, 41))
    #define SSig0(x) (SHA512_ROR64(x,  1) ^ SHA512_ROR64(x,  8) ^ (x >> 7))
    #define SSig1(x) (SHA512_ROR64(x, 19) ^ SHA512_ROR64(x, 61) ^ (x >> 6))

    #define W(i) w[(i+16)%16]

    #define ROUND(i,a,b,c,d,e,f,g,h) do                                             \
    {                                                                               \
        uint64_t w0;                                                                \
        if (i <  16) W(i) = w0 = SHA512_GET64BE(block + i*sizeof(uint64_t));        \
        if (i >= 16) W(i) = w0 = SSig1(W(i-2)) + W(i-7) + SSig0(W(i-15)) + W(i-16); \
                                                                                    \
        uint64_t t1 = h + BSig1(e) + Ch(e,f,g) + SHA512_K[i] + w0;                  \
        uint64_t t2 = BSig0(a) + Maj(a,b,c);                                        \
        d += t1;                                                                    \
        h = t1 + t2;                                                                \
    } while (0)

    do
    {
        uint64_t a = state[0];
        uint64_t b = state[1];
        uint64_t c = state[2];
        uint64_t d = state[3];
        uint64_t e = state[4];
        uint64_t f = state[5];
        uint64_t g = state[6];
        uint64_t h = state[7];

        uint64_t w[16];

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
        ROUND(64, a, b, c, d, e, f, g, h);
        ROUND(65, h, a, b, c, d, e, f, g);
        ROUND(66, g, h, a, b, c, d, e, f);
        ROUND(67, f, g, h, a, b, c, d, e);
        ROUND(68, e, f, g, h, a, b, c, d);
        ROUND(69, d, e, f, g, h, a, b, c);
        ROUND(70, c, d, e, f, g, h, a, b);
        ROUND(71, b, c, d, e, f, g, h, a);
        ROUND(72, a, b, c, d, e, f, g, h);
        ROUND(73, h, a, b, c, d, e, f, g);
        ROUND(74, g, h, a, b, c, d, e, f);
        ROUND(75, f, g, h, a, b, c, d, e);
        ROUND(76, e, f, g, h, a, b, c, d);
        ROUND(77, d, e, f, g, h, a, b, c);
        ROUND(78, c, d, e, f, g, h, a, b);
        ROUND(79, b, c, d, e, f, g, h, a);

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;

        block += SHA512_BLOCK_SIZE;
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

void sha512_init(sha512_ctx* ctx)
{
    ctx->count[0] = 0;
    ctx->count[1] = 0;
    ctx->state[0] = 0x6a09e667f3bcc908;
    ctx->state[1] = 0xbb67ae8584caa73b;
    ctx->state[2] = 0x3c6ef372fe94f82b;
    ctx->state[3] = 0xa54ff53a5f1d36f1;
    ctx->state[4] = 0x510e527fade682d1;
    ctx->state[5] = 0x9b05688c2b3e6c1f;
    ctx->state[6] = 0x1f83d9abfb41bd6b;
    ctx->state[7] = 0x5be0cd19137e2179;
}

void sha512_update(sha512_ctx* ctx, const void* data, size_t size)
{
    const uint8_t* buffer = (const uint8_t*)data;

    size_t pending = ctx->count[0] % SHA512_BLOCK_SIZE;
    ctx->count[0] += size;
    ctx->count[1] += size > ctx->count[0];

    size_t available = SHA512_BLOCK_SIZE - pending;
    if (pending && size >= available)
    {
        memcpy(ctx->buffer + pending, buffer, available);
        sha512_process(ctx->state, ctx->buffer, 1);
        buffer += available;
        size -= available;
        pending = 0;
    }

    size_t count = size / SHA512_BLOCK_SIZE;
    if (count)
    {
        sha512_process(ctx->state, buffer, count);
        buffer += count * SHA512_BLOCK_SIZE;
        size -= count * SHA512_BLOCK_SIZE;
    }

    memcpy(ctx->buffer + pending, buffer, size);
}

void sha512_finish(sha512_ctx* ctx, uint8_t digest[SHA512_DIGEST_SIZE])
{
    uint64_t count0 = ctx->count[0];
    uint64_t count1 = ctx->count[1];
    uint64_t bitcount[2] = { (count0 << 3), (count1 << 3) | (count0 >> 61) };

    size_t pending = count0 % SHA512_BLOCK_SIZE;
    size_t blocks = pending < SHA512_BLOCK_SIZE - sizeof(bitcount) ? 1 : 2;

    ctx->buffer[pending++] = 0x80;

    uint8_t padding[2 * SHA512_BLOCK_SIZE];
    memcpy(padding, ctx->buffer, SHA512_BLOCK_SIZE);
    memset(padding + pending, 0, SHA512_BLOCK_SIZE);
    SHA512_SET64BE(padding + blocks * SHA512_BLOCK_SIZE - 2*sizeof(uint64_t), bitcount[1]);
    SHA512_SET64BE(padding + blocks * SHA512_BLOCK_SIZE - 1*sizeof(uint64_t), bitcount[0]);

    sha512_process(ctx->state, padding, blocks);

    for (size_t i=0; i<8; i++)
    {
        SHA512_SET64BE(digest + i*sizeof(uint64_t), ctx->state[i]);
    }
}

void sha384_init(sha384_ctx* ctx)
{
    ctx->count[0] = 0;
    ctx->count[1] = 0;
    ctx->state[0] = 0xcbbb9d5dc1059ed8;
    ctx->state[1] = 0x629a292a367cd507;
    ctx->state[2] = 0x9159015a3070dd17;
    ctx->state[3] = 0x152fecd8f70e5939;
    ctx->state[4] = 0x67332667ffc00b31;
    ctx->state[5] = 0x8eb44a8768581511;
    ctx->state[6] = 0xdb0c2e0d64f98fa7;
    ctx->state[7] = 0x47b5481dbefa4fa4;
}

void sha384_update(sha512_ctx* ctx, const void* data, size_t size)
{
    sha512_update(ctx, data, size);
}

void sha384_finish(sha384_ctx* ctx, uint8_t digest[SHA384_DIGEST_SIZE])
{
    uint8_t temp[SHA512_DIGEST_SIZE];
    sha512_finish(ctx, temp);

    memcpy(digest, temp, SHA384_DIGEST_SIZE);
}

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(_MSC_VER)
#   pragma warning (pop)
#endif
