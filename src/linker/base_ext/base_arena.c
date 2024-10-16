// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
push_cstr(Arena *arena, String8 str)
{
  U64 buffer_size = str.size + 1;
  U8 *buffer = push_array_no_zero(arena, U8, buffer_size);
  MemoryCopy(buffer, str.str, str.size);
  buffer[str.size] = 0;
  String8 result = str8(buffer, buffer_size);
  return result;
}

internal U32 *
push_u32(Arena *arena, U32 value)
{
  U32 *result = push_array_no_zero(arena, U32, 1);
  *result = value;
  return result;
}

internal U64 *
push_u64(Arena *arena, U64 value)
{
  U64 *result = push_array_no_zero(arena, U64, 1);
  *result = value;
  return result;
}

internal U32 *
push_array_copy_u32(Arena *arena, U32 *v, U64 count)
{
  U32 *result = push_array_no_zero(arena, U32, count);
  MemoryCopyTyped(result, v, count);
  return result;
}

internal U64 *
push_array_copy_u64(Arena *arena, U64 *v, U64 count)
{
  U64 *result = push_array_no_zero(arena, U64, count);
  MemoryCopyTyped(result, v, count);
  return result;
}

internal U64 **
push_matrix_u64(Arena *arena, U64 rows, U64 columns)
{
  U64 **result = push_array_no_zero(arena, U64 *, rows);
  for (U64 row_idx = 0; row_idx < rows; row_idx += 1) {
    result[row_idx] = push_array(arena, U64, columns);
  }
  return result;
}

internal Arena **
alloc_fixed_size_arena_array(Arena *arena, U64 count, U64 res, U64 cmt)
{
  U64 data_size = sizeof(count) + sizeof(Arena *) * count;
  U8 *data = push_array_no_zero(arena, U8, data_size);
  U64 *count_ptr = (U64 *)data;
  Arena **arr = (Arena **)(count_ptr + 1);
  *count_ptr = count;

  ArenaParams params  = {0};
  params.reserve_size = res;
  params.commit_size  = cmt;

  for (U64 i = 0; i < count; i += 1) {
    Arena *fixed_arena = arena_alloc_(&params);
    arr[i] = fixed_arena;
  }

  return arr;
}

internal void
release_arena_array(Arena **arr)
{
  U64 *count_ptr = (U64 *)arr - 1;
  U64 count = *count_ptr;
  for (U64 i = 0; i < count; i += 1) {
    arena_release(arr[i]);
    arr[i] = 0;
  }
}

