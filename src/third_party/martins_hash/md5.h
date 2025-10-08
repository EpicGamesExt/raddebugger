#pragma once

// https://www.rfc-editor.org/rfc/rfc1321.html

#include <stddef.h>
#include <stdint.h>

//
// interface
//

#define MD5_DIGEST_SIZE     16
#define MD5_BLOCK_SIZE      64

typedef struct {
    uint8_t buffer[MD5_BLOCK_SIZE];
    uint64_t count;
    uint32_t state[4];
} md5_ctx;

static inline void md5_init(md5_ctx* ctx);
static inline void md5_update(md5_ctx* ctx, const void* data, size_t size);
static inline void md5_finish(md5_ctx* ctx, uint8_t digest[MD5_DIGEST_SIZE]);

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
#endif

#if defined(__clang__)
#   define MD5_ROL32(x,n) __builtin_rotateleft32(x, n)
#elif defined(_MSC_VER)
#   include <stdlib.h>
#   define MD5_ROL32(x,n) _rotl(x, n)
#else
#   define MD5_ROL32(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#endif

#if defined(_MSC_VER)
#   define MD5_GET32LE(ptr) *((const _UNALIGNED uint32_t*)(ptr))
#   define MD5_SET32LE(ptr,x) *((_UNALIGNED uint32_t*)(ptr)) = (x)
#   define MD5_SET64LE(ptr,x) *((_UNALIGNED uint64_t*)(ptr)) = (x)
#else
#   define MD5_GET32LE(ptr) \
    (                       \
        ((ptr)[0] <<  0) |  \
        ((ptr)[1] <<  8) |  \
        ((ptr)[2] << 16) |  \
        ((ptr)[3] << 24)    \
    )
#   define MD5_SET32LE(ptr, x) do           \
    {                                       \
        (ptr)[0] = (uint8_t)((x) >>  0);    \
        (ptr)[1] = (uint8_t)((x) >>  8);    \
        (ptr)[2] = (uint8_t)((x) >> 16);    \
        (ptr)[3] = (uint8_t)((x) >> 24);    \
    }                                       \
    while (0)
#   define MD5_SET64LE(ptr, x) do           \
    {                                       \
        (ptr)[0] = (uint8_t)((x) >>  0);    \
        (ptr)[1] = (uint8_t)((x) >>  8);    \
        (ptr)[2] = (uint8_t)((x) >> 16);    \
        (ptr)[3] = (uint8_t)((x) >> 24);    \
        (ptr)[4] = (uint8_t)((x) >> 32);    \
        (ptr)[5] = (uint8_t)((x) >> 40);    \
        (ptr)[6] = (uint8_t)((x) >> 48);    \
        (ptr)[7] = (uint8_t)((x) >> 56);    \
    }                                       \
    while (0)
#endif

// MD5_COMPILER_BARRIER forces clang to do better codegen without spilling registers to stack too much
#if defined(__clang__) || defined(__GNUC__)
#   define MD5_COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")
#else
#   define MD5_COMPILER_BARRIER()
#endif


#if defined(__x86_64__) || defined(_M_AMD64)

#if defined(__clang__) || defined(__GNUC__)
#   include <cpuid.h>
#   define MD5_TARGET(str)          __attribute__((target(str)))
#   define MD5_CPUID_EX(x, y, info) __cpuid_count(x, y, info[0], info[1], info[2], info[3])
#   define MD5_ANDN_U32(x,y)        (~(x) & (y))
#else
#   include <intrin.h>
#   define MD5_TARGET(str)
#   define MD5_CPUID_EX(x, y, info) __cpuidex(info, x, y)
#   define MD5_ANDN_U32(x,y)        _andn_u32(x,y)
#endif

#if defined(__clang__)
#   define MD5_RORX_U32(x,n) __builtin_rotateright32(x, n)
#elif defined(_MSC_VER)
#   define MD5_RORX_U32(x,n) _rorx_u32(x,n)
#else
#   define MD5_RORX_U32(x,n) ( ((x) >> (n)) | ((x) << (32-(n))) )
#endif

