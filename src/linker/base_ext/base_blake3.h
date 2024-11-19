// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#if defined(__clang__) && defined(__x86_64__)
#  if defined(__IMMINTRIN_H)
#    error "include this header before immintrin.h / x86intrin.h / intrin.h"
#  endif
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#  pragma push_macro("__AVX__")
#  pragma push_macro("__AVX2__")
#  pragma push_macro("__SSE4_1__")
#  pragma push_macro("__AVX512F__")
#  pragma push_macro("__AVX512VL__")
#  define __AVX__ 1
#  define __AVX2__ 1
#  define __SSE4_1__ 1
#  define __AVX512F__ 1
#  define __AVX512VL__ 1
#  include <immintrin.h>
#  pragma pop_macro("__AVX512VL__")
#  pragma pop_macro("__AVX512F__")
#  pragma pop_macro("__SSE4_1__")
#  pragma pop_macro("__AVX2__")
#  pragma pop_macro("__AVX__")
#  pragma clang diagnostic pop
#endif

#include "third_party/blake3/c/blake3.h"

static void
blake3(void* out, size_t outlen, void* in, size_t inlen)
{
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, in, inlen);
  blake3_hasher_finalize(&hasher, (uint8_t*)out, outlen);
}

