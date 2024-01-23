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
df_expand_key_make(U64 uniquifier, U64 parent_hash, U64 child_num)
{
  DF_ExpandKey key;
  {
    key.uniquifier = uniquifier;
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
    key.uniquifier,
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

internal void
df_expand_tree_table_animate(DF_ExpandTreeTable *table, F32 dt)
{
  F32 rate = 1 - pow_f32(2, (-50.f * dt));
  for(U64 slot_idx = 0; slot_idx < table->slots_count; slot_idx += 1)
  {
    for(DF_ExpandNode *node = table->slots[slot_idx].first;
        node != 0;
        node = node->hash_next)
    {
      node->expanded_t += (((F32)!!node->expanded) - node->expanded_t) * rate;
    }
  }
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
//~ rjf: Disassembling

#include "third_party/udis86/config.h"
#include "third_party/udis86/udis86.h"
#include "third_party/udis86/libudis86/syn.h"

internal DF_Inst
df_single_inst_from_machine_code__x64(Arena *arena, U64 start_voff, String8 string)
{
  Architecture arch = Architecture_x64;
  
  //- rjf: prep ud state
  struct ud ud_ctx_;
  struct ud *ud_ctx = &ud_ctx_;
  ud_init(ud_ctx);
  ud_set_mode(ud_ctx, bit_size_from_arch(arch));
  ud_set_pc(ud_ctx, start_voff);
  ud_set_input_buffer(ud_ctx, string.str, string.size);
  ud_set_vendor(ud_ctx, UD_VENDOR_ANY);
  ud_set_syntax(ud_ctx, UD_SYN_INTEL);
  
  //- rjf: disassembly + get info
  U32 bytes_disassembled = ud_disassemble(ud_ctx);
  struct ud_operand *first_op = (struct ud_operand *)ud_insn_opr(ud_ctx, 0);
  U64 rel_voff = (first_op != 0 && first_op->type == UD_OP_JIMM) ? ud_syn_rel_target(ud_ctx, first_op) : 0;
  DF_InstFlags flags = 0;
  enum ud_mnemonic_code code = ud_insn_mnemonic(ud_ctx);
  switch(code)
  {
    case UD_Icall:
    {
      flags |= DF_InstFlag_Call;
    }break;
    
    /* TODO(wonchun)
  case UD_Iiretd:
  case UD_Iiretw:
    */
    
    case UD_Ija:
    case UD_Ijae:
    case UD_Ijb:
    case UD_Ijbe:
    case UD_Ijcxz:
    case UD_Ijecxz:
    case UD_Ijg:
    case UD_Ijge:
    case UD_Ijl:
    case UD_Ijle:
    {
      flags |= DF_InstFlag_Branch;
    }break;
    
    case UD_Ijmp:
    {
      flags |= DF_InstFlag_UnconditionalJump;
    }break;
    
    case UD_Ijno:
    case UD_Ijnp:
    case UD_Ijns:
    case UD_Ijnz:
    case UD_Ijo:
    case UD_Ijp:
    case UD_Ijrcxz:
    case UD_Ijs:
    case UD_Ijz:
    case UD_Iloop:
    case UD_Iloope:
    case UD_Iloopne:
    {
      flags |= DF_InstFlag_Branch;
    }break;
    
    case UD_Iret:
    case UD_Iretf:
    {
      flags |= DF_InstFlag_Return;
    }break;
    
    /* TODO(wonchun)
  case UD_Isyscall:
  case UD_Isysenter:
  case UD_Isysexit:
  case UD_Isysret:
  case UD_Ivmcall:
  case UD_Ivmmcall:
    */
    default:
    {
      flags |= DF_InstFlag_NonFlow;
    }break;
  }
  
  //- rjf: check for stack pointer modifications
  S64 sp_delta = 0;
  {
    struct ud_operand *dst_op = (struct ud_operand *)ud_insn_opr(ud_ctx, 0);
    struct ud_operand *src_op = (struct ud_operand *)ud_insn_opr(ud_ctx, 1);
    
    // rjf: direct additions/subtractions to RSP
    if(dst_op && src_op && dst_op->base == UD_R_RSP && dst_op->type == UD_OP_REG)
    {
      flags |= DF_InstFlag_ChangesStackPointer;
      // TODO(rjf): does the library report constant changes to the stack pointer
      // as UD_OP_CONST too? what does UD_OP_JIMM refer to?
      if(src_op->base == UD_NONE && src_op->type == UD_OP_IMM && code == UD_Isub)
      {
        S64 sign = -1;
        sp_delta = sign * src_op->lval.sqword;
      }
      else if(src_op->base == UD_NONE && src_op->type == UD_OP_IMM && code == UD_Iadd)
      {
        S64 sign = +1;
        sp_delta = sign * src_op->lval.sqword;
      }
      else
      {
        flags |= DF_InstFlag_ChangesStackPointerVariably;
      }
    }
    
    // rjf: push/pop
    if(code == UD_Ipush)
    {
      flags |= DF_InstFlag_ChangesStackPointer;
      sp_delta = -8;
    }
    else if(code == UD_Ipop)
    {
      flags |= DF_InstFlag_ChangesStackPointer;
      sp_delta = +8;
    }
    
    // rjf: mark extra flags
    if(ud_ctx->pfx_rep != 0 ||
       ud_ctx->pfx_repe != 0 ||
       ud_ctx->pfx_repne != 0)
    {
      flags |= DF_InstFlag_Repeats;
    }
  }
  
  //- rjf: fill+return
  DF_Inst inst = {0};
  inst.size = bytes_disassembled;
  inst.string = push_str8_copy(arena, str8_cstring((char *)ud_insn_asm(ud_ctx)));
  inst.rel_voff = rel_voff;
  inst.sp_delta = sp_delta;
  inst.flags = flags;
  return inst;
}

internal DF_Inst
df_single_inst_from_machine_code(Arena *arena, Architecture arch, U64 start_voff, String8 string)
{
  DF_Inst result = {0};
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:
    {
      result = df_single_inst_from_machine_code__x64(arena, start_voff, string);
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Control Flow Analysis Functions

internal DF_CtrlFlowInfo
df_ctrl_flow_info_from_vaddr_code__x64(Arena *arena, DF_InstFlags exit_points_mask, U64 vaddr, String8 code)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_CtrlFlowInfo info = {0};
  for(U64 offset = 0; offset < code.size;)
  {
    DF_Inst inst = df_single_inst_from_machine_code__x64(scratch.arena, 0, str8_skip(code, offset));
    U64 inst_vaddr = vaddr+offset;
    info.cumulative_sp_delta += inst.sp_delta;
    offset += inst.size;
    info.total_size += inst.size;
    if(inst.flags & exit_points_mask)
    {
      DF_CtrlFlowPoint point = {0};
      point.inst_flags = inst.flags;
      point.vaddr = inst_vaddr;
      point.jump_dest_vaddr = 0;
      point.expected_sp_delta = info.cumulative_sp_delta;
      if(inst.rel_voff != 0)
      {
        point.jump_dest_vaddr = (U64)(point.vaddr + (S64)((S32)inst.rel_voff));
      }
      DF_CtrlFlowPointNode *node = push_array(arena, DF_CtrlFlowPointNode, 1);
      node->point = point;
      SLLQueuePush(info.exit_points.first, info.exit_points.last, node);
      info.exit_points.count += 1;
    }
  }
  scratch_end(scratch);
  return info;
}

internal DF_CtrlFlowInfo
df_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DF_InstFlags exit_points_mask, Architecture arch, U64 vaddr, String8 code)
{
  DF_CtrlFlowInfo result = {0};
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:
    {
      result = df_ctrl_flow_info_from_vaddr_code__x64(arena, exit_points_mask, vaddr, code);
    }break;
  }
  return result;
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
  qsort(array.v, array.count, sizeof(DF_CmdSpec *), (int (*)(const void *, const void *))df_qsort_compare_cmd_spec__run_counter);
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
      DBGI_Scope *scope = dbgi_scope_open();
      DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
      U64 vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx->unwind_count);
      DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
      EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, vaddr);
      DF_Eval eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, &parse_ctx, query);
      if(eval.errors.count == 0)
      {
        TG_Kind eval_type_kind = tg_kind_from_key(tg_unwrapped_from_graph_raddbg_key(parse_ctx.type_graph, parse_ctx.rdbg, eval.type_key));
        if(eval_type_kind == TG_Kind_Ptr || eval_type_kind == TG_Kind_LRef || eval_type_kind == TG_Kind_RRef)
        {
          eval = df_value_mode_eval_from_eval(parse_ctx.type_graph, parse_ctx.rdbg, ctrl_ctx, eval);
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
      dbgi_scope_close(scope);
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

//- rjf: entity -> text info

internal TXTI_Handle
df_txti_handle_from_entity(DF_Entity *entity)
{
  TXTI_Handle handle = {0};
  Temp scratch = scratch_begin(0, 0);
  String8 path = df_full_path_from_entity(scratch.arena, entity);
  handle = txti_handle_from_path(path);
  scratch_end(scratch);
  return handle;
}

//- rjf: entity -> disasm info

internal DASM_Handle
df_dasm_handle_from_process_vaddr(DF_Entity *process, U64 vaddr)
{
  Rng1U64 disasm_vaddr_rng = r1u64(AlignDownPow2(vaddr, KB(4)), AlignDownPow2(vaddr, KB(4)) + KB(16));
  DASM_Handle dasm_handle = dasm_handle_from_ctrl_process_range(process->ctrl_machine_id, process->ctrl_handle, disasm_vaddr_rng);
  return dasm_handle;
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
      result = push_str8_copy(arena, entity->name);
      result = str8_skip_last_slash(result);
    }break;
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
    if(e == entity && flags & DF_EntityKindFlag_LeafMutationProfileConfig)
    {
      DF_CmdParams p = {0};
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProfileData));
    }
    if(e == entity && flags & DF_EntityKindFlag_LeafMutationSoftHalt)
    {
      df_state->entities_mut_soft_halt = 1;
    }
    if(e == entity && flags & DF_EntityKindFlag_LeafMutationDebugInfoMap)
    {
      df_state->entities_mut_dbg_info_map = 1;
    }
    if(flags & DF_EntityKindFlag_TreeMutationSoftHalt)
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
  
  // rjf: dirtify caches
  df_state->kind_alloc_gens[kind] += 1;
  df_entity_notify_mutation(entity);
  
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
df_entity_equip_ctrl_handle(DF_Entity *entity, CTRL_Handle handle)
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
    file_no_override = parent;
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
    file_overrides_applied = parent;
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
df_entity_from_ctrl_handle(CTRL_MachineID machine_id, CTRL_Handle handle)
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
  }
  
  // rjf: frozen => not frozen
  if(is_frozen && !should_be_frozen)
  {
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
//~ rjf: Debug Info Mapping

internal String8
df_debug_info_path_from_module(Arena *arena, DF_Entity *module)
{
  ProfBeginFunction();
  String8 result = {0};
  DF_Entity *override_entity = df_entity_child_from_kind(module, DF_EntityKind_DebugInfoOverride);
  if(!df_entity_is_nil(override_entity) && override_entity->name.size != 0)
  {
    result = override_entity->name;
  }
  else
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 exe_path = module->name;
    String8 dbg_path = ctrl_og_dbg_path_from_exe_path(arena, exe_path);
    result = dbg_path;
    scratch_end(scratch);
  }
  ProfEnd();
  return result;
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
  U64 ip_vaddr = df_rip_from_thread(thread);
  
  // rjf: ip => machine code
  String8 machine_code = {0};
  {
    Rng1U64 rng = r1u64(ip_vaddr, ip_vaddr+max_instruction_size_from_arch(arch));
    machine_code.str = push_array_no_zero(scratch.arena, U8, max_instruction_size_from_arch(arch));
    machine_code.size = ctrl_process_read(process->ctrl_machine_id, process->ctrl_handle, rng, machine_code.str);
  }
  
  // rjf: build traps if machine code was read successfully
  if(machine_code.size != 0)
  {
    // rjf: decode instruction
    DF_Inst inst = df_single_inst_from_machine_code(scratch.arena, arch, ip_vaddr, machine_code);
    
    // rjf: call => run until call returns
    if(inst.flags & DF_InstFlag_Call || inst.flags & DF_InstFlag_Repeats)
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
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_Entity *module = df_module_from_thread(thread);
  DF_Entity *binary = df_binary_file_from_module(module);
  Architecture arch = df_architecture_from_entity(thread);
  U64 ip_vaddr = df_rip_from_thread(thread);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = df_voff_from_vaddr(module, ip_vaddr);
    DF_TextLineDasm2SrcInfo line_info = df_text_line_dasm2src_info_from_binary_voff(binary, ip_voff);
    Rng1U64 line_voff_rng = line_info.voff_range;
    if(line_voff_rng.max != 0)
    {
      line_vaddr_rng = df_vaddr_range_from_voff_range(module, line_voff_rng);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  if(good_line_info)
  {
    machine_code.str = push_array_no_zero(scratch.arena, U8, dim_1u64(line_vaddr_rng));
    machine_code.size = ctrl_process_read(process->ctrl_machine_id, process->ctrl_handle, line_vaddr_rng, machine_code.str);
  }
  
  // rjf: machine code => ctrl flow analysis
  DF_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_line_info)
  {
    ctrl_flow_info = df_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                            DF_InstFlag_Call|
                                                            DF_InstFlag_Branch|
                                                            DF_InstFlag_UnconditionalJump|
                                                            DF_InstFlag_ChangesStackPointer|
                                                            DF_InstFlag_Return,
                                                            arch,
                                                            line_vaddr_rng.min,
                                                            machine_code);
  }
  
  // rjf: push traps for all exit points
  if(good_line_info) for(DF_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DF_CtrlFlowPoint *point = &n->point;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DF_InstFlag_Branch|
                            DF_InstFlag_UnconditionalJump|
                            DF_InstFlag_Return))
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
    else if(point->inst_flags & DF_InstFlag_Call)
    {
      flags |= (CTRL_TrapFlag_BeginSpoofMode|CTRL_TrapFlag_SingleStepAfterHit);
    }
    
    // rjf: instruction changes stack pointer => save off the stack pointer, single-step over, keep stepping
    else if(point->inst_flags & DF_InstFlag_ChangesStackPointer)
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

