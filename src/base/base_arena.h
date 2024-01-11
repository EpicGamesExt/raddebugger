// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_ARENA_H
#define BASE_ARENA_H

////////////////////////////////
//~ rjf: Constants

#define ARENA_HEADER_SIZE 128

#ifndef ARENA_RESERVE_SIZE
# define ARENA_RESERVE_SIZE MB(64)
#endif
#ifndef ARENA_COMMIT_SIZE
# define ARENA_COMMIT_SIZE KB(64)
#endif

#ifndef ARENA_RESERVE_SIZE_LARGE_PAGES
# define ARENA_RESERVE_SIZE_LARGE_PAGES MB(8)
#endif
#ifndef ARENA_COMMIT_SIZE_LARGE_PAGES
# define ARENA_COMMIT_SIZE_LARGE_PAGES MB(2)
#endif

////////////////////////////////
//~ rjf: Arena Types

typedef struct Arena Arena;
struct Arena
{
  struct Arena *prev;
  struct Arena *current;
  U64 base_pos;
  U64 pos;
  U64 cmt;
  U64 res;
  U64 align;
  struct ArenaDev *dev;
  B8 grow;
  B8 large_pages;
};

typedef struct Temp Temp;
struct Temp
{
  Arena *arena;
  U64 pos;
};

////////////////////////////////
// Implementation

internal Arena* arena_alloc__sized(U64 init_res, U64 init_cmt);

internal Arena* arena_alloc(void);
internal void   arena_release(Arena *arena);

internal void*  arena_push__impl(Arena *arena, U64 size);
internal U64    arena_pos(Arena *arena);
internal void   arena_pop_to(Arena *arena, U64 pos);

internal void   arena_absorb(Arena *arena, Arena *sub);

////////////////////////////////
// Wrappers

internal void* arena_push(Arena *arena, U64 size);
internal void* arena_push_contiguous(Arena *arena, U64 size);
internal void  arena_clear(Arena *arena);
internal void  arena_push_align(Arena *arena, U64 align);
internal void  arena_put_back(Arena *arena, U64 amt);

internal Temp  temp_begin(Arena *arena);
internal void  temp_end(Temp temp);

////////////////////////////////
//~ NOTE(allen): "Mini-Arena" Helper

internal B32 ensure_commit(void **cmt, void *pos, U64 cmt_block_size);

////////////////////////////////
//~ NOTE(allen): Main API Macros

#if !ENABLE_DEV
# define push_array_no_zero(a,T,c) (T*)arena_push((a), sizeof(T)*(c))
#else
# define push_array_no_zero(a,T,c) (tctx_write_this_srcloc(), (T*)arena_push((a), sizeof(T)*(c)))
#endif
#define push_array_no_zero__no_annotation(a,T,c) (T*)arena_push__impl((a), sizeof(T)*(c))

#define push_array(a,T,c) (T*)MemoryZero(push_array_no_zero(a,T,c), sizeof(T)*(c))

#endif // BASE_ARENA_H
