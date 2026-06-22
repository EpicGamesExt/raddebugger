#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if !defined(MEM_DISABLE_ASAN)
#  if defined(__SANITIZE_ADDRESS__)
#    if defined(_MSC_VER) && !defined(__clang__)
#      define MEM_DISABLE_ASAN __declspec(no_sanitize_address)
#    else
#      define MEM_DISABLE_ASAN __attribute__((no_sanitize("address")))
#    endif
#  else
#    define MEM_DISABLE_ASAN
#  endif
#endif

#if !defined(MEM_API)
#  if defined(MEM_STATIC)
#    define MEM_API static inline MEM_DISABLE_ASAN
#  else
#    define MEM_API extern MEM_DISABLE_ASAN
#  endif
#endif

//
// interface
//

#ifdef __cplusplus
extern "C" {
#endif

// compares bytes in lexicographic order and returns:
//  < 0 if ptr1 comes before ptr2
//  = 0 if ptr1 contains same values as ptr2
//  > 0 if ptr1 comes after ptr2
MEM_API int MemCompare(const void* ptr1, const void* ptr2, size_t size);

// same as above, but case insensitive for ASCII characters
// value returned is produced as if comparing bytes converted to ASCII lowercase
MEM_API int MemCompareI(const void* ptr1, const void* ptr2, size_t size);

// returns true if all bytes of ptr1 are the same as ptr2
MEM_API bool MemIsEqual(const void* ptr1, const void* ptr2, size_t size);

// returns first offset of "value" byte, or "size" if not found
MEM_API size_t MemFind(const void* ptr, size_t size, uint8_t value);


MEM_API int MemCompare_sse2  (const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompare_avx2  (const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompare_avx512(const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompare_arm64 (const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompare_rvv   (const void* ptr1, const void* ptr2, size_t size);


MEM_API int MemCompareI_sse2  (const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompareI_avx2  (const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompareI_avx512(const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompareI_arm64 (const void* ptr1, const void* ptr2, size_t size);
MEM_API int MemCompareI_rvv   (const void* ptr1, const void* ptr2, size_t size);


MEM_API bool MemIsEqual_sse2  (const void* ptr1, const void* ptr2, size_t size);
MEM_API bool MemIsEqual_avx2  (const void* ptr1, const void* ptr2, size_t size);
MEM_API bool MemIsEqual_avx512(const void* ptr1, const void* ptr2, size_t size);
MEM_API bool MemIsEqual_arm64 (const void* ptr1, const void* ptr2, size_t size);
MEM_API bool MemIsEqual_rvv   (const void* ptr1, const void* ptr2, size_t size);


MEM_API size_t MemFind_sse2  (const void* ptr, size_t size, uint8_t value);
MEM_API size_t MemFind_avx2  (const void* ptr, size_t size, uint8_t value);
MEM_API size_t MemFind_avx512(const void* ptr, size_t size, uint8_t value);
MEM_API size_t MemFind_arm64 (const void* ptr, size_t size, uint8_t value);
MEM_API size_t MemFind_rvv   (const void* ptr, size_t size, uint8_t value);


#ifdef __cplusplus
}
#endif

//
// implementation
//

#if defined(MEM_STATIC) || defined(MEM_IMPLEMENTATION)

// compiler
#if defined(__clang__)
#  define MEM_COMPILER_CLANG 1
#elif defined(__GNUC__)
#  define MEM_COMPILER_GCC 1
#elif defined(_MSC_VER)
#  define MEM_COMPILER_MSVC 1
#  include <intrin.h>
#else
#  error unknown compiler
#endif

// architecture
#if defined(__x86_64__) || defined(_M_AMD64)
#  define MEM_ARCH_X64 1
#  include <immintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define MEM_ARCH_ARM64 1
#  include <arm_neon.h>
#elif defined(__riscv) && __riscv_v >= 1000000
#  define MEM_ARCH_RVV 1
#  include <riscv_vector.h>
#else
#  error unsupported arch
#endif

// cpuid, only for x64
#if MEM_ARCH_X64
#  if MEM_COMPILER_CLANG || MEM_COMPILER_GCC
#    include <cpuid.h>
#    define MEM_CPUID(x, info)            __cpuid(x, info[0], info[1], info[2], info[3])
#    define MEM_CPUID2(x, y, info)        __cpuid_count(x, y, info[0], info[1], info[2], info[3])
#    define MEM_XGETBV(x)                 __builtin_ia32_xgetbv(x)
#    define MEM_GET32_RELAXED(ptr)        __atomic_load_n(ptr, __ATOMIC_RELAXED)
#    define MEM_SET32_RELAXED(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
#  elif MEM_COMPILER_MSVC
#    define MEM_CPUID(x, info)            __cpuid(info, x)
#    define MEM_CPUID2(x, y, info)        __cpuidex(info, x, y)
#    define MEM_XGETBV(x)                 _xgetbv(x)
#    define MEM_GET32_RELAXED(ptr)        __iso_volatile_load32(ptr)
#    define MEM_SET32_RELAXED(ptr, value) __iso_volatile_store32(ptr, value);
#  endif
#endif

// unaligned memory access
#pragma pack(push, 1)
typedef struct { uint16_t value; } MemUnalignedPtr16;
typedef struct { uint32_t value; } MemUnalignedPtr32;
typedef struct { uint64_t value; } MemUnalignedPtr64;
#define MEM_PTR16U(ptr) (((MemUnalignedPtr16*)(ptr))->value)
#define MEM_PTR32U(ptr) (((MemUnalignedPtr32*)(ptr))->value)
#define MEM_PTR64U(ptr) (((MemUnalignedPtr64*)(ptr))->value)
#pragma pack(pop)

// big-endian load, only for x64, compiles to movbe
#if MEM_ARCH_X64
#  if MEM_COMPILER_MSVC
#    define MEM_GET16BE(ptr) _load_be_u16(ptr)
#    define MEM_GET32BE(ptr) _load_be_u32(ptr)
#    define MEM_GET64BE(ptr) _load_be_u64(ptr)
#  elif MEM_COMPILER_CLANG || MEM_COMPILER_GCC
#    define MEM_GET16BE(ptr) __builtin_bswap16(MEM_PTR16U(ptr))
#    define MEM_GET32BE(ptr) __builtin_bswap32(MEM_PTR32U(ptr))
#    define MEM_GET64BE(ptr) __builtin_bswap64(MEM_PTR64U(ptr))
#  endif
#endif

// byteswap
#if MEM_COMPILER_CLANG || MEM_COMPILER_GCC
#  define MEM_BSWAP64(x) __builtin_bswap64(x)
#elif MEM_COMPILER_MSVC
#  define MEM_BSWAP64(x) _byteswap_uint64(x)
#endif

// count trailing zeroes
#if MEM_COMPILER_CLANG || MEM_COMPILER_GCC
#  define MEM_CTZ32(x) (size_t)__builtin_ctz(x)
#  define MEM_CTZ64(x) (size_t)__builtin_ctzll(x)
#elif MEM_COMPILER_MSVC
#  if MEM_ARCH_X64
#    define MEM_CTZ32(x) (size_t)_tzcnt_u32(x)
#    define MEM_CTZ64(x) (size_t)_tzcnt_u64(x)
#  elif MEM_ARCH_ARM64
#    define MEM_CTZ32(x) (size_t)_CountTrailingZeros(x)
#    define MEM_CTZ64(x) (size_t)_CountTrailingZeros64(x)
#  endif
#endif

// shrx for x64
#if MEM_ARCH_X64
#  if MEM_COMPILER_MSVC
#    define MEM_SHRX_32(x, n)   _shrx_u32(x, n)
#  elif MEM_COMPILER_CLANG || MEM_COMPILER_GCC
#    define MEM_SHRX_32(x, n)   ((x) >> (n))
#  endif
#endif

// x64 function attributes
#if MEM_ARCH_X64 && (MEM_COMPILER_CLANG || MEM_COMPILER_GCC)
#  define MEM_TARGET_XSAVE  __attribute__((target("xsave")))
#  define MEM_TARGET_AVX2   __attribute__((target("avx2,bmi,bmi2,movbe")))
#  define MEM_TARGET_AVX512 __attribute__((target("avx512f,avx512bw,avx512vbmi,bmi,bmi2")))
#else
#  define MEM_TARGET_XSAVE
#  define MEM_TARGET_AVX2
#  define MEM_TARGET_AVX512
#endif

static inline uint8_t MemToLower1(uint8_t x)
{
    return (uint8_t)(x - 'A') <= ('Z' - 'A') ? (x + 'a' - 'A') : x;
}

static inline uint64_t MemToLower8(uint64_t x)
{
    const uint64_t splat = ~0ULL / 255;
    uint64_t heptets = x & (0x7F * splat);
    uint64_t is_ascii = ~x & (0x80 * splat);
    uint64_t is_gt_Z = heptets + (0x7F - 'Z') * splat;
    uint64_t is_ge_A = heptets + (0x80 - 'A') * splat;
    uint64_t is_upper = (is_ge_A ^ is_gt_Z) & is_ascii;
    return x | (is_upper >> 2);
}


#if MEM_ARCH_X64

static inline __m128i MemToLower16(__m128i x)
{
    __m128i tmp = _mm_sub_epi8(x, _mm_set1_epi8('A' - 128));
    tmp = _mm_cmpgt_epi8(tmp, _mm_set1_epi8('Z' - 'A' - 128));
    tmp = _mm_andnot_si128(tmp, _mm_set1_epi8('a' - 'A'));
    return _mm_add_epi8(x, tmp);
}

MEM_TARGET_AVX2
static inline __m256i MemToLower32(__m256i x)
{
    __m256i tmp = _mm256_sub_epi8(x, _mm256_set1_epi8('A' - 128));
    tmp = _mm256_cmpgt_epi8(tmp, _mm256_set1_epi8('Z' - 'A' - 128));
    tmp = _mm256_andnot_si256(tmp, _mm256_set1_epi8('a' - 'A'));
    return _mm256_add_epi8(x, tmp);
}

MEM_TARGET_AVX512
static inline __m512i MemToLower64(__m512i x)
{
#if 0 // version without VBMI
    __m512i tmp = _mm512_sub_epi8(x, _mm512_set1_epi8('A'));
    __mmask64 mask = _mm512_cmple_epu8_mask(tmp, _mm512_set1_epi8('Z' - 'A'));
    return _mm512_mask_add_epi8(x, mask, x, _mm512_set1_epi8('a' - 'A'));
#else
    static const uint8_t table[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
         'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    };
    const __m512i c0 = _mm512_loadu_epi8(table + 0x00);
    const __m512i c1 = _mm512_loadu_epi8(table + 0x40);
    __mmask64 mask = _mm512_cmplt_epu8_mask(x, _mm512_set1_epi8((char)0x80));
    return _mm512_mask2_permutex2var_epi8(c0, x, mask, c1);
#endif
}

MEM_DISABLE_ASAN
int MemCompare_sse2(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return 0;
    }

    if (size <= 16)
    {
        const uint32_t PAGE_SIZE = 4096;

        uint32_t address = (uint32_t)(uintptr_t)p1 | (uint32_t)(uintptr_t)p2;
        if ((address & (PAGE_SIZE - 1)) <= PAGE_SIZE - 16)
        {
            __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
            __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
            __m128i r0 = _mm_cmpeq_epi8(a0, b0);

            uint32_t mask = (1 + (uint16_t)_mm_movemask_epi8(r0)) & ((1 << size) - 1);
            if (mask)
            {
                size_t index = MEM_CTZ32(mask);
                return p1[index] - p2[index];
            }
            return 0;
        }

        if (size < 2) // size == 1
        {
            return p1[0] - p2[0];
        }

        uint64_t a0, b0, a1, b1;
        if (size < 4) // 2 <= size < 4
        {
            a0 = MEM_PTR16U(p1);
            b0 = MEM_PTR16U(p2);
            a1 = MEM_PTR16U(p1 + size - 2);
            b1 = MEM_PTR16U(p2 + size - 2);
        }
        else if (size < 8) // 4 <= size < 8
        {
            a0 = MEM_PTR32U(p1);
            b0 = MEM_PTR32U(p2);
            a1 = MEM_PTR32U(p1 + size - 4);
            b1 = MEM_PTR32U(p2 + size - 4);
        }
        else // 8 <= size <= 16
        {
            a0 = MEM_PTR64U(p1);
            b0 = MEM_PTR64U(p2);
            a1 = MEM_PTR64U(p1 + size - 8);
            b1 = MEM_PTR64U(p2 + size - 8);
        }

        uint64_t a = MEM_BSWAP64(a0 != b0 ? a0 : a1);
        uint64_t b = MEM_BSWAP64(a0 != b0 ? b0 : b1);
        return (a > b) - (a < b);
    }

    while (size >= 64)
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)(p1 + 0x00));
        __m128i b0 = _mm_loadu_si128((const __m128i*)(p2 + 0x00));
        __m128i a1 = _mm_loadu_si128((const __m128i*)(p1 + 0x10));
        __m128i b1 = _mm_loadu_si128((const __m128i*)(p2 + 0x10));
        __m128i a2 = _mm_loadu_si128((const __m128i*)(p1 + 0x20));
        __m128i b2 = _mm_loadu_si128((const __m128i*)(p2 + 0x20));
        __m128i a3 = _mm_loadu_si128((const __m128i*)(p1 + 0x30));
        __m128i b3 = _mm_loadu_si128((const __m128i*)(p2 + 0x30));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);
        __m128i r1 = _mm_cmpeq_epi8(a1, b1);
        __m128i r2 = _mm_cmpeq_epi8(a2, b2);
        __m128i r3 = _mm_cmpeq_epi8(a3, b3);
        __m128i r = _mm_and_si128(_mm_and_si128(r0, r1), _mm_and_si128(r2, r3));

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r);
        if (mask)
        {
            mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
            if (mask)
            {
                size_t index = 0x00 + MEM_CTZ32(mask);
                return p1[index] - p2[index];
            }

            mask = 1 + (uint16_t)_mm_movemask_epi8(r1);
            if (mask)
            {
                size_t index = 0x10 + MEM_CTZ32(mask);
                return p1[index] - p2[index];
            }

            mask = 1 + (uint16_t)_mm_movemask_epi8(r2);
            if (mask)
            {
                size_t index = 0x20 + MEM_CTZ32(mask);
                return p1[index] - p2[index];
            }

            mask = 1 + (uint16_t)_mm_movemask_epi8(r3);
            size_t index = 0x30 + MEM_CTZ32(mask);
            return p1[index] - p2[index];
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
        __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);
            return p1[index] - p2[index];
        }

        __m128i a1 = _mm_loadu_si128((const __m128i*)(p1 + 0x10));
        __m128i b1 = _mm_loadu_si128((const __m128i*)(p2 + 0x10));
        __m128i r1 = _mm_cmpeq_epi8(a1, b1);

        mask = 1 + (uint16_t)_mm_movemask_epi8(r1);
        if (mask)
        {
            size_t index = 0x10 + MEM_CTZ32(mask);
            return p1[index] - p2[index];
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
        __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);
            return p1[index] - p2[index];
        }

        size -= 16;
        p1 += 16;
        p2 += 16;
    }

    if (size) // size < 16
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)(p1 + size - 16));
        __m128i b0 = _mm_loadu_si128((const __m128i*)(p2 + size - 16));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask) + size - 16;
            return p1[index] - p2[index];
        }
    }

    return 0;
}

