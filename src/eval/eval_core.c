// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "eval/generated/eval.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

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
//~ rjf: Expr Kind Enum Functions

internal RDI_EvalOp
e_opcode_from_expr_kind(E_ExprKind kind)
{
  RDI_EvalOp result = RDI_EvalOp_Stop;
  switch(kind)
  {
    case E_ExprKind_Neg:    result = RDI_EvalOp_Neg;    break;
    case E_ExprKind_LogNot: result = RDI_EvalOp_LogNot; break;
    case E_ExprKind_BitNot: result = RDI_EvalOp_BitNot; break;
    case E_ExprKind_Mul:    result = RDI_EvalOp_Mul;    break;
    case E_ExprKind_Div:    result = RDI_EvalOp_Div;    break;
    case E_ExprKind_Mod:    result = RDI_EvalOp_Mod;    break;
    case E_ExprKind_Add:    result = RDI_EvalOp_Add;    break;
    case E_ExprKind_Sub:    result = RDI_EvalOp_Sub;    break;
    case E_ExprKind_LShift: result = RDI_EvalOp_LShift; break;
    case E_ExprKind_RShift: result = RDI_EvalOp_RShift; break;
    case E_ExprKind_Less:   result = RDI_EvalOp_Less;   break;
    case E_ExprKind_LsEq:   result = RDI_EvalOp_LsEq;   break;
    case E_ExprKind_Grtr:   result = RDI_EvalOp_Grtr;   break;
    case E_ExprKind_GrEq:   result = RDI_EvalOp_GrEq;   break;
    case E_ExprKind_EqEq:   result = RDI_EvalOp_EqEq;   break;
    case E_ExprKind_NtEq:   result = RDI_EvalOp_NtEq;   break;
    case E_ExprKind_BitAnd: result = RDI_EvalOp_BitAnd; break;
    case E_ExprKind_BitXor: result = RDI_EvalOp_BitXor; break;
    case E_ExprKind_BitOr:  result = RDI_EvalOp_BitOr;  break;
    case E_ExprKind_LogAnd: result = RDI_EvalOp_LogAnd; break;
    case E_ExprKind_LogOr:  result = RDI_EvalOp_LogOr;  break;
  }
  return result;
}

