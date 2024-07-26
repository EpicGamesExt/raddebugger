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
df_state_delta_history_push_batch(DF_StateDeltaHistory *hist, U64 *optional_gen_ptr)
{
  if(hist == 0) { return; }
  if(hist->side_arenas[Side_Max] != 0)
  {
    arena_clear(hist->side_arenas[Side_Max]);
    hist->side_tops[Side_Max] = 0;
  }
  DF_StateDeltaBatch *batch = push_array(hist->side_arenas[Side_Min], DF_StateDeltaBatch, 1);
  SLLStackPush(hist->side_tops[Side_Min], batch);
  if(optional_gen_ptr != 0)
  {
    batch->gen = *optional_gen_ptr;
    batch->gen_vaddr = (U64)optional_gen_ptr;
  }
}

internal void
df_state_delta_history_push_delta(DF_StateDeltaHistory *hist, void *ptr, U64 size)
{
  if(hist == 0) { return; }
  DF_StateDeltaBatch *batch = hist->side_tops[Side_Min];
  if(batch == 0)
  {
    df_state_delta_history_push_batch(hist, 0);
    batch = hist->side_tops[Side_Min];
  }
  DF_StateDeltaNode *n = push_array(hist->side_arenas[Side_Min], DF_StateDeltaNode, 1);
  SLLQueuePush(batch->first, batch->last, n);
  n->v.vaddr = (U64)ptr;
  n->v.data = push_str8_copy(hist->arena, str8((U8*)ptr, size));
}

internal void
df_state_delta_history_wind(DF_StateDeltaHistory *hist, Side side)
{
  if(hist == 0) { return; }
  DF_StateDeltaBatch *src_batch = hist->side_tops[side];
  if(src_batch != 0)
  {
    B32 src_batch_gen_good = (src_batch->gen_vaddr == 0 || src_batch->gen == *(U64 *)(src_batch->gen_vaddr));
    U64 pop_pos = (U64)hist->side_tops[side] - (U64)hist->side_arenas[side];
    SLLStackPop(hist->side_tops[side]);
    if(src_batch_gen_good)
    {
      DF_StateDeltaBatch *dst_batch = push_array(hist->side_arenas[side_flip(side)], DF_StateDeltaBatch, 1);
      SLLStackPush(hist->side_tops[side_flip(side)], dst_batch);
      for(DF_StateDeltaNode *src_n = src_batch->first; src_n != 0; src_n = src_n->next)
      {
        DF_StateDelta *src_delta = &src_n->v;
        DF_StateDeltaNode *dst_n = push_array(hist->side_arenas[side_flip(side)], DF_StateDeltaNode, 1);
        SLLQueuePush(dst_batch->first, dst_batch->last, dst_n);
        dst_n->v.vaddr = src_delta->vaddr;
        dst_n->v.data = push_str8_copy(hist->side_arenas[side_flip(side)], str8((U8 *)src_delta->vaddr, src_delta->data.size));
        MemoryCopy((void *)src_delta->vaddr, src_delta->data.str, src_delta->data.size);
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

internal DF_CfgNode *
df_cfg_tree_copy(Arena *arena, DF_CfgNode *src_root)
{
  DF_CfgNode *dst_root = &df_g_nil_cfg_node;
  DF_CfgNode *dst_parent = dst_root;
  {
    DF_CfgNodeRec rec = {0};
    for(DF_CfgNode *src = src_root; src != &df_g_nil_cfg_node; src = rec.next)
    {
      DF_CfgNode *dst = push_array(arena, DF_CfgNode, 1);
      dst->first = dst->last = dst->parent = dst->next = &df_g_nil_cfg_node;
      dst->flags = src->flags;
      dst->string = push_str8_copy(arena, src->string);
      dst->source = src->source;
      dst->parent = dst_parent;
      if(dst_parent != &df_g_nil_cfg_node)
      {
        SLLQueuePush_NZ(&df_g_nil_cfg_node, dst_parent->first, dst_parent->last, dst, next);
      }
      else
      {
        dst_root = dst_parent = dst;
      }
      rec = df_cfg_node_rec__depth_first_pre(src, src_root);
      if(rec.push_count != 0)
      {
        dst_parent = dst;
      }
      else for(U64 idx = 0; idx < rec.pop_count; idx += 1)
      {
        dst_parent = dst_parent->parent;
      }
    }
  }
  return dst_root;
}

internal DF_CfgNodeRec
df_cfg_node_rec__depth_first_pre(DF_CfgNode *node, DF_CfgNode *root)
{
  DF_CfgNodeRec rec = {0};
  rec.next = &df_g_nil_cfg_node;
  if(node->first != &df_g_nil_cfg_node)
  {
    rec.next = node->first;
    rec.push_count = 1;
  }
  else for(DF_CfgNode *p = node; p != &df_g_nil_cfg_node && p != root; p = p->parent, rec.pop_count += 1)
  {
    if(p->next != &df_g_nil_cfg_node)
    {
      rec.next = p->next;
      break;
    }
  }
  return rec;
}

internal void
df_cfg_table_push_unparsed_string(Arena *arena, DF_CfgTable *table, String8 string, DF_CfgSrc source)
{
  Temp scratch = scratch_begin(&arena, 1);
  if(table->slot_count == 0)
  {
    table->slot_count = 64;
    table->slots = push_array(arena, DF_CfgSlot, table->slot_count);
  }
  MD_TokenizeResult tokenize = md_tokenize_from_text(scratch.arena, string);
  MD_ParseResult parse = md_parse_from_text_tokens(scratch.arena, str8_lit(""), string, tokenize.tokens);
  MD_Node *md_root = parse.root;
  for(MD_EachNode(tln, md_root->first)) if(tln->string.size != 0)
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
    
    // rjf: deep copy tree into streamlined config structure
    DF_CfgNode *dst_root = &df_g_nil_cfg_node;
    {
      DF_CfgNode *dst_parent = &df_g_nil_cfg_node;
      for(MD_Node *src = tln, *src_next = 0; !md_node_is_nil(src); src = src_next)
      {
        src_next = 0;
        
        // rjf: copy
        DF_CfgNode *dst = push_array(arena, DF_CfgNode, 1);
        dst->first = dst->last = dst->parent = dst->next = &df_g_nil_cfg_node;
        if(dst_parent == &df_g_nil_cfg_node)
        {
          dst_root = dst;
        }
        else
        {
          SLLQueuePush_NZ(&df_g_nil_cfg_node, dst_parent->first, dst_parent->last, dst, next);
          dst->parent = dst_parent;
        }
        {
          dst->flags |= !!(src->flags & MD_NodeFlag_Identifier)    * DF_CfgNodeFlag_Identifier;
          dst->flags |= !!(src->flags & MD_NodeFlag_Numeric)       * DF_CfgNodeFlag_Numeric;
          dst->flags |= !!(src->flags & MD_NodeFlag_StringLiteral) * DF_CfgNodeFlag_StringLiteral;
          dst->string = push_str8_copy(arena, str8(src->string.str, src->string.size));
          dst->source = source;
        }
        
        // rjf: grab next
        if(!md_node_is_nil(src->first))
        {
          src_next = src->first;
          dst_parent = dst;
        }
        else for(MD_Node *p = src; !md_node_is_nil(p) && p != tln; p = p->parent, dst_parent = dst_parent->parent)
        {
          if(!md_node_is_nil(p->next))
          {
            src_next = p->next;
            break;
          }
        }
      }
    }
    
    // rjf: push tree into value
    SLLQueuePush_NZ(&df_g_nil_cfg_node, val->first, val->last, dst_root, next);
  }
  scratch_end(scratch);
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

internal DF_CfgNode *
df_cfg_node_child_from_string(DF_CfgNode *node, String8 string, StringMatchFlags flags)
{
  DF_CfgNode *result = &df_g_nil_cfg_node;
  for(DF_CfgNode *child = node->first; child != &df_g_nil_cfg_node; child = child->next)
  {
    if(str8_match(child->string, string, flags))
    {
      result = child;
      break;
    }
  }
  return result;
}

internal DF_CfgNode *
df_first_cfg_node_child_from_flags(DF_CfgNode *node, DF_CfgNodeFlags flags)
{
  DF_CfgNode *result = &df_g_nil_cfg_node;
  for(DF_CfgNode *child = node->first; child != &df_g_nil_cfg_node; child = child->next)
  {
    if(child->flags & flags)
    {
      result = child;
      break;
    }
  }
  return result;
}

internal String8
df_string_from_cfg_node_children(Arena *arena, DF_CfgNode *node)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  for(DF_CfgNode *child = node->first; child != &df_g_nil_cfg_node; child = child->next)
  {
    str8_list_push(scratch.arena, &strs, child->string);
  }
  String8 result = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return result;
}

internal Vec4F32
df_hsva_from_cfg_node(DF_CfgNode *node)
{
  Vec4F32 result = {0};
  DF_CfgNode *hsva = df_cfg_node_child_from_string(node, str8_lit("hsva"), StringMatchFlag_CaseInsensitive);
  DF_CfgNode *rgba = df_cfg_node_child_from_string(node, str8_lit("rgba"), StringMatchFlag_CaseInsensitive);
  DF_CfgNode *hsv  = df_cfg_node_child_from_string(node, str8_lit("hsv"),  StringMatchFlag_CaseInsensitive);
  DF_CfgNode *rgb  = df_cfg_node_child_from_string(node, str8_lit("rgb"),  StringMatchFlag_CaseInsensitive);
  if(hsva != &df_g_nil_cfg_node)
  {
    DF_CfgNode *hue = hsva->first;
    DF_CfgNode *sat = hue->next;
    DF_CfgNode *val = sat->next;
    DF_CfgNode *alp = val->next;
    F32 hue_f32 = (F32)f64_from_str8(hue->string);
    F32 sat_f32 = (F32)f64_from_str8(sat->string);
    F32 val_f32 = (F32)f64_from_str8(val->string);
    F32 alp_f32 = (F32)f64_from_str8(alp->string);
    result = v4f32(hue_f32, sat_f32, val_f32, alp_f32);
  }
  else if(hsv != &df_g_nil_cfg_node)
  {
    DF_CfgNode *hue = hsva->first;
    DF_CfgNode *sat = hue->next;
    DF_CfgNode *val = sat->next;
    F32 hue_f32 = (F32)f64_from_str8(hue->string);
    F32 sat_f32 = (F32)f64_from_str8(sat->string);
    F32 val_f32 = (F32)f64_from_str8(val->string);
    result = v4f32(hue_f32, sat_f32, val_f32, 1.f);
  }
  else if(rgba != &df_g_nil_cfg_node)
  {
    DF_CfgNode *red = rgba->first;
    DF_CfgNode *grn = red->next;
    DF_CfgNode *blu = grn->next;
    DF_CfgNode *alp = blu->next;
    F32 red_f32 = (F32)f64_from_str8(red->string);
    F32 grn_f32 = (F32)f64_from_str8(grn->string);
    F32 blu_f32 = (F32)f64_from_str8(blu->string);
    F32 alp_f32 = (F32)f64_from_str8(alp->string);
    Vec3F32 hsv = hsv_from_rgb(v3f32(red_f32, grn_f32, blu_f32));
    result = v4f32(hsv.x, hsv.y, hsv.z, alp_f32);
  }
  else if(rgb != &df_g_nil_cfg_node)
  {
    DF_CfgNode *red = rgba->first;
    DF_CfgNode *grn = red->next;
    DF_CfgNode *blu = grn->next;
    F32 red_f32 = (F32)f64_from_str8(red->string);
    F32 grn_f32 = (F32)f64_from_str8(grn->string);
    F32 blu_f32 = (F32)f64_from_str8(blu->string);
    Vec3F32 hsv = hsv_from_rgb(v3f32(red_f32, grn_f32, blu_f32));
    result = v4f32(hsv.x, hsv.y, hsv.z, 1.f);
  }
  return result;
}

internal String8
df_string_from_cfg_node_key(DF_CfgNode *node, String8 key, StringMatchFlags flags)
{
  DF_CfgNode *child = df_cfg_node_child_from_string(node, key, flags);
  return child->first->string;
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
df_cmd_params_apply_spec_query(Arena *arena, DF_CtrlCtx *ctrl_ctx, DF_CmdParams *params, DF_CmdSpec *spec, String8 query)
{
  String8 error = {0};
  B32 prefer_imm = 0;
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
    case DF_CmdParamSlot_VirtualAddr: prefer_imm = 0; goto use_numeric_eval;
    case DF_CmdParamSlot_VirtualOff: prefer_imm = 0; goto use_numeric_eval;
    case DF_CmdParamSlot_Index: prefer_imm = 1; goto use_numeric_eval;
    case DF_CmdParamSlot_ID: prefer_imm = 1; goto use_numeric_eval;
    use_numeric_eval:
    {
      Temp scratch = scratch_begin(&arena, 1);
      DI_Scope *scope = di_scope_open();
      DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
      U64 vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx->unwind_count);
      DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
      EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, vaddr);
      DF_Eval eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, query);
      if(eval.errors.count == 0)
      {
        TG_Kind eval_type_kind = tg_kind_from_key(tg_unwrapped_from_graph_rdi_key(parse_ctx.type_graph, parse_ctx.rdi, eval.type_key));
        if(eval_type_kind == TG_Kind_Ptr || eval_type_kind == TG_Kind_LRef || eval_type_kind == TG_Kind_RRef)
        {
          eval = df_value_mode_eval_from_eval(parse_ctx.type_graph, parse_ctx.rdi, ctrl_ctx, eval);
          prefer_imm = 1;
        }
        U64 u64 = !prefer_imm && eval.offset ? eval.offset : eval.imm_u64;
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
        error = push_str8f(scratch.arena, "Couldn't evaluate \"%S\" as an address", query);
      }
      di_scope_close(scope);
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
    handle.u64[1] = entity->generation;
  }
  return handle;
}

internal DF_Entity *
df_entity_from_handle(DF_Handle handle)
{
  DF_Entity *result = df_state->entities_base + handle.u64[0];
  if(handle.u64[0] >= df_state->entities_count || result->generation != handle.u64[1])
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
    if(!df_entity_is_nil(*MemberFromOffset(DF_Entity **, parent, sib_off)))
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
    if(!child->deleted && child->kind == kind)
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
    if(!child->deleted && child->kind == kind)
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
    if(!child->deleted && str8_match(child->name, string, 0) && child->kind == kind)
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
      if(e->kind == DF_EntityKind_File ||
         e->kind == DF_EntityKind_OverrideFileLink)
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
      else if(entity->flags & DF_EntityFlag_HasVAddr)
      {
        result = str8_from_u64(arena, entity->vaddr, 16, 16, 0);
      }
      else
      {
        DF_Entity *symb = df_entity_child_from_kind(entity, DF_EntityKind_EntryPointName);
        DF_Entity *file = df_entity_ancestor_from_kind(entity, DF_EntityKind_File);
        if(!df_entity_is_nil(symb))
        {
          result = push_str8_copy(arena, symb->name);
        }
        else if(!df_entity_is_nil(file) && entity->flags & DF_EntityFlag_HasTextPoint)
        {
          result = push_str8f(arena, "%S:%I64d:%I64d", file->name, entity->text_point.line, entity->text_point.column);
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
      String8 procedure_name = df_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff);
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
df_name_alloc(DF_StateDeltaHistory *hist, String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = df_name_bucket_idx_from_string_size(string.size);
  DF_NameChunkNode *node = df_state->free_name_chunks[bucket_idx];
  
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
            df_state->free_name_chunks[bucket_idx] = n->next;
          }
          else
          {
            prev->next = n->next;
          }
          node = n;
          break;
        }
      }
    }
    else
    {
      SLLStackPop(df_state->free_name_chunks[bucket_idx]);
    }
  }
  
  // rjf: no found node -> allocate new
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
    node = (DF_NameChunkNode *)chunk_memory;
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
df_name_release(DF_StateDeltaHistory *hist, String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = df_name_bucket_idx_from_string_size(string.size);
  DF_NameChunkNode *node = (DF_NameChunkNode *)string.str;
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
    if(e == entity && flags & DF_EntityKindFlag_LeafMutationProjectConfig)
    {
      DF_CmdParams p = {0};
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProjectData));
    }
    if(e == entity && flags & DF_EntityKindFlag_LeafMutationSoftHalt && df_ctrl_targets_running())
    {
      df_state->entities_mut_soft_halt = 1;
    }
    if(e == entity && flags & DF_EntityKindFlag_LeafMutationDebugInfoMap)
    {
      df_state->entities_mut_dbg_info_map = 1;
    }
    if(flags & DF_EntityKindFlag_TreeMutationSoftHalt && df_ctrl_targets_running())
    {
      df_state->entities_mut_soft_halt = 1;
    }
    if(flags & DF_EntityKindFlag_TreeMutationDebugInfoMap)
    {
      df_state->entities_mut_dbg_info_map = 1;
    }
  }
}

//- rjf: entity allocation + tree forming

