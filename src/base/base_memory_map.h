// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MEMORY_MAP_H
#define BASE_MEMORY_MAP_H

////////////////////////////////
//~ rjf: Memory Map Types

typedef struct MemoryMapRange MemoryMapRange;
struct MemoryMapRange
{
  Rng1U64 vaddr_range;
  void *base;
};

typedef struct MemoryMapRangeNode MemoryMapRangeNode;
struct MemoryMapRangeNode
{
  MemoryMapRangeNode *next;
  MemoryMapRange v;
};

typedef struct MemoryMap MemoryMap;
struct MemoryMap
{
  MemoryMapRangeNode *first_range;
  MemoryMapRangeNode *last_range;
};

////////////////////////////////
//~ rjf: Memory Map Functions

internal void memory_map_push(Arena *arena, MemoryMap *map, Rng1U64 vaddr_range, void *data);
internal U64 memory_map_read(MemoryMap *map, Rng1U64 range, void *dst);
#define memory_map_read_struct(map, vaddr, ptr) memory_map_read((map), r1u64((vaddr), (vaddr)+sizeof(*(ptr))), (ptr))
internal String8 memory_map_data_from_range(Arena *arena, MemoryMap *map, Rng1U64 range);

#endif // BASE_MEMORY_MAP_H
