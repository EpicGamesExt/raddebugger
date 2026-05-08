// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

typedef struct SharedMemory SharedMemory;
struct SharedMemory
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: @per_os_impl Platform Memory Allocation

//- rjf: basic
internal void *reserve_memory(U64 size);
internal B32 commit_memory(void *ptr, U64 size);
internal void decommit_memory(void *ptr, U64 size);
internal void release_memory(void *ptr, U64 size);

//- rjf: large pages
internal void *reserve_memory_large(U64 size);
internal B32 commit_memory_large(void *ptr, U64 size);

#endif // BASE_MEMORY_H
