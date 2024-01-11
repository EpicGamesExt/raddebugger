// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_ARENA_DEV_H
#define BASE_ARENA_DEV_H

////////////////////////////////
//~ NOTE(allen): Dev Arena Types

typedef struct ArenaDev ArenaDev;
struct ArenaDev
{
  Arena *arena;
  struct ArenaProf *prof;
};

typedef struct ArenaProf ArenaProf;
struct ArenaProf
{
  struct ArenaProfNode *first;
  struct ArenaProfNode *last;
  U64 count;
};

typedef struct ArenaProfNode ArenaProfNode;
struct ArenaProfNode
{
  ArenaProfNode *next;
  String8 file_name;
  U64 line;
  U64 size;
  U64 count;
};

////////////////////////////////
//~ NOTE(allen): Dev Arena Functions

#if ENABLE_DEV
internal void      arena_annotate_push__dev(Arena *arena, U64 size, void *ptr);
internal void      arena_annotate_absorb__dev(Arena *arena, Arena *sub);
internal ArenaDev* arena_equip__dev(Arena *arena);
internal void      arena_equip_profile__dev(Arena *arena);
internal void      arena_print_profile__dev(Arena *arena, Arena *out_arena, String8List *out);
internal void      arena_prof_inc_counters__dev(Arena *dev_arena, ArenaProf *prof, String8 file_name, U64 line, U64 size, U64 count);
#endif

#endif // BASE_ARENA_DEV_H
