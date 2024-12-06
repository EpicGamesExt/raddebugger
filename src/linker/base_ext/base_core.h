// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

#if COMPILER_MSVC
# define COMPILER_STRING "MSVC"
#elif COMPILER_CLANG
# define COMPILER_STRING "Clang"
#elif COMPILER_GCC
# define COMPILER_STRING "GCC"
#else
# error "undefined compiler string"
#endif

#if BUILD_DEBUG
# define BUILD_MODE_STRING "Debug"
#else
# define BUILD_MODE_STRING "Release"
#endif

////////////////////////////////

#define BitExtract(x, count, shift) (((x) >> (shift)) & ((1 << (count)) - 1))

////////////////////////////////
// Linked List Helpers

#define DLLConcatInPlace(list, to_concat) do { \
  if ((to_concat)->count) {                    \
    if ((list)->count) {                       \
      (list)->last->next = (to_concat)->first; \
      (to_concat)->first->prev = (list)->last; \
      (list)->last = (to_concat)->last;        \
    } else {                                   \
      (list)->first = (to_concat)->first;      \
      (list)->last = (to_concat)->last;        \
    }                                          \
    (list)->count += (to_concat)->count;       \
    MemoryZeroStruct(to_concat);               \
  }                                            \
} while (0)
#define DLLConcatInPlaceArray(list, to_concat_arr, count) for (U64 i = 0; i < (count); i += 1) { DLLConcatInPlace(list, &(to_concat_arr)[i]); }

#define SLLQueuePushCount(list, node) do { \
  SLLQueuePush((list)->first, (list)->last, node); \
  ++(list)->count; \
} while (0)

#define SLLConcatInPlaceNoCount(list, to_concat) do { \
  if ((to_concat)->first) {                           \
    if ((list)->first) {                              \
      (list)->last->next = (to_concat)->first;        \
      (list)->last = (to_concat)->last;               \
    } else {                                          \
      (list)->first = (to_concat)->first;             \
      (list)->last = (to_concat)->last;               \
    }                                                 \
    MemoryZeroStruct(to_concat);                      \
  }                                                   \
} while (0)

#define SLLConcatInPlace(list, to_concat) do { \
  if ((to_concat)->count) {                    \
    if ((list)->count) {                       \
      (list)->last->next = (to_concat)->first; \
      (list)->last = (to_concat)->last;        \
    } else {                                   \
      (list)->first = (to_concat)->first;      \
      (list)->last = (to_concat)->last;        \
    }                                          \
    (list)->count += (to_concat)->count;       \
    MemoryZeroStruct(to_concat);               \
  }                                            \
} while (0)
#define SLLConcatInPlaceArray(list, to_concat_arr, count) for (U64 i = 0; i < (count); ++i) { SLLConcatInPlace(list, &(to_concat_arr)[i]); }

#define SLLConcatInPlaceChunkList(list, to_concat, chunk_type) do {   \
    if ((list)->last != 0) {                                          \
      U64 base_cursor = (list)->last->base + (list)->last->count;     \
      for (chunk_type *c = (to_concat)->first; c != 0; c = c->next) { \
        c->base = base_cursor;                                        \
        base_cursor += c->count;                                      \
      }                                                               \
    }                                                                 \
    SLLConcatInPlace(list, to_concat);                                \
  } while (0)

#define SLLConcatInPlaceChunkListArray(list, to_concat_arr, type, count) for (U64 i = 0; i < (count); ++i) { SLLConcatInPlaceChunkList(list, &(to_concat_arr)[i], type); }

#define SLLChunkListPush(_arena, _list, _cap, _value_type) do {                      \
  if ((_list)->last == 0 || (_list)->last->count >= (_list)->last->cap) {            \
    _value_type##Chunk *new_chunk = push_array(_arena, _value_type##Chunk, 1);       \
    new_chunk->v     = push_array(_arena, _value_type, _cap);                \
    new_chunk->cap   = _cap;                                                         \
    new_chunk->base  = (_list)->last ? (_list)->last->base + (_list)->last->cap : 0; \
    SLLQueuePushCount(_list, new_chunk);                                             \
  }                                                                                  \
  _value_type *v = &(_list)->last->v[(_list)->last->count++];                        \
  v->chunk = (_list)->last;                                                          \
} while (0)

#define SLLChunkListPushZero(_arena, _list, _cap, _value_type) do { \
  SLLChunkListPush(_arena, _list, _cap, _value_type);               \
  MemoryZeroStruct(SLLChunkListLastItem(_list));                    \
  SLLChunkListLastItem(_list)->chunk = (_list)->last;               \
} while(0)

#define SLLChunkListLastItem(_list) (&(_list)->last->v[(_list)->last->count - 1])

////////////////////////////////

#define MemoryIsZeroStruct(p) memory_is_zero(p, sizeof(*p))

////////////////////////////////

typedef struct
{
  U64 major;
  U64 minor;
} Version;

////////////////////////////////

typedef struct ISectOff
{
  U32 isect;
  U32 off;
} ISectOff;

////////////////////////////////

typedef struct PairU32
{
  U32 v0;
  U32 v1;
} PairU32;

typedef struct PairU64
{
  U64 v0;
  U64 v1;
} PairU64;

////////////////////////////////

internal U16 safe_cast_u16x(U64 x);

////////////////////////////////

internal U64 u128_mod64(U128 a, U64 b);

////////////////////////////////

internal Version make_version(U64 major, U64 minor);
internal int     version_compar(Version a, Version b);

////////////////////////////////

internal ISectOff isect_off(U32 isect, U32 off);

////////////////////////////////

internal int u16_compar(const void *raw_a, const void *raw_b);
internal int u32_compar(const void *raw_a, const void *raw_b);
internal int u64_compar(const void *raw_a, const void *raw_b);

internal int u8_is_before(void *raw_a, void *raw_b);
internal int u16_is_before(void *raw_a, void *raw_b);
internal int u32_is_before(void *raw_a, void *raw_b);
internal int u64_is_before(void *raw_a, void *raw_b);

internal int pair_u32_is_before_v0(void *raw_a, void *raw_b);
internal int pair_u32_is_before_v1(void *raw_a, void *raw_b);
internal int pair_u64_is_before_v0(void *raw_a, void *raw_b);
internal int pair_u64_is_before_v1(void *raw_a, void *raw_b);

////////////////////////////////

internal void str8_list_concat_in_place_array(String8List *list, String8List *arr, U64 count);