internal B32
e_expr_kind_is_comparison(E_ExprKind kind)
{
  B32 result = 0;
  switch(kind)
  {
    default:{}break;
    case E_ExprKind_EqEq:
    case E_ExprKind_NtEq:
    case E_ExprKind_Less:
    case E_ExprKind_Grtr:
    case E_ExprKind_LsEq:
    case E_ExprKind_GrEq:
    {
      result = 1;
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Key Type Functions

internal B32
e_key_match(E_Key a, E_Key b)
{
  B32 result = (a.u64 == b.u64);
  return result;
}

internal E_Key
e_key_zero(void)
{
  E_Key key = {0};
  return key;
}

////////////////////////////////
//~ rjf: Type Key Type Functions

internal void
e_type_key_list_push(Arena *arena, E_TypeKeyList *list, E_TypeKey key)
{
  E_TypeKeyNode *n = push_array(arena, E_TypeKeyNode, 1);
  n->v = key;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal void
e_type_key_list_push_front(Arena *arena, E_TypeKeyList *list, E_TypeKey key)
{
  E_TypeKeyNode *n = push_array(arena, E_TypeKeyNode, 1);
  n->v = key;
  SLLQueuePushFront(list->first, list->last, n);
  list->count += 1;
}

internal E_TypeKeyList
e_type_key_list_copy(Arena *arena, E_TypeKeyList *src)
{
  E_TypeKeyList dst = {0};
  for(E_TypeKeyNode *n = src->first; n != 0; n = n->next)
  {
    e_type_key_list_push(arena, &dst, n->v);
  }
  return dst;
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

internal E_MsgList
e_msg_list_copy(Arena *arena, E_MsgList *src)
{
  E_MsgList dst = {0};
  for(E_Msg *msg = src->first; msg != 0; msg = msg->next)
  {
    e_msg(arena, &dst, msg->kind, msg->location, msg->text);
  }
  return dst;
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
//~ rjf: Map Functions

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
e_string2expr_map_lookup(E_String2ExprMap *map, String8 string)
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

//- rjf: string -> type-key

internal E_String2TypeKeyMap
e_string2typekey_map_make(Arena *arena, U64 slots_count)
{
  E_String2TypeKeyMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_String2TypeKeySlot, map.slots_count);
  return map;
}

internal void
e_string2typekey_map_insert(Arena *arena, E_String2TypeKeyMap *map, String8 string, E_TypeKey key)
{
  E_String2TypeKeyNode *n = push_array(arena, E_String2TypeKeyNode, 1);
  U64 hash = e_hash_from_string(5381, string);
  U64 slot_idx = hash%map->slots_count;
  SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, n);
  n->string = push_str8_copy(arena, string);
  n->key = key;
}

internal E_TypeKey
e_string2typekey_map_lookup(E_String2TypeKeyMap *map, String8 string)
{
  E_TypeKey key = zero_struct;
  U64 hash = e_hash_from_string(5381, string);
  U64 slot_idx = hash%map->slots_count;
  for(E_String2TypeKeyNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
  {
    if(str8_match(n->string, string, 0))
    {
      key = n->key;
      break;
    }
  }
  return key;
}

//- rjf: auto hooks

internal E_AutoHookMap
e_auto_hook_map_make(Arena *arena, U64 slots_count)
{
  E_AutoHookMap map = {0};
  map.slots_count = slots_count;
  map.slots = push_array(arena, E_AutoHookSlot, map.slots_count);
  return map;
}

internal void
e_auto_hook_map_insert_new_(Arena *arena, E_AutoHookMap *map, E_AutoHookParams *params)
{
  // rjf: get type key
  E_TypeKey type_key = params->type_key;
  if(params->type_pattern.size != 0)
  {
    E_Parse parse = e_push_parse_from_string(arena, params->type_pattern);
    type_key = e_type_key_from_expr(parse.expr);
  }
  
  // rjf: get type pattern parts
  String8List pattern_parts = {0};
  if(e_type_key_match(e_type_key_zero(), type_key))
  {
    U8 pattern_split = '?';
    pattern_parts = str8_split(arena, params->type_pattern, &pattern_split, 1, StringSplitFlag_KeepEmpties);
  }
  
  // rjf: if the type key is nonzero, *or* we have type patterns, then insert
  // into map accordingle
  if(!e_type_key_match(e_type_key_zero(), type_key) ||
     pattern_parts.node_count != 0)
  {
    E_AutoHookNode *node = push_array(arena, E_AutoHookNode, 1);
    node->type_string = str8_skip_chop_whitespace(e_type_string_from_key(arena, type_key));
    node->type_pattern_parts = pattern_parts;
    node->expr = e_parse_from_string(params->tag_expr_string).expr;
    if(!e_type_key_match(e_type_key_zero(), type_key))
    {
      U64 hash = e_hash_from_string(5381, node->type_string);
      U64 slot_idx = hash%map->slots_count;
      SLLQueuePush_N(map->slots[slot_idx].first, map->slots[slot_idx].last, node, hash_next);
    }
    else
    {
      SLLQueuePush_N(map->first_pattern, map->last_pattern, node, pattern_order_next);
    }
  }
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

////////////////////////////////
//~ rjf: Cache Creation & Selection

internal E_Cache *
e_cache_alloc(void)
{
  Arena *arena = arena_alloc();
  E_Cache *cache = push_array(arena, E_Cache, 1);
  cache->arena = arena;
  cache->arena_eval_start_pos = arena_pos(arena);
  return cache;
}

internal void
e_cache_release(E_Cache *cache)
{
  arena_release(cache->arena);
}

internal void
e_select_cache(E_Cache *cache)
{
  e_cache = cache;
}

////////////////////////////////
//~ rjf: Evaluation Phase Markers

internal void
e_select_base_ctx(E_BaseCtx *ctx)
{
  //- rjf: select base context
  if(ctx->modules == 0)        { ctx->modules = &e_module_nil; }
  if(ctx->primary_module == 0) { ctx->primary_module = &e_module_nil; }
  e_base_ctx = ctx;
  
  //- rjf: reset the evaluation cache
  arena_pop_to(e_cache->arena, e_cache->arena_eval_start_pos);
  e_cache->key_id_gen = 0;
  e_cache->key_slots_count = 4096;
  e_cache->key_slots = push_array(e_cache->arena, E_CacheSlot, e_cache->key_slots_count);
  e_cache->string_slots_count = 4096;
  e_cache->string_slots = push_array(e_cache->arena, E_CacheSlot, e_cache->string_slots_count);
  e_cache->free_parent_node = 0;
  e_cache->top_parent_node = 0;
  e_cache->cons_id_gen = 0;
  e_cache->cons_content_slots_count = 256;
  e_cache->cons_key_slots_count = 256;
  e_cache->cons_content_slots = push_array(e_cache->arena, E_ConsTypeSlot, e_cache->cons_content_slots_count);
  e_cache->cons_key_slots = push_array(e_cache->arena, E_ConsTypeSlot, e_cache->cons_key_slots_count);
  e_cache->member_cache_slots_count = 256;
  e_cache->member_cache_slots = push_array(e_cache->arena, E_MemberCacheSlot, e_cache->member_cache_slots_count);
  e_cache->type_cache_slots_count = 1024;
  e_cache->type_cache_slots = push_array(e_cache->arena, E_TypeCacheSlot, e_cache->type_cache_slots_count);
  e_cache->file_type_key = e_type_key_cons(.kind = E_TypeKind_Set,
                                           .name = str8_lit("file"),
                                           .irext  = E_TYPE_IREXT_FUNCTION_NAME(file),
                                           .access = E_TYPE_ACCESS_FUNCTION_NAME(file),
                                           .expand =
                                           {
                                             .info = E_TYPE_EXPAND_INFO_FUNCTION_NAME(file),
                                             .range= E_TYPE_EXPAND_RANGE_FUNCTION_NAME(file),
                                           });
  e_cache->folder_type_key = e_type_key_cons(.kind = E_TypeKind_Set,
                                             .name = str8_lit("folder"),
                                             .expand =
                                             {
                                               .info        = E_TYPE_EXPAND_INFO_FUNCTION_NAME(folder),
                                               .range       = E_TYPE_EXPAND_RANGE_FUNCTION_NAME(folder),
                                               .id_from_num = E_TYPE_EXPAND_ID_FROM_NUM_FUNCTION_NAME(folder),
                                               .num_from_id = E_TYPE_EXPAND_NUM_FROM_ID_FUNCTION_NAME(folder),
                                             });
  e_cache->thread_ip_procedure = rdi_procedure_from_voff(e_base_ctx->primary_module->rdi, e_base_ctx->thread_ip_voff);
  e_cache->used_expr_map = push_array(e_cache->arena, E_UsedExprMap, 1);
  e_cache->used_expr_map->slots_count = 64;
  e_cache->used_expr_map->slots = push_array(e_cache->arena, E_UsedExprSlot, e_cache->used_expr_map->slots_count);
  e_cache->type_auto_hook_cache_map = push_array(e_cache->arena, E_TypeAutoHookCacheMap, 1);
  e_cache->type_auto_hook_cache_map->slots_count = 256;
  e_cache->type_auto_hook_cache_map->slots = push_array(e_cache->arena, E_TypeAutoHookCacheSlot, e_cache->type_auto_hook_cache_map->slots_count);
  e_cache->string_id_gen = 0;
  e_cache->string_id_map = push_array(e_cache->arena, E_StringIDMap, 1);
  e_cache->string_id_map->id_slots_count = 1024;
  e_cache->string_id_map->id_slots = push_array(e_cache->arena, E_StringIDSlot, e_cache->string_id_map->id_slots_count);
  e_cache->string_id_map->hash_slots_count = 1024;
  e_cache->string_id_map->hash_slots = push_array(e_cache->arena, E_StringIDSlot, e_cache->string_id_map->hash_slots_count);
}

internal void
e_select_ir_ctx(E_IRCtx *ctx)
{
  if(ctx->regs_map == 0)       { ctx->regs_map = &e_string2num_map_nil; }
  if(ctx->reg_alias_map == 0)  { ctx->reg_alias_map = &e_string2num_map_nil; }
  if(ctx->locals_map == 0)     { ctx->locals_map = &e_string2num_map_nil; }
  if(ctx->member_map == 0)     { ctx->member_map = &e_string2num_map_nil; }
  if(ctx->macro_map == 0)      { ctx->macro_map = push_array(e_cache->arena, E_String2ExprMap, 1); ctx->macro_map[0] = e_string2expr_map_make(e_cache->arena, 512); }
  e_ir_ctx = ctx;
}

////////////////////////////////
//~ rjf: Cache Accessing Functions

//- rjf: parent key stack

internal E_Key
e_parent_key_push(E_Key key)
{
  E_Key top = {0};
  if(e_cache->top_parent_node != 0)
  {
    top = e_cache->top_parent_node->key;
  }
  E_CacheParentNode *n = e_cache->free_parent_node;
  if(n != 0)
  {
    SLLStackPop(e_cache->free_parent_node);
  }
  else
  {
    n = push_array(e_cache->arena, E_CacheParentNode, 1);
  }
  SLLStackPush(e_cache->top_parent_node, n);
  n->key = key;
  return top;
}

internal E_Key
e_parent_key_pop(void)
{
  E_CacheParentNode *n = e_cache->top_parent_node;
  SLLStackPop(e_cache->top_parent_node);
  SLLStackPush(e_cache->free_parent_node, n);
  E_Key popped = n->key;
  return popped;
}

//- rjf: key construction

internal E_Key
e_key_from_string(String8 string)
{
  E_Key parent_key = {0};
  if(e_cache->top_parent_node)
  {
    parent_key = e_cache->top_parent_node->key;
  }
  U64 hash = e_hash_from_string(parent_key.u64, string);
  U64 slot_idx = hash%e_cache->string_slots_count;
  E_CacheSlot *slot = &e_cache->string_slots[slot_idx];
  E_CacheNode *node = 0;
  for(E_CacheNode *n = slot->first; n != 0; n = n->string_next)
  {
    if(e_key_match(parent_key, n->bundle.parent_key) &&
       str8_match(n->bundle.string, string, 0) &&
       (n->bundle.interpretation.space.kind == E_SpaceKind_Null ||
        e_space_gen(n->bundle.interpretation.space) == n->bundle.space_gen))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    e_cache->key_id_gen += 1;
    E_Key key = {e_cache->key_id_gen};
    U64 key_hash = e_hash_from_string(5381, str8_struct(&key));
    U64 key_slot_idx = key_hash%e_cache->key_slots_count;
    E_CacheSlot *key_slot = &e_cache->key_slots[key_slot_idx];
    node = push_array(e_cache->arena, E_CacheNode, 1);
    SLLQueuePush_N(slot->first, slot->last, node, string_next);
    SLLQueuePush_N(key_slot->first, key_slot->last, node, key_next);
    node->bundle.key = key;
    node->bundle.parent_key = parent_key;
    node->bundle.string = push_str8_copy(e_cache->arena, string);
  }
  return node->bundle.key;
}

internal E_Key
e_key_from_stringf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Key result = e_key_from_string(string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal E_Key
e_key_from_expr(E_Expr *expr)
{
  Temp scratch = scratch_begin(0, 0);
  String8 string = e_string_from_expr(scratch.arena, expr, str8_zero());
  E_Key key = e_key_from_string(string);
  scratch_end(scratch);
  return key;
}

//- rjf: base key -> node helper

internal E_CacheBundle *
e_cache_bundle_from_key(E_Key key)
{
  U64 hash = e_hash_from_string(5381, str8_struct(&key));
  U64 slot_idx = hash%e_cache->key_slots_count;
  E_CacheSlot *slot = &e_cache->key_slots[slot_idx];
  E_CacheNode *node = 0;
  for(E_CacheNode *n = slot->first; n != 0; n = n->key_next)
  {
    if(e_key_match(n->bundle.key, key))
    {
      node = n;
      break;
    }
  }
  E_CacheBundle *bundle = &e_cache_bundle_nil;
  if(node != 0)
  {
    bundle = &node->bundle;
  }
  return bundle;
}

//- rjf: bundle -> pipeline stage outputs

internal E_Parse
e_parse_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_Parse))
  {
    bundle->flags |= E_CacheBundleFlag_Parse;
    bundle->parse = e_push_parse_from_string(e_cache->arena, bundle->string);
    E_MsgList msgs_copy = e_msg_list_copy(e_cache->arena, &bundle->parse.msgs);
    e_msg_list_concat_in_place(&bundle->msgs, &msgs_copy);
  }
  E_Parse parse = bundle->parse;
  return parse;
}

internal E_IRTreeAndType
e_irtree_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_IRTree))
  {
    bundle->flags |= E_CacheBundleFlag_IRTree;
    E_IRTreeAndType parent = e_irtree_from_key(bundle->parent_key);
    E_Parse parse = e_parse_from_bundle(bundle);
    bundle->irtree = e_push_irtree_and_type_from_expr(e_cache->arena, &parent, 0, 0, 0, parse.expr);
    E_MsgList msgs_copy = e_msg_list_copy(e_cache->arena, &bundle->irtree.msgs);
    e_msg_list_concat_in_place(&bundle->msgs, &msgs_copy);
  }
  E_IRTreeAndType result = bundle->irtree;
  return result;
}

internal String8
e_bytecode_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_Bytecode))
  {
    bundle->flags |= E_CacheBundleFlag_Bytecode;
    Temp scratch = scratch_begin(0, 0);
    E_IRTreeAndType irtree = e_irtree_from_bundle(bundle);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, irtree.root);
    bundle->bytecode = e_bytecode_from_oplist(e_cache->arena, &oplist);
    scratch_end(scratch);
  }
  String8 result = bundle->bytecode;
  return result;
}