MEM_DISABLE_ASAN
int MemCompareI_sse2(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return 0;
    }

    if (size <= 16)
    {
        const uint32_t PAGE_SIZE = 4096;

        uint32_t address = (uint32_t)(uintptr_t)p1 | (uint32_t)(uintptr_t)p2;
        if ((address & (PAGE_SIZE - 1)) <= PAGE_SIZE - 16)
        {
            __m128i a0 = MemToLower16(_mm_loadu_si128((const __m128i*)p1));
            __m128i b0 = MemToLower16(_mm_loadu_si128((const __m128i*)p2));
            __m128i r0 = _mm_cmpeq_epi8(a0, b0);

            uint32_t mask = (1 + (uint16_t)_mm_movemask_epi8(r0)) & ((1 << size) - 1);
            if (mask)
            {
                size_t index = MEM_CTZ32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            return 0;
        }

        if (size < 2) // size == 1
        {
            return MemToLower1(p1[0]) - MemToLower1(p2[0]);
        }

        size_t n;
        __m128i a0, b0, a1, b1;

        if (size < 4) // 2 <= size < 4
        {
            a0 = _mm_loadu_si16((const __m128i*)p1);
            b0 = _mm_loadu_si16((const __m128i*)p2);
            a1 = _mm_loadu_si16((const __m128i*)(p1 + size - 2));
            b1 = _mm_loadu_si16((const __m128i*)(p2 + size - 2));
            n = 2;
        }
        else if (size < 8) // 4 <= size < 8
        {
            a0 = _mm_loadu_si32((const __m128i*)p1);
            b0 = _mm_loadu_si32((const __m128i*)p2);
            a1 = _mm_loadu_si32((const __m128i*)(p1 + size - 4));
            b1 = _mm_loadu_si32((const __m128i*)(p2 + size - 4));
            n = 4;
        }
        else // 8 <= size <= 16
        {
            a0 = _mm_loadu_si64((const __m128i*)p1);
            b0 = _mm_loadu_si64((const __m128i*)p2);
            a1 = _mm_loadu_si64((const __m128i*)(p1 + size - 8));
            b1 = _mm_loadu_si64((const __m128i*)(p2 + size - 8));
            n = 8;
        }

        __m128i a = _mm_unpacklo_epi64(a0, a1);
        __m128i b = _mm_unpacklo_epi64(b0, b1);
        __m128i r = _mm_cmpeq_epi8(MemToLower16(a), MemToLower16(b));

        uint16_t mask = (1 + (uint16_t)_mm_movemask_epi8(r));
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);

            // index = (index < 8) ? index : (index - 8) + (size - n);
            size -= 8 + n;
            index += index < 8 ? 0 : size;

            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        return 0;
    }

    while (size >= 64)
    {
        __m128i a0 = MemToLower16(_mm_loadu_si128((const __m128i*)(p1 + 0x00)));
        __m128i b0 = MemToLower16(_mm_loadu_si128((const __m128i*)(p2 + 0x00)));
        __m128i a1 = MemToLower16(_mm_loadu_si128((const __m128i*)(p1 + 0x10)));
        __m128i b1 = MemToLower16(_mm_loadu_si128((const __m128i*)(p2 + 0x10)));
        __m128i a2 = MemToLower16(_mm_loadu_si128((const __m128i*)(p1 + 0x20)));
        __m128i b2 = MemToLower16(_mm_loadu_si128((const __m128i*)(p2 + 0x20)));
        __m128i a3 = MemToLower16(_mm_loadu_si128((const __m128i*)(p1 + 0x30)));
        __m128i b3 = MemToLower16(_mm_loadu_si128((const __m128i*)(p2 + 0x30)));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);
        __m128i r1 = _mm_cmpeq_epi8(a1, b1);
        __m128i r2 = _mm_cmpeq_epi8(a2, b2);
        __m128i r3 = _mm_cmpeq_epi8(a3, b3);
        __m128i r = _mm_and_si128(_mm_and_si128(r0, r1), _mm_and_si128(r2, r3));

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r);
        if (mask)
        {
            mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
            if (mask)
            {
                size_t index = 0x00 + MEM_CTZ32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            mask = 1 + (uint16_t)_mm_movemask_epi8(r1);
            if (mask)
            {
                size_t index = 0x10 + MEM_CTZ32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            mask = 1 + (uint16_t)_mm_movemask_epi8(r2);
            if (mask)
            {
                size_t index = 0x20 + MEM_CTZ32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            mask = 1 + (uint16_t)_mm_movemask_epi8(r3);
            size_t index = 0x30 + MEM_CTZ32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m128i a0 = MemToLower16(_mm_loadu_si128((const __m128i*)p1));
        __m128i b0 = MemToLower16(_mm_loadu_si128((const __m128i*)p2));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        __m128i a1 = MemToLower16(_mm_loadu_si128((const __m128i*)(p1 + 0x10)));
        __m128i b1 = MemToLower16(_mm_loadu_si128((const __m128i*)(p2 + 0x10)));
        __m128i r1 = _mm_cmpeq_epi8(a1, b1);

        mask = 1 + (uint16_t)_mm_movemask_epi8(r1);
        if (mask)
        {
            size_t index = 0x10 + MEM_CTZ32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        __m128i a0 = MemToLower16(_mm_loadu_si128((const __m128i*)p1));
        __m128i b0 = MemToLower16(_mm_loadu_si128((const __m128i*)p2));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        size -= 16;
        p1 += 16;
        p2 += 16;
    }

    if (size) // size < 16
    {
        __m128i a0 = MemToLower16(_mm_loadu_si128((const __m128i*)(p1 + size - 16)));
        __m128i b0 = MemToLower16(_mm_loadu_si128((const __m128i*)(p2 + size - 16)));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask) + size - 16;
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }
    }

    return 0;
}

MEM_DISABLE_ASAN
bool MemIsEqual_sse2(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return true;
    }

    if (size <= 16)
    {
        const uint32_t PAGE_SIZE = 4096;

        uint32_t address = (uint32_t)(uintptr_t)p1 | (uint32_t)(uintptr_t)p2;
        if ((address & (PAGE_SIZE - 1)) <= PAGE_SIZE - 16)
        {
            __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
            __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
            __m128i r0 = _mm_cmpeq_epi8(a0, b0);

            uint32_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
            return (mask << (32 - size)) == 0;
        }

        if (size < 2) // size == 1
        {
            return p1[0] == p2[0];
        }

        uint64_t a0, b0, a1, b1;
        if (size < 4) // 2 <= size < 4
        {
            a0 = MEM_PTR16U(p1);
            b0 = MEM_PTR16U(p2);
            a1 = MEM_PTR16U(p1 + size - 2);
            b1 = MEM_PTR16U(p2 + size - 2);
        }
        else if (size < 8) // 4 <= size < 8
        {
            a0 = MEM_PTR32U(p1);
            b0 = MEM_PTR32U(p2);
            a1 = MEM_PTR32U(p1 + size - 4);
            b1 = MEM_PTR32U(p2 + size - 4);
        }
        else // 8 <= size <= 16
        {
            a0 = MEM_PTR64U(p1);
            b0 = MEM_PTR64U(p2);
            a1 = MEM_PTR64U(p1 + size - 8);
            b1 = MEM_PTR64U(p2 + size - 8);
        }

        return (a0 == b0) & (a1 == b1);
    }

    while (size >= 64)
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)(p1 + 0x00));
        __m128i b0 = _mm_loadu_si128((const __m128i*)(p2 + 0x00));
        __m128i a1 = _mm_loadu_si128((const __m128i*)(p1 + 0x10));
        __m128i b1 = _mm_loadu_si128((const __m128i*)(p2 + 0x10));
        __m128i a2 = _mm_loadu_si128((const __m128i*)(p1 + 0x20));
        __m128i b2 = _mm_loadu_si128((const __m128i*)(p2 + 0x20));
        __m128i a3 = _mm_loadu_si128((const __m128i*)(p1 + 0x30));
        __m128i b3 = _mm_loadu_si128((const __m128i*)(p2 + 0x30));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);
        __m128i r1 = _mm_cmpeq_epi8(a1, b1);
        __m128i r2 = _mm_cmpeq_epi8(a2, b2);
        __m128i r3 = _mm_cmpeq_epi8(a3, b3);
        __m128i r = _mm_and_si128(_mm_and_si128(r0, r1), _mm_and_si128(r2, r3));

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r);
        if (mask)
        {
            return false;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
        __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
        __m128i a1 = _mm_loadu_si128((const __m128i*)(p1 + 0x10));
        __m128i b1 = _mm_loadu_si128((const __m128i*)(p2 + 0x10));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);
        __m128i r1 = _mm_cmpeq_epi8(a1, b1);
        __m128i r = _mm_and_si128(r0, r1);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r);
        if (mask)
        {
            return false;
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
        __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            return false;
        }

        size -= 16;
        p1 += 16;
        p2 += 16;
    }

    if (size) // size < 16
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)(p1 + size - 16));
        __m128i b0 = _mm_loadu_si128((const __m128i*)(p2 + size - 16));
        __m128i r0 = _mm_cmpeq_epi8(a0, b0);

        uint16_t mask = 1 + (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            return false;
        }
    }

    return true;
}

