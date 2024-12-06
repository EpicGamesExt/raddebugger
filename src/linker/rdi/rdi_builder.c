// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal RDIB_DataModel
rdib_infer_data_model(OperatingSystem os, RDI_Arch arch)
{
  RDIB_DataModel data_model = RDIB_DataModel_Null;
  switch (os) {
  case OperatingSystem_Null: break;
  case OperatingSystem_Windows: {
    switch (arch) {
    case RDI_Arch_X86:
    case RDI_Arch_X64:
      data_model = RDIB_DataModel_LLP64; break;
    default: NotImplemented;
    }
  } break;
  case OperatingSystem_Linux: {
    switch (arch) {
    case RDI_Arch_X86: data_model = RDIB_DataModel_ILP32; break;
    case RDI_Arch_X64: data_model = RDIB_DataModel_LLP64; break;
    default: NotImplemented;
    }
  } break;
  case OperatingSystem_Mac: {
    switch (arch) {
    case RDI_Arch_X86: NotImplemented; break;
    case RDI_Arch_X64: data_model = RDIB_DataModel_LP64; break;
    }
  } break;
  default: InvalidPath;
  }
  return data_model;
}

internal RDI_TypeKind
rdib_short_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_S16;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_S16;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_S16;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_S16;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_S64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_unsigned_short_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_U16;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_U16;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_U16;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_U16;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_U64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_int_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_S32;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_S32;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_S32;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_S64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_S64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_unsigned_int_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_U32;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_U32;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_U32;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_U64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_U64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_long_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_S32;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_S32;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_S64;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_S64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_S64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_unsigned_long_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_U32;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_U32;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_U64;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_U64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_U64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_long_long_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_S64;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_S64;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_S64;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_S64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_S64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_unsigned_long_long_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_U64;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_U64;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_U64;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_U64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_U64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

internal RDI_TypeKind
rdib_pointer_size_t_type_from_data_model(RDIB_DataModel data_model)
{
  switch (data_model) {
  case RDIB_DataModel_Null  : break;
  case RDIB_DataModel_ILP32 : return RDI_TypeKind_U32;
  case RDIB_DataModel_LLP64 : return RDI_TypeKind_U64;
  case RDIB_DataModel_LP64  : return RDI_TypeKind_U64;
  case RDIB_DataModel_ILP64 : return RDI_TypeKind_U64;
  case RDIB_DataModel_SILP64: return RDI_TypeKind_U64;
  default: InvalidPath;
  }
  return RDI_TypeKind_NULL;
}

////////////////////////////////

internal void
rdib_udt_member_list_push_node(RDIB_UDTMemberList *list, RDIB_UDTMember *node)
{
  SLLQueuePushCount(list, node);
}

internal void
rdib_udt_member_list_concat_in_place(RDIB_UDTMemberList *list, RDIB_UDTMemberList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal void
rdib_line_table_push_fragment_node(RDIB_LineTable *list, RDIB_LineTableFragment *n)
{
  SLLQueuePush_N(list->first, list->last, n, next_line_table);
  ++list->count;
}

internal RDIB_LineTableFragment *
rdib_line_table_push(Arena *arena, RDIB_LineTable *list)
{
  RDIB_LineTableFragment *n = push_array(arena, RDIB_LineTableFragment, 1);
  rdib_line_table_push_fragment_node(list, n);
  return n;
}

////////////////////////////////

internal RDIB_LineTableFragment *
rdib_line_table_fragment_chunk_list_push(Arena *arena, RDIB_LineTableFragmentChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_LineTableFragment);
  return SLLChunkListLastItem(list);
}

internal RDIB_Unit *
rdib_unit_chunk_list_push(Arena *arena, RDIB_UnitChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_Unit);
  return SLLChunkListLastItem(list);
}

internal RDIB_Scope *
rdib_scope_chunk_list_push(Arena *arena, RDIB_ScopeChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_Scope);
  return SLLChunkListLastItem(list);
}

internal RDIB_Procedure *
rdib_procedure_chunk_list_push(Arena *arena, RDIB_ProcedureChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_Procedure);
  return SLLChunkListLastItem(list);
}

internal RDIB_Variable *
rdib_variable_chunk_list_push(Arena *arena, RDIB_VariableChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_Variable);
  return SLLChunkListLastItem(list);
}

internal RDIB_LineTable *
rdib_line_table_chunk_list_push(Arena *arena, RDIB_LineTableChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_LineTable);
  return SLLChunkListLastItem(list);
}

internal RDIB_Type *
rdib_type_chunk_list_push(Arena *arena, RDIB_TypeChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_Type);
  RDIB_Type *type = SLLChunkListLastItem(list);
  type->final_idx = 0;
  return type;
}

internal RDIB_UDTMember *
rdib_udt_member_chunk_list_push(Arena *arena, RDIB_UDTMemberChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_UDTMember);
  return SLLChunkListLastItem(list);
}

internal RDIB_SourceFile *
rdib_source_file_chunk_list_push(Arena *arena, RDIB_SourceFileChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_SourceFile);
  return SLLChunkListLastItem(list);
}

internal RDIB_InlineSite *
rdib_inline_site_chunk_list_push(Arena *arena, RDIB_InlineSiteChunkList *list, U64 cap)
{
  SLLChunkListPush(arena, list, cap, RDIB_InlineSite);
  return SLLChunkListLastItem(list);
}

internal RDIB_Unit *
rdib_unit_chunk_list_push_zero(Arena *arena, RDIB_UnitChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_Unit);
  return SLLChunkListLastItem(list);
}

internal RDIB_Scope *
rdib_scope_chunk_list_push_zero(Arena *arena, RDIB_ScopeChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_Scope);
  return SLLChunkListLastItem(list);
}

internal RDIB_Procedure *
rdib_procedure_chunk_list_push_zero(Arena *arena, RDIB_ProcedureChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_Procedure);
  return SLLChunkListLastItem(list);
}

internal RDIB_Variable *
rdib_variable_chunk_list_push_zero(Arena *arena, RDIB_VariableChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_Variable);
  return SLLChunkListLastItem(list);
}

internal RDIB_LineTable *
rdib_line_table_chunk_list_push_zero(Arena *arena, RDIB_LineTableChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_LineTable);
  return SLLChunkListLastItem(list);
}

internal RDIB_Type *
rdib_type_chunk_list_push_zero(Arena *arena, RDIB_TypeChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_Type);
  return SLLChunkListLastItem(list);
}

internal RDIB_UDTMember *
rdib_udt_member_chunk_list_push_zero(Arena *arena, RDIB_UDTMemberChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_UDTMember);
  return SLLChunkListLastItem(list);
}

internal RDIB_SourceFile *
rdib_source_file_chunk_list_push_zero(Arena *arena, RDIB_SourceFileChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_SourceFile);
  return SLLChunkListLastItem(list);
}

internal RDIB_InlineSite *
rdib_inline_site_chunk_list_push_zero(Arena *arena, RDIB_InlineSiteChunkList *list, U64 cap)
{
  SLLChunkListPushZero(arena, list, cap, RDIB_InlineSite);
  return SLLChunkListLastItem(list);
}

internal RDIB_UnitChunk *
rdib_unit_chunk_list_reserve_ex(Arena *arena, RDIB_UnitChunkList *list, U64 count_per_chunk, U64 item_count)
{
  U64             chunk_count = CeilIntegerDiv(item_count, count_per_chunk);
  RDIB_UnitChunk *chunks      = push_array(arena, RDIB_UnitChunk, chunk_count);
  U64             base        = list->last ? list->last->base : 0;

  for (U64 i = 0; i+1 < chunk_count; i += 1, item_count -= count_per_chunk, base += count_per_chunk) {
    chunks[i].base  = base;
    chunks[i].count = count_per_chunk;
    chunks[i].cap   = count_per_chunk;
    chunks[i].v     = push_array(arena, RDIB_Unit, count_per_chunk);
    SLLQueuePush(list->first, list->last, &chunks[i]);

    for (U64 k = 0; k < count_per_chunk; ++k) {
      chunks[i].v[k].chunk = &chunks[i];
    }
  }

  chunks[chunk_count-1].base  = base;
  chunks[chunk_count-1].count = item_count;
  chunks[chunk_count-1].cap   = item_count;
  chunks[chunk_count-1].v     = push_array(arena, RDIB_Unit, item_count);
  for (U64 k = 0; k < item_count; ++k) {
    chunks[chunk_count-1].v[k].chunk = &chunks[chunk_count-1];
  }

  SLLQueuePush(list->first, list->last, &chunks[chunk_count-1]);
  list->count += chunk_count;

  return chunks;
}

internal void
rdib_unit_chunk_list_reserve(Arena *arena, RDIB_UnitChunkList *list, U64 cap)
{
  // fill out node
  RDIB_UnitChunk *chunk = push_array(arena, RDIB_UnitChunk, 1);
  chunk->cap            = cap;
  chunk->v              = push_array(arena, RDIB_Unit, cap);

  // push node to list
  SLLQueuePush(list->first, list->last, chunk);
  list->count += 1;
}

internal void
rdib_type_chunk_list_reserve(Arena *arena, RDIB_TypeChunkList *list, U64 cap)
{
  // fill out node
  RDIB_TypeChunk *chunk = push_array(arena, RDIB_TypeChunk, 1);
  chunk->cap            = cap;
  chunk->v              = push_array(arena, RDIB_Type, cap);

  // push node to list
  SLLQueuePush(list->first, list->last, chunk);
  list->count += 1;
}

internal void
rdib_source_file_list_reserve(Arena *arena, RDIB_SourceFileChunkList *list, U64 cap)
{
  // fill out node
  RDIB_SourceFileChunk *chunk = push_array(arena, RDIB_SourceFileChunk, 1);
  chunk->cap                  = cap;
  chunk->v                    = push_array(arena, RDIB_SourceFile, cap);

  // push node to list
  SLLQueuePush(list->first, list->last, chunk);
  list->count += 1;
}

internal void
rdib_unit_chunk_list_concat_in_place(RDIB_UnitChunkList *list, RDIB_UnitChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_UnitChunk);
}

internal void
rdib_scope_chunk_list_concat_in_place(RDIB_ScopeChunkList *list, RDIB_ScopeChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_ScopeChunk);
}

internal void
rdib_udt_member_chunk_list_concat_in_place(RDIB_UDTMemberChunkList *list, RDIB_UDTMemberChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_UDTMemberChunk);
}

internal void
rdib_procedure_chunk_list_concat_in_place(RDIB_ProcedureChunkList *list, RDIB_ProcedureChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_ProcedureChunk);
}

internal void
rdib_variable_chunk_list_concat_in_place(RDIB_VariableChunkList *list, RDIB_VariableChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_VariableChunk);
}

internal void
rdib_line_table_chunk_list_concat_in_place(RDIB_LineTableChunkList *list, RDIB_LineTableChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_LineTableChunk);
}

internal void
rdib_inline_site_chunk_list_concat_in_place(RDIB_InlineSiteChunkList *list, RDIB_InlineSiteChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_InlineSiteChunk);
}

internal void
rdib_type_chunk_list_concat_in_place(RDIB_TypeChunkList *list, RDIB_TypeChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_TypeChunk);
}

internal void
rdib_source_file_chunk_list_concat_in_place(RDIB_SourceFileChunkList *list, RDIB_SourceFileChunkList *to_concat)
{
  SLLConcatInPlaceChunkList(list, to_concat, RDIB_SourceFileChunk);
}

internal void
rdib_line_table_chunk_list_concat_in_place_many(RDIB_LineTableChunkList *list, RDIB_LineTableChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_LineTableChunk, count);
}

internal void
rdib_scope_chunk_list_concat_in_place_many(RDIB_ScopeChunkList *list, RDIB_ScopeChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_ScopeChunk, count);
}

internal void
rdib_variable_chunk_list_concat_in_place_many(RDIB_VariableChunkList *list, RDIB_VariableChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_VariableChunk, count);
}

internal void
rdib_procedure_chunk_list_concat_in_place_many(RDIB_ProcedureChunkList *list, RDIB_ProcedureChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_ProcedureChunk, count);
}

internal void
rdib_inline_site_chunk_list_concat_in_place_many(RDIB_InlineSiteChunkList *list, RDIB_InlineSiteChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_InlineSiteChunk, count);
}

internal void
rdib_type_chunk_list_concat_in_place_many(RDIB_TypeChunkList *list, RDIB_TypeChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_TypeChunk, count);
}

internal void
rdib_udt_member_chunk_list_concat_in_place_many(RDIB_UDTMemberChunkList *list, RDIB_UDTMemberChunkList *to_concat, U64 count)
{
  SLLConcatInPlaceChunkListArray(list, to_concat, RDIB_UDTMemberChunk, count);
}