internal E_Interpretation
e_interpretation_from_bundle(E_CacheBundle *bundle)
{
  if(bundle != &e_cache_bundle_nil && !(bundle->flags & E_CacheBundleFlag_Interpret))
  {
    bundle->flags |= E_CacheBundleFlag_Interpret;
    String8 bytecode = e_bytecode_from_bundle(bundle);
    E_Interpretation interpret = e_interpret(bytecode);
    if(E_InterpretationCode_Good < interpret.code && interpret.code < E_InterpretationCode_COUNT)
    {
      e_msg(e_cache->arena, &bundle->msgs, E_MsgKind_InterpretationError, 0, e_interpretation_code_display_strings[interpret.code]);
    }
    bundle->interpretation = interpret;
    bundle->space_gen = e_space_gen(interpret.space);
  }
  E_Interpretation interpret = bundle->interpretation;
  return interpret;
}

//- rjf: key -> full expression string

internal String8
e_full_expr_string_from_key(Arena *arena, E_Key key)
{
  E_CacheBundle *bundle = e_cache_bundle_from_key(key);
  String8 result = push_str8_copy(arena, bundle->string);
  if(!e_key_match(bundle->parent_key, e_key_zero()))
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct ParentResolveTask ParentResolveTask;
    struct ParentResolveTask
    {
      ParentResolveTask *next;
      E_Key key;
    };
    scratch_end(scratch);
  }
  return result;
}

