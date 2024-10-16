// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal MD5Hash
md5_hash_from_string(String8 data)
{
  MD5_CTX ctx; MD5_Init(&ctx);
  MD5_Update(&ctx, (void*)data.str, safe_cast_u32(data.size));
  MD5Hash hash; MD5_Final((unsigned char*)&hash, &ctx);
  return hash;
}