MEM_DISABLE_ASAN
size_t MemFind_sse2(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;
    const __m128i value16 = _mm_set1_epi8((char)value);

    if (size == 0)
    {
        return 0;
    }

    if (size <= 16)
    {
        size_t address = (uint32_t)(uintptr_t)p % 16;
        size_t extra = (address + size) <= 16 ? address : 0;

        __m128i a0 = _mm_loadu_si128((const __m128i*)(p - extra));
        __m128i r0 = _mm_cmpeq_epi8(a0, value16);
        uint32_t mask = ((uint16_t)_mm_movemask_epi8(r0) >> extra) & ((1 << size) - 1);

        return mask ? MEM_CTZ32(mask) : size;
    }

    size_t offset = 0;
    while (size >= 64)
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)(p + 0x00));
        __m128i a1 = _mm_loadu_si128((const __m128i*)(p + 0x10));
        __m128i a2 = _mm_loadu_si128((const __m128i*)(p + 0x20));
        __m128i a3 = _mm_loadu_si128((const __m128i*)(p + 0x30));
        __m128i r0 = _mm_cmpeq_epi8(a0, value16);
        __m128i r1 = _mm_cmpeq_epi8(a1, value16);
        __m128i r2 = _mm_cmpeq_epi8(a2, value16);
        __m128i r3 = _mm_cmpeq_epi8(a3, value16);
        __m128i r = _mm_or_si128(_mm_or_si128(r0, r1), _mm_or_si128(r2, r3));

        uint16_t mask = (uint16_t)_mm_movemask_epi8(r);
        if (mask)
        {
            uint64_t m0 = (uint16_t)_mm_movemask_epi8(r0);
            uint64_t m1 = (uint16_t)_mm_movemask_epi8(r1);
            uint64_t m2 = (uint16_t)_mm_movemask_epi8(r2);
            uint64_t m3 = mask; // if r0=r1=r2=0, then r3=r

            uint64_t m4 = m0 | (m1 << 16) | (m2 << 32) | (m3 << 48);
            size_t index = MEM_CTZ64(m4);
            return offset + index;
        }

        offset += 64;
        size -= 64;
        p += 64;
    }

    if (size & 32)
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)p);
        __m128i r0 = _mm_cmpeq_epi8(a0, value16);

        uint16_t mask = (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);
            return offset + index;
        }

        __m128i a1 = _mm_loadu_si128((const __m128i*)(p + 0x10));
        __m128i r1 = _mm_cmpeq_epi8(a1, value16);

        mask = (uint16_t)_mm_movemask_epi8(r1);
        if (mask)
        {
            size_t index = 0x10 + MEM_CTZ32(mask);
            return offset + index;
        }

        offset += 32;
        size -= 32;
        p += 32;
    }

    if (size & 16)
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)p);
        __m128i r0 = _mm_cmpeq_epi8(a0, value16);

        uint16_t mask = (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask);
            return offset + index;
        }

        offset += 16;
        size -= 16;
        p += 16;
    }

    if (size) // size < 16
    {
        __m128i a0 = _mm_loadu_si128((const __m128i*)(p + size - 16));
        __m128i r0 = _mm_cmpeq_epi8(a0, value16);

        uint16_t mask = (uint16_t)_mm_movemask_epi8(r0);
        if (mask)
        {
            size_t index = MEM_CTZ32(mask) + size - 16;
            return offset + index;
        }

        offset += size;
    }

    return offset;
}

MEM_DISABLE_ASAN
MEM_TARGET_AVX2
int MemCompare_avx2(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return 0;
    }

    if (size <= 32)
    {
        const uint32_t PAGE_SIZE = 4096;

        uint32_t address = (uint32_t)(uintptr_t)p1 | (uint32_t)(uintptr_t)p2;
        if ((address & (PAGE_SIZE - 1)) <= PAGE_SIZE - 32)
        {
            __m256i a0 = _mm256_loadu_si256((const __m256i*)p1);
            __m256i b0 = _mm256_loadu_si256((const __m256i*)p2);
            __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

            uint32_t mask = _bzhi_u32(1 + (uint32_t)_mm256_movemask_epi8(r0), (uint32_t)size);
            if (mask)
            {
                size_t index = _tzcnt_u32(mask);
                return p1[index] - p2[index];
            }

            return 0;
        }

        if (size < 2) // size == 1
        {
            return p1[0] - p2[0];
        }
        else if (size < 4) // 2 <= size < 4
        {
            uint32_t a0 = MEM_GET16BE(p1);
            uint32_t b0 = MEM_GET16BE(p2);
            uint32_t a1 = MEM_GET16BE(p1 + size - 2);
            uint32_t b1 = MEM_GET16BE(p2 + size - 2);
            uint32_t a = (a0 != b0 ? a0 : a1);
            uint32_t b = (a0 != b0 ? b0 : b1);
            return (int)a - (int)b;
        }
        else if (size < 8) // 4 <= size < 8
        {
            uint32_t a0 = MEM_GET32BE(p1);
            uint32_t b0 = MEM_GET32BE(p2);
            uint32_t a1 = MEM_GET32BE(p1 + size - 4);
            uint32_t b1 = MEM_GET32BE(p2 + size - 4);
            uint32_t a = (a0 != b0 ? a0 : a1);
            uint32_t b = (a0 != b0 ? b0 : b1);
            return (a > b) - (a < b);
        }
        else if (size <= 16) // 8 <= size <= 16
        {
            uint64_t a0 = MEM_GET64BE(p1);
            uint64_t b0 = MEM_GET64BE(p2);
            uint64_t a1 = MEM_GET64BE(p1 + size - 8);
            uint64_t b1 = MEM_GET64BE(p2 + size - 8);
            uint64_t a = (a0 != b0 ? a0 : a1);
            uint64_t b = (a0 != b0 ? b0 : b1);
            return (a > b) - (a < b);
        }
        else // 16 < size <= 32
        {
            __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
            __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
            __m128i a1 = _mm_loadu_si128((const __m128i*)(p1 + size - 16));
            __m128i b1 = _mm_loadu_si128((const __m128i*)(p2 + size - 16));
            __m256i a = _mm256_inserti128_si256(_mm256_castsi128_si256(a0), a1, 1);
            __m256i b = _mm256_inserti128_si256(_mm256_castsi128_si256(b0), b1, 1);
            __m256i r = _mm256_cmpeq_epi8(a, b);

            uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r);
            if (mask)
            {
                size_t index = _tzcnt_u64(mask);

                // index = (index < 16) ? index : (index - 16) + (size - 16);
                size -= 32;
                index += index < 16 ? 0 : size;

                return p1[index] - p2[index];
            }

            return 0;
        }
    }

    while (size >= 128)
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p1 + 0x00));
        __m256i b0 = _mm256_loadu_si256((const __m256i*)(p2 + 0x00));
        __m256i a1 = _mm256_loadu_si256((const __m256i*)(p1 + 0x20));
        __m256i b1 = _mm256_loadu_si256((const __m256i*)(p2 + 0x20));
        __m256i a2 = _mm256_loadu_si256((const __m256i*)(p1 + 0x40));
        __m256i b2 = _mm256_loadu_si256((const __m256i*)(p2 + 0x40));
        __m256i a3 = _mm256_loadu_si256((const __m256i*)(p1 + 0x60));
        __m256i b3 = _mm256_loadu_si256((const __m256i*)(p2 + 0x60));
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);
        __m256i r1 = _mm256_cmpeq_epi8(a1, b1);
        __m256i r2 = _mm256_cmpeq_epi8(a2, b2);
        __m256i r3 = _mm256_cmpeq_epi8(a3, b3);
        __m256i r = _mm256_and_si256(_mm256_and_si256(r0, r1), _mm256_and_si256(r2, r3));

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r);
        if (mask)
        {
            mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
            if (mask)
            {
                size_t index = 0x00 + _tzcnt_u32(mask);
                return p1[index] - p2[index];
            }

            mask = 1 + (uint32_t)_mm256_movemask_epi8(r1);
            if (mask)
            {
                size_t index = 0x20 + _tzcnt_u32(mask);
                return p1[index] - p2[index];
            }

            mask = 1 + (uint32_t)_mm256_movemask_epi8(r2);
            if (mask)
            {
                size_t index = 0x40 + _tzcnt_u32(mask);
                return p1[index] - p2[index];
            }

            mask = 1 + (uint32_t)_mm256_movemask_epi8(r3);
            size_t index = 0x60 + _tzcnt_u32(mask);
            return p1[index] - p2[index];
        }

        size -= 128;
        p1 += 128;
        p2 += 128;
    }

    if (size & 64) // 64 <= size < 128
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)p1);
        __m256i b0 = _mm256_loadu_si256((const __m256i*)p2);
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask);
            return p1[index] - p2[index];
        }

        __m256i a1 = _mm256_loadu_si256((const __m256i*)(p1 + 0x20));
        __m256i b1 = _mm256_loadu_si256((const __m256i*)(p2 + 0x20));
        __m256i r1 = _mm256_cmpeq_epi8(a1, b1);

        mask = 1 + (uint32_t)_mm256_movemask_epi8(r1);
        if (mask)
        {
            size_t index = 0x20 + _tzcnt_u32(mask);
            return p1[index] - p2[index];
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)p1);
        __m256i b0 = _mm256_loadu_si256((const __m256i*)p2);
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask);
            return p1[index] - p2[index];
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size) // size < 32
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p1 + size - 32));
        __m256i b0 = _mm256_loadu_si256((const __m256i*)(p2 + size - 32));
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask) + size - 32;
            return p1[index] - p2[index];
        }
    }

    return 0;
}