//- rjf: comprehensive bundle

internal E_Eval
e_eval_from_bundle(E_CacheBundle *bundle)
{
  E_Eval eval =
  {
    .key       = bundle->key,
    .string    = bundle->string,
    .expr      = e_parse_from_bundle(bundle).expr,
    .irtree    = e_irtree_from_bundle(bundle),
    .bytecode  = e_bytecode_from_bundle(bundle),
    .msgs      = bundle->msgs,
  };
  E_Interpretation interpretation = e_interpretation_from_bundle(bundle);
  eval.code = interpretation.code;
  eval.value = interpretation.value;
  eval.space = interpretation.space;
  return eval;
}

internal E_Eval
e_value_eval_from_eval(E_Eval eval)
{
  ProfBeginFunction();
  if(eval.irtree.mode == E_Mode_Offset)
  {
    E_TypeKey type_key = e_type_key_unwrap(eval.irtree.type_key, E_TypeUnwrapFlag_AllDecorative);
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    if(type_kind == E_TypeKind_Array)
    {
      eval.irtree.mode = E_Mode_Value;
    }
    else
    {
      U64 type_byte_size = e_type_byte_size_from_key(type_key);
      Rng1U64 value_vaddr_range = r1u64(eval.value.u64, eval.value.u64 + type_byte_size);
      MemoryZeroStruct(&eval.value);
      if(!e_type_key_match(type_key, e_type_key_zero()) &&
         type_byte_size <= sizeof(E_Value) &&
         e_space_read(eval.space, &eval.value, value_vaddr_range))
      {
        eval.irtree.mode = E_Mode_Value;
        
        // rjf: mask&shift, for bitfields
        if(type_kind == E_TypeKind_Bitfield && type_byte_size <= sizeof(U64))
        {
          Temp scratch = scratch_begin(0, 0);
          E_Type *type = e_type_from_key__cached(type_key);
          U64 valid_bits_mask = 0;
          for(U64 idx = 0; idx < type->count; idx += 1)
          {
            valid_bits_mask |= (1ull<<idx);
          }
          eval.value.u64 = eval.value.u64 >> type->off;
          eval.value.u64 = eval.value.u64 & valid_bits_mask;
          eval.irtree.type_key = type->direct_type_key;
          scratch_end(scratch);
        }
        
        // rjf: manually sign-extend
        switch(type_kind)
        {
          default: break;
          case E_TypeKind_Char8:
          case E_TypeKind_S8:  {eval.value.s64 = (S64)*((S8 *)&eval.value.u64);}break;
          case E_TypeKind_Char16:
          case E_TypeKind_S16: {eval.value.s64 = (S64)*((S16 *)&eval.value.u64);}break;
          case E_TypeKind_Char32:
          case E_TypeKind_S32: {eval.value.s64 = (S64)*((S32 *)&eval.value.u64);}break;
        }
      }
    }
  }
  ProfEnd();
  return eval;
}