internal CTRL_TrapList
df_trap_net_from_thread__step_into_line(Arena *arena, DF_Entity *thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_TrapList result = {0};
  
  // rjf: thread => info
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_Entity *module = df_module_from_thread(thread);
  DF_Entity *binary = df_binary_file_from_module(module);
  Architecture arch = df_architecture_from_entity(thread);
  U64 ip_vaddr = df_rip_from_thread(thread);
  
  // rjf: ip => line vaddr range
  Rng1U64 line_vaddr_rng = {0};
  {
    U64 ip_voff = df_voff_from_vaddr(module, ip_vaddr);
    DF_TextLineDasm2SrcInfo line_info = df_text_line_dasm2src_info_from_binary_voff(binary, ip_voff);
    Rng1U64 line_voff_rng = line_info.voff_range;
    if(line_voff_rng.max != 0)
    {
      line_vaddr_rng = df_vaddr_range_from_voff_range(module, line_voff_rng);
    }
  }
  
  // rjf: line vaddr range => did we find anything successfully?
  B32 good_line_info = (line_vaddr_rng.max != 0);
  
  // rjf: line vaddr range => line's machine code
  String8 machine_code = {0};
  if(good_line_info)
  {
    machine_code.str = push_array_no_zero(scratch.arena, U8, dim_1u64(line_vaddr_rng));
    machine_code.size = ctrl_process_read(process->ctrl_machine_id, process->ctrl_handle, line_vaddr_rng, machine_code.str);
  }
  
  // rjf: machine code => ctrl flow analysis
  DF_CtrlFlowInfo ctrl_flow_info = {0};
  if(good_line_info)
  {
    ctrl_flow_info = df_ctrl_flow_info_from_arch_vaddr_code(scratch.arena,
                                                            DF_InstFlag_Call|
                                                            DF_InstFlag_Branch|
                                                            DF_InstFlag_UnconditionalJump|
                                                            DF_InstFlag_ChangesStackPointer|
                                                            DF_InstFlag_Return,
                                                            arch,
                                                            line_vaddr_rng.min,
                                                            machine_code);
  }
  
  // rjf: push traps for all exit points
  if(good_line_info) for(DF_CtrlFlowPointNode *n = ctrl_flow_info.exit_points.first; n != 0; n = n->next)
  {
    DF_CtrlFlowPoint *point = &n->point;
    CTRL_TrapFlags flags = 0;
    B32 add = 1;
    U64 trap_addr = point->vaddr;
    
    // rjf: branches/jumps/returns => single-step & end, OR trap @ destination.
    if(point->inst_flags & (DF_InstFlag_Call|
                            DF_InstFlag_Branch|
                            DF_InstFlag_UnconditionalJump|
                            DF_InstFlag_Return))
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
    else if(point->inst_flags & DF_InstFlag_ChangesStackPointer)
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

//- rjf: module <=> binary file

internal DF_Entity *
df_binary_file_from_module(DF_Entity *module)
{
  DF_Entity *binary = df_entity_from_handle(module->entity_handle);
  return binary;
}

internal DF_EntityList
df_modules_from_binary_file(Arena *arena, DF_Entity *binary_info)
{
  DF_EntityList list = {0};
  DF_EntityList all_modules = df_query_cached_entity_list_with_kind(DF_EntityKind_Module);
  for(DF_EntityNode *n = all_modules.first; n != 0; n = n->next)
  {
    DF_Entity *module = n->entity;
    DF_Entity *module_binary_info = df_binary_file_from_module(module);
    if(module_binary_info == binary_info)
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

//- rjf: binary file -> dbgi parse

internal DBGI_Parse *
df_dbgi_parse_from_binary_file(DBGI_Scope *scope, DF_Entity *binary)
{
  Temp scratch = scratch_begin(0, 0);
  String8 exe_path = df_full_path_from_entity(scratch.arena, binary);
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, exe_path, 0);
  scratch_end(scratch);
  return dbgi;
}

//- rjf: symbol lookups

internal String8
df_symbol_name_from_binary_voff(Arena *arena, DF_Entity *binary, U64 voff)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    DBGI_Scope *scope = dbgi_scope_open();
    String8 path = df_full_path_from_entity(scratch.arena, binary);
    DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, path, 0);
    RADDBG_Parsed *rdbg = &dbgi->rdbg;
    if(result.size == 0 && rdbg->scope_vmap != 0)
    {
      U64 scope_idx = raddbg_vmap_idx_from_voff(rdbg->scope_vmap, rdbg->scope_vmap_count, voff);
      RADDBG_Scope *scope = &rdbg->scopes[scope_idx];
      U64 proc_idx = scope->proc_idx;
      RADDBG_Procedure *procedure = &rdbg->procedures[proc_idx];
      U64 name_size = 0;
      U8 *name_ptr = raddbg_string_from_idx(rdbg, procedure->name_string_idx, &name_size);
      result = push_str8_copy(arena, str8(name_ptr, name_size));
    }
    if(result.size == 0 && rdbg->global_vmap != 0)
    {
      U64 global_idx = raddbg_vmap_idx_from_voff(rdbg->global_vmap, rdbg->global_vmap_count, voff);
      RADDBG_GlobalVariable *global_var = &rdbg->global_variables[global_idx];
      U64 name_size = 0;
      U8 *name_ptr = raddbg_string_from_idx(rdbg, global_var->name_string_idx, &name_size);
      result = push_str8_copy(arena, str8(name_ptr, name_size));
    }
    dbgi_scope_close(scope);
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
    DF_Entity *binary = df_binary_file_from_module(module);
    U64 voff = df_voff_from_vaddr(module, vaddr);
    result = df_symbol_name_from_binary_voff(arena, binary, voff);
  }
  return result;
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
  DBGI_Scope *scope = dbgi_scope_open();
  DF_EntityList binaries = df_push_active_binary_list(scratch.arena);
  DF_EntityList overrides = df_possible_overrides_from_entity(scratch.arena, file);
  for(DF_EntityNode *override_n = overrides.first;
      override_n != 0;
      override_n = override_n->next)
  {
    DF_Entity *override = override_n->entity;
    String8 file_path = df_full_path_from_entity(scratch.arena, override);
    String8 file_path_normalized = lower_from_str8(scratch.arena, file_path);
    for(DF_EntityNode *binary_n = binaries.first;
        binary_n != 0;
        binary_n = binary_n->next)
    {
      // rjf: binary -> rdbg
      DF_Entity *binary = binary_n->entity;
      String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
      DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
      RADDBG_Parsed *rdbg = &dbgi->rdbg;
      
      // rjf: file_path_normalized * rdbg -> src_id
      B32 good_src_id = 0;
      U32 src_id = 0;
      if(dbgi != &dbgi_parse_nil)
      {
        RADDBG_NameMap *mapptr = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_NormalSourcePaths);
        if(mapptr != 0)
        {
          RADDBG_ParsedNameMap map = {0};
          raddbg_name_map_parse(rdbg, mapptr, &map);
          RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, file_path_normalized.str, file_path_normalized.size);
          if(node != 0)
          {
            U32 id_count = 0;
            U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
            if(id_count > 0)
            {
              good_src_id = 1;
              src_id = ids[0];
            }
          }
        }
      }
      
      // rjf: good src-id -> look up line info for visible range
      if(good_src_id)
      {
        RADDBG_SourceFile *src = rdbg->source_files+src_id;
        RADDBG_ParsedLineMap line_map = {0};
        raddbg_line_map_from_source_file(rdbg, src, &line_map);
        U64 line_idx = 0;
        for(S64 line_num = line_num_range.min;
            line_num <= line_num_range.max;
            line_num += 1, line_idx += 1)
        {
          DF_TextLineSrc2DasmInfoList *src2dasm_list = &src2dasm_array.v[line_idx];
          U32 voff_count = 0;
          U64 *voffs = raddbg_line_voffs_from_num(&line_map, u32_from_u64_saturate((U64)line_num), &voff_count);
          for(U64 idx = 0; idx < voff_count; idx += 1)
          {
            U64 base_voff = voffs[idx];
            U64 unit_idx = raddbg_vmap_idx_from_voff(rdbg->unit_vmap, rdbg->unit_vmap_count, base_voff);
            RADDBG_Unit *unit = &rdbg->units[unit_idx];
            RADDBG_ParsedLineInfo unit_line_info = {0};
            raddbg_line_info_from_unit(rdbg, unit, &unit_line_info);
            U64 line_info_idx = raddbg_line_info_idx_from_voff(&unit_line_info, base_voff);
            if(unit_line_info.voffs != 0)
            {
              Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
              S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
              DF_TextLineSrc2DasmInfoNode *src2dasm_n = push_array(arena, DF_TextLineSrc2DasmInfoNode, 1);
              src2dasm_n->v.voff_range = range;
              src2dasm_n->v.remap_line = (S64)actual_line;
              src2dasm_n->v.binary = binary;
              SLLQueuePush(src2dasm_list->first, src2dasm_list->last, src2dasm_n);
              src2dasm_list->count += 1;
            }
          }
        }
      }
      
      // rjf: good src id -> push to relevant binaries
      if(good_src_id)
      {
        df_entity_list_push(arena, &src2dasm_array.binaries, binary);
      }
    }
  }
  dbgi_scope_close(scope);
  scratch_end(scratch);
  return src2dasm_array;
}

//- rjf: voff -> src lookups

internal DF_TextLineDasm2SrcInfo
df_text_line_dasm2src_info_from_binary_voff(DF_Entity *binary, U64 voff)
{
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  String8 path = df_full_path_from_entity(scratch.arena, binary);
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, path, 0);
  RADDBG_Parsed *rdbg = &dbgi->rdbg;
  DF_TextLineDasm2SrcInfo result = {0};
  result.file = result.binary = &df_g_nil_entity;
  if(rdbg->unit_vmap != 0 && rdbg->units != 0 && rdbg->source_files != 0)
  {
    U64 unit_idx = raddbg_vmap_idx_from_voff(rdbg->unit_vmap, rdbg->unit_vmap_count, voff);
    RADDBG_Unit *unit = &rdbg->units[unit_idx];
    RADDBG_ParsedLineInfo unit_line_info = {0};
    raddbg_line_info_from_unit(rdbg, unit, &unit_line_info);
    U64 line_info_idx = raddbg_line_info_idx_from_voff(&unit_line_info, voff);
    if(line_info_idx < unit_line_info.count)
    {
      RADDBG_Line *line = &unit_line_info.lines[line_info_idx];
      RADDBG_Column *column = (line_info_idx < unit_line_info.col_count) ? &unit_line_info.cols[line_info_idx] : 0;
      RADDBG_SourceFile *file = &rdbg->source_files[line->file_idx];
      String8 file_normalized_full_path = {0};
      file_normalized_full_path.str = raddbg_string_from_idx(rdbg, file->normal_full_path_string_idx, &file_normalized_full_path.size);
      result.binary = binary;
      if(line->file_idx != 0 && file_normalized_full_path.size != 0)
      {
        result.file = df_entity_from_path(file_normalized_full_path, DF_EntityFromPathFlag_All);
      }
      result.pt = txt_pt(line->line_num, column ? column->col_first : 1);
      result.voff_range = r1u64(unit_line_info.voffs[line_info_idx], unit_line_info.voffs[line_info_idx+1]);
    }
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  return result;
}

