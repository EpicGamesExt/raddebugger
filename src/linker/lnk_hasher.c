// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
lnk_hasher_init(LNK_Hasher *hasher, LNK_HashKind kind)
{
  hasher->kind = kind;
  switch (kind) {
  case LNK_HashKind_BLAKE3: blake3_hasher_init(&hasher->u.blake3); break;
  case LNK_HashKind_XXHash: XXH3_64bits_reset(&hasher->u.xxhash); break;
  default: InvalidPath; break;
  }
}

internal void
lnk_hasher_update(LNK_Hasher *hasher, void *data, U64 size)
{
  switch (hasher->kind) {
  case LNK_HashKind_BLAKE3: blake3_hasher_update(&hasher->u.blake3, data, size); break;
  case LNK_HashKind_XXHash: XXH3_64bits_update(&hasher->u.xxhash, data, size); break;
  default: InvalidPath; break;
  }
}

internal U64
lnk_hasher_digest(LNK_Hasher *hasher)
{
  U64 hash = 0;
  switch (hasher->kind) {
  case LNK_HashKind_BLAKE3: blake3_hasher_finalize(&hasher->u.blake3, (U8 *)&hash, sizeof(hash)); break;
  case LNK_HashKind_XXHash: hash = XXH3_64bits_digest(&hasher->u.xxhash); break;
  default: InvalidPath; break;
  }
  return hash;
}

internal String8
lnk_string_hash_kind(LNK_HashKind hash_kind)
{
#define X(NAME) case LNK_HashKind_##NAME: return str8_lit(Stringify(NAME));
  switch (hash_kind) {
    LNK_HashKind_XList
  }
#undef X
  return str8_zero();
}

internal LNK_HashKind
lnk_hash_kind_from_llvm(LLVM_GHashAlgEnum v)
{
  switch (v) {
  case LLVM_GHashAlg_SHA1:
  case LLVM_GHashAlg_SHA1_8:
    return LNK_HashKind_Null;
  case LLVM_GHashAlg_BLAKE3:
    return LNK_HashKind_BLAKE3;
  }
  return LNK_HashKind_Null;
}

