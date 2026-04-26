// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

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

internal Arena **
alloc_arena_many(Arena *arena, U64 count, U64 *sizes)
{
  Arena **result = push_array(arena, Arena *, count);
  U64  total_size = sum_array_u64(count, sizes);
  U8  *buffer     = push_array(arena, U8, total_size + (ARENA_HEADER_SIZE * count));
  U64  offset     = 0;
  for EachIndex(i, count) {
    if (sizes[i] > 0) {
      void *backing_buffer = buffer + offset;
      result[i] = arena_alloc_(&(ArenaParams){ .flags = ArenaFlag_NoChain, .optional_backing_buffer = backing_buffer, .reserve_size = ARENA_HEADER_SIZE + sizes[i], .commit_size = ARENA_HEADER_SIZE + sizes[i] });
      offset += ARENA_HEADER_SIZE;
      offset += sizes[i];
    }
  }
  return result;
}

internal Arena **
alloc_arena_array_(Arena *arena, U64 count, U64 *counts, U64 element_size)
{
  Temp scratch = scratch_begin(&arena, 1);
  U64 *sizes = push_array(scratch.arena, U64, count);
  for EachIndex(i, count) { sizes[i] = counts[i] * element_size; }
  Arena **result = alloc_arena_many(arena, count, sizes);
  scratch_end(scratch);
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

  for EachIndex(i, count) {
    Arena *fixed_arena = arena_alloc_(&(ArenaParams){ .reserve_size = res, .commit_size = cmt });
    arr[i] = fixed_arena;
  }

  return arr;
}

