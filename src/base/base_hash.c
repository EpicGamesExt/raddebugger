// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Hash Functions

#if !defined(MD5_API)
# define MD5_API static
# include "third_party/md5/md5.c"
# include "third_party/md5/md5.h"
#endif

internal MD5
md5_from_data(String8 data)
{
  MD5_CTX ctx = {0};
  MD5_Init(&ctx);
  MD5_Update(&ctx, (void*)data.str, data.size);
  MD5 result = {0};
  MD5_Final(result.u8, &ctx);
  return result;
}

internal SHA1
sha1_from_data(String8 data)
{
  SHA1 result = {0};
  return result;
}

internal SHA256
sha256_from_data(String8 data)
{
  SHA256 result = {0};
  return result;
}
