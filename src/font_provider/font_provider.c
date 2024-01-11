// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Type Functions

internal FP_Handle
fp_handle_zero(void)
{
  FP_Handle result = {0};
  return result;
}

internal B32
fp_handle_match(FP_Handle a, FP_Handle b)
{
  return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1]);
}