MEM_DISABLE_ASAN
MEM_TARGET_AVX2
int MemCompareI_avx2(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return 0;
    }

    if (size <= 32)
    {
        const uint32_t PAGE_SIZE = 4096;

        uint32_t address = (uint32_t)(uintptr_t)p1 | (uint32_t)(uintptr_t)p2;
        if ((address & (PAGE_SIZE - 1)) <= PAGE_SIZE - 32)
        {
            __m256i a0 = MemToLower32(_mm256_loadu_si256((const __m256i*)p1));
            __m256i b0 = MemToLower32(_mm256_loadu_si256((const __m256i*)p2));
            __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

            uint32_t mask = _bzhi_u32(1 + (uint32_t)_mm256_movemask_epi8(r0), (uint32_t)size);
            if (mask)
            {
                size_t index = _tzcnt_u32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            return 0;
        }

        if (size < 2) // size == 1
        {
            return MemToLower1(p1[0]) - MemToLower1(p2[0]);
        }

        size_t n;
        __m128i a0, b0, a1, b1;

        if (size < 4) // 2 <= size < 4
        {
            a0 = _mm_loadu_si16((const __m128i*)p1);
            b0 = _mm_loadu_si16((const __m128i*)p2);
            a1 = _mm_loadu_si16((const __m128i*)(p1 + size - 2));
            b1 = _mm_loadu_si16((const __m128i*)(p2 + size - 2));
            n = 2;
        }
        else if (size < 8) // 4 <= size < 8
        {
            a0 = _mm_loadu_si32((const __m128i*)p1);
            b0 = _mm_loadu_si32((const __m128i*)p2);
            a1 = _mm_loadu_si32((const __m128i*)(p1 + size - 4));
            b1 = _mm_loadu_si32((const __m128i*)(p2 + size - 4));
            n = 4;
        }
        else if (size < 16) // 8 <= size < 16
        {
            a0 = _mm_loadu_si64((const __m128i*)p1);
            b0 = _mm_loadu_si64((const __m128i*)p2);
            a1 = _mm_loadu_si64((const __m128i*)(p1 + size - 8));
            b1 = _mm_loadu_si64((const __m128i*)(p2 + size - 8));
            n = 8;
        }
        else // 16 <= size <= 32
        {
            a0 = _mm_loadu_si128((const __m128i*)p1);
            b0 = _mm_loadu_si128((const __m128i*)p2);
            a1 = _mm_loadu_si128((const __m128i*)(p1 + size - 16));
            b1 = _mm_loadu_si128((const __m128i*)(p2 + size - 16));
            n = 16;
        }

        __m256i a = _mm256_inserti128_si256(_mm256_castsi128_si256(a0), a1, 1);
        __m256i b = _mm256_inserti128_si256(_mm256_castsi128_si256(b0), b1, 1);
        __m256i r = _mm256_cmpeq_epi8(MemToLower32(a), MemToLower32(b));

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r);
        if (mask)
        {
            size_t index = _tzcnt_u64(mask);

            // index = (index < 16) ? index : (index - 16) + (size - n);
            size -= 16 + n;
            index += index < 16 ? 0 : size;

            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        return 0;
    }

    while (size >= 128)
    {
        __m256i a0 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p1 + 0x00)));
        __m256i b0 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p2 + 0x00)));
        __m256i a1 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p1 + 0x20)));
        __m256i b1 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p2 + 0x20)));
        __m256i a2 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p1 + 0x40)));
        __m256i b2 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p2 + 0x40)));
        __m256i a3 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p1 + 0x60)));
        __m256i b3 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p2 + 0x60)));
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);
        __m256i r1 = _mm256_cmpeq_epi8(a1, b1);
        __m256i r2 = _mm256_cmpeq_epi8(a2, b2);
        __m256i r3 = _mm256_cmpeq_epi8(a3, b3);
        __m256i r = _mm256_and_si256(_mm256_and_si256(r0, r1), _mm256_and_si256(r2, r3));

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r);
        if (mask)
        {
            mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
            if (mask)
            {
                size_t index = 0x00 + _tzcnt_u32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            mask = 1 + (uint32_t)_mm256_movemask_epi8(r1);
            if (mask)
            {
                size_t index = 0x20 + _tzcnt_u32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            mask = 1 + (uint32_t)_mm256_movemask_epi8(r2);
            if (mask)
            {
                size_t index = 0x40 + _tzcnt_u32(mask);
                return MemToLower1(p1[index]) - MemToLower1(p2[index]);
            }

            mask = 1 + (uint32_t)_mm256_movemask_epi8(r3);
            size_t index = 0x60 + _tzcnt_u32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        size -= 128;
        p1 += 128;
        p2 += 128;
    }

    if (size & 64) // 64 <= size < 128
    {
        __m256i a0 = MemToLower32(_mm256_loadu_si256((const __m256i*)p1));
        __m256i b0 = MemToLower32(_mm256_loadu_si256((const __m256i*)p2));
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        __m256i a1 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p1 + 0x20)));
        __m256i b1 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p2 + 0x20)));
        __m256i r1 = _mm256_cmpeq_epi8(a1, b1);

        mask = 1 + (uint32_t)_mm256_movemask_epi8(r1);
        if (mask)
        {
            size_t index = 0x20 + _tzcnt_u32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m256i a0 = MemToLower32(_mm256_loadu_si256((const __m256i*)p1));
        __m256i b0 = MemToLower32(_mm256_loadu_si256((const __m256i*)p2));
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask);
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size) // size < 32
    {
        __m256i a0 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p1 + size - 32)));
        __m256i b0 = MemToLower32(_mm256_loadu_si256((const __m256i*)(p2 + size - 32)));
        __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

        uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask) + size - 32;
            return MemToLower1(p1[index]) - MemToLower1(p2[index]);
        }
    }

    return 0;
}