//- rjf: type key -> auto hooks

internal E_ExprList
e_auto_hook_exprs_from_type_key(Arena *arena, E_TypeKey type_key)
{
  ProfBeginFunction();
  E_ExprList exprs = {0};
  if(e_ir_ctx != 0)
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_AutoHookMap *map = e_ir_ctx->auto_hook_map;
    String8 type_string = str8_skip_chop_whitespace(e_type_string_from_key(scratch.arena, type_key));
    
    //- rjf: gather exact-type-key-matches from the map
    if(map != 0 && map->slots_count != 0)
    {
      U64 hash = e_hash_from_string(5381, type_string);
      U64 slot_idx = hash%map->slots_count;
      for(E_AutoHookNode *n = map->slots[slot_idx].first; n != 0; n = n->hash_next)
      {
        if(str8_match(n->type_string, type_string, 0))
        {
          e_expr_list_push(arena, &exprs, n->expr);
        }
      }
    }
    
    //- rjf: gather fuzzy matches from all patterns in the map
    if(map != 0 && map->first_pattern != 0)
    {
      for(E_AutoHookNode *auto_hook_node = map->first_pattern;
          auto_hook_node != 0;
          auto_hook_node = auto_hook_node->pattern_order_next)
      {
        B32 fits_this_type_string = 1;
        U64 scan_pos = 0;
        for(String8Node *n = auto_hook_node->type_pattern_parts.first; n != 0; n = n->next)
        {
          if(n->string.size == 0)
          {
            continue;
          }
          U64 pattern_part_pos = str8_find_needle(type_string, scan_pos, n->string, 0);
          if(pattern_part_pos >= type_string.size)
          {
            fits_this_type_string = 0;
            break;
          }
          scan_pos = pattern_part_pos + n->string.size;
        }
        if(fits_this_type_string)
        {
          e_expr_list_push(arena, &exprs, auto_hook_node->expr);
        }
      }
    }
    
    scratch_end(scratch);
  }
  ProfEnd();
  return exprs;
}