#define MD5_CPUID_INIT  (1 << 0)
#define MD5_CPUID_BMI2  (1 << 1)

static inline int md5_cpuid(void)
{
    static int cpuid;

    int result = cpuid;
    if (result == 0)
    {
        int info[4];

        MD5_CPUID_EX(7, 0, info);
        int has_bmi = info[1] & (1 << 3);
        int has_bmi2 = info[1] & (1 << 8);

        result |= MD5_CPUID_INIT;
        if (has_bmi && has_bmi2)
        {
            result |= MD5_CPUID_BMI2;
        }

        cpuid = result;
    }

#if defined(MD5_CPUID_MASK)
    result &= MD5_CPUID_MASK;
#endif

    return result;
}

MD5_TARGET("bmi,bmi2,tune=znver1")
static void md5_process_bmi2(uint32_t* state, const uint8_t* block, size_t count)
{
    // "tune=znver1" allows clang to use LEA with [reg+reg+imm] operand which helps performance on modern CPU's
    // -1 in I will get folded together with constant k

    #define F(x,y,z) (x & y) + MD5_ANDN_U32(x, z)
    #define G(x,y,z) (x & z) + MD5_ANDN_U32(z, y)
    #define H(x,y,z) (x ^ y ^ z)
    #define I(x,y,z) 0 - 1 - (y ^ MD5_ANDN_U32(x, z))

    #define X(i) MD5_GET32LE(block + i*sizeof(uint32_t))

    #define ROUND(F, a, b, c, d, x, k, r) do {  \
        a += (k) + F(b, c, d) + (x);            \
        a = MD5_RORX_U32(a, 32-r) + b;          \
    } while (0)

    #define QROUND_F(x0, x1, x2, x3, k0, k1, k2, k3) do {    \
        ROUND(F, a, b, c, d, X(x0), k0,  7);                 \
        ROUND(F, d, a, b, c, X(x1), k1, 12);                 \
        ROUND(F, c, d, a, b, X(x2), k2, 17);                 \
        ROUND(F, b, c, d, a, X(x3), k3, 22);                 \
    } while (0)

    #define QROUND_G(x0, x1, x2, x3, k0, k1, k2, k3) do {   \
        ROUND(G, a, b, c, d, X(x0), k0,  5);                \
        ROUND(G, d, a, b, c, X(x1), k1,  9);                \
        ROUND(G, c, d, a, b, X(x2), k2, 14);                \
        ROUND(G, b, c, d, a, X(x3), k3, 20);                \
    } while (0)

    #define QROUND_H(x0, x1, x2, x3, k0, k1, k2, k3) do {   \
        ROUND(H, a, b, c, d, X(x0), k0,  4);                \
        ROUND(H, d, a, b, c, X(x1), k1, 11);                \
        ROUND(H, c, d, a, b, X(x2), k2, 16);                \
        ROUND(H, b, c, d, a, X(x3), k3, 23);                \
    } while (0)

    #define QROUND_I(x0, x1, x2, x3, k0, k1, k2, k3) do {   \
        ROUND(I, a, b, c, d, X(x0), k0,  6);                \
        ROUND(I, d, a, b, c, X(x1), k1, 10);                \
        ROUND(I, c, d, a, b, X(x2), k2, 15);                \
        ROUND(I, b, c, d, a, X(x3), k3, 21);                \
    } while (0)

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];

    do
    {
        uint32_t last_a = a;
        uint32_t last_b = b;
        uint32_t last_c = c;
        uint32_t last_d = d;

        QROUND_F( 0,  1,  2,  3, 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee);
        QROUND_F( 4,  5,  6,  7, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501);
        QROUND_F( 8,  9, 10, 11, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be);
        QROUND_F(12, 13, 14, 15, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821);
        MD5_COMPILER_BARRIER();

        QROUND_G( 1,  6, 11,  0, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa);
        QROUND_G( 5, 10, 15,  4, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8);
        QROUND_G( 9, 14,  3,  8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed);
        QROUND_G(13,  2,  7, 12, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a);
        MD5_COMPILER_BARRIER();

        QROUND_H( 5,  8, 11, 14, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c);
        QROUND_H( 1,  4,  7, 10, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70);
        QROUND_H(13,  0,  3,  6, 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05);
        QROUND_H( 9, 12, 15,  2, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665);
        MD5_COMPILER_BARRIER();

        QROUND_I( 0,  7, 14,  5, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039);
        QROUND_I(12,  3, 10,  1, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1);
        QROUND_I( 8, 15,  6, 13, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1);
        QROUND_I( 4, 11,  2,  9, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391);
        MD5_COMPILER_BARRIER();

        a += last_a;
        b += last_b;
        c += last_c;
        d += last_d;

        block += MD5_BLOCK_SIZE;
    }
    while (--count);

    state[0] = a;
    state[1] = b;
    state[2] = c;
    state[3] = d;

    #undef QROUND_F
    #undef QROUND_G
    #undef QROUND_H
    #undef QROUND_I
    #undef ROUND
    #undef X
    #undef F
    #undef G
    #undef H
    #undef I
}

