// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_HASH_H
#define BASE_HASH_H

////////////////////////////////
//~ rjf: Hash Result Types

typedef union MD5 MD5;
union MD5
{
  U8 u8[16];
  U16 u16[8];
  U32 u32[4];
  U64 u64[2];
  U128 u128;
};

typedef union SHA1 SHA1;
union SHA1
{
  U8 u8[20];
};

typedef union SHA256 SHA256;
union SHA256
{
  U8 u8[32];
  U16 u16[16];
  U32 u32[8];
  U64 u64[4];
  U128 u128[2];
  U256 u256;
};

////////////////////////////////
//~ rjf: Hash Functions

internal MD5 md5_from_data(String8 data);
internal SHA1 sha1_from_data(String8 data);
internal SHA256 sha256_from_data(String8 data);

#endif // BASE_HASH_H