internal E_ExprList
e_auto_hook_exprs_from_type_key__cached(E_TypeKey type_key)
{
  E_ExprList exprs = {0};
  {
    U64 hash = e_hash_from_string(5381, str8_struct(&type_key));
    U64 slot_idx = hash%e_cache->type_auto_hook_cache_map->slots_count;
    E_TypeAutoHookCacheNode *node = 0;
    for(E_TypeAutoHookCacheNode *n = e_cache->type_auto_hook_cache_map->slots[slot_idx].first;
        n != 0;
        n = n->next)
    {
      if(e_type_key_match(n->key, type_key))
      {
        node = n;
      }
    }
    if(node == 0)
    {
      node = push_array(e_cache->arena, E_TypeAutoHookCacheNode, 1);
      SLLQueuePush(e_cache->type_auto_hook_cache_map->slots[slot_idx].first, e_cache->type_auto_hook_cache_map->slots[slot_idx].last, node);
      node->key = type_key;
      node->exprs = e_auto_hook_exprs_from_type_key(e_cache->arena, type_key);
    }
    exprs = node->exprs;
  }
  return exprs;
}

//- rjf: string IDs

internal U64
e_id_from_string(String8 string)
{
  U64 hash = e_hash_from_string(5381, string);
  U64 hash_slot_idx = hash%e_cache->string_id_map->hash_slots_count;
  E_StringIDNode *node = 0;
  for(E_StringIDNode *n = e_cache->string_id_map->hash_slots[hash_slot_idx].first; n != 0; n = n->hash_next)
  {
    if(str8_match(n->string, string, 0))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    e_cache->string_id_gen += 1;
    U64 id = e_cache->string_id_gen;
    U64 id_slot_idx = id%e_cache->string_id_map->id_slots_count;
    node = push_array(e_cache->arena, E_StringIDNode, 1);
    SLLQueuePush_N(e_cache->string_id_map->hash_slots[hash_slot_idx].first, e_cache->string_id_map->hash_slots[hash_slot_idx].last, node, hash_next);
    SLLQueuePush_N(e_cache->string_id_map->id_slots[id_slot_idx].first, e_cache->string_id_map->hash_slots[id_slot_idx].last, node, id_next);
    node->id = id;
    node->string = push_str8_copy(e_cache->arena, string);
  }
  U64 result = node->id;
  return result;
}