internal DF_Entity *
df_entity_alloc(DF_StateDeltaHistory *hist, DF_Entity *parent, DF_EntityKind kind)
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
  
  // rjf: user-defined lifetimes -> push record of df_state info
  if(user_defined_lifetime)
  {
    df_state_delta_history_push_struct_delta(hist, &df_state->entities_root);
    df_state_delta_history_push_struct_delta(hist, &df_state->entities_free_count);
    df_state_delta_history_push_struct_delta(hist, &df_state->entities_active_count);
    df_state_delta_history_push_struct_delta(hist, &df_state->entities_free[free_list_idx]);
    df_state_delta_history_push_struct_delta(hist, &df_state->kind_alloc_gens[kind]);
  }
  
  // rjf: pop new entity off free-list
  DF_Entity *entity = df_state->entities_free[free_list_idx];
  SLLStackPop(df_state->entities_free[free_list_idx]);
  df_state->entities_free_count -= 1;
  df_state->entities_active_count += 1;
  
  // rjf: user-defined lifetimes -> push records of initial entity data
  if(user_defined_lifetime)
  {
    df_state_delta_history_push_struct_delta(hist, &entity->next);
    df_state_delta_history_push_struct_delta(hist, &entity->prev);
    df_state_delta_history_push_struct_delta(hist, &entity->first);
    df_state_delta_history_push_struct_delta(hist, &entity->last);
    df_state_delta_history_push_struct_delta(hist, &entity->parent);
    df_state_delta_history_push_struct_delta(hist, &entity->generation);
    df_state_delta_history_push_struct_delta(hist, &entity->id);
    df_state_delta_history_push_struct_delta(hist, &entity->kind);
    if(!df_entity_is_nil(parent))
    {
      df_state_delta_history_push_struct_delta(hist, &parent->first);
      df_state_delta_history_push_struct_delta(hist, &parent->last);
    }
    if(!df_entity_is_nil(parent->last))
    {
      df_state_delta_history_push_struct_delta(hist, &parent->last->next);
    }
  }
  
  // rjf: zero entity
  {
    U64 generation = entity->generation;
    MemoryZeroStruct(entity);
    entity->generation = generation;
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
  entity->generation += 1;
  entity->alloc_time_us = os_now_microseconds();
  
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
df_entity_release(DF_StateDeltaHistory *hist, DF_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: unpack
  U64 free_list_idx = !!(df_g_entity_kind_flags_table[entity->kind] & DF_EntityKindFlag_UserDefinedLifetime);
  
  // rjf: record pre-deletion entity state
  df_state_delta_history_push_struct_delta(hist, &df_state->entities_free_count);
  df_state_delta_history_push_struct_delta(hist, &df_state->entities_active_count);
  
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
    df_state_delta_history_push_struct_delta(hist, &task->e->first);
    df_state_delta_history_push_struct_delta(hist, &task->e->last);
    df_state_delta_history_push_struct_delta(hist, &task->e->next);
    df_state_delta_history_push_struct_delta(hist, &task->e->prev);
    df_state_delta_history_push_struct_delta(hist, &task->e->parent);
    df_state_delta_history_push_struct_delta(hist, &df_state->kind_alloc_gens[task->e->kind]);
    df_state_delta_history_push_struct_delta(hist, &df_state->entities_free[free_list_idx]);
    df_set_thread_freeze_state(task->e, 0);
    SLLStackPush(df_state->entities_free[free_list_idx], task->e);
    df_state->entities_free_count += 1;
    df_state->entities_active_count -= 1;
    task->e->generation += 1;
    if(task->e->name.size != 0)
    {
      df_name_release(hist, task->e->name);
    }
    df_state->kind_alloc_gens[task->e->kind] += 1;
  }
  
  scratch_end(scratch);
}

internal void
df_entity_change_parent(DF_StateDeltaHistory *hist, DF_Entity *entity, DF_Entity *old_parent, DF_Entity *new_parent)
{
  Assert(entity->parent == old_parent);
  
  // rjf: push delta records
  if(hist != 0)
  {
    if(!df_entity_is_nil(old_parent))
    {
      df_state_delta_history_push_struct_delta(df_state->hist, &old_parent->first);
      df_state_delta_history_push_struct_delta(df_state->hist, &old_parent->last);
    }
    if(!df_entity_is_nil(new_parent))
    {
      df_state_delta_history_push_struct_delta(df_state->hist, &new_parent->first);
      df_state_delta_history_push_struct_delta(df_state->hist, &new_parent->last);
    }
    if(!df_entity_is_nil(entity->prev))
    {
      df_state_delta_history_push_struct_delta(df_state->hist, &entity->prev->next);
    }
    if(!df_entity_is_nil(entity->next))
    {
      df_state_delta_history_push_struct_delta(df_state->hist, &entity->next->prev);
    }
    df_state_delta_history_push_struct_delta(df_state->hist, &entity->next);
    df_state_delta_history_push_struct_delta(df_state->hist, &entity->prev);
    df_state_delta_history_push_struct_delta(df_state->hist, &entity->parent);
  }
  
  // rjf: fix up links
  if(!df_entity_is_nil(old_parent))
  {
    DLLRemove_NPZ(&df_g_nil_entity, old_parent->first, old_parent->last, entity, next, prev);
  }
  if(!df_entity_is_nil(new_parent))
  {
    DLLPushBack_NPZ(&df_g_nil_entity, new_parent->first, new_parent->last, entity, next, prev);
  }
  entity->parent = new_parent;
  
  // rjf: notify
  df_entity_notify_mutation(entity);
  df_entity_notify_mutation(new_parent);
  df_entity_notify_mutation(old_parent);
}

//- rjf: entity simple equipment