internal DF_TextLineDasm2SrcInfoList
df_text_line_dasm2src_info_from_voff(Arena *arena, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  DF_TextLineDasm2SrcInfoList result = {0};
  DF_EntityList binaries = df_push_active_binary_list(scratch.arena);
  for(DF_EntityNode *n = binaries.first; n != 0; n = n->next)
  {
    DF_TextLineDasm2SrcInfo info = df_text_line_dasm2src_info_from_binary_voff(n->entity, voff);
    if(!df_entity_is_nil(info.file))
    {
      DF_TextLineDasm2SrcInfoNode *dst_n = push_array(arena, DF_TextLineDasm2SrcInfoNode, 1);
      dst_n->v = info;
      SLLQueuePush(result.first, result.last, dst_n);
      result.count += 1;
    }
  }
  scratch_end(scratch);
  return result;
}

//- rjf: symbol -> voff lookups

internal U64
df_voff_from_binary_symbol_name(DF_Entity *binary, String8 symbol_name)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  U64 result = 0;
  {
    String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
    DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
    RADDBG_Parsed *rdbg = &dbgi->rdbg;
    RADDBG_NameMapKind name_map_kinds[] =
    {
      RADDBG_NameMapKind_GlobalVariables,
      RADDBG_NameMapKind_Procedures,
    };
    if(dbgi != &dbgi_parse_nil)
    {
      for(U64 name_map_kind_idx = 0;
          name_map_kind_idx < ArrayCount(name_map_kinds);
          name_map_kind_idx += 1)
      {
        RADDBG_NameMapKind name_map_kind = name_map_kinds[name_map_kind_idx];
        RADDBG_NameMap *name_map = raddbg_name_map_from_kind(rdbg, name_map_kind);
        RADDBG_ParsedNameMap parsed_name_map = {0};
        raddbg_name_map_parse(rdbg, name_map, &parsed_name_map);
        RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &parsed_name_map, symbol_name.str, symbol_name.size);
        
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
              U32 *run = raddbg_matches_from_map_node(rdbg, node, &num);
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
          case RADDBG_NameMapKind_GlobalVariables: if(entity_num <= rdbg->global_variable_count)
          {
            RADDBG_GlobalVariable *global_var = &rdbg->global_variables[entity_num-1];
            voff = global_var->voff;
          }break;
          case RADDBG_NameMapKind_Procedures: if(entity_num <= rdbg->procedure_count)
          {
            RADDBG_Procedure *procedure = &rdbg->procedures[entity_num-1];
            RADDBG_Scope *scope = &rdbg->scopes[procedure->root_scope_idx];
            voff = rdbg->scope_voffs[scope->voff_range_first];
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
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal U64
df_type_num_from_binary_name(DF_Entity *binary, String8 name)
{
  ProfBeginFunction();
  DBGI_Scope *scope = dbgi_scope_open();
  Temp scratch = scratch_begin(0, 0);
  U64 result = 0;
  {
    String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
    DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
    RADDBG_Parsed *rdbg = &dbgi->rdbg;
    RADDBG_NameMap *name_map = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_Types);
    RADDBG_ParsedNameMap parsed_name_map = {0};
    raddbg_name_map_parse(rdbg, name_map, &parsed_name_map);
    RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &parsed_name_map, name.str, name.size);
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
          U32 *run = raddbg_matches_from_map_node(rdbg, node, &num);
          if(num != 0)
          {
            entity_num = run[0]+1;
          }
        }break;
      }
    }
    result = entity_num;
  }
  scratch_end(scratch);
  dbgi_scope_close(scope);
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Process/Thread Info Lookups

//- rjf: thread info extraction helpers

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
df_tls_base_vaddr_from_thread(DF_Entity *thread)
{
  U64 base_vaddr = 0;
  Temp scratch = scratch_begin(0, 0);
  DBGI_Scope *scope = dbgi_scope_open();
  
  //- rjf: unpack thread info
  DF_Entity *module = df_module_from_thread(thread);
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  DF_Entity *binary = df_binary_file_from_module(module);
  DBGI_Parse *dbgi = df_dbgi_parse_from_binary_file(scope, binary);
  String8 bin_data = str8((U8 *)dbgi->exe_base, dbgi->exe_props.size);
  PE_BinInfo *bin = &dbgi->pe;
  B32 bin_is_pe = 1; // TODO(rjf): this path needs to change for ELF
  U64 addr_size = bit_size_from_arch(bin->arch)/8;
  
  //- rjf: grab tls range
  Rng1U64 tls_vaddr_range = pe_tls_rng_from_bin_base_vaddr(bin_data, bin, df_base_vaddr_from_module(module));
  
  //- rjf: read module's TLS index
  // TODO(allen): migrate all of this logic into DEMON
  U64 tls_index = 0;
  {
    U64 bytes_read = ctrl_process_read(process->ctrl_machine_id, process->ctrl_handle, tls_vaddr_range, &tls_index);
    if(bytes_read < sizeof(U64))
    {
      tls_index = 0;
    }
  }
  
  //- rjf: PE path
  if(bin_is_pe)
  {
    U64 thread_info_addr = ctrl_tls_root_vaddr_from_thread(thread->ctrl_machine_id, thread->ctrl_handle);
    U64 tls_addr_off = tls_index*addr_size;
    U64 tls_addr_array = 0;
    String8 tls_addr_array_data = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, r1u64(thread_info_addr, thread_info_addr+addr_size));
    if(tls_addr_array_data.size >= 8)
    {
      MemoryCopy(&tls_addr_array, tls_addr_array_data.str, sizeof(U64));
    }
    String8 result_data = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, r1u64(tls_addr_array + tls_addr_off, tls_addr_array + tls_addr_off + addr_size));
    if(result_data.size >= 8)
    {
      MemoryCopy(&base_vaddr, result_data.str, sizeof(U64));
    }
  }
  
  //- rjf: non-PE path (not implemented)
  if(!bin_is_pe)
  {
    // TODO(rjf): not supported. old code from the prototype that Nick had sketched out:
#if 0
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
#endif
  }
  
  dbgi_scope_close(scope);
  scratch_end(scratch);
  return base_vaddr;
}

internal Architecture
df_architecture_from_entity(DF_Entity *entity)
{
  return entity->arch;
}

internal DF_Unwind
df_push_unwind_from_thread(Arena *arena, DF_Entity *thread)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  DBGI_Scope *scope = dbgi_scope_open();
  Architecture arch = df_architecture_from_entity(thread);
  U64 arch_reg_block_size = regs_block_size_from_architecture(arch);
  DF_Unwind unwind = {0};
  unwind.error = 1;
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:
    {
      DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
      
      // rjf: grab initial register block
      void *regs_block = push_array(scratch.arena, U8, arch_reg_block_size);
      B32 regs_block_good = 0;
      {
        void *regs_raw = ctrl_reg_block_from_thread(thread->ctrl_machine_id, thread->ctrl_handle);
        if(regs_raw != 0)
        {
          MemoryCopy(regs_block, regs_raw, arch_reg_block_size);
          regs_block_good = 1;
        }
      }
      
      // rjf: grab initial memory view
      B32 stack_memview_good = 0;
      UNW_MemView stack_memview = {0};
      if(regs_block_good)
      {
        U64 stack_base_unrounded = thread->stack_base;
        U64 stack_top_unrounded = regs_rsp_from_arch_block(arch, regs_block);
        U64 stack_base = AlignPow2(stack_base_unrounded, KB(4));
        U64 stack_top = AlignDownPow2(stack_top_unrounded, KB(4));
        U64 stack_size = stack_base - stack_top;
        if(stack_base >= stack_top)
        {
          String8 stack_memory = {0};
          stack_memory.str = push_array_no_zero(scratch.arena, U8, stack_size);
          stack_memory.size = ctrl_process_read(process->ctrl_machine_id, process->ctrl_handle, r1u64(stack_top, stack_top+stack_size), stack_memory.str);
          if(stack_memory.size != 0)
          {
            stack_memview_good = 1;
            stack_memview.data = stack_memory.str;
            stack_memview.addr_first = stack_top;
            stack_memview.addr_opl = stack_base;
          }
        }
      }
      
      // rjf: loop & unwind
      UNW_MemView memview = stack_memview;
      if(stack_memview_good) for(;;)
      {
        unwind.error = 0;
        
        // rjf: regs -> rip*module*binary
        U64 rip = regs_rip_from_arch_block(arch, regs_block);
        DF_Entity *module = df_module_from_process_vaddr(process, rip);
        DF_Entity *binary = df_binary_file_from_module(module);
        
        // rjf: cancel on 0 rip
        if(rip == 0)
        {
          break;
        }
        
        // rjf: binary -> all the binary info
        String8 binary_full_path = df_full_path_from_entity(scratch.arena, binary);
        DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_full_path, 0);
        String8 binary_data = str8((U8 *)dbgi->exe_base, dbgi->exe_props.size);
        
        // rjf: valid step -> push frame
        DF_UnwindFrame *frame = push_array(arena, DF_UnwindFrame, 1);
        frame->rip = rip;
        frame->regs = push_array_no_zero(arena, U8, arch_reg_block_size);
        MemoryCopy(frame->regs, regs_block, arch_reg_block_size);
        SLLQueuePush(unwind.first, unwind.last, frame);
        unwind.count += 1;
        
        // rjf: unwind one step
        UNW_Result unwind_step = unw_pe_x64(binary_data, &dbgi->pe, df_base_vaddr_from_module(module), &memview, (UNW_X64_Regs *)regs_block);
        
        // rjf: cancel on bad step
        if(unwind_step.dead != 0)
        {
          break;
        }
        if(unwind_step.missed_read != 0)
        {
          unwind.error = 1;
          break;
        }
        if(unwind_step.stack_pointer == 0)
        {
          break;
        }
      }
    }break;
  }
  dbgi_scope_close(scope);
  scratch_end(scratch);
  ProfEnd();
  return unwind;
}

internal U64
df_rip_from_thread(DF_Entity *thread)
{
  U64 result = ctrl_rip_from_thread(thread->ctrl_machine_id, thread->ctrl_handle);
  return result;
}

internal U64
df_rip_from_thread_unwind(DF_Entity *thread, U64 unwind_count)
{
  Temp scratch = scratch_begin(0, 0);
  U64 result = df_rip_from_thread(thread);
  if(unwind_count != 0)
  {
    DF_Unwind unwind = df_push_unwind_from_thread(scratch.arena, thread);
    U64 unwind_idx = 0;
    for(DF_UnwindFrame *frame = unwind.first; frame != 0; frame = frame->next, unwind_idx += 1)
    {
      if(unwind_count == unwind_idx)
      {
        result = frame->rip;
        break;
      }
    }
  }
  scratch_end(scratch);
  return result;
}

internal EVAL_String2NumMap *
df_push_locals_map_from_binary_voff(Arena *arena, DBGI_Scope *scope, DF_Entity *binary, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
  RADDBG_Parsed *rdbg = &dbgi->rdbg;
  EVAL_String2NumMap *result = eval_push_locals_map_from_raddbg_voff(arena, rdbg, voff);
  scratch_end(scratch);
  return result;
}

internal EVAL_String2NumMap *
df_push_member_map_from_binary_voff(Arena *arena, DBGI_Scope *scope, DF_Entity *binary, U64 voff)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
  RADDBG_Parsed *rdbg = &dbgi->rdbg;
  EVAL_String2NumMap *result = eval_push_member_map_from_raddbg_voff(arena, rdbg, voff);
  scratch_end(scratch);
  return result;
}