MEM_DISABLE_ASAN
MEM_TARGET_AVX2
bool MemIsEqual_avx2(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return true;
    }

    if (size <= 32)
    {
        const uint32_t PAGE_SIZE = 4096;

        uint32_t address = (uint32_t)(uintptr_t)p1 | (uint32_t)(uintptr_t)p2;
        if ((address & (PAGE_SIZE - 1)) <= PAGE_SIZE - 32)
        {
            __m256i a0 = _mm256_loadu_si256((const __m256i*)p1);
            __m256i b0 = _mm256_loadu_si256((const __m256i*)p2);
            __m256i r0 = _mm256_cmpeq_epi8(a0, b0);

            uint32_t mask = 1 + (uint32_t)_mm256_movemask_epi8(r0);
            return _bzhi_u32(mask, (uint32_t)size) == 0;
        }

        if (size < 2) // size == 1
        {
            return p1[0] == p2[0];
        }
        else if (size < 4) // 2 <= size < 4
        {
            uint32_t a0 = MEM_PTR16U(p1);
            uint32_t b0 = MEM_PTR16U(p2);
            uint32_t a1 = MEM_PTR16U(p1 + size - 2);
            uint32_t b1 = MEM_PTR16U(p2 + size - 2);
            return (a0 == b0) & (a1 == b1);
        }
        else if (size < 8) // 4 <= size < 8
        {
            uint32_t a0 = MEM_PTR32U(p1);
            uint32_t b0 = MEM_PTR32U(p2);
            uint32_t a1 = MEM_PTR32U(p1 + size - 4);
            uint32_t b1 = MEM_PTR32U(p2 + size - 4);
            return (a0 == b0) & (a1 == b1);
        }
        else if (size <= 16) // 8 <= size <= 16
        {
            uint64_t a0 = MEM_PTR64U(p1);
            uint64_t b0 = MEM_PTR64U(p2);
            uint64_t a1 = MEM_PTR64U(p1 + size - 8);
            uint64_t b1 = MEM_PTR64U(p2 + size - 8);
            return (a0 == b0) & (a1 == b1);
        }
        else // 16 < size <= 32
        {
            __m128i a0 = _mm_loadu_si128((const __m128i*)p1);
            __m128i b0 = _mm_loadu_si128((const __m128i*)p2);
            __m128i a1 = _mm_loadu_si128((const __m128i*)(p1 + size - 16));
            __m128i b1 = _mm_loadu_si128((const __m128i*)(p2 + size - 16));
            __m256i a = _mm256_inserti128_si256(_mm256_castsi128_si256(a1), a0, 1);
            __m256i b = _mm256_inserti128_si256(_mm256_castsi128_si256(b1), b0, 1);
            __m256i r = _mm256_xor_si256(a, b);
            return !!_mm256_testz_si256(r, r);
        }
    }

    while (size >= 128)
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p1 + 0x00));
        __m256i b0 = _mm256_loadu_si256((const __m256i*)(p2 + 0x00));
        __m256i a1 = _mm256_loadu_si256((const __m256i*)(p1 + 0x20));
        __m256i b1 = _mm256_loadu_si256((const __m256i*)(p2 + 0x20));
        __m256i a2 = _mm256_loadu_si256((const __m256i*)(p1 + 0x40));
        __m256i b2 = _mm256_loadu_si256((const __m256i*)(p2 + 0x40));
        __m256i a3 = _mm256_loadu_si256((const __m256i*)(p1 + 0x60));
        __m256i b3 = _mm256_loadu_si256((const __m256i*)(p2 + 0x60));
        __m256i r0 = _mm256_xor_si256(a0, b0);
        __m256i r1 = _mm256_xor_si256(a1, b1);
        __m256i r2 = _mm256_xor_si256(a2, b2);
        __m256i r3 = _mm256_xor_si256(a3, b3);
        __m256i r = _mm256_or_si256(_mm256_or_si256(r0, r1), _mm256_or_si256(r2, r3));

        if (!_mm256_testz_si256(r, r))
        {
            return false;
        }

        size -= 128;
        p1 += 128;
        p2 += 128;
    }

    if (size & 64) // 64 <= size < 128
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)p1);
        __m256i b0 = _mm256_loadu_si256((const __m256i*)p2);
        __m256i a1 = _mm256_loadu_si256((const __m256i*)(p1 + 0x20));
        __m256i b1 = _mm256_loadu_si256((const __m256i*)(p2 + 0x20));
        __m256i r0 = _mm256_xor_si256(a0, b0);
        __m256i r1 = _mm256_xor_si256(a1, b1);
        __m256i r = _mm256_or_si256(r0, r1);

        if (!_mm256_testz_si256(r, r))
        {
            return false;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)p1);
        __m256i b0 = _mm256_loadu_si256((const __m256i*)p2);
        __m256i r0 = _mm256_xor_si256(a0, b0);

        if (!_mm256_testz_si256(r0, r0))
        {
            return false;
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size) // size < 32
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p1 + size - 32));
        __m256i b0 = _mm256_loadu_si256((const __m256i*)(p2 + size - 32));
        __m256i r0 = _mm256_xor_si256(a0, b0);

        if (!_mm256_testz_si256(r0, r0))
        {
            return false;
        }
    }

    return true;
}

MEM_DISABLE_ASAN
MEM_TARGET_AVX2
size_t MemFind_avx2(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;
    __m256i value32 = _mm256_set1_epi8((char)value);

    if (size == 0)
    {
        return 0;
    }

    if (size <= 32)
    {
        size_t address = (uint32_t)(uintptr_t)p % 32;
        size_t extra = (address + size) <= 32 ? address : 0;

        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p - extra));
        __m256i r0 = _mm256_cmpeq_epi8(value32, a0);

        uint32_t mask = (uint32_t)_mm256_movemask_epi8(r0);
        mask = _bzhi_u32(MEM_SHRX_32(mask, (uint32_t)extra), (uint32_t)size);

        return mask ? _tzcnt_u32(mask) : size;
    }

    size_t offset = 0;
    while (size >= 128)
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p + 0x00));
        __m256i a1 = _mm256_loadu_si256((const __m256i*)(p + 0x20));
        __m256i a2 = _mm256_loadu_si256((const __m256i*)(p + 0x40));
        __m256i a3 = _mm256_loadu_si256((const __m256i*)(p + 0x60));
        __m256i r0 = _mm256_cmpeq_epi8(value32, a0);
        __m256i r1 = _mm256_cmpeq_epi8(value32, a1);
        __m256i r2 = _mm256_cmpeq_epi8(value32, a2);
        __m256i r3 = _mm256_cmpeq_epi8(value32, a3);
        __m256i r = _mm256_or_si256(_mm256_or_si256(r0, r1), _mm256_or_si256(r2, r3));

        uint32_t mask = (uint32_t)_mm256_movemask_epi8(r);
        if (mask)
        {
            uint64_t m0 = (uint32_t)_mm256_movemask_epi8(r0);
            uint64_t m1 = (uint32_t)_mm256_movemask_epi8(r1);
            uint64_t m2 = (uint32_t)_mm256_movemask_epi8(r2);
            uint64_t m3 = mask; // if r0=r1=r2=0, then r3=r

            uint64_t m01 = m0 | (m1 << 32);
            uint64_t m23 = m2 | (m3 << 32);

            size_t idx0 = _tzcnt_u64(m01);
            size_t idx1 = _tzcnt_u64(m23);

            offset += idx0;
            offset += m01 ? 0 : idx1;
            return offset;
        }

        offset += 128;
        size -= 128;
        p += 128;
    }

    if (size & 64) // 64 <= size < 128
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)p);
        __m256i r0 = _mm256_cmpeq_epi8(value32, a0);

        uint32_t mask = (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask);
            return offset + index;
        }

        __m256i a1 = _mm256_loadu_si256((const __m256i*)(p + 0x20));
        __m256i r1 = _mm256_cmpeq_epi8(value32, a1);

        mask = (uint32_t)_mm256_movemask_epi8(r1);
        if (mask)
        {
            size_t index = 0x20 + _tzcnt_u32(mask);
            return offset + index;
        }

        offset += 64;
        size -= 64;
        p += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)p);
        __m256i r0 = _mm256_cmpeq_epi8(value32, a0);

        uint32_t mask = (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask);
            return offset + index;
        }

        offset += 32;
        size -= 32;
        p += 32;
    }

    if (size) // size < 32
    {
        __m256i a0 = _mm256_loadu_si256((const __m256i*)(p + size - 32));
        __m256i r0 = _mm256_cmpeq_epi8(value32, a0);

        uint32_t mask = (uint32_t)_mm256_movemask_epi8(r0);
        if (mask)
        {
            size_t index = _tzcnt_u32(mask) + size - 32;
            return offset + index;
        }

        offset += size;
    }

    return offset;
}

MEM_TARGET_AVX512
int MemCompare_avx512(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    size_t extra = size & 63;
    if (extra)
    {
        __mmask64 mask = _cvtu64_mask64(_bzhi_u64(~0ULL, (uint32_t)extra));

        __m512i a = _mm512_maskz_loadu_epi8(mask, p1);
        __m512i b = _mm512_maskz_loadu_epi8(mask, p2);

        __mmask64 m = _mm512_cmpneq_epu8_mask(a, b);
        if (!_kortestz_mask64_u8(m, m))
        {
            int r1 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a, b)));
            int r2 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b, a)));
            return r1 - r2;
        }

        size -= extra;
        p1 += extra;
        p2 += extra;
    }

    if (size & 64)
    {
        __m512i a = _mm512_loadu_epi8(p1);
        __m512i b = _mm512_loadu_epi8(p2);

        __mmask64 m = _mm512_cmpneq_epu8_mask(a, b);
        if (!_kortestz_mask64_u8(m, m))
        {
            int r1 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a, b)));
            int r2 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b, a)));
            return r1 - r2;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    while (size)
    {
        __m512i a0 = _mm512_loadu_epi8(p1 + 0x00);
        __m512i b0 = _mm512_loadu_epi8(p2 + 0x00);
        __m512i a1 = _mm512_loadu_epi8(p1 + 0x40);
        __m512i b1 = _mm512_loadu_epi8(p2 + 0x40);

        __mmask64 m0 = _mm512_cmpneq_epu8_mask(a0, b0);
        __mmask64 m1 = _mm512_cmpneq_epu8_mask(a1, b1);
        if (!_kortestz_mask64_u8(m0, m1))
        {
            int r10 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a0, b0)));
            int r20 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b0, a0)));
            int r11 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a1, b1)));
            int r21 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b1, a1)));

            int r1 = (r10 == r20) ? r11 : r10;
            int r2 = (r10 == r20) ? r21 : r20;
            return r1 - r2;
        }

        size -= 128;
        p1 += 128;
        p2 += 128;
    }

    return 0;
}

MEM_TARGET_AVX512
int MemCompareI_avx512(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    size_t extra = size & 63;
    if (extra)
    {
        __mmask64 mask = _cvtu64_mask64(_bzhi_u64(~0ULL, (uint32_t)extra));

        __m512i a = MemToLower64(_mm512_maskz_loadu_epi8(mask, p1));
        __m512i b = MemToLower64(_mm512_maskz_loadu_epi8(mask, p2));

        __mmask64 m = _mm512_cmpneq_epu8_mask(a, b);
        if (!_kortestz_mask64_u8(m, m))
        {
            int r1 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a, b)));
            int r2 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b, a)));
            return r1 - r2;
        }

        size -= extra;
        p1 += extra;
        p2 += extra;
    }

    if (size & 64)
    {
        __m512i a = MemToLower64(_mm512_loadu_epi8(p1));
        __m512i b = MemToLower64(_mm512_loadu_epi8(p2));

        __mmask64 m = _mm512_cmpneq_epu8_mask(a, b);
        if (!_kortestz_mask64_u8(m, m))
        {
            int r1 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a, b)));
            int r2 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b, a)));
            return r1 - r2;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    while (size)
    {
        __m512i a0 = MemToLower64(_mm512_loadu_epi8(p1 + 0x00));
        __m512i b0 = MemToLower64(_mm512_loadu_epi8(p2 + 0x00));
        __m512i a1 = MemToLower64(_mm512_loadu_epi8(p1 + 0x40));
        __m512i b1 = MemToLower64(_mm512_loadu_epi8(p2 + 0x40));

        __mmask64 m0 = _mm512_cmpneq_epu8_mask(a0, b0);
        __mmask64 m1 = _mm512_cmpneq_epu8_mask(a1, b1);
        if (!_kortestz_mask64_u8(m0, m1))
        {
            int r10 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a0, b0)));
            int r20 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b0, a0)));
            int r11 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(a1, b1)));
            int r21 = (int)_tzcnt_u64(_cvtmask64_u64(_mm512_cmplt_epu8_mask(b1, a1)));

            int r1 = (r10 == r20) ? r11 : r10;
            int r2 = (r10 == r20) ? r21 : r20;
            return r1 - r2;
        }

        size -= 128;
        p1 += 128;
        p2 += 128;
    }

    return 0;
}

