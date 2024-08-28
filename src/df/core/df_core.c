// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef RADDBG_LAYER_COLOR
#define RADDBG_LAYER_COLOR 0.70f, 0.50f, 0.25f

////////////////////////////////
//~ rjf: Generated Code

#include "df/core/generated/df_core.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
df_hash_from_seed_string(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
df_hash_from_string(String8 string)
{
  return df_hash_from_seed_string(5381, string);
}

internal U64
df_hash_from_seed_string__case_insensitive(U64 seed, String8 string)
{
  U64 result = seed;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + char_to_lower(string.str[i]);
  }
  return result;
}

internal U64
df_hash_from_string__case_insensitive(String8 string)
{
  return df_hash_from_seed_string__case_insensitive(5381, string);
}

////////////////////////////////
//~ rjf: Handles

internal DF_Handle
df_handle_zero(void)
{
  DF_Handle result = {0};
  return result;
}

internal B32
df_handle_match(DF_Handle a, DF_Handle b)
{
  return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1]);
}

internal void
df_handle_list_push_node(DF_HandleList *list, DF_HandleNode *node)
{
  DLLPushBack(list->first, list->last, node);
  list->count += 1;
}

internal void
df_handle_list_push(Arena *arena, DF_HandleList *list, DF_Handle handle)
{
  DF_HandleNode *n = push_array(arena, DF_HandleNode, 1);
  n->handle = handle;
  df_handle_list_push_node(list, n);
}

internal void
df_handle_list_remove(DF_HandleList *list, DF_HandleNode *node)
{
  DLLRemove(list->first, list->last, node);
  list->count -= 1;
}

internal DF_HandleNode *
df_handle_list_find(DF_HandleList *list, DF_Handle handle)
{
  DF_HandleNode *result = 0;
  for(DF_HandleNode *n = list->first; n != 0; n = n->next)
  {
    if(df_handle_match(n->handle, handle))
    {
      result = n;
      break;
    }
  }
  return result;
}

internal DF_HandleList
df_push_handle_list_copy(Arena *arena, DF_HandleList list)
{
  DF_HandleList result = {0};
  for(DF_HandleNode *n = list.first; n != 0; n = n->next)
  {
    df_handle_list_push(arena, &result, n->handle);
  }
  return result;
}

////////////////////////////////
//~ rjf: State History Data Structure

internal DF_StateDeltaHistory *
df_state_delta_history_alloc(void)
{
  Arena *arena = arena_alloc();
  DF_StateDeltaHistory *hist = push_array(arena, DF_StateDeltaHistory, 1);
  hist->arena = arena;
  for(Side side = (Side)0; side < Side_COUNT; side = (Side)(side+1))
  {
    hist->side_arenas[side] = arena_alloc();
  }
  return hist;
}

internal void
df_state_delta_history_release(DF_StateDeltaHistory *hist)
{
  for(Side side = (Side)0; side < Side_COUNT; side = (Side)(side+1))
  {
    arena_release(hist->side_arenas[side]);
  }
  arena_release(hist->arena);
}

internal void
df_state_delta_history_batch_begin(DF_StateDeltaHistory *hist)
{
  if(hist == 0) { return; }
  if(hist->side_arenas[Side_Max] != 0)
  {
    arena_clear(hist->side_arenas[Side_Max]);
    hist->side_tops[Side_Max] = 0;
  }
  DF_StateDeltaBatch *batch = push_array(hist->side_arenas[Side_Min], DF_StateDeltaBatch, 1);
  SLLStackPush(hist->side_tops[Side_Min], batch);
  hist->batch_is_active = 1;
}

internal void
df_state_delta_history_batch_end(DF_StateDeltaHistory *hist)
{
  if(hist == 0) { return; }
  hist->batch_is_active = 0;
}

internal void
df_state_delta_history_push_delta_(DF_StateDeltaHistory *hist, DF_StateDeltaParams *params)
{
  if(hist == 0) { return; }
  DF_StateDeltaBatch *batch = hist->side_tops[Side_Min];
  if(batch == 0 || hist->batch_is_active == 0) { return; }
  DF_StateDeltaNode *n = push_array(hist->side_arenas[Side_Min], DF_StateDeltaNode, 1);
  SLLQueuePush(batch->first, batch->last, n);
  n->v.guard_entity = df_handle_from_entity(params->guard_entity);
  n->v.vaddr = (U64)params->ptr;
  n->v.data = push_str8_copy(hist->arena, str8((U8*)params->ptr, params->size));
}

internal void
df_state_delta_history_wind(DF_StateDeltaHistory *hist, Side side)
{
  if(hist == 0) { return; }
  B32 done = 0;
  for(DF_StateDeltaBatch *src_batch = hist->side_tops[side];
      src_batch != 0 && !done;
      src_batch = hist->side_tops[side])
  {
    U64 pop_pos = (U64)hist->side_tops[side] - (U64)hist->side_arenas[side];
    SLLStackPop(hist->side_tops[side]);
    {
      DF_StateDeltaBatch *dst_batch = push_array(hist->side_arenas[side_flip(side)], DF_StateDeltaBatch, 1);
      SLLStackPush(hist->side_tops[side_flip(side)], dst_batch);
      for(DF_StateDeltaNode *src_n = src_batch->first; src_n != 0; src_n = src_n->next)
      {
        DF_StateDelta *src_delta = &src_n->v;
        B32 handle_is_good = (df_handle_match(src_delta->guard_entity, df_handle_zero()) ||
                              !df_entity_is_nil(df_entity_from_handle(src_delta->guard_entity)));
        if(handle_is_good)
        {
          DF_StateDeltaNode *dst_n = push_array(hist->side_arenas[side_flip(side)], DF_StateDeltaNode, 1);
          SLLQueuePush(dst_batch->first, dst_batch->last, dst_n);
          dst_n->v.vaddr = src_delta->vaddr;
          dst_n->v.data = push_str8_copy(hist->side_arenas[side_flip(side)], str8((U8 *)src_delta->vaddr, src_delta->data.size));
          MemoryCopy((void *)src_delta->vaddr, src_delta->data.str, src_delta->data.size);
          done = 1;
        }
      }
    }
    arena_pop_to(hist->side_arenas[side], pop_pos);
  }
}

////////////////////////////////
//~ rjf: Sparse Tree Expansion State Data Structure

//- rjf: keys

internal DF_ExpandKey
df_expand_key_make(U64 parent_hash, U64 child_num)
{
  DF_ExpandKey key;
  {
    key.parent_hash = parent_hash;
    key.child_num = child_num;
  }
  return key;
}

internal DF_ExpandKey
df_expand_key_zero(void)
{
  DF_ExpandKey key = {0};
  return key;
}

internal B32
df_expand_key_match(DF_ExpandKey a, DF_ExpandKey b)
{
  return MemoryMatchStruct(&a, &b);
}

internal U64
df_hash_from_expand_key(DF_ExpandKey key)
{
  U64 data[] =
  {
    key.child_num,
  };
  U64 hash = df_hash_from_seed_string(key.parent_hash, str8((U8 *)data, sizeof(data)));
  return hash;
}

//- rjf: table

internal void
df_expand_tree_table_init(Arena *arena, DF_ExpandTreeTable *table, U64 slot_count)
{
  MemoryZeroStruct(table);
  table->slots_count = slot_count;
  table->slots = push_array(arena, DF_ExpandSlot, table->slots_count);
}

internal DF_ExpandNode *
df_expand_node_from_key(DF_ExpandTreeTable *table, DF_ExpandKey key)
{
  U64 hash = df_hash_from_expand_key(key);
  U64 slot_idx = hash%table->slots_count;
  DF_ExpandSlot *slot = &table->slots[slot_idx];
  DF_ExpandNode *node = 0;
  for(DF_ExpandNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(df_expand_key_match(n->key, key))
    {
      node = n;
      break;
    }
  }
  return node;
}

internal B32
df_expand_key_is_set(DF_ExpandTreeTable *table, DF_ExpandKey key)
{
  DF_ExpandNode *node = df_expand_node_from_key(table, key);
  return (node != 0 && node->expanded);
}

internal void
df_expand_set_expansion(Arena *arena, DF_ExpandTreeTable *table, DF_ExpandKey parent_key, DF_ExpandKey key, B32 expanded)
{
  // rjf: map keys => nodes
  DF_ExpandNode *parent_node = df_expand_node_from_key(table, parent_key);
  DF_ExpandNode *node = df_expand_node_from_key(table, key);
  
  // rjf: make node if we don't have one, and we need one
  if(node == 0 && expanded)
  {
    node = table->free_node;
    if(node != 0)
    {
      table->free_node = table->free_node->next;
      MemoryZeroStruct(node);
    }
    else
    {
      node = push_array(arena, DF_ExpandNode, 1);
    }
    
    // rjf: link into table
    U64 hash = df_hash_from_expand_key(key);
    U64 slot = hash % table->slots_count;
    DLLPushBack_NP(table->slots[slot].first, table->slots[slot].last, node, hash_next, hash_prev);
    
    // rjf: link into parent
    if(parent_node != 0)
    {
      DF_ExpandNode *prev = 0;
      for(DF_ExpandNode *n = parent_node->first; n != 0; n = n->next)
      {
        if(n->key.child_num < key.child_num)
        {
          prev = n;
        }
        else
        {
          break;
        }
      }
      DLLInsert_NP(parent_node->first, parent_node->last, prev, node, next, prev);
      node->parent = parent_node;
    }
  }
  
  // rjf: fill
  if(node != 0)
  {
    node->key = key;
    node->expanded = expanded;
  }
  
  // rjf: unlink node & free if we don't need it anymore
  if(expanded == 0 && node != 0 && node->first == 0)
  {
    // rjf: unlink from table
    U64 hash = df_hash_from_expand_key(key);
    U64 slot = hash % table->slots_count;
    DLLRemove_NP(table->slots[slot].first, table->slots[slot].last, node, hash_next, hash_prev);
    
    // rjf: unlink from tree
    if(parent_node != 0)
    {
      DLLRemove_NP(parent_node->first, parent_node->last, node, next, prev);
    }
    
    // rjf: free
    node->next = table->free_node;
    table->free_node = node;
  }
}

////////////////////////////////
//~ rjf: Config Type Functions

internal DF_CfgTree *
df_cfg_tree_copy(Arena *arena, DF_CfgTree *src)
{
  DF_CfgTree *dst = push_array(arena, DF_CfgTree, 1);
  dst->source = src->source;
  dst->root = md_tree_copy(arena, src->root);
  return dst;
}

internal void
df_cfg_table_push_unparsed_string(Arena *arena, DF_CfgTable *table, String8 string, DF_CfgSrc source)
{
  if(table->slot_count == 0)
  {
    table->slot_count = 64;
    table->slots = push_array(arena, DF_CfgSlot, table->slot_count);
  }
  MD_TokenizeResult tokenize = md_tokenize_from_text(arena, string);
  MD_ParseResult parse = md_parse_from_text_tokens(arena, str8_lit(""), string, tokenize.tokens);
  for(MD_EachNode(tln, parse.root->first)) if(tln->string.size != 0)
  {
    // rjf: map string -> hash*slot
    String8 string = str8(tln->string.str, tln->string.size);
    U64 hash = df_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % table->slot_count;
    DF_CfgSlot *slot = &table->slots[slot_idx];
    
    // rjf: find existing value for this string
    DF_CfgVal *val = 0;
    for(DF_CfgVal *v = slot->first; v != 0; v = v->hash_next)
    {
      if(str8_match(v->string, string, StringMatchFlag_CaseInsensitive))
      {
        val = v;
        break;
      }
    }
    
    // rjf: create new value if needed
    if(val == 0)
    {
      val = push_array(arena, DF_CfgVal, 1);
      val->string = push_str8_copy(arena, string);
      val->insertion_stamp = table->insertion_stamp_counter;
      SLLStackPush_N(slot->first, val, hash_next);
      SLLQueuePush_N(table->first_val, table->last_val, val, linear_next);
      table->insertion_stamp_counter += 1;
    }
    
    // rjf: create new node within this value
    DF_CfgTree *tree = push_array(arena, DF_CfgTree, 1);
    SLLQueuePush_NZ(&df_g_nil_cfg_tree, val->first, val->last, tree, next);
    tree->source = source;
    tree->root   = md_tree_copy(arena, tln);
  }
}

internal DF_CfgTable
df_cfg_table_from_inheritance(Arena *arena, DF_CfgTable *src)
{
  DF_CfgTable dst_ = {0};
  DF_CfgTable *dst = &dst_;
  {
    dst->slot_count = src->slot_count;
    dst->slots = push_array(arena, DF_CfgSlot, dst->slot_count);
  }
  for(DF_CfgVal *src_val = src->first_val; src_val != 0 && src_val != &df_g_nil_cfg_val; src_val = src_val->linear_next)
  {
    DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(src_val->string);
    if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_Inherited)
    {
      U64 hash = df_hash_from_string(spec->info.string);
      U64 dst_slot_idx = hash%dst->slot_count;
      DF_CfgSlot *dst_slot = &dst->slots[dst_slot_idx];
      DF_CfgVal *dst_val = push_array(arena, DF_CfgVal, 1);
      dst_val->first = src_val->first;
      dst_val->last = src_val->last;
      dst_val->string = src_val->string;
      dst_val->insertion_stamp = dst->insertion_stamp_counter;
      SLLStackPush_N(dst_slot->first, dst_val, hash_next);
      dst->insertion_stamp_counter += 1;
    }
  }
  return dst_;
}

internal DF_CfgTable
df_cfg_table_copy(Arena *arena, DF_CfgTable *src)
{
  DF_CfgTable result = {0};
  result.slot_count = src->slot_count;
  result.slots = push_array(arena, DF_CfgSlot, result.slot_count);
  MemoryCopy(result.slots, src->slots, sizeof(DF_CfgSlot)*result.slot_count);
  return result;
}

internal DF_CfgVal *
df_cfg_val_from_string(DF_CfgTable *table, String8 string)
{
  DF_CfgVal *result = &df_g_nil_cfg_val;
  if(table->slot_count != 0)
  {
    U64 hash = df_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % table->slot_count;
    DF_CfgSlot *slot = &table->slots[slot_idx];
    for(DF_CfgVal *val = slot->first; val != 0; val = val->hash_next)
    {
      if(str8_match(val->string, string, StringMatchFlag_CaseInsensitive))
      {
        result = val;
        break;
      }
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Debug Info Extraction Type Pure Functions

internal DF_LineList
df_line_list_copy(Arena *arena, DF_LineList *list)
{
  DF_LineList dst = {0};
  for(DF_LineNode *src_n = list->first; src_n != 0; src_n = src_n->next)
  {
    DF_LineNode *dst_n = push_array(arena, DF_LineNode, 1);
    MemoryCopyStruct(dst_n, src_n);
    dst_n->v.dbgi_key = di_key_copy(arena, &src_n->v.dbgi_key);
    SLLQueuePush(dst.first, dst.last, dst_n);
    dst.count += 1;
  }
  return dst;
}

////////////////////////////////
//~ rjf: Control Flow Analysis Functions

internal DF_CtrlFlowInfo
df_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DASM_InstFlags exit_points_mask, Architecture arch, U64 vaddr, String8 code)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_CtrlFlowInfo info = {0};
  for(U64 offset = 0; offset < code.size;)
  {
    DASM_Inst inst = dasm_inst_from_code(scratch.arena, arch, vaddr+offset, str8_skip(code, offset), DASM_Syntax_Intel);
    U64 inst_vaddr = vaddr+offset;
    offset += inst.size;
    info.total_size += inst.size;
    if(inst.flags & exit_points_mask)
    {
      DF_CtrlFlowPoint point = {0};
      point.inst_flags = inst.flags;
      point.vaddr = inst_vaddr;
      point.jump_dest_vaddr = inst.jump_dest_vaddr;
      DF_CtrlFlowPointNode *node = push_array(arena, DF_CtrlFlowPointNode, 1);
      node->v = point;
      SLLQueuePush(info.exit_points.first, info.exit_points.last, node);
      info.exit_points.count += 1;
    }
  }
  scratch_end(scratch);
  return info;
}

////////////////////////////////
//~ rjf: Command Type Pure Functions

//- rjf: specs

internal B32
df_cmd_spec_is_nil(DF_CmdSpec *spec)
{
  return (spec == 0 || spec == &df_g_nil_cmd_spec);
}

internal void
df_cmd_spec_list_push(Arena *arena, DF_CmdSpecList *list, DF_CmdSpec *spec)
{
  DF_CmdSpecNode *n = push_array(arena, DF_CmdSpecNode, 1);
  n->spec = spec;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal DF_CmdSpecArray
df_cmd_spec_array_from_list(Arena *arena, DF_CmdSpecList list)
{
  DF_CmdSpecArray result = {0};
  result.count = list.count;
  result.v = push_array(arena, DF_CmdSpec *, list.count);
  U64 idx = 0;
  for(DF_CmdSpecNode *n = list.first; n != 0; n = n->next, idx += 1)
  {
    result.v[idx] = n->spec;
  }
  return result;
}

internal int
df_qsort_compare_cmd_spec__run_counter(DF_CmdSpec **a, DF_CmdSpec **b)
{
  int result = 0;
  if(a[0]->run_count > b[0]->run_count)
  {
    result = -1;
  }
  else if(a[0]->run_count < b[0]->run_count)
  {
    result = +1;
  }
  return result;
}

internal void
df_cmd_spec_array_sort_by_run_counter__in_place(DF_CmdSpecArray array)
{
  quick_sort(array.v, array.count, sizeof(DF_CmdSpec *), df_qsort_compare_cmd_spec__run_counter);
}

internal DF_Handle
df_handle_from_cmd_spec(DF_CmdSpec *spec)
{
  DF_Handle handle = {0};
  handle.u64[0] = (U64)spec;
  return handle;
}

internal DF_CmdSpec *
df_cmd_spec_from_handle(DF_Handle handle)
{
  DF_CmdSpec *result = (DF_CmdSpec *)handle.u64[0];
  if(result == 0)
  {
    result = &df_g_nil_cmd_spec;
  }
  return result;
}

//- rjf: string -> command parsing

internal String8
df_cmd_name_part_from_string(String8 string)
{
  String8 result = string;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    if(idx == string.size || char_is_space(string.str[idx]))
    {
      result = str8_prefix(string, idx);
      break;
    }
  }
  return result;
}

internal String8
df_cmd_arg_part_from_string(String8 string)
{
  String8 result = str8_lit("");
  B32 found_space = 0;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    if(found_space && (idx == string.size || !char_is_space(string.str[idx])))
    {
      result = str8_skip(string, idx);
      break;
    }
    else if(!found_space && (idx == string.size || char_is_space(string.str[idx])))
    {
      found_space = 1;
    }
  }
  return result;
}

//- rjf: command parameter bundles

internal DF_CmdParams
df_cmd_params_zero(void)
{
  DF_CmdParams p = {0};
  return p;
}

internal void
df_cmd_params_mark_slot(DF_CmdParams *params, DF_CmdParamSlot slot)
{
  params->slot_props[slot/64] |= (1ull<<(slot%64));
}

internal B32
df_cmd_params_has_slot(DF_CmdParams *params, DF_CmdParamSlot slot)
{
  return !!(params->slot_props[slot/64] & (1ull<<(slot%64)));
}

internal String8
df_cmd_params_apply_spec_query(Arena *arena, DF_CmdParams *params, DF_CmdSpec *spec, String8 query)
{
  String8 error = {0};
  switch(spec->info.query.slot)
  {
    default:
    case DF_CmdParamSlot_String:
    {
      params->string = push_str8_copy(arena, query);
      df_cmd_params_mark_slot(params, DF_CmdParamSlot_String);
    }break;
    case DF_CmdParamSlot_FilePath:
    {
      String8TxtPtPair pair = str8_txt_pt_pair_from_string(query);
      params->file_path = push_str8_copy(arena, pair.string);
      params->text_point = pair.pt;
      df_cmd_params_mark_slot(params, DF_CmdParamSlot_FilePath);
      df_cmd_params_mark_slot(params, DF_CmdParamSlot_TextPoint);
    }break;
    case DF_CmdParamSlot_TextPoint:
    {
      U64 v = 0;
      if(try_u64_from_str8_c_rules(query, &v))
      {
        params->text_point.column = 1;
        params->text_point.line = v;
        df_cmd_params_mark_slot(params, DF_CmdParamSlot_TextPoint);
      }
      else
      {
        error = str8_lit("Couldn't interpret as a line number.");
      }
    }break;
    case DF_CmdParamSlot_VirtualAddr: goto use_numeric_eval;
    case DF_CmdParamSlot_VirtualOff: goto use_numeric_eval;
    case DF_CmdParamSlot_Index: goto use_numeric_eval;
    case DF_CmdParamSlot_ID: goto use_numeric_eval;
    use_numeric_eval:
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_Eval eval = e_eval_from_string(scratch.arena, query);
      if(eval.msgs.max_kind == E_MsgKind_Null)
      {
        E_TypeKind eval_type_kind = e_type_kind_from_key(e_type_unwrap(eval.type_key));
        if(eval_type_kind == E_TypeKind_Ptr ||
           eval_type_kind == E_TypeKind_LRef ||
           eval_type_kind == E_TypeKind_RRef)
        {
          eval = e_value_eval_from_eval(eval);
        }
        U64 u64 = eval.value.u64;
        switch(spec->info.query.slot)
        {
          default:{}break;
          case DF_CmdParamSlot_VirtualAddr:
          {
            params->vaddr = u64;
            df_cmd_params_mark_slot(params, DF_CmdParamSlot_VirtualAddr);
          }break;
          case DF_CmdParamSlot_VirtualOff:
          {
            params->voff = u64;
            df_cmd_params_mark_slot(params, DF_CmdParamSlot_VirtualOff);
          }break;
          case DF_CmdParamSlot_Index:
          {
            params->index = u64;
            df_cmd_params_mark_slot(params, DF_CmdParamSlot_Index);
          }break;
          case DF_CmdParamSlot_UnwindIndex:
          {
            params->unwind_index = u64;
            df_cmd_params_mark_slot(params, DF_CmdParamSlot_UnwindIndex);
          }break;
          case DF_CmdParamSlot_InlineDepth:
          {
            params->inline_depth = u64;
            df_cmd_params_mark_slot(params, DF_CmdParamSlot_InlineDepth);
          }break;
          case DF_CmdParamSlot_ID:
          {
            params->id = u64;
            df_cmd_params_mark_slot(params, DF_CmdParamSlot_ID);
          }break;
        }
      }
      else
      {
        error = push_str8f(scratch.arena, "Couldn't evaluate \"%S\" as an address.", query);
      }
      scratch_end(scratch);
    }break;
  }
  return error;
}

//- rjf: command lists

internal void
df_cmd_list_push(Arena *arena, DF_CmdList *cmds, DF_CmdParams *params, DF_CmdSpec *spec)
{
  DF_CmdNode *n = push_array(arena, DF_CmdNode, 1);
  n->cmd.spec = spec;
  n->cmd.params = df_cmd_params_copy(arena, params);
  DLLPushBack(cmds->first, cmds->last, n);
  cmds->count += 1;
}

//- rjf: string -> core layer command kind

internal DF_CoreCmdKind
df_core_cmd_kind_from_string(String8 string)
{
  DF_CoreCmdKind result = DF_CoreCmdKind_Null;
  for(U64 idx = 0; idx < ArrayCount(df_g_core_cmd_kind_spec_info_table); idx += 1)
  {
    if(str8_match(string, df_g_core_cmd_kind_spec_info_table[idx].string, StringMatchFlag_CaseInsensitive))
    {
      result = (DF_CoreCmdKind)idx;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Entity Functions

//- rjf: nil

internal B32
df_entity_is_nil(DF_Entity *entity)
{
  return (entity == 0 || entity == &df_g_nil_entity);
}

//- rjf: handle <-> entity conversions

internal U64
df_index_from_entity(DF_Entity *entity)
{
  return (U64)(entity - df_state->entities_base);
}

internal DF_Handle
df_handle_from_entity(DF_Entity *entity)
{
  DF_Handle handle = df_handle_zero();
  if(!df_entity_is_nil(entity))
  {
    handle.u64[0] = df_index_from_entity(entity);
    handle.u64[1] = entity->gen;
  }
  return handle;
}

internal DF_Entity *
df_entity_from_handle(DF_Handle handle)
{
  DF_Entity *result = df_state->entities_base + handle.u64[0];
  if(handle.u64[0] >= df_state->entities_count || result->gen != handle.u64[1])
  {
    result = &df_g_nil_entity;
  }
  return result;
}

internal DF_EntityList
df_entity_list_from_handle_list(Arena *arena, DF_HandleList handles)
{
  DF_EntityList result = {0};
  for(DF_HandleNode *n = handles.first; n != 0; n = n->next)
  {
    DF_Entity *entity = df_entity_from_handle(n->handle);
    if(!df_entity_is_nil(entity))
    {
      df_entity_list_push(arena, &result, entity);
    }
  }
  return result;
}

internal DF_HandleList
df_handle_list_from_entity_list(Arena *arena, DF_EntityList entities)
{
  DF_HandleList result = {0};
  for(DF_EntityNode *n = entities.first; n != 0; n = n->next)
  {
    DF_Handle handle = df_handle_from_entity(n->entity);
    df_handle_list_push(arena, &result, handle);
  }
  return result;
}

//- rjf: entity recursion iterators

internal DF_EntityRec
df_entity_rec_df(DF_Entity *entity, DF_Entity *subtree_root, U64 sib_off, U64 child_off)
{
  DF_EntityRec result = {0};
  if(!df_entity_is_nil(*MemberFromOffset(DF_Entity **, entity, child_off)))
  {
    result.next = *MemberFromOffset(DF_Entity **, entity, child_off);
    result.push_count = 1;
  }
  else for(DF_Entity *parent = entity; parent != subtree_root && !df_entity_is_nil(parent); parent = parent->parent)
  {
    if(parent != subtree_root && !df_entity_is_nil(*MemberFromOffset(DF_Entity **, parent, sib_off)))
    {
      result.next = *MemberFromOffset(DF_Entity **, parent, sib_off);
      break;
    }
    result.pop_count += 1;
  }
  return result;
}

//- rjf: ancestor/child introspection

internal DF_Entity *
df_entity_child_from_kind(DF_Entity *entity, DF_EntityKind kind)
{
  DF_Entity *result = &df_g_nil_entity;
  for(DF_Entity *child = entity->first; !df_entity_is_nil(child); child = child->next)
  {
    if(child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

internal DF_Entity *
df_entity_ancestor_from_kind(DF_Entity *entity, DF_EntityKind kind)
{
  DF_Entity *result = &df_g_nil_entity;
  for(DF_Entity *p = entity->parent; !df_entity_is_nil(p); p = p->parent)
  {
    if(p->kind == kind)
    {
      result = p;
      break;
    }
  }
  return result;
}

internal DF_EntityList
df_push_entity_child_list_with_kind(Arena *arena, DF_Entity *entity, DF_EntityKind kind)
{
  DF_EntityList result = {0};
  for(DF_Entity *child = entity->first; !df_entity_is_nil(child); child = child->next)
  {
    if(child->kind == kind)
    {
      df_entity_list_push(arena, &result, child);
    }
  }
  return result;
}

internal DF_Entity *
df_entity_child_from_name_and_kind(DF_Entity *parent, String8 string, DF_EntityKind kind)
{
  DF_Entity *result = &df_g_nil_entity;
  for(DF_Entity *child = parent->first; !df_entity_is_nil(child); child = child->next)
  {
    if(str8_match(child->name, string, 0) && child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

//- rjf: entity list building

internal void
df_entity_list_push(Arena *arena, DF_EntityList *list, DF_Entity *entity)
{
  DF_EntityNode *n = push_array(arena, DF_EntityNode, 1);
  n->entity = entity;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal DF_EntityArray
df_entity_array_from_list(Arena *arena, DF_EntityList *list)
{
  DF_EntityArray result = {0};
  result.count = list->count;
  result.v = push_array(arena, DF_Entity *, result.count);
  U64 idx = 0;
  for(DF_EntityNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    result.v[idx] = n->entity;
  }
  return result;
}

//- rjf: entity fuzzy list building

internal DF_EntityFuzzyItemArray
df_entity_fuzzy_item_array_from_entity_list_needle(Arena *arena, DF_EntityList *list, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_EntityArray array = df_entity_array_from_list(scratch.arena, list);
  DF_EntityFuzzyItemArray result = df_entity_fuzzy_item_array_from_entity_array_needle(arena, &array, needle);
  return result;
}

internal DF_EntityFuzzyItemArray
df_entity_fuzzy_item_array_from_entity_array_needle(Arena *arena, DF_EntityArray *array, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_EntityFuzzyItemArray result = {0};
  result.count = array->count;
  result.v = push_array(arena, DF_EntityFuzzyItem, result.count);
  U64 result_idx = 0;
  for(U64 src_idx = 0; src_idx < array->count; src_idx += 1)
  {
    DF_Entity *entity = array->v[src_idx];
    String8 display_string = df_display_string_from_entity(scratch.arena, entity);
    FuzzyMatchRangeList matches = fuzzy_match_find(arena, needle, display_string);
    if(matches.count >= matches.needle_part_count)
    {
      result.v[result_idx].entity = entity;
      result.v[result_idx].matches = matches;
      result_idx += 1;
    }
    else
    {
      String8 search_tags = df_search_tags_from_entity(scratch.arena, entity);
      if(search_tags.size != 0)
      {
        FuzzyMatchRangeList tag_matches = fuzzy_match_find(scratch.arena, needle, search_tags);
        if(tag_matches.count >= tag_matches.needle_part_count)
        {
          result.v[result_idx].entity = entity;
          result.v[result_idx].matches = matches;
          result_idx += 1;
        }
      }
    }
  }
  result.count = result_idx;
  scratch_end(scratch);
  return result;
}

//- rjf: full path building, from file/folder entities

internal String8
df_full_path_from_entity(Arena *arena, DF_Entity *entity)
{
  String8 string = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List strs = {0};
    for(DF_Entity *e = entity; !df_entity_is_nil(e); e = e->parent)
    {
      if(e->kind == DF_EntityKind_File)
      {
        str8_list_push_front(scratch.arena, &strs, e->name);
      }
    }
    StringJoin join = {0};
    join.sep = str8_lit("/");
    string = str8_list_join(arena, &strs, &join);
    scratch_end(scratch);
  }
  return string;
}

//- rjf: display string entities, for referencing entities in ui

internal String8
df_display_string_from_entity(Arena *arena, DF_Entity *entity)
{
  String8 result = {0};
  switch(entity->kind)
  {
    default:
    {
      if(entity->name.size != 0)
      {
        result = push_str8_copy(arena, entity->name);
      }
      else
      {
        String8 kind_string = df_g_entity_kind_display_string_table[entity->kind];
        result = push_str8f(arena, "%S $%I64u", kind_string, entity->id);
      }
    }break;
    
    case DF_EntityKind_Target:
    {
      if(entity->name.size != 0)
      {
        result = push_str8_copy(arena, entity->name);
      }
      else
      {
        DF_Entity *exe = df_entity_child_from_kind(entity, DF_EntityKind_Executable);
        result = push_str8_copy(arena, exe->name);
      }
    }break;
    
    case DF_EntityKind_Breakpoint:
    {
      if(entity->name.size != 0)
      {
        result = push_str8_copy(arena, entity->name);
      }
      else
      {
        DF_Entity *loc = df_entity_child_from_kind(entity, DF_EntityKind_Location);
        if(loc->flags & DF_EntityFlag_HasTextPoint)
        {
          result = push_str8f(arena, "%S:%I64d:%I64d", str8_skip_last_slash(loc->name), loc->text_point.line, loc->text_point.column);
        }
        else if(loc->flags & DF_EntityFlag_HasVAddr)
        {
          result = str8_from_u64(arena, loc->vaddr, 16, 16, 0);
        }
        else if(loc->name.size != 0)
        {
          result = push_str8_copy(arena, loc->name);
        }
      }
    }break;
    
    case DF_EntityKind_Process:
    {
      DF_Entity *main_mod_child = df_entity_child_from_kind(entity, DF_EntityKind_Module);
      String8 main_mod_name = str8_skip_last_slash(main_mod_child->name);
      result = push_str8f(arena, "%S%s%sPID: %i%s",
                          main_mod_name,
                          main_mod_name.size != 0 ? " " : "",
                          main_mod_name.size != 0 ? "(" : "",
                          entity->ctrl_id,
                          main_mod_name.size != 0 ? ")" : "");
    }break;
    
    case DF_EntityKind_Thread:
    {
      String8 name = entity->name;
      if(name.size == 0)
      {
        DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
        DF_Entity *first_thread = df_entity_child_from_kind(process, DF_EntityKind_Thread);
        if(first_thread == entity)
        {
          name = str8_lit("Main Thread");
        }
      }
      result = push_str8f(arena, "%S%s%sTID: %i%s",
                          name,
                          name.size != 0 ? " " : "",
                          name.size != 0 ? "(" : "",
                          entity->ctrl_id,
                          name.size != 0 ? ")" : "");
    }break;
    
    case DF_EntityKind_Module:
    {
      result = push_str8_copy(arena, str8_skip_last_slash(entity->name));
    }break;
    
    case DF_EntityKind_RecentProject:
    {
      result = push_str8_copy(arena, str8_skip_last_slash(entity->name));
    }break;
  }
  return result;
}

//- rjf: extra search tag strings for fuzzy filtering entities

internal String8
df_search_tags_from_entity(Arena *arena, DF_Entity *entity)
{
  String8 result = {0};
  if(entity->kind == DF_EntityKind_Thread)
  {
    Temp scratch = scratch_begin(&arena, 1);
    DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
    CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
    String8List strings = {0};
    for(U64 frame_num = unwind.frames.count; frame_num > 0; frame_num -= 1)
    {
      CTRL_UnwindFrame *f = &unwind.frames.v[frame_num-1];
      U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
      DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
      U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
      DI_Key dbgi_key = df_dbgi_key_from_module(module);
      String8 procedure_name = df_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff, 0);
      if(procedure_name.size != 0)
      {
        str8_list_push(scratch.arena, &strings, procedure_name);
      }
    }
    StringJoin join = {0};
    join.sep = str8_lit(",");
    result = str8_list_join(arena, &strings, &join);
    scratch_end(scratch);
  }
  return result;
}

//- rjf: entity -> color operations

internal Vec4F32
df_hsva_from_entity(DF_Entity *entity)
{
  Vec4F32 result = {0};
  if(entity->flags & DF_EntityFlag_HasColor)
  {
    result = entity->color_hsva;
  }
  return result;
}

internal Vec4F32
df_rgba_from_entity(DF_Entity *entity)
{
  Vec4F32 result = {0};
  if(entity->flags & DF_EntityFlag_HasColor)
  {
    Vec3F32 hsv = v3f32(entity->color_hsva.x, entity->color_hsva.y, entity->color_hsva.z);
    Vec3F32 rgb = rgb_from_hsv(hsv);
    result = v4f32(rgb.x, rgb.y, rgb.z, entity->color_hsva.w);
  }
  return result;
}

//- rjf: entity -> expansion tree keys

internal DF_ExpandKey
df_expand_key_from_entity(DF_Entity *entity)
{
  DF_ExpandKey parent_key = df_parent_expand_key_from_entity(entity);
  DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(parent_key), (U64)entity);
  return key;
}

internal DF_ExpandKey
df_parent_expand_key_from_entity(DF_Entity *entity)
{
  DF_ExpandKey parent_key = df_expand_key_make(5381, (U64)entity);
  return parent_key;
}

//- rjf: entity -> evaluation

internal DF_EntityEval *
df_eval_from_entity(Arena *arena, DF_Entity *entity)
{
  DF_EntityEval *eval = push_array(arena, DF_EntityEval, 1);
  {
    DF_Entity *loc = df_entity_child_from_kind(entity, DF_EntityKind_Location);
    DF_Entity *cnd = df_entity_child_from_kind(entity, DF_EntityKind_Condition);
    String8 label_string = push_str8_copy(arena, entity->name);
    String8 loc_string = {0};
    if(loc->flags & DF_EntityFlag_HasTextPoint)
    {
      loc_string = push_str8f(arena, "%S:%I64u:%I64u", loc->name, loc->text_point.line, loc->text_point.column);
    }
    else if(loc->flags & DF_EntityFlag_HasVAddr)
    {
      loc_string = push_str8f(arena, "0x%I64x", loc->vaddr);
    }
    String8 cnd_string = push_str8_copy(arena, cnd->name);
    eval->enabled      = !entity->disabled;
    eval->hit_count    = entity->u64;
    eval->label_off    = (U64)((U8 *)label_string.str - (U8 *)eval);
    eval->location_off = (U64)((U8 *)loc_string.str - (U8 *)eval);
    eval->condition_off= (U64)((U8 *)cnd_string.str - (U8 *)eval);
  }
  return eval;
}

////////////////////////////////
//~ rjf: Name Allocation

internal U64
df_name_bucket_idx_from_string_size(U64 size)
{
  U64 size_rounded = u64_up_to_pow2(size+1);
  size_rounded = ClampBot((1<<4), size_rounded);
  U64 bucket_idx = 0;
  switch(size_rounded)
  {
    case 1<<4: {bucket_idx = 0;}break;
    case 1<<5: {bucket_idx = 1;}break;
    case 1<<6: {bucket_idx = 2;}break;
    case 1<<7: {bucket_idx = 3;}break;
    case 1<<8: {bucket_idx = 4;}break;
    case 1<<9: {bucket_idx = 5;}break;
    case 1<<10:{bucket_idx = 6;}break;
    default:{bucket_idx = ArrayCount(df_state->free_name_chunks)-1;}break;
  }
  return bucket_idx;
}

internal String8
df_name_alloc(String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = df_name_bucket_idx_from_string_size(string.size);
  
  // rjf: loop -> find node, allocate if not there
  //
  // (we do a loop here so that all allocation logic goes through
  // the same path, such that we *always* pull off a free list,
  // rather than just using what was pushed onto an arena directly,
  // which is not undoable; the free lists we control, and are thus
  // trivially undoable)
  //
  DF_NameChunkNode *node = 0;
  for(;node == 0;)
  {
    node = df_state->free_name_chunks[bucket_idx];
    
    // rjf: pull from bucket free list
    if(node != 0)
    {
      if(bucket_idx == ArrayCount(df_state->free_name_chunks)-1)
      {
        node = 0;
        DF_NameChunkNode *prev = 0;
        for(DF_NameChunkNode *n = df_state->free_name_chunks[bucket_idx];
            n != 0;
            prev = n, n = n->next)
        {
          if(n->size >= string.size+1)
          {
            if(prev == 0)
            {
              df_state_delta_history_push_struct_delta(df_state_delta_history(), &df_state->free_name_chunks[bucket_idx]);
              df_state->free_name_chunks[bucket_idx] = n->next;
            }
            else
            {
              df_state_delta_history_push_struct_delta(df_state_delta_history(), &prev->next);
              prev->next = n->next;
            }
            node = n;
            break;
          }
        }
      }
      else
      {
        df_state_delta_history_push_struct_delta(df_state_delta_history(), &df_state->free_name_chunks[bucket_idx]);
        SLLStackPop(df_state->free_name_chunks[bucket_idx]);
      }
    }
    
    // rjf: no found node -> allocate new, push onto associated free list
    if(node == 0)
    {
      U64 chunk_size = 0;
      if(bucket_idx < ArrayCount(df_state->free_name_chunks)-1)
      {
        chunk_size = 1<<(bucket_idx+4);
      }
      else
      {
        chunk_size = u64_up_to_pow2(string.size);
      }
      U8 *chunk_memory = push_array(df_state->arena, U8, chunk_size);
      DF_NameChunkNode *chunk = (DF_NameChunkNode *)chunk_memory;
      SLLStackPush(df_state->free_name_chunks[bucket_idx], chunk);
    }
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  df_state_delta_history_push_delta(df_state_delta_history(), .ptr = allocated_string.str, .size = Max(allocated_string.size, sizeof(*node)));
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
df_name_release(String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = df_name_bucket_idx_from_string_size(string.size);
  DF_NameChunkNode *node = (DF_NameChunkNode *)string.str;
  df_state_delta_history_push_delta(df_state_delta_history(), .ptr = node, .size = Max(node->size, sizeof(*node)));
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &df_state->free_name_chunks[bucket_idx]);
  node->size = u64_up_to_pow2(string.size);
  SLLStackPush(df_state->free_name_chunks[bucket_idx], node);
}

////////////////////////////////
//~ rjf: Entity State Functions

//- rjf: entity mutation notification codepath

internal void
df_entity_notify_mutation(DF_Entity *entity)
{
  for(DF_Entity *e = entity; !df_entity_is_nil(e); e = e->parent)
  {
    DF_EntityKindFlags flags = df_g_entity_kind_flags_table[entity->kind];
    if(e == entity && flags & DF_EntityKindFlag_LeafMutProjectConfig)
    {
      DF_CmdParams p = {0};
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProjectData));
    }
    if(e == entity && flags & DF_EntityKindFlag_LeafMutSoftHalt && df_ctrl_targets_running())
    {
      df_state->entities_mut_soft_halt = 1;
    }
    if(e == entity && flags & DF_EntityKindFlag_LeafMutDebugInfoMap)
    {
      df_state->entities_mut_dbg_info_map = 1;
    }
    if(flags & DF_EntityKindFlag_TreeMutSoftHalt && df_ctrl_targets_running())
    {
      df_state->entities_mut_soft_halt = 1;
    }
    if(flags & DF_EntityKindFlag_TreeMutDebugInfoMap)
    {
      df_state->entities_mut_dbg_info_map = 1;
    }
  }
}

//- rjf: entity allocation + tree forming

internal DF_Entity *
df_entity_alloc(DF_Entity *parent, DF_EntityKind kind)
{
  B32 user_defined_lifetime = !!(df_g_entity_kind_flags_table[kind] & DF_EntityKindFlag_UserDefinedLifetime);
  U64 free_list_idx = !!user_defined_lifetime;
  if(df_entity_is_nil(parent)) { parent = df_state->entities_root; }
  
  // rjf: empty free list -> push new
  if(!df_state->entities_free[free_list_idx])
  {
    DF_Entity *entity = push_array(df_state->entities_arena, DF_Entity, 1);
    df_state->entities_count += 1;
    df_state->entities_free_count += 1;
    SLLStackPush(df_state->entities_free[free_list_idx], entity);
  }
  
  // rjf: pop new entity off free-list
  DF_Entity *entity = df_state->entities_free[free_list_idx];
  SLLStackPop(df_state->entities_free[free_list_idx]);
  df_state->entities_free_count -= 1;
  df_state->entities_active_count += 1;
  
  // rjf: zero entity
  {
    U64 gen = entity->gen;
    MemoryZeroStruct(entity);
    entity->gen = gen;
  }
  
  // rjf: set up alloc'd entity links
  entity->first = entity->last = entity->next = entity->prev = entity->parent = &df_g_nil_entity;
  entity->parent = parent;
  
  // rjf: stitch up parent links
  if(df_entity_is_nil(parent))
  {
    df_state->entities_root = entity;
  }
  else
  {
    DLLPushBack_NPZ(&df_g_nil_entity, parent->first, parent->last, entity, next, prev);
  }
  
  // rjf: fill out metadata
  entity->kind = kind;
  df_state->entities_id_gen += 1;
  entity->id = df_state->entities_id_gen;
  entity->gen += 1;
  entity->alloc_time_us = os_now_microseconds();
  
  // rjf: initialize to deleted, record history, then "undelete" if this allocation can be undone
  if(user_defined_lifetime)
  {
    // TODO(rjf)
  }
  
  // rjf: dirtify caches
  df_state->kind_alloc_gens[kind] += 1;
  df_entity_notify_mutation(entity);
  
  // rjf: log
  LogInfoNamedBlockF("new_entity")
  {
    log_infof("kind: \"%S\"\n", df_g_entity_kind_display_string_table[kind]);
    log_infof("id: $0x%I64x\n", entity->id);
  }
  
  return entity;
}

internal void
df_entity_mark_for_deletion(DF_Entity *entity)
{
  if(!df_entity_is_nil(entity))
  {
    entity->flags |= DF_EntityFlag_MarkedForDeletion;
    df_entity_notify_mutation(entity);
  }
}

internal void
df_entity_release(DF_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: unpack
  U64 free_list_idx = !!(df_g_entity_kind_flags_table[entity->kind] & DF_EntityKindFlag_UserDefinedLifetime);
  
  // rjf: release whole tree
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    DF_Entity *e;
  };
  Task start_task = {0, entity};
  Task *first_task = &start_task;
  Task *last_task = &start_task;
  for(Task *task = first_task; task != 0; task = task->next)
  {
    for(DF_Entity *child = task->e->first; !df_entity_is_nil(child); child = child->next)
    {
      Task *t = push_array(scratch.arena, Task, 1);
      t->e = child;
      SLLQueuePush(first_task, last_task, t);
    }
    LogInfoNamedBlockF("end_entity")
    {
      String8 name = df_display_string_from_entity(scratch.arena, task->e);
      log_infof("kind: \"%S\"\n", df_g_entity_kind_display_string_table[task->e->kind]);
      log_infof("id: $0x%I64x\n", task->e->id);
      log_infof("display_string: \"%S\"\n", name);
    }
    df_set_thread_freeze_state(task->e, 0);
    SLLStackPush(df_state->entities_free[free_list_idx], task->e);
    df_state->entities_free_count += 1;
    df_state->entities_active_count -= 1;
    task->e->gen += 1;
    if(task->e->name.size != 0)
    {
      df_name_release(task->e->name);
    }
    df_state->kind_alloc_gens[task->e->kind] += 1;
  }
  
  scratch_end(scratch);
}

internal void
df_entity_change_parent(DF_Entity *entity, DF_Entity *old_parent, DF_Entity *new_parent, DF_Entity *prev_child)
{
  Assert(entity->parent == old_parent);
  Assert(prev_child->parent == old_parent || df_entity_is_nil(prev_child));
  
  // rjf: fix up links
  if(!df_entity_is_nil(old_parent))
  {
    DLLRemove_NPZ(&df_g_nil_entity, old_parent->first, old_parent->last, entity, next, prev);
  }
  if(!df_entity_is_nil(new_parent))
  {
    DLLInsert_NPZ(&df_g_nil_entity, new_parent->first, new_parent->last, prev_child, entity, next, prev);
  }
  entity->parent = new_parent;
  
  // rjf: notify
  df_entity_notify_mutation(entity);
  df_entity_notify_mutation(new_parent);
  df_entity_notify_mutation(old_parent);
  df_entity_notify_mutation(prev_child);
  df_state->kind_alloc_gens[entity->kind] += 1;
}

//- rjf: entity simple equipment

internal void
df_entity_equip_txt_pt(DF_Entity *entity, TxtPt point)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->text_point, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->text_point = point;
  entity->flags |= DF_EntityFlag_HasTextPoint;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_entity_handle(DF_Entity *entity, DF_Handle handle)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->entity_handle, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->entity_handle = handle;
  entity->flags |= DF_EntityFlag_HasEntityHandle;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_disabled(DF_Entity *entity, B32 value)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->disabled, .guard_entity = entity);
  entity->disabled = value;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_u64(DF_Entity *entity, U64 u64)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->u64, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->u64 = u64;
  entity->flags |= DF_EntityFlag_HasU64;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_color_rgba(DF_Entity *entity, Vec4F32 rgba)
{
  df_require_entity_nonnil(entity, return);
  Vec3F32 rgb = v3f32(rgba.x, rgba.y, rgba.z);
  Vec3F32 hsv = hsv_from_rgb(rgb);
  Vec4F32 hsva = v4f32(hsv.x, hsv.y, hsv.z, rgba.w);
  df_entity_equip_color_hsva(entity, hsva);
}

internal void
df_entity_equip_color_hsva(DF_Entity *entity, Vec4F32 hsva)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->color_hsva, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->color_hsva = hsva;
  entity->flags |= DF_EntityFlag_HasColor;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_cfg_src(DF_Entity *entity, DF_CfgSrc cfg_src)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->cfg_src, .guard_entity = entity);
  entity->cfg_src = cfg_src;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_timestamp(DF_Entity *entity, U64 timestamp)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->timestamp, .guard_entity = entity);
  entity->timestamp = timestamp;
  df_entity_notify_mutation(entity);
}

//- rjf: control layer correllation equipment

internal void
df_entity_equip_ctrl_machine_id(DF_Entity *entity, CTRL_MachineID machine_id)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->ctrl_machine_id, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->ctrl_machine_id = machine_id;
  entity->flags |= DF_EntityFlag_HasCtrlMachineID;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_ctrl_handle(DF_Entity *entity, DMN_Handle handle)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->ctrl_handle, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->ctrl_handle = handle;
  entity->flags |= DF_EntityFlag_HasCtrlHandle;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_arch(DF_Entity *entity, Architecture arch)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->arch, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->arch = arch;
  entity->flags |= DF_EntityFlag_HasArch;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_ctrl_id(DF_Entity *entity, U32 id)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->ctrl_id, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->ctrl_id = id;
  entity->flags |= DF_EntityFlag_HasCtrlID;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_stack_base(DF_Entity *entity, U64 stack_base)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->stack_base, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->stack_base = stack_base;
  entity->flags |= DF_EntityFlag_HasStackBase;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_vaddr_rng(DF_Entity *entity, Rng1U64 range)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->vaddr_rng, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->vaddr_rng = range;
  entity->flags |= DF_EntityFlag_HasVAddrRng;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_vaddr(DF_Entity *entity, U64 vaddr)
{
  df_require_entity_nonnil(entity, return);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->vaddr, .guard_entity = entity);
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->flags, .guard_entity = entity);
  entity->vaddr = vaddr;
  entity->flags |= DF_EntityFlag_HasVAddr;
  df_entity_notify_mutation(entity);
}