internal void
df_entity_equip_txt_pt(DF_Entity *entity, TxtPt point)
{
  df_require_entity_nonnil(entity, return);
  entity->text_point = point;
  entity->flags |= DF_EntityFlag_HasTextPoint;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_txt_pt_alt(DF_Entity *entity, TxtPt point)
{
  df_require_entity_nonnil(entity, return);
  entity->text_point_alt = point;
  entity->flags |= DF_EntityFlag_HasTextPointAlt;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_entity_handle(DF_Entity *entity, DF_Handle handle)
{
  df_require_entity_nonnil(entity, return);
  entity->entity_handle = handle;
  entity->flags |= DF_EntityFlag_HasEntityHandle;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_b32(DF_Entity *entity, B32 b32)
{
  df_require_entity_nonnil(entity, return);
  entity->b32 = b32;
  entity->flags |= DF_EntityFlag_HasB32;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_u64(DF_Entity *entity, U64 u64)
{
  df_require_entity_nonnil(entity, return);
  entity->u64 = u64;
  entity->flags |= DF_EntityFlag_HasU64;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_rng1u64(DF_Entity *entity, Rng1U64 range)
{
  df_require_entity_nonnil(entity, return);
  entity->rng1u64 = range;
  entity->flags |= DF_EntityFlag_HasRng1U64;
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
  entity->color_hsva = hsva;
  entity->flags |= DF_EntityFlag_HasColor;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_death_timer(DF_Entity *entity, F32 seconds_til_death)
{
  df_require_entity_nonnil(entity, return);
  entity->flags |= DF_EntityFlag_DiesWithTime;
  entity->life_left = seconds_til_death;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_cfg_src(DF_Entity *entity, DF_CfgSrc cfg_src)
{
  df_require_entity_nonnil(entity, return);
  entity->cfg_src = cfg_src;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_timestamp(DF_Entity *entity, U64 timestamp)
{
  df_require_entity_nonnil(entity, return);
  entity->timestamp = timestamp;
  df_entity_notify_mutation(entity);
}

//- rjf: control layer correllation equipment

internal void
df_entity_equip_ctrl_machine_id(DF_Entity *entity, CTRL_MachineID machine_id)
{
  df_require_entity_nonnil(entity, return);
  entity->ctrl_machine_id = machine_id;
  entity->flags |= DF_EntityFlag_HasCtrlMachineID;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_ctrl_handle(DF_Entity *entity, DMN_Handle handle)
{
  df_require_entity_nonnil(entity, return);
  entity->ctrl_handle = handle;
  entity->flags |= DF_EntityFlag_HasCtrlHandle;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_arch(DF_Entity *entity, Architecture arch)
{
  df_require_entity_nonnil(entity, return);
  entity->arch = arch;
  entity->flags |= DF_EntityFlag_HasArch;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_ctrl_id(DF_Entity *entity, U32 id)
{
  df_require_entity_nonnil(entity, return);
  entity->ctrl_id = id;
  entity->flags |= DF_EntityFlag_HasCtrlID;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_stack_base(DF_Entity *entity, U64 stack_base)
{
  df_require_entity_nonnil(entity, return);
  entity->stack_base = stack_base;
  entity->flags |= DF_EntityFlag_HasStackBase;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_tls_root(DF_Entity *entity, U64 tls_root)
{
  df_require_entity_nonnil(entity, return);
  entity->tls_root = tls_root;
  entity->flags |= DF_EntityFlag_HasTLSRoot;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_vaddr_rng(DF_Entity *entity, Rng1U64 range)
{
  df_require_entity_nonnil(entity, return);
  entity->vaddr_rng = range;
  entity->flags |= DF_EntityFlag_HasVAddrRng;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_vaddr(DF_Entity *entity, U64 vaddr)
{
  df_require_entity_nonnil(entity, return);
  entity->vaddr = vaddr;
  entity->flags |= DF_EntityFlag_HasVAddr;
  df_entity_notify_mutation(entity);
}

//- rjf: name equipment

internal void
df_entity_equip_name(DF_StateDeltaHistory *hist, DF_Entity *entity, String8 name)
{
  df_require_entity_nonnil(entity, return);
  if(entity->name.size != 0)
  {
    df_name_release(hist, entity->name);
  }
  if(name.size != 0)
  {
    entity->name = df_name_alloc(hist, name);
  }
  else
  {
    entity->name = str8_zero();
  }
  entity->name_generation += 1;
  df_entity_notify_mutation(entity);
}

internal void
df_entity_equip_namef(DF_StateDeltaHistory *hist, DF_Entity *entity, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  df_entity_equip_name(hist, entity, string);
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
            next_parent = df_entity_alloc(0, parent, DF_EntityKind_File);
            df_entity_equip_name(0, next_parent, path_part_n->string);
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
        if(name_matches && child->kind == DF_EntityKind_OverrideFileLink)
        {
          next_parent = df_entity_from_handle(child->entity_handle);
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
            next_parent = df_entity_alloc(0, parent, DF_EntityKind_File);
            df_entity_equip_name(0, next_parent, path_part_n->string);
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

internal DF_EntityList
df_possible_overrides_from_entity(Arena *arena, DF_Entity *entity)
{
  Temp scratch = scratch_begin(&arena, 1);
  StringMatchFlags path_match_flags = path_match_flags_from_os(operating_system_from_context());
  DF_EntityList result = {0};
  df_entity_list_push(arena, &result, entity);
  {
    DF_EntityList links = df_query_cached_entity_list_with_kind(DF_EntityKind_OverrideFileLink);
    String8List p_chain_names_to_entity = {0};
    for(DF_Entity *p = entity;
        !df_entity_is_nil(p);
        str8_list_push_front(scratch.arena, &p_chain_names_to_entity, p->name), p = p->parent)
    {
      // rjf: gather all links which would redirect to this chain
      DF_EntityList links_going_to_p = {0};
      for(DF_EntityNode *n = links.first; n != 0; n = n->next)
      {
        DF_Entity *link_src = n->entity;
        DF_Entity *link_dst = df_entity_from_handle(link_src->entity_handle);
        if(link_dst == p)
        {
          df_entity_list_push(scratch.arena, &links_going_to_p, link_src);
        }
      }
      
      // rjf: for each link, gather possible overrides
      for(DF_EntityNode *n = links_going_to_p.first; n != 0; n = n->next)
      {
        DF_Entity *link_src = n->entity;
        DF_Entity *link_src_parent = link_src->parent;
        
        // rjf: find the sibling that this link overrides
        DF_Entity *link_overridden_sibling = &df_g_nil_entity;
        for(DF_Entity *child = link_src_parent->first;
            !df_entity_is_nil(child);
            child = child->next)
        {
          B32 name_matches = str8_match(child->name, link_src->name, path_match_flags);
          if(name_matches && child->kind == DF_EntityKind_File)
          {
            link_overridden_sibling = child;
            break;
          }
        }
        
        // rjf: descend tree if needed, by the chain names, find override
        DF_Entity *override = link_overridden_sibling;
        if(!df_entity_is_nil(override))
        {
          DF_Entity *parent = override;
          for(String8Node *path_part_n = p_chain_names_to_entity.first;
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
              next_parent = df_entity_alloc(0, parent, DF_EntityKind_File);
              df_entity_equip_name(0, next_parent, path_part_n->string);
              String8 path = df_full_path_from_entity(scratch.arena, next_parent);
              FileProperties file_properties = os_properties_from_file_path(path);
              next_parent->timestamp = file_properties.modified;
              next_parent->flags |= DF_EntityFlag_IsFolder * !!(file_properties.flags & FilePropertyFlag_IsFolder);
              next_parent->flags |= DF_EntityFlag_IsMissing * !!(file_properties.created == 0);
            }
            
            // rjf: next parent -> follow it
            parent = next_parent;
          }
          override = parent;
        }
        
        // rjf: valid override -> push
        if(!df_entity_is_nil(override))
        {
          df_entity_list_push(arena, &result, override);
        }
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
    if(!entity->deleted && entity->kind == kind)
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
    result = df_entity_alloc(0, df_entity_root(), DF_EntityKind_Machine);
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
      DF_Entity *file = df_entity_from_handle(lines.first->v.file);
      log_infof("line: {%S:%I64i}\n", file->name, lines.first->v.pt.line);
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
df_symbol_name_from_dbgi_key_voff(Arena *arena, DI_Key *dbgi_key, U64 voff)
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
      U64 name_size = 0;
      U8 *name_ptr = rdi_string_from_idx(rdi, procedure->name_string_idx, &name_size);
      result = push_str8_copy(arena, str8(name_ptr, name_size));
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
df_symbol_name_from_process_vaddr(Arena *arena, DF_Entity *process, U64 vaddr)
{
  String8 result = {0};
  {
    DF_Entity *module = df_module_from_process_vaddr(process, vaddr);
    DI_Key dbgi_key = df_dbgi_key_from_module(module);
    U64 voff = df_voff_from_vaddr(module, vaddr);
    result = df_symbol_name_from_dbgi_key_voff(arena, &dbgi_key, voff);
  }
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
    LineTableNode *top_line_table = &start_line_table;
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
    
    //- rjf: gather lines in each line table
    Rng1U64 shallowest_voff_range = {0};
    for(LineTableNode *n = top_line_table; n != 0; n = n->next)
    {
      RDI_ParsedLineTable parsed_line_table = n->parsed_line_table;
      U64 line_info_idx = rdi_line_info_idx_from_voff(&parsed_line_table, voff);
      if(line_info_idx < parsed_line_table.count)
      {
        RDI_Line *line = &parsed_line_table.lines[line_info_idx];
        RDI_Column *column = (line_info_idx < parsed_line_table.col_count) ? &parsed_line_table.cols[line_info_idx] : 0;
        RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
        String8 file_normalized_full_path = {0};
        file_normalized_full_path.str = rdi_string_from_idx(rdi, file->normal_full_path_string_idx, &file_normalized_full_path.size);
        DF_LineNode *n = push_array(arena, DF_LineNode, 1);
        SLLQueuePushFront(result.first, result.last, n);
        result.count += 1;
        if(line->file_idx != 0 && file_normalized_full_path.size != 0)
        {
          n->v.file = df_handle_from_entity(df_entity_from_path(file_normalized_full_path, DF_EntityFromPathFlag_All));
        }
        n->v.pt = txt_pt(line->line_num, column ? column->col_first : 1);
        n->v.voff_range = r1u64(parsed_line_table.voffs[line_info_idx], parsed_line_table.voffs[line_info_idx+1]);
        n->v.dbgi_key = *dbgi_key;
        shallowest_voff_range = n->v.voff_range;
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
df_lines_array_from_file_line_range(Arena *arena, DF_Entity *file, Rng1S64 line_num_range)
{
  DF_LineListArray array = {0};
  {
    array.count = dim_1s64(line_num_range)+1;
    array.v = push_array(arena, DF_LineList, array.count);
  }
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  DI_KeyList dbgi_keys = df_push_active_dbgi_key_list(scratch.arena);
  DF_EntityList overrides = df_possible_overrides_from_entity(scratch.arena, file);
  for(DF_EntityNode *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    DF_Entity *override = override_n->entity;
    String8 file_path = df_full_path_from_entity(scratch.arena, override);
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
df_lines_from_file_line_num(Arena *arena, DF_Entity *file, S64 line_num)
{
  DF_LineListArray array = df_lines_array_from_file_line_range(arena, file, r1s64(line_num, line_num+1));
  DF_LineList list = {0};
  if(array.count != 0)
  {
    list = array.v[0];
  }
  return list;
}

//- rjf: src -> voff lookups

internal DF_TextLineSrc2DasmInfoListArray
df_text_line_src2dasm_info_list_array_from_src_line_range(Arena *arena, DF_Entity *file, Rng1S64 line_num_range)
{
  DF_TextLineSrc2DasmInfoListArray src2dasm_array = {0};
  {
    src2dasm_array.count = dim_1s64(line_num_range)+1;
    src2dasm_array.v = push_array(arena, DF_TextLineSrc2DasmInfoList, src2dasm_array.count);
  }
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *scope = di_scope_open();
  DI_KeyList dbgi_keys = df_push_active_dbgi_key_list(scratch.arena);
  DF_EntityList overrides = df_possible_overrides_from_entity(scratch.arena, file);
  for(DF_EntityNode *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    DF_Entity *override = override_n->entity;
    String8 file_path = df_full_path_from_entity(scratch.arena, override);
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
          DF_TextLineSrc2DasmInfoList *src2dasm_list = &src2dasm_array.v[line_idx];
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
              DF_TextLineSrc2DasmInfoNode *src2dasm_n = push_array(arena, DF_TextLineSrc2DasmInfoNode, 1);
              src2dasm_n->v.voff_range = range;
              src2dasm_n->v.remap_line = (S64)actual_line;
              src2dasm_n->v.dbgi_key = key;
              SLLQueuePush(src2dasm_list->first, src2dasm_list->last, src2dasm_n);
              src2dasm_list->count += 1;
            }
          }
        }
      }
      
      // rjf: good src id -> push to relevant dbgi keys
      if(good_src_id)
      {
        di_key_list_push(arena, &src2dasm_array.dbgi_keys, &key);
      }
    }
  }
  di_scope_close(scope);
  scratch_end(scratch);
  return src2dasm_array;
}

////////////////////////////////
//~ rjf: Process/Thread/Module Info Lookups

internal DF_Entity *
df_module_from_process_vaddr(DF_Entity *process, U64 vaddr)
{
  ProfBeginFunction();
  DF_Entity *module = &df_g_nil_entity;
  for(DF_Entity *child = process->first; !df_entity_is_nil(child); child = child->next)
  {
    if(child->kind == DF_EntityKind_Module && contains_1u64(child->vaddr_rng, vaddr))
    {
      module = child;
      break;
    }
  }
  ProfEnd();
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

internal EVAL_String2NumMap *
df_push_locals_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff)
{
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  EVAL_String2NumMap *result = eval_push_locals_map_from_rdi_voff(arena, rdi, voff);
  return result;
}

internal EVAL_String2NumMap *
df_push_member_map_from_dbgi_key_voff(Arena *arena, DI_Scope *scope, DI_Key *dbgi_key, U64 voff)
{
  RDI_Parsed *rdi = di_rdi_from_key(scope, dbgi_key, 0);
  EVAL_String2NumMap *result = eval_push_member_map_from_rdi_voff(arena, rdi, voff);
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
      if(user_bp->b32 == 0)
      {
        continue;
      }
      DF_Entity *file = df_entity_ancestor_from_kind(user_bp, DF_EntityKind_File);
      DF_Entity *symb = df_entity_child_from_kind(user_bp, DF_EntityKind_EntryPointName);
      DF_EntityList overrides = df_possible_overrides_from_entity(scratch.arena, file);
      for(DF_EntityNode *override_n = overrides.first; override_n != 0; override_n = override_n->next)
      {
        DF_Entity *override = override_n->entity;
        DF_Entity *condition_child = df_entity_child_from_kind(user_bp, DF_EntityKind_Condition);
        String8 condition = condition_child->name;
        
        // rjf: generate user breakpoint info depending on breakpoint placement
        CTRL_UserBreakpointKind ctrl_user_bp_kind = CTRL_UserBreakpointKind_FileNameAndLineColNumber;
        String8 ctrl_user_bp_string = {0};
        TxtPt ctrl_user_bp_pt = {0};
        U64 ctrl_user_bp_u64 = 0;
        {
          if(user_bp->flags & DF_EntityFlag_HasTextPoint)
          {
            ctrl_user_bp_kind = CTRL_UserBreakpointKind_FileNameAndLineColNumber;
            ctrl_user_bp_string = df_full_path_from_entity(scratch.arena, override);
            ctrl_user_bp_pt = user_bp->text_point;
          }
          else if(user_bp->flags & DF_EntityFlag_HasVAddr)
          {
            ctrl_user_bp_kind = CTRL_UserBreakpointKind_VirtualAddress;
            ctrl_user_bp_u64 = user_bp->vaddr;
          }
          else if(!df_entity_is_nil(symb))
          {
            ctrl_user_bp_kind = CTRL_UserBreakpointKind_SymbolNameAndOffset;
            ctrl_user_bp_string = symb->name;
          }
        }
        
        // rjf: push user breakpoint to list
        {
          CTRL_UserBreakpoint ctrl_user_bp = {ctrl_user_bp_kind};
          ctrl_user_bp.string = ctrl_user_bp_string;
          ctrl_user_bp.pt = ctrl_user_bp_pt;
          ctrl_user_bp.u64 = ctrl_user_bp_u64;
          ctrl_user_bp.condition = condition;
          ctrl_user_breakpoint_list_push(scratch.arena, &msg.user_bps, &ctrl_user_bp);
        }
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
  
  // rjf: set control context to top unwind
  df_state->ctrl_ctx.unwind_count = 0;
  df_state->ctrl_ctx.inline_depth = 0;
  
  scratch_end(scratch);
}

//- rjf: stopped info from the control thread

internal CTRL_Event
df_ctrl_last_stop_event(void)
{
  return df_state->ctrl_last_stop_event;
}

////////////////////////////////
//~ rjf: Evaluation

internal B32
df_eval_memory_read(void *u, void *out, U64 addr, U64 size)
{
  DF_Entity *process = (DF_Entity *)u;
  Assert(process->kind == DF_EntityKind_Process);
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, r1u64(addr, addr+size), 0);
  String8 data = slice.data;
  if(data.size == size)
  {
    result = 1;
    MemoryCopy(out, data.str, data.size);
  }
  scratch_end(scratch);
  return result;
}

internal EVAL_ParseCtx
df_eval_parse_ctx_from_process_vaddr(DI_Scope *scope, DF_Entity *process, U64 vaddr)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: extract info
  DF_Entity *module = df_module_from_process_vaddr(process, vaddr);
  U64 voff = df_voff_from_vaddr(module, vaddr);
  DI_Key dbgi_key = df_dbgi_key_from_module(module);
  RDI_Parsed *rdi = di_rdi_from_key(scope, &dbgi_key, 0);
  Architecture arch = df_architecture_from_entity(process);
  EVAL_String2NumMap *reg_map = ctrl_string2reg_from_arch(arch);
  EVAL_String2NumMap *reg_alias_map = ctrl_string2alias_from_arch(arch);
  EVAL_String2NumMap *locals_map = df_query_cached_locals_map_from_dbgi_key_voff(&dbgi_key, voff);
  EVAL_String2NumMap *member_map = df_query_cached_member_map_from_dbgi_key_voff(&dbgi_key, voff);
  
  //- rjf: build ctx
  EVAL_ParseCtx ctx = zero_struct;
  {
    ctx.arch            = arch;
    ctx.ip_voff         = voff;
    ctx.rdi             = rdi;
    ctx.type_graph      = tg_graph_begin(bit_size_from_arch(arch)/8, 256);
    ctx.regs_map        = reg_map;
    ctx.reg_alias_map   = reg_alias_map;
    ctx.locals_map      = locals_map;
    ctx.member_map      = member_map;
  }
  scratch_end(scratch);
  return ctx;
}

internal EVAL_ParseCtx
df_eval_parse_ctx_from_src_loc(DI_Scope *scope, DF_Entity *file, TxtPt pt)
{
  Temp scratch = scratch_begin(0, 0);
  EVAL_ParseCtx ctx = zero_struct;
  DI_KeyList dbgi_keys = df_push_active_dbgi_key_list(scratch.arena);
  DF_TextLineSrc2DasmInfoList src2dasm_list = {0};
  
  //- rjf: search for line info in all binaries for this file:pt
  DF_EntityList overrides = df_possible_overrides_from_entity(scratch.arena, file);
  for(DF_EntityNode *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    DF_Entity *override = override_n->entity;
    String8 file_path = df_full_path_from_entity(scratch.arena, override);
    String8 file_path_normalized = lower_from_str8(scratch.arena, file_path);
    for(DI_KeyNode *dbgi_key_n = dbgi_keys.first;
        dbgi_key_n != 0;
        dbgi_key_n = dbgi_key_n->next)
    {
      // rjf: key -> rdi
      DI_Key key = dbgi_key_n->v;
      RDI_Parsed *rdi = di_rdi_from_key(scope, &key, 0);
      
      // rjf: file_path_normalized * rdi -> src_id
      B32 good_src_id = 0;
      U32 src_id = 0;
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
        U32 voff_count = 0;
        U64 *voffs = rdi_line_voffs_from_num(&line_map, (U32)pt.line, &voff_count);
        for(U64 idx = 0; idx < voff_count; idx += 1)
        {
          U64 base_voff = voffs[idx];
          U64 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, base_voff);
          RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
          RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
          RDI_ParsedLineTable unit_line_info = {0};
          rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
          U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, base_voff);
          Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
          S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
          DF_TextLineSrc2DasmInfoNode *src2dasm_n = push_array(scratch.arena, DF_TextLineSrc2DasmInfoNode, 1);
          src2dasm_n->v.voff_range = range;
          src2dasm_n->v.remap_line = (S64)actual_line;
          src2dasm_n->v.dbgi_key = key;
          SLLQueuePush(src2dasm_list.first, src2dasm_list.last, src2dasm_n);
          src2dasm_list.count += 1;
        }
      }
    }
  }
  
  //- rjf: try to form ctx from line info
  B32 good_ctx = 0;
  if(src2dasm_list.count != 0)
  {
    for(DF_TextLineSrc2DasmInfoNode *n = src2dasm_list.first; n != 0; n = n->next)
    {
      DF_TextLineSrc2DasmInfo *src2dasm = &n->v;
      DF_EntityList modules = df_modules_from_dbgi_key(scratch.arena, &src2dasm->dbgi_key);
      if(modules.count != 0)
      {
        DF_Entity *module = modules.first->entity;
        DF_Entity *process = df_entity_ancestor_from_kind(module, DF_EntityKind_Process);
        U64 voff = src2dasm->voff_range.min;
        U64 vaddr = df_vaddr_from_voff(module, voff);
        ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, vaddr);
        good_ctx = 1;
        break;
      }
    }
  }
  
  //- rjf: bad ctx -> reset with graceful defaults
  if(good_ctx == 0)
  {
    ctx.rdi             = &di_rdi_parsed_nil;
    ctx.type_graph      = tg_graph_begin(8, 256);
    ctx.regs_map        = &eval_string2num_map_nil;
    ctx.regs_map        = &eval_string2num_map_nil;
    ctx.reg_alias_map   = &eval_string2num_map_nil;
    ctx.locals_map      = &eval_string2num_map_nil;
    ctx.member_map      = &eval_string2num_map_nil;
  }
  
  scratch_end(scratch);
  return ctx;
}

internal DF_Eval
df_eval_from_string(Arena *arena, DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, String8 string)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: unpack arguments
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  U64 tls_root_vaddr = ctrl_query_cached_tls_root_vaddr_from_thread(df_state->ctrl_entity_store, thread->ctrl_machine_id, thread->ctrl_handle);
  DF_Entity *process = thread->parent;
  U64 unwind_count = ctrl_ctx->unwind_count;
  CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
  Architecture arch = df_architecture_from_entity(thread);
  U64 reg_size = regs_block_size_from_architecture(arch);
  U64 thread_unwind_ip_vaddr = 0;
  void *thread_unwind_regs_block = push_array(scratch.arena, U8, reg_size);
  if(unwind.frames.count != 0)
  {
    thread_unwind_regs_block = unwind.frames.v[unwind_count%unwind.frames.count].regs;
    thread_unwind_ip_vaddr = regs_rip_from_arch_block(arch, thread_unwind_regs_block);
  }
  
  //- rjf: unpack module info & produce eval machine
  DF_Entity *module = df_module_from_process_vaddr(process, thread_unwind_ip_vaddr);
  U64 module_base = df_base_vaddr_from_module(module);
  U64 tls_base = df_query_cached_tls_base_vaddr_from_process_root_rip(process, tls_root_vaddr, thread_unwind_ip_vaddr);
  EVAL_Machine machine = {0};
  machine.u = (void *)thread->parent;
  machine.arch = arch;
  machine.memory_read = df_eval_memory_read;
  machine.reg_data = thread_unwind_regs_block;
  machine.reg_size = reg_size;
  machine.module_base = &module_base;
  machine.tls_base = &tls_base;
  
  //- rjf: lex & parse
  EVAL_TokenArray tokens = eval_token_array_from_text(arena, string);
  EVAL_ParseResult parse = eval_parse_expr_from_text_tokens(arena, parse_ctx, string, &tokens);
  EVAL_ErrorList errors = parse.errors;
  B32 parse_has_expr = (parse.expr != &eval_expr_nil);
  B32 parse_is_type = (parse_has_expr && parse.expr->kind == EVAL_ExprKind_TypeIdent);
  
  //- rjf: produce IR tree & type
  EVAL_IRTreeAndType ir_tree_and_type = {&eval_irtree_nil};
  if(parse_has_expr && errors.count == 0)
  {
    ir_tree_and_type = eval_irtree_and_type_from_expr(arena, parse_ctx->type_graph, parse_ctx->rdi, macro_map, parse.expr, &errors);
  }
  
  //- rjf: get list of ops
  EVAL_OpList op_list = {0};
  if(parse_has_expr && ir_tree_and_type.tree != &eval_irtree_nil)
  {
    eval_oplist_from_irtree(arena, ir_tree_and_type.tree, &op_list);
  }
  
  //- rjf: get bytecode string
  String8 bytecode = {0};
  if(parse_has_expr && parse_is_type == 0 && op_list.encoded_size != 0)
  {
    bytecode = eval_bytecode_from_oplist(arena, &op_list);
  }
  
  //- rjf: evaluate
  EVAL_Result eval = {0};
  if(bytecode.size != 0)
  {
    eval = eval_interpret(&machine, bytecode);
  }
  
  //- rjf: fill result
  DF_Eval result = zero_struct;
  {
    result.type_key = ir_tree_and_type.type_key;
    result.mode = ir_tree_and_type.mode;
    switch(result.mode)
    {
      default:
      case EVAL_EvalMode_Value:
      {
        MemoryCopyArray(result.imm_u128, eval.value.u128);
      }break;
      case EVAL_EvalMode_Addr:
      {
        result.offset = eval.value.u64;
      }break;
      case EVAL_EvalMode_Reg:
      {
        U64 reg_off  = (eval.value.u64 & 0x0000ffff) >> 0;
        U64 reg_size = (eval.value.u64 & 0xffff0000) >> 16;
        result.offset = reg_off;
        (void)reg_size;
      }break;
    }
    result.errors = errors;
    if(EVAL_ResultCode_Good < eval.code && eval.code < EVAL_ResultCode_COUNT)
    {
      eval_error(arena, &result.errors, EVAL_ErrorKind_InterpretationError, 0, eval_result_code_display_strings[eval.code]);
    }
  }
  
  //- rjf: apply dynamic type overrides
  if(parse.expr != 0 && parse.expr->kind != EVAL_ExprKind_Cast)
  {
    result = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, result);
  }
  
  //- rjf: try to resolve basic integral values into symbols
  if((result.mode == EVAL_EvalMode_Value || result.mode == EVAL_EvalMode_Reg) && parse.expr->kind != EVAL_ExprKind_Cast &&
     (tg_key_match(result.type_key, tg_key_basic(TG_Kind_S64)) ||
      tg_key_match(result.type_key, tg_key_basic(TG_Kind_U64)) ||
      tg_key_match(result.type_key, tg_key_basic(TG_Kind_S32)) ||
      tg_key_match(result.type_key, tg_key_basic(TG_Kind_U32))))
  {
    U64 vaddr = result.imm_u64;
    DF_Entity *module = df_module_from_process_vaddr(process, vaddr);
    DI_Key dbgi_key = df_dbgi_key_from_module(module);
    U64 voff = df_voff_from_vaddr(module, vaddr);
    String8 symbol_name = df_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, voff);
    if(symbol_name.size != 0)
    {
      result.type_key = tg_cons_type_make(parse_ctx->type_graph, TG_Kind_Ptr, tg_key_basic(TG_Kind_Void), 0);
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal DF_Eval
df_value_mode_eval_from_eval(TG_Graph *graph, RDI_Parsed *rdi, DF_CtrlCtx *ctrl_ctx, DF_Eval eval)
{
  ProfBeginFunction();
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = thread->parent;
  switch(eval.mode)
  {
    //- rjf: no work to be done. already in value mode
    default:
    case EVAL_EvalMode_Value:{}break;
    
    //- rjf: address => resolve into value, if leaf
    case EVAL_EvalMode_Addr:
    {
      TG_Key type_key = eval.type_key;
      TG_Kind type_kind = tg_kind_from_key(type_key);
      U64 type_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, type_key);
      if(!tg_key_match(type_key, tg_key_zero()) && type_byte_size <= sizeof(U64)*2)
      {
        Temp scratch = scratch_begin(0, 0);
        Rng1U64 vaddr_range = r1u64(eval.offset, eval.offset + type_byte_size);
        if(dim_1u64(vaddr_range) == type_byte_size)
        {
          CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, vaddr_range, 0);
          String8 data = slice.data;
          MemoryZeroArray(eval.imm_u128);
          MemoryCopy(eval.imm_u128, data.str, Min(data.size, sizeof(U64)*2));
          eval.mode = EVAL_EvalMode_Value;
          
          // rjf: mask&shift, for bitfields
          if(type_kind == TG_Kind_Bitfield && type_byte_size <= sizeof(U64))
          {
            TG_Type *type = tg_type_from_graph_rdi_key(scratch.arena, graph, rdi, type_key);
            U64 valid_bits_mask = 0;
            for(U64 idx = 0; idx < type->count; idx += 1)
            {
              valid_bits_mask |= (1<<idx);
            }
            eval.imm_u64 = eval.imm_u64 >> type->off;
            eval.imm_u64 = eval.imm_u64 & valid_bits_mask;
            eval.type_key = type->direct_type_key;
          }
          
          // rjf: manually sign-extend
          switch(type_kind)
          {
            default: break;
            case TG_Kind_S8:  {eval.imm_s64 = (S64)*((S8 *)&eval.imm_u64);}break;
            case TG_Kind_S16: {eval.imm_s64 = (S64)*((S16 *)&eval.imm_u64);}break;
            case TG_Kind_S32: {eval.imm_s64 = (S64)*((S32 *)&eval.imm_u64);}break;
          }
        }
        scratch_end(scratch);
      }
    }break;
    
    //- rjf: register => resolve into value
    case EVAL_EvalMode_Reg:
    {
      TG_Key type_key = eval.type_key;
      U64 type_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, type_key);
      U64 reg_off = eval.offset;
      CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
      if(unwind.frames.count != 0)
      {
        CTRL_UnwindFrame *frame = &unwind.frames.v[ctrl_ctx->unwind_count%unwind.frames.count];
        MemoryCopy(&eval.imm_u128[0], ((U8 *)frame->regs + reg_off), Min(type_byte_size, sizeof(U64)*2));
      }
      eval.mode = EVAL_EvalMode_Value;
    }break;
  }
  
  ProfEnd();
  return eval;
}

internal DF_Eval
df_dynamically_typed_eval_from_eval(TG_Graph *graph, RDI_Parsed *rdi, DF_CtrlCtx *ctrl_ctx, DF_Eval eval)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  Architecture arch = df_architecture_from_entity(thread);
  DF_Entity *process = thread->parent;
  U64 unwind_count = ctrl_ctx->unwind_count;
  U64 thread_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
  DF_Entity *module = df_module_from_process_vaddr(process, thread_rip_vaddr);
  TG_Key type_key = eval.type_key;
  TG_Kind type_kind = tg_kind_from_key(type_key);
  if(type_kind == TG_Kind_Ptr)
  {
    TG_Key ptee_type_key = tg_unwrapped_direct_from_graph_rdi_key(graph, rdi, type_key);
    TG_Kind ptee_type_kind = tg_kind_from_key(ptee_type_key);
    if(ptee_type_kind == TG_Kind_Struct || ptee_type_kind == TG_Kind_Class)
    {
      TG_Type *ptee_type = tg_type_from_graph_rdi_key(scratch.arena, graph, rdi, ptee_type_key);
      B32 has_vtable = 0;
      for(U64 idx = 0; idx < ptee_type->count; idx += 1)
      {
        if(ptee_type->members[idx].kind == TG_MemberKind_VirtualMethod)
        {
          has_vtable = 1;
          break;
        }
      }
      if(has_vtable)
      {
        U64 ptr_vaddr = eval.offset;
        U64 addr_size = bit_size_from_arch(arch)/8;
        CTRL_ProcessMemorySlice ptr_value_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle,
                                                                                                  r1u64(ptr_vaddr, ptr_vaddr+addr_size), 0);
        String8 ptr_value_memory = ptr_value_slice.data;
        if(ptr_value_memory.size >= addr_size)
        {
          U64 class_base_vaddr = 0;
          MemoryCopy(&class_base_vaddr, ptr_value_memory.str, addr_size);
          CTRL_ProcessMemorySlice vtable_base_ptr_slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle,
                                                                                                          r1u64(class_base_vaddr, class_base_vaddr+addr_size), 0);
          String8 vtable_base_ptr_memory = vtable_base_ptr_slice.data;
          if(vtable_base_ptr_memory.size >= addr_size)
          {
            U64 vtable_vaddr = 0;
            MemoryCopy(&vtable_vaddr, vtable_base_ptr_memory.str, addr_size);
            U64 vtable_voff = df_voff_from_vaddr(module, vtable_vaddr);
            U64 global_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_GlobalVMap, vtable_voff);
            RDI_GlobalVariable *global_var = rdi_element_from_name_idx(rdi, GlobalVariables, global_idx);
            if(global_var->link_flags & RDI_LinkFlag_TypeScoped)
            {
              RDI_UDT *udt = rdi_element_from_name_idx(rdi, UDTs, global_var->container_idx);
              RDI_TypeNode *type = rdi_element_from_name_idx(rdi, TypeNodes, udt->self_type_idx);
              TG_Key derived_type_key = tg_key_ext(tg_kind_from_rdi_type_kind(type->kind), (U64)udt->self_type_idx);
              TG_Key ptr_to_derived_type_key = tg_cons_type_make(graph, TG_Kind_Ptr, derived_type_key, 0);
              eval.type_key = ptr_to_derived_type_key;
            }
          }
        }
      }
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return eval;
}

internal DF_Eval
df_eval_from_eval_cfg_table(Arena *arena, DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_Eval eval, DF_CfgTable *cfg)
{
  ProfBeginFunction();
  
  //- rjf: apply view rules
  for(DF_CfgVal *val = cfg->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
  {
    DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
    if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_EvalResolution)
    {
      eval = spec->info.eval_resolution(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval, val);
      goto end_resolve;
    }
  }
  end_resolve:;
  ProfEnd();
  return eval;
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
df_string_from_simple_typed_eval(Arena *arena, TG_Graph *graph, RDI_Parsed *rdi, DF_EvalVizStringFlags flags, U32 radix, DF_Eval eval)
{
  ProfBeginFunction();
  String8 result = {0};
  TG_Key type_key = tg_unwrapped_from_graph_rdi_key(graph, rdi, eval.type_key);
  TG_Kind type_kind = tg_kind_from_key(type_key);
  U64 type_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, type_key);
  U8 digit_group_separator = 0;
  if(!(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules))
  {
    digit_group_separator = 0;
  }
  switch(type_kind)
  {
    default:{}break;
    
    case TG_Kind_Handle:
    {
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      result = str8_from_s64(arena, eval.imm_s64, radix, 0, digit_group_separator);
    }break;
    
    case TG_Kind_Char8:
    case TG_Kind_Char16:
    case TG_Kind_Char32:
    case TG_Kind_UChar8:
    case TG_Kind_UChar16:
    case TG_Kind_UChar32:
    {
      String8 char_str = df_string_from_ascii_value(arena, eval.imm_s64);
      if(char_str.size != 0)
      {
        if(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules)
        {
          String8 imm_string = str8_from_s64(arena, eval.imm_s64, radix, 0, digit_group_separator);
          result = push_str8f(arena, "'%S' (%S)", char_str, imm_string);
        }
        else
        {
          result = push_str8f(arena, "'%S'", char_str);
        }
      }
      else
      {
        result = str8_from_s64(arena, eval.imm_s64, radix, 0, digit_group_separator);
      }
    }break;
    
    case TG_Kind_S8:
    case TG_Kind_S16:
    case TG_Kind_S32:
    case TG_Kind_S64:
    {
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      result = str8_from_s64(arena, eval.imm_s64, radix, 0, digit_group_separator);
    }break;
    
    case TG_Kind_U8:
    case TG_Kind_U16:
    case TG_Kind_U32:
    case TG_Kind_U64:
    {
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      result = str8_from_u64(arena, eval.imm_u64, radix, min_digits, digit_group_separator);
    }break;
    
    case TG_Kind_U128:
    {
      Temp scratch = scratch_begin(&arena, 1);
      U64 min_digits = (radix == 16) ? type_byte_size*2 : 0;
      String8 upper64 = str8_from_u64(scratch.arena, eval.imm_u128[0], radix, min_digits, digit_group_separator);
      String8 lower64 = str8_from_u64(scratch.arena, eval.imm_u128[1], radix, min_digits, digit_group_separator);
      result = push_str8f(arena, "%S:%S", upper64, lower64);
      scratch_end(scratch);
    }break;
    
    case TG_Kind_F32: {result = push_str8f(arena, "%f", eval.imm_f32);}break;
    case TG_Kind_F64: {result = push_str8f(arena, "%f", eval.imm_f64);}break;
    case TG_Kind_Bool:{result = push_str8f(arena, "%s", eval.imm_u64 ? "true" : "false");}break;
    case TG_Kind_Ptr: {result = push_str8f(arena, "0x%I64x", eval.imm_u64);}break;
    case TG_Kind_LRef:{result = push_str8f(arena, "0x%I64x", eval.imm_u64);}break;
    case TG_Kind_RRef:{result = push_str8f(arena, "0x%I64x", eval.imm_u64);}break;
    case TG_Kind_Function:{result = push_str8f(arena, "0x%I64x", eval.imm_u64);}break;
    
    case TG_Kind_Enum:
    {
      Temp scratch = scratch_begin(&arena, 1);
      TG_Type *type = tg_type_from_graph_rdi_key(scratch.arena, graph, rdi, type_key);
      String8 constant_name = {0};
      for(U64 val_idx = 0; val_idx < type->count; val_idx += 1)
      {
        if(eval.imm_u64 == type->enum_vals[val_idx].val)
        {
          constant_name = type->enum_vals[val_idx].name;
          break;
        }
      }
      if(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules)
      {
        if(constant_name.size != 0)
        {
          result = push_str8f(arena, "0x%I64x (%S)", eval.imm_u64, constant_name);
        }
        else
        {
          result = push_str8f(arena, "0x%I64x (%I64u)", eval.imm_u64, eval.imm_u64);
        }
      }
      else if(constant_name.size != 0)
      {
        result = push_str8_copy(arena, constant_name);
      }
      else
      {
        result = push_str8f(arena, "0x%I64x (%I64u)", eval.imm_u64, eval.imm_u64);
      }
      scratch_end(scratch);
    }break;
  }
  
  ProfEnd();
  return result;
}

//- rjf: writing values back to child processes

internal B32
df_commit_eval_value(TG_Graph *graph, RDI_Parsed *rdi, DF_CtrlCtx *ctrl_ctx, DF_Eval dst_eval, DF_Eval src_eval)
{
  B32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: unpack arguments
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = thread->parent;
  TG_Key dst_type_key = dst_eval.type_key;
  TG_Key src_type_key = src_eval.type_key;
  TG_Kind dst_type_kind = tg_kind_from_key(dst_type_key);
  TG_Kind src_type_kind = tg_kind_from_key(src_type_key);
  U64 dst_type_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, dst_type_key);
  U64 src_type_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, src_type_key);
  
  //- rjf: get commit data based on destination type
  String8 commit_data = {0};
  if(src_eval.errors.count == 0)
  {
    result = 1;
    switch(dst_type_kind)
    {
      default:
      {
        // NOTE(rjf): not supported
        result = 0;
      }break;
      
      //- rjf: pointers
      case TG_Kind_Ptr:
      case TG_Kind_LRef:
      if((TG_Kind_Char8 <= src_type_kind && src_type_kind <= TG_Kind_Bool) || src_type_kind == TG_Kind_Ptr)
      {
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, src_eval);
        commit_data = str8((U8 *)&value_eval.imm_u64, dst_type_byte_size);
        commit_data = push_str8_copy(scratch.arena, commit_data);
      }break;
      
      //- rjf: integers
      case TG_Kind_Char8:
      case TG_Kind_Char16:
      case TG_Kind_Char32:
      case TG_Kind_S8:
      case TG_Kind_S16:
      case TG_Kind_S32:
      case TG_Kind_S64:
      case TG_Kind_UChar8:
      case TG_Kind_UChar16:
      case TG_Kind_UChar32:
      case TG_Kind_U8:
      case TG_Kind_U16:
      case TG_Kind_U32:
      case TG_Kind_U64:
      case TG_Kind_Bool:
      if(TG_Kind_Char8 <= src_type_kind && src_type_kind <= TG_Kind_Bool)
      {
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, src_eval);
        commit_data = str8((U8 *)&value_eval.imm_u64, dst_type_byte_size);
        commit_data = push_str8_copy(scratch.arena, commit_data);
      }break;
      
      //- rjf: float32s
      case TG_Kind_F32:
      if((TG_Kind_Char8 <= src_type_kind && src_type_kind <= TG_Kind_Bool) ||
         src_type_kind == TG_Kind_F32 ||
         src_type_kind == TG_Kind_F64)
      {
        F32 value = 0;
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, src_eval);
        switch(src_type_kind)
        {
          case TG_Kind_F32:{value = value_eval.imm_f32;}break;
          case TG_Kind_F64:{value = (F32)value_eval.imm_f64;}break;
          default:{value = (F32)value_eval.imm_s64;}break;
        }
        commit_data = str8((U8 *)&value, sizeof(F32));
        commit_data = push_str8_copy(scratch.arena, commit_data);
      }break;
      
      //- rjf: float64s
      case TG_Kind_F64:
      if((TG_Kind_Char8 <= src_type_kind && src_type_kind <= TG_Kind_Bool) ||
         src_type_kind == TG_Kind_F32 ||
         src_type_kind == TG_Kind_F64)
      {
        F64 value = 0;
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, src_eval);
        switch(src_type_kind)
        {
          case TG_Kind_F32:{value = (F64)value_eval.imm_f32;}break;
          case TG_Kind_F64:{value = value_eval.imm_f64;}break;
          default:{value = (F64)value_eval.imm_s64;}break;
        }
        commit_data = str8((U8 *)&value, sizeof(F64));
        commit_data = push_str8_copy(scratch.arena, commit_data);
      }break;
      
      //- rjf: enums
      case TG_Kind_Enum:
      if(TG_Kind_Char8 <= src_type_kind && src_type_kind <= TG_Kind_Bool)
      {
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, src_eval);
        commit_data = str8((U8 *)&value_eval.imm_u64, dst_type_byte_size);
        commit_data = push_str8_copy(scratch.arena, commit_data);
      }break;
    }
  }
  
  //- rjf: commit
  if(result && commit_data.size != 0)
  {
    switch(dst_eval.mode)
    {
      default:{}break;
      case EVAL_EvalMode_Addr:
      {
        ctrl_process_write(process->ctrl_machine_id, process->ctrl_handle, r1u64(dst_eval.offset, dst_eval.offset+commit_data.size), commit_data.str);
      }break;
      case EVAL_EvalMode_Reg:
      {
        CTRL_Unwind unwind = df_query_cached_unwind_from_thread(thread);
        Architecture arch = df_architecture_from_entity(thread);
        U64 reg_block_size = regs_block_size_from_architecture(arch);
        if(unwind.frames.count != 0 &&
           (0 <= dst_eval.offset && dst_eval.offset+commit_data.size < reg_block_size))
        {
          void *new_regs = push_array(scratch.arena, U8, reg_block_size);
          MemoryCopy(new_regs, unwind.frames.v[0].regs, reg_block_size);
          MemoryCopy((U8 *)new_regs+dst_eval.offset, commit_data.str, commit_data.size);
          result = ctrl_thread_write_reg_block(thread->ctrl_machine_id, thread->ctrl_handle, new_regs);
        }
      }break;
    }
  }
  
  scratch_end(scratch);
  return result;
}

//- rjf: type helpers

internal TG_MemberArray
df_filtered_data_members_from_members_cfg_table(Arena *arena, TG_MemberArray members, DF_CfgTable *cfg)
{
  DF_CfgVal *only = df_cfg_val_from_string(cfg, str8_lit("only"));
  DF_CfgVal *omit = df_cfg_val_from_string(cfg, str8_lit("omit"));
  TG_MemberArray filtered_members = members;
  if(only != &df_g_nil_cfg_val || omit != &df_g_nil_cfg_val)
  {
    Temp scratch = scratch_begin(&arena, 1);
    typedef struct DF_TypeMemberLooseNode DF_TypeMemberLooseNode;
    struct DF_TypeMemberLooseNode
    {
      DF_TypeMemberLooseNode *next;
      TG_Member *member;
    };
    DF_TypeMemberLooseNode *first_member = 0;
    DF_TypeMemberLooseNode *last_member = 0;
    U64 member_count = 0;
    MemoryZeroStruct(&filtered_members);
    for(U64 idx = 0; idx < members.count; idx += 1)
    {
      // rjf: check if included by 'only's
      B32 is_included = 1;
      for(DF_CfgNode *r = only->first; r != &df_g_nil_cfg_node; r = r->next)
      {
        is_included = 0;
        for(DF_CfgNode *name_node = r->first; name_node != &df_g_nil_cfg_node; name_node = name_node->next)
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
      for(DF_CfgNode *r = omit->first; r != &df_g_nil_cfg_node; r = r->next)
      {
        for(DF_CfgNode *name_node = r->first; name_node != &df_g_nil_cfg_node; name_node = name_node->next)
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
      filtered_members.v = push_array_no_zero(arena, TG_Member, filtered_members.count);
      U64 idx = 0;
      for(DF_TypeMemberLooseNode *n = first_member; n != 0; n = n->next, idx += 1)
      {
        MemoryCopyStruct(&filtered_members.v[idx], n->member);
        filtered_members.v[idx].name = push_str8_copy(arena, filtered_members.v[idx].name);
      }
    }
    scratch_end(scratch);
  }
  return filtered_members;
}

internal DF_EvalLinkBaseChunkList
df_eval_link_base_chunk_list_from_eval(Arena *arena, TG_Graph *graph, RDI_Parsed *rdi, TG_Key link_member_type_key, U64 link_member_off, DF_CtrlCtx *ctrl_ctx, DF_Eval eval, U64 cap)
{
  DF_EvalLinkBaseChunkList list = {0};
  for(DF_Eval base_eval = eval, last_eval = zero_struct; list.count < cap;)
  {
    // rjf: check this ptr's validity
    if(base_eval.offset == 0 || (base_eval.offset == last_eval.offset && base_eval.mode == last_eval.mode))
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
      chunk->b[chunk->count].mode = base_eval.mode;
      chunk->b[chunk->count].offset = base_eval.offset;
      chunk->count += 1;
      list.count += 1;
    }
    
    // rjf: grab link member
    DF_Eval link_member_eval =
    {
      link_member_type_key,
      base_eval.mode,
      base_eval.offset + link_member_off,
    };
    DF_Eval link_member_value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, link_member_eval);
    
    // rjf: advance to next link
    last_eval = base_eval;
    base_eval.mode = EVAL_EvalMode_Addr;
    base_eval.offset = link_member_value_eval.imm_u64;
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
  return &n->v;
}

internal DF_EvalVizBlock *
df_eval_viz_block_split_and_continue(Arena *arena, DF_EvalVizBlockList *list, DF_EvalVizBlock *split_block, U64 split_idx)
{
  U64 total_count = split_block->semantic_idx_range.max;
  split_block->visual_idx_range.max = split_block->semantic_idx_range.max = split_idx;
  df_eval_viz_block_end(list, split_block);
  DF_EvalVizBlock *continue_block = df_eval_viz_block_begin(arena, split_block->kind, split_block->parent_key, split_block->key, split_block->depth);
  continue_block->eval = split_block->eval;
  continue_block->string = split_block->string;
  continue_block->member = split_block->member;
  continue_block->visual_idx_range = continue_block->semantic_idx_range = r1u64(split_idx+1, total_count);
  continue_block->fzy_backing_items = split_block->fzy_backing_items;
  continue_block->fzy_target = split_block->fzy_target;
  continue_block->cfg_table = split_block->cfg_table;
  continue_block->link_member_type_key = split_block->link_member_type_key;
  continue_block->link_member_off = split_block->link_member_off;
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
df_append_viz_blocks_for_parent__rec(Arena *arena, DI_Scope *scope, DF_EvalView *eval_view, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_ExpandKey parent_key, DF_ExpandKey key, String8 string, DF_Eval eval, TG_Member *opt_member, DF_CfgTable *cfg_table, S32 depth, DF_EvalVizBlockList *list_out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //////////////////////////////
  //- rjf: determine if this key is expanded
  //
  DF_ExpandNode *node = df_expand_node_from_key(&eval_view->expand_tree_table, key);
  B32 parent_is_expanded = (node != 0 && node->expanded && !tg_key_match(tg_key_zero(), eval.type_key));
  
  //////////////////////////////
  //- rjf: apply view rules & resolve eval
  //
  eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
  eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval, cfg_table);
  
  //////////////////////////////
  //- rjf: unpack eval
  //
  TG_Key eval_type_key = tg_unwrapped_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, eval.type_key);
  TG_Kind eval_type_kind = tg_kind_from_key(eval_type_key);
  String8 eval_string = push_str8_copy(arena, string);
  
  //////////////////////////////
  //- rjf: make and push block for root
  //
  {
    DF_EvalVizBlock *block = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Root, parent_key, key, depth);
    block->eval                        = eval;
    block->cfg_table                   = *cfg_table;
    block->string                      = eval_string;
    block->visual_idx_range            = r1u64(key.child_num-1, key.child_num+0);
    block->semantic_idx_range          = r1u64(key.child_num-1, key.child_num+0);
    if(opt_member != 0)
    {
      block->member = tg_member_copy(arena, opt_member);
    }
    df_eval_viz_block_end(list_out, block);
  }
  
  //////////////////////////////
  //- rjf: (pointers) extract type & info to use for members and/or arrays
  //
  DF_Eval udt_eval = eval;
  DF_Eval arr_eval = eval;
  DF_Eval ptr_eval = zero_struct;
  TG_Kind udt_type_kind = eval_type_kind;
  TG_Kind arr_type_kind = eval_type_kind;
  TG_Kind ptr_type_kind = TG_Kind_Null;
  if(eval_type_kind == TG_Kind_Ptr || eval_type_kind == TG_Kind_LRef || eval_type_kind == TG_Kind_RRef)
  {
    TG_Key direct_type_key = tg_ptee_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, eval_type_key);
    TG_Kind direct_type_kind = tg_kind_from_key(direct_type_key);
    DF_Eval ptr_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
    
    // rjf: ptrs to udts
    if(parent_is_expanded &&
       (direct_type_kind == TG_Kind_Struct ||
        direct_type_kind == TG_Kind_Union ||
        direct_type_kind == TG_Kind_Class ||
        direct_type_kind == TG_Kind_IncompleteStruct ||
        direct_type_kind == TG_Kind_IncompleteUnion ||
        direct_type_kind == TG_Kind_IncompleteClass))
    {
      udt_eval.type_key = direct_type_key;
      udt_eval.mode = EVAL_EvalMode_Addr;
      udt_eval.offset = ptr_val_eval.imm_u64;
      udt_type_kind = tg_kind_from_key(direct_type_key);
    }
    
    // rjf: ptrs to arrays
    if(direct_type_kind == TG_Kind_Array)
    {
      arr_eval.type_key = direct_type_key;
      arr_eval.mode = EVAL_EvalMode_Addr;
      arr_eval.offset = ptr_val_eval.imm_u64;
      arr_type_kind = tg_kind_from_key(direct_type_key);
    }
    
    // rjf: ptrs to ptrs
    if(direct_type_kind == TG_Kind_Ptr || direct_type_kind == TG_Kind_LRef || direct_type_kind == TG_Kind_RRef)
    {
      ptr_eval.type_key = direct_type_key;
      ptr_eval.mode = EVAL_EvalMode_Addr;
      ptr_eval.offset = ptr_val_eval.imm_u64;
      ptr_type_kind = tg_kind_from_key(direct_type_key);
    }
  }
  
  //////////////////////////////
  //- rjf: determine rule for building expansion children
  //
  typedef enum DF_EvalVizExpandRule
  {
    DF_EvalVizExpandRule_Default,
    DF_EvalVizExpandRule_List,
    DF_EvalVizExpandRule_ViewRule,
  }
  DF_EvalVizExpandRule;
  DF_EvalVizExpandRule expand_rule = DF_EvalVizExpandRule_Default;
  DF_CoreViewRuleSpec *expand_view_rule_spec = &df_g_nil_core_view_rule_spec;
  DF_CfgVal *expand_view_rule_cfg = &df_g_nil_cfg_val;
  String8 list_next_link_member_name = {0};
  {
    //- rjf: look for view rules which have their own custom viz block building rules
    if(expand_rule == DF_EvalVizExpandRule_Default && parent_is_expanded)
    {
      for(DF_CfgVal *val = cfg_table->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
      {
        DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
        if(str8_match(spec->info.string, str8_lit("list"), 0) ||
           str8_match(spec->info.string, str8_lit("omit"), 0) ||
           str8_match(spec->info.string, str8_lit("only"), 0))
        {
          // TODO(rjf): "list" view rule needs to be formally moved into the visualization
          // engine hooks when the system is mature enough to support it
          // also "omit", "only"
          continue;
        }
        if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_VizBlockProd)
        {
          expand_rule = DF_EvalVizExpandRule_ViewRule;
          expand_view_rule_spec = spec;
          expand_view_rule_cfg = val;
          break;
        }
      }
    }
    
    //- rjf: get linked list viz view rule info for structs
    if(expand_rule == DF_EvalVizExpandRule_Default &&
       parent_is_expanded &&
       (udt_type_kind == TG_Kind_Struct ||
        udt_type_kind == TG_Kind_Union ||
        udt_type_kind == TG_Kind_Class))
    {
      DF_CfgVal *list_cfg = df_cfg_val_from_string(cfg_table, str8_lit("list"));
      if(list_cfg != &df_g_nil_cfg_val)
      {
        list_next_link_member_name = list_cfg->first->first->string;
        expand_rule = DF_EvalVizExpandRule_List;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: (all) descend to make blocks according to lens
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_ViewRule &&
     expand_view_rule_spec != &df_g_nil_core_view_rule_spec &&
     expand_view_rule_cfg != &df_g_nil_cfg_val)
    ProfScope("build viz blocks for lens")
  {
    expand_view_rule_spec->info.viz_block_prod(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval_view, eval, string, cfg_table, parent_key, key, depth+1, expand_view_rule_cfg->last, list_out);
  }
  
  //////////////////////////////
  //- rjf: (structs, unions, classes) descend to members & make block(s), normally
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_Default &&
     (udt_type_kind == TG_Kind_Struct ||
      udt_type_kind == TG_Kind_Union ||
      udt_type_kind == TG_Kind_Class))
    ProfScope("build viz blocks for UDT members")
  {
    //- rjf: type -> filtered data members
    TG_MemberArray data_members = tg_data_members_from_graph_rdi_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, udt_eval.type_key);
    TG_MemberArray filtered_data_members = df_filtered_data_members_from_members_cfg_table(scratch.arena, data_members, cfg_table);
    
    //- rjf: build blocks for all members, split by sub-expansions
    DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Members, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth+1);
    {
      last_vb->eval = udt_eval;
      last_vb->string = eval_string;
      last_vb->cfg_table = *cfg_table;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, filtered_data_members.count);
    }
    for(DF_ExpandNode *child = node->first; child != 0; child = child->next)
    {
      // rjf: unpack expansion info; skip out-of-bounds splits
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      if(child_idx >= filtered_data_members.count)
      {
        continue;
      }
      
      // rjf: form split: truncate & complete last block; begin next block
      last_vb = df_eval_viz_block_split_and_continue(arena, list_out, last_vb, child_idx);
      
      // rjf: recurse for sub-expansion
      {
        DF_CfgTable child_cfg = *cfg_table;
        {
          String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(df_hash_from_expand_key(key), child_num));
          child_cfg = df_cfg_table_from_inheritance(arena, cfg_table);
          if(view_rule_string.size != 0)
          {
            df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
          }
        }
        TG_Member *member = &filtered_data_members.v[child_idx];
        DF_Eval child_eval = zero_struct;
        {
          child_eval.type_key = member->type_key;
          child_eval.mode = udt_eval.mode;
          child_eval.offset = udt_eval.offset + member->off;
        }
        df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, macro_map, key, child->key, member->name, child_eval, member, &child_cfg, depth+1, list_out);
      }
    }
    df_eval_viz_block_end(list_out, last_vb);
  }
  
  //////////////////////////////
  //- rjf: (enums) descend to members & make block(s)
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_Default &&
     udt_eval.mode == EVAL_EvalMode_NULL &&
     udt_type_kind == TG_Kind_Enum)
    ProfScope("build viz blocks for UDT type-eval enums")
  {
    //- rjf: type -> full type info
    TG_Type *type = tg_type_from_graph_rdi_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, udt_eval.type_key);
    
    //- rjf: build block for all members (cannot be expanded)
    DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_EnumMembers, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth+1);
    {
      last_vb->eval = udt_eval;
      last_vb->string = eval_string;
      last_vb->cfg_table = *cfg_table;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, type->count);
    }
    df_eval_viz_block_end(list_out, last_vb);
  }
  
  //////////////////////////////
  //- rjf: (structs, unions, classes) descend to members & make block(s), with linked list view
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_List &&
     (udt_type_kind == TG_Kind_Struct ||
      udt_type_kind == TG_Kind_Union ||
      udt_type_kind == TG_Kind_Class))
    ProfScope("(structs, unions, classes) descend to members & make block(s), with linked list view")
  {
    //- rjf: type -> data members
    TG_MemberArray data_members = tg_data_members_from_graph_rdi_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, udt_eval.type_key);
    
    //- rjf: find link member
    TG_Member *link_member = 0;
    TG_Kind link_member_type_kind = TG_Kind_Null;
    TG_Key link_member_ptee_type_key = zero_struct;
    for(U64 idx = 0; idx < data_members.count; idx += 1)
    {
      TG_Member *mem = &data_members.v[idx];
      if(str8_match(mem->name, list_next_link_member_name, 0))
      {
        link_member = mem;
        link_member_type_kind = tg_kind_from_key(link_member->type_key);
        link_member_ptee_type_key = tg_ptee_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, link_member->type_key);
        break;
      }
    }
    
    //- rjf: check if link member is good
    B32 link_member_is_good = 1;
    if(link_member == 0 ||
       link_member_type_kind != TG_Kind_Ptr ||
       !tg_key_match(link_member_ptee_type_key, udt_eval.type_key))
    {
      link_member_is_good = 0;
    }
    
    //- rjf: gather link bases
    DF_EvalLinkBaseChunkList link_bases = {0};
    if(link_member_is_good)
    {
      link_bases = df_eval_link_base_chunk_list_from_eval(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, link_member->type_key, link_member->off, ctrl_ctx, udt_eval, 512);
    }
    
    //- rjf: build blocks for all links, split by sub-expansions
    if(link_member_is_good)
    {
      DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Links, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth+1);
      {
        last_vb->eval = udt_eval;
        last_vb->string = eval_string;
        last_vb->cfg_table = *cfg_table;
        last_vb->link_member_type_key = link_member->type_key;
        last_vb->link_member_off = link_member->off;
        last_vb->visual_idx_range     = r1u64(0, link_bases.count);
        last_vb->semantic_idx_range   = r1u64(0, link_bases.count);
      }
      for(DF_ExpandNode *child = node->first; child != 0; child = child->next)
      {
        // rjf: unpack expansion info; skip out-of-bounds splits
        U64 child_num = child->key.child_num;
        U64 child_idx = child_num-1;
        if(child_idx >= link_bases.count)
        {
          continue;
        }
        
        // rjf: form split: truncate & complete last block; begin next block
        last_vb = df_eval_viz_block_split_and_continue(arena, list_out, last_vb, child_idx);
        
        // rjf: find mode/offset of this link
        DF_EvalLinkBase link_base = df_eval_link_base_from_chunk_list_index(&link_bases, child_idx);
        
        // rjf: recurse for sub-expansion
        {
          DF_CfgTable child_cfg = *cfg_table;
          {
            String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(df_hash_from_expand_key(key), child_num));
            child_cfg = df_cfg_table_from_inheritance(arena, cfg_table);
            if(view_rule_string.size != 0)
            {
              df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
            }
          }
          DF_Eval child_eval = zero_struct;
          {
            child_eval.type_key = udt_eval.type_key;
            child_eval.mode     = link_base.mode;
            child_eval.offset   = link_base.offset;
          }
          df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, macro_map, key, child->key, push_str8f(arena, "[%I64u]", child_idx), child_eval, 0, &child_cfg, depth+1, list_out);
        }
      }
      df_eval_viz_block_end(list_out, last_vb);
    }
  }
  
  //////////////////////////////
  //- rjf: (arrays) descend to elements & make block(s), normally
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_Default &&
     arr_type_kind == TG_Kind_Array)
    ProfScope("(arrays) descend to elements & make block(s)")
  {
    //- rjf: unpack array type info
    TG_Type *array_type = tg_type_from_graph_rdi_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, arr_eval.type_key);
    U64 array_count = array_type->count;
    TG_Key element_type_key = array_type->direct_type_key;
    U64 element_type_byte_size = tg_byte_size_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, element_type_key);
    
    //- rjf: build blocks for all elements, split by sub-expansions
    DF_EvalVizBlock *last_vb = df_eval_viz_block_begin(arena, DF_EvalVizBlockKind_Elements, key, df_expand_key_make(df_hash_from_expand_key(key), 0), depth+1);
    {
      last_vb->eval = arr_eval;
      last_vb->string = eval_string;
      last_vb->cfg_table = *cfg_table;
      last_vb->visual_idx_range = last_vb->semantic_idx_range = r1u64(0, array_count);
    }
    for(DF_ExpandNode *child = node->first; child != 0; child = child->next)
    {
      // rjf: unpack expansion info; skip out-of-bounds splits
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      if(child_idx >= array_count)
      {
        continue;
      }
      
      // rjf: form split: truncate & complete last block; begin next block
      last_vb = df_eval_viz_block_split_and_continue(arena, list_out, last_vb, child_idx);
      
      // rjf: recurse for sub-expansion
      {
        DF_CfgTable child_cfg = *cfg_table;
        {
          String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(df_hash_from_expand_key(key), child_num));
          child_cfg = df_cfg_table_from_inheritance(arena, cfg_table);
          if(view_rule_string.size != 0)
          {
            df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
          }
        }
        DF_Eval child_eval = zero_struct;
        {
          child_eval.type_key = element_type_key;
          child_eval.mode     = arr_eval.mode;
          child_eval.offset   = arr_eval.offset + child_idx*element_type_byte_size;
        }
        df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, macro_map, key, child->key, push_str8f(arena, "[%I64u]", child_idx), child_eval, 0, &child_cfg, depth+1, list_out);
      }
    }
    df_eval_viz_block_end(list_out, last_vb);
  }
  
  //////////////////////////////
  //- rjf: (ptr to ptrs) descend to make blocks for pointed-at-pointer
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_Default && (ptr_type_kind == TG_Kind_Ptr || ptr_type_kind == TG_Kind_LRef || ptr_type_kind == TG_Kind_RRef))
    ProfScope("build viz blocks for ptr-to-ptrs")
  {
    String8 subexpr = push_str8f(arena, "*(%S)", string);
    df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, macro_map, key, df_expand_key_make(df_hash_from_expand_key(key), 1), subexpr, ptr_eval, 0, cfg_table, depth+1, list_out);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal DF_EvalVizBlockList