MEM_TARGET_AVX512
bool MemIsEqual_avx512(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    size_t extra = size & 63;
    if (extra)
    {
        __mmask64 mask = _cvtu64_mask64(_bzhi_u64(~0ULL, (uint32_t)extra));

        __m512i a = _mm512_maskz_loadu_epi8(mask, p1);
        __m512i b = _mm512_maskz_loadu_epi8(mask, p2);

        __mmask64 m = _mm512_cmpneq_epu8_mask(a, b);
        if (!_kortestz_mask64_u8(m, m))
        {
            return false;
        }

        size -= extra;
        p1 += extra;
        p2 += extra;
    }

    if (size & 64)
    {
        __m512i a = _mm512_loadu_epi8(p1);
        __m512i b = _mm512_loadu_epi8(p2);

        __mmask64 m = _mm512_cmpneq_epu8_mask(a, b);
        if (!_kortestz_mask64_u8(m, m))
        {
            return false;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    while (size)
    {
        __m512i a0 = _mm512_loadu_epi8(p1 + 0x00);
        __m512i b0 = _mm512_loadu_epi8(p2 + 0x00);
        __m512i a1 = _mm512_loadu_epi8(p1 + 0x40);
        __m512i b1 = _mm512_loadu_epi8(p2 + 0x40);

        __mmask64 m0 = _mm512_cmpneq_epu8_mask(a0, b0);
        __mmask64 m1 = _mm512_cmpneq_epu8_mask(a1, b1);

        if (!_kortestz_mask64_u8(m0, m1))
        {
            return false;
        }

        size -= 128;
        p1 += 128;
        p2 += 128;
    }

    return true;
}

MEM_TARGET_AVX512
size_t MemFind_avx512(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;
    const __m512i value64 = _mm512_set1_epi8((char)value);

    size_t offset = 0;

    size_t extra = size & 63;
    if (extra)
    {
        __mmask64 mask = _cvtu64_mask64(_bzhi_u64(~0ULL, (uint32_t)extra));

        __m512i a = _mm512_mask_loadu_epi8(_mm512_set1_epi8((char)~value), mask, p);

        __mmask64 m = _mm512_cmpeq_epu8_mask(value64, a);
        if (!_kortestz_mask64_u8(m, m))
        {
            return (size_t)_tzcnt_u64(_cvtmask64_u64(m));
        }

        offset += extra;
        size -= extra;
        p += extra;
    }

    if (size & 64)
    {
        __m512i a = _mm512_loadu_epi8(p);

        __mmask64 m = _mm512_cmpeq_epu8_mask(value64, a);
        if (!_kortestz_mask64_u8(m, m))
        {
            return offset + (size_t)_tzcnt_u64(_cvtmask64_u64(m));
        }

        offset += 64;
        size -= 64;
        p += 64;
    }

    while (size)
    {
        __m512i a0 = _mm512_loadu_epi8(p + 0x00);
        __m512i a1 = _mm512_loadu_epi8(p + 0x40);

        __mmask64 m0 = _mm512_cmpeq_epu8_mask(value64, a0);
        __mmask64 m1 = _mm512_cmpeq_epu8_mask(value64, a1);

        if (!_kortestz_mask64_u8(m0, m1))
        {
            size_t r0 = _tzcnt_u64(_cvtmask64_u64(m0));
            size_t r1 = _tzcnt_u64(_cvtmask64_u64(m1));

            offset += r0;
            offset += r0 == 64 ? r1 : 0;
            return offset;
        }

        offset += 128;
        size -= 128;
        p += 128;
    }

    return offset;
}

#endif


#if MEM_ARCH_ARM64

static inline uint8x16_t MemToLower16(uint8x16_t x)
{
    uint8x16_t tmp = vsubq_u8(x, vdupq_n_u8('A'));
    tmp = vcleq_u8(tmp, vdupq_n_u8('Z' - 'A'));
    tmp = vandq_u8(tmp, vdupq_n_u8('a' - 'A'));
    return vaddq_u8(x, tmp);
}

MEM_DISABLE_ASAN
int MemCompare_arm64(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return 0;
    }

    if (size <= 16)
    {
        if (size < 2) // size == 1
        {
            return p1[0] - p2[0];
        }

        uint64_t a0, b0, a1, b1;
        if (size < 4) // 2 <= size < 4
        {
            a0 = MEM_PTR16U(p1);
            b0 = MEM_PTR16U(p2);
            a1 = MEM_PTR16U(p1 + size - 2);
            b1 = MEM_PTR16U(p2 + size - 2);
        }
        else if (size < 8) // 4 <= size < 8
        {
            a0 = MEM_PTR32U(p1);
            b0 = MEM_PTR32U(p2);
            a1 = MEM_PTR32U(p1 + size - 4);
            b1 = MEM_PTR32U(p2 + size - 4);
        }
        else // 8 <= size <= 16
        {
            a0 = MEM_PTR64U(p1);
            b0 = MEM_PTR64U(p2);
            a1 = MEM_PTR64U(p1 + size - 8);
            b1 = MEM_PTR64U(p2 + size - 8);
        }

        uint64_t a = MEM_BSWAP64(a0 != b0 ? a0 : a1);
        uint64_t b = MEM_BSWAP64(a0 != b0 ? b0 : b1);
        return (a > b) - (a < b);
    }

    uint8x16_t va = vdupq_n_u8(0);
    uint8x16_t vb = vdupq_n_u8(0);

    while (size >= 64)
    {
        uint8x16x4_t a = vld1q_u8_x4(p1);
        uint8x16x4_t b = vld1q_u8_x4(p2);

        uint8x16_t c0 = vceqq_u8(a.val[0], b.val[0]);
        uint8x16_t c1 = vceqq_u8(a.val[1], b.val[1]);
        uint8x16_t c2 = vceqq_u8(a.val[2], b.val[2]);
        uint8x16_t c3 = vceqq_u8(a.val[3], b.val[3]);
        uint8x16_t c = vandq_u8(vandq_u8(c0, c1), vandq_u8(c2, c3));

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            uint8x16_t vm;
            va = a.val[3];
            vb = b.val[3];

            vm = vdupq_n_u8(vminvq_u8(c2));
            va = vbslq_u8(vm, va, a.val[2]);
            vb = vbslq_u8(vm, vb, b.val[2]);

            vm = vdupq_n_u8(vminvq_u8(c1));
            va = vbslq_u8(vm, va, a.val[1]);
            vb = vbslq_u8(vm, vb, b.val[1]);

            vm = vdupq_n_u8(vminvq_u8(c0));
            va = vbslq_u8(vm, va, a.val[0]);
            vb = vbslq_u8(vm, vb, b.val[0]);

            goto done;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        uint8x16x2_t a = vld1q_u8_x2(p1);
        uint8x16x2_t b = vld1q_u8_x2(p2);

        uint8x16_t c0 = vceqq_u8(a.val[0], b.val[0]);
        uint8x16_t c1 = vceqq_u8(a.val[1], b.val[1]);

        uint8x8_t mask0 = vshrn_n_u16(vreinterpretq_u16_u8(c0), 4);
        uint64_t nibbles0 = vget_lane_u64(vreinterpret_u64_u8(mask0), 0);
        if (nibbles0 + 1)
        {
            va = a.val[0];
            vb = b.val[0];
            goto done;
        }

        uint8x8_t mask1 = vshrn_n_u16(vreinterpretq_u16_u8(c1), 4);
        uint64_t nibbles1 = vget_lane_u64(vreinterpret_u64_u8(mask1), 0);
        if (nibbles1 + 1)
        {
            va = a.val[1];
            vb = b.val[1];
            goto done;
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        va = vld1q_u8(p1);
        vb = vld1q_u8(p2);

        uint8x16_t c = vceqq_u8(va, vb);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            goto done;
        }

        size -= 16;
        p1 += 16;
        p2 += 16;
    }

    if (size) // size < 16
    {
        va = vld1q_u8(p1 + size - 16);
        vb = vld1q_u8(p2 + size - 16);
    }

done:;
    uint64x2_t a64 = vreinterpretq_u64_u8(vrev64q_u8(va));
    uint64x2_t b64 = vreinterpretq_u64_u8(vrev64q_u8(vb));
    uint64_t a0 = vgetq_lane_u64(a64, 0);
    uint64_t b0 = vgetq_lane_u64(b64, 0);
    uint64_t a1 = vgetq_lane_u64(a64, 1);
    uint64_t b1 = vgetq_lane_u64(b64, 1);
    uint64_t a = (a0 != b0 ? a0 : a1);
    uint64_t b = (a0 != b0 ? b0 : b1);
    return (a > b) - (a < b);
}

MEM_DISABLE_ASAN
int MemCompareI_arm64(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return 0;
    }

    if (size <= 16)
    {
        if (size < 2) // size == 1
        {
            return MemToLower1(p1[0]) - MemToLower1(p2[0]);
        }

        if (size < 4) // 2 <= size < 4
        {
            uint64_t a0 = MEM_PTR16U(p1);
            uint64_t b0 = MEM_PTR16U(p2);
            uint64_t a1 = MEM_PTR16U(p1 + size - 2);
            uint64_t b1 = MEM_PTR16U(p2 + size - 2);
            uint64_t tmp = MEM_BSWAP64(MemToLower8(a0 | (a1 << 16) | (b0 << 32) | (b1 << 48)));
            uint32_t a = (uint32_t)(tmp >> 32);
            uint32_t b = (uint32_t)tmp;
            return (a > b) - (a < b);
        }
        else if (size < 8) // 4 <= size < 8
        {
            uint64_t a0 = MEM_PTR32U(p1);
            uint64_t b0 = MEM_PTR32U(p2);
            uint64_t a1 = MEM_PTR32U(p1 + size - 4);
            uint64_t b1 = MEM_PTR32U(p2 + size - 4);
            uint64_t a = MEM_BSWAP64(MemToLower8(a0 | (a1 << 32)));
            uint64_t b = MEM_BSWAP64(MemToLower8(b0 | (b1 << 32)));
            return (a > b) - (a < b);
        }
        else // 8 <= size <= 16
        {
            uint8x16_t va = vcombine_u8(vld1_u8(p1), vld1_u8(p1 + size - 8));
            uint8x16_t vb = vcombine_u8(vld1_u8(p2), vld1_u8(p2 + size - 8));
            uint64x2_t a64 = vreinterpretq_u64_u8(vrev64q_u8(MemToLower16(va)));
            uint64x2_t b64 = vreinterpretq_u64_u8(vrev64q_u8(MemToLower16(vb)));
            uint64_t a0 = vgetq_lane_u64(a64, 0);
            uint64_t b0 = vgetq_lane_u64(b64, 0);
            uint64_t a1 = vgetq_lane_u64(a64, 1);
            uint64_t b1 = vgetq_lane_u64(b64, 1);
            uint64_t a = (a0 != b0 ? a0 : a1);
            uint64_t b = (a0 != b0 ? b0 : b1);
            return (a > b) - (a < b);
        }
    }

    uint8x16_t va = vdupq_n_u8(0);
    uint8x16_t vb = vdupq_n_u8(0);

    while (size >= 64)
    {
        uint8x16x4_t a = vld1q_u8_x4(p1);
        uint8x16x4_t b = vld1q_u8_x4(p2);

        a.val[0] = MemToLower16(a.val[0]);
        b.val[0] = MemToLower16(b.val[0]);
        a.val[1] = MemToLower16(a.val[1]);
        b.val[1] = MemToLower16(b.val[1]);
        a.val[2] = MemToLower16(a.val[2]);
        b.val[2] = MemToLower16(b.val[2]);
        a.val[3] = MemToLower16(a.val[3]);
        b.val[3] = MemToLower16(b.val[3]);

        uint8x16_t c0 = vceqq_u8(a.val[0], b.val[0]);
        uint8x16_t c1 = vceqq_u8(a.val[1], b.val[1]);
        uint8x16_t c2 = vceqq_u8(a.val[2], b.val[2]);
        uint8x16_t c3 = vceqq_u8(a.val[3], b.val[3]);
        uint8x16_t c = vandq_u8(vandq_u8(c0, c1), vandq_u8(c2, c3));

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            uint8x16_t vm;
            va = a.val[3];
            vb = b.val[3];

            vm = vdupq_n_u8(vminvq_u8(c2));
            va = vbslq_u8(vm, va, a.val[2]);
            vb = vbslq_u8(vm, vb, b.val[2]);

            vm = vdupq_n_u8(vminvq_u8(c1));
            va = vbslq_u8(vm, va, a.val[1]);
            vb = vbslq_u8(vm, vb, b.val[1]);

            vm = vdupq_n_u8(vminvq_u8(c0));
            va = vbslq_u8(vm, va, a.val[0]);
            vb = vbslq_u8(vm, vb, b.val[0]);

            goto done;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        uint8x16x2_t a = vld1q_u8_x2(p1);
        uint8x16x2_t b = vld1q_u8_x2(p2);

        a.val[0] = MemToLower16(a.val[0]);
        b.val[0] = MemToLower16(b.val[0]);
        a.val[1] = MemToLower16(a.val[1]);
        b.val[1] = MemToLower16(b.val[1]);

        uint8x16_t c0 = vceqq_u8(a.val[0], b.val[0]);
        uint8x16_t c1 = vceqq_u8(a.val[1], b.val[1]);

        uint8x8_t mask0 = vshrn_n_u16(vreinterpretq_u16_u8(c0), 4);
        uint64_t nibbles0 = vget_lane_u64(vreinterpret_u64_u8(mask0), 0);
        if (nibbles0 + 1)
        {
            va = a.val[0];
            vb = b.val[0];
            goto done;
        }

        uint8x8_t mask1 = vshrn_n_u16(vreinterpretq_u16_u8(c1), 4);
        uint64_t nibbles1 = vget_lane_u64(vreinterpret_u64_u8(mask1), 0);
        if (nibbles1 + 1)
        {
            va = a.val[1];
            vb = b.val[1];
            goto done;
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        va = MemToLower16(vld1q_u8(p1));
        vb = MemToLower16(vld1q_u8(p2));

        uint8x16_t c = vceqq_u8(va, vb);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            goto done;
        }

        size -= 16;
        p1 += 16;
        p2 += 16;
    }

    if (size) // size < 16
    {
        va = MemToLower16(vld1q_u8(p1 + size - 16));
        vb = MemToLower16(vld1q_u8(p2 + size - 16));
    }

done:;
    uint64x2_t a64 = vreinterpretq_u64_u8(vrev64q_u8(va));
    uint64x2_t b64 = vreinterpretq_u64_u8(vrev64q_u8(vb));
    uint64_t a0 = vgetq_lane_u64(a64, 0);
    uint64_t b0 = vgetq_lane_u64(b64, 0);
    uint64_t a1 = vgetq_lane_u64(a64, 1);
    uint64_t b1 = vgetq_lane_u64(b64, 1);
    uint64_t a = (a0 != b0 ? a0 : a1);
    uint64_t b = (a0 != b0 ? b0 : b1);
    return (a > b) - (a < b);
}