//- rjf: name equipment

internal void
df_entity_equip_name(DF_Entity *entity, String8 name)
{
  df_require_entity_nonnil(entity, return);
  if(entity->name.size != 0)
  {
    df_name_release(entity->name);
  }
  df_state_delta_history_push_struct_delta(df_state_delta_history(), &entity->name, .guard_entity = entity);
  if(name.size != 0)
  {
    entity->name = df_name_alloc(name);
  }
  else
  {
    entity->name = str8_zero();
  }
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_namef(DF_Entity *entity, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  df_entity_equip_name(entity, string);
  scratch_end(scratch);
}

//- rjf: opening folders/files & maintaining the entity model of the filesystem

internal DF_Entity *
df_entity_from_path(String8 path, DF_EntityFromPathFlags flags)
{
  Temp scratch = scratch_begin(0, 0);
  PathStyle path_style = PathStyle_Relative;
  String8List path_parts = path_normalized_list_from_string(scratch.arena, path, &path_style);
  StringMatchFlags path_match_flags = path_match_flags_from_os(operating_system_from_context());
  
  //- rjf: pass 1: open parts, ignore overrides
  DF_Entity *file_no_override = &df_g_nil_entity;
  {
    DF_Entity *parent = df_entity_root();
    for(String8Node *path_part_n = path_parts.first;
        path_part_n != 0;
        path_part_n = path_part_n->next)
    {
      // rjf: find next child
      DF_Entity *next_parent = &df_g_nil_entity;
      for(DF_Entity *child = parent->first; !df_entity_is_nil(child); child = child->next)
      {
        B32 name_matches = str8_match(child->name, path_part_n->string, path_match_flags);
        if(name_matches && child->kind == DF_EntityKind_File)
        {
          next_parent = child;
          break;
        }
      }
      
      // rjf: no next -> allocate one
      if(df_entity_is_nil(next_parent))
      {
        if(flags & DF_EntityFromPathFlag_OpenAsNeeded)
        {
          String8 parent_path = df_full_path_from_entity(scratch.arena, parent);
          String8 path = push_str8f(scratch.arena, "%S%s%S", parent_path, parent_path.size != 0 ? "/" : "", path_part_n->string);
          FileProperties file_properties = os_properties_from_file_path(path);
          if(file_properties.created != 0 || flags & DF_EntityFromPathFlag_OpenMissing)
          {
            next_parent = df_entity_alloc(parent, DF_EntityKind_File);
            df_entity_equip_name(next_parent, path_part_n->string);
            next_parent->timestamp = file_properties.modified;
            next_parent->flags |= DF_EntityFlag_IsFolder * !!(file_properties.flags & FilePropertyFlag_IsFolder);
            next_parent->flags |= DF_EntityFlag_IsMissing * !!(file_properties.created == 0);
            if(path_part_n->next != 0)
            {
              next_parent->flags |= DF_EntityFlag_IsFolder;
            }
          }
        }
        else
        {
          parent = &df_g_nil_entity;
          break;
        }
      }
      
      // rjf: next parent -> follow it
      parent = next_parent;
    }
    file_no_override = (parent != df_entity_root() ? parent : &df_g_nil_entity);
  }
  
  //- rjf: pass 2: follow overrides
  DF_Entity *file_overrides_applied = &df_g_nil_entity;
  if(flags & DF_EntityFromPathFlag_AllowOverrides)
  {
    DF_Entity *parent = df_entity_root();
    for(String8Node *path_part_n = path_parts.first;
        path_part_n != 0;
        path_part_n = path_part_n->next)
    {
      // rjf: find next child
      DF_Entity *next_parent = &df_g_nil_entity;
      for(DF_Entity *child = parent->first; !df_entity_is_nil(child); child = child->next)
      {
        B32 name_matches = str8_match(child->name, path_part_n->string, path_match_flags);
        if(name_matches && child->kind == DF_EntityKind_File)
        {
          next_parent = child;
        }
      }
      
      // rjf: no next -> allocate one
      if(df_entity_is_nil(next_parent))
      {
        if(flags & DF_EntityFromPathFlag_OpenAsNeeded)
        {
          String8 parent_path = df_full_path_from_entity(scratch.arena, parent);
          String8 path = push_str8f(scratch.arena, "%S%s%S", parent_path, parent_path.size != 0 ? "/" : "", path_part_n->string);
          FileProperties file_properties = os_properties_from_file_path(path);
          if(file_properties.created != 0 || flags & DF_EntityFromPathFlag_OpenMissing)
          {
            next_parent = df_entity_alloc(parent, DF_EntityKind_File);
            df_entity_equip_name(next_parent, path_part_n->string);
            next_parent->timestamp = file_properties.modified;
            next_parent->flags |= DF_EntityFlag_IsFolder * !!(file_properties.flags & FilePropertyFlag_IsFolder);
            next_parent->flags |= DF_EntityFlag_IsMissing * !!(file_properties.created == 0);
            if(path_part_n->next != 0)
            {
              next_parent->flags |= DF_EntityFlag_IsFolder;
            }
          }
        }
        else
        {
          parent = &df_g_nil_entity;
          break;
        }
      }
      
      // rjf: next parent -> follow it
      parent = next_parent;
    }
    file_overrides_applied = (parent != df_entity_root() ? parent : &df_g_nil_entity);;
  }
  
  //- rjf: pick & return result
  DF_Entity *result = (flags & DF_EntityFromPathFlag_AllowOverrides) ? file_overrides_applied : file_no_override;
  if(flags & DF_EntityFromPathFlag_AllowOverrides &&
     result == file_overrides_applied &&
     result->flags & DF_EntityFlag_IsMissing)
  {
    result = file_no_override;
  }
  
  scratch_end(scratch);
  return result;
}

//- rjf: file path map override lookups

internal String8List
df_possible_overrides_from_file_path(Arena *arena, String8 file_path)
{
  // NOTE(rjf): This path, given some target file path, scans all file path map
  // overrides, and collects the set of file paths which could've redirected
  // to the target file path given the set of file path maps.
  //
  // For example, if I have a rule saying D:/devel/ maps to C:/devel/, and I
  // feed in C:/devel/foo/bar.txt, then this path will construct
  // D:/devel/foo/bar.txt, as a possible option.
  //
  // It will also preserve C:/devel/foo/bar.txt in the resultant list, so that
  // overrideless files still work through this path, and both redirected
  // files and non-redirected files can go through the same path.
  //
  String8List result = {0};
  str8_list_push(arena, &result, file_path);
  Temp scratch = scratch_begin(&arena, 1);
  PathStyle pth_style = PathStyle_Relative;
  String8List pth_parts = path_normalized_list_from_string(scratch.arena, file_path, &pth_style);
  {
    DF_EntityList links = df_query_cached_entity_list_with_kind(DF_EntityKind_FilePathMap);
    for(DF_EntityNode *n = links.first; n != 0; n = n->next)
    {
      //- rjf: unpack link
      DF_Entity *link = n->entity;
      DF_Entity *src = df_entity_child_from_kind(link, DF_EntityKind_Source);
      DF_Entity *dst = df_entity_child_from_kind(link, DF_EntityKind_Dest);
      PathStyle src_style = PathStyle_Relative;
      PathStyle dst_style = PathStyle_Relative;
      String8List src_parts = path_normalized_list_from_string(scratch.arena, src->name, &src_style);
      String8List dst_parts = path_normalized_list_from_string(scratch.arena, dst->name, &dst_style);
      
      //- rjf: determine if this link can possibly redirect to the target file path
      B32 dst_redirects_to_pth = 0;
      String8Node *non_redirected_pth_first = 0;
      if(dst_style == pth_style && dst_parts.first != 0 && pth_parts.first != 0)
      {
        dst_redirects_to_pth = 1;
        String8Node *dst_n = dst_parts.first;
        String8Node *pth_n = pth_parts.first;
        for(;dst_n != 0 && pth_n != 0; dst_n = dst_n->next, pth_n = pth_n->next)
        {
          if(!str8_match(dst_n->string, pth_n->string, StringMatchFlag_CaseInsensitive))
          {
            dst_redirects_to_pth = 0;
            break;
          }
          non_redirected_pth_first = pth_n->next;
        }
      }
      
      //- rjf: if this link can redirect to this path via `src` -> `dst`, compute
      // possible full source path, by taking `src` and appending non-redirected
      // suffix (which did not show up in `dst`)
      if(dst_redirects_to_pth)
      {
        String8List candidate_parts = src_parts;
        for(String8Node *p = non_redirected_pth_first; p != 0; p = p->next)
        {
          str8_list_push(scratch.arena, &candidate_parts, p->string);
        }
        StringJoin join = {0};
        join.sep = str8_lit("/");
        String8 candidate_path = str8_list_join(arena, &candidate_parts, 0);
        str8_list_push(arena, &result, candidate_path);
      }
    }
  }
  scratch_end(scratch);
  return result;
}

//- rjf: top-level state queries

internal DF_Entity *
df_entity_root(void)
{
  return df_state->entities_root;
}

internal DF_EntityList
df_push_entity_list_with_kind(Arena *arena, DF_EntityKind kind)
{
  ProfBeginFunction();
  DF_EntityList result = {0};
  for(DF_Entity *entity = df_state->entities_root;
      !df_entity_is_nil(entity);
      entity = df_entity_rec_df_pre(entity, &df_g_nil_entity).next)
  {
    if(entity->kind == kind)
    {
      df_entity_list_push(arena, &result, entity);
    }
  }
  ProfEnd();
  return result;
}

internal DF_Entity *
df_entity_from_id(DF_EntityID id)
{
  DF_Entity *result = &df_g_nil_entity;
  for(DF_Entity *e = df_entity_root();
      !df_entity_is_nil(e);
      e = df_entity_rec_df_pre(e, &df_g_nil_entity).next)
  {
    if(e->id == id)
    {
      result = e;
      break;
    }
  }
  return result;
}

internal DF_Entity *
df_machine_entity_from_machine_id(CTRL_MachineID machine_id)
{
  DF_Entity *result = &df_g_nil_entity;
  for(DF_Entity *e = df_entity_root();
      !df_entity_is_nil(e);
      e = df_entity_rec_df_pre(e, &df_g_nil_entity).next)
  {
    if(e->kind == DF_EntityKind_Machine && e->ctrl_machine_id == machine_id)
    {
      result = e;
      break;
    }
  }
  if(df_entity_is_nil(result))
  {
    result = df_entity_alloc(df_entity_root(), DF_EntityKind_Machine);
    df_entity_equip_ctrl_machine_id(result, machine_id);
  }
  return result;
}

internal DF_Entity *
df_entity_from_ctrl_handle(CTRL_MachineID machine_id, DMN_Handle handle)
{
  DF_Entity *result = &df_g_nil_entity;
  if(handle.u64[0] != 0)
  {
    for(DF_Entity *e = df_entity_root();
        !df_entity_is_nil(e);
        e = df_entity_rec_df_pre(e, &df_g_nil_entity).next)
    {
      if(e->flags & DF_EntityFlag_HasCtrlMachineID &&
         e->flags & DF_EntityFlag_HasCtrlHandle &&
         e->ctrl_machine_id == machine_id &&
         MemoryMatchStruct(&e->ctrl_handle, &handle))
      {
        result = e;
        break;
      }
    }
  }
  return result;
}

internal DF_Entity *
df_entity_from_ctrl_id(CTRL_MachineID machine_id, U32 id)
{
  DF_Entity *result = &df_g_nil_entity;
  if(id != 0)
  {
    for(DF_Entity *e = df_entity_root();
        !df_entity_is_nil(e);
        e = df_entity_rec_df_pre(e, &df_g_nil_entity).next)
    {
      if(e->flags & DF_EntityFlag_HasCtrlMachineID &&
         e->flags & DF_EntityFlag_HasCtrlID &&
         e->ctrl_machine_id == machine_id &&
         e->ctrl_id == id)
      {
        result = e;
        break;
      }
    }
  }
  return result;
}

internal DF_Entity *
df_entity_from_name_and_kind(String8 string, DF_EntityKind kind)
{
  DF_Entity *result = &df_g_nil_entity;
  DF_EntityList all_of_this_kind = df_query_cached_entity_list_with_kind(kind);
  for(DF_EntityNode *n = all_of_this_kind.first; n != 0; n = n->next)
  {
    if(str8_match(n->entity->name, string, 0))
    {
      result = n->entity;
      break;
    }
  }
  return result;
}

internal DF_Entity *
df_entity_from_u64_and_kind(U64 u64, DF_EntityKind kind)
{
  DF_Entity *result = &df_g_nil_entity;
  DF_EntityList all_of_this_kind = df_query_cached_entity_list_with_kind(kind);
  for(DF_EntityNode *n = all_of_this_kind.first; n != 0; n = n->next)
  {
    if(n->entity->u64 == u64)
    {
      result = n->entity;
      break;
    }
  }
  return result;
}

//- rjf: entity freezing state

internal void
df_set_thread_freeze_state(DF_Entity *thread, B32 frozen)
{
  DF_Handle thread_handle = df_handle_from_entity(thread);
  DF_HandleNode *already_frozen_node = df_handle_list_find(&df_state->frozen_threads, thread_handle);
  B32 is_frozen = !!already_frozen_node;
  B32 should_be_frozen = frozen;
  
  // rjf: not frozen => frozen
  if(!is_frozen && should_be_frozen)
  {
    DF_HandleNode *node = df_state->free_handle_node;
    if(node)
    {
      SLLStackPop(df_state->free_handle_node);
    }
    else
    {
      node = push_array(df_state->arena, DF_HandleNode, 1);
    }
    node->handle = thread_handle;
    df_handle_list_push_node(&df_state->frozen_threads, node);
    df_state->entities_mut_soft_halt = 1;
  }
  
  // rjf: frozen => not frozen
  if(is_frozen && !should_be_frozen)
  {
    df_state->entities_mut_soft_halt = 1;
    df_handle_list_remove(&df_state->frozen_threads, already_frozen_node);
    SLLStackPush(df_state->free_handle_node, already_frozen_node);
  }
  
  df_entity_notify_mutation(thread);
}

internal B32
df_entity_is_frozen(DF_Entity *entity)
{
  B32 is_frozen = !df_entity_is_nil(entity);
  for(DF_Entity *e = entity; !df_entity_is_nil(e); e = df_entity_rec_df_pre(e, entity).next)
  {
    if(e->kind == DF_EntityKind_Thread)
    {
      B32 thread_is_frozen = !!df_handle_list_find(&df_state->frozen_threads, df_handle_from_entity(e));
      if(!thread_is_frozen)
      {
        is_frozen = 0;
        break;
      }
    }
  }
  return is_frozen;
}

////////////////////////////////
//~ rjf: Command Stateful Functions

internal void
df_register_cmd_specs(DF_CmdSpecInfoArray specs)
{
  U64 registrar_idx = df_state->total_registrar_count;
  df_state->total_registrar_count += 1;
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    // rjf: extract info from array slot
    DF_CmdSpecInfo *info = &specs.v[idx];
    
    // rjf: skip empties
    if(info->string.size == 0)
    {
      continue;
    }
    
    // rjf: determine hash/slot
    U64 hash = df_hash_from_string(info->string);
    U64 slot = hash % df_state->cmd_spec_table_size;
    
    // rjf: allocate node & push
    DF_CmdSpec *spec = push_array(df_state->arena, DF_CmdSpec, 1);
    SLLStackPush_N(df_state->cmd_spec_table[slot], spec, hash_next);
    
    // rjf: fill node
    DF_CmdSpecInfo *info_copy = &spec->info;
    info_copy->string                 = push_str8_copy(df_state->arena, info->string);
    info_copy->description            = push_str8_copy(df_state->arena, info->description);
    info_copy->search_tags            = push_str8_copy(df_state->arena, info->search_tags);
    info_copy->display_name           = push_str8_copy(df_state->arena, info->display_name);
    info_copy->flags                  = info->flags;
    info_copy->query                  = info->query;
    info_copy->canonical_icon_kind    = info->canonical_icon_kind;
    spec->registrar_index = registrar_idx;
    spec->ordering_index = idx;
  }
}

internal DF_CmdSpec *
df_cmd_spec_from_string(String8 string)
{
  DF_CmdSpec *result = &df_g_nil_cmd_spec;
  {
    U64 hash = df_hash_from_string(string);
    U64 slot = hash%df_state->cmd_spec_table_size;
    for(DF_CmdSpec *n = df_state->cmd_spec_table[slot]; n != 0; n = n->hash_next)
    {
      if(str8_match(n->info.string, string, 0))
      {
        result = n;
        break;
      }
    }
  }
  return result;
}

internal DF_CmdSpec *
df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind core_cmd_kind)
{
  String8 string = df_g_core_cmd_kind_spec_info_table[core_cmd_kind].string;
  DF_CmdSpec *result = df_cmd_spec_from_string(string);
  return result;
}

internal void
df_cmd_spec_counter_inc(DF_CmdSpec *spec)
{
  if(!df_cmd_spec_is_nil(spec))
  {
    spec->run_count += 1;
  }
}

internal DF_CmdSpecList
df_push_cmd_spec_list(Arena *arena)
{
  DF_CmdSpecList list = {0};
  for(U64 idx = 0; idx < df_state->cmd_spec_table_size; idx += 1)
  {
    for(DF_CmdSpec *spec = df_state->cmd_spec_table[idx]; spec != 0; spec = spec->hash_next)
    {
      df_cmd_spec_list_push(arena, &list, spec);
    }
  }
  return list;
}

////////////////////////////////
//~ rjf: View Rule Spec Stateful Functions

internal void
df_register_core_view_rule_specs(DF_CoreViewRuleSpecInfoArray specs)
{
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    // rjf: extract info from array slot
    DF_CoreViewRuleSpecInfo *info = &specs.v[idx];
    
    // rjf: skip empties
    if(info->string.size == 0)
    {
      continue;
    }
    
    // rjf: determine hash/slot
    U64 hash = df_hash_from_string(info->string);
    U64 slot_idx = hash%df_state->view_rule_spec_table_size;
    
    // rjf: allocate node & push
    DF_CoreViewRuleSpec *spec = push_array(df_state->arena, DF_CoreViewRuleSpec, 1);
    SLLStackPush_N(df_state->view_rule_spec_table[slot_idx], spec, hash_next);
    
    // rjf: fill node
    DF_CoreViewRuleSpecInfo *info_copy = &spec->info;
    MemoryCopyStruct(info_copy, info);
    info_copy->string         = push_str8_copy(df_state->arena, info->string);
    info_copy->display_string = push_str8_copy(df_state->arena, info->display_string);
    info_copy->description    = push_str8_copy(df_state->arena, info->description);
  }
}

internal DF_CoreViewRuleSpec *
df_core_view_rule_spec_from_string(String8 string)
{
  DF_CoreViewRuleSpec *spec = &df_g_nil_core_view_rule_spec;
  {
    U64 hash = df_hash_from_string(string);
    U64 slot_idx = hash%df_state->view_rule_spec_table_size;
    for(DF_CoreViewRuleSpec *s = df_state->view_rule_spec_table[slot_idx]; s != 0; s = s->hash_next)
    {
      if(str8_match(string, s->info.string, 0))
      {
        spec = s;
        break;
      }
    }
  }
  return spec;
}

////////////////////////////////
//~ rjf: Stepping "Trap Net" Builders

// NOTE(rjf): Stepping Algorithm Overview (2024/01/17)
//
// The basic idea behind all stepping algorithms in the debugger are setting up
// a "trap net". A "trap net" is just a collection of high-level traps that are
// meant to "catch" a thread after letting it run. This trap net is submitted
// when the debugger frontend sends a "run" command (it is just empty if doing
// an actual 'run' or 'continue'). The debugger control thread then uses this
// trap net to program a state machine, to appropriately respond to a variety
// of debug events which it is passed from the OS.
//
// These are "high-level traps" because they can have specific behavioral info
// attached to them. These are encoded via the `CTRL_TrapFlags` type, which
// allow expression of the following behaviors:
//
//  - end-stepping: when this trap is hit, it will end the stepping operation,
//      and the target will not continue.
//  - ignore-stack-pointer-check: when a trap in the trap net is hit, it will
//      by-default be ignored if the thread's stack pointer has changed. this
//      flag disables that behavior, for when the stack pointer is expected to
//      change (e.g. step-out).
//  - single-step-after-hit: when a trap with this flag is hit, the debugger
//      will immediately single-step the thread which hit it.
//  - save-stack-pointer: when a trap with this flag is hit, it will rewrite
//      the stack pointer which is used to compare against, when deciding
//      whether or not to filter a trap (based on stack pointer changes).
//  - begin-spoof-mode: this enables "spoof mode". "spoof mode" is a special
//      mode that disables the trap net entirely, and lets the thread run
//      freely - but it catches the thread not with a trap, but a false return
//      address. the debugger will overwrite a specific return address on the
//      stack. this address will be overwritten with an address which does NOT
//      point to a valid page, such that when the thread returns out of a
//      particular call frame, the debugger will receive a debug event, at
//      which point it can move the thread back to the correct return address,
//      and resume with the trap net enabled. this is used in "step over"
//      operations, because it avoids target <-> debugger "roundtrips" (e.g.
//      target being stopped, debugger being called with debug events, then
//      target resumes when debugger's control thread is done running) for
//      recursions. (it doesn't make a difference with non-recursive calls,
//      but the debugger can't detect the difference).
//
// Each stepping command prepares its trap net differently.
//
// --- Instruction Step Into --------------------------------------------------
// In this case, no trap net is prepared, and only a low-level single-step is
// performed.
//
// --- Instruction Step Over --------------------------------------------------
// To build a trap net for an instruction-level step-over, the next instruction
// at the thread's current instruction pointer is decoded. If it is a call
// instruction, or if it is a repeating instruction, then a trap with the
// 'end-stepping' behavior is placed at the instruction immediately following
// the 'call' instruction.
//
// --- Line Step Into ---------------------------------------------------------
// For a source-line step-into, the thread's instruction pointer is first used
// to look up into the debug info's line info, to find the machine code in the
// thread's current source line. Every instruction in this range is decoded.
// Traps are then built in the following way:
//
// - 'call' instruction -> if can decode call destination address, place
//     "end-stepping | ignore-stack-pointer-check" trap at destination. if
//     can't, "end-stepping | single-step-after | ignore-stack-pointer-check"
//     trap at call.
// - 'jmp' (both unconditional & conditional) -> if can decode jump destination
//     address, AND if jump leaves the line, place "end-stepping | ignore-
//     stack-pointer-check" trap at destination. if can't, "end-stepping |
//     single-step-after | ignore-stack-pointer-check" trap at jmp. if jump
//     stays within the line, do nothing.
// - 'return' -> place "end-stepping | single-step-after" trap at return inst.
// - "end-stepping" trap is placed at the first address after the line, to
//     catch all steps which simply proceed linearly through the instruction
//     stream.
//
// --- Line Step Over ---------------------------------------------------------
// For a source-line step-over, the thread's instruction pointer is first used
// to look up into the debug info's line info, to find the machine code in the
// thread's current source line. Every instruction in this range is decoded.
// Traps are then built in the following way:
//
// - 'call' instruction -> place "single-step-after | begin-spoof-mode" trap at
//     call instruction.
// - 'jmp' (both unconditional & conditional) -> if can decode jump destination
//     address, AND if jump leaves the line, place "end-stepping" trap at
//     destination. if can't, "end-stepping | single-step-after" trap at jmp.
//     if jump stays within the line, do nothing.
// - 'return' -> place "end-stepping | single-step-after" trap at return inst.
// - "end-stepping" trap is placed at the first address after the line, to
//     catch all steps which simply proceed linearly through the instruction
//     stream.
// - for any instructions which may change the stack pointer, traps are placed
//     at them with the "save-stack-pointer | single-step-after" behaviors.

