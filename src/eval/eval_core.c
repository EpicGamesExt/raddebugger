// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "eval/generated/eval.meta.c"

////////////////////////////////
//~ rjf: Basic Helper Functions

#if !defined(XXH_IMPLEMENTATION)
# define XXH_IMPLEMENTATION
# define XXH_STATIC_LINKING_ONLY
# include "third_party/xxHash/xxhash.h"
#endif

internal U64
e_hash_from_string(U64 seed, String8 string)
{
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

////////////////////////////////
//~ rjf: Message Functions

internal void
e_msg(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, String8 text)
{
  E_Msg *msg = push_array(arena, E_Msg, 1);
  SLLQueuePush(msgs->first, msgs->last, msg);
  msgs->count += 1;
  msgs->max_kind = Max(kind, msgs->max_kind);
  msg->kind = kind;
  msg->location = location;
  msg->text = text;
}

internal void
e_msgf(Arena *arena, E_MsgList *msgs, E_MsgKind kind, void *location, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  String8 text = push_str8fv(arena, fmt, args);
  va_end(args);
  e_msg(arena, msgs, kind, location, text);
}

internal void
e_msg_list_concat_in_place(E_MsgList *dst, E_MsgList *to_push)
{
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->count += to_push->count;
    dst->max_kind = Max(dst->max_kind, to_push->max_kind);
  }
  else if(to_push->first != 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  MemoryZeroStruct(to_push);
}

////////////////////////////////
//~ rjf: Space Functions

internal E_Space
e_space_make(E_SpaceKind kind)
{
  E_Space space = {0};
  space.kind = kind;
  return space;
}

////////////////////////////////
//~ rjf: Basic Map Functions

//- rjf: string -> num

internal E_String2NumMap
e_string2num_map_make(Arena *arena, U64 slot_count)
{
  E_String2NumMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, E_String2NumMapSlot, map.slots_count);
  return map;
}

