// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_BaseRelocPage
{
  U64     voff;
  U64List entries;
} LNK_BaseRelocPage;

typedef struct LNK_BaseRelocPageNode
{
  struct LNK_BaseRelocPageNode *next;
  LNK_BaseRelocPage             v;
} LNK_BaseRelocPageNode;

typedef struct LNK_BaseRelocPageList
{
  U64                    count;
  LNK_BaseRelocPageNode *first;
  LNK_BaseRelocPageNode *last;
} LNK_BaseRelocPageList;

typedef struct LNK_BaseRelocPageArray
{
  U64                count;
  LNK_BaseRelocPage *v;
} LNK_BaseRelocPageArray;

typedef struct
{
  U64                     page_size;
  LNK_Section           **sect_id_map;
  LNK_Reloc             **reloc_arr;
  Rng1U64                *range_arr;
  LNK_BaseRelocPageList  *list_arr;
  HashTable             **page_ht_arr;
} LNK_BaseRelocTask;

typedef struct
{
  U64                     page_size;
  LNK_Section           **sect_id_map;
  LNK_BaseRelocPageList  *list_arr;
  LNK_Obj               **obj_arr;
  HashTable             **page_ht_arr;
} LNK_ObjBaseRelocTask;

////////////////////////////////

internal LNK_BaseRelocPageArray lnk_base_reloc_page_array_from_list(Arena* arena, LNK_BaseRelocPageList list);
internal void lnk_emit_base_reloc_info(Arena *arena, LNK_Section **sect_id_map, U64 page_size, HashTable *page_ht, LNK_BaseRelocPageList *page_list, LNK_Reloc *reloc);
internal void lnk_base_reloc_page_array_sort(LNK_BaseRelocPageArray arr);
internal void lnk_build_base_relocs(TP_Context *tp, LNK_SectionTable *st, LNK_SymbolTable *symtab, COFF_MachineType machine, U64 page_size, LNK_ObjList obj_list);