internal CTRL_TrapList
df_trap_net_from_thread__step_over_inst(Arena *arena, DF_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => unpacked info
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  Architecture arch = df_architecture_from_entity(thread);
  U64 ip_vaddr = ctrl_query_cached_rip_from_thread(df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  
  // rjf: ip => machine code
  String8 machine_code = {0};
  {
    Rng1U64 rng = r1u64(ip_vaddr, ip_vaddr+max_instruction_size_from_arch(arch));
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, rng, os_now_microseconds()+5000);
    machine_code = machine_code_slice.data;
  }
  
  // rjf: build traps if machine code was read successfully
  if(machine_code.size != 0)
  {
    // rjf: decode instruction
    DASM_Inst inst = dasm_inst_from_code(scratch.arena, arch, ip_vaddr, machine_code, DASM_Syntax_Intel);
    
    // rjf: call => run until call returns
    if(inst.flags & DASM_InstFlag_Call || inst.flags & DASM_InstFlag_Repeats)
    {
      CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, ip_vaddr+inst.size};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal CTRL_TrapList
df_trap_net_from_thread__step_over_line(Arena *arena, DF_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  log_infof("step_over_line:\n{\n");
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_Entity *module = df_module_from_thread(thread);
  DI_Key dbgi_key = df_dbgi_key_from_module(module);
  Architecture arch = df_architecture_from_entity(thread);
  U64 ip_vaddr = ctrl_query_cached_rip_from_thread(df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  log_infof("ip_vaddr: 0x%I64x\n", ip_vaddr);
  log_infof("dbgi_key: {%S, 0x%I64x}\n", dbgi_key.path, dbgi_key.min_timestamp);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = df_voff_from_vaddr(module, ip_vaddr);
    DF_LineList lines = df_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, ip_voff);
    Rng1U64 line_voff_rng = {0};
    if(lines.first != 0)
    {
      line_voff_rng = lines.first->v.voff_range;
      line_vaddr_rng = df_vaddr_range_from_voff_range(module, line_voff_rng);
      log_infof("line: {%S:%I64i}\n", lines.first->v.file_path, lines.first->v.pt.line);
    }
    log_infof("voff_range: {0x%I64x, 0x%I64x}\n", line_voff_rng.min, line_voff_rng.max);
    log_infof("vaddr_range: {0x%I64x, 0x%I64x}\n", line_vaddr_rng.min, line_vaddr_rng.max);
  }
  
  // rjf: opl line_vaddr_rng -> 0xf00f00 or 0xfeefee? => include in line vaddr range
  //
  // MSVC exports line info at these line numbers when /JMC (Just My Code) debugging
  // is enabled. This is enabled by default normally.
  {
    U64 opl_line_voff_rng = df_voff_from_vaddr(module, line_vaddr_rng.max);
    DF_LineList lines = df_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, opl_line_voff_rng);
    if(lines.first != 0 && (lines.first->v.pt.line == 0xf00f00 || lines.first->v.pt.line == 0xfeefee))
    {
      line_vaddr_rng.max = df_vaddr_from_voff(module, lines.first->v.voff_range.max);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  if(good_line_info)
  {
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, line_vaddr_rng, os_now_microseconds()+50000);
    machine_code = machine_code_slice.data;
    LogInfoNamedBlockF("machine_code_slice")
    {
      log_infof("stale: %i\n", machine_code_slice.stale);
      log_infof("any_byte_bad: %i\n", machine_code_slice.any_byte_bad);
      log_infof("any_byte_changed: %i\n", machine_code_slice.any_byte_changed);
      log_infof("bytes:\n[\n");
      for(U64 idx = 0; idx < machine_code_slice.data.size; idx += 1)
      {
        log_infof("0x%x,", machine_code_slice.data.str[idx]);
        if(idx%16 == 15 || idx+1 == machine_code_slice.data.size)
        {
          log_infof("\n");
        }
      }
      log_infof("]\n");
    }
  }
  
  // rjf: machine code => ctrl flow analysis
  DF_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_line_info)
  {
    ctrl_flow_info = df_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                            DASM_InstFlag_Call|
                                                            DASM_InstFlag_Branch|
                                                            DASM_InstFlag_UnconditionalJump|
                                                            DASM_InstFlag_ChangesStackPointer|
                                                            DASM_InstFlag_Return,
                                                            arch,
                                                            line_vaddr_rng.min,
                                                            machine_code);
    LogInfoNamedBlockF("ctrl_flow_info")
    {
      log_infof("flags: %x\n", ctrl_flow_info.flags);
      LogInfoNamedBlockF("exit_points") for(DF_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
      {
        log_infof("{vaddr:0x%I64x, jump_dest_vaddr:0x%I64x, inst_flags:%x}\n", n->v.vaddr, n->v.jump_dest_vaddr, n->v.inst_flags);
      }
    }
  }
  
  // rjf: push traps for all exit points
  if(good_line_info) for(DF_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DF_CtrlFlowPoint *point = &n->v;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DASM_InstFlag_Branch|
                            DASM_InstFlag_UnconditionalJump|
                            DASM_InstFlag_Return))
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_EndStepping);
      
      // rjf: omit if this jump stays inside of this line
      if(contains_1u64(line_vaddr_rng, point->jump_dest_vaddr))
      {
        add = 0;
      }
      
      // rjf: trap @ destination, if we can - we can avoid a single-step this way.
      if(point->jump_dest_vaddr != 0)
      {
        trap_addr = point->jump_dest_vaddr;
        flags &= ~CTRL_TrapFlag_SingleStepAfterHit;
      }
      
    }
    
    // rjf: call => place spoof at return spot in stack, single-step after hitting
    else if(point->inst_flags & DASM_InstFlag_Call)
    {
      flags |= (CTRL_TrapFlag_BeginSpoofMode|CTRL_TrapFlag_SingleStepAfterHit);
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DASM_InstFlag_ChangesStackPointer)
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_SaveStackPointer);
    }
    
    // rjf: add if appropriate
    if(add)
    {
      CTRL_Trap trap = {flags, trap_addr};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  // rjf: push trap for natural linear flow
  if(good_line_info)
  {
    CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, line_vaddr_rng.max};
    ctrl_trap_list_push(arena, &result, &trap);
  }
  
  // rjf: log
  LogInfoNamedBlockF("traps") for(CTRL_TrapNode *n = result.first; n != 0; n = n->next)
  {
    log_infof("{flags:0x%x, vaddr:0x%I64x}\n", n->v.flags, n->v.vaddr);
  }
  
  scratch_end(scratch);
  log_infof("}\n\n");
  return result;
}

internal CTRL_TrapList
df_trap_net_from_thread__step_into_line(Arena *arena, DF_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_Entity *module = df_module_from_thread(thread);
  DI_Key dbgi_key = df_dbgi_key_from_module(module);
  Architecture arch = df_architecture_from_entity(thread);
  U64 ip_vaddr = ctrl_query_cached_rip_from_thread(df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = df_voff_from_vaddr(module, ip_vaddr);
    DF_LineList lines = df_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, ip_voff);
    Rng1U64 line_voff_rng = {0};
    if(lines.first != 0)
    {
      line_voff_rng = lines.first->v.voff_range;
      line_vaddr_rng = df_vaddr_range_from_voff_range(module, line_voff_rng);
    }
  }
  
  // rjf: opl line_vaddr_rng -> 0xf00f00 or 0xfeefee? => include in line vaddr range
  //
  // MSVC exports line info at these line numbers when /JMC (Just My Code) debugging
  // is enabled. This is enabled by default normally.
  {
    U64 opl_line_voff_rng = df_voff_from_vaddr(module, line_vaddr_rng.max);
    DF_LineList lines = df_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, opl_line_voff_rng);
    if(lines.first != 0 && (lines.first->v.pt.line == 0xf00f00 || lines.first->v.pt.line == 0xfeefee))
    {
      line_vaddr_rng.max = df_vaddr_from_voff(module, lines.first->v.voff_range.max);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  if(good_line_info)
  {
    CTRL_ProcessMemorySlice machine_code_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, line_vaddr_rng, os_now_microseconds()+5000);
    machine_code = machine_code_slice.data;
  }
  
  // rjf: machine code => ctrl flow analysis
  DF_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_line_info)
  {
    ctrl_flow_info = df_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                            DASM_InstFlag_Call|
                                                            DASM_InstFlag_Branch|
                                                            DASM_InstFlag_UnconditionalJump|
                                                            DASM_InstFlag_ChangesStackPointer|
                                                            DASM_InstFlag_Return,
                                                            arch,
                                                            line_vaddr_rng.min,
                                                            machine_code);
  }
  
  // rjf: push traps for all exit points
  if(good_line_info) for(DF_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DF_CtrlFlowPoint *point = &n->v;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DASM_InstFlag_Call|
                            DASM_InstFlag_Branch|
                            DASM_InstFlag_UnconditionalJump|
                            DASM_InstFlag_Return))
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck);
      
      // rjf: omit if this jump stays inside of this line
      if(contains_1u64(line_vaddr_rng, point->jump_dest_vaddr))
      {
        add = 0;
      }
      
      // rjf: trap @ destination, if we can - we can avoid a single-step this way.
      if(point->jump_dest_vaddr != 0)
      {
        trap_addr = point->jump_dest_vaddr;
        flags &= ~CTRL_TrapFlag_SingleStepAfterHit;
      }
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DASM_InstFlag_ChangesStackPointer)
    {
      flags |= (CTRL_TrapFlag_SingleStepAfterHit|CTRL_TrapFlag_SaveStackPointer);
    }
    
    // rjf: add if appropriate
    if(add)
    {
      CTRL_Trap trap = {flags, trap_addr};
      ctrl_trap_list_push(arena, &result, &trap);
    }
  }
  
  // rjf: push trap for natural linear flow
  if(good_line_info)
  {
    CTRL_Trap trap = {CTRL_TrapFlag_EndStepping, line_vaddr_rng.max};
    ctrl_trap_list_push(arena, &result, &trap);
  }
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Modules & Debug Info Mappings

//- rjf: module <=> debug info keys

internal DI_Key
df_dbgi_key_from_module(DF_Entity *module)
{
  DF_Entity *debug_info_path = df_entity_child_from_kind(module, DF_EntityKind_DebugInfoPath);
  DI_Key key = {debug_info_path->name, debug_info_path->timestamp};
  return key;
}

internal DF_EntityList
df_modules_from_dbgi_key(Arena *arena, DI_Key *dbgi_key)
{
  DF_EntityList list = {0};
  DF_EntityList all_modules = df_query_cached_entity_list_with_kind(DF_EntityKind_Module);
  for(DF_EntityNode *n = all_modules.first; n != 0; n = n->next)
  {
    DF_Entity *module = n->entity;
    DI_Key module_dbgi_key = df_dbgi_key_from_module(module);
    if(di_key_match(&module_dbgi_key, dbgi_key))
    {
      df_entity_list_push(arena, &list, module);
    }
  }
  return list;
}

//- rjf: voff <=> vaddr

internal U64
df_base_vaddr_from_module(DF_Entity *module)
{
  U64 module_base_vaddr = module->vaddr;
  return module_base_vaddr;
}

internal U64
df_voff_from_vaddr(DF_Entity *module, U64 vaddr)
{
  U64 module_base_vaddr = df_base_vaddr_from_module(module);
  U64 voff = vaddr - module_base_vaddr;
  return voff;
}

internal U64
df_vaddr_from_voff(DF_Entity *module, U64 voff)
{
  U64 module_base_vaddr = df_base_vaddr_from_module(module);
  U64 vaddr = voff + module_base_vaddr;
  return vaddr;
}

internal Rng1U64
df_voff_range_from_vaddr_range(DF_Entity *module, Rng1U64 vaddr_rng)
{
  U64 rng_size = dim_1u64(vaddr_rng);
  Rng1U64 voff_rng = {0};
  voff_rng.min = df_voff_from_vaddr(module, vaddr_rng.min);
  voff_rng.max = voff_rng.min + rng_size;
  return voff_rng;
}

internal Rng1U64
df_vaddr_range_from_voff_range(DF_Entity *module, Rng1U64 voff_rng)
{
  U64 rng_size = dim_1u64(voff_rng);
  Rng1U64 vaddr_rng = {0};
  vaddr_rng.min = df_vaddr_from_voff(module, voff_rng.min);
  vaddr_rng.max = vaddr_rng.min + rng_size;
  return vaddr_rng;
}

////////////////////////////////
//~ rjf: Debug Info Lookups

//- rjf: symbol lookups

internal String8
df_symbol_name_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff, B32 decorated)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    DI_Scope *scope = di_scope_open();
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
    if(result.size == 0)
    {
      U64 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, voff);
      RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
      U64 proc_idx = scope->proc_idx;
      RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, proc_idx);
      E_TypeKey type = e_type_key_ext(E_TypeKind_Function, procedure->type_idx, e_parse_ctx_module_idx_from_rdi(rdi));
      String8 name = {0};
      name.str = rdi_string_from_idx(rdi, procedure->name_string_idx, &name.size);
      if(decorated && procedure->type_idx != 0)
      {
        String8List list = {0};
        e_type_lhs_string_from_key(scratch.arena, type, &list, 0, 0);
        str8_list_push(scratch.arena, &list, name);
        e_type_rhs_string_from_key(scratch.arena, type, &list, 0);
        result = str8_list_join(arena, &list, 0);
      }
      else
      {
        result = push_str8_copy(arena, name);
      }
    }
    if(result.size == 0)
    {
      U64 global_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_GlobalVMap, voff);
      RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, global_idx);
      U64 name_size = 0;
      U8 *name_ptr = rdi_string_from_idx(rdi, global_var->name_string_idx, &name_size);
      result = push_str8_copy(arena, str8(name_ptr, name_size));
    }
    di_scope_close(scope);
    scratch_end(scratch);
  }
  return result;
}

internal String8
df_symbol_name_from_process_vaddr(Arena *arena, DF_Entity *process, U64 vaddr, B32 decorated)
{
  ProfBeginFunction();
  String8 result = {0};
  {
    DF_Entity *module = df_module_from_process_vaddr(process, vaddr);
    DI_Key dbgi_key = df_dbgi_key_from_module(module);
    U64 voff = df_voff_from_vaddr(module, vaddr);
    result = df_symbol_name_from_dbgi_key_voff(arena, &dbgi_key, voff, decorated);
  }
  ProfEnd();
  return result;
}

//- rjf: symbol -> voff lookups

internal U64
df_voff_from_dbgi_key_symbol_name(DI_Key *dbgi_key, String8 symbol_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DI_Scope *scope = di_scope_open();
  U64 result = 0;
  {
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
    RDI_NameMapKind name_map_kinds[] =
    {
      RDI_NameMapKind_GlobalVariables,
      RDI_NameMapKind_Procedures,
    };
    if(rdi != &di_rdi_parsed_nil)
    {
      for(U64 name_map_kind_idx = 0;
          name_map_kind_idx < ArrayCount(name_map_kinds);
          name_map_kind_idx += 1)
      {
        RDI_NameMapKind name_map_kind = name_map_kinds[name_map_kind_idx];
        RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, name_map_kind);
        RDI_ParsedNameMap parsed_name_map = {0};
        rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, symbol_name.str, symbol_name.size);
        
        // rjf: node -> num
        U64 entity_num = 0;
        if(node != 0)
        {
          switch(node->match_count)
          {
            case 1:
            {
              entity_num = node->match_idx_or_idx_run_first + 1;
            }break;
            default:
            {
              U32 num = 0;
              U32 *run = rdi_matches_from_map_node(rdi, node, &num);
              if(num != 0)
              {
                entity_num = run[0]+1;
              }
            }break;
          }
        }
        
        // rjf: num -> voff
        U64 voff = 0;
        if(entity_num != 0) switch(name_map_kind)
        {
          default:{}break;
          case RDI_NameMapKind_GlobalVariables:
          {
            RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, entity_num-1);
            voff = global_var->voff;
          }break;
          case RDI_NameMapKind_Procedures:
          {
            RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, entity_num-1);
            RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, procedure->root_scope_idx);
            voff = *rdi_element_from_name_idx(rdi, ScopeVOffData, scope->voff_range_first);
          }break;
        }
        
        // rjf: nonzero voff -> break
        if(voff != 0)
        {
          result = voff;
          break;
        }
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal U64
df_type_num_from_dbgi_key_name(DI_Key *dbgi_key, String8 name)
{
  ProfBeginFunction();
  DI_Scope *scope = di_scope_open();
  U64 result = 0;
  {
    RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
    RDI_NameMap *name_map = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_Types);
    RDI_ParsedNameMap parsed_name_map = {0};
    rdi_parsed_from_name_map(rdi, name_map, &parsed_name_map);
    RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &parsed_name_map, name.str, name.size);
    U64 entity_num = 0;
    if(node != 0)
    {
      switch(node->match_count)
      {
        case 1:
        {
          entity_num = node->match_idx_or_idx_run_first + 1;
        }break;
        default:
        {
          U32 num = 0;
          U32 *run = rdi_matches_from_map_node(rdi, node, &num);
          if(num != 0)
          {
            entity_num = run[0]+1;
          }
        }break;
      }
    }
    result = entity_num;
  }
  di_scope_close(scope);
  ProfEnd();
  return result;
}

//- rjf: voff -> line info

internal DF_LineList
df_lines_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  DF_LineList result = {0};
  {
    //- rjf: gather line tables
    typedef struct LineTableNode LineTableNode;
    struct LineTableNode
    {
      LineTableNode *next;
      RDI_ParsedLineTable parsed_line_table;
    };
    LineTableNode start_line_table = {0};
    RDI_Unit *unit = rdi_unit_from_voff(rdi, voff);
    RDI_LineTable *unit_line_table = rdi_line_table_from_unit(rdi, unit);
    rdi_parsed_from_line_table(rdi, unit_line_table, &start_line_table.parsed_line_table);
    LineTableNode *top_line_table = 0;
    RDI_Scope *scope = rdi_scope_from_voff(rdi, voff);
    {
      for(RDI_Scope *s = scope;
          s->inline_site_idx != 0;
          s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
      {
        RDI_InlineSite *inline_site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
        if(inline_site->line_table_idx != 0)
        {
          LineTableNode *n = push_array(scratch.arena, LineTableNode, 1);
          SLLStackPush(top_line_table, n);
          RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, inline_site->line_table_idx);
          rdi_parsed_from_line_table(rdi, line_table, &n->parsed_line_table);
        }
      }
    }
    SLLStackPush(top_line_table, &start_line_table);
    
    //- rjf: gather lines in each line table
    Rng1U64 shallowest_voff_range = {0};
    for(LineTableNode *line_table_n = top_line_table; line_table_n != 0; line_table_n = line_table_n->next)
    {
      RDI_ParsedLineTable parsed_line_table = line_table_n->parsed_line_table;
      U64 line_info_idx = rdi_line_info_idx_from_voff(&parsed_line_table, voff);
      if(line_info_idx < parsed_line_table.count)
      {
        RDI_Line *line = &parsed_line_table.lines[line_info_idx];
        RDI_Column *column = (line_info_idx < parsed_line_table.col_count) ? &parsed_line_table.cols[line_info_idx] : 0;
        RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
        String8List path_parts = {0};
        for(RDI_FilePathNode *fpn = rdi_element_from_name_idx(rdi, FilePathNodes, file->file_path_node_idx);
            fpn != rdi_element_from_name_idx(rdi, FilePathNodes, 0);
            fpn = rdi_element_from_name_idx(rdi, FilePathNodes, fpn->parent_path_node))
        {
          String8 path_part = {0};
          path_part.str = rdi_string_from_idx(rdi, fpn->name_string_idx, &path_part.size);
          str8_list_push_front(scratch.arena, &path_parts, path_part);
        }
        StringJoin join = {0};
        join.sep = str8_lit("/");
        String8 file_normalized_full_path = str8_list_join(arena, &path_parts, &join);
        DF_LineNode *n = push_array(arena, DF_LineNode, 1);
        SLLQueuePush(result.first, result.last, n);
        result.count += 1;
        if(line->file_idx != 0 && file_normalized_full_path.size != 0)
        {
          n->v.file_path = file_normalized_full_path;
        }
        n->v.pt = txt_pt(line->line_num, column ? column->col_first : 1);
        n->v.voff_range = r1u64(parsed_line_table.voffs[line_info_idx], parsed_line_table.voffs[line_info_idx+1]);
        n->v.dbgi_key = *dbgi_key;
        if(line_table_n == top_line_table)
        {
          shallowest_voff_range = n->v.voff_range;
        }
      }
    }
    
    //- rjf: clamp all lines from all tables by shallowest (most unwound) range
    for(DF_LineNode *n = result.first; n != 0; n = n->next)
    {
      n->v.voff_range = intersect_1u64(n->v.voff_range, shallowest_voff_range);
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return result;
}

//- rjf: file:line -> line info

internal DF_LineListArray
df_lines_array_from_file_path_line_range(Arena *arena, String8 file_path, Rng1S64 line_num_range)
{
  DF_LineListArray array = {0};
  {
    array.count = dim_1s64(line_num_range)+1;
    array.v = push_array(arena, DF_LineList, array.count);
  }
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  DI_KeyList dbgi_keys = df_push_active_dbgi_key_list(scratch.arena);
  String8List overrides = df_possible_overrides_from_file_path(scratch.arena, file_path);
  for(String8Node *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    String8 file_path = override_n->string;
    String8 file_path_normalized = lower_from_str8(scratch.arena, file_path);
    for(DI_KeyNode *dbgi_key_n = dbgi_keys.first;
        dbgi_key_n != 0;
        dbgi_key_n = dbgi_key_n->next)
    {
      // rjf: binary -> rdi
      DI_Key key = dbgi_key_n->v;
      RDI_Parsed *rdi = di_rdi_from_key(scope, &key, 0);
      
      // rjf: file_path_normalized * rdi -> src_id
      B32 good_src_id = 0;
      U32 src_id = 0;
      if(rdi != &di_rdi_parsed_nil)
      {
        RDI_NameMap *mapptr = rdi_element_from_name_idx(rdi, NameMaps, RDI_NameMapKind_NormalSourcePaths);
        RDI_ParsedNameMap map = {0};
        rdi_parsed_from_name_map(rdi, mapptr, &map);
        RDI_NameMapNode *node = rdi_name_map_lookup(rdi, &map, file_path_normalized.str, file_path_normalized.size);
        if(node != 0)
        {
          U32 id_count = 0;
          U32 *ids = rdi_matches_from_map_node(rdi, node, &id_count);
          if(id_count > 0)
          {
            good_src_id = 1;
            src_id = ids[0];
          }
        }
      }
      
      // rjf: good src-id -> look up line info for visible range
      if(good_src_id)
      {
        RDI_SourceFile *src = rdi_element_from_name_idx(rdi, SourceFiles, src_id);
        RDI_SourceLineMap *src_line_map = rdi_element_from_name_idx(rdi, SourceLineMaps, src->source_line_map_idx);
        RDI_ParsedSourceLineMap line_map = {0};
        rdi_parsed_from_source_line_map(rdi, src_line_map, &line_map);
        U64 line_idx = 0;
        for(S64 line_num = line_num_range.min;
            line_num <= line_num_range.max;
            line_num += 1, line_idx += 1)
        {
          DF_LineList *list = &array.v[line_idx];
          U32 voff_count = 0;
          U64 *voffs = rdi_line_voffs_from_num(&line_map, u32_from_u64_saturate((U64)line_num), &voff_count);
          for(U64 idx = 0; idx < voff_count; idx += 1)
          {
            U64 base_voff = voffs[idx];
            U64 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, base_voff);
            RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
            RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
            RDI_ParsedLineTable unit_line_info = {0};
            rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
            U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, base_voff);
            if(unit_line_info.voffs != 0)
            {
              Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
              S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
              DF_LineNode *n = push_array(arena, DF_LineNode, 1);
              n->v.voff_range = range;
              n->v.pt.line = (S64)actual_line;
              n->v.pt.column = 1;
              n->v.dbgi_key = key;
              SLLQueuePush(list->first, list->last, n);
              list->count += 1;
            }
          }
        }
      }
      
      // rjf: good src id -> push to relevant dbgi keys
      if(good_src_id)
      {
        di_key_list_push(arena, &array.dbgi_keys, &key);
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return array;
}

internal DF_LineList
df_lines_from_file_path_line_num(Arena *arena, String8 file_path, S64 line_num)
{
  DF_LineListArray array = df_lines_array_from_file_path_line_range(arena, file_path, r1s64(line_num, line_num+1));
  DF_LineList list = {0};
  if(array.count != 0)
  {
    list = array.v[0];
  }
  return list;
}

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal DF_Entity *
df_module_from_process_vaddr(DF_Entity *process, U64 vaddr)
{
  DF_Entity *module = &df_g_nil_entity;
  for(DF_Entity *child = process->first; !df_entity_is_nil(child); child = child->next)
  {
    if(child->kind == DF_EntityKind_Module && contains_1u64(child->vaddr_rng, vaddr))
    {
      module = child;
      break;
    }
  }
  return module;
}

internal DF_Entity *
df_module_from_thread(DF_Entity *thread)
{
  DF_Entity *process = thread->parent;
  U64 rip = df_query_cached_rip_from_thread(thread);
  return df_module_from_process_vaddr(process, rip);
}

internal U64
df_tls_base_vaddr_from_process_root_rip(DF_Entity *process, U64 root_vaddr, U64 rip_vaddr)
{
  ProfBeginFunction();
  U64 base_vaddr = 0;
  Temp scratch = scratch_begin(0, 0);
  if(!df_ctrl_targets_running())
  {
    //- rjf: unpack module info
    DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
    Rng1U64 tls_vaddr_range = ctrl_tls_vaddr_range_from_module(module->ctrl_machine_id, module->ctrl_handle);
    U64 addr_size = bit_size_from_arch(process->arch)/8;
    
    //- rjf: read module's TLS index
    U64 tls_index = 0;
    if(addr_size != 0)
    {
      CTRL_ProcessMemorySlice tls_index_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, tls_vaddr_range, 0);
      if(tls_index_slice.data.size >= addr_size)
      {
        tls_index = *(U64 *)tls_index_slice.data.str;
      }
    }
    
    //- rjf: PE path
    if(addr_size != 0)
    {
      U64 thread_info_addr = root_vaddr;
      U64 tls_addr_off = tls_index*addr_size;
      U64 tls_addr_array = 0;
      CTRL_ProcessMemorySlice tls_addr_array_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, r1u64(thread_info_addr, thread_info_addr+addr_size), 0);
      String8 tls_addr_array_data = tls_addr_array_slice.data;
      if(tls_addr_array_data.size >= 8)
      {
        MemoryCopy(&tls_addr_array, tls_addr_array_data.str, sizeof(U64));
      }
      CTRL_ProcessMemorySlice result_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, r1u64(tls_addr_array + tls_addr_off, tls_addr_array + tls_addr_off + addr_size), 0);
      String8 result_data = result_slice.data;
      if(result_data.size >= 8)
      {
        MemoryCopy(&base_vaddr, result_data.str, sizeof(U64));
      }
    }
    
    //- rjf: non-PE path (not implemented)
#if 0
    if(!bin_is_pe)
    {
      // TODO(rjf): not supported. old code from the prototype that Nick had sketched out:
      // TODO(nick): This code works only if the linked c runtime library is glibc.
      // Implement CRT detection here.
      
      U64 dtv_addr = UINT64_MAX;
      demon_read_memory(process->demon_handle, &dtv_addr, thread_info_addr, addr_size);
      
      /*
        union delta_thread_vector
        {
          size_t counter;
          struct
          {
            void *value;
            void *to_free;
          } pointer;
        };
      */
      
      U64 dtv_size = 16;
      U64 dtv_count = 0;
      demon_read_memory(process->demon_handle, &dtv_count, dtv_addr - dtv_size, addr_size);
      
      if (tls_index > 0 && tls_index < dtv_count)
      {
        demon_read_memory(process->demon_handle, &result, dtv_addr + dtv_size*tls_index, addr_size);
      }
    }
#endif
  }
  scratch_end(scratch);
  ProfEnd();
  return base_vaddr;
}

internal Architecture
df_architecture_from_entity(DF_Entity *entity)
{
  return entity->arch;
}

internal E_String2NumMap *
df_push_locals_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff)
{
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  E_String2NumMap *result = e_push_locals_map_from_rdi_voff(arena, rdi, voff);
  return result;
}

internal E_String2NumMap *
df_push_member_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff)
{
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  E_String2NumMap *result = e_push_member_map_from_rdi_voff(arena, rdi, voff);
  return result;
}

