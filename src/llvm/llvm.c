// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
llvm_string_from_ghash_alg(LLVM_GHashAlg v)
{
  switch (v) {
  case LLVM_GHashAlg_SHA1:   return str8_lit("SHA1");
  case LLVM_GHashAlg_SHA1_8: return str8_lit("SHA1_8");
  case LLVM_GHashAlg_BLAKE3: return str8_lit("BLAKE3");
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

internal B32
llvm_is_bitcode(String8 data)
{
  if (data.size < 4) {
    return 0;
  }

  // raw LLVM bitcode magic
  if (data.str[0] == 'B' && data.str[1] == 'C' && data.str[2] == 0xc0 && data.str[3] == 0xde) {
    return 1;
  }

  // LLVM bitcode wrapper magic
  if (data.str[0] == 0xde && data.str[1] == 0xc0 && data.str[2] == 0x17 && data.str[3] == 0x0b) {
    return 1;
  }

  return 0;
}

