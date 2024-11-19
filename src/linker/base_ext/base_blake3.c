// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#elif defined(_MSC_VER)
#pragma warning (push, 0)
#endif

#include "third_party/blake3/c/blake3_portable.c"

#if defined(_M_AMD64) || defined(__x86_64__)

#define round_fn           sse2_round_fn
#define compress_pre       sse2_compress_pre

#include "third_party/blake3/c/blake3_sse2.c"

#define loadu              sse41_loadu
#define storeu             sse41_storeu
#define addv               sse41_addv
#define xorv               sse41_xorv
#define set1               sse41_set1
#define set4               sse41_set4
#define rot16              sse41_rot16
#define rot12              sse41_rot12
#define rot8               sse41_rot8
#define rot7               sse41_rot7
#define g1                 sse41_g1
#define g2                 sse41_g2
#define diagonalize        sse41_diagonalize
#define undiagonalize      sse41_undiagonalize
#define compress_pre       sse41_compress_pre
#define round_fn           sse41_round_fn
#define transpose_vecs     sse41_transpose_vecs
#define transpose_msg_vecs sse41_transpose_msg_vecs
#define load_counters      sse41_load_counters

#if defined(__clang__)
#pragma clang attribute push(__attribute__((target("sse4.1"))), apply_to=function)
#endif
#include "third_party/blake3/c/blake3_sse41.c"
#if defined(__clang__)
#pragma clang attribute pop
#endif

#define loadu              avx2_loadu
#define storeu             avx2_storeu
#define addv               avx2_addv
#define xorv               avx2_xorv
#define set1               avx2_set1
#define rot7               avx2_rot7
#define rot8               avx2_rot8
#define rot12              avx2_rot12
#define rot16              avx2_rot16
#define round_fn           avx2_round_fn
#define transpose_vecs     avx2_transpose_vecs
#define transpose_msg_vecs avx2_transpose_msg_vecs
#define load_counters      avx2_load_counters

#if defined(__clang__)
#pragma clang attribute push(__attribute__((target("avx2"))), apply_to=function)
#endif
#include "third_party/blake3/c/blake3_avx2.c"
#if defined(__clang__)
#pragma clang attribute pop
#endif

#define set4               avx512_set4
#define g1                 avx512_g1
#define g2                 avx512_g2
#define diagonalize        avx512_diagonalize
#define undiagonalize      avx512_undiagonalize
#define compress_pre       avx512_compress_pre
#define transpose_vecs     avx512_transpose_vecs
#define transpose_msg_vecs avx512_transpose_msg_vecs
#define load_counters      avx512_load_counters

#if defined(__clang__)
#pragma clang attribute push(__attribute__((target("avx512f,avx512vl"))), apply_to=function)
#endif
#include "third_party/blake3/c/blake3_avx512.c"
#if defined(__clang__)
#pragma clang attribute pop
#endif

#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#include "third_party/blake3/c/blake3_neon.c"
#endif

#include "third_party/blake3/c/blake3_dispatch.c"
#include "third_party/blake3/c/blake3.c"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning (pop, 0)
#endif