internal B32
df_set_thread_rip(DF_Entity *thread, U64 vaddr)
{
  B32 result = ctrl_thread_write_rip(thread->ctrl_machine_id, thread->ctrl_handle, vaddr);
  
  // rjf: early mutation of unwind cache for immediate frontend effect
  if(result)
  {
    DF_RunUnwindCache *unwind_cache = &df_state->unwind_cache;
    DF_Handle thread_handle = df_handle_from_entity(thread);
    U64 hash = df_hash_from_string(str8_struct(&thread_handle));
    U64 slot_idx = hash % unwind_cache->table_size;
    DF_RunUnwindCacheSlot *slot = &unwind_cache->table[slot_idx];
    for(DF_RunUnwindCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(df_handle_match(n->thread, thread_handle) && n->unwind.first != 0)
      {
        n->unwind.first->rip = vaddr;
        break;
      }
    }
  }
  
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

////////////////////////////////
//~ rjf: Entity -> Log Entities

internal DF_Entity *
df_log_from_entity(DF_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  String8 log_name = {0};
  switch(entity->kind)
  {
    default:
    {
      log_name = push_str8f(scratch.arena, "id_%I64u", entity->id);
    }break;
    case DF_EntityKind_Root:
    {
      U32 session_pid = os_get_pid();
      log_name = push_str8f(scratch.arena, "session_%i", session_pid);
    }break;
    case DF_EntityKind_Machine:
    {
      log_name = push_str8f(scratch.arena, "machine_%I64u", entity->id);
    }break;
    case DF_EntityKind_Process:
    {
      log_name = push_str8f(scratch.arena, "pid_%i", entity->ctrl_id);
    }break;
    case DF_EntityKind_Thread:
    {
      log_name = push_str8f(scratch.arena, "tid_%i", entity->ctrl_id);
    }break;
  }
  String8 user_program_data_path = os_string_from_system_path(scratch.arena, OS_SystemPath_UserProgramData);
  String8 user_data_folder = push_str8f(scratch.arena, "%S/%S", user_program_data_path, str8_lit("raddbg/logs"));
  String8 log_path = push_str8f(scratch.arena, "%S/log%s%S.txt", user_data_folder, log_name.size != 0 ? "_" : "", log_name);
  DF_Entity *log = df_entity_from_path(log_path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
  log->flags |= DF_EntityFlag_Output;
  scratch_end(scratch);
  return log;
}

////////////////////////////////
//~ rjf: Target Controls

//- rjf: control message dispatching

internal void
df_push_ctrl_msg(CTRL_Msg *msg)
{
  CTRL_Msg *dst = ctrl_msg_list_push(df_state->ctrl_msg_arena, &df_state->ctrl_msgs);
  ctrl_msg_deep_copy(df_state->ctrl_msg_arena, dst, msg);
  if(dst->kind == CTRL_MsgKind_LaunchAndInit)
  {
    df_state->ctrl_is_running = 1;
  }
  if(df_state->ctrl_soft_halt_issued == 0 && df_ctrl_targets_running())
  {
    df_state->ctrl_soft_halt_issued = 1;
    ctrl_halt();
  }
}

//- rjf: control thread running

internal void
df_ctrl_run(DF_RunKind run, DF_Entity *run_thread, CTRL_TrapList *run_traps)
{
  DBGI_Scope *scope = dbgi_scope_open();
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: build run message
  CTRL_Msg msg = {(run == DF_RunKind_Run || run == DF_RunKind_Step) ? CTRL_MsgKind_Run : CTRL_MsgKind_SingleStep};
  {
    DF_EntityList user_bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
    DF_Entity *process = df_entity_ancestor_from_kind(run_thread, DF_EntityKind_Process);
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
            ctrl_user_bp_u64 = user_bp->u64;
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
    if(df_state->ctrl_solo_stepping_mode && !df_entity_is_nil(run_thread))
    {
      msg.freeze_state_is_frozen = 0;
      CTRL_MachineIDHandlePair pair = {run_thread->ctrl_machine_id, run_thread->ctrl_handle};
      ctrl_machine_id_handle_pair_list_push(scratch.arena, &msg.freeze_state_threads, &pair);
    }
    else
    {
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
  df_state->ctrl_last_run_traps = ctrl_trap_list_copy(df_state->ctrl_last_run_arena, &run_traps_copy);
  df_state->ctrl_is_running = 1;
  
  scratch_end(scratch);
  dbgi_scope_close(scope);
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
  String8 data = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, r1u64(addr, addr+size));
  if(data.size == size)
  {
    result = 1;
    MemoryCopy(out, data.str, data.size);
  }
  scratch_end(scratch);
  return result;
}

internal EVAL_ParseCtx
df_eval_parse_ctx_from_process_vaddr(DBGI_Scope *scope, DF_Entity *process, U64 vaddr)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: extract info
  DF_Entity *module = df_module_from_process_vaddr(process, vaddr);
  U64 voff = df_voff_from_vaddr(module, vaddr);
  DF_Entity *binary = df_binary_file_from_module(module);
  String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
  DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
  RADDBG_Parsed *rdbg = &dbgi->rdbg;
  Architecture arch = df_architecture_from_entity(process);
  EVAL_String2NumMap *reg_map = ctrl_string2reg_from_arch(arch);
  EVAL_String2NumMap *reg_alias_map = ctrl_string2alias_from_arch(arch);
  EVAL_String2NumMap *locals_map = df_query_cached_locals_map_from_binary_voff(binary, voff);
  EVAL_String2NumMap *member_map = df_query_cached_member_map_from_binary_voff(binary, voff);
  
  //- rjf: build ctx
  EVAL_ParseCtx ctx = zero_struct;
  {
    ctx.arch            = arch;
    ctx.ip_voff         = voff;
    ctx.rdbg            = rdbg;
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
df_eval_parse_ctx_from_src_loc(DBGI_Scope *scope, DF_Entity *file, TxtPt pt)
{
  Temp scratch = scratch_begin(0, 0);
  EVAL_ParseCtx ctx = zero_struct;
  DF_EntityList binaries = df_push_active_binary_list(scratch.arena);
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
    for(DF_EntityNode *binary_n = binaries.first;
        binary_n != 0;
        binary_n = binary_n->next)
    {
      // rjf: binary -> rdbg
      DF_Entity *binary = binary_n->entity;
      String8 binary_path = df_full_path_from_entity(scratch.arena, binary);
      DBGI_Parse *dbgi = dbgi_parse_from_exe_path(scope, binary_path, 0);
      RADDBG_Parsed *rdbg = &dbgi->rdbg;
      
      // rjf: file_path_normalized * rdbg -> src_id
      B32 good_src_id = 0;
      U32 src_id = 0;
      {
        RADDBG_NameMap *mapptr = raddbg_name_map_from_kind(rdbg, RADDBG_NameMapKind_NormalSourcePaths);
        if(mapptr != 0)
        {
          RADDBG_ParsedNameMap map = {0};
          raddbg_name_map_parse(rdbg, mapptr, &map);
          RADDBG_NameMapNode *node = raddbg_name_map_lookup(rdbg, &map, file_path_normalized.str, file_path_normalized.size);
          if(node != 0)
          {
            U32 id_count = 0;
            U32 *ids = raddbg_matches_from_map_node(rdbg, node, &id_count);
            if(id_count > 0)
            {
              good_src_id = 1;
              src_id = ids[0];
            }
          }
        }
      }
      
      // rjf: good src-id -> look up line info for visible range
      if(good_src_id)
      {
        RADDBG_SourceFile *src = rdbg->source_files+src_id;
        RADDBG_ParsedLineMap line_map = {0};
        raddbg_line_map_from_source_file(rdbg, src, &line_map);
        U32 voff_count = 0;
        U64 *voffs = raddbg_line_voffs_from_num(&line_map, (U32)pt.line, &voff_count);
        for(U64 idx = 0; idx < voff_count; idx += 1)
        {
          U64 base_voff = voffs[idx];
          U64 unit_idx = raddbg_vmap_idx_from_voff(rdbg->unit_vmap, rdbg->unit_vmap_count, base_voff);
          RADDBG_Unit *unit = &rdbg->units[unit_idx];
          RADDBG_ParsedLineInfo unit_line_info = {0};
          raddbg_line_info_from_unit(rdbg, unit, &unit_line_info);
          U64 line_info_idx = raddbg_line_info_idx_from_voff(&unit_line_info, base_voff);
          Rng1U64 range = r1u64(base_voff, unit_line_info.voffs[line_info_idx+1]);
          S64 actual_line = (S64)unit_line_info.lines[line_info_idx].line_num;
          DF_TextLineSrc2DasmInfoNode *src2dasm_n = push_array(scratch.arena, DF_TextLineSrc2DasmInfoNode, 1);
          src2dasm_n->v.voff_range = range;
          src2dasm_n->v.remap_line = (S64)actual_line;
          src2dasm_n->v.binary = binary;
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
      DF_EntityList modules = df_modules_from_binary_file(scratch.arena, src2dasm->binary);
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
    ctx.rdbg            = &dbgi_parse_nil.rdbg;
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
df_eval_from_string(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, String8 string)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: unpack arguments
  DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *process = thread->parent;
  U64 unwind_count = ctrl_ctx->unwind_count;
  DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
  Architecture arch = df_architecture_from_entity(thread);
  U64 reg_size = regs_block_size_from_architecture(arch);
  U64 thread_unwind_ip_vaddr = 0;
  void *thread_unwind_regs_block = push_array(scratch.arena, U8, reg_size);
  {
    U64 idx = 0;
    for(DF_UnwindFrame *f = unwind.first; f != 0; f = f->next, idx += 1)
    {
      if(idx == unwind_count)
      {
        thread_unwind_ip_vaddr = f->rip;
        thread_unwind_regs_block = f->regs;
        break;
      }
    }
  }
  
  //- rjf: unpack module info & produce eval machine
  DF_Entity *module = df_module_from_process_vaddr(process, thread_unwind_ip_vaddr);
  U64 module_base = df_base_vaddr_from_module(module);
  U64 tls_base = df_tls_base_vaddr_from_thread(thread);
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
    ir_tree_and_type = eval_irtree_and_type_from_expr(arena, parse_ctx->type_graph, parse_ctx->rdbg, parse.expr, &errors);
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
  }
  
  //- rjf: apply dynamic type overrides
  if(parse.expr != 0 && parse.expr->kind != EVAL_ExprKind_Cast)
  {
    result = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdbg, ctrl_ctx, result);
  }
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal DF_Eval
df_value_mode_eval_from_eval(TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, DF_Eval eval)
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
      U64 type_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, type_key);
      if(!tg_key_match(type_key, tg_key_zero()) && type_byte_size <= 8)
      {
        Temp scratch = scratch_begin(0, 0);
        Rng1U64 vaddr_range = r1u64(eval.offset, eval.offset + type_byte_size);
        if(dim_1u64(vaddr_range) == type_byte_size)
        {
          String8 data = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, vaddr_range);
          MemoryZeroArray(eval.imm_u128);
          MemoryCopy(eval.imm_u128, data.str, Min(data.size, sizeof(U64)*2));
          eval.mode = EVAL_EvalMode_Value;
          
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
      U64 type_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, type_key);
      U64 reg_off = eval.offset;
      DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
      if(unwind.first != 0)
      {
        U64 unwind_idx = 0;
        for(DF_UnwindFrame *frame = unwind.first; frame != 0; frame = frame->next, unwind_idx += 1)
        {
          if(unwind_idx == ctrl_ctx->unwind_count && frame->regs != 0)
          {
            MemoryCopy(&eval.imm_u128[0], ((U8 *)frame->regs + reg_off), Min(type_byte_size, sizeof(U64)*2));
            break;
          }
        }
      }
      eval.mode = EVAL_EvalMode_Value;
    }break;
  }
  
  ProfEnd();
  return eval;
}

internal DF_Eval
df_dynamically_typed_eval_from_eval(TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, DF_Eval eval)
{
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
    TG_Key ptee_type_key = tg_unwrapped_direct_from_graph_raddbg_key(graph, rdbg, type_key);
    TG_Kind ptee_type_kind = tg_kind_from_key(ptee_type_key);
    if(ptee_type_kind == TG_Kind_Struct || ptee_type_kind == TG_Kind_Class)
    {
      TG_Type *ptee_type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, ptee_type_key);
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
        String8 ptr_value_memory = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle,
                                                                                   r1u64(ptr_vaddr, ptr_vaddr+addr_size));
        if(ptr_value_memory.size >= addr_size)
        {
          U64 class_base_vaddr = 0;
          MemoryCopy(&class_base_vaddr, ptr_value_memory.str, addr_size);
          String8 vtable_base_ptr_memory = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle,
                                                                                           r1u64(class_base_vaddr, class_base_vaddr+addr_size));
          if(vtable_base_ptr_memory.size >= addr_size)
          {
            U64 vtable_vaddr = 0;
            MemoryCopy(&vtable_vaddr, vtable_base_ptr_memory.str, addr_size);
            U64 vtable_voff = df_voff_from_vaddr(module, vtable_vaddr);
            U64 global_idx = raddbg_vmap_idx_from_voff(rdbg->global_vmap, rdbg->global_vmap_count, vtable_voff);
            if(0 < global_idx && global_idx < rdbg->global_variable_count)
            {
              RADDBG_GlobalVariable *global_var = &rdbg->global_variables[global_idx];
              if(global_var->link_flags & RADDBG_LinkFlag_TypeScoped &&
                 0 < global_var->container_idx && global_var->container_idx < rdbg->udt_count)
              {
                RADDBG_UDT *udt = &rdbg->udts[global_var->container_idx];
                if(0 < udt->self_type_idx && udt->self_type_idx < rdbg->type_node_count)
                {
                  RADDBG_TypeNode *type = &rdbg->type_nodes[udt->self_type_idx];
                  TG_Key derived_type_key = tg_key_ext(tg_kind_from_raddbg_type_kind(type->kind), (U64)udt->self_type_idx);
                  TG_Key ptr_to_derived_type_key = tg_cons_type_make(graph, TG_Kind_Ptr, derived_type_key, 0);
                  eval.type_key = ptr_to_derived_type_key;
                }
              }
            }
          }
        }
      }
    }
  }
  scratch_end(scratch);
  return eval;
}

