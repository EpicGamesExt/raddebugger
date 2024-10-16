// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_Reloc *
lnk_reloc_list_reserve(Arena *arena, LNK_RelocList *list, U64 count)
{
  LNK_Reloc *arr = NULL;
  if (count) {
    arr = push_array(arena, LNK_Reloc, count);
    for (LNK_Reloc *ptr = arr, *opl = arr + count; ptr < opl; ++ptr) {
      SLLQueuePush(list->first, list->last, ptr);
    }
    list->count += count;
  }
  return arr;
}

internal LNK_Reloc *
lnk_reloc_list_push(Arena *arena, LNK_RelocList *list)
{
  LNK_Reloc *node = push_array(arena, LNK_Reloc, 1);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  return node;
}

internal LNK_RelocList
lnk_reloc_list_copy(Arena *arena, LNK_RelocList *list)
{
  LNK_RelocList result = {0};
  for (LNK_Reloc *n = list->first; n != NULL; n = n->next) {
    LNK_Reloc *r = lnk_reloc_list_push(arena, &result);
    r->chunk = n->chunk;
    r->type = n->type;
    r->apply_off = n->apply_off;
    r->symbol = n->symbol;
  }
  return result;
}

internal void
lnk_reloc_list_concat_in_place(LNK_RelocList *list, LNK_RelocList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal void
lnk_reloc_list_concat_in_place_arr(LNK_RelocList *list, LNK_RelocList *arr, U64 count)
{
  SLLConcatInPlaceArray(list, arr, count);
}

internal LNK_RelocList **
lnk_make_reloc_list_arr_arr(Arena *arena, U64 slot_count, U64 per_count)
{
  LNK_RelocList **arr_arr = push_array_no_zero(arena, LNK_RelocList *, slot_count);
  for (U64 i = 0; i < slot_count; i += 1) {
    arr_arr[i] = push_array(arena, LNK_RelocList, per_count);
  }
  return arr_arr;
}

internal LNK_RelocList
lnk_reloc_list_from_coff_reloc_array(Arena *arena, COFF_MachineType machine, LNK_Chunk *chunk, LNK_SymbolArray symbol_array, COFF_Reloc *reloc_v, U64 reloc_count)
{
  LNK_RelocList reloc_list = {0};
  LNK_Reloc *reloc_arr = lnk_reloc_list_reserve(arena, &reloc_list, reloc_count);
  LNK_Reloc *reloc_ptr = reloc_arr;
  LNK_Reloc *reloc_opl = reloc_arr + reloc_count;
  COFF_Reloc *coff_reloc_ptr = reloc_v;
  for (; reloc_ptr < reloc_opl; reloc_ptr += 1, coff_reloc_ptr += 1) {
    Assert(coff_reloc_ptr->isymbol < symbol_array.count);
    reloc_ptr->chunk     = chunk;
    reloc_ptr->type      = lnk_ext_reloc_type_from_coff(machine, coff_reloc_ptr->type);
    reloc_ptr->apply_off = coff_reloc_ptr->apply_off;
    reloc_ptr->symbol    = symbol_array.v + coff_reloc_ptr->isymbol;
  }
  return reloc_list;
}

internal LNK_Reloc **
lnk_reloc_array_from_list(Arena *arena, LNK_RelocList list)
{
  LNK_Reloc **arr = push_array_no_zero(arena, LNK_Reloc *, list.count);
  U64 count = 0;
  for (LNK_Reloc *node = list.first; node != 0; node = node->next) {
    Assert(count < list.count);
    arr[count++] = node;
  }
  return arr;
}

internal LNK_RelocType
lnk_ext_reloc_type_from_coff(COFF_MachineType machine, U32 type)
{
  LNK_RelocType result = LNK_Reloc_NULL;
  switch (machine) {
  case COFF_MachineType_UNKNOWN: break;
  case COFF_MachineType_X64: {
    switch (type) {
    case COFF_RelocTypeX64_ABS:       result = LNK_Reloc_NULL;        break;
    case COFF_RelocTypeX64_ADDR64:    result = LNK_Reloc_ADDR_64;     break;
    case COFF_RelocTypeX64_ADDR32:    result = LNK_Reloc_ADDR_32;     break;
    case COFF_RelocTypeX64_ADDR32NB:  result = LNK_Reloc_VIRT_OFF_32; break;
    case COFF_RelocTypeX64_REL32:     result = LNK_Reloc_REL32;       break;
    case COFF_RelocTypeX64_REL32_1:   result = LNK_Reloc_REL32_1;     break;
    case COFF_RelocTypeX64_REL32_2:   result = LNK_Reloc_REL32_2;     break;
    case COFF_RelocTypeX64_REL32_3:   result = LNK_Reloc_REL32_3;     break;
    case COFF_RelocTypeX64_REL32_4:   result = LNK_Reloc_REL32_4;     break;
    case COFF_RelocTypeX64_REL32_5:   result = LNK_Reloc_REL32_5;     break;
    case COFF_RelocTypeX64_SECTION:   result = LNK_Reloc_SECT_IDX;    break;
    case COFF_RelocTypeX64_SECREL:    result = LNK_Reloc_SECT_REL;    break;
    case COFF_RelocTypeX64_SECREL7:   lnk_not_implemented("TODO: COFF_RelocTypeX64_SECREL7"); break;
    case COFF_RelocTypeX64_TOKEN:     lnk_not_implemented("TODO: COFF_RelocTypeX64_TOKEN");   break;
    case COFF_RelocTypeX64_SREL32:    lnk_not_implemented("TODO: COFF_RelocTypeX64_SREL32");  break;
    case COFF_RelocTypeX64_PAIR:      lnk_not_implemented("TODO: COFF_RelocTypeX64_PAIR");    break;
    case COFF_RelocTypeX64_SSPAN32:   lnk_not_implemented("TODO: COFF_RelocTypeX64_SSPAN32"); break;
    default: lnk_invalid_path("unknown relocation type 0x%X", type);
    }
  } break;
  default: lnk_not_implemented("TODO: define remap for coff reloc types"); break;
  }
  return result;
}

internal U32
lnk_ext_reloc_type_to_coff(COFF_MachineType machine, LNK_RelocType type)
{
  U32 result = 0;
  switch (machine) {
  case COFF_MachineType_X64: {
    switch (type) {
    case LNK_Reloc_NULL:        result = COFF_RelocTypeX64_ABS;      break;
    case LNK_Reloc_ADDR_64:     result = COFF_RelocTypeX64_ADDR64;   break;
    case LNK_Reloc_ADDR_32:     result = COFF_RelocTypeX64_ADDR32;   break;
    case LNK_Reloc_VIRT_OFF_32: result = COFF_RelocTypeX64_ADDR32NB; break;
    case LNK_Reloc_REL32:       result = COFF_RelocTypeX64_REL32;    break;
    case LNK_Reloc_REL32_1:     result = COFF_RelocTypeX64_REL32_1;  break;
    case LNK_Reloc_REL32_2:     result = COFF_RelocTypeX64_REL32_2;  break;
    case LNK_Reloc_REL32_3:     result = COFF_RelocTypeX64_REL32_3;  break;
    case LNK_Reloc_REL32_4:     result = COFF_RelocTypeX64_REL32_4;  break;
    case LNK_Reloc_REL32_5:     result = COFF_RelocTypeX64_REL32_5;  break;
    case LNK_Reloc_SECT_IDX:    result = COFF_RelocTypeX64_SECTION;  break;
    case LNK_Reloc_SECT_REL:    result = COFF_RelocTypeX64_SECREL;   break;
    default: InvalidPath;
    }
  } break;
  default: lnk_not_implemented("TODO: support for machine 0x%X", machine); break;
  }
  return result;
}