internal B32
df_set_thread_rip(DF_Entity *thread, U64 vaddr)
{
  Temp scratch = scratch_begin(0, 0);
  void *block = ctrl_query_cached_reg_block_from_thread(scratch.arena, df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  regs_arch_block_write_rip(thread->arch, block, vaddr);
  B32 result = ctrl_thread_write_reg_block(thread->ctrl_machine_id, thread->ctrl_handle, block);
  
  // rjf: early mutation of unwind cache for immediate frontend effect
  if(result)
  {
    DF_UnwindCache *cache = &df_state->unwind_cache;
    if(cache->slots_count != 0)
    {
      DF_Handle thread_handle = df_handle_from_entity(thread);
      U64 hash = df_hash_from_string(str8_struct(&thread_handle));
      U64 slot_idx = hash%cache->slots_count;
      DF_UnwindCacheSlot *slot = &cache->slots[slot_idx];
      for(DF_UnwindCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(df_handle_match(n->thread, thread_handle) && n->unwind.frames.count != 0)
        {
          regs_arch_block_write_rip(thread->arch, n->unwind.frames.v[0].regs, vaddr);
          break;
        }
      }
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal DF_Entity *
df_module_from_thread_candidates(DF_Entity *thread, DF_EntityList *candidates)
{
  DF_Entity *src_module = df_module_from_thread(thread);
  DF_Entity *module = &df_g_nil_entity;
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  for(DF_EntityNode *n = candidates->first; n != 0; n = n->next)
  {
    DF_Entity *candidate_module = n->entity;
    DF_Entity *candidate_process = df_entity_ancestor_from_kind(candidate_module, DF_EntityKind_Process);
    if(candidate_process == process)
    {
      module = candidate_module;
    }
    if(candidate_module == src_module)
    {
      break;
    }
  }
  return module;
}

internal DF_Unwind
df_unwind_from_ctrl_unwind(Arena *arena, DI_Scope *di_scope, DF_Entity *process, CTRL_Unwind *base_unwind)
{
  Architecture arch = df_architecture_from_entity(process);
  DF_Unwind result = {0};
  result.frames.concrete_frame_count = base_unwind->frames.count;
  result.frames.total_frame_count = result.frames.concrete_frame_count;
  result.frames.v = push_array(arena, DF_UnwindFrame, result.frames.concrete_frame_count);
  for(U64 idx = 0; idx < result.frames.concrete_frame_count; idx += 1)
  {
    CTRL_UnwindFrame *src = &base_unwind->frames.v[idx];
    DF_UnwindFrame *dst = &result.frames.v[idx];
    U64 rip_vaddr = regs_rip_from_arch_block(arch, src->regs);
    DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
    U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
    DI_Key dbgi_key = df_dbgi_key_from_module(module);
    RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
    RDI_Scope *scope = rdi_scope_from_voff(rdi, rip_voff);
    
    // rjf: fill concrete frame info
    dst->regs = src->regs;
    dst->rdi = rdi;
    dst->procedure = rdi_element_from_name_idx(rdi, Procedures, scope->proc_idx);
    
    // rjf: push inline frames
    for(RDI_Scope *s = scope;
        s->inline_site_idx != 0;
        s = rdi_element_from_name_idx(rdi, Scopes, s->parent_scope_idx))
    {
      RDI_InlineSite *site = rdi_element_from_name_idx(rdi, InlineSites, s->inline_site_idx);
      DF_UnwindInlineFrame *inline_frame = push_array(arena, DF_UnwindInlineFrame, 1);
      DLLPushFront(dst->first_inline_frame, dst->last_inline_frame, inline_frame);
      inline_frame->inline_site = site;
      dst->inline_frame_count += 1;
      result.frames.inline_frame_count += 1;
      result.frames.total_frame_count += 1;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Target Controls

//- rjf: control message dispatching

internal void
df_push_ctrl_msg(CTRL_Msg *msg)
{
  CTRL_Msg *dst = ctrl_msg_list_push(df_state->ctrl_msg_arena, &df_state->ctrl_msgs);
  ctrl_msg_deep_copy(df_state->ctrl_msg_arena, dst, msg);
  if(df_state->ctrl_soft_halt_issued == 0 && df_ctrl_targets_running())
  {
    df_state->ctrl_soft_halt_issued = 1;
    ctrl_halt();
  }
}

//- rjf: control thread running

internal void
df_ctrl_run(DF_RunKind run, DF_Entity *run_thread, CTRL_RunFlags flags, CTRL_TrapList *run_traps)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: build run message
  CTRL_Msg msg = {(run == DF_RunKind_Run || run == DF_RunKind_Step) ? CTRL_MsgKind_Run : CTRL_MsgKind_SingleStep};
  {
    DF_EntityList user_bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
    DF_Entity *process = df_entity_ancestor_from_kind(run_thread, DF_EntityKind_Process);
    msg.run_flags = flags;
    msg.machine_id = run_thread->ctrl_machine_id;
    msg.entity = run_thread->ctrl_handle;
    msg.parent = process->ctrl_handle;
    MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
    if(run_traps != 0)
    {
      MemoryCopyStruct(&msg.traps, run_traps);
    }
    for(DF_EntityNode *user_bp_n = user_bps.first;
        user_bp_n != 0;
        user_bp_n = user_bp_n->next)
    {
      // rjf: unpack user breakpoint entity
      DF_Entity *user_bp = user_bp_n->entity;
      if(user_bp->disabled)
      {
        continue;
      }
      DF_Entity *loc = df_entity_child_from_kind(user_bp, DF_EntityKind_Location);
      DF_Entity *cnd = df_entity_child_from_kind(user_bp, DF_EntityKind_Condition);
      
      // rjf: textual location -> add breakpoints for all possible override locations
      if(loc->flags & DF_EntityFlag_HasTextPoint)
      {
        String8List overrides = df_possible_overrides_from_file_path(scratch.arena, loc->name);
        for(String8Node *n = overrides.first; n != 0; n = n->next)
        {
          CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_FileNameAndLineColNumber};
          ctrl_user_bp.string    = n->string;
          ctrl_user_bp.pt        = loc->text_point;
          ctrl_user_bp.condition = cnd->name;
          ctrl_user_breakpoint_list_push(scratch.arena, &msg.user_bps, &ctrl_user_bp);
        }
      }
      
      // rjf: virtual address location -> add breakpoint for address
      else if(loc->flags & DF_EntityFlag_HasVAddr)
      {
        CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_VirtualAddress};
        ctrl_user_bp.u64       = loc->vaddr;
        ctrl_user_bp.condition = cnd->name;
        ctrl_user_breakpoint_list_push(scratch.arena, &msg.user_bps, &ctrl_user_bp);
      }
      
      // rjf: symbol name location -> add breakpoint for symbol name
      else if(loc->name.size != 0)
      {
        CTRL_UserBreakpoint ctrl_user_bp = {CTRL_UserBreakpointKind_SymbolNameAndOffset};
        ctrl_user_bp.string    = loc->name;
        ctrl_user_bp.condition = cnd->name;
        ctrl_user_breakpoint_list_push(scratch.arena, &msg.user_bps, &ctrl_user_bp);
      }
    }
    for(DF_HandleNode *n = df_state->frozen_threads.first; n != 0; n = n->next)
    {
      DF_Entity *thread = df_entity_from_handle(n->handle);
      if(!df_entity_is_nil(thread))
      {
        CTRL_MachineIDHandlePair pair = {thread->ctrl_machine_id, thread->ctrl_handle};
        ctrl_machine_id_handle_pair_list_push(scratch.arena, &msg.freeze_state_threads, &pair);
      }
    }
    msg.freeze_state_is_frozen = 1;
  }
  
  // rjf: push msg
  df_push_ctrl_msg(&msg);
  
  // rjf: copy run traps to scratch (needed, if the caller can pass `df_state->ctrl_last_run_traps`)
  CTRL_TrapList run_traps_copy = {0};
  if(run_traps != 0)
  {
    run_traps_copy = ctrl_trap_list_copy(scratch.arena, run_traps);
  }
  
  // rjf: store last run info
  arena_clear(df_state->ctrl_last_run_arena);
  df_state->ctrl_last_run_kind = run;
  df_state->ctrl_last_run_frame_idx = df_frame_index();
  df_state->ctrl_last_run_thread = df_handle_from_entity(run_thread);
  df_state->ctrl_last_run_flags = flags;
  df_state->ctrl_last_run_traps = ctrl_trap_list_copy(df_state->ctrl_last_run_arena, &run_traps_copy);
  df_state->ctrl_is_running = 1;
  
  // rjf: reset selected frame to top unwind
  df_state->base_interact_regs.v.unwind_count = 0;
  df_state->base_interact_regs.v.inline_depth = 0;
  
  scratch_end(scratch);
}

//- rjf: stopped info from the control thread

internal CTRL_Event
df_ctrl_last_stop_event(void)
{
  return df_state->ctrl_last_stop_event;
}

////////////////////////////////
//~ rjf: Evaluation Context

//- rjf: entity <-> eval space

internal DF_Entity *
df_entity_from_eval_space(E_Space space)
{
  DF_Entity *entity = &df_g_nil_entity;
  if(space.u64[0] == 0 && space.u64[1] != 0)
  {
    entity = (DF_Entity *)space.u64[1];
  }
  return entity;
}

internal E_Space
df_eval_space_from_entity(DF_Entity *entity)
{
  E_Space space = {0};
  space.u64[1] = (U64)entity;
  return space;
}

//- rjf: eval space reads/writes

internal B32
df_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range)
{
  B32 result = 0;
  DF_Entity *entity = df_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: nil-space -> fall back to file system
    case DF_EntityKind_Nil:
    {
      U128 key = space;
      U128 hash = hs_hash_from_key(key, 0);
      HS_Scope *scope = hs_scope_open();
      {
        String8 data = hs_data_from_hash(scope, hash);
        Rng1U64 legal_range = r1u64(0, data.size);
        Rng1U64 read_range = intersect_1u64(range, legal_range);
        if(read_range.min < read_range.max)
        {
          result = 1;
          MemoryCopy(out, data.str + read_range.min, dim_1u64(read_range));
        }
      }
      hs_scope_close(scope);
    }break;
    
    //- rjf: default -> evaluating a debugger entity; read from entity POD evaluation
    default:
    {
      Temp scratch = scratch_begin(0, 0);
      arena_push(scratch.arena, 0, 64);
      U64 pos_min = arena_pos(scratch.arena);
      DF_EntityEval *eval = df_eval_from_entity(scratch.arena, entity);
      U64 pos_opl = arena_pos(scratch.arena);
      Rng1U64 legal_range = r1u64(0, pos_opl-pos_min);
      if(contains_1u64(legal_range, range.min))
      {
        result = 1;
        U64 range_dim = dim_1u64(range);
        U64 bytes_to_read = Min(range_dim, (legal_range.max - range.min));
        MemoryCopy(out, ((U8 *)eval) + range.min, bytes_to_read);
        if(bytes_to_read < range_dim)
        {
          MemoryZero((U8 *)out + bytes_to_read, range_dim - bytes_to_read);
        }
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: process -> reading process memory
    case DF_EntityKind_Process:
    {
      Temp scratch = scratch_begin(0, 0);
      CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, entity->ctrl_machine_id, entity->ctrl_handle, range, df_state->frame_eval_memread_endt_us);
      String8 data = slice.data;
      if(data.size == dim_1u64(range))
      {
        result = 1;
        MemoryCopy(out, data.str, data.size);
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: thread -> reading from thread register block
    case DF_EntityKind_Thread:
    {
      Temp scratch = scratch_begin(0, 0);
      CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
      U64 frame_idx = e_interpret_ctx->reg_unwind_count;
      if(frame_idx < unwind.frames.count)
      {
        CTRL_UnwindFrame *f = &unwind.frames.v[frame_idx];
        U64 regs_size = regs_block_size_from_architecture(e_interpret_ctx->reg_arch);
        Rng1U64 legal_range = r1u64(0, regs_size);
        Rng1U64 read_range = intersect_1u64(legal_range, range);
        U64 read_size = dim_1u64(read_range);
        MemoryCopy(out, (U8 *)f->regs + read_range.min, read_size);
        result = (read_size == dim_1u64(range));
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal B32
df_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range)
{
  B32 result = 0;
  DF_Entity *entity = df_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: default -> making commits to entity evaluation
    default:
    {
      Temp scratch = scratch_begin(0, 0);
      DF_EntityEval *eval = df_eval_from_entity(scratch.arena, entity);
      U64 range_dim = dim_1u64(range);
      if(range.min == OffsetOf(DF_EntityEval, enabled) &&
         range_dim >= 1)
      {
        result = 1;
        B32 new_enabled = !!((U8 *)in)[0];
        df_entity_equip_disabled(entity, !new_enabled);
      }
      else if(range.min == eval->label_off &&
              range_dim >= 1)
      {
        result = 1;
        String8 new_name = str8_cstring_capped((U8 *)in, (U8 *)in+range_dim);
        df_entity_equip_name(entity, new_name);
      }
      else if(range.min == eval->condition_off &&
              range_dim >= 1)
      {
        result = 1;
        DF_Entity *condition = df_entity_child_from_kind(entity, DF_EntityKind_Condition);
        if(df_entity_is_nil(condition))
        {
          condition = df_entity_alloc(entity, DF_EntityKind_Condition);
        }
        String8 new_name = str8_cstring_capped((U8 *)in, (U8 *)in+range_dim);
        df_entity_equip_name(condition, new_name);
      }
      scratch_end(scratch);
    }break;
    
    //- rjf: process -> commit to process memory
    case DF_EntityKind_Process:
    {
      result = ctrl_process_write(entity->ctrl_machine_id, entity->ctrl_handle, range, in);
    }break;
    
    //- rjf: thread -> commit to thread's register block
    case DF_EntityKind_Thread:
    {
      CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
      U64 frame_idx = 0;
      if(frame_idx < unwind.frames.count)
      {
        Temp scratch = scratch_begin(0, 0);
        U64 regs_size = regs_block_size_from_architecture(df_architecture_from_entity(entity));
        Rng1U64 legal_range = r1u64(0, regs_size);
        Rng1U64 write_range = intersect_1u64(legal_range, range);
        U64 write_size = dim_1u64(write_range);
        CTRL_UnwindFrame *f = &unwind.frames.v[frame_idx];
        void *new_regs = push_array(scratch.arena, U8, regs_size);
        MemoryCopy(new_regs, f->regs, regs_size);
        MemoryCopy((U8 *)new_regs + write_range.min, in, write_size);
        result = ctrl_thread_write_reg_block(entity->ctrl_machine_id, entity->ctrl_handle, new_regs);
        scratch_end(scratch);
      }
    }break;
  }
  return result;
}

//- rjf: asynchronous streamed reads -> hashes from spaces

internal U128
df_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated)
{
  U128 result = {0};
  DF_Entity *entity = df_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: nil space -> filesystem key encoded inside of `space`
    case DF_EntityKind_Nil:
    {
      result = space;
    }break;
    
    //- rjf: process space -> query 
    case DF_EntityKind_Process:
    {
      result = ctrl_hash_store_key_from_process_vaddr_range(entity->ctrl_machine_id, entity->ctrl_handle, range, zero_terminated);
    }break;
  }
  return result;
}

//- rjf: space -> entire range

internal Rng1U64
df_whole_range_from_eval_space(E_Space space)
{
  Rng1U64 result = r1u64(0, 0);
  DF_Entity *entity = df_entity_from_eval_space(space);
  switch(entity->kind)
  {
    //- rjf: nil space -> filesystem key encoded inside of `space`
    case DF_EntityKind_Nil:
    {
      HS_Scope *scope = hs_scope_open();
      U128 hash = {0};
      for(U64 idx = 0; idx < 2; idx += 1)
      {
        hash = hs_hash_from_key(space, idx);
        if(!u128_match(hash, u128_zero()))
        {
          break;
        }
      }
      String8 data = hs_data_from_hash(scope, hash);
      result = r1u64(0, data.size);
      hs_scope_close(scope);
    }break;
    case DF_EntityKind_Process:
    {
      result = r1u64(0, 0x7FFFFFFFFFFFull);
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Evaluation Views

#if !defined(BLAKE2_H)
#define HAVE_SSE2
#include "third_party/blake2/blake2.h"
#include "third_party/blake2/blake2b.c"
#endif

internal DF_EvalViewKey
df_eval_view_key_make(U64 v0, U64 v1)
{
  DF_EvalViewKey v = {v0, v1};
  return v;
}

internal DF_EvalViewKey
df_eval_view_key_from_string(String8 string)
{
  DF_EvalViewKey key = {0};
  blake2b((U8 *)&key.u64[0], sizeof(key), string.str, string.size, 0, 0);
  return key;
}

internal DF_EvalViewKey
df_eval_view_key_from_stringf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  DF_EvalViewKey key = df_eval_view_key_from_string(string);
  scratch_end(scratch);
  return key;
}

internal B32
df_eval_view_key_match(DF_EvalViewKey a, DF_EvalViewKey b)
{
  return MemoryMatchStruct(&a, &b);
}

internal DF_EvalView *
df_eval_view_from_key(DF_EvalViewKey key)
{
  DF_EvalView *eval_view = &df_g_nil_eval_view;
  {
    U64 slot_idx = key.u64[1]%df_state->eval_view_cache.slots_count;
    DF_EvalViewSlot *slot = &df_state->eval_view_cache.slots[slot_idx];
    for(DF_EvalView *v = slot->first; v != &df_g_nil_eval_view && v != 0; v = v->hash_next)
    {
      if(df_eval_view_key_match(key, v->key))
      {
        eval_view = v;
        break;
      }
    }
    if(eval_view == &df_g_nil_eval_view)
    {
      eval_view = push_array(df_state->arena, DF_EvalView, 1);
      DLLPushBack_NPZ(&df_g_nil_eval_view, slot->first, slot->last, eval_view, hash_next, hash_prev);
      eval_view->key = key;
      eval_view->arena = arena_alloc();
      df_expand_tree_table_init(eval_view->arena, &eval_view->expand_tree_table, 256);
      eval_view->view_rule_table.slot_count = 64;
      eval_view->view_rule_table.slots = push_array(eval_view->arena, DF_EvalViewRuleCacheSlot, eval_view->view_rule_table.slot_count);
    }
  }
  return eval_view;
}

//- rjf: key -> view rules

internal void
df_eval_view_set_key_rule(DF_EvalView *eval_view, DF_ExpandKey key, String8 view_rule_string)
{
  //- rjf: key -> hash * slot idx * slot
  String8 key_string = str8_struct(&key);
  U64 hash = df_hash_from_string(key_string);
  U64 slot_idx = hash%eval_view->view_rule_table.slot_count;
  DF_EvalViewRuleCacheSlot *slot = &eval_view->view_rule_table.slots[slot_idx];
  
  //- rjf: slot -> existing node
  DF_EvalViewRuleCacheNode *existing_node = 0;
  for(DF_EvalViewRuleCacheNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(df_expand_key_match(n->key, key))
    {
      existing_node = n;
      break;
    }
  }
  
  //- rjf: existing node * new node -> node
  DF_EvalViewRuleCacheNode *node = existing_node;
  if(node == 0)
  {
    node = push_array(eval_view->arena, DF_EvalViewRuleCacheNode, 1);
    DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);
    node->key = key;
    node->buffer_cap = 512;
    node->buffer = push_array(eval_view->arena, U8, node->buffer_cap);
  }
  
  //- rjf: mutate node
  if(node != 0)
  {
    node->buffer_string_size = ClampTop(view_rule_string.size, node->buffer_cap);
    MemoryCopy(node->buffer, view_rule_string.str, node->buffer_string_size);
  }
}

internal String8
df_eval_view_rule_from_key(DF_EvalView *eval_view, DF_ExpandKey key)
{
  String8 result = {0};
  
  //- rjf: key -> hash * slot idx * slot
  String8 key_string = str8_struct(&key);
  U64 hash = df_hash_from_string(key_string);
  U64 slot_idx = hash%eval_view->view_rule_table.slot_count;
  DF_EvalViewRuleCacheSlot *slot = &eval_view->view_rule_table.slots[slot_idx];
  
  //- rjf: slot -> existing node
  DF_EvalViewRuleCacheNode *existing_node = 0;
  for(DF_EvalViewRuleCacheNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(df_expand_key_match(n->key, key))
    {
      existing_node = n;
      break;
    }
  }
  
  //- rjf: node -> result
  if(existing_node != 0)
  {
    result = str8(existing_node->buffer, existing_node->buffer_string_size);
  }
  
  return result;
}

////////////////////////////////
//~ rjf: Evaluation View Visualization & Interaction

//- rjf: expr * view rule table -> expr

internal E_Expr *
df_expr_from_expr_cfg(Arena *arena, E_Expr *expr, DF_CfgTable *cfg)
{
  for(DF_CfgVal *val = cfg->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
  {
    DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
    if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_ExprResolution)
    {
      expr = spec->info.expr_resolution(arena, expr, val->last->root);
    }
  }
  return expr;
}

//- rjf: evaluation value string builder helpers

internal String8
df_string_from_ascii_value(Arena *arena, U8 val)
{
  String8 result = {0};
  switch(val)
  {
    case 0x00:{result = str8_lit("\\0");}break;
    case 0x07:{result = str8_lit("\\a");}break;
    case 0x08:{result = str8_lit("\\b");}break;
    case 0x0c:{result = str8_lit("\\f");}break;
    case 0x0a:{result = str8_lit("\\n");}break;
    case 0x0d:{result = str8_lit("\\r");}break;
    case 0x09:{result = str8_lit("\\t");}break;
    case 0x0b:{result = str8_lit("\\v");}break;
    case 0x3f:{result = str8_lit("\\?");}break;
    case '"': {result = str8_lit("\\\"");}break;
    case '\'':{result = str8_lit("\\'");}break;
    case '\\':{result = str8_lit("\\\\");}break;
    default:
    if(32 <= val && val < 255)
    {
      result = push_str8f(arena, "%c", val);
    }break;
  }
  return result;
}

internal String8
df_string_from_hresult_facility_code(U32 code)
{
  String8 result = {0};
  switch(code)
  {
    default:{}break;
    case 0x1:{result = str8_lit("RPC");}break;
    case 0x2:{result = str8_lit("DISPATCH");}break;
    case 0x3:{result = str8_lit("STORAGE");}break;
    case 0x4:{result = str8_lit("ITF");}break;
    case 0x7:{result = str8_lit("WIN32");}break;
    case 0x8:{result = str8_lit("WINDOWS");}break;
    case 0x9:{result = str8_lit("SECURITY|SSPI");}break;
    case 0xA:{result = str8_lit("CONTROL");}break;
    case 0xB:{result = str8_lit("CERT");}break;
    case 0xC:{result = str8_lit("INTERNET");}break;
    case 0xD:{result = str8_lit("MEDIASERVER");}break;
    case 0xE:{result = str8_lit("MSMQ");}break;
    case 0xF:{result = str8_lit("SETUPAPI");}break;
    case 0x10:{result = str8_lit("SCARD");}break;
    case 0x11:{result = str8_lit("COMPLUS");}break;
    case 0x12:{result = str8_lit("AAF");}break;
    case 0x13:{result = str8_lit("URT");}break;
    case 0x14:{result = str8_lit("ACS");}break;
    case 0x15:{result = str8_lit("DPLAY");}break;
    case 0x16:{result = str8_lit("UMI");}break;
    case 0x17:{result = str8_lit("SXS");}break;
    case 0x18:{result = str8_lit("WINDOWS_CE");}break;
    case 0x19:{result = str8_lit("HTTP");}break;
    case 0x1A:{result = str8_lit("USERMODE_COMMONLOG");}break;
    case 0x1B:{result = str8_lit("WER");}break;
    case 0x1F:{result = str8_lit("USERMODE_FILTER_MANAGER");}break;
    case 0x20:{result = str8_lit("BACKGROUNDCOPY");}break;
    case 0x21:{result = str8_lit("CONFIGURATION|WIA");}break;
    case 0x22:{result = str8_lit("STATE_MANAGEMENT");}break;
    case 0x23:{result = str8_lit("METADIRECTORY");}break;
    case 0x24:{result = str8_lit("WINDOWSUPDATE");}break;
    case 0x25:{result = str8_lit("DIRECTORYSERVICE");}break;
    case 0x26:{result = str8_lit("GRAPHICS");}break;
    case 0x27:{result = str8_lit("SHELL|NAP");}break;
    case 0x28:{result = str8_lit("TPM_SERVICES");}break;
    case 0x29:{result = str8_lit("TPM_SOFTWARE");}break;
    case 0x2A:{result = str8_lit("UI");}break;
    case 0x2B:{result = str8_lit("XAML");}break;
    case 0x2C:{result = str8_lit("ACTION_QUEUE");}break;
    case 0x30:{result = str8_lit("WINDOWS_SETUP|PLA");}break;
    case 0x31:{result = str8_lit("FVE");}break;
    case 0x32:{result = str8_lit("FWP");}break;
    case 0x33:{result = str8_lit("WINRM");}break;
    case 0x34:{result = str8_lit("NDIS");}break;
    case 0x35:{result = str8_lit("USERMODE_HYPERVISOR");}break;
    case 0x36:{result = str8_lit("CMI");}break;
    case 0x37:{result = str8_lit("USERMODE_VIRTUALIZATION");}break;
    case 0x38:{result = str8_lit("USERMODE_VOLMGR");}break;
    case 0x39:{result = str8_lit("BCD");}break;
    case 0x3A:{result = str8_lit("USERMODE_VHD");}break;
    case 0x3C:{result = str8_lit("SDIAG");}break;
    case 0x3D:{result = str8_lit("WINPE|WEBSERVICES");}break;
    case 0x3E:{result = str8_lit("WPN");}break;
    case 0x3F:{result = str8_lit("WINDOWS_STORE");}break;
    case 0x40:{result = str8_lit("INPUT");}break;
    case 0x42:{result = str8_lit("EAP");}break;
    case 0x50:{result = str8_lit("WINDOWS_DEFENDER");}break;
    case 0x51:{result = str8_lit("OPC");}break;
    case 0x52:{result = str8_lit("XPS");}break;
    case 0x53:{result = str8_lit("RAS");}break;
    case 0x54:{result = str8_lit("POWERSHELL|MBN");}break;
    case 0x55:{result = str8_lit("EAS");}break;
    case 0x62:{result = str8_lit("P2P_INT");}break;
    case 0x63:{result = str8_lit("P2P");}break;
    case 0x64:{result = str8_lit("DAF");}break;
    case 0x65:{result = str8_lit("BLUETOOTH_ATT");}break;
    case 0x66:{result = str8_lit("AUDIO");}break;
    case 0x6D:{result = str8_lit("VISUALCPP");}break;
    case 0x70:{result = str8_lit("SCRIPT");}break;
    case 0x71:{result = str8_lit("PARSE");}break;
    case 0x78:{result = str8_lit("BLB");}break;
    case 0x79:{result = str8_lit("BLB_CLI");}break;
    case 0x7A:{result = str8_lit("WSBAPP");}break;
    case 0x80:{result = str8_lit("BLBUI");}break;
    case 0x81:{result = str8_lit("USN");}break;
    case 0x82:{result = str8_lit("USERMODE_VOLSNAP");}break;
    case 0x83:{result = str8_lit("TIERING");}break;
    case 0x85:{result = str8_lit("WSB_ONLINE");}break;
    case 0x86:{result = str8_lit("ONLINE_ID");}break;
    case 0x99:{result = str8_lit("DLS");}break;
    case 0xA0:{result = str8_lit("SOS");}break;
    case 0xB0:{result = str8_lit("DEBUGGERS");}break;
    case 0xE7:{result = str8_lit("USERMODE_SPACES");}break;
    case 0x100:{result = str8_lit("DMSERVER|RESTORE|SPP");}break;
    case 0x101:{result = str8_lit("DEPLOYMENT_SERVICES_SERVER");}break;
    case 0x102:{result = str8_lit("DEPLOYMENT_SERVICES_IMAGING");}break;
    case 0x103:{result = str8_lit("DEPLOYMENT_SERVICES_MANAGEMENT");}break;
    case 0x104:{result = str8_lit("DEPLOYMENT_SERVICES_UTIL");}break;
    case 0x105:{result = str8_lit("DEPLOYMENT_SERVICES_BINLSVC");}break;
    case 0x107:{result = str8_lit("DEPLOYMENT_SERVICES_PXE");}break;
    case 0x108:{result = str8_lit("DEPLOYMENT_SERVICES_TFTP");}break;
    case 0x110:{result = str8_lit("DEPLOYMENT_SERVICES_TRANSPORT_MANAGEMENT");}break;
    case 0x116:{result = str8_lit("DEPLOYMENT_SERVICES_DRIVER_PROVISIONING");}break;
    case 0x121:{result = str8_lit("DEPLOYMENT_SERVICES_MULTICAST_SERVER");}break;
    case 0x122:{result = str8_lit("DEPLOYMENT_SERVICES_MULTICAST_CLIENT");}break;
    case 0x125:{result = str8_lit("DEPLOYMENT_SERVICES_CONTENT_PROVIDER");}break;
    case 0x131:{result = str8_lit("LINGUISTIC_SERVICES");}break;
    case 0x375:{result = str8_lit("WEB");}break;
    case 0x376:{result = str8_lit("WEB_SOCKET");}break;
    case 0x446:{result = str8_lit("AUDIOSTREAMING");}break;
    case 0x600:{result = str8_lit("ACCELERATOR");}break;
    case 0x701:{result = str8_lit("MOBILE");}break;
    case 0x7CC:{result = str8_lit("WMAAECMA");}break;
    case 0x801:{result = str8_lit("WEP");}break;
    case 0x802:{result = str8_lit("SYNCENGINE");}break;
    case 0x878:{result = str8_lit("DIRECTMUSIC");}break;
    case 0x879:{result = str8_lit("DIRECT3D10");}break;
    case 0x87A:{result = str8_lit("DXGI");}break;
    case 0x87B:{result = str8_lit("DXGI_DDI");}break;
    case 0x87C:{result = str8_lit("DIRECT3D11");}break;
    case 0x888:{result = str8_lit("LEAP");}break;
    case 0x889:{result = str8_lit("AUDCLNT");}break;
    case 0x898:{result = str8_lit("WINCODEC_DWRITE_DWM");}break;
    case 0x899:{result = str8_lit("DIRECT2D");}break;
    case 0x900:{result = str8_lit("DEFRAG");}break;
    case 0x901:{result = str8_lit("USERMODE_SDBUS");}break;
    case 0x902:{result = str8_lit("JSCRIPT");}break;
    case 0xA01:{result = str8_lit("PIDGENX");}break;
  }
  return result;
}

internal String8
df_string_from_hresult_code(U32 code)
{
  String8 result = {0};
  switch(code)
  {
    default:{}break;
    case 0x00000000: {result = str8_lit("S_OK: Operation successful");}break;
    case 0x00000001: {result = str8_lit("S_FALSE: Operation successful but returned no results");}break;
    case 0x80004004: {result = str8_lit("E_ABORT: Operation aborted");}break;
    case 0x80004005: {result = str8_lit("E_FAIL: Unspecified failure");}break;
    case 0x80004002: {result = str8_lit("E_NOINTERFACE: No such interface supported");}break;
    case 0x80004001: {result = str8_lit("E_NOTIMPL: Not implemented");}break;
    case 0x80004003: {result = str8_lit("E_POINTER: Pointer that is not valid");}break;
    case 0x8000FFFF: {result = str8_lit("E_UNEXPECTED: Unexpected failure");}break;
    case 0x80070005: {result = str8_lit("E_ACCESSDENIED: General access denied error");}break;
    case 0x80070006: {result = str8_lit("E_HANDLE: Handle that is not valid");}break;
    case 0x80070057: {result = str8_lit("E_INVALIDARG: One or more arguments are not valid");}break;
    case 0x8007000E: {result = str8_lit("E_OUTOFMEMORY: Failed to allocate necessary memory");}break;
  }
  return result;
}

internal String8
df_string_from_simple_typed_eval(Arena *arena, DF_EvalVizStringFlags flags, U32 radix, E_Eval eval)
{
  String8 result = {0};
  E_TypeKey type_key = e_type_unwrap(eval.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  U64 type_byte_size = e_type_byte_size_from_key(type_key);
  U8 digit_group_separator = 0;
  if(!(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules))
  {
    digit_group_separator = 0;
  }
  switch(type_kind)
  {
    default:{}break;
    
    case E_TypeKind_Handle:
    {
      result = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
    }break;
    
    case E_TypeKind_HResult:
    {
      if(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules)
      {
        Temp scratch = scratch_begin(&arena, 1);
        U32 hresult_value = (U32)eval.value.u64;
        U32 is_error   = !!(hresult_value & (1ull<<31));
        U32 error_code = (hresult_value);
        U32 facility   = (hresult_value & 0x7ff0000) >> 16;
        String8 value_string = str8_from_s64(scratch.arena, eval.value.u64, radix, 0, digit_group_separator);
        String8 facility_string = df_string_from_hresult_facility_code(facility);
        String8 error_string = df_string_from_hresult_code(error_code);
        result = push_str8f(arena, "%S%s%s%S%s%s%S%s",
                            error_string,
                            error_string.size != 0 ? " " : "",
                            facility_string.size != 0 ? "[" : "",
                            facility_string,
                            facility_string.size != 0 ? "] ": "",
                            error_string.size != 0 ? "(" : "",
                            value_string,
                            error_string.size != 0 ? ")" : "");
        scratch_end(scratch);
      }
      else
      {
        result = str8_from_s64(arena, eval.value.u64, radix, 0, digit_group_separator);
      }
    }break;
    
    case E_TypeKind_Char8:
    case E_TypeKind_Char16:
    case E_TypeKind_Char32:
    case E_TypeKind_UChar8:
    case E_TypeKind_UChar16:
    case E_TypeKind_UChar32:
    {
      String8 char_str = df_string_from_ascii_value(arena, eval.value.s64);
      if(char_str.size != 0)
      {
        if(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules)
        {
          String8 imm_string = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
          result = push_str8f(arena, "'%S' (%S)", char_str, imm_string);
        }
        else
        {
          result = push_str8f(arena, "'%S'", char_str);
        }
      }
      else
      {
        result = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
      }
    }break;
    
    case E_TypeKind_S8:
    case E_TypeKind_S16:
    case E_TypeKind_S32:
    case E_TypeKind_S64:
    {
      result = str8_from_s64(arena, eval.value.s64, radix, 0, digit_group_separator);
    }break;
    
    case E_TypeKind_U8:
    case E_TypeKind_U16:
    case E_TypeKind_U32:
    case E_TypeKind_U64:
    {
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      result = str8_from_u64(arena, eval.value.u64, radix, min_digits, digit_group_separator);
    }break;
    
    case E_TypeKind_U128:
    {
      Temp scratch = scratch_begin(&arena, 1);
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      String8 upper64 = str8_from_u64(scratch.arena, eval.value.u128.u64[0], radix, min_digits, digit_group_separator);
      String8 lower64 = str8_from_u64(scratch.arena, eval.value.u128.u64[1], radix, min_digits, digit_group_separator);
      result = push_str8f(arena, "%S:%S", upper64, lower64);
      scratch_end(scratch);
    }break;
    
    case E_TypeKind_F32: {result = push_str8f(arena, "%f", eval.value.f32);}break;
    case E_TypeKind_F64: {result = push_str8f(arena, "%f", eval.value.f64);}break;
    case E_TypeKind_Bool:{result = push_str8f(arena, "%s", eval.value.u64 ? "true" : "false");}break;
    case E_TypeKind_Ptr: {result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_LRef:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_RRef:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    case E_TypeKind_Function:{result = push_str8f(arena, "0x%I64x", eval.value.u64);}break;
    
    case E_TypeKind_Enum:
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_Type *type = e_type_from_key(scratch.arena, type_key);
      String8 constant_name = {0};
      for(U64 val_idx = 0; val_idx < type->count; val_idx += 1)
      {
        if(eval.value.u64 == type->enum_vals[val_idx].val)
        {
          constant_name = type->enum_vals[val_idx].name;
          break;
        }
      }
      if(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules)
      {
        if(constant_name.size != 0)
        {
          result = push_str8f(arena, "0x%I64x (%S)", eval.value.u64, constant_name);
        }
        else
        {
          result = push_str8f(arena, "0x%I64x (%I64u)", eval.value.u64, eval.value.u64);
        }
      }
      else if(constant_name.size != 0)
      {
        result = push_str8_copy(arena, constant_name);
      }
      else
      {
        result = push_str8f(arena, "0x%I64x (%I64u)", eval.value.u64, eval.value.u64);
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal String8
df_escaped_from_raw_string(Arena *arena, String8 raw)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  U64 start_split_idx = 0;
  for(U64 idx = 0; idx <= raw.size; idx += 1)
  {
    U8 byte = (idx < raw.size) ? raw.str[idx] : 0;
    B32 split = 1;
    String8 separator_replace = {0};
    switch(byte)
    {
      default:{split = 0;}break;
      case 0:    {}break;
      case '\a': {separator_replace = str8_lit("\\a");}break;
      case '\b': {separator_replace = str8_lit("\\b");}break;
      case '\f': {separator_replace = str8_lit("\\f");}break;
      case '\n': {separator_replace = str8_lit("\\n");}break;
      case '\r': {separator_replace = str8_lit("\\r");}break;
      case '\t': {separator_replace = str8_lit("\\t");}break;
      case '\v': {separator_replace = str8_lit("\\v");}break;
      case '\\': {separator_replace = str8_lit("\\\\");}break;
      case '"':  {separator_replace = str8_lit("\\\"");}break;
      case '?':  {separator_replace = str8_lit("\\?");}break;
    }
    if(split)
    {
      String8 substr = str8_substr(raw, r1u64(start_split_idx, idx));
      start_split_idx = idx+1;
      str8_list_push(scratch.arena, &parts, substr);
      if(separator_replace.size != 0)
      {
        str8_list_push(scratch.arena, &parts, separator_replace);
      }
    }
  }
  StringJoin join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}

//- rjf: type info -> expandability/editablity

internal B32
df_type_key_is_expandable(E_TypeKey type_key)
{
  B32 result = 0;
  for(E_TypeKey t = type_key; !result; t = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(t))))
  {
    E_TypeKind kind = e_type_kind_from_key(t);
    if(kind == E_TypeKind_Null || kind == E_TypeKind_Function)
    {
      break;
    }
    if(kind == E_TypeKind_Struct ||
       kind == E_TypeKind_Union ||
       kind == E_TypeKind_Class ||
       kind == E_TypeKind_Array ||
       kind == E_TypeKind_Enum)
    {
      result = 1;
    }
  }
  return result;
}

internal B32
df_type_key_is_editable(E_TypeKey type_key)
{
  B32 result = 0;
  for(E_TypeKey t = type_key; !result; t = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(t))))
  {
    E_TypeKind kind = e_type_kind_from_key(t);
    if(kind == E_TypeKind_Null || kind == E_TypeKind_Function)
    {
      break;
    }
    if((E_TypeKind_FirstBasic <= kind && kind <= E_TypeKind_LastBasic) || e_type_kind_is_pointer_or_ref(kind))
    {
      result = 1;
    }
  }
  return result;
}

//- rjf: writing values back to child processes

internal B32
df_commit_eval_value_string(E_Eval dst_eval, String8 string)
{
  B32 result = 0;
  if(dst_eval.mode == E_Mode_Offset)
  {
    Temp scratch = scratch_begin(0, 0);
    E_TypeKey type_key = e_type_unwrap(dst_eval.type_key);
    E_TypeKind type_kind = e_type_kind_from_key(type_key);
    E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(e_type_unwrap(dst_eval.type_key)));
    E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
    String8 commit_data = {0};
    B32 commit_at_ptr_dest = 0;
    if(E_TypeKind_FirstBasic <= type_kind && type_kind <= E_TypeKind_LastBasic)
    {
      E_Eval src_eval = e_eval_from_string(scratch.arena, string);
      commit_data = push_str8_copy(scratch.arena, str8_struct(&src_eval.value));
      commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(type_key));
    }
    else if(type_kind == E_TypeKind_Ptr || type_kind == E_TypeKind_Array)
    {
      E_Eval src_eval = e_eval_from_string(scratch.arena, string);
      E_Eval src_eval_value = e_value_eval_from_eval(src_eval);
      E_TypeKind src_eval_value_type_kind = e_type_kind_from_key(src_eval_value.type_key);
      if(type_kind == E_TypeKind_Ptr &&
         (e_type_kind_is_pointer_or_ref(src_eval_value_type_kind) ||
          e_type_kind_is_integer(src_eval_value_type_kind)) &&
         src_eval_value.value.u64 != 0 && src_eval_value.mode == E_Mode_Value)
      {
        commit_data = push_str8_copy(scratch.arena, str8_struct(&src_eval.value));
        commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(type_key));
      }
      else if(direct_type_kind == E_TypeKind_Char8 ||
              direct_type_kind == E_TypeKind_UChar8 ||
              e_type_kind_is_integer(direct_type_kind))
      {
        if(string.size >= 1 && string.str[0] == '"')
        {
          string = str8_skip(string, 1);
        }
        if(string.size >= 1 && string.str[string.size-1] == '"')
        {
          string = str8_chop(string, 1);
        }
        commit_data = e_raw_from_escaped_string(scratch.arena, string);
        commit_data.size += 1;
        if(type_kind == E_TypeKind_Ptr)
        {
          commit_at_ptr_dest = 1;
        }
      }
    }
    if(commit_data.size != 0 && e_type_byte_size_from_key(type_key) != 0)
    {
      U64 dst_offset = dst_eval.value.u64;
      if(dst_eval.mode == E_Mode_Offset && commit_at_ptr_dest)
      {
        E_Eval dst_value_eval = e_value_eval_from_eval(dst_eval);
        dst_offset = dst_value_eval.value.u64;
      }
      result = e_space_write(dst_eval.space, commit_data.str, r1u64(dst_offset, dst_offset + commit_data.size));
    }
    scratch_end(scratch);
  }
  return result;
}

//- rjf: type helpers

internal E_MemberArray
df_filtered_data_members_from_members_cfg_table(Arena *arena, E_MemberArray members, DF_CfgTable *cfg)
{
  DF_CfgVal *only = df_cfg_val_from_string(cfg, str8_lit("only"));
  DF_CfgVal *omit = df_cfg_val_from_string(cfg, str8_lit("omit"));
  E_MemberArray filtered_members = members;
  if(only != &df_g_nil_cfg_val || omit != &df_g_nil_cfg_val)
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct DF_TypeMemberLooseNode DF_TypeMemberLooseNode;
    struct DF_TypeMemberLooseNode
    {
      DF_TypeMemberLooseNode *next;
      E_Member *member;
    };
    DF_TypeMemberLooseNode *first_member = 0;
    DF_TypeMemberLooseNode *last_member = 0;
    U64 member_count = 0;
    MemoryZeroStruct(&filtered_members);
    for(U64 idx = 0; idx < members.count; idx += 1)
    {
      // rjf: check if included by 'only's
      B32 is_included = 1;
      for(DF_CfgTree *r = only->first; r != &df_g_nil_cfg_tree; r = r->next)
      {
        is_included = 0;
        for(MD_EachNode(name_node, r->root->first))
        {
          String8 name = name_node->string;
          if(str8_match(members.v[idx].name, name, 0))
          {
            is_included = 1;
            goto end_inclusion_check;
          }
        }
      }
      end_inclusion_check:;
      
      // rjf: remove if excluded by 'omit's
      for(DF_CfgTree *r = omit->first; r != &df_g_nil_cfg_tree; r = r->next)
      {
        for(MD_EachNode(name_node, r->root->first))
        {
          String8 name = name_node->string;
          if(str8_match(members.v[idx].name, name, 0))
          {
            is_included = 0;
            goto end_exclusion_check;
          }
        }
      }
      end_exclusion_check:;
      
      // rjf: push if included
      if(is_included)
      {
        DF_TypeMemberLooseNode *n = push_array(scratch.arena, DF_TypeMemberLooseNode, 1);
        n->member = &members.v[idx];
        SLLQueuePush(first_member, last_member, n);
        member_count += 1;
      }
    }
    
    // rjf: bake
    {
      filtered_members.count = member_count;
      filtered_members.v = push_array_no_zero(arena, E_Member, filtered_members.count);
      U64 idx = 0;
      for(DF_TypeMemberLooseNode *n = first_member; n != 0; n = n->next, idx += 1)
      {
        MemoryCopyStruct(&filtered_members.v[idx], n->member);
        filtered_members.v[idx].name = push_str8_copy(arena, filtered_members.v[idx].name);
        filtered_members.v[idx].inheritance_key_chain = e_type_key_list_copy(arena, &filtered_members.v[idx].inheritance_key_chain);
      }
    }
    scratch_end(scratch);
  }
  return filtered_members;
}

internal DF_EvalLinkBaseChunkList
df_eval_link_base_chunk_list_from_eval(Arena *arena, E_TypeKey link_member_type_key, U64 link_member_off, E_Eval eval, U64 cap)
{
  DF_EvalLinkBaseChunkList list = {0};
  for(E_Eval base_eval = eval, last_eval = zero_struct; list.count < cap;)
  {
    // rjf: check this ptr's validity
    if(base_eval.value.u64 == 0 || (base_eval.value.u64 == last_eval.value.u64 && base_eval.mode == last_eval.mode))
    {
      break;
    }
    
    // rjf: gather
    {
      DF_EvalLinkBaseChunkNode *chunk = list.last;
      if(chunk == 0 || chunk->count == ArrayCount(chunk->b))
      {
        chunk = push_array_no_zero(arena, DF_EvalLinkBaseChunkNode, 1);
        chunk->next = 0;
        chunk->count = 0;
        SLLQueuePush(list.first, list.last, chunk);
      }
      chunk->b[chunk->count].offset = base_eval.value.u64;
      chunk->count += 1;
      list.count += 1;
    }
    
    // rjf: grab link member
    E_Eval link_member_eval =
    {
      .value = {.u64 = base_eval.value.u64 + link_member_off},
      .mode = E_Mode_Offset,
      .space = base_eval.space,
      .type_key = link_member_type_key,
    };
    E_Eval link_member_value_eval = e_value_eval_from_eval(link_member_eval);
    
    // rjf: advance to next link
    last_eval = base_eval;
    base_eval.mode = E_Mode_Offset;
    base_eval.value.u64 = link_member_value_eval.value.u64;
  }
  return list;
}

internal DF_EvalLinkBase
df_eval_link_base_from_chunk_list_index(DF_EvalLinkBaseChunkList *list, U64 idx)
{
  DF_EvalLinkBase result = zero_struct;
  U64 scan_idx = 0;
  for(DF_EvalLinkBaseChunkNode *chunk = list->first; chunk != 0; chunk = chunk->next)
  {
    U64 chunk_idx_opl = scan_idx+chunk->count;
    if(scan_idx <= idx && idx < chunk_idx_opl)
    {
      result = chunk->b[idx - scan_idx];
    }
    scan_idx = chunk_idx_opl;
  }
  return result;
}

internal DF_EvalLinkBaseArray
df_eval_link_base_array_from_chunk_list(Arena *arena, DF_EvalLinkBaseChunkList *chunks)
{
  DF_EvalLinkBaseArray array = {0};
  array.count = chunks->count;
  array.v = push_array_no_zero(arena, DF_EvalLinkBase, array.count);
  U64 idx = 0;
  for(DF_EvalLinkBaseChunkNode *n = chunks->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v + idx, n->b, n->count * sizeof(DF_EvalLinkBase));
    idx += n->count;
  }
  return array;
}

//- rjf: viz block collection building

internal DF_EvalVizBlock *
df_eval_viz_block_begin(Arena *arena, DF_EvalVizBlockKind kind, DF_ExpandKey parent_key, DF_ExpandKey key, S32 depth)
{
  DF_EvalVizBlockNode *n = push_array(arena, DF_EvalVizBlockNode, 1);
  n->v.kind       = kind;
  n->v.parent_key = parent_key;
  n->v.key        = key;
  n->v.depth      = depth;
  n->v.expr       = &e_expr_nil;
  n->v.cfg_table  = &df_g_nil_cfg_table;
  return &n->v;
}

internal DF_EvalVizBlock *
df_eval_viz_block_split_and_continue(Arena *arena, DF_EvalVizBlockList *list, DF_EvalVizBlock *split_block, U64 split_idx)
{
  U64 total_count = split_block->semantic_idx_range.max;
  split_block->visual_idx_range.max = split_block->semantic_idx_range.max = split_idx;
  df_eval_viz_block_end(list, split_block);
  DF_EvalVizBlock *continue_block = df_eval_viz_block_begin(arena, split_block->kind, split_block->parent_key, split_block->key, split_block->depth);
  continue_block->string            = split_block->string;
  continue_block->expr              = split_block->expr;
  continue_block->visual_idx_range  = continue_block->semantic_idx_range = r1u64(split_idx+1, total_count);
  continue_block->cfg_table         = split_block->cfg_table;
  continue_block->link_bases        = split_block->link_bases;
  continue_block->members           = split_block->members;
  continue_block->enum_vals         = split_block->enum_vals;
  continue_block->fzy_target        = split_block->fzy_target;
  continue_block->fzy_backing_items = split_block->fzy_backing_items;
  return continue_block;
}

internal void
df_eval_viz_block_end(DF_EvalVizBlockList *list, DF_EvalVizBlock *block)
{
  DF_EvalVizBlockNode *n = CastFromMember(DF_EvalVizBlockNode, v, block);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  list->total_visual_row_count += dim_1u64(block->visual_idx_range);
  list->total_semantic_row_count += dim_1u64(block->semantic_idx_range);
}

internal void
df_append_expr_eval_viz_blocks__rec(Arena *arena, DF_EvalView *eval_view, DF_ExpandKey parent_key, DF_ExpandKey key, String8 string, E_Expr *expr, DF_CfgTable *cfg_table, S32 depth, DF_EvalVizBlockList *list_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: apply expr resolution view rules
  expr = df_expr_from_expr_cfg(arena, expr, cfg_table);
  
  //- rjf: determine if this key is expanded
  DF_ExpandNode *node = df_expand_node_from_key(&eval_view->expand_tree_table, key);
  B32 parent_is_expanded = (node != 0 && node->expanded);
  
  //- rjf: push block for expression root
  {
    DF_EvalVizBlock *block = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Root, parent_key, key, depth);
    block->string                      = string;
    block->expr                        = expr;
    block->cfg_table                   = cfg_table;
    block->visual_idx_range            = r1u64(key.child_num-1, key.child_num+0);
    block->semantic_idx_range          = r1u64(key.child_num-1, key.child_num+0);
    df_eval_viz_block_end(list_out, block);
  }
  
  //- rjf: determine view rule to generate children blocks
  DF_CoreViewRuleSpec *expand_view_rule_spec = df_core_view_rule_spec_from_string(str8_lit("default"));
  MD_Node *expand_view_rule_params = &md_nil_node;
  if(parent_is_expanded)
  {
    for(DF_CfgVal *val = cfg_table->first_val;
        val != 0 && val != &df_g_nil_cfg_val;
        val = val->linear_next)
    {
      DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
      if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_VizBlockProd)
      {
        expand_view_rule_spec = spec;
        expand_view_rule_params = val->last->root;
        break;
      }
    }
  }
  
  //- rjf: do view rule children block generation, if we have an applicable view rule
  if(parent_is_expanded && expand_view_rule_spec != &df_g_nil_core_view_rule_spec)
  {
    expand_view_rule_spec->info.viz_block_prod(arena, eval_view, parent_key, key, node, string, expr, cfg_table, depth+1, expand_view_rule_params, list_out);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal DF_EvalVizBlockList
df_eval_viz_block_list_from_eval_view_expr_keys(Arena *arena, DF_EvalView *eval_view, DF_CfgTable *cfg_table, String8 expr, DF_ExpandKey parent_key, DF_ExpandKey key)
{
  ProfBeginFunction();
  DF_EvalVizBlockList blocks = {0};
  {
    E_TokenArray tokens = e_token_array_from_text(arena, expr);
    E_Parse parse = e_parse_expr_from_text_tokens(arena, expr, &tokens);
    U64 parse_opl = parse.last_token >= tokens.v + tokens.count ? expr.size : parse.last_token->range.min;
    U64 expr_comma_pos = str8_find_needle(expr, parse_opl, str8_lit(","), 0);
    U64 passthrough_pos = str8_find_needle(expr, parse_opl, str8_lit("--"), 0);
    String8List default_view_rules = {0};
    if(expr_comma_pos < expr.size && expr_comma_pos < passthrough_pos)
    {
      String8 expr_extension = str8_substr(expr, r1u64(expr_comma_pos+1, passthrough_pos));
      expr_extension = str8_skip_chop_whitespace(expr_extension);
      if(str8_match(expr_extension, str8_lit("x"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(arena, &default_view_rules, "hex");
      }
      else if(str8_match(expr_extension, str8_lit("b"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(arena, &default_view_rules, "bin");
      }
      else if(str8_match(expr_extension, str8_lit("o"), StringMatchFlag_CaseInsensitive))
      {
        str8_list_pushf(arena, &default_view_rules, "oct");
      }
      else if(expr_extension.size != 0)
      {
        str8_list_pushf(arena, &default_view_rules, "array:{%S}", expr_extension);
      }
    }
    if(passthrough_pos < expr.size)
    {
      String8 passthrough_view_rule = str8_skip_chop_whitespace(str8_skip(expr, passthrough_pos+2));
      if(passthrough_view_rule.size != 0)
      {
        str8_list_push(arena, &default_view_rules, passthrough_view_rule);
      }
    }
    String8 view_rule_string = df_eval_view_rule_from_key(eval_view, key);
    DF_CfgTable *cfg_table_inherited = push_array(arena, DF_CfgTable, 1);
    *cfg_table_inherited = df_cfg_table_from_inheritance(arena, cfg_table);
    for(String8Node *n = default_view_rules.first; n != 0; n = n->next)
    {
      df_cfg_table_push_unparsed_string(arena, cfg_table_inherited, n->string, DF_CfgSrc_User);
    }
    df_cfg_table_push_unparsed_string(arena, cfg_table_inherited, view_rule_string, DF_CfgSrc_User);
    df_append_expr_eval_viz_blocks__rec(arena, eval_view, parent_key, key, expr, parse.expr, cfg_table_inherited, 0, &blocks);
  }
  ProfEnd();
  return blocks;
}

internal void
df_eval_viz_block_list_concat__in_place(DF_EvalVizBlockList *dst, DF_EvalVizBlockList *to_push)
{
  if(dst->last == 0)
  {
    *dst = *to_push;
  }
  else if(to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->count += to_push->count;
    dst->total_visual_row_count += to_push->total_visual_row_count;
    dst->total_semantic_row_count += to_push->total_semantic_row_count;
  }
  MemoryZeroStruct(to_push);
}

internal S64
df_row_num_from_viz_block_list_key(DF_EvalVizBlockList *blocks, DF_ExpandKey key)
{
  S64 row_num = 1;
  B32 found = 0;
  for(DF_EvalVizBlockNode *n = blocks->first; n != 0; n = n->next)
  {
    DF_EvalVizBlock *block = &n->v;
    if(key.parent_hash == block->key.parent_hash)
    {
      B32 this_block_contains_this_key = 0;
      {
        if(block->fzy_backing_items.v != 0)
        {
          U64 item_num = fzy_item_num_from_array_element_idx__linear_search(&block->fzy_backing_items, key.child_num);
          this_block_contains_this_key = (item_num != 0 && contains_1u64(block->semantic_idx_range, item_num-1));
        }
        else
        {
          this_block_contains_this_key = (block->semantic_idx_range.min+1 <= key.child_num && key.child_num < block->semantic_idx_range.max+1);
        }
      }
      if(this_block_contains_this_key)
      {
        found = 1;
        if(block->fzy_backing_items.v != 0)
        {
          U64 item_num = fzy_item_num_from_array_element_idx__linear_search(&block->fzy_backing_items, key.child_num);
          row_num += item_num-1-block->semantic_idx_range.min;
        }
        else
        {
          row_num += key.child_num-1-block->semantic_idx_range.min;
        }
        break;
      }
    }
    if(!found)
    {
      row_num += (S64)dim_1u64(block->semantic_idx_range);
    }
  }
  if(!found)
  {
    row_num = 0;
  }
  return row_num;
}

internal DF_ExpandKey
df_key_from_viz_block_list_row_num(DF_EvalVizBlockList *blocks, S64 row_num)
{
  DF_ExpandKey key = {0};
  S64 scan_y = 1;
  for(DF_EvalVizBlockNode *n = blocks->first; n != 0; n = n->next)
  {
    DF_EvalVizBlock *vb = &n->v;
    Rng1S64 vb_row_num_range = r1s64(scan_y, scan_y + (S64)dim_1u64(vb->semantic_idx_range));
    if(contains_1s64(vb_row_num_range, row_num))
    {
      key = vb->key;
      if(vb->fzy_backing_items.v != 0)
      {
        U64 item_idx = (U64)((row_num - vb_row_num_range.min) + vb->semantic_idx_range.min);
        if(item_idx < vb->fzy_backing_items.count)
        {
          key.child_num = vb->fzy_backing_items.v[item_idx].idx;
        }
      }
      else
      {
        key.child_num = vb->semantic_idx_range.min + (row_num - vb_row_num_range.min) + 1;
      }
      break;
    }
    scan_y += dim_1s64(vb_row_num_range);
  }
  return key;
}

internal DF_ExpandKey
df_parent_key_from_viz_block_list_row_num(DF_EvalVizBlockList *blocks, S64 row_num)
{
  DF_ExpandKey key = {0};
  S64 scan_y = 1;
  for(DF_EvalVizBlockNode *n = blocks->first; n != 0; n = n->next)
  {
    DF_EvalVizBlock *vb = &n->v;
    Rng1S64 vb_row_num_range = r1s64(scan_y, scan_y + (S64)dim_1u64(vb->semantic_idx_range));
    if(contains_1s64(vb_row_num_range, row_num))
    {
      key = vb->parent_key;
      break;
    }
    scan_y += dim_1s64(vb_row_num_range);
  }
  return key;
}

//- rjf: viz block * index -> expression

internal E_Expr *
df_expr_from_eval_viz_block_index(Arena *arena, DF_EvalVizBlock *block, U64 index)
{
  E_Expr *result = block->expr;
  switch(block->kind)
  {
    default:{}break;
    case DF_EvalVizBlockKind_Members:
    {
      E_MemberArray *members = &block->members;
      if(index < members->count)
      {
        E_Member *member = &members->v[index];
        E_Expr *dot_expr = e_expr_ref_member_access(arena, block->expr, member->name);
        result = dot_expr;
      }
    }break;
    case DF_EvalVizBlockKind_EnumMembers:
    {
      E_EnumValArray *enum_vals = &block->enum_vals;
      if(index < enum_vals->count)
      {
        E_EnumVal *val = &enum_vals->v[index];
        E_Expr *dot_expr = e_expr_ref_member_access(arena, block->expr, val->name);
        result = dot_expr;
      }
    }break;
    case DF_EvalVizBlockKind_Elements:
    {
      E_Expr *idx_expr = e_expr_ref_array_index(arena, block->expr, index);
      result = idx_expr;
    }break;
    case DF_EvalVizBlockKind_DebugInfoTable:
    {
      // rjf: unpack row info
      FZY_Item *item = &block->fzy_backing_items.v[index];
      DF_ExpandKey parent_key = block->parent_key;
      DF_ExpandKey key = block->key;
      key.child_num = block->fzy_backing_items.v[index].idx;
      
      // rjf: determine module to which this item belongs
      E_Module *module = e_parse_ctx->primary_module;
      U64 base_idx = 0;
      {
        for(U64 module_idx = 0; module_idx < e_parse_ctx->modules_count; module_idx += 1)
        {
          U64 all_items_count = 0;
          rdi_section_raw_table_from_kind(e_parse_ctx->modules[module_idx].rdi, block->fzy_target, &all_items_count);
          if(base_idx <= item->idx && item->idx < base_idx + all_items_count)
          {
            module = &e_parse_ctx->modules[module_idx];
            break;
          }
          base_idx += all_items_count;
        }
      }
      
      // rjf: build expr
      E_Expr *item_expr = &e_expr_nil;
      {
        RDI_SectionKind section = block->fzy_target;
        U64 element_idx = block->fzy_backing_items.v[index].idx - base_idx;
        switch(section)
        {
          default:{}break;
          case RDI_SectionKind_Procedures:
          {
            RDI_Procedure *procedure = rdi_element_from_name_idx(module->rdi, Procedures, element_idx);
            RDI_Scope *scope = rdi_element_from_name_idx(module->rdi, Scopes, procedure->root_scope_idx);
            U64 voff = *rdi_element_from_name_idx(module->rdi, ScopeVOffData, scope->voff_range_first);
            E_OpList oplist = {0};
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_ModuleOff, e_value_u64(voff));
            String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
            U32 type_idx = procedure->type_idx;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_LeafBytecode, 0);
            item_expr->mode     = E_Mode_Value;
            item_expr->space    = module->space;
            item_expr->type_key = type_key;
            item_expr->bytecode = bytecode;
            item_expr->string.str = rdi_string_from_idx(module->rdi, procedure->name_string_idx, &item_expr->string.size);
          }break;
          case RDI_SectionKind_GlobalVariables:
          {
            RDI_GlobalVariable *gvar = rdi_element_from_name_idx(module->rdi, GlobalVariables, element_idx);
            U64 voff = gvar->voff;
            E_OpList oplist = {0};
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_ModuleOff, e_value_u64(voff));
            String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
            U32 type_idx = gvar->type_idx;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_LeafBytecode, 0);
            item_expr->mode     = E_Mode_Offset;
            item_expr->space    = module->space;
            item_expr->type_key = type_key;
            item_expr->bytecode = bytecode;
            item_expr->string.str = rdi_string_from_idx(module->rdi, gvar->name_string_idx, &item_expr->string.size);
          }break;
          case RDI_SectionKind_ThreadVariables:
          {
            RDI_ThreadVariable *tvar = rdi_element_from_name_idx(module->rdi, ThreadVariables, element_idx);
            E_OpList oplist = {0};
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_TLSOff, e_value_u64(tvar->tls_off));
            String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
            U32 type_idx = tvar->type_idx;
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_LeafBytecode, 0);
            item_expr->mode     = E_Mode_Offset;
            item_expr->space    = module->space;
            item_expr->type_key = type_key;
            item_expr->bytecode = bytecode;
            item_expr->string.str = rdi_string_from_idx(module->rdi, tvar->name_string_idx, &item_expr->string.size);
          }break;
          case RDI_SectionKind_UDTs:
          {
            RDI_UDT *udt = rdi_element_from_name_idx(module->rdi, UDTs, element_idx);
            RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, udt->self_type_idx);
            E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), udt->self_type_idx, (U32)(module - e_parse_ctx->modules));
            item_expr = e_push_expr(arena, E_ExprKind_TypeIdent, 0);
            item_expr->type_key = type_key;
          }break;
        }
      }
      
      result = item_expr;
    }break;
  }
  return result;
}

