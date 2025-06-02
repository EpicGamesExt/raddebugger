// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U32
pdb_hash_v1(String8 string)
{
  U32 result = 0;
  U8 *ptr = string.str;
  U8 *opl = ptr + (string.size&(~3));
  for(; ptr < opl; ptr += 4)
  {
    result ^= *(U32*)ptr;
  }
  if((string.size&2) != 0)
  {
    result ^= *(U16*)ptr; ptr += 2;
  }
  if((string.size&1) != 0)
  {
    result ^= *ptr;
  }
  result |= 0x20202020;
  result ^= (result >> 11);
  result ^= (result >> 16);
  return result;
}

