// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_SectionContrib
{
  U16 align;
  union {
    String8Node *data_list;
    U64          bss_size;
  };
  union {
    struct {
      U16  sort_idx_size;
      U32  obj_idx;
      U8  *sort_idx;
    };
    struct {
      U16 sect_idx;
      U32 off;
      U32 size;
    };
  } u;
} LNK_SectionContrib;

typedef struct LNK_CommonBlockContrib
{
  struct LNK_Symbol *symbol;
  union {
    U32 size;
    U32 offset;
  } u;
} LNK_CommonBlockContrib;

typedef struct LNK_SectionContribChunk
{
  struct LNK_SectionContribChunk *next;
  U64                             count;
  U64                             cap;
  LNK_SectionContrib            **v;
  LNK_SectionContrib             *v2;
} LNK_SectionContribChunk;

typedef struct LNK_SectionContribChunkList
{
  U64                      chunk_count;
  LNK_SectionContribChunk *first;
  LNK_SectionContribChunk *last;
} LNK_SectionContribChunkList;

typedef struct LNK_SectionDefinition
{
  String8           name;
  COFF_SectionFlags flags;
  U64               contribs_count;
  struct LNK_Obj   *obj;
  U64               obj_sect_idx;
} LNK_SectionDefinition;

typedef struct LNK_Section
{
  U64                id;
  String8            name;
  COFF_SectionFlags  flags;
  B32                has_layout;
  B32                is_merged;

  LNK_SectionContribChunkList contribs;
  LNK_SectionContrib *first_contrib_chunk;

  U64 voff;
  U64 vsize;
  U64 fsize;
  U64 foff;
  U64 sect_idx;
} LNK_Section;

typedef struct LNK_SectionNode
{
  struct LNK_SectionNode *next;
  struct LNK_SectionNode *prev;
  LNK_Section             data;
} LNK_SectionNode;

typedef struct LNK_SectionList
{
  U64              count;
  LNK_SectionNode *first;
  LNK_SectionNode *last;
} LNK_SectionList;

typedef struct LNK_SectionArray
{
  U64           count;
  LNK_Section **v;
} LNK_SectionArray;

typedef struct LNK_SectionTable
{
  Arena           *arena;
  U64              id_max;
  U64              next_sect_idx;
  LNK_SectionList  list;
  LNK_SectionList  free_list;
  HashTable       *sect_ht;   // name -> LNK_Section *
} LNK_SectionTable;

////////////////////////////////

internal U8  lnk_code_align_byte_from_machine(COFF_MachineType machine);
internal U16 lnk_default_align_from_machine(COFF_MachineType machine);

////////////////////////////////

internal LNK_SectionArray lnk_section_array_from_list(Arena *arena, LNK_SectionList list);

////////////////////////////////

internal LNK_SectionTable *  lnk_section_table_alloc(void);
internal void                lnk_section_table_release(LNK_SectionTable **st_ptr);
internal LNK_Section *       lnk_section_table_push(LNK_SectionTable *sectab, String8 name, COFF_SectionFlags flags);
internal void                lnk_section_table_remove(LNK_SectionTable *sectab, String8 name);
internal LNK_Section *       lnk_section_table_search(LNK_SectionTable *sectab, String8 name, COFF_SectionFlags flags);
internal void                lnk_section_table_merge(LNK_SectionTable *sectab, LNK_MergeDirectiveList merge_list);
internal LNK_SectionArray    lnk_section_table_get_output_sections(Arena *arena, LNK_SectionTable *sectab);