MEM_DISABLE_ASAN
bool MemIsEqual_arm64(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    if (size == 0)
    {
        return true;
    }

    if (size <= 16)
    {
        if (size < 2) // size == 1
        {
            return p1[0] == p2[0];
        }

        uint64_t a0, b0, a1, b1;
        if (size < 4) // 2 <= size < 4
        {
            a0 = MEM_PTR16U(p1);
            b0 = MEM_PTR16U(p2);
            a1 = MEM_PTR16U(p1 + size - 2);
            b1 = MEM_PTR16U(p2 + size - 2);
        }
        else if (size < 8) // 4 <= size < 8
        {
            a0 = MEM_PTR32U(p1);
            b0 = MEM_PTR32U(p2);
            a1 = MEM_PTR32U(p1 + size - 4);
            b1 = MEM_PTR32U(p2 + size - 4);
        }
        else // 8 <= size <= 16
        {
            a0 = MEM_PTR64U(p1);
            b0 = MEM_PTR64U(p2);
            a1 = MEM_PTR64U(p1 + size - 8);
            b1 = MEM_PTR64U(p2 + size - 8);
        }

        return (a0 == b0) & (a1 == b1);
    }

    while (size >= 64)
    {
        uint8x16x4_t a = vld1q_u8_x4(p1);
        uint8x16x4_t b = vld1q_u8_x4(p2);

        uint8x16_t c0 = vceqq_u8(a.val[0], b.val[0]);
        uint8x16_t c1 = vceqq_u8(a.val[1], b.val[1]);
        uint8x16_t c2 = vceqq_u8(a.val[2], b.val[2]);
        uint8x16_t c3 = vceqq_u8(a.val[3], b.val[3]);
        uint8x16_t c = vandq_u8(vandq_u8(c0, c1), vandq_u8(c2, c3));

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            return false;
        }

        size -= 64;
        p1 += 64;
        p2 += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        uint8x16x2_t a = vld1q_u8_x2(p1);
        uint8x16x2_t b = vld1q_u8_x2(p2);

        uint8x16_t c0 = vceqq_u8(a.val[0], b.val[0]);
        uint8x16_t c1 = vceqq_u8(a.val[1], b.val[1]);
        uint8x16_t c = vandq_u8(c0, c1);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            return false;
        }

        size -= 32;
        p1 += 32;
        p2 += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        uint8x16_t a = vld1q_u8(p1);
        uint8x16_t b = vld1q_u8(p2);
        uint8x16_t c = vceqq_u8(a, b);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            return false;
        }

        size -= 16;
        p1 += 16;
        p2 += 16;
    }

    if (size) // size < 16
    {
        uint8x16_t a = vld1q_u8(p1 + size - 16);
        uint8x16_t b = vld1q_u8(p2 + size - 16);
        uint8x16_t c = vceqq_u8(a, b);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(c), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles + 1)
        {
            return false;
        }
    }

    return true;
}

MEM_DISABLE_ASAN
size_t MemFind_arm64(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;

    const uint8x16_t index4 = vreinterpretq_u8_u64(vdupq_n_u64(0x8040201008040201));
    const uint8x16_t value16 = vdupq_n_u8(value);

    if (size == 0)
    {
        return 0;
    }

    if (size <= 16)
    {
        size_t address = (uint32_t)(uintptr_t)p % 16;
        size_t extra = (address + size) <= 16 ? address : 0;

        uint8x16_t a = vld1q_u8(p - extra);
        uint8x16_t b = vceqq_u8(a, value16);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(b), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles)
        {
            size_t index = MEM_CTZ64(nibbles >> (4*extra)) / 4;
            return index < size ? index : size;
        }

        return size;
    }

    size_t offset = 0;
    while (size >= 64)
    {
        uint8x16x4_t a = vld1q_u8_x4(p);
        uint8x16_t b0 = vceqq_u8(a.val[0], value16);
        uint8x16_t b1 = vceqq_u8(a.val[1], value16);
        uint8x16_t b2 = vceqq_u8(a.val[2], value16);
        uint8x16_t b3 = vceqq_u8(a.val[3], value16);
        uint8x16_t b = vorrq_u8(vorrq_u8(b0, b1), vorrq_u8(b2, b3));

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(b), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles)
        {
            uint8x16_t mask0 = vandq_u8(b0, index4);
            uint8x16_t mask1 = vandq_u8(b1, index4);
            uint8x16_t mask2 = vandq_u8(b2, index4);
            uint8x16_t mask3 = vandq_u8(b3, index4);

            uint8x16_t sum = vpaddq_u8(vpaddq_u8(mask0, mask1), vpaddq_u8(mask2, mask3));
            uint64_t sum64 = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(sum, sum)), 0);
            return offset + MEM_CTZ64(sum64);
        }

        offset += 64;
        size -= 64;
        p += 64;
    }

    if (size & 32) // 32 <= size < 64
    {
        uint8x16x2_t a = vld1q_u8_x2(p);
        uint8x16_t b0 = vceqq_u8(a.val[0], value16);
        uint8x16_t b1 = vceqq_u8(a.val[1], value16);

        uint8x16_t mask0 = vandq_u8(b0, index4);
        uint8x16_t mask1 = vandq_u8(b1, index4);

        uint8x16_t sum1 = vpaddq_u8(mask0, mask1);
        uint8x16_t sum2 = vpaddq_u8(sum1, sum1);
        uint32_t sum32 = vgetq_lane_u32(vreinterpretq_u32_u8(vpaddq_u8(sum2, sum2)), 0);
        if (sum32)
        {
            return offset + MEM_CTZ32(sum32);
        }

        offset += 32;
        size -= 32;
        p += 32;
    }

    if (size & 16) // 16 <= size < 32
    {
        uint8x16_t a = vld1q_u8(p);
        uint8x16_t b = vceqq_u8(a, value16);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(b), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles)
        {
            return offset + MEM_CTZ64(nibbles) / 4;
        }

        offset += 16;
        size -= 16;
        p += 16;
    }

    if (size) // size < 16
    {
        uint8x16_t a = vld1q_u8(p + size - 16);
        uint8x16_t b = vceqq_u8(a, value16);

        uint8x8_t mask = vshrn_n_u16(vreinterpretq_u16_u8(b), 4);
        uint64_t nibbles = vget_lane_u64(vreinterpret_u64_u8(mask), 0);
        if (nibbles)
        {
            return offset + MEM_CTZ64(nibbles) / 4 + size - 16;
        }
        offset += size;
    }

    return offset;
}

