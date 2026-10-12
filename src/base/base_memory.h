// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

////////////////////////////////
//~ rjf: @per_os_impl Platform Memory Allocation

//- rjf: basic
internal void *reserve_memory(U64 size);
internal B32 commit_memory(void *ptr, U64 size);
internal void decommit_memory(void *ptr, U64 size);
internal void release_memory(void *ptr, U64 size);

// memory placeholders
internal B32   memory_placeholders_supported(void);
internal void *reserve_memory_placeholders(U64 size, U64 block_size);
internal B32   release_memory_placeholders(void *ptr, U64 size, U64 block_size);
internal B32   unmap_memory_preserve_placeholder(void *ptr, U64 size);
internal B32   split_memory_placeholder(void *ptr, U64 size, U64 first_size);
internal B32   coalesce_memory_placeholders(void *ptr, U64 size);
union Rng1U64;
internal void  prefetch_memory_ranges(U64 count, union Rng1U64 *ranges);

// generic memory page fault handler (calls may run concurrently)
typedef B32 MemoryReadFaultFunction(void *address, void *user_data);
internal B32 memory_read_fault_handler_set(MemoryReadFaultFunction *func, void *user_data);

//- rjf: large pages
internal void *reserve_memory_large(U64 size);
internal B32 commit_memory_large(void *ptr, U64 size);

#endif // BASE_MEMORY_H
