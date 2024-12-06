// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef enum
{
  LNK_Reloc_NULL,
  LNK_Reloc_ADDR_16,
  LNK_Reloc_ADDR_32,
  LNK_Reloc_ADDR_64,
  LNK_Reloc_CHUNK_SIZE_FILE_16,
  LNK_Reloc_CHUNK_SIZE_FILE_32,
  LNK_Reloc_CHUNK_SIZE_VIRT_32,
  LNK_Reloc_FILE_ALIGN_32,
  LNK_Reloc_FILE_OFF_15,
  LNK_Reloc_FILE_OFF_32,
  LNK_Reloc_FILE_OFF_64,
  LNK_Reloc_REL32,
  LNK_Reloc_REL32_1,
  LNK_Reloc_REL32_2,
  LNK_Reloc_REL32_3,
  LNK_Reloc_REL32_4,
  LNK_Reloc_REL32_5,
  LNK_Reloc_SECT_REL,
  LNK_Reloc_SECT_IDX,
  LNK_Reloc_VIRT_ALIGN_32,
  LNK_Reloc_VIRT_OFF_32,
} LNK_RelocType;

typedef struct LNK_Reloc
{
  struct LNK_Reloc  *next;
  LNK_Chunk         *chunk;
  LNK_RelocType      type;
  U64                apply_off;
  struct LNK_Symbol *symbol;
} LNK_Reloc;

typedef struct LNK_RelocList
{
  U64        count;
  LNK_Reloc *first;
  LNK_Reloc *last;
} LNK_RelocList;

internal LNK_Reloc *      lnk_reloc_list_reserve(Arena *arena, LNK_RelocList *list, U64 count);
internal LNK_Reloc *      lnk_reloc_list_push(Arena *arena, LNK_RelocList *list);
internal LNK_RelocList    lnk_reloc_list_copy(Arena *arena, LNK_RelocList *list);
internal void             lnk_reloc_list_concat_in_place(LNK_RelocList *list, LNK_RelocList *to_concat);
internal void             lnk_reloc_list_concat_in_place_arr(LNK_RelocList *list, LNK_RelocList *arr, U64 count);
internal LNK_RelocList ** lnk_make_reloc_list_arr_arr(Arena *arena, U64 slot_count, U64 per_count);
internal LNK_Reloc **     lnk_reloc_array_from_list(Arena *arena, LNK_RelocList list);
internal LNK_RelocType    lnk_ext_reloc_type_from_coff(COFF_MachineType machine, U32 type);
internal U32              lnk_ext_reloc_type_to_coff(COFF_MachineType machine, LNK_RelocType type);

