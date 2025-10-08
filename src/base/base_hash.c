// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: MD5

#include "third_party/martins_hash/md5.h"

internal MD5
md5_from_data(String8 data)
{
  md5_ctx ctx = {0};
  md5_init(&ctx);
  md5_update(&ctx, (void*)data.str, data.size);
  MD5 result = {0};
  md5_finish(&ctx, result.u8);
  return result;
}

////////////////////////////////
//~ rjf: SHA

#include "third_party/martins_hash/sha1.h"
#include "third_party/martins_hash/sha256.h"

internal SHA1
sha1_from_data(String8 data)
{
  SHA1 result = {0};
  {
    sha1_ctx ctx = {0};
    sha1_init(&ctx);
    sha1_update(&ctx, data.str, data.size);
    sha1_finish(&ctx, result.u8);
  }
  return result;
}

internal SHA256
sha256_from_data(String8 data)
{
  SHA256 result = {0};
  {
    sha256_ctx ctx = {0};
    sha256_init(&ctx);
    sha256_update(&ctx, data.str, data.size);
    sha256_finish(&ctx, result.u8);
  }
  return result;
}