internal void
e_string2num_map_insert(Arena *arena, E_String2NumMap *map, String8 string, U64 num)
{
  U64 hash = e_hash_from_string(5381, string);
  U64 slot_idx = hash%map->slots_count;
  E_String2NumMapNode *existing_node = 0;
  for(E_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
  {
    if(str8_match(node->string, string, 0) && node->num == num)
    {
      existing_node = node;
      break;
    }
  }
  if(existing_node == 0)
  {
    E_String2NumMapNode *node = push_array(arena, E_String2NumMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    SLLQueuePush_N(map->first, map->last, node, order_next);
    node->string = push_str8_copy(arena, string);
    node->num = num;
    map->node_count += 1;
  }
}

internal U64
e_num_from_string(E_String2NumMap *map, String8 string)
{
  U64 num = 0;
  if(map->slots_count != 0)
  {
    U64 hash = e_hash_from_string(5381, string);
    U64 slot_idx = hash%map->slots_count;
    E_String2NumMapNode *existing_node = 0;
    for(E_String2NumMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
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

internal E_String2NumMapNodeArray
e_string2num_map_node_array_from_map(Arena *arena, E_String2NumMap *map)
{
  E_String2NumMapNodeArray result = {0};
  result.count = map->node_count;
  result.v = push_array(arena, E_String2NumMapNode *, result.count);
  U64 idx = 0;
  for(E_String2NumMapNode *n = map->first; n != 0; n = n->order_next, idx += 1)
  {
    result.v[idx] = n;
  }
  return result;
}

internal int
e_string2num_map_node_qsort_compare__num_ascending(E_String2NumMapNode **a, E_String2NumMapNode **b)
{
  int result = 0;
  if(a[0]->num < b[0]->num)
  {
    result = -1;
  }
  else if(a[0]->num > b[0]->num)
  {
    result = +1;
  }
  return result;
}

internal void
e_string2num_map_node_array_sort__in_place(E_String2NumMapNodeArray *array)
{
  quick_sort(array->v, array->count, sizeof(array->v[0]), e_string2num_map_node_qsort_compare__num_ascending);
}

//- rjf: string -> expr

internal E_String2ExprMap
e_string2expr_map_make(Arena *arena, U64 slot_count)
{
  E_String2ExprMap map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, E_String2ExprMapSlot, map.slots_count);
  return map;
}

internal void
e_string2expr_map_insert(Arena *arena, E_String2ExprMap *map, String8 string, E_Expr *expr)
{
  U64 hash = e_hash_from_string(5381, string);
  U64 slot_idx = hash%map->slots_count;
  E_String2ExprMapNode *existing_node = 0;
  for(E_String2ExprMapNode *node = map->slots[slot_idx].first;
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
    E_String2ExprMapNode *node = push_array(arena, E_String2ExprMapNode, 1);
    SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    node->string = push_str8_copy(arena, string);
    existing_node = node;
    existing_node->expr = expr;
  }
}

internal void
e_string2expr_map_inc_poison(E_String2ExprMap *map, String8 string)
{
  U64 hash = e_hash_from_string(5381, string);
  U64 slot_idx = hash%map->slots_count;
  for(E_String2ExprMapNode *node = map->slots[slot_idx].first;
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
e_string2expr_map_dec_poison(E_String2ExprMap *map, String8 string)
{
  U64 hash = e_hash_from_string(5381, string);
  U64 slot_idx = hash%map->slots_count;
  for(E_String2ExprMapNode *node = map->slots[slot_idx].first;
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

internal E_Expr *
e_string2expr_lookup(E_String2ExprMap *map, String8 string)
{
  E_Expr *expr = &e_expr_nil;
  if(map->slots_count != 0)
  {
    U64 hash = e_hash_from_string(5381, string);
    U64 slot_idx = hash%map->slots_count;
    E_String2ExprMapNode *existing_node = 0;
    for(E_String2ExprMapNode *node = map->slots[slot_idx].first; node != 0; node = node->hash_next)
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

////////////////////////////////
//~ rjf: Debug-Info-Driven Map Building Functions

internal E_String2NumMap *
e_push_locals_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: gather scopes to walk
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    RDI_Scope *scope;
  };
  Task *first_task = 0;
  Task *last_task = 0;
  
  //- rjf: voff -> tightest scope
  RDI_Scope *tightest_scope = 0;
  {
    U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
    RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
    Task *task = push_array(scratch.arena, Task, 1);
    task->scope = scope;
    SLLQueuePush(first_task, last_task, task);
    tightest_scope = scope;
  }
  
  //- rjf: voff-1 -> scope
  if(voff > 0)
  {
    U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff-1);
    RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
    if(scope != tightest_scope)
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->scope = scope;
      SLLQueuePush(first_task, last_task, task);
    }
  }
  
  //- rjf: tightest scope -> walk up the tree & build tasks for each parent scope
  if(tightest_scope != 0)
  {
    RDI_Scope *nil_scope = rdi_element_from_name_idx(rdi, Scopes, 0);
    for(RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, tightest_scope->parent_scope_idx);
        scope != 0 && scope != nil_scope;
        scope = rdi_element_from_name_idx(rdi, Scopes, scope->parent_scope_idx))
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->scope = scope;
      SLLQueuePush(first_task, last_task, task);
    }
  }
  
  //- rjf: build blank map
  E_String2NumMap *map = push_array(arena, E_String2NumMap, 1);
  *map = e_string2num_map_make(arena, 1024);
  
  //- rjf: accumulate locals for all tasks
  for(Task *task = first_task; task != 0; task = task->next)
  {
    RDI_Scope *scope = task->scope;
    if(scope != 0)
    {
      U32 local_opl_idx = scope->local_first + scope->local_count;
      for(U32 local_idx = scope->local_first; local_idx < local_opl_idx; local_idx += 1)
      {
        RDI_Local *local_var = rdi_element_from_name_idx(rdi, Locals, local_idx);
        U64 local_name_size = 0;
        U8 *local_name_str = rdi_string_from_idx(rdi, local_var->name_string_idx, &local_name_size);
        String8 name = push_str8_copy(arena, str8(local_name_str, local_name_size));
        e_string2num_map_insert(arena, map, name, (U64)local_idx+1);
      }
    }
  }
  
  scratch_end(scratch);
  return map;
}

internal E_String2NumMap *
e_push_member_map_from_rdi_voff(Arena *arena, RDI_Parsed *rdi, U64 voff)
{
  //- rjf: voff -> tightest scope
  U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
  RDI_Scope *tightest_scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
  
  //- rjf: tightest scope -> procedure
  U32 proc_idx = tightest_scope->proc_idx;
  RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, proc_idx);
  
  //- rjf: procedure -> udt
  U32 udt_idx = procedure->container_idx;
  RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, udt_idx);
  
  //- rjf: build blank map
  E_String2NumMap *map = push_array(arena, E_String2NumMap, 1);
  *map = e_string2num_map_make(arena, 64);
  
  //- rjf: udt -> fill member map
  if(!(udt->flags & RDI_UDTFlag_EnumMembers))
  {
    U64 data_member_num = 1;
    for(U32 member_idx = udt->member_first;
        member_idx < udt->member_first+udt->member_count;
        member_idx += 1)
    {
      RDI_Member *m = rdi_element_from_name_idx(rdi, Members, member_idx);
      if(m->kind == RDI_MemberKind_DataField)
      {
        String8 name = {0};
        name.str = rdi_string_from_idx(rdi, m->name_string_idx, &name.size);
        e_string2num_map_insert(arena, map, name, data_member_num);
        data_member_num += 1;
      }
    }
  }
  
  return map;
}
