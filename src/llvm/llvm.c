// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
llvm_string_from_ghash_alg(LLVM_GHashAlg v)
{
  switch (v) {
  case LLVM_GHashAlg_SHA1:   return str8_lit("SHA1");
  case LLVM_GHashAlg_SHA1_8: return str8_lit("SHA1_8");
  case LLVM_GHashAlg_BLAKE3: return str8_lit("BALK3");
  }
  return str8_zero();
}

internal U64
llvm_hash_size_from_alg(LLVM_GHashAlg v)
{
  switch (v) {
  case LLVM_GHashAlg_SHA1:   return 20;
  case LLVM_GHashAlg_SHA1_8: return 8;
  case LLVM_GHashAlg_BLAKE3: return 8;
  }
  return 0;
}
