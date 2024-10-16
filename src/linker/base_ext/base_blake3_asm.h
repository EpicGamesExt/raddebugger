// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_BLAKE3_H
#define BASE_BLAKE3_H

#include "../third_party_ext/blake3/blake3.h"

static void
blake3(void* out, size_t outlen, void* in, size_t inlen)
{
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, in, inlen);
  blake3_hasher_finalize(&hasher, (uint8_t*)out, outlen);
}

#endif // BASE_BLAKE3_H
