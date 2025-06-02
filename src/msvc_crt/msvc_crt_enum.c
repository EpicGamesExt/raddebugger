// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal String8
mscrt_string_from_eh_adjectives(Arena *arena, MSCRT_EhHandlerTypeFlags adjectives)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List adj_list = {0};
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsConst) {
    str8_list_pushf(scratch.arena, &adj_list, "Const");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsVolatile) {
    str8_list_pushf(scratch.arena, &adj_list, "Volatile");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsUnaligned) {
    str8_list_pushf(scratch.arena, &adj_list, "Unaligned");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsReference) {
    str8_list_pushf(scratch.arena, &adj_list, "Reference");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsResumable) {
    str8_list_pushf(scratch.arena, &adj_list, "Resumable");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsStdDotDot) {
    str8_list_pushf(scratch.arena, &adj_list, "StdDotDot");
  }
  if (adjectives & MSCRT_EhHandlerTypeFlag_IsComplusEH) {
    str8_list_pushf(scratch.arena, &adj_list, "ComplusEH");
  }
  String8 result = str8_list_join(arena, &adj_list, &(StringJoin){.sep=str8_lit(", ")});
  scratch_end(scratch);
  return result;
}

