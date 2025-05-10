// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Backend Implementations

fp_hook void
fp_init(void)
{
  
}

fp_hook FP_Handle
fp_font_open(String8 path)
{
  FP_Handle f = {0};
  return f;
}

fp_hook FP_Handle
fp_font_open_from_static_data_string(String8 *data_ptr)
{
  FP_Handle f = {0};
  return f;
}

fp_hook void
fp_font_close(FP_Handle handle)
{
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle font)
{
  FP_Metrics m = {0};
  return m;
}

fp_hook NO_ASAN FP_RasterResult
fp_raster(Arena *arena, FP_Handle font, F32 size, FP_RasterFlags flags, String8 string)
{
  FP_RasterResult r = {0};
  return r;
}