internal String8
e_string_from_id(U64 id)
{
  U64 id_slot_idx = id%e_cache->string_id_map->id_slots_count;
  E_StringIDNode *node = 0;
  for(E_StringIDNode *n = e_cache->string_id_map->id_slots[id_slot_idx].first; n != 0; n = n->id_next)
  {
    if(n->id == id)
    {
      node = n;
      break;
    }
  }
  String8 result = {0};
  if(node != 0)
  {
    result = node->string;
  }
  return result;
}

////////////////////////////////
//~ rjf: Key Extension Functions

internal E_Key
e_key_wrap(E_Key key, String8 string)
{
  e_parent_key_push(key);
  E_Key result = e_key_from_string(string);
  e_parent_key_pop();
  return result;
}

internal E_Key
e_key_wrapf(E_Key key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  E_Key result = e_key_wrap(key, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Eval Info Extraction

internal U64
e_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(e_type_key_unwrap(eval.irtree.type_key, E_TypeUnwrapFlag_AllDecorative))))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

internal Rng1U64
e_range_from_eval(E_Eval eval)
{
  U64 size = 0;
  E_Type *type = e_type_from_key__cached(eval.irtree.type_key);
  if(type->kind == E_TypeKind_Lens)
  {
    for EachIndex(idx, type->count)
    {
      E_Expr *arg = type->args[idx];
      if(arg->kind == E_ExprKind_Define && str8_match(arg->first->string, str8_lit("size"), 0))
      {
        size = e_value_from_expr(arg->first->next).u64;
        break;
      }
    }
  }
  E_TypeKey type_key = e_type_key_unwrap(eval.irtree.type_key, E_TypeUnwrapFlag_AllDecorative);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_key_unwrap(type_key, E_TypeUnwrapFlag_All);
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  if(size == 0 && e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                               direct_type_kind == E_TypeKind_Union ||
                                                               direct_type_kind == E_TypeKind_Class ||
                                                               direct_type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(direct_type_key);
  }
  if(size == 0 && eval.irtree.mode == E_Mode_Offset && (type_kind == E_TypeKind_Struct ||
                                                        type_kind == E_TypeKind_Union ||
                                                        type_kind == E_TypeKind_Class ||
                                                        type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(type_key);
  }
  if(size == 0)
  {
    size = KB(16);
  }
  Rng1U64 result = {0};
  result.min = e_base_offset_from_eval(eval);
  result.max = result.min + size;
  return result;
}


////////////////////////////////
//~ rjf: Debug Functions

internal String8
e_debug_log_from_expr_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  char *indent_spaces = "                                                                                                                                ";
  String8List strings = {0};
  
  //- rjf: begin expression
  String8 expr_text = string;
  str8_list_pushf(scratch.arena, &strings, "`%S`\n", expr_text);
  
  //- rjf: parse
  E_Parse parse = e_push_parse_from_string(scratch.arena, expr_text);
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E_Expr *expr;
      S32 indent;
    };
    E_TokenArray tokens = parse.tokens;
    str8_list_pushf(scratch.arena, &strings, "    tokens:\n");
    for EachIndex(idx, tokens.count)
    {
      E_Token token = tokens.v[idx];
      String8 token_string = str8_substr(expr_text, token.range);
      str8_list_pushf(scratch.arena, &strings, "        %S: `%S`\n", e_token_kind_strings[token.kind], token_string);
    }
    str8_list_pushf(scratch.arena, &strings, "    expr:\n");
    Task start_task = {0, parse.expr, 2};
    Task *first_task = &start_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      E_Expr *expr = t->expr;
      str8_list_pushf(scratch.arena, &strings, "%.*s%S", (int)t->indent*4, indent_spaces, e_expr_kind_strings[expr->kind]);
      switch(expr->kind)
      {
        default:{}break;
        case E_ExprKind_LeafU64:
        {
          str8_list_pushf(scratch.arena, &strings, " (%I64u)", expr->value.u64);
        }break;
        case E_ExprKind_LeafIdentifier:
        {
          str8_list_pushf(scratch.arena, &strings, " (`%S`)", expr->string);
        }break;
      }
      str8_list_pushf(scratch.arena, &strings, "\n");
      Task *last_task = t;
      for(E_Expr *child = expr->first; child != &e_expr_nil; child = child->next)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->next = last_task->next;
        last_task->next = task;
        task->expr = child;
        task->indent = t->indent+1;
        last_task = task;
      }
    }
  }
  
  //- rjf: type
  E_IRTreeAndType irtree = e_push_irtree_and_type_from_expr(scratch.arena, 0, 0, 0, 0, parse.expr);
  {
    str8_list_pushf(scratch.arena, &strings, "    type:\n");
    S32 indent = 2;
    for(E_TypeKey type_key = irtree.type_key;
        !e_type_key_match(e_type_key_zero(), type_key);
        type_key = e_type_key_direct(type_key),
        indent += 1)
    {
      E_Type *type = e_type_from_key(scratch.arena, type_key);
      str8_list_pushf(scratch.arena, &strings, "%.*s%S\n", (int)indent*4, indent_spaces, e_type_kind_basic_string_table[type->kind]);
    }
  }
  
  //- rjf: irtree
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      E_IRNode *irnode;
      S32 indent;
    };
    str8_list_pushf(scratch.arena, &strings, "    ir_tree:\n");
    Task start_task = {0, irtree.root, 2};
    Task *first_task = &start_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      E_IRNode *irnode = t->irnode;
      str8_list_pushf(scratch.arena, &strings, "%.*s", (int)t->indent*4, indent_spaces);
      switch(irnode->op)
      {
        default:{}break;
#define X(name) case RDI_EvalOp_##name:{str8_list_pushf(scratch.arena, &strings, #name);}break;
        RDI_EvalOp_XList
#undef X
      }
      if(irnode->value.u64 != 0)
      {
        str8_list_pushf(scratch.arena, &strings, " (%I64u)", irnode->value.u64);
      }
      str8_list_pushf(scratch.arena, &strings, "\n");
      Task *last_task = t;
      for(E_IRNode *child = irnode->first; child != &e_irnode_nil; child = child->next)
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->next = last_task->next;
        last_task->next = task;
        task->irnode = child;
        task->indent = t->indent+1;
        last_task = task;
      }
    }
  }
  
  str8_list_pushf(scratch.arena, &strings, "\n");
  
  String8 result = str8_list_join(arena, &strings, 0);
  scratch_end(scratch);
  return result;
}