//- rjf: viz row list building

internal DF_EvalVizRow *
df_eval_viz_row_list_push_new(Arena *arena, DF_EvalView *eval_view, DF_EvalVizWindowedRowList *rows, DF_EvalVizBlock *block, DF_ExpandKey key, E_Expr *expr)
{
  // rjf: push
  DF_EvalVizRow *row = push_array(arena, DF_EvalVizRow, 1);
  SLLQueuePush(rows->first, rows->last, row);
  rows->count += 1;
  
  // rjf: pick cfg table; resolve expression if needed
  DF_CfgTable *cfg_table = 0;
  switch(block->kind)
  {
    default:
    {
      cfg_table = push_array(arena, DF_CfgTable, 1);
      *cfg_table = df_cfg_table_from_inheritance(arena, block->cfg_table);
      String8 row_view_rules = df_eval_view_rule_from_key(eval_view, key);
      if(row_view_rules.size != 0)
      {
        df_cfg_table_push_unparsed_string(arena, cfg_table, row_view_rules, DF_CfgSrc_User);
      }
      expr = df_expr_from_expr_cfg(arena, expr, cfg_table);
    }break;
    case DF_EvalVizBlockKind_Root:
    case DF_EvalVizBlockKind_Canvas:
    {
      cfg_table = block->cfg_table;
    }break;
  }
  
  // rjf: determine row ui hook to use for this row
  DF_GfxViewRuleSpec *value_ui_rule_spec = &df_g_nil_gfx_view_rule_spec;
  MD_Node *value_ui_rule_params = &md_nil_node;
  for(DF_CfgVal *val = cfg_table->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
  {
    DF_GfxViewRuleSpec *spec = df_gfx_view_rule_spec_from_string(val->string);
    if(spec->info.flags & DF_GfxViewRuleSpecInfoFlag_RowUI)
    {
      value_ui_rule_spec = spec;
      value_ui_rule_params = val->last->root;
      break;
    }
  }
  
  // rjf: determine block ui hook to use for this row
  DF_GfxViewRuleSpec *expand_ui_rule_spec = &df_g_nil_gfx_view_rule_spec;
  MD_Node *expand_ui_rule_params = &md_nil_node;
  if(block->kind == DF_EvalVizBlockKind_Canvas)
  {
    for(DF_CfgVal *val = cfg_table->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
    {
      DF_GfxViewRuleSpec *spec = df_gfx_view_rule_spec_from_string(val->string);
      if(spec->info.flags & DF_GfxViewRuleSpecInfoFlag_ViewUI)
      {
        expand_ui_rule_spec = spec;
        expand_ui_rule_params = val->last->root;
        break;
      }
    }
  }
  
  // rjf: fill
  row->depth        = block->depth;
  row->parent_key   = block->parent_key;
  row->key          = key;
  row->size_in_rows = 1;
  row->string       = block->string;
  row->expr         = expr;
  if(row->expr->kind == E_ExprKind_MemberAccess)
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr->first);
    E_TypeKey type = irtree.type_key;
    E_MemberArray data_members = e_type_data_members_from_key(scratch.arena, type);
    E_Member *member = e_type_member_from_array_name(&data_members, row->expr->last->string);
    if(member != 0)
    {
      row->member = e_type_member_copy(arena, member);
    }
    scratch_end(scratch);
  }
  row->cfg_table             = cfg_table;
  row->value_ui_rule_spec    = value_ui_rule_spec;
  row->value_ui_rule_params  = value_ui_rule_params;
  row->expand_ui_rule_spec   = expand_ui_rule_spec;
  row->expand_ui_rule_params = expand_ui_rule_params;
  return row;
}

internal DF_EvalVizWindowedRowList
df_eval_viz_windowed_row_list_from_viz_block_list(Arena *arena, DF_EvalView *eval_view, Rng1S64 visible_range, DF_EvalVizBlockList *blocks)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////
  //- rjf: produce windowed rows, per block
  //
  U64 visual_idx_off = 0;
  U64 semantic_idx_off = 0;
  DF_EvalVizWindowedRowList list = {0};
  for(DF_EvalVizBlockNode *n = blocks->first; n != 0; n = n->next)
  {
    DF_EvalVizBlock *block = &n->v;
    
    //////////////////////////////
    //- rjf: extract block info
    //
    U64 block_num_visual_rows     = dim_1u64(block->visual_idx_range);
    U64 block_num_semantic_rows   = dim_1u64(block->semantic_idx_range);
    Rng1S64 block_visual_range    = r1s64(visual_idx_off, visual_idx_off + block_num_visual_rows);
    Rng1S64 block_semantic_range  = r1s64(semantic_idx_off, semantic_idx_off + block_num_semantic_rows);
    
    //////////////////////////////
    //- rjf: get skip/chop of block's index range
    //
    U64 num_skipped_visual = 0;
    U64 num_chopped_visual = 0;
    {
      if(visible_range.min > block_visual_range.min)
      {
        num_skipped_visual = (visible_range.min - block_visual_range.min);
        num_skipped_visual = Min(num_skipped_visual, block_num_visual_rows);
      }
      if(visible_range.max < block_visual_range.max)
      {
        num_chopped_visual = (block_visual_range.max - visible_range.max);
        num_chopped_visual = Min(num_chopped_visual, block_num_visual_rows);
      }
    }
    
    //////////////////////////////
    //- rjf: get visible idx range & invisible counts
    //
    Rng1U64 visible_idx_range = block->visual_idx_range;
    {
      visible_idx_range.min += num_skipped_visual;
      visible_idx_range.max -= num_chopped_visual;
    }
    
    //////////////////////////////
    //- rjf: sum & advance
    //
    list.count_before_visual += num_skipped_visual;
    if(block_num_visual_rows != 0)
    {
      list.count_before_semantic += block_num_semantic_rows * num_skipped_visual / block_num_visual_rows;
    }
    visual_idx_off += block_num_visual_rows;
    semantic_idx_off += block_num_semantic_rows;
    
    //////////////////////////////
    //- rjf: produce rows, depending on block's kind
    //
    if(visible_idx_range.max > visible_idx_range.min) switch(block->kind)
    {
      default:{}break;
      
      //////////////////////////////
      //- rjf: single rows, piping in info from the originating block
      //
      case DF_EvalVizBlockKind_Null:
      case DF_EvalVizBlockKind_Root:
      {
        df_eval_viz_row_list_push_new(arena, eval_view, &list, block, block->key, block->expr);
      }break;
      
      //////////////////////////////
      //- rjf: canvas -> produce blank row, sized by the idx range specified in the block
      //
      case DF_EvalVizBlockKind_Canvas:
      if(num_skipped_visual < block_num_visual_rows)
      {
        DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), 1);
        DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, eval_view, &list, block, key, block->expr);
        row->size_in_rows        = dim_1u64(intersect_1u64(visible_idx_range, r1u64(0, dim_1u64(block->visual_idx_range))));
        row->skipped_size_in_rows= (visible_idx_range.min > block->visual_idx_range.min) ? visible_idx_range.min - block->visual_idx_range.min : 0;
        row->chopped_size_in_rows= (visible_idx_range.max < block->visual_idx_range.max) ? block->visual_idx_range.max - visible_idx_range.max : 0;
      }break;
      
      //////////////////////////////
      //- rjf: all elements of a debug info table -> produce rows for visible range
      //
      case DF_EvalVizBlockKind_DebugInfoTable:
      for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
      {
        FZY_Item *item = &block->fzy_backing_items.v[idx];
        DF_ExpandKey parent_key = block->parent_key;
        DF_ExpandKey key = block->key;
        key.child_num = block->fzy_backing_items.v[idx].idx;
        E_Expr *row_expr = df_expr_from_eval_viz_block_index(arena, block, idx);
        df_eval_viz_row_list_push_new(arena, eval_view, &list, block, key, row_expr);
      }break;
      
      //////////////////////////////
      //- rjf: members/elements/enum-members
      //
      case DF_EvalVizBlockKind_Members:
      case DF_EvalVizBlockKind_EnumMembers:
      case DF_EvalVizBlockKind_Elements:
      {
        for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
        {
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), idx+1);
          E_Expr *expr = df_expr_from_eval_viz_block_index(arena, block, idx);
          df_eval_viz_row_list_push_new(arena, eval_view, &list, block, key, expr);
        }
      }break;
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return list;
}

//- rjf: viz row -> strings

internal String8
df_expr_string_from_viz_row(Arena *arena, DF_EvalVizRow *row)
{
  String8 result = row->string;
  if(result.size == 0) switch(row->expr->kind)
  {
    default:
    {
      result = e_string_from_expr(arena, row->expr);
    }break;
    case E_ExprKind_ArrayIndex:
    {
      result = push_str8f(arena, "[%S]", e_string_from_expr(arena, row->expr->last));
    }break;
    case E_ExprKind_MemberAccess:
    {
      result = push_str8f(arena, ".%S", e_string_from_expr(arena, row->expr->last));
    }break;
  }
  return result;
}

//- rjf: viz row -> expandability/editability

internal B32
df_viz_row_is_expandable(DF_EvalVizRow *row)
{
  B32 result = 0;
  {
    // rjf: determine if view rules force expandability
    if(!result)
    {
      for(DF_CfgVal *val = row->cfg_table->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
      {
        DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
        if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_Expandable)
        {
          result = 1;
          break;
        }
      }
    }
    
    // rjf: determine if type info force expandability
    if(!result)
    {
      Temp scratch = scratch_begin(0, 0);
      E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr);
      result = df_type_key_is_expandable(irtree.type_key);
      scratch_end(scratch);
    }
  }
  return result;
}

internal B32
df_viz_row_is_editable(DF_EvalVizRow *row)
{
  B32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, row->expr);
  result = df_type_key_is_editable(irtree.type_key);
  scratch_end(scratch);
  return result;
}

//- rjf: view rule config tree info extraction

internal U64
df_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(eval.type_key)))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

internal E_Value
df_value_from_params(MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  String8 expr = md_string_from_children(scratch.arena, params);
  E_Eval eval = e_eval_from_string(scratch.arena, expr);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  scratch_end(scratch);
  return value_eval.value;
}

internal E_TypeKey
df_type_key_from_params(MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  String8 expr = md_string_from_children(scratch.arena, params);
  E_TokenArray tokens = e_token_array_from_text(scratch.arena, expr);
  E_Parse parse = e_parse_type_from_text_tokens(scratch.arena, expr, &tokens);
  E_TypeKey type_key = e_type_from_expr(parse.expr);
  scratch_end(scratch);
  return type_key;
}

internal E_Value
df_value_from_params_key(MD_Node *params, String8 key)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *key_node = md_child_from_string(params, key, 0);
  String8 expr = md_string_from_children(scratch.arena, key_node);
  E_Eval eval = e_eval_from_string(scratch.arena, expr);
  E_Eval value_eval = e_value_eval_from_eval(eval);
  scratch_end(scratch);
  return value_eval.value;
}

internal Rng1U64
df_range_from_eval_params(E_Eval eval, MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  U64 size = df_value_from_params_key(params, str8_lit("size")).u64;
  E_TypeKey type_key = e_type_unwrap(eval.type_key);
  E_TypeKind type_kind = e_type_kind_from_key(type_key);
  E_TypeKey direct_type_key = e_type_unwrap(e_type_direct_from_key(eval.type_key));
  E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
  if(size == 0 && e_type_kind_is_pointer_or_ref(type_kind) && (direct_type_kind == E_TypeKind_Struct ||
                                                               direct_type_kind == E_TypeKind_Union ||
                                                               direct_type_kind == E_TypeKind_Class ||
                                                               direct_type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(e_type_direct_from_key(e_type_unwrap(eval.type_key)));
  }
  if(size == 0 && eval.mode == E_Mode_Offset && (type_kind == E_TypeKind_Struct ||
                                                 type_kind == E_TypeKind_Union ||
                                                 type_kind == E_TypeKind_Class ||
                                                 type_kind == E_TypeKind_Array))
  {
    size = e_type_byte_size_from_key(e_type_unwrap(eval.type_key));
  }
  if(size == 0)
  {
    size = 16384;
  }
  Rng1U64 result = {0};
  result.min = df_base_offset_from_eval(eval);
  result.max = result.min + size;
  scratch_end(scratch);
  return result;
}

internal TXT_LangKind
df_lang_kind_from_eval_params(E_Eval eval, MD_Node *params)
{
  TXT_LangKind lang_kind = TXT_LangKind_Null;
  if(eval.expr->kind == E_ExprKind_LeafFilePath)
  {
    lang_kind = txt_lang_kind_from_extension(str8_skip_last_dot(eval.expr->string));
  }
  else
  {
    MD_Node *lang_node = md_child_from_string(params, str8_lit("lang"), 0);
    String8 lang_kind_string = lang_node->first->string;
    lang_kind = txt_lang_kind_from_extension(lang_kind_string);
  }
  return lang_kind;
}

internal Architecture
df_architecture_from_eval_params(E_Eval eval, MD_Node *params)
{
  Architecture arch = Architecture_Null;
  MD_Node *arch_node = md_child_from_string(params, str8_lit("arch"), 0);
  String8 arch_kind_string = arch_node->first->string;
  if(str8_match(arch_kind_string, str8_lit("x64"), StringMatchFlag_CaseInsensitive))
  {
    arch = Architecture_x64;
  }
  return arch;
}

internal Vec2S32
df_dim2s32_from_eval_params(E_Eval eval, MD_Node *params)
{
  Vec2S32 dim = v2s32(1, 1);
  {
    dim.x = df_value_from_params_key(params, str8_lit("w")).s32;
    dim.y = df_value_from_params_key(params, str8_lit("h")).s32;
  }
  return dim;
}

internal R_Tex2DFormat
df_tex2dformat_from_eval_params(E_Eval eval, MD_Node *params)
{
  R_Tex2DFormat result = R_Tex2DFormat_RGBA8;
  {
    MD_Node *fmt_node = md_child_from_string(params, str8_lit("fmt"), 0);
    for(EachNonZeroEnumVal(R_Tex2DFormat, fmt))
    {
      if(str8_match(r_tex2d_format_display_string_table[fmt], fmt_node->first->string, StringMatchFlag_CaseInsensitive))
      {
        result = fmt;
        break;
      }
    }
  }
  return result;
}

//- rjf: eval -> entity

internal DF_Entity *
df_entity_from_eval_string(String8 string)
{
  DF_Entity *entity = &df_g_nil_entity;
  {
    Temp scratch = scratch_begin(0, 0);
    E_Eval eval = e_eval_from_string(scratch.arena, string);
    entity = df_entity_from_eval_space(eval.space);
    scratch_end(scratch);
  }
  return entity;
}

internal String8
df_eval_string_from_entity(Arena *arena, DF_Entity *entity)
{
  String8 eval_string = push_str8f(arena, "macro:`$%I64u`", entity->id);
  return eval_string;
}

//- rjf: eval <-> file path

internal String8
df_file_path_from_eval_string(Arena *arena, String8 string)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Eval eval = e_eval_from_string(scratch.arena, string);
    if(eval.expr->kind == E_ExprKind_LeafFilePath)
    {
      result = df_cfg_raw_from_escaped_string(arena, eval.expr->string);
    }
    scratch_end(scratch);
  }
  return result;
}

internal String8
df_eval_string_from_file_path(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 string_escaped = df_cfg_escaped_from_raw_string(scratch.arena, string);
  String8 result = push_str8f(arena, "file:\"%S\"", string_escaped);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame data

internal F32
df_dt(void)
{
  return df_state->dt;
}

internal U64
df_frame_index(void)
{
  return df_state->frame_index;
}

internal Arena *
df_frame_arena(void)
{
  return df_state->frame_arenas[df_state->frame_index%ArrayCount(df_state->frame_arenas)];
}

internal F64
df_time_in_seconds(void)
{
  return df_state->time_in_seconds;
}

//- rjf: interaction registers

internal DF_InteractRegs *
df_interact_regs(void)
{
  DF_InteractRegs *regs = &df_state->top_interact_regs->v;
  return regs;
}

internal DF_InteractRegs *
df_base_interact_regs(void)
{
  DF_InteractRegs *regs = &df_state->base_interact_regs.v;
  return regs;
}

internal DF_InteractRegs *
df_push_interact_regs(void)
{
  DF_InteractRegs *top = df_interact_regs();
  DF_InteractRegsNode *n = push_array(df_frame_arena(), DF_InteractRegsNode, 1);
  MemoryCopyStruct(&n->v, top);
  SLLStackPush(df_state->top_interact_regs, n);
  return &n->v;
}

internal DF_InteractRegs *
df_pop_interact_regs(void)
{
  DF_InteractRegs *regs = &df_state->top_interact_regs->v;
  SLLStackPop(df_state->top_interact_regs);
  if(df_state->top_interact_regs == 0)
  {
    df_state->top_interact_regs = &df_state->base_interact_regs;
  }
  return regs;
}

//- rjf: undo/redo history

internal DF_StateDeltaHistory *
df_state_delta_history(void)
{
  return df_state->hist;
}

//- rjf: control state

internal DF_RunKind
df_ctrl_last_run_kind(void)
{
  return df_state->ctrl_last_run_kind;
}

internal U64
df_ctrl_last_run_frame_idx(void)
{
  return df_state->ctrl_last_run_frame_idx;
}

internal U64
df_ctrl_run_gen(void)
{
  return df_state->ctrl_run_gen;
}

internal B32
df_ctrl_targets_running(void)
{
  return df_state->ctrl_is_running;
}

//- rjf: config paths

internal String8
df_cfg_path_from_src(DF_CfgSrc src)
{
  return df_state->cfg_paths[src];
}

//- rjf: config state

internal DF_CfgTable *
df_cfg_table(void)
{
  return &df_state->cfg_table;
}

//- rjf: config serialization

internal String8
df_cfg_escaped_from_raw_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  U64 split_start_idx = 0;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    U8 byte = (idx < string.size ? string.str[idx] : 0);
    if(byte == 0 || byte == '\"' || byte == '\\')
    {
      String8 part = str8_substr(string, r1u64(split_start_idx, idx));
      str8_list_push(scratch.arena, &parts, part);
      switch(byte)
      {
        default:{}break;
        case '\"':{str8_list_push(scratch.arena, &parts, str8_lit("\\\""));}break;
        case '\\':{str8_list_push(scratch.arena, &parts, str8_lit("\\\\"));}break;
      }
      split_start_idx = idx+1;
    }
  }
  StringJoin join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}

internal String8
df_cfg_raw_from_escaped_string(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List parts = {0};
  U64 split_start_idx = 0;
  U64 extra_advance = 0;
  for(U64 idx = 0; idx <= string.size; ((idx += 1+extra_advance), extra_advance=0))
  {
    U8 byte = (idx < string.size ? string.str[idx] : 0);
    if(byte == 0 || byte == '\\')
    {
      String8 part = str8_substr(string, r1u64(split_start_idx, idx));
      str8_list_push(scratch.arena, &parts, part);
      if(byte == '\\' && idx+1 < string.size)
      {
        switch(string.str[idx+1])
        {
          default:{}break;
          case '"': {extra_advance = 1; str8_list_push(scratch.arena, &parts, str8_lit("\""));}break;
          case '\\':{extra_advance = 1; str8_list_push(scratch.arena, &parts, str8_lit("\\"));}break;
        }
      }
      split_start_idx = idx+1+extra_advance;
    }
  }
  StringJoin join = {0};
  String8 result = str8_list_join(arena, &parts, &join);
  scratch_end(scratch);
  return result;
}

