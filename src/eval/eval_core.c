// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Functions

internal U64
eval_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

////////////////////////////////
//~ rjf: Error List Building Functions

internal void
eval_error(Arena *arena, EVAL_ErrorList *list, EVAL_ErrorKind kind, void *location, String8 text){
  EVAL_Error *error = push_array_no_zero(arena, EVAL_Error, 1);
  SLLQueuePush(list->first, list->last, error);
  list->count += 1;
  list->max_kind = Max(kind, list->max_kind);
  error->kind = kind;
  error->location = location;
  error->text = text;
}

internal void
eval_errorf(Arena *arena, EVAL_ErrorList *list, EVAL_ErrorKind kind, void *location, char *fmt, ...){
  va_list args;
  va_start(args, fmt);
  String8 text = push_str8fv(arena, fmt, args);
  va_end(args);
  eval_error(arena, list, kind, location, text);
}

internal void
eval_error_list_concat_in_place(EVAL_ErrorList *dst, EVAL_ErrorList *to_push){
  if (dst->last != 0){
    if (to_push->last != 0){
      dst->last->next = to_push->first;
      dst->last = to_push->last;
      dst->count += to_push->count;
    }
  }
  else{
    *dst = *to_push;
  }
  MemoryZeroStruct(to_push);
}

////////////////////////////////
//~ rjf: Map Functions

//- rjf: string -> num

internal EVAL_String2NumMap
eval_string2num_map_make(Arena *arena, U64 slot_count)
{
  EVAL_String2NumMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, EVAL_String2NumMapSlot, map.slots_count);
  return map;
}

internal void
eval_string2num_map_insert(Arena *arena, EVAL_String2NumMap *map, String8 string, U64 num)
{
  U64 hash = eval_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  EVAL_String2NumMapNode *existing_node = 0;
  for(EVAL_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
  {
    if(str8_match(node->string, string, 0) && node->num == num)
    {
      existing_node = node;
      break;
    }
  }
  if(existing_node == 0)
  {
    EVAL_String2NumMapNode *node = push_array(arena, EVAL_String2NumMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    SLLQueuePush_N(map->first, map->last, node, order_next);
    node->string = push_str8_copy(arena, string);
    node->num = num;
  }
}

internal U64
eval_num_from_string(EVAL_String2NumMap *map, String8 string)
{
  U64 num = 0;
  if(map->slots_count != 0)
  {
    U64 hash = eval_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    EVAL_String2NumMapNode *existing_node = 0;
    for(EVAL_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
    {
      if(str8_match(node->string, string, 0))
      {
        existing_node = node;
        break;
      }
    }
    if(existing_node != 0)
    {
      num = existing_node->num;
    }
  }
  return num;
}

//- rjf: string -> expr

internal EVAL_String2ExprMap
eval_string2expr_map_make(Arena *arena, U64 slot_count)
{
  EVAL_String2ExprMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, EVAL_String2ExprMapSlot, map.slots_count);
  return map;
}

internal void
eval_string2expr_map_insert(Arena *arena, EVAL_String2ExprMap *map, String8 string, EVAL_Expr *expr)
{
  U64 hash = eval_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  EVAL_String2ExprMapNode *existing_node = 0;
  for(EVAL_String2ExprMapNode *node = map->slots[slot_idx].first;
      node != 0;
      node = node->hash_next)
  {
    if(str8_match(node->string, string, 0))
    {
      existing_node = node;
      break;
    }
  }
  if(existing_node == 0)
  {
    EVAL_String2ExprMapNode *node = push_array(arena, EVAL_String2ExprMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    node->string = push_str8_copy(arena, string);
    existing_node = node;
  }
  existing_node->expr = expr;
}

internal void
eval_string2expr_map_inc_poison(EVAL_String2ExprMap *map, String8 string)
{
  U64 hash = eval_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  for(EVAL_String2ExprMapNode *node = map->slots[slot_idx].first;
      node != 0;
      node = node->hash_next)
  {
    if(str8_match(node->string, string, 0))
    {
      node->poison_count += 1;
      break;
    }
  }
}

internal void
eval_string2expr_map_dec_poison(EVAL_String2ExprMap *map, String8 string)
{
  U64 hash = eval_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  for(EVAL_String2ExprMapNode *node = map->slots[slot_idx].first;
      node != 0;
      node = node->hash_next)
  {
    if(str8_match(node->string, string, 0) && node->poison_count > 0)
    {
      node->poison_count -= 1;
      break;
    }
  }
}

internal EVAL_Expr *
eval_expr_from_string(EVAL_String2ExprMap *map, String8 string)
{
  EVAL_Expr *expr = &eval_expr_nil;
  if(map->slots_count != 0)
  {
    U64 hash = eval_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    EVAL_String2ExprMapNode *existing_node = 0;
    for(EVAL_String2ExprMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
    {
      if(str8_match(node->string, string, 0) && node->poison_count == 0)
      {
        existing_node = node;
        break;
      }
    }
    if(existing_node != 0)
    {
      expr = existing_node->expr;
    }
  }
  return expr;
}
