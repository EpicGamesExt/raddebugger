// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define BLAKE3_API static
#define BLAKE3_PRIVATE static

#include "third_party/blake3/c/blake3.h"

static void
blake3(void* out, size_t outlen, void* in, size_t inlen)
{
  blake3_hasher hasher;
  blake3_hasher_init(&hasher);
  blake3_hasher_update(&hasher, in, inlen);
  blake3_hasher_finalize(&hasher, (uint8_t*)out, outlen);
}