#endif // MEM_ARCH_ARM64


#if MEM_ARCH_RVV

int MemCompare_rvv(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    do
    {
        size_t vl = __riscv_vsetvl_e8m8(size);

        vuint8m8_t a = __riscv_vle8_v_u8m8(p1, vl);
        vuint8m8_t b = __riscv_vle8_v_u8m8(p2, vl);
        vbool1_t m = __riscv_vmsne_vv_u8m8_b1(a, b, vl);

        long index = __riscv_vfirst_m_b1(m, vl);
        if (index >= 0)
        {
            return p1[index] - p2[index];
        }

        size -= vl;
        p1 += vl;
        p2 += vl;
    }
    while (size);

    return 0;
}

int MemCompareI_rvv(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    do
    {
        size_t vl = __riscv_vsetvl_e8m8(size);

        vuint8m8_t a = __riscv_vle8_v_u8m8(p1, vl);
        vuint8m8_t b = __riscv_vle8_v_u8m8(p2, vl);

        // mask = (uint8_t)(x - 'A') <= ('Z' - 'A')
        vbool1_t am = __riscv_vmsleu_vx_u8m8_b1(__riscv_vsub_vx_u8m8(a, 'A', vl), 'Z' - 'A', vl);
        vbool1_t bm = __riscv_vmsleu_vx_u8m8_b1(__riscv_vsub_vx_u8m8(b, 'A', vl), 'Z' - 'A', vl);

        // x = mask ? (x + 'a' - 'A') : x
        a = __riscv_vadd_vx_u8m8_mu(am, a, a, 'a' - 'A', vl);
        b = __riscv_vadd_vx_u8m8_mu(bm, b, b, 'a' - 'A', vl);

        vbool1_t m = __riscv_vmsne_vv_u8m8_b1(a, b, vl);

        long index = __riscv_vfirst_m_b1(m, vl);
        if (index >= 0)
        {
            a = __riscv_vslidedown_vx_u8m8(a, (unsigned long)index, vl);
            b = __riscv_vslidedown_vx_u8m8(b, (unsigned long)index, vl);
            return __riscv_vmv_x_s_u8m8_u8(a) - __riscv_vmv_x_s_u8m8_u8(b);
        }

        size -= vl;
        p1 += vl;
        p2 += vl;
    }
    while (size);

    return 0;
}

bool MemIsEqual_rvv(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    do
    {
        size_t vl = __riscv_vsetvl_e8m8(size);

        vuint8m8_t a = __riscv_vle8_v_u8m8(p1, vl);
        vuint8m8_t b = __riscv_vle8_v_u8m8(p2, vl);
        vbool1_t m = __riscv_vmsne_vv_u8m8_b1(a, b, vl);

        if (__riscv_vcpop_m_b1(m, vl))
        {
            return false;
        }

        size -= vl;
        p1 += vl;
        p2 += vl;
    }
    while (size);

    return true;
}

size_t MemFind_rvv(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;

    size_t offset = 0;
    do
    {
        size_t vl = __riscv_vsetvl_e8m8(size);

        vuint8m8_t a = __riscv_vle8_v_u8m8(p, vl);
        vbool1_t m = __riscv_vmseq_vx_u8m8_b1(a, value, vl);

        long index = __riscv_vfirst_m_b1(m, vl);
        if (index >= 0)
        {
            return offset + (unsigned long)index;
        }

        offset += vl;
        size -= vl;
        p += vl;
    }
    while (size);

    return offset;
}

#endif // MEM_ARCH_RVV


#if MEM_ARCH_X64

#define MEM_CPUID_INIT   (1 << 0)
#define MEM_CPUID_AVX2   (1 << 1)
#define MEM_CPUID_AVX512 (1 << 2)

MEM_TARGET_XSAVE
static int MemDoCPUID(void)
{
    int info[4];

    MEM_CPUID(1, info);
    int movbe = info[2] & (1 << 22);
    int xsave = info[2] & (1 << 26);

    MEM_CPUID2(7, 0, info);
    int bmi1 = info[1] & (1 << 3);
    int avx2 = info[1] & (1 << 5);
    int bmi2 = info[1] & (1 << 8);

    int avx512f    = info[1] & (1 << 16);
    int avx512bw   = info[1] & (1 << 30);
    int avx512vbmi = info[2] & (1 << 1);

    uint64_t xcr0 = xsave ? MEM_XGETBV(0) : 0;
    int ymm = (xcr0 & 0x04) == 0x04;
    int zmm = (xcr0 & 0xe0) == 0xe0;

    int cpuid = 0;
    cpuid |= (ymm && avx2 && bmi1 && bmi2 && movbe)                     ? MEM_CPUID_AVX2   : 0;
    cpuid |= (zmm && avx512f && avx512bw && avx512vbmi && bmi1 && bmi2) ? MEM_CPUID_AVX512 : 0;
    return cpuid;
}

#if MEM_COMPILER_MSVC
__forceinline
#endif
static int MemCPUID(void)
{
    int result = 0;

#if (MEM_COMPILER_CLANG || MEM_COMPILER_GCC) && defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512VBMI__) && defined(__BMI__) && defined(__BMI2__)
    result |= MEM_CPUID_AVX512;
#endif
#if MEM_COMPILER_MSVC && defined(__AVX512F__) && defined(__AVX512BW__)
    result |= MEM_CPUID_AVX512;
#endif
#if (MEM_COMPILER_CLANG || MEM_COMPILER_GCC) && defined(__AVX2__) && defined(__BMI__) && defined(__BMI2__) && defined(__MOVBE__)
    result |= MEM_CPUID_AVX2;
#endif
#if MEM_COMPILER_MSVC && defined(__AVX2__)
    result |= MEM_CPUID_AVX2;
#endif

    if (result == 0)
    {
        static int cpuid;

        result = MEM_GET32_RELAXED(&cpuid);
        if (result == 0)
        {
            result = MEM_CPUID_INIT | MemDoCPUID();
            MEM_SET32_RELAXED(&cpuid, result);
        }
    }

    return result;
}

#endif // MEM_ARCH_X64


int MemCompare(const void* ptr1, const void* ptr2, size_t size)
{
#if MEM_ARCH_X64
    int cpuid = MemCPUID();
    if (cpuid & MEM_CPUID_AVX512)
    {
        return MemCompare_avx512(ptr1, ptr2, size);
    }
    else if (cpuid & MEM_CPUID_AVX2)
    {
        return MemCompare_avx2(ptr1, ptr2, size);
    }
    return MemCompare_sse2(ptr1, ptr2, size);
#elif MEM_ARCH_ARM64
    return MemCompare_arm64(ptr1, ptr2, size);
#elif MEM_ARCH_RVV
    return MemCompare_rvv(ptr1, ptr2, size);
#else
#   error N/A
#endif
}

int MemCompareI(const void* ptr1, const void* ptr2, size_t size)
{
#if MEM_ARCH_X64
    int cpuid = MemCPUID();
    if (cpuid & MEM_CPUID_AVX512)
    {
        return MemCompareI_avx512(ptr1, ptr2, size);
    }
    else if (cpuid & MEM_CPUID_AVX2)
    {
        return MemCompareI_avx2(ptr1, ptr2, size);
    }
    return MemCompareI_sse2(ptr1, ptr2, size);
#elif MEM_ARCH_ARM64
    return MemCompareI_arm64(ptr1, ptr2, size);
#elif MEM_ARCH_RVV
    return MemCompareI_rvv(ptr1, ptr2, size);
#else
#   error N/A
#endif
}

bool MemIsEqual(const void* ptr1, const void* ptr2, size_t size)
{
#if MEM_ARCH_X64
    int cpuid = MemCPUID();
    if (cpuid & MEM_CPUID_AVX512)
    {
        return MemIsEqual_avx512(ptr1, ptr2, size);
    }
    else if (cpuid & MEM_CPUID_AVX2)
    {
        return MemIsEqual_avx2(ptr1, ptr2, size);
    }
    return MemIsEqual_sse2(ptr1, ptr2, size);
#elif MEM_ARCH_ARM64
    return MemIsEqual_arm64(ptr1, ptr2, size);
#elif MEM_ARCH_RVV
    return MemIsEqual_rvv(ptr1, ptr2, size);
#else
#   error N/A
#endif
}

size_t MemFind(const void* ptr, size_t size, uint8_t value)
{
#if MEM_ARCH_X64
    int cpuid = MemCPUID();
    if (cpuid & MEM_CPUID_AVX512)
    {
        return MemFind_avx512(ptr, size, value);
    }
    else if (cpuid & MEM_CPUID_AVX2)
    {
        return MemFind_avx2(ptr, size, value);
    }
    return MemFind_sse2(ptr, size, value);
#elif MEM_ARCH_ARM64
    return MemFind_arm64(ptr, size, value);
#elif MEM_ARCH_RVV
    return MemFind_rvv(ptr, size, value);
#else
#   error N/A
#endif
}

#endif // defined(MEM_STATIC) || defined(MEM_IMPLEMENTATION)
