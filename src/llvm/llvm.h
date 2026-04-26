// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LLVM_H
#define LLVM_H

#define LLVM_GHash_Magic 0x133c9c5
#define LLVM_GHash_CurrentVersion 0

typedef U16 LLVM_GHashAlg;
typedef enum LLVM_GHashAlgEnum
{
  LLVM_GHashAlg_SHA1   = 0,
  LLVM_GHashAlg_SHA1_8 = 1,
  LLVM_GHashAlg_BLAKE3 = 2,
} LLVM_GHashAlgEnum;

typedef struct LLVM_GHash
{
  U32           magic;
  U16           version;
  LLVM_GHashAlg hash_alg;
  // * hashes[]
} LLVM_GHash;

internal String8 llvm_string_from_ghash_alg(LLVM_GHashAlg v);
internal U64     llvm_hash_size_from_alg(LLVM_GHashAlg v);

#endif // LLVM_H