internal String8List
df_cfg_strings_from_core(Arena *arena, String8 root_path, DF_CfgSrc source)
{
  ProfBeginFunction();
  local_persist char *spaces = "                                                                                ";
  local_persist char *slashes= "////////////////////////////////////////////////////////////////////////////////";
  String8List strs = {0};
  
  //- rjf: write all entities
  {
    for(EachEnumVal(DF_EntityKind, k))
    {
      DF_EntityKindFlags k_flags = df_g_entity_kind_flags_table[k];
      if(!(k_flags & DF_EntityKindFlag_IsSerializedToConfig))
      {
        continue;
      }
      B32 first = 1;
      DF_EntityList entities = df_query_cached_entity_list_with_kind(k);
      for(DF_EntityNode *n = entities.first; n != 0; n = n->next)
      {
        DF_Entity *entity = n->entity;
        if(entity->cfg_src != source)
        {
          continue;
        }
        if(first)
        {
          first = 0;
          String8 title_name = df_g_entity_kind_name_lower_plural_table[k];
          str8_list_pushf(arena, &strs, "/// %S %.*s\n\n",
                          title_name,
                          (int)Max(0, 79 - (title_name.size + 5)),
                          slashes);
        }
        DF_EntityRec rec = {0};
        S64 depth = 0;
        for(DF_Entity *e = entity; !df_entity_is_nil(e); e = rec.next)
        {
          //- rjf: get next iteration
          rec = df_entity_rec_df_pre(e, entity);
          
          //- rjf: unpack entity info
          typedef U32 EntityInfoFlags;
          enum
          {
            EntityInfoFlag_HasName     = (1<<0),
            EntityInfoFlag_HasDisabled = (1<<1),
            EntityInfoFlag_HasTxtPt    = (1<<2),
            EntityInfoFlag_HasVAddr    = (1<<3),
            EntityInfoFlag_HasColor    = (1<<4),
            EntityInfoFlag_HasChildren = (1<<5),
          };
          String8 entity_name_escaped = e->name;
          if(df_g_entity_kind_flags_table[e->kind] & DF_EntityKindFlag_NameIsPath)
          {
            Temp scratch = scratch_begin(&arena, 1);
            String8 path_normalized = path_normalized_from_string(scratch.arena, e->name);
            entity_name_escaped = path_relative_dst_from_absolute_dst_src(arena, path_normalized, root_path);
            scratch_end(scratch);
          }
          else
          {
            entity_name_escaped = df_cfg_escaped_from_raw_string(arena, e->name);
          }
          EntityInfoFlags info_flags = 0;
          if(entity_name_escaped.size != 0)         { info_flags |= EntityInfoFlag_HasName; }
          if(!!e->disabled)                         { info_flags |= EntityInfoFlag_HasDisabled; }
          if(e->flags & DF_EntityFlag_HasTextPoint) { info_flags |= EntityInfoFlag_HasTxtPt; }
          if(e->flags & DF_EntityFlag_HasVAddr)     { info_flags |= EntityInfoFlag_HasVAddr; }
          if(e->flags & DF_EntityFlag_HasColor)     { info_flags |= EntityInfoFlag_HasColor; }
          if(!df_entity_is_nil(e->first))           { info_flags |= EntityInfoFlag_HasChildren; }
          
          //- rjf: write entity info
          B32 opened_brace = 0;
          switch(info_flags)
          {
            //- rjf: default path -> entity has lots of stuff, so write all info generically
            default:
            {
              opened_brace = 1;
              
              // rjf: write entity title
              str8_list_pushf(arena, &strs, "%S:\n{\n", df_g_entity_kind_name_lower_table[e->kind]);
              
              // rjf: write this entity's info
              if(entity_name_escaped.size != 0)
              {
                str8_list_pushf(arena, &strs, "name: \"%S\"\n", entity_name_escaped);
              }
              if(e->disabled)
              {
                str8_list_pushf(arena, &strs, "disabled: 1\n");
              }
              if(e->flags & DF_EntityFlag_HasColor)
              {
                Vec4F32 hsva = df_hsva_from_entity(e);
                Vec4F32 rgba = rgba_from_hsva(hsva);
                U32 rgba_hex = u32_from_rgba(rgba);
                str8_list_pushf(arena, &strs, "color: 0x%x\n", rgba_hex);
              }
              if(e->flags & DF_EntityFlag_HasTextPoint)
              {
                str8_list_pushf(arena, &strs, "line: %I64d\n", e->text_point.line);
              }
              if(e->flags & DF_EntityFlag_HasVAddr)
              {
                str8_list_pushf(arena, &strs, "vaddr: (0x%I64x)\n", e->vaddr);
              }
            }break;
            
            //- rjf: single-line fast-paths
            case EntityInfoFlag_HasName:
            {str8_list_pushf(arena, &strs, "%S: \"%S\"\n", df_g_entity_kind_name_lower_table[e->kind], entity_name_escaped);}break;
            case EntityInfoFlag_HasName|EntityInfoFlag_HasTxtPt:
            {str8_list_pushf(arena, &strs, "%S: (\"%S\":%I64d)\n", df_g_entity_kind_name_lower_table[e->kind], entity_name_escaped, e->text_point.line);}break;
            case EntityInfoFlag_HasVAddr:
            {str8_list_pushf(arena, &strs, "%S: (0x%I64x)\n", df_g_entity_kind_name_lower_table[e->kind], e->vaddr);}break;
            
            //- rjf: empty
            case 0:
            {}break;
          }
          
          // rjf: push
          depth += rec.push_count;
          
          // rjf: pop
          if(rec.push_count == 0)
          {
            for(S64 pop_idx = 0; pop_idx < rec.pop_count + opened_brace; pop_idx += 1)
            {
              if(depth > 0)
              {
                depth -= 1;
              }
              str8_list_pushf(arena, &strs, "}\n");
            }
          }
          
          // rjf: separate top-level entities with extra newline
          if(df_entity_is_nil(rec.next) && (rec.pop_count != 0 || n->next == 0))
          {
            str8_list_pushf(arena, &strs, "\n");
          }
        }
      }
    }
  }
  
  //- rjf: write exception code filters
  if(source == DF_CfgSrc_Project)
  {
    str8_list_push(arena, &strs, str8_lit("/// exception code filters ////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    str8_list_push(arena, &strs, str8_lit("exception_code_filters:\n"));
    str8_list_push(arena, &strs, str8_lit("{\n"));
    for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)(CTRL_ExceptionCodeKind_Null+1);
        k < CTRL_ExceptionCodeKind_COUNT;
        k = (CTRL_ExceptionCodeKind)(k+1))
    {
      String8 name = ctrl_exception_code_kind_lowercase_code_string_table[k];
      B32 value = !!(df_state->ctrl_exception_code_filters[k/64] & (1ull<<(k%64)));
      str8_list_pushf(arena, &strs, "  %S: %i\n", name, value);
    }
    str8_list_push(arena, &strs, str8_lit("}\n\n"));
  }
  
  //- rjf: write eval view cache
#if 0
  if(source == DF_CfgSrc_Project)
  {
    B32 first = 1;
    for(U64 eval_view_slot_idx = 0;
        eval_view_slot_idx < df_state->eval_view_cache.slots_count;
        eval_view_slot_idx += 1)
    {
      for(DF_EvalView *ev = df_state->eval_view_cache.slots[idx].first;
          ev != &df_g_nil_eval_view && ev != 0;
          ev = ev->hash_next)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// eval view state ///////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        str8_list_push(arena, &strs, str8_lit("eval_view:\n"));
        str8_list_push(arena, &strs, str8_lit("{\n"));
        str8_list_pushf(arena, &strs, "  key: (%I64x, %I64x)\n", ev->key.u64[0], ev->key.u64[1]);
        for(U64 expand_slot_idx = 0;
            expand_slot_idx < ev->expand_tree_table.slots_count;
            expand_slot_idx += 1)
        {
          for(DF_ExpandNode *expand_node = ev->expand_tree_table.slots[expand_slot_idx].first;
              expand_node != 0;
              expand_node = expand_node->hash_next)
          {
            DF_ExpandKey key = expand_node->key;
            B32 expanded = expand_node->expanded;
            str8_list_pushf(arena, &strs, "  node: ()\n");
          }
        }
        str8_list_push(arena, &strs, str8_lit("}\n\n"));
      }
    }
  }
#endif
  
  ProfEnd();
  return strs;
}

internal void
df_cfg_push_write_string(DF_CfgSrc src, String8 string)
{
  str8_list_push(df_state->cfg_write_arenas[src], &df_state->cfg_write_data[src], push_str8_copy(df_state->cfg_write_arenas[src], string));
}

//- rjf: current path

internal String8
df_current_path(void)
{
  return df_state->current_path;
}

//- rjf: entity kind cache

internal DF_EntityList
df_query_cached_entity_list_with_kind(DF_EntityKind kind)
{
  ProfBeginFunction();
  DF_EntityListCache *cache = &df_state->kind_caches[kind];
  
  // rjf: build cached list if we're out-of-date
  if(cache->alloc_gen != df_state->kind_alloc_gens[kind])
  {
    cache->alloc_gen = df_state->kind_alloc_gens[kind];
    if(cache->arena == 0)
    {
      cache->arena = arena_alloc();
    }
    arena_clear(cache->arena);
    cache->list = df_push_entity_list_with_kind(cache->arena, kind);
  }
  
  // rjf: grab & return cached list
  DF_EntityList result = cache->list;
  ProfEnd();
  return result;
}

//- rjf: active entity based queries

internal DI_KeyList
df_push_active_dbgi_key_list(Arena *arena)
{
  DI_KeyList dbgis = {0};
  DF_EntityList modules = df_query_cached_entity_list_with_kind(DF_EntityKind_Module);
  for(DF_EntityNode *n = modules.first; n != 0; n = n->next)
  {
    DF_Entity *module = n->entity;
    DI_Key key = df_dbgi_key_from_module(module);
    di_key_list_push(arena, &dbgis, &key);
  }
  return dbgis;
}

internal DF_EntityList
df_push_active_target_list(Arena *arena)
{
  DF_EntityList active_targets = {0};
  DF_EntityList all_targets = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
  for(DF_EntityNode *n = all_targets.first; n != 0; n = n->next)
  {
    if(!n->entity->disabled)
    {
      df_entity_list_push(arena, &active_targets, n->entity);
    }
  }
  return active_targets;
}

//- rjf: expand key based entity queries

internal DF_Entity *
df_entity_from_expand_key_and_kind(DF_ExpandKey key, DF_EntityKind kind)
{
  DF_Entity *result = &df_g_nil_entity;
  DF_EntityList list = df_query_cached_entity_list_with_kind(kind);
  for(DF_EntityNode *n = list.first; n != 0; n = n->next)
  {
    DF_Entity *entity = n->entity;
    if(df_expand_key_match(df_expand_key_from_entity(entity), key))
    {
      result = entity;
      break;
    }
  }
  return result;
}

//- rjf: per-run caches

internal CTRL_Unwind
df_query_cached_unwind_from_thread(DF_Entity *thread)
{
  Temp scratch = scratch_begin(0, 0);
  CTRL_Unwind result = {0};
  if(thread->kind == DF_EntityKind_Thread)
  {
    U64 reg_gen = ctrl_reg_gen();
    U64 mem_gen = ctrl_mem_gen();
    DF_UnwindCache *cache = &df_state->unwind_cache;
    DF_Handle handle = df_handle_from_entity(thread);
    U64 hash = df_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash%cache->slots_count;
    DF_UnwindCacheSlot *slot = &cache->slots[slot_idx];
    DF_UnwindCacheNode *node = 0;
    for(DF_UnwindCacheNode *n = slot->first; n != 0; n = n->next)
    {
      if(df_handle_match(handle, n->thread))
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = cache->free_node;
      if(node != 0)
      {
        SLLStackPop(cache->free_node);
      }
      else
      {
        node = push_array_no_zero(df_state->arena, DF_UnwindCacheNode, 1);
      }
      MemoryZeroStruct(node);
      DLLPushBack(slot->first, slot->last, node);
      node->arena = arena_alloc();
      node->thread = handle;
    }
    if(node->reggen != reg_gen ||
       node->memgen != mem_gen)
    {
      CTRL_Unwind new_unwind = ctrl_unwind_from_thread(scratch.arena, df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle, os_now_microseconds()+100);
      if(!(new_unwind.flags & (CTRL_UnwindFlag_Error|CTRL_UnwindFlag_Stale)) && new_unwind.frames.count != 0)
      {
        node->unwind = ctrl_unwind_deep_copy(node->arena, thread->arch, &new_unwind);
        node->reggen = reg_gen;
        node->memgen = mem_gen;
      }
    }
    result = node->unwind;
  }
  scratch_end(scratch);
  return result;
}

internal U64
df_query_cached_rip_from_thread(DF_Entity *thread)
{
  U64 result = df_query_cached_rip_from_thread_unwind(thread, 0);
  return result;
}

internal U64
df_query_cached_rip_from_thread_unwind(DF_Entity *thread, U64 unwind_count)
{
  U64 result = 0;
  if(unwind_count == 0)
  {
    result = ctrl_query_cached_rip_from_thread(df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  }
  else
  {
    CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
    if(unwind.frames.count != 0)
    {
      result = regs_rip_from_arch_block(thread->arch, unwind.frames.v[unwind_count%unwind.frames.count].regs);
    }
  }
  return result;
}

internal U64
df_query_cached_tls_base_vaddr_from_process_root_rip(DF_Entity *process, U64 root_vaddr, U64 rip_vaddr)
{
  U64 result = 0;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(df_state->tls_base_caches); cache_idx += 1)
  {
    DF_RunTLSBaseCache *cache = &df_state->tls_base_caches[(df_state->tls_base_cache_gen+cache_idx)%ArrayCount(df_state->tls_base_caches)];
    if(cache_idx == 0 && cache->slots_count == 0)
    {
      cache->slots_count = 256;
      cache->slots = push_array(cache->arena, DF_RunTLSBaseCacheSlot, cache->slots_count);
    }
    else if(cache->slots_count == 0)
    {
      break;
    }
    DF_Handle handle = df_handle_from_entity(process);
    U64 hash = df_hash_from_seed_string(df_hash_from_string(str8_struct(&handle)), str8_struct(&rip_vaddr));
    U64 slot_idx = hash%cache->slots_count;
    DF_RunTLSBaseCacheSlot *slot = &cache->slots[slot_idx];
    DF_RunTLSBaseCacheNode *node = 0;
    for(DF_RunTLSBaseCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(df_handle_match(n->process, handle) && n->root_vaddr == root_vaddr && n->rip_vaddr == rip_vaddr)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      U64 tls_base_vaddr = df_tls_base_vaddr_from_process_root_rip(process, root_vaddr, rip_vaddr);
      if(tls_base_vaddr != 0)
      {
        node = push_array(cache->arena, DF_RunTLSBaseCacheNode, 1);
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
        node->process = handle;
        node->root_vaddr = root_vaddr;
        node->rip_vaddr = rip_vaddr;
        node->tls_base_vaddr = tls_base_vaddr;
      }
    }
    if(node != 0 && node->tls_base_vaddr != 0)
    {
      result = node->tls_base_vaddr;
      break;
    }
  }
  return result;
}

internal E_String2NumMap *
df_query_cached_locals_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  E_String2NumMap *map = &e_string2num_map_nil;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(df_state->locals_caches); cache_idx += 1)
  {
    DF_RunLocalsCache *cache = &df_state->locals_caches[(df_state->locals_cache_gen+cache_idx)%ArrayCount(df_state->locals_caches)];
    if(cache_idx == 0 && cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, DF_RunLocalsCacheSlot, cache->table_size);
    }
    else if(cache->table_size == 0)
    {
      break;
    }
    U64 hash = di_hash_from_key(dbgi_key);
    U64 slot_idx = hash % cache->table_size;
    DF_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    DF_RunLocalsCacheNode *node = 0;
    for(DF_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(di_key_match(&n->dbgi_key, dbgi_key) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DI_Scope *scope = di_scope_open();
      E_String2NumMap *map = df_push_locals_map_from_dbgi_key_voff(cache->arena, scope, dbgi_key, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, DF_RunLocalsCacheNode, 1);
        node->dbgi_key = di_key_copy(cache->arena, dbgi_key);
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      di_scope_close(scope);
    }
    if(node != 0 && node->locals_map->slots_count != 0)
    {
      map = node->locals_map;
      break;
    }
  }
  ProfEnd();
  return map;
}

internal E_String2NumMap *
df_query_cached_member_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  E_String2NumMap *map = &e_string2num_map_nil;
  for(U64 cache_idx = 0; cache_idx < ArrayCount(df_state->member_caches); cache_idx += 1)
  {
    DF_RunLocalsCache *cache = &df_state->member_caches[(df_state->member_cache_gen+cache_idx)%ArrayCount(df_state->member_caches)];
    if(cache_idx == 0 && cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, DF_RunLocalsCacheSlot, cache->table_size);
    }
    else if(cache->table_size == 0)
    {
      break;
    }
    U64 hash = di_hash_from_key(dbgi_key);
    U64 slot_idx = hash % cache->table_size;
    DF_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    DF_RunLocalsCacheNode *node = 0;
    for(DF_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(di_key_match(&n->dbgi_key, dbgi_key) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DI_Scope *scope = di_scope_open();
      E_String2NumMap *map = df_push_member_map_from_dbgi_key_voff(cache->arena, scope, dbgi_key, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, DF_RunLocalsCacheNode, 1);
        node->dbgi_key = di_key_copy(cache->arena, dbgi_key);
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      di_scope_close(scope);
    }
    if(node != 0 && node->locals_map->slots_count != 0)
    {
      map = node->locals_map;
      break;
    }
  }
  ProfEnd();
  return map;
}

//- rjf: top-level command dispatch

internal void
df_push_cmd__root(DF_CmdParams *params, DF_CmdSpec *spec)
{
  // rjf: log
  if(params->os_event == 0 || params->os_event->kind != OS_EventKind_MouseMove)
  {
    Temp scratch = scratch_begin(0, 0);
    DF_Entity *entity = df_entity_from_handle(params->entity);
    log_infof("df_cmd:\n{\n", spec->info.string);
    log_infof("spec: \"%S\"\n", spec->info.string);
#define HandleParamPrint(mem_name) if(!df_handle_match(df_handle_zero(), params->mem_name)) { log_infof("%s: [0x%I64x, 0x%I64x]\n", #mem_name, params->mem_name.u64[0], params->mem_name.u64[1]); }
    HandleParamPrint(window);
    HandleParamPrint(panel);
    HandleParamPrint(dest_panel);
    HandleParamPrint(prev_view);
    HandleParamPrint(view);
    if(!df_entity_is_nil(entity))
    {
      String8 entity_name = df_display_string_from_entity(scratch.arena, entity);
      log_infof("entity: \"%S\"\n", entity_name);
    }
    U64 idx = 0;
    for(DF_HandleNode *n = params->entity_list.first; n != 0; n = n->next, idx += 1)
    {
      DF_Entity *entity = df_entity_from_handle(n->handle);
      if(!df_entity_is_nil(entity))
      {
        String8 entity_name = df_display_string_from_entity(scratch.arena, entity);
        log_infof("entity_list[%I64u]: \"%S\"\n", idx, entity_name);
      }
    }
    if(!df_cmd_spec_is_nil(params->cmd_spec))
    {
      log_infof("cmd_spec: \"%S\"\n", params->cmd_spec->info.string);
    }
    if(params->string.size != 0)        { log_infof("string: \"%S\"\n", params->string); }
    if(params->file_path.size != 0)     { log_infof("file_path: \"%S\"\n", params->file_path); }
    if(params->text_point.line != 0)    { log_infof("text_point: [line:%I64d, col:%I64d]\n", params->text_point.line, params->text_point.column); }
    if(params->vaddr != 0)              { log_infof("vaddr: 0x%I64x\n", params->vaddr); }
    if(params->voff != 0)               { log_infof("voff: 0x%I64x\n", params->voff); }
    if(params->index != 0)              { log_infof("index: 0x%I64x\n", params->index); }
    if(params->unwind_index != 0)       { log_infof("unwind_index: 0x%I64x\n", params->unwind_index); }
    if(params->inline_depth != 0)       { log_infof("inline_depth: 0x%I64x\n", params->inline_depth); }
    if(params->id != 0)                 { log_infof("id: 0x%I64x\n", params->id); }
    if(params->os_event != 0)
    {
      String8 kind_string = str8_lit("<unknown>");
      switch(params->os_event->kind)
      {
        default:{}break;
        case OS_EventKind_Press:          {kind_string = str8_lit("press");}break;
        case OS_EventKind_Release:        {kind_string = str8_lit("release");}break;
        case OS_EventKind_MouseMove:      {kind_string = str8_lit("mousemove");}break;
        case OS_EventKind_Text:           {kind_string = str8_lit("text");}break;
        case OS_EventKind_Scroll:         {kind_string = str8_lit("scroll");}break;
        case OS_EventKind_WindowLoseFocus:{kind_string = str8_lit("losefocus");}break;
        case OS_EventKind_WindowClose:    {kind_string = str8_lit("closewindow");}break;
        case OS_EventKind_FileDrop:       {kind_string = str8_lit("filedrop");}break;
        case OS_EventKind_Wakeup:         {kind_string = str8_lit("wakeup");}break;
      }
      log_infof("os_event->kind: %S\n", kind_string);
    }
#undef HandleParamPrint
    log_infof("}\n\n");
    scratch_end(scratch);
  }
  df_cmd_list_push(df_state->root_cmd_arena, &df_state->root_cmds, params, spec);
}

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void
df_core_init(CmdLine *cmdln, DF_StateDeltaHistory *hist)
{
  Arena *arena = arena_alloc();
  df_state = push_array(arena, DF_State, 1);
  df_state->arena = arena;
  for(U64 idx = 0; idx < ArrayCount(df_state->frame_arenas); idx += 1)
  {
    df_state->frame_arenas[idx] = arena_alloc();
  }
  df_state->root_cmd_arena = arena_alloc();
  df_state->output_log_key = hs_hash_from_data(str8_lit("df_output_log_key"));
  df_state->entities_arena = arena_alloc(.reserve_size = GB(64), .commit_size = KB(64));
  df_state->entities_root = &df_g_nil_entity;
  df_state->entities_base = push_array(df_state->entities_arena, DF_Entity, 0);
  df_state->entities_count = 0;
  df_state->ctrl_msg_arena = arena_alloc();
  df_state->ctrl_entity_store = ctrl_entity_store_alloc();
  df_state->ctrl_stop_arena = arena_alloc();
  df_state->entities_root = df_entity_alloc(&df_g_nil_entity, DF_EntityKind_Root);
  df_state->cmd_spec_table_size = 1024;
  df_state->cmd_spec_table = push_array(arena, DF_CmdSpec *, df_state->cmd_spec_table_size);
  df_state->view_rule_spec_table_size = 1024;
  df_state->view_rule_spec_table = push_array(arena, DF_CoreViewRuleSpec *, df_state->view_rule_spec_table_size);
  df_state->seconds_til_autosave = 0.5f;
  df_state->hist = hist;
  
  // rjf: set up initial exception filtering rules
  for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)0; k < CTRL_ExceptionCodeKind_COUNT; k = (CTRL_ExceptionCodeKind)(k+1))
  {
    if(ctrl_exception_code_kind_default_enable_table[k])
    {
      df_state->ctrl_exception_code_filters[k/64] |= 1ull<<(k%64);
    }
  }
  
  // rjf: set up initial entities
  {
    DF_Entity *local_machine = df_entity_alloc(df_state->entities_root, DF_EntityKind_Machine);
    df_entity_equip_ctrl_machine_id(local_machine, CTRL_MachineID_Local);
    df_entity_equip_name(local_machine, str8_lit("This PC"));
  }
  
  // rjf: register core commands
  {
    DF_CmdSpecInfoArray array = {df_g_core_cmd_kind_spec_info_table, ArrayCount(df_g_core_cmd_kind_spec_info_table)};
    df_register_cmd_specs(array);
  }
  
  // rjf: register core view rules
  {
    DF_CoreViewRuleSpecInfoArray array = {df_g_core_view_rule_spec_info_table, ArrayCount(df_g_core_view_rule_spec_info_table)};
    df_register_core_view_rule_specs(array);
  }
  
  // rjf: set up caches
  df_state->unwind_cache.slots_count = 1024;
  df_state->unwind_cache.slots = push_array(arena, DF_UnwindCacheSlot, df_state->unwind_cache.slots_count);
  for(U64 idx = 0; idx < ArrayCount(df_state->tls_base_caches); idx += 1)
  {
    df_state->tls_base_caches[idx].arena = arena_alloc();
  }
  for(U64 idx = 0; idx < ArrayCount(df_state->locals_caches); idx += 1)
  {
    df_state->locals_caches[idx].arena = arena_alloc();
  }
  for(U64 idx = 0; idx < ArrayCount(df_state->member_caches); idx += 1)
  {
    df_state->member_caches[idx].arena = arena_alloc();
  }
  
  // rjf: set up eval view cache
  df_state->eval_view_cache.slots_count = 4096;
  df_state->eval_view_cache.slots = push_array(arena, DF_EvalViewSlot, df_state->eval_view_cache.slots_count);
  
  // rjf: set up run state
  df_state->ctrl_last_run_arena = arena_alloc();
  
  // rjf: set up config reading state
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: unpack command line arguments
    String8 user_cfg_path = cmd_line_string(cmdln, str8_lit("user"));
    String8 project_cfg_path = cmd_line_string(cmdln, str8_lit("project"));
    if(project_cfg_path.size == 0)
    {
      project_cfg_path = cmd_line_string(cmdln, str8_lit("profile"));
    }
    {
      String8 user_program_data_path = os_get_process_info()->user_program_data_path;
      String8 user_data_folder = push_str8f(scratch.arena, "%S/%S", user_program_data_path, str8_lit("raddbg"));
      os_make_directory(user_data_folder);
      if(user_cfg_path.size == 0)
      {
        user_cfg_path = push_str8f(scratch.arena, "%S/default.raddbg_user", user_data_folder);
      }
      if(project_cfg_path.size == 0)
      {
        project_cfg_path = push_str8f(scratch.arena, "%S/default.raddbg_project", user_data_folder);
      }
    }
    
    // rjf: set up config path state
    String8 cfg_src_paths[DF_CfgSrc_COUNT] = {user_cfg_path, project_cfg_path};
    for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
    {
      df_state->cfg_path_arenas[src] = arena_alloc();
      DF_CmdParams params = df_cmd_params_zero();
      params.file_path = path_normalized_from_string(scratch.arena, cfg_src_paths[src]);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(df_g_cfg_src_load_cmd_kind_table[src]));
    }
    
    // rjf: set up config table arena
    df_state->cfg_arena = arena_alloc();
    scratch_end(scratch);
  }
  
  // rjf: set up config write state
  for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
  {
    df_state->cfg_write_arenas[src] = arena_alloc();
  }
  
  // rjf: set up initial browse path
  {
    Temp scratch = scratch_begin(0, 0);
    String8 current_path = os_get_current_path(scratch.arena);
    String8 current_path_with_slash = push_str8f(scratch.arena, "%S/", current_path);
    df_state->current_path_arena = arena_alloc();
    df_state->current_path = push_str8_copy(df_state->current_path_arena, current_path_with_slash);
    scratch_end(scratch);
  }
}

internal DF_CmdList
df_core_gather_root_cmds(Arena *arena)
{
  DF_CmdList cmds = {0};
  for(DF_CmdNode *n = df_state->root_cmds.first; n != 0; n = n->next)
  {
    df_cmd_list_push(arena, &cmds, &n->cmd.params, n->cmd.spec);
  }
  return cmds;
}