df_eval_viz_block_list_from_eval_view_expr_keys(Arena *arena, DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_EvalView *eval_view, String8 expr, DF_ExpandKey parent_key, DF_ExpandKey key)
{
  ProfBeginFunction();
  DF_EvalVizBlockList blocks = {0};
  {
    DF_Eval eval = df_eval_from_string(arena, scope, ctrl_ctx, parse_ctx, macro_map, expr);
    U64 expr_comma_pos = str8_find_needle(expr, 0, str8_lit(","), 0);
    U64 passthrough_pos = str8_find_needle(expr, 0, str8_lit("--"), 0);
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
    DF_CfgTable view_rule_table = {0};
    for(String8Node *n = default_view_rules.first; n != 0; n = n->next)
    {
      df_cfg_table_push_unparsed_string(arena, &view_rule_table, n->string, DF_CfgSrc_User);
    }
    df_cfg_table_push_unparsed_string(arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
    df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, macro_map, parent_key, key, expr, eval, 0, &view_rule_table, 0, &blocks);
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

//- rjf: viz row list building

internal DF_EvalVizRow *
df_eval_viz_row_list_push_new(Arena *arena, EVAL_ParseCtx *parse_ctx, DF_EvalVizWindowedRowList *rows, DF_EvalVizBlock *block, DF_ExpandKey key, DF_Eval eval)
{
  // rjf: push
  DF_EvalVizRow *row = push_array(arena, DF_EvalVizRow, 1);
  SLLQueuePush(rows->first, rows->last, row);
  rows->count += 1;
  
  // rjf: fill basics
  row->depth        = block->depth;
  row->parent_key   = block->parent_key;
  row->key          = key;
  row->eval         = eval;
  row->size_in_rows = 1;
  
  // rjf: determine exandability, editability
  if(tg_kind_from_key(eval.type_key) != TG_Kind_Null)
  {
    for(TG_Key t = eval.type_key;; t = tg_unwrapped_direct_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, t))
    {
      TG_Kind kind = tg_kind_from_key(t);
      if(kind == TG_Kind_Null)
      {
        break;
      }
      if(eval.mode != EVAL_EvalMode_NULL && ((TG_Kind_FirstBasic <= kind && kind <= TG_Kind_LastBasic) || kind == TG_Kind_Ptr || kind == TG_Kind_LRef || kind == TG_Kind_RRef))
      {
        row->flags |= DF_EvalVizRowFlag_CanEditValue;
      }
      if(eval.mode == EVAL_EvalMode_NULL && kind == TG_Kind_Enum)
      {
        row->flags |= DF_EvalVizRowFlag_CanExpand;
      }
      if(kind == TG_Kind_Struct ||
         kind == TG_Kind_Union ||
         kind == TG_Kind_Class ||
         kind == TG_Kind_Array)
      {
        row->flags |= DF_EvalVizRowFlag_CanExpand;
      }
      if(row->flags & DF_EvalVizRowFlag_CanExpand)
      {
        break;
      }
      if(eval.mode == EVAL_EvalMode_NULL)
      {
        break;
      }
      if(kind == TG_Kind_Function)
      {
        break;
      }
    }
  }
  
  return row;
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

//- rjf: control context

internal DF_CtrlCtx
df_ctrl_ctx(void)
{
  return df_state->ctrl_ctx;
}

internal void
df_ctrl_ctx_apply_overrides(DF_CtrlCtx *ctx, DF_CtrlCtx *overrides)
{
  if(!df_handle_match(overrides->thread, df_handle_zero()))
  {
    ctx->thread = overrides->thread;
    ctx->unwind_count = overrides->unwind_count;
    ctx->inline_depth = overrides->inline_depth;
  }
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
  String8List strs = {0};
  
  //- rjf: write recent projects
  {
    B32 first = 1;
    DF_EntityList recent_projects = df_query_cached_entity_list_with_kind(DF_EntityKind_RecentProject);
    for(DF_EntityNode *n = recent_projects.first; n != 0; n = n->next)
    {
      DF_Entity *rp = n->entity;
      if(rp->cfg_src == source)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// recent projects ///////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        Temp scratch = scratch_begin(&arena, 1);
        String8 path_absolute = path_normalized_from_string(scratch.arena, rp->name);
        String8 path_relative = path_relative_dst_from_absolute_dst_src(scratch.arena, path_absolute, root_path);
        str8_list_pushf(arena, &strs,  "recent_project: {\"%S\"}\n", path_relative);
        scratch_end(scratch);
      }
    }
    if(!first)
    {
      str8_list_push(arena, &strs, str8_lit("\n"));
    }
  }
  
  //- rjf: write targets
  {
    B32 first = 1;
    DF_EntityList targets = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
    for(DF_EntityNode *n = targets.first; n != 0; n = n->next)
    {
      DF_Entity *target = n->entity;
      if(target->cfg_src == source)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// targets ///////////////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        Temp scratch = scratch_begin(&arena, 1);
        DF_Entity *exe__ent  = df_entity_child_from_kind(target, DF_EntityKind_Executable);
        DF_Entity *args__ent = df_entity_child_from_kind(target, DF_EntityKind_Arguments);
        DF_Entity *wdir__ent = df_entity_child_from_kind(target, DF_EntityKind_ExecutionPath);
        DF_Entity *entr__ent = df_entity_child_from_kind(target, DF_EntityKind_EntryPointName);
        String8 label = target->name;
        String8 exe = exe__ent->name;
        String8 exe_normalized = path_normalized_from_string(scratch.arena, exe);
        String8 exe_normalized_rel = path_relative_dst_from_absolute_dst_src(scratch.arena, exe_normalized, root_path);
        String8 wdir = wdir__ent->name;
        String8 wdir_normalized = path_normalized_from_string(scratch.arena, wdir);
        String8 wdir_normalized_rel = path_relative_dst_from_absolute_dst_src(scratch.arena, wdir_normalized, root_path);
        String8 entry_point_name = entr__ent->name;
        String8 label_escaped = df_cfg_escaped_from_raw_string(arena, label);
        String8 args_escaped = df_cfg_escaped_from_raw_string(arena, args__ent->name);
        String8 entry_escaped = df_cfg_escaped_from_raw_string(arena, entry_point_name);
        str8_list_push (arena, &strs,  str8_lit("target:\n"));
        str8_list_push (arena, &strs,  str8_lit("{\n"));
        if(label.size != 0)
        {
          str8_list_pushf(arena, &strs,          "  label:             \"%S\"\n", label_escaped);
        }
        str8_list_pushf(arena, &strs,           "  exe:               \"%S\"\n", exe_normalized_rel);
        str8_list_pushf(arena, &strs,           "  arguments:         \"%S\"\n", args_escaped);
        str8_list_pushf(arena, &strs,           "  working_directory: \"%S\"\n", wdir_normalized_rel);
        if(entry_point_name.size != 0)
        {
          str8_list_pushf(arena, &strs,          "  entry_point:       \"%S\"\n", entry_escaped);
        }
        str8_list_pushf(arena, &strs,           "  active:            %i\n", (int)target->b32);
        if(target->flags & DF_EntityFlag_HasColor)
        {
          Vec4F32 hsva = df_hsva_from_entity(target);
          str8_list_pushf(arena, &strs,          "  hsva:              %.2f %.2f %.2f %.2f\n", hsva.x, hsva.y, hsva.z, hsva.w);
        }
        str8_list_push (arena, &strs,  str8_lit("}\n"));
        str8_list_push (arena, &strs,  str8_lit("\n"));
        scratch_end(scratch);
      }
    }
  }
  
  //- rjf: write path maps
  {
    B32 first = 1;
    DF_EntityList path_maps = df_query_cached_entity_list_with_kind(DF_EntityKind_OverrideFileLink);
    for(DF_EntityNode *n = path_maps.first; n != 0; n = n->next)
    {
      DF_Entity *map = n->entity;
      if(map->cfg_src == source)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// file path maps ////////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        String8 src_path = df_full_path_from_entity(arena, map);
        String8 dst_path = df_full_path_from_entity(arena, df_entity_from_handle(map->entity_handle));
        str8_list_push (arena, &strs,  str8_lit("file_path_map:\n"));
        str8_list_push (arena, &strs,  str8_lit("{\n"));
        str8_list_pushf(arena, &strs,           "  source_path: \"%S\"\n", src_path);
        str8_list_pushf(arena, &strs,           "  dest_path:   \"%S\"\n", dst_path);
        str8_list_push (arena, &strs,  str8_lit("}\n"));
        str8_list_push (arena, &strs,  str8_lit("\n"));
      }
    }
  }
  
  //- rjf: write auto view rules
  {
    B32 first = 1;
    DF_EntityList avrs = df_query_cached_entity_list_with_kind(DF_EntityKind_AutoViewRule);
    for(DF_EntityNode *n = avrs.first; n != 0; n = n->next)
    {
      DF_Entity *map = n->entity;
      if(map->cfg_src == source)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// auto view rules ///////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        String8 type      = df_entity_child_from_kind(map, DF_EntityKind_Source)->name;
        String8 view_rule = df_entity_child_from_kind(map, DF_EntityKind_Dest)->name;
        type = df_cfg_escaped_from_raw_string(arena, type);
        view_rule= df_cfg_escaped_from_raw_string(arena, view_rule);
        str8_list_push (arena, &strs,  str8_lit("auto_view_rule:\n"));
        str8_list_push (arena, &strs,  str8_lit("{\n"));
        str8_list_pushf(arena, &strs,           "  type:      \"%S\"\n", type);
        str8_list_pushf(arena, &strs,           "  view_rule: \"%S\"\n", view_rule);
        str8_list_push (arena, &strs,  str8_lit("}\n"));
        str8_list_push (arena, &strs,  str8_lit("\n"));
      }
    }
  }
  
  //- rjf: write breakpoints
  {
    B32 first = 1;
    DF_EntityList bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
    for(DF_EntityNode *n = bps.first; n != 0; n = n->next)
    {
      DF_Entity *bp = n->entity;
      if(bp->cfg_src == source)
      {
        DF_Entity *file = df_entity_ancestor_from_kind(bp, DF_EntityKind_File);
        DF_Entity *symb = df_entity_child_from_kind(bp, DF_EntityKind_EntryPointName);
        DF_Entity *cond = df_entity_child_from_kind(bp, DF_EntityKind_Condition);
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// breakpoints ///////////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        
        // rjf: begin
        str8_list_push(arena, &strs, str8_lit("breakpoint:\n"));
        str8_list_push(arena, &strs, str8_lit("{\n"));
        
        // rjf: textual breakpoints
        if(!df_entity_is_nil(file) && bp->flags & DF_EntityFlag_HasTextPoint)
        {
          String8 bp_file_path = df_full_path_from_entity(arena, file);
          String8 srlized_bp_file_path = path_relative_dst_from_absolute_dst_src(arena, bp_file_path, root_path);
          String8 string = push_str8f(arena, "  line: (\"%S\":%I64d)\n", srlized_bp_file_path, bp->text_point.line);
          str8_list_push(arena, &strs, string);
        }
        
        // rjf: function name breakpoints
        else if(!df_entity_is_nil(symb) && symb->name.size != 0)
        {
          String8 symb_escaped = df_cfg_escaped_from_raw_string(arena, symb->name);
          str8_list_pushf(arena, &strs, "  symbol: \"%S\"\n", symb_escaped);
        }
        
        // rjf: address breakpoints
        else if(bp->flags & DF_EntityFlag_HasVAddr)
        {
          str8_list_pushf(arena, &strs, "  addr: 0x%I64x\n", bp->vaddr);
        }
        
        // rjf: conditions
        if(!df_entity_is_nil(cond))
        {
          String8 cond_escaped = df_cfg_escaped_from_raw_string(arena, cond->name);
          str8_list_pushf(arena, &strs, "  condition: \"%S\"\n", cond_escaped);
        }
        
        // rjf: universal options
        str8_list_pushf(arena, &strs, "  enabled: %i\n", (int)bp->b32);
        if(bp->name.size != 0)
        {
          String8 label_escaped = df_cfg_escaped_from_raw_string(arena, bp->name);
          str8_list_pushf(arena, &strs, "  label: \"%S\"\n", bp->name);
        }
        if(bp->flags & DF_EntityFlag_HasColor)
        {
          Vec4F32 hsva = df_hsva_from_entity(bp);
          str8_list_pushf(arena, &strs, "  hsva: %.2f %.2f %.2f %.2f\n", hsva.x, hsva.y, hsva.z, hsva.w);
        }
        
        // rjf: end
        str8_list_push(arena, &strs, str8_lit("}\n\n"));
      }
    }
  }
  
  //- rjf: write watch pins
  {
    B32 first = 1;
    DF_EntityList pins = df_query_cached_entity_list_with_kind(DF_EntityKind_WatchPin);
    for(DF_EntityNode *n = pins.first; n != 0; n = n->next)
    {
      DF_Entity *pin = n->entity;
      if(pin->cfg_src == source)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// watch pins ////////////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        
        // rjf: write
        str8_list_push(arena, &strs, str8_lit("watch_pin:\n"));
        str8_list_push(arena, &strs, str8_lit("{\n"));
        String8 expr_escaped = df_cfg_escaped_from_raw_string(arena, pin->name);
        str8_list_pushf(arena, &strs, "  expression: \"%S\"\n", expr_escaped);
        DF_Entity *file = df_entity_ancestor_from_kind(pin, DF_EntityKind_File);
        if(pin->flags & DF_EntityFlag_HasTextPoint && !df_entity_is_nil(file))
        {
          String8 project_path = root_path;
          String8 pin_file_path = df_full_path_from_entity(arena, file);
          project_path = path_normalized_from_string(arena, project_path);
          pin_file_path = path_normalized_from_string(arena, pin_file_path);
          String8 srlized_pin_file_path = path_relative_dst_from_absolute_dst_src(arena, pin_file_path, project_path);
          str8_list_pushf(arena, &strs, "  line: (\"%S\":%I64d)\n", srlized_pin_file_path, pin->text_point.line);
        }
        else if(pin->flags & DF_EntityFlag_HasVAddr)
        {
          str8_list_pushf(arena, &strs, "  addr: (0x%I64x)\n", pin->vaddr);
        }
        if(pin->flags & DF_EntityFlag_HasColor)
        {
          Vec4F32 hsva = df_hsva_from_entity(pin);
          str8_list_pushf(arena, &strs, "  hsva: %.2f %.2f %.2f %.2f\n", hsva.x, hsva.y, hsva.z, hsva.w);
        }
        str8_list_push(arena, &strs, str8_lit("}\n\n"));
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

//- rjf: architecture info table lookups

internal String8
df_info_summary_from_string__x64(String8 string)
{
  String8 result = {0};
  {
    U64 hash = df_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % df_state->arch_info_x64_table_size;
    DF_ArchInfoSlot *slot = &df_state->arch_info_x64_table[slot_idx];
    for(DF_ArchInfoNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(str8_match(n->key, string, StringMatchFlag_CaseInsensitive))
      {
        result = n->val;
        break;
      }
    }
  }
  return result;
}

internal String8
df_info_summary_from_string(Architecture arch, String8 string)
{
  String8 result = {0};
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:
    {
      result = df_info_summary_from_string__x64(string);
    }break;
  }
  return result;
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
    if(n->entity->b32)
    {
      df_entity_list_push(arena, &active_targets, n->entity);
    }
  }
  return active_targets;
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

internal EVAL_String2NumMap *
df_query_cached_locals_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  EVAL_String2NumMap *map = &eval_string2num_map_nil;
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
      EVAL_String2NumMap *map = df_push_locals_map_from_dbgi_key_voff(cache->arena, scope, dbgi_key, voff);
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

internal EVAL_String2NumMap *
df_query_cached_member_map_from_dbgi_key_voff(DI_Key *dbgi_key, U64 voff)
{
  ProfBeginFunction();
  EVAL_String2NumMap *map = &eval_string2num_map_nil;
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
      EVAL_String2NumMap *map = df_push_member_map_from_dbgi_key_voff(cache->arena, scope, dbgi_key, voff);
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
  df_state->entities_root = df_entity_alloc(0, &df_g_nil_entity, DF_EntityKind_Root);
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
    DF_Entity *local_machine = df_entity_alloc(0, df_state->entities_root, DF_EntityKind_Machine);
    df_entity_equip_ctrl_machine_id(local_machine, CTRL_MachineID_Local);
    df_entity_equip_name(0, local_machine, str8_lit("This PC"));
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
  
  // rjf: set up architecture info tables
  df_state->arch_info_x64_table_size = 1024;
  df_state->arch_info_x64_table = push_array(df_state->arena, DF_ArchInfoSlot, df_state->arch_info_x64_table_size);
  for(U64 idx = 0; idx < ArrayCount(df_g_inst_table_x64); idx += 1)
  {
    String8 key = df_g_inst_table_x64[idx].mnemonic;
    String8 val = df_g_inst_table_x64[idx].summary;
    U64 hash = df_hash_from_string__case_insensitive(key);
    U64 slot_idx = hash % df_state->arch_info_x64_table_size;
    DF_ArchInfoSlot *slot = &df_state->arch_info_x64_table[slot_idx];
    DF_ArchInfoNode *n = push_array(df_state->arena, DF_ArchInfoNode, 1);
    SLLQueuePush_N(slot->first, slot->last, n, hash_next);
    n->key = key;
    n->val = val;
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
  df_state->dt = dt;
  df_state->time_in_seconds += dt;
  df_state->top_interact_regs = &df_state->base_interact_regs;
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
            DF_Entity *selected_thread = df_entity_from_handle(df_state->ctrl_ctx.thread);
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
              DF_Entity *symb = df_entity_child_from_kind(bp, DF_EntityKind_EntryPointName);
              if(bp->flags & DF_EntityFlag_HasVAddr && bp->vaddr == stop_thread_vaddr)
              {
                bp->u64 += 1;
              }
              if(bp->flags & DF_EntityFlag_HasTextPoint)
              {
                DF_Entity *bp_file = df_entity_ancestor_from_kind(bp, DF_EntityKind_File);
                DF_LineList lines = df_lines_from_file_line_num(scratch.arena, bp_file, bp->text_point.line);
                for(DF_LineNode *n = lines.first; n != 0; n = n->next)
                {
                  if(contains_1u64(n->v.voff_range, stop_thread_voff))
                  {
                    bp->u64 += 1;
                    break;
                  }
                }
              }
              if(!df_entity_is_nil(symb))
              {
                U64 symb_voff = df_voff_from_dbgi_key_symbol_name(&dbgi_key, symb->name);
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
          DF_Entity *entity = df_entity_alloc(0, machine, DF_EntityKind_Process);
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
          DF_Entity *entity = df_entity_alloc(0, parent, DF_EntityKind_Thread);
          df_entity_equip_ctrl_machine_id(entity, event->machine_id);
          df_entity_equip_ctrl_handle(entity, event->entity);
          df_entity_equip_arch(entity, event->arch);
          df_entity_equip_ctrl_id(entity, event->entity_id);
          df_entity_equip_stack_base(entity, event->stack_base);
          df_entity_equip_tls_root(entity, event->tls_root);
          df_entity_equip_vaddr(entity, event->rip_vaddr);
          if(event->string.size != 0)
          {
            df_entity_equip_name(0, entity, event->string);
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
                df_entity_equip_name(0, entity, pending_thread_name->name);
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
          DF_Entity *selected_thread = df_entity_from_handle(df_state->ctrl_ctx.thread);
          if(df_entity_is_nil(selected_thread))
          {
            df_state->ctrl_ctx.thread = df_handle_from_entity(entity);
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
          DF_Entity *module = df_entity_alloc(0, parent, DF_EntityKind_Module);
          df_entity_equip_ctrl_machine_id(module, event->machine_id);
          df_entity_equip_ctrl_handle(module, event->entity);
          df_entity_equip_arch(module, event->arch);
          df_entity_equip_name(0, module, event->string);
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
            debug_info = df_entity_alloc(0, module, DF_EntityKind_DebugInfoPath);
          }
          df_entity_equip_name(0, debug_info, event->string);
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
              entity = df_entity_alloc(0, process, DF_EntityKind_PendingThreadName);
              df_entity_equip_name(0, entity, string);
              df_entity_equip_ctrl_machine_id(entity, event->machine_id);
              df_entity_equip_ctrl_id(entity, event->entity_id);
            }
          }
          if(!df_entity_is_nil(entity))
          {
            df_entity_equip_name(0, entity, string);
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
          DF_Entity *task = df_entity_alloc(0, df_entity_root(), DF_EntityKind_ConversionTask);
          df_entity_equip_name(0, task, event->string);
        }break;
        case DI_EventKind_ConversionEnded:
        {
          DF_Entity *task = df_entity_from_name_and_kind(event->string, DF_EntityKind_ConversionTask);
          if(!df_entity_is_nil(task))
          {
            df_entity_mark_for_deletion(task);
          }
        }break;
        case DI_EventKind_ConversionFailureUnsupportedFormat:
        {
          // DF_Entity *task = df_entity_alloc(df_entity_root(), DF_EntityKind_ConversionFail);
          // df_entity_equip_name(task, event->string);
          // df_entity_equip_death_timer(task, 15.f);
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
              String8 path = df_entity_child_from_kind(target, DF_EntityKind_ExecutionPath)->name;
              String8 entry= df_entity_child_from_kind(target, DF_EntityKind_EntryPointName)->name;
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
          DF_Entity *file = df_entity_from_handle(params.entity);
          TxtPt point = params.text_point;
          if(file->kind == DF_EntityKind_File)
          {
            DF_Entity *bp = df_entity_alloc(0, file, DF_EntityKind_Breakpoint);
            bp->flags |= DF_EntityFlag_DiesOnRunStop;
            df_entity_equip_b32(bp, 1);
            df_entity_equip_txt_pt(bp, point);
            df_entity_equip_cfg_src(bp, DF_CfgSrc_Transient);
            DF_CmdParams p = df_cmd_params_zero();
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Run));
          }
        }break;
        case DF_CoreCmdKind_RunToAddress:
        {
          DF_Entity *bp = df_entity_alloc(0, df_entity_root(), DF_EntityKind_Breakpoint);
          bp->flags |= DF_EntityFlag_DiesOnRunStop;
          df_entity_equip_b32(bp, 1);
          df_entity_equip_vaddr(bp, params.vaddr);
          df_entity_equip_cfg_src(bp, DF_CfgSrc_Transient);
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
          MemoryZeroStruct(&df_state->ctrl_ctx);
          df_state->ctrl_ctx.thread = params.entity;
        }break;
        case DF_CoreCmdKind_SelectUnwind:
        {
          DI_Scope *di_scope = di_scope_open();
          DF_Entity *thread = df_entity_from_handle(df_state->ctrl_ctx.thread);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          CTRL_Unwind base_unwind = df_query_cached_unwind_from_thread(thread);
          DF_Unwind rich_unwind = df_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
          if(params.unwind_index < rich_unwind.frames.concrete_frame_count)
          {
            DF_UnwindFrame *frame = &rich_unwind.frames.v[params.unwind_index];
            df_state->ctrl_ctx.unwind_count = params.unwind_index;
            df_state->ctrl_ctx.inline_depth = 0;
            if(params.inline_depth < frame->inline_frame_count)
            {
              df_state->ctrl_ctx.inline_depth = params.inline_depth;
            }
          }
          di_scope_close(di_scope);
        }break;
        case DF_CoreCmdKind_UpOneFrame:
        case DF_CoreCmdKind_DownOneFrame:
        {
          // TODO(rjf)
#if 0
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx();
          DI_Scope *di_scope = di_scope_open();
          DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          CTRL_Unwind base_unwind = df_query_cached_unwind_from_thread(thread);
          DF_Unwind rich_unwind = df_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
          DF_UnwindFrame *current_frame = 0;
          if(ctrl_ctx.unwind_count < rich_unwind.frames.concrete_frame_count)
          {
            current_frame = &rich_unwind.frames.v[ctrl_ctx.unwind_count];
          }
          for(U64 idx = 0; idx < rich_unwind.frames.count; idx += 1)
          {
            if(rich_unwind.frames.v[idx].base_unwind_idx == ctrl_ctx.unwind_count &&
               rich_unwind.frames.v[idx].inline_unwind_idx == ctrl_ctx.inline_unwind_count)
            {
              current_frame = &rich_unwind.frames.v[idx];
              break;
            }
          }
          if(current_frame == 0 && rich_unwind.frames.count != 0)
          {
            current_frame = &rich_unwind.frames.v[0];
          }
          DF_UnwindFrame *next_frame = current_frame;
          switch(core_cmd_kind)
          {
            default:{}break;
            case DF_CoreCmdKind_UpOneFrame:
            if(current_frame != 0 && (current_frame - rich_unwind.frames.v) > 0)
            {
              next_frame -= 1;
            }break;
            case DF_CoreCmdKind_DownOneFrame:
            if(current_frame != 0 && (current_frame - rich_unwind.frames.v)+1 < rich_unwind.frames.count)
            {
              next_frame += 1;
            }break;
          }
          if(next_frame != 0)
          {
            DF_CmdParams p = params;
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_BaseUnwindIndex);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_InlineUnwindIndex);
            p.base_unwind_index = next_frame->base_unwind_idx;
            p.inline_unwind_index = next_frame->base_unwind_idx;
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
          }
          di_scope_close(di_scope);