#endif // defined(__x86_64__) || defined(_M_AMD64)

static void md5_process(uint32_t* state, const uint8_t* block, size_t count)
{
#if defined(__x86_64__) || defined(_M_AMD64)
    int cpuid = md5_cpuid();
    if (cpuid & MD5_CPUID_BMI2)
    {
        md5_process_bmi2(state, block, count);
        return;
    }
#endif

    // F function uses 3 operations instead of 4 when "bit select" instruction is not available
    // (x & y) | (~x & z) == (z ^ (x & (y ^ z))

    // G function uses + instead of | for better ILP

    // #define F(x,y,z) ((x & y) | (~x & z))
    #define F(x,y,z) (z ^ (x & (y ^ z)))
    #define G(x,y,z) (x & z) + (y & ~z)
    #define H(x,y,z) (x ^ y ^ z)
    #define I(x,y,z) (y ^ (x | ~z))

    #define X(i) MD5_GET32LE(block + i*sizeof(uint32_t))

    #define ROUND(F, a, b, c, d, x, k, r) do {  \
        a += F(b, c, d) + (x) + (k);            \
        a = MD5_ROL32(a, r) + b;                \
    } while (0)

    #define QROUND_F(x0, x1, x2, x3, k0, k1, k2, k3) do {    \
        ROUND(F, a, b, c, d, X(x0), k0,  7);                 \
        ROUND(F, d, a, b, c, X(x1), k1, 12);                 \
        ROUND(F, c, d, a, b, X(x2), k2, 17);                 \
        ROUND(F, b, c, d, a, X(x3), k3, 22);                 \
    } while (0)

    #define QROUND_G(x0, x1, x2, x3, k0, k1, k2, k3) do {   \
        ROUND(G, a, b, c, d, X(x0), k0,  5);                \
        ROUND(G, d, a, b, c, X(x1), k1,  9);                \
        ROUND(G, c, d, a, b, X(x2), k2, 14);                \
        ROUND(G, b, c, d, a, X(x3), k3, 20);                \
    } while (0)

    #define QROUND_H(x0, x1, x2, x3, k0, k1, k2, k3) do {   \
        ROUND(H, a, b, c, d, X(x0), k0,  4);                \
        ROUND(H, d, a, b, c, X(x1), k1, 11);                \
        ROUND(H, c, d, a, b, X(x2), k2, 16);                \
        ROUND(H, b, c, d, a, X(x3), k3, 23);                \
    } while (0)

    #define QROUND_I(x0, x1, x2, x3, k0, k1, k2, k3) do {   \
        ROUND(I, a, b, c, d, X(x0), k0,  6);                \
        ROUND(I, d, a, b, c, X(x1), k1, 10);                \
        ROUND(I, c, d, a, b, X(x2), k2, 15);                \
        ROUND(I, b, c, d, a, X(x3), k3, 21);                \
    } while (0)

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];

    do
    {
        uint32_t last_a = a;
        uint32_t last_b = b;
        uint32_t last_c = c;
        uint32_t last_d = d;

        QROUND_F( 0,  1,  2,  3, 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee);
        QROUND_F( 4,  5,  6,  7, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501);
        QROUND_F( 8,  9, 10, 11, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be);
        QROUND_F(12, 13, 14, 15, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821);
        MD5_COMPILER_BARRIER();

        QROUND_G( 1,  6, 11,  0, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa);
        QROUND_G( 5, 10, 15,  4, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8);
        QROUND_G( 9, 14,  3,  8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed);
        QROUND_G(13,  2,  7, 12, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a);
        MD5_COMPILER_BARRIER();

        QROUND_H( 5,  8, 11, 14, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c);
        QROUND_H( 1,  4,  7, 10, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70);
        QROUND_H(13,  0,  3,  6, 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05);
        QROUND_H( 9, 12, 15,  2, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665);
        MD5_COMPILER_BARRIER();

        QROUND_I( 0,  7, 14,  5, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039);
        QROUND_I(12,  3, 10,  1, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1);
        QROUND_I( 8, 15,  6, 13, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1);
        QROUND_I( 4, 11,  2,  9, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391);
        MD5_COMPILER_BARRIER();

        a += last_a;
        b += last_b;
        c += last_c;
        d += last_d;

        block += MD5_BLOCK_SIZE;
    }
    while (--count);

    state[0] = a;
    state[1] = b;
    state[2] = c;
    state[3] = d;

    #undef QROUND_F
    #undef QROUND_G
    #undef QROUND_H
    #undef QROUND_I
    #undef ROUND
    #undef X
    #undef F
    #undef G
    #undef H
    #undef I
}

