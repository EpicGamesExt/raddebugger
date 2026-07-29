// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Memory Map Functions

internal void
memory_map_push(Arena *arena, MemoryMap *map, Rng1U64 vaddr_range, void *data)
{
  MemoryMapRangeNode *n = push_array(arena, MemoryMapRangeNode, 1);
  n->v.vaddr_range = vaddr_range;
  n->v.base = data;
  SLLQueuePush(map->first_range, map->last_range, n);
}

internal U64
memory_map_read(MemoryMap *map, Rng1U64 range, void *dst)
{
  U64 dst_vaddr = range.min;
  {
    for(;;)
    {
      U64 start_dst_vaddr = dst_vaddr;
      B32 found = 0;
      for(MemoryMapRangeNode *n = map->first_range; n != 0; n = n->next)
      {
        if(contains_1u64(n->v.vaddr_range, dst_vaddr))
        {
          U64 src_off = dst_vaddr - n->v.vaddr_range.min;
          U64 num_bytes_possible = n->v.vaddr_range.max - dst_vaddr;
          U64 num_bytes_needed = range.max - dst_vaddr;
          U64 num_bytes_to_read = Min(num_bytes_needed, num_bytes_possible);
          MemoryCopy((U8 *)dst + (dst_vaddr - range.min), (U8 *)n->v.base + src_off, num_bytes_to_read);
          dst_vaddr += num_bytes_to_read;
          found = 1;
        }
      }
      if(!found || start_dst_vaddr == dst_vaddr)
      {
        break;
      }
    }
  }
  U64 bytes_read = (dst_vaddr - range.min);
  return bytes_read;
}

internal String8
memory_map_data_from_range(Arena *arena, MemoryMap *map, Rng1U64 range)
{
  String8 result = {0};
  result.size = dim_1u64(range);
  result.str = push_array(arena, U8, result.size);
  memory_map_read(map, range, result.str);
  return result;
}