internal void
df_core_begin_frame(Arena *arena, DF_CmdList *cmds, F32 dt)
{
  ProfBeginFunction();
  df_state->frame_index += 1;
  arena_clear(df_frame_arena());
  df_state->frame_di_scope = di_scope_open();
  df_state->frame_eval_memread_endt_us = os_now_microseconds() + 5000;
  df_state->dt = dt;
  df_state->time_in_seconds += dt;
  df_state->top_interact_regs = &df_state->base_interact_regs;
  df_state->top_interact_regs->v.file_path = push_str8_copy(df_frame_arena(), df_state->top_interact_regs->v.file_path);
  df_state->top_interact_regs->v.lines = df_line_list_copy(df_frame_arena(), &df_state->top_interact_regs->v.lines);
  df_state->top_interact_regs->v.dbgi_key = di_key_copy(df_frame_arena(), &df_state->top_interact_regs->v.dbgi_key);
  
  //- rjf: sync with ctrl thread
  ProfScope("sync with ctrl thread")
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: grab next reggen/memgen
    U64 new_mem_gen = ctrl_mem_gen();
    U64 new_reg_gen = ctrl_reg_gen();
    
    //- rjf: consume & process events
    CTRL_EventList events = ctrl_c2u_pop_events(scratch.arena);
    ctrl_entity_store_apply_events(df_state->ctrl_entity_store, &events);
    for(CTRL_EventNode *event_n = events.first; event_n != 0; event_n = event_n->next)
    {
      CTRL_Event *event = &event_n->v;
      log_infof("ctrl_event:\n{\n");
      log_infof("kind: \"%S\"\n", ctrl_string_from_event_kind(event->kind));
      log_infof("entity_id: %u\n", event->entity_id);
      switch(event->kind)
      {
        default:{}break;
        
        //- rjf: errors
        
        case CTRL_EventKind_Error:
        {
          log_user_error(event->string);
        }break;
        
        //- rjf: starts/stops
        
        case CTRL_EventKind_Started:
        {
          df_state->ctrl_is_running = 1;
        }break;
        
        case CTRL_EventKind_Stopped:
        {
          B32 should_snap = !(df_state->ctrl_soft_halt_issued);
          df_state->ctrl_is_running = 0;
          df_state->ctrl_soft_halt_issued = 0;
          DF_Entity *stop_thread = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          
          // rjf: gather stop info
          {
            arena_clear(df_state->ctrl_stop_arena);
            MemoryCopyStruct(&df_state->ctrl_last_stop_event, event);
            df_state->ctrl_last_stop_event.string = push_str8_copy(df_state->ctrl_stop_arena, df_state->ctrl_last_stop_event.string);
          }
          
          // rjf: select & snap to thread causing stop
          if(should_snap && stop_thread->kind == DF_EntityKind_Thread)
          {
            log_infof("stop_thread: \"%S\"\n", df_display_string_from_entity(scratch.arena, stop_thread));
            DF_CmdParams params = df_cmd_params_zero();
            params.entity = df_handle_from_entity(stop_thread);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
          }
          
          // rjf: if no stop-causing thread, and if selected thread, snap to selected
          if(should_snap && df_entity_is_nil(stop_thread))
          {
            DF_Entity *selected_thread = df_entity_from_handle(df_interact_regs()->thread);
            if(!df_entity_is_nil(selected_thread))
            {
              DF_CmdParams params = df_cmd_params_zero();
              params.entity = df_handle_from_entity(selected_thread);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindThread));
            }
          }
          
          // rjf: thread hit user breakpoint -> increment breakpoint hit count
          if(should_snap && event->cause == CTRL_EventCause_UserBreakpoint)
          {
            U64 stop_thread_vaddr = ctrl_query_cached_rip_from_thread(df_state->ctrl_entity_store, stop_thread->ctrl_machine_id, stop_thread->ctrl_handle);
            DF_Entity *process = df_entity_ancestor_from_kind(stop_thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, stop_thread_vaddr);
            DI_Key dbgi_key = df_dbgi_key_from_module(module);
            U64 stop_thread_voff = df_voff_from_vaddr(module, stop_thread_vaddr);
            DF_EntityList user_bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
            for(DF_EntityNode *n = user_bps.first; n != 0; n = n->next)
            {
              DF_Entity *bp = n->entity;
              DF_Entity *loc = df_entity_child_from_kind(bp, DF_EntityKind_Location);
              DF_LineList loc_lines = df_lines_from_file_path_line_num(scratch.arena, loc->name, loc->text_point.line);
              if(loc_lines.first != 0)
              {
                for(DF_LineNode *n = loc_lines.first; n != 0; n = n->next)
                {
                  if(contains_1u64(n->v.voff_range, stop_thread_voff))
                  {
                    bp->u64 += 1;
                    break;
                  }
                }
              }
              else if(loc->flags & DF_EntityFlag_HasVAddr && stop_thread_vaddr == loc->vaddr)
              {
                bp->u64 += 1;
              }
              else if(loc->name.size != 0)
              {
                U64 symb_voff = df_voff_from_dbgi_key_symbol_name(&dbgi_key, loc->name);
                if(symb_voff == stop_thread_voff)
                {
                  bp->u64 += 1;
                }
              }
            }
          }
          
          // rjf: exception or unexpected trap -> push error
          if(event->cause == CTRL_EventCause_InterruptedByException ||
             event->cause == CTRL_EventCause_InterruptedByTrap)
          {
            DF_CmdParams params = df_cmd_params_zero();
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
          
          // rjf: kill all entities which are marked to die on stop
          {
            DF_Entity *request = df_entity_from_id(event->msg_id);
            if(df_entity_is_nil(request))
            {
              for(DF_Entity *entity = df_entity_root();
                  !df_entity_is_nil(entity);
                  entity = df_entity_rec_df_pre(entity, df_entity_root()).next)
              {
                if(entity->flags & DF_EntityFlag_DiesOnRunStop)
                {
                  df_entity_mark_for_deletion(entity);
                }
              }
            }
          }
        }break;
        
        //- rjf: entity creation/deletion
        
        case CTRL_EventKind_NewProc:
        {
          // rjf: the first process? -> clear session output & reset all bp hit counts
          DF_EntityList existing_processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          if(existing_processes.count == 0)
          {
            MTX_Op op = {r1u64(0, 0xffffffffffffffffull), str8_lit("[new session]\n")};
            mtx_push_op(df_state->output_log_key, op);
            DF_EntityList bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
            for(DF_EntityNode *n = bps.first; n != 0; n = n->next)
            {
              n->entity->u64 = 0;
            }
          }
          
          // rjf: create entity
          DF_Entity *machine = df_machine_entity_from_machine_id(event->machine_id);
          DF_Entity *entity = df_entity_alloc(machine, DF_EntityKind_Process);
          df_entity_equip_u64(entity, event->msg_id);
          df_entity_equip_ctrl_machine_id(entity, event->machine_id);
          df_entity_equip_ctrl_handle(entity, event->entity);
          df_entity_equip_ctrl_id(entity, event->entity_id);
          df_entity_equip_arch(entity, event->arch);
        }break;
        
        case CTRL_EventKind_NewThread:
        {
          // rjf: create entity
          DF_Entity *parent = df_entity_from_ctrl_handle(event->machine_id, event->parent);
          DF_Entity *entity = df_entity_alloc(parent, DF_EntityKind_Thread);
          df_entity_equip_ctrl_machine_id(entity, event->machine_id);
          df_entity_equip_ctrl_handle(entity, event->entity);
          df_entity_equip_arch(entity, event->arch);
          df_entity_equip_ctrl_id(entity, event->entity_id);
          df_entity_equip_stack_base(entity, event->stack_base);
          df_entity_equip_vaddr(entity, event->rip_vaddr);
          if(event->string.size != 0)
          {
            df_entity_equip_name(entity, event->string);
          }
          
          // rjf: find any pending thread names correllating with this TID -> equip name if found match
          {
            DF_EntityList pending_thread_names = df_query_cached_entity_list_with_kind(DF_EntityKind_PendingThreadName);
            for(DF_EntityNode *n = pending_thread_names.first; n != 0; n = n->next)
            {
              DF_Entity *pending_thread_name = n->entity;
              if(event->machine_id == pending_thread_name->ctrl_machine_id && event->entity_id == pending_thread_name->ctrl_id)
              {
                df_entity_mark_for_deletion(pending_thread_name);
                df_entity_equip_name(entity, pending_thread_name->name);
                break;
              }
            }
          }
          
          // rjf: determine index in process
          U64 thread_idx_in_process = 0;
          for(DF_Entity *child = parent->first; !df_entity_is_nil(child); child = child->next)
          {
            if(child == entity)
            {
              break;
            }
            if(child->kind == DF_EntityKind_Thread)
            {
              thread_idx_in_process += 1;
            }
          }
          
          // rjf: build default thread color table
          Vec4F32 thread_colors[] =
          {
            df_rgba_from_theme_color(DF_ThemeColor_Thread0),
            df_rgba_from_theme_color(DF_ThemeColor_Thread1),
            df_rgba_from_theme_color(DF_ThemeColor_Thread2),
            df_rgba_from_theme_color(DF_ThemeColor_Thread3),
            df_rgba_from_theme_color(DF_ThemeColor_Thread4),
            df_rgba_from_theme_color(DF_ThemeColor_Thread5),
            df_rgba_from_theme_color(DF_ThemeColor_Thread6),
            df_rgba_from_theme_color(DF_ThemeColor_Thread7),
          };
          
          // rjf: pick color
          Vec4F32 thread_color = thread_colors[thread_idx_in_process % ArrayCount(thread_colors)];
          
          // rjf: equip color
          df_entity_equip_color_rgba(entity, thread_color);
          
          // rjf: automatically select if we don't have a selected thread
          DF_Entity *selected_thread = df_entity_from_handle(df_state->base_interact_regs.v.thread);
          if(df_entity_is_nil(selected_thread))
          {
            df_state->base_interact_regs.v.thread = df_handle_from_entity(entity);
          }
          
          // rjf: do initial snap
          DF_EntityList already_existing_processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          B32 do_initial_snap = (already_existing_processes.count == 1 && thread_idx_in_process == 0);
          if(do_initial_snap)
          {
            DF_CmdParams params = df_cmd_params_zero();
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
          }
        }break;
        
        case CTRL_EventKind_NewModule:
        {
          // rjf: grab process
          DF_Entity *parent = df_entity_from_ctrl_handle(event->machine_id, event->parent);
          
          // rjf: determine if this is the first module
          B32 is_first = 0;
          if(df_entity_is_nil(df_entity_child_from_kind(parent, DF_EntityKind_Module)))
          {
            is_first = 1;
          }
          
          // rjf: create module entity
          DF_Entity *module = df_entity_alloc(parent, DF_EntityKind_Module);
          df_entity_equip_ctrl_machine_id(module, event->machine_id);
          df_entity_equip_ctrl_handle(module, event->entity);
          df_entity_equip_arch(module, event->arch);
          df_entity_equip_name(module, event->string);
          df_entity_equip_vaddr_rng(module, event->vaddr_rng);
          df_entity_equip_vaddr(module, event->rip_vaddr);
          df_entity_equip_timestamp(module, event->timestamp);
          
          // rjf: is first -> find target, equip process & module & first thread with target color
          if(is_first)
          {
            DF_EntityList targets = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
            for(DF_EntityNode *n = targets.first; n != 0; n = n->next)
            {
              DF_Entity *target = n->entity;
              DF_Entity *exe = df_entity_child_from_kind(target, DF_EntityKind_Executable);
              String8 exe_name = exe->name;
              String8 exe_name_normalized = path_normalized_from_string(scratch.arena, exe_name);
              String8 module_name_normalized = path_normalized_from_string(scratch.arena, module->name);
              if(str8_match(exe_name_normalized, module_name_normalized, StringMatchFlag_CaseInsensitive) &&
                 target->flags & DF_EntityFlag_HasColor)
              {
                DF_Entity *first_thread = df_entity_child_from_kind(parent, DF_EntityKind_Thread);
                Vec4F32 rgba = df_rgba_from_entity(target);
                df_entity_equip_color_rgba(parent, rgba);
                df_entity_equip_color_rgba(first_thread, rgba);
                df_entity_equip_color_rgba(module, rgba);
                break;
              }
            }
          }
        }break;
        
        case CTRL_EventKind_EndProc:
        {
          U32 pid = event->entity_id;
          DF_Entity *process = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          df_entity_mark_for_deletion(process);
        }break;
        
        case CTRL_EventKind_EndThread:
        {
          DF_Entity *thread = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          df_set_thread_freeze_state(thread, 0);
          df_entity_mark_for_deletion(thread);
        }break;
        
        case CTRL_EventKind_EndModule:
        {
          DF_Entity *module = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          df_entity_mark_for_deletion(module);
        }break;
        
        //- rjf: debug info changes
        
        case CTRL_EventKind_ModuleDebugInfoPathChange:
        {
          DF_Entity *module = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          DF_Entity *debug_info = df_entity_child_from_kind(module, DF_EntityKind_DebugInfoPath);
          if(df_entity_is_nil(debug_info))
          {
            debug_info = df_entity_alloc(module, DF_EntityKind_DebugInfoPath);
          }
          df_entity_equip_name(debug_info, event->string);
          df_entity_equip_timestamp(debug_info, event->timestamp);
        }break;
        
        //- rjf: debug strings
        
        case CTRL_EventKind_DebugString:
        {
          MTX_Op op = {r1u64(max_U64, max_U64), event->string};
          mtx_push_op(df_state->output_log_key, op);
        }break;
        
        case CTRL_EventKind_ThreadName:
        {
          String8 string = event->string;
          DF_Entity *entity = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          if(event->entity_id != 0)
          {
            entity = df_entity_from_ctrl_id(event->machine_id, event->entity_id);
          }
          if(df_entity_is_nil(entity))
          {
            DF_Entity *process = df_entity_from_ctrl_handle(event->machine_id, event->parent);
            if(!df_entity_is_nil(process))
            {
              entity = df_entity_alloc(process, DF_EntityKind_PendingThreadName);
              df_entity_equip_name(entity, string);
              df_entity_equip_ctrl_machine_id(entity, event->machine_id);
              df_entity_equip_ctrl_id(entity, event->entity_id);
            }
          }
          if(!df_entity_is_nil(entity))
          {
            df_entity_equip_name(entity, string);
          }
        }break;
        
        //- rjf: memory
        
        case CTRL_EventKind_MemReserve:{}break;
        case CTRL_EventKind_MemCommit:{}break;
        case CTRL_EventKind_MemDecommit:{}break;
        case CTRL_EventKind_MemRelease:{}break;
      }
      log_infof("}\n\n");
    }
    
    //- rjf: clear tls base cache
    if((df_state->tls_base_cache_reggen_idx != new_reg_gen ||
        df_state->tls_base_cache_memgen_idx != new_mem_gen) &&
       !df_ctrl_targets_running())
    {
      df_state->tls_base_cache_gen += 1;
      DF_RunTLSBaseCache *cache = &df_state->tls_base_caches[df_state->tls_base_cache_gen%ArrayCount(df_state->tls_base_caches)];
      arena_clear(cache->arena);
      cache->slots_count = 0;
      cache->slots = 0;
      df_state->tls_base_cache_reggen_idx = new_reg_gen;
      df_state->tls_base_cache_memgen_idx = new_mem_gen;
    }
    
    //- rjf: clear locals cache
    if(df_state->locals_cache_reggen_idx != new_reg_gen &&
       !df_ctrl_targets_running())
    {
      df_state->locals_cache_gen += 1;
      DF_RunLocalsCache *cache = &df_state->locals_caches[df_state->locals_cache_gen%ArrayCount(df_state->locals_caches)];
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      df_state->locals_cache_reggen_idx = new_reg_gen;
    }
    
    //- rjf: clear members cache
    if(df_state->member_cache_reggen_idx != new_reg_gen &&
       !df_ctrl_targets_running())
    {
      df_state->member_cache_gen += 1;
      DF_RunLocalsCache *cache = &df_state->member_caches[df_state->member_cache_gen%ArrayCount(df_state->member_caches)];
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      df_state->member_cache_reggen_idx = new_reg_gen;
    }
    
    scratch_end(scratch);
  }
  
  //- rjf: sync with di parsers
  ProfScope("sync with di parsers")
  {
    Temp scratch = scratch_begin(&arena, 1);
    DI_EventList events = di_p2u_pop_events(scratch.arena, 0);
    for(DI_EventNode *n = events.first; n != 0; n = n->next)
    {
      DI_Event *event = &n->v;
      switch(event->kind)
      {
        default:{}break;
        case DI_EventKind_ConversionStarted:
        {
          DF_Entity *task = df_entity_alloc(df_entity_root(), DF_EntityKind_ConversionTask);
          df_entity_equip_name(task, event->string);
        }break;
        case DI_EventKind_ConversionEnded:
        {
          DF_Entity *task = df_entity_from_name_and_kind(event->string, DF_EntityKind_ConversionTask);
          if(!df_entity_is_nil(task))
          {
            df_entity_mark_for_deletion(task);
          }
        }break;
      }
    }
    scratch_end(scratch);
  }
  
  //- rjf: start/stop telemetry captures
  ProfScope("start/stop telemetry captures")
  {
    if(!ProfIsCapturing() && DEV_telemetry_capture)
    {
      ProfBeginCapture("raddbg");
    }
    if(ProfIsCapturing() && !DEV_telemetry_capture)
    {
      ProfEndCapture();
    }
  }
  
  //- rjf: clear root level commands
  {
    arena_clear(df_state->root_cmd_arena);
    MemoryZeroStruct(&df_state->root_cmds);
  }
  
  //- rjf: autosave
  {
    df_state->seconds_til_autosave -= dt;
    if(df_state->seconds_til_autosave <= 0.f)
    {
      DF_CmdParams params = df_cmd_params_zero();
      df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteUserData));
      df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProjectData));
      df_state->seconds_til_autosave = 5.f;
    }
  }
  
  //- rjf: process top-level commands
  ProfScope("process top-level commands")
  {
    Temp scratch = scratch_begin(&arena, 1);
    for(DF_CmdNode *cmd_node = cmds->first;
        cmd_node != 0;
        cmd_node = cmd_node->next)
    {
      temp_end(scratch);
      
      // rjf: unpack command
      DF_Cmd *cmd = &cmd_node->cmd;
      DF_CmdParams params = cmd->params;
      DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
      df_cmd_spec_counter_inc(cmd->spec);
      
      // rjf: process command
      switch(core_cmd_kind)
      {
        default:{}break;
        
        //- rjf: command fast paths
        case DF_CoreCmdKind_RunCommand:
        {
          DF_CmdSpec *spec = params.cmd_spec;
          if(spec != cmd->spec)
          {
            df_cmd_spec_counter_inc(spec);
            if(!(spec->info.query.flags & DF_CmdQueryFlag_Required) &&
               (spec->info.query.slot == DF_CmdParamSlot_Null ||
                df_cmd_params_has_slot(&params, spec->info.query.slot)))
            {
              df_cmd_list_push(arena, cmds, &params, spec);
            }
          }
        }break;
        
        //- rjf: low-level target control operations
        case DF_CoreCmdKind_LaunchAndRun:
        case DF_CoreCmdKind_LaunchAndInit:
        {
          // rjf: get list of targets to launch
          DF_EntityList targets = df_entity_list_from_handle_list(scratch.arena, params.entity_list);
          
          // rjf: no targets => assume all active targets
          if(targets.count == 0)
          {
            targets = df_push_active_target_list(scratch.arena);
          }
          
          // rjf: launch
          if(targets.count != 0)
          {
            for(DF_EntityNode *n = targets.first; n != 0; n = n->next)
            {
              // rjf: extract data from target
              DF_Entity *target = n->entity;
              String8 name = df_entity_child_from_kind(target, DF_EntityKind_Executable)->name;
              String8 args = df_entity_child_from_kind(target, DF_EntityKind_Arguments)->name;
              String8 path = df_entity_child_from_kind(target, DF_EntityKind_WorkingDirectory)->name;
              String8 entry= df_entity_child_from_kind(target, DF_EntityKind_EntryPoint)->name;
              name  = str8_skip_chop_whitespace(name);
              args  = str8_skip_chop_whitespace(args);
              path  = str8_skip_chop_whitespace(path);
              entry = str8_skip_chop_whitespace(entry);
              if(path.size == 0)
              {
                path = os_get_current_path(scratch.arena);
              }
              
              // rjf: build launch options
              String8List cmdln_strings = {0};
              {
                str8_list_push(scratch.arena, &cmdln_strings, name);
                {
                  U64 start_split_idx = 0;
                  B32 quoted = 0;
                  for(U64 idx = 0; idx <= args.size; idx += 1)
                  {
                    U8 byte = idx < args.size ? args.str[idx] : 0;
                    if(byte == '"')
                    {
                      quoted ^= 1;
                    }
                    B32 splitter_found = (!quoted && (byte == 0 || char_is_space(byte)));
                    if(splitter_found)
                    {
                      String8 string = str8_substr(args, r1u64(start_split_idx, idx));
                      if(string.size > 0)
                      {
                        str8_list_push(scratch.arena, &cmdln_strings, string);
                      }
                      start_split_idx = idx+1;
                    }
                  }
                }
              }
              
              // rjf: push message to launch
              {
                CTRL_Msg msg = {CTRL_MsgKind_Launch};
                msg.path = path;
                msg.cmd_line_string_list = cmdln_strings;
                msg.env_inherit = 1;
                MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
                str8_list_push(scratch.arena, &msg.entry_points, entry);
                df_push_ctrl_msg(&msg);
              }
            }
            
            // rjf: run
            df_ctrl_run(DF_RunKind_Run, &df_g_nil_entity, CTRL_RunFlag_StopOnEntryPoint * (core_cmd_kind == DF_CoreCmdKind_LaunchAndInit), 0);
          }
          
          // rjf: no targets -> error
          if(targets.count == 0)
          {
            DF_CmdParams p = params;
            p.string = str8_lit("No active targets exist; cannot launch.");
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        case DF_CoreCmdKind_Kill:
        {
          DF_EntityList processes = df_entity_list_from_handle_list(scratch.arena, params.entity_list);
          
          // rjf: no processes => kill everything
          if(processes.count == 0)
          {
            processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          }
          
          // rjf: kill processes
          if(processes.count != 0)
          {
            for(DF_EntityNode *n = processes.first; n != 0; n = n->next)
            {
              DF_Entity *process = n->entity;
              CTRL_Msg msg = {CTRL_MsgKind_Kill};
              {
                msg.exit_code = 1;
                msg.machine_id = process->ctrl_machine_id;
                msg.entity = process->ctrl_handle;
                MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
              }
              df_push_ctrl_msg(&msg);
            }
          }
          
          // rjf: no processes -> error
          if(processes.count == 0)
          {
            DF_CmdParams p = params;
            p.string = str8_lit("No attached running processes exist; cannot kill.");
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        case DF_CoreCmdKind_KillAll:
        {
          DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          if(processes.count != 0)
          {
            for(DF_EntityNode *n = processes.first; n != 0; n = n->next)
            {
              DF_Entity *process = n->entity;
              CTRL_Msg msg = {CTRL_MsgKind_Kill};
              {
                msg.exit_code = 1;
                msg.machine_id = process->ctrl_machine_id;
                msg.entity = process->ctrl_handle;
                MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
              }
              df_push_ctrl_msg(&msg);
            }
          }
          if(processes.count == 0)
          {
            DF_CmdParams p = params;
            p.string = str8_lit("No attached running processes exist; cannot kill.");
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        case DF_CoreCmdKind_Detach:
        {
          for(DF_HandleNode *n = params.entity_list.first; n != 0; n = n->next)
          {
            DF_Entity *entity = df_entity_from_handle(n->handle);
            if(entity->kind == DF_EntityKind_Process)
            {
              CTRL_Msg msg = {CTRL_MsgKind_Detach};
              msg.machine_id = entity->ctrl_machine_id;
              msg.entity = entity->ctrl_handle;
              MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
              df_push_ctrl_msg(&msg);
            }
          }
        }break;
        case DF_CoreCmdKind_Continue:
        {
          B32 good_to_run = 0;
          DF_EntityList machines = df_query_cached_entity_list_with_kind(DF_EntityKind_Machine);
          for(DF_EntityNode *n = machines.first; n != 0; n = n->next)
          {
            DF_Entity *machine = n->entity;
            if(!df_entity_is_frozen(machine))
            {
              good_to_run = 1;
              break;
            }
          }
          if(good_to_run)
          {
            df_ctrl_run(DF_RunKind_Run, &df_g_nil_entity, 0, 0);
          }
          else
          {
            DF_CmdParams p = params;
            p.string = str8_lit("Cannot run with all threads frozen.");
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        case DF_CoreCmdKind_StepIntoInst:
        case DF_CoreCmdKind_StepOverInst:
        case DF_CoreCmdKind_StepIntoLine:
        case DF_CoreCmdKind_StepOverLine:
        case DF_CoreCmdKind_StepOut:
        {
          DF_Entity *thread = df_entity_from_handle(params.entity);
          if(df_ctrl_targets_running())
          {
            if(df_ctrl_last_run_kind() == DF_RunKind_Run)
            {
              DF_CmdParams p = params;
              p.string = str8_lit("Must halt before stepping.");
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
            }
          }
          else if(df_entity_is_frozen(thread))
          {
            DF_CmdParams p = params;
            p.string = str8_lit("Must thaw selected thread before stepping.");
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
          else
          {
            B32 good = 1;
            CTRL_TrapList traps = {0};
            switch(core_cmd_kind)
            {
              default: break;
              case DF_CoreCmdKind_StepIntoInst: {}break;
              case DF_CoreCmdKind_StepOverInst: {traps = df_trap_net_from_thread__step_over_inst(scratch.arena, thread);}break;
              case DF_CoreCmdKind_StepIntoLine: {traps = df_trap_net_from_thread__step_into_line(scratch.arena, thread);}break;
              case DF_CoreCmdKind_StepOverLine: {traps = df_trap_net_from_thread__step_over_line(scratch.arena, thread);}break;
              case DF_CoreCmdKind_StepOut:
              {
                // rjf: thread => full unwind
                CTRL_Unwind unwind = ctrl_unwind_from_thread(scratch.arena, df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle, os_now_microseconds()+10000);
                
                // rjf: use first unwind frame to generate trap
                if(unwind.flags == 0 && unwind.frames.count > 1)
                {
                  U64 vaddr = regs_rip_from_arch_block(thread->arch, unwind.frames.v[1].regs);
                  CTRL_Trap trap = {CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck, vaddr};
                  ctrl_trap_list_push(scratch.arena, &traps, &trap);
                }
                else
                {
                  DF_CmdParams p = params;
                  p.string = str8_lit("Could not find the return address of the current callstack frame successfully.");
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
                  df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                  good = 0;
                }
              }break;
            }
            if(good && traps.count != 0)
            {
              df_ctrl_run(DF_RunKind_Step, thread, 0, &traps);
            }
            if(good && traps.count == 0)
            {
              df_ctrl_run(DF_RunKind_SingleStep, thread, 0, &traps);
            }
          }
        }break;
        case DF_CoreCmdKind_Halt:
        if(df_ctrl_targets_running())
        {
          ctrl_halt();
        }break;
        case DF_CoreCmdKind_SoftHaltRefresh:
        {
          if(df_ctrl_targets_running())
          {
            df_ctrl_run(df_state->ctrl_last_run_kind, df_entity_from_handle(df_state->ctrl_last_run_thread), df_state->ctrl_last_run_flags, &df_state->ctrl_last_run_traps);
          }
        }break;
        case DF_CoreCmdKind_SetThreadIP:
        {
          DF_Entity *thread = df_entity_from_handle(params.entity);
          U64 vaddr = params.vaddr;
          if(thread->kind == DF_EntityKind_Thread && vaddr != 0)
          {
            df_set_thread_rip(thread, vaddr);
          }
        }break;
        
        //- rjf: high-level composite target control operations
        case DF_CoreCmdKind_RunToLine:
        {
          String8 file_path = params.file_path;
          TxtPt point = params.text_point;
          DF_Entity *bp = df_entity_alloc(df_entity_root(), DF_EntityKind_Breakpoint);
          df_entity_equip_cfg_src(bp, DF_CfgSrc_Transient);
          bp->flags |= DF_EntityFlag_DiesOnRunStop;
          DF_Entity *loc = df_entity_alloc(bp, DF_EntityKind_Location);
          df_entity_equip_name(loc, file_path);
          df_entity_equip_txt_pt(loc, point);
          DF_CmdParams p = df_cmd_params_zero();
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Run));
        }break;
        case DF_CoreCmdKind_RunToAddress:
        {
          DF_Entity *bp = df_entity_alloc(df_entity_root(), DF_EntityKind_Breakpoint);
          df_entity_equip_cfg_src(bp, DF_CfgSrc_Transient);
          bp->flags |= DF_EntityFlag_DiesOnRunStop;
          DF_Entity *loc = df_entity_alloc(bp, DF_EntityKind_Location);
          df_entity_equip_vaddr(loc, params.vaddr);
          DF_CmdParams p = df_cmd_params_zero();
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Run));
        }break;
        case DF_CoreCmdKind_Run:
        {
          DF_CmdParams params = df_cmd_params_zero();
          DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          if(processes.count != 0)
          {
            DF_CmdParams params = df_cmd_params_zero();
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Continue));
          }
          else if(!df_ctrl_targets_running())
          {
            DF_CmdParams params = df_cmd_params_zero();
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
          }
        }break;
        case DF_CoreCmdKind_Restart:
        {
          // rjf: kill all
          {
            DF_CmdParams params = df_cmd_params_zero();
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_KillAll));
          }
          
          // rjf: gather targets corresponding to all launched processes
          DF_EntityList targets = {0};
          {
            DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
            for(DF_EntityNode *n = processes.first; n != 0; n = n->next)
            {
              DF_Entity *process = n->entity;
              DF_Entity *target = df_entity_from_handle(process->entity_handle);
              if(!df_entity_is_nil(target))
              {
                df_entity_list_push(scratch.arena, &targets, target);
              }
            }
          }
          
          // rjf: re-launch targets
          {
            DF_CmdParams params = df_cmd_params_zero();
            params.entity_list = df_handle_list_from_entity_list(scratch.arena, targets);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_EntityList);
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
          }
        }break;
        case DF_CoreCmdKind_StepInto:
        case DF_CoreCmdKind_StepOver:
        {
          DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          if(processes.count != 0)
          {
            DF_CoreCmdKind step_cmd_kind = (core_cmd_kind == DF_CoreCmdKind_StepInto
                                            ? DF_CoreCmdKind_StepIntoLine
                                            : DF_CoreCmdKind_StepOverLine);
            B32 prefer_dasm = params.prefer_dasm;
            if(prefer_dasm)
            {
              step_cmd_kind = (core_cmd_kind == DF_CoreCmdKind_StepInto
                               ? DF_CoreCmdKind_StepIntoInst
                               : DF_CoreCmdKind_StepOverInst);
            }
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(step_cmd_kind));
          }
          else if(!df_ctrl_targets_running())
          {
            DF_EntityList targets = df_push_active_target_list(scratch.arena);
            DF_CmdParams p = params;
            p.entity_list = df_handle_list_from_entity_list(scratch.arena, targets);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_EntityList);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndInit));
          }
        }break;
        
        //- rjf: debug control context management operations
        case DF_CoreCmdKind_SelectThread:
        {
          DF_Entity *thread = df_entity_from_handle(params.entity);
          DF_Entity *module = df_module_from_thread(thread);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          df_state->base_interact_regs.v.unwind_count = 0;
          df_state->base_interact_regs.v.inline_depth = 0;
          df_state->base_interact_regs.v.thread = df_handle_from_entity(thread);
          df_state->base_interact_regs.v.module = df_handle_from_entity(module);
          df_state->base_interact_regs.v.process = df_handle_from_entity(process);
        }break;
        case DF_CoreCmdKind_SelectUnwind:
        {
          DI_Scope *di_scope = di_scope_open();
          DF_Entity *thread = df_entity_from_handle(df_state->base_interact_regs.v.thread);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          CTRL_Unwind base_unwind = df_query_cached_unwind_from_thread(thread);
          DF_Unwind rich_unwind = df_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
          if(params.unwind_index < rich_unwind.frames.concrete_frame_count)
          {
            DF_UnwindFrame *frame = &rich_unwind.frames.v[params.unwind_index];
            df_state->base_interact_regs.v.unwind_count = params.unwind_index;
            df_state->base_interact_regs.v.inline_depth = 0;
            if(params.inline_depth <= frame->inline_frame_count)
            {
              df_state->base_interact_regs.v.inline_depth = params.inline_depth;
            }
          }
          di_scope_close(di_scope);
        }break;
        case DF_CoreCmdKind_UpOneFrame:
        case DF_CoreCmdKind_DownOneFrame:
        {
          DI_Scope *di_scope = di_scope_open();
          DF_Entity *thread = df_entity_from_handle(df_state->base_interact_regs.v.thread);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          CTRL_Unwind base_unwind = df_query_cached_unwind_from_thread(thread);
          DF_Unwind rich_unwind = df_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
          U64 crnt_unwind_idx = df_state->base_interact_regs.v.unwind_count;
          U64 crnt_inline_dpt = df_state->base_interact_regs.v.inline_depth;
          U64 next_unwind_idx = crnt_unwind_idx;
          U64 next_inline_dpt = crnt_inline_dpt;
          if(crnt_unwind_idx < rich_unwind.frames.concrete_frame_count)
          {
            DF_UnwindFrame *f = &rich_unwind.frames.v[crnt_unwind_idx];
            switch(core_cmd_kind)
            {
              default:{}break;
              case DF_CoreCmdKind_UpOneFrame:
              {
                if(crnt_inline_dpt < f->inline_frame_count)
                {
                  next_inline_dpt += 1;
                }
                else if(crnt_unwind_idx > 0)
                {
                  next_unwind_idx -= 1;
                  next_inline_dpt = 0;
                }
              }break;
              case DF_CoreCmdKind_DownOneFrame:
              {
                if(crnt_inline_dpt > 0)
                {
                  next_inline_dpt -= 1;
                }
                else if(crnt_unwind_idx < rich_unwind.frames.concrete_frame_count)
                {
                  next_unwind_idx += 1;
                  next_inline_dpt = (f+1)->inline_frame_count;
                }
              }break;
            }
          }
          DF_CmdParams p = params;
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_UnwindIndex);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_InlineDepth);
          p.unwind_index = next_unwind_idx;
          p.inline_depth = next_inline_dpt;
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
          di_scope_close(di_scope);
        }break;
        case DF_CoreCmdKind_FreezeThread:
        case DF_CoreCmdKind_ThawThread:
        case DF_CoreCmdKind_FreezeProcess:
        case DF_CoreCmdKind_ThawProcess:
        case DF_CoreCmdKind_FreezeMachine:
        case DF_CoreCmdKind_ThawMachine:
        {
          DF_CoreCmdKind disptch_kind = ((core_cmd_kind == DF_CoreCmdKind_FreezeThread ||
                                          core_cmd_kind == DF_CoreCmdKind_FreezeProcess ||
                                          core_cmd_kind == DF_CoreCmdKind_FreezeMachine)
                                         ? DF_CoreCmdKind_FreezeEntity
                                         : DF_CoreCmdKind_ThawEntity);
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(disptch_kind));
        }break;
        case DF_CoreCmdKind_FreezeLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Local;
          DF_CmdParams params = df_cmd_params_zero();
          params.entity = df_handle_from_entity(df_machine_entity_from_machine_id(machine_id));
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FreezeMachine));
        }break;
        case DF_CoreCmdKind_ThawLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Local;
          DF_CmdParams params = df_cmd_params_zero();
          params.entity = df_handle_from_entity(df_machine_entity_from_machine_id(machine_id));
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ThawMachine));
        }break;
        
        //- rjf: undo/redo
        case DF_CoreCmdKind_Undo:
        {
          df_state_delta_history_wind(df_state->hist, Side_Min);
        }break;
        case DF_CoreCmdKind_Redo:
        {
          df_state_delta_history_wind(df_state->hist, Side_Max);
        }break;
        
        //- rjf: files
        case DF_CoreCmdKind_SetCurrentPath:
        {
          arena_clear(df_state->current_path_arena);
          df_state->current_path = push_str8_copy(df_state->current_path_arena, params.file_path);
        }break;
        
        //- rjf: config path saving/loading/applying
        case DF_CoreCmdKind_OpenRecentProject:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          if(entity->kind == DF_EntityKind_RecentProject)
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.file_path = entity->name;
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_OpenProject));
          }
        }break;
        case DF_CoreCmdKind_OpenUser:
        case DF_CoreCmdKind_OpenProject:
        {
          B32 load_cfg[DF_CfgSrc_COUNT] = {0};
          for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
          {
            load_cfg[src] = (core_cmd_kind == df_g_cfg_src_load_cmd_kind_table[src]);
          }
          
          //- rjf: normalize path
          String8 new_path = path_normalized_from_string(scratch.arena, params.file_path);
          
          //- rjf: path -> data
          FileProperties props = {0};
          String8 data = {0};
          {
            OS_Handle file = os_file_open(OS_AccessFlag_ShareRead|OS_AccessFlag_Read, new_path);
            props = os_properties_from_file(file);
            data = os_string_from_file_range(scratch.arena, file, r1u64(0, props.size));
            os_file_close(file);
          }
          
          //- rjf: investigate file path/data
          B32 file_is_okay = 1;
          if(props.modified != 0 && data.size != 0 && !str8_match(str8_prefix(data, 9), str8_lit("// raddbg"), 0))
          {
            file_is_okay = 0;
          }
          
          //- rjf: set new config paths
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              if(load_cfg[src])
              {
                arena_clear(df_state->cfg_path_arenas[src]);
                df_state->cfg_paths[src] = push_str8_copy(df_state->cfg_path_arenas[src], new_path);
              }
            }
          }
          
          //- rjf: get config file properties
          FileProperties cfg_props[DF_CfgSrc_COUNT] = {0};
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              String8 path = df_cfg_path_from_src(src);
              cfg_props[src] = os_properties_from_file_path(path);
            }
          }
          
          //- rjf: load files
          String8 cfg_data[DF_CfgSrc_COUNT] = {0};
          U64 cfg_timestamps[DF_CfgSrc_COUNT] = {0};
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              String8 path = df_cfg_path_from_src(src);
              OS_Handle file = os_file_open(OS_AccessFlag_ShareRead|OS_AccessFlag_Read, path);
              FileProperties props = os_properties_from_file(file);
              String8 data = os_string_from_file_range(scratch.arena, file, r1u64(0, props.size));
              if(data.size != 0)
              {
                cfg_data[src] = data;
                cfg_timestamps[src] = props.modified;
              }
              os_file_close(file);
            }
          }
          
          //- rjf: determine if we need to save config
          B32 cfg_save[DF_CfgSrc_COUNT] = {0};
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              cfg_save[src] = (load_cfg[src] && cfg_props[src].created == 0);
            }
          }
          
          //- rjf: determine if we need to reload config
          B32 cfg_load[DF_CfgSrc_COUNT] = {0};
          B32 cfg_load_any = 0;
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              cfg_load[src] = (load_cfg[src] && ((cfg_save[src] == 0 && df_state->cfg_cached_timestamp[src] != cfg_timestamps[src]) || cfg_props[src].created == 0));
              cfg_load_any = cfg_load_any || cfg_load[src];
            }
          }
          
          //- rjf: load => build new config table
          if(cfg_load_any)
          {
            arena_clear(df_state->cfg_arena);
            MemoryZeroStruct(&df_state->cfg_table);
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              df_cfg_table_push_unparsed_string(df_state->cfg_arena, &df_state->cfg_table, cfg_data[src], src);
            }
          }
          
          //- rjf: load => dispatch apply
          //
          // NOTE(rjf): must happen before `save`. we need to create a default before saving, which
          // occurs in the 'apply' path.
          //
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              if(cfg_load[src])
              {
                DF_CoreCmdKind cmd_kind = df_g_cfg_src_apply_cmd_kind_table[src];
                DF_CmdParams params = df_cmd_params_zero();
                df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(cmd_kind));
                df_state->cfg_cached_timestamp[src] = cfg_timestamps[src];
              }
            }
          }
          
          //- rjf: save => dispatch write
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              if(cfg_save[src])
              {
                DF_CoreCmdKind cmd_kind = df_g_cfg_src_write_cmd_kind_table[src];
                DF_CmdParams params = df_cmd_params_zero();
                df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(cmd_kind));
              }
            }
          }
          
          //- rjf: bad file -> alert user
          if(!file_is_okay)
          {
            DF_CmdParams p = params;
            p.string = push_str8f(scratch.arena, "\"%S\" appears to refer to an existing file which is not a RADDBG config file. This would overwrite the file.", new_path);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        
        //- rjf: loading/applying stateful config changes
        case DF_CoreCmdKind_ApplyUserData:
        case DF_CoreCmdKind_ApplyProjectData:
        {
          DF_CfgTable *table = df_cfg_table();
          
          //- rjf: get config source
          DF_CfgSrc src = DF_CfgSrc_User;
          for(DF_CfgSrc s = (DF_CfgSrc)0; s < DF_CfgSrc_COUNT; s = (DF_CfgSrc)(s+1))
          {
            if(core_cmd_kind == df_g_cfg_src_apply_cmd_kind_table[s])
            {
              src = s;
              break;
            }
          }
          
          //- rjf: get paths
          String8 cfg_path   = df_cfg_path_from_src(src);
          String8 cfg_folder = str8_chop_last_slash(cfg_path);
          
          //- rjf: keep track of recent projects
          if(src == DF_CfgSrc_Project)
          {
            DF_EntityList recent_projects = df_query_cached_entity_list_with_kind(DF_EntityKind_RecentProject);
            DF_Entity *recent_project = &df_g_nil_entity;
            for(DF_EntityNode *n = recent_projects.first; n != 0; n = n->next)
            {
              if(path_match_normalized(cfg_path, n->entity->name))
              {
                recent_project = n->entity;
                break;
              }
            }
            if(df_entity_is_nil(recent_project))
            {
              recent_project = df_entity_alloc(df_entity_root(), DF_EntityKind_RecentProject);
              df_entity_equip_name(recent_project, path_normalized_from_string(scratch.arena, cfg_path));
              df_entity_equip_cfg_src(recent_project, DF_CfgSrc_User);
            }
          }
          
          //- rjf: eliminate all existing entities which are derived from config
          {
            for(EachEnumVal(DF_EntityKind, k))
            {
              DF_EntityKindFlags k_flags = df_g_entity_kind_flags_table[k];
              if(k_flags & DF_EntityKindFlag_IsSerializedToConfig)
              {
                DF_EntityList entities = df_query_cached_entity_list_with_kind(k);
                for(DF_EntityNode *n = entities.first; n != 0; n = n->next)
                {
                  if(n->entity->cfg_src == src)
                  {
                    df_entity_mark_for_deletion(n->entity);
                  }
                }
              }
            }
          }
          
          //- rjf: apply all entities
          {
            for(EachEnumVal(DF_EntityKind, k))
            {
              DF_EntityKindFlags k_flags = df_g_entity_kind_flags_table[k];
              if(k_flags & DF_EntityKindFlag_IsSerializedToConfig)
              {
                DF_CfgVal *k_val = df_cfg_val_from_string(table, df_g_entity_kind_name_lower_table[k]);
                for(DF_CfgTree *k_tree = k_val->first;
                    k_tree != &df_g_nil_cfg_tree;
                    k_tree = k_tree->next)
                {
                  if(k_tree->source != src)
                  {
                    continue;
                  }
                  DF_Entity *entity = df_entity_alloc(df_entity_root(), k);
                  df_entity_equip_cfg_src(entity, k_tree->source);
                  
                  // rjf: iterate config tree
                  typedef struct Task Task;
                  struct Task
                  {
                    Task *next;
                    DF_Entity *entity;
                    MD_Node *n;
                  };
                  Task start_task = {0, entity, k_tree->root};
                  Task *first_task = &start_task;
                  Task *last_task = first_task;
                  for(Task *t = first_task; t != 0; t = t->next)
                  {
                    MD_Node *node = t->n;
                    for(MD_EachNode(child, node->first))
                    {
                      // rjf: standalone string literals under an entity -> name
                      if(child->flags & MD_NodeFlag_StringLiteral && child->first == &md_nil_node)
                      {
                        String8 string = df_cfg_raw_from_escaped_string(scratch.arena, child->string);
                        if(df_g_entity_kind_flags_table[t->entity->kind] & DF_EntityKindFlag_NameIsPath)
                        {
                          string = path_absolute_dst_from_relative_dst_src(scratch.arena, string, cfg_folder);
                        }
                        df_entity_equip_name(t->entity, string);
                      }
                      
                      // rjf: standalone string literals under an entity, with a numeric child -> name & text location
                      if(child->flags & MD_NodeFlag_StringLiteral && child->first->flags & MD_NodeFlag_Numeric && child->first->first == &md_nil_node)
                      {
                        String8 string = df_cfg_raw_from_escaped_string(scratch.arena, child->string);
                        if(df_g_entity_kind_flags_table[t->entity->kind] & DF_EntityKindFlag_NameIsPath)
                        {
                          string = path_absolute_dst_from_relative_dst_src(scratch.arena, string, cfg_folder);
                        }
                        df_entity_equip_name(t->entity, string);
                        S64 line = 0;
                        try_s64_from_str8_c_rules(child->first->string, &line);
                        TxtPt pt = txt_pt(line, 1);
                        df_entity_equip_txt_pt(t->entity, pt);
                      }
                      
                      // rjf: standalone hex literals under an entity -> vaddr
                      if(child->flags & MD_NodeFlag_Numeric && child->first == &md_nil_node && str8_match(str8_substr(child->string, r1u64(0, 2)), str8_lit("0x"), 0))
                      {
                        U64 vaddr = 0;
                        try_u64_from_str8_c_rules(child->string, &vaddr);
                        df_entity_equip_vaddr(t->entity, vaddr);
                      }
                      
                      // rjf: specifically named entity equipment
                      if((str8_match(child->string, str8_lit("name"), StringMatchFlag_CaseInsensitive) ||
                          str8_match(child->string, str8_lit("label"), StringMatchFlag_CaseInsensitive)) &&
                         child->first != &md_nil_node)
                      {
                        String8 string = df_cfg_raw_from_escaped_string(scratch.arena, child->first->string);
                        if(df_g_entity_kind_flags_table[t->entity->kind] & DF_EntityKindFlag_NameIsPath)
                        {
                          string = path_absolute_dst_from_relative_dst_src(scratch.arena, string, cfg_folder);
                        }
                        df_entity_equip_name(t->entity, string);
                      }
                      if((str8_match(child->string, str8_lit("active"), StringMatchFlag_CaseInsensitive) ||
                          str8_match(child->string, str8_lit("enabled"), StringMatchFlag_CaseInsensitive)) &&
                         child->first != &md_nil_node)
                      {
                        df_entity_equip_disabled(t->entity, !str8_match(child->first->string, str8_lit("1"), 0));
                      }
                      if(str8_match(child->string, str8_lit("disabled"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                      {
                        df_entity_equip_disabled(t->entity, str8_match(child->first->string, str8_lit("1"), 0));
                      }
                      if(str8_match(child->string, str8_lit("hsva"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                      {
                        Vec4F32 hsva = {0};
                        hsva.x = (F32)f64_from_str8(child->first->string);
                        hsva.y = (F32)f64_from_str8(child->first->next->string);
                        hsva.z = (F32)f64_from_str8(child->first->next->next->string);
                        hsva.w = (F32)f64_from_str8(child->first->next->next->next->string);
                        df_entity_equip_color_hsva(t->entity, hsva);
                      }
                      if(str8_match(child->string, str8_lit("color"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                      {
                        Vec4F32 rgba = rgba_from_hex_string_4f32(child->first->string);
                        Vec4F32 hsva = hsva_from_rgba(rgba);
                        df_entity_equip_color_hsva(t->entity, hsva);
                      }
                      if(str8_match(child->string, str8_lit("line"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                      {
                        S64 line = 0;
                        try_s64_from_str8_c_rules(child->first->string, &line);
                        TxtPt pt = txt_pt(line, 1);
                        df_entity_equip_txt_pt(t->entity, pt);
                      }
                      if((str8_match(child->string, str8_lit("vaddr"), StringMatchFlag_CaseInsensitive) ||
                          str8_match(child->string, str8_lit("addr"), StringMatchFlag_CaseInsensitive)) &&
                         child->first != &md_nil_node)
                      {
                        U64 vaddr = 0;
                        try_u64_from_str8_c_rules(child->first->string, &vaddr);
                        df_entity_equip_vaddr(t->entity, vaddr);
                      }
                      
                      // rjf: sub-entity -> create new task
                      DF_EntityKind sub_entity_kind = DF_EntityKind_Nil;
                      for(EachEnumVal(DF_EntityKind, k2))
                      {
                        if(child->flags & MD_NodeFlag_Identifier && child->first != &md_nil_node &&
                           (str8_match(child->string, df_g_entity_kind_name_lower_table[k2], StringMatchFlag_CaseInsensitive) ||
                            (k2 == DF_EntityKind_Executable && str8_match(child->string, str8_lit("exe"), StringMatchFlag_CaseInsensitive))))
                        {
                          Task *task = push_array(scratch.arena, Task, 1);
                          task->next = t->next;
                          task->entity = df_entity_alloc(t->entity, k2);
                          task->n = child;
                          t->next = task;
                          break;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          
          //- rjf: apply exception code filters
          DF_CfgVal *filter_tables = df_cfg_val_from_string(table, str8_lit("exception_code_filters"));
          for(DF_CfgTree *table = filter_tables->first;
              table != &df_g_nil_cfg_tree;
              table = table->next)
          {
            for(MD_EachNode(rule, table->root->first))
            {
              String8 name = rule->string;
              String8 val_string = rule->first->string;
              U64 val = 0;
              if(try_u64_from_str8_c_rules(val_string, &val))
              {
                CTRL_ExceptionCodeKind kind = CTRL_ExceptionCodeKind_Null;
                for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)(CTRL_ExceptionCodeKind_Null+1);
                    k < CTRL_ExceptionCodeKind_COUNT;
                    k = (CTRL_ExceptionCodeKind)(k+1))
                {
                  if(str8_match(name, ctrl_exception_code_kind_lowercase_code_string_table[k], 0))
                  {
                    kind = k;
                    break;
                  }
                }
                if(kind != CTRL_ExceptionCodeKind_Null)
                {
                  if(val)
                  {
                    df_state->ctrl_exception_code_filters[kind/64] |= (1ull<<(kind%64));
                  }
                  else
                  {
                    df_state->ctrl_exception_code_filters[kind/64] &= ~(1ull<<(kind%64));
                  }
                }
              }
            }
          }
        }break;
        
        //- rjf: writing config changes
        case DF_CoreCmdKind_WriteUserData:
        case DF_CoreCmdKind_WriteProjectData:
        {
          DF_CfgSrc src = DF_CfgSrc_User;
          for(DF_CfgSrc s = (DF_CfgSrc)0; s < DF_CfgSrc_COUNT; s = (DF_CfgSrc)(s+1))
          {
            if(core_cmd_kind == df_g_cfg_src_write_cmd_kind_table[s])
            {
              src = s;
              break;
            }
          }
          arena_clear(df_state->cfg_write_arenas[src]);
          MemoryZeroStruct(&df_state->cfg_write_data[src]);
          String8 path = df_cfg_path_from_src(src);
          String8List strs = df_cfg_strings_from_core(scratch.arena, path, src);
          String8 header = push_str8f(scratch.arena, "// raddbg %s file\n\n", df_g_cfg_src_string_table[src].str);
          str8_list_push_front(scratch.arena, &strs, header);
          String8 data = str8_list_join(scratch.arena, &strs, 0);
          String8 data_indented = indented_from_string(scratch.arena, data);
          df_state->cfg_write_issued[src] = 1;
          df_cfg_push_write_string(src, data_indented);
        }break;
        
        //- rjf: override file links
        case DF_CoreCmdKind_SetFileOverrideLinkSrc:
        case DF_CoreCmdKind_SetFileOverrideLinkDst:
        {
          // rjf: unpack args
          DF_Entity *map = df_entity_from_handle(params.entity);
          String8 path = path_normalized_from_string(scratch.arena, params.file_path);
          String8 path_folder = str8_chop_last_slash(path);
          String8 path_file = str8_skip_last_slash(path);
          
          // rjf: src -> move map & commit name; dst -> open destination file & refer to it in map
          switch(core_cmd_kind)
          {
            default:{}break;
            case DF_CoreCmdKind_SetFileOverrideLinkSrc:
            {
              DF_Entity *map_parent = (params.file_path.size != 0) ? df_entity_from_path(path_folder, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing) : df_entity_root();
              if(df_entity_is_nil(map))
              {
                map = df_entity_alloc(map_parent, DF_EntityKind_FilePathMap);
              }
              else
              {
                df_entity_change_parent(map, map->parent, map_parent, &df_g_nil_entity);
              }
              df_entity_equip_name(map, path_file);
            }break;
            case DF_CoreCmdKind_SetFileOverrideLinkDst:
            {
              if(df_entity_is_nil(map))
              {
                map = df_entity_alloc(df_entity_root(), DF_EntityKind_FilePathMap);
              }
              DF_Entity *map_dst_entity = &df_g_nil_entity;
              if(params.file_path.size != 0)
              {
                map_dst_entity = df_entity_from_path(path, DF_EntityFromPathFlag_All);
              }
              df_entity_equip_entity_handle(map, df_handle_from_entity(map_dst_entity));
            }break;
          }
          
          // rjf: empty src/dest -> delete
          if(!df_entity_is_nil(map) && map->name.size == 0 && df_entity_is_nil(df_entity_from_handle(map->entity_handle)))
          {
            df_entity_mark_for_deletion(map);
          }
        }break;
        case DF_CoreCmdKind_SetFileReplacementPath:
        {
          // NOTE(rjf):
          //
          // C:/foo/bar/baz.c
          // D:/foo/bar/baz.c
          // -> override C: -> D:
          //
          // C:/1/2/foo/bar.c
          // C:/2/3/foo/bar.c
          // -> override C:/1/2 -> C:2/3
          //
          // C:/foo/bar/baz.c
          // D:/1/2/3.c
          // -> override C:/foo/bar/baz.c -> D:/1/2/3.c
          
          //- rjf: unpack
          String8 src_path = params.string;
          String8 dst_path = params.file_path;
          // TODO(rjf):
          
          //- rjf: grab src file & chosen replacement
          DF_Entity *file = df_entity_from_handle(params.entity);
          DF_Entity *replacement = df_entity_from_path(params.file_path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
          
          //- rjf: find 
          DF_Entity *first_diff_src = file;
          DF_Entity *first_diff_dst = replacement;
          for(;!df_entity_is_nil(first_diff_src) && !df_entity_is_nil(first_diff_dst);)
          {
            if(!str8_match(first_diff_src->name, first_diff_dst->name, StringMatchFlag_CaseInsensitive) ||
               first_diff_src->parent->kind != DF_EntityKind_File ||
               first_diff_src->parent->parent->kind != DF_EntityKind_File ||
               first_diff_dst->parent->kind != DF_EntityKind_File ||
               first_diff_dst->parent->parent->kind != DF_EntityKind_File)
            {
              break;
            }
            first_diff_src = first_diff_src->parent;
            first_diff_dst = first_diff_dst->parent;
          }
          
          //- rjf: override first different
          if(!df_entity_is_nil(first_diff_src) && !df_entity_is_nil(first_diff_dst))
          {
            DF_Entity *link = df_entity_child_from_name_and_kind(first_diff_src->parent, first_diff_src->name, DF_EntityKind_FilePathMap);
            if(df_entity_is_nil(link))
            {
              link = df_entity_alloc(first_diff_src->parent, DF_EntityKind_FilePathMap);
              df_entity_equip_name(link, first_diff_src->name);
            }
            df_entity_equip_entity_handle(link, df_handle_from_entity(first_diff_dst));
          }
        }break;
        
        //- rjf: auto view rules
        case DF_CoreCmdKind_SetAutoViewRuleType:
        case DF_CoreCmdKind_SetAutoViewRuleViewRule:
        {
          DF_Entity *map = df_entity_from_handle(params.entity);
          if(df_entity_is_nil(map))
          {
            map = df_entity_alloc(df_entity_root(), DF_EntityKind_AutoViewRule);
            df_entity_equip_cfg_src(map, DF_CfgSrc_Project);
          }
          DF_Entity *src = df_entity_child_from_kind(map, DF_EntityKind_Source);
          if(df_entity_is_nil(src))
          {
            src = df_entity_alloc(map, DF_EntityKind_Source);
          }
          DF_Entity *dst = df_entity_child_from_kind(map, DF_EntityKind_Dest);
          if(df_entity_is_nil(dst))
          {
            dst = df_entity_alloc(map, DF_EntityKind_Dest);
          }
          if(map->kind == DF_EntityKind_AutoViewRule)
          {
            DF_Entity *edit_child = (core_cmd_kind == DF_CoreCmdKind_SetAutoViewRuleType ? src : dst);
            df_entity_equip_name(edit_child, params.string);
          }
          if(src->name.size == 0 && dst->name.size == 0)
          {
            df_entity_mark_for_deletion(map);
          }
          {
            DF_AutoViewRuleMapCache *cache = &df_state->auto_view_rule_cache;
            if(cache->arena == 0)
            {
              cache->arena = arena_alloc();
            }
            arena_clear(cache->arena);
            cache->slots_count = 1024;
            cache->slots = push_array(cache->arena, DF_AutoViewRuleSlot, cache->slots_count);
            DF_EntityList maps = df_query_cached_entity_list_with_kind(DF_EntityKind_AutoViewRule);
            for(DF_EntityNode *n = maps.first; n != 0; n = n->next)
            {
              DF_Entity *map = n->entity;
              DF_Entity *src = df_entity_child_from_kind(map, DF_EntityKind_Source);
              DF_Entity *dst = df_entity_child_from_kind(map, DF_EntityKind_Dest);
              String8 type = src->name;
              String8 view_rule = dst->name;
              U64 hash = df_hash_from_string(type);
              U64 slot_idx = hash%cache->slots_count;
              DF_AutoViewRuleSlot *slot = &cache->slots[slot_idx];
              DF_AutoViewRuleNode *node = push_array(cache->arena, DF_AutoViewRuleNode, 1);
              node->type = push_str8_copy(cache->arena, type);
              node->view_rule = push_str8_copy(cache->arena, view_rule);
              SLLQueuePush(slot->first, slot->last, node);
            }
          }
        }break;
        
        //- rjf: general entity operations
        case DF_CoreCmdKind_EnableEntity:
        case DF_CoreCmdKind_EnableBreakpoint:
        case DF_CoreCmdKind_EnableTarget:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          DF_StateDeltaHistoryBatch(df_state_delta_history())
          {
            df_entity_equip_disabled(entity, 0);
          }
        }break;
        case DF_CoreCmdKind_DisableEntity:
        case DF_CoreCmdKind_DisableBreakpoint:
        case DF_CoreCmdKind_DisableTarget:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          DF_StateDeltaHistoryBatch(df_state_delta_history())
          {
            df_entity_equip_disabled(entity, 1);
          }
        }break;
        case DF_CoreCmdKind_FreezeEntity:
        case DF_CoreCmdKind_ThawEntity:
        {
          B32 should_freeze = (core_cmd_kind == DF_CoreCmdKind_FreezeEntity);
          DF_Entity *root = df_entity_from_handle(params.entity);
          for(DF_Entity *e = root; !df_entity_is_nil(e); e = df_entity_rec_df_pre(e, root).next)
          {
            if(e->kind == DF_EntityKind_Thread)
            {
              df_set_thread_freeze_state(e, should_freeze);
            }
          }
        }break;
        case DF_CoreCmdKind_RemoveEntity:
        case DF_CoreCmdKind_RemoveBreakpoint:
        case DF_CoreCmdKind_RemoveTarget:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          DF_EntityKindFlags kind_flags = df_g_entity_kind_flags_table[entity->kind];
          if(kind_flags & DF_EntityKindFlag_CanDelete)
          {
            df_entity_mark_for_deletion(entity);
          }
        }break;
        case DF_CoreCmdKind_NameEntity:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          String8 string = params.string;
          df_entity_equip_name(entity, string);
        }break;
        case DF_CoreCmdKind_EditEntity:{}break;
        case DF_CoreCmdKind_DuplicateEntity:
        {
          DF_Entity *src = df_entity_from_handle(params.entity);
          if(!df_entity_is_nil(src))
          {
            typedef struct Task Task;
            struct Task
            {
              Task *next;
              DF_Entity *src_n;
              DF_Entity *dst_parent;
            };
            Task starter_task = {0, src, src->parent};
            Task *first_task = &starter_task;
            Task *last_task = &starter_task;
            for(Task *task = first_task; task != 0; task = task->next)
            {
              DF_Entity *src_n = task->src_n;
              if(task == first_task)
              {
                df_state_delta_history_batch_begin(df_state_delta_history());
              }
              DF_Entity *dst_n = df_entity_alloc(task->dst_parent, task->src_n->kind);
              if(task == first_task)
              {
                df_state_delta_history_batch_end(df_state_delta_history());
              }
              if(src_n->flags & DF_EntityFlag_HasTextPoint)    {df_entity_equip_txt_pt(dst_n, src_n->text_point);}
              if(src_n->flags & DF_EntityFlag_HasU64)          {df_entity_equip_u64(dst_n, src_n->u64);}
              if(src_n->flags & DF_EntityFlag_HasColor)        {df_entity_equip_color_hsva(dst_n, df_hsva_from_entity(src_n));}
              if(src_n->flags & DF_EntityFlag_HasVAddrRng)     {df_entity_equip_vaddr_rng(dst_n, src_n->vaddr_rng);}
              if(src_n->flags & DF_EntityFlag_HasVAddr)        {df_entity_equip_vaddr(dst_n, src_n->vaddr);}
              if(src_n->disabled)                              {df_entity_equip_disabled(dst_n, 1);}
              if(src_n->name.size != 0)                        {df_entity_equip_name(dst_n, src_n->name);}
              dst_n->cfg_src = src_n->cfg_src;
              for(DF_Entity *src_child = task->src_n->first; !df_entity_is_nil(src_child); src_child = src_child->next)
              {
                Task *child_task = push_array(scratch.arena, Task, 1);
                child_task->src_n = src_child;
                child_task->dst_parent = dst_n;
                SLLQueuePush(first_task, last_task, child_task);
              }
            }
          }
        }break;
        case DF_CoreCmdKind_RelocateEntity:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          DF_Entity *location = df_entity_child_from_kind(entity, DF_EntityKind_Location);
          if(df_entity_is_nil(location))
          {
            location = df_entity_alloc(entity, DF_EntityKind_Location);
          }
          location->flags &= ~DF_EntityFlag_HasTextPoint;
          location->flags &= ~DF_EntityFlag_HasVAddr;
          if(params.text_point.line != 0)
          {
            df_entity_equip_txt_pt(location, params.text_point);
          }
          if(params.vaddr != 0)
          {
            df_entity_equip_vaddr(location, params.vaddr);
          }
          if(params.file_path.size != 0)
          {
            df_entity_equip_name(location, params.file_path);
          }
        }break;
        
        //- rjf: breakpoints
        case DF_CoreCmdKind_AddBreakpoint:
        case DF_CoreCmdKind_ToggleBreakpoint:
        {
          String8 file_path = params.file_path;
          TxtPt pt = params.text_point;
          String8 name = params.string;
          U64 vaddr = params.vaddr;
          B32 removed_already_existing = 0;
          if(core_cmd_kind == DF_CoreCmdKind_ToggleBreakpoint)
          {
            DF_EntityList bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
            for(DF_EntityNode *n = bps.first; n != 0; n = n->next)
            {
              DF_Entity *bp = n->entity;
              DF_Entity *loc = df_entity_child_from_kind(bp, DF_EntityKind_Location);
              if((loc->flags & DF_EntityFlag_HasTextPoint && path_match_normalized(loc->name, file_path) && loc->text_point.line == pt.line) ||
                 (loc->flags & DF_EntityFlag_HasVAddr && loc->vaddr == vaddr) ||
                 (!(loc->flags & DF_EntityFlag_HasTextPoint) && str8_match(loc->name, name, 0)))
              {
                df_entity_mark_for_deletion(bp);
                removed_already_existing = 1;
                break;
              }
            }
          }
          if(!removed_already_existing)
          {
            DF_Entity *bp = df_entity_alloc(df_entity_root(), DF_EntityKind_Breakpoint);
            df_entity_equip_cfg_src(bp, DF_CfgSrc_Project);
            DF_Entity *loc = df_entity_alloc(bp, DF_EntityKind_Location);
            if(file_path.size != 0 && pt.line != 0)
            {
              df_entity_equip_name(loc, file_path);
              df_entity_equip_txt_pt(loc, pt);
            }
            else if(name.size != 0)
            {
              df_entity_equip_name(loc, name);
            }
            else if(vaddr != 0)
            {
              df_entity_equip_vaddr(loc, vaddr);
            }
          }
        }break;
        case DF_CoreCmdKind_AddAddressBreakpoint:
        case DF_CoreCmdKind_AddFunctionBreakpoint:
        {
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddBreakpoint));
        }break;
        
        //- rjf: watches
        case DF_CoreCmdKind_AddWatchPin:
        case DF_CoreCmdKind_ToggleWatchPin:
        {
          String8 file_path = params.file_path;
          TxtPt pt = params.text_point;
          String8 name = params.string;
          U64 vaddr = params.vaddr;
          B32 removed_already_existing = 0;
          if(core_cmd_kind == DF_CoreCmdKind_ToggleWatchPin)
          {
            DF_EntityList wps = df_query_cached_entity_list_with_kind(DF_EntityKind_WatchPin);
            for(DF_EntityNode *n = wps.first; n != 0; n = n->next)
            {
              DF_Entity *wp = n->entity;
              DF_Entity *loc = df_entity_child_from_kind(wp, DF_EntityKind_Location);
              if((loc->flags & DF_EntityFlag_HasTextPoint && path_match_normalized(loc->name, file_path) && loc->text_point.line == pt.line) ||
                 (loc->flags & DF_EntityFlag_HasVAddr && loc->vaddr == vaddr) ||
                 (!(loc->flags & DF_EntityFlag_HasTextPoint) && str8_match(loc->name, name, 0)))
              {
                df_entity_mark_for_deletion(wp);
                removed_already_existing = 1;
                break;
              }
            }
          }
          if(!removed_already_existing)
          {
            DF_Entity *wp = df_entity_alloc(df_entity_root(), DF_EntityKind_WatchPin);
            df_entity_equip_name(wp, name);
            df_entity_equip_cfg_src(wp, DF_CfgSrc_Project);
            DF_Entity *loc = df_entity_alloc(wp, DF_EntityKind_Location);
            if(file_path.size != 0 && pt.line != 0)
            {
              df_entity_equip_name(loc, file_path);
              df_entity_equip_txt_pt(loc, pt);
            }
            else if(vaddr != 0)
            {
              df_entity_equip_vaddr(loc, vaddr);
            }
          }
        }break;
        
        //- rjf: cursor operations
        case DF_CoreCmdKind_ToggleBreakpointAtCursor:
        {
          DF_InteractRegs *regs = df_interact_regs();
          DF_CmdParams p = df_cmd_params_zero();
          p.file_path  = regs->file_path;
          p.text_point = regs->cursor;
          p.vaddr      = regs->vaddr_range.min;
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleBreakpoint));
        }break;
        case DF_CoreCmdKind_ToggleWatchPinAtCursor:
        {
          DF_InteractRegs *regs = df_interact_regs();
          DF_CmdParams p = df_cmd_params_zero();
          p.file_path  = regs->file_path;
          p.text_point = regs->cursor;
          p.vaddr      = regs->vaddr_range.min;
          p.string     = params.string;
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
        }break;
        case DF_CoreCmdKind_GoToNameAtCursor:
        case DF_CoreCmdKind_ToggleWatchExpressionAtCursor:
        {
          HS_Scope *hs_scope = hs_scope_open();
          TXT_Scope *txt_scope = txt_scope_open();
          DF_InteractRegs *regs = df_interact_regs();
          U128 text_key = regs->text_key;
          TXT_LangKind lang_kind = regs->lang_kind;
          TxtRng range = txt_rng(regs->cursor, regs->mark);
          U128 hash = {0};
          TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, text_key, lang_kind, &hash);
          String8 data = hs_data_from_hash(hs_scope, hash);
          Rng1U64 expr_off_range = {0};
          if(range.min.column != range.max.column)
          {
            expr_off_range = r1u64(txt_off_from_info_pt(&info, range.min), txt_off_from_info_pt(&info, range.max));
          }
          else
          {
            expr_off_range = txt_expr_off_range_from_info_data_pt(&info, data, range.min);
          }
          String8 expr = str8_substr(data, expr_off_range);
          DF_CmdParams p = df_cmd_params_zero();
          p.string = expr;
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(core_cmd_kind == DF_CoreCmdKind_GoToNameAtCursor ? DF_CoreCmdKind_GoToName :
                                                                           core_cmd_kind == DF_CoreCmdKind_ToggleWatchExpressionAtCursor ? DF_CoreCmdKind_ToggleWatchExpression :
                                                                           DF_CoreCmdKind_GoToName));
          txt_scope_close(txt_scope);
          hs_scope_close(hs_scope);
        }break;
        case DF_CoreCmdKind_RunToCursor:
        {
          String8 file_path = df_interact_regs()->file_path;
          if(file_path.size != 0)
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.file_path = file_path;
            p.text_point = df_interact_regs()->cursor;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToLine));
          }
          else
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.vaddr = df_interact_regs()->vaddr_range.min;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToAddress));
          }
        }break;
        case DF_CoreCmdKind_SetNextStatement:
        {
          DF_Entity *thread = df_entity_from_handle(df_interact_regs()->thread);
          String8 file_path = df_interact_regs()->file_path;
          U64 new_rip_vaddr = df_interact_regs()->vaddr_range.min;
          if(file_path.size != 0)
          {
            DF_LineList *lines = &df_interact_regs()->lines;
            for(DF_LineNode *n = lines->first; n != 0; n = n->next)
            {
              DF_EntityList modules = df_modules_from_dbgi_key(scratch.arena, &n->v.dbgi_key);
              DF_Entity *module = df_module_from_thread_candidates(thread, &modules);
              if(!df_entity_is_nil(module))
              {
                new_rip_vaddr = df_vaddr_from_voff(module, n->v.voff_range.min);
                break;
              }
            }
          }
          DF_CmdParams p = df_cmd_params_zero();
          p.entity = df_handle_from_entity(thread);
          p.vaddr = new_rip_vaddr;
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
        }break;
        
        //- rjf: targets
        case DF_CoreCmdKind_AddTarget:
        {
          // rjf: build target
          DF_Entity *entity = &df_g_nil_entity;
          DF_StateDeltaHistoryBatch(df_state_delta_history())
          {
            entity = df_entity_alloc(df_entity_root(), DF_EntityKind_Target);
          }
          df_entity_equip_disabled(entity, 1);
          df_entity_equip_cfg_src(entity, DF_CfgSrc_Project);
          DF_Entity *exe = df_entity_alloc(entity, DF_EntityKind_Executable);
          df_entity_equip_name(exe, params.file_path);
          String8 working_dir = str8_chop_last_slash(params.file_path);
          if(working_dir.size != 0)
          {
            String8 working_dir_path = push_str8f(scratch.arena, "%S/", working_dir);
            DF_Entity *execution_path = df_entity_alloc(entity, DF_EntityKind_WorkingDirectory);
            df_entity_equip_name(execution_path, working_dir_path);
          }
          DF_CmdParams p = params;
          p.entity = df_handle_from_entity(entity);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EditTarget));
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectTarget));
        }break;
        case DF_CoreCmdKind_SelectTarget:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          if(entity->kind == DF_EntityKind_Target)
          {
            DF_StateDeltaHistoryBatch(df_state_delta_history())
            {
              DF_EntityList all_targets = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
              B32 is_selected = !entity->disabled;
              for(DF_EntityNode *n = all_targets.first; n != 0; n = n->next)
              {
                DF_Entity *target = n->entity;
                df_entity_equip_disabled(target, 1);
              }
              if(!is_selected)
              {
                df_entity_equip_disabled(entity, 0);
              }
            }
          }
        }break;
        
        //- rjf: ended processes
        case DF_CoreCmdKind_RetryEndedProcess:
        {
          DF_Entity *ended_process = df_entity_from_handle(params.entity);
          DF_Entity *target = df_entity_from_handle(ended_process->entity_handle);
          if(target->kind == DF_EntityKind_Target)
          {
            DF_CmdParams p = params;
            p.entity = df_handle_from_entity(target);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
          }
          else if(df_entity_is_nil(target))
          {
            DF_CmdParams p = params;
            p.string = str8_lit("The ended process' corresponding target is missing.");
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
          else if(df_entity_is_nil(ended_process))
          {
            DF_CmdParams p = params;
            p.string = str8_lit("Invalid ended process.");
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        
        //- rjf: attaching
        case DF_CoreCmdKind_Attach:
        {
          U64 pid = params.id;
          if(pid != 0)
          {
            CTRL_Msg msg = {CTRL_MsgKind_Attach};
            msg.entity_id = (U32)pid;
            MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
            df_push_ctrl_msg(&msg);
          }
        }break;
        
        //- rjf: jit-debugger registration
        case DF_CoreCmdKind_RegisterAsJITDebugger:
        {
#if OS_WINDOWS
          char filename_cstr[MAX_PATH] = {0};
          GetModuleFileName(0, filename_cstr, sizeof(filename_cstr));
          String8 debugger_binary_path = str8_cstring(filename_cstr);
          String8 name8 = str8_lit("Debugger");
          String8 data8 = push_str8f(scratch.arena, "%S --jit_pid:%%ld --jit_code:%%ld --jit_addr:0x%%p", debugger_binary_path);
          String16 name16 = str16_from_8(scratch.arena, name8);
          String16 data16 = str16_from_8(scratch.arena, data8);
          B32 likely_not_in_admin_mode = 0;
          {
            HKEY reg_key = 0;
            LSTATUS status = 0;
            status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug\\", 0, KEY_SET_VALUE, &reg_key);
            likely_not_in_admin_mode = (status == ERROR_ACCESS_DENIED);
            status = RegSetValueExW(reg_key, (LPCWSTR)name16.str, 0, REG_SZ, (BYTE *)data16.str, data16.size*sizeof(U16)+2);
            RegCloseKey(reg_key);
          }
          {
            HKEY reg_key = 0;
            LSTATUS status = 0;
            status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug\\", 0, KEY_SET_VALUE, &reg_key);
            likely_not_in_admin_mode = (status == ERROR_ACCESS_DENIED);
            status = RegSetValueExW(reg_key, (LPCWSTR)name16.str, 0, REG_SZ, (BYTE *)data16.str, data16.size*sizeof(U16)+2);
            RegCloseKey(reg_key);
          }
          if(likely_not_in_admin_mode)
          {
            DF_CmdParams p = params;
            p.string = str8_lit("Could not register as the just-in-time debugger, access was denied; try running the debugger as administrator.");
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
#else
          DF_CmdParams p = params;
          p.string = str8_lit("Registering as the just-in-time debugger is currently not supported on this system.");
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
#endif
        }break;
        
        //- rjf: developer commands
        case DF_CoreCmdKind_LogMarker:
        {
          log_infof("\"#MARKER\"");
        }break;
      }
    }
    scratch_end(scratch);
  }
  
  //- rjf: unpack eval-dependent info
  DF_Entity *process = df_entity_from_handle(df_interact_regs()->process);
  DF_Entity *thread = df_entity_from_handle(df_interact_regs()->thread);
  Architecture arch = df_architecture_from_entity(thread);
  U64 unwind_count = df_interact_regs()->unwind_count;
  U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
  CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
  DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
  U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
  U64 tls_root_vaddr = ctrl_query_cached_tls_root_vaddr_from_thread(df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  DF_EntityList all_modules = df_query_cached_entity_list_with_kind(DF_EntityKind_Module);
  U64 eval_modules_count = Max(1, all_modules.count);
  E_Module *eval_modules = push_array(arena, E_Module, eval_modules_count);
  E_Module *eval_modules_primary = &eval_modules[0];
  eval_modules_primary->rdi = &di_rdi_parsed_nil;
  eval_modules_primary->vaddr_range = r1u64(0, max_U64);
  DI_Key primary_dbgi_key = {0};
  {
    U64 eval_module_idx = 0;
    for(DF_EntityNode *n = all_modules.first; n != 0; n = n->next, eval_module_idx += 1)
    {
      DF_Entity *m = n->entity;
      DI_Key dbgi_key = df_dbgi_key_from_module(m);
      eval_modules[eval_module_idx].arch        = df_architecture_from_entity(m);
      eval_modules[eval_module_idx].rdi         = di_rdi_from_key(df_state->frame_di_scope, &dbgi_key, 0);
      eval_modules[eval_module_idx].vaddr_range = m->vaddr_rng;
      eval_modules[eval_module_idx].space       = df_eval_space_from_entity(df_entity_ancestor_from_kind(m, DF_EntityKind_Process));
      if(module == m)
      {
        eval_modules_primary = &eval_modules[eval_module_idx];
      }
    }
  }
  U64 rdis_count = Max(1, all_modules.count);
  RDI_Parsed **rdis = push_array(arena, RDI_Parsed *, rdis_count);
  rdis[0] = &di_rdi_parsed_nil;
  U64 rdis_primary_idx = 0;
  Rng1U64 *rdis_vaddr_ranges = push_array(arena, Rng1U64, rdis_count);
  {
    U64 idx = 0;
    for(DF_EntityNode *n = all_modules.first; n != 0; n = n->next, idx += 1)
    {
      DI_Key dbgi_key = df_dbgi_key_from_module(n->entity);
      rdis[idx] = di_rdi_from_key(df_state->frame_di_scope, &dbgi_key, 0);
      rdis_vaddr_ranges[idx] = n->entity->vaddr_rng;
      if(n->entity == module)
      {
        primary_dbgi_key = dbgi_key;
        rdis_primary_idx = idx;
      }
    }
  }
  
  //- rjf: build eval type context
  E_TypeCtx *type_ctx = push_array(arena, E_TypeCtx, 1);
  {
    E_TypeCtx *ctx = type_ctx;
    ctx->ip_vaddr          = rip_vaddr;
    ctx->ip_voff           = rip_voff;
    ctx->modules           = eval_modules;
    ctx->modules_count     = eval_modules_count;
    ctx->primary_module    = eval_modules_primary;
  }
  e_select_type_ctx(type_ctx);
  
  //- rjf: build eval parse context
  E_ParseCtx *parse_ctx = push_array(arena, E_ParseCtx, 1);
  ProfScope("build eval parse context")
  {
    E_ParseCtx *ctx = parse_ctx;
    ctx->ip_vaddr          = rip_vaddr;
    ctx->ip_voff           = rip_voff;
    ctx->ip_thread_space   = df_eval_space_from_entity(thread);
    ctx->modules           = eval_modules;
    ctx->modules_count     = eval_modules_count;
    ctx->primary_module    = eval_modules_primary;
    ctx->regs_map      = ctrl_string2reg_from_arch(ctx->primary_module->arch);
    ctx->reg_alias_map = ctrl_string2alias_from_arch(ctx->primary_module->arch);
    ctx->locals_map    = df_query_cached_locals_map_from_dbgi_key_voff(&primary_dbgi_key, rip_voff);
    ctx->member_map    = df_query_cached_member_map_from_dbgi_key_voff(&primary_dbgi_key, rip_voff);
  }
  e_select_parse_ctx(parse_ctx);
  
  //- rjf: build eval IR context
  E_IRCtx *ir_ctx = push_array(arena, E_IRCtx, 1);
  {
    E_IRCtx *ctx = ir_ctx;
    ctx->macro_map     = push_array(arena, E_String2ExprMap, 1);
    ctx->macro_map[0]  = e_string2expr_map_make(arena, 512);
    
    //- rjf: add macros for constants
    {
      // rjf: pid -> current process' ID
      if(!df_entity_is_nil(process))
      {
        E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafU64, 0);
        expr->value.u64 = process->ctrl_id;
        e_string2expr_map_insert(arena, ctx->macro_map, str8_lit("pid"), expr);
      }
      
      // rjf: tid -> current thread's ID
      if(!df_entity_is_nil(thread))
      {
        E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafU64, 0);
        expr->value.u64 = thread->ctrl_id;
        e_string2expr_map_insert(arena, ctx->macro_map, str8_lit("tid"), expr);
      }
    }
    
    //- rjf: add macros for entities
    {
      Temp scratch = scratch_begin(&arena, 1);
      E_MemberList entity_members = {0};
      {
        e_member_list_push_new(scratch.arena, &entity_members, .name = str8_lit("Enabled"),  .off = 0,        .type_key = e_type_key_basic(E_TypeKind_S64));
        e_member_list_push_new(scratch.arena, &entity_members, .name = str8_lit("Hit Count"),.off = 0+8,      .type_key = e_type_key_basic(E_TypeKind_U64));
        e_member_list_push_new(scratch.arena, &entity_members, .name = str8_lit("Label"),    .off = 0+8+8,    .type_key = e_type_key_cons_ptr(architecture_from_context(), e_type_key_basic(E_TypeKind_Char8)));
        e_member_list_push_new(scratch.arena, &entity_members, .name = str8_lit("Location"), .off = 0+8+8+8,  .type_key = e_type_key_cons_ptr(architecture_from_context(), e_type_key_basic(E_TypeKind_Char8)));
        e_member_list_push_new(scratch.arena, &entity_members, .name = str8_lit("Condition"),.off = 0+8+8+8+8,.type_key = e_type_key_cons_ptr(architecture_from_context(), e_type_key_basic(E_TypeKind_Char8)));
      }
      E_MemberArray entity_members_array = e_member_array_from_list(scratch.arena, &entity_members);
      E_TypeKey entity_type = e_type_key_cons(.arch = architecture_from_context(),
                                              .kind = E_TypeKind_Struct,
                                              .name = str8_lit("Entity"),
                                              .members = entity_members_array.v,
                                              .count = entity_members_array.count);
      DF_EntityKind evallable_kinds[] =
      {
        DF_EntityKind_Breakpoint,
        DF_EntityKind_WatchPin,
        DF_EntityKind_Target,
      };
      for(U64 idx = 0; idx < ArrayCount(evallable_kinds); idx += 1)
      {
        DF_EntityList entities = df_query_cached_entity_list_with_kind(evallable_kinds[idx]);
        for(DF_EntityNode *n = entities.first; n != 0; n = n->next)
        {
          DF_Entity *entity = n->entity;
          E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafOffset, 0);
          expr->space    = df_eval_space_from_entity(entity);
          expr->mode     = E_Mode_Offset;
          expr->type_key = entity_type;
          e_string2expr_map_insert(arena, ctx->macro_map, push_str8f(arena, "$%I64u", entity->id), expr);
          if(entity->name.size != 0)
          {
            e_string2expr_map_insert(arena, ctx->macro_map, entity->name, expr);
          }
        }
      }
      scratch_end(scratch);
    }
    
    //- rjf: add macros for all watches which define identifiers
    DF_EntityList watches = df_query_cached_entity_list_with_kind(DF_EntityKind_Watch);
    for(DF_EntityNode *n = watches.first; n != 0; n = n->next)
    {
      DF_Entity *watch = n->entity;
      String8 expr = watch->name;
      E_TokenArray tokens   = e_token_array_from_text(arena, expr);
      E_Parse      parse    = e_parse_expr_from_text_tokens(arena, expr, &tokens);
      if(parse.msgs.max_kind == E_MsgKind_Null)
      {
        e_push_leaf_ident_exprs_from_expr__in_place(arena, ctx->macro_map, parse.expr);
      }
    }
  }
  e_select_ir_ctx(ir_ctx);
  
  //- rjf: build eval interpretation context
  E_InterpretCtx *interpret_ctx = push_array(arena, E_InterpretCtx, 1);
  {
    E_InterpretCtx *ctx = interpret_ctx;
    ctx->space_read        = df_eval_space_read;
    ctx->space_write       = df_eval_space_write;
    ctx->primary_space     = eval_modules_primary->space;
    ctx->reg_arch          = eval_modules_primary->arch;
    ctx->reg_space         = df_eval_space_from_entity(thread);
    ctx->reg_unwind_count  = unwind_count;
    ctx->module_base       = push_array(arena, U64, 1);
    ctx->module_base[0]    = module->vaddr_rng.min;
    ctx->tls_base          = push_array(arena, U64, 1);
    ctx->tls_base[0]       = df_query_cached_tls_base_vaddr_from_process_root_rip(process, tls_root_vaddr, rip_vaddr);
  }
  e_select_interpret_ctx(interpret_ctx);
  
  ProfEnd();
}