void md5_init(md5_ctx* ctx)
{
    ctx->count = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

void md5_update(md5_ctx* ctx, const void* data, size_t size)
{
    const uint8_t* buffer = (const uint8_t*)data;

    size_t pending = ctx->count % MD5_BLOCK_SIZE;
    ctx->count += size;

    size_t available = MD5_BLOCK_SIZE - pending;
    if (pending && size >= available)
    {
        memcpy(ctx->buffer + pending, buffer, available);
        md5_process(ctx->state, ctx->buffer, 1);
        buffer += available;
        size -= available;
        pending = 0;
    }

    size_t count = size / MD5_BLOCK_SIZE;
    if (count)
    {
        md5_process(ctx->state, buffer, count);
        buffer += count * MD5_BLOCK_SIZE;
        size -= count * MD5_BLOCK_SIZE;
    }

    memcpy(ctx->buffer + pending, buffer, size);
}

void md5_finish(md5_ctx* ctx, uint8_t digest[MD5_DIGEST_SIZE])
{
    uint64_t count = ctx->count;
    uint64_t bitcount = count * 8;

    size_t pending = count % MD5_BLOCK_SIZE;
    size_t blocks = pending < MD5_BLOCK_SIZE - sizeof(bitcount) ? 1 : 2;

    ctx->buffer[pending++] = 0x80;

    uint8_t padding[2 * MD5_BLOCK_SIZE];
    memcpy(padding, ctx->buffer, MD5_BLOCK_SIZE);
    memset(padding + pending, 0, MD5_BLOCK_SIZE);
    MD5_SET64LE(padding + blocks * MD5_BLOCK_SIZE - sizeof(bitcount), bitcount);

    md5_process(ctx->state, padding, blocks);

    for (size_t i=0; i<4; i++)
    {
        MD5_SET32LE(digest + i*sizeof(uint32_t), ctx->state[i]);
    }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