internal RDIB_UnitChunk **
rdib_array_from_unit_chunk_list(Arena *arena, RDIB_UnitChunkList list)
{
  ProfBeginFunction();
  RDIB_UnitChunk **result = push_array_no_zero(arena, RDIB_UnitChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_UnitChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_ScopeChunk **
rdib_array_from_scope_chunk_list(Arena *arena, RDIB_ScopeChunkList list)
{
  ProfBeginFunction();
  RDIB_ScopeChunk **result = push_array_no_zero(arena, RDIB_ScopeChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_ScopeChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_VariableChunk **
rdib_array_from_variable_chunk_list(Arena *arena, RDIB_VariableChunkList list)
{
  ProfBeginFunction();
  RDIB_VariableChunk **result = push_array_no_zero(arena, RDIB_VariableChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_VariableChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_LineTableChunk **
rdib_array_from_line_table_chunk_list(Arena *arena, RDIB_LineTableChunkList list)
{
  ProfBeginFunction();
  RDIB_LineTableChunk **result = push_array_no_zero(arena, RDIB_LineTableChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_LineTableChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_ProcedureChunk **
rdib_array_from_procedure_chunk_list(Arena *arena, RDIB_ProcedureChunkList list)
{
  ProfBeginFunction();
  RDIB_ProcedureChunk **result = push_array_no_zero(arena, RDIB_ProcedureChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_ProcedureChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_InlineSiteChunk **
rdib_array_from_inline_site_chunk_list(Arena *arena, RDIB_InlineSiteChunkList list)
{
  ProfBeginFunction();
  RDIB_InlineSiteChunk **result = push_array_no_zero(arena, RDIB_InlineSiteChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_InlineSiteChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_UDTMemberChunk **
rdib_array_from_udt_member_chunk_list(Arena *arena, RDIB_UDTMemberChunkList list)
{
  ProfBeginFunction();
  RDIB_UDTMemberChunk **result = push_array_no_zero(arena, RDIB_UDTMemberChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_UDTMemberChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_TypeChunk **
rdib_array_from_type_chunk_list(Arena *arena, RDIB_TypeChunkList list)
{
  ProfBeginFunction();
  RDIB_TypeChunk **result = push_array_no_zero(arena, RDIB_TypeChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_TypeChunk *chunk = list.first; chunk != 0; chunk = chunk->next, ++chunk_idx) {
    result[chunk_idx] = chunk;
  }
  ProfEnd();
  return result;
}

internal RDIB_SourceFileChunk **
rdib_array_from_source_file_chunk_list(Arena *arena, RDIB_SourceFileChunkList list)
{
  ProfBeginFunction();
  RDIB_SourceFileChunk **result = push_array_no_zero(arena, RDIB_SourceFileChunk *, list.count);
  U64 chunk_idx = 0;
  for (RDIB_SourceFileChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    result[chunk_idx++] = chunk;
  }
  ProfEnd();
  return result;
}

internal U64
rdib_unit_chunk_list_total_count(RDIB_UnitChunkList list)
{
  U64 total_count = 0;
  for (RDIB_UnitChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_scope_chunk_list_total_count(RDIB_ScopeChunkList list)
{
  U64 total_count = 0;
  for (RDIB_ScopeChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_variable_chunk_list_total_count(RDIB_VariableChunkList list)
{
  U64 total_count = 0;
  for (RDIB_VariableChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_line_table_chunk_list_total_count(RDIB_LineTableChunkList list)
{
  U64 total_count = 0;
  for (RDIB_LineTableChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_procedure_chunk_list_total_count(RDIB_ProcedureChunkList list)
{
  U64 total_count = 0;
  for (RDIB_ProcedureChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_inline_site_chunk_list_total_count(RDIB_InlineSiteChunkList list)
{
  U64 total_count = 0;
  for (RDIB_InlineSiteChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_udt_member_chunk_list_total_count(RDIB_UDTMemberChunkList list)
{
  U64 total_count = 0;
  for (RDIB_UDTMemberChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_type_chunk_list_total_count(RDIB_TypeChunkList list)
{
  U64 total_count = 0;
  for (RDIB_TypeChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U64
rdib_source_file_chunk_list_total_count(RDIB_SourceFileChunkList list)
{
  U64 total_count = 0;
  for (RDIB_SourceFileChunk *chunk = list.first; chunk != 0; chunk = chunk->next) {
    total_count += chunk->count;
  }
  return total_count;
}

internal U32
rdib_idx_from_unit(RDIB_Unit *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_scope(RDIB_Scope *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_inline_site(RDIB_InlineSite *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_variable(RDIB_Variable *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_procedure(RDIB_Procedure *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_source_file(RDIB_SourceFile *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_line_table(RDIB_LineTable *n)
{
  U32 idx = 0;
  if (n) {
    Assert(n->chunk->v <= n && n < (n->chunk->v + n->chunk->count));
    idx = safe_cast_u32(n->chunk->base + (n - n->chunk->v));
    Assert(idx - n->chunk->base < n->chunk->count);
  }
  return idx;
}

internal U32
rdib_idx_from_type(RDIB_Type *n)
{
  U32 idx = 0;
  if (n) {
    idx = safe_cast_u32(n->final_idx);
  }
  return idx;
}

internal U32
rdib_idx_from_udt_type(RDIB_Type *n)
{
  U32 idx = 0;
  if (n && RDI_IsUserDefinedType(n->kind)) {
    idx = safe_cast_u32(n->udt.udt_idx);
  }
  return idx;
}

////////////////////////////////
// Source File

internal B32
rdib_source_file_match(RDIB_SourceFile *a, RDIB_SourceFile *b, OperatingSystem os)
{
  StringMatchFlags match_flags = path_match_flags_from_os(os);
  if (str8_match(a->normal_full_path, b->normal_full_path, match_flags)) {
    if (a->checksum_kind == b->checksum_kind) {
      if (str8_match(a->checksum, b->checksum, 0)) {
        return 1;
      }
    }
  }
  return 0;
}

////////////////////////////////
// Eval Ops

internal RDIB_EvalBytecodeOp *
rdib_bytecode_push_op(Arena *arena, RDIB_EvalBytecode *bytecode, RDI_EvalOp op, RDI_U64 p)
{
  RDIB_EvalBytecodeOp *node = push_array(arena, RDIB_EvalBytecodeOp, 1);
  node->op                  = op;
  node->p_size              = RDI_DECODEN_FROM_CTRLBITS(rdi_eval_op_ctrlbits_table[op]);
  node->p                   = p;

  SLLQueuePush(bytecode->first, bytecode->last, node);
  bytecode->count += 1;
  bytecode->size  += 1 + node->p_size;

  return node;
}

internal void
rdib_bytecode_push_ucsont(Arena *arena, RDIB_EvalBytecode *bytecode, RDI_U64 uconst)
{
  if (uconst <= max_U8) {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU8, uconst);
  } else if (uconst <= max_U16) {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU16, uconst);
  } else if (uconst <= max_U32) {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU32, uconst);
  } else {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU64, uconst);
  }
}

internal void
rdib_bytecode_push_sconst(Arena *arena, RDIB_EvalBytecode *bytecode, RDI_S64 sconst)
{
  if (min_S8 <= sconst && sconst <= max_S8) {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU8, (RDI_U64)sconst);
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_TruncSigned, 8);
  } else if (min_S16 <= sconst && sconst <= max_S16) {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU16, (RDI_U64)sconst);
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_TruncSigned, 16);
  } else if (min_S32 <= sconst && sconst <= max_S32) {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU32, (RDI_U64)sconst);
  } else {
    rdib_bytecode_push_op(arena, bytecode, RDI_EvalOp_ConstU64, (RDI_U64)sconst);
  }
}

////////////////////////////////
// Location

internal RDIB_Location
rdib_make_location_addr_byte_stream(Rng1U64List ranges, RDIB_EvalBytecode bytecode)
{
  RDIB_Location loc = {0};
  loc.ranges        = ranges;
  loc.kind          = RDI_LocationKind_AddrBytecodeStream;
  loc.bytecode      = bytecode;
  return loc;
}

internal RDIB_Location
rdib_make_location_addr_bytecode_stream(Rng1U64List ranges, RDIB_EvalBytecode bytecode)
{
  RDIB_Location loc = {0};
  loc.ranges        = ranges;
  loc.kind          = RDI_LocationKind_AddrBytecodeStream;
  loc.bytecode      = bytecode;
  return loc;
}

internal RDIB_Location
rdib_make_location_val_bytecode_stream(Rng1U64List ranges, RDIB_EvalBytecode bytecode)
{
  RDIB_Location loc = {0};
  loc.ranges        = ranges;
  loc.kind          = RDI_LocationKind_ValBytecodeStream;
  loc.bytecode      = bytecode;
  return loc;
}

internal RDIB_Location
rdib_make_location_addr_reg_plus_u16(Rng1U64List ranges, RDI_RegCode reg_code, RDI_U16 offset)
{
  RDIB_Location loc = {0};
  loc.ranges        = ranges;
  loc.kind          = RDI_LocationKind_AddrRegPlusU16;
  loc.reg_code      = reg_code;
  loc.offset        = offset;
  return loc;
}

internal RDIB_Location
rdib_make_location_addr_addr_reg_plus_u16(Rng1U64List ranges, RDI_RegCode reg_code, RDI_U16 offset)
{
  RDIB_Location loc = {0};
  loc.kind          = RDI_LocationKind_AddrAddrRegPlusU16;
  loc.ranges        = ranges;
  loc.reg_code      = reg_code;
  loc.offset        = offset;
  return loc;
}

internal RDIB_Location
rdib_make_location_val_reg(Rng1U64List ranges, RDI_RegCode reg_code)
{
  RDIB_Location loc = {0};
  loc.kind          = RDI_LocationKind_ValReg;
  loc.ranges        = ranges;
  loc.reg_code      = reg_code;
  return loc;
}

internal RDIB_LocationNode *
rdib_location_list_push(Arena *arena, RDIB_LocationList *list, RDIB_Location v)
{
  RDIB_LocationNode *node = push_array(arena, RDIB_LocationNode, 1);
  node->v = v;
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
  return node;
}

internal RDIB_LocationNode *
rdib_push_location_addr_reg_off(Arena *arena, RDIB_LocationList *list, RDI_Arch arch, RDI_RegCode reg_code, U32 reg_byte_size, U32 reg_byte_pos, S64 offset, B32 is_reference, Rng1U64List ranges)
{
  RDIB_Location loc;

  if (0 <= offset && offset <= (S64)max_U16) {
    if (is_reference) {
      loc = rdib_make_location_addr_addr_reg_plus_u16(ranges, reg_code, (U16)offset);
    } else {
      loc = rdib_make_location_addr_reg_plus_u16(ranges, reg_code, (U16)offset);
    }
  }

  // long offset, emit byte code
  else {
    RDIB_EvalBytecode bytecode = {0};
    U32 reg_read_param = RDI_EncodeRegReadParam(reg_code, reg_byte_size, reg_byte_pos);
    rdib_bytecode_push_op(arena, &bytecode, RDI_EvalOp_RegRead, reg_read_param);
    rdib_bytecode_push_sconst(arena, &bytecode, offset);
    rdib_bytecode_push_op(arena, &bytecode, RDI_EvalOp_Add, 0);

    if (is_reference) {
      U64 addr_size = rdi_addr_size_from_arch(arch);
      rdib_bytecode_push_op(arena, &bytecode, RDI_EvalOp_MemRead, addr_size);
    }

    loc = rdib_make_location_addr_bytecode_stream(ranges, bytecode);
  }

  RDIB_LocationNode *node = rdib_location_list_push(arena, list, loc);
  return node;
}

internal void
rdib_variable_list_push_node(RDIB_VariableList *list, RDIB_VariableNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  ++list->count;
}

internal RDIB_VariableNode *
rdib_variable_list_push(Arena *arena, RDIB_VariableList *list)
{
  RDIB_VariableNode *node = push_array(arena, RDIB_VariableNode, 1);
  rdib_variable_list_push_node(list, node);
  return node;
}

////////////////////////////////
// Types

internal U64
rdib_size_from_type(RDIB_Type *type)
{
  if (type) {
    switch (type->kind) {
    case RDI_TypeKind_Void:
    case RDI_TypeKind_Char8:
    case RDI_TypeKind_Char16:
    case RDI_TypeKind_Char32:
    case RDI_TypeKind_UChar8:
    case RDI_TypeKind_UChar16:
    case RDI_TypeKind_UChar32:
    case RDI_TypeKind_U8:
    case RDI_TypeKind_U16:
    case RDI_TypeKind_U32:
    case RDI_TypeKind_U64:
    case RDI_TypeKind_U128:
    case RDI_TypeKind_U256:
    case RDI_TypeKind_U512:
    case RDI_TypeKind_S8:
    case RDI_TypeKind_S16:
    case RDI_TypeKind_S32:
    case RDI_TypeKind_S64:
    case RDI_TypeKind_S128:
    case RDI_TypeKind_S256:
    case RDI_TypeKind_S512:
    case RDI_TypeKind_Bool:
    case RDI_TypeKind_F16:
    case RDI_TypeKind_F32:
    case RDI_TypeKind_F32PP:
    case RDI_TypeKind_F48:
    case RDI_TypeKind_F64:
    case RDI_TypeKind_F80:
    case RDI_TypeKind_F128:
    case RDI_TypeKind_ComplexF32:
    case RDI_TypeKind_ComplexF64:
    case RDI_TypeKind_ComplexF80:
    case RDI_TypeKind_ComplexF128:
    case RDI_TypeKind_Handle: 
      return type->builtin.size;
    case RDI_TypeKind_Modifier:
      return rdib_size_from_type((RDIB_Type *)type->modifier.type_ref);
    case RDI_TypeKind_Ptr:
    case RDI_TypeKind_LRef:
    case RDI_TypeKind_RRef:
      return type->ptr.size;
    case RDI_TypeKind_Array:
      return type->array.size;

    case RDI_TypeKind_Function:
    case RDI_TypeKind_Method:
    case RDI_TypeKindExt_StaticMethod: {
      Assert(!"check");
      return 0;
    }
    case RDI_TypeKind_Struct:
    case RDI_TypeKind_Class:
    case RDI_TypeKind_IncompleteStruct:
    case RDI_TypeKind_IncompleteClass:
      return type->udt.struct_type.size;

    case RDI_TypeKind_Union:
    case RDI_TypeKind_IncompleteUnion:
      return type->udt.union_type.size;

    case RDI_TypeKind_Alias:
      Assert(!"check");

    case RDI_TypeKind_Enum:
    case RDI_TypeKind_IncompleteEnum:
      return rdib_size_from_type(type->udt.enum_type.base_type);

    case RDI_TypeKind_MemberPtr:
    case RDI_TypeKind_Bitfield:
    case RDI_TypeKind_Variadic:
    case RDI_TypeKindExt_Members:
    case RDI_TypeKindExt_Params:
      InvalidPath; // no size
    }
  }
  return 0;
}

internal RDIB_TypeRef
rdib_make_type_ref(Arena *arena, RDIB_Type *type)
{
  RDIB_Type **ref = push_array(arena, RDIB_Type *, 1);
  ref[0] = type;
  return ref;
}

internal void
rdib_deref_type_refs(TP_Context *tp, RDIB_TypeChunkList *list)
{
  for (RDIB_TypeChunk *chunk = list->first; chunk != 0; chunk = chunk->next) {
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Type *type = &chunk->v[i];
      if (type->kind == RDI_TypeKind_Struct || type->kind == RDI_TypeKind_Class ||
          type->kind == RDI_TypeKind_IncompleteStruct || type->kind == RDI_TypeKind_IncompleteClass) {
        type->udt.members             = *(RDIB_Type **)type->udt.members;
        type->udt.struct_type.derived = *(RDIB_Type **)type->udt.struct_type.derived;
        type->udt.struct_type.vtshape = *(RDIB_Type **)type->udt.struct_type.vtshape;
      } else if (type->kind == RDI_TypeKind_Enum || type->kind == RDI_TypeKind_IncompleteEnum) {
        type->udt.members             = *(RDIB_Type **)type->udt.members;
        type->udt.enum_type.base_type = *(RDIB_Type **)type->udt.enum_type.base_type;
      } else if (type->kind == RDI_TypeKind_Union || type->kind == RDI_TypeKind_IncompleteUnion) {
        type->udt.members = *(RDIB_Type **)type->udt.members;
      } else if (type->kind == RDI_TypeKind_Array) {
        type->array.entry_type = *(RDIB_Type **)type->array.entry_type;
      } else if (type->kind == RDI_TypeKind_Function) {
        type->func.return_type = *(RDIB_Type **)type->func.return_type;
        type->func.params_type = *(RDIB_Type **)type->func.params_type;
      } else if (type->kind == RDI_TypeKind_Method) {
        type->method.class_type  = *(RDIB_Type **)type->method.class_type;
        type->method.this_type   = *(RDIB_Type **)type->method.this_type;
        type->method.return_type = *(RDIB_Type **)type->method.return_type;
        type->method.params_type = *(RDIB_Type **)type->method.params_type;
      } else if (type->kind == RDI_TypeKindExt_StaticMethod) {
        type->static_method.class_type  = *(RDIB_Type **)type->static_method.class_type;
        type->static_method.return_type = *(RDIB_Type **)type->static_method.return_type;
        type->static_method.params_type = *(RDIB_Type **)type->static_method.params_type;
      } else if (type->kind == RDI_TypeKind_Ptr || type->kind == RDI_TypeKind_LRef || type->kind == RDI_TypeKind_RRef) {
        type->ptr.type_ref = *(RDIB_Type **)type->ptr.type_ref;
      } else if (type->kind == RDI_TypeKind_Modifier) {
        type->modifier.type_ref = *(RDIB_Type **)type->modifier.type_ref;
      } else if (type->kind == RDI_TypeKind_Bitfield) {
        type->bitfield.value_type = *(RDIB_Type **)type->bitfield.value_type;
      } else if (type->kind == RDI_TypeKindExt_Params) {
        for (U64 i = 0; i < type->params.count; ++i) {
          type->params.types[i] = *(RDIB_Type **)type->params.types[i];
        }
      } else if (type->kind == RDI_TypeKindExt_Members) {
        for (RDIB_UDTMember *member = type->members.list.first; member != 0; member = member->next) {
          switch (member->kind) {
          case RDI_MemberKind_NULL: break;
          case RDI_MemberKind_DataField: {
            member->data_field.type_ref = *(RDIB_Type **)member->data_field.type_ref;
          } break;
          case RDI_MemberKind_StaticData: {
            member->static_data.type_ref = *(RDIB_Type **)member->static_data.type_ref;
          } break;
          case RDI_MemberKind_Method: {
            member->method.type_ref = *(RDIB_Type **)member->method.type_ref;
          } break;
          case RDI_MemberKind_NestedType: {
            member->nested_type.type_ref = *(RDIB_Type **)member->nested_type.type_ref;
          } break;
          case RDI_MemberKind_Base: {
            member->base_class.type_ref = *(RDIB_Type **)member->base_class.type_ref;
          } break;
          case RDI_MemberKind_VirtualBase: {
            member->virtual_base_class.type_ref = *(RDIB_Type **)member->virtual_base_class.type_ref;
          } break;
          case RDI_MemberKindExt_MemberListPointer: {
            member->member_list_pointer = *(RDIB_Type **)member->member_list_pointer;
          } break;
#if 0
          case RDI_MemberKind_Enumerate: {
            // no types
          } break;
#endif
          default: InvalidPath;
          }
        }
      }
    }
  }
}

internal U64
rdib_sizeof_type(RDIB_Type *type)
{
  U64 size = 0;
  if (RDI_TypeKind_FirstBuiltIn <= type->kind && type->kind < RDI_TypeKind_LastBuiltIn) {
    size = type->builtin.size;
  } else if (type->kind == RDI_TypeKind_Modifier) {
    size = rdib_sizeof_type(type->modifier.type_ref);
  } else if (type->kind == RDI_TypeKind_Ptr || type->kind == RDI_TypeKind_LRef || type->kind == RDI_TypeKind_RRef) {
    size = type->ptr.size;
  } else if (type->kind == RDI_TypeKind_Struct || type->kind == RDI_TypeKind_Class ||
             type->kind == RDI_TypeKind_IncompleteStruct || type->kind == RDI_TypeKind_IncompleteClass) {
    size = type->udt.struct_type.size;
  } else if (type->kind == RDI_TypeKind_Union || type->kind == RDI_TypeKind_IncompleteUnion) {
    size = type->udt.union_type.size;
  } else if (type->kind == RDI_TypeKind_Enum || type->kind == RDI_TypeKind_IncompleteEnum) {
    size = rdib_sizeof_type(type->udt.enum_type.base_type);
  } else if (type->kind == RDI_TypeKind_Bitfield) {
    size = rdib_sizeof_type(type->bitfield.value_type);
  } else if (type->kind == RDI_TypeKind_Array) {
    size = type->array.size;
  } else {
    Assert(!"error: type doens't have a size");
  }
  return size;
}

internal U64
rdib_count_members_deep(RDIB_Type *type)
{
  U64 member_count = 0;
  for (RDIB_UDTMember *member = type->members.list.first; member != 0; member = member->next) {
    if (member->kind == RDI_MemberKindExt_MemberListPointer) {
      member_count += rdib_count_members_deep(member->member_list_pointer);
    } else {
      member_count += 1;
    }
  }
  return member_count;
}

internal
THREAD_POOL_TASK_FUNC(rdib_type_stats_task)
{
  ProfBeginFunction();

  RDIB_TypeStatsTask *task  = raw_task;
  RDIB_TypeChunk     *chunk = task->chunks[task_id];

  for (U64 itype = 0; itype < chunk->count; ++itype) {
    RDIB_Type *type = chunk->v + itype;

    if (type->kind == RDI_TypeKind_Class || type->kind == RDI_TypeKind_Struct || type->kind == RDI_TypeKind_Union || type->kind == RDI_TypeKind_Enum) {
      task->type_stats->udt_counts[task_id] += 1;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_concat_members_task)
{
  ProfBeginFunction();
  RDIB_MembersTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_TypeChunk *chunk = task->type_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Type          *type = &chunk->v[i];
      RDIB_UDTMemberList  acc  = {0};

      for (RDIB_Type *curr = type; ;) {
        // concat members
        rdib_udt_member_list_concat_in_place(&acc, &curr->members.list);

        // does this type continue member list?
        if (acc.count == 0 || acc.last->kind != RDI_MemberKindExt_MemberListPointer) {
          break;
        }

        // remove member list pointer
        RDIB_UDTMember *continuation = acc.last;
        SLLQueuePop(acc.first, acc.last);
        --acc.count;

        // advance to next type
        curr = continuation->member_list_pointer;

        // other types should not reference any part of member list except for head type.
        Assert(curr->kind == RDI_TypeKindExt_Members);
        curr->kind = RDI_TypeKind_NULL;
      }

      // update member list
      type->members.list = acc;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_count_head_members_task)
{
  ProfBeginFunction();
  RDIB_MembersTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_TypeChunk *chunk = task->type_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Type *type = &chunk->v[i];
      if (type->kind == RDI_TypeKindExt_Members) {
        task->counts[task_id] += type->members.list.count;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_assign_head_member_indices_task)
{
  ProfBeginFunction();
  RDIB_MembersTask *task = raw_task;
  U64 cursor = task->offsets[task_id];
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_TypeChunk *chunk = task->type_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Type *type = &chunk->v[i];
      if (type->kind == RDI_TypeKindExt_Members) {
        type->members.first_member_idx = cursor;
        cursor += type->members.list.count;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_udt_members_task)
{
  ProfBeginFunction();
  RDIB_MembersTask *task  = raw_task;
  RDIB_TypeChunk   *chunk = task->type_chunks[task_id];
  for (U64 i = 0; i < chunk->count; ++i) {
    RDIB_Type *type = chunk->v + i;
    Assert(type->kind == RDI_TypeKindExt_Members);

    U64 member_idx = 0;
    for (RDIB_UDTMember *src = type->members.list.first; src != 0; src = src->next, ++member_idx) {
      U64        idx  = type->members.first_member_idx + member_idx;
      RDI_Member *dst = &task->udt_members_rdi[idx];

      switch (src->kind) {
      case RDI_MemberKind_NULL: {
        MemoryZeroStruct(dst);
      } break;
      case RDI_MemberKind_DataField: {
        dst->kind            = RDI_MemberKind_DataField;
        dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->data_field.name);
        dst->type_idx        = rdib_idx_from_type(src->data_field.type_ref);
        dst->off             = src->data_field.offset;
      } break;
      case RDI_MemberKind_StaticData: {
        dst->kind            = RDI_MemberKind_StaticData;
        dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->static_data.name);
        dst->type_idx        = rdib_idx_from_type(src->static_data.type_ref);
      } break;
      case RDI_MemberKind_Method:
      case RDI_MemberKind_StaticMethod:
      case RDI_MemberKind_VirtualMethod: {
        dst->kind            = src->kind;
        dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->method.name);
        dst->type_idx        = rdib_idx_from_type(src->method.type_ref);
        dst->off             = src->method.vftable_offset;
      } break;
      case RDI_MemberKind_Base: {
        dst->kind            = RDI_MemberKind_Base;
        dst->name_string_idx = 0;
        dst->type_idx        = rdib_idx_from_type(src->base_class.type_ref);
        dst->off             = src->base_class.offset;
      } break;
      case RDI_MemberKind_VirtualBase: {
        dst->kind            = RDI_MemberKind_VirtualBase;
        dst->name_string_idx = 0;
        dst->type_idx        = rdib_idx_from_type(src->virtual_base_class.type_ref);
        dst->off             = 0; // TODO: ???
      } break;
      case RDI_MemberKind_NestedType: {
        dst->kind            = RDI_MemberKind_NestedType;
        dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->nested_type.name);
        dst->type_idx        = rdib_idx_from_type(src->nested_type.type_ref);
        dst->off             = 0;
      } break;
      case RDI_MemberKindExt_MemberListPointer: {
        InvalidPath;
      } break;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_enum_members_task)
{
  ProfBeginFunction();
  RDIB_MembersTask *task = raw_task;
  RDIB_TypeChunk   *chunk = task->type_chunks[task_id];
  for (U64 i = 0; i < chunk->count; ++i) {
    RDIB_Type *type = chunk->v + i;

    if (type->kind != RDI_TypeKindExt_Members) continue;

    U64 member_idx = 0;
    for (RDIB_UDTMember *src = type->members.list.first; src != 0; src = src->next, ++member_idx) {
      U64            idx   = type->members.first_member_idx + member_idx;
      RDI_EnumMember *dst  = &task->enum_members_rdi[idx];
      dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->enumerate.name);
      dst->val             = src->enumerate.value;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_udts_task)
{
  ProfBeginFunction();
  RDIB_UserDefinesTask *task = raw_task;

  U64             ichunk     = task_id;
  RDIB_TypeChunk *chunk      = task->type_chunks[ichunk];
  U64             udt_cursor = task->udt_base_idx[ichunk];
  U64             udt_cap    = task->type_stats.udt_counts[ichunk];

  for (U64 i = 0; i < chunk->count; ++i) {
    RDIB_Type *type = &chunk->v[i];

    if (RDI_IsCompleteUserDefinedTypeKind(type->kind)) {
      RDIB_Type *members_type = type->udt.members;

      // assign UDT idx
      type->udt.udt_idx = udt_cursor;

      // fill out struct/class UDT
      Assert(udt_cursor < task->udt_base_idx[ichunk] + udt_cap);
      RDI_UDT *udt       = &task->udts[udt_cursor++];
      udt->self_type_idx = rdib_idx_from_type(type);
      udt->flags         = type->kind == RDI_TypeKind_Enum ? RDI_UDTFlag_EnumMembers : 0;
      if (members_type->members.list.count > 0) {
        udt->member_first = members_type->members.first_member_idx;
        udt->member_count = members_type->members.list.count;
      } else {
        udt->member_first = 0;
        udt->member_count = 0;
      }
      udt->file_idx      = 0;
      udt->line          = 0;
      udt->col           = 0;
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_type_nodes_task)
{
  Temp scratch = scratch_begin(0, 0);

  U64                 ichunk = task_id;
  RDIB_TypeNodesTask *task   = raw_task;
  RDIB_TypeChunk     *chunk  = task->type_chunks[ichunk];

  for (U64 itype = 0; itype < chunk->count; ++itype) {
    RDIB_Type *src = &chunk->v[itype];
    U64 dst_idx = rdib_idx_from_type(src);
    RDI_TypeNode *dst = &task->type_nodes[dst_idx];

    if (src->kind == RDI_TypeKind_NULL) {
      MemoryZeroStruct(dst);
      dst->kind = RDI_TypeKind_NULL;
    } else if (RDI_TypeKind_FirstBuiltIn <= src->kind && src->kind <= RDI_TypeKind_LastBuiltIn) {
      dst->kind                     = src->kind;
      dst->flags                    = 0;
      dst->byte_size                = src->builtin.size;
      dst->built_in.name_string_idx = rdib_idx_from_string_map(task->string_map, src->builtin.name);
    } else if (src->kind == RDI_TypeKind_Modifier) {
      dst->kind                        = RDI_TypeKind_Modifier;
      dst->byte_size                   = rdib_sizeof_type(src->modifier.type_ref);
      dst->flags                       = src->modifier.flags;
      dst->constructed.direct_type_idx = rdib_idx_from_type(src->modifier.type_ref);
    } else if (src->kind == RDI_TypeKind_Ptr || src->kind == RDI_TypeKind_LRef || src->kind == RDI_TypeKind_RRef) {
      dst->kind                        = src->kind;
      dst->byte_size                   = src->ptr.size;
      dst->flags                       = 0;
      dst->constructed.direct_type_idx = rdib_idx_from_type(src->ptr.type_ref);
    } else if (src->kind == RDI_TypeKind_Method) {
      RDIB_Type *params_type = src->method.params_type;
      Assert(params_type->kind == RDI_TypeKindExt_Params);
      RDIB_IndexRunBucket *param_idx_run = task->idx_run_map->buckets[src->method.param_idx_run_bucket_idx];

      dst->kind                            = RDI_TypeKind_Method;
      dst->flags                           = 0;
      dst->byte_size                       = 0;
      dst->constructed.direct_type_idx     = rdib_idx_from_type(src->method.return_type);
      dst->constructed.count               = param_idx_run->indices.count;
      dst->constructed.param_idx_run_first = param_idx_run->index_in_output_array;
    } else if (src->kind == RDI_TypeKindExt_StaticMethod) {
      RDIB_Type *params_type = src->static_method.params_type;
      Assert(params_type->kind == RDI_TypeKindExt_Params);
      RDIB_IndexRunBucket *param_idx_run = task->idx_run_map->buckets[src->static_method.param_idx_run_bucket_idx];

      dst->kind                            = RDI_TypeKind_Method;
      dst->flags                           = 0;
      dst->byte_size                       = 0;
      dst->constructed.direct_type_idx     = rdib_idx_from_type(src->static_method.return_type);
      dst->constructed.count               = param_idx_run->indices.count;
      dst->constructed.param_idx_run_first = param_idx_run->index_in_output_array;
    } else if (src->kind == RDI_TypeKind_Function) {
      RDIB_Type *params_type = src->func.params_type;
      Assert(params_type->kind == RDI_TypeKindExt_Params);
      RDIB_IndexRunBucket *param_idx_run = task->idx_run_map->buckets[src->func.param_idx_run_bucket_idx];

      dst->kind                            = RDI_TypeKind_Function;
      dst->flags                           = 0;
      dst->byte_size                       = 0;
      dst->constructed.direct_type_idx     = rdib_idx_from_type(src->func.return_type);
      dst->constructed.count               = param_idx_run->indices.count;
      dst->constructed.param_idx_run_first = param_idx_run->index_in_output_array;
    } else if (src->kind == RDI_TypeKind_Array) {
      U64 entry_size = rdib_size_from_type(src->array.entry_type);
      U64 array_size = src->array.size;
      U64 array_count = entry_size > 0 ? array_size / entry_size : 0;

      dst->kind                        = src->kind;
      dst->flags                       = 0;
      dst->byte_size                   = array_size;
      dst->constructed.direct_type_idx = rdib_idx_from_type(src->array.entry_type);
      dst->constructed.count           = array_count;
    } else if (src->kind == RDI_TypeKind_Bitfield) {
      dst->kind                     = RDI_TypeKind_Bitfield;
      dst->flags                    = 0;
      dst->byte_size                = rdib_sizeof_type(src->bitfield.value_type);
      dst->bitfield.direct_type_idx = rdib_idx_from_type(src->bitfield.value_type); 
      dst->bitfield.off             = src->bitfield.off;
      dst->bitfield.size            = src->bitfield.count;
    } else if (src->kind == RDI_TypeKind_Struct || src->kind == RDI_TypeKind_Class ||
               src->kind == RDI_TypeKind_IncompleteStruct || src->kind == RDI_TypeKind_IncompleteClass) {
      dst->kind                         = src->kind;
      dst->flags                        = 0;
      dst->byte_size                    = src->udt.struct_type.size;
      dst->user_defined.name_string_idx = rdib_idx_from_string_map(task->string_map, src->udt.name);
      dst->user_defined.udt_idx         = src->udt.udt_idx;
      dst->user_defined.direct_type_idx = 0;
    } else if (src->kind == RDI_TypeKind_Union || src->kind == RDI_TypeKind_IncompleteUnion) {
      dst->kind                         = src->kind;
      dst->flags                        = 0;
      dst->byte_size                    = src->udt.union_type.size;
      dst->user_defined.name_string_idx = rdib_idx_from_string_map(task->string_map, src->udt.name);
      dst->user_defined.udt_idx         = src->udt.udt_idx;
      dst->user_defined.direct_type_idx = 0;
    } else if (src->kind == RDI_TypeKind_Enum || src->kind == RDI_TypeKind_IncompleteEnum) {
      dst->kind                         = RDI_TypeKind_Enum;
      dst->flags                        = 0;
      dst->byte_size                    = rdib_size_from_type(src->udt.enum_type.base_type);
      dst->user_defined.name_string_idx = rdib_idx_from_string_map(task->string_map, src->udt.name);
      dst->user_defined.udt_idx         = src->udt.udt_idx;
      dst->user_defined.direct_type_idx = rdib_idx_from_type(src->udt.enum_type.base_type);
    } else if (src->kind == RDI_TypeKind_Alias) {
      // TODO
      NotImplemented;
    } else if (src->kind == RDI_TypeKind_MemberPtr) {
      // TODO
      NotImplemented;
    } else if (src->kind == RDI_TypeKind_Variadic) {
      MemoryZeroStruct(dst);
      dst->kind = RDI_TypeKind_Variadic;
    } else if (src->kind == RDI_TypeKindExt_VirtualTable) {
      // TODO
      MemoryZeroStruct(dst);
      dst->kind = RDI_TypeKind_NULL;
    } else {
      InvalidPath;
    }
  }

  scratch_end(scratch);
}

internal void
rdib_data_sections_from_types(TP_Context            *tp,
                              Arena                 *arena,
                              RDIB_DataSectionList  *sect_list,
                              RDI_Arch               arch,
                              RDIB_StringMap        *string_map,
                              RDIB_IndexRunMap      *idx_run_map,
                              U64                    udt_member_chunk_count,
                              RDIB_TypeChunk       **udt_member_type_chunks,
                              U64                    enum_member_chunk_count,
                              RDIB_TypeChunk       **enum_member_type_chunks,
                              U64                    total_type_node_count,
                              U64                    type_chunk_count,
                              RDIB_TypeChunk       **type_chunks,
                              RDIB_TypeStats         type_stats)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  ProfBegin("UDT Members");
  U64         udt_member_count_rdi;
  RDI_Member *udt_members_rdi;
  {
    RDIB_MembersTask task = {0};

    ProfBegin("Concat");
    task.ranges      = tp_divide_work(scratch.arena, udt_member_chunk_count, tp->worker_count);
    task.type_chunks = udt_member_type_chunks;
    tp_for_parallel(tp, 0, tp->worker_count, rdib_concat_members_task, &task);
    ProfEnd();

    ProfBegin("Count");
    task.counts = push_array(scratch.arena, U64, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_count_head_members_task, &task);
    ProfEnd();

    ProfBegin("Assign Indices");
    task.offsets = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_assign_head_member_indices_task, &task);
    ProfEnd();

    udt_member_count_rdi = sum_array_u64(tp->worker_count, task.counts);
    udt_members_rdi      = push_array_no_zero(arena, RDI_Member, udt_member_count_rdi);

    ProfBegin("Fill");
    task.string_map      = string_map;
    task.udt_members_rdi = udt_members_rdi;
    tp_for_parallel(tp, 0, udt_member_chunk_count, rdib_fill_udt_members_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBegin("Enum Members");
  U64             enum_member_count_rdi;
  RDI_EnumMember *enum_members_rdi;
  {
    RDIB_MembersTask task = {0};

    ProfBegin("Concat");
    task.ranges      = tp_divide_work(scratch.arena, enum_member_chunk_count, tp->worker_count);
    task.type_chunks = enum_member_type_chunks;
    tp_for_parallel(tp, 0, tp->worker_count, rdib_concat_members_task, &task);
    ProfEnd();

    ProfBegin("Count");
    task.counts = push_array(scratch.arena, U64, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_count_head_members_task, &task);
    ProfEnd();

    ProfBegin("Assign Indices");
    task.offsets = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_assign_head_member_indices_task, &task);
    ProfEnd();

    enum_member_count_rdi = sum_array_u64(tp->worker_count, task.counts);
    enum_members_rdi      = push_array_no_zero(arena, RDI_EnumMember, enum_member_count_rdi);

    ProfBegin("Fill");
    task.string_map       = string_map;
    task.enum_members_rdi = enum_members_rdi;
    tp_for_parallel(tp, 0, enum_member_chunk_count, rdib_fill_enum_members_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBegin("Sum type stats");
  U64 total_udt_count = sum_array_u64(type_chunk_count, type_stats.udt_counts);
  ProfEnd();

  ProfBegin("Up front pushes");
  RDI_UDT      *udts       = push_array_no_zero(arena, RDI_UDT,      total_udt_count      );
  RDI_TypeNode *type_nodes = push_array_no_zero(arena, RDI_TypeNode, total_type_node_count);
  ProfEnd();

  ProfBegin("Fill out UDTs");
  RDIB_UserDefinesTask udts_task = {0};
  udts_task.type_chunks          = type_chunks;
  udts_task.type_stats           = type_stats;
  udts_task.udt_base_idx         = offsets_from_counts_array_u64(scratch.arena, type_stats.udt_counts, type_chunk_count);
  udts_task.udts                 = udts;
  tp_for_parallel(tp, 0, type_chunk_count, rdib_fill_udts_task, &udts_task);
  ProfEnd();

  ProfBegin("Fill out type nodes");
  RDIB_TypeNodesTask type_nodes_task = {0};
  type_nodes_task.addr_size          = rdi_addr_size_from_arch(arch);
  type_nodes_task.string_map         = string_map;
  type_nodes_task.idx_run_map        = idx_run_map;
  type_nodes_task.type_chunks        = type_chunks;
  type_nodes_task.type_stats         = type_stats;
  type_nodes_task.type_nodes         = type_nodes;
  tp_for_parallel(tp, 0, type_chunk_count, rdib_type_nodes_task, &type_nodes_task);
  ProfEnd();

  RDIB_DataSection udt_member_sect  = { .tag = RDI_SectionKind_Members     };
  RDIB_DataSection enum_member_sect = { .tag = RDI_SectionKind_EnumMembers };
  RDIB_DataSection udt_sect         = { .tag = RDI_SectionKind_UDTs        };
  RDIB_DataSection type_nodes_sect  = { .tag = RDI_SectionKind_TypeNodes   };

  str8_list_push(arena, &udt_member_sect.data,  str8_array(udt_members_rdi,  udt_member_count_rdi ));
  str8_list_push(arena, &enum_member_sect.data, str8_array(enum_members_rdi, enum_member_count_rdi));
  str8_list_push(arena, &udt_sect.data,         str8_array(udts,             total_udt_count      ));
  str8_list_push(arena, &type_nodes_sect.data,  str8_array(type_nodes,       total_type_node_count));

  rdib_data_section_list_push(arena, sect_list, enum_member_sect);
  rdib_data_section_list_push(arena, sect_list, udt_member_sect );
  rdib_data_section_list_push(arena, sect_list, udt_sect        );
  rdib_data_section_list_push(arena, sect_list, type_nodes_sect );

  scratch_end(scratch);
  ProfEnd();
}

////////////////////////////////

internal RDIB_PathTree *
rdib_path_tree_init(Arena *arena, U64 list_count)
{
  RDIB_PathTree *tree = push_array(arena, RDIB_PathTree, 1);
  tree->root          = push_array(arena, RDIB_PathTreeNode, 1);
  tree->list_count    = list_count;
  tree->node_lists    = push_array(arena, RDIB_PathTreeNodeList, list_count);
  return tree;
}

internal void
rdib_path_tree_insert(Arena *arena, RDIB_PathTree *tree, String8 path, RDIB_SourceFile *src_file)
{
  Temp scratch = scratch_begin(&arena, 1);

  RDIB_PathTreeNode *curr_sub_path = tree->root;
  String8List        sub_paths     = str8_split_path(scratch.arena, path);
  str8_path_list_resolve_dots_in_place(&sub_paths, path_style_from_str8(path));

  for (String8Node *n = sub_paths.first; n != 0; n = n->next) {
    RDIB_PathTreeNode *sub_child;

    // is there directory or file defined on this level?
    for (sub_child = curr_sub_path->first_child; sub_child != 0; sub_child = sub_child->next_sibling) {
      if (str8_match(sub_child->sub_path, n->string, 0)) {
        break;
      }
    }

    // new directory/file
    if (sub_child == 0) {
      sub_child           = push_array(arena, RDIB_PathTreeNode, 1);
      sub_child->node_idx = tree->node_count;
      sub_child->parent   = curr_sub_path;
      sub_child->sub_path = n->string;
      sub_child->src_file = 0;
      SLLQueuePush_N(curr_sub_path->first_child, curr_sub_path->last_child, sub_child, next_sibling);
      ++tree->node_count;

      // last node, insert file
      if (n->next == 0) {
        sub_child->src_file = src_file;
      }

      // HACK: setup node list per thread for serialization step
      U64 list_idx = tree->next_list_idx % tree->list_count;
      SLLQueuePush_N(tree->node_lists[list_idx].first, tree->node_lists[list_idx].last, sub_child, next_order);
      ++tree->next_list_idx;
    }

    // descend to sub node
    curr_sub_path = sub_child;
  }

  scratch_end(scratch);
}

internal U32
rdib_idx_from_path_tree(RDIB_PathTree *tree, String8 path)
{
  Temp scratch = scratch_begin(0,0);

  // redirect to special nil string
  if (path.size == 0) {
    path = RDIB_PATH_TREE_NIL_STRING;
  }

  // begin traverse from tree root
  RDIB_PathTreeNode *curr_sub_path = tree->root;

  // split path & resolve dots
  String8List sub_paths = str8_split_path(scratch.arena, path);
  str8_path_list_resolve_dots_in_place(&sub_paths, path_style_from_str8(path));

  for (String8Node *n = sub_paths.first; n != 0; n = n->next) {
    // scan children sub-path match
    RDIB_PathTreeNode *sub_child;
    for (sub_child = curr_sub_path->first_child; sub_child != 0; sub_child = sub_child->next_sibling) {
      if (str8_match(sub_child->sub_path, n->string, 0)) {
        break;
      }
    }

    // found match?
    if (sub_child == 0) {
      break;
    }

    // descend to sub directory
    curr_sub_path = sub_child;
  }

  // did we find source file?
  U64 idx = 0;
  if (curr_sub_path != 0 && curr_sub_path->src_file != 0) {
    idx = curr_sub_path->node_idx;
  } else {
    Assert(!"unable to find source file path");
  }

  scratch_end(scratch);
  return safe_cast_u32(idx);
}


////////////////////////////////

internal U64
rdib_string_map_hash(String8 string)
{
  XXH64_hash_t hash64 = XXH3_64bits(string.str, string.size);
  return hash64;
}

internal RDIB_StringMap *
rdib_init_string_map(Arena *arena, U64 cap)
{
  RDIB_StringMap *string_map = push_array(arena, RDIB_StringMap, 1);
  string_map->cap            = (U64)((F64)cap * 1.3);
  string_map->buckets        = push_array(arena, RDIB_StringMapBucket *, string_map->cap);
  return string_map;
}

internal U32
rdib_idx_from_string_map(RDIB_StringMap *string_map, String8 string)
{
  U64 hash     = rdib_string_map_hash(string);
  U64 best_idx = hash % string_map->cap;
  U64 idx      = best_idx;

  do {
    RDIB_StringMapBucket *bucket = string_map->buckets[idx];

    if (bucket == 0) {
      break;
    }

    if (str8_match(bucket->string, string, 0)) {
      return safe_cast_u32(bucket->idx);
    }

    idx = (idx + 1) % string_map->cap;
  } while (idx != best_idx);

  Assert(!"incomplete string map");
  return max_U32;
}

internal RDIB_StringMapBucket *
rdib_string_map_insert_or_update(RDIB_StringMapBucket **buckets, U64 cap, U64 hash, RDIB_StringMapBucket *new_bucket, RDIB_StringMapUpdateFunc *update_func)
{
  RDIB_StringMapBucket *result                         = 0;
  B32                   was_bucket_inserted_or_updated = 0;

  U64 best_idx = hash % cap;
  U64 idx      = best_idx;

  do {
    retry:;
    RDIB_StringMapBucket *curr_bucket = buckets[idx];

    if (curr_bucket == 0) {
      RDIB_StringMapBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&buckets[idx], new_bucket, curr_bucket);

      if (compare_bucket == curr_bucket) {
        // success, bucket was inserted
        was_bucket_inserted_or_updated = 1;
        break;
      }

      // another thread took the bucket...
      goto retry;
    } else if (str8_match(curr_bucket->string, new_bucket->string, 0)) {
      if (curr_bucket->sorter.v <= new_bucket->sorter.v) {
        if (new_bucket->raw_values != 0) {
          void_node_concat_atomic(&curr_bucket->raw_values, new_bucket->raw_values);
          new_bucket->raw_values = 0;
        }

        // recycle bucket
        result = new_bucket;

        // don't need to update, more recent leaf is in the bucket
        was_bucket_inserted_or_updated = 1;

        break;
      }

      if (new_bucket->raw_values) {
        new_bucket->raw_values->next = buckets[idx]->raw_values;
      }

      RDIB_StringMapBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&buckets[idx], new_bucket, curr_bucket);

      if (compare_bucket == curr_bucket) {

        // recycle bucket
        result = compare_bucket;

        // new bucket is in the hash table, exit
        was_bucket_inserted_or_updated = 1;
        break;
      }

      if (new_bucket->raw_values) {
        new_bucket->raw_values->next = 0;
      }

      // another thread took the bucket...
      goto retry;
    }

    // advance
    idx = (idx + 1) % cap;
  } while (idx != best_idx);

  // are there enough free buckets?
  Assert(was_bucket_inserted_or_updated);

  return result;
}

internal void
rdib_string_map_insert_item(Arena *arena, RDIB_CollectStringsTask *task, U64 task_id, String8 string, void *value)
{
  // do we have a free bucket?
  RDIB_StringMapBucket **bucket = &task->free_buckets[task_id];
  if (*bucket == 0) {
    *bucket = push_array(arena, RDIB_StringMapBucket, 1);
  }

  // fill out bucket
  (*bucket)->string     = string;
  (*bucket)->raw_values = value;
  (*bucket)->sorter.hi  = safe_cast_u32(task_id);
  (*bucket)->sorter.lo  = safe_cast_u32(task->element_indices[task_id]);

  // insert bucket into string map
  U64                   hash             = rdib_string_map_hash(string);
  RDIB_StringMapBucket *insert_or_update = rdib_string_map_insert_or_update(task->string_map->buckets, task->string_map->cap, hash, *bucket, task->string_map_update_func);

  // advance element index
  if (insert_or_update != *bucket) {
    ++task->element_indices[task_id];
  }

  // recycle bucket
  *bucket = insert_or_update;
}

internal
THREAD_POOL_TASK_FUNC(rdib_count_extant_buckets_string_map_task)
{
  ProfBeginFunction();
  RDIB_GetExtantBucketsStringMapTask *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    if (task->string_map->buckets[bucket_idx] != 0) {
      task->counts[task_id] += 1;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_get_extant_buckets_string_map_task)
{
  ProfBeginFunction();
  RDIB_GetExtantBucketsStringMapTask *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min, cursor = task->offsets[task_id]; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_StringMapBucket *bucket = task->string_map->buckets[bucket_idx];
    if (bucket != 0) {
      task->result[cursor] = bucket;
      ++cursor;
    }
  }
  ProfEnd();
}

internal RDIB_StringMapBucket **
rdib_extant_buckets_from_string_map(TP_Context *tp, Arena *arena, RDIB_StringMap *string_map, U64 *bucket_count_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  RDIB_GetExtantBucketsStringMapTask task = {0};
  task.string_map = string_map;

  ProfBegin("Count Extant Buckets");
  task.counts = push_array(scratch.arena, U64, tp->worker_count);
  task.ranges = tp_divide_work(scratch.arena, string_map->cap, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_count_extant_buckets_string_map_task, &task);
  ProfEnd();

  *bucket_count_out = sum_array_u64(tp->worker_count, task.counts);

  ProfBegin("Copy Extant Buckets");
  task.offsets = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
  task.result  = push_array(arena, RDIB_StringMapBucket *, *bucket_count_out);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_get_extant_buckets_string_map_task, &task);
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return task.result;
}

internal
THREAD_POOL_TASK_FUNC(rdib_string_map_bucket_chunk_idx_histo_task)
{
  ProfBeginFunction();
  RDIB_StringMapRadixSort *task    = raw_task;
  Temp                     scratch = scratch_begin(0,0);

  U32 *range_histo = push_array(scratch.arena, U32, task->chunk_idx_opl);

  // count items per sorter
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_StringMapBucket *bucket = task->src[bucket_idx];
    U64 chunk_idx = bucket->sorter.hi;
    Assert(chunk_idx < task->chunk_idx_opl);
    ++range_histo[chunk_idx];
  }

  // add in per thread sorter counts
  for (U64 i = 0; i < task->chunk_idx_opl; ++i) {
    ins_atomic_u32_add_eval(&task->chunk_histo[i], range_histo[i]);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_string_map_radix_sort_chunk_idx_task)
{
  ProfBeginFunction();
  RDIB_StringMapRadixSort *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_StringMapBucket *bucket    = task->src[bucket_idx];
    U32                   chunk_idx = bucket->sorter.hi;
    U32                   dst_idx   = ins_atomic_u32_inc_eval(&task->chunk_offsets[chunk_idx]) - 1;
    task->dst[dst_idx] = bucket;
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_string_map_radix_sort_element_idx_task)
{
  ProfBeginFunction();
  RDIB_StringMapRadixSort *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    U64 range_lo = task->chunk_offsets[chunk_idx];
    U64 range_hi = task->chunk_offsets[chunk_idx] + task->chunk_histo[chunk_idx];

    ProfBegin("Zero out Histogram");
    U32 histo_bot[1 << 10]; MemoryZeroArray(histo_bot);
    U32 histo_mid[1 << 11]; MemoryZeroArray(histo_mid);
    U32 histo_top[1 << 11]; MemoryZeroArray(histo_top);
    ProfEnd();

    ProfBegin("Element Histogram");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_StringMapBucket *elem = task->dst[i];
      U32 elem_idx  = elem->sorter.lo;
      U32 digit_bot = (elem_idx >>  0) % ArrayCount(histo_bot);
      U32 digit_mid = (elem_idx >> 10) % ArrayCount(histo_mid);
      U32 digit_top = (elem_idx >> 21) % ArrayCount(histo_top);
      histo_bot[digit_bot] += 1;
      histo_mid[digit_mid] += 1;
      histo_top[digit_top] += 1;
    }
    ProfEnd();

    ProfBegin("Histogram Counts -> Offsets");
    counts_to_offsets_array_u32(ArrayCount(histo_bot), &histo_bot[0]);
    counts_to_offsets_array_u32(ArrayCount(histo_mid), &histo_mid[0]);
    counts_to_offsets_array_u32(ArrayCount(histo_top), &histo_top[0]);
    ProfEnd();

    ProfBegin("Sort Bot");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_StringMapBucket *elem     = task->dst[i];
      U32                   elem_idx = elem->sorter.lo;
      U32                   digit    = (elem_idx >> 0) % ArrayCount(histo_bot);
      U32                   src_idx  = range_lo + histo_bot[digit]++;
      task->src[src_idx] = elem;
    }
    ProfEnd();

    ProfBegin("Sort Mid");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_StringMapBucket *elem     = task->src[i];
      U32                   elem_idx = elem->sorter.lo;
      U32                   digit    = (elem_idx >> 10) % ArrayCount(histo_mid);
      U32                   dst_idx  = range_lo + histo_mid[digit]++;
      task->dst[dst_idx] = elem;
    }
    ProfEnd();

    ProfBegin("Sort Top");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_StringMapBucket *elem     = task->dst[i];
      U32                   elem_idx = elem->sorter.lo;
      U32                   digit    = (elem_idx >> 21) % ArrayCount(histo_top);
      U32                   src_idx  = range_lo + histo_top[digit]++;
      task->src[src_idx] = elem;
    }
    ProfEnd();
  }

  ProfEnd();
}

internal void
rdib_string_map_sort_buckets(TP_Context *tp, RDIB_StringMapBucket **buckets, U64 bucket_count, U64 chunk_idx_opl)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  RDIB_StringMapRadixSort task = {0};
  task.chunk_idx_opl = chunk_idx_opl;
  task.ranges        = tp_divide_work(scratch.arena, bucket_count, tp->worker_count);
  task.src           = buckets;
  task.dst           = push_array_no_zero(scratch.arena, RDIB_StringMapBucket *, bucket_count);

  ProfBegin("Chunk Index Histogram");
  task.chunk_histo = push_array(scratch.arena, U32, chunk_idx_opl);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_string_map_bucket_chunk_idx_histo_task, &task);
  ProfEnd();

  // sort correctness check on chunk index
#if 0
  for (U64 i = 1; i < bucket_count; ++i) {
    RDIB_StringMapBucket *prev = buckets[i - 1];
    RDIB_StringMapBucket *curr = buckets[i + 0];
    U32 prev_chunk_idx = prev->sorter.hi;
    U32 curr_chunk_idx = curr->sorter.hi;
    AssertAlways(prev_chunk_idx <= curr_chunk_idx);
  }
#endif

  ProfBegin("Chunk Histo -> Offsets");
  task.chunk_offsets = offsets_from_counts_array_u32(scratch.arena, task.chunk_histo, chunk_idx_opl);
  ProfEnd();

  ProfBegin("Sort on chunk index");
  tp_for_parallel(tp, 0, tp->worker_count, rdib_string_map_radix_sort_chunk_idx_task, &task);
  ProfEnd();

  ProfBegin("Sort on element index");
  task.chunk_offsets = offsets_from_counts_array_u32(scratch.arena, task.chunk_histo, chunk_idx_opl);
  task.ranges        = tp_divide_work(scratch.arena, chunk_idx_opl, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_string_map_radix_sort_element_idx_task, &task);
  ProfEnd();

  // sort correctness check on element index
#if 0
  {
    for (U64 i = 1; i < bucket_count; ++i) {
      RDIB_StringMapBucket *prev = buckets[i - 1];
      RDIB_StringMapBucket *curr = buckets[i + 0];
      U32 prev_chunk_idx = prev->sorter.hi;
      U32 curr_chunk_idx = curr->sorter.hi;
      if (prev_chunk_idx == curr_chunk_idx) {
        U32 prev_elem_idx = prev->sorter.lo;
        U32 curr_elem_idx = curr->sorter.lo;
        AssertAlways(prev_elem_idx < curr_elem_idx);
      }
    }
  }
#endif

  scratch_end(scratch);
  ProfEnd();
}

internal void
rdib_string_map_assign_indices(RDIB_StringMapBucket **buckets, U64 bucket_count)
{
  ProfBeginFunction();
  for (U64 idx = 0; idx < bucket_count; ++idx) {
    buckets[idx]->idx = idx;
  }
  ProfEnd();
}

// Specialized Inserts

internal void
rdib_string_map_insert_string_table_item(Arena *arena, RDIB_CollectStringsTask *task, U64 task_id, String8 string)
{
  rdib_string_map_insert_item(arena, task, task_id, string, 0);
}

internal void
rdib_string_map_insert_name_map_item(Arena *arena, RDIB_CollectStringsTask *task, U64 task_id, String8 string, VoidNode *node)
{
  rdib_string_map_insert_item(arena, task, task_id, string, node);
}

RDIB_STRING_MAP_UPDATE_FUNC(rdib_string_map_update_null)
{
  // null update
}

RDIB_STRING_MAP_UPDATE_FUNC(rdib_string_map_update_concat_void_list_atomic)
{
  node->next = ins_atomic_ptr_eval_assign(head, node);
}

////////////////////////////////
// String Table Tasks

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_sects_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 sect_idx = task->ranges[task_id].min; sect_idx < task->ranges[task_id].max; ++sect_idx) {
    RDIB_BinarySection *sect = &task->sects[sect_idx];
    rdib_string_map_insert_string_table_item(arena, task, task_id, sect->name);
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_units_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_UnitChunk *chunk = task->units[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Unit *unit = &chunk->v[i];
      rdib_string_map_insert_string_table_item(arena, task, task_id, unit->unit_name);
      rdib_string_map_insert_string_table_item(arena, task, task_id, unit->compiler_name);
      rdib_string_map_insert_string_table_item(arena, task, task_id, unit->source_file);
      rdib_string_map_insert_string_table_item(arena, task, task_id, unit->object_file);
      rdib_string_map_insert_string_table_item(arena, task, task_id, unit->archive_file);
      rdib_string_map_insert_string_table_item(arena, task, task_id, unit->build_path);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_source_files_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_SourceFileChunk *chunk = task->src_file_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_SourceFile *src_file = chunk->v + i;
      rdib_string_map_insert_string_table_item(arena, task, task_id, src_file->normal_full_path);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_vars_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->vars[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Variable *var = &chunk->v[i];
      rdib_string_map_insert_string_table_item(arena, task, task_id, var->name);
      rdib_string_map_insert_string_table_item(arena, task, task_id, var->link_name);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_procs_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ProcedureChunk *chunk = task->procs[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Procedure *proc = &chunk->v[i];
      rdib_string_map_insert_string_table_item(arena, task, task_id, proc->name);
      rdib_string_map_insert_string_table_item(arena, task, task_id, proc->link_name);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_inline_sites_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_InlineSiteChunk *chunk = task->inline_sites[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_InlineSite *inline_site = &chunk->v[i];
      rdib_string_map_insert_string_table_item(arena, task, task_id, inline_site->name);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_udt_members_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_UDTMemberChunk *chunk = task->udt_members[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_UDTMember *udt_member = &chunk->v[i];
      switch (udt_member->kind) {
      case RDI_MemberKind_NULL            : break;
      case RDI_MemberKind_DataField       : rdib_string_map_insert_string_table_item(arena, task, task_id, udt_member->data_field.name ); break;
      case RDI_MemberKind_StaticData      : rdib_string_map_insert_string_table_item(arena, task, task_id, udt_member->static_data.name); break;
      case RDI_MemberKind_Method          : rdib_string_map_insert_string_table_item(arena, task, task_id, udt_member->method.name     ); break;
      case RDI_MemberKind_NestedType      : rdib_string_map_insert_string_table_item(arena, task, task_id, udt_member->nested_type.name); break;
      case RDI_MemberKind_Base            : break;
      case RDI_MemberKind_VirtualBase     : break;
      //case RDI_MemberKind_Enumerate       : rdib_string_map_insert_string_table_item(arena, task, task_id, udt_member->enumerate.name); break;
      case RDI_MemberKindExt_MemberListPointer: break;
      default: InvalidPath;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_enum_members_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_UDTMemberChunk *chunk = task->enum_members[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      rdib_string_map_insert_string_table_item(arena, task, task_id, chunk->v[i].enumerate.name);
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_types_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_TypeChunk *chunk = task->types[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Type *type = &chunk->v[i];
      if (RDI_TypeKind_FirstBuiltIn <= type->kind && type->kind <= RDI_TypeKind_LastBuiltIn) {
        rdib_string_map_insert_string_table_item(arena, task, task_id, type->builtin.name);
      } else if (RDI_IsUserDefinedType(type->kind)) {
        rdib_string_map_insert_string_table_item(arena, task, task_id, type->udt.name);
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_collect_strings_path_nodes_task)
{
  ProfBeginFunction();
  RDIB_CollectStringsTask *task = raw_task;
  for (RDIB_PathTreeNode *n = task->path_node_lists[task_id].first; n != 0; n = n->next_order) {
    rdib_string_map_insert_string_table_item(arena, task, task_id, n->sub_path);
  }
  ProfEnd();
}

////////////////////////////////
// Name Map Tasks

internal
THREAD_POOL_TASK_FUNC(rdib_name_map_var_task)
{
  RDIB_CollectStringsTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->vars[chunk_idx];
    VoidNode           *nodes = push_array(arena, VoidNode, chunk->count);
    for (U64 var_idx = 0; var_idx < chunk->count; ++var_idx) {
      RDIB_Variable *n = &chunk->v[var_idx];
      nodes[var_idx].v = n;
      rdib_string_map_insert_name_map_item(arena, task, task_id, n->name, &nodes[var_idx]);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(rdib_name_map_var_link_name_task)
{
  RDIB_CollectStringsTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->vars[chunk_idx];
    VoidNode           *nodes = push_array(arena, VoidNode, chunk->count);
    for (U64 var_idx = 0; var_idx < chunk->count; ++var_idx) {
      RDIB_Variable *n = &chunk->v[var_idx];
      nodes[var_idx].v = n;
      rdib_string_map_insert_name_map_item(arena, task, task_id, n->link_name, &nodes[var_idx]);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(rdib_name_map_procedure_task)
{
  RDIB_CollectStringsTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ProcedureChunk *chunk = task->procs[chunk_idx];
    VoidNode *nodes = push_array(arena, VoidNode, chunk->count);
    for (U64 proc_idx = 0; proc_idx < chunk->count; ++proc_idx) {
      RDIB_Procedure *n = &chunk->v[proc_idx];
      nodes[proc_idx].v = n;
      rdib_string_map_insert_name_map_item(arena, task, task_id, n->name, &nodes[proc_idx]);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(rdib_name_map_procedures_link_name_task)
{
  RDIB_CollectStringsTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ProcedureChunk *chunk = task->procs[chunk_idx];
    VoidNode            *nodes = push_array(arena, VoidNode, chunk->count);
    for (U64 proc_idx = 0; proc_idx < chunk->count; ++proc_idx) {
      RDIB_Procedure *n = &chunk->v[proc_idx];
      nodes[proc_idx].v = n;
      rdib_string_map_insert_name_map_item(arena, task, task_id, n->link_name, &nodes[proc_idx]);
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(rdib_name_map_types_task)
{
  RDIB_CollectStringsTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_TypeChunk *chunk = task->types[chunk_idx];
    VoidNode *nodes       = push_array(arena, VoidNode, chunk->count);
    VoidNode *node_cursor = nodes;
    for (U64 type_idx = 0; type_idx < chunk->count; ++type_idx) {
      RDIB_Type *type = &chunk->v[type_idx];
      node_cursor->v = type;

      if (RDI_IsUserDefinedType(type->kind)) {
        rdib_string_map_insert_name_map_item(arena, task, task_id, type->udt.name, node_cursor);
        ++node_cursor;
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(rdib_name_map_normal_paths_task)
{
  RDIB_CollectStringsTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_SourceFileChunk *chunk = task->src_file_chunks[chunk_idx];
    VoidNode *nodes             = push_array(arena, VoidNode, chunk->count);
    VoidNode *node_cursor       = nodes;
    for (U64 i = 0; i < chunk->count; ++i, ++node_cursor) {
      node_cursor->v = &chunk->v[i];
      rdib_string_map_insert_name_map_item(arena, task, task_id, chunk->v[i].normal_full_path, node_cursor);
    }
  }
}

////////////////////////////////
// Index Run Map

internal U64
rdib_index_run_hash(U32 count, U32 *idxs)
{
  XXH64_hash_t hash64 = XXH3_64bits(idxs, count * sizeof(idxs[0]));
  return hash64;
}

internal RDIB_IndexRunMap *
rdib_init_index_run_map(Arena *arena, U64 cap)
{
  ProfBeginFunction();
  RDIB_IndexRunMap *map = push_array(arena, RDIB_IndexRunMap, 1);
  map->cap              = cap;
  map->buckets          = push_array(arena, RDIB_IndexRunBucket *, cap);
  ProfEnd();
  return map;
}

internal RDIB_IndexRunBucket *
rdib_index_run_map_insert_or_update(Arena *arena, RDIB_IndexRunBucket **buckets, U64 cap, U64 hash, RDIB_IndexRunBucket *new_bucket, U64 *bucket_idx_out)
{
  B32 was_bucket_inserted_or_updated = 0;

  RDIB_IndexRunBucket *result = 0;

  U64 best_idx = hash % cap;
  U64 idx      = best_idx;

  do {
    retry:;
    RDIB_IndexRunBucket *curr_bucket = buckets[idx];

    if (curr_bucket == 0) {
      RDIB_IndexRunBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&buckets[idx], new_bucket, curr_bucket);

      if (compare_bucket == curr_bucket) {
        // success, bucket was inserted
        was_bucket_inserted_or_updated = 1;
        break;
      }

      // another thread took the bucket...
      goto retry;
    } else if (u32_array_compare(curr_bucket->indices, new_bucket->indices)) {
      if (curr_bucket->sorter.v <= new_bucket->sorter.v) {
        // recycle bucket
        result = new_bucket;

        // don't need to update, more recent leaf is in the bucket
        was_bucket_inserted_or_updated = 1;
        break;
      }

      RDIB_IndexRunBucket *compare_bucket = ins_atomic_ptr_eval_cond_assign(&buckets[idx], new_bucket, curr_bucket);
      if (compare_bucket == curr_bucket) {
        // recycle bucket
        result = compare_bucket;

        // new bucket is in the hash table, exit
        was_bucket_inserted_or_updated = 1;
        break;
      }

      // another thread took the bucket...
      goto retry;
    }

    // advance
    idx = (idx + 1) % cap;
  } while (idx != best_idx);

  // are there enough free buckets?
  Assert(was_bucket_inserted_or_updated);

  // output bucket index
  *bucket_idx_out = idx;

  return result;
}

internal U32
rdib_idx_run_from_bucket_idx(RDIB_IndexRunMap *map, U64 bucket_idx)
{
  RDIB_IndexRunBucket *bucket = map->buckets[bucket_idx];
  U32 idx_run32 = safe_cast_u32(bucket->index_in_output_array);
  return idx_run32;
}

internal
THREAD_POOL_TASK_FUNC(rdib_count_extant_buckets_index_run_map_task)
{
  ProfBeginFunction();
  RDIB_GetExtantBucketsIndexRunMapTask *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    if (task->idx_run_map->buckets[bucket_idx] != 0) {
      task->counts[task_id] += 1;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_get_extant_buckets_index_run_map_task)
{
  ProfBeginFunction();
  RDIB_GetExtantBucketsIndexRunMapTask *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min, cursor = task->offsets[task_id]; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_IndexRunBucket *bucket = task->idx_run_map->buckets[bucket_idx];
    if (bucket != 0) {
      task->result[cursor] = bucket;
      ++cursor;
    }
  }
  ProfEnd();
}

internal RDIB_IndexRunBucket **
rdib_extant_buckets_from_index_run_map(TP_Context *tp, Arena *arena, RDIB_IndexRunMap *idx_run_map, U64 *bucket_count_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  RDIB_GetExtantBucketsIndexRunMapTask task = {0};
  task.idx_run_map = idx_run_map;

  ProfBegin("Count Extant Buckets");
  task.counts = push_array(scratch.arena, U64, tp->worker_count);
  task.ranges = tp_divide_work(scratch.arena, idx_run_map->cap, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_count_extant_buckets_index_run_map_task, &task);
  ProfEnd();

  *bucket_count_out = sum_array_u64(tp->worker_count, task.counts);

  ProfBegin("Copy Extant Buckets");
  task.offsets = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
  task.result  = push_array(arena, RDIB_IndexRunBucket *, *bucket_count_out);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_get_extant_buckets_index_run_map_task, &task);
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return task.result;
}

internal
THREAD_POOL_TASK_FUNC(rdib_index_run_map_bucket_chunk_idx_histo_task)
{
  ProfBeginFunction();
  RDIB_IndexRunMapRadixSort *task    = raw_task;
  Temp                       scratch = scratch_begin(0,0);

  U32 *range_histo = push_array(scratch.arena, U32, task->chunk_idx_opl);

  // count items per sorter
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_IndexRunBucket *bucket = task->src[bucket_idx];
    U32 chunk_idx = bucket->sorter.hi;
    Assert(chunk_idx < task->chunk_idx_opl);
    ++range_histo[chunk_idx];
  }

  // add in per thread sorter counts
  for (U64 i = 0; i < task->chunk_idx_opl; ++i) {
    ins_atomic_u32_add_eval(&task->chunk_histo[i], range_histo[i]);
  }

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_index_run_map_radix_sort_chunk_idx_task)
{
  ProfBeginFunction();
  RDIB_IndexRunMapRadixSort *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_IndexRunBucket *bucket    = task->src[bucket_idx];
    U32                  chunk_idx = bucket->sorter.hi;
    U32                  dst_idx   = ins_atomic_u32_inc_eval(&task->chunk_offsets[chunk_idx]) - 1;
    task->dst[dst_idx] = bucket;
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_index_run_map_radix_sort_element_idx_task)
{
  ProfBeginFunction();
  RDIB_IndexRunMapRadixSort *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    U64 range_lo = task->chunk_offsets[chunk_idx];
    U64 range_hi = task->chunk_offsets[chunk_idx] + task->chunk_histo[chunk_idx];

    ProfBegin("Zero out Histogram");
    U32 histo_bot[1 << 10]; MemoryZeroArray(histo_bot);
    U32 histo_mid[1 << 11]; MemoryZeroArray(histo_mid);
    U32 histo_top[1 << 11]; MemoryZeroArray(histo_top);
    ProfEnd();

    ProfBegin("Element Histogram");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_IndexRunBucket *elem = task->dst[i];
      U32 elem_idx  = elem->sorter.lo;
      U32 digit_bot = (elem_idx >>  0) % ArrayCount(histo_bot);
      U32 digit_mid = (elem_idx >> 10) % ArrayCount(histo_mid);
      U32 digit_top = (elem_idx >> 21) % ArrayCount(histo_top);
      histo_bot[digit_bot] += 1;
      histo_mid[digit_mid] += 1;
      histo_top[digit_top] += 1;
    }
    ProfEnd();

    ProfBegin("Histogram Counts -> Offsets");
    counts_to_offsets_array_u32(ArrayCount(histo_bot), &histo_bot[0]);
    counts_to_offsets_array_u32(ArrayCount(histo_mid), &histo_mid[0]);
    counts_to_offsets_array_u32(ArrayCount(histo_top), &histo_top[0]);
    ProfEnd();

    ProfBegin("Sort Bot");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_IndexRunBucket *elem     = task->dst[i];
      U32                  elem_idx = elem->sorter.lo;
      U32                  digit    = (elem_idx >> 0) % ArrayCount(histo_bot);
      U32                  src_idx  = range_lo + histo_bot[digit]++;
      task->src[src_idx] = elem;
    }
    ProfEnd();

    ProfBegin("Sort Mid");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_IndexRunBucket *elem     = task->src[i];
      U32                  elem_idx = elem->sorter.lo;
      U32                  digit    = (elem_idx >> 10) % ArrayCount(histo_mid);
      U32                  dst_idx  = range_lo + histo_mid[digit]++;
      task->dst[dst_idx] = elem;
    }
    ProfEnd();

    ProfBegin("Sort Top");
    for (U64 i = range_lo; i < range_hi; ++i) {
      RDIB_IndexRunBucket *elem     = task->dst[i];
      U32                  elem_idx = elem->sorter.lo;
      U32                  digit    = (elem_idx >> 21) % ArrayCount(histo_top);
      U32                  src_idx  = range_lo + histo_top[digit]++;
      task->src[src_idx] = elem;
    }
    ProfEnd();
  }

  ProfEnd();
}

internal void
rdib_index_run_map_sort_buckets(TP_Context *tp, RDIB_IndexRunBucket **buckets, U64 bucket_count, U64 chunk_idx_opl)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  RDIB_IndexRunMapRadixSort task = {0};
  task.chunk_idx_opl = chunk_idx_opl;
  task.ranges        = tp_divide_work(scratch.arena, bucket_count, tp->worker_count);
  task.src           = buckets;
  task.dst           = push_array_no_zero(scratch.arena, RDIB_IndexRunBucket *, bucket_count);

  ProfBegin("Chunk Index Histogram");
  task.chunk_histo = push_array(scratch.arena, U32, chunk_idx_opl);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_index_run_map_bucket_chunk_idx_histo_task, &task);
  ProfEnd();

  // sort correctness check on chunk index
#if 0
  for (U64 i = 1; i < bucket_count; ++i) {
    RDIB_StringMapBucket *prev = buckets[i - 1];
    RDIB_StringMapBucket *curr = buckets[i + 0];
    U32 prev_chunk_idx = RDIB_StringMap_ChunkIdx32FromSorter(prev->sorter);
    U32 curr_chunk_idx = RDIB_StringMap_ChunkIdx32FromSorter(curr->sorter);
    AssertAlways(prev_chunk_idx <= curr_chunk_idx);
  }
#endif

  ProfBegin("Chunk Histo -> Offsets");
  task.chunk_offsets = offsets_from_counts_array_u32(scratch.arena, task.chunk_histo, chunk_idx_opl);
  ProfEnd();

  ProfBegin("Sort on chunk index");
  tp_for_parallel(tp, 0, tp->worker_count, rdib_index_run_map_radix_sort_chunk_idx_task, &task);
  ProfEnd();

  ProfBegin("Sort on element index");
  task.chunk_offsets = offsets_from_counts_array_u32(scratch.arena, task.chunk_histo, chunk_idx_opl);
  task.ranges        = tp_divide_work(scratch.arena, chunk_idx_opl, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_index_run_map_radix_sort_element_idx_task, &task);
  ProfEnd();

  // sort correctness check on element index
#if 0
  {
    for (U64 i = 1; i < bucket_count; ++i) {
      RDIB_IndexRunBucket *prev = buckets[i - 1];
      RDIB_IndexRunBucket *curr = buckets[i + 0];
      U32 prev_chunk_idx = prev->sorter.hi;
      U32 curr_chunk_idx = curr->sorter.hi;
      if (prev_chunk_idx == curr_chunk_idx) {
        U32 prev_elem_idx = prev->sorter.lo;
        U32 curr_elem_idx = curr->sorter.lo;
        AssertAlways(prev_elem_idx < curr_elem_idx);
      }
    }
  }
#endif

  scratch_end(scratch);
  ProfEnd();
}

internal void
rdib_index_run_map_assign_indices(RDIB_IndexRunBucket **buckets, U64 bucket_count)
{
  ProfBeginFunction();
  for (U64 bucket_idx = 0, cursor = 0; bucket_idx < bucket_count; ++bucket_idx) {
    buckets[bucket_idx]->index_in_output_array = cursor;
    cursor += buckets[bucket_idx]->indices.count;
  }
  ProfEnd();
}

// index run map specialization

internal U64
rdib_index_run_map_insert_item(Arena *arena, RDIB_BuildIndexRunsTask *task, U64 worker_id, U64 item_idx, U64 count, U32 *idxs)
{
  Assert(item_idx < max_U32);

  // do we have a free bucket?
  RDIB_IndexRunBucket *bucket = task->free_buckets[worker_id];
  if (bucket == 0) {
    bucket = push_array(arena, RDIB_IndexRunBucket, 1);
  }

  // fill out bucket
  bucket->indices.count = count;
  bucket->indices.v     = idxs;
  bucket->sorter.v      = task->sorter_idx << 32 | (U32)item_idx;

  // insert bucket
  U64                  hash       = rdib_index_run_hash(count, idxs);
  U64                  bucket_idx = max_U64;
  RDIB_IndexRunBucket *free_bucket = rdib_index_run_map_insert_or_update(arena,
                                                                         task->idx_run_map->buckets,
                                                                         task->idx_run_map->cap,
                                                                         hash,
                                                                         bucket,
                                                                         &bucket_idx);
  Assert(bucket_idx != max_U64);

  // recycle bucket
  task->free_buckets[worker_id] = free_bucket;

  return bucket_idx;
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_idx_runs_params_task)
{
  ProfBeginFunction();

  RDIB_BuildIndexRunsTask *task  = raw_task;
  RDIB_TypeChunk          *chunk = task->type_chunks[task_id];

  for (RDIB_Type *type = &chunk->v[0], *opl = chunk->v + chunk->count; type < opl; ++type) {
    if (type->kind == RDI_TypeKind_Function) {
      RDIB_Type *params = type->func.params_type;

      // pack params
      U64  type_index_count = params->params.count;
      U32 *type_indices     = push_array_no_zero(arena, U32, type_index_count);
      for (U64 param_idx = 0; param_idx < params->params.count; ++param_idx) {
        type_indices[param_idx] = rdib_idx_from_type(params->params.types[param_idx]);
      }

      // insert type indices
      U32 func_type_idx = rdib_idx_from_type(type);
      type->func.param_idx_run_bucket_idx = rdib_index_run_map_insert_item(arena, task, worker_id, func_type_idx, type_index_count, type_indices);
    } else if (type->kind == RDI_TypeKind_Method) {
      RDIB_Type *params = type->method.params_type;

      U64  type_index_count = params->params.count + 1;
      U32 *type_indices     = push_array_no_zero(arena, U32, type_index_count);
      U64  type_idx_cursor  = 0;

      // pack 'this' type
      type_indices[type_idx_cursor++] = rdib_idx_from_type(type->method.this_type);

      // pack params
      for (U64 param_idx = 0; param_idx < params->params.count; ++param_idx) {
        type_indices[type_idx_cursor++] = rdib_idx_from_type(params->params.types[param_idx]);
      }

      // insert type indices
      U32 method_type_idx = rdib_idx_from_type(type);
      type->method.param_idx_run_bucket_idx = rdib_index_run_map_insert_item(arena, task, worker_id, method_type_idx, type_index_count, type_indices);
    } else if (type->kind == RDI_TypeKindExt_StaticMethod) {
      RDIB_Type *params = type->static_method.params_type;

      U64  type_index_count = params->params.count + 1;
      U32 *type_indices     = push_array_no_zero(arena, U32, type_index_count);
      U64  type_idx_cursor  = 0;

      // static methods don't have 'this'
      type_indices[type_idx_cursor++] = 0;

      // pack params
      for (U64 param_idx = 0; param_idx < params->params.count; ++param_idx) {
        type_indices[type_idx_cursor++] = rdib_idx_from_type(params->params.types[param_idx]);
      }

      // insert type indices
      U32 static_method_type_idx = rdib_idx_from_type(type);
      type->static_method.param_idx_run_bucket_idx = rdib_index_run_map_insert_item(arena, task, worker_id, static_method_type_idx, type_index_count, type_indices);
    }
  }

  ProfEnd();
}

internal U32
rdib_idx_from_name_map_void_node(RDIB_BuildIndexRunsTask *task, VoidNode *node)
{
  U64 idx = 0;
  switch (task->name_map_kind) {
  case RDI_NameMapKind_NULL              : break;
  case RDI_NameMapKind_GlobalVariables   : idx = rdib_idx_from_variable   ((RDIB_Variable*   ) node); break;
  case RDI_NameMapKind_ThreadVariables   : idx = rdib_idx_from_variable   ((RDIB_Variable *  ) node); break;
  case RDI_NameMapKind_Procedures        : idx = rdib_idx_from_procedure  ((RDIB_Procedure * ) node); break;
  case RDI_NameMapKind_Types             : idx = rdib_idx_from_type       ((RDIB_Type *      ) node); break;
  case RDI_NameMapKind_LinkNameProcedures: idx = rdib_idx_from_procedure  ((RDIB_Procedure * ) node); break;
  case RDI_NameMapKind_NormalSourcePaths : idx = rdib_idx_from_source_file((RDIB_SourceFile *) node); break;
  default: InvalidPath;
  }
  U32 idx32 = safe_cast_u32(idx);
  return idx32;
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_idx_runs_name_map_buckets_task)
{
  ProfBeginFunction();

  RDIB_BuildIndexRunsTask *task = raw_task;

  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_StringMapBucket *bucket = task->name_map_buckets[bucket_idx];
    U64                   count  = void_list_count_nodes(bucket->raw_values);

    if (count > 1) {
      // build array of indices that point to name map respective arrays
      U32 *idxs = push_array_no_zero(arena, U32, count);
      {
        U64 curr_idx = 0;
        for (VoidNode *curr = bucket->raw_values; curr != 0; curr = curr->next, ++curr_idx) {
          idxs[curr_idx] = rdib_idx_from_name_map_void_node(task, curr->v);
        }
      }

      // make index array deterministic
      u32_array_sort(count, idxs); // TODO: we don't need to sort with one worker thread

      // :string_map_bucket_sorter_copy
      U64 idx_run_bucket_idx = rdib_index_run_map_insert_item(arena, task, worker_id, bucket_idx, count, idxs); // TODO: fix `idx` leak when we insert same runs
      
      // fill out bucket
      bucket->count              = count;
      bucket->idx_run_bucket_idx = idx_run_bucket_idx;
    } if (count == 1) {
      U32 match_idx = rdib_idx_from_name_map_void_node(task, bucket->raw_values->v);

      // fill out bucket
      bucket->count     = 1;
      bucket->match_idx = match_idx;
    }
  }

  ProfEnd();
}

////////////////////////////////

#if 0
internal U32
rdib_idx_from_params(RDIB_IndexRunMap *map, RDIB_Type *params)
{
  Assert(params->kind == RDI_TypeKindExt_Params);
  U32 idx = params->params.idx_run_bucket->index_in_output_array;
  return idx;
}
#endif

////////////////////////////////
// Data Sections

internal void
rdib_data_section_list_push_node(RDIB_DataSectionList *list, RDIB_DataSectionNode *node)
{
  SLLQueuePushCount(list, node);
}

internal RDIB_DataSectionNode *
rdib_data_section_list_push(Arena *arena, RDIB_DataSectionList *list, RDIB_DataSection v)
{
  RDIB_DataSectionNode *node = push_array(arena, RDIB_DataSectionNode, 1);
  node->v                    = v;
  rdib_data_section_list_push_node(list, node);
  return node;
}

internal void
rdib_data_section_list_concat_in_place(RDIB_DataSectionList *list, RDIB_DataSectionList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal void
rdib_data_sections_from_top_level_info(Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_TopLevelInfo *src)
{
  ProfBeginFunction();

  RDI_TopLevelInfo *dst         = push_array(arena, RDI_TopLevelInfo, 1);
  dst->arch                     = src->arch;
  dst->exe_name_string_idx      = rdib_idx_from_string_map(string_map, src->exe_name);
  dst->exe_hash                 = src->exe_hash;
  dst->voff_max                 = src->voff_max;
  dst->producer_name_string_idx = rdib_idx_from_string_map(string_map, src->producer_string);

  RDIB_DataSection sect = { .tag = RDI_SectionKind_TopLevelInfo };
  str8_list_push(arena, &sect.data, str8_struct(dst));
  rdib_data_section_list_push(arena, sect_list, sect);

  ProfEnd();
}

internal void
rdib_data_sections_from_binary_sections(Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_BinarySection *binary_sects, U64 binary_sects_count)
{
  ProfBeginFunction();

  RDI_BinarySection *dst_arr = push_array(arena, RDI_BinarySection, binary_sects_count);

  for (U64 sect_idx = 0; sect_idx < binary_sects_count; ++sect_idx) {
    RDIB_BinarySection *src = &binary_sects[sect_idx];
    RDI_BinarySection  *dst = &dst_arr[sect_idx];

    dst->name_string_idx = rdib_idx_from_string_map(string_map, src->name);
    dst->flags           = src->flags;
    dst->voff_first      = src->voff_first;
    dst->voff_opl        = src->voff_opl;
    dst->foff_first      = src->foff_first;
    dst->foff_opl        = src->foff_opl;
  }

  RDIB_DataSection sect = { .tag = RDI_SectionKind_BinarySections };
  str8_list_push(arena, &sect.data, str8_array(dst_arr, binary_sects_count));
  rdib_data_section_list_push(arena, sect_list, sect);

  ProfEnd();
}

internal void
rdib_data_sections_from_units(Arena                *arena,
                              RDIB_DataSectionList *sect_list,
                              RDIB_StringMap       *string_map,
                              RDIB_PathTree        *path_tree,
                              U64                   total_unit_count,
                              U64                   unit_chunk_count,
                              RDIB_UnitChunk      **unit_chunks)
{
  ProfBeginFunction();

  RDI_Unit *dst_arr = push_array(arena, RDI_Unit, total_unit_count);
  for (U64 chunk_idx = 0; chunk_idx < unit_chunk_count; chunk_idx += 1) {
    RDIB_UnitChunk *chunk = unit_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; i += 1) {
      RDIB_Unit *src     = &chunk->v[i];
      U64       unit_idx = rdib_idx_from_unit(src);
      RDI_Unit  *dst     = &dst_arr[unit_idx];
      dst->unit_name_string_idx     = rdib_idx_from_string_map(string_map, src->unit_name);
      dst->compiler_name_string_idx = rdib_idx_from_string_map(string_map, src->compiler_name);
      dst->source_file_path_node    = rdib_idx_from_path_tree(path_tree, src->source_file);
      dst->object_file_path_node    = rdib_idx_from_path_tree(path_tree, src->object_file);
      dst->archive_file_path_node   = rdib_idx_from_path_tree(path_tree, src->archive_file);
      dst->build_path_node          = rdib_idx_from_path_tree(path_tree, src->build_path);
      dst->language                 = src->language;
      dst->line_table_idx           = src->line_table->output_array_idx;
    }
  }

  RDIB_DataSection sect = { .tag = RDI_SectionKind_Units };
  str8_list_push(arena, &sect.data, str8_array(dst_arr, total_unit_count));
  rdib_data_section_list_push(arena, sect_list, sect);

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_vmap_count_ranges_unit_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_UnitChunk *chunk = task->unit_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Unit *unit = &chunk->v[i];
      task->counts[task_id] += unit->virt_range_count;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_vmap_count_ranges_gvar_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->gvar_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Variable *var = &chunk->v[i];
      for (RDIB_LocationNode *loc_n = var->locations.first; loc_n != 0; loc_n = loc_n->next) {
        task->counts[task_id] += loc_n->v.ranges.count;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_vmap_count_ranges_scope_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ScopeChunk *chunk = task->scope_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      task->counts[task_id] += chunk->v[i].ranges.count;
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_vmap_entries_unit_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task            = raw_task;
  U64                  range_cursor     = task->offsets[task_id];
  U64                  range_cursor_opl = task->offsets[task_id] + task->counts[task_id];

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_UnitChunk *chunk = task->unit_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Unit *unit = &chunk->v[i];
      for (Rng1U64 *range_ptr = unit->virt_ranges, *range_opl = unit->virt_ranges + unit->virt_range_count;
           range_ptr < range_opl; ++range_ptr) {
        Assert(range_cursor < range_cursor_opl);
        Assert(range_ptr->min <= range_ptr->max);

        RDIB_VMapRange *vmap_range = task->vmap + range_cursor;
        vmap_range->voff           = range_ptr->min;
        vmap_range->size           = range_ptr->max - range_ptr->min;
        vmap_range->idx            = rdib_idx_from_unit(unit);
        range_cursor += 1;
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_vmap_entries_gvar_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task = raw_task;

  U64 range_cursor     = task->offsets[task_id];
  U64 range_cursor_opl = task->offsets[task_id] + task->counts[task_id];

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->gvar_chunks[chunk_idx];
    for (U64 var_idx = 0; var_idx < chunk->count; ++var_idx) {
      RDIB_Variable *var = &chunk->v[var_idx];
      for (RDIB_LocationNode *loc_n = var->locations.first; loc_n != 0; loc_n = loc_n->next) {
        for (Rng1U64Node *range_n = loc_n->v.ranges.first; range_n != 0; range_n = range_n->next) {
          Assert(range_cursor < range_cursor_opl);
          Assert(range_n->v.min <= range_n->v.max);
          U64 size = range_n->v.max - range_n->v.min;

          RDIB_VMapRange *vmap_range = task->vmap + range_cursor;
          vmap_range->voff           = range_n->v.min;
          vmap_range->size           = size;
          vmap_range->idx            = rdib_idx_from_variable(var);
          range_cursor += 1;
        }
      }
    }
  }

  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_vmap_entries_scope_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task = raw_task;

  U64 range_cursor     = task->offsets[task_id];
  U64 range_cursor_opl = task->offsets[task_id] + task->counts[task_id];

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ScopeChunk *chunk = task->scope_chunks[chunk_idx];
    for (U64 scope_idx = 0; scope_idx < chunk->count; ++scope_idx) {
      RDIB_Scope *scope = &chunk->v[scope_idx];
      for (Rng1U64Node *range_n = scope->ranges.first; range_n != 0; range_n = range_n->next) {
        Assert(range_cursor  < range_cursor_opl);
        Assert(range_n->v.min <= range_n->v.max);

        RDIB_VMapRange *vmap_range = task->vmap + range_cursor;
        vmap_range->voff           = range_n->v.min;
        vmap_range->size           = range_n->v.max - range_n->v.min;
        vmap_range->idx            = rdib_idx_from_scope(scope);
        range_cursor += 1;
      }
    }
  }

  ProfEnd();
}

internal void
rdib_sort_procs_radix_32(RDIB_Procedure **v, U64 count)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  RDIB_Procedure **temp = push_array_no_zero(scratch.arena, RDIB_Procedure *, count);
  RDIB_Procedure **src  = v;
  RDIB_Procedure **dst  = temp;

  ProfBegin("Count Memzero");
  U32 count_8lo[256];    MemoryZeroArray(count_8lo);
  U32 count_8hi[256];    MemoryZeroArray(count_8hi);
  U32 count_16[1 << 16]; MemoryZeroArray(count_16);
  ProfEnd();

  ProfBegin("Histogram");
  for (U64 i = 0; i < count; i += 1) {
    RDIB_Procedure *p = src[i];

    U64 digit_8lo  = (p->scope->ranges.first->v.min >> 0)  % ArrayCount(count_8lo);
    U64 digit_8hi  = (p->scope->ranges.first->v.min >> 8)  % ArrayCount(count_8hi);
    U64 digit_16   = (p->scope->ranges.first->v.min >> 16) % ArrayCount(count_16);

    count_8lo[digit_8lo]  += 1;
    count_8hi[digit_8hi]  += 1;
    count_16[digit_16]    += 1;
  }
  ProfEnd();

  ProfBegin("Counts -> Offsets");
  counts_to_offsets_array_u32(ArrayCount(count_8lo),  count_8lo);
  counts_to_offsets_array_u32(ArrayCount(count_8hi),  count_8hi);
  counts_to_offsets_array_u32(ArrayCount(count_16),   count_16 );
  ProfEnd();

  ProfBegin("Order 8 Lo");
  for (U64 i = 0; i < count; i += 1) {
    RDIB_Procedure *p = src[i];
    U64 digit = (p->scope->ranges.first->v.min >> 0) % ArrayCount(count_8lo);
    dst[count_8lo[digit]++] = p;
  }
  ProfEnd();

  ProfBegin("Order 8 Hi");
  for (U64 i = 0; i < count; i += 1) {
    RDIB_Procedure *p = dst[i];
    U64 digit = (p->scope->ranges.first->v.min >> 8) % ArrayCount(count_8hi);
    src[count_8hi[digit]++] = p;
  }
  ProfEnd();

  ProfBegin("Order 16");
  for (U64 i = 0; i < count; i += 1) {
    RDIB_Procedure *p = src[i];
    U64 digit = (p->scope->ranges.first->v.min >> 16) % ArrayCount(count_16);
    dst[count_16[digit]++] = p;
  }
  ProfEnd();

  MemoryCopyTyped(src, dst, count);

  scratch_end(scratch);
  ProfEnd();
}

internal String8List
rdib_data_from_vmap(Arena *arena, U64 range_count, RDIB_VMapRange *ranges)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  const U64 size_bit_count0 = 8;
  const U64 size_bit_count1 = 8;
  const U64 size_bit_count2 = 16;

  const U64 voff_bit_count0 = 11;
  const U64 voff_bit_count1 = 11;
  const U64 voff_bit_count2 = 10;

  ProfBegin("Push shared buffer");
  U64 radix_memory_size = sizeof(RDIB_VMapRange) * range_count +
                          sizeof(U32) * ((1 << size_bit_count0) + (1 << size_bit_count1) + (1 << size_bit_count2)) +
                          sizeof(U32) * ((1 << voff_bit_count0) + (1 << voff_bit_count1) + (1 << voff_bit_count2));
  U8 *radix_memory      = push_array_no_zero(arena, U8, radix_memory_size);
  ProfEnd();

  // TODO: windows caps images at 4GiB so we use 32-bit radix sort, but on linux
  // images can have > 4GiB and we need to detect when vmap uses upper 32bits
  // in voffs do a 64-bit radix sort.
  ProfBegin("Sort");
  {
    RDIB_VMapRange *src = ranges;
    RDIB_VMapRange *dst = (RDIB_VMapRange *)radix_memory;

    U32 *size_count0 = (U32 *)(dst + range_count);
    U32 *size_count1 = size_count0 + (1 << size_bit_count0);
    U32 *size_count2 = size_count1 + (1 << size_bit_count1);

    U32 *voff_count0 = size_count2 + (1 << size_bit_count2);
    U32 *voff_count1 = voff_count0 + (1 << voff_bit_count0);
    U32 *voff_count2 = voff_count1 + (1 << voff_bit_count1);

    //
    // Build histogram
    //

    MemoryZeroTyped(size_count0, 1 << size_bit_count0);
    MemoryZeroTyped(size_count1, 1 << size_bit_count1);
    MemoryZeroTyped(size_count2, 1 << size_bit_count2);

    MemoryZeroTyped(voff_count0, 1 << voff_bit_count0);
    MemoryZeroTyped(voff_count1, 1 << voff_bit_count1);
    MemoryZeroTyped(voff_count2, 1 << voff_bit_count2);

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange *r = src+i;

      U32 size_digit0 = (-r->size >> 0)                                   % (1 << size_bit_count0);
      U32 size_digit1 = (-r->size >> size_bit_count0)                     % (1 << size_bit_count1);
      U32 size_digit2 = (-r->size >> (size_bit_count0 + size_bit_count1)) % (1 << size_bit_count2);

      U64 voff_digit0 = (r->voff >> 0)                                   % (1 << voff_bit_count0);
      U64 voff_digit1 = (r->voff >> voff_bit_count0)                     % (1 << voff_bit_count1);
      U64 voff_digit2 = (r->voff >> (voff_bit_count0 + voff_bit_count1)) % (1 << voff_bit_count2);

      ++size_count0[size_digit0];
      ++size_count1[size_digit1];
      ++size_count2[size_digit2];

      ++voff_count0[voff_digit0];
      ++voff_count1[voff_digit1];
      ++voff_count2[voff_digit2];
    }

    counts_to_offsets_array_u32((1 << size_bit_count0), size_count0);
    counts_to_offsets_array_u32((1 << size_bit_count1), size_count1);
    counts_to_offsets_array_u32((1 << size_bit_count2), size_count2);

    counts_to_offsets_array_u32((1 << voff_bit_count0), voff_count0);
    counts_to_offsets_array_u32((1 << voff_bit_count1), voff_count1);
    counts_to_offsets_array_u32((1 << voff_bit_count2), voff_count2);

    //
    // Sort on range size (high to low)
    //

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange r = src[i];
      U32 digit = (-r.size >> 0) % (1 << size_bit_count0);
      dst[size_count0[digit]++] = r;
    }

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange r = dst[i];
      U32 digit = (-r.size >> size_bit_count0) % (1 << size_bit_count1);
      src[size_count1[digit]++] = r;
    }

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange r = src[i];
      U32 digit = (-r.size >> (size_bit_count0 + size_bit_count1)) % (1 << size_bit_count2);
      dst[size_count2[digit]++] = r;
    }

    //
    // Sort on range voff (low to high)
    //

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange r = dst[i];
      U32 digit = (r.voff >> 0) % (1 << voff_bit_count0);
      src[voff_count0[digit]++] = r;
    }

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange r = src[i];
      U32 digit = (r.voff >> voff_bit_count0) % (1 << voff_bit_count1);
      dst[voff_count1[digit]++] = r;
    }

    for (U64 i = 0; i < range_count; ++i) {
      RDIB_VMapRange r = dst[i];
      U32 digit = (r.voff >> (voff_bit_count0 + voff_bit_count1)) % (1 << voff_bit_count2);
      src[voff_count2[digit]++] = r;
    }
  }
  ProfEnd();

  ProfBegin("Layout virtual map");
  String8List raw_vmap = {0};
  {
    U64            default_vme_cap = 4096;
    U64            vme_block_cap   = radix_memory_size / sizeof(RDI_VMapEntry);
    U64            vme_block_size  = 0;
    RDI_VMapEntry *vme_block       = (RDI_VMapEntry *)radix_memory;

    // Recycle radix sort memory
    str8_list_push(arena, &raw_vmap, str8_array(vme_block, vme_block_cap));

#define push_vme() (vme_block_size < raw_vmap.last->string.size/sizeof(vme_block[0])) ? &vme_block[vme_block_size++] :    \
                                                  (vme_block = push_array(arena, RDI_VMapEntry, vme_block_cap),           \
                                                  vme_block_cap  = default_vme_cap,                                       \
                                                  vme_block_size = 0,                                                     \
                                                  str8_list_push(arena, &raw_vmap, str8_array(vme_block, vme_block_cap)), \
                                                  &vme_block[vme_block_size++])

    struct Stack {
      RDIB_VMapRange *range;
      struct Stack   *next;
    };
    struct Stack *stack      = 0;
    struct Stack *free_stack = 0;
    stack        = push_array(scratch.arena, struct Stack, 1);
    stack->range = &ranges[0];

    for (U64 range_idx = 1; range_idx < range_count; ++range_idx) {
      RDIB_VMapRange *r = ranges+range_idx;
      RDIB_VMapRange *last_bot_range = stack->range;
      RDIB_VMapRange *last_pop_range = 0;
      while (stack->range->idx != 0) {
        if (r->voff < stack->range->voff + stack->range->size) {
          // Current range is a subset, keep building stack
          break;
        }

        struct Stack *frame = stack;
        SLLStackPop(stack);

        // Did we reach bottom most range?
        if (last_pop_range == 0) {
          // Don't push VME for index with adjacent ranges
          if (vme_block_size > 0 && vme_block[vme_block_size-1].idx != frame->range->idx) {
            RDI_VMapEntry *vme = push_vme();
            vme->voff          = frame->range->voff;
            vme->idx           = frame->range->idx;
          }
        }

        // Reopen parent range
        //
        //  Does parent range extend past child range?
        if (stack->range->voff + stack->range->size != frame->range->voff + frame->range->size && 
        //  Does next range open on where stack range ends?
            r->voff != frame->range->voff + frame->range->size) {
          RDI_VMapEntry *vme = push_vme();
          vme->idx           = stack->range->idx;
          vme->voff          = frame->range->voff + frame->range->size;
        }

        last_pop_range = stack->range;

        // Recycle stack frame
        SLLStackPush(free_stack, frame);
      }

      // Prefix
      if (last_pop_range == 0 && last_bot_range->voff != r->voff) {
        RDI_VMapEntry* vme = push_vme();
        vme->voff          = last_bot_range->voff;
        vme->idx           = last_bot_range->idx;
      }

      struct Stack *frame;
      if (free_stack == 0) {
        frame = push_array(scratch.arena, struct Stack, 1);
      } else {
        frame = free_stack;
        SLLStackPop(free_stack);
      }
      frame->range = r;
      SLLStackPush(stack, frame);
    }

    // Empty stack
    {
      RDIB_VMapRange *last_pop_range = 0;
      while (stack->range->idx != 0) {
        struct Stack *frame = stack;
        SLLStackPop(stack);

        if (last_pop_range == 0) {
          if (vme_block_size > 0 && vme_block[vme_block_size-1].idx != frame->range->idx) {
            RDI_VMapEntry *vme = push_vme();
            vme->voff          = frame->range->voff;
            vme->idx           = frame->range->idx;
          }
        }

        if (stack->range->voff + stack->range->size != frame->range->voff + frame->range->size) {
          RDI_VMapEntry *vme = push_vme();
          vme->voff          = frame->range->voff + frame->range->size;
          vme->idx           = stack->range->idx;
        }

        last_pop_range = stack->range;
      }
    }

    // Subtract unsued vmap entries
    U64 last_vme_unused         = raw_vmap.last->string.size - sizeof(vme_block[0]) * vme_block_size;
    raw_vmap.last->string.size -= last_vme_unused;
    raw_vmap.total_size        -= last_vme_unused;

#undef push_vme
  }
  ProfEnd();


  // duplicate voff check
#if 0
  U64 prev = max_U64;
  for (String8Node *node = raw_vmap.first; node != 0; node = node->next) {
    RDI_VMapEntry *e = (RDI_VMapEntry*)node->string.str;
    for (U64 i = 0, c = node->string.size / sizeof(RDI_VMapEntry); i < c; ++i) {
      Assert(e[i].voff != prev);
      prev = e[i].voff;
    }
  }
#endif

  scratch_end(scratch);
  ProfEnd();
  return raw_vmap;
}

THREAD_POOL_TASK_FUNC(rdib_fill_scope_vmaps_task)
{
  ProfBeginFunction();
  RDIB_VMapBuilderTask *task = raw_task;
  task->raw_vmaps[task_id] = rdib_data_from_vmap(arena, task->vmap_counts[task_id], task->vmaps[task_id]);
  ProfEnd();
}

internal void
rdib_data_sections_from_unit_gvar_scope_vmaps(TP_Context           *tp,
                                              TP_Arena             *arena,
                                              RDIB_DataSectionList *sect_list,
                                              U64 unit_chunk_count,  RDIB_UnitChunk     **unit_chunks,
                                              U64 gvar_chunk_count,  RDIB_VariableChunk **gvar_chunks,
                                              U64 scope_chunk_count, RDIB_ScopeChunk    **scope_chunks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  RDIB_VMapBuilderTask task = {0};
  task.counts               = push_array(scratch.arena, U64, tp->worker_count);
  task.ranges               = tp_divide_work(scratch.arena, unit_chunk_count, tp->worker_count);

  ProfBegin("Unit VMap");
  U64             unit_vmap_count;
  RDIB_VMapRange *unit_vmaps;
  {
    ProfBegin("Count Ranges");
    MemoryZeroTyped(task.counts, tp->worker_count);
    task.unit_chunks = unit_chunks;
    task.ranges      = tp_divide_work(scratch.arena, unit_chunk_count, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_vmap_count_ranges_unit_task, &task);
    ProfEnd();

    ProfBegin("Push");
    unit_vmap_count = sum_array_u64(tp->worker_count, task.counts);
    unit_vmaps      = push_array_no_zero(scratch.arena, RDIB_VMapRange, unit_vmap_count);
    ProfEnd();

    ProfBegin("Fill");
    task.vmap        = unit_vmaps;
    task.unit_chunks = unit_chunks;
    task.offsets = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_fill_vmap_entries_unit_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBegin("Global Variables");
  U64             gvar_vmap_count;
  RDIB_VMapRange *gvar_vmaps;
  {
    ProfBegin("Count");
    MemoryZeroTyped(task.counts, tp->worker_count);
    task.ranges      = tp_divide_work(scratch.arena, gvar_chunk_count, tp->worker_count);
    task.gvar_chunks = gvar_chunks;
    tp_for_parallel(tp, 0, tp->worker_count, rdib_vmap_count_ranges_gvar_task, &task);
    ProfEnd();

    ProfBegin("Push");
    gvar_vmap_count = sum_array_u64(tp->worker_count, task.counts);
    gvar_vmaps      = push_array_no_zero(scratch.arena, RDIB_VMapRange, gvar_vmap_count);
    ProfEnd();

    ProfBegin("Fill");
    task.vmap        = gvar_vmaps;
    task.gvar_chunks = gvar_chunks;
    task.offsets     = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_fill_vmap_entries_gvar_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBegin("Scopes");
  U64             scope_vmap_count;
  RDIB_VMapRange *scope_vmaps;
  {
    ProfBegin("Count");
    MemoryZeroTyped(task.counts, tp->worker_count);
    task.ranges       = tp_divide_work(scratch.arena, scope_chunk_count, tp->worker_count);
    task.scope_chunks = scope_chunks;
    tp_for_parallel(tp, 0, tp->worker_count, rdib_vmap_count_ranges_scope_task, &task);
    ProfEnd();

    ProfBegin("Push");
    scope_vmap_count = sum_array_u64(tp->worker_count, task.counts);
    scope_vmaps      = push_array_no_zero(scratch.arena, RDIB_VMapRange, scope_vmap_count);
    ProfEnd();

    ProfBegin("Fill");
    task.vmap         = scope_vmaps;
    task.scope_chunks = scope_chunks;
    task.offsets      = offsets_from_counts_array_u64(scratch.arena, task.counts, tp->worker_count);
    tp_for_parallel(tp, 0, tp->worker_count, rdib_fill_vmap_entries_scope_task, &task);
    ProfEnd();
  }
  ProfEnd();

  task.vmap_counts[0] = unit_vmap_count;
  task.vmap_counts[1] = gvar_vmap_count;
  task.vmap_counts[2] = scope_vmap_count;
  task.vmaps[0]       = unit_vmaps;
  task.vmaps[1]       = gvar_vmaps;
  task.vmaps[2]       = scope_vmaps;

  ProfBegin("Fill RDI VMaps");
  MemoryZeroArray(task.raw_vmaps);
  tp_for_parallel(tp, arena, 3, rdib_fill_scope_vmaps_task, &task);
  ProfEnd();

  RDIB_DataSection unit_vmap_sect  = { .tag = RDI_SectionKind_UnitVMap,   .data = task.raw_vmaps[0] };
  RDIB_DataSection gvar_vmap_sect  = { .tag = RDI_SectionKind_GlobalVMap, .data = task.raw_vmaps[1] };
  RDIB_DataSection scope_vmap_sect = { .tag = RDI_SectionKind_ScopeVMap,  .data = task.raw_vmaps[2] };
  rdib_data_section_list_push(arena->v[0], sect_list, unit_vmap_sect );
  rdib_data_section_list_push(arena->v[0], sect_list, gvar_vmap_sect );
  rdib_data_section_list_push(arena->v[0], sect_list, scope_vmap_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_copy_string_data_task)
{
  RDIB_CopyStringDataTask *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_StringMapBucket *bucket              = task->buckets[bucket_idx];
    U64                   string_table_offset = task->string_table[bucket_idx];
    Assert(string_table_offset + bucket->string.size <= task->string_data_size);
    MemoryCopy(task->string_data + string_table_offset, bucket->string.str, bucket->string.size);
  }
}

internal void
rdib_data_sections_from_string_map(TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMapBucket **buckets, U64 bucket_count)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  // assign string table offset for each bucket
  U64  cursor       = 0;
  U32 *string_table = push_array_no_zero(arena, U32, bucket_count);
  for (U64 bucket_idx = 0; bucket_idx < bucket_count; ++bucket_idx) {
    string_table[bucket_idx] = cursor;
    cursor += buckets[bucket_idx]->string.size;
  }

  // populate string data buffer with bucket strings
  RDIB_CopyStringDataTask task = {0};
  task.string_table     = string_table;
  task.string_data_size = cursor;
  task.string_data      = push_array_no_zero(arena, U8, task.string_data_size);
  task.buckets          = buckets;
  task.ranges           = tp_divide_work(scratch.arena, bucket_count, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_copy_string_data_task, &task);

  // fill out string table section
  RDIB_DataSection string_table_sect = {0};
  string_table_sect.tag = RDI_SectionKind_StringTable;
  str8_list_push(arena, &string_table_sect.data, str8((U8 *)task.string_table, sizeof(task.string_table[0]) * bucket_count));

  // fill out string data section
  RDIB_DataSection string_data_sect = { .tag = RDI_SectionKind_StringData };
  str8_list_push(arena, &string_data_sect.data, str8(task.string_data, task.string_data_size));
  
  // push sections to list
  rdib_data_section_list_push(arena, sect_list, string_table_sect);
  rdib_data_section_list_push(arena, sect_list, string_data_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_idx_run_copy_task)
{
  RDIB_IdxRunCopyTask *task = raw_task;
  for (U64 bucket_idx = task->ranges[task_id].min; bucket_idx < task->ranges[task_id].max; ++bucket_idx) {
    RDIB_IndexRunBucket *bucket = task->buckets[bucket_idx];
    MemoryCopyTyped(&task->output_array[bucket->index_in_output_array], bucket->indices.v, bucket->indices.count);
  }
}

internal void
rdib_data_sections_from_index_runs(TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_IndexRunBucket **buckets, U64 bucket_count)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  ProfBegin("Count Indices");
  U64 total_index_count = 0;
  for (U64 bucket_idx = 0; bucket_idx < bucket_count; ++bucket_idx) {
    total_index_count += buckets[bucket_idx]->indices.count;
  }
  ProfEnd();

  U32 *output_array = push_array_no_zero(arena, U32, total_index_count);

  RDIB_IdxRunCopyTask task = {0};
  task.buckets             = buckets;
  task.ranges              = tp_divide_work(scratch.arena, bucket_count, tp->worker_count);
  task.output_array        = output_array;
  tp_for_parallel(tp, 0, tp->worker_count, rdib_idx_run_copy_task, &task);

  RDIB_DataSection data_sect = { .tag = RDI_SectionKind_IndexRuns };
  str8_list_push(arena, &data_sect.data, str8_array(output_array, total_index_count));

  rdib_data_section_list_push(arena, sect_list, data_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_file_path_nodes_task)
{
  ProfBeginFunction();
  RDIB_BuildFilePathNodesTask *task       = raw_task;
  RDIB_StringMap              *string_map = task->string_map;
  RDIB_PathTree               *path_tree  = task->path_tree;
  for (RDIB_PathTreeNode *n = path_tree->node_lists[task_id].first; n != 0; n = n->next_order) {
    RDI_FilePathNode *dst = task->nodes_dst + n->node_idx;
    dst->name_string_idx  = rdib_idx_from_string_map(string_map, n->sub_path);

    B32 is_source_file_node = (n->first_child == 0);
    if (is_source_file_node) {
      dst->source_file_idx = rdib_idx_from_source_file(n->src_file);
    } else {
      // directories don't have a source file
      Assert(n->src_file == 0);
      dst->source_file_idx = 0;
    }

    if(n->parent) {
      dst->parent_path_node = n->parent->node_idx;
    }
    if (n->first_child) {
      dst->first_child = n->first_child->node_idx;
    }
    if (n->next_sibling) {
      dst->next_sibling = n->next_sibling->node_idx;
    }
  }
  ProfEnd();
}

internal void
rdib_data_sections_from_path_tree(TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list, RDIB_StringMap *string_map, RDIB_PathTree *path_tree)
{
  ProfBeginFunction();
  RDI_FilePathNode *nodes_dst = push_array_no_zero(arena, RDI_FilePathNode, path_tree->node_count);

  RDIB_BuildFilePathNodesTask task = {0};
  task.path_tree  = path_tree;
  task.string_map = string_map;
  task.nodes_dst  = nodes_dst;
  tp_for_parallel(tp, 0, path_tree->list_count, rdib_build_file_path_nodes_task, &task);

  RDIB_DataSection data_sect = { .tag = RDI_SectionKind_FilePathNodes };
  str8_list_push(arena, &data_sect.data, str8_array(nodes_dst, path_tree->node_count));

  rdib_data_section_list_push(arena, sect_list, data_sect);

  ProfEnd();
}

internal RDIB_PathTree *
rdib_build_path_tree(Arena                 *arena,
                     U64                    worker_count,
                     RDIB_SourceFile       *null_src_file,
                     U64                    unit_chunk_count,
                     RDIB_UnitChunk       **unit_chunks,
                     U64                    src_file_chunk_count,
                     RDIB_SourceFileChunk **src_file_chunks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  RDIB_PathTree *tree = rdib_path_tree_init(arena, worker_count);
  rdib_path_tree_insert(arena, tree, RDIB_PATH_TREE_NIL_STRING, null_src_file);

  ProfBegin("Units");
  for (U64 ichunk = 0; ichunk < unit_chunk_count; ++ichunk) {
    RDIB_UnitChunk *chunk = unit_chunks[ichunk];
    for (U64 iunit = 0; iunit < chunk->count; ++iunit) {
      RDIB_Unit *unit = &chunk->v[iunit];
      rdib_path_tree_insert(arena, tree, unit->source_file,  null_src_file);
      rdib_path_tree_insert(arena, tree, unit->object_file,  null_src_file);
      rdib_path_tree_insert(arena, tree, unit->archive_file, null_src_file);
      rdib_path_tree_insert(arena, tree, unit->build_path,   null_src_file);
    }
  }
  ProfEnd();

  ProfBegin("Source Files");
  for (U64 chunk_idx = 0; chunk_idx < src_file_chunk_count; ++chunk_idx) {
    RDIB_SourceFileChunk *chunk = src_file_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_SourceFile *src_file = chunk->v + i;
      rdib_path_tree_insert(arena, tree, src_file->file_path, src_file);
    }
  }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return tree;
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_var_section_task)
{
  ProfBeginDynamic("Global Variables Task %llu", task_id);
  RDIB_BuildSymbolSectionTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->gvars_rdib[chunk_idx];
    RDI_GlobalVariable *vars  = push_array_no_zero(arena, RDI_GlobalVariable, chunk->count);

    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Variable      *src = &chunk->v[i];
      RDI_GlobalVariable *dst = &vars[i];

      // TODO: temporary hack while we don't have bytecode eval in RDI_GlobalVariable
      U64 voff = 0;
      if (src->locations.first != 0) {
        if (src->locations.first->v.kind == RDI_LocationKind_AddrBytecodeStream && src->locations.first->v.bytecode.first->op == RDI_EvalOp_ModuleOff) {
          voff = src->locations.first->v.bytecode.first->p;
        }
      }

      dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->name);
      dst->voff            = voff;
      dst->type_idx        = rdib_idx_from_type(src->type);
      dst->link_flags      = src->link_flags;

      if (src->container_type != 0) {
        Assert(!src->container_proc);
        dst->link_flags    |= RDI_LinkFlag_TypeScoped;
        dst->container_idx  = rdib_idx_from_udt_type(src->container_type);
      }
      if (src->container_proc != 0) {
        Assert(!src->container_type);
        dst->link_flags    |= RDI_LinkFlag_ProcScoped;
        dst->container_idx  = rdib_idx_from_procedure(src->container_proc);
      }
    }

    str8_list_push(arena, &task->gvars_out[task_id], str8_array(vars, chunk->count));
  }

  ProfEnd();
}

internal void
rdib_data_sections_from_global_variables(TP_Context           *tp,
                                         TP_Arena             *arena,
                                         RDIB_DataSectionList *sect_list,
                                         RDIB_StringMap       *string_map,
                                         U64                   total_count,
                                         U64                   chunk_count,
                                         RDIB_VariableChunk   **chunks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Build");
  RDIB_BuildSymbolSectionTask task = {0};
  task.string_map                  = string_map;
  task.ranges                      = tp_divide_work(scratch.arena, chunk_count, tp->worker_count);
  task.gvars_rdib                  = chunks;
  task.gvars_out                   = push_array(scratch.arena, String8List, tp->worker_count);
  tp_for_parallel(tp, arena, tp->worker_count, rdib_build_var_section_task, &task);
  ProfEnd();

  RDIB_DataSection gvars_sect = { .tag = RDI_SectionKind_GlobalVariables };
  str8_list_concat_in_place_array(&gvars_sect.data, task.gvars_out, tp->worker_count);
  rdib_data_section_list_push(arena->v[0], sect_list, gvars_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_tvar_section_task)
{
  RDIB_BuildSymbolSectionTask *task = raw_task;
  ProfBeginDynamic("Thread Variables Task [Chunk Count: %llu]", task->ranges[task_id].max - task->ranges[task_id].min);

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_VariableChunk *chunk = task->tvars_rdib[chunk_idx];
    RDI_ThreadVariable *vars  = push_array_no_zero(arena, RDI_ThreadVariable, chunk->count);

    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Variable      *src = &chunk->v[i];
      RDI_ThreadVariable *dst = &vars[i];

      U32 tls_off = 0;
      if (src->locations.first != 0) {
        if (src->locations.first->v.kind == RDI_LocationKind_AddrBytecodeStream && src->locations.first->v.bytecode.first->op == RDI_EvalOp_TLSOff) {
          tls_off = src->locations.first->v.bytecode.first->p;
        }
      }

      dst->name_string_idx = rdib_idx_from_string_map(task->string_map, src->name);
      dst->tls_off         = tls_off;
      dst->type_idx        = rdib_idx_from_type(src->type);

      if (src->container_type != 0) {
        Assert(!src->container_proc);
        dst->link_flags    |= RDI_LinkFlag_TypeScoped;
        dst->container_idx  = rdib_idx_from_udt_type(src->container_type);
      }
      if (src->container_proc != 0) {
        Assert(!src->container_type);
        dst->link_flags    |= RDI_LinkFlag_ProcScoped;
        dst->container_idx  = rdib_idx_from_procedure(src->container_proc);
      }
    }

    str8_list_push(arena, &task->tvars_out[task_id], str8_array(vars, chunk->count));
  }

  ProfEnd();
}

internal void
rdib_data_sections_from_thread_variables(TP_Context           *tp,
                                         TP_Arena             *arena,
                                         RDIB_DataSectionList *sect_list,
                                         RDIB_StringMap       *string_map,
                                         U64                   total_count,
                                         U64                   chunk_count,
                                         RDIB_VariableChunk   **chunks)
{
  ProfBeginDynamic("Thread Variables [Chunk Count: %llu, Total Count %llu]", total_count, chunk_count);
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Build");
  RDIB_BuildSymbolSectionTask task = {0};
  task.string_map                  = string_map;
  task.ranges                      = tp_divide_work(scratch.arena, chunk_count, tp->worker_count);
  task.tvars_rdib                  = chunks;
  task.tvars_out                   = push_array(scratch.arena, String8List, tp->worker_count);
  tp_for_parallel(tp, arena, tp->worker_count, rdib_build_tvar_section_task, &task);
  ProfEnd();

  RDIB_DataSection tvars_sect = { .tag = RDI_SectionKind_ThreadVariables };
  str8_list_concat_in_place_array(&tvars_sect.data, task.tvars_out, tp->worker_count);
  rdib_data_section_list_push(arena->v[0], sect_list, tvars_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_procs_section_task)
{
  RDIB_BuildSymbolSectionTask *task = raw_task;
  ProfBeginDynamic("Procedures Task [Chunk Count: %llu]", task->ranges[task_id].max - task->ranges[task_id].min);

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ProcedureChunk *chunk = task->procs_rdib[chunk_idx];
    RDI_Procedure       *procs = push_array_no_zero(arena, RDI_Procedure, chunk->count);

    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_Procedure *src = &chunk->v[i];
      RDI_Procedure  *dst = &procs[i];

      dst->name_string_idx      = rdib_idx_from_string_map(task->string_map, src->name);
      dst->link_name_string_idx = rdib_idx_from_string_map(task->string_map, src->link_name);
      dst->link_flags           = src->link_flags;
      dst->type_idx             = rdib_idx_from_type(src->type);
      dst->root_scope_idx       = rdib_idx_from_scope(src->scope);

      if (src->container_type != 0) {
        AssertAlways(!src->container_proc);
        dst->link_flags    |= RDI_LinkFlag_TypeScoped;
        dst->container_idx  = rdib_idx_from_udt_type(src->container_type);
      }

      if (src->container_proc != 0) {
        AssertAlways(!src->container_type);
        dst->link_flags    |= RDI_LinkFlag_ProcScoped;
        dst->container_idx  = rdib_idx_from_procedure(0); Assert(!"TODO"); // src->container_proc
      }
    }

    str8_list_push(arena, &task->procs_out[task_id], str8_array(procs, chunk->count));
  }

  ProfEnd();
}

internal void
rdib_data_sections_from_procedures(TP_Context           *tp,
                                   TP_Arena             *arena,
                                   RDIB_DataSectionList *sect_list,
                                   RDIB_StringMap       *string_map,
                                   U64                   total_count,
                                   U64                   chunk_count,
                                   RDIB_ProcedureChunk  **chunks)
{
  ProfBeginDynamic("Procedures [Total Count: %llu, Chunk Count: %llu]", total_count, chunk_count);
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Build");
  RDIB_BuildSymbolSectionTask task = {0};
  task.string_map                  = string_map;
  task.ranges                      = tp_divide_work(scratch.arena, chunk_count, tp->worker_count);
  task.procs_rdib                  = chunks;
  task.procs_out                   = push_array(scratch.arena, String8List, tp->worker_count);
  tp_for_parallel(tp, arena, tp->worker_count, rdib_build_procs_section_task, &task);
  ProfEnd();

  RDIB_DataSection procs_sect = { .tag = RDI_SectionKind_Procedures };
  str8_list_concat_in_place_array(&procs_sect.data, task.procs_out, tp->worker_count);
  rdib_data_section_list_push(arena->v[0], sect_list, procs_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_count_scopes_task)
{
  ProfBeginFunction();
  RDIB_BuildSymbolSectionTask *task = raw_task;
  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_ScopeChunk *chunk = task->scopes_rdib[chunk_idx];
    for (U64 scope_i = 0; scope_i < chunk->count; ++scope_i) {
      RDIB_Scope *scope = &chunk->v[scope_i];

      task->scope_voff_counts[task_id] += scope->ranges.count * 2;
      task->local_counts[task_id]      += scope->local_count;

      for (RDIB_Variable *var = scope->local_first; var != 0; var = var->next) {
        for (RDIB_LocationNode *loc_n = var->locations.first; loc_n != 0; loc_n = loc_n->next) {
          switch (loc_n->v.kind) {
          case RDI_LocationKind_NULL: break;
          case RDI_LocationKind_AddrBytecodeStream:
          case RDI_LocationKind_ValBytecodeStream: {
            task->loc_data_sizes[task_id] += loc_n->v.bytecode.size + /* stream ender: */ 1;
          } break;
          case RDI_LocationKind_ValReg: {
            task->loc_data_sizes[task_id] += sizeof(RDI_LocationReg);
          } break;
          case RDI_LocationKind_AddrRegPlusU16:
          case RDI_LocationKind_AddrAddrRegPlusU16: {
            task->loc_data_sizes[task_id] += sizeof(RDI_LocationRegPlusU16);
          } break;
          default: InvalidPath;
          }

          task->loc_block_counts[task_id] += loc_n->v.ranges.count;
          task->loc_data_sizes[task_id]   += AlignPadPow2(task->loc_data_sizes[task_id], 8);
        }
      }
    }
  }
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_scopes_task)
{
  RDIB_BuildSymbolSectionTask *task = raw_task;
  ProfBeginDynamic("Scopes [Chunk Count: %llu]", task->ranges[task_id].max - task->ranges[task_id].min);

  // scope voff fill info
  U64  scope_voff_cursor = task->scope_voff_offsets[task_id];
  U64  scope_voff_max    = task->scope_voff_offsets[task_id] + task->scope_voff_counts[task_id];
  U64 *scope_voff_ptr    = task->scope_voffs_rdi;
 
  // local fill info
  U64 local_cursor  = task->local_offsets[task_id];
  U64 local_max     = task->local_offsets[task_id] + task->local_counts[task_id];
  RDI_Local *locals = task->locals_rdi;

  // location data fill info
  U64  loc_data_max    = task->loc_data_offsets[task_id] + task->loc_data_sizes[task_id];
  U64  loc_data_cursor = task->loc_data_offsets[task_id];
  U8  *loc_data        = task->loc_data_rdi;

  // location block fill info
  U64                loc_block_cursor = task->loc_block_offsets[task_id];
  U64                loc_block_max    = task->loc_block_offsets[task_id] + task->loc_block_counts[task_id];
  RDI_LocationBlock *loc_blocks       = task->loc_blocks_rdi;

  for (U64 ichunk = task->ranges[task_id].min; ichunk < task->ranges[task_id].max; ++ichunk) {
    RDIB_ScopeChunk *chunk = task->scopes_rdib[ichunk];
    for (U64 iscope = 0; iscope < chunk->count; ++iscope) {
      RDIB_Scope *scope_src = &chunk->v[iscope];
      U64         scope_idx = rdib_idx_from_scope(scope_src);
      RDI_Scope  *scope_dst = &task->scopes_rdi[scope_idx];

      scope_dst->proc_idx               = rdib_idx_from_procedure(scope_src->container_proc);
      scope_dst->parent_scope_idx       = rdib_idx_from_scope(scope_src->parent);
      scope_dst->first_child_scope_idx  = rdib_idx_from_scope(scope_src->first_child);
      scope_dst->next_sibling_scope_idx = rdib_idx_from_scope(scope_src->next_sibling);
      scope_dst->voff_range_first       = scope_voff_cursor;
      scope_dst->voff_range_opl         = scope_voff_cursor + scope_src->ranges.count * 2;
      scope_dst->local_count            = scope_src->local_count;
      if (scope_src->local_count > 0) {
        scope_dst->local_first = local_cursor;
      } else {
        scope_dst->local_first = 0;
      }
      // TODO: static locals can be exported as local variables
      //scope_dst->static_local_idx_run_first = ???;
      //scope_dst->static_local_count         = ???;
      scope_dst->inline_site_idx = rdib_idx_from_inline_site(scope_src->inline_site);

      // fill out scope voffs
      for (Rng1U64Node *range_n = scope_src->ranges.first; range_n != 0; range_n = range_n->next) {
        Assert(scope_voff_cursor + 2 <= scope_voff_max);
        scope_voff_ptr[scope_voff_cursor + 0] = range_n->v.min;
        scope_voff_ptr[scope_voff_cursor + 1] = range_n->v.max;
        scope_voff_cursor += 2;
      }

      // fill out locals & locations
      for (RDIB_Variable *local_src = scope_src->local_first; local_src != 0; local_src = local_src->next, ++local_cursor) {
        U64 loc_block_first = loc_block_cursor;

        for (RDIB_LocationNode *loc_n = local_src->locations.first; loc_n != 0; loc_n = loc_n->next) {
          RDIB_Location *loc = &loc_n->v;

          // fill out location data
          U64 location_data_off = loc_data_cursor;
          switch (loc->kind) {
          case RDI_LocationKind_NULL: break;
          case RDI_LocationKind_AddrBytecodeStream:
          case RDI_LocationKind_ValBytecodeStream: {
            // write opcodes & operands
            for (RDIB_EvalBytecodeOp *op_node = loc->bytecode.first; op_node != 0; op_node = op_node->next) {
              // opcode
              Assert(loc_data_cursor + sizeof(op_node->op) <= loc_data_max);
              MemoryCopy(loc_data + loc_data_cursor, &op_node->op, sizeof(op_node->op));
              loc_data_cursor += sizeof(op_node->op);

              // operand
              Assert(loc_data_cursor + op_node->p_size <= loc_data_max);
              MemoryCopy(loc_data + loc_data_cursor, &op_node->p, op_node->p_size);
              loc_data_cursor += op_node->p_size;
            }

            // stream ender
            Assert(loc_data_cursor + 1 <= loc_data_max);
            loc_data[loc_data_cursor] = 0;
            loc_data_cursor += 1;
          } break;
          case RDI_LocationKind_AddrRegPlusU16:
          case RDI_LocationKind_AddrAddrRegPlusU16: {
            Assert(loc_data_cursor + sizeof(RDI_LocationRegPlusU16) <= loc_data_max);
            RDI_LocationRegPlusU16 *dst = (RDI_LocationRegPlusU16 *) (loc_data + loc_data_cursor);
            dst->kind                   = loc->kind;
            dst->reg_code               = loc->reg_code;
            dst->offset                 = loc->offset;

            loc_data_cursor += sizeof(*dst);
          } break;
          case RDI_LocationKind_ValReg: {
            Assert(loc_data_cursor + sizeof(RDI_LocationReg) <= loc_data_max);
            RDI_LocationReg *dst  = (RDI_LocationReg *) (loc_data + loc_data_cursor);
            dst->kind             = loc->kind;
            dst->reg_code         = loc->reg_code;
            loc_data_cursor      += sizeof(*dst);
          } break;
          default: InvalidPath;
          }

          // zero out align bytes
          U64 align_size = AlignPadPow2(loc_data_cursor, 8);
          Assert(loc_data_cursor + align_size <= loc_data_max);
          MemorySet(loc_data + loc_data_cursor, 0, align_size);
          loc_data_cursor += align_size;

          // fill out location block
          for (Rng1U64Node *range_n = loc->ranges.first; range_n != 0; range_n = range_n->next, ++loc_block_cursor) {
            Assert(loc_block_cursor < loc_block_max);
            RDI_LocationBlock *loc_block_dst = &loc_blocks[loc_block_cursor];
            loc_block_dst->scope_off_first   = range_n->v.min;
            loc_block_dst->scope_off_opl     = range_n->v.max;
            loc_block_dst->location_data_off = location_data_off;
          }
        }

        Assert(local_cursor <= local_max);
        RDI_Local *local_dst       = &locals[local_cursor];
        local_dst->kind            = local_src->kind;
        local_dst->name_string_idx = rdib_idx_from_string_map(task->string_map, local_src->name);
        local_dst->type_idx        = rdib_idx_from_type(local_src->type);
        if (local_src->locations.count > 0) {
          local_dst->location_first = loc_block_first;
          local_dst->location_opl   = loc_block_cursor;
        } else {
          local_dst->location_first = 0;
          local_dst->location_opl   = 0;
        }
      }
    }
  }

  Assert(scope_voff_cursor == scope_voff_max);
  Assert(local_cursor == local_max);
  Assert(loc_data_cursor == loc_data_max);

  ProfEnd();
}

internal void
rdib_data_sections_from_scopes(TP_Context            *tp,
                               TP_Arena              *arena,
                               RDIB_DataSectionList  *sect_list,
                               RDIB_StringMap        *string_map,
                               U64                    total_scope_count,
                               U64                    chunk_count,
                               RDIB_ScopeChunk      **scopes)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  RDIB_BuildSymbolSectionTask task = {0};
  task.string_map                  = string_map;
  task.ranges                      = tp_divide_work(scratch.arena, chunk_count, tp->worker_count);
  task.scopes_rdib                 = scopes;
  
  ProfBegin("Count Locals & Locations");
  task.scope_voff_counts = push_array(scratch.arena, U64, tp->worker_count);
  task.local_counts      = push_array(scratch.arena, U64, tp->worker_count);
  task.loc_block_counts  = push_array(scratch.arena, U64, tp->worker_count);
  task.loc_data_sizes    = push_array(scratch.arena, U64, tp->worker_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_count_scopes_task, &task);
  ProfEnd();

  U64 total_scope_voff_count = sum_array_u64(tp->worker_count, task.scope_voff_counts);
  U64 total_local_count      = sum_array_u64(tp->worker_count, task.local_counts     );
  U64 total_loc_block_count  = sum_array_u64(tp->worker_count, task.loc_block_counts );
  U64 total_loc_data_size    = sum_array_u64(tp->worker_count, task.loc_data_sizes   );

  ProfBegin("Fill out scopes, locals, location blocks, and location data");
  task.scope_voff_offsets = offsets_from_counts_array_u64(scratch.arena, task.scope_voff_counts, tp->worker_count);
  task.local_offsets      = offsets_from_counts_array_u64(scratch.arena, task.local_counts,      tp->worker_count);
  task.loc_block_offsets  = offsets_from_counts_array_u64(scratch.arena, task.loc_block_counts,  tp->worker_count);
  task.loc_data_offsets   = offsets_from_counts_array_u64(scratch.arena, task.loc_data_sizes,    tp->worker_count);

  ProfBegin("Push");
  task.scope_voffs_rdi = push_array_no_zero(arena->v[0], U64,               total_scope_voff_count);
  task.scopes_rdi      = push_array_no_zero(arena->v[0], RDI_Scope,         total_scope_count     );
  task.locals_rdi      = push_array_no_zero(arena->v[0], RDI_Local,         total_local_count     );
  task.loc_blocks_rdi  = push_array_no_zero(arena->v[0], RDI_LocationBlock, total_loc_block_count );
  task.loc_data_rdi    = push_array_no_zero(arena->v[0], U8,                total_loc_data_size   );
  ProfEnd();

  tp_for_parallel(tp, 0, tp->worker_count, rdib_build_scopes_task, &task);
  ProfEnd();

  RDIB_DataSection scopes_sect      = { .tag = RDI_SectionKind_Scopes         };
  RDIB_DataSection scope_voffs_sect = { .tag = RDI_SectionKind_ScopeVOffData  };
  RDIB_DataSection locals_sect      = { .tag = RDI_SectionKind_Locals         };
  RDIB_DataSection loc_blocks_sect  = { .tag = RDI_SectionKind_LocationBlocks };
  RDIB_DataSection loc_data_sect    = { .tag = RDI_SectionKind_LocationData   };

  str8_list_push(arena->v[0], &scopes_sect.data,      str8_array(task.scopes_rdi,      total_scope_count     ));
  str8_list_push(arena->v[0], &scope_voffs_sect.data, str8_array(task.scope_voffs_rdi, total_scope_voff_count));
  str8_list_push(arena->v[0], &locals_sect.data,      str8_array(task.locals_rdi,      total_local_count     ));
  str8_list_push(arena->v[0], &loc_blocks_sect.data,  str8_array(task.loc_blocks_rdi,  total_loc_block_count ));
  str8_list_push(arena->v[0], &loc_data_sect.data,    str8_array(task.loc_data_rdi,    total_loc_data_size   ));

  rdib_data_section_list_push(arena->v[0], sect_list, scopes_sect     );
  rdib_data_section_list_push(arena->v[0], sect_list, scope_voffs_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, locals_sect     );
  rdib_data_section_list_push(arena->v[0], sect_list, loc_blocks_sect );
  rdib_data_section_list_push(arena->v[0], sect_list, loc_data_sect   );
  
  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_name_map_task)
{
  ProfBeginFunction("Build Name Map");
  Temp scratch = scratch_begin(&arena, 1);

  RDIB_NameMapBuilderTask *task         = raw_task;
  RDI_NameMapKind          name_map_idx = (RDI_NameMapKind)task_id;

  U64 out_node_count   = task->in_bucket_counts[name_map_idx];
  U64 load_factor      = 4;
  U64 out_bucket_count = CeilIntegerDiv(out_node_count, load_factor);

  ProfBegin("Build temp hash map");
  struct Node {
    struct Node          *next;
    RDIB_StringMapBucket *name;
  };
  struct NodeList {
    struct Node *first;
    struct Node *last;
    U64          node_count;
  };
  struct NodeList *temp_map   = push_array(scratch.arena,         struct NodeList, out_bucket_count);
  struct Node     *temp_nodes = push_array_no_zero(scratch.arena, struct Node,     out_node_count);
  for (U64 i = 0; i < task->in_bucket_counts[name_map_idx]; ++i) {
    RDIB_StringMapBucket *src_bucket = task->in_buckets[name_map_idx][i];

    U64 hash       = rdi_hash(src_bucket->string.str, src_bucket->string.size);
    U64 bucket_idx = hash % out_bucket_count;

    struct Node *node = temp_nodes + i;
    node->next = 0;
    node->name = src_bucket;

    SLLQueuePush(temp_map[bucket_idx].first, temp_map[bucket_idx].last, node);
    ++temp_map[bucket_idx].node_count;
  }
  ProfEnd();

  ProfBegin("Push buckets and nodes");
  RDI_NameMapBucket *out_buckets = push_array_no_zero(arena, RDI_NameMapBucket, out_bucket_count);
  RDI_NameMapNode   *out_nodes   = push_array_no_zero(arena, RDI_NameMapNode,   out_node_count);
  ProfEnd();

  ProfBegin("Fill out buckets");
  for (U64 bucket_idx = 0, node_cursor = 0; bucket_idx < out_bucket_count; ++bucket_idx) {
    struct NodeList   *src_bucket = &temp_map[bucket_idx];
    RDI_NameMapBucket *dst_bucket = &out_buckets[bucket_idx];

    if (src_bucket->node_count == 0) {
      dst_bucket->first_node = 0;
      dst_bucket->node_count = 0;
      continue;
    }

    dst_bucket->first_node = safe_cast_u32(node_cursor);
    dst_bucket->node_count = src_bucket->node_count;

    for (struct Node *n = src_bucket->first; n != 0; n = n->next, ++node_cursor) {
      RDIB_StringMapBucket *src_name = n->name;

      RDI_NameMapNode *dst_node = &out_nodes[node_cursor];
      dst_node->string_idx      = rdib_idx_from_string_map(task->string_map, src_name->string);
      dst_node->match_count     = src_name->count;
      if (src_name->count > 1) {
        dst_node->match_idx_or_idx_run_first = task->idx_run_map->buckets[src_name->idx_run_bucket_idx]->index_in_output_array;
      } else {
        dst_node->match_idx_or_idx_run_first = src_name->match_idx;
      }
    }
  }
  ProfEnd();

  // fill out output
  task->out_buckets[name_map_idx]       = out_buckets;
  task->out_nodes[name_map_idx]         = out_nodes;
  task->out_bucket_counts[name_map_idx] = out_bucket_count;
  task->out_node_counts[name_map_idx]   = out_node_count;

  scratch_end(scratch);
  ProfEnd();
}

internal void
rdib_data_sections_from_name_maps(TP_Context            *tp,
                                  TP_Arena              *arena,
                                  RDIB_DataSectionList  *sect_list,
                                  RDIB_StringMap        *string_map,
                                  RDIB_IndexRunMap      *idx_run_map,
                                  RDIB_StringMapBucket **src_name_maps[RDI_NameMapKind_COUNT],
                                  U64                    src_name_map_counts[RDI_NameMapKind_COUNT])
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Build Name Maps");
  RDIB_NameMapBuilderTask task = {0};
  task.string_map        = string_map;
  task.idx_run_map       = idx_run_map;
  task.in_bucket_counts  = src_name_map_counts;
  task.in_buckets        = src_name_maps;
  task.out_buckets       = push_array(scratch.arena, RDI_NameMapBucket *, RDI_NameMapKind_COUNT);
  task.out_nodes         = push_array(scratch.arena, RDI_NameMapNode *,   RDI_NameMapKind_COUNT);
  task.out_bucket_counts = push_array(scratch.arena, U64,                 RDI_NameMapKind_COUNT);
  task.out_node_counts   = push_array(scratch.arena, U64,                 RDI_NameMapKind_COUNT);
  tp_for_parallel(tp, arena, RDI_NameMapKind_COUNT, rdib_build_name_map_task, &task);
  ProfEnd();

  U64 *bucket_offsets = offsets_from_counts_array_u64(scratch.arena, task.out_bucket_counts, RDI_NameMapKind_COUNT);
  U64 *node_offsets   = offsets_from_counts_array_u64(scratch.arena, task.out_node_counts,   RDI_NameMapKind_COUNT);

  String8List raw_name_maps = {0}, raw_name_map_buckets = {0}, raw_name_map_nodes = {0}; 
  for (U64 i = 0; i < RDI_NameMapKind_COUNT; ++i) {
    RDI_NameMap *dst_name_map = push_array(arena->v[0], RDI_NameMap, 1);
    dst_name_map->bucket_base_idx = bucket_offsets[i];
    dst_name_map->node_base_idx   = node_offsets[i];
    dst_name_map->bucket_count    = task.out_bucket_counts[i];
    dst_name_map->node_count      = task.out_node_counts[i];

    str8_list_push(arena->v[0], &raw_name_maps,        str8_struct(dst_name_map));
    str8_list_push(arena->v[0], &raw_name_map_buckets, str8_array(task.out_buckets[i], task.out_bucket_counts[i]));
    str8_list_push(arena->v[0], &raw_name_map_nodes,   str8_array(task.out_nodes[i],   task.out_node_counts[i]));
  }

  RDIB_DataSection name_maps_sect        = { .tag = RDI_SectionKind_NameMaps,       .data = raw_name_maps        };
  RDIB_DataSection name_map_buckets_sect = { .tag = RDI_SectionKind_NameMapBuckets, .data = raw_name_map_buckets };
  RDIB_DataSection name_map_nodes_sect   = { .tag = RDI_SectionKind_NameMapNodes,   .data = raw_name_map_nodes   };
  rdib_data_section_list_push(arena->v[0], sect_list, name_maps_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, name_map_buckets_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, name_map_nodes_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_src_line_map_task)
{
  // Line tables are ordered to perform 'virtual offset -> line number' maps,
  // and thus we potentially can have multiple virtual offsets map to same line number.
  // (e.g. in C/C++ if for-loop declaration site is on one line, intial-statment-condition
  // and expression parts map to same line number). And so to make things easy on debugger
  // we remove duplicates from source line map and reorient mapping to 'line number -> virtual offset'
  // this way debugger can quickly compute virtual offsets when placing a breakpoint on a source line.

  Temp                  scratch = scratch_begin(&arena, 1);
  RDIB_SrcLineMapsTask *task    = raw_task;

  RDIB_SourceFile *src_file     = task->src_file_arr[task_id];
  U64              src_file_idx = rdib_idx_from_source_file(src_file);

  //ProfBeginDynamic("Build Source Line Map [%.*s]", str8_varg(src_file->file_path));
  ProfBegin("Build Source Line Map");

  ProfBegin("Count lines/virt offsets");
  U64 ln_voff_count = 0;
  for (RDIB_LineTableFragment *frag = src_file->line_table_frags; frag != 0; frag = frag->next_src_file) {
    ln_voff_count += frag->line_count;
  }
  ProfEnd();

  ProfBegin("Push ln_voff_arr");
  PairU32 *ln_voff_arr = push_array_no_zero(scratch.arena, PairU32, ln_voff_count);
  ProfEnd();

  ProfBegin("Fill out ln_voff_arr");
  {
    U64 cursor = 0;
    for (RDIB_LineTableFragment *frag = src_file->line_table_frags; frag != 0; frag = frag->next_src_file) {
      for (U64 line_idx = 0; line_idx < frag->line_count; ++line_idx) {
        ln_voff_arr[cursor].v0 = frag->line_nums[line_idx];
        ln_voff_arr[cursor].v1 = frag->voffs[line_idx];
        ++cursor;
      }
    }
  }
  ProfEnd();

  // sort on line number
  ProfBegin("Sort");
  if (ln_voff_count < 512) {
    // TODO: Radsort is buggy and inifte loops if we sort pair of u64.
    // Check-in with Jeff on Monday about bugfix. For now workaround
    // the bug wiht pair of u32s. There is no virtual offset larger
    // than 4GiB in line table anyway.
    radsort(ln_voff_arr, ln_voff_count, pair_u32_is_before_v0);
  } else {
    u32_pair_radix_sort(ln_voff_count, ln_voff_arr);
  }
  ProfEnd();

  // TODO: leak, precompute unique line number count and push exact array lengths
  U32 *line_nums   = push_array_no_zero(arena, U32, ln_voff_count);
  U32 *line_ranges = push_array_no_zero(arena, U32, ln_voff_count + 1);
  U64 *voffs       = push_array_no_zero(arena, U64, ln_voff_count);

  U64 voff_cursor     = 0;
  U64 line_num_cursor = 0;
  if (ln_voff_count > 0) {
    line_nums[line_num_cursor]   = ln_voff_arr[0].v0;
    voffs[voff_cursor]           = ln_voff_arr[0].v1;
    line_ranges[line_num_cursor] = voff_cursor;

    ++voff_cursor;
    ++line_num_cursor;

    ProfBegin("Fill out output array");
    for (U64 i = 1; i < ln_voff_count; ++i) {
      // does this voff belong to next line number?
      if (ln_voff_arr[i].v0 != line_nums[line_num_cursor-1]) {
        line_nums[line_num_cursor]   = ln_voff_arr[i].v0;
        line_ranges[line_num_cursor] = (U32)voff_cursor;
        ++line_num_cursor;
      }
      voffs[voff_cursor++] = ln_voff_arr[i].v1;
    }
    ProfEnd();

    // did we fill out voff array correctly?
    Assert(voff_cursor == ln_voff_count);

    // close last line range
    line_ranges[line_num_cursor] = voff_cursor;
  }

  // fill out result
  task->out_line_counts[src_file_idx] = line_num_cursor;
  task->out_voff_counts[src_file_idx] = safe_cast_u32(voff_cursor);
  task->out_line_nums[src_file_idx]   = line_nums;
  task->out_line_ranges[src_file_idx] = line_ranges;
  task->out_voffs[src_file_idx]       = voffs;

  scratch_end(scratch);
  ProfEnd();
}

internal void
rdib_data_sections_from_source_line_maps(TP_Context            *tp,
                                         TP_Arena              *arena,
                                         RDIB_DataSectionList  *sect_list,
                                         U64                    total_src_file_count,
                                         U64                    src_file_chunk_count,
                                         RDIB_SourceFileChunk **src_file_chunks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  ProfBegin("Prepare Source File Array");
  RDIB_SourceFile **src_file_arr = push_array_no_zero(scratch.arena, RDIB_SourceFile *, total_src_file_count);
  for (U64 chunk_idx = 0, cursor = 0; chunk_idx < src_file_chunk_count; ++chunk_idx) {
    RDIB_SourceFileChunk *chunk = src_file_chunks[chunk_idx];  
    for (U64 i = 0; i < chunk->count; ++i) {
      src_file_arr[cursor++] = &chunk->v[i];
    }
  }
  ProfEnd();

  ProfBegin("Init Task Context");
  RDIB_SrcLineMapsTask task = {0};
  task.src_file_arr    = src_file_arr;
  task.out_line_counts = push_array_no_zero(scratch.arena, U32,   total_src_file_count);
  task.out_voff_counts = push_array_no_zero(scratch.arena, U32,   total_src_file_count);
  task.out_line_nums   = push_array_no_zero(scratch.arena, U32 *, total_src_file_count);
  task.out_line_ranges = push_array_no_zero(scratch.arena, U32 *, total_src_file_count);
  task.out_voffs       = push_array_no_zero(scratch.arena, U64 *, total_src_file_count);
  ProfEnd();

  ProfBegin("Build Source Line Maps");
  tp_for_parallel(tp, arena, total_src_file_count, rdib_build_src_line_map_task, &task);
  ProfEnd();

  ProfBegin("Fill out Source Line Maps");
  RDIB_DataSection src_line_maps_sect   = { .tag = RDI_SectionKind_SourceLineMaps       };
  RDIB_DataSection src_line_nums_sect   = { .tag = RDI_SectionKind_SourceLineMapNumbers };
  RDIB_DataSection src_line_ranges_sect = { .tag = RDI_SectionKind_SourceLineMapRanges  };
  RDIB_DataSection src_line_voffs_sect  = { .tag = RDI_SectionKind_SourceLineMapVOffs   };

  ProfBegin("Push");
  RDI_SourceLineMap *src_line_maps = push_array_no_zero(arena->v[0], RDI_SourceLineMap, total_src_file_count + 1);
  ProfEnd();

  U64 src_line_map_cursor = 0;
  U64 line_num_cursor     = 0;
  U64 line_range_cursor   = 0;
  U64 voff_cursor         = 0;

  // zero-out null source line map
  MemoryZeroStruct(&src_line_maps[src_line_map_cursor]);
  ++src_line_map_cursor;

  for (U64 chunk_idx = 0; chunk_idx < src_file_chunk_count; ++chunk_idx) {
    RDIB_SourceFileChunk *chunk = src_file_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_SourceFile *src_file     = chunk->v + i;
      U64              src_file_idx = rdib_idx_from_source_file(src_file);

      if (task.out_line_counts[src_file_idx] > 0) {
        src_file->src_line_map_idx = src_line_map_cursor;

        RDI_SourceLineMap *sm = src_line_maps + src_line_map_cursor++;
        sm->line_count              = task.out_line_counts[src_file_idx];
        sm->voff_count              = task.out_voff_counts[src_file_idx];
        sm->line_map_nums_base_idx  = line_num_cursor;
        sm->line_map_range_base_idx = line_range_cursor;
        sm->line_map_voff_base_idx  = voff_cursor;

        str8_list_push(arena->v[0], &src_line_nums_sect.data,   str8_array(task.out_line_nums[src_file_idx],   task.out_line_counts[src_file_idx]));
        str8_list_push(arena->v[0], &src_line_ranges_sect.data, str8_array(task.out_line_ranges[src_file_idx], task.out_line_counts[src_file_idx] + 1));
        str8_list_push(arena->v[0], &src_line_voffs_sect.data,  str8_array(task.out_voffs[src_file_idx],       task.out_voff_counts[src_file_idx]));
        
        line_num_cursor   += task.out_line_counts[src_file_idx];
        line_range_cursor += task.out_line_counts[src_file_idx] + 1;
        voff_cursor       += task.out_voff_counts[src_file_idx];
      } else {
        src_file->src_line_map_idx = 0;
      }
    }
  }
  ProfEnd();

  str8_list_push(arena->v[0], &src_line_maps_sect.data, str8_array(src_line_maps, src_line_map_cursor));

  rdib_data_section_list_push(arena->v[0], sect_list, src_line_maps_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, src_line_nums_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, src_line_ranges_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, src_line_voffs_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_build_line_tables_task)
{
  ProfBeginFunction();
  Temp                      scratch = scratch_begin(&arena, 1);
  RDIB_BuildLineTablesTask *task    = raw_task;
  Rng1U64                   range   = task->ranges[task_id];

  for (U64 chunk_idx = range.min; chunk_idx < range.max; ++chunk_idx) {
    RDIB_LineTableChunk *chunk = task->chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_LineTable *line_table     = &chunk->v[i];
      U64             line_table_idx = chunk->base + i;

      U64 total_line_count = 0;
      for (RDIB_LineTableFragment *frag = line_table->first; frag != 0; frag = frag->next_line_table) {
        total_line_count += frag->line_count + /* range terminator */ 1;
      }

      if (total_line_count > 0) {
        struct Value {
          U32 src_file_idx;
          U32 line_num;
          U16 col_first;
          U16 col_opl;
        };
        KeyValuePair *pairs       = push_array_no_zero(scratch.arena, KeyValuePair, total_line_count);
        struct Value *values      = push_array_no_zero(scratch.arena, struct Value, total_line_count);
        U64           pair_cursor = 0;

        for (RDIB_LineTableFragment *frag = line_table->first; frag != 0; frag = frag->next_line_table) {
          for (U64 line_idx = 0; line_idx < frag->line_count; ++line_idx, ++pair_cursor) {
            struct Value *value = &values[pair_cursor];
            KeyValuePair *pair  = &pairs[pair_cursor];

            value->src_file_idx = rdib_idx_from_source_file(frag->src_file);
            value->line_num     = frag->line_nums[line_idx];
            if (frag->col_count > 0) {
              value->col_first = frag->col_nums[line_idx*2];
              value->col_opl   = frag->col_nums[line_idx*2 + 1];
            } else {
              value->col_first = 0;
              value->col_opl   = 0;
            }

            pair->key_u64      = frag->voffs[line_idx];
            pair->value_raw    = value;
          }

          // emit terminator
          {
            KeyValuePair *pair  = &pairs[pair_cursor];
            struct Value *value = &values[pair_cursor];
            pair_cursor += 1;

            value->src_file_idx = 0;
            value->line_num     = 0;
            value->col_first    = 0;
            value->col_opl      = 0;

            pair->key_u64 = frag->voffs[frag->line_count];
            pair->value_raw = value;
          }
        }

        // sort on virtual offset
        sort_key_value_pairs_as_u64(pairs, pair_cursor);

        // fill out RDI_Line output
        U64       line_count = pair_cursor + 1;
        U64      *voffs      = push_array_no_zero(arena, U64,      line_count);
        RDI_Line *lines      = push_array_no_zero(arena, RDI_Line, line_count);

        U64 line_cursor = 0;
        for (U64 line_idx = 0; line_idx < pair_cursor; ++line_idx) {
          // remove terminator if there is a real line number
          if (line_idx + 1 < pair_cursor && pairs[line_idx].key_u64 == pairs[line_idx+1].key_u64) {
            continue;
          }
          struct Value *value         = pairs[line_idx].value_raw;
          voffs[line_cursor]          = pairs[line_idx].key_u64;
          lines[line_cursor].file_idx = value->src_file_idx;
          lines[line_cursor].line_num = value->line_num;
          line_cursor += 1;
        }

        // fill out terminators
        voffs[line_cursor] = ~0llu;
        MemoryZeroStruct(&lines[line_cursor]);
        line_cursor += 1;

        // fill out line table output
        task->out_line_table_counts[line_table_idx] = line_cursor;
        task->out_line_table_voffs[line_table_idx]  = voffs;
        task->out_line_table_lines[line_table_idx]  = lines;
      } else {
        task->out_line_table_counts[line_table_idx] = 0;
        task->out_line_table_voffs[line_table_idx]  = 0;
        task->out_line_table_lines[line_table_idx]  = 0;
      }
    }
  }

  scratch_end(scratch);
  ProfEnd();
}

internal void
rdib_data_sections_from_line_tables(TP_Context            *tp,
                                    TP_Arena              *arena,
                                    RDIB_DataSectionList  *sect_list,
                                    U64                    total_line_table_count,
                                    U64                    chunk_count,
                                    RDIB_LineTableChunk  **chunks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  ProfBegin("Build Line Tables");
  RDIB_BuildLineTablesTask task = {0};
  task.chunks                   = chunks;
  task.ranges                   = tp_divide_work(scratch.arena, chunk_count, tp->worker_count);
  task.out_line_table_counts    = push_array_no_zero(scratch.arena, U64,        total_line_table_count);
  task.out_line_table_voffs     = push_array_no_zero(scratch.arena, U64 *,      total_line_table_count);
  task.out_line_table_lines     = push_array_no_zero(scratch.arena, RDI_Line *, total_line_table_count);
  tp_for_parallel(tp, arena, tp->worker_count, rdib_build_line_tables_task, &task);
  ProfEnd();

  RDIB_DataSection line_tables_sect      = { .tag = RDI_SectionKind_LineTables      };
  RDIB_DataSection line_table_voffs_sect = { .tag = RDI_SectionKind_LineInfoVOffs   };
  RDIB_DataSection line_table_lines_sect = { .tag = RDI_SectionKind_LineInfoLines   };
  RDIB_DataSection line_table_cols_sect  = { .tag = RDI_SectionKind_LineInfoColumns };

  ProfBegin("Fill out Line Tables");

  ProfBegin("Push");
  RDI_LineTable *line_tables_rdi = push_array_no_zero(arena->v[0], RDI_LineTable, total_line_table_count);
  ProfEnd();

  U64 line_table_cursor      = 0;
  U64 line_table_voff_cursor = 0;
  U64 line_table_line_cursor = 0;

  for (U64 chunk_idx = 0; chunk_idx < chunk_count; ++chunk_idx) {
    RDIB_LineTableChunk *chunk = chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_LineTable *src     = &chunk->v[i];
      U64             src_idx = rdib_idx_from_line_table(src);
      if (task.out_line_table_counts[src_idx] > 0) {
        RDI_LineTable *dst = &line_tables_rdi[line_table_cursor];

        src->output_array_idx = line_table_cursor;

        dst->voffs_base_idx = line_table_voff_cursor;
        dst->lines_base_idx = line_table_line_cursor;
        dst->cols_base_idx  = 0;
        dst->lines_count    = task.out_line_table_counts[src_idx] - 1;
        dst->cols_count     = 0;

        str8_list_push(arena->v[0], &line_table_voffs_sect.data, str8_array(task.out_line_table_voffs[src_idx], task.out_line_table_counts[src_idx]));
        str8_list_push(arena->v[0], &line_table_lines_sect.data, str8_array(task.out_line_table_lines[src_idx], task.out_line_table_counts[src_idx]));

        line_table_voff_cursor += task.out_line_table_counts[src_idx];
        line_table_line_cursor += task.out_line_table_counts[src_idx];

        line_table_cursor += 1;
      } else {
        src->output_array_idx = 0;
      }
    }
  }

  str8_list_push(arena->v[0], &line_tables_sect.data, str8_array(line_tables_rdi, line_table_cursor));

  ProfEnd();

  rdib_data_section_list_push(arena->v[0], sect_list, line_tables_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, line_table_voffs_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, line_table_lines_sect);
  rdib_data_section_list_push(arena->v[0], sect_list, line_table_cols_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal
THREAD_POOL_TASK_FUNC(rdib_fill_src_files_task)
{
  RDIB_FillSourceFilesTask *task = raw_task;

  for (U64 chunk_idx = task->ranges[task_id].min; chunk_idx < task->ranges[task_id].max; ++chunk_idx) {
    RDIB_SourceFileChunk *chunk = task->src_file_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_SourceFile *src          = chunk->v + i;
      U32              src_file_idx = rdib_idx_from_source_file(src);
      RDI_SourceFile  *dst          = task->src_files_dst + src_file_idx;

      dst->file_path_node_idx          = rdib_idx_from_path_tree(task->path_tree, src->file_path);
      dst->normal_full_path_string_idx = rdib_idx_from_string_map(task->string_map, src->normal_full_path);
      dst->source_line_map_idx         = src->src_line_map_idx;
    }
  }
}

internal void
rdib_data_sections_from_source_files(TP_Context            *tp,
                                     TP_Arena              *arena,
                                     RDIB_DataSectionList  *sect_list,
                                     RDIB_StringMap        *string_map,
                                     RDIB_PathTree         *path_tree,
                                     U64                    total_src_file_count,
                                     U64                    src_file_chunk_count,
                                     RDIB_SourceFileChunk **src_file_chunks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  RDIB_FillSourceFilesTask task = {0};
  task.ranges          = tp_divide_work(scratch.arena, src_file_chunk_count, tp->worker_count);
  task.string_map      = string_map;
  task.path_tree       = path_tree;
  task.src_file_chunks = src_file_chunks;
  task.src_files_dst   = push_array_no_zero(arena->v[0], RDI_SourceFile, total_src_file_count);
  tp_for_parallel(tp, 0, tp->worker_count, rdib_fill_src_files_task, &task);

  RDIB_DataSection src_files_sect = { .tag = RDI_SectionKind_SourceFiles };
  str8_list_push(arena->v[0], &src_files_sect.data, str8_array(task.src_files_dst, total_src_file_count));
  rdib_data_section_list_push(arena->v[0], sect_list, src_files_sect);

  scratch_end(scratch);
  ProfEnd();
}

internal void
rdib_data_sections_from_inline_sites(TP_Context *tp,
                                     Arena *arena,
                                     RDIB_DataSectionList *sect_list,
                                     RDIB_StringMap *string_map,
                                     U64 total_inline_site_count,
                                     U64 inline_site_chunk_count,
                                     RDIB_InlineSiteChunk **inline_site_chunks)
{
  ProfBeginFunction();

  RDI_InlineSite *dst_arr = push_array(arena, RDI_InlineSite, total_inline_site_count);

  for (U64 chunk_idx = 0; chunk_idx < inline_site_chunk_count; ++chunk_idx) {
    RDIB_InlineSiteChunk *chunk = inline_site_chunks[chunk_idx];
    for (U64 i = 0; i < chunk->count; ++i) {
      RDIB_InlineSite *src = &chunk->v[i];
      U64 idx = rdib_idx_from_inline_site(src);
      RDI_InlineSite *dst = &dst_arr[idx];

      dst->name_string_idx = rdib_idx_from_string_map(string_map, src->name);
      dst->type_idx        = rdib_idx_from_type(src->type);
      dst->owner_type_idx  = rdib_idx_from_type(src->owner);
      dst->line_table_idx  = src->line_table->output_array_idx;
    }
  }

  RDIB_DataSection inline_site_sect = { .tag = RDI_SectionKind_InlineSites };
  str8_list_push(arena, &inline_site_sect.data, str8_array(dst_arr, total_inline_site_count));
  rdib_data_section_list_push(arena, sect_list, inline_site_sect);

  ProfEnd();
}

internal void
rdib_data_sections_from_checksums(TP_Context *tp, Arena *arena, RDIB_DataSectionList *sect_list)
{
  NotImplemented;
}

////////////////////////////////

internal RDIB_Input
rdib_init_input(Arena *arena)
{
  ProfBeginFunction();

  RDIB_Input input         = {0};
  input.unit_chunk_cap     = 128;
  input.src_file_chunk_cap = 4096;
  input.symbol_chunk_cap   = 4096;
  input.line_table_cap     = 4096;
  input.inline_site_cap    = 4096;
  input.type_cap           = 1024;
  input.udt_cap            = 4096;

  RDIB_SourceFile        *null_src_file    = rdib_source_file_chunk_list_push_zero(arena, &input.src_files,    1);
  RDIB_LineTable         *null_line_table  = rdib_line_table_chunk_list_push_zero (arena, &input.line_tables,  1);
  RDIB_LineTableFragment *null_frag        = rdib_line_table_push                 (arena, null_line_table);
  RDIB_Type              *null_type        = rdib_type_chunk_list_push_zero       (arena, &input.types,        1);
  RDIB_Scope             *null_scope       = rdib_scope_chunk_list_push_zero      (arena, &input.scopes,       1);
  RDIB_Unit              *null_unit        = rdib_unit_chunk_list_push_zero       (arena, &input.units,        1);
  RDIB_Procedure         *null_proc        = rdib_procedure_chunk_list_push_zero  (arena, &input.procs,        1);
  RDIB_Variable          *null_local       = rdib_variable_chunk_list_push_zero   (arena, &input.locals,       1);
  RDIB_Variable          *null_gvar        = rdib_variable_chunk_list_push_zero   (arena, &input.gvars,        1);
  RDIB_Variable          *null_tvar        = rdib_variable_chunk_list_push_zero   (arena, &input.tvars,        1);
  RDIB_UDTMember         *null_udt_member  = rdib_udt_member_chunk_list_push_zero (arena, &input.udt_members,  1);
  RDIB_UDTMember         *null_enum_member = rdib_udt_member_chunk_list_push_zero (arena, &input.enum_members, 1);
  RDIB_InlineSite        *null_inline_site = rdib_inline_site_chunk_list_push_zero(arena, &input.inline_sites, 1);
  {
    // Line Table Fragment
    null_frag->src_file = null_src_file;
    null_frag->voffs    = push_array(arena, U64, 1);

    // Source File
    null_src_file->line_table_frags = null_frag;

    // Unit
    null_unit->arch             = RDI_Arch_NULL;
    null_unit->unit_name        = str8_zero();
    null_unit->compiler_name    = str8_zero();
    null_unit->source_file      = str8_zero();
    null_unit->object_file      = str8_zero();
    null_unit->archive_file     = str8_zero();
    null_unit->build_path       = str8_zero();
    null_unit->virt_range_count = 1;
    null_unit->virt_ranges      = push_array(arena, Rng1U64, 1);
    null_unit->virt_ranges[0]   = rng_1u64(0,0);
    null_unit->line_table       = null_line_table;

    // Scope
    rng1u64_list_push(arena, &null_scope->ranges, rng_1u64(0,max_U32));

    // Location
    RDIB_Location null_loc = {0};
    rng1u64_list_push(arena, &null_loc.ranges, rng_1u64(0,0));
    RDIB_LocationList null_loc_list = {0};
    rdib_location_list_push(arena, &null_loc_list, null_loc);

    // Proc
    null_proc->type  = null_type;
    null_proc->scope = null_scope;

    // Global Var
    null_gvar->link_flags = RDI_LinkFlag_External;
    null_gvar->type       = null_type;
    null_gvar->locations  = null_loc_list;

    // Thread Var
    null_tvar->link_flags = RDI_LinkFlag_External;
    null_tvar->type       = null_type;
    null_tvar->locations  = null_loc_list;

    // Local Var
    null_local->type      = null_type;
    null_local->locations = null_loc_list;

    // Inline Site
    null_inline_site->type       = null_type;
    null_inline_site->owner      = 0;
    null_inline_site->line_table = null_line_table;
  }

  input.null_src_file    = null_src_file;
  input.null_line_table  = null_line_table;
  input.null_frag        = null_frag;
  input.null_type        = null_type;
  input.null_scope       = null_scope;
  input.null_unit        = null_unit;
  input.null_proc        = null_proc;
  input.null_local       = null_local;
  input.null_gvar        = null_gvar;
  input.null_tvar        = null_tvar;
  input.null_udt_member  = null_udt_member;
  input.null_enum_member = null_enum_member;
  input.null_inline_site = null_inline_site;

  input.variadic_type       = rdib_type_chunk_list_push(arena, &input.types, 1);
  input.variadic_type->kind = RDI_TypeKind_Variadic;

  ProfEnd();
  return input;
}

internal String8List
rdib_finish(TP_Context *tp, TP_Arena *arena, RDIB_Input *input)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  RDIB_UnitChunkList       all_units             = {0};
  RDIB_SourceFileChunkList all_src_files         = {0};
  RDIB_LineTableChunkList  all_line_tables       = {0};
  RDIB_VariableChunkList   all_locals            = {0};
  RDIB_VariableChunkList   all_tvars             = {0};
  RDIB_VariableChunkList   all_gvars             = {0};
  RDIB_ProcedureChunkList  all_procs             = {0};
  RDIB_ScopeChunkList      all_scopes            = {0};
  RDIB_InlineSiteChunkList all_inline_sites      = {0};
  RDIB_TypeChunkList       all_types             = {0};
  RDIB_TypeChunkList       all_param_types       = {0};
  RDIB_TypeChunkList       all_udt_member_types  = {0};
  RDIB_TypeChunkList       all_enum_member_types = {0};
  RDIB_UDTMemberChunkList  all_udt_members       = {0};
  RDIB_UDTMemberChunkList  all_enum_members      = {0};

  //U64 type_chunk_count        = types.count;
  //U64 struct_chunk_count      = struct_list.count;
  //U64 union_chunk_count       = union_list.count;
  //U64 enum_chunk_count        = enum_list.count;
  //U64 total_struct_count      = rdib_type_chunk_list_total_count(struct_list);
  //U64 total_union_count       = rdib_type_chunk_list_total_count(union_list);
  //U64 total_enum_count        = rdib_type_chunk_list_total_count(enum_list);
  //U64 extern_gvar_chunk_count = extern_gvars.count;
  //U64 extern_tvar_chunk_count = extern_tvars.count;
  //U64 extern_proc_chunk_count = extern_procs.count;
  //U64 static_gvar_chunk_count = static_gvars.count;
  //U64 static_tvar_chunk_count = static_tvars.count;
  //U64 static_proc_chunk_count = static_procs.count;
  //U64 total_extern_gvar_count = rdib_variable_chunk_list_total_count (extern_gvars);
  //U64 total_extern_tvar_count = rdib_variable_chunk_list_total_count (extern_tvars);
  //U64 total_extern_proc_count = rdib_procedure_chunk_list_total_count(extern_procs);

  ProfBegin("Concat Chunk Lists");
  rdib_unit_chunk_list_concat_in_place       (&all_units,             &input->units            );
  rdib_source_file_chunk_list_concat_in_place(&all_src_files,         &input->src_files        );
  rdib_line_table_chunk_list_concat_in_place (&all_line_tables,       &input->line_tables      );
  rdib_scope_chunk_list_concat_in_place      (&all_scopes,            &input->scopes           );
  rdib_variable_chunk_list_concat_in_place   (&all_locals,            &input->locals           );
  rdib_variable_chunk_list_concat_in_place   (&all_tvars,             &input->tvars            );
  rdib_variable_chunk_list_concat_in_place   (&all_tvars,             &input->extern_tvars     );
  rdib_variable_chunk_list_concat_in_place   (&all_tvars,             &input->static_tvars     );
  rdib_variable_chunk_list_concat_in_place   (&all_gvars,             &input->gvars            );
  rdib_variable_chunk_list_concat_in_place   (&all_gvars,             &input->extern_gvars     );
  rdib_variable_chunk_list_concat_in_place   (&all_gvars,             &input->static_gvars     );
  rdib_procedure_chunk_list_concat_in_place  (&all_procs,             &input->procs            );
  rdib_procedure_chunk_list_concat_in_place  (&all_procs,             &input->extern_procs     );
  rdib_procedure_chunk_list_concat_in_place  (&all_procs,             &input->static_procs     );
  rdib_inline_site_chunk_list_concat_in_place(&all_inline_sites,      &input->inline_sites     );
  rdib_type_chunk_list_concat_in_place       (&all_types,             &input->types            );
  rdib_type_chunk_list_concat_in_place       (&all_types,             &input->struct_list      );
  rdib_type_chunk_list_concat_in_place       (&all_types,             &input->union_list       );
  rdib_type_chunk_list_concat_in_place       (&all_types,             &input->enum_list        );
  rdib_type_chunk_list_concat_in_place       (&all_param_types,       &input->param_types      );
  rdib_type_chunk_list_concat_in_place       (&all_udt_member_types,  &input->member_types     );
  rdib_type_chunk_list_concat_in_place       (&all_enum_member_types, &input->enum_types       );
  rdib_udt_member_chunk_list_concat_in_place (&all_udt_members,       &input->udt_members      );
  rdib_udt_member_chunk_list_concat_in_place (&all_enum_members,      &input->enum_members     );
  ProfEnd();

  ProfBegin("Chunk Lists -> Chunk Arrays");
  RDIB_UnitChunk       **all_unit_chunks             = rdib_array_from_unit_chunk_list       (scratch.arena, all_units            );
  RDIB_SourceFileChunk **all_src_file_chunks         = rdib_array_from_source_file_chunk_list(scratch.arena, all_src_files        );
  RDIB_LineTableChunk  **all_line_table_chunks       = rdib_array_from_line_table_chunk_list (scratch.arena, all_line_tables      );
  RDIB_ScopeChunk      **all_scope_chunks            = rdib_array_from_scope_chunk_list      (scratch.arena, all_scopes           );
  RDIB_VariableChunk   **all_local_chunks            = rdib_array_from_variable_chunk_list   (scratch.arena, all_locals           );
  RDIB_VariableChunk   **all_gvar_chunks             = rdib_array_from_variable_chunk_list   (scratch.arena, all_gvars            );
  RDIB_VariableChunk   **all_tvar_chunks             = rdib_array_from_variable_chunk_list   (scratch.arena, all_tvars            );
  RDIB_ProcedureChunk  **all_proc_chunks             = rdib_array_from_procedure_chunk_list  (scratch.arena, all_procs            );
  RDIB_InlineSiteChunk **all_inline_site_chunks      = rdib_array_from_inline_site_chunk_list(scratch.arena, all_inline_sites     );
  RDIB_TypeChunk       **all_type_chunks             = rdib_array_from_type_chunk_list       (scratch.arena, all_types            );
  //RDIB_TypeChunk       **all_param_type_chunks       = rdib_array_from_type_chunk_list       (scratch.arena, all_param_types      );
  RDIB_TypeChunk       **all_udt_member_type_chunks  = rdib_array_from_type_chunk_list       (scratch.arena, all_udt_member_types );
  RDIB_TypeChunk       **all_enum_member_type_chunks = rdib_array_from_type_chunk_list       (scratch.arena, all_enum_member_types);
  RDIB_UDTMemberChunk  **all_udt_member_chunks       = rdib_array_from_udt_member_chunk_list (scratch.arena, all_udt_members      );
  RDIB_UDTMemberChunk  **all_enum_member_chunks      = rdib_array_from_udt_member_chunk_list (scratch.arena, all_enum_members     );
  ProfEnd();

  ProfBegin("Count Symbols, Types, and etc.");
  U64 total_unit_count             = rdib_unit_chunk_list_total_count       (all_units            );
  U64 total_src_file_count         = rdib_source_file_chunk_list_total_count(all_src_files        );
  U64 total_line_table_count       = rdib_line_table_chunk_list_total_count (all_line_tables      );
  U64 total_scope_count            = rdib_scope_chunk_list_total_count      (all_scopes           );
  U64 total_local_count            = rdib_variable_chunk_list_total_count   (all_locals           );
  U64 total_inline_site_count      = rdib_inline_site_chunk_list_total_count(all_inline_sites     );
  U64 total_udt_member_count       = rdib_udt_member_chunk_list_total_count (all_udt_members      );
  U64 total_enum_member_count      = rdib_udt_member_chunk_list_total_count (all_enum_members     );
  U64 total_type_count             = rdib_type_chunk_list_total_count       (all_types            );
  U64 total_param_type_count       = rdib_type_chunk_list_total_count       (all_param_types      );
  //U64 total_udt_member_type_count  = rdib_type_chunk_list_total_count       (all_udt_member_types );
  //U64 total_enum_member_type_count = rdib_type_chunk_list_total_count       (all_enum_member_types); 
  U64 total_tvar_count             = rdib_variable_chunk_list_total_count   (all_tvars            );
  U64 total_gvar_count             = rdib_variable_chunk_list_total_count   (all_gvars            );
  U64 total_proc_count             = rdib_procedure_chunk_list_total_count  (all_procs            );
  ProfEnd();

  // +1 to skip nulls
  //RDIB_VariableChunk  **extern_gvar_chunks = all_gvar_chunks + 1;
  //RDIB_VariableChunk  **extern_tvar_chunks = all_tvar_chunks + 1;
  //RDIB_ProcedureChunk **extern_proc_chunks = all_proc_chunks + 1;
  //RDIB_VariableChunk  **static_gvar_chunks = extern_gvar_chunks + extern_gvar_chunk_count;
  //RDIB_VariableChunk  **static_tvar_chunks = extern_tvar_chunks + extern_tvar_chunk_count;
  //RDIB_ProcedureChunk **static_proc_chunks = extern_proc_chunks + extern_proc_chunk_count;
  //RDIB_TypeChunk      **type_chunks        = all_type_chunks + 1;
  //RDIB_TypeChunk      **struct_chunks      = type_chunks + type_chunk_count;
  //RDIB_TypeChunk      **union_chunks       = struct_chunks + struct_chunk_count;
  //RDIB_TypeChunk      **enum_chunks        = union_chunks + union_chunk_count;
  //RDIB_TypeChunk      **udt_chunks         = struct_chunks;
  //U64                  udt_chunk_count     = struct_chunk_count + union_chunk_count + enum_chunk_count;

  ProfBegin("Assign Type Indices");
  U64 total_type_node_count = 1;
  {
    struct TypeNode {
      struct TypeNode *next;
      RDIB_Type       *type;
    };
    struct TypeNode *stack      = 0;
    struct TypeNode *free_nodes = 0;
#define push_node(t) do {                                           \
if (((RDIB_Type*)(t))->kind == RDI_TypeKindExt_VirtualTable) break; \
  struct TypeNode *n;                                               \
  if (free_nodes == 0) {                                            \
    n = push_array(scratch.arena, struct TypeNode, 1);              \
  } else {                                                          \
    n = free_nodes;                                                 \
    SLLStackPop(free_nodes);                                        \
  }                                                                 \
  Assert(t);                                                        \
  n->type = t;                                                      \
  SLLStackPush(stack, n);                                           \
} while (0)

    for (U64 chunk_idx = 0; chunk_idx < all_types.count; ++chunk_idx) {
      RDIB_TypeChunk *chunk = all_type_chunks[chunk_idx];
      for (U64 i = 0; i < chunk->count; ++i) {
        push_node(&chunk->v[i]);

        for (struct TypeNode *cursor = stack; cursor != 0; cursor = cursor->next) {
          if (cursor->type->kind == RDI_TypeKind_NULL){
            // no type refs
          } else if (cursor->type->kind == RDI_TypeKind_Variadic) {
            // no type refs
          } else if (cursor->type->kind == RDI_TypeKind_Union) {
            // no type refs
          } else if (RDI_IsBuiltinType(cursor->type->kind)) {
            // no type refs
          } else if (cursor->type->kind == RDI_TypeKind_IncompleteStruct) {
            // no type refs
          } else if (cursor->type->kind == RDI_TypeKind_IncompleteUnion) {
            // no type refs
          } else if (cursor->type->kind == RDI_TypeKind_IncompleteClass) {
            // no type refs
          } else if (cursor->type->kind == RDI_TypeKind_IncompleteEnum) {
            push_node(cursor->type->udt.enum_type.base_type);
          } else if (cursor->type->kind == RDI_TypeKind_Modifier) {
            push_node(cursor->type->modifier.type_ref);
          } else if (RDI_IsPtrType(cursor->type->kind)) {
            push_node(cursor->type->ptr.type_ref);
          } else if (cursor->type->kind == RDI_TypeKind_Function) {
            push_node(cursor->type->func.return_type);
            push_node(cursor->type->func.params_type);
            RDIB_Type *params = cursor->type->func.params_type;
            for (U64 i = 0; i < params->params.count; ++i) {
              push_node(params->params.types[i]);
            }
          } else if (cursor->type->kind == RDI_TypeKind_Method) {
            push_node(cursor->type->method.class_type);
            push_node(cursor->type->method.this_type);
            push_node(cursor->type->method.return_type);
            RDIB_Type *params = cursor->type->method.params_type;
            for (U64 i = 0; i < params->params.count; ++i) {
              push_node(params->params.types[i]);
            }
          } else if (cursor->type->kind == RDI_TypeKindExt_StaticMethod) {
            push_node(cursor->type->static_method.class_type);
            push_node(cursor->type->static_method.return_type);
            RDIB_Type *params = cursor->type->static_method.params_type;
            for (U64 i = 0; i < params->params.count; ++i) {
              push_node(params->params.types[i]);
            }
          } else if (cursor->type->kind == RDI_TypeKind_Bitfield) {
            push_node(cursor->type->bitfield.value_type);
          } else if (cursor->type->kind == RDI_TypeKind_Array) {
            push_node(cursor->type->array.entry_type);
          } else if (cursor->type->kind == RDI_TypeKind_Struct || cursor->type->kind == RDI_TypeKind_Class) {
            if (cursor->type->udt.struct_type.derived != 0) {
              push_node(cursor->type->udt.struct_type.derived);
            }
            //push_node(cursor->type->udt.struct_type.vtshape);
          } else if (cursor->type->kind == RDI_TypeKind_Enum) {
            push_node(cursor->type->udt.enum_type.base_type);
          } else if (cursor->type->kind > RDI_TypeKindExt_Lo) {
            InvalidPath;
          } else {
            InvalidPath;
          }
        }

        for (struct TypeNode *cursor = stack; cursor != 0; cursor = cursor->next) {
          // was this type visisted?
          if (cursor->type != input->null_type && cursor->type->final_idx == 0) {
            cursor->type->final_idx = total_type_node_count;
            ++total_type_node_count;
          }
        }

        free_nodes = stack;
        stack      = 0;
      }
    }
#undef push_node
  }
  ProfEnd();

  ProfBegin("Type Stats");
  RDIB_TypeStats type_stats = {0};
  {
    type_stats.udt_counts = push_array(scratch.arena, U64, all_types.count);
    RDIB_TypeStatsTask task = { .chunks = all_type_chunks, .type_stats = &type_stats };
    tp_for_parallel(tp, 0, all_types.count, rdib_type_stats_task, &task);
  }
  ProfEnd();

  RDIB_PathTree *path_tree = rdib_build_path_tree(arena->v[0],
                                                  tp->worker_count,
                                                  input->null_src_file,
                                                  all_units.count,
                                                  all_unit_chunks,
                                                  all_src_files.count,
                                                  all_src_file_chunks);

  // loop over structs and build a map with every possible string
  ProfBegin("String Map");
  RDIB_StringMap *string_map;
  {
    U64 top_level_string_count   = 2;
    U64 sect_string_count        = 1;
    U64 src_file_string_count    = 1;
    U64 unit_string_count        = 6;
    U64 variable_string_count    = 2;
    U64 procedure_string_count   = 2;
    U64 scope_string_count       = 0;
    U64 inline_site_string_count = 0;
    U64 member_string_count      = 2;
    U64 type_string_count        = 3;
    U64 path_tree_node_count     = 1;

    U64 total_string_count = 1 /* :string_map_null */                           +
                             1                       * top_level_string_count   +
                             input->sect_count       * sect_string_count        +
                             total_src_file_count    * src_file_string_count    +
                             total_unit_count        * unit_string_count        +
                             total_local_count       * variable_string_count    + 
                             total_gvar_count        * variable_string_count    + 
                             total_tvar_count        * variable_string_count    +
                             total_proc_count        * procedure_string_count   +
                             total_inline_site_count * inline_site_string_count +
                             total_udt_member_count  * member_string_count      +
                             total_enum_member_count * member_string_count      +
                             total_type_count        * type_string_count        +
                             path_tree->node_count   * path_tree_node_count     +
                             total_scope_count       * scope_string_count;

    string_map = rdib_init_string_map(arena->v[0], total_string_count);

    RDIB_CollectStringsTask task = {0};
    task.string_map             = string_map;
    task.string_map_update_func = rdib_string_map_update_null;
    task.free_buckets           = push_array(scratch.arena, RDIB_StringMapBucket *, tp->worker_count);
    task.element_indices        = push_array(scratch.arena, U64,                    tp->worker_count);

    // :string_map_null
    rdib_string_map_insert_string_table_item(arena->v[0], &task, 0, str8_lit(""));

    // top level info
    rdib_string_map_insert_string_table_item(arena->v[0], &task, 0, input->top_level_info.exe_name);
    rdib_string_map_insert_string_table_item(arena->v[0], &task, 0, input->top_level_info.producer_string);

    ProfBegin("Sections");
    task.ranges = tp_divide_work(scratch.arena, input->sect_count, tp->worker_count);
    task.sects  = input->sections;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_sects_task, &task);
    ProfEnd();

    ProfBegin("Units");
    task.ranges = tp_divide_work(scratch.arena, all_units.count, tp->worker_count);
    task.units  = all_unit_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_units_task, &task);
    ProfEnd();

    ProfBegin("Source Files");
    task.ranges          = tp_divide_work(scratch.arena, all_src_files.count, tp->worker_count);
    task.src_file_chunks = all_src_file_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_source_files_task, &task);
    ProfEnd();

    ProfBegin("Locals");
    task.ranges = tp_divide_work(scratch.arena, all_locals.count, tp->worker_count);
    task.vars   = all_local_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_vars_task, &task);
    ProfEnd();

    ProfBegin("Global Variables");
    task.ranges = tp_divide_work(scratch.arena, all_gvars.count, tp->worker_count);
    task.vars   = all_gvar_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_vars_task, &task);
    ProfEnd();

    ProfBegin("Thread Variables");
    task.ranges = tp_divide_work(scratch.arena, all_tvars.count, tp->worker_count);
    task.vars   = all_tvar_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_vars_task, &task);
    ProfEnd();

    ProfBegin("Procedures");
    task.ranges = tp_divide_work(scratch.arena, all_procs.count, tp->worker_count);
    task.procs  = all_proc_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_procs_task, &task);
    ProfEnd();

    ProfBegin("Inline Sites");
    task.ranges       = tp_divide_work(scratch.arena, all_inline_sites.count, tp->worker_count);
    task.inline_sites = all_inline_site_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_inline_sites_task, &task);
    ProfEnd();

    ProfBegin("UDT Members");
    task.ranges      = tp_divide_work(scratch.arena, all_udt_members.count, tp->worker_count);
    task.udt_members = all_udt_member_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_udt_members_task, &task);
    ProfEnd();

    ProfBegin("Enum Members");
    task.ranges      = tp_divide_work(scratch.arena, all_enum_members.count, tp->worker_count);
    task.udt_members = all_enum_member_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_enum_members_task, &task);
    ProfEnd();

    ProfBegin("Types");
    task.ranges     = tp_divide_work(scratch.arena, all_types.count, tp->worker_count);
    task.types      = all_type_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_types_task, &task);
    ProfEnd();

    ProfBegin("Path Tree");
    task.ranges          = tp_divide_work(scratch.arena, path_tree->list_count, tp->worker_count);
    task.path_node_lists = path_tree->node_lists;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_collect_strings_path_nodes_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBegin("Name Maps");
  RDIB_StringMap *name_maps[RDI_NameMapKind_COUNT] = {0};
  {
    name_maps[RDI_NameMapKind_NULL              ] = rdib_init_string_map(scratch.arena, 1                   );
    name_maps[RDI_NameMapKind_GlobalVariables   ] = rdib_init_string_map(scratch.arena, total_gvar_count    );
    name_maps[RDI_NameMapKind_ThreadVariables   ] = rdib_init_string_map(scratch.arena, total_tvar_count    );
    name_maps[RDI_NameMapKind_Procedures        ] = rdib_init_string_map(scratch.arena, total_proc_count    );
    name_maps[RDI_NameMapKind_Types             ] = rdib_init_string_map(scratch.arena, total_type_count    );
    name_maps[RDI_NameMapKind_LinkNameProcedures] = rdib_init_string_map(scratch.arena, total_proc_count    );
    name_maps[RDI_NameMapKind_NormalSourcePaths ] = rdib_init_string_map(scratch.arena, total_src_file_count);

    RDIB_CollectStringsTask task = {0};
    task.string_map             = 0;
    task.string_map_update_func = rdib_string_map_update_concat_void_list_atomic;
    task.free_buckets           = push_array(scratch.arena, RDIB_StringMapBucket *, tp->worker_count);
    task.element_indices        = push_array(scratch.arena, U64,                    tp->worker_count);

    ProfBegin("Global Variables");
    task.string_map = name_maps[RDI_NameMapKind_GlobalVariables];
    task.ranges     = tp_divide_work(scratch.arena, all_gvars.count, tp->worker_count);
    task.vars       = all_gvar_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_name_map_var_task, &task);
    ProfEnd();

    ProfBegin("Thread Variables");
    task.string_map = name_maps[RDI_NameMapKind_ThreadVariables];
    task.ranges     = tp_divide_work(scratch.arena, all_tvars.count, tp->worker_count);
    task.vars       = all_tvar_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_name_map_var_task, &task);
    ProfEnd();

    ProfBegin("Procedure Names");
    task.string_map = name_maps[RDI_NameMapKind_Procedures];
    task.ranges     = tp_divide_work(scratch.arena, all_procs.count, tp->worker_count);
    task.procs      = all_proc_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_name_map_procedure_task, &task);
    ProfEnd();

    ProfBegin("Types");
    task.string_map = name_maps[RDI_NameMapKind_Types];
    task.ranges     = tp_divide_work(scratch.arena, all_types.count, tp->worker_count);
    task.types      = all_type_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_name_map_types_task, &task);
    ProfEnd();

    ProfBegin("Normal Source Paths");
    task.string_map      = name_maps[RDI_NameMapKind_NormalSourcePaths];
    task.ranges          = tp_divide_work(scratch.arena, all_src_files.count, tp->worker_count);
    task.src_file_chunks = all_src_file_chunks;
    tp_for_parallel(tp, arena, tp->worker_count, rdib_name_map_normal_paths_task, &task);
    ProfEnd();
  }
  ProfEnd();

  ProfBeginDynamic("Extract String Table Buckets [Cap: %llu]", string_map->cap);
  U64                    string_map_bucket_count;
  RDIB_StringMapBucket **string_map_buckets = rdib_extant_buckets_from_string_map(tp, scratch.arena, string_map, &string_map_bucket_count);
  rdib_string_map_sort_buckets(tp, string_map_buckets, string_map_bucket_count, tp->worker_count);
  rdib_string_map_assign_indices(string_map_buckets, string_map_bucket_count);
  ProfEnd();

  ProfBegin("Extract Name Maps Buckets");
  RDIB_StringMapBucket **name_map_buckets[RDI_NameMapKind_COUNT];
  U64                    name_map_bucket_counts[RDI_NameMapKind_COUNT];
  for (U64 i = 0; i < ArrayCount(name_map_buckets); ++i) {
    ProfBeginDynamic("Name Map: %.*s", str8_varg(rdi_string_from_name_map_kind(i)));
    name_map_buckets[i] = rdib_extant_buckets_from_string_map(tp, scratch.arena, name_maps[i], &name_map_bucket_counts[i]);
    rdib_string_map_sort_buckets(tp, name_map_buckets[i], name_map_bucket_counts[i], tp->worker_count);
    rdib_string_map_assign_indices(name_map_buckets[i], name_map_bucket_counts[i]);
    ProfEnd();
  }
  ProfEnd();

  ProfBegin("Index Run Map");
  RDIB_IndexRunMap     *idx_run_map;
  RDIB_IndexRunBucket **idx_run_buckets;
  U64                   idx_run_bucket_count;
  {
    // TODO: we over-allocate for name map index runs since not every bucket has > 1 value
    U64 total_name_map_value_count = 0;
    for (U64 i = 0; i < ArrayCount(name_map_bucket_counts); ++i) {
      total_name_map_value_count += name_map_bucket_counts[i];
    }

    // rough bucket estimate
    U64 idx_run_cap = (total_param_type_count + total_name_map_value_count) * 2;
    idx_run_map     = rdib_init_index_run_map(arena->v[0], idx_run_cap);

    // setup task context
    RDIB_BuildIndexRunsTask task = {0};
    task.idx_run_map    = idx_run_map;
    task.free_buckets   = push_array(scratch.arena, RDIB_IndexRunBucket *, tp->worker_count);

    ProfBegin("Type Params Pass");
    task.type_chunks = all_type_chunks;
    tp_for_parallel(tp, arena, all_types.count, rdib_build_idx_runs_params_task, &task);
    task.sorter_idx += 1;
    ProfEnd();

    ProfBegin("Name Maps Pass - Build Index Runs");
    for (U64 name_map_kind = 0; name_map_kind < ArrayCount(name_maps); ++name_map_kind) {
      ProfBeginDynamic("Name Map: %.*s", str8_varg(rdi_string_from_name_map_kind(name_map_kind)));
      task.name_map_kind     = name_map_kind;
      task.ranges            = tp_divide_work(scratch.arena, name_map_bucket_counts[name_map_kind], tp->worker_count);
      task.name_map_buckets  = name_map_buckets[name_map_kind];
      tp_for_parallel(tp, arena, tp->worker_count, rdib_build_idx_runs_name_map_buckets_task, &task);
      task.sorter_idx       += 1;
      ProfEnd();
    }
    ProfEnd();

    idx_run_buckets = rdib_extant_buckets_from_index_run_map(tp, arena->v[0], idx_run_map, &idx_run_bucket_count);
    rdib_index_run_map_sort_buckets(tp, idx_run_buckets, idx_run_bucket_count, task.sorter_idx);
    rdib_index_run_map_assign_indices(idx_run_buckets, idx_run_bucket_count);
  }
  ProfEnd();

  ProfBegin("Serialize Data Sections");
  RDIB_DataSectionList sections = {0};
  rdib_data_sections_from_top_level_info(arena->v[0], &sections, string_map, &input->top_level_info);
  rdib_data_sections_from_binary_sections(arena->v[0], &sections, string_map, input->sections, input->sect_count);
  rdib_data_sections_from_path_tree(tp, arena->v[0], &sections, string_map, path_tree);
  rdib_data_sections_from_string_map(tp, arena->v[0], &sections, string_map_buckets, string_map_bucket_count);
  rdib_data_sections_from_index_runs(tp, arena->v[0], &sections, idx_run_buckets, idx_run_bucket_count);
  rdib_data_sections_from_name_maps(tp, arena, &sections, string_map, idx_run_map, name_map_buckets, name_map_bucket_counts);
  rdib_data_sections_from_types(tp, arena->v[0], &sections, input->top_level_info.arch, string_map, idx_run_map, all_udt_member_types.count, all_udt_member_type_chunks, all_enum_member_types.count, all_enum_member_type_chunks, total_type_node_count, all_types.count, all_type_chunks, type_stats);
  rdib_data_sections_from_line_tables(tp, arena, &sections, total_line_table_count, all_line_tables.count, all_line_table_chunks);
  rdib_data_sections_from_source_line_maps(tp, arena, &sections, total_src_file_count, all_src_files.count, all_src_file_chunks);
  rdib_data_sections_from_source_files(tp, arena, &sections, string_map, path_tree, total_src_file_count, all_src_files.count, all_src_file_chunks);
  rdib_data_sections_from_units(arena->v[0], &sections, string_map, path_tree, total_unit_count, all_units.count, all_unit_chunks);
  rdib_data_sections_from_global_variables(tp, arena, &sections, string_map, total_gvar_count, all_gvars.count, all_gvar_chunks);
  rdib_data_sections_from_thread_variables(tp, arena, &sections, string_map, total_tvar_count, all_tvars.count, all_tvar_chunks);
  rdib_data_sections_from_procedures(tp, arena, &sections, string_map, total_proc_count, all_procs.count, all_proc_chunks);
  rdib_data_sections_from_scopes(tp, arena, &sections, string_map, total_scope_count, all_scopes.count, all_scope_chunks);
  rdib_data_sections_from_unit_gvar_scope_vmaps(tp, arena, &sections, all_units.count, all_unit_chunks, all_gvars.count, all_gvar_chunks, all_scopes.count, all_scope_chunks);
  rdib_data_sections_from_inline_sites(tp, arena->v[0], &sections, string_map, total_inline_site_count, all_inline_sites.count, all_inline_site_chunks);
  //rdib_data_sections_from_checksums(tp, arena->v[0], &sections);
  ProfEnd();

  ProfBegin("Make RDI header and sections");
  String8List rdi_data = {0};
  {
    // concat section datas
    String8List raw_section_datas[RDI_SectionKind_COUNT] = {0};
    for (RDIB_DataSectionNode *n = sections.first; n != 0; n = n->next) {
      str8_list_concat_in_place(&raw_section_datas[n->v.tag], &n->v.data);
    }

    RDI_Header  *rdi_header   = push_array(arena->v[0], RDI_Header,  1);
    RDI_Section *rdi_sections = push_array(arena->v[0], RDI_Section, RDI_SectionKind_COUNT);

    rdi_header->magic              = RDI_MAGIC_CONSTANT;
    rdi_header->encoding_version   = RDI_ENCODING_VERSION;
    rdi_header->data_section_off   = sizeof(*rdi_header);
    rdi_header->data_section_count = RDI_SectionKind_COUNT;

    str8_list_push(arena->v[0], &rdi_data, str8_struct(rdi_header));
    str8_list_push(arena->v[0], &rdi_data, str8_array(rdi_sections, RDI_SectionKind_COUNT));

    for (U64 sect_idx = 0; sect_idx < RDI_SectionKind_COUNT; ++sect_idx) {
      RDI_Section *dst   = &rdi_sections[sect_idx];
      dst->encoding      = RDI_SectionEncoding_Unpacked;
      dst->pad           = 0;
      dst->off           = 0;
      dst->encoded_size  = 0;
      dst->unpacked_size = 0;

      if (raw_section_datas[sect_idx].total_size > 0) {
        str8_list_push_aligner(arena->v[0], &rdi_data, 0, 8);

        dst->off           = rdi_data.total_size;
        dst->encoded_size  = raw_section_datas[sect_idx].total_size;
        dst->unpacked_size = raw_section_datas[sect_idx].total_size;

        str8_list_concat_in_place(&rdi_data, &raw_section_datas[sect_idx]);

#if BUILD_DEBUG
        {
          U64 expected_total_size = 0;
          for (String8Node *n = rdi_data.first; n != 0; n = n->next) {
            expected_total_size += n->string.size;
          }
          Assert(expected_total_size == rdi_data.total_size);
        }
#endif
      }
    }
  }
  ProfEnd();

  scratch_end(scratch);
  ProfEnd();
  return rdi_data;
}