#endif
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
        case DF_CoreCmdKind_Open:
        {
          String8 path = path_normalized_from_string(scratch.arena, params.file_path);
          if(path.size == 0)
          {
            DF_CmdParams p = params;
            p.string = str8_lit("File name not specified.");
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
          else
          {
            DF_Entity *loaded_file = df_entity_from_path(path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
            if(loaded_file->flags & DF_EntityFlag_IsMissing)
            {
              DF_CmdParams p = params;
              p.string = push_str8f(scratch.arena, "Could not load \"%S\".", path);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
            }
          }
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
          
          //- rjf: get config files
          DF_Entity *cfg_files[DF_CfgSrc_COUNT] = {0};
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              String8 path = df_cfg_path_from_src(src);
              cfg_files[src] = df_entity_from_path(path, DF_EntityFromPathFlag_OpenMissing|DF_EntityFromPathFlag_OpenAsNeeded);
            }
          }
          
          //- rjf: load files
          String8 cfg_data[DF_CfgSrc_COUNT] = {0};
          U64 cfg_timestamps[DF_CfgSrc_COUNT] = {0};
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              DF_Entity *file_entity = cfg_files[src];
              String8 path = df_full_path_from_entity(scratch.arena, file_entity);
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
              cfg_save[src] = (load_cfg[src] && cfg_files[src]->flags & DF_EntityFlag_IsMissing);
            }
          }
          
          //- rjf: determine if we need to reload config
          B32 cfg_load[DF_CfgSrc_COUNT] = {0};
          B32 cfg_load_any = 0;
          if(file_is_okay)
          {
            for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
            {
              cfg_load[src] = (load_cfg[src] && ((cfg_save[src] == 0 && df_state->cfg_cached_timestamp[src] != cfg_timestamps[src]) || cfg_files[src]->timestamp == 0));
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
            DF_Entity *recent_project = df_entity_from_name_and_kind(cfg_path, DF_EntityKind_RecentProject);
            if(df_entity_is_nil(recent_project))
            {
              recent_project = df_entity_alloc(0, df_entity_root(), DF_EntityKind_RecentProject);
              df_entity_equip_name(0, recent_project, cfg_path);
              df_entity_equip_cfg_src(recent_project, DF_CfgSrc_User);
            }
          }
          
          //- rjf: eliminate all existing entities
          {
            DF_EntityList rps = df_query_cached_entity_list_with_kind(DF_EntityKind_RecentProject);
            for(DF_EntityNode *n = rps.first; n != 0; n = n->next)
            {
              if(n->entity->cfg_src == src)
              {
                df_entity_mark_for_deletion(n->entity);
              }
            }
            DF_EntityList targets = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
            for(DF_EntityNode *n = targets.first; n != 0; n = n->next)
            {
              if(n->entity->cfg_src == src)
              {
                df_entity_mark_for_deletion(n->entity);
              }
            }
            DF_EntityList bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
            for(DF_EntityNode *n = bps.first; n != 0; n = n->next)
            {
              if(n->entity->cfg_src == src)
              {
                df_entity_mark_for_deletion(n->entity);
              }
            }
            DF_EntityList pins = df_query_cached_entity_list_with_kind(DF_EntityKind_WatchPin);
            for(DF_EntityNode *n = pins.first; n != 0; n = n->next)
            {
              if(n->entity->cfg_src == src)
              {
                df_entity_mark_for_deletion(n->entity);
              }
            }
            DF_EntityList links = df_query_cached_entity_list_with_kind(DF_EntityKind_OverrideFileLink);
            for(DF_EntityNode *n = links.first; n != 0; n = n->next)
            {
              if(n->entity->cfg_src == src)
              {
                df_entity_mark_for_deletion(n->entity);
              }
            }
          }
          
          //- rjf: apply recent projects
          DF_CfgVal *recent_projects = df_cfg_val_from_string(table, str8_lit("recent_project"));
          for(DF_CfgNode *rp = recent_projects->first;
              rp != &df_g_nil_cfg_node;
              rp = rp->next)
          {
            if(rp->source == src)
            {
              String8 path_saved = rp->first->string;
              String8 path_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, path_saved, cfg_folder);
              DF_Entity *existing = df_entity_from_name_and_kind(path_absolute, DF_EntityKind_RecentProject);
              if(df_entity_is_nil(existing))
              {
                DF_Entity *rp_ent = df_entity_alloc(0, df_entity_root(), DF_EntityKind_RecentProject);
                df_entity_equip_cfg_src(rp_ent, src);
                df_entity_equip_name(0, rp_ent, path_absolute);
              }
            }
          }
          
          //- rjf: apply targets
          DF_CfgVal *targets = df_cfg_val_from_string(table, str8_lit("target"));
          {
            B32 cmd_line_target_present = 0;
            {
              DF_EntityList existing_target_entities = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
              for(DF_EntityNode *n = existing_target_entities.first; n != 0; n = n->next)
              {
                DF_Entity *target = n->entity;
                if(target->cfg_src == DF_CfgSrc_CommandLine && target->b32)
                {
                  cmd_line_target_present = 1;
                }
              }
            }
            for(DF_CfgNode *target = targets->first;
                target != &df_g_nil_cfg_node;
                target = target->next)
            {
              if(target->source == src)
              {
                DF_CfgNode *label_cfg  = df_cfg_node_child_from_string(target, str8_lit("label"),             StringMatchFlag_CaseInsensitive);
                DF_CfgNode *exe_cfg    = df_cfg_node_child_from_string(target, str8_lit("exe"),               StringMatchFlag_CaseInsensitive);
                if(exe_cfg == &df_g_nil_cfg_node)
                {
                  exe_cfg = df_cfg_node_child_from_string(target, str8_lit("name"), StringMatchFlag_CaseInsensitive);
                }
                DF_CfgNode *args_cfg   = df_cfg_node_child_from_string(target, str8_lit("arguments"),         StringMatchFlag_CaseInsensitive);
                DF_CfgNode *wdir_cfg   = df_cfg_node_child_from_string(target, str8_lit("working_directory"), StringMatchFlag_CaseInsensitive);
                DF_CfgNode *entry_cfg  = df_cfg_node_child_from_string(target, str8_lit("entry_point"),       StringMatchFlag_CaseInsensitive);
                DF_CfgNode *active_cfg = df_cfg_node_child_from_string(target, str8_lit("active"),            StringMatchFlag_CaseInsensitive);
                Vec4F32 hsva = df_hsva_from_cfg_node(target);
                U64 is_active_u64 = 0;
                if(!cmd_line_target_present)
                {
                  try_u64_from_str8_c_rules(active_cfg->first->string, &is_active_u64);
                }
                DF_Entity *target__ent = df_entity_alloc(0, df_entity_root(), DF_EntityKind_Target);
                DF_Entity *exe__ent    = df_entity_alloc(0, target__ent, DF_EntityKind_Executable);
                DF_Entity *args__ent   = df_entity_alloc(0, target__ent, DF_EntityKind_Arguments);
                DF_Entity *path__ent   = df_entity_alloc(0, target__ent, DF_EntityKind_ExecutionPath);
                DF_Entity *entry__ent  = df_entity_alloc(0, target__ent, DF_EntityKind_EntryPointName);
                String8 saved_label = label_cfg->first->string;
                String8 saved_exe = exe_cfg->first->string;
                String8 saved_exe_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, saved_exe, cfg_folder);
                String8 saved_wdir = wdir_cfg->first->string;
                String8 saved_wdir_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, saved_wdir, cfg_folder);
                String8 saved_entry_point = entry_cfg->first->string;
                String8 saved_label_raw = df_cfg_raw_from_escaped_string(scratch.arena, saved_label);
                String8 saved_entry_raw = df_cfg_raw_from_escaped_string(scratch.arena, saved_entry_point);
                String8 saved_args_raw = df_cfg_raw_from_escaped_string(scratch.arena, args_cfg->first->string);
                df_entity_equip_b32(target__ent, active_cfg != &df_g_nil_cfg_node ? !!is_active_u64 : 1);
                df_entity_equip_name(0, target__ent, saved_label_raw);
                df_entity_equip_name(0, exe__ent,    saved_exe_absolute);
                df_entity_equip_name(0, args__ent,   saved_args_raw);
                df_entity_equip_name(0, path__ent,   saved_wdir_absolute);
                df_entity_equip_name(0, entry__ent,  saved_entry_raw);
                df_entity_equip_cfg_src(target__ent, src);
                if(!memory_is_zero(&hsva, sizeof(hsva)))
                {
                  df_entity_equip_color_hsva(target__ent, hsva);
                }
              }
            }
          }
          
          //- rjf: apply path maps
          DF_CfgVal *path_maps = df_cfg_val_from_string(table, str8_lit("file_path_map"));
          for(DF_CfgNode *map = path_maps->first;
              map != &df_g_nil_cfg_node;
              map = map->next)
          {
            if(map->source == src)
            {
              DF_CfgNode *src_cfg = df_cfg_node_child_from_string(map, str8_lit("source_path"), StringMatchFlag_CaseInsensitive);
              DF_CfgNode *dst_cfg = df_cfg_node_child_from_string(map, str8_lit("dest_path"),   StringMatchFlag_CaseInsensitive);
              String8 src_path = src_cfg->first->string;
              String8 dst_path = dst_cfg->first->string;
              DF_Entity *link_loc_entity = df_entity_from_path(src_path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
              DF_Entity *link_entity = df_entity_alloc(0, link_loc_entity->parent, DF_EntityKind_OverrideFileLink);
              DF_Entity *link_dst_entity = df_entity_from_path(dst_path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
              df_entity_equip_name(0, link_entity, str8_skip_last_slash(src_path));
              df_entity_equip_entity_handle(link_entity, df_handle_from_entity(link_dst_entity));
              df_entity_equip_cfg_src(link_entity, src);
            }
          }
          
          //- rjf: apply auto view rules
          DF_CfgVal *avrs = df_cfg_val_from_string(table, str8_lit("auto_view_rule"));
          for(DF_CfgNode *map = avrs->first;
              map != &df_g_nil_cfg_node;
              map = map->next)
          {
            if(map->source == src)
            {
              DF_CfgNode *src_cfg = df_cfg_node_child_from_string(map, str8_lit("type"), StringMatchFlag_CaseInsensitive);
              DF_CfgNode *dst_cfg = df_cfg_node_child_from_string(map, str8_lit("view_rule"), StringMatchFlag_CaseInsensitive);
              String8 type = src_cfg->first->string;
              String8 view_rule = dst_cfg->first->string;
              type = df_cfg_raw_from_escaped_string(scratch.arena, type);
              view_rule = df_cfg_raw_from_escaped_string(scratch.arena, view_rule);
              DF_Entity *map_entity = df_entity_alloc(0, df_entity_root(), DF_EntityKind_AutoViewRule);
              DF_Entity *src_entity = df_entity_alloc(0, map_entity, DF_EntityKind_Source);
              DF_Entity *dst_entity = df_entity_alloc(0, map_entity, DF_EntityKind_Dest);
              df_entity_equip_name(0, src_entity, type);
              df_entity_equip_name(0, dst_entity, view_rule);
              df_entity_equip_cfg_src(map_entity, src);
            }
          }
          
          //- rjf: apply breakpoints
          DF_CfgVal *bps = df_cfg_val_from_string(table, str8_lit("breakpoint"));
          for(DF_CfgNode *bp = bps->first;
              bp != &df_g_nil_cfg_node;
              bp = bp->next)
          {
            if(bp->source != src)
            {
              continue;
            }
            
            // rjf: get metadata
            Vec4F32 hsva = df_hsva_from_cfg_node(bp);
            
            // rjf: get nodes encoding location info
            B32 is_enabled = 1;
            DF_CfgNode *line_cfg = &df_g_nil_cfg_node;
            DF_CfgNode *addr_cfg = &df_g_nil_cfg_node;
            DF_CfgNode *symb_cfg = &df_g_nil_cfg_node;
            DF_CfgNode *labl_cfg = &df_g_nil_cfg_node;
            for(DF_CfgNode *child = bp->first; child != &df_g_nil_cfg_node; child = child->next)
            {
              if(child->flags & DF_CfgNodeFlag_Identifier && str8_match(child->string, str8_lit("line"), StringMatchFlag_CaseInsensitive))
              {
                line_cfg = child;
              }
              if(child->flags & DF_CfgNodeFlag_Identifier && str8_match(child->string, str8_lit("addr"), StringMatchFlag_CaseInsensitive))
              {
                addr_cfg = child;
              }
              if(child->flags & DF_CfgNodeFlag_Identifier && str8_match(child->string, str8_lit("symbol"), StringMatchFlag_CaseInsensitive))
              {
                symb_cfg = child;
              }
              else if(child->flags & DF_CfgNodeFlag_Identifier && str8_match(child->string, str8_lit("label"), StringMatchFlag_CaseInsensitive))
              {
                labl_cfg = child;
              }
              else if(child->flags & DF_CfgNodeFlag_Identifier && str8_match(child->string, str8_lit("enabled"), StringMatchFlag_CaseInsensitive))
              {
                U64 is_enabled_u64 = 0;
                try_u64_from_str8_c_rules(child->first->string, &is_enabled_u64);
                is_enabled = (B32)is_enabled_u64;
              }
            }
            
            // rjf: extract textual location bp info
            DF_Entity *bp_parent_ent = df_entity_root();
            TxtPt pt = {0};
            if(line_cfg != &df_g_nil_cfg_node)
            {
              DF_CfgNode *file = line_cfg->first;
              DF_CfgNode *line = file->first;
              U64 line_num = 0;
              if(try_u64_from_str8_c_rules(line->string, &line_num))
              {
                String8 saved_path = file->string;
                String8 saved_path_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, saved_path, cfg_folder);
                bp_parent_ent = df_entity_from_path(saved_path_absolute, DF_EntityFromPathFlag_All);
                pt = txt_pt((S64)line_num, 1);
              }
            }
            
            // rjf: get condition info
            DF_CfgNode *cond_cfg = df_cfg_node_child_from_string(bp, str8_lit("condition"), StringMatchFlag_CaseInsensitive);
            
            // rjf: build entity
            {
              DF_Entity *bp_ent = df_entity_alloc(0, bp_parent_ent, DF_EntityKind_Breakpoint);
              df_entity_equip_b32(bp_ent, is_enabled);
              df_entity_equip_cfg_src(bp_ent, src);
              if(pt.line != 0)
              {
                df_entity_equip_txt_pt(bp_ent, pt);
              }
              if(addr_cfg != &df_g_nil_cfg_node)
              {
                U64 u64 = 0;
                try_u64_from_str8_c_rules(addr_cfg->first->string, &u64);
                df_entity_equip_vaddr(bp_ent, u64);
              }
              if(symb_cfg != &df_g_nil_cfg_node)
              {
                String8 symb_raw = df_cfg_raw_from_escaped_string(scratch.arena, symb_cfg->first->string);
                DF_Entity *symb = df_entity_alloc(0, bp_ent, DF_EntityKind_EntryPointName);
                df_entity_equip_name(0, symb, symb_raw);
              }
              if(labl_cfg->string.size != 0)
              {
                String8 label_raw = df_cfg_raw_from_escaped_string(scratch.arena, labl_cfg->string);
                df_entity_equip_name(0, bp_ent, label_raw);
              }
              if(!memory_is_zero(&hsva, sizeof(hsva)))
              {
                df_entity_equip_color_hsva(bp_ent, hsva);
              }
              if(cond_cfg->first->string.size != 0)
              {
                String8 cond_raw = df_cfg_raw_from_escaped_string(scratch.arena, cond_cfg->first->string);
                DF_Entity *cond = df_entity_alloc(0, bp_ent, DF_EntityKind_Condition);
                df_entity_equip_name(0, cond, cond_raw);
              }
            }
          }
          
          //- rjf: apply watch pins
          DF_CfgVal *pins = df_cfg_val_from_string(table, str8_lit("watch_pin"));
          for(DF_CfgNode *pin = pins->first;
              pin != &df_g_nil_cfg_node;
              pin = pin->next)
          {
            if(pin->source != src)
            {
              continue;
            }
            Vec4F32 hsva = df_hsva_from_cfg_node(pin);
            String8 string = df_string_from_cfg_node_key(pin, str8_lit("expression"), StringMatchFlag_CaseInsensitive);
            String8 string_raw = df_cfg_raw_from_escaped_string(scratch.arena, string);
            DF_CfgNode *line_cfg = df_cfg_node_child_from_string(pin, str8_lit("line"), StringMatchFlag_CaseInsensitive);
            DF_CfgNode *addr_cfg = df_cfg_node_child_from_string(pin, str8_lit("addr"), StringMatchFlag_CaseInsensitive);
            DF_Entity *pin_parent_ent = df_entity_root();
            TxtPt pt = {0};
            if(line_cfg != &df_g_nil_cfg_node)
            {
              String8 saved_path = line_cfg->first->string;
              String8 line_num_string = line_cfg->first->first->string;
              String8 saved_path_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, saved_path, cfg_folder);
              pin_parent_ent = df_entity_from_path(saved_path_absolute, DF_EntityFromPathFlag_All);
              U64 line_num = 0;
              if(try_u64_from_str8_c_rules(line_num_string, &line_num))
              {
                if(line_num != 0)
                {
                  pt = txt_pt((S64)line_num, 1);
                }
              }
            }
            U64 vaddr = 0;
            if(addr_cfg != &df_g_nil_cfg_node)
            {
              try_u64_from_str8_c_rules(addr_cfg->first->string, &vaddr);
            }
            DF_Entity *pin_ent = df_entity_alloc(0, pin_parent_ent, DF_EntityKind_WatchPin);
            df_entity_equip_cfg_src(pin_ent, src);
            df_entity_equip_name(0, pin_ent, string_raw);
            if(!memory_is_zero(&hsva, sizeof(hsva)))
            {
              df_entity_equip_color_hsva(pin_ent, hsva);
            }
            if(pt.line != 0)
            {
              df_entity_equip_txt_pt(pin_ent, pt);
            }
            if(vaddr != 0)
            {
              df_entity_equip_vaddr(pin_ent, vaddr);
            }
          }
          
          //- rjf: apply exception code filters
          DF_CfgVal *filter_tables = df_cfg_val_from_string(table, str8_lit("exception_code_filters"));
          for(DF_CfgNode *table = filter_tables->first;
              table != &df_g_nil_cfg_node;
              table = table->next)
          {
            for(DF_CfgNode *rule = table->first;
                rule != &df_g_nil_cfg_node;
                rule = rule->next)
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
          df_state->cfg_write_issued[src] = 1;
          df_cfg_push_write_string(src, data);
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
                map = df_entity_alloc(0, map_parent, DF_EntityKind_OverrideFileLink);
              }
              else
              {
                df_entity_change_parent(0, map, map->parent, map_parent);
              }
              df_entity_equip_name(0, map, path_file);
            }break;
            case DF_CoreCmdKind_SetFileOverrideLinkDst:
            {
              if(df_entity_is_nil(map))
              {
                map = df_entity_alloc(0, df_entity_root(), DF_EntityKind_OverrideFileLink);
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
            DF_Entity *link = df_entity_child_from_name_and_kind(first_diff_src->parent, first_diff_src->name, DF_EntityKind_OverrideFileLink);
            if(df_entity_is_nil(link))
            {
              link = df_entity_alloc(0, first_diff_src->parent, DF_EntityKind_OverrideFileLink);
              df_entity_equip_name(0, link, first_diff_src->name);
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
            map = df_entity_alloc(df_state_delta_history(), df_entity_root(), DF_EntityKind_AutoViewRule);
            df_entity_equip_cfg_src(map, DF_CfgSrc_Project);
          }
          DF_Entity *src = df_entity_child_from_kind(map, DF_EntityKind_Source);
          if(df_entity_is_nil(src))
          {
            src = df_entity_alloc(df_state_delta_history(), map, DF_EntityKind_Source);
          }
          DF_Entity *dst = df_entity_child_from_kind(map, DF_EntityKind_Dest);
          if(df_entity_is_nil(dst))
          {
            dst = df_entity_alloc(df_state_delta_history(), map, DF_EntityKind_Dest);
          }
          if(map->kind == DF_EntityKind_AutoViewRule)
          {
            DF_Entity *edit_child = (core_cmd_kind == DF_CoreCmdKind_SetAutoViewRuleType ? src : dst);
            df_entity_equip_name(df_state_delta_history(), edit_child, params.string);
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
          df_state_delta_history_push_batch(df_state->hist, &entity->generation);
          df_state_delta_history_push_struct_delta(df_state->hist, &entity->b32);
          df_entity_equip_b32(entity, 1);
        }break;
        case DF_CoreCmdKind_DisableEntity:
        case DF_CoreCmdKind_DisableBreakpoint:
        case DF_CoreCmdKind_DisableTarget:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          df_state_delta_history_push_batch(df_state->hist, &entity->generation);
          df_state_delta_history_push_struct_delta(df_state->hist, &entity->b32);
          df_entity_equip_b32(entity, 0);
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
          DF_EntityOpFlags op_flags = df_g_entity_kind_op_flags_table[entity->kind];
          if(op_flags & DF_EntityOpFlag_Delete)
          {
            df_entity_mark_for_deletion(entity);
          }
        }break;
        case DF_CoreCmdKind_NameEntity:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          String8 string = params.string;
          df_state_delta_history_push_batch(df_state_delta_history(), &entity->generation);
          df_entity_equip_name(df_state_delta_history(), entity, string);
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
            df_state_delta_history_push_batch(df_state_delta_history(), 0);
            for(Task *task = first_task; task != 0; task = task->next)
            {
              DF_Entity *src_n = task->src_n;
              DF_Entity *dst_n = df_entity_alloc(df_state_delta_history(), task->dst_parent, task->src_n->kind);
              if(src_n->flags & DF_EntityFlag_HasTextPoint)    {df_entity_equip_txt_pt(dst_n, src_n->text_point);}
              if(src_n->flags & DF_EntityFlag_HasTextPointAlt) {df_entity_equip_txt_pt_alt(dst_n, src_n->text_point_alt);}
              if(src_n->flags & DF_EntityFlag_HasB32)          {df_entity_equip_b32(dst_n, src_n->b32);}
              if(src_n->flags & DF_EntityFlag_HasU64)          {df_entity_equip_u64(dst_n, src_n->u64);}
              if(src_n->flags & DF_EntityFlag_HasRng1U64)      {df_entity_equip_rng1u64(dst_n, src_n->rng1u64);}
              if(src_n->flags & DF_EntityFlag_HasColor)        {df_entity_equip_color_hsva(dst_n, df_hsva_from_entity(src_n));}
              if(src_n->flags & DF_EntityFlag_HasVAddrRng)     {df_entity_equip_vaddr_rng(dst_n, src_n->vaddr_rng);}
              if(src_n->flags & DF_EntityFlag_HasVAddr)        {df_entity_equip_vaddr(dst_n, src_n->vaddr);}
              if(src_n->name.size != 0)                        {df_entity_equip_name(df_state_delta_history(), dst_n, src_n->name);}
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
        
        //- rjf: breakpoints
        case DF_CoreCmdKind_TextBreakpoint:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          if(df_entity_is_nil(entity))
          {
            entity = df_entity_from_path(params.file_path, 0);
          }
          if(!df_entity_is_nil(entity))
          {
            S64 line_num = params.text_point.line;
            B32 removed_existing = 0;
            for(DF_Entity *child = entity->first, *next = 0; !df_entity_is_nil(child); child = next)
            {
              next = child->next;
              if(child->deleted) { continue; }
              if(child->kind == DF_EntityKind_Breakpoint && child->flags & DF_EntityFlag_HasTextPoint && child->text_point.line == line_num)
              {
                removed_existing = 1;
                df_entity_mark_for_deletion(child);
              }
            }
            if(removed_existing == 0)
            {
              df_state_delta_history_push_batch(df_state_delta_history(), 0);
              DF_Entity *bp = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_Breakpoint);
              df_entity_equip_txt_pt(bp, params.text_point);
              df_entity_equip_b32(bp, 1);
              df_entity_equip_cfg_src(bp, DF_CfgSrc_Project);
            }
          }
        }break;
        case DF_CoreCmdKind_AddressBreakpoint:
        {
          U64 vaddr = params.vaddr;
          if(vaddr != 0)
          {
            DF_Entity *bp = &df_g_nil_entity;
            DF_EntityList existing_bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
            for(DF_EntityNode *n = existing_bps.first; n != 0; n = n->next)
            {
              if(n->entity->vaddr == vaddr)
              {
                bp = n->entity;
                break;
              }
            }
            if(df_entity_is_nil(bp))
            {
              df_state_delta_history_push_batch(df_state_delta_history(), 0);
              bp = df_entity_alloc(df_state_delta_history(), df_entity_root(), DF_EntityKind_Breakpoint);
              df_entity_equip_vaddr(bp, vaddr);
              df_entity_equip_b32(bp, 1);
              df_entity_equip_cfg_src(bp, DF_CfgSrc_Project);
            }
            else
            {
              df_entity_mark_for_deletion(bp);
            }
          }
        }break;
        case DF_CoreCmdKind_FunctionBreakpoint:
        {
          String8 function_name = params.string;
          if(function_name.size != 0)
          {
            DF_Entity *symb = df_entity_from_name_and_kind(function_name, DF_EntityKind_EntryPointName);
            DF_Entity *bp = df_entity_ancestor_from_kind(symb, DF_EntityKind_Breakpoint);
            if(df_entity_is_nil(bp))
            {
              df_state_delta_history_push_batch(df_state_delta_history(), 0);
              bp = df_entity_alloc(df_state_delta_history(), df_entity_root(), DF_EntityKind_Breakpoint);
              DF_Entity *symbol_name_entity = df_entity_alloc(df_state_delta_history(), bp, DF_EntityKind_EntryPointName);
              df_entity_equip_name(df_state_delta_history(), symbol_name_entity, function_name);
              df_entity_equip_b32(bp, 1);
              df_entity_equip_cfg_src(bp, DF_CfgSrc_Project);
            }
            else
            {
              df_entity_mark_for_deletion(bp);
            }
          }
        }break;
        
        //- rjf: watches
        case DF_CoreCmdKind_ToggleWatchPin:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          S64 line_num = params.text_point.line;
          if(!df_entity_is_nil(entity) && line_num != 0)
          {
            B32 removed_existing = 0;
            for(DF_Entity *child = entity->first, *next = 0; !df_entity_is_nil(child); child = next)
            {
              next = child->next;
              if(child->deleted) { continue; }
              if(child->kind == DF_EntityKind_WatchPin && child->flags & DF_EntityFlag_HasTextPoint && child->text_point.line == line_num &&
                 str8_match(child->name, params.string, 0))
              {
                removed_existing = 1;
                df_entity_mark_for_deletion(child);
              }
            }
            if(removed_existing == 0)
            {
              df_state_delta_history_push_batch(df_state_delta_history(), 0);
              DF_Entity *watch = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_WatchPin);
              df_entity_equip_txt_pt(watch, params.text_point);
              df_entity_equip_name(df_state_delta_history(), watch, params.string);
              df_entity_equip_cfg_src(watch, DF_CfgSrc_Project);
            }
          }
          else if(params.vaddr != 0)
          {
            B32 removed_existing = 0;
            DF_EntityList pins = df_query_cached_entity_list_with_kind(DF_EntityKind_WatchPin);
            for(DF_EntityNode *n = pins.first; n != 0; n = n->next)
            {
              DF_Entity *pin = n->entity;
              if(pin->flags & DF_EntityFlag_HasVAddr && pin->vaddr == params.vaddr && str8_match(pin->name, params.string, 0))
              {
                removed_existing = 1;
                df_entity_mark_for_deletion(pin);
              }
            }
            if(!removed_existing)
            {
              df_state_delta_history_push_batch(df_state_delta_history(), 0);
              DF_Entity *pin = df_entity_alloc(df_state_delta_history(), df_entity_root(), DF_EntityKind_WatchPin);
              df_entity_equip_vaddr(pin, params.vaddr);
              df_entity_equip_name(df_state_delta_history(), pin, params.string);
              df_entity_equip_cfg_src(pin, DF_CfgSrc_Project);
            }
          }
        }break;
        
        //- rjf: cursor operations
        case DF_CoreCmdKind_ToggleBreakpointAtCursor:
        {
          DF_InteractRegs *regs = df_interact_regs();
          DF_Entity *file = df_entity_from_handle(regs->file);
          if(file->kind == DF_EntityKind_File && regs->cursor.line != 0)
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.entity = df_handle_from_entity(file);
            p.text_point = regs->cursor;
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_TextBreakpoint));
          }
          else if(regs->vaddr_range.min != 0)
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.vaddr = regs->vaddr_range.min;
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddressBreakpoint));
          }
        }break;
        case DF_CoreCmdKind_ToggleWatchPinAtCursor:
        {
          DF_InteractRegs *regs = df_interact_regs();
          DF_Entity *file = df_entity_from_handle(regs->file);
          if(file->kind == DF_EntityKind_File && regs->cursor.line != 0)
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.entity = df_handle_from_entity(file);
            p.text_point = regs->cursor;
            p.string = params.string;
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
          }
          else if(regs->vaddr_range.min != 0)
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.vaddr = regs->vaddr_range.min;
            p.string = params.string;
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
          }
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
          DF_Entity *file = df_entity_from_handle(df_interact_regs()->file);
          if(!df_entity_is_nil(file))
          {
            DF_CmdParams p = df_cmd_params_zero();
            p.entity = df_handle_from_entity(file);
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
          DF_Entity *file = df_entity_from_handle(df_interact_regs()->file);
          DF_Entity *thread = df_entity_from_handle(df_interact_regs()->thread);
          U64 new_rip_vaddr = df_interact_regs()->vaddr_range.min;
          if(!df_entity_is_nil(file))
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
          df_state_delta_history_push_batch(df_state_delta_history(), 0);
          DF_Entity *entity = df_entity_alloc(df_state_delta_history(), df_entity_root(), DF_EntityKind_Target);
          df_entity_equip_cfg_src(entity, DF_CfgSrc_Project);
          DF_Entity *exe = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_Executable);
          df_entity_equip_name(df_state_delta_history(), exe, params.file_path);
          String8 working_dir = str8_chop_last_slash(params.file_path);
          if(working_dir.size != 0)
          {
            String8 working_dir_path = push_str8f(scratch.arena, "%S/", working_dir);
            DF_Entity *execution_path = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_ExecutionPath);
            df_entity_equip_name(df_state_delta_history(), execution_path, working_dir_path);
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
            DF_EntityList all_targets = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
            B32 is_selected = entity->b32;
            for(DF_EntityNode *n = all_targets.first; n != 0; n = n->next)
            {
              DF_Entity *target = n->entity;
              df_entity_equip_b32(target, 0);
            }
            if(!is_selected)
            {
              df_entity_equip_b32(entity, 1);
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
  
  //- rjf: fill core interaction register info
  {
    DF_Entity *thread = df_entity_from_handle(df_state->ctrl_ctx.thread);
    DF_Entity *module = df_module_from_thread(thread);
    DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
    df_interact_regs()->thread = df_handle_from_entity(thread);
    df_interact_regs()->module = df_handle_from_entity(module);
    df_interact_regs()->process = df_handle_from_entity(process);
    df_interact_regs()->unwind_count = df_state->ctrl_ctx.unwind_count;
    df_interact_regs()->inline_depth = df_state->ctrl_ctx.inline_depth;
  }
  
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
  
  //- rjf: eliminate entities that are marked for deletion + kill off entities with a death-timer
  ProfScope("eliminate deleted/deletion-timer entities")
  {
    for(DF_Entity *entity = df_entity_root(), *next = 0; !df_entity_is_nil(entity); entity = next)
    {
      next = df_entity_rec_df_pre(entity, &df_g_nil_entity).next;
      if(entity->flags & DF_EntityFlag_DiesWithTime)
      {
        entity->life_left -= df_dt();
        if(entity->life_left <= 0.f)
        {
          df_entity_mark_for_deletion(entity);
        }
      }
      if(entity->flags & DF_EntityFlag_MarkedForDeletion)
      {
        B32 undoable = (df_g_entity_kind_flags_table[entity->kind] & DF_EntityKindFlag_UserDefinedLifetime);
        
        // rjf: fixup next entity to iterate to
        next = df_entity_rec_df(entity, &df_g_nil_entity, OffsetOf(DF_Entity, next), OffsetOf(DF_Entity, next)).next;
        
        // rjf: undoable -> just mark as deleted; this must be able to be trivially undone
        if(undoable)
        {
          df_state_delta_history_push_batch(df_state->hist, 0);
          df_state_delta_history_push_struct_delta(df_state->hist, &entity->deleted);
          df_state_delta_history_push_struct_delta(df_state->hist, &entity->generation);
          df_state_delta_history_push_struct_delta(df_state->hist, &df_state->kind_alloc_gens[entity->kind]);
          entity->deleted = 1;
          entity->generation += 1;
          entity->flags &= ~DF_EntityFlag_MarkedForDeletion;
          df_state->kind_alloc_gens[entity->kind] += 1;
        }
        
        // rjf: not undoable -> actually release
        if(!undoable)
        {
          // rjf: eliminate root entity if we're freeing it
          if(entity == df_state->entities_root)
          {
            df_state->entities_root = &df_g_nil_entity;
          }
          
          // rjf: unhook & release this entity tree
          df_entity_change_parent(0, entity, entity->parent, &df_g_nil_entity);
          df_entity_release(0, entity);
        }
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
  
  ProfEnd();
}
