// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#define LNK_HashKind_XList \
  X(Null)   \
  X(BLAKE3) \
  X(XXHash)

typedef enum
{
#define X(NAME) LNK_HashKind_##NAME,
  LNK_HashKind_XList
#undef X
} LNK_HashKind;

typedef struct LNK_Hasher
{
  LNK_HashKind kind;
  union {
    blake3_hasher blake3;
    XXH3_state_t  xxhash;
  } u;
} LNK_Hasher;

#if BUILD_DEBUG
#define LNK_HASHER_FN internal
#else
#define LNK_HASHER_FN always_inline
#endif

LNK_HASHER_FN void    lnk_hasher_init     (LNK_Hasher *hasher, LNK_HashKind kind);
LNK_HASHER_FN void    lnk_hasher_update   (LNK_Hasher *hasher, void *data, U64 size);
LNK_HASHER_FN U64     lnk_hasher_digest64 (LNK_Hasher *hasher);
LNK_HASHER_FN U128    lnk_hasher_digest128(LNK_Hasher *hasher);
LNK_HASHER_FN String8 lnk_string_hash_kind(LNK_HashKind kind);
#define lnk_hasher_update_string(hasher, string) lnk_hasher_update(hasher, (string).str, (string).size)
#define lnk_hasher_update_struct(hasher, ptr)    lnk_hasher_update(hasher, ptr, sizeof(*ptr))

// @LLVM-ext
LNK_HASHER_FN LNK_HashKind lnk_hash_kind_from_llvm(LLVM_GHashAlgEnum v);