internal DF_Eval
df_eval_from_eval_cfg_table(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_Eval eval, DF_CfgTable *cfg)
{
  ProfBeginFunction();
  
  //- rjf: apply view rules
  for(DF_CfgVal *val = cfg->first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
  {
    DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
    if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_EvalResolution)
    {
      eval = spec->info.eval_resolution(arena, scope, ctrl_ctx, parse_ctx, eval, val);
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
      eval_view->history_cache_table_size = 64;
      eval_view->history_cache_table = push_array(eval_view->arena, DF_EvalHistoryCacheSlot, eval_view->history_cache_table_size);
      eval_view->view_rule_table.slot_count = 64;
      eval_view->view_rule_table.slots = push_array(eval_view->arena, DF_EvalViewRuleCacheSlot, eval_view->view_rule_table.slot_count);
    }
  }
  return eval_view;
}

//- rjf: key -> eval history

internal DF_EvalHistoryCacheNode *
df_eval_history_cache_node_from_key(DF_EvalView *eval_view, DF_ExpandKey key)
{
  //- rjf: key -> hash * slot idx * slot
  String8 key_string = str8_struct(&key);
  U64 hash = df_hash_from_string(key_string);
  U64 slot_idx = hash%eval_view->history_cache_table_size;
  DF_EvalHistoryCacheSlot *slot = &eval_view->history_cache_table[slot_idx];
  
  //- rjf: slot idx -> existing node
  DF_EvalHistoryCacheNode *existing_node = 0;
  for(DF_EvalHistoryCacheNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(df_expand_key_match(n->key, key))
    {
      existing_node = n;
      break;
    }
  }
  
  return existing_node;
}

internal B32
df_eval_view_record_history_val(DF_EvalView *eval_view, DF_ExpandKey key, DF_EvalHistoryVal val)
{
  B32 change = 0;
  
  //- rjf: key -> hash * slot idx * slot
  String8 key_string = str8_struct(&key);
  U64 hash = df_hash_from_string(key_string);
  U64 slot_idx = hash%eval_view->history_cache_table_size;
  DF_EvalHistoryCacheSlot *slot = &eval_view->history_cache_table[slot_idx];
  
  //- rjf: slot idx -> existing node
  DF_EvalHistoryCacheNode *existing_node = 0;
  for(DF_EvalHistoryCacheNode *n = slot->first; n != 0; n = n->hash_next)
  {
    if(df_expand_key_match(n->key, key))
    {
      existing_node = n;
      break;
    }
  }
  
  //- rjf: grab existing node - if there is none, we need to allocate one
  DF_EvalHistoryCacheNode *node = existing_node;
  if(node == 0)
  {
    // TODO(rjf): check lru cache, allocate from there, prevent too much growth
    node = push_array(eval_view->arena, DF_EvalHistoryCacheNode, 1);
    DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);
    node->key = key;
    node->first_run_idx = node->last_run_idx = df_ctrl_run_gen();
  }
  
  //- rjf: record value
  if(node != 0)
  {
    DF_EvalHistoryVal *newest_val = &node->values[node->newest_val_idx];
    if(newest_val->mode != val.mode ||
       newest_val->imm_u128[0] != val.imm_u128[0] ||
       newest_val->imm_u128[1] != val.imm_u128[1] ||
       newest_val->offset != val.offset)
    {
      change = 1;
      node->newest_val_idx = (node->newest_val_idx + 1) % ArrayCount(node->values);
      node->values[node->newest_val_idx] = val;
      node->last_run_idx = df_ctrl_run_gen();
    }
  }
  
  return change;
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
    {
      result = push_str8f(arena, "%c", val);
    }break;
  }
  return result;
}

internal String8
df_string_from_simple_typed_eval(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, DF_EvalVizStringFlags flags, U32 radix, DF_Eval eval)
{
  ProfBeginFunction();
  String8 result = {0};
  TG_Key type_key = tg_unwrapped_from_graph_raddbg_key(graph, rdbg, eval.type_key);
  TG_Kind type_kind = tg_kind_from_key(type_key);
  U64 type_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, type_key);
  U8 digit_group_separator = 0;
  if(!(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules))
  {
    digit_group_separator = 0;
  }
  switch(type_kind)
  {
    default:{}break;
    
    case TG_Kind_Char8:
    case TG_Kind_Char16:
    case TG_Kind_Char32:
    case TG_Kind_UChar8:
    case TG_Kind_UChar16:
    case TG_Kind_UChar32:
    {
      String8 char_str = df_string_from_ascii_value(arena, eval.imm_s64);
      if(flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules)
      {
        String8 imm_string = str8_from_s64(arena, eval.imm_s64, radix, 0, digit_group_separator);
        result = push_str8f(arena, "'%S' (%S)", char_str, imm_string);
      }
      else
      {
        result = push_str8f(arena, "'%S'", char_str);
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
      TG_Type *type = tg_type_from_graph_raddbg_key(scratch.arena, graph, rdbg, type_key);
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
df_commit_eval_value(TG_Graph *graph, RADDBG_Parsed *rdbg, DF_CtrlCtx *ctrl_ctx, DF_Eval dst_eval, DF_Eval src_eval)
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
  U64 dst_type_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, dst_type_key);
  U64 src_type_byte_size = tg_byte_size_from_graph_raddbg_key(graph, rdbg, src_type_key);
  
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
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdbg, ctrl_ctx, src_eval);
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
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdbg, ctrl_ctx, src_eval);
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
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdbg, ctrl_ctx, src_eval);
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
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdbg, ctrl_ctx, src_eval);
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
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdbg, ctrl_ctx, src_eval);
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
        ctrl_process_write_data(process->ctrl_machine_id, process->ctrl_handle, dst_eval.offset, commit_data);
      }break;
      case EVAL_EvalMode_Reg:
      {
        DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
        Architecture arch = df_architecture_from_entity(thread);
        U64 reg_block_size = regs_block_size_from_architecture(arch);
        if(unwind.first != 0 &&
           (0 <= dst_eval.offset && dst_eval.offset+commit_data.size < reg_block_size))
        {
          void *new_regs = push_array(scratch.arena, U8, reg_block_size);
          MemoryCopy(new_regs, unwind.first->regs, reg_block_size);
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
df_eval_link_base_chunk_list_from_eval(Arena *arena, TG_Graph *graph, RADDBG_Parsed *rdbg, TG_Key link_member_type_key, U64 link_member_off, DF_CtrlCtx *ctrl_ctx, DF_Eval eval, U64 cap)
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
    DF_Eval link_member_value_eval = df_value_mode_eval_from_eval(graph, rdbg, ctrl_ctx, link_member_eval);
    
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

//- rjf: watch tree visualization

internal void
df_append_viz_blocks_for_parent__rec(Arena *arena, DBGI_Scope *scope, DF_EvalView *eval_view, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_ExpandKey parent_key, DF_ExpandKey key, String8 string, DF_Eval eval, DF_CfgTable *cfg_table, S32 depth, DF_EvalVizBlockList *list_out)
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
  eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdbg, ctrl_ctx, eval);
  eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, eval, cfg_table);
  
  //////////////////////////////
  //- rjf: unpack eval
  //
  TG_Key eval_type_key = tg_unwrapped_from_graph_raddbg_key(parse_ctx->type_graph, parse_ctx->rdbg, eval.type_key);
  TG_Kind eval_type_kind = tg_kind_from_key(eval_type_key);
  
  //////////////////////////////
  //- rjf: make and push block for root
  //
  DF_EvalVizBlock *block = push_array(arena, DF_EvalVizBlock, 1);
  {
    block->kind                        = DF_EvalVizBlockKind_Root;
    block->eval_view                   = eval_view;
    block->eval                        = eval;
    block->cfg_table                   = *cfg_table;
    block->string                      = push_str8_copy(arena, string);
    block->parent_key                  = parent_key;
    block->key                         = key;
    block->visual_idx_range            = r1u64(key.child_num-1, key.child_num+0);
    block->semantic_idx_range          = r1u64(key.child_num-1, key.child_num+0);
    block->depth                       = depth;
    SLLQueuePush(list_out->first, list_out->last, block);
    list_out->count += 1;
    list_out->total_visual_row_count += 1;
    list_out->total_semantic_row_count += 1;
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
    TG_Key direct_type_key = tg_ptee_from_graph_raddbg_key(parse_ctx->type_graph, parse_ctx->rdbg, eval_type_key);
    TG_Kind direct_type_kind = tg_kind_from_key(direct_type_key);
    DF_Eval ptr_val_eval = df_value_mode_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdbg, ctrl_ctx, block->eval);
    
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
    expand_view_rule_spec->info.viz_block_prod(arena, scope, ctrl_ctx, parse_ctx, eval_view, eval, cfg_table, parent_key, key, depth+1, expand_view_rule_cfg->last, list_out);
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
    // rjf: get members
    TG_MemberArray data_members = tg_data_members_from_graph_raddbg_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdbg, udt_eval.type_key);
    TG_MemberArray filtered_data_members = df_filtered_data_members_from_members_cfg_table(scratch.arena, data_members, cfg_table);
    
    // rjf: make block for all members (assume no members are expanded)
    DF_EvalVizBlock *memblock = push_array(arena, DF_EvalVizBlock, 1);
    {
      memblock->kind                   = DF_EvalVizBlockKind_Members;
      memblock->eval_view              = eval_view;
      memblock->eval                   = udt_eval;
      memblock->cfg_table              = *cfg_table;
      memblock->parent_key             = key;
      memblock->key                    = df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 0);
      memblock->visual_idx_range       = r1u64(0, filtered_data_members.count);
      memblock->semantic_idx_range     = r1u64(0, filtered_data_members.count);
      memblock->depth                  = depth+1;
      SLLQueuePush(list_out->first, list_out->last, memblock);
      list_out->count += 1;
      list_out->total_visual_row_count += filtered_data_members.count;
      list_out->total_semantic_row_count += filtered_data_members.count;
    }
    
    // rjf: split memblock by sub-expansions
    for(DF_ExpandNode *child = node->first; child != 0; child = child->next)
    {
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      if(child_idx >= filtered_data_members.count)
      {
        continue;
      }
      
      // rjf: truncate existing memblock
      memblock->visual_idx_range.max = child_idx;
      memblock->semantic_idx_range.max = child_idx;
      
      // rjf: build inheriting cfg table
      DF_CfgTable child_cfg = *cfg_table;
      {
        String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), child_num));
        child_cfg = df_cfg_table_from_inheritance(arena, cfg_table);
        if(view_rule_string.size != 0)
        {
          df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
        }
      }
      
      // rjf: recurse for sub-block
      {
        TG_Member *member = &filtered_data_members.v[child_idx];
        DF_Eval child_eval = zero_struct;
        {
          child_eval.type_key = member->type_key;
          child_eval.mode = udt_eval.mode;
          child_eval.offset = udt_eval.offset + member->off;
        }
        list_out->total_visual_row_count -= 1;
        list_out->total_semantic_row_count -= 1;
        df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, key, child->key, member->name, child_eval, &child_cfg, depth+1, list_out);
      }
      
      // rjf: make new memblock for remainder of children (if any)
      if(child_idx+1 < filtered_data_members.count)
      {
        DF_EvalVizBlock *next_memblock   = push_array(arena, DF_EvalVizBlock, 1);
        next_memblock->kind              = DF_EvalVizBlockKind_Members;
        next_memblock->eval_view         = eval_view;
        next_memblock->eval              = udt_eval;
        next_memblock->cfg_table         = *cfg_table;
        next_memblock->parent_key        = key;
        next_memblock->key               = df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 0);
        next_memblock->visual_idx_range  = r1u64(child_idx+1, filtered_data_members.count);
        next_memblock->semantic_idx_range= r1u64(child_idx+1, filtered_data_members.count);
        next_memblock->depth             = depth+1;
        SLLQueuePush(list_out->first, list_out->last, next_memblock);
        list_out->count += 1;
        memblock = next_memblock;
      }
    }
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
    // rjf: get members
    TG_MemberArray data_members = tg_data_members_from_graph_raddbg_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdbg, udt_eval.type_key);
    TG_MemberArray filtered_data_members = df_filtered_data_members_from_members_cfg_table(scratch.arena, data_members, cfg_table);
    
    // rjf: find link member
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
        link_member_ptee_type_key = tg_ptee_from_graph_raddbg_key(parse_ctx->type_graph, parse_ctx->rdbg, link_member->type_key);
        break;
      }
    }
    
    // rjf: invalid link member -> early-out!
    if(link_member == 0 ||
       link_member_type_kind != TG_Kind_Ptr ||
       !tg_key_match(link_member_ptee_type_key, udt_eval.type_key))
    {
      goto end_linked_struct_expansion_build;
    }
    
    // rjf: gather link bases
    DF_EvalLinkBaseChunkList link_bases = df_eval_link_base_chunk_list_from_eval(scratch.arena, parse_ctx->type_graph, parse_ctx->rdbg, link_member->type_key, link_member->off, ctrl_ctx, udt_eval, 512);
    
    // rjf: make block for all links (assume no members are expanded)
    DF_EvalVizBlock *linkblock = push_array(arena, DF_EvalVizBlock, 1);
    {
      linkblock->kind                 = DF_EvalVizBlockKind_Links;
      linkblock->eval_view            = eval_view;
      linkblock->eval                 = udt_eval;
      linkblock->link_member_type_key = link_member->type_key;
      linkblock->link_member_off      = link_member->off;
      linkblock->cfg_table            = *cfg_table;
      linkblock->parent_key           = key;
      linkblock->key                  = df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 0);
      linkblock->visual_idx_range     = r1u64(0, link_bases.count);
      linkblock->semantic_idx_range   = r1u64(0, link_bases.count);
      linkblock->depth                = depth+1;
      SLLQueuePush(list_out->first, list_out->last, linkblock);
      list_out->count += 1;
      list_out->total_visual_row_count += link_bases.count;
      list_out->total_semantic_row_count += link_bases.count;
    }
    
    // rjf: split linkblock by sub-expansions
    for(DF_ExpandNode *child = node->first; child != 0; child = child->next)
    {
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      if(child_idx >= link_bases.count)
      {
        continue;
      }
      
      // rjf: truncate existing elemblock
      linkblock->visual_idx_range.max = child_idx;
      linkblock->semantic_idx_range.max = child_idx;
      
      // rjf: build inheriting cfg table
      DF_CfgTable child_cfg = *cfg_table;
      {
        String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), child_num));
        child_cfg = df_cfg_table_from_inheritance(arena, cfg_table);
        if(view_rule_string.size != 0)
        {
          df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
        }
      }
      
      // rjf: find mode/offset of this link
      DF_EvalLinkBase link_base = df_eval_link_base_from_chunk_list_index(&link_bases, child_idx);
      
      // rjf: recurse for sub-block
      DF_Eval child_eval = zero_struct;
      {
        child_eval.type_key = udt_eval.type_key;
        child_eval.mode     = link_base.mode;
        child_eval.offset   = link_base.offset;
      }
      list_out->total_visual_row_count -= 1;
      list_out->total_semantic_row_count -= 1;
      df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, key, child->key, push_str8f(arena, "[%I64u]", child_idx), child_eval, &child_cfg, depth+1, list_out);
      
      // rjf: make new elemblock for remainder of children (if any)
      if(child_idx+1 < link_bases.count)
      {
        DF_EvalVizBlock *next_linkblock = push_array(arena, DF_EvalVizBlock, 1);
        next_linkblock->kind                 = DF_EvalVizBlockKind_Links;
        next_linkblock->eval_view            = eval_view;
        next_linkblock->eval                 = udt_eval;
        next_linkblock->link_member_type_key = link_member->type_key;
        next_linkblock->link_member_off      = link_member->off;
        next_linkblock->cfg_table            = *cfg_table;
        next_linkblock->parent_key           = key;
        next_linkblock->key                  = df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 0);
        next_linkblock->visual_idx_range     = r1u64(child_idx+1, link_bases.count);
        next_linkblock->semantic_idx_range   = r1u64(child_idx+1, link_bases.count);
        next_linkblock->depth                = depth+1;
        SLLQueuePush(list_out->first, list_out->last, next_linkblock);
        list_out->count += 1;
        linkblock = next_linkblock;
      }
    }
  }
  end_linked_struct_expansion_build:;
  
  //////////////////////////////
  //- rjf: (arrays) descend to elements & make block(s), normally
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_Default &&
     arr_type_kind == TG_Kind_Array)
    ProfScope("(arrays) descend to elements & make block(s)")
  {
    TG_Type *array_type = tg_type_from_graph_raddbg_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdbg, arr_eval.type_key);
    U64 array_count = array_type->count;
    TG_Key element_type_key = array_type->direct_type_key;
    U64 element_type_byte_size = tg_byte_size_from_graph_raddbg_key(parse_ctx->type_graph, parse_ctx->rdbg, element_type_key);
    
    // rjf: make block for all elements (assume no elements are expanded)
    DF_EvalVizBlock *elemblock = push_array(arena, DF_EvalVizBlock, 1);
    {
      elemblock->kind                  = DF_EvalVizBlockKind_Elements;
      elemblock->eval_view             = eval_view;
      elemblock->eval                  = arr_eval;
      elemblock->cfg_table             = *cfg_table;
      elemblock->parent_key            = key;
      elemblock->key                   = df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 0);
      elemblock->visual_idx_range      = r1u64(0, array_count);
      elemblock->semantic_idx_range    = r1u64(0, array_count);
      elemblock->depth                 = depth+1;
      SLLQueuePush(list_out->first, list_out->last, elemblock);
      list_out->count += 1;
      list_out->total_visual_row_count += array_count;
      list_out->total_semantic_row_count += array_count;
    }
    
    // rjf: split elemblock by sub-expansions
    for(DF_ExpandNode *child = node->first; child != 0; child = child->next)
    {
      U64 child_num = child->key.child_num;
      U64 child_idx = child_num-1;
      if(child_idx >= array_count)
      {
        continue;
      }
      
      // rjf: truncate existing elemblock
      elemblock->visual_idx_range.max = child_idx;
      elemblock->semantic_idx_range.max = child_idx;
      
      // rjf: build inheriting cfg table
      DF_CfgTable child_cfg = *cfg_table;
      {
        String8 view_rule_string = df_eval_view_rule_from_key(eval_view, df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), child_num));
        child_cfg = df_cfg_table_from_inheritance(arena, cfg_table);
        if(view_rule_string.size != 0)
        {
          df_cfg_table_push_unparsed_string(arena, &child_cfg, view_rule_string, DF_CfgSrc_User);
        }
      }
      
      // rjf: recurse for sub-block
      DF_Eval child_eval = zero_struct;
      {
        child_eval.type_key = element_type_key;
        child_eval.mode     = arr_eval.mode;
        child_eval.offset   = arr_eval.offset + child_idx*element_type_byte_size;
      }
      list_out->total_visual_row_count -= 1;
      list_out->total_semantic_row_count -= 1;
      df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, key, child->key, push_str8f(arena, "[%I64u]", child_idx), child_eval, &child_cfg, depth+1, list_out);
      
      // rjf: make new elemblock for remainder of children (if any)
      if(child_idx+1 < array_count)
      {
        DF_EvalVizBlock *next_elemblock = push_array(arena, DF_EvalVizBlock, 1);
        next_elemblock->kind                  = DF_EvalVizBlockKind_Elements;
        next_elemblock->eval_view             = eval_view;
        next_elemblock->eval                  = arr_eval;
        next_elemblock->cfg_table             = *cfg_table;
        next_elemblock->parent_key            = key;
        next_elemblock->key                   = df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 0);
        next_elemblock->visual_idx_range      = r1u64(child_idx+1, array_count);
        next_elemblock->semantic_idx_range    = r1u64(child_idx+1, array_count);
        next_elemblock->depth                 = depth+1;
        SLLQueuePush(list_out->first, list_out->last, next_elemblock);
        list_out->count += 1;
        elemblock = next_elemblock;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: (ptr to ptrs) descend to make blocks for pointed-at-pointer
  //
  if(parent_is_expanded && expand_rule == DF_EvalVizExpandRule_Default && (ptr_type_kind == TG_Kind_Ptr || ptr_type_kind == TG_Kind_LRef || ptr_type_kind == TG_Kind_RRef))
    ProfScope("build viz blocks for ptr-to-ptrs")
  {
    String8 subexpr = push_str8f(arena, "*(%S)", string);
    df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, key, df_expand_key_make(key.uniquifier, df_hash_from_expand_key(key), 1), subexpr, ptr_eval, cfg_table, depth+1, list_out);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal DF_EvalVizBlockList