internal void
df_core_end_frame(void)
{
  ProfBeginFunction();
  
  //- rjf: entity mutation -> soft halt
  if(df_state->entities_mut_soft_halt)
  {
    df_state->entities_mut_soft_halt = 0;
    DF_CmdParams params = df_cmd_params_zero();
    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SoftHaltRefresh));
  }
  
  //- rjf: entity mutation -> send refreshed debug info map
  if(df_state->entities_mut_dbg_info_map) ProfScope("entity mutation -> send refreshed debug info map")
  {
    df_state->entities_mut_dbg_info_map = 0;
    // TODO(rjf)
  }
  
  //- rjf: send messages
  if(df_state->ctrl_msgs.count != 0)
  {
    if(ctrl_u2c_push_msgs(&df_state->ctrl_msgs, os_now_microseconds()+100))
    {
      MemoryZeroStruct(&df_state->ctrl_msgs);
      arena_clear(df_state->ctrl_msg_arena);
    }
  }
  
  //- rjf: eliminate entities that are marked for deletion
  ProfScope("eliminate deleted entities")
  {
    for(DF_Entity *entity = df_entity_root(), *next = 0; !df_entity_is_nil(entity); entity = next)
    {
      next = df_entity_rec_df_pre(entity, &df_g_nil_entity).next;
      if(entity->flags & DF_EntityFlag_MarkedForDeletion)
      {
        B32 undoable = (df_g_entity_kind_flags_table[entity->kind] & DF_EntityKindFlag_UserDefinedLifetime);
        
        // rjf: fixup next entity to iterate to
        next = df_entity_rec_df(entity, &df_g_nil_entity, OffsetOf(DF_Entity, next), OffsetOf(DF_Entity, next)).next;
        
        // rjf: eliminate root entity if we're freeing it
        if(entity == df_state->entities_root)
        {
          df_state->entities_root = &df_g_nil_entity;
        }
        
        // rjf: unhook & release this entity tree
        df_entity_change_parent(entity, entity->parent, &df_g_nil_entity, &df_g_nil_entity);
        df_entity_release(entity);
      }
    }
  }
  
  //- rjf: garbage collect eliminated thread unwinds
  for(U64 slot_idx = 0; slot_idx < df_state->unwind_cache.slots_count; slot_idx += 1)
  {
    DF_UnwindCacheSlot *slot = &df_state->unwind_cache.slots[slot_idx];
    for(DF_UnwindCacheNode *n = slot->first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      if(df_entity_is_nil(df_entity_from_handle(n->thread)))
      {
        DLLRemove(slot->first, slot->last, n);
        arena_release(n->arena);
        SLLStackPush(df_state->unwind_cache.free_node, n);
      }
    }
  }
  
  //- rjf: write config changes
  ProfScope("write config changes")
  {
    for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1)) ProfScope("write %.*s config data", str8_varg(df_g_cfg_src_string_table[src]))
    {
      if(df_state->cfg_write_issued[src])
      {
        df_state->cfg_write_issued[src] = 0;
        String8 path = df_cfg_path_from_src(src);
        os_write_data_list_to_file_path(path, df_state->cfg_write_data[src]);
      }
      arena_clear(df_state->cfg_write_arenas[src]);
      MemoryZeroStruct(&df_state->cfg_write_data[src]);
    }
  }
  
  //- rjf: end scopes
  di_scope_close(df_state->frame_di_scope);
  
  ProfEnd();
}
