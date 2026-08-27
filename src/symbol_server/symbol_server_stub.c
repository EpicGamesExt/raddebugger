// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void
smsv_init(void)
{
}

internal void
smsv_async_tick(void)
{
}

internal String8
smsv_cache_path(void)
{
  return s("");
}

internal String8
smsv_local_module_path_from_key(Arena *arena, String8 module_name, U32 timestamp, U32 size_of_image)
{
  return s("");
}

internal String8
smsv_local_debug_info_path_from_key(Arena *arena, String8 dbg_name, Guid guid, U64 age)
{
  return s("");
}

internal void
smsv_fill_local_path(String8 path)
{
}

internal SMSV_Status
smsv_status_from_local_path(String8 path)
{
  SMSV_Status status = SMSV_Status_Null;
  return status;
}