df_eval_viz_block_list_from_eval_view_expr(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_EvalView *eval_view, String8 expr)
{
  ProfBeginFunction();
  DF_EvalVizBlockList blocks = {0};
  {
    DF_ExpandKey start_parent_key = df_expand_key_make((U64)eval_view, 0, 0);
    DF_ExpandKey start_key = df_expand_key_make((U64)eval_view, 5381, 1);
    DF_Eval eval = df_eval_from_string(arena, scope, ctrl_ctx, parse_ctx, expr);
    U64 expr_comma_pos = str8_find_needle(expr, 0, str8_lit(","), 0);
    String8List default_view_rules = {0};
    if(expr_comma_pos < expr.size)
    {
      String8 expr_extension = str8_skip(expr, expr_comma_pos+1);
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
      else
      {
        str8_list_pushf(arena, &default_view_rules, "array:{%S}", expr_extension);
      }
    }
    String8 view_rule_string = df_eval_view_rule_from_key(eval_view, start_key);
    DF_CfgTable view_rule_table = {0};
    for(String8Node *n = default_view_rules.first; n != 0; n = n->next)
    {
      df_cfg_table_push_unparsed_string(arena, &view_rule_table, n->string, DF_CfgSrc_User);
    }
    df_cfg_table_push_unparsed_string(arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
    df_append_viz_blocks_for_parent__rec(arena, scope, eval_view, ctrl_ctx, parse_ctx, start_parent_key, start_key, expr, eval, &view_rule_table, 0, &blocks);
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

////////////////////////////////
//~ rjf: Main State Accessors/Mutators

//- rjf: frame metadata

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

internal F64
df_time_in_seconds(void)
{
  return df_state->time_in_seconds;
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

internal String8List
df_cfg_strings_from_core(Arena *arena, String8 root_path, DF_CfgSrc source)
{
  ProfBeginFunction();
  String8List strs = {0};
  
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
        str8_list_push (arena, &strs,  str8_lit("target:\n"));
        str8_list_push (arena, &strs,  str8_lit("{\n"));
        if(label.size != 0)
        {
          str8_list_pushf(arena, &strs,          "  label:             \"%S\"\n", label);
        }
        str8_list_pushf(arena, &strs,           "  exe:               \"%S\"\n", exe_normalized_rel);
        str8_list_pushf(arena, &strs,           "  arguments:         \"%S\"\n", args__ent->name);
        str8_list_pushf(arena, &strs,           "  working_directory: \"%S\"\n", wdir_normalized_rel);
        if(entry_point_name.size != 0)
        {
          str8_list_pushf(arena, &strs,          "  entry_point:       \"%S\"\n", entry_point_name);
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
          str8_list_pushf(arena, &strs, "  symbol: \"%S\"\n", symb->name);
        }
        
        // rjf: address breakpoints
        else if(bp->flags & DF_EntityFlag_HasVAddr)
        {
          str8_list_pushf(arena, &strs, "  addr: 0x%I64x\n", bp->vaddr);
        }
        
        // rjf: conditions
        if(!df_entity_is_nil(cond))
        {
          str8_list_pushf(arena, &strs, "  condition: \"%S\"\n", cond->name);
        }
        
        // rjf: universal options
        str8_list_pushf(arena, &strs, "  enabled: %i\n", (int)bp->b32);
        if(bp->name.size != 0)
        {
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
        str8_list_pushf(arena, &strs, "  expression: \"%S\"\n", pin->name);
        DF_Entity *file = df_entity_ancestor_from_kind(pin, DF_EntityKind_File);
        if(pin->flags & DF_EntityFlag_HasTextPoint && !df_entity_is_nil(file))
        {
          String8 profile_path = root_path;
          String8 pin_file_path = df_full_path_from_entity(arena, file);
          profile_path = path_normalized_from_string(arena, profile_path);
          pin_file_path = path_normalized_from_string(arena, pin_file_path);
          String8 srlized_pin_file_path = path_relative_dst_from_absolute_dst_src(arena, pin_file_path, profile_path);
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
  if(source == DF_CfgSrc_Profile)
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
  
  //- rjf: write control settings
  if(source == DF_CfgSrc_Profile)
  {
    str8_list_push(arena, &strs, str8_lit("/// control settings //////////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    str8_list_pushf(arena, &strs, "solo_stepping_mode: %i\n", df_state->ctrl_solo_stepping_mode);
    str8_list_push(arena, &strs, str8_lit("\n"));
  }
  
  //- rjf: write eval view cache
#if 0
  if(source == DF_CfgSrc_Profile)
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

internal DF_EntityList
df_push_active_binary_list(Arena *arena)
{
  DF_EntityList binaries = {0};
  DF_EntityList modules = df_query_cached_entity_list_with_kind(DF_EntityKind_Module);
  for(DF_EntityNode *n = modules.first; n != 0; n = n->next)
  {
    DF_Entity *module = n->entity;
    DF_Entity *binary = df_binary_file_from_module(module);
    df_entity_list_push(arena, &binaries, binary);
  }
  return binaries;
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

internal DF_Unwind
df_query_cached_unwind_from_thread(DF_Entity *thread)
{
  ProfBeginFunction();
  DF_Unwind result = {0};
  DF_RunUnwindCache *cache = &df_state->unwind_cache;
  if(cache->table_size != 0)
  {
    DF_Handle handle = df_handle_from_entity(thread);
    U64 hash = df_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash % cache->table_size;
    DF_RunUnwindCacheSlot *slot = &cache->table[slot_idx];
    for(DF_RunUnwindCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(df_handle_match(n->thread, handle))
      {
        result = n->unwind;
        break;
      }
    }
  }
  ProfEnd();
  return result;
}

internal U64
df_query_cached_rip_from_thread(DF_Entity *thread)
{
  U64 result = 0;
  DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
  if(unwind.first != 0)
  {
    result = unwind.first->rip;
  }
  return result;
}

internal U64
df_query_cached_rip_from_thread_unwind(DF_Entity *thread, U64 unwind_count)
{
  U64 result = 0;
  DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
  U64 unwind_idx = 0;
  for(DF_UnwindFrame *frame = unwind.first; frame != 0; frame = frame->next, unwind_idx += 1)
  {
    if(unwind_idx == unwind_count)
    {
      result = frame->rip;
      break;
    }
  }
  return result;
}

internal EVAL_String2NumMap *
df_query_cached_locals_map_from_binary_voff(DF_Entity *binary, U64 voff)
{
  ProfBeginFunction();
  EVAL_String2NumMap *map = &eval_string2num_map_nil;
  {
    DF_RunLocalsCache *cache = &df_state->locals_cache;
    if(cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, DF_RunLocalsCacheSlot, cache->table_size);
    }
    DF_Handle handle = df_handle_from_entity(binary);
    U64 hash = df_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash % cache->table_size;
    DF_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    DF_RunLocalsCacheNode *node = 0;
    for(DF_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(df_handle_match(n->binary, handle) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DBGI_Scope *scope = dbgi_scope_open();
      EVAL_String2NumMap *map = df_push_locals_map_from_binary_voff(cache->arena, scope, binary, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, DF_RunLocalsCacheNode, 1);
        node->binary = handle;
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      dbgi_scope_close(scope);
    }
    if(node != 0)
    {
      map = node->locals_map;
    }
  }
  ProfEnd();
  return map;
}

internal EVAL_String2NumMap *
df_query_cached_member_map_from_binary_voff(DF_Entity *binary, U64 voff)
{
  ProfBeginFunction();
  EVAL_String2NumMap *map = &eval_string2num_map_nil;
  {
    DF_RunLocalsCache *cache = &df_state->member_cache;
    if(cache->table_size == 0)
    {
      cache->table_size = 256;
      cache->table = push_array(cache->arena, DF_RunLocalsCacheSlot, cache->table_size);
    }
    DF_Handle handle = df_handle_from_entity(binary);
    U64 hash = df_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash % cache->table_size;
    DF_RunLocalsCacheSlot *slot = &cache->table[slot_idx];
    DF_RunLocalsCacheNode *node = 0;
    for(DF_RunLocalsCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
      if(df_handle_match(n->binary, handle) && n->voff == voff)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      DBGI_Scope *scope = dbgi_scope_open();
      EVAL_String2NumMap *map = df_push_member_map_from_binary_voff(cache->arena, scope, binary, voff);
      if(map->slots_count != 0)
      {
        node = push_array(cache->arena, DF_RunLocalsCacheNode, 1);
        node->binary = handle;
        node->voff = voff;
        node->locals_map = map;
        SLLQueuePush_N(slot->first, slot->last, node, hash_next);
      }
      dbgi_scope_close(scope);
    }
    if(node != 0)
    {
      map = node->locals_map;
    }
  }
  ProfEnd();
  return map;
}

//- rjf: top-level command dispatch

internal void
df_push_cmd__root(DF_CmdParams *params, DF_CmdSpec *spec)
{
  df_cmd_list_push(df_state->root_cmd_arena, &df_state->root_cmds, params, spec);
}

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void
df_core_init(String8 user_path, String8 profile_path, DF_StateDeltaHistory *hist)
{
  Arena *arena = arena_alloc();
  df_state = push_array(arena, DF_State, 1);
  df_state->arena = arena;
  df_state->root_cmd_arena = arena_alloc();
  df_state->entities_arena = arena_alloc__sized(GB(64), KB(64));
  df_state->entities_root = &df_g_nil_entity;
  df_state->entities_base = push_array(df_state->entities_arena, DF_Entity, 0);
  df_state->entities_count = 0;
  df_state->ctrl_msg_arena = arena_alloc();
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
    df_entity_equip_ctrl_machine_id(local_machine, CTRL_MachineID_Client);
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
  
  // rjf: set up per-run caches
  df_state->unwind_cache.arena = arena_alloc();
  df_state->locals_cache.arena = arena_alloc();
  df_state->member_cache.arena = arena_alloc();
  
  // rjf: set up eval view cache
  df_state->eval_view_cache.slots_count = 4096;
  df_state->eval_view_cache.slots = push_array(arena, DF_EvalViewSlot, df_state->eval_view_cache.slots_count);
  
  // rjf: set up run state
  df_state->ctrl_last_run_arena = arena_alloc();
  
  // rjf: set up config reading state
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: set up config path state
    String8 cfg_src_paths[DF_CfgSrc_COUNT] = {user_path, profile_path};
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
    String8List current_path_strs = {0};
    os_string_list_from_system_path(scratch.arena, OS_SystemPath_Current, &current_path_strs);
    df_state->current_path_arena = arena_alloc();
    String8 current_path = str8_list_first(&current_path_strs);
    String8 current_path_with_slash = push_str8f(scratch.arena, "%S/", current_path);
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
  df_state->dt = dt;
  df_state->time_in_seconds += dt;
  
  //- rjf: sync with ctrl thread
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: grab next reggen/memgen
    U64 new_memgen_idx = ctrl_memgen_idx();
    U64 new_reggen_idx = ctrl_reggen_idx();
    
    //- rjf: consume & process events
    CTRL_EventList events = ctrl_c2u_pop_events(scratch.arena);
    for(CTRL_EventNode *event_n = events.first; event_n != 0; event_n = event_n->next)
    {
      CTRL_Event *event = &event_n->v;
      switch(event->kind)
      {
        default:{}break;
        
        //- rjf: starts/stops
        
        case CTRL_EventKind_Started:
        {
          df_state->ctrl_is_running = 1;
        }break;
        
        case CTRL_EventKind_Stopped:
        {
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
          if(!df_entity_is_nil(stop_thread))
          {
            DF_CmdParams params = df_cmd_params_zero();
            params.entity = df_handle_from_entity(stop_thread);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
          }
          
          // rjf: thread hit user breakpoint -> increment breakpoint hit count
          if(event->cause == CTRL_EventCause_UserBreakpoint)
          {
            U64 stop_thread_vaddr = df_rip_from_thread(stop_thread);
            DF_Entity *process = df_entity_ancestor_from_kind(stop_thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, stop_thread_vaddr);
            DF_Entity *binary = df_binary_file_from_module(module);
            U64 stop_thread_voff = df_voff_from_vaddr(module, stop_thread_vaddr);
            DF_TextLineDasm2SrcInfo dasm2src_info = df_text_line_dasm2src_info_from_binary_voff(binary, stop_thread_voff);
            DF_EntityList user_bps = df_query_cached_entity_list_with_kind(DF_EntityKind_Breakpoint);
            for(DF_EntityNode *n = user_bps.first; n != 0; n = n->next)
            {
              DF_Entity *bp = n->entity;
              if(bp->flags & DF_EntityFlag_HasVAddr && bp->vaddr == stop_thread_vaddr)
              {
                bp->u64 += 1;
              }
              if(bp->flags & DF_EntityFlag_HasTextPoint)
              {
                DF_Entity *bp_file = df_entity_ancestor_from_kind(bp, DF_EntityKind_File);
                if(bp_file == dasm2src_info.file && bp->text_point.line == dasm2src_info.pt.line)
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
        }break;
        
        //- rjf: entity creation/deletion
        
        case CTRL_EventKind_NewProc:
        {
          // rjf: the first process? -> clear session output & reset all bp hit counts
          DF_EntityList existing_processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          if(existing_processes.count == 0)
          {
            Temp scratch = scratch_begin(0, 0);
            DF_Entity *session_log = df_log_from_entity(df_entity_root());
            TXTI_Handle session_log_handle = df_txti_handle_from_entity(session_log);
            txti_reload(session_log_handle, df_full_path_from_entity(scratch.arena, session_log));
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
          
          // rjf: create & attach binary file
          String8 bin_path = module->name;
          DF_Entity *binary = df_entity_from_path(bin_path, DF_EntityFromPathFlag_All);
          df_entity_equip_entity_handle(module, df_handle_from_entity(binary));
          
          // rjf: is first -> attach process color if applicable
          if(is_first && parent->flags & DF_EntityFlag_HasColor)
          {
            Vec4F32 rgba = df_rgba_from_entity(parent);
            df_entity_equip_color_rgba(module, rgba);
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
        
        //- rjf: debug strings
        
        case CTRL_EventKind_DebugString:
        {
          String8 string = event->string;
          DF_Entity *root = df_entity_root();
          DF_Entity *thread = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          DF_Entity *machine = df_entity_ancestor_from_kind(process, DF_EntityKind_Machine);
          DF_Entity *root_log = df_log_from_entity(root);
          DF_Entity *thread_log = df_log_from_entity(thread);
          DF_Entity *process_log = df_log_from_entity(process);
          DF_Entity *machine_log = df_log_from_entity(machine);
          TXTI_Handle root_log_handle = df_txti_handle_from_entity(root_log);
          TXTI_Handle thread_log_handle = df_txti_handle_from_entity(thread_log);
          TXTI_Handle process_log_handle = df_txti_handle_from_entity(process_log);
          TXTI_Handle machine_log_handle = df_txti_handle_from_entity(machine_log);
          txti_append(root_log_handle, string);
          txti_append(thread_log_handle, string);
          txti_append(process_log_handle, string);
          txti_append(machine_log_handle, string);
        }break;
        
        case CTRL_EventKind_ThreadName:
        {
          String8 string = event->string;
          DF_Entity *thread = df_entity_from_ctrl_handle(event->machine_id, event->entity);
          df_entity_equip_name(0, thread, string);
        }break;
        
        //- rjf: memory
        
        case CTRL_EventKind_MemReserve:{}break;
        case CTRL_EventKind_MemCommit:{}break;
        case CTRL_EventKind_MemDecommit:{}break;
        case CTRL_EventKind_MemRelease:{}break;
        
        //- rjf: ctrl requests
        
        case CTRL_EventKind_LaunchAndInitDone:
        case CTRL_EventKind_LaunchAndHandshakeDone:
        case CTRL_EventKind_AttachDone:
        case CTRL_EventKind_KillDone:
        case CTRL_EventKind_DetachDone:
        {
          // rjf: resolve request entities
          DF_EntityID id = event->msg_id;
          DF_Entity *request_entity = df_entity_from_id(id);
          if(!df_entity_is_nil(request_entity))
          {
            df_entity_mark_for_deletion(request_entity);
            switch(request_entity->subkind)
            {
              case CTRL_MsgKind_LaunchAndInit:
              case CTRL_MsgKind_LaunchAndHandshake:
              {
                DF_Entity *target = df_entity_from_handle(request_entity->entity_handle);
                DF_Entity *process = df_entity_from_ctrl_id(event->machine_id, event->entity_id);
                DF_Entity *thread = df_entity_child_from_kind(process, DF_EntityKind_Thread);
                if(!df_entity_is_nil(target) && !df_entity_is_nil(process) && !df_entity_is_nil(thread))
                {
                  df_entity_equip_entity_handle(process, df_handle_from_entity(target));
                  if(target->flags & DF_EntityFlag_HasColor)
                  {
                    Vec4F32 color = df_rgba_from_entity(target);
                    df_entity_equip_color_rgba(process, color);
                    df_entity_equip_color_rgba(thread, color);
                  }
                }
              }break;
            }
          }
          
          // rjf: collect s top info
          arena_clear(df_state->ctrl_stop_arena);
          MemoryCopyStruct(&df_state->ctrl_last_stop_event, event);
          df_state->ctrl_last_stop_event.string = push_str8_copy(df_state->ctrl_stop_arena, df_state->ctrl_last_stop_event.string);
        }break;
      }
    }
    
    //- rjf: refresh unwind cache
    if((df_state->unwind_cache_memgen_idx != new_memgen_idx ||
        df_state->unwind_cache_reggen_idx != new_reggen_idx) &&
       !df_ctrl_targets_running()) ProfScope("per-thread unwind gather")
    {
      B32 good = 1;
      DF_EntityList all_threads = df_query_cached_entity_list_with_kind(DF_EntityKind_Thread);
      DF_RunUnwindCache *cache = &df_state->unwind_cache;
      arena_clear(cache->arena);
      cache->table_size = 1024;
      cache->table = push_array(cache->arena, DF_RunUnwindCacheSlot, cache->table_size);
      for(DF_EntityNode *n = all_threads.first; n != 0; n = n->next)
      {
        DF_Entity *thread = n->entity;
        DF_Handle thread_handle = df_handle_from_entity(thread);
        U64 hash = df_hash_from_string(str8_struct(&thread_handle));
        U64 slot_idx = hash % cache->table_size;
        DF_RunUnwindCacheSlot *slot = &cache->table[slot_idx];
        DF_RunUnwindCacheNode *cache_node = push_array(cache->arena, DF_RunUnwindCacheNode, 1);
        cache_node->thread = thread_handle;
        cache_node->unwind = df_push_unwind_from_thread(cache->arena, thread);
        SLLQueuePush_NZ(0, slot->first, slot->last, cache_node, hash_next);
        if(cache_node->unwind.error != 0)
        {
          good = 0;
          break;
        }
      }
      df_state->unwind_cache_memgen_idx = new_memgen_idx;
      df_state->unwind_cache_reggen_idx = new_reggen_idx;
    }
    
    //- rjf: clear locals cache
    if(df_state->locals_cache_reggen_idx != new_reggen_idx && !df_ctrl_targets_running())
    {
      DF_RunLocalsCache *cache = &df_state->locals_cache;
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      df_state->locals_cache_reggen_idx = new_reggen_idx;
    }
    
    //- rjf: clear members cache
    if(df_state->member_cache_reggen_idx != new_reggen_idx && !df_ctrl_targets_running())
    {
      DF_RunLocalsCache *cache = &df_state->member_cache;
      arena_clear(cache->arena);
      cache->table_size = 0;
      cache->table = 0;
      df_state->member_cache_reggen_idx = new_reggen_idx;
    }
    
    scratch_end(scratch);
  }
  
  //- rjf: sync with dbgi parsers
  {
    Temp scratch = scratch_begin(&arena, 1);
    DBGI_EventList events = dbgi_p2u_pop_events(scratch.arena, 0);
    for(DBGI_EventNode *n = events.first; n != 0; n = n->next)
    {
      DBGI_Event *event = &n->v;
      switch(event->kind)
      {
        default:{}break;
        case DBGI_EventKind_ConversionStarted:
        {
          DF_Entity *task = df_entity_alloc(0, df_entity_root(), DF_EntityKind_ConversionTask);
          df_entity_equip_name(0, task, event->string);
        }break;
        case DBGI_EventKind_ConversionEnded:
        {
          DF_Entity *task = df_entity_from_name_and_kind(event->string, DF_EntityKind_ConversionTask);
          if(!df_entity_is_nil(task))
          {
            df_entity_mark_for_deletion(task);
          }
        }break;
        case DBGI_EventKind_ConversionFailureUnsupportedFormat:
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
      df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProfileData));
      df_state->seconds_til_autosave = 5.f;
    }
  }
  
  //- rjf: process top-level commands
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
              if(path.size == 0)
              {
                String8List current_path_strs = {0};
                os_string_list_from_system_path(scratch.arena, OS_SystemPath_Current, &current_path_strs);
                path = str8_list_first(&current_path_strs);
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
                        if(str8_match(str8_prefix(string, 1), str8_lit("\""), 0))
                        {
                          string = str8_skip(string, 1);
                        }
                        if(str8_match(str8_postfix(string, 1), str8_lit("\""), 0))
                        {
                          string = str8_chop(string, 1);
                        }
                        str8_list_push(scratch.arena, &cmdln_strings, string);
                      }
                      start_split_idx = idx+1;
                    }
                  }
                }
              }
              
              // rjf: build corresponding request entity
              DF_Entity *request_entity = df_entity_alloc(0, df_entity_root(), DF_EntityKind_CtrlRequest);
              {
                request_entity->subkind = CTRL_MsgKind_LaunchAndInit;
                request_entity->entity_handle = df_handle_from_entity(target);
              }
              
              // rjf: push message to launch
              {
                CTRL_Msg msg = {CTRL_MsgKind_LaunchAndInit};
                msg.msg_id = request_entity->id;
                msg.path = path;
                msg.cmd_line_string_list = cmdln_strings;
                msg.env_inherit = 1;
                MemoryCopyArray(msg.exception_code_filters, df_state->ctrl_exception_code_filters);
                str8_list_push(scratch.arena, &msg.strings, entry);
                df_push_ctrl_msg(&msg);
              }
            }
            
            // rjf: run if needed
            if(core_cmd_kind == DF_CoreCmdKind_LaunchAndRun)
            {
              df_ctrl_run(DF_RunKind_Run, &df_g_nil_entity, 0);
            }
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
            df_ctrl_run(DF_RunKind_Run, &df_g_nil_entity, 0);
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
        case DF_CoreCmdKind_RunToAddress:
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
                DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
                
                // rjf: use first unwind frame to generate trap
                if(unwind.first != 0 && unwind.first->next != 0)
                {
                  U64 vaddr = unwind.first->next->rip;
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
              case DF_CoreCmdKind_RunToAddress:
              {
                CTRL_Trap trap = {CTRL_TrapFlag_EndStepping|CTRL_TrapFlag_IgnoreStackPointerCheck, params.vaddr};
                ctrl_trap_list_push(scratch.arena, &traps, &trap);
              }break;
            }
            if(good && traps.count != 0)
            {
              df_ctrl_run(DF_RunKind_Step, thread, &traps);
            }
            if(good && traps.count == 0)
            {
              df_ctrl_run(DF_RunKind_SingleStep, thread, &traps);
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
            df_ctrl_run(df_state->ctrl_last_run_kind, df_entity_from_handle(df_state->ctrl_last_run_thread), &df_state->ctrl_last_run_traps);
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
        
        //- rjf: solo-stepping mode
        case DF_CoreCmdKind_EnableSoloSteppingMode:
        {
          df_state->ctrl_solo_stepping_mode = 1;
        }break;
        case DF_CoreCmdKind_DisableSoloSteppingMode:
        {
          df_state->ctrl_solo_stepping_mode = 0;
        }break;
        
        //- rjf: debug control context management operations
        case DF_CoreCmdKind_SelectThread:
        {
          df_state->ctrl_ctx.thread = params.entity;
          df_state->ctrl_ctx.unwind_count = 0;
        }break;
        case DF_CoreCmdKind_SelectUnwind:
        {
          DF_Entity *thread = df_entity_from_handle(df_state->ctrl_ctx.thread);
          DF_Unwind unwind = df_query_cached_unwind_from_thread(thread);
          U64 max_unwind = unwind.count ? unwind.count-1 : 0;
          U64 index = Clamp(0, params.index, max_unwind);
          df_state->ctrl_ctx.unwind_count = index;
        }break;
        case DF_CoreCmdKind_UpOneFrame:
        {
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx();
          DF_CmdParams p = params;
          p.index = (ctrl_ctx.unwind_count > 0 ? ctrl_ctx.unwind_count - 1 : 0);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Index);
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
        }break;
        case DF_CoreCmdKind_DownOneFrame:
        {
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx();
          DF_CmdParams p = params;
          p.index = ctrl_ctx.unwind_count+1;
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Index);
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectUnwind));
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
          CTRL_MachineID machine_id = CTRL_MachineID_Client;
          DF_CmdParams params = df_cmd_params_zero();
          params.entity = df_handle_from_entity(df_machine_entity_from_machine_id(machine_id));
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FreezeMachine));
        }break;
        case DF_CoreCmdKind_ThawLocalMachine:
        {
          CTRL_MachineID machine_id = CTRL_MachineID_Client;
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
        case DF_CoreCmdKind_Reload:
        {
          DF_Entity *file = df_entity_from_handle(params.entity);
          if(file->kind == DF_EntityKind_File)
          {
            TXTI_Handle txti_handle = df_txti_handle_from_entity(file);
            txti_reload(txti_handle, df_full_path_from_entity(scratch.arena, file));
          }
        }break;
        
        //- rjf: config path saving/loading/applying
        case DF_CoreCmdKind_LoadUser:
        case DF_CoreCmdKind_LoadProfile:
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
        case DF_CoreCmdKind_ApplyProfileData:
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
          
          //- rjf: eliminate all existing entities
          {
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
          
          //- rjf: apply targets
          DF_CfgVal *targets = df_cfg_val_from_string(table, str8_lit("target"));
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
              try_u64_from_str8_c_rules(active_cfg->first->string, &is_active_u64);
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
              df_entity_equip_b32(target__ent, active_cfg != &df_g_nil_cfg_node ? !!is_active_u64 : 1);
              df_entity_equip_name(0, target__ent, saved_label);
              df_entity_equip_name(0, exe__ent,    saved_exe_absolute);
              df_entity_equip_name(0, args__ent,   args_cfg->first->string);
              df_entity_equip_name(0, path__ent,   saved_wdir_absolute);
              df_entity_equip_name(0, entry__ent,  saved_entry_point);
              df_entity_equip_cfg_src(target__ent, src);
              if(!memory_is_zero(&hsva, sizeof(hsva)))
              {
                df_entity_equip_color_hsva(target__ent, hsva);
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
                DF_Entity *symb = df_entity_alloc(0, bp_ent, DF_EntityKind_EntryPointName);
                df_entity_equip_name(0, symb, symb_cfg->first->string);
              }
              if(labl_cfg->string.size != 0)
              {
                df_entity_equip_name(0, bp_ent, labl_cfg->first->string);
              }
              if(!memory_is_zero(&hsva, sizeof(hsva)))
              {
                df_entity_equip_color_hsva(bp_ent, hsva);
              }
              if(cond_cfg->string.size != 0)
              {
                DF_Entity *cond = df_entity_alloc(0, bp_ent, DF_EntityKind_Condition);
                df_entity_equip_name(0, cond, cond_cfg->first->string);
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
            df_entity_equip_name(0, pin_ent, string);
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
          
          //- rjf: apply control settings
          {
            DF_CfgVal *solo_stepping_mode_cfg_val = df_cfg_val_from_string(table, str8_lit("solo_stepping_mode"));
            if(solo_stepping_mode_cfg_val != &df_g_nil_cfg_val)
            {
              DF_CfgNode *value_cfg = solo_stepping_mode_cfg_val->last->first;
              U64 val = 0;
              try_u64_from_str8_c_rules(value_cfg->string, &val);
              df_state->ctrl_solo_stepping_mode = (B32)val;
            }
          }
          
        }break;
        
        //- rjf: writing config changes
        case DF_CoreCmdKind_WriteUserData:
        case DF_CoreCmdKind_WriteProfileData:
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
          DF_Entity *file = df_entity_from_handle(params.entity);
          DF_Entity *replacement = df_entity_from_path(params.file_path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
          if(!df_entity_is_nil(file) && !df_entity_is_nil(replacement))
          {
            DF_Entity *link = df_entity_child_from_name_and_kind(file->parent, file->name, DF_EntityKind_OverrideFileLink);
            if(df_entity_is_nil(link))
            {
              link = df_entity_alloc(0, file->parent, DF_EntityKind_OverrideFileLink);
              df_entity_equip_name(0, link, file->name);
            }
            df_entity_equip_entity_handle(link, df_handle_from_entity(replacement));
          }
        }break;
        
        //- rjf: general entity operations
        case DF_CoreCmdKind_EnableEntity:
        case DF_CoreCmdKind_EnableBreakpoint:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          df_state_delta_history_push_batch(df_state->hist, &entity->generation);
          df_state_delta_history_push_struct_delta(df_state->hist, &entity->b32);
          df_entity_equip_b32(entity, 1);
        }break;
        case DF_CoreCmdKind_DisableEntity:
        case DF_CoreCmdKind_DisableBreakpoint:
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
              df_entity_equip_cfg_src(bp, DF_CfgSrc_Profile);
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
              df_entity_equip_cfg_src(bp, DF_CfgSrc_Profile);
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
              df_entity_equip_cfg_src(bp, DF_CfgSrc_Profile);
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
              df_entity_equip_cfg_src(watch, DF_CfgSrc_Profile);
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
              df_entity_equip_cfg_src(pin, DF_CfgSrc_Profile);
            }
          }
        }break;
        
        //- rjf: targets
        case DF_CoreCmdKind_AddTarget:
        {
          // rjf: build target
          df_state_delta_history_push_batch(df_state_delta_history(), 0);
          DF_Entity *entity = df_entity_alloc(df_state_delta_history(), df_entity_root(), DF_EntityKind_Target);
          df_entity_equip_b32(entity, 1);
          df_entity_equip_cfg_src(entity, DF_CfgSrc_Profile);
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
          String8 path_to_debugger_binary = os_get_command_line_arguments().first->string;
          String8 name8 = str8_lit("Debugger");
          String8 data8 = push_str8f(scratch.arena, "%S --jit_pid:%%ld --jit_code:%%ld --jit_addr:0x%%p", path_to_debugger_binary);
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
      }
    }
    scratch_end(scratch);
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
