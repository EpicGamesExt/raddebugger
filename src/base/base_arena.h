// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_ARENA_H
#define BASE_ARENA_H

////////////////////////////////
//~ rjf: Arena Types

#define ARENA_HEADER_SIZE 128

typedef U64 ArenaFlags;
enum
{
  ArenaFlag_NoChain    = (1<<0),
  ArenaFlag_LargePages = (1<<1),
};

typedef struct ArenaParams ArenaParams;
struct ArenaParams
{
  ArenaFlags flags;
  U64 reserve_size;
  U64 commit_size;
  void *optional_backing_buffer;
  char *allocation_site_file;
  int allocation_site_line;
};

typedef struct Arena Arena;
struct Arena
{
  Arena *prev;    // previous arena in chain
  Arena *current; // current arena in chain
  ArenaFlags flags;
  U64 cmt_size;
  U64 res_size;
  U64 base_pos;
  U64 pos;
  U64 cmt;
  U64 res;
  char *allocation_site_file;
  int allocation_site_line;
#if ARENA_FREE_LIST
  U64 free_size;
  Arena *free_last;
#endif
};
StaticAssert(sizeof(Arena) <= ARENA_HEADER_SIZE, arena_header_size_check);

typedef struct Temp Temp;
struct Temp
{
  Arena *arena;
  U64 pos;
};

////////////////////////////////
//~ rjf: Arena Functions

global U64 arena_default_reserve_size = MB(64);
global U64 arena_default_commit_size  = KB(64);
global ArenaFlags arena_default_flags = 0;

//- rjf: arena creation/destruction
internal Arena *arena_alloc_(ArenaParams *params);
#define arena_alloc(...) arena_alloc_(&(ArenaParams){.reserve_size = arena_default_reserve_size, .commit_size = arena_default_commit_size, .flags = arena_default_flags, .allocation_site_file = __FILE__, .allocation_site_line = __LINE__, __VA_ARGS__})
internal void arena_release(Arena *arena);

//- rjf: arena push/pop/pos core functions
internal void *arena_push(Arena *arena, U64 size, U64 align);
internal U64   arena_pos(Arena *arena);
internal void  arena_pop_to(Arena *arena, U64 pos);

//- rjf: arena push/pop helpers
internal void arena_clear(Arena *arena);
internal void arena_pop(Arena *arena, U64 amt);

//- rjf: temporary arena scopes
internal Temp temp_begin(Arena *arena);
internal void temp_end(Temp temp);

//- rjf: push helper macros
#define push_array_no_zero_aligned(a, T, c, align) (T *)arena_push((a), sizeof(T)*(c), (align))
#define push_array_aligned(a, T, c, align) (T *)MemoryZero(push_array_no_zero_aligned(a, T, c, align), sizeof(T)*(c))
#define push_array_no_zero(a, T, c) push_array_no_zero_aligned(a, T, c, Max(8, AlignOf(T)))
#define push_array(a, T, c) push_array_aligned(a, T, c, Max(8, AlignOf(T)))

#endif // BASE_ARENA_H
