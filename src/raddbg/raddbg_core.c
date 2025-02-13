// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef MARKUP_LAYER_COLOR
#define MARKUP_LAYER_COLOR 0.10f, 0.20f, 0.25f

////////////////////////////////
//~ rjf: Generated Code

#include "generated/raddbg.meta.c"

////////////////////////////////
//~ rjf: Config ID Type Functions

internal void
rd_cfg_id_list_push(Arena *arena, RD_CfgIDList *list, RD_CfgID id)
{
  RD_CfgIDNode *n = push_array(arena, RD_CfgIDNode, 1);
  n->v = id;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal RD_CfgIDList
rd_cfg_id_list_copy(Arena *arena, RD_CfgIDList *src)
{
  RD_CfgIDList result = {0};
  for(RD_CfgIDNode *n = src->first; n != 0; n = n->next)
  {
    rd_cfg_id_list_push(arena, &result, n->v);
  }
  return result;
}

////////////////////////////////
//~ rjf: Registers Type Functions

internal void
rd_regs_copy_contents(Arena *arena, RD_Regs *dst, RD_Regs *src)
{
  MemoryCopyStruct(dst, src);
  dst->cfg_list    = rd_cfg_id_list_copy(arena, &src->cfg_list);
  dst->file_path   = push_str8_copy(arena, src->file_path);
  dst->lines       = d_line_list_copy(arena, &src->lines);
  dst->dbgi_key    = di_key_copy(arena, &src->dbgi_key);
  dst->string      = push_str8_copy(arena, src->string);
  dst->cmd_name    = push_str8_copy(arena, src->cmd_name);
  dst->params_tree = md_tree_copy(arena, src->params_tree);
  if(dst->cfg_list.count == 0 && dst->cfg != 0)
  {
    rd_cfg_id_list_push(arena, &dst->cfg_list, dst->cfg);
  }
}

internal RD_Regs *
rd_regs_copy(Arena *arena, RD_Regs *src)
{
  RD_Regs *dst = push_array(arena, RD_Regs, 1);
  rd_regs_copy_contents(arena, dst, src);
  return dst;
}

////////////////////////////////
//~ rjf: Commands Type Functions

internal void
rd_cmd_list_push_new(Arena *arena, RD_CmdList *cmds, String8 name, RD_Regs *regs)
{
  RD_CmdNode *n = push_array(arena, RD_CmdNode, 1);
  n->cmd.name = push_str8_copy(arena, name);
  n->cmd.regs = rd_regs_copy(arena, regs);
  DLLPushBack(cmds->first, cmds->last, n);
  cmds->count += 1;
}

////////////////////////////////
//~ rjf: View UI Rule Functions

internal RD_ViewUIRuleMap *
rd_view_ui_rule_map_make(Arena *arena, U64 slots_count)
{
  RD_ViewUIRuleMap *map = push_array(arena, RD_ViewUIRuleMap, 1);
  map->slots_count = slots_count;
  map->slots = push_array(arena, RD_ViewUIRuleSlot, map->slots_count);
  return map;
}

internal void
rd_view_ui_rule_map_insert(Arena *arena, RD_ViewUIRuleMap *map, String8 string, RD_ViewUIFunctionType *ui)
{
  U64 hash = d_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  RD_ViewUIRuleNode *n = push_array(arena, RD_ViewUIRuleNode, 1);
  n->v.name = push_str8_copy(arena, string);
  n->v.ui = ui;
  SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, n);
}

internal RD_ViewUIRule *
rd_view_ui_rule_from_string(String8 string)
{
  RD_ViewUIRule *rule = &rd_nil_view_ui_rule;
  {
    RD_ViewUIRuleMap *map = rd_state->view_ui_rule_map;
    U64 hash = d_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    for(RD_ViewUIRuleNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
    {
      if(str8_match(n->v.name, string, 0))
      {
        rule = &n->v;
        break;
      }
    }
  }
  return rule;
}

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32
rd_drag_is_active(void)
{
  return ((rd_state->drag_drop_state == RD_DragDropState_Dragging) ||
          (rd_state->drag_drop_state == RD_DragDropState_Dropping));
}

internal void
rd_drag_begin(RD_RegSlot slot)
{
  if(!rd_drag_is_active())
  {
    arena_clear(rd_state->drag_drop_arena);
    rd_state->drag_drop_regs = rd_regs_copy(rd_state->drag_drop_arena, rd_regs());
    rd_state->drag_drop_regs_slot = slot;
    rd_state->drag_drop_state = RD_DragDropState_Dragging;
  }
}

internal B32
rd_drag_drop(void)
{
  B32 result = 0;
  if(rd_state->drag_drop_state == RD_DragDropState_Dropping)
  {
    result = 1;
    rd_state->drag_drop_state = RD_DragDropState_Null;
  }
  return result;
}

internal void
rd_drag_kill(void)
{
  rd_state->drag_drop_state = RD_DragDropState_Null;
}

internal void
rd_set_hover_regs(RD_RegSlot slot)
{
  rd_state->next_hover_regs = rd_regs_copy(rd_frame_arena(), rd_regs());
  rd_state->next_hover_regs_slot = slot;
}

internal RD_Regs *
rd_get_hover_regs(void)
{
  return rd_state->hover_regs;
}

internal void
rd_open_ctx_menu(UI_Key anchor_box_key, Vec2F32 anchor_box_off, RD_RegSlot slot)
{
  RD_Cfg *window_cfg = rd_cfg_from_id(rd_regs()->window);
  RD_WindowState *ws = rd_window_state_from_cfg(window_cfg);
  if(ws != 0)
  {
    ui_ctx_menu_open(rd_state->ctx_menu_key, anchor_box_key, anchor_box_off);
    arena_clear(ws->ctx_menu_arena);
    ws->ctx_menu_regs = rd_regs_copy(ws->ctx_menu_arena, rd_regs());
    ws->ctx_menu_regs_slot = slot;
  }
}

////////////////////////////////
//~ rjf: Name Allocation

internal U64
rd_name_bucket_idx_from_string_size(U64 size)
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
    default:{bucket_idx = ArrayCount(rd_state->free_name_chunks)-1;}break;
  }
  return bucket_idx;
}

internal String8
rd_name_alloc(String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = rd_name_bucket_idx_from_string_size(string.size);
  
  // rjf: loop -> find node, allocate if not there
  //
  // (we do a loop here so that all allocation logic goes through
  // the same path, such that we *always* pull off a free list,
  // rather than just using what was pushed onto an arena directly,
  // which is not undoable; the free lists we control, and are thus
  // trivially undoable)
  //
  RD_NameChunkNode *node = 0;
  for(;node == 0;)
  {
    node = rd_state->free_name_chunks[bucket_idx];
    
    // rjf: pull from bucket free list
    if(node != 0)
    {
      if(bucket_idx == ArrayCount(rd_state->free_name_chunks)-1)
      {
        node = 0;
        RD_NameChunkNode *prev = 0;
        for(RD_NameChunkNode *n = rd_state->free_name_chunks[bucket_idx];
            n != 0;
            prev = n, n = n->next)
        {
          if(n->size >= string.size)
          {
            if(prev == 0)
            {
              rd_state->free_name_chunks[bucket_idx] = n->next;
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
        SLLStackPop(rd_state->free_name_chunks[bucket_idx]);
      }
    }
    
    // rjf: no found node -> allocate new, push onto associated free list
    if(node == 0)
    {
      U64 chunk_size = 0;
      if(bucket_idx < ArrayCount(rd_state->free_name_chunks)-1)
      {
        chunk_size = 1<<(bucket_idx+4);
      }
      else
      {
        chunk_size = u64_up_to_pow2(string.size);
      }
      U8 *chunk_memory = push_array(rd_state->arena, U8, chunk_size);
      RD_NameChunkNode *chunk = (RD_NameChunkNode *)chunk_memory;
      chunk->size = chunk_size;
      SLLStackPush(rd_state->free_name_chunks[bucket_idx], chunk);
    }
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
rd_name_release(String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = rd_name_bucket_idx_from_string_size(string.size);
  RD_NameChunkNode *node = (RD_NameChunkNode *)string.str;
  node->size = u64_up_to_pow2(string.size);
  SLLStackPush(rd_state->free_name_chunks[bucket_idx], node);
}

////////////////////////////////
//~ rjf: Config Tree Functions

internal RD_Cfg *
rd_cfg_alloc(void)
{
  // rjf: allocate
  RD_Cfg *result = rd_state->free_cfg;
  {
    if(result)
    {
      SLLStackPop(rd_state->free_cfg);
    }
    else
    {
      result = push_array_no_zero(rd_state->arena, RD_Cfg, 1);
    }
  }
  
  // rjf: generate ID & fill
  rd_state->cfg_id_gen += 1;
  MemoryZeroStruct(result);
  result->first = result->last = result->next = result->prev = result->parent = &rd_nil_cfg;
  result->id = rd_state->cfg_id_gen;
  
  // rjf: store to ID -> cfg map
  {
    RD_CfgNode *cfg_id_node = rd_state->free_cfg_id_node;
    if(cfg_id_node != 0)
    {
      SLLStackPop(rd_state->free_cfg_id_node);
    }
    else
    {
      cfg_id_node = push_array(rd_state->arena, RD_CfgNode, 1);
    }
    U64 hash = d_hash_from_string(str8_struct(&result->id));
    U64 slot_idx = hash%rd_state->cfg_id_slots_count;
    DLLPushBack(rd_state->cfg_id_slots[slot_idx].first, rd_state->cfg_id_slots[slot_idx].last, cfg_id_node);
    cfg_id_node->v = result;
  }
  
  return result;
}

internal void
rd_cfg_release(RD_Cfg *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: unhook from context
  rd_cfg_unhook(cfg->parent, cfg);
  
  // rjf: gather root & all descendants
  RD_CfgList nodes = {0};
  for(RD_Cfg *c = cfg; c != &rd_nil_cfg; c = rd_cfg_rec__depth_first(cfg, c).next)
  {
    rd_cfg_list_push(scratch.arena, &nodes, c);
  }
  
  // rjf: release all nodes
  for(RD_CfgNode *n = nodes.first; n != 0; n = n->next)
  {
    RD_Cfg *c = n->v;
    rd_name_release(c->string);
    SLLStackPush(rd_state->free_cfg, c);
    U64 hash = d_hash_from_string(str8_struct(&c->id));
    U64 slot_idx = hash%rd_state->cfg_id_slots_count;
    for(RD_CfgNode *n = rd_state->cfg_id_slots[slot_idx].first; n != 0; n = n->next)
    {
      if(n->v == c)
      {
        DLLRemove(rd_state->cfg_id_slots[slot_idx].first, rd_state->cfg_id_slots[slot_idx].last, n);
        SLLStackPush(rd_state->free_cfg_id_node, n);
        break;
      }
    }
  }
  
  scratch_end(scratch);
}

internal void
rd_cfg_release_all_children(RD_Cfg *cfg)
{
  for(RD_Cfg *child = cfg->first, *next = &rd_nil_cfg; child != &rd_nil_cfg; child = next)
  {
    next = child->next;
    rd_cfg_release(child);
  }
}

internal RD_Cfg *
rd_cfg_from_id(RD_CfgID id)
{
  RD_Cfg *result = &rd_nil_cfg;
  U64 hash = d_hash_from_string(str8_struct(&id));
  U64 slot_idx = hash%rd_state->cfg_id_slots_count;
  for(RD_CfgNode *n = rd_state->cfg_id_slots[slot_idx].first; n != 0; n = n->next)
  {
    if(n->v->id == id)
    {
      result = n->v;
      break;
    }
  }
  return result;
}

internal RD_Cfg *
rd_cfg_new(RD_Cfg *parent, String8 string)
{
  RD_Cfg *cfg = rd_cfg_alloc();
  rd_cfg_insert_child(parent, parent->last, cfg);
  rd_cfg_equip_string(cfg, string);
  return cfg;
}

internal RD_Cfg *
rd_cfg_newf(RD_Cfg *parent, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  RD_Cfg *result = rd_cfg_new(parent, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal RD_Cfg *
rd_cfg_new_replace(RD_Cfg *parent, String8 string)
{
  for(RD_Cfg *child = parent->first->next, *next = &rd_nil_cfg; child != &rd_nil_cfg; child = next)
  {
    next = child->next;
    rd_cfg_release(child);
  }
  if(parent->first == &rd_nil_cfg)
  {
    rd_cfg_new(parent, str8_zero());
  }
  RD_Cfg *child = parent->first;
  rd_cfg_equip_string(child, string);
  return child;
}

internal RD_Cfg *
rd_cfg_new_replacef(RD_Cfg *parent, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  RD_Cfg *result = rd_cfg_new_replace(parent, string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal RD_Cfg *
rd_cfg_deep_copy(RD_Cfg *src_root)
{
  RD_CfgRec rec = {0};
  RD_Cfg *dst_root = &rd_nil_cfg;
  RD_Cfg *dst_parent = &rd_nil_cfg;
  for(RD_Cfg *src = src_root; src != &rd_nil_cfg; src = rec.next)
  {
    RD_Cfg *dst = rd_cfg_new(dst_parent, src->string);
    if(dst_root == &rd_nil_cfg)
    {
      dst_root = dst;
    }
    rec = rd_cfg_rec__depth_first(src_root, src);
    if(rec.push_count > 0)
    {
      dst_parent = dst;
    }
    else for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
    {
      dst_parent = dst_parent->parent;
    }
  }
  return dst_root;
}

internal void
rd_cfg_equip_string(RD_Cfg *cfg, String8 string)
{
  if(cfg->string.size != 0)
  {
    rd_name_release(cfg->string);
  }
  cfg->string = rd_name_alloc(string);
}

internal void
rd_cfg_equip_stringf(RD_Cfg *cfg, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  rd_cfg_equip_string(cfg, string);
  va_end(args);
  scratch_end(scratch);
}

internal void
rd_cfg_insert_child(RD_Cfg *parent, RD_Cfg *prev_child, RD_Cfg *new_child)
{
  if(parent != &rd_nil_cfg)
  {
    if(new_child->parent != &rd_nil_cfg)
    {
      rd_cfg_unhook(new_child->parent, new_child);
    }
    DLLInsert_NPZ(&rd_nil_cfg, parent->first, parent->last, prev_child, new_child, next, prev);
    new_child->parent = parent;
  }
}

internal void
rd_cfg_unhook(RD_Cfg *parent, RD_Cfg *child)
{
  if(child != &rd_nil_cfg && parent == child->parent && parent != &rd_nil_cfg)
  {
    DLLRemove_NPZ(&rd_nil_cfg, parent->first, parent->last, child, next, prev);
    child->parent = &rd_nil_cfg;
  }
}

internal RD_Cfg *
rd_cfg_child_from_string(RD_Cfg *parent, String8 string)
{
  RD_Cfg *child = &rd_nil_cfg;
  for(RD_Cfg *c = parent->first; c != &rd_nil_cfg; c = c->next)
  {
    if(str8_match(c->string, string, 0))
    {
      child = c;
      break;
    }
  }
  return child;
}

internal RD_Cfg *
rd_cfg_child_from_string_or_alloc(RD_Cfg *parent, String8 string)
{
  RD_Cfg *child = rd_cfg_child_from_string(parent, string);
  if(child == &rd_nil_cfg)
  {
    child = rd_cfg_new(parent, string);
  }
  return child;
}

internal RD_CfgList
rd_cfg_child_list_from_string(Arena *arena, RD_Cfg *parent, String8 string)
{
  RD_CfgList result = {0};
  for(RD_Cfg *child = parent->first; child != &rd_nil_cfg; child = child->next)
  {
    if(str8_match(child->string, string, 0))
    {
      rd_cfg_list_push(arena, &result, child);
    }
  }
  return result;
}

internal RD_CfgList
rd_cfg_top_level_list_from_string(Arena *arena, String8 string)
{
  RD_CfgList result = {0};
  for(RD_Cfg *bucket = rd_state->root_cfg->first; bucket != &rd_nil_cfg; bucket = bucket->next)
  {
    for(RD_Cfg *tln = bucket->first; tln != &rd_nil_cfg; tln = tln->next)
    {
      if(str8_match(tln->string, string, 0))
      {
        rd_cfg_list_push(arena, &result, tln);
      }
    }
  }
  return result;
}

internal RD_CfgArray
rd_cfg_array_from_list(Arena *arena, RD_CfgList *list)
{
  RD_CfgArray array = {0};
  array.count = list->count;
  array.v = push_array_no_zero(arena, RD_Cfg *, array.count);
  U64 idx = 0;
  for(RD_CfgNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    array.v[idx] = n->v;
  }
  return array;
}

internal RD_CfgList
rd_cfg_tree_list_from_string(Arena *arena, String8 string)
{
  RD_CfgList result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  MD_Node *root = md_tree_from_string(scratch.arena, string);
  for MD_EachNode(tln, root->first)
  {
    RD_Cfg *dst_root_n = &rd_nil_cfg;
    RD_Cfg *dst_active_parent_n = &rd_nil_cfg;
    MD_NodeRec rec = {0};
    for(MD_Node *src_n = tln; !md_node_is_nil(src_n); src_n = rec.next)
    {
      RD_Cfg *dst_n = rd_cfg_alloc();
      String8 src_n_string = src_n->string;
      String8 src_n_string__raw = raw_from_escaped_str8(scratch.arena, src_n_string);
      rd_cfg_equip_string(dst_n, src_n_string__raw);
      if(dst_active_parent_n != &rd_nil_cfg)
      {
        rd_cfg_insert_child(dst_active_parent_n, dst_active_parent_n->last, dst_n);
      }
      rec = md_node_rec_depth_first_pre(src_n, tln);
      if(rec.push_count > 0)
      {
        if(dst_active_parent_n == &rd_nil_cfg)
        {
          dst_root_n = dst_n;
        }
        dst_active_parent_n = dst_n;
      }
      else for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
      {
        dst_active_parent_n = dst_active_parent_n->parent;
      }
    }
    rd_cfg_list_push(arena, &result, dst_root_n);
  }
  scratch_end(scratch);
  return result;
}

internal String8
rd_string_from_cfg_tree(Arena *arena, RD_Cfg *cfg)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strings = {0};
  {
    typedef struct NestTask NestTask;
    struct NestTask
    {
      NestTask *next;
      RD_Cfg *cfg;
      B32 is_simple;
    };
    NestTask *top_nest_task = 0;
    RD_CfgRec rec = {0};
    for(RD_Cfg *c = cfg; c != &rd_nil_cfg; c = rec.next)
    {
      // rjf: push name of this node
      if(c->string.size != 0 || c->first == &rd_nil_cfg)
      {
        String8List c_name_strings = {0};
        B32 name_can_be_pushed_standalone = 0;
        {
          Temp temp = temp_begin(scratch.arena);
          MD_TokenizeResult c_name_tokenize = md_tokenize_from_text(temp.arena, c->string);
          name_can_be_pushed_standalone = (c_name_tokenize.tokens.count == 1 && c_name_tokenize.tokens.v[0].flags & (MD_TokenFlag_Identifier|
                                                                                                                     MD_TokenFlag_Numeric|
                                                                                                                     MD_TokenFlag_StringLiteral|
                                                                                                                     MD_TokenFlag_Symbol));
          temp_end(temp);
        }
        if(name_can_be_pushed_standalone)
        {
          str8_list_push(scratch.arena, &c_name_strings, c->string);
        }
        else
        {
          String8 c_name_escaped = escaped_from_raw_str8(scratch.arena, c->string);
          str8_list_push(scratch.arena, &c_name_strings, str8_lit("\""));
          str8_list_push(scratch.arena, &c_name_strings, c_name_escaped);
          str8_list_push(scratch.arena, &c_name_strings, str8_lit("\""));
        }
        if(top_nest_task != 0 && top_nest_task->is_simple)
        {
          str8_list_push(scratch.arena, &strings, str8_lit(" "));
        }
        str8_list_concat_in_place(&strings, &c_name_strings);
      }
      
      // rjf: grab next recursion
      rec = rd_cfg_rec__depth_first(cfg, c);
      
      // rjf: determine if this node is simple, and can be encoded on a single line -
      // if so, push a new nesting task onto the stack
      if(c->first != &rd_nil_cfg)
      {
        B32 is_simple_children_list = 1;
        for(RD_Cfg *child = c->first; child != &rd_nil_cfg; child = child->next)
        {
          if(child->first != &rd_nil_cfg && child != c->last)
          {
            is_simple_children_list = 0;
            break;
          }
        }
        NestTask *task = push_array(scratch.arena, NestTask, 1);
        task->cfg = c;
        task->is_simple = is_simple_children_list;
        SLLStackPush(top_nest_task, task);
      }
      
      // rjf: tree navigations -> encode hierarchy
      if(rec.push_count > 0)
      {
        if(top_nest_task->is_simple && c->string.size != 0)
        {
          str8_list_push(scratch.arena, &strings, str8_lit(":"));
        }
        else
        {
          if(c->string.size != 0)
          {
            str8_list_push(scratch.arena, &strings, str8_lit(":\n"));
          }
          str8_list_push(scratch.arena, &strings, str8_lit("{"));
        }
      }
      else
      {
        for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1, SLLStackPop(top_nest_task))
        {
          if(top_nest_task->is_simple)
          {
            if(top_nest_task->cfg->string.size == 0)
            {
              str8_list_push(scratch.arena, &strings, str8_lit(" }"));
            }
          }
          else
          {
            str8_list_push(scratch.arena, &strings, str8_lit("\n}"));
          }
        }
      }
      if(!top_nest_task || top_nest_task->is_simple == 0)
      {
        str8_list_push(scratch.arena, &strings, str8_lit("\n"));
      }
    }
  }
  String8 result_unindented = str8_list_join(scratch.arena, &strings, 0);
  String8 result = indented_from_string(arena, result_unindented);
  scratch_end(scratch);
  return result;
}

internal RD_CfgRec
rd_cfg_rec__depth_first(RD_Cfg *root, RD_Cfg *cfg)
{
  RD_CfgRec rec = {&rd_nil_cfg};
  if(cfg->first != &rd_nil_cfg)
  {
    rec.next = cfg->first;
    rec.push_count = 1;
  }
  else for(RD_Cfg *p = cfg; p != root; p = p->parent, rec.pop_count += 1)
  {
    if(p->next != &rd_nil_cfg)
    {
      rec.next = p->next;
      break;
    }
  }
  return rec;
}

internal void
rd_cfg_list_push(Arena *arena, RD_CfgList *list, RD_Cfg *cfg)
{
  RD_CfgNode *n = push_array(arena, RD_CfgNode, 1);
  n->v = cfg;
  DLLPushBack(list->first, list->last, n);
  list->count += 1;
}

internal RD_PanelTree
rd_panel_tree_from_cfg(Arena *arena, RD_Cfg *cfg)
{
  Temp scratch = scratch_begin(&arena, 1);
  RD_Cfg *wcfg = rd_window_from_cfg(cfg);
  RD_Cfg *src_root = rd_cfg_child_from_string(wcfg, str8_lit("panels"));
  RD_PanelNode *dst_root = &rd_nil_panel_node;
  RD_PanelNode *dst_focused = &rd_nil_panel_node;
  {
    Axis2 active_split_axis = rd_cfg_child_from_string(wcfg, str8_lit("split_x")) != &rd_nil_cfg ? Axis2_X : Axis2_Y;
    RD_CfgRec rec = {0};
    RD_PanelNode *dst_active_parent = &rd_nil_panel_node;
    for(RD_Cfg *src = src_root; src != &rd_nil_cfg; src = rec.next)
    {
      // rjf: build a panel node
      RD_PanelNode *dst = push_array(arena, RD_PanelNode, 1);
      MemoryCopyStruct(dst, &rd_nil_panel_node);
      dst->parent = dst_active_parent;
      if(dst_active_parent != &rd_nil_panel_node)
      {
        DLLPushBack_NPZ(&rd_nil_panel_node, dst_active_parent->first, dst_active_parent->last, dst, next, prev);
        dst_active_parent->child_count += 1;
      }
      if(dst_root == &rd_nil_panel_node)
      {
        dst_root = dst;
      }
      
      // rjf: extract cfg info
      B32 panel_has_children = 0;
      dst->cfg = src;
      dst->pct_of_parent = (src == src_root ? 1.f : (F32)f64_from_str8(src->string));
      dst->tab_side = (rd_cfg_child_from_string(src, str8_lit("tabs_on_bottom")) != &rd_nil_cfg ? Side_Max : Side_Min);
      dst->split_axis = active_split_axis;
      for(RD_Cfg *src_child = src->first; src_child != &rd_nil_cfg; src_child = src_child->next)
      {
        MD_TokenizeResult tokenize = md_tokenize_from_text(scratch.arena, src_child->string);
        if(tokenize.tokens.count == 1 && tokenize.tokens.v[0].flags & MD_TokenFlag_Numeric)
        {
          panel_has_children = 1;
        }
        else if(str8_match(src_child->string, str8_lit("tabs_on_bottom"), 0))
        {
          // NOTE(rjf): skip - this is a panel option.
        }
        else if(str8_match(src_child->string, str8_lit("selected"), 0))
        {
          dst_focused = dst;
        }
        else if(tokenize.tokens.count == 1 && tokenize.tokens.v[0].flags & MD_TokenFlag_Identifier)
        {
          rd_cfg_list_push(arena, &dst->tabs, src_child);
          if(rd_cfg_child_from_string(src_child, str8_lit("selected")) != &rd_nil_cfg)
          {
            dst->selected_tab = src_child;
          }
        }
      }
      
      // rjf: recurse
      rec = rd_cfg_rec__depth_first(src_root, src);
      if(!panel_has_children)
      {
        MemoryZeroStruct(&rec);
        rec.next = &rd_nil_cfg;
        for(RD_Cfg *p = src; p != src_root && p != &rd_nil_cfg; p = p->parent, rec.pop_count += 1)
        {
          if(p->next != &rd_nil_cfg)
          {
            rec.next = p->next;
            break;
          }
        }
      }
      if(rec.push_count > 0)
      {
        dst_active_parent = dst;
        active_split_axis = axis2_flip(active_split_axis);
      }
      else for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
      {
        dst_active_parent = dst_active_parent->parent;
        active_split_axis = axis2_flip(active_split_axis);
      }
    }
  }
  scratch_end(scratch);
  RD_PanelTree tree = {dst_root, dst_focused};
  return tree;
}

internal RD_PanelNodeRec
rd_panel_node_rec__depth_first(RD_PanelNode *root, RD_PanelNode *panel, U64 sib_off, U64 child_off)
{
  RD_PanelNodeRec rec = {&rd_nil_panel_node};
  if(*MemberFromOffset(RD_PanelNode **, panel, child_off) != &rd_nil_panel_node)
  {
    rec.next = *MemberFromOffset(RD_PanelNode **, panel, child_off);
    rec.push_count += 1;
  }
  else for(RD_PanelNode *p = panel; p != &rd_nil_panel_node && p != root; p = p->parent, rec.pop_count += 1)
  {
    if(*MemberFromOffset(RD_PanelNode **, p, sib_off) != &rd_nil_panel_node)
    {
      rec.next = *MemberFromOffset(RD_PanelNode **, p, sib_off);
      break;
    }
  }
  return rec;
}

internal RD_PanelNode *
rd_panel_node_from_tree_cfg(RD_PanelNode *root, RD_Cfg *cfg)
{
  RD_PanelNode *result = &rd_nil_panel_node;
  for(RD_PanelNode *p = root;
      p != &rd_nil_panel_node;
      p = rd_panel_node_rec__depth_first_pre(root, p).next)
  {
    if(p->cfg == cfg)
    {
      result = p;
      break;
    }
  }
  return result;
}

internal Rng2F32
rd_target_rect_from_panel_node_child(Rng2F32 parent_rect, RD_PanelNode *parent, RD_PanelNode *panel)
{
  Rng2F32 rect = parent_rect;
  if(parent != &rd_nil_panel_node)
  {
    Vec2F32 parent_rect_size = dim_2f32(parent_rect);
    Axis2 axis = parent->split_axis;
    rect.p1.v[axis] = rect.p0.v[axis];
    for(RD_PanelNode *child = parent->first; child != &rd_nil_panel_node; child = child->next)
    {
      rect.p1.v[axis] += parent_rect_size.v[axis] * child->pct_of_parent;
      if(child == panel)
      {
        break;
      }
      rect.p0.v[axis] = rect.p1.v[axis];
    }
    //rect.p0.v[axis] += parent_rect_size.v[axis] * panel->off_pct_of_parent.v[axis];
    //rect.p0.v[axis2_flip(axis)] += parent_rect_size.v[axis2_flip(axis)] * panel->off_pct_of_parent.v[axis2_flip(axis)];
  }
  rect.x0 = round_f32(rect.x0);
  rect.x1 = round_f32(rect.x1);
  rect.y0 = round_f32(rect.y0);
  rect.y1 = round_f32(rect.y1);
  return rect;
}

internal Rng2F32
rd_target_rect_from_panel_node(Rng2F32 root_rect, RD_PanelNode *root, RD_PanelNode *panel)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: count ancestors
  U64 ancestor_count = 0;
  for(RD_PanelNode *p = panel->parent; p != &rd_nil_panel_node; p = p->parent)
  {
    ancestor_count += 1;
  }
  
  // rjf: gather ancestors
  RD_PanelNode **ancestors = push_array(scratch.arena, RD_PanelNode *, ancestor_count);
  {
    U64 ancestor_idx = 0;
    for(RD_PanelNode *p = panel->parent; p != &rd_nil_panel_node; p = p->parent)
    {
      ancestors[ancestor_idx] = p;
      ancestor_idx += 1;
    }
  }
  
  // rjf: go from highest ancestor => panel and calculate rect
  Rng2F32 parent_rect = root_rect;
  for(S64 ancestor_idx = (S64)ancestor_count-1;
      0 <= ancestor_idx && ancestor_idx < ancestor_count;
      ancestor_idx -= 1)
  {
    RD_PanelNode *ancestor = ancestors[ancestor_idx];
    RD_PanelNode *parent = ancestor->parent;
    if(parent != &rd_nil_panel_node)
    {
      parent_rect = rd_target_rect_from_panel_node_child(parent_rect, parent, ancestor);
    }
  }
  
  // rjf: calculate final rect
  Rng2F32 rect = rd_target_rect_from_panel_node_child(parent_rect, panel->parent, panel);
  
  scratch_end(scratch);
  return rect;
}

internal B32
rd_cfg_is_project_filtered(RD_Cfg *cfg)
{
  RD_Cfg *project = rd_cfg_child_from_string(cfg, str8_lit("project"));
  B32 result = path_match_normalized(rd_state->project_path, project->first->string);
  return result;
}

internal RD_KeyMapNodePtrList
rd_key_map_node_ptr_list_from_name(Arena *arena, String8 string)
{
  RD_KeyMapNodePtrList list = {0};
  {
    U64 hash = d_hash_from_string(string);
    U64 slot_idx = hash%rd_state->key_map->name_slots_count;
    for(RD_KeyMapNode *n = rd_state->key_map->name_slots[slot_idx].first; n != 0; n = n->name_hash_next)
    {
      if(str8_match(n->name, string, 0))
      {
        RD_KeyMapNodePtr *ptr = push_array(arena, RD_KeyMapNodePtr, 1);
        ptr->v = n;
        SLLQueuePush(list.first, list.last, ptr);
        list.count += 1;
      }
    }
  }
  return list;
}

internal RD_KeyMapNodePtrList
rd_key_map_node_ptr_list_from_binding(Arena *arena, RD_Binding binding)
{
  RD_KeyMapNodePtrList list = {0};
  {
    U64 hash = d_hash_from_string(str8_struct(&binding));
    U64 slot_idx = hash%rd_state->key_map->binding_slots_count;
    for(RD_KeyMapNode *n = rd_state->key_map->binding_slots[slot_idx].first; n != 0; n = n->binding_hash_next)
    {
      if(MemoryMatchStruct(&binding, &n->binding))
      {
        RD_KeyMapNodePtr *ptr = push_array(arena, RD_KeyMapNodePtr, 1);
        ptr->v = n;
        SLLQueuePush(list.first, list.last, ptr);
        list.count += 1;
      }
    }
  }
  return list;
}

internal Vec4F32
rd_hsva_from_cfg(RD_Cfg *cfg)
{
  Vec4F32 hsva = {0};
  RD_Cfg *hsva_root = rd_cfg_child_from_string(cfg, str8_lit("hsva"));
  RD_Cfg *h = hsva_root->first;
  RD_Cfg *s = h->next;
  RD_Cfg *v = s->next;
  RD_Cfg *a = v->next;
  hsva.x = (F32)f64_from_str8(h->string);
  hsva.y = (F32)f64_from_str8(s->string);
  hsva.z = (F32)f64_from_str8(v->string);
  hsva.w = (F32)f64_from_str8(a->string);
  return hsva;
}

internal Vec4F32
rd_rgba_from_cfg(RD_Cfg *cfg)
{
  Vec4F32 hsva = rd_hsva_from_cfg(cfg);
  Vec4F32 rgba = rgba_from_hsva(hsva);
  return rgba;
}

internal B32
rd_disabled_from_cfg(RD_Cfg *cfg)
{
  B32 is_disabled = (rd_cfg_child_from_string(cfg, str8_lit("disabled")) != &rd_nil_cfg);
  return is_disabled;
}

internal RD_Location
rd_location_from_cfg(RD_Cfg *cfg)
{
  RD_Location dst_loc = {0};
  {
    RD_Cfg *src_loc = rd_cfg_child_from_string(cfg, str8_lit("location"));
    if(src_loc->first != &rd_nil_cfg && src_loc->first->first != &rd_nil_cfg)
    {
      dst_loc.file_path = src_loc->first->string;
      try_s64_from_str8_c_rules(src_loc->first->first->string, &dst_loc.pt.line);
      if(!try_s64_from_str8_c_rules(src_loc->first->first->first->string, &dst_loc.pt.column))
      {
        dst_loc.pt.column = 1;
      }
    }
    else
    {
      Temp scratch = scratch_begin(0, 0);
      MD_TokenizeResult tokenize = md_tokenize_from_text(scratch.arena, src_loc->first->string);
      if(tokenize.tokens.count == 1 && tokenize.tokens.v[0].flags & (MD_TokenFlag_Identifier|MD_TokenFlag_StringLiteral))
      {
        dst_loc.name = src_loc->first->string;
      }
      else if(tokenize.tokens.count == 1 && tokenize.tokens.v[0].flags & MD_TokenFlag_Numeric)
      {
        try_u64_from_str8_c_rules(src_loc->first->string, &dst_loc.vaddr);
      }
      scratch_end(scratch);
    }
  }
  return dst_loc;
}

internal String8
rd_label_from_cfg(RD_Cfg *cfg)
{
  RD_Cfg *label_root = rd_cfg_child_from_string(cfg, str8_lit("label"));
  String8 result = label_root->first->string;
  return result;
}

internal String8
rd_expr_from_cfg(RD_Cfg *cfg)
{
  RD_Cfg *expr_root = rd_cfg_child_from_string(cfg, str8_lit("expression"));
  String8 result = expr_root->first->string;
  return result;
}

internal D_Target
rd_target_from_cfg(Arena *arena, RD_Cfg *cfg)
{
  D_Target target = {0};
  target.exe                        = rd_cfg_child_from_string(cfg, str8_lit("executable"))->first->string;
  target.args                       = rd_cfg_child_from_string(cfg, str8_lit("arguments"))->first->string;
  target.working_directory          = rd_cfg_child_from_string(cfg, str8_lit("working_directory"))->first->string;
  target.custom_entry_point_name    = rd_cfg_child_from_string(cfg, str8_lit("entry_point"))->first->string;
  target.stdout_path                = rd_cfg_child_from_string(cfg, str8_lit("stdout_path"))->first->string;
  target.stderr_path                = rd_cfg_child_from_string(cfg, str8_lit("stderr_path"))->first->string;
  target.stdin_path                 = rd_cfg_child_from_string(cfg, str8_lit("stdin_path"))->first->string;
  target.debug_subprocesses         = (rd_cfg_child_from_string(cfg, str8_lit("debug_subprocesses")) != &rd_nil_cfg);
  for(RD_Cfg *child = cfg->first; child != &rd_nil_cfg; child = child->next)
  {
    if(str8_match(child->string, str8_lit("environment"), 0))
    {
      str8_list_push(arena, &target.env, child->first->string);
    }
  }
  return target;
}

internal DR_FStrList
rd_title_fstrs_from_cfg(Arena *arena, RD_Cfg *cfg, Vec4F32 secondary_color, F32 size)
{
  DR_FStrList result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: unpack config
    B32 is_disabled = rd_disabled_from_cfg(cfg);
    RD_Location loc = rd_location_from_cfg(cfg);
    D_Target target = rd_target_from_cfg(scratch.arena, cfg);
    String8 label_string = rd_label_from_cfg(cfg);
    String8 expr_string = rd_expr_from_cfg(cfg);
    String8 collection_name = {0};
    String8 file_path = {0};
    Vec4F32 rgba = rd_rgba_from_cfg(cfg);
    if(rgba.w == 0)
    {
      rgba = ui_top_palette()->text;
    }
    RD_IconKind icon_kind = rd_icon_kind_from_code_name(cfg->string);
    B32 is_from_command_line = 0;
    {
      RD_Cfg *cmd_line_root = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("command_line"));
      for(RD_Cfg *p = cfg->parent; p != &rd_nil_cfg; p = p->parent)
      {
        if(p == cmd_line_root)
        {
          is_from_command_line = 1;
          break;
        }
      }
    }
    B32 is_within_window = 0;
    {
      for(RD_Cfg *p = cfg->parent; p != &rd_nil_cfg; p = p->parent)
      {
        if(str8_match(p->string, str8_lit("window"), 0))
        {
          is_within_window = 1;
          break;
        }
      }
    }
    if(expr_string.size != 0)
    {
      String8 query_name = rd_query_from_eval_string(arena, expr_string);
      if(query_name.size != 0 && !str8_match(query_name, str8_lit("watches"), 0))
      {
        String8 query_code_name = query_name;
        String8 query_display_name = rd_display_from_code_name(query_code_name);
        collection_name = query_display_name;
        if(query_display_name.size == 0)
        {
          query_code_name = rd_singular_from_code_name_plural(query_name);
          collection_name = rd_display_plural_from_code_name(query_code_name);
        }
        RD_IconKind query_icon_kind = rd_icon_kind_from_code_name(query_code_name);
        if(query_icon_kind != RD_IconKind_Null)
        {
          icon_kind = query_icon_kind;
        }
      }
      else
      {
        file_path = rd_file_path_from_eval_string(arena, expr_string);
        if(file_path.size != 0)
        {
          icon_kind = RD_IconKind_FileOutline;
        }
      }
    }
    
    //- rjf: set up color/size for all parts of the title
    //
    // the "running" part implies that it changes as things are added - 
    // so if a primary title is pushed, we can make the rest of the title
    // more faded/smaller, but only after a primary title is pushed,
    // which could be caused by many different potential parts of a cfg.
    //
    DR_FStrParams params = {rd_font_from_slot(RD_FontSlot_Main), rd_raster_flags_from_slot(RD_FontSlot_Main), rgba, size};
    B32 running_is_secondary = 0;
#define start_secondary() if(!running_is_secondary){running_is_secondary = 1; params.color = secondary_color; params.size = size*0.8f;}
    
    //- rjf: push icon
    if(icon_kind != RD_IconKind_Null)
    {
      dr_fstrs_push_new(arena, &result, &params, rd_icon_kind_text_table[icon_kind], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons), .color = secondary_color);
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
    }
    
    //- rjf: push warning icon for command-line entities
    if(is_from_command_line)
    {
      dr_fstrs_push_new(arena, &result, &params, rd_icon_kind_text_table[RD_IconKind_Info], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons), .color = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative));
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
    }
    
    //- rjf: push view title, if from window, and no file path
    if(is_within_window && file_path.size == 0 && collection_name.size == 0)
    {
      String8 view_display_name = rd_display_from_code_name(cfg->string);
      if(view_display_name.size != 0)
      {
        dr_fstrs_push_new(arena, &result, &params, view_display_name);
        dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
        start_secondary();
      }
    }
    
    //- rjf: push label
    if(label_string.size != 0)
    {
      dr_fstrs_push_new(arena, &result, &params, label_string, .font = rd_font_from_slot(RD_FontSlot_Code));
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      start_secondary();
    }
    
    //- rjf: push expression
    if(collection_name.size != 0)
    {
      dr_fstrs_push_new(arena, &result, &params, collection_name);
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      start_secondary();
    }
    
    //- rjf: query is file path - do specific file name strings
    else if(file_path.size != 0)
    {
      // rjf: compute disambiguated file name
      String8List qualifiers = {0};
      String8 file_name = str8_skip_last_slash(file_path);
      if(rd_state->ambiguous_path_slots_count != 0)
      {
        U64 hash = d_hash_from_string__case_insensitive(file_name);
        U64 slot_idx = hash%rd_state->ambiguous_path_slots_count;
        RD_AmbiguousPathNode *node = 0;
        {
          for(RD_AmbiguousPathNode *n = rd_state->ambiguous_path_slots[slot_idx];
              n != 0;
              n = n->next)
          {
            if(str8_match(n->name, file_name, StringMatchFlag_CaseInsensitive))
            {
              node = n;
              break;
            }
          }
        }
        if(node != 0 && node->paths.node_count > 1)
        {
          // rjf: get all colliding paths
          String8Array collisions = str8_array_from_list(scratch.arena, &node->paths);
          
          // rjf: get all reversed path parts for each collision
          String8List *collision_parts_reversed = push_array(scratch.arena, String8List, collisions.count);
          for EachIndex(idx, collisions.count)
          {
            String8List parts = str8_split_path(scratch.arena, collisions.v[idx]);
            for(String8Node *n = parts.first; n != 0; n = n->next)
            {
              str8_list_push_front(scratch.arena, &collision_parts_reversed[idx], n->string);
            }
          }
          
          // rjf: get the search path & its reversed parts
          String8List parts = str8_split_path(scratch.arena, file_path);
          String8List parts_reversed = {0};
          for(String8Node *n = parts.first; n != 0; n = n->next)
          {
            str8_list_push_front(scratch.arena, &parts_reversed, n->string);
          }
          
          // rjf: iterate all collision part reversed lists, in lock-step with
          // search path; disqualify until we only have one path remaining; gather
          // qualifiers
          {
            U64 num_collisions_left = collisions.count;
            String8Node **collision_nodes = push_array(scratch.arena, String8Node *, collisions.count);
            for EachIndex(idx, collisions.count)
            {
              collision_nodes[idx] = collision_parts_reversed[idx].first;
            }
            for(String8Node *n = parts_reversed.first; num_collisions_left > 1 && n != 0; n = n->next)
            {
              B32 part_is_qualifier = 0;
              for EachIndex(idx, collisions.count)
              {
                if(collision_nodes[idx] != 0 && !str8_match(collision_nodes[idx]->string, n->string, StringMatchFlag_CaseInsensitive))
                {
                  collision_nodes[idx] = 0;
                  num_collisions_left -= 1;
                  part_is_qualifier = 1;
                }
                else if(collision_nodes[idx] != 0)
                {
                  collision_nodes[idx] = collision_nodes[idx]->next;
                }
              }
              if(part_is_qualifier)
              {
                str8_list_push_front(scratch.arena, &qualifiers, n->string);
              }
            }
          }
        }
      }
      
      // rjf: push qualifiers
      for(String8Node *n = qualifiers.first; n != 0; n = n->next)
      {
        String8 string = push_str8f(arena, "<%S> ", n->string);
        dr_fstrs_push_new(arena, &result, &params, string, .color = secondary_color);
      }
      
      // rjf: push file name
      dr_fstrs_push_new(arena, &result, &params, push_str8_copy(arena, str8_skip_last_slash(file_path)));
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      start_secondary();
    }
    
    //- rjf: cfg has expression attached -> use that
    else if(expr_string.size != 0 && !str8_match(cfg->string, str8_lit("watch"), 0))
    {
      dr_fstrs_push_new(arena, &result, &params, expr_string);
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      start_secondary();
    }
    
    //- rjf: push text location
    if(loc.file_path.size != 0)
    {
      String8 location_string = push_str8f(arena, "%S:%I64d:%I64d", str8_skip_last_slash(loc.file_path), loc.pt.line, loc.pt.column);
      dr_fstrs_push_new(arena, &result, &params, location_string);
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      start_secondary();
    }
    
    //- rjf: push target executable name
    if(target.exe.size != 0)
    {
      dr_fstrs_push_new(arena, &result, &params, str8_skip_last_slash(target.exe));
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      start_secondary();
    }
    
    //- rjf: push target arguments
    if(target.args.size != 0)
    {
      dr_fstrs_push_new(arena, &result, &params, target.args);
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
    }
    
    //- rjf: push conditions
    {
      String8 condition = rd_cfg_child_from_string(cfg, str8_lit("condition"))->first->string;
      if(condition.size != 0)
      {
        dr_fstrs_push_new(arena, &result, &params, str8_lit("if "), .font = rd_font_from_slot(RD_FontSlot_Code));
        DR_FStrList fstrs = rd_fstrs_from_code_string(arena, 1.f, 0, params.color, condition);
        dr_fstrs_concat_in_place(&result, &fstrs);
        dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      }
    }
    
    //- rjf: push disabled marker
    if(is_disabled)
    {
      dr_fstrs_push_new(arena, &result, &params, str8_lit("(Disabled)"));
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
    }
    
    //- rjf: push hit count
    {
      String8 hit_count_value_string = rd_cfg_child_from_string(cfg, str8_lit("hit_count"))->first->string;
      U64 hit_count = 0;
      if(try_u64_from_str8_c_rules(hit_count_value_string, &hit_count))
      {
        String8 hit_count_text = push_str8f(arena, "(%I64u hit%s)", hit_count, hit_count == 1 ? "" : "s");
        dr_fstrs_push_new(arena, &result, &params, hit_count_text);
      }
    }
    
    //- rjf: special case: auto view rule
    if(str8_match(cfg->string, str8_lit("auto_view_rule"), 0))
    {
      String8 src_string = rd_cfg_child_from_string(cfg, str8_lit("source"))->first->string;
      String8 dst_string = rd_cfg_child_from_string(cfg, str8_lit("dest"))->first->string;
      Vec4F32 src_color = rgba;
      Vec4F32 dst_color = rgba;
      DR_FStrList src_fstrs = {0};
      DR_FStrList dst_fstrs = {0};
      if(src_string.size == 0)
      {
        src_string = str8_lit("(type)");
        src_color = secondary_color;
        dr_fstrs_push_new(arena, &src_fstrs, &params, src_string, .color = src_color);
      }
      else RD_Font(RD_FontSlot_Code)
      {
        src_fstrs = rd_fstrs_from_code_string(arena, 1.f, 0, src_color, src_string);
      }
      if(dst_string.size == 0)
      {
        dst_string = str8_lit("(view rule)");
        dst_color = secondary_color;
        dr_fstrs_push_new(arena, &dst_fstrs, &params, dst_string, .color = dst_color);
      }
      else RD_Font(RD_FontSlot_Code)
      {
        dst_fstrs = rd_fstrs_from_code_string(arena, 1.f, 0, dst_color, dst_string);
      }
      dr_fstrs_concat_in_place(&result, &src_fstrs);
      dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
      dr_fstrs_push_new(arena, &result, &params, rd_icon_kind_text_table[RD_IconKind_RightArrow], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons), .color = secondary_color);
      dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
      dr_fstrs_concat_in_place(&result, &dst_fstrs);
    }
    
    //- rjf: special case: file path maps
    if(str8_match(cfg->string, str8_lit("file_path_map"), 0))
    {
      String8 src_string = rd_cfg_child_from_string(cfg, str8_lit("source"))->first->string;
      String8 dst_string = rd_cfg_child_from_string(cfg, str8_lit("dest"))->first->string;
      Vec4F32 src_color = rgba;
      Vec4F32 dst_color = rgba;
      if(src_string.size == 0)
      {
        src_string = str8_lit("(source path)");
        src_color = secondary_color;
      }
      if(dst_string.size == 0)
      {
        dst_string = str8_lit("(destination path)");
        dst_color = secondary_color;
      }
      dr_fstrs_push_new(arena, &result, &params, src_string, .color = src_color);
      dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
      dr_fstrs_push_new(arena, &result, &params, rd_icon_kind_text_table[RD_IconKind_RightArrow], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons), .color = secondary_color);
      dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
      dr_fstrs_push_new(arena, &result, &params, dst_string, .color = dst_color);
    }
    
#undef start_secondary
    scratch_end(scratch);
  }
  return result;
}

internal MD_Node *
rd_schema_from_name(Arena *arena, String8 name)
{
  MD_Node *schema = &md_nil_node;
  for EachElement(idx, rd_name_schema_info_table)
  {
    if(str8_match(name, rd_name_schema_info_table[idx].name, 0))
    {
      schema = rd_state->schemas[idx];
      break;
    }
  }
  return schema;
}

internal String8
rd_setting_from_name(String8 name)
{
  String8 result = {0};
  {
    // rjf: find most-granular config scope to begin looking for the setting
    RD_Cfg *start_cfg = rd_cfg_from_id(rd_regs()->view);
    for(RD_Cfg *p = start_cfg->parent; p != &rd_nil_cfg; p = p->parent)
    {
      if(str8_match(p->string, str8_lit("transient"), 0))
      {
        start_cfg = &rd_nil_cfg;
        break;
      }
    }
    if(start_cfg == &rd_nil_cfg) { start_cfg = rd_cfg_from_id(rd_regs()->panel); }
    if(start_cfg == &rd_nil_cfg) { start_cfg = rd_cfg_from_id(rd_regs()->window); }
    
    // rjf: scan upwards the config tree until we find the setting
    RD_Cfg *setting = &rd_nil_cfg;
    for(RD_Cfg *cfg = start_cfg; cfg != &rd_nil_cfg && setting == &rd_nil_cfg; cfg = cfg->parent)
    {
      setting = rd_cfg_child_from_string(cfg, name);
    }
    
    // rjf: return resultant child string stored under this key
    result = setting->first->string;
    
    // rjf: no result -> look for default in settings schema
    if(result.size == 0) ProfScope("default setting schema lookup")
    {
      Temp scratch = scratch_begin(0, 0);
      MD_Node *schema = rd_schema_from_name(scratch.arena, str8_lit("settings"));
      MD_Node *setting = md_child_from_string(schema, name, 0);
      MD_Node *default_tag = md_tag_from_string(setting, str8_lit("default"), 0);
      result = default_tag->first->string;
      scratch_end(scratch);
    }
  }
  return result;
}

internal RD_Cfg *
rd_immediate_cfg_from_key(String8 string)
{
  RD_Cfg *transient = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("transient"));
  RD_Cfg *immediate = &rd_nil_cfg;
  RD_Cfg *cfg = &rd_nil_cfg;
  for(RD_Cfg *child = transient->first; child != &rd_nil_cfg; child = child->next)
  {
    if(str8_match(child->string, str8_lit("immediate"), 0))
    {
      cfg = rd_cfg_child_from_string(child, string);
      if(cfg != &rd_nil_cfg)
      {
        immediate = child;
        break;
      }
    }
  }
  if(cfg == &rd_nil_cfg)
  {
    immediate = rd_cfg_new(transient, str8_lit("immediate"));
    cfg = rd_cfg_new(immediate, string);
  }
  rd_cfg_new(immediate, str8_lit("hot"));
  return cfg;
}

internal RD_Cfg *
rd_immediate_cfg_from_keyf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 key = push_str8fv(scratch.arena, fmt, args);
  RD_Cfg *result = rd_immediate_cfg_from_key(key);
  va_end(args);
  scratch_end(scratch);
  return result;
}

internal String8
rd_mapped_from_file_path(Arena *arena, String8 file_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 result = file_path;
  if(file_path.size != 0)
  {
    String8 file_path__normalized = path_normalized_from_string(scratch.arena, file_path);
    String8List file_path_parts = str8_split_path(scratch.arena, file_path__normalized);
    RD_CfgList maps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("file_path_map"));
    String8 best_map_dst = {0};
    U64 best_map_match_length = max_U64;
    String8Node *best_map_remaining_suffix_first = 0;
    for(RD_CfgNode *n = maps.first; n != 0; n = n->next)
    {
      String8 map_src = rd_cfg_child_from_string(n->v, str8_lit("source"))->first->string;
      String8 map_src__normalized = path_normalized_from_string(scratch.arena, map_src);
      String8List map_src_parts = str8_split_path(scratch.arena, map_src__normalized);
      B32 matches = 1;
      U64 match_length = 0;
      String8Node *file_path_part_n = file_path_parts.first;
      for(String8Node *map_src_n = map_src_parts.first;
          map_src_n != 0 && file_path_part_n != 0;
          map_src_n = map_src_n->next, file_path_part_n = file_path_part_n->next)
      {
        if(!str8_match(map_src_n->string, file_path_part_n->string, 0))
        {
          matches = 0;
          break;
        }
        match_length += 1;
      }
      if(matches && match_length < best_map_match_length)
      {
        best_map_match_length = match_length;
        best_map_dst = rd_cfg_child_from_string(n->v, str8_lit("dest"))->first->string;
        best_map_remaining_suffix_first = file_path_part_n;
      }
    }
    if(best_map_dst.size != 0)
    {
      String8 best_map_dst__normalized = path_normalized_from_string(scratch.arena, best_map_dst);
      String8List best_map_dst_parts = str8_split_path(scratch.arena, best_map_dst__normalized);
      for(String8Node *n = best_map_remaining_suffix_first; n != 0; n = n->next)
      {
        str8_list_push(scratch.arena, &best_map_dst_parts, n->string);
      }
      StringJoin join = {.sep = str8_lit("/")};
      result = str8_list_join(arena, &best_map_dst_parts, &join);
    }
    else
    {
      result = path_normalized_from_string(arena, result);
    }
  }
  scratch_end(scratch);
  return result;
}

internal String8List
rd_possible_overrides_from_file_path(Arena *arena, String8 file_path)
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
    RD_CfgList links = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("file_path_map"));
    for(RD_CfgNode *n = links.first; n != 0; n = n->next)
    {
      //- rjf: unpack link
      RD_Cfg *link = n->v;
      RD_Cfg *src = rd_cfg_child_from_string(link, str8_lit("source"));
      RD_Cfg *dst = rd_cfg_child_from_string(link, str8_lit("dest"));
      PathStyle src_style = PathStyle_Relative;
      PathStyle dst_style = PathStyle_Relative;
      String8List src_parts = path_normalized_list_from_string(scratch.arena, src->first->string, &src_style);
      String8List dst_parts = path_normalized_list_from_string(scratch.arena, dst->first->string, &dst_style);
      
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
        String8 candidate_path = str8_list_join(arena, &candidate_parts, &join);
        str8_list_push(arena, &result, candidate_path);
      }
    }
  }
  
  scratch_end(scratch);
  return result;
}

internal E_Expr *
rd_tag_from_cfg(Arena *arena, RD_Cfg *cfg)
{
  E_Expr *expr = &e_expr_nil;
  {
    // TODO(rjf): @cfg
  }
  return expr;
}

////////////////////////////////
//~ rjf: Control Entity Info Extraction

internal Vec4F32
rd_rgba_from_ctrl_entity(CTRL_Entity *entity)
{
  Vec4F32 result = rd_rgba_from_theme_color(RD_ThemeColor_Text);
  if(entity->rgba != 0)
  {
    result = rgba_from_u32(entity->rgba);
  }
  if(entity->rgba == 0) switch(entity->kind)
  {
    default:{}break;
    case CTRL_EntityKind_Thread:
    {
      CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity, CTRL_EntityKind_Process);
      CTRL_Entity *main_thread = ctrl_entity_child_from_kind(process, CTRL_EntityKind_Thread);
      if(main_thread != entity)
      {
        result = rd_rgba_from_theme_color(RD_ThemeColor_Thread1);
      }
      else
      {
        result = rd_rgba_from_theme_color(RD_ThemeColor_Thread0);
      }
    }break;
  }
  return result;
}

internal String8
rd_name_from_ctrl_entity(Arena *arena, CTRL_Entity *entity)
{
  String8 string = entity->string;
  if(string.size == 0)
  {
    string = str8_lit("unnamed");
  }
  if(entity->kind == CTRL_EntityKind_Module)
  {
    string = str8_skip_last_slash(string);
  }
  return string;
}

internal DR_FStrList
rd_title_fstrs_from_ctrl_entity(Arena *arena, CTRL_Entity *entity, Vec4F32 secondary_color, F32 size, B32 include_extras)
{
  DR_FStrList result = {0};
  
  //- rjf: unpack entity info
  F32 extras_size = size*0.95f;
  Vec4F32 color = rd_rgba_from_ctrl_entity(entity);
  String8 name = rd_name_from_ctrl_entity(arena, entity);
  RD_IconKind icon_kind = RD_IconKind_Null;
  B32 name_is_code = 0;
  switch(entity->kind)
  {
    default:{}break;
    case CTRL_EntityKind_Machine: {icon_kind = RD_IconKind_Machine;}break;
    case CTRL_EntityKind_Process: {icon_kind = RD_IconKind_Threads;}break;
    case CTRL_EntityKind_Thread:  {icon_kind = RD_IconKind_Thread; name_is_code = 1;}break;
    case CTRL_EntityKind_Module:  {icon_kind = RD_IconKind_Module;}break;
  }
  
  //- rjf: set up drawing params
  DR_FStrParams params = {rd_font_from_slot(RD_FontSlot_Code), rd_raster_flags_from_slot(RD_FontSlot_Code), rd_rgba_from_theme_color(RD_ThemeColor_Text), size};
  
  //- rjf: push icon
  if(icon_kind != RD_IconKind_Null)
  {
    dr_fstrs_push_new(arena, &result, &params, rd_icon_kind_text_table[icon_kind], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons), .color = secondary_color);
    dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
  }
  
  //- rjf: push containing process prefix
  if(entity->kind == CTRL_EntityKind_Thread ||
     entity->kind == CTRL_EntityKind_Module)
  {
    CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
    if(processes.count > 1)
    {
      CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity, CTRL_EntityKind_Process);
      String8 process_name = rd_name_from_ctrl_entity(arena, process);
      Vec4F32 process_color = rd_rgba_from_ctrl_entity(process);
      if(process_name.size != 0)
      {
        dr_fstrs_push_new(arena, &result, &params, process_name, .font = rd_font_from_slot(RD_FontSlot_Main), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Main), .color = process_color);
        dr_fstrs_push_new(arena, &result, &params, str8_lit(" / "), .color = secondary_color);
      }
    }
  }
  
  //- rjf: push name
  dr_fstrs_push_new(arena, &result, &params, name,
                    .font         = rd_font_from_slot(name_is_code ? RD_FontSlot_Code : RD_FontSlot_Main),
                    .raster_flags = rd_raster_flags_from_slot(name_is_code ? RD_FontSlot_Code : RD_FontSlot_Main),
                    .color        = color);
  
  //- rjf: threads get callstack extras
  if(entity->kind == CTRL_EntityKind_Thread && include_extras)
  {
    dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
    DI_Scope *di_scope = di_scope_open();
    CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity, CTRL_EntityKind_Process);
    Arch arch = entity->arch;
    CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity);
    for(U64 idx = 0, limit = 6; idx < unwind.frames.count && idx < limit; idx += 1)
    {
      CTRL_UnwindFrame *f = &unwind.frames.v[unwind.frames.count - 1 - idx];
      U64 rip_vaddr = regs_rip_from_arch_block(arch, f->regs);
      CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
      U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
      DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
      RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
      if(rdi != &di_rdi_parsed_nil)
      {
        RDI_Procedure *procedure = rdi_procedure_from_voff(rdi, rip_voff);
        String8 name = {0};
        name.str = rdi_string_from_idx(rdi, procedure->name_string_idx, &name.size);
        name = push_str8_copy(arena, name);
        if(name.size != 0)
        {
          dr_fstrs_push_new(arena, &result, &params, name, .size = extras_size, .color = rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol));
          if(idx+1 < unwind.frames.count)
          {
            dr_fstrs_push_new(arena, &result, &params, str8_lit(" > "), .color = secondary_color, .size = extras_size);
            if(idx+1 == limit)
            {
              dr_fstrs_push_new(arena, &result, &params, str8_lit("..."), .color = secondary_color, .size = extras_size);
            }
          }
        }
      }
    }
    di_scope_close(di_scope);
  }
  
  //- rjf: modules get debug info status extras
  if(entity->kind == CTRL_EntityKind_Module && include_extras)
  {
    DI_Scope *di_scope = di_scope_open();
    DI_Key dbgi_key = ctrl_dbgi_key_from_module(entity);
    RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
    if(rdi->raw_data_size == 0)
    {
      dr_fstrs_push_new(arena, &result, &params, str8_lit(" "));
      dr_fstrs_push_new(arena, &result, &params, str8_lit("(Symbols not found)"), .font = rd_font_from_slot(RD_FontSlot_Main), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Main), .size = extras_size, .color = secondary_color);
    }
    di_scope_close(di_scope);
  }
  
  return result;
}

////////////////////////////////
//~ rjf: Evaluation Spaces

//- rjf: cfg <-> eval space

internal RD_Cfg *
rd_cfg_from_eval_space(E_Space space)
{
  RD_Cfg *cfg = &rd_nil_cfg;
  if(space.kind == RD_EvalSpaceKind_MetaCfg)
  {
    RD_CfgID id = space.u64s[0];
    cfg = rd_cfg_from_id(id);
  }
  return cfg;
}

internal E_Space
rd_eval_space_from_cfg(RD_Cfg *cfg)
{
  E_Space space = e_space_make(RD_EvalSpaceKind_MetaCfg);
  space.u64s[0] = cfg->id;
  return space;
}

//- rjf: ctrl entity <-> eval space

internal CTRL_Entity *
rd_ctrl_entity_from_eval_space(E_Space space)
{
  CTRL_Entity *entity = &ctrl_entity_nil;
  if(space.kind == RD_EvalSpaceKind_CtrlEntity ||
     space.kind == RD_EvalSpaceKind_MetaCtrlEntity)
  {
    CTRL_Handle handle;
    handle.machine_id = space.u64s[0];
    handle.dmn_handle.u64[0] = space.u64s[1];
    entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, handle);
  }
  return entity;
}

internal E_Space
rd_eval_space_from_ctrl_entity(CTRL_Entity *entity, E_SpaceKind kind)
{
  E_Space space = e_space_make(kind);
  space.u64s[0] = entity->handle.machine_id;
  space.u64s[1] = entity->handle.dmn_handle.u64[0];
  return space;
}

//- rjf: cfg -> eval blob

internal String8
rd_eval_blob_from_cfg(Arena *arena, RD_Cfg *cfg)
{
  String8 result = {0};
  String8 name = cfg->string;
  E_TypeKey type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, name);
  if(e_type_key_match(e_type_key_zero(), type_key))
  {
    String8List parts = {0};
    U64 offset = 8;
    String8Node offset_node = {0, str8_struct(&offset)};
    String8Node string_node = {0, cfg->first->string};
    str8_list_push_node(&parts, &offset_node);
    str8_list_push_node(&parts, &string_node);
    result = str8_list_join(arena, &parts, 0);
  }
  else
  {
    Temp scratch = scratch_begin(&arena, 1);
    MD_Node *schema = rd_schema_from_name(scratch.arena, name);
    String8List fixed_width_parts = {0};
    String8List variable_width_parts = {0};
    {
      E_Type *type = e_type_from_key__cached(type_key);
      if(type->members != 0) for EachIndex(member_idx, type->count)
      {
        E_Member *member = &type->members[member_idx];
        String8 child_name = member->name;
        MD_Node *member_schema = md_child_from_string(schema, child_name, 0);
        String8 member_type_name = member_schema->first->string;
        RD_Cfg *child = rd_cfg_child_from_string(cfg, child_name);
        if(str8_match(member_type_name, str8_lit("location"), 0))
        {
          U64 off = type->byte_size + variable_width_parts.total_size;
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8_struct(&off)));
          for(RD_Cfg *loc_child = child->first; loc_child != &rd_nil_cfg; loc_child = loc_child->first)
          {
            if(loc_child != child->first)
            {
              str8_list_push(scratch.arena, &variable_width_parts, str8_lit(":"));
            }
            str8_list_push(scratch.arena, &variable_width_parts, loc_child->string);
          }
          str8_list_push(scratch.arena, &variable_width_parts, str8_lit("\0"));
        }
        else if(str8_match(member_type_name, str8_lit("code_string"), 0) ||
                str8_match(member_type_name, str8_lit("path"), 0) ||
                str8_match(member_type_name, str8_lit("string"), 0))
        {
          U64 off = type->byte_size + variable_width_parts.total_size;
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8_struct(&off)));
          str8_list_push(scratch.arena, &variable_width_parts, child->first->string);
          str8_list_push(scratch.arena, &variable_width_parts, str8_lit("\0"));
        }
        else if(str8_match(member_type_name, str8_lit("u64"), 0))
        {
          U64 val = 0;
          try_u64_from_str8_c_rules(child->first->string, &val);
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8_struct(&val)));
        }
        else if(str8_match(member_type_name, str8_lit("bool"), 0))
        {
          B32 val = str8_match(child->first->string, str8_lit("1"), 0);
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8((U8 *)&val, e_type_byte_size_from_key(member->type_key))));
        }
      }
    }
    String8List all_parts = {0};
    str8_list_concat_in_place(&all_parts, &fixed_width_parts);
    str8_list_concat_in_place(&all_parts, &variable_width_parts);
    result = str8_list_join(arena, &all_parts, 0);
  }
  return result;
}

internal String8
rd_eval_blob_from_cfg__cached(RD_Cfg *cfg)
{
  String8 result = {0};
  {
    RD_Cfg2EvalBlobMap *map = rd_state->cfg2evalblob_map;
    RD_CfgID id = cfg->id;
    U64 hash = d_hash_from_string(str8_struct(&id));
    U64 slot_idx = hash%map->slots_count;
    RD_Cfg2EvalBlobNode *node = 0;
    for(RD_Cfg2EvalBlobNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
    {
      if(n->id == id)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = push_array(rd_frame_arena(), RD_Cfg2EvalBlobNode, 1);
      SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, node);
      node->id = id;
      node->blob = rd_eval_blob_from_cfg(rd_frame_arena(), cfg);
    }
    result = node->blob;
  }
  return result;
}

//- rjf: ctrl entity -> eval blob

internal String8
rd_eval_blob_from_entity(Arena *arena, CTRL_Entity *entity)
{
  String8 result = {0};
  String8 name = ctrl_entity_kind_code_name_table[entity->kind];
  E_TypeKey type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, name);
  if(!e_type_key_match(e_type_key_zero(), type_key))
  {
    Temp scratch = scratch_begin(&arena, 1);
    MD_Node *schema = rd_schema_from_name(scratch.arena, name);
    String8List fixed_width_parts = {0};
    String8List variable_width_parts = {0};
    {
      E_Type *type = e_type_from_key__cached(type_key);
      if(type->members != 0) for EachIndex(member_idx, type->count)
      {
        E_Member *member = &type->members[member_idx];
        String8 member_name = member->name;
        if(0){}
        else if(str8_match(member_name, str8_lit("frozen"), 0))
        {
          str8_list_push(scratch.arena, &fixed_width_parts, str8((U8 *)&entity->is_frozen, 1));
        }
        else if(str8_match(member_name, str8_lit("vaddr_range"), 0))
        {
          str8_list_push(scratch.arena, &fixed_width_parts, str8_struct(&entity->vaddr_range));
        }
        else if(str8_match(member_name, str8_lit("id"), 0))
        {
          str8_list_push(scratch.arena, &fixed_width_parts, str8_struct(&entity->id));
        }
        else if(str8_match(member_name, str8_lit("label"), 0))
        {
          String8 label = entity->string;
          if(entity->kind == CTRL_EntityKind_Module)
          {
            label = str8_skip_last_slash(label);
          }
          U64 off = type->byte_size + variable_width_parts.total_size;
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8_struct(&off)));
          str8_list_push(scratch.arena, &variable_width_parts, label);
          str8_list_push(scratch.arena, &variable_width_parts, str8_lit("\0"));
        }
        else if(str8_match(member_name, str8_lit("exe"), 0))
        {
          String8 string = entity->string;
          U64 off = type->byte_size + variable_width_parts.total_size;
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8_struct(&off)));
          str8_list_push(scratch.arena, &variable_width_parts, string);
          str8_list_push(scratch.arena, &variable_width_parts, str8_lit("\0"));
        }
        else if(str8_match(member_name, str8_lit("dbg"), 0))
        {
          String8 string = ctrl_entity_child_from_kind(entity, CTRL_EntityKind_DebugInfoPath)->string;
          U64 off = type->byte_size + variable_width_parts.total_size;
          str8_list_push(scratch.arena, &fixed_width_parts, push_str8_copy(scratch.arena, str8_struct(&off)));
          str8_list_push(scratch.arena, &variable_width_parts, string);
          str8_list_push(scratch.arena, &variable_width_parts, str8_lit("\0"));
        }
      }
    }
    String8List all_parts = {0};
    str8_list_concat_in_place(&all_parts, &fixed_width_parts);
    str8_list_concat_in_place(&all_parts, &variable_width_parts);
    result = str8_list_join(arena, &all_parts, 0);
  }
  return result;
}

internal String8
rd_eval_blob_from_entity__cached(CTRL_Entity *entity)
{
  String8 result = {0};
  {
    RD_Entity2EvalBlobMap *map = rd_state->entity2evalblob_map;
    CTRL_Handle handle = entity->handle;
    U64 hash = ctrl_hash_from_handle(handle);
    U64 slot_idx = hash%map->slots_count;
    RD_Entity2EvalBlobNode *node = 0;
    for(RD_Entity2EvalBlobNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
    {
      if(ctrl_handle_match(n->handle, handle))
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = push_array(rd_frame_arena(), RD_Entity2EvalBlobNode, 1);
      SLLQueuePush(map->slots[slot_idx].first, map->slots[slot_idx].last, node);
      node->handle = handle;
      node->blob = rd_eval_blob_from_entity(rd_frame_arena(), entity);
    }
    result = node->blob;
  }
  return result;
}

//- rjf: eval space reads/writes

internal B32
rd_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  switch(space.kind)
  {
    //- rjf: filesystem reads
    case E_SpaceKind_HashStoreKey:
    {
      U128 key = space.u128;
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
    
    //- rjf: interior control entity reads (inside process address space or thread register block)
    case RD_EvalSpaceKind_CtrlEntity:
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      switch(entity->kind)
      {
        default:{}break;
        case CTRL_EntityKind_Process:
        {
          CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, entity->handle, range, d_state->frame_eval_memread_endt_us);
          String8 data = slice.data;
          if(data.size == dim_1u64(range))
          {
            result = 1;
            MemoryCopy(out, data.str, data.size);
          }
        }break;
        case CTRL_EntityKind_Thread:
        {
          CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity);
          U64 frame_idx = e_interpret_ctx->reg_unwind_count;
          if(frame_idx < unwind.frames.count)
          {
            CTRL_UnwindFrame *f = &unwind.frames.v[frame_idx];
            U64 regs_size = regs_block_size_from_arch(e_interpret_ctx->reg_arch);
            Rng1U64 legal_range = r1u64(0, regs_size);
            Rng1U64 read_range = intersect_1u64(legal_range, range);
            U64 read_size = dim_1u64(read_range);
            MemoryCopy(out, (U8 *)f->regs + read_range.min, read_size);
            result = (read_size == dim_1u64(range));
          }
        }break;
      }
    }break;
    
    //- rjf: meta-config reads
    case RD_EvalSpaceKind_MetaCfg:
    {
      RD_Cfg *cfg = rd_cfg_from_eval_space(space);
      String8 cfg_eval_blob = rd_eval_blob_from_cfg__cached(cfg);
      Rng1U64 legal_range = r1u64(0, cfg_eval_blob.size);
      Rng1U64 read_range = intersect_1u64(range, legal_range);
      if(read_range.min < read_range.max)
      {
        result = 1;
        MemoryCopy(out, cfg_eval_blob.str + read_range.min, dim_1u64(read_range));
      }
    }break;
    
    //- rjf: meta-entity reads
    case RD_EvalSpaceKind_MetaCtrlEntity:
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      String8 entity_eval_blob = rd_eval_blob_from_entity__cached(entity);
      Rng1U64 legal_range = r1u64(0, entity_eval_blob.size);
      Rng1U64 read_range = intersect_1u64(range, legal_range);
      if(read_range.min < read_range.max)
      {
        result = 1;
        MemoryCopy(out, entity_eval_blob.str + read_range.min, dim_1u64(read_range));
      }
    }break;
  }
  scratch_end(scratch);
  return result;
}

internal B32
rd_eval_space_write(void *u, E_Space space, void *in, Rng1U64 range)
{
  B32 result = 0;
  switch(space.kind)
  {
    default:{}break;
    
    //- rjf: interior control entity writes (inside process address space or
    // thread register block)
    case RD_EvalSpaceKind_CtrlEntity:
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      switch(entity->kind)
      {
        default:{}break;
        case CTRL_EntityKind_Process:
        {
          result = ctrl_process_write(entity->handle, range, in);
        }break;
        case CTRL_EntityKind_Thread:
        {
          Temp scratch = scratch_begin(0, 0);
          U64 regs_size = regs_block_size_from_arch(entity->arch);
          Rng1U64 legal_range = r1u64(0, regs_size);
          Rng1U64 write_range = intersect_1u64(legal_range, range);
          U64 write_size = dim_1u64(write_range);
          void *new_regs = ctrl_query_cached_reg_block_from_thread(scratch.arena, d_state->ctrl_entity_store, entity->handle);
          MemoryCopy((U8 *)new_regs + write_range.min, in, write_size);
          result = ctrl_thread_write_reg_block(entity->handle, new_regs);
          scratch_end(scratch);
        }break;
      }
    }break;
    
    //- rjf: meta-config writes
    case RD_EvalSpaceKind_MetaCfg:
    {
      Temp scratch = scratch_begin(0, 0);
      
      // rjf: unpack
      RD_Cfg *cfg = rd_cfg_from_eval_space(space);
      String8 eval_blob = rd_eval_blob_from_cfg__cached(cfg);
      MD_Node *schema = rd_schema_from_name(scratch.arena, cfg->string);
      String8 name = cfg->string;
      E_TypeKey type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, name);
      E_Type *type = e_type_from_key__cached(type_key);
      
      // rjf: find member to which this write applies, reflect back in the cfg tree
      if(type->members != 0) for EachIndex(member_idx, type->count)
      {
        E_Member *member = &type->members[member_idx];
        Rng1U64 member_range = r1u64(member->off, member->off + e_type_byte_size_from_key(member->type_key));
        String8 child_name = member->name;
        MD_Node *member_schema = md_child_from_string(schema, child_name, 0);
        String8 member_type_name = member_schema->first->string;
        RD_Cfg *child = rd_cfg_child_from_string(cfg, child_name);
        if((str8_match(member_type_name, str8_lit("code_string"), 0) ||
            str8_match(member_type_name, str8_lit("path"), 0) ||
            str8_match(member_type_name, str8_lit("string"), 0)) &&
           member->off+sizeof(U64) <= eval_blob.size)
        {
          U64 string_off = *(U64 *)(eval_blob.str + member->off);
          U64 string_opl = string_off + child->first->string.size + 1;
          Rng1U64 string_range = r1u64(string_off, string_opl);
          if(contains_1u64(string_range, range.min))
          {
            String8 new_string = str8_cstring_capped(in, (U8 *)in + dim_1u64(range));
            if(new_string.size == 0)
            {
              rd_cfg_release(child);
            }
            else
            {
              if(child == &rd_nil_cfg)
              {
                child = rd_cfg_new(cfg, child_name);
              }
              rd_cfg_new_replace(child, new_string);
            }
            result = 1;
            break;
          }
        }
        else if(str8_match(member_type_name, str8_lit("u64"), 0) && dim_1u64(range) >= 1 && contains_1u64(member_range, range.min))
        {
          U64 value = 0;
          MemoryCopy(&value, in, dim_1u64(range));
          if(value == 0)
          {
            rd_cfg_release(child);
          }
          else
          {
            if(child == &rd_nil_cfg)
            {
              child = rd_cfg_new(cfg, child_name);
            }
            rd_cfg_new_replacef(child, "%I64u", value);
          }
          result = 1;
          break;
        }
        else if(str8_match(member_type_name, str8_lit("bool"), 0) && dim_1u64(range) >= 1 && contains_1u64(member_range, range.min))
        {
          U64 value = 0;
          MemoryCopy(&value, in, dim_1u64(range));
          if(value == 0)
          {
            rd_cfg_release(child);
          }
          else
          {
            if(child == &rd_nil_cfg)
            {
              child = rd_cfg_new(cfg, child_name);
            }
            rd_cfg_new_replacef(child, "%I64u", value);
          }
          result = 1;
          break;
        }
      }
      
      // rjf: if no members? -> treat this as a commit to the cfg's children
      if(type->members == 0)
      {
        String8 new_string = str8_cstring_capped(in, (U8 *)in + dim_1u64(range));
        rd_cfg_new_replace(cfg, new_string);
        result = 1;
      }
      
      // rjf: commit to the eval blob cache
      {
        RD_Cfg2EvalBlobMap *map = rd_state->cfg2evalblob_map;
        RD_CfgID id = cfg->id;
        U64 hash = d_hash_from_string(str8_struct(&id));
        U64 slot_idx = hash%map->slots_count;
        
        // rjf: cfg -> cached node
        RD_Cfg2EvalBlobNode *node = 0;
        for(RD_Cfg2EvalBlobNode *n = map->slots[slot_idx].first; n != 0; n = n->next)
        {
          if(n->id == id)
          {
            node = n;
            break;
          }
        }
        
        // rjf: if node -> commit
        if(node)
        {
          node->blob = rd_eval_blob_from_cfg(rd_frame_arena(), cfg);
        }
      }
      
      scratch_end(scratch);
    }break;
    
    case RD_EvalSpaceKind_MetaCtrlEntity:
    {
#if 0  // TODO(rjf): @cfg
      Temp scratch = scratch_begin(0, 0);
      
      // rjf: get entity, produce meta-eval
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      CTRL_MetaEval *meval = rd_ctrl_meta_eval_from_ctrl_entity(scratch.arena, entity);
      
      // rjf: copy meta evaluation to scratch arena, to form range of legal reads
      arena_push(scratch.arena, 0, 64);
      String8 meval_srlzed = serialized_from_struct(scratch.arena, CTRL_MetaEval, meval);
      U64 pos_min = arena_pos(scratch.arena);
      CTRL_MetaEval *meval_read = struct_from_serialized(scratch.arena, CTRL_MetaEval, meval_srlzed);
      U64 pos_opl = arena_pos(scratch.arena);
      
      // rjf: rebase all pointer values in meta evaluation to be relative to base pointer
      struct_rebase_ptrs(CTRL_MetaEval, meval_read, meval_read);
      
      // rjf: perform write to entity
      if(0){}
#define FlatMemberCase(name) else if(range.min == OffsetOf(CTRL_MetaEval, name) && dim_1u64(range) <= sizeof(meval_read->name))
#define StringMemberCase(name) else if(range.min == (U64)meval_read->name.str)
      StringMemberCase(label) {result = 1; ctrl_entity_equip_string(d_state->ctrl_entity_store, entity, str8_cstring_capped(in, (U8 *)in + 4096));}
#undef FlatMemberCase
#undef StringMemberCase
      scratch_end(scratch);
#endif
    }break;
  }
  return result;
}

//- rjf: asynchronous streamed reads -> hashes from spaces

internal U128
rd_key_from_eval_space_range(E_Space space, Rng1U64 range, B32 zero_terminated)
{
  U128 result = {0};
  switch(space.kind)
  {
    case E_SpaceKind_HashStoreKey:
    {
      result = space.u128;
    }break;
    case RD_EvalSpaceKind_CtrlEntity:
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      if(entity->kind == CTRL_EntityKind_Process)
      {
        result = ctrl_hash_store_key_from_process_vaddr_range(entity->handle, range, zero_terminated);
      }
    }break;
  }
  return result;
}

//- rjf: space -> entire range

internal Rng1U64
rd_whole_range_from_eval_space(E_Space space)
{
  Rng1U64 result = {0};
  switch(space.kind)
  {
    case E_SpaceKind_HashStoreKey:
    {
      HS_Scope *scope = hs_scope_open();
      U128 hash = {0};
      for(U64 idx = 0; idx < 2; idx += 1)
      {
        hash = hs_hash_from_key(space.u128, idx);
        if(!u128_match(hash, u128_zero()))
        {
          break;
        }
      }
      String8 data = hs_data_from_hash(scope, hash);
      result = r1u64(0, data.size);
      hs_scope_close(scope);
    }break;
    case RD_EvalSpaceKind_CtrlEntity:
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      if(entity->kind == CTRL_EntityKind_Process)
      {
        result = r1u64(0, 0x7FFFFFFFFFFFull);
      }
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: Evaluation View Visualization & Interaction

//- rjf: writing values back to child processes

internal B32
rd_commit_eval_value_string(E_Eval dst_eval, String8 string, B32 string_needs_unescaping)
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
    if((E_TypeKind_FirstBasic <= type_kind && type_kind <= E_TypeKind_LastBasic) ||
       type_kind == E_TypeKind_Enum)
    {
      E_Expr *src_expr = e_parse_expr_from_text(scratch.arena, string);
      E_Expr *src_expr__casted = e_expr_ref_cast(scratch.arena, type_key, src_expr);
      E_Eval src_eval = e_eval_from_expr(scratch.arena, src_expr__casted);
      commit_data = push_str8_copy(scratch.arena, str8_struct(&src_eval.value));
      commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(type_key));
    }
    else if(type_kind == E_TypeKind_Ptr || type_kind == E_TypeKind_Array)
    {
      E_Eval src_eval = e_eval_from_string(scratch.arena, string);
      E_Eval src_eval_value = e_value_eval_from_eval(src_eval);
      E_TypeKind src_eval_value_type_kind = e_type_kind_from_key(src_eval_value.type_key);
      if(direct_type_kind == E_TypeKind_Char8 ||
         direct_type_kind == E_TypeKind_UChar8 ||
         e_type_kind_is_integer(direct_type_kind))
      {
        B32 is_quoted = 0;
        if(string_needs_unescaping)
        {
          if(string.size >= 1 && string.str[0] == '"')
          {
            string = str8_skip(string, 1);
            is_quoted = 1;
          }
          if(string.size >= 1 && string.str[string.size-1] == '"')
          {
            string = str8_chop(string, 1);
          }
        }
        if(is_quoted)
        {
          commit_data = raw_from_escaped_str8(scratch.arena, string);
        }
        else
        {
          commit_data = push_str8_copy(scratch.arena, string);
        }
        commit_data.size += 1;
        if(type_kind == E_TypeKind_Ptr)
        {
          commit_at_ptr_dest = 1;
        }
      }
      else if(type_kind == E_TypeKind_Ptr &&
              (e_type_kind_is_pointer_or_ref(src_eval_value_type_kind) ||
               e_type_kind_is_integer(src_eval_value_type_kind)) &&
              src_eval_value.mode == E_Mode_Value)
      {
        commit_data = push_str8_copy(scratch.arena, str8_struct(&src_eval.value));
        commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(src_eval.type_key));
        commit_data.size = Min(commit_data.size, e_type_byte_size_from_key(type_key));
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

//- rjf: view rule config tree info extraction

internal E_Value
rd_value_from_params_key(MD_Node *params, String8 key)
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
rd_range_from_eval_params(E_Eval eval, MD_Node *params)
{
  Temp scratch = scratch_begin(0, 0);
  U64 size = rd_value_from_params_key(params, str8_lit("size")).u64;
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
    size = KB(16);
  }
  Rng1U64 result = {0};
  result.min = rd_base_offset_from_eval(eval);
  result.max = result.min + size;
  scratch_end(scratch);
  return result;
}

internal TXT_LangKind
rd_lang_kind_from_eval_params(E_Eval eval, MD_Node *params)
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

internal Arch
rd_arch_from_eval_params(E_Eval eval, MD_Node *params)
{
  Arch arch = Arch_Null;
  MD_Node *arch_node = md_child_from_string(params, str8_lit("arch"), 0);
  String8 arch_kind_string = arch_node->first->string;
  if(str8_match(arch_kind_string, str8_lit("x64"), StringMatchFlag_CaseInsensitive))
  {
    arch = Arch_x64;
  }
  return arch;
}

internal Vec2S32
rd_dim2s32_from_eval_params(E_Eval eval, MD_Node *params)
{
  Vec2S32 dim = v2s32(1, 1);
  {
    dim.x = rd_value_from_params_key(params, str8_lit("w")).s32;
    dim.y = rd_value_from_params_key(params, str8_lit("h")).s32;
  }
  return dim;
}

internal R_Tex2DFormat
rd_tex2dformat_from_eval_params(E_Eval eval, MD_Node *params)
{
  R_Tex2DFormat result = R_Tex2DFormat_RGBA8;
  {
    MD_Node *fmt_node = md_child_from_string(params, str8_lit("fmt"), 0);
    for EachNonZeroEnumVal(R_Tex2DFormat, fmt)
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

//- rjf: eval <-> file path

internal String8
rd_file_path_from_eval_string(Arena *arena, String8 string)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Expr *expr = e_parse_expr_from_text(scratch.arena, string);
    if(expr->kind == E_ExprKind_LeafFilePath)
    {
      result = raw_from_escaped_str8(arena, expr->string);
    }
    scratch_end(scratch);
  }
  return result;
}

internal String8
rd_eval_string_from_file_path(Arena *arena, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 string_escaped = escaped_from_raw_str8(scratch.arena, string);
  String8 result = push_str8f(arena, "file:\"%S\"", string_escaped);
  scratch_end(scratch);
  return result;
}

//- rjf: eval -> query

internal String8
rd_query_from_eval_string(Arena *arena, String8 string)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_Expr *expr = e_parse_expr_from_text(scratch.arena, string);
    if(expr->kind == E_ExprKind_LeafIdent &&
       str8_match(expr->qualifier, str8_lit("query"), 0))
    {
      result = expr->string;
    }
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: View Functions

internal RD_ViewState *
rd_view_state_from_cfg(RD_Cfg *cfg)
{
  RD_CfgID id = cfg->id;
  U64 hash = d_hash_from_string(str8_struct(&id));
  U64 slot_idx = hash%rd_state->view_state_slots_count;
  RD_ViewStateSlot *slot = &rd_state->view_state_slots[slot_idx];
  RD_ViewState *view_state = &rd_nil_view_state;
  for(RD_ViewState *v = slot->first; v != 0; v = v->hash_next)
  {
    if(v->cfg_id == id)
    {
      view_state = v;
      break;
    }
  }
  if(view_state == &rd_nil_view_state)
  {
    view_state = rd_state->free_view_state;
    if(view_state)
    {
      SLLStackPop_N(rd_state->free_view_state, hash_next);
    }
    else
    {
      view_state = push_array(rd_state->arena, RD_ViewState, 1);
    }
    MemoryCopyStruct(view_state, &rd_nil_view_state);
    DLLPushBack_NP(slot->first, slot->last, view_state, hash_next, hash_prev);
    view_state->cfg_id = id;
    view_state->arena = arena_alloc();
    view_state->ev_view = ev_view_alloc();
    view_state->loading_t = 1.f;
  }
  if(view_state != &rd_nil_view_state)
  {
    view_state->last_frame_index_touched = rd_state->frame_index;
  }
  return view_state;
}

internal void
rd_view_ui(Rng2F32 rect)
{
  ProfBeginFunction();
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  String8 view_name = view->string;
  String8 expr_string = rd_expr_from_cfg(view);
  
  //////////////////////////////
  //- rjf: special-case view: "getting started"
  //
  if(0){}
  else if(str8_match(view_name, str8_lit("getting_started"), 0))
  {
    Temp scratch = scratch_begin(0, 0);
    ui_set_next_flags(UI_BoxFlag_DefaultFocusNav);
    UI_Focus(UI_FocusKind_On) UI_WidthFill UI_HeightFill UI_NamedColumn(str8_lit("empty_view"))
      UI_Padding(ui_pct(1, 0)) UI_Focus(UI_FocusKind_Null)
    {
      RD_CfgList targets = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("target"));
      CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
      
      //- rjf: icon & info
      UI_Padding(ui_em(2.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      {
        //- rjf: icon
        {
          F32 icon_dim = ui_top_font_size()*10.f;
          UI_PrefHeight(ui_px(icon_dim, 1.f))
            UI_Row
            UI_Padding(ui_pct(1, 0))
            UI_PrefWidth(ui_px(icon_dim, 1.f))
          {
            R_Handle texture = rd_state->icon_texture;
            Vec2S32 texture_dim = r_size_from_tex2d(texture);
            ui_image(texture, R_Tex2DSampleKind_Linear, r2f32p(0, 0, texture_dim.x, texture_dim.y), v4f32(1, 1, 1, 1), 0, str8_lit(""));
          }
        }
        
        //- rjf: info
        UI_Padding(ui_em(2.f, 1.f))
          UI_WidthFill UI_PrefHeight(ui_em(2.f, 1.f))
          UI_Row
          UI_Padding(ui_pct(1, 0))
          UI_TextAlignment(UI_TextAlign_Center)
          UI_PrefWidth(ui_text_dim(10, 1))
        {
          ui_label(str8_lit(BUILD_TITLE_STRING_LITERAL));
        }
      }
      
      //- rjf: targets state dependent helper
      B32 helper_built = 0;
      if(processes.count == 0)
      {
        helper_built = 1;
        switch(targets.count)
        {
          //- rjf: user has no targets. build helper for adding them
          case 0:
          {
            UI_PrefHeight(ui_em(3.75f, 1.f))
              UI_Row
              UI_Padding(ui_pct(1, 0))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_PrefWidth(ui_em(22.f, 1.f))
              UI_CornerRadius(ui_top_font_size()/2.f)
              RD_Palette(RD_PaletteCode_NeutralPopButton)
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_Add, 0, "Add Target")))
            {
              rd_cmd(RD_CmdKind_RunCommand, .cmd_name = rd_cmd_kind_info_table[RD_CmdKind_AddTarget].string);
            }
          }break;
          
          //- rjf: user has 1 target. build helper for launching it
          case 1:
          {
            RD_Cfg *target_cfg = rd_cfg_list_first(&targets);
            D_Target target = rd_target_from_cfg(scratch.arena, target_cfg);
            String8 target_full_path = target.exe;
            String8 target_name = str8_skip_last_slash(target_full_path);
            UI_PrefHeight(ui_em(3.75f, 1.f))
              UI_Row
              UI_Padding(ui_pct(1, 0))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_PrefWidth(ui_em(22.f, 1.f))
              UI_CornerRadius(ui_top_font_size()/2.f)
              RD_Palette(RD_PaletteCode_PositivePopButton)
            {
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_Play, 0, "Launch %S", target_name)))
              {
                rd_cmd(RD_CmdKind_LaunchAndRun, .cfg = target_cfg->id);
              }
              ui_spacer(ui_em(1.5f, 1));
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_StepInto, 0, "Step Into %S", target_name)))
              {
                rd_cmd(RD_CmdKind_LaunchAndInit, .cfg = target_cfg->id);
              }
            }
          }break;
          
          //- rjf: user has N targets.
          default:
          {
            helper_built = 0;
          }break;
        }
      }
      
      //- rjf: or text
      if(helper_built)
      {
        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
          UI_PrefHeight(ui_em(2.25f, 1.f))
          UI_Row
          UI_Padding(ui_pct(1, 0))
          UI_TextAlignment(UI_TextAlign_Center)
          UI_WidthFill
          ui_labelf("- or -");
      }
      
      //- rjf: helper text for command lister activation
      UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
        UI_PrefHeight(ui_em(2.25f, 1.f)) UI_Row
        UI_PrefWidth(ui_text_dim(10, 1))
        UI_TextAlignment(UI_TextAlign_Center)
        UI_Padding(ui_pct(1, 0))
        RD_Palette(RD_PaletteCode_Floating)
      {
        ui_labelf("use");
        UI_TextAlignment(UI_TextAlign_Center) rd_cmd_binding_buttons(rd_cmd_kind_info_table[RD_CmdKind_OpenLister].string);
        ui_labelf("to open the lister for commands and options");
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: special-case view: pending
  //
  else if(str8_match(view_name, str8_lit("pending"), 0))
  {
    Temp scratch = scratch_begin(0, 0);
    typedef struct State State;
    struct State
    {
      Arena *deferred_cmd_arena;
      RD_CmdList deferred_cmds;
    };
    State *state = rd_view_state(State);
    if(state->deferred_cmd_arena == 0)
    {
      state->deferred_cmd_arena = rd_push_view_arena();
    }
    rd_store_view_loading_info(1, 0, 0);
    
    // rjf: any commands sent to this view need to be deferred until loading is complete
    for(RD_Cmd *cmd = 0; rd_next_view_cmd(&cmd);)
    {
      RD_CmdKind kind = rd_cmd_kind_from_string(cmd->name);
      switch(kind)
      {
        default:{}break;
        case RD_CmdKind_GoToLine:
        case RD_CmdKind_GoToAddress:
        case RD_CmdKind_CenterCursor:
        case RD_CmdKind_ContainCursor:
        {
          rd_cmd_list_push_new(state->deferred_cmd_arena, &state->deferred_cmds, cmd->name, cmd->regs);
        }break;
      }
    }
    
    // rjf: unpack view's target expression & hash
    String8 expr_string = rd_expr_from_cfg(view);
    E_Eval eval = e_eval_from_string(scratch.arena, expr_string);
    Rng1U64 range = r1u64(0, 1024);
    U128 key = rd_key_from_eval_space_range(eval.space, range, 0);
    U128 hash = hs_hash_from_key(key, 0);
    
    // rjf: determine if hash's blob is ready, and which viewer to use
    B32 data_is_ready = 0;
    String8 new_view_name = {0};
    {
      HS_Scope *hs_scope = hs_scope_open();
      if(!u128_match(hash, u128_zero()))
      {
        String8 data = hs_data_from_hash(hs_scope, hash);
        U64 num_utf8_bytes = 0;
        U64 num_unknown_bytes = 0;
        for(U64 idx = 0; idx < data.size && idx < range.max;)
        {
          UnicodeDecode decode = utf8_decode(data.str+idx, data.size-idx);
          if(decode.codepoint != max_U32 && (decode.inc > 1 ||
                                             (10 <= decode.codepoint && decode.codepoint <= 13) ||
                                             (32 <= decode.codepoint && decode.codepoint <= 126)))
          {
            num_utf8_bytes += decode.inc;
            idx += decode.inc;
          }
          else
          {
            num_unknown_bytes += 1;
            idx += 1;
          }
        }
        data_is_ready = 1;
        if(num_utf8_bytes > num_unknown_bytes*4 || num_unknown_bytes == 0)
        {
          new_view_name = str8_lit("text");
        }
        else
        {
          new_view_name = str8_lit("memory");
        }
      }
      hs_scope_close(hs_scope);
    }
    
    // rjf: if we don't have a viewer, just use the memory viewer.
    if(new_view_name.size != 0)
    {
      new_view_name = str8_lit("memory");
    }
    
    // rjf: if data is ready and we have the name of a new visualizer,
    // dispatch deferred commands & change this view's string to be
    // that of the new visualizer.
    if(data_is_ready && new_view_name.size != 0)
    {
      for(RD_CmdNode *cmd_node = state->deferred_cmds.first;
          cmd_node != 0;
          cmd_node = cmd_node->next)
      {
        RD_Cmd *cmd = &cmd_node->cmd;
        rd_push_cmd(cmd->name, cmd->regs);
      }
      RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
      rd_cfg_equip_string(view, new_view_name);
    }
    
    // rjf: if we don't have a viewer, for whatever reason, then just
    // close the tab.
    if(data_is_ready && new_view_name.size == 0)
    {
      rd_cmd(RD_CmdKind_CloseTab);
    }
    
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: visualizer hook
  //
  else
  {
    Temp scratch = scratch_begin(0, 0);
    RD_ViewUIRule *view_ui_rule = rd_view_ui_rule_from_string(view_name);
    E_Eval expr_eval = e_eval_from_string(scratch.arena, expr_string);
    E_Expr *tag = rd_tag_from_cfg(scratch.arena, view);
    view_ui_rule->ui(expr_eval, tag, rect);
    scratch_end(scratch);
  }
  
  ProfEnd();
}

////////////////////////////////
//~ rjf: View Building API

//- rjf: view info extraction

internal Arena *
rd_view_arena(void)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  return view_state->arena;
}

internal UI_ScrollPt2
rd_view_scroll_pos(void)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  return view_state->scroll_pos;
}

internal EV_View *
rd_view_eval_view(void)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  return view_state->ev_view;
}

internal String8
rd_view_filter(void)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_Cfg *filter = rd_cfg_child_from_string(view, str8_lit("filter"));
  String8 filter_string = filter->first->string;
  return filter_string;
}

internal RD_Cfg *
rd_view_cfg_from_string(String8 string)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_Cfg *cfg = rd_cfg_child_from_string(view, string);
  return cfg;
}

internal E_Value
rd_view_cfg_value_from_string(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  RD_Cfg *root = rd_view_cfg_from_string(string);
  String8 expr = root->first->string;
  E_Eval eval = e_eval_from_string(scratch.arena, expr);
  E_Value result = e_value_eval_from_eval(eval).value;
  scratch_end(scratch);
  return result;
}

//- rjf: evaluation & tag (a view's 'call') parameter extraction

internal U64
rd_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(eval.type_key)))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

internal Rng1U64
rd_range_from_eval_tag(E_Eval eval, E_Expr *tag)
{
  U64 size = 0;
  for(E_Expr *param = tag->first->next; param != &e_expr_nil; param = param->next)
  {
    if(param->kind == E_ExprKind_Define && str8_match(param->first->string, str8_lit("size"), 0))
    {
      size = e_value_from_expr(param->first->next).u64;
      break;
    }
  }
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
    size = KB(16);
  }
  Rng1U64 result = {0};
  result.min = rd_base_offset_from_eval(eval);
  result.max = result.min + size;
  return result;
}

internal TXT_LangKind
rd_lang_kind_from_eval_tag(E_Eval eval, E_Expr *tag)
{
  TXT_LangKind lang_kind = TXT_LangKind_Null;
  if(eval.expr->kind == E_ExprKind_LeafFilePath)
  {
    lang_kind = txt_lang_kind_from_extension(str8_skip_last_dot(eval.expr->string));
  }
  else for(E_Expr *param = tag->first->next; param != &e_expr_nil; param = param->next)
  {
    if(param->kind == E_ExprKind_Define && str8_match(param->first->string, str8_lit("lang"), 0))
    {
      lang_kind = txt_lang_kind_from_extension(param->first->next->string);
      break;
    }
  }
  return lang_kind;
}

internal Arch
rd_arch_from_eval_tag(E_Eval eval, E_Expr *tag)
{
  // rjf: try implicitly from either `eval` itself, or from context
  CTRL_Entity *ctrl_entity = rd_ctrl_entity_from_eval_space(eval.space);
  CTRL_Entity *process = ctrl_process_from_entity(ctrl_entity);
  if(process == &ctrl_entity_nil)
  {
    process = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->process);
  }
  Arch arch = process->arch;
  if(arch == Arch_Null)
  {
    arch = arch_from_context();
  }
  
  // rjf: try arch parameters
  for(E_Expr *param = tag->first->next; param != &e_expr_nil; param = param->next)
  {
    String8 param_arch_string = param->string;
    if(param->kind == E_ExprKind_Define && str8_match(param->first->string, str8_lit("arch"), 0))
    {
      param_arch_string = param->first->next->string;
    }
    if(str8_match(param->first->next->string, str8_lit("x64"), 0))
    {
      arch = Arch_x64;
      break;
    }
  }
  
  return arch;
}

internal Vec2S32
rd_dim2s32_from_eval_tag(E_Eval eval, E_Expr *tag)
{
  Vec2S32 dim = v2s32(1, 1);
  B32 got_x = 0;
  B32 got_y = 0;
  
  // rjf: try explicitly passed dimensions
  for(E_Expr *param = tag->first->next; param != &e_expr_nil; param = param->next)
  {
    if(param->kind == E_ExprKind_Define)
    {
      if(str8_match(param->first->string, str8_lit("w"), 0))
      {
        got_x = 1;
        dim.x = e_value_from_expr(param->first->next).s64;
      }
      if(str8_match(param->first->string, str8_lit("h"), 0))
      {
        got_y = 1;
        dim.y = e_value_from_expr(param->first->next).s64;
      }
    }
  }
  
  // rjf: try ordered non-define arguments
  for(E_Expr *param = tag->first->next; param != &e_expr_nil; param = param->next)
  {
    if(param->kind != E_ExprKind_Define)
    {
      if(!got_x)
      {
        got_x = 1;
        dim.x = e_value_from_expr(param).s64;
      }
      else if(!got_y)
      {
        got_y = 1;
        dim.y = e_value_from_expr(param).s64;
        break;
      }
    }
  }
  
  return dim;
}

internal R_Tex2DFormat
rd_tex2dformat_from_eval_tag(E_Eval eval, E_Expr *tag)
{
  R_Tex2DFormat fmt = R_Tex2DFormat_RGBA8;
  B32 got_fmt = 0;
  
  // rjf: try explicitly passed formats
  for(E_Expr *param = tag->first->next; param != &e_expr_nil; param = param->next)
  {
    if(param->kind == E_ExprKind_Define && str8_match(param->first->string, str8_lit("fmt"), 0))
    {
      got_fmt = 1;
      for EachEnumVal(R_Tex2DFormat, f)
      {
        if(str8_match(param->first->next->string, r_tex2d_format_display_string_table[f], StringMatchFlag_CaseInsensitive))
        {
          fmt = f;
          break;
        }
      }
    }
  }
  
  // rjf: try implicit non-define arguments
  for(E_Expr *param = tag->first->next; param != &e_expr_nil && !got_fmt; param = param->next)
  {
    if(param->kind == E_ExprKind_LeafIdent)
    {
      for EachEnumVal(R_Tex2DFormat, f)
      {
        if(str8_match(param->string, r_tex2d_format_display_string_table[f], StringMatchFlag_CaseInsensitive))
        {
          fmt = f;
          break;
        }
      }
    }
  }
  
  return fmt;
}

//- rjf: pushing/attaching view resources

internal void *
rd_view_state_by_size(U64 size)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  if(view_state->user_data == 0)
  {
    view_state->user_data = push_array(view_state->arena, U8, size);
  }
  return view_state->user_data;
}

internal Arena *
rd_push_view_arena(void)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  RD_ArenaExt *ext = push_array(view_state->arena, RD_ArenaExt, 1);
  ext->arena = arena_alloc();
  SLLQueuePush(view_state->first_arena_ext, view_state->last_arena_ext, ext);
  return ext->arena;
}

//- rjf: storing view-attached state

internal void
rd_store_view_expr_string(String8 string)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_Cfg *expr = rd_cfg_child_from_string_or_alloc(view, str8_lit("expression"));
  rd_cfg_new_replace(expr, string);
}

internal void
rd_store_view_filter(String8 string)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_Cfg *filter = rd_cfg_child_from_string_or_alloc(view, str8_lit("filter"));
  rd_cfg_new_replace(filter, string);
}

internal void
rd_store_view_loading_info(B32 is_loading, U64 progress_u64, U64 progress_u64_target)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  view_state->loading_t_target = (F32)!!is_loading;
  view_state->loading_progress_v = progress_u64;
  view_state->loading_progress_v_target = progress_u64_target;
}

internal void
rd_store_view_scroll_pos(UI_ScrollPt2 pos)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_ViewState *view_state = rd_view_state_from_cfg(view);
  view_state->scroll_pos = pos;
}

internal void
rd_store_view_param(String8 key, String8 value)
{
  RD_Cfg *view = rd_cfg_from_id(rd_regs()->view);
  RD_Cfg *child = rd_cfg_child_from_string_or_alloc(view, key);
  rd_cfg_new_replace(child, value);
}

internal void
rd_store_view_paramf(String8 key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  rd_store_view_param(key, string);
  va_end(args);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: Window Functions

internal RD_Cfg *
rd_window_from_cfg(RD_Cfg *cfg)
{
  RD_Cfg *result = &rd_nil_cfg;
  for(RD_Cfg *c = cfg; c != &rd_nil_cfg; c = c->parent)
  {
    if(c->parent->parent == rd_state->root_cfg && str8_match(c->string, str8_lit("window"), 0))
    {
      result = c;
      break;
    }
  }
  return result;
}

internal RD_WindowState *
rd_window_state_from_cfg(RD_Cfg *cfg)
{
  //- rjf: unpack
  RD_Cfg *window_cfg = rd_window_from_cfg(cfg);
  RD_CfgID id = window_cfg->id;
  U64 hash = d_hash_from_string(str8_struct(&id));
  U64 slot_idx = hash%rd_state->window_state_slots_count;
  RD_WindowStateSlot *slot = &rd_state->window_state_slots[slot_idx];
  
  //- rjf: scan for existing window
  RD_WindowState *ws = &rd_nil_window_state;
  for(RD_WindowState *w = slot->first; w != 0; w = w->hash_next)
  {
    if(w->cfg_id == id)
    {
      ws = w;
      break;
    }
  }
  
  //- rjf: allocate/open new window if one was not found
  if(window_cfg != &rd_nil_cfg && ws == &rd_nil_window_state)
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: unpack configuration options
    Vec2F32 size = {0};
    OS_Handle preferred_monitor = {0};
    {
      RD_Cfg *size_cfg = rd_cfg_child_from_string(window_cfg, str8_lit("size"));
      RD_Cfg *monitor_cfg = rd_cfg_child_from_string(window_cfg, str8_lit("monitor"));
      size.x = (F32)f64_from_str8(size_cfg->first->string);
      size.y = (F32)f64_from_str8(size_cfg->first->next->string);
      OS_HandleArray monitors = os_push_monitors_array(scratch.arena);
      for EachIndex(idx, monitors.count)
      {
        String8 monitor_name = os_name_from_monitor(scratch.arena, monitors.v[idx]);
        if(str8_match(monitor_name, monitor_cfg->first->string, StringMatchFlag_CaseInsensitive))
        {
          preferred_monitor = monitors.v[idx];
          break;
        }
      }
    }
    
    // rjf: allocate window
    ws = rd_state->free_window_state;
    if(ws != 0)
    {
      SLLStackPop_N(rd_state->free_window_state, order_next);
    }
    else
    {
      ws = push_array_no_zero(rd_state->arena, RD_WindowState, 1);
    }
    MemoryZeroStruct(ws);
    
    // rjf: fill out window
    ws->cfg_id = id;
    ws->arena = arena_alloc();
    {
      String8 title = str8_lit_comp(BUILD_TITLE_STRING_LITERAL);
      ws->os = os_window_open(size, OS_WindowFlag_CustomBorder, title);
    }
    ws->r = r_window_equip(ws->os);
    ws->ui = ui_state_alloc();
    ws->ctx_menu_arena = arena_alloc();
    ws->ctx_menu_regs = push_array(ws->ctx_menu_arena, RD_Regs, 1);
    ws->ctx_menu_input_buffer_size = KB(4);
    ws->ctx_menu_input_buffer = push_array(ws->arena, U8, ws->ctx_menu_input_buffer_size);
    ws->drop_completion_arena = arena_alloc();
    ws->hover_eval_arena = arena_alloc();
    ws->last_dpi = os_dpi_from_window(ws->os);
    OS_Handle zero_monitor = {0};
    if(!os_handle_match(zero_monitor, preferred_monitor))
    {
      os_window_set_monitor(ws->os, preferred_monitor);
    }
    if(rd_cfg_child_from_string(window_cfg, str8_lit("fullscreen")) != &rd_nil_cfg)
    {
      os_window_set_fullscreen(ws->os, 1);
    }
    if(rd_cfg_child_from_string(window_cfg, str8_lit("maximized")) != &rd_nil_cfg)
    {
      os_window_set_maximized(ws->os, 1);
    }
    
    // rjf: hook up window links
    DLLPushBack_NPZ(&rd_nil_window_state, rd_state->first_window_state, rd_state->last_window_state, ws, order_next, order_prev);
    DLLPushBack_NP(slot->first, slot->last, ws, hash_next, hash_prev);
    
    scratch_end(scratch);
  }
  
  //- rjf: touch window for this frame
  if(ws != &rd_nil_window_state)
  {
    ws->last_frame_index_touched = rd_state->frame_index;
  }
  
  return ws;
}

internal RD_WindowState *
rd_window_state_from_os_handle(OS_Handle os)
{
  RD_WindowState *ws = &rd_nil_window_state;
  {
    for(RD_WindowState *w = rd_state->first_window_state;
        w != &rd_nil_window_state;
        w = w->order_next)
    {
      if(os_handle_match(w->os, os))
      {
        ws = w;
        break;
      }
    }
  }
  return ws;
}

#if COMPILER_MSVC && !BUILD_DEBUG
#pragma optimize("", off)
#endif

internal void
rd_window_frame(void)
{
  Temp scratch = scratch_begin(0, 0);
  ProfBeginFunction();
  
  //////////////////////////////
  //- rjf: unpack context
  //
  RD_Cfg *window          = rd_cfg_from_id(rd_regs()->window);
  RD_WindowState *ws      = rd_window_state_from_cfg(rd_cfg_from_id(rd_regs()->window));
  RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
  B32 window_is_focused   = (os_window_is_focused(ws->os) || ws->window_temporarily_focused_ipc);
  B32 popup_is_open       = (rd_state->popup_active);
  B32 query_is_open       = (ws->top_query_lister != 0);
  B32 hover_eval_is_open  = (!popup_is_open &&
                             ws->hover_eval_string.size != 0 &&
                             ws->hover_eval_first_frame_idx+20 < ws->hover_eval_last_frame_idx &&
                             rd_state->frame_index-ws->hover_eval_last_frame_idx < 20);
  if(!window_is_focused || popup_is_open)
  {
    ws->menu_bar_key_held = 0;
  }
  ws->window_temporarily_focused_ipc = 0;
  ui_select_state(ws->ui);
  
  //////////////////////////////
  //- rjf: pre-emptively rasterize common glyphs on the first frame
  //
  if(rd_state->first_window_state == ws && rd_state->last_window_state == ws && ws->frames_alive == 0)
  {
    RD_FontSlot english_font_slots[] = {RD_FontSlot_Main, RD_FontSlot_Code};
    RD_FontSlot icon_font_slot = RD_FontSlot_Icons;
    for(U64 idx = 0; idx < ArrayCount(english_font_slots); idx += 1)
    {
      Temp scratch = scratch_begin(0, 0);
      RD_FontSlot slot = english_font_slots[idx];
      String8 sample_text = str8_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#$%^&*()-_+=[{]}\\|;:'\",<.>/?");
      fnt_push_run_from_string(scratch.arena,
                               rd_font_from_slot(slot),
                               rd_font_size_from_slot(RD_FontSlot_Code),
                               0, 0, 0,
                               sample_text);
      fnt_push_run_from_string(scratch.arena,
                               rd_font_from_slot(slot),
                               rd_font_size_from_slot(RD_FontSlot_Main),
                               0, 0, 0,
                               sample_text);
      scratch_end(scratch);
    }
    for(RD_IconKind icon_kind = RD_IconKind_Null; icon_kind < RD_IconKind_COUNT; icon_kind = (RD_IconKind)(icon_kind+1))
    {
      Temp scratch = scratch_begin(0, 0);
      fnt_push_run_from_string(scratch.arena,
                               rd_font_from_slot(icon_font_slot),
                               rd_font_size_from_slot(icon_font_slot),
                               0, 0, FNT_RasterFlag_Smooth,
                               rd_icon_kind_text_table[icon_kind]);
      fnt_push_run_from_string(scratch.arena,
                               rd_font_from_slot(icon_font_slot),
                               rd_font_size_from_slot(RD_FontSlot_Main),
                               0, 0, FNT_RasterFlag_Smooth,
                               rd_icon_kind_text_table[icon_kind]);
      fnt_push_run_from_string(scratch.arena,
                               rd_font_from_slot(icon_font_slot),
                               rd_font_size_from_slot(RD_FontSlot_Code),
                               0, 0, FNT_RasterFlag_Smooth,
                               rd_icon_kind_text_table[icon_kind]);
      scratch_end(scratch);
    }
  }
  
  //////////////////////////////
  //- rjf: fill panel/view interaction registers
  //
  rd_regs()->panel = panel_tree.focused->cfg->id;
  rd_regs()->view  = panel_tree.focused->selected_tab->id;
  
  //////////////////////////////
  //- rjf: compute ui palettes from theme
  //
  {
    RD_Theme *current = rd_state->theme;
    for EachEnumVal(RD_PaletteCode, code)
    {
      ws->cfg_palettes[code].null       = v4f32(1, 0, 1, 1);
      ws->cfg_palettes[code].cursor     = current->colors[RD_ThemeColor_Cursor];
      ws->cfg_palettes[code].selection  = current->colors[RD_ThemeColor_SelectionOverlay];
    }
    ws->cfg_palettes[RD_PaletteCode_Base].background = current->colors[RD_ThemeColor_BaseBackground];
    ws->cfg_palettes[RD_PaletteCode_Base].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_Base].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_Base].border     = current->colors[RD_ThemeColor_BaseBorder];
    ws->cfg_palettes[RD_PaletteCode_MenuBar].background = current->colors[RD_ThemeColor_MenuBarBackground];
    ws->cfg_palettes[RD_PaletteCode_MenuBar].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_MenuBar].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_MenuBar].border     = current->colors[RD_ThemeColor_MenuBarBorder];
    ws->cfg_palettes[RD_PaletteCode_Floating].background = current->colors[RD_ThemeColor_FloatingBackground];
    ws->cfg_palettes[RD_PaletteCode_Floating].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_Floating].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_Floating].border     = current->colors[RD_ThemeColor_FloatingBorder];
    ws->cfg_palettes[RD_PaletteCode_ImplicitButton].background = current->colors[RD_ThemeColor_ImplicitButtonBackground];
    ws->cfg_palettes[RD_PaletteCode_ImplicitButton].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_ImplicitButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_ImplicitButton].border     = current->colors[RD_ThemeColor_ImplicitButtonBorder];
    ws->cfg_palettes[RD_PaletteCode_PlainButton].background = current->colors[RD_ThemeColor_PlainButtonBackground];
    ws->cfg_palettes[RD_PaletteCode_PlainButton].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_PlainButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_PlainButton].border     = current->colors[RD_ThemeColor_PlainButtonBorder];
    ws->cfg_palettes[RD_PaletteCode_PositivePopButton].background = current->colors[RD_ThemeColor_PositivePopButtonBackground];
    ws->cfg_palettes[RD_PaletteCode_PositivePopButton].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_PositivePopButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_PositivePopButton].border     = current->colors[RD_ThemeColor_PositivePopButtonBorder];
    ws->cfg_palettes[RD_PaletteCode_NegativePopButton].background = current->colors[RD_ThemeColor_NegativePopButtonBackground];
    ws->cfg_palettes[RD_PaletteCode_NegativePopButton].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_NegativePopButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_NegativePopButton].border     = current->colors[RD_ThemeColor_NegativePopButtonBorder];
    ws->cfg_palettes[RD_PaletteCode_NeutralPopButton].background = current->colors[RD_ThemeColor_NeutralPopButtonBackground];
    ws->cfg_palettes[RD_PaletteCode_NeutralPopButton].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_NeutralPopButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_NeutralPopButton].border     = current->colors[RD_ThemeColor_NeutralPopButtonBorder];
    ws->cfg_palettes[RD_PaletteCode_ScrollBarButton].background = current->colors[RD_ThemeColor_ScrollBarButtonBackground];
    ws->cfg_palettes[RD_PaletteCode_ScrollBarButton].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_ScrollBarButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_ScrollBarButton].border     = current->colors[RD_ThemeColor_ScrollBarButtonBorder];
    ws->cfg_palettes[RD_PaletteCode_Tab].background = current->colors[RD_ThemeColor_TabBackground];
    ws->cfg_palettes[RD_PaletteCode_Tab].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_Tab].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_Tab].border     = current->colors[RD_ThemeColor_TabBorder];
    ws->cfg_palettes[RD_PaletteCode_TabInactive].background = current->colors[RD_ThemeColor_TabBackgroundInactive];
    ws->cfg_palettes[RD_PaletteCode_TabInactive].text       = current->colors[RD_ThemeColor_Text];
    ws->cfg_palettes[RD_PaletteCode_TabInactive].text_weak  = current->colors[RD_ThemeColor_TextWeak];
    ws->cfg_palettes[RD_PaletteCode_TabInactive].border     = current->colors[RD_ThemeColor_TabBorderInactive];
    ws->cfg_palettes[RD_PaletteCode_DropSiteOverlay].background = current->colors[RD_ThemeColor_DropSiteOverlay];
    ws->cfg_palettes[RD_PaletteCode_DropSiteOverlay].text       = current->colors[RD_ThemeColor_DropSiteOverlay];
    ws->cfg_palettes[RD_PaletteCode_DropSiteOverlay].text_weak  = current->colors[RD_ThemeColor_DropSiteOverlay];
    ws->cfg_palettes[RD_PaletteCode_DropSiteOverlay].border     = current->colors[RD_ThemeColor_BaseBorder];
    if(rd_setting_b32_from_name(str8_lit("opaque_backgrounds")))
    {
      for EachEnumVal(RD_PaletteCode, code)
      {
        if(ws->cfg_palettes[code].background.x != 0 ||
           ws->cfg_palettes[code].background.y != 0 ||
           ws->cfg_palettes[code].background.z != 0)
        {
          ws->cfg_palettes[code].background.w = 1;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: gather listers
  //
  typedef struct ListerTask ListerTask;
  struct ListerTask
  {
    ListerTask *next;
    RD_Lister *lister;
    UI_Key root_key;
  };
  ListerTask *first_lister_task = 0;
  ListerTask *last_lister_task = 0;
  ProfScope("build all listers")
  {
    if(ws->autocomp_lister != 0 && ws->autocomp_lister_last_frame_idx+1 >= rd_state->frame_index)
    {
      ListerTask *task = push_array(scratch.arena, ListerTask, 1);
      SLLQueuePush(first_lister_task, last_lister_task, task);
      task->lister = ws->autocomp_lister;
      task->root_key = ui_key_from_stringf(ui_key_zero(), "###lister_%p", task->lister);
    }
    if(ws->top_query_lister != 0)
    {
      ListerTask *task = push_array(scratch.arena, ListerTask, 1);
      SLLQueuePush(first_lister_task, last_lister_task, task);
      task->lister = ws->top_query_lister;
      task->root_key = ui_key_from_stringf(ui_key_zero(), "###lister_%p", task->lister);
    }
  }
  
  //////////////////////////////
  //- rjf: build UI
  //
  UI_Box *lister_box = &ui_nil_box;
  UI_Box *hover_eval_box = &ui_nil_box;
  ProfScope("build UI")
  {
    ////////////////////////////
    //- rjf: set up
    //
    {
      // rjf: gather font info
      FNT_Tag main_font = rd_font_from_slot(RD_FontSlot_Main);
      F32 main_font_size = rd_font_size_from_slot(RD_FontSlot_Main);
      FNT_Tag icon_font = rd_font_from_slot(RD_FontSlot_Icons);
      
      // rjf: build icon info
      UI_IconInfo icon_info = {0};
      {
        icon_info.icon_font = icon_font;
        icon_info.icon_kind_text_map[UI_IconKind_RightArrow]     = rd_icon_kind_text_table[RD_IconKind_RightScroll];
        icon_info.icon_kind_text_map[UI_IconKind_DownArrow]      = rd_icon_kind_text_table[RD_IconKind_DownScroll];
        icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]      = rd_icon_kind_text_table[RD_IconKind_LeftScroll];
        icon_info.icon_kind_text_map[UI_IconKind_UpArrow]        = rd_icon_kind_text_table[RD_IconKind_UpScroll];
        icon_info.icon_kind_text_map[UI_IconKind_RightCaret]     = rd_icon_kind_text_table[RD_IconKind_RightCaret];
        icon_info.icon_kind_text_map[UI_IconKind_DownCaret]      = rd_icon_kind_text_table[RD_IconKind_DownCaret];
        icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]      = rd_icon_kind_text_table[RD_IconKind_LeftCaret];
        icon_info.icon_kind_text_map[UI_IconKind_UpCaret]        = rd_icon_kind_text_table[RD_IconKind_UpCaret];
        icon_info.icon_kind_text_map[UI_IconKind_CheckHollow]    = rd_icon_kind_text_table[RD_IconKind_CheckHollow];
        icon_info.icon_kind_text_map[UI_IconKind_CheckFilled]    = rd_icon_kind_text_table[RD_IconKind_CheckFilled];
      }
      
      // rjf: build widget palette info
      UI_WidgetPaletteInfo widget_palette_info = {0};
      {
        widget_palette_info.tooltip_palette   = rd_palette_from_code(RD_PaletteCode_Floating);
        widget_palette_info.ctx_menu_palette  = rd_palette_from_code(RD_PaletteCode_Floating);
        widget_palette_info.scrollbar_palette = rd_palette_from_code(RD_PaletteCode_ScrollBarButton);
      }
      
      // rjf: build animation info
      UI_AnimationInfo animation_info = {0};
      {
        if(rd_setting_b32_from_name(str8_lit("hover_animations")))       {animation_info.flags |= UI_AnimationInfoFlag_HotAnimations;}
        if(rd_setting_b32_from_name(str8_lit("press_animations")))       {animation_info.flags |= UI_AnimationInfoFlag_ActiveAnimations;}
        if(rd_setting_b32_from_name(str8_lit("focus_animations")))       {animation_info.flags |= UI_AnimationInfoFlag_FocusAnimations;}
        if(rd_setting_b32_from_name(str8_lit("tooltip_animations")))     {animation_info.flags |= UI_AnimationInfoFlag_TooltipAnimations;}
        if(rd_setting_b32_from_name(str8_lit("menu_animations")))        {animation_info.flags |= UI_AnimationInfoFlag_ContextMenuAnimations;}
        if(rd_setting_b32_from_name(str8_lit("scrolling_animations")))   {animation_info.flags |= UI_AnimationInfoFlag_ScrollingAnimations;}
      }
      
      // rjf: begin & push initial stack values
      ui_begin_build(ws->os, &ws->ui_events, &icon_info, &widget_palette_info, &animation_info, rd_state->frame_dt, rd_state->frame_dt);
      ui_push_font(main_font);
      ui_push_font_size(main_font_size);
      ui_push_text_padding(main_font_size*0.3f);
      ui_push_pref_width(ui_em(20.f, 1));
      ui_push_pref_height(ui_em(2.75f, 1.f));
      ui_push_palette(rd_palette_from_code(RD_PaletteCode_Base));
      ui_push_blur_size(10.f);
      FNT_RasterFlags text_raster_flags = 0;
      if(rd_setting_b32_from_name(str8_lit("smooth_main_text"))) {text_raster_flags |= FNT_RasterFlag_Smooth;}
      if(rd_setting_b32_from_name(str8_lit("hint_main_text"))) {text_raster_flags |= FNT_RasterFlag_Hinted;}
      ui_push_text_raster_flags(text_raster_flags);
    }
    
    ////////////////////////////
    //- rjf: calculate top-level rectangles
    //
    Rng2F32 window_rect = os_client_rect_from_window(ws->os);
    Vec2F32 window_rect_dim = dim_2f32(window_rect);
    Rng2F32 top_bar_rect = r2f32p(window_rect.x0, window_rect.y0, window_rect.x0+window_rect_dim.x+1, window_rect.y0+ui_top_pref_height().value);
    Rng2F32 bottom_bar_rect = r2f32p(window_rect.x0, window_rect_dim.y - ui_top_pref_height().value, window_rect.x0+window_rect_dim.x, window_rect.y0+window_rect_dim.y);
    Rng2F32 content_rect = r2f32p(window_rect.x0, top_bar_rect.y1, window_rect.x0+window_rect_dim.x, bottom_bar_rect.y0);
    F32 window_edge_px = os_dpi_from_window(ws->os)*0.035f;
    content_rect = pad_2f32(content_rect, -window_edge_px);
    
    ////////////////////////////
    //- rjf: truncated string hover
    //
    if(ui_string_hover_active()) UI_Tooltip
    {
      Temp scratch = scratch_begin(0, 0);
      DR_FStrList fstrs = ui_string_hover_fstrs(scratch.arena);
      UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
      ui_box_equip_display_fstrs(box, &fstrs);
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: rich hover, drag/drop tooltips
    //
    if(rd_state->hover_regs_slot != RD_RegSlot_Null ||
       (rd_state->drag_drop_regs_slot != RD_RegSlot_Null && rd_drag_is_active()))
    {
      Temp scratch = scratch_begin(0, 0);
      RD_RegSlot slot = ((rd_state->drag_drop_regs_slot != RD_RegSlot_Null && rd_drag_is_active()) ? rd_state->drag_drop_regs_slot : rd_state->hover_regs_slot);
      RD_Regs *regs = (((rd_state->drag_drop_regs_slot != RD_RegSlot_Null && rd_drag_is_active()) ? rd_state->drag_drop_regs : rd_state->hover_regs));
      CTRL_Entity *ctrl_entity = &ctrl_entity_nil;
      RD_Palette(RD_PaletteCode_Floating) switch(slot)
      {
        default:{}break;
        
        ////////////////////////
        //- rjf: cfg tooltips
        //
        case RD_RegSlot_Cfg:
        UI_Tooltip
        {
          // rjf: unpack
          RD_Cfg *cfg = rd_cfg_from_id(regs->cfg);
          DR_FStrList fstrs = rd_title_fstrs_from_cfg(scratch.arena, cfg, rd_rgba_from_theme_color(RD_ThemeColor_TextWeak), ui_top_font_size());
          
          // rjf: title
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(5, 1))
          {
            UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
            ui_box_equip_display_fstrs(box, &fstrs);
          }
        }break;
        
        ////////////////////////
        //- rjf: control entity tooltips
        //
        case RD_RegSlot_Machine:   {ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->machine);     }goto ctrl_entity_tooltip;
        case RD_RegSlot_Process:   {ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->process);     }goto ctrl_entity_tooltip;
        case RD_RegSlot_Module:    {ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->module);      }goto ctrl_entity_tooltip;
        case RD_RegSlot_Thread:    {ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->thread);      }goto ctrl_entity_tooltip;
        case RD_RegSlot_CtrlEntity:{ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->ctrl_entity); }goto ctrl_entity_tooltip;
        ctrl_entity_tooltip:;
        UI_Tooltip
        {
          // rjf: unpack
          DI_Scope *di_scope = di_scope_open();
          Arch arch = ctrl_entity->arch;
          String8 arch_str = string_from_arch(arch);
          DR_FStrList fstrs = rd_title_fstrs_from_ctrl_entity(scratch.arena, ctrl_entity,
                                                              rd_rgba_from_theme_color(RD_ThemeColor_TextWeak),
                                                              ui_top_font_size(), 0);
          
          // rjf: title
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(5, 1))
          {
            UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
            ui_box_equip_display_fstrs(box, &fstrs);
            ui_spacer(ui_em(0.5f, 1.f));
            UI_FontSize(ui_top_font_size() - 1.f)
              UI_CornerRadius(ui_top_font_size()*0.5f)
              RD_Palette(RD_PaletteCode_NeutralPopButton)
            {
              UI_FlagsAdd(UI_BoxFlag_DrawTextWeak|UI_BoxFlag_DrawBorder) ui_label(arch_str);
              ui_spacer(ui_em(0.5f, 1.f));
              if(ctrl_entity->kind == CTRL_EntityKind_Thread ||
                 ctrl_entity->kind == CTRL_EntityKind_Process)
              {
                UI_FlagsAdd(UI_BoxFlag_DrawTextWeak|UI_BoxFlag_DrawBorder) ui_labelf("ID: %i", (U32)ctrl_entity->id);
              }
            }
          }
          
          // rjf: debug info status
          if(ctrl_entity->kind == CTRL_EntityKind_Module) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
          {
            DI_Scope *di_scope = di_scope_open();
            DI_Key dbgi_key = ctrl_dbgi_key_from_module(ctrl_entity);
            RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_key, 0);
            if(rdi->raw_data_size != 0)
            {
              ui_labelf("Symbols successfully loaded from %S", dbgi_key.path);
            }
            else if(dbgi_key.path.size != 0)
            {
              ui_labelf("Symbols not found at %S", dbgi_key.path);
            }
            else if(dbgi_key.path.size == 0)
            {
              ui_labelf("Symbol information not found in module file");
            }
            di_scope_close(di_scope);
          }
          
          // rjf: unwind
          if(ctrl_entity->kind == CTRL_EntityKind_Thread)
          {
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(ctrl_entity, CTRL_EntityKind_Process);
            CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(ctrl_entity);
            CTRL_CallStack call_stack = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
            if(call_stack.count != 0)
            {
              ui_spacer(ui_em(1.5f, 1.f));
            }
            for(U64 idx = 0; idx < call_stack.count; idx += 1)
            {
              CTRL_CallStackFrame *f = &call_stack.frames[idx];
              RDI_Parsed *rdi = f->rdi;
              RDI_Procedure *procedure = f->procedure;
              U64 rip_vaddr = regs_rip_from_arch_block(arch, f->regs);
              CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
              String8 module_name = module == &ctrl_entity_nil ? str8_lit("???") : str8_skip_last_slash(module->string);
              UI_PrefWidth(ui_children_sum(1)) UI_Row
              {
                String8 name = {0};
                if(f->inline_site != 0)
                {
                  name.str = rdi_string_from_idx(rdi, f->inline_site->name_string_idx, &name.size);
                  name.size = Min(512, name.size);
                }
                else if(f->procedure != 0)
                {
                  name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
                  name.size = Min(512, name.size);
                }
                UI_TextAlignment(UI_TextAlign_Left) RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(12.f, 1)) ui_labelf("0x%I64x", rip_vaddr);
                if(f->parent_num != 0)
                {
                  RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_label(str8_lit("[inlined]"));
                }
                if(name.size != 0)
                {
                  RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
                  {
                    rd_code_label(1.f, 0, rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol), name);
                  }
                }
                else
                {
                  RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
                }
              }
            }
          }
          
          di_scope_close(di_scope);
        }break;
      }
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: drag/drop visualization tooltips
    //
    if(rd_drag_is_active() && window_is_focused)
      RD_RegsScope(.window = rd_state->drag_drop_regs->window,
                   .panel = rd_state->drag_drop_regs->panel,
                   .view = rd_state->drag_drop_regs->view)
    {
      Temp scratch = scratch_begin(0, 0);
      RD_Cfg *view = rd_cfg_from_id(rd_state->drag_drop_regs->view);
      {
        //- rjf: tab dragging
        if(rd_state->drag_drop_regs_slot == RD_RegSlot_View && view != &rd_nil_cfg)
        {
          UI_Size main_width = ui_top_pref_width();
          UI_Size main_height = ui_top_pref_height();
          UI_TextAlign main_text_align = ui_top_text_alignment();
          RD_Palette(RD_PaletteCode_Tab)
            UI_Tooltip
            UI_PrefWidth(main_width)
            UI_PrefHeight(main_height)
            UI_TextAlignment(main_text_align)
          {
            ui_set_next_pref_width(ui_em(60.f, 1.f));
            ui_set_next_pref_height(ui_em(40.f, 1.f));
            ui_set_next_child_layout_axis(Axis2_Y);
            UI_Box *container = ui_build_box_from_key(0, ui_key_zero());
            UI_Parent(container)
            {
              UI_Row UI_PrefWidth(ui_text_dim(10, 1))
              {
                DR_FStrList fstrs = rd_title_fstrs_from_cfg(scratch.arena, view, ui_top_palette()->text_weak, ui_top_font_size());
                UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                ui_box_equip_display_fstrs(name_box, &fstrs);
              }
              ui_set_next_pref_width(ui_pct(1, 0));
              ui_set_next_pref_height(ui_pct(1, 0));
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *view_preview_container = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip, "###view_preview_container");
              UI_Parent(view_preview_container) UI_Focus(UI_FocusKind_Off) UI_WidthFill
              {
                rd_view_ui(view_preview_container->rect);
              }
            }
          }
        }
      }
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: developer menu
    //
    if(ws->dev_menu_is_open) RD_Font(RD_FontSlot_Code)
    {
      ui_set_next_flags(UI_BoxFlag_ViewScrollY|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_ViewClamp);
      UI_PaneF(r2f32p(30, 30, 30+ui_top_font_size()*100, ui_top_font_size()*150), "###dev_ctx_menu")
      {
        //- rjf: capture
        if(!ProfIsCapturing() && ui_clicked(ui_buttonf("Begin Profiler Capture###prof_cap")))
        {
          ProfBeginCapture("raddbg");
        }
        else if(ProfIsCapturing() && ui_clicked(ui_buttonf("End Profiler Capture###prof_cap")))
        {
          ProfEndCapture();
        }
        
        //- rjf: toggles
        for(U64 idx = 0; idx < ArrayCount(DEV_toggle_table); idx += 1)
        {
          if(ui_clicked(rd_icon_button(*DEV_toggle_table[idx].value_ptr ? RD_IconKind_CheckFilled : RD_IconKind_CheckHollow, 0, DEV_toggle_table[idx].name)))
          {
            *DEV_toggle_table[idx].value_ptr ^= 1;
          }
        }
        
        ui_divider(ui_em(1.f, 1.f));
        
        //- rjf: draw match store stats
        ui_labelf("name match nodes: %I64u", rd_state->match_store->active_match_name_nodes_count);
        
        ui_divider(ui_em(1.f, 1.f));
        
        //- rjf: draw registers
        ui_labelf("hover_reg_slot: %i", rd_state->hover_regs_slot);
        struct
        {
          String8 name;
          RD_Regs *regs;
        }
        regs_info[] =
        {
          {str8_lit("regs"),       rd_regs()},
          {str8_lit("hover_regs"), rd_state->hover_regs},
        };
        for EachElement(idx, regs_info)
        {
          ui_divider(ui_em(1.f, 1.f));
          ui_label(regs_info[idx].name);
          RD_Regs *regs = regs_info[idx].regs;
#define ID(name) ui_labelf("%s: $0x%I64x", #name, (regs->name))
          ID(window);
          ID(panel);
          ID(view);
#undef ID
#define Handle(name) ui_labelf("%s: [0x%I64x, 0x%I64x]", #name, (regs->name).machine_id, (regs->name).dmn_handle.u64[0])
          Handle(machine);
          Handle(process);
          Handle(module);
          Handle(thread);
#undef Handle
          ui_labelf("file_path: \"%S\"", regs->file_path);
          ui_labelf("cursor: (L:%I64d, C:%I64d)", regs->cursor.line, regs->cursor.column);
          ui_labelf("mark: (L:%I64d, C:%I64d)", regs->mark.line, regs->mark.column);
          ui_labelf("unwind_count: %I64u", regs->unwind_count);
          ui_labelf("inline_depth: %I64u", regs->inline_depth);
          ui_labelf("text_key: [0x%I64x, 0x%I64x]", regs->text_key.u64[0], regs->text_key.u64[1]);
          ui_labelf("lang_kind: '%S'", txt_extension_from_lang_kind(regs->lang_kind));
          ui_labelf("vaddr_range: [0x%I64x, 0x%I64x)", regs->vaddr_range.min, regs->vaddr_range.max);
          ui_labelf("voff_range: [0x%I64x, 0x%I64x)", regs->voff_range.min, regs->voff_range.max);
        }
        
        ui_divider(ui_em(1.f, 1.f));
        
        //- rjf: draw per-window stats
        for(RD_WindowState *w = rd_state->first_window_state; w != &rd_nil_window_state; w = w->order_next)
        {
          // rjf: calc ui hash chain length
          F64 avg_ui_hash_chain_length = 0;
          {
            F64 chain_count = 0;
            F64 chain_length_sum = 0;
            for(U64 idx = 0; idx < w->ui->box_table_size; idx += 1)
            {
              F64 chain_length = 0;
              for(UI_Box *b = w->ui->box_table[idx].hash_first; !ui_box_is_nil(b); b = b->hash_next)
              {
                chain_length += 1;
              }
              if(chain_length > 0)
              {
                chain_length_sum += chain_length;
                chain_count += 1;
              }
            }
            avg_ui_hash_chain_length = chain_length_sum / chain_count;
          }
          ui_labelf("Target Hz: %.2f", 1.f/rd_state->frame_dt);
          ui_labelf("Ctrl Run Index: %I64u", ctrl_run_gen());
          ui_labelf("Ctrl Mem Gen Index: %I64u", ctrl_mem_gen());
          ui_labelf("Window %p", w);
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_pref_height(ui_children_sum(1));
          UI_Row
          {
            ui_spacer(ui_em(2.f, 1.f));
            ui_labelf("Box Count: %I64u", w->ui->last_build_box_count);
          }
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_pref_height(ui_children_sum(1));
          UI_Row
          {
            ui_spacer(ui_em(2.f, 1.f));
            ui_labelf("Average UI Hash Chain Length: %f", avg_ui_hash_chain_length);
          }
        }
        
        ui_divider(ui_em(1.f, 1.f));
      }
    }
    
    ////////////////////////////
    //- rjf: do query lister stack controls
    //
    if(ws->top_query_lister != 0)
    {
      if(ui_slot_press(UI_EventActionSlot_Cancel))
      {
        rd_cmd(RD_CmdKind_CancelQuery);
      }
      if(ui_slot_press(UI_EventActionSlot_Accept))
      {
        Temp scratch = scratch_begin(0, 0);
        RD_RegsScope()
        {
          //rd_regs_fill_slot_from_string(query->slot, str8(ws->query_input_buffer, ws->query_input_string_size));
          rd_cmd(RD_CmdKind_CompleteQuery);
        }
        scratch_end(scratch);
      }
    }
    
    ////////////////////////////
    //- rjf: build listers
    //
    for(ListerTask *task = first_lister_task; task != 0; task = task->next)
    {
      DI_Scope *di_scope = di_scope_open();
      
      //////////////////////////
      //- rjf: unpack
      //
      RD_Lister *lister = task->lister;
      RD_ListerFlags flags = lister->regs->lister_flags;
      UI_Box *anchor_box = ui_box_from_key(lister->regs->ui_key);
      
      //////////////////////////
      //- rjf: parameters -> lister items
      //
      RD_ListerItemArray item_array = rd_lister_item_array_from_regs_needle_cursor_off(scratch.arena, lister->regs, str8(lister->input_buffer, lister->input_string_size), lister->input_cursor.column-1);
      
      //////////////////////////
      //- rjf: selected item hash -> cursor
      //
      Vec2S64 cursor = {0};
      for EachIndex(idx, item_array.count)
      {
        RD_ListerItem *item = &item_array.v[idx];
        U64 hash = rd_hash_from_lister_item(item);
        if(hash == lister->selected_item_hash)
        {
          cursor.y = (S64)(idx+1);
          break;
        }
      }
      
      //////////////////////////
      //- rjf: push autocompletion hint
      //
      if(1 <= cursor.y && cursor.y <= item_array.count)
      {
        RD_ListerItem *item = &item_array.v[cursor.y-1];
        if(item->flags & RD_ListerItemFlag_Autocompletion)
        {
          UI_Event evt = zero_struct;
          evt.kind   = UI_EventKind_AutocompleteHint;
          evt.string = item->string;
          ui_event_list_push(ui_build_arena(), &ws->ui_events, &evt);
        }
      }
      
      //////////////////////////
      //- rjf: animate values
      //
      F32 fast_rate = 1 - pow_f32(2, (-40.f * rd_state->frame_dt));
      F32 lister_open_t        = ui_anim(ui_key_from_stringf(ui_key_zero(), "lister_open_%p", lister), 1.f);
      F32 lister_num_of_rows_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "lister_num_of_rows_%p", lister), (F32)item_array.count);
      lister->scroll_pt.off -= lister->scroll_pt.off*fast_rate;
      
      //////////////////////////
      //- rjf: unpack build parameters
      //
      F32 squish       = (0.25f - 0.25f*lister_open_t);
      F32 transparency = (1.f - lister_open_t);
      F32 row_height_px = floor_f32(ui_top_font_size()*3.f);
      if(lister->regs->lister_flags & RD_ListerFlag_Descriptions)
      {
        row_height_px += floor_f32(ui_top_font_size()*3.f);
      }
      Vec2F32 content_rect_dim = dim_2f32(content_rect);
      Vec2F32 content_rect_center = center_2f32(content_rect);
      F32 line_edit_height_px = 0;
      if(lister->regs->lister_flags & RD_ListerFlag_LineEdit)
      {
        line_edit_height_px = floor_f32(ui_top_font_size()*3.f);
      }
      line_edit_height_px = Min(line_edit_height_px, row_height_px);
      F32 lister_height_max = content_rect_dim.y*0.9f;
      Vec2F32 lister_dim_px =
      {
        content_rect_dim.x*0.5f,
        line_edit_height_px + lister_num_of_rows_t*row_height_px,
      };
      if(!ui_box_is_nil(anchor_box) && flags & RD_ListerFlag_SizeByAnchor)
      {
        lister_dim_px.x = dim_2f32(anchor_box->rect).x;
      }
      else if(!ui_box_is_nil(anchor_box))
      {
        lister_dim_px.x = ui_top_font_size()*50.f;
      }
      lister_dim_px.x = Max(lister_dim_px.x, ui_top_font_size()*50.f);
      lister_dim_px.y = Min(lister_dim_px.y, lister_height_max);
      Vec2F32 lister_pos_px =
      {
        content_rect_center.x - lister_dim_px.x/2,
        content_rect_center.y - content_rect_dim.y*0.9f/2,
      };
      if(!ui_box_is_nil(anchor_box))
      {
        lister_pos_px = v2f32(anchor_box->rect.x0 + lister->regs->off_px.x,
                              anchor_box->rect.y0 + lister->regs->off_px.y);
      }
      
      //////////////////////////
      //- rjf: build top-level lister box
      //
      ui_set_next_fixed_x(lister_pos_px.x);
      ui_set_next_fixed_y(lister_pos_px.y);
      ui_set_next_pref_width(ui_px(lister_dim_px.x, 1.f));
      ui_set_next_pref_height(ui_px(lister_dim_px.y, 1.f));
      ui_set_next_child_layout_axis(Axis2_Y);
      ui_set_next_corner_radius_01(ui_top_font_size()*0.25f);
      ui_set_next_corner_radius_11(ui_top_font_size()*0.25f);
      UI_Focus(UI_FocusKind_On)
        UI_Squish(squish)
        UI_Transparency(transparency)
        RD_Palette(RD_PaletteCode_Floating)
      {
        lister_box = ui_build_box_from_key(UI_BoxFlag_Clickable|
                                           UI_BoxFlag_Clip|
                                           UI_BoxFlag_RoundChildrenByParent|
                                           UI_BoxFlag_DisableFocusOverlay|
                                           UI_BoxFlag_DrawBorder|
                                           UI_BoxFlag_DrawBackgroundBlur|
                                           UI_BoxFlag_DrawDropShadow|
                                           UI_BoxFlag_DrawBackground,
                                           task->root_key);
      }
      
      //////////////////////////
      //- rjf: build lister line edit
      //
      if(flags & RD_ListerFlag_LineEdit)
        UI_WidthFill
        UI_Parent(lister_box)
        UI_Focus(UI_FocusKind_On)
        RD_Font(RD_FontSlot_Code)
      {
        UI_PrefHeight(ui_px(line_edit_height_px, 1.f))
        {
#if 0 // TODO(rjf): @cfg
          UI_Signal sig = rd_line_edit(RD_LineEditFlag_Border|RD_LineEditFlag_CodeContents,
                                       0,
                                       0,
                                       &lister->input_cursor,
                                       &lister->input_mark,
                                       lister->input_buffer,
                                       sizeof(lister->input_buffer),
                                       &lister->input_string_size,
                                       0,
                                       str8(lister->input_buffer, lister->input_string_size),
                                       str8_lit("###lister_text_input"));
#endif
        }
      }
      
      //////////////////////////
      //- rjf: build lister contents
      //
      UI_ScrollListParams params =
      {
        .flags         = UI_ScrollListFlag_All,
        .dim_px        = v2f32(lister_dim_px.x, lister_dim_px.y - line_edit_height_px),
        .row_height_px = row_height_px,
        .cursor_range  = r2s64(v2s64(0, 0), v2s64(0, item_array.count)),
        .item_range    = r1s64(0, item_array.count),
        .cursor_min_is_empty_selection[Axis2_Y] = 1,
      };
      Rng1S64 visible_row_range = {0};
      UI_ScrollListSignal scroll_list_signal = {0};
      UI_WidthFill UI_HeightFill
        UI_Parent(lister_box)
        UI_Focus(UI_FocusKind_On)
        UI_ScrollList(&params, &lister->scroll_pt, &cursor, 0, &visible_row_range, &scroll_list_signal)
        UI_Focus(UI_FocusKind_Null)
        RD_Palette(RD_PaletteCode_ImplicitButton)
      {
        UI_HoverCursor(OS_Cursor_HandPoint)
          for(S64 idx = visible_row_range.min; 0 <= idx && idx < visible_row_range.max && idx < item_array.count; idx += 1)
        {
          RD_ListerItem *item = &item_array.v[idx];
          B32 item_is_selected = (idx == cursor.y-1);
          UI_FocusHot(item_is_selected ? UI_FocusKind_On : UI_FocusKind_Off)
          {
            //- rjf: build the top-level box for the item
            UI_Box *item_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects|UI_BoxFlag_MouseClickable, "autocomp_%I64x", idx);
            
            //- rjf: build item contents
            UI_Parent(item_box) UI_Padding(ui_em(1.f, 1.f))
            {
              // rjf: icon
              if(item->icon_kind != RD_IconKind_Null)
                UI_PrefWidth(ui_em(2.f, 1.f))
                UI_Column
                UI_Padding(ui_pct(1, 0))
                RD_Font(RD_FontSlot_Icons)
              {
                ui_label(rd_icon_kind_text_table[item->icon_kind]);
              }
              
              // rjf: name / description
              UI_Column UI_Padding(ui_pct(1, 0)) UI_PrefHeight(ui_em(2.f, 1.f))
              {
                // rjf: name
                UI_WidthFill UI_Row UI_PrefWidth(ui_text_dim(1, 0)) UI_TextAlignment(UI_TextAlign_Center)
                {
                  // rjf: display name
                  RD_Font(item->flags & RD_ListerItemFlag_IsNonCode ? RD_FontSlot_Main : RD_FontSlot_Code)
                  {
                    UI_Box *box = item->flags & RD_ListerItemFlag_IsNonCode ? ui_label(item->display_name).box : rd_code_label(1.f, 0, ui_top_palette()->text, item->display_name);
                    ui_box_equip_fuzzy_match_ranges(box, &item->display_name__matches);
                  }
                  
                  // rjf: kind name
                  if(flags & RD_ListerFlag_KindLabel && item->kind_name.size != 0)
                  {
                    ui_spacer(ui_em(1.f, 1.f));
                    RD_Palette(RD_PaletteCode_Floating) UI_CornerRadius(ui_top_font_size()*0.5f)
                    {
                      ui_set_next_pref_width(ui_children_sum(1));
                      ui_set_next_flags(UI_BoxFlag_DrawBorder);
                      UI_Row UI_Padding(ui_em(0.5f, 1.f))
                      {
                        UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText|UI_BoxFlag_DrawTextWeak, ui_key_zero());
                        ui_box_equip_display_string(box, item->kind_name);
                      }
                    }
                  }
                }
                
                // rjf: description
                if(flags & RD_ListerFlag_Descriptions)
                {
                  if(item->description.size != 0) UI_WidthFill UI_Row UI_PrefWidth(ui_text_dim(1, 0)) UI_Flags(UI_BoxFlag_DrawTextWeak)
                  {
                    UI_Box *box = ui_label(item->description).box;
                    ui_box_equip_fuzzy_match_ranges(box, &item->description__matches);
                  }
                }
              }
              
              // rjf: bindings
              if(item->flags & RD_ListerItemFlag_Bindings) RD_Palette(RD_PaletteCode_Floating) UI_Focus(UI_FocusKind_Off)
              {
                ui_set_next_flags(UI_BoxFlag_Clickable);
                UI_PrefWidth(ui_children_sum(1.f)) UI_HeightFill UI_NamedColumn(str8_lit("binding_column")) UI_Padding(ui_px(row_height_px/4.f, 0.f))
                {
                  ui_set_next_flags(UI_BoxFlag_Clickable);
                  UI_NamedRow(str8_lit("binding_row"))
                  {
                    rd_cmd_binding_buttons(item->string);
                  }
                }
              }
            }
            
            //- rjf: do interaction with top-level item
            UI_Signal item_sig = ui_signal_from_box(item_box);
            if(ui_clicked(item_sig))
            {
#if 0 // TODO(rjf): @cfg_lister
              UI_Event move_back_evt = zero_struct;
              move_back_evt.kind = UI_EventKind_Navigate;
              move_back_evt.flags = UI_EventFlag_KeepMark;
              move_back_evt.delta_2s32.x = -(S32)query_word.size;
              ui_event_list_push(ui_build_arena(), &ws->ui_events, &move_back_evt);
              UI_Event paste_evt = zero_struct;
              paste_evt.kind = UI_EventKind_Text;
              paste_evt.string = item->string;
              ui_event_list_push(ui_build_arena(), &ws->ui_events, &paste_evt);
              lister_box->default_nav_focus_hot_key = lister_box->default_nav_focus_active_key = lister_box->default_nav_focus_next_hot_key = lister_box->default_nav_focus_next_active_key = ui_key_zero();
#endif
            }
            else if(item_box->flags & UI_BoxFlag_FocusHot && !(item_box->flags & UI_BoxFlag_FocusHotDisabled))
            {
              UI_Event evt = zero_struct;
              evt.kind   = UI_EventKind_AutocompleteHint;
              evt.string = item->string;
              ui_event_list_push(ui_build_arena(), &ws->ui_events, &evt);
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: cursor -> selected item hash
      //
      if(1 <= cursor.y && cursor.y <= item_array.count)
      {
        RD_ListerItem *item = &item_array.v[cursor.y-1];
        U64 hash = rd_hash_from_lister_item(item);
        lister->selected_item_hash = hash;
      }
      else
      {
        lister->selected_item_hash = 0;
      }
      
      di_scope_close(di_scope);
    }
    
#if 0 // TODO(rjf): @cfg_lister
    ////////////////////////////
    //- rjf: prepare query state for the in-progress query command
    //
    if(ws->query_cmd_name.size != 0)
    {
      RD_CmdKindInfo *cmd_kind_info = rd_cmd_kind_info_from_string(ws->query_cmd_name);
      RD_RegSlot missing_slot = cmd_kind_info->query.slot;
      
      //- rjf: if our input is stale, w.r.t. the current query command, initialize input state
      if(ws->query_input_gen < ws->query_cmd_gen)
      {
        ws->query_input_gen = ws->query_cmd_gen;
        String8 default_query_input = {0};
        ws->query_input_string_size = Min(sizeof(ws->query_input_buffer), default_query_input.size);
        MemoryCopy(ws->query_input_buffer, default_query_input.str, ws->query_input_string_size);
        ws->query_input_cursor = ws->query_input_mark = txt_pt(1, ws->query_input_string_size+1);
      }
      
#if 0 // TODO(rjf): @cfg_lister (query state prep)
      String8 query_view_name = cmd_kind_info->query.view_name;
      if(query_view_name.size == 0)
      {
        switch(missing_slot)
        {
          default:{}break;
          case RD_RegSlot_Thread:
          case RD_RegSlot_Module:
          case RD_RegSlot_Process:
          case RD_RegSlot_Machine:
          case RD_RegSlot_CtrlEntity:{query_view_name = rd_view_rule_kind_info_table[RD_ViewRuleKind_CtrlEntityLister].string;}break;
          case RD_RegSlot_Entity:    {query_view_name = rd_view_rule_kind_info_table[RD_ViewRuleKind_EntityLister].string;}break;
          case RD_RegSlot_EntityList:{query_view_name = rd_view_rule_kind_info_table[RD_ViewRuleKind_EntityLister].string;}break;
          case RD_RegSlot_FilePath:  {query_view_name = rd_view_rule_kind_info_table[RD_ViewRuleKind_FileSystem].string;}break;
          case RD_RegSlot_PID:       {query_view_name = rd_view_rule_kind_info_table[RD_ViewRuleKind_SystemProcesses].string;}break;
        }
      }
      RD_ViewRuleInfo *view_spec = rd_view_rule_info_from_string(query_view_name);
      if(!str8_match(ws->query_view_stack_top->string, view_spec->string, 0) ||
         ws->query_view_stack_top == &rd_nil_cfg)
      {
        Temp scratch = scratch_begin(0, 0);
        
        // rjf: clear existing query stack
        rd_cfg_release(ws->query_view_stack_top);
        
        // rjf: determine default query
        String8 default_query = {0};
        switch(missing_slot)
        {
          default:
          if(cmd_kind_info->query.flags & RD_QueryFlag_KeepOldInput)
          {
            default_query = rd_push_search_string(scratch.arena);
          }break;
          case RD_RegSlot_FilePath:
          {
            default_query = path_normalized_from_string(scratch.arena, rd_state->current_path);
            default_query = push_str8f(scratch.arena, "%S/", default_query);
          }break;
        }
        
        // rjf: construct & push new view
        RD_Cfg *bucket = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("transient"));
        RD_Cfg *view = rd_cfg_new(bucket, view_spec->string);
        RD_Cfg *filter = rd_cfg_new(view, str8_lit("filter"));
        rd_cfg_new(filter, default_query);
        RD_ViewState *view_state = rd_view_state_from_cfg(view);
        view_state->filter_cursor = txt_pt(1, default_query.size+1);
        if(cmd_kind_info->query.flags & RD_QueryFlag_SelectOldInput)
        {
          view_state->filter_mark = txt_pt(1, 1);
        }
        ws->query_view_stack_top = view;
        ws->query_view_selected = 1;
        
        scratch_end(scratch);
      }
#endif
    }
    
    ////////////////////////////
    //- rjf: build query
    //
    if(query_is_open)
      UI_Focus((window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && ws->query_input_selected) ? UI_FocusKind_On : UI_FocusKind_Off)
      RD_Palette(RD_PaletteCode_Floating)
    {
      String8 query_cmd_name = ws->query_cmd_name;
      RD_CmdKindInfo *query_cmd_info = rd_cmd_kind_info_from_string(query_cmd_name);
      RD_Query *query = &query_cmd_info->query;
      F32 query_view_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "###%p_query_view_t", ws), 1.f);
      F32 query_squish = 0.25f - query_view_t*0.25f;
      F32 query_transparency = 1.f - query_view_t;
      
      //- rjf: calculate rectangles
      Vec2F32 window_center = center_2f32(window_rect);
      Vec2F32 query_input_dim = v2f32(dim_2f32(window_rect).x*0.5f, ui_top_font_size()*3.f);
      F32 query_input_margin = ui_top_font_size()*8.f;
      Rng2F32 query_input_rect = r2f32p(window_center.x - query_input_dim.x/2,
                                        window_rect.y0 + query_input_margin,
                                        window_center.x + query_input_dim.x/2,
                                        window_rect.y0 + query_input_margin + query_input_dim.y);
      
      //- rjf: build floating query container
      UI_Box *query_container_box = &ui_nil_box;
      UI_Rect(query_input_rect)
        UI_CornerRadius00(ui_top_font_size()*0.2f)
        UI_CornerRadius10(ui_top_font_size()*0.2f)
        UI_ChildLayoutAxis(Axis2_Y)
        UI_Squish(query_squish)
        UI_Transparency(query_transparency)
      {
        query_container_box = ui_build_box_from_stringf(UI_BoxFlag_Floating|
                                                        UI_BoxFlag_AllowOverflow|
                                                        UI_BoxFlag_Clickable|
                                                        UI_BoxFlag_Clip|
                                                        UI_BoxFlag_DisableFocusOverlay|
                                                        UI_BoxFlag_DrawBorder|
                                                        UI_BoxFlag_DrawBackground|
                                                        UI_BoxFlag_DrawBackgroundBlur|
                                                        UI_BoxFlag_DrawDropShadow,
                                                        "query_container");
      }
      
      //- rjf: set up autocompletion lister info
      if(ui_is_focus_active())
      {
        rd_set_autocomp_lister_query(.anchor_key  = query_container_box->key,
                                     .anchor_off  = v2f32(0, dim_2f32(query_container_box->rect).y - dim_2f32(query_container_box->rect).y*(1-query_view_t)*0.25f - 2.f),
                                     .flags       = 0xffffffff,
                                     .input       = str8(ws->query_input_buffer, ws->query_input_string_size),
                                     .cursor_off  = ws->query_input_cursor.column-1,
                                     .squish      = query_squish,
                                     .transparency= query_transparency);
      }
      
      //- rjf: build query text input
      B32 query_completed = 0;
      B32 query_cancelled = 0;
      UI_Parent(query_container_box)
        UI_WidthFill UI_HeightFill
        UI_Focus(UI_FocusKind_On)
      {
        ui_set_next_flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder);
        UI_Row
        {
          UI_PrefWidth(ui_text_dim(0.f, 1.f)) UI_Padding(ui_em(1.f, 1.f))
          {
            RD_IconKind icon_kind = query_cmd_info->icon_kind;
            if(icon_kind != RD_IconKind_Null)
            {
              RD_Font(RD_FontSlot_Icons) ui_label(rd_icon_kind_text_table[icon_kind]);
            }
            ui_labelf("%S", query_cmd_info->display_name);
          }
          RD_Font((query->flags & RD_QueryFlag_CodeInput) ? RD_FontSlot_Code : RD_FontSlot_Main)
            UI_TextPadding(ui_top_font_size()*0.5f)
          {
            UI_Signal sig = rd_line_edit(RD_LineEditFlag_Border|
                                         (RD_LineEditFlag_CodeContents * !!(query->flags & RD_QueryFlag_CodeInput)),
                                         0,
                                         0,
                                         &ws->query_input_cursor,
                                         &ws->query_input_mark,
                                         ws->query_input_buffer,
                                         sizeof(ws->query_input_buffer),
                                         &ws->query_input_string_size,
                                         0,
                                         str8(ws->query_input_buffer, ws->query_input_string_size),
                                         str8_lit("###query_text_input"));
            if(ui_pressed(sig))
            {
              ws->query_input_selected = 1;
            }
          }
          UI_PrefWidth(ui_em(5.f, 1.f)) UI_Focus(UI_FocusKind_Off) RD_Palette(RD_PaletteCode_PositivePopButton)
          {
            if(ui_clicked(rd_icon_buttonf(RD_IconKind_RightArrow, 0, "##complete_query")))
            {
              query_completed = 1;
            }
          }
          UI_PrefWidth(ui_em(3.f, 1.f)) UI_Focus(UI_FocusKind_Off) RD_Palette(RD_PaletteCode_PlainButton)
          {
            if(ui_clicked(rd_icon_buttonf(RD_IconKind_X, 0, "##cancel_query")))
            {
              query_cancelled = 1;
            }
          }
        }
      }
      
      //- rjf: query submission
      if(((ui_is_focus_active() || (window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && !ws->query_input_selected)) &&
          ui_slot_press(UI_EventActionSlot_Cancel)) || query_cancelled)
      {
        rd_cmd(RD_CmdKind_CancelQuery);
      }
      if((ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept)) || query_completed)
      {
        Temp scratch = scratch_begin(0, 0);
        RD_RegsScope()
        {
          rd_regs_fill_slot_from_string(query->slot, str8(ws->query_input_buffer, ws->query_input_string_size));
          rd_cmd(RD_CmdKind_CompleteQuery);
        }
        scratch_end(scratch);
      }
      
      //- rjf: take fallthrough interaction in query view
      {
        UI_Signal sig = ui_signal_from_box(query_container_box);
        if(ui_pressed(sig))
        {
          ws->query_input_selected = 1;
        }
      }
    }
    else
    {
      ws->query_input_selected = 0;
    }
    
    ////////////////////////////
    //- rjf: darken lower layer ui when query is selected
    //
    F32 query_input_selected_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "%p_query_input_selected", ws), (F32)!!ws->query_input_selected);
    UI_Palette(ui_build_palette(0, .background = mix_4f32(rd_rgba_from_theme_color(RD_ThemeColor_InactivePanelOverlay), v4f32(0, 0, 0, 0), 1-query_input_selected_t)))
      UI_Rect(window_rect)
    {
      ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
    }
#endif
    
    ////////////////////////////
    //- rjf: top-level registers context menu
    //
    RD_Palette(RD_PaletteCode_Floating) UI_CtxMenu(rd_state->ctx_menu_key)
      UI_PrefWidth(ui_em(50.f, 1.f))
      RD_Palette(RD_PaletteCode_ImplicitButton)
    {
      Temp scratch = scratch_begin(0, 0);
      RD_Regs *regs = ws->ctx_menu_regs;
      RD_RegSlot slot = ws->ctx_menu_regs_slot;
      CTRL_Entity *ctrl_entity = &ctrl_entity_nil;
      {
        switch(slot)
        {
          default:{}break;
          
          //////////////////////
          //- rjf: source code locations
          //
          case RD_RegSlot_Cursor:
          {
            // TODO(rjf): with new registers-based commands, all of this can be deduplicated with the
            // command-based path, but I am holding off on that until post 0.9.12 - these should be
            // able to just all push commands for their corresponding actions
            //
            TXT_Scope *txt_scope = txt_scope_open();
            HS_Scope *hs_scope = hs_scope_open();
            TxtRng range = txt_rng(regs->cursor, regs->mark);
            D_LineList lines = regs->lines;
            if(!txt_pt_match(range.min, range.max) && ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_Copy].string)))
            {
              U128 hash = {0};
              TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, regs->text_key, regs->lang_kind, &hash);
              String8 data = hs_data_from_hash(hs_scope, hash);
              String8 copy_data = txt_string_from_info_data_txt_rng(&info, data, range);
              os_set_clipboard_text(copy_data);
              ui_ctx_menu_close();
            }
            if(range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_RightArrow, 0, "Set Next Statement")))
            {
              CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
              U64 new_rip_vaddr = regs->vaddr;
              if(regs->file_path.size != 0)
              {
                for(D_LineNode *n = lines.first; n != 0; n = n->next)
                {
                  CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &n->v.dbgi_key);
                  CTRL_Entity *module = ctrl_module_from_thread_candidates(d_state->ctrl_entity_store, thread, &modules);
                  if(module != &ctrl_entity_nil)
                  {
                    new_rip_vaddr = ctrl_vaddr_from_voff(module, n->v.voff_range.min);
                    break;
                  }
                }
              }
              rd_cmd(RD_CmdKind_SetThreadIP, .vaddr = new_rip_vaddr);
              ui_ctx_menu_close();
            }
            if(range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_Play, 0, "Run To Line")))
            {
              if(regs->file_path.size != 0)
              {
                rd_cmd(RD_CmdKind_RunToLine, .file_path = regs->file_path, .cursor = range.min);
              }
              else
              {
                rd_cmd(RD_CmdKind_RunToLine, .vaddr = regs->vaddr);
              }
              ui_ctx_menu_close();
            }
            if(range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_Null, 0, "Go To Name")))
            {
              U128 hash = {0};
              TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, regs->text_key, regs->lang_kind, &hash);
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
              rd_cmd(RD_CmdKind_GoToName, .string = expr);
              ui_ctx_menu_close();
            }
            if(range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_CircleFilled, 0, "Toggle Breakpoint")))
            {
              rd_cmd(RD_CmdKind_ToggleBreakpoint,
                     .file_path = regs->file_path,
                     .cursor    = range.min,
                     .vaddr     = regs->vaddr);
              ui_ctx_menu_close();
            }
            if(range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_Binoculars, 0, "Toggle Watch Expression")))
            {
              U128 hash = {0};
              TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, regs->text_key, regs->lang_kind, &hash);
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
              rd_cmd(RD_CmdKind_ToggleWatchExpression, .string = expr);
              ui_ctx_menu_close();
            }
            if(regs->file_path.size == 0 && range.min.line == range.max.line && ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_GoToSource].string)))
            {
              rd_cmd(RD_CmdKind_GoToSource, .lines = lines);
              ui_ctx_menu_close();
            }
            if(regs->file_path.size != 0 && range.min.line == range.max.line && ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_GoToDisassembly].string)))
            {
              rd_cmd(RD_CmdKind_GoToDisassembly, .lines = lines);
              ui_ctx_menu_close();
            }
            hs_scope_close(hs_scope);
            txt_scope_close(txt_scope);
          }break;
          
          //////////////////////
          //- rjf: tabs
          //
          case RD_RegSlot_View:
          {
#if 0 // TODO(rjf): @cfg
            RD_Cfg *tab = rd_cfg_from_id(regs->view);
            RD_RegsScope(.view = regs->view)
            {
              String8 expr = rd_view_expr_string();
              RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(tab->string);
              RD_IconKind view_icon = view_rule_info->icon_kind;
              DR_FStrList fstrs = rd_title_fstrs_from_view(scratch.arena, view_rule_info->display_name, expr, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
              String8 file_path = rd_file_path_from_eval_string(scratch.arena, expr);
              
              // rjf: title
              UI_Row
              {
                ui_spacer(ui_em(1.f, 1.f));
                RD_Font(RD_FontSlot_Icons)
                  UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
                  UI_PrefWidth(ui_em(2.f, 1.f))
                  UI_PrefHeight(ui_pct(1, 0))
                  UI_TextAlignment(UI_TextAlign_Center)
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  ui_label(rd_icon_kind_text_table[view_icon]);
                UI_PrefWidth(ui_text_dim(10, 1))
                {
                  UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                  ui_box_equip_display_fstrs(name_box, &fstrs);
                }
              }
              
              RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
              
              // rjf: copy name
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_Clipboard, 0, "Copy Name")))
              {
                os_set_clipboard_text(dr_string_from_fstrs(scratch.arena, &fstrs));
                ui_ctx_menu_close();
              }
              
              // rjf: copy full path
              if(file_path.size != 0)
              {
                UI_Signal copy_full_path_sig = rd_icon_buttonf(RD_IconKind_Clipboard, 0, "Copy Full Path");
                String8 full_path = path_normalized_from_string(scratch.arena, file_path);
                if(ui_clicked(copy_full_path_sig))
                {
                  os_set_clipboard_text(full_path);
                  ui_ctx_menu_close();
                }
                if(ui_hovering(copy_full_path_sig)) UI_Tooltip
                {
                  ui_label(full_path);
                }
              }
              
              // rjf: show in explorer
              if(file_path.size != 0)
              {
                UI_Signal sig = rd_icon_buttonf(RD_IconKind_FolderClosedFilled, 0, "Show In Explorer");
                if(ui_clicked(sig))
                {
                  String8 full_path = path_normalized_from_string(scratch.arena, file_path);
                  os_show_in_filesystem_ui(full_path);
                  ui_ctx_menu_close();
                }
              }
              
              // rjf: filter controls
              if(view_rule_info->flags & RD_ViewRuleInfoFlag_CanFilter)
              {
                if(ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_Filter].string)))
                {
                  rd_cmd(RD_CmdKind_Filter);
                  ui_ctx_menu_close();
                }
                if(ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_ClearFilter].string)))
                {
                  rd_cmd(RD_CmdKind_ClearFilter);
                  ui_ctx_menu_close();
                }
              }
              
              // rjf: close tab
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_X, 0, "Close Tab")))
              {
                rd_cmd(RD_CmdKind_CloseTab);
                ui_ctx_menu_close();
              }
              
              // rjf: param tree editing
#if 0
              UI_TextPadding(ui_top_font_size()*1.5f) RD_Font(RD_FontSlot_Code)
              {
                Temp scratch = scratch_begin(0, 0);
                String8 schema_string = view->spec->params_schema;
                MD_TokenizeResult schema_tokenize = md_tokenize_from_text(scratch.arena, schema_string);
                MD_ParseResult schema_parse = md_parse_from_text_tokens(scratch.arena, str8_zero(), schema_string, schema_tokenize.tokens);
                MD_Node *schema_root = schema_parse.root->first;
                if(!md_node_is_nil(schema_root))
                {
                  if(!md_node_is_nil(schema_root->first))
                  {
                    RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
                  }
                  for MD_EachNode(key, schema_root->first)
                  {
                    UI_Row
                    {
                      MD_Node *params = view->params_roots[view->params_write_gen%ArrayCount(view->params_roots)];
                      MD_Node *param_tree = md_child_from_string(params, key->string, 0);
                      String8 pre_edit_value = md_string_from_children(scratch.arena, param_tree);
                      UI_PrefWidth(ui_em(10.f, 1.f)) ui_label(key->string);
                      UI_Signal sig = rd_line_editf(RD_LineEditFlag_Border|RD_LineEditFlag_CodeContents, 0, 0, &ws->ctx_menu_input_cursor, &ws->ctx_menu_input_mark, ws->ctx_menu_input_buffer, ws->ctx_menu_input_buffer_size, &ws->ctx_menu_input_string_size, 0, pre_edit_value, "%S##view_param", key->string);
                      if(ui_committed(sig))
                      {
                        String8 new_string = str8(ws->ctx_menu_input_buffer, ws->ctx_menu_input_string_size);
                        rd_view_store_param(view, key->string, new_string);
                      }
                    }
                  }
                }
                scratch_end(scratch);
              }
#endif
            }
#endif
          }break;
          
          //////////////////////
          //- rjf: ctrl entities
          //
          case RD_RegSlot_Machine:     ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->machine);     goto ctrl_entity_title;
          case RD_RegSlot_Process:     ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->process);     goto ctrl_entity_title;
          case RD_RegSlot_Module:      ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->module);      goto ctrl_entity_title;
          case RD_RegSlot_Thread:      ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->thread);      goto ctrl_entity_title;
          case RD_RegSlot_CtrlEntity:  ctrl_entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, regs->ctrl_entity); goto ctrl_entity_title;
          ctrl_entity_title:;
          {
            //- rjf: title
            UI_Row
              UI_PrefWidth(ui_text_dim(5, 1))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_TextPadding(ui_top_font_size()*1.5f)
            {
              DR_FStrList fstrs = rd_title_fstrs_from_ctrl_entity(scratch.arena, ctrl_entity, ui_top_palette()->text_weak, ui_top_font_size(), 0);
              UI_Box *title_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
              ui_box_equip_display_fstrs(title_box, &fstrs);
              if(ctrl_entity->kind == CTRL_EntityKind_Thread)
              {
                ui_spacer(ui_em(0.5f, 1.f));
                UI_FontSize(ui_top_font_size() - 1.f)
                  UI_CornerRadius(ui_top_font_size()*0.5f)
                  RD_Palette(RD_PaletteCode_NeutralPopButton)
                  UI_TextPadding(ui_top_font_size()*0.5f)
                {
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak|UI_BoxFlag_DrawBorder) ui_label(string_from_arch(ctrl_entity->arch));
                  ui_spacer(ui_em(0.5f, 1.f));
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak|UI_BoxFlag_DrawBorder) ui_labelf("TID: %i", (U32)ctrl_entity->id);
                }
              }
            }
            
            RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
            
            //- rjf: name editor
            if(ctrl_entity->kind == CTRL_EntityKind_Thread) RD_Font(RD_FontSlot_Code) UI_TextPadding(ui_top_font_size()*1.5f)
            {
#if 0 // TODO(rjf): @cfg
              UI_Signal sig = rd_line_editf(RD_LineEditFlag_Border|RD_LineEditFlag_CodeContents, 0, 0, &ws->ctx_menu_input_cursor, &ws->ctx_menu_input_mark, ws->ctx_menu_input_buffer, ws->ctx_menu_input_buffer_size, &ws->ctx_menu_input_string_size, 0, ctrl_entity->string, "Name###ctrl_entity_string_edit_%p", ctrl_entity);
              if(ui_committed(sig))
              {
                rd_cmd(RD_CmdKind_SetEntityName, .ctrl_entity = ctrl_entity->handle, .string = str8(ws->ctx_menu_input_buffer, ws->ctx_menu_input_string_size));
              }
#endif
            }
            
            // rjf: copy full path
            if(ctrl_entity->kind == CTRL_EntityKind_Module) if(ui_clicked(rd_icon_buttonf(RD_IconKind_Clipboard, 0, "Copy Full Path")))
            {
              os_set_clipboard_text(ctrl_entity->string);
              ui_ctx_menu_close();
            }
            
            // rjf: copy ID
            if((ctrl_entity->kind == CTRL_EntityKind_Thread ||
                ctrl_entity->kind == CTRL_EntityKind_Process) &&
               ui_clicked(rd_icon_buttonf(RD_IconKind_Clipboard, 0, "Copy ID")))
            {
              String8 string = str8_from_u64(scratch.arena, ctrl_entity->id, 10, 0, 0);
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            
            // rjf: find
            if(ctrl_entity->kind == CTRL_EntityKind_Thread)
            {
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_FileOutline, 0, "Find")))
              {
                rd_cmd(RD_CmdKind_FindThread, .thread = ctrl_entity->handle);
                ui_ctx_menu_close();
              }
            }
            
            // rjf: selection
            if(ctrl_entity->kind == CTRL_EntityKind_Thread)
            {
              B32 is_selected = ctrl_handle_match(rd_base_regs()->thread, ctrl_entity->handle);
              if(is_selected)
              {
                rd_icon_buttonf(RD_IconKind_Thread, 0, "[Selected]###select_entity");
              }
              else if(ui_clicked(rd_icon_buttonf(RD_IconKind_Thread, 0, "Select###select_entity")))
              {
                rd_cmd(RD_CmdKind_SelectThread, .thread = ctrl_entity->handle);
                ui_ctx_menu_close();
              }
            }
            
            // rjf: freezing
            if(ctrl_entity->kind == CTRL_EntityKind_Thread ||
               ctrl_entity->kind == CTRL_EntityKind_Process ||
               ctrl_entity->kind == CTRL_EntityKind_Machine)
            {
              B32 is_frozen = ctrl_entity_tree_is_frozen(ctrl_entity);
              ui_set_next_palette(rd_palette_from_code(is_frozen ? RD_PaletteCode_NegativePopButton : RD_PaletteCode_PositivePopButton));
              if(is_frozen && ui_clicked(rd_icon_buttonf(RD_IconKind_Locked, 0, "Thaw###freeze_thaw")))
              {
                rd_cmd(RD_CmdKind_ThawThread, .ctrl_entity = ctrl_entity->handle);
              }
              if(!is_frozen && ui_clicked(rd_icon_buttonf(RD_IconKind_Unlocked, 0, "Freeze###freeze_thaw")))
              {
                rd_cmd(RD_CmdKind_FreezeThread, .ctrl_entity = ctrl_entity->handle);
              }
            }
            
            // rjf: callstack
#if 0
            RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
            if(ctrl_entity->kind == CTRL_EntityKind_Thread) UI_TextPadding(ui_top_font_size()*1.5f)
            {
              DI_Scope *di_scope = di_scope_open();
              CTRL_Entity *thread = ctrl_entity;
              CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
              CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(thread);
              CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
              for(U64 idx = 0; idx < rich_unwind.concrete_frame_count; idx += 1)
              {
                CTRL_CallStackFrame *f = &rich_unwind.frames[idx];
                RDI_Parsed *rdi = f->rdi;
                RDI_Procedure *procedure = f->procedure;
                U64 rip_vaddr = regs_rip_from_arch_block(thread->arch, f->regs);
                CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
                String8 module_name = module == &ctrl_entity_nil ? str8_lit("???") : str8_skip_last_slash(module->string);
                
                // rjf: inline frames
                for(CTRL_CallStackInlineFrame *fin = f->last_inline_frame; fin != 0; fin = fin->prev)
                {
                  UI_Box *row = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_ClickToFocus, "###callstack_row_%I64x", idx);
                  UI_Signal sig = ui_signal_from_box(row);
                  ui_push_parent(row);
                  String8 name = {0};
                  name.str = rdi_string_from_idx(rdi, fin->inline_site->name_string_idx, &name.size);
                  UI_TextAlignment(UI_TextAlign_Left) RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(16.f, 1)) ui_labelf("0x%I64x", rip_vaddr);
                  RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_label(str8_lit("[inlined]"));
                  if(name.size != 0)
                  {
                    RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
                    {
                      rd_code_label(1.f, 0, rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol), name);
                    }
                  }
                  else
                  {
                    RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
                  }
                  ui_pop_parent();
                }
                
                // rjf: concrete frame
                {
                  UI_Box *row = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_ClickToFocus, "###callstack_row_%I64x", idx);
                  UI_Signal sig = ui_signal_from_box(row);
                  ui_push_parent(row);
                  String8 name = {0};
                  name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
                  UI_TextAlignment(UI_TextAlign_Left) RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(16.f, 1)) ui_labelf("0x%I64x", rip_vaddr);
                  if(name.size != 0)
                  {
                    RD_Font(RD_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
                    {
                      rd_code_label(1.f, 0, rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol), name);
                    }
                  }
                  else
                  {
                    RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
                  }
                  ui_pop_parent();
                }
              }
              di_scope_close(di_scope);
            }
#endif
            
            // rjf: color editor
#if 0
            RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
            {
              UI_Padding(ui_em(1.5f, 1.f))
              {
                ui_set_next_pref_height(ui_em(9.f, 1.f));
                UI_Row UI_Padding(ui_pct(1, 0))
                {
                  UI_PrefWidth(ui_em(1.5f, 1.f)) UI_PrefHeight(ui_em(9.f, 1.f)) UI_Column UI_PrefHeight(ui_em(1.5f, 0.f))
                  {
                    Vec4F32 presets[] =
                    {
                      v4f32(1.0f, 0.2f, 0.1f, 1.0f),
                      v4f32(1.0f, 0.8f, 0.2f, 1.0f),
                      v4f32(0.3f, 0.8f, 0.2f, 1.0f),
                      v4f32(0.1f, 0.8f, 0.4f, 1.0f),
                      v4f32(0.1f, 0.6f, 0.8f, 1.0f),
                      v4f32(0.5f, 0.3f, 0.8f, 1.0f),
                      v4f32(0.8f, 0.3f, 0.5f, 1.0f),
                    };
                    UI_CornerRadius(ui_em(0.3f, 1.f).value)
                      for(U64 preset_idx = 0; preset_idx < ArrayCount(presets); preset_idx += 1)
                    {
                      ui_set_next_hover_cursor(OS_Cursor_HandPoint);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = presets[preset_idx]));
                      UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|
                                                              UI_BoxFlag_DrawBorder|
                                                              UI_BoxFlag_Clickable|
                                                              UI_BoxFlag_DrawHotEffects|
                                                              UI_BoxFlag_DrawActiveEffects,
                                                              "###color_preset_%i", (int)preset_idx);
                      UI_Signal sig = ui_signal_from_box(box);
                      if(ui_clicked(sig))
                      {
                        Vec3F32 hsv = hsv_from_rgb(v3f32(presets[preset_idx].x, presets[preset_idx].y, presets[preset_idx].z));
                        Vec4F32 hsva = v4f32(hsv.x, hsv.y, hsv.z, 1);
                        entity->color_hsva = hsva;
                      }
                      ui_spacer(ui_em(0.3f, 1.f));
                    }
                  }
                  
                  ui_spacer(ui_em(0.75f, 1.f));
                  
                  UI_PrefWidth(ui_em(9.f, 1.f)) UI_PrefHeight(ui_em(9.f, 1.f))
                  {
                    ui_sat_val_pickerf(entity->color_hsva.x, &entity->color_hsva.y, &entity->color_hsva.z, "###ent_satval_picker");
                  }
                  
                  ui_spacer(ui_em(0.75f, 1.f));
                  
                  UI_PrefWidth(ui_em(1.5f, 1.f)) UI_PrefHeight(ui_em(9.f, 1.f))
                    ui_hue_pickerf(&entity->color_hsva.x, entity->color_hsva.y, entity->color_hsva.z, "###ent_hue_picker");
                }
              }
              
              UI_Row UI_Padding(ui_pct(1, 0)) UI_PrefWidth(ui_em(16.f, 1.f)) UI_CornerRadius(8.f) UI_TextAlignment(UI_TextAlign_Center)
                RD_Palette(RD_PaletteCode_Floating)
              {
                if(ui_clicked(rd_icon_buttonf(RD_IconKind_Trash, 0, "Remove Color###color_toggle")))
                {
                  entity->flags &= ~RD_EntityFlag_HasColor;
                }
              }
              
              ui_spacer(ui_em(1.5f, 1.f));
            }
#endif
          }break;
        }
      }
      
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: drop-completion context menu
    //
    if(ws->drop_completion_paths.node_count != 0)
    {
      RD_Palette(RD_PaletteCode_Floating) UI_CtxMenu(rd_state->drop_completion_key)
        RD_Palette(RD_PaletteCode_ImplicitButton)
        UI_PrefWidth(ui_em(40.f, 1.f))
      {
        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
          for(String8Node *n = ws->drop_completion_paths.first; n != 0; n = n->next)
        {
          UI_Row UI_Padding(ui_em(1.f, 1.f))
          {
            UI_PrefWidth(ui_em(2.f, 1.f)) RD_Font(RD_FontSlot_Icons) ui_label(rd_icon_kind_text_table[RD_IconKind_FileOutline]);
            UI_PrefWidth(ui_text_dim(10, 1)) ui_label(n->string);
          }
        }
        RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
        if(ui_clicked(rd_icon_buttonf(RD_IconKind_Target, 0, "Add File%s As Target%s",
                                      (ws->drop_completion_paths.node_count > 1) ? "s" : "",
                                      (ws->drop_completion_paths.node_count > 1) ? "s" : "")))
        {
          for(String8Node *n = ws->drop_completion_paths.first; n != 0; n = n->next)
          {
            rd_cmd(RD_CmdKind_AddTarget, .file_path = n->string);
          }
          ui_ctx_menu_close();
        }
        if(ws->drop_completion_paths.node_count == 1)
        {
          if(ui_clicked(rd_icon_buttonf(RD_IconKind_Play, 0, "Add File%s As Target%s And Run",
                                        (ws->drop_completion_paths.node_count > 1) ? "s" : "",
                                        (ws->drop_completion_paths.node_count > 1) ? "s" : "")))
          {
            for(String8Node *n = ws->drop_completion_paths.first; n != 0; n = n->next)
            {
              rd_cmd(RD_CmdKind_AddTarget, .file_path = n->string);
            }
            CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
            if(processes.count != 0)
            {
              rd_cmd(RD_CmdKind_KillAll);
            }
            rd_cmd(RD_CmdKind_Run);
            ui_ctx_menu_close();
          }
        }
        if(ws->drop_completion_paths.node_count == 1)
        {
          if(ui_clicked(rd_icon_buttonf(RD_IconKind_StepInto, 0, "Add File%s As Target%s And Step Into",
                                        (ws->drop_completion_paths.node_count > 1) ? "s" : "",
                                        (ws->drop_completion_paths.node_count > 1) ? "s" : "")))
          {
            for(String8Node *n = ws->drop_completion_paths.first; n != 0; n = n->next)
            {
              rd_cmd(RD_CmdKind_AddTarget, .file_path = n->string);
            }
            CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
            if(processes.count != 0)
            {
              rd_cmd(RD_CmdKind_KillAll);
            }
            rd_cmd(RD_CmdKind_StepInto);
            ui_ctx_menu_close();
          }
        }
        if(ui_clicked(rd_icon_buttonf(RD_IconKind_Target, 0, "View File%s",
                                      (ws->drop_completion_paths.node_count > 1) ? "s" : "")))
        {
          for(String8Node *n = ws->drop_completion_paths.first; n != 0; n = n->next)
          {
            rd_cmd(RD_CmdKind_Open, .file_path = n->string);
          }
          ui_ctx_menu_close();
        }
      }
    }
    
    ////////////////////////////
    //- rjf: popup
    //
    {
      if(rd_state->popup_t > 0.005f) UI_TextAlignment(UI_TextAlign_Center) UI_Focus(rd_state->popup_active ? UI_FocusKind_Root : UI_FocusKind_Off)
      {
        Vec2F32 window_dim = dim_2f32(window_rect);
        UI_Box *bg_box = &ui_nil_box;
        UI_Palette *palette = ui_build_palette(rd_palette_from_code(RD_PaletteCode_Floating));
        palette->background.w *= rd_state->popup_t;
        UI_Rect(window_rect)
          UI_ChildLayoutAxis(Axis2_X)
          UI_Focus(UI_FocusKind_On)
          UI_BlurSize(10*rd_state->popup_t)
          UI_Palette(palette)
        {
          bg_box = ui_build_box_from_stringf(UI_BoxFlag_FixedSize|
                                             UI_BoxFlag_Floating|
                                             UI_BoxFlag_Clickable|
                                             UI_BoxFlag_Scroll|
                                             UI_BoxFlag_DefaultFocusNav|
                                             UI_BoxFlag_DisableFocusOverlay|
                                             UI_BoxFlag_DrawBackgroundBlur|
                                             UI_BoxFlag_DrawBackground, "###popup_%p", ws);
        }
        if(rd_state->popup_active) UI_Parent(bg_box) UI_Transparency(1-rd_state->popup_t)
        {
          ui_ctx_menu_close();
          UI_WidthFill UI_PrefHeight(ui_children_sum(1.f)) UI_Column UI_Padding(ui_pct(1, 0))
          {
            UI_TextRasterFlags(rd_raster_flags_from_slot(RD_FontSlot_Main)) UI_FontSize(ui_top_font_size()*2.f) UI_PrefHeight(ui_em(3.f, 1.f)) ui_label(rd_state->popup_title);
            UI_PrefHeight(ui_em(3.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_label(rd_state->popup_desc);
            ui_spacer(ui_em(1.5f, 1.f));
            UI_Row UI_Padding(ui_pct(1.f, 0.f)) UI_PrefWidth(ui_em(16.f, 1.f)) UI_PrefHeight(ui_em(3.5f, 1.f)) UI_CornerRadius(ui_top_font_size()*0.5f)
            {
              RD_Palette(RD_PaletteCode_NeutralPopButton)
                if(ui_clicked(ui_buttonf("OK")) || (ui_key_match(bg_box->default_nav_focus_hot_key, ui_key_zero()) && ui_slot_press(UI_EventActionSlot_Accept)))
              {
                rd_cmd(RD_CmdKind_PopupAccept);
              }
              ui_spacer(ui_em(1.f, 1.f));
              if(ui_clicked(ui_buttonf("Cancel")) || ui_slot_press(UI_EventActionSlot_Cancel))
              {
                rd_cmd(RD_CmdKind_PopupCancel);
              }
            }
            ui_spacer(ui_em(3.f, 1.f));
          }
        }
        ui_signal_from_box(bg_box);
      }
    }
    
    ////////////////////////////
    //- rjf: top bar
    //
    ProfScope("build top bar")
    {
      os_window_clear_custom_border_data(ws->os);
      os_window_push_custom_edges(ws->os, window_edge_px);
      os_window_push_custom_title_bar(ws->os, dim_2f32(top_bar_rect).y);
      ui_set_next_flags(UI_BoxFlag_DefaultFocusNav|UI_BoxFlag_DisableFocusOverlay);
      RD_Palette(RD_PaletteCode_MenuBar)
        UI_Focus((ws->menu_bar_focused && window_is_focused && !ui_any_ctx_menu_is_open() && !ws->hover_eval_focused) ? UI_FocusKind_On : UI_FocusKind_Null)
        UI_Pane(top_bar_rect, str8_lit("###top_bar"))
        UI_WidthFill UI_Row
        UI_Focus(UI_FocusKind_Null)
      {
        UI_Key menu_bar_group_key = ui_key_from_string(ui_key_zero(), str8_lit("###top_bar_group"));
        MemoryZeroArray(ui_top_parent()->parent->corner_radii);
        
        //- rjf: left column
        ui_set_next_flags(UI_BoxFlag_Clip|UI_BoxFlag_ViewScrollX|UI_BoxFlag_ViewClamp);
        UI_WidthFill UI_NamedRow(str8_lit("###menu_bar"))
        {
          //- rjf: icon
          UI_Padding(ui_em(0.5f, 1.f))
          {
            UI_PrefWidth(ui_px(dim_2f32(top_bar_rect).y - ui_top_font_size()*0.8f, 1.f))
              UI_Column
              UI_Padding(ui_em(0.4f, 1.f))
              UI_HeightFill
            {
              R_Handle texture = rd_state->icon_texture;
              Vec2S32 texture_dim = r_size_from_tex2d(texture);
              ui_image(texture, R_Tex2DSampleKind_Linear, r2f32p(0, 0, texture_dim.x, texture_dim.y), v4f32(1, 1, 1, 1), 0, str8_lit(""));
            }
          }
          
          //- rjf: menu items
          ui_set_next_flags(UI_BoxFlag_DrawBackground);
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(20, 1)) UI_GroupKey(menu_bar_group_key)
          {
            // rjf: file menu
            UI_Key file_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_file_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(file_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              String8 cmds[] =
              {
                rd_cmd_kind_info_table[RD_CmdKind_Open].string,
                rd_cmd_kind_info_table[RD_CmdKind_OpenUser].string,
                rd_cmd_kind_info_table[RD_CmdKind_OpenProject].string,
                rd_cmd_kind_info_table[RD_CmdKind_OpenRecentProject].string,
                rd_cmd_kind_info_table[RD_CmdKind_Exit].string,
              };
              U32 codepoints[] =
              {
                'o',
                'u',
                'p',
                'r',
                'x',
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              rd_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: window menu
            UI_Key window_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_window_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(window_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              String8 cmds[] =
              {
                rd_cmd_kind_info_table[RD_CmdKind_OpenWindow].string,
                rd_cmd_kind_info_table[RD_CmdKind_CloseWindow].string,
                rd_cmd_kind_info_table[RD_CmdKind_ToggleFullscreen].string,
              };
              U32 codepoints[] =
              {
                'w',
                'c',
                'f',
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              rd_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: panel menu
            UI_Key panel_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_panel_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(panel_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              String8 cmds[] =
              {
                rd_cmd_kind_info_table[RD_CmdKind_NewPanelUp].string,
                rd_cmd_kind_info_table[RD_CmdKind_NewPanelDown].string,
                rd_cmd_kind_info_table[RD_CmdKind_NewPanelRight].string,
                rd_cmd_kind_info_table[RD_CmdKind_NewPanelLeft].string,
                rd_cmd_kind_info_table[RD_CmdKind_ClosePanel].string,
                rd_cmd_kind_info_table[RD_CmdKind_RotatePanelColumns].string,
                rd_cmd_kind_info_table[RD_CmdKind_NextPanel].string,
                rd_cmd_kind_info_table[RD_CmdKind_PrevPanel].string,
                rd_cmd_kind_info_table[RD_CmdKind_CloseTab].string,
                rd_cmd_kind_info_table[RD_CmdKind_NextTab].string,
                rd_cmd_kind_info_table[RD_CmdKind_PrevTab].string,
                rd_cmd_kind_info_table[RD_CmdKind_TabBarTop].string,
                rd_cmd_kind_info_table[RD_CmdKind_TabBarBottom].string,
                rd_cmd_kind_info_table[RD_CmdKind_ResetToDefaultPanels].string,
                rd_cmd_kind_info_table[RD_CmdKind_ResetToCompactPanels].string,
              };
              U32 codepoints[] =
              {
                'u',
                'd',
                'r',
                'l',
                'x',
                'c',
                'n',
                'p',
                't',
                'b',
                'v',
                0,
                0,
                0,
                0,
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              rd_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: view menu
            UI_Key view_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_view_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(view_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              String8 cmds[] =
              {
                rd_cmd_kind_info_table[RD_CmdKind_Targets].string,
                rd_cmd_kind_info_table[RD_CmdKind_Scheduler].string,
                rd_cmd_kind_info_table[RD_CmdKind_CallStack].string,
                rd_cmd_kind_info_table[RD_CmdKind_Modules].string,
                rd_cmd_kind_info_table[RD_CmdKind_Output].string,
                rd_cmd_kind_info_table[RD_CmdKind_Memory].string,
                rd_cmd_kind_info_table[RD_CmdKind_Disassembly].string,
                rd_cmd_kind_info_table[RD_CmdKind_Watch].string,
                rd_cmd_kind_info_table[RD_CmdKind_Locals].string,
                rd_cmd_kind_info_table[RD_CmdKind_Registers].string,
                rd_cmd_kind_info_table[RD_CmdKind_Globals].string,
                rd_cmd_kind_info_table[RD_CmdKind_ThreadLocals].string,
                rd_cmd_kind_info_table[RD_CmdKind_Types].string,
                rd_cmd_kind_info_table[RD_CmdKind_Procedures].string,
                rd_cmd_kind_info_table[RD_CmdKind_Breakpoints].string,
                rd_cmd_kind_info_table[RD_CmdKind_WatchPins].string,
                rd_cmd_kind_info_table[RD_CmdKind_FilePathMap].string,
                rd_cmd_kind_info_table[RD_CmdKind_AutoViewRules].string,
                rd_cmd_kind_info_table[RD_CmdKind_Settings].string,
                rd_cmd_kind_info_table[RD_CmdKind_GettingStarted].string,
              };
              U32 codepoints[] =
              {
                't',
                's',
                'k',
                'd',
                'o',
                'm',
                'y',
                'w',
                'l',
                'r',
                0,
                0,
                0,
                0,
                'b',
                'h',
                'p',
                'v',
                'g',
                0,
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              rd_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: targets menu
            UI_Key targets_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_targets_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(targets_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              Temp scratch = scratch_begin(0, 0);
              String8 cmds[] =
              {
                rd_cmd_kind_info_table[RD_CmdKind_AddTarget].string,
              };
              U32 codepoints[] =
              {
                'a',
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              rd_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
              scratch_end(scratch);
            }
            
            // rjf: ctrl menu
            UI_Key ctrl_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_ctrl_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(ctrl_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              String8 cmds[] =
              {
                rd_cmd_kind_info_table[D_CmdKind_Run].string,
                rd_cmd_kind_info_table[D_CmdKind_KillAll].string,
                rd_cmd_kind_info_table[D_CmdKind_Restart].string,
                rd_cmd_kind_info_table[D_CmdKind_Halt].string,
                rd_cmd_kind_info_table[D_CmdKind_SoftHaltRefresh].string,
                rd_cmd_kind_info_table[D_CmdKind_StepInto].string,
                rd_cmd_kind_info_table[D_CmdKind_StepOver].string,
                rd_cmd_kind_info_table[D_CmdKind_StepOut].string,
                rd_cmd_kind_info_table[D_CmdKind_Attach].string,
              };
              U32 codepoints[] =
              {
                'r',
                'k',
                's',
                'h',
                'f',
                'i',
                'o',
                't',
                'a',
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              rd_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: help menu
            UI_Key help_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_help_menu_key_"));
            RD_Palette(RD_PaletteCode_Floating)
              UI_CtxMenu(help_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              RD_Palette(RD_PaletteCode_ImplicitButton)
            {
              UI_Row UI_TextAlignment(UI_TextAlign_Center) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                ui_label(str8_lit(BUILD_TITLE_STRING_LITERAL));
              ui_spacer(ui_em(1.f, 1.f));
              UI_PrefHeight(ui_children_sum(1)) UI_Row UI_Padding(ui_pct(1, 0))
              {
                R_Handle texture = rd_state->icon_texture;
                Vec2S32 texture_dim = r_size_from_tex2d(texture);
                UI_PrefWidth(ui_px(ui_top_font_size()*10.f, 1.f))
                  UI_PrefHeight(ui_px(ui_top_font_size()*10.f, 1.f))
                  ui_image(texture, R_Tex2DSampleKind_Linear, r2f32p(0, 0, texture_dim.x, texture_dim.y), v4f32(1, 1, 1, 1), 0, str8_lit(""));
              }
              ui_spacer(ui_em(1.f, 1.f));
              UI_Row
                UI_PrefWidth(ui_text_dim(10, 1))
                UI_TextAlignment(UI_TextAlign_Center)
                UI_Padding(ui_pct(1, 0))
              {
                ui_labelf("Search for commands by pressing ");
                UI_Flags(UI_BoxFlag_DrawBorder)
                  UI_TextAlignment(UI_TextAlign_Center)
                  rd_cmd_binding_buttons(rd_cmd_kind_info_table[RD_CmdKind_RunCommand].string);
              }
              ui_spacer(ui_em(1.f, 1.f));
              RD_Palette(RD_PaletteCode_NeutralPopButton)
                UI_Row UI_Padding(ui_pct(1, 0)) UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_text_dim(10, 1))
                UI_CornerRadius(ui_top_font_size()*0.5f)
              {
                String8 url = str8_lit("https://github.com/EpicGamesExt/raddebugger/issues");
                UI_Signal sig = ui_button(str8_lit("Submit Request, Issue, or Bug Report"));
                if(ui_clicked(sig))
                {
                  os_open_in_browser(url);
                }
              }
              ui_spacer(ui_em(0.5f, 1.f));
            }
            
            // rjf: buttons
            UI_TextAlignment(UI_TextAlign_Center) UI_HeightFill
            {
              // rjf: set up table
              struct
              {
                String8 name;
                U32 codepoint;
                OS_Key key;
                UI_Key menu_key;
              }
              items[] =
              {
                {str8_lit("File"),     'f', OS_Key_F, file_menu_key},
                {str8_lit("Window"),   'w', OS_Key_W, window_menu_key},
                {str8_lit("Panel"),    'p', OS_Key_P, panel_menu_key},
                {str8_lit("View"),     'v', OS_Key_V, view_menu_key},
                {str8_lit("Targets"),  't', OS_Key_T, targets_menu_key},
                {str8_lit("Control"),  'c', OS_Key_C, ctrl_menu_key},
                {str8_lit("Help"),     'h', OS_Key_H, help_menu_key},
              };
              
              // rjf: determine if one of the menus is already open
              B32 menu_open = 0;
              U64 open_menu_idx = 0;
              for(U64 idx = 0; idx < ArrayCount(items); idx += 1)
              {
                if(ui_ctx_menu_is_open(items[idx].menu_key))
                {
                  menu_open = 1;
                  open_menu_idx = idx;
                  break;
                }
              }
              
              // rjf: navigate between menus
              U64 open_menu_idx_prime = open_menu_idx;
              if(menu_open && ws->menu_bar_focused && window_is_focused)
              {
                for(UI_Event *evt = 0; ui_next_event(&evt);)
                {
                  B32 taken = 0;
                  if(evt->delta_2s32.x > 0)
                  {
                    taken = 1;
                    open_menu_idx_prime += 1;
                    open_menu_idx_prime = open_menu_idx_prime%ArrayCount(items);
                  }
                  if(evt->delta_2s32.x < 0)
                  {
                    taken = 1;
                    open_menu_idx_prime = open_menu_idx_prime > 0 ? open_menu_idx_prime-1 : (ArrayCount(items)-1);
                  }
                  if(taken)
                  {
                    ui_eat_event(evt);
                  }
                }
              }
              
              // rjf: make ui
              for(U64 idx = 0; idx < ArrayCount(items); idx += 1)
              {
                ui_set_next_fastpath_codepoint(items[idx].codepoint);
                B32 alt_fastpath_key = 0;
                if(ui_key_press(OS_Modifier_Alt, items[idx].key))
                {
                  alt_fastpath_key = 1;
                }
                if((ws->menu_bar_key_held || ws->menu_bar_focused) && !ui_any_ctx_menu_is_open())
                {
                  ui_set_next_flags(UI_BoxFlag_DrawTextFastpathCodepoint);
                }
                UI_Signal sig = rd_menu_bar_button(items[idx].name);
                os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
                if(menu_open)
                {
                  if((ui_hovering(sig) && !ui_ctx_menu_is_open(items[idx].menu_key)) || (open_menu_idx_prime == idx && open_menu_idx_prime != open_menu_idx))
                  {
                    ui_ctx_menu_open(items[idx].menu_key, sig.box->key, v2f32(0, sig.box->rect.y1-sig.box->rect.y0));
                  }
                }
                else if(ui_pressed(sig) || alt_fastpath_key)
                {
                  if(ui_ctx_menu_is_open(items[idx].menu_key))
                  {
                    ui_ctx_menu_close();
                  }
                  else
                  {
                    ui_ctx_menu_open(items[idx].menu_key, sig.box->key, v2f32(0, sig.box->rect.y1-sig.box->rect.y0));
                  }
                }
              }
            }
          }
          
          ui_spacer(ui_em(0.75f, 1));
          
          // rjf: conversion task visualization
          UI_PrefWidth(ui_text_dim(10, 1)) UI_HeightFill
            RD_Palette(RD_PaletteCode_NeutralPopButton)
          {
            Temp scratch = scratch_begin(0, 0);
            RD_CfgList tasks = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("conversion_task"));
            for(RD_CfgNode *n = tasks.first; n != 0; n = n->next)
            {
              RD_Cfg *task = n->v;
              F32 task_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "task_anim_%I64u", task->id), 1.f);
              if(task_t > 0.5f)
              {
                String8 rdi_path = task->first->string;
                String8 rdi_name = str8_skip_last_slash(rdi_path);
                String8 task_text = push_str8f(scratch.arena, "Creating %S...", rdi_name);
                UI_Key key = ui_key_from_stringf(ui_key_zero(), "task_%p", task);
                UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable, key);
                os_window_push_custom_title_bar_client_area(ws->os, box->rect);
                UI_Signal sig = ui_signal_from_box(box);
                if(ui_hovering(sig)) UI_Tooltip
                {
                  ui_label(rdi_path);
                }
                ui_box_equip_display_string(box, task_text);
              }
            }
            scratch_end(scratch);
          }
        }
        
        //- rjf: center column
        UI_PrefWidth(ui_children_sum(1.f)) UI_Row
          UI_PrefWidth(ui_em(2.25f, 1))
          RD_Font(RD_FontSlot_Icons)
          UI_FontSize(ui_top_font_size()*0.85f)
        {
          Temp scratch = scratch_begin(0, 0);
          RD_CfgList targets = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("target"));
          CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
          B32 have_targets = (targets.count != 0);
          B32 can_send_signal = !d_ctrl_targets_running();
          B32 can_play  = (have_targets && (can_send_signal || d_ctrl_last_run_frame_idx()+4 > d_frame_index()));
          B32 can_pause = (!can_send_signal);
          B32 can_stop  = (processes.count != 0);
          B32 can_step =  (processes.count != 0 && can_send_signal);
          
          //- rjf: play button
          if(can_play || !have_targets || processes.count == 0)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_Flags((can_play ? 0 : UI_BoxFlag_Disabled))
            UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextPositive)))
          {
            UI_Signal sig = ui_button(rd_icon_kind_text_table[RD_IconKind_Play]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_play)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Disabled: %s", have_targets ? "Targets are currently running" : "No active targets exist");
            }
            if(ui_hovering(sig) && can_play)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
              {
                if(can_stop)
                {
                  ui_labelf("Resume all processes");
                }
                else
                {
                  ui_labelf("Launch all active targets:");
                  for(RD_CfgNode *n = targets.first; n != 0; n = n->next)
                  {
                    RD_Cfg *target = n->v;
                    DR_FStrList title_fstrs = rd_title_fstrs_from_cfg(ui_build_arena(), target, ui_top_palette()->text_weak, ui_top_font_size());
                    UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                    ui_box_equip_display_fstrs(box, &title_fstrs);
                  }
                }
              }
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_Run);
            }
          }
          
          //- rjf: restart button
          else UI_TextAlignment(UI_TextAlign_Center)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextPositive)))
          {
            UI_Signal sig = ui_button(rd_icon_kind_text_table[RD_IconKind_Redo]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig))
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
              {
                ui_labelf("Restart");
              }
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_Restart);
            }
          }
          
          //- rjf: pause button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_pause ? 0 : UI_BoxFlag_Disabled)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextNeutral)))
          {
            UI_Signal sig = ui_button(rd_icon_kind_text_table[RD_IconKind_Pause]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_pause)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Disabled: Already halted");
            }
            if(ui_hovering(sig) && can_pause)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Halt all attached processes");
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_Halt);
            }
          }
          
          //- rjf: stop button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_stop ? 0 : UI_BoxFlag_Disabled)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = rd_rgba_from_theme_color(RD_ThemeColor_TextNegative)))
          {
            UI_Signal sig = {0};
            {
              sig = ui_button(rd_icon_kind_text_table[RD_IconKind_Stop]);
              os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            }
            if(ui_hovering(sig) && !can_stop)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_stop)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Kill all attached processes");
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_KillAll);
            }
          }
          
          //- rjf: step over button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags((can_play ? 0 : UI_BoxFlag_Disabled))
          {
            UI_Signal sig = ui_button(rd_icon_kind_text_table[RD_IconKind_StepOver]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig))
            {
              if(can_play)
              {
                UI_Tooltip
                  RD_Font(RD_FontSlot_Main)
                  UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                  ui_labelf("Step Over");
              }
              else
              {
                UI_Tooltip
                  RD_Font(RD_FontSlot_Main)
                  UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                  ui_labelf("Disabled: %s", have_targets ? "Targets are currently running" : "No active targets exist");
              }
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_StepOver);
            }
          }
          
          //- rjf: step into button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags((can_play ? 0 : UI_BoxFlag_Disabled))
          {
            UI_Signal sig = ui_button(rd_icon_kind_text_table[RD_IconKind_StepInto]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig))
            {
              if(can_play)
              {
                UI_Tooltip
                  RD_Font(RD_FontSlot_Main)
                  UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                  ui_labelf("Step Into");
              }
              else
              {
                UI_Tooltip
                  RD_Font(RD_FontSlot_Main)
                  UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                  ui_labelf("Disabled: %s", have_targets ? "Targets are currently running" : "No active targets exist");
              }
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_StepInto);
            }
          }
          
          //- rjf: step out button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_step ? 0 : UI_BoxFlag_Disabled)
          {
            UI_Signal sig = ui_button(rd_icon_kind_text_table[RD_IconKind_StepOut]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_step && can_pause)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                RD_Font(RD_FontSlot_Main)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                ui_labelf("Step Out");
            }
            if(ui_clicked(sig))
            {
              rd_cmd(RD_CmdKind_StepOut);
            }
          }
          
          scratch_end(scratch);
        }
        
        //- rjf: right column
        UI_WidthFill UI_Row
        {
          B32 do_user_prof = (dim_2f32(top_bar_rect).x > ui_top_font_size()*80);
          
          ui_spacer(ui_pct(1, 0));
          
          // rjf: loaded user viz
          if(do_user_prof) RD_Palette(RD_PaletteCode_NeutralPopButton)
          {
            ui_set_next_pref_width(ui_children_sum(1));
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            UI_Box *user_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                                         UI_BoxFlag_DrawBorder|
                                                         UI_BoxFlag_DrawBackground|
                                                         UI_BoxFlag_DrawHotEffects|
                                                         UI_BoxFlag_DrawActiveEffects,
                                                         "###loaded_user_button");
            os_window_push_custom_title_bar_client_area(ws->os, user_box->rect);
            UI_Parent(user_box) UI_PrefWidth(ui_text_dim(10, 0)) UI_TextAlignment(UI_TextAlign_Center)
            {
              String8 user_path = rd_state->user_path;
              user_path = str8_chop_last_dot(user_path);
              RD_Font(RD_FontSlot_Icons)
                UI_TextRasterFlags(rd_raster_flags_from_slot(RD_FontSlot_Icons))
                ui_label(rd_icon_kind_text_table[RD_IconKind_Person]);
              ui_label(str8_skip_last_slash(user_path));
            }
            UI_Signal user_sig = ui_signal_from_box(user_box);
            if(ui_clicked(user_sig))
            {
              rd_cmd(RD_CmdKind_RunCommand, .cmd_name = rd_cmd_kind_info_table[RD_CmdKind_OpenUser].string);
            }
          }
          
          if(do_user_prof)
          {
            ui_spacer(ui_em(0.75f, 0));
          }
          
          // rjf: loaded project viz
          if(do_user_prof) RD_Palette(RD_PaletteCode_NeutralPopButton)
          {
            ui_set_next_pref_width(ui_children_sum(1));
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            UI_Box *prof_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                                         UI_BoxFlag_DrawBorder|
                                                         UI_BoxFlag_DrawBackground|
                                                         UI_BoxFlag_DrawHotEffects|
                                                         UI_BoxFlag_DrawActiveEffects,
                                                         "###loaded_project_button");
            os_window_push_custom_title_bar_client_area(ws->os, prof_box->rect);
            UI_Parent(prof_box) UI_PrefWidth(ui_text_dim(10, 0)) UI_TextAlignment(UI_TextAlign_Center)
            {
              String8 prof_path = rd_state->project_path;
              prof_path = str8_chop_last_dot(prof_path);
              RD_Font(RD_FontSlot_Icons)
                ui_label(rd_icon_kind_text_table[RD_IconKind_Briefcase]);
              ui_label(str8_skip_last_slash(prof_path));
            }
            UI_Signal prof_sig = ui_signal_from_box(prof_box);
            if(ui_clicked(prof_sig))
            {
              rd_cmd(RD_CmdKind_RunCommand, .cmd_name = rd_cmd_kind_info_table[RD_CmdKind_OpenProject].string);
            }
          }
          
          if(do_user_prof)
          {
            ui_spacer(ui_em(0.75f, 0));
          }
          
          // rjf: close dropdown
          UI_Key close_ctx_menu_key = ui_key_from_stringf(ui_key_zero(), "###close_ctx_menu");
          UI_CtxMenu(close_ctx_menu_key) RD_Palette(RD_PaletteCode_ImplicitButton)
          {
            if(ui_clicked(rd_icon_buttonf(RD_IconKind_Window, 0, "Close Window")))
            {
              rd_cmd(RD_CmdKind_CloseWindow);
            }
            if(ui_clicked(rd_icon_buttonf(RD_IconKind_X, 0, "Exit")))
            {
              rd_cmd(RD_CmdKind_Exit);
            }
          }
          
          // rjf: min/max/close buttons
          {
            UI_Signal min_sig = {0};
            UI_Signal max_sig = {0};
            UI_Signal cls_sig = {0};
            Vec2F32 bar_dim = dim_2f32(top_bar_rect);
            F32 button_dim = floor_f32(bar_dim.y);
            UI_PrefWidth(ui_px(button_dim, 1.f))
            {
              min_sig = rd_icon_buttonf(RD_IconKind_Minus,  0, "##minimize");
              max_sig = rd_icon_buttonf(RD_IconKind_Window, 0, "##maximize");
            }
            UI_PrefWidth(ui_px(button_dim, 1.f))
              RD_Palette(RD_PaletteCode_NegativePopButton)
            {
              cls_sig = rd_icon_buttonf(RD_IconKind_X,      0, "##close");
            }
            if(ui_clicked(min_sig))
            {
              os_window_set_minimized(ws->os, 1);
            }
            if(ui_clicked(max_sig))
            {
              os_window_set_maximized(ws->os, !os_window_is_maximized(ws->os));
            }
            if(ui_clicked(cls_sig))
            {
              if(ws->order_next != &rd_nil_window_state ||
                 ws->order_prev != &rd_nil_window_state)
              {
                ui_ctx_menu_open(close_ctx_menu_key, cls_sig.box->key, v2f32(0, dim_2f32(cls_sig.box->rect).y));
              }
              else
              {
                rd_cmd(RD_CmdKind_Exit);
              }
            }
            os_window_push_custom_title_bar_client_area(ws->os, min_sig.box->rect);
            os_window_push_custom_title_bar_client_area(ws->os, max_sig.box->rect);
            os_window_push_custom_title_bar_client_area(ws->os, pad_2f32(cls_sig.box->rect, 2.f));
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build hover eval
    //
    ProfScope("build hover eval")
    {
      B32 build_hover_eval = hover_eval_is_open;
      
      // rjf: disable hover eval if hovered view is actively scrolling
      if(hover_eval_is_open)
      {
#if 0 // TODO(rjf): @cfg_panels
        for(RD_Panel *panel = ws->root_panel;
            !rd_panel_is_nil(panel);
            panel = rd_panel_rec_depth_first_pre(panel).next)
        {
          if(!rd_panel_is_nil(panel->first)) { continue; }
          Rng2F32 panel_rect = rd_target_rect_from_panel(content_rect, ws->root_panel, panel);
          RD_View *view = rd_selected_tab_from_panel(panel);
          if(!rd_view_is_nil(view) &&
             contains_2f32(panel_rect, ui_mouse()) &&
             (abs_f32(view->scroll_pos.x.off) > 0.01f ||
              abs_f32(view->scroll_pos.y.off) > 0.01f))
          {
            build_hover_eval = 0;
            ws->hover_eval_first_frame_idx = rd_state->frame_index;
          }
        }
#endif
      }
      
      // rjf: evaluate hover-evaluation expression - if it doesn't evaluate, then don't build anything
      E_Eval hover_eval = e_eval_from_string(scratch.arena, ws->hover_eval_string);
      if(hover_eval.msgs.max_kind > E_MsgKind_Null)
      {
        build_hover_eval = 0;
      }
      
      // rjf: reset open animation
      if(ws->hover_eval_string.size == 0)
      {
        ws->hover_eval_open_t = 0;
        ws->hover_eval_num_visible_rows_t = 0;
      }
      
      // rjf: reset animation, but request frames if we're waiting to open
      if(ws->hover_eval_string.size != 0 && !hover_eval_is_open && ws->hover_eval_last_frame_idx < ws->hover_eval_first_frame_idx+20 && rd_state->frame_index-ws->hover_eval_last_frame_idx < 50)
      {
        rd_request_frame();
        ws->hover_eval_num_visible_rows_t = 0;
        ws->hover_eval_open_t = 0;
      }
      
      // rjf: reset focus state if hover eval is not being built
      if(!build_hover_eval || ws->hover_eval_string.size == 0 || !hover_eval_is_open)
      {
        ws->hover_eval_focused = 0;
      }
      
      // rjf: build hover eval
      if(build_hover_eval && ws->hover_eval_string.size != 0 && hover_eval_is_open)
        RD_Font(RD_FontSlot_Code)
        UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
        RD_Palette(RD_PaletteCode_Floating)
      {
        F32 hover_eval_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "hover_eval_open_t"), 1.f);
        RD_Cfg *root = rd_immediate_cfg_from_keyf("hover_eval_%p", ws);
        RD_Cfg *view = rd_cfg_child_from_string_or_alloc(root, str8_lit("watch"));
        RD_Cfg *expr = rd_cfg_child_from_string_or_alloc(view, str8_lit("expression"));
        RD_Cfg *explicit_root = rd_cfg_child_from_string_or_alloc(view, str8_lit("explicit_root"));
        rd_cfg_new(explicit_root, str8_lit("1"));
        RD_RegsScope(.view = view->id)
        {
          rd_cfg_new_replace(expr, ws->hover_eval_string);
          EV_BlockTree predicted_block_tree = ev_block_tree_from_eval(scratch.arena, rd_view_eval_view(), str8_zero(), hover_eval);
          F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
          U64 max_row_count = (U64)floor_f32(ui_top_font_size()*10.f / row_height_px);
          if(ws->hover_eval_focused)
          {
            max_row_count *= 3;
          }
          U64 needed_row_count = Min(max_row_count, predicted_block_tree.total_row_count);
          F32 num_rows_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "hover_eval_num_rows_t"), (F32)needed_row_count);
          UI_Focus(ws->hover_eval_focused ? UI_FocusKind_On : UI_FocusKind_Off)
            UI_PrefHeight(ui_px(row_height_px, 1.f))
          {
            ui_set_next_fixed_x(ws->hover_eval_spawn_pos.x);
            ui_set_next_fixed_y(ws->hover_eval_spawn_pos.y);
            ui_set_next_pref_width(ui_em(60.f, 1.f));
            ui_set_next_pref_height(ui_px(num_rows_t*row_height_px, 1.f));
            ui_set_next_child_layout_axis(Axis2_Y);
            ui_set_next_squish(0.25f-0.25f*hover_eval_t);
            ui_set_next_transparency(1.f-hover_eval_t);
            UI_Box *container = ui_build_box_from_string(UI_BoxFlag_Clickable|
                                                         UI_BoxFlag_DrawBorder|
                                                         UI_BoxFlag_DrawBackground|
                                                         UI_BoxFlag_DrawBackgroundBlur|
                                                         UI_BoxFlag_DisableFocusOverlay|
                                                         UI_BoxFlag_DrawDropShadow,
                                                         str8_lit("hover_eval_container"));
            if(!ws->hover_eval_focused)
            {
              for(UI_Event *evt = 0; ui_next_event(&evt);)
              {
                if(evt->kind == UI_EventKind_Press &&
                   evt->key == OS_Key_LeftMouseButton &&
                   contains_2f32(container->rect, evt->pos))
                {
                  ws->hover_eval_focused = 1;
                  break;
                }
              }
            }
            UI_Parent(container) UI_Focus(ws->hover_eval_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
            {
              ui_set_next_pref_width(ui_pct(1, 0));
              ui_set_next_pref_height(ui_pct(1, 0));
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *view_contents_container = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip, "###view_contents_container");
              UI_Parent(view_contents_container) UI_WidthFill
              {
                rd_view_ui(view_contents_container->rect);
              }
            }
            UI_Signal sig = ui_signal_from_box(container);
            if(ui_pressed(sig))
            {
              ws->hover_eval_focused = 1;
            }
            if(ui_mouse_over(sig) || ws->hover_eval_focused)
            {
              ws->hover_eval_last_frame_idx = rd_state->frame_index;
            }
            else if(ws->hover_eval_last_frame_idx+2 < rd_state->frame_index)
            {
              rd_request_frame();
            }
            if(ws->hover_eval_focused)
            {
              for(UI_Event *evt = 0; ui_next_event(&evt);)
              {
                if(evt->kind == UI_EventKind_Press &&
                   evt->key == OS_Key_LeftMouseButton &&
                   !contains_2f32(container->rect, evt->pos))
                {
                  ws->hover_eval_focused = 0;
                  MemoryZeroStruct(&ws->hover_eval_string);
                  arena_clear(ws->hover_eval_arena);
                  rd_request_frame();
                  break;
                }
              }
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: bottom bar
    //
    ProfScope("build bottom bar")
    {
      B32 is_running = d_ctrl_targets_running() && d_ctrl_last_run_frame_idx() < d_frame_index();
      CTRL_Event stop_event = d_ctrl_last_stop_event();
      UI_Palette *positive_scheme = rd_palette_from_code(RD_PaletteCode_PositivePopButton);
      UI_Palette *running_scheme  = rd_palette_from_code(RD_PaletteCode_NeutralPopButton);
      UI_Palette *negative_scheme = rd_palette_from_code(RD_PaletteCode_NegativePopButton);
      UI_Palette *palette = running_scheme;
      if(!is_running)
      {
        switch(stop_event.cause)
        {
          default:
          case CTRL_EventCause_Finished:
          {
            palette = positive_scheme;
          }break;
          case CTRL_EventCause_UserBreakpoint:
          case CTRL_EventCause_InterruptedByException:
          case CTRL_EventCause_InterruptedByTrap:
          case CTRL_EventCause_InterruptedByHalt:
          {
            palette = negative_scheme;
          }break;
        }
      }
      if(ws->error_t > 0.01f)
      {
        UI_Palette *blended_scheme = push_array(ui_build_arena(), UI_Palette, 1);
        MemoryCopyStruct(blended_scheme, palette);
        for EachEnumVal(UI_ColorCode, code)
        {
          for(U64 idx = 0; idx < 4; idx += 1)
          {
            blended_scheme->colors[code].v[idx] += (negative_scheme->colors[code].v[idx] - blended_scheme->colors[code].v[idx]) * ws->error_t;
          }
        }
        palette = blended_scheme;
      }
      UI_Flags(UI_BoxFlag_DrawBackground) UI_CornerRadius(0)
        UI_Palette(palette)
        UI_Pane(bottom_bar_rect, str8_lit("###bottom_bar")) UI_WidthFill UI_Row
        UI_Flags(0)
      {
        // rjf: developer frame-time indicator
        if(DEV_updating_indicator)
        {
          F32 animation_t = pow_f32(sin_f32(rd_state->time_in_seconds/2.f), 2.f);
          ui_spacer(ui_em(0.3f, 1.f));
          ui_spacer(ui_em(1.5f*animation_t, 1.f));
          UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("*");
          ui_spacer(ui_em(1.5f*(1-animation_t), 1.f));
        }
        
        // rjf: status
        {
          ui_spacer(ui_em(1.f, 1.f));
          if(is_running)
          {
            ui_label(str8_lit("Running"));
          }
          else
          {
            Temp scratch = scratch_begin(0, 0);
            DR_FStrList explanation_fstrs = rd_stop_explanation_fstrs_from_ctrl_event(scratch.arena, &stop_event);
            UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
            ui_box_equip_display_fstrs(box, &explanation_fstrs);
            scratch_end(scratch);
          }
        }
        
        ui_spacer(ui_pct(1, 0));
        
        // rjf: bind change visualization
        if(rd_state->bind_change_active)
        {
          RD_CmdKindInfo *info = rd_cmd_kind_info_from_string(rd_state->bind_change_cmd_name);
          String8 display_name = rd_display_from_code_name(info->string);
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_Flags(UI_BoxFlag_DrawBackground)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_CornerRadius(4)
            RD_Palette(RD_PaletteCode_NeutralPopButton)
            ui_labelf("Currently rebinding \"%S\" hotkey", display_name);
        }
        
        // rjf: error visualization
        else if(ws->error_t >= 0.01f)
        {
          ws->error_t -= rd_state->frame_dt/8.f;
          rd_request_frame();
          String8 error_string = str8(ws->error_buffer, ws->error_string_size);
          if(error_string.size != 0)
          {
            ui_set_next_pref_width(ui_children_sum(1));
            UI_CornerRadius(4)
              UI_Row
              UI_PrefWidth(ui_text_dim(10, 1))
              UI_TextAlignment(UI_TextAlign_Center)
            {
              RD_Font(RD_FontSlot_Icons)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
                ui_label(rd_icon_kind_text_table[RD_IconKind_WarningBig]);
              rd_label(error_string);
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: panel non-leaf UI (drag boundaries, drag/drop sites)
    //
    B32 is_changing_panel_boundaries = 0;
    ProfScope("non-leaf panel UI")
      for(RD_PanelNode *panel = panel_tree.root;
          panel != &rd_nil_panel_node;
          panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
    {
      //////////////////////////
      //- rjf: continue on leaf panels
      //
      if(panel->first == &rd_nil_panel_node)
      {
        continue;
      }
      
      //////////////////////////
      //- rjf: grab info
      //
      Axis2 split_axis = panel->split_axis;
      Rng2F32 panel_rect = rd_target_rect_from_panel_node(content_rect, panel_tree.root, panel);
      
      //////////////////////////
      //- rjf: boundary tab-drag/drop sites
      //
      {
        RD_Cfg *drag_view = rd_cfg_from_id(rd_state->drag_drop_regs->view);
        if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && drag_view != &rd_nil_cfg)
        {
          //- rjf: params
          F32 drop_site_major_dim_px = ceil_f32(ui_top_font_size()*7.f);
          F32 drop_site_minor_dim_px = ceil_f32(ui_top_font_size()*5.f);
          F32 corner_radius = ui_top_font_size()*0.5f;
          F32 padding = ceil_f32(ui_top_font_size()*0.5f);
          
          //- rjf: special case - build Y boundary drop sites on root panel
          //
          // (this does not naturally follow from the below algorithm, since the
          // root level panel only splits on X)
          if(panel == panel_tree.root) UI_CornerRadius(corner_radius)
          {
            Vec2F32 panel_rect_center = center_2f32(panel_rect);
            Axis2 axis = axis2_flip(panel_tree.root->split_axis);
            for EachEnumVal(Side, side)
            {
              UI_Key key = ui_key_from_stringf(ui_key_zero(), "root_extra_split_%i", side);
              Rng2F32 site_rect = panel_rect;
              site_rect.p0.v[axis2_flip(axis)] = panel_rect_center.v[axis2_flip(axis)] - drop_site_major_dim_px/2;
              site_rect.p1.v[axis2_flip(axis)] = panel_rect_center.v[axis2_flip(axis)] + drop_site_major_dim_px/2;
              site_rect.p0.v[axis] = panel_rect.v[side].v[axis] - drop_site_minor_dim_px/2;
              site_rect.p1.v[axis] = panel_rect.v[side].v[axis] + drop_site_minor_dim_px/2;
              
              // rjf: build
              UI_Box *site_box = &ui_nil_box;
              {
                F32 site_open_t = ui_anim(ui_key_from_stringf(key, "open_t"), 1.f);
                UI_Rect(site_rect) UI_Squish(0.25f-0.25f*site_open_t) UI_Transparency(1-site_open_t)
                {
                  site_box = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
                  ui_signal_from_box(site_box);
                }
                UI_Box *site_box_viz = &ui_nil_box;
                UI_Parent(site_box) UI_WidthFill UI_HeightFill
                  UI_Padding(ui_px(padding, 1.f))
                  UI_Column
                  UI_Padding(ui_px(padding, 1.f))
                {
                  ui_set_next_child_layout_axis(axis2_flip(axis));
                  if(ui_key_match(key, ui_drop_hot_key()))
                  {
                    ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = rd_rgba_from_theme_color(RD_ThemeColor_Hover)));
                  }
                  site_box_viz = ui_build_box_from_key(UI_BoxFlag_DrawBackground|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_DrawDropShadow|
                                                       UI_BoxFlag_DrawBackgroundBlur, ui_key_zero());
                }
                UI_Parent(site_box_viz) UI_WidthFill UI_HeightFill UI_Padding(ui_px(padding, 1.f))
                {
                  ui_set_next_child_layout_axis(axis);
                  UI_Box *row_or_column = ui_build_box_from_key(0, ui_key_zero()); UI_Parent(row_or_column) UI_Padding(ui_px(padding, 1.f))
                  {
                    ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                    ui_spacer(ui_px(padding, 1.f));
                    ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                  }
                }
              }
              
              // rjf: viz
              if(ui_key_match(site_box->key, ui_drop_hot_key()))
              {
                Rng2F32 future_split_rect_target = site_rect;
                future_split_rect_target.p0.v[axis] -= drop_site_major_dim_px;
                future_split_rect_target.p1.v[axis] += drop_site_major_dim_px;
                future_split_rect_target.p0.v[axis2_flip(axis)] = panel_rect.p0.v[axis2_flip(axis)];
                future_split_rect_target.p1.v[axis2_flip(axis)] = panel_rect.p1.v[axis2_flip(axis)];
                future_split_rect_target = pad_2f32(future_split_rect_target, -ui_top_font_size()*2.f);
                Vec2F32 future_split_rect_target_center = center_2f32(future_split_rect_target);
                Rng2F32 future_split_rect =
                {
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v0"), future_split_rect_target.x0, .initial = future_split_rect_target_center.x),
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v1"), future_split_rect_target.y0, .initial = future_split_rect_target_center.y),
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v2"), future_split_rect_target.x1, .initial = future_split_rect_target_center.x),
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v3"), future_split_rect_target.y1, .initial = future_split_rect_target_center.y),
                };
                UI_Rect(future_split_rect) RD_Palette(RD_PaletteCode_DropSiteOverlay) UI_CornerRadius(ui_top_font_size()*2.f)
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                }
              }
              
              // rjf: drop
              if(ui_key_match(site_box->key, ui_drop_hot_key()) && rd_drag_drop())
              {
                Dir2 dir = (axis == Axis2_Y ? (side == Side_Min ? Dir2_Up : Dir2_Down) :
                            axis == Axis2_X ? (side == Side_Min ? Dir2_Left : Dir2_Right) :
                            Dir2_Invalid);
                if(dir != Dir2_Invalid)
                {
                  RD_PanelNode *split_panel = panel;
                  rd_cmd(RD_CmdKind_SplitPanel,
                         .dst_panel  = split_panel->cfg->id,
                         .panel      = rd_state->drag_drop_regs->panel,
                         .view       = rd_state->drag_drop_regs->view,
                         .dir2       = dir);
                }
              }
            }
          }
          
          //- rjf: iterate all children, build boundary drop sites
          Axis2 split_axis = panel->split_axis;
          UI_CornerRadius(corner_radius) for(RD_PanelNode *child = panel->first;; child = child->next)
          {
            // rjf: form rect
            Rng2F32 child_rect = rd_target_rect_from_panel_node_child(panel_rect, panel, child);
            Vec2F32 child_rect_center = center_2f32(child_rect);
            UI_Key key = ui_key_from_stringf(ui_key_zero(), "drop_boundary_%p_%p", panel->cfg, child->cfg);
            Rng2F32 site_rect = r2f32(child_rect_center, child_rect_center);
            site_rect.p0.v[split_axis] = child_rect.p0.v[split_axis] - drop_site_minor_dim_px/2;
            site_rect.p1.v[split_axis] = child_rect.p0.v[split_axis] + drop_site_minor_dim_px/2;
            site_rect.p0.v[axis2_flip(split_axis)] -= drop_site_major_dim_px/2;
            site_rect.p1.v[axis2_flip(split_axis)] += drop_site_major_dim_px/2;
            
            // rjf: build
            UI_Box *site_box = &ui_nil_box;
            {
              F32 site_open_t = ui_anim(ui_key_from_stringf(key, "open_t"), 1.f);
              UI_Rect(site_rect) UI_Squish(0.25f-0.25f*site_open_t) UI_Transparency(1-site_open_t)
              {
                site_box = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
                ui_signal_from_box(site_box);
              }
              UI_Box *site_box_viz = &ui_nil_box;
              UI_Parent(site_box) UI_WidthFill UI_HeightFill
                UI_Padding(ui_px(padding, 1.f))
                UI_Column
                UI_Padding(ui_px(padding, 1.f))
              {
                ui_set_next_child_layout_axis(axis2_flip(split_axis));
                if(ui_key_match(key, ui_drop_hot_key()))
                {
                  ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = rd_rgba_from_theme_color(RD_ThemeColor_Hover)));
                }
                site_box_viz = ui_build_box_from_key(UI_BoxFlag_DrawBackground|
                                                     UI_BoxFlag_DrawBorder|
                                                     UI_BoxFlag_DrawDropShadow|
                                                     UI_BoxFlag_DrawBackgroundBlur, ui_key_zero());
              }
              UI_Parent(site_box_viz) UI_WidthFill UI_HeightFill UI_Padding(ui_px(padding, 1.f))
              {
                ui_set_next_child_layout_axis(split_axis);
                UI_Box *row_or_column = ui_build_box_from_key(0, ui_key_zero()); UI_Parent(row_or_column) UI_Padding(ui_px(padding, 1.f))
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                  ui_spacer(ui_px(padding, 1.f));
                  ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                }
              }
            }
            
            // rjf: viz
            if(ui_key_match(site_box->key, ui_drop_hot_key()))
            {
              Rng2F32 future_split_rect_target = site_rect;
              future_split_rect_target.p0.v[split_axis] -= drop_site_major_dim_px;
              future_split_rect_target.p1.v[split_axis] += drop_site_major_dim_px;
              future_split_rect_target.p0.v[axis2_flip(split_axis)] = child_rect.p0.v[axis2_flip(split_axis)];
              future_split_rect_target.p1.v[axis2_flip(split_axis)] = child_rect.p1.v[axis2_flip(split_axis)];
              future_split_rect_target = pad_2f32(future_split_rect_target, -ui_top_font_size()*2.f);
              Vec2F32 future_split_rect_target_center = center_2f32(future_split_rect_target);
              Rng2F32 future_split_rect =
              {
                ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v0"), future_split_rect_target.x0, .initial = future_split_rect_target_center.x),
                ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v1"), future_split_rect_target.y0, .initial = future_split_rect_target_center.y),
                ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v2"), future_split_rect_target.x1, .initial = future_split_rect_target_center.x),
                ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v3"), future_split_rect_target.y1, .initial = future_split_rect_target_center.y),
              };
              UI_Rect(future_split_rect) RD_Palette(RD_PaletteCode_DropSiteOverlay) UI_CornerRadius(ui_top_font_size()*2.f)
              {
                ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
              }
            }
            
            // rjf: drop
            if(ui_key_match(site_box->key, ui_drop_hot_key()) && rd_drag_drop())
            {
              Dir2 dir = (panel->split_axis == Axis2_X ? Dir2_Left : Dir2_Up);
              RD_PanelNode *split_panel = child;
              if(split_panel == &rd_nil_panel_node)
              {
                split_panel = panel->last;
                dir = (panel->split_axis == Axis2_X ? Dir2_Right : Dir2_Down);
              }
              rd_cmd(RD_CmdKind_SplitPanel,
                     .dst_panel  = split_panel->cfg->id,
                     .panel      = rd_state->drag_drop_regs->panel,
                     .view       = rd_state->drag_drop_regs->view,
                     .dir2       = dir);
            }
            
            // rjf: exit on opl child
            if(child == &rd_nil_panel_node)
            {
              break;
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: do UI for drag boundaries between all children
      //
      for(RD_PanelNode *child = panel->first;
          child != &rd_nil_panel_node && child->next != &rd_nil_panel_node;
          child = child->next)
      {
        RD_PanelNode *min_child = child;
        RD_PanelNode *max_child = min_child->next;
        Rng2F32 min_child_rect = rd_target_rect_from_panel_node_child(panel_rect, panel, min_child);
        Rng2F32 max_child_rect = rd_target_rect_from_panel_node_child(panel_rect, panel, max_child);
        Rng2F32 boundary_rect = {0};
        {
          boundary_rect.p0.v[split_axis] = min_child_rect.p1.v[split_axis] - ui_top_font_size()/3;
          boundary_rect.p1.v[split_axis] = max_child_rect.p0.v[split_axis] + ui_top_font_size()/3;
          boundary_rect.p0.v[axis2_flip(split_axis)] = panel_rect.p0.v[axis2_flip(split_axis)];
          boundary_rect.p1.v[axis2_flip(split_axis)] = panel_rect.p1.v[axis2_flip(split_axis)];
        }
        
        UI_Rect(boundary_rect)
        {
          ui_set_next_hover_cursor(split_axis == Axis2_X ? OS_Cursor_LeftRight : OS_Cursor_UpDown);
          UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_%p", min_child->cfg, max_child->cfg);
          UI_Signal sig = ui_signal_from_box(box);
          if(ui_double_clicked(sig))
          {
            ui_kill_action();
            F32 sum_pct = min_child->pct_of_parent + max_child->pct_of_parent;
            min_child->pct_of_parent = 0.5f * sum_pct;
            max_child->pct_of_parent = 0.5f * sum_pct;
            rd_cfg_equip_stringf(min_child->cfg, "%f", min_child->pct_of_parent);
            rd_cfg_equip_stringf(max_child->cfg, "%f", max_child->pct_of_parent);
          }
          else if(ui_pressed(sig))
          {
            Vec2F32 v = {min_child->pct_of_parent, max_child->pct_of_parent};
            ui_store_drag_struct(&v);
          }
          else if(ui_dragging(sig))
          {
            Vec2F32 v = *ui_get_drag_struct(Vec2F32);
            Vec2F32 mouse_delta      = ui_drag_delta();
            F32 total_size           = dim_2f32(panel_rect).v[split_axis];
            F32 min_pct__before      = v.v[0];
            F32 min_pixels__before   = min_pct__before * total_size;
            F32 min_pixels__after    = min_pixels__before + mouse_delta.v[split_axis];
            if(min_pixels__after < 50.f)
            {
              min_pixels__after = 50.f;
            }
            F32 min_pct__after       = min_pixels__after / total_size;
            F32 pct_delta            = min_pct__after - min_pct__before;
            F32 max_pct__before      = v.v[1];
            F32 max_pct__after       = max_pct__before - pct_delta;
            F32 max_pixels__after    = max_pct__after * total_size;
            if(max_pixels__after < 50.f)
            {
              max_pixels__after = 50.f;
              max_pct__after = max_pixels__after / total_size;
              pct_delta = -(max_pct__after - max_pct__before);
              min_pct__after = min_pct__before + pct_delta;
            }
            min_child->pct_of_parent = min_pct__after;
            max_child->pct_of_parent = max_pct__after;
            rd_cfg_equip_stringf(min_child->cfg, "%f", min_pct__after);
            rd_cfg_equip_stringf(max_child->cfg, "%f", max_pct__after);
            is_changing_panel_boundaries = 1;
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: animate panels
    //
#if 0 // TODO(rjf): @cfg_panels
    {
      F32 rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-50.f * rd_state->frame_dt)) : 1.f;
      Vec2F32 content_rect_dim = dim_2f32(content_rect);
      if(content_rect_dim.x > 0 && content_rect_dim.y > 0)
      {
        for(RD_PanelNode *panel = panel_tree.root;
            panel != &rd_nil_panel_node;
            panel = rd_panel_node_rec__depth_first_pre(panel).next)
        {
          Rng2F32 target_rect_px = rd_target_rect_from_panel_node(content_rect, panel_tree.root, panel);
          Rng2F32 target_rect_pct = r2f32p(target_rect_px.x0/content_rect_dim.x,
                                           target_rect_px.y0/content_rect_dim.y,
                                           target_rect_px.x1/content_rect_dim.x,
                                           target_rect_px.y1/content_rect_dim.y);
          if(abs_f32(target_rect_pct.x0 - panel->animated_rect_pct.x0) > 0.005f ||
             abs_f32(target_rect_pct.y0 - panel->animated_rect_pct.y0) > 0.005f ||
             abs_f32(target_rect_pct.x1 - panel->animated_rect_pct.x1) > 0.005f ||
             abs_f32(target_rect_pct.y1 - panel->animated_rect_pct.y1) > 0.005f)
          {
            rd_request_frame();
          }
          panel->animated_rect_pct.x0 += rate * (target_rect_pct.x0 - panel->animated_rect_pct.x0);
          panel->animated_rect_pct.y0 += rate * (target_rect_pct.y0 - panel->animated_rect_pct.y0);
          panel->animated_rect_pct.x1 += rate * (target_rect_pct.x1 - panel->animated_rect_pct.x1);
          panel->animated_rect_pct.y1 += rate * (target_rect_pct.y1 - panel->animated_rect_pct.y1);
          if(ws->frames_alive < 5 || is_changing_panel_boundaries)
          {
            panel->animated_rect_pct = target_rect_pct;
          }
        }
      }
    }
#endif
    
    ////////////////////////////
    //- rjf: panel leaf UI
    //
    ProfScope("leaf panel UI")
      for(RD_PanelNode *panel = panel_tree.root;
          panel != &rd_nil_panel_node;
          panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
    {
      if(panel->first != &rd_nil_panel_node) {continue;}
      B32 panel_is_focused = (window_is_focused &&
                              !ws->menu_bar_focused &&
                              !query_is_open &&
                              !ui_any_ctx_menu_is_open() &&
                              !ws->hover_eval_focused &&
                              panel_tree.focused == panel);
      RD_Cfg *selected_tab = panel->selected_tab;
      RD_ViewState *selected_tab_view_state = rd_view_state_from_cfg(selected_tab);
      F32 selected_tab_is_filtering_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "###is_filtering_t_%p", selected_tab), (F32)!!selected_tab_view_state->is_filtering);
      ProfScope("leaf panel UI work - %.*s", str8_varg(selected_tab->string))
        UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
      {
        //////////////////////////
        //- rjf: calculate UI rectangles
        //
        Vec2F32 content_rect_dim = dim_2f32(content_rect);
        Rng2F32 target_rect_px = rd_target_rect_from_panel_node(content_rect, panel_tree.root, panel);
        Rng2F32 target_rect_pct = r2f32p(target_rect_px.x0 / content_rect_dim.x,
                                         target_rect_px.y0 / content_rect_dim.y,
                                         target_rect_px.x1 / content_rect_dim.x,
                                         target_rect_px.y1 / content_rect_dim.y);
        // TODO(rjf): @cfg_panels animate `target_rect_pct`
        Rng2F32 panel_rect_pct = target_rect_pct;
        Rng2F32 panel_rect = r2f32p(panel_rect_pct.x0*content_rect_dim.x,
                                    panel_rect_pct.y0*content_rect_dim.y,
                                    panel_rect_pct.x1*content_rect_dim.x,
                                    panel_rect_pct.y1*content_rect_dim.y);
        panel_rect = pad_2f32(panel_rect, -1.f);
        F32 tab_bar_rheight = ui_top_font_size()*3.f;
        F32 tab_bar_vheight = ui_top_font_size()*2.6f;
        F32 tab_bar_rv_diff = tab_bar_rheight - tab_bar_vheight;
        F32 tab_spacing = ui_top_font_size()*0.4f;
        F32 filter_bar_height = ui_top_font_size()*3.f;
        Rng2F32 tab_bar_rect = r2f32p(panel_rect.x0, panel_rect.y0, panel_rect.x1, panel_rect.y0 + tab_bar_vheight);
        Rng2F32 content_rect = r2f32p(panel_rect.x0, panel_rect.y0+tab_bar_vheight, panel_rect.x1, panel_rect.y1);
        Rng2F32 filter_rect = {0};
        if(panel->tab_side == Side_Max)
        {
          tab_bar_rect.y0 = panel_rect.y1 - tab_bar_vheight;
          tab_bar_rect.y1 = panel_rect.y1;
          content_rect.y0 = panel_rect.y0;
          content_rect.y1 = panel_rect.y1 - tab_bar_vheight;
        }
        if(selected_tab_is_filtering_t > 0.01f)
        {
          filter_rect.x0 = content_rect.x0;
          filter_rect.y0 = content_rect.y0;
          filter_rect.x1 = content_rect.x1;
          content_rect.y0 += filter_bar_height*selected_tab_is_filtering_t;
          filter_rect.y1 = content_rect.y0;
        }
        
        //////////////////////////
        //- rjf: build combined split+movetab drag/drop sites
        //
        {
          RD_Cfg *view = rd_cfg_from_id(rd_state->drag_drop_regs->view);
          if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && view != &rd_nil_cfg && contains_2f32(panel_rect, ui_mouse()) && ui_key_match(ui_drop_hot_key(), ui_key_zero()))
          {
            F32 drop_site_dim_px = ceil_f32(ui_top_font_size()*7.f);
            Vec2F32 drop_site_half_dim = v2f32(drop_site_dim_px/2, drop_site_dim_px/2);
            Vec2F32 panel_center = center_2f32(panel_rect);
            F32 corner_radius = ui_top_font_size()*0.5f;
            F32 padding = ceil_f32(ui_top_font_size()*0.5f);
            struct
            {
              UI_Key key;
              Dir2 split_dir;
              Rng2F32 rect;
            }
            sites[] =
            {
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_center_%p", panel->cfg),
                Dir2_Invalid,
                r2f32(sub_2f32(panel_center, drop_site_half_dim),
                      add_2f32(panel_center, drop_site_half_dim))
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_up_%p", panel->cfg),
                Dir2_Up,
                r2f32p(panel_center.x-drop_site_half_dim.x,
                       panel_center.y-drop_site_half_dim.y - drop_site_half_dim.y*2,
                       panel_center.x+drop_site_half_dim.x,
                       panel_center.y+drop_site_half_dim.y - drop_site_half_dim.y*2),
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_down_%p", panel->cfg),
                Dir2_Down,
                r2f32p(panel_center.x-drop_site_half_dim.x,
                       panel_center.y-drop_site_half_dim.y + drop_site_half_dim.y*2,
                       panel_center.x+drop_site_half_dim.x,
                       panel_center.y+drop_site_half_dim.y + drop_site_half_dim.y*2),
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_left_%p", panel->cfg),
                Dir2_Left,
                r2f32p(panel_center.x-drop_site_half_dim.x - drop_site_half_dim.x*2,
                       panel_center.y-drop_site_half_dim.y,
                       panel_center.x+drop_site_half_dim.x - drop_site_half_dim.x*2,
                       panel_center.y+drop_site_half_dim.y),
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_right_%p", panel->cfg),
                Dir2_Right,
                r2f32p(panel_center.x-drop_site_half_dim.x + drop_site_half_dim.x*2,
                       panel_center.y-drop_site_half_dim.y,
                       panel_center.x+drop_site_half_dim.x + drop_site_half_dim.x*2,
                       panel_center.y+drop_site_half_dim.y),
              },
            };
            UI_CornerRadius(corner_radius)
              for(U64 idx = 0; idx < ArrayCount(sites); idx += 1)
            {
              UI_Key key = sites[idx].key;
              Dir2 dir = sites[idx].split_dir;
              Rng2F32 rect = sites[idx].rect;
              Axis2 split_axis = axis2_from_dir2(dir);
              Side split_side = side_from_dir2(dir);
              if(dir != Dir2_Invalid && split_axis == panel->parent->split_axis)
              {
                continue;
              }
              UI_Box *site_box = &ui_nil_box;
              {
                F32 site_open_t = ui_anim(ui_key_from_stringf(key, "open_t"), 1.f);
                UI_Rect(rect) UI_Squish(0.25f-0.25f*site_open_t) UI_Transparency(1-site_open_t)
                {
                  site_box = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
                  ui_signal_from_box(site_box);
                }
                UI_Box *site_box_viz = &ui_nil_box;
                UI_Parent(site_box) UI_WidthFill UI_HeightFill
                  UI_Padding(ui_px(padding, 1.f))
                  UI_Column
                  UI_Padding(ui_px(padding, 1.f))
                {
                  ui_set_next_child_layout_axis(axis2_flip(split_axis));
                  if(ui_key_match(key, ui_drop_hot_key()))
                  {
                    ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = rd_rgba_from_theme_color(RD_ThemeColor_Hover)));
                  }
                  site_box_viz = ui_build_box_from_key(UI_BoxFlag_DrawBackground|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_DrawDropShadow|
                                                       UI_BoxFlag_DrawBackgroundBlur, ui_key_zero());
                }
                if(dir != Dir2_Invalid)
                {
                  UI_Parent(site_box_viz) UI_WidthFill UI_HeightFill UI_Padding(ui_px(padding, 1.f))
                  {
                    ui_set_next_child_layout_axis(split_axis);
                    UI_Box *row_or_column = ui_build_box_from_key(0, ui_key_zero()); UI_Parent(row_or_column) UI_Padding(ui_px(padding, 1.f))
                    {
                      if(split_side == Side_Min) { ui_set_next_flags(UI_BoxFlag_DrawBackground); }
                      RD_Palette(RD_PaletteCode_DropSiteOverlay) ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                      ui_spacer(ui_px(padding, 1.f));
                      if(split_side == Side_Max) { ui_set_next_flags(UI_BoxFlag_DrawBackground); }
                      RD_Palette(RD_PaletteCode_DropSiteOverlay) ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                    }
                  }
                }
                else
                {
                  UI_Parent(site_box_viz) UI_WidthFill UI_HeightFill UI_Padding(ui_px(padding, 1.f))
                  {
                    ui_set_next_child_layout_axis(split_axis);
                    UI_Box *row_or_column = ui_build_box_from_key(0, ui_key_zero());
                    UI_Parent(row_or_column) UI_Padding(ui_px(padding, 1.f)) RD_Palette(RD_PaletteCode_DropSiteOverlay)
                    {
                      ui_build_box_from_key(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, ui_key_zero());
                    }
                  }
                }
              }
              if(ui_key_match(site_box->key, ui_drop_hot_key()) && rd_drag_drop())
              {
                if(dir != Dir2_Invalid)
                {
                  rd_cmd(RD_CmdKind_SplitPanel,
                         .dst_panel = panel->cfg->id,
                         .panel = rd_state->drag_drop_regs->panel,
                         .view = rd_state->drag_drop_regs->view,
                         .dir2 = dir);
                }
                else
                {
                  rd_cmd(RD_CmdKind_MoveTab,
                         .dst_panel = panel->cfg->id,
                         .panel = rd_state->drag_drop_regs->panel,
                         .view = rd_state->drag_drop_regs->view,
                         .prev_view = rd_cfg_list_last(&panel->tabs)->id);
                }
              }
            }
            for(U64 idx = 0; idx < ArrayCount(sites); idx += 1)
            {
              B32 is_drop_hot = ui_key_match(ui_drop_hot_key(), sites[idx].key);
              if(is_drop_hot)
              {
                Axis2 split_axis = axis2_from_dir2(sites[idx].split_dir);
                Side split_side = side_from_dir2(sites[idx].split_dir);
                Rng2F32 future_split_rect_target = panel_rect;
                if(sites[idx].split_dir != Dir2_Invalid)
                {
                  Vec2F32 panel_center = center_2f32(panel_rect);
                  future_split_rect_target.v[side_flip(split_side)].v[split_axis] = panel_center.v[split_axis];
                }
                future_split_rect_target = pad_2f32(future_split_rect_target, -ui_top_font_size()*2.f);
                Vec2F32 future_split_rect_target_center = center_2f32(future_split_rect_target);
                Rng2F32 future_split_rect =
                {
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v0"), future_split_rect_target.x0, .initial = future_split_rect_target_center.x),
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v1"), future_split_rect_target.y0, .initial = future_split_rect_target_center.y),
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v2"), future_split_rect_target.x1, .initial = future_split_rect_target_center.x),
                  ui_anim(ui_key_from_stringf(ui_key_zero(), "drop_site_v3"), future_split_rect_target.y1, .initial = future_split_rect_target_center.y),
                };
                UI_Rect(future_split_rect) RD_Palette(RD_PaletteCode_DropSiteOverlay) UI_CornerRadius(ui_top_font_size()*2.f)
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                }
              }
            }
          }
        }
        
        //////////////////////////
        //- rjf: build catch-all panel drop-site
        //
        B32 catchall_drop_site_hovered = 0;
        if(rd_drag_is_active() && ui_key_match(ui_key_zero(), ui_drop_hot_key()))
        {
          UI_Rect(panel_rect)
          {
            UI_Key key = ui_key_from_stringf(ui_key_zero(), "catchall_drop_site_%p", panel->cfg);
            UI_Box *catchall_drop_site = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
            ui_signal_from_box(catchall_drop_site);
            catchall_drop_site_hovered = ui_key_match(key, ui_drop_hot_key());
          }
        }
        
        //////////////////////////
        //- rjf: build filtering box
        //
#if 0 // TODO(rjf): @cfg
        {
          RD_Cfg *view = selected_tab;
          RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(view->string);
          RD_ViewState *view_state = rd_view_state_from_cfg(view);
          UI_Focus(UI_FocusKind_On) RD_RegsScope(.view = view->id)
          {
            if(view_state->is_filtering && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept))
            {
              rd_cmd(RD_CmdKind_ApplyFilter);
            }
            if(view_state->is_filtering || selected_tab_is_filtering_t > 0.01f)
            {
              UI_Box *filter_box = &ui_nil_box;
              UI_Rect(filter_rect)
              {
                ui_set_next_child_layout_axis(Axis2_X);
                filter_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip|UI_BoxFlag_DrawBorder, "filter_box_%p", view);
              }
              UI_Parent(filter_box) UI_WidthFill UI_HeightFill
              {
                UI_PrefWidth(ui_em(3.f, 1.f))
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  RD_Font(RD_FontSlot_Icons)
                  UI_TextAlignment(UI_TextAlign_Center)
                  ui_label(rd_icon_kind_text_table[RD_IconKind_Find]);
                UI_PrefWidth(ui_text_dim(10, 1))
                {
                  ui_label(str8_lit("Filter"));
                }
                ui_spacer(ui_em(0.5f, 1.f));
                RD_Font(view_rule_info->flags & RD_ViewRuleInfoFlag_FilterIsCode ? RD_FontSlot_Code : RD_FontSlot_Main)
                  UI_Focus(view_state->is_filtering ? UI_FocusKind_On : UI_FocusKind_Off)
                  UI_TextPadding(ui_top_font_size()*0.5f)
                {
                  UI_Signal sig = rd_line_edit(RD_LineEditFlag_CodeContents*!!(view_rule_info->flags & RD_ViewRuleInfoFlag_FilterIsCode),
                                               0,
                                               0,
                                               &view_state->filter_cursor,
                                               &view_state->filter_mark,
                                               view_state->filter_buffer,
                                               sizeof(view_state->filter_buffer),
                                               &view_state->filter_string_size,
                                               0,
                                               str8(view_state->filter_buffer, view_state->filter_string_size),
                                               str8_lit("###filter_text_input"));
                  if(ui_pressed(sig))
                  {
                    rd_cmd(RD_CmdKind_FocusPanel);
                  }
                }
              }
            }
          }
        }
#endif
        
        //////////////////////////
        //- rjf: panel not selected? -> darken
        //
        if(panel != panel_tree.focused)
        {
          UI_Palette(ui_build_palette(0, .background = rd_rgba_from_theme_color(RD_ThemeColor_InactivePanelOverlay)))
            UI_Rect(content_rect)
          {
            ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
          }
        }
        
        //////////////////////////
        //- rjf: build panel container box
        //
        UI_Box *panel_box = &ui_nil_box;
        UI_Rect(content_rect) UI_ChildLayoutAxis(Axis2_Y) UI_CornerRadius(0) UI_Focus(UI_FocusKind_On)
        {
          UI_Key panel_key = ui_key_from_stringf(ui_key_zero(), "panel_box_%p", panel->cfg);
          panel_box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                            UI_BoxFlag_Clip|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DisableFocusOverlay|
                                            ((panel_tree.focused != panel)*UI_BoxFlag_DisableFocusBorder),
                                            panel_key);
        }
        
        //////////////////////////
        //- rjf: loading animation for stable view
        //
        UI_Box *loading_overlay_container = &ui_nil_box;
        UI_Parent(panel_box) UI_WidthFill UI_HeightFill
        {
          loading_overlay_container = ui_build_box_from_key(UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
        }
        
        //////////////////////////
        //- rjf: build selected tab view
        //
        UI_Parent(panel_box)
          UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
          UI_WidthFill
        {
          //- rjf: push interaction registers, fill with per-view states
          rd_push_regs(.panel = panel->cfg->id,
                       .view  = selected_tab->id);
          {
            String8 view_expr = rd_expr_from_cfg(selected_tab);
            String8 view_file_path = rd_file_path_from_eval_string(rd_frame_arena(), view_expr);
            rd_regs()->file_path = view_file_path;
          }
          
          //- rjf: build view container
          UI_Box *view_container_box = &ui_nil_box;
          UI_FixedWidth(dim_2f32(content_rect).x)
            UI_FixedHeight(dim_2f32(content_rect).y)
            UI_ChildLayoutAxis(Axis2_Y)
          {
            view_container_box = ui_build_box_from_key(0, ui_key_zero());
          }
          
          //- rjf: build empty view
          UI_Parent(view_container_box) if(selected_tab == &rd_nil_cfg)
          {
            ui_set_next_flags(UI_BoxFlag_DefaultFocusNav);
            UI_Focus(UI_FocusKind_On) UI_WidthFill UI_HeightFill UI_NamedColumn(str8_lit("empty_view")) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
              UI_Padding(ui_pct(1, 0)) UI_Focus(UI_FocusKind_Null)
            {
              UI_PrefHeight(ui_em(3.f, 1.f))
                UI_Row
                UI_Padding(ui_pct(1, 0))
                UI_TextAlignment(UI_TextAlign_Center)
                UI_PrefWidth(ui_em(15.f, 1.f))
                UI_CornerRadius(ui_top_font_size()/2.f)
                RD_Palette(RD_PaletteCode_NegativePopButton)
              {
                if(ui_clicked(rd_icon_buttonf(RD_IconKind_X, 0, "Close Panel")))
                {
                  rd_cmd(RD_CmdKind_ClosePanel);
                }
              }
            }
          }
          
          //- rjf: build tab view
          UI_Parent(view_container_box) if(selected_tab != &rd_nil_cfg) ProfScope("build tab view")
          {
            rd_view_ui(content_rect);
          }
          
          //- rjf: pop interaction registers; commit if this is the selected view
          RD_Regs *view_regs = rd_pop_regs();
          if(panel_tree.focused == panel)
          {
            MemoryCopyStruct(rd_regs(), view_regs);
          }
        }
        
        ////////////////////////
        //- rjf: loading? -> fill loading overlay container
        //
        {
          F32 selected_tab_loading_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "###is_view_loading_%p", selected_tab), selected_tab_view_state->loading_t_target);
          if(selected_tab_loading_t > 0.01f) UI_Parent(loading_overlay_container)
          {
            rd_loading_overlay(panel_rect, selected_tab_loading_t, selected_tab_view_state->loading_progress_v, selected_tab_view_state->loading_progress_v_target);
          }
        }
        
        //////////////////////////
        //- rjf: take events to automatically start/end filtering, if applicable
        //
#if 0 // TODO(rjf): @cfg
        UI_Focus(UI_FocusKind_On)
        {
          RD_Cfg *view = selected_tab;
          RD_ViewState *view_state = selected_tab_view_state;
          RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(view->string);
          if(ui_is_focus_active() && view_rule_info->flags & RD_ViewRuleInfoFlag_TypingAutomaticallyFilters && !view_state->is_filtering)
          {
            for(UI_Event *evt = 0; ui_next_event(&evt);)
            {
              if(evt->flags & UI_EventFlag_Paste)
              {
                ui_eat_event(evt);
                rd_cmd(RD_CmdKind_Filter);
                rd_cmd(RD_CmdKind_Paste);
              }
              else if(evt->string.size != 0 && evt->kind == UI_EventKind_Text)
              {
                ui_eat_event(evt);
                rd_cmd(RD_CmdKind_Filter);
                rd_cmd(RD_CmdKind_InsertText, .string = evt->string);
              }
            }
          }
          if(view_rule_info->flags & RD_ViewRuleInfoFlag_CanFilter && (view_state->filter_string_size != 0 || view_state->is_filtering) && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Cancel))
          {
            rd_cmd(RD_CmdKind_ClearFilter);
          }
        }
#endif
        
        //////////////////////////
        //- rjf: consume panel fallthrough interaction events
        //
        UI_Signal panel_sig = ui_signal_from_box(panel_box);
        if(ui_pressed(panel_sig))
        {
          rd_cmd(RD_CmdKind_FocusPanel, .panel = panel->cfg->id);
        }
        if(ui_right_clicked(panel_sig))
        {
          rd_cmd(RD_CmdKind_PushQuery,
                 .view         = panel->selected_tab->id,
                 .file_path    = rd_file_path_from_eval_string(rd_frame_arena(), rd_expr_from_cfg(panel->selected_tab)),
                 .ui_key       = panel_box->key,
                 .off_px       = sub_2f32(ui_mouse(), panel_box->rect.p0),
                 .reg_slot     = RD_RegSlot_View,
                 .lister_flags = RD_ListerFlag_LineEdit|RD_ListerFlag_Commands|RD_ListerFlag_Settings);
        }
        
        //////////////////////////
        //- rjf: build tab bar
        //
        UI_Focus(UI_FocusKind_Off)
        {
          Temp scratch = scratch_begin(0, 0);
          
          // rjf: types
          typedef struct DropSite DropSite;
          struct DropSite
          {
            F32 p;
            RD_Cfg *prev_view;
          };
          
          // rjf: prep output data
          RD_Cfg *next_selected_tab = selected_tab;
          UI_Box *tab_bar_box = &ui_nil_box;
          U64 drop_site_count = panel->tabs.count+1;
          DropSite *drop_sites = push_array(scratch.arena, DropSite, drop_site_count);
          F32 drop_site_max_p = 0;
          U64 view_idx = 0;
          
          // rjf: build
          UI_CornerRadius(0)
          {
            UI_Rect(tab_bar_rect) tab_bar_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_ViewClampX|UI_BoxFlag_ViewScrollX|UI_BoxFlag_Clickable, "tab_bar_%p", panel->cfg);
            if(panel->tab_side == Side_Max)
            {
              tab_bar_box->view_off.y = tab_bar_box->view_off_target.y = (tab_bar_rheight - tab_bar_vheight);
            }
            else
            {
              tab_bar_box->view_off.y = tab_bar_box->view_off_target.y = 0;
            }
          }
          UI_Parent(tab_bar_box) UI_PrefHeight(ui_pct(1, 0))
          {
            Temp scratch = scratch_begin(0, 0);
            F32 corner_radius = ui_em(0.6f, 1.f).value;
            ui_spacer(ui_px(1.f, 1.f));
            
            // rjf: build tabs
            RD_Cfg *tab = &rd_nil_cfg;
            RD_Cfg *prev_tab = &rd_nil_cfg;
            UI_PrefWidth(ui_em(18.f, 0.5f))
              UI_CornerRadius00(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius01(panel->tab_side == Side_Min ? 0 : corner_radius)
              UI_CornerRadius10(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius11(panel->tab_side == Side_Min ? 0 : corner_radius)
              for(RD_CfgNode *tab_n = panel->tabs.first; tab_n != 0; tab_n = tab_n->next, view_idx += 1)
            {
              prev_tab = tab;
              tab = tab_n->v;
              temp_end(scratch);
              if(rd_cfg_is_project_filtered(tab)) { continue; }
              
              // rjf: if before this tab is the prev-view of the current tab drag,
              // draw empty space
              if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && catchall_drop_site_hovered)
              {
#if 0 // TODO(rjf): @cfg_dragdrop
                RD_Panel *dst_panel = rd_panel_from_handle(rd_last_drag_drop_panel);
                RD_View *drag_view = rd_view_from_handle(rd_state->drag_drop_regs->view);
                RD_View *dst_prev_view = rd_view_from_handle(rd_last_drag_drop_prev_tab);
                if(dst_panel == panel &&
                   ((!rd_view_is_nil(view) && dst_prev_view == view->order_prev && drag_view != view && drag_view != view->order_prev) ||
                    (rd_view_is_nil(view) && dst_prev_view == panel->last_tab_view && drag_view != panel->last_tab_view)))
                {
                  UI_PrefWidth(ui_em(9.f, 0.2f)) UI_Column
                  {
                    ui_spacer(ui_em(0.2f, 1.f));
                    UI_CornerRadius00(corner_radius)
                      UI_CornerRadius10(corner_radius)
                      RD_Palette(RD_PaletteCode_DropSiteOverlay)
                    {
                      ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                    }
                  }
                }
#endif
              }
              
              // rjf: end on nil view
              if(tab == &rd_nil_cfg)
              {
                break;
              }
              
              // rjf: build per-tab info
              RD_RegsScope(.panel = panel->cfg->id, .view = tab->id)
              {
                // rjf: gather info for this tab
                B32 view_is_selected = (tab == panel->selected_tab);
                DR_FStrList title_fstrs = rd_title_fstrs_from_cfg(scratch.arena, tab, ui_top_palette()->text_weak, ui_top_font_size());
                
                // rjf: begin vertical region for this tab
                ui_set_next_child_layout_axis(Axis2_Y);
                UI_Box *tab_column_box = ui_build_box_from_stringf(!is_changing_panel_boundaries*UI_BoxFlag_AnimatePosX, "tab_column_%p", tab);
                
                // rjf: build tab container box
                UI_Parent(tab_column_box) UI_PrefHeight(ui_px(tab_bar_vheight, 1)) RD_Palette(view_is_selected ? RD_PaletteCode_Tab : RD_PaletteCode_TabInactive)
                {
                  if(panel->tab_side == Side_Max)
                  {
                    ui_spacer(ui_px(tab_bar_rv_diff-1.f, 1.f));
                  }
                  else
                  {
                    ui_spacer(ui_px(1.f, 1.f));
                  }
                  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
                  UI_Box *tab_box = ui_build_box_from_stringf(UI_BoxFlag_DrawHotEffects|
                                                              UI_BoxFlag_DrawBackground|
                                                              UI_BoxFlag_DrawBorder|
                                                              (UI_BoxFlag_DrawDropShadow*view_is_selected)|
                                                              UI_BoxFlag_Clickable,
                                                              "tab_%p", tab);
                  
                  // rjf: build tab contents
                  UI_Parent(tab_box)
                  {
                    UI_WidthFill UI_Row
                    {
                      ui_spacer(ui_em(0.5f, 1.f));
                      UI_PrefWidth(ui_text_dim(10, 0))
                      {
                        UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                        ui_box_equip_display_fstrs(name_box, &title_fstrs);
                      }
                    }
                    UI_PrefWidth(ui_em(2.35f, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
                      RD_Font(RD_FontSlot_Icons)
                      UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons)*0.75f)
                      UI_Flags(UI_BoxFlag_DrawTextWeak)
                      UI_CornerRadius00(0)
                      UI_CornerRadius01(0)
                    {
                      UI_Palette *palette = ui_build_palette(ui_top_palette());
                      palette->background = v4f32(0, 0, 0, 0);
                      ui_set_next_palette(palette);
                      UI_Signal sig = ui_buttonf("%S###close_view_%p", rd_icon_kind_text_table[RD_IconKind_X], tab);
                      if(ui_clicked(sig) || ui_middle_clicked(sig))
                      {
                        rd_cmd(RD_CmdKind_CloseTab);
                      }
                    }
                  }
                  
                  // rjf: consume events for tab clicking
                  {
                    UI_Signal sig = ui_signal_from_box(tab_box);
                    if(ui_pressed(sig))
                    {
                      next_selected_tab = tab;
                      rd_cmd(RD_CmdKind_FocusPanel);
                    }
                    else if(ui_dragging(sig) && !rd_drag_is_active() && length_2f32(ui_drag_delta()) > 10.f)
                    {
                      rd_drag_begin(RD_RegSlot_View);
                    }
                    else if(ui_right_clicked(sig))
                    {
                      rd_cmd(RD_CmdKind_PushQuery,
                             .reg_slot     = RD_RegSlot_View,
                             .ui_key       = sig.box->key,
                             .off_px       = v2f32(0, sig.box->rect.y1 - sig.box->rect.y0),
                             .lister_flags = RD_ListerFlag_LineEdit|RD_ListerFlag_Commands|RD_ListerFlag_Settings);
                    }
                    else if(ui_middle_clicked(sig))
                    {
                      rd_cmd(RD_CmdKind_CloseTab);
                    }
                  }
                }
                
                // rjf: space for next tab
                {
                  ui_spacer(ui_em(0.3f, 1.f));
                }
                
                // rjf: store off drop-site
                drop_sites[view_idx].p = tab_column_box->rect.x0 - tab_spacing/2;
                drop_sites[view_idx].prev_view = prev_tab;
                drop_site_max_p = Max(tab_column_box->rect.x1, drop_site_max_p);
              }
            }
            
            // rjf: build add-new-tab button
            UI_TextAlignment(UI_TextAlign_Center)
              UI_PrefWidth(ui_px(tab_bar_vheight, 1.f))
              UI_PrefHeight(ui_px(tab_bar_vheight, 1.f))
              UI_Column
            {
              if(panel->tab_side == Side_Max)
              {
                ui_spacer(ui_px(tab_bar_rv_diff-1.f, 1.f));
              }
              else
              {
                ui_spacer(ui_px(1.f, 1.f));
              }
              UI_CornerRadius00(panel->tab_side == Side_Min ? corner_radius : 0)
                UI_CornerRadius10(panel->tab_side == Side_Min ? corner_radius : 0)
                UI_CornerRadius01(panel->tab_side == Side_Max ? corner_radius : 0)
                UI_CornerRadius11(panel->tab_side == Side_Max ? corner_radius : 0)
                RD_Font(RD_FontSlot_Icons)
                UI_FontSize(ui_top_font_size())
                UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                UI_HoverCursor(OS_Cursor_HandPoint)
                RD_Palette(RD_PaletteCode_ImplicitButton)
              {
                UI_Box *add_new_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|
                                                                UI_BoxFlag_DrawText|
                                                                UI_BoxFlag_DrawBorder|
                                                                UI_BoxFlag_DrawHotEffects|
                                                                UI_BoxFlag_DrawActiveEffects|
                                                                UI_BoxFlag_Clickable|
                                                                UI_BoxFlag_DisableTextTrunc,
                                                                "%S##add_new_tab_button_%p",
                                                                rd_icon_kind_text_table[RD_IconKind_Add],
                                                                panel->cfg);
                UI_Signal sig = ui_signal_from_box(add_new_box);
                if(ui_clicked(sig))
                {
                  rd_cmd(RD_CmdKind_FocusPanel);
                  UI_Key view_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_view_menu_key_"));
                  ui_ctx_menu_open(view_menu_key, add_new_box->key, v2f32(0, tab_bar_vheight));
                }
              }
            }
            
            scratch_end(scratch);
          }
          
          // rjf: interact with tab bar
          ui_signal_from_box(tab_bar_box);
          
          // rjf: fill out last drop site
          {
            drop_sites[drop_site_count-1].p = drop_site_max_p;
            drop_sites[drop_site_count-1].prev_view = rd_cfg_list_last(&panel->tabs);
          }
          
          // rjf: more precise drop-sites on tab bar
          {
            Vec2F32 mouse = ui_mouse();
            RD_Cfg *drag_tab = rd_cfg_from_id(rd_state->drag_drop_regs->view);
            if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && window_is_focused && contains_2f32(panel_rect, mouse) && drag_tab != &rd_nil_cfg)
            {
              // rjf: mouse => hovered drop site
              F32 min_distance = 0;
              DropSite *active_drop_site = 0;
              if(catchall_drop_site_hovered)
              {
                for(U64 drop_site_idx = 0; drop_site_idx < drop_site_count; drop_site_idx += 1)
                {
                  F32 distance = abs_f32(drop_sites[drop_site_idx].p - mouse.x);
                  if(drop_site_idx == 0 || distance < min_distance)
                  {
                    active_drop_site = &drop_sites[drop_site_idx];
                    min_distance = distance;
                  }
                }
              }
              
              // rjf: store closest prev-view
              if(active_drop_site != 0)
              {
                rd_last_drag_drop_prev_tab = active_drop_site->prev_view->id;
              }
              else
              {
                rd_last_drag_drop_prev_tab = 0;
              }
              
              // rjf: vis
              if(drag_tab != &rd_nil_cfg && active_drop_site != 0) 
              {
                RD_Palette(RD_PaletteCode_DropSiteOverlay) UI_Rect(tab_bar_rect)
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
              }
              
              // rjf: drop
              if(catchall_drop_site_hovered && (active_drop_site != 0 && rd_drag_drop()))
              {
                RD_Cfg *drag_view = rd_cfg_from_id(rd_state->drag_drop_regs->view);
                RD_Cfg *src_panel = rd_cfg_from_id(rd_state->drag_drop_regs->panel);
                RD_Cfg *dst_panel = panel->cfg;
                if(dst_panel != &rd_nil_cfg && drag_view != &rd_nil_cfg)
                {
                  rd_cmd(RD_CmdKind_MoveTab,
                         .panel     = src_panel->id,
                         .dst_panel = dst_panel->id,
                         .view      = drag_view->id,
                         .prev_view = active_drop_site->prev_view->id);
                }
              }
            }
          }
          
          // rjf: apply tab change
          if(next_selected_tab != panel->selected_tab)
          {
            rd_cmd(RD_CmdKind_FocusTab, .view = next_selected_tab->id);
          }
          
          scratch_end(scratch);
        }
        
        //////////////////////////
        //- rjf: less granular panel-wide drop-site
        //
        if(catchall_drop_site_hovered)
        {
#if 0 // TODO(rjf): @cfg_dragdrop
          rd_last_drag_drop_panel = rd_handle_from_panel(panel);
          
          RD_View *dragged_view = rd_view_from_handle(rd_state->drag_drop_regs->view);
          B32 view_is_in_panel = 0;
          for(RD_View *view = panel->first_tab_view; !rd_view_is_nil(view); view = view->order_next)
          {
            if(rd_view_is_project_filtered(view)) { continue; }
            if(view == dragged_view)
            {
              view_is_in_panel = 1;
              break;
            }
          }
          
          if(view_is_in_panel == 0)
          {
            // rjf: vis
            {
              RD_Palette(RD_PaletteCode_DropSiteOverlay) UI_Rect(content_rect)
                ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
            }
            
            // rjf: drop
            {
              if(rd_drag_drop())
              {
                RD_Panel *src_panel = rd_panel_from_handle(rd_state->drag_drop_regs->panel);
                RD_View *view = rd_view_from_handle(rd_state->drag_drop_regs->view);
                if(rd_state->drag_drop_regs_slot == RD_RegSlot_View && !rd_view_is_nil(view))
                {
                  rd_cmd(RD_CmdKind_MoveTab,
                         .prev_view = rd_handle_from_view(panel->last_tab_view),
                         .panel = rd_handle_from_panel(src_panel),
                         .dst_panel = rd_handle_from_panel(panel),
                         .view = rd_handle_from_view(view));
                }
              }
            }
          }
#endif
        }
        
        //////////////////////////
        //- rjf: accept file drops
        //
        {
          for(UI_Event *evt = 0; ui_next_event(&evt);)
          {
            if(evt->kind == UI_EventKind_FileDrop && contains_2f32(content_rect, evt->pos))
            {
              B32 need_drop_completion = 0;
              arena_clear(ws->drop_completion_arena);
              MemoryZeroStruct(&ws->drop_completion_paths);
              for(String8Node *n = evt->paths.first; n != 0; n = n->next)
              {
                Temp scratch = scratch_begin(0, 0);
                String8 path = path_normalized_from_string(scratch.arena, n->string);
                if(str8_match(str8_skip_last_dot(path), str8_lit("exe"), StringMatchFlag_CaseInsensitive))
                {
                  str8_list_push(ws->drop_completion_arena, &ws->drop_completion_paths, push_str8_copy(ws->drop_completion_arena, path));
                  need_drop_completion = 1;
                }
                else
                {
                  rd_cmd(RD_CmdKind_Open, .file_path = path);
                }
                scratch_end(scratch);
              }
              if(need_drop_completion)
              {
                ui_ctx_menu_open(rd_state->drop_completion_key, ui_key_zero(), evt->pos);
              }
              ui_eat_event(evt);
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: drag/drop cancelling
    //
    if(rd_drag_is_active() && ui_slot_press(UI_EventActionSlot_Cancel))
    {
      rd_drag_kill();
      ui_kill_action();
    }
    
    ////////////////////////////
    //- rjf: font size changing
    //
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      if(evt->kind == UI_EventKind_Scroll && evt->modifiers & OS_Modifier_Ctrl)
      {
        ui_eat_event(evt);
        if(evt->delta_2f32.y < 0)
        {
          rd_cmd(RD_CmdKind_IncUIFontScale);
        }
        else if(evt->delta_2f32.y > 0)
        {
          rd_cmd(RD_CmdKind_DecUIFontScale);
        }
      }
    }
    
    ui_end_build();
  }
  
  //////////////////////////////
  //- rjf: ensure hover eval is in-bounds
  //
  if(!ui_box_is_nil(hover_eval_box))
  {
    UI_Box *root = hover_eval_box;
    Rng2F32 window_rect = os_client_rect_from_window(ui_window());
    Rng2F32 root_rect = root->rect;
    Vec2F32 shift =
    {
      -ClampBot(0, root_rect.x1 - window_rect.x1),
      -ClampBot(0, root_rect.y1 - window_rect.y1),
    };
    Rng2F32 new_root_rect = shift_2f32(root_rect, shift);
    root->fixed_position = new_root_rect.p0;
    root->fixed_size = dim_2f32(new_root_rect);
    root->rect = new_root_rect;
    for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis + 1))
    {
      ui_calc_sizes_standalone__in_place_rec(root, axis);
      ui_calc_sizes_upwards_dependent__in_place_rec(root, axis);
      ui_calc_sizes_downwards_dependent__in_place_rec(root, axis);
      ui_layout_enforce_constraints__in_place_rec(root, axis);
      ui_layout_position__in_place_rec(root, axis);
    }
  }
  
  //////////////////////////////
  //- rjf: attach lister boxes to root, or hide if it has not been renewed
  //
  for(ListerTask *task = first_lister_task; task != 0; task = task->next)
  {
    RD_Lister *lister = task->lister;
    RD_Regs *regs = lister->regs;
    UI_Box *lister_box = ui_box_from_key(task->root_key);
    if(!ui_box_is_nil(lister_box) && (lister != ws->autocomp_lister || ws->autocomp_lister_last_frame_idx+1 >= rd_state->frame_index+1))
    {
      UI_Box *anchor_box = ui_box_from_key(lister->regs->ui_key);
      if(!ui_box_is_nil(anchor_box))
      {
        Vec2F32 size = lister_box->fixed_size;
        lister_box->fixed_position = v2f32(anchor_box->rect.x0 + lister->regs->off_px.x,
                                           anchor_box->rect.y0 + lister->regs->off_px.y);
        lister_box->rect = r2f32(lister_box->fixed_position, add_2f32(lister_box->fixed_position, size));
        for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis + 1))
        {
          ui_calc_sizes_standalone__in_place_rec(lister_box, axis);
          ui_calc_sizes_upwards_dependent__in_place_rec(lister_box, axis);
          ui_calc_sizes_downwards_dependent__in_place_rec(lister_box, axis);
          ui_layout_enforce_constraints__in_place_rec(lister_box, axis);
          ui_layout_position__in_place_rec(lister_box, axis);
        }
      }
    }
    else if(!ui_box_is_nil(lister_box) && lister == ws->autocomp_lister && ws->autocomp_lister_last_frame_idx+1 < rd_state->frame_index+1)
    {
      UI_Box *anchor_box = ui_box_from_key(lister->regs->ui_key);
      if(!ui_box_is_nil(anchor_box))
      {
        Vec2F32 size = lister_box->fixed_size;
        Rng2F32 window_rect = os_client_rect_from_window(ws->os);
        lister_box->fixed_position = v2f32(window_rect.x1, window_rect.y1);
        lister_box->rect = r2f32(lister_box->fixed_position, add_2f32(lister_box->fixed_position, size));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: hover eval cancelling
  //
  if(ws->hover_eval_string.size != 0 && ui_slot_press(UI_EventActionSlot_Cancel))
  {
    MemoryZeroStruct(&ws->hover_eval_string);
    arena_clear(ws->hover_eval_arena);
    rd_request_frame();
  }
  
  //////////////////////////////
  //- rjf: animate
  //
  if(ui_animating_from_state(ws->ui))
  {
    rd_request_frame();
  }
  
  //////////////////////////////
  //- rjf: draw UI
  //
  ws->draw_bucket = dr_bucket_make();
  DR_BucketScope(ws->draw_bucket)
    ProfScope("draw UI")
  {
    Temp scratch = scratch_begin(0, 0);
    F32 box_squish_epsilon = 0.01f;
    Rng2F32 window_rect = os_client_rect_from_window(ws->os);
    
    //- rjf: unpack settings
    B32 do_background_blur = rd_setting_b32_from_name(str8_lit("background_blur"));
    
    //- rjf: set up heatmap buckets
    F32 heatmap_bucket_size = 32.f;
    U64 *heatmap_buckets = 0;
    U64 heatmap_bucket_pitch = 0;
    U64 heatmap_bucket_count = 0;
    if(DEV_draw_ui_box_heatmap)
    {
      Rng2F32 rect = os_client_rect_from_window(ws->os);
      Vec2F32 size = dim_2f32(rect);
      Vec2S32 buckets_dim = {(S32)(size.x/heatmap_bucket_size), (S32)(size.y/heatmap_bucket_size)};
      heatmap_bucket_pitch = buckets_dim.x;
      heatmap_bucket_count = buckets_dim.x*buckets_dim.y;
      heatmap_buckets = push_array(scratch.arena, U64, heatmap_bucket_count);
    }
    
    //- rjf: draw background color
    {
      Vec4F32 bg_color = rd_rgba_from_theme_color(RD_ThemeColor_BaseBackground);
      dr_rect(os_client_rect_from_window(ws->os), bg_color, 0, 0, 0);
    }
    
    //- rjf: draw window border
    {
      Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_BaseBorder);
      dr_rect(os_client_rect_from_window(ws->os), color, 0, 1.f, 0.5f);
    }
    
    //- rjf: recurse & draw
    U64 total_heatmap_sum_count = 0;
    for(UI_Box *box = ui_root_from_state(ws->ui); !ui_box_is_nil(box);)
    {
      // rjf: get recursion
      UI_BoxRec rec = ui_box_rec_df_post(box, &ui_nil_box);
      
      // rjf: sum to box heatmap
      if(DEV_draw_ui_box_heatmap)
      {
        Vec2F32 center = center_2f32(box->rect);
        Vec2S32 p = v2s32(center.x / heatmap_bucket_size, center.y / heatmap_bucket_size);
        U64 bucket_idx = p.y * heatmap_bucket_pitch + p.x;
        if(bucket_idx < heatmap_bucket_count)
        {
          heatmap_buckets[bucket_idx] += 1;
          total_heatmap_sum_count += 1;
        }
      }
      
      // rjf: push transparency
      if(box->transparency != 0)
      {
        dr_push_transparency(box->transparency);
      }
      
      // rjf: push squish
      if(box->squish > box_squish_epsilon)
      {
        Vec2F32 box_dim = dim_2f32(box->rect);
        Mat3x3F32 box2origin_xform = make_translate_3x3f32(v2f32(-box->rect.x0 - box_dim.x/2, -box->rect.y0));
        Mat3x3F32 scale_xform = make_scale_3x3f32(v2f32(1-box->squish, 1-box->squish));
        Mat3x3F32 origin2box_xform = make_translate_3x3f32(v2f32(box->rect.x0 + box_dim.x/2, box->rect.y0));
        Mat3x3F32 xform = mul_3x3f32(origin2box_xform, mul_3x3f32(scale_xform, box2origin_xform));
        dr_push_xform2d(xform);
        dr_push_tex2d_sample_kind(R_Tex2DSampleKind_Linear);
      }
      
      // rjf: draw drop shadow
      if(box->flags & UI_BoxFlag_DrawDropShadow)
      {
        Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
        Vec4F32 drop_shadow_color = rd_rgba_from_theme_color(RD_ThemeColor_DropShadow);
        dr_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
      }
      
      // rjf: blur background
      if(box->flags & UI_BoxFlag_DrawBackgroundBlur && do_background_blur)
      {
        R_PassParams_Blur *params = dr_blur(pad_2f32(box->rect, 1.f), box->blur_size*(1-box->transparency), 0);
        MemoryCopyArray(params->corner_radii, box->corner_radii);
      }
      
      // rjf: draw background
      if(box->flags & UI_BoxFlag_DrawBackground)
      {
        F32 effective_active_t = box->active_t;
        if(!(box->flags & UI_BoxFlag_DrawActiveEffects))
        {
          effective_active_t = 0;
        }
        F32 t = box->hot_t*(1-effective_active_t);
        
        // rjf: hot effect extension (drop shadow)
        if(box->flags & UI_BoxFlag_DrawHotEffects)
        {
          Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
          Vec4F32 drop_shadow_color = rd_rgba_from_theme_color(RD_ThemeColor_DropShadow);
          drop_shadow_color.w *= t;
          drop_shadow_color.w *= box->palette->colors[UI_ColorCode_Background].w;
          dr_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
        }
        
        // rjf: main rectangle
        {
          R_Rect2DInst *inst = dr_rect(pad_2f32(box->rect, 1.f), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
          MemoryCopyArray(inst->corner_radii, box->corner_radii);
        }
        
        // rjf: hot effect extension
        if(box->flags & UI_BoxFlag_DrawHotEffects)
        {
          // rjf: brighten
          {
            R_Rect2DInst *inst = dr_rect(box->rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Hover);
            color.w *= t*0.2f;
            inst->colors[Corner_00] = color;
            inst->colors[Corner_10] = color;
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: slight emboss fadeoff
          if(0)
          {
            Rng2F32 rect = r2f32p(box->rect.x0,
                                  box->rect.y0,
                                  box->rect.x1,
                                  box->rect.y1);
            R_Rect2DInst *inst = dr_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
            inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
            inst->colors[Corner_10] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
            inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
        }
        
        // rjf: active effect extension
        if(box->flags & UI_BoxFlag_DrawActiveEffects)
        {
          Vec4F32 shadow_color = rd_rgba_from_theme_color(RD_ThemeColor_DropShadow);
          shadow_color.w *= 0.5f*box->active_t;
          Vec2F32 shadow_size =
          {
            (box->rect.x1 - box->rect.x0)*0.60f*box->active_t,
            (box->rect.y1 - box->rect.y0)*0.60f*box->active_t,
          };
          shadow_size.x = Clamp(0, shadow_size.x, box->font_size*2.f);
          shadow_size.y = Clamp(0, shadow_size.y, box->font_size*2.f);
          
          // rjf: top -> bottom dark effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
            inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: bottom -> top light effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0, box->rect.y1 - shadow_size.y, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
            inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.4f, 0.4f, 0.4f, 0.4f*box->active_t);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: left -> right dark effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x0 + shadow_size.x, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.f);
            inst->colors[Corner_00] = shadow_color;
            inst->colors[Corner_01] = shadow_color;
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: right -> left dark effect
          {
            R_Rect2DInst *inst = dr_rect(r2f32p(box->rect.x1 - shadow_size.x, box->rect.y0, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.f);
            inst->colors[Corner_10] = shadow_color;
            inst->colors[Corner_11] = shadow_color;
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
        }
      }
      
      // rjf: draw string
      if(box->flags & UI_BoxFlag_DrawText)
      {
        Vec2F32 text_position = ui_box_text_position(box);
        if(DEV_draw_ui_text_pos)
        {
          dr_rect(r2f32p(text_position.x-4, text_position.y-4, text_position.x+4, text_position.y+4),
                  v4f32(1, 0, 1, 1), 1, 0, 1);
        }
        F32 max_x = 100000.f;
        FNT_Run ellipses_run = {0};
        if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
        {
          FNT_Tag ellipses_font = box->font;
          F32 ellipses_size = box->font_size;
          FNT_RasterFlags ellipses_raster_flags = box->text_raster_flags;
          if(box->display_fstrs.last)
          {
            ellipses_font = box->display_fstrs.last->v.params.font;
            ellipses_size = box->display_fstrs.last->v.params.size;
            ellipses_raster_flags = box->display_fstrs.last->v.params.raster_flags;
          }
          max_x = (box->rect.x1-text_position.x);
          ellipses_run = fnt_push_run_from_string(scratch.arena, ellipses_font, ellipses_size, 0, box->tab_size, ellipses_raster_flags, str8_lit("..."));
        }
        dr_truncated_fancy_run_list(text_position, &box->display_fruns, max_x, ellipses_run);
        if(box->flags & UI_BoxFlag_HasFuzzyMatchRanges)
        {
          Vec4F32 match_color = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlay);
          dr_truncated_fancy_run_fuzzy_matches(text_position, &box->display_fruns, max_x, &box->fuzzy_match_ranges, match_color);
        }
      }
      
      // rjf: draw focus viz
      if(DEV_draw_ui_focus_debug)
      {
        B32 focused = (box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive) &&
                       box->flags & UI_BoxFlag_Clickable);
        B32 disabled = 0;
        for(UI_Box *p = box; !ui_box_is_nil(p); p = p->parent)
        {
          if(p->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
          {
            disabled = 1;
            break;
          }
        }
        if(focused)
        {
          Vec4F32 color = v4f32(0.3f, 0.8f, 0.3f, 1.f);
          if(disabled)
          {
            color = v4f32(0.8f, 0.3f, 0.3f, 1.f);
          }
          dr_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), color, 2, 0, 1);
          dr_rect(box->rect, color, 2, 2, 1);
        }
        if(box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive))
        {
          if(box->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
          {
            dr_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(1, 0, 0, 0.2f), 2, 0, 1);
          }
          else
          {
            dr_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(0, 1, 0, 0.2f), 2, 0, 1);
          }
        }
      }
      
      // rjf: push clip
      if(box->flags & UI_BoxFlag_Clip)
      {
        Rng2F32 top_clip = dr_top_clip();
        Rng2F32 new_clip = pad_2f32(box->rect, -1);
        if(top_clip.x1 != 0 || top_clip.y1 != 0)
        {
          new_clip = intersect_2f32(new_clip, top_clip);
        }
        dr_push_clip(new_clip);
      }
      
      // rjf: custom draw list
      if(box->flags & UI_BoxFlag_DrawBucket)
      {
        Mat3x3F32 xform = make_translate_3x3f32(box->position_delta);
        DR_XForm2DScope(xform)
        {
          dr_sub_bucket(box->draw_bucket);
        }
      }
      
      // rjf: call custom draw callback
      if(box->custom_draw != 0)
      {
        box->custom_draw(box, box->custom_draw_user_data);
      }
      
      // rjf: pop
      {
        S32 pop_idx = 0;
        for(UI_Box *b = box; !ui_box_is_nil(b) && pop_idx <= rec.pop_count; b = b->parent)
        {
          pop_idx += 1;
          if(b == box && rec.push_count != 0)
          {
            continue;
          }
          
          // rjf: pop clips
          if(b->flags & UI_BoxFlag_Clip)
          {
            dr_pop_clip();
          }
          
          // rjf: draw overlay
          if(b->flags & UI_BoxFlag_DrawOverlay)
          {
            R_Rect2DInst *inst = dr_rect(b->rect, b->palette->colors[UI_ColorCode_Overlay], 0, 0, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw border
          if(b->flags & UI_BoxFlag_DrawBorder)
          {
            Rng2F32 b_border_rect = pad_2f32(b->rect, 1.f);
            R_Rect2DInst *inst = dr_rect(b_border_rect, b->palette->colors[UI_ColorCode_Border], 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
            
            // rjf: hover effect
            if(b->flags & UI_BoxFlag_DrawHotEffects)
            {
              Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Hover);
              color.w *= b->hot_t;
              R_Rect2DInst *inst = dr_rect(b_border_rect, color, 0, 1.f, 1.f);
              inst->colors[Corner_01].w *= 0.2f;
              inst->colors[Corner_11].w *= 0.2f;
              MemoryCopyArray(inst->corner_radii, b->corner_radii);
            }
          }
          
          // rjf: debug border rendering
          if(0)
          {
            R_Rect2DInst *inst = dr_rect(b->rect, v4f32(1, 0, 1, 0.25f), 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw sides
          {
            Rng2F32 r = b->rect;
            F32 half_thickness = 1.f;
            F32 softness = 0.f;
            if(b->flags & UI_BoxFlag_DrawSideTop)
            {
              dr_rect(r2f32p(r.x0, r.y0, r.x1, r.y0+2*half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideBottom)
            {
              dr_rect(r2f32p(r.x0, r.y1-2*half_thickness, r.x1, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideLeft)
            {
              dr_rect(r2f32p(r.x0, r.y0, r.x0+2*half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideRight)
            {
              dr_rect(r2f32p(r.x1-2*half_thickness, r.y0, r.x1, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
          }
          
          // rjf: draw focus overlay
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusOverlay) && b->focus_hot_t > 0.01f)
          {
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
            color.w *= 0.2f*b->focus_hot_t;
            R_Rect2DInst *inst = dr_rect(b->rect, color, 0, 0, 0.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw focus border
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
          {
            Rng2F32 rect = b->rect;
            if(b->flags & UI_BoxFlag_Floating)
            {
              rect = pad_2f32(rect, 1.f);
              rect = intersect_2f32(window_rect, rect);
            }
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
            color.w *= b->focus_active_t;
            R_Rect2DInst *inst = dr_rect(rect, color, 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: disabled overlay
          if(b->disabled_t >= 0.005f)
          {
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_DisabledOverlay);
            color.w *= b->disabled_t;
            R_Rect2DInst *inst = dr_rect(b->rect, color, 0, 0, 1);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: pop squish
          if(b->squish > box_squish_epsilon)
          {
            dr_pop_xform2d();
            dr_pop_tex2d_sample_kind();
          }
          
          // rjf: pop transparency
          if(b->transparency != 0)
          {
            dr_pop_transparency();
          }
        }
      }
      
      // rjf: next
      box = rec.next;
    }
    
    //- rjf: draw heatmap
    if(DEV_draw_ui_box_heatmap)
    {
      U64 uniform_dist_count = total_heatmap_sum_count / heatmap_bucket_count;
      uniform_dist_count = ClampBot(uniform_dist_count, 10);
      for(U64 bucket_idx = 0; bucket_idx < heatmap_bucket_count; bucket_idx += 1)
      {
        U64 x = bucket_idx % heatmap_bucket_pitch;
        U64 y = bucket_idx / heatmap_bucket_pitch;
        U64 bucket = heatmap_buckets[bucket_idx];
        F32 pct = (F32)bucket / uniform_dist_count;
        pct = Clamp(0, pct, 1);
        Vec3F32 hsv = v3f32((1-pct) * 0.9411f, 1, 0.5f);
        Vec3F32 rgb = rgb_from_hsv(hsv);
        Rng2F32 rect = r2f32p(x*heatmap_bucket_size, y*heatmap_bucket_size, (x+1)*heatmap_bucket_size, (y+1)*heatmap_bucket_size);
        dr_rect(rect, v4f32(rgb.x, rgb.y, rgb.z, 0.3f), 0, 0, 0);
      }
    }
    
    //- rjf: draw border/overlay color to signify error
    if(ws->error_t > 0.01f)
    {
      Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_NegativePopButtonBackground);
      color.w *= ws->error_t;
      Rng2F32 rect = os_client_rect_from_window(ws->os);
      dr_rect(pad_2f32(rect, 24.f), color, 0, 16.f, 12.f);
      dr_rect(rect, v4f32(color.x, color.y, color.z, color.w*0.05f), 0, 0, 0);
    }
    
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: increment per-window frame counter
  //
  ws->frames_alive += 1;
  
  ProfEnd();
  scratch_end(scratch);
}

#if COMPILER_MSVC && !BUILD_DEBUG
#pragma optimize("", on)
#endif

////////////////////////////////
//~ rjf: Eval Visualization

typedef struct RD_CtrlEntityExpandAccel RD_CtrlEntityExpandAccel;
struct RD_CtrlEntityExpandAccel
{
  CTRL_EntityArray entities;
};

#if 0 // TODO(rjf): @cfg
typedef struct RD_EntityExpandAccel RD_EntityExpandAccel;
struct RD_EntityExpandAccel
{
  RD_EntityArray entities;
};

//- rjf: control entity hierarchies

EV_EXPAND_RULE_INFO_FUNCTION_DEF(scheduler_machine)
{
  EV_ExpandInfo info = {0};
  Temp scratch = scratch_begin(&arena, 1);
  E_Eval eval = e_eval_from_expr(scratch.arena, expr);
  CTRL_Entity *machine = rd_ctrl_entity_from_eval_space(eval.space);
  if(machine->kind == CTRL_EntityKind_Machine)
  {
    CTRL_EntityList processes = {0};
    for(CTRL_Entity *child = machine->first; child != &ctrl_entity_nil; child = child->next)
    {
      if(child->kind == CTRL_EntityKind_Process)
      {
        ctrl_entity_list_push(scratch.arena, &processes, child);
      }
    }
    CTRL_EntityArray *processes_array = push_array(arena, CTRL_EntityArray, 1);
    *processes_array = ctrl_entity_array_from_list(arena, &processes);
    info.user_data = processes_array;
    info.row_count = processes.count;
    info.rows_default_expanded = 1;
  }
  scratch_end(scratch);
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(scheduler_machine)
{
  EV_ExpandRangeInfo info = {0};
  {
    CTRL_EntityArray *processes = (CTRL_EntityArray *)user_data;
    if(processes != 0)
    {
      info.row_exprs_count = dim_1u64(idx_range);
      info.row_strings     = push_array(arena, String8,    info.row_exprs_count);
      info.row_view_rules  = push_array(arena, String8,    info.row_exprs_count);
      info.row_exprs       = push_array(arena, E_Expr *,   info.row_exprs_count);
      info.row_members     = push_array(arena, E_Member *, info.row_exprs_count);
      U64 row_expr_idx = 0;
      for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, row_expr_idx += 1)
      {
        CTRL_Entity *process = processes->v[idx];
        E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafOffset, 0);
        expr->space    = rd_eval_space_from_ctrl_entity(process, RD_EvalSpaceKind_MetaCtrlEntity);
        expr->mode     = E_Mode_Offset;
        expr->type_key = e_type_key_cons_base(type(CTRL_ProcessMetaEval));;
        info.row_exprs[row_expr_idx]   = expr;
        info.row_members[row_expr_idx] = &e_member_nil;
      }
    }
  }
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(scheduler_machine)
{
  return num;
}

EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(scheduler_machine)
{
  return id;
}

EV_EXPAND_RULE_INFO_FUNCTION_DEF(scheduler_process)
{
  EV_ExpandInfo info = {0};
  Temp scratch = scratch_begin(&arena, 1);
  E_Eval eval = e_eval_from_expr(scratch.arena, expr);
  CTRL_Entity *process = rd_ctrl_entity_from_eval_space(eval.space);
  if(process->kind == CTRL_EntityKind_Process)
  {
    CTRL_EntityList threads = {0};
    for(CTRL_Entity *child = process->first; child != &ctrl_entity_nil; child = child->next)
    {
      if(child->kind == CTRL_EntityKind_Thread)
      {
        B32 is_in_filter = 1;
        if(filter.size != 0)
        {
          is_in_filter = 0;
          FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, child->string);
          if(matches.count == matches.needle_part_count)
          {
            is_in_filter = 1;
          }
          else
          {
            DI_Scope *di_scope = di_scope_open();
            CTRL_Unwind unwind = d_query_cached_unwind_from_thread(child);
            CTRL_CallStack call_stack = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &unwind);
            for(U64 idx = 0; idx < call_stack.concrete_frame_count && idx < 5; idx += 1)
            {
              CTRL_CallStackFrame *f = &call_stack.frames[idx];
              String8 name = {0};
              name.str = rdi_string_from_idx(f->rdi, f->procedure->name_string_idx, &name.size);
              FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, name);
              if(matches.count == matches.needle_part_count)
              {
                is_in_filter = 1;
                break;
              }
            }
            di_scope_close(di_scope);
          }
        }
        if(is_in_filter)
        {
          ctrl_entity_list_push(scratch.arena, &threads, child);
        }
      }
    }
    CTRL_EntityArray *threads_array = push_array(arena, CTRL_EntityArray, 1);
    *threads_array = ctrl_entity_array_from_list(arena, &threads);
    info.user_data = threads_array;
    info.row_count = threads.count;
  }
  scratch_end(scratch);
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(scheduler_process)
{
  EV_ExpandRangeInfo info = {0};
  {
    CTRL_EntityArray *threads = (CTRL_EntityArray *)user_data;
    if(threads != 0)
    {
      info.row_exprs_count = dim_1u64(idx_range);
      info.row_strings     = push_array(arena, String8,    info.row_exprs_count);
      info.row_view_rules  = push_array(arena, String8,    info.row_exprs_count);
      info.row_exprs       = push_array(arena, E_Expr *,   info.row_exprs_count);
      info.row_members     = push_array(arena, E_Member *, info.row_exprs_count);
      U64 row_expr_idx = 0;
      for(U64 idx = idx_range.min; idx < idx_range.max; idx += 1, row_expr_idx += 1)
      {
        CTRL_Entity *thread = threads->v[idx];
        E_Expr *expr = e_push_expr(arena, E_ExprKind_LeafOffset, 0);
        expr->space    = rd_eval_space_from_ctrl_entity(thread, RD_EvalSpaceKind_MetaCtrlEntity);
        expr->mode     = E_Mode_Offset;
        expr->type_key = e_type_key_cons_base(type(CTRL_ThreadMetaEval));
        info.row_exprs[row_expr_idx]   = expr;
        info.row_members[row_expr_idx] = &e_member_nil;
      }
    }
  }
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(scheduler_process)
{
  return num;
}

EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(scheduler_process)
{
  return id;
}
#endif

//- rjf: commands

E_LOOKUP_INFO_FUNCTION_DEF(commands)
{
  E_LookupInfo result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List cmd_names = {0};
    for EachNonZeroEnumVal(RD_CmdKind, k)
    {
      RD_CmdKindInfo *info = &rd_cmd_kind_info_table[k];
      String8 name = info->string;
      str8_list_push(scratch.arena, &cmd_names, name);
    }
    String8Array *accel = push_array(arena, String8Array, 1);
    *accel = str8_array_from_list(arena, &cmd_names);
    result.user_data = accel;
    result.idxed_expr_count = accel->count;
    scratch_end(scratch);
  }
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(commands)
{
  E_LookupAccess result = {{&e_irnode_nil}};
  if(kind == E_ExprKind_ArrayIndex)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8Array *accel = (String8Array *)user_data;
    E_IRTreeAndType rhs_irtree = e_irtree_and_type_from_expr(scratch.arena, rhs);
    E_OpList rhs_oplist = e_oplist_from_irtree(scratch.arena, rhs_irtree.root);
    String8 rhs_bytecode = e_bytecode_from_oplist(scratch.arena, &rhs_oplist);
    E_Interpretation rhs_interp = e_interpret(rhs_bytecode);
    E_Value rhs_value = rhs_interp.value;
    if(rhs_value.u64 < accel->count)
    {
      String8 cmd_name = accel->v[rhs_value.u64];
      RD_CmdKind cmd_kind = rd_cmd_kind_from_string(cmd_name);
      result.irtree_and_type.root      = e_irtree_set_space(arena, e_space_make(RD_EvalSpaceKind_MetaCmd), e_irtree_const_u(arena, (U64)cmd_kind));
      result.irtree_and_type.type_key  = e_type_key_basic(E_TypeKind_U64);
      result.irtree_and_type.mode      = E_Mode_Value;
    }
    scratch_end(scratch);
  }
  return result;
}

//- rjf: watch expressions

E_LOOKUP_INFO_FUNCTION_DEF(watches)
{
  E_LookupInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    RD_CfgList cfgs_list = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("watch"));
    RD_CfgList cfgs_list__filtered = cfgs_list;
    if(filter.size != 0)
    {
      MemoryZeroStruct(&cfgs_list__filtered);
      for(RD_CfgNode *n = cfgs_list.first; n != 0; n = n->next)
      {
        String8 expr = rd_expr_from_cfg(n->v);
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, expr);
        if(matches.count == matches.needle_part_count)
        {
          rd_cfg_list_push(scratch.arena, &cfgs_list__filtered, n->v);
        }
      }
    }
    RD_CfgArray *cfgs = push_array(arena, RD_CfgArray, 1);
    cfgs[0] = rd_cfg_array_from_list(arena, &cfgs_list__filtered);
    result.user_data = cfgs;
    result.idxed_expr_count = cfgs->count + 1;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(watches)
{
  RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, cfgs->count);
  Rng1U64 read_range = intersect_1u64(idx_range, legal_idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    U64 cfg_idx = read_range.min + idx;
    if(cfg_idx < cfgs->count)
    {
      String8 expr_string = rd_cfg_child_from_string(cfgs->v[cfg_idx], str8_lit("expression"))->first->string;
      exprs[idx] = e_parse_expr_from_text(arena, expr_string);
      exprs_strings[idx] = push_str8_copy(arena, expr_string);
    }
  }
}

E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(watches)
{
  U64 id = 0;
  RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
  if(1 <= num && num <= cfgs->count)
  {
    U64 idx = (num-1);
    id = cfgs->v[idx]->id;
  }
  else if(num == cfgs->count+1)
  {
    id = max_U64;
  }
  return id;
}

E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(watches)
{
  U64 num = 0;
  RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
  if(id != 0 && id != max_U64)
  {
    for EachIndex(idx, cfgs->count)
    {
      if(cfgs->v[idx]->id == id)
      {
        num = idx+1;
        break;
      }
    }
  }
  else if(id == max_U64)
  {
    num = cfgs->count + 1;
  }
  return num;
}

//- rjf: local variables

E_LOOKUP_INFO_FUNCTION_DEF(locals)
{
  E_LookupInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    E_String2NumMapNodeArray nodes = e_string2num_map_node_array_from_map(scratch.arena, e_parse_state->ctx->locals_map);
    e_string2num_map_node_array_sort__in_place(&nodes);
    String8List exprs_filtered = {0};
    for EachIndex(idx, nodes.count)
    {
      String8 local_expr_string = nodes.v[idx]->string;
      FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, local_expr_string);
      if(matches.count == matches.needle_part_count)
      {
        str8_list_push(scratch.arena, &exprs_filtered, local_expr_string);
      }
    }
    String8Array *accel = push_array(arena, String8Array, 1);
    *accel = str8_array_from_list(arena, &exprs_filtered);
    result.user_data = accel;
    result.idxed_expr_count = accel->count;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(locals)
{
  String8Array *accel = (String8Array *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, accel->count);
  Rng1U64 read_range = intersect_1u64(idx_range, legal_idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    String8 expr_string = accel->v[read_range.min + idx];
    exprs[idx] = e_parse_expr_from_text(arena, expr_string);
    exprs_strings[idx] = push_str8_copy(arena, expr_string);
  }
}

//- rjf: registers

E_LOOKUP_INFO_FUNCTION_DEF(registers)
{
  Temp scratch = scratch_begin(&arena, 1);
  CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
  Arch arch = thread->arch;
  U64 reg_count     = regs_reg_code_count_from_arch(arch);
  U64 alias_count   = regs_alias_code_count_from_arch(arch);
  String8 *reg_strings   = regs_reg_code_string_table_from_arch(arch);
  String8 *alias_strings = regs_alias_code_string_table_from_arch(arch);
  String8List exprs_list = {0};
  for(U64 idx = 1; idx < reg_count; idx += 1)
  {
    FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, reg_strings[idx]);
    if(matches.count == matches.needle_part_count)
    {
      str8_list_push(scratch.arena, &exprs_list, reg_strings[idx]);
    }
  }
  for(U64 idx = 1; idx < alias_count; idx += 1)
  {
    FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, alias_strings[idx]);
    if(matches.count == matches.needle_part_count)
    {
      str8_list_push(scratch.arena, &exprs_list, alias_strings[idx]);
    }
  }
  String8Array *accel = push_array(arena, String8Array, 1);
  *accel = str8_array_from_list(arena, &exprs_list);
  E_LookupInfo info = {accel, 0, accel->count};
  scratch_end(scratch);
  return info;
}

E_LOOKUP_RANGE_FUNCTION_DEF(registers)
{
  String8Array *accel = (String8Array *)user_data;
  Rng1U64 legal_idx_range = r1u64(0, accel->count);
  Rng1U64 read_range = intersect_1u64(legal_idx_range, idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    String8 register_name = accel->v[read_range.min + idx];
    String8 register_expr = push_str8f(arena, "reg:%S", register_name);
    exprs_strings[idx] = register_name;
    exprs[idx] = e_parse_expr_from_text(arena, register_expr);
  }
}

//- rjf: top-level configurations

typedef struct RD_TopLevelCfgLookupAccel RD_TopLevelCfgLookupAccel;
struct RD_TopLevelCfgLookupAccel
{
  String8Array cmds;
  RD_CfgArray cfgs;
  Rng1U64 cmds_idx_range;
  Rng1U64 cfgs_idx_range;
};

E_LOOKUP_INFO_FUNCTION_DEF(top_level_cfg)
{
  E_LookupInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    E_TypeKey lhs_type_key = lhs->type_key;
    E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
    String8 cfg_name = rd_singular_from_code_name_plural(lhs_type->name);
    RD_CfgList cfgs_list = rd_cfg_top_level_list_from_string(scratch.arena, cfg_name);\
    String8List cmds_list = {0};
    // TODO(rjf): @cfg hack - probably want to table-drive this
    if(str8_match(cfg_name, str8_lit("target"), 0))
    {
      str8_list_push(arena, &cmds_list, rd_cmd_kind_info_table[RD_CmdKind_AddTarget].string);
    }
    else if(str8_match(cfg_name, str8_lit("breakpoint"), 0))
    {
      str8_list_push(arena, &cmds_list, rd_cmd_kind_info_table[RD_CmdKind_AddBreakpoint].string);
      str8_list_push(arena, &cmds_list, rd_cmd_kind_info_table[RD_CmdKind_AddAddressBreakpoint].string);
      str8_list_push(arena, &cmds_list, rd_cmd_kind_info_table[RD_CmdKind_AddFunctionBreakpoint].string);
    }
    else if(str8_match(cfg_name, str8_lit("watch_pin"), 0))
    {
      str8_list_push(arena, &cmds_list, rd_cmd_kind_info_table[RD_CmdKind_AddWatchPin].string);
    }
    RD_TopLevelCfgLookupAccel *accel = push_array(arena, RD_TopLevelCfgLookupAccel, 1);
    accel->cfgs = rd_cfg_array_from_list(arena, &cfgs_list);
    accel->cmds = str8_array_from_list(arena, &cmds_list);
    accel->cmds_idx_range = r1u64(0, accel->cmds.count);
    accel->cfgs_idx_range = r1u64(accel->cmds_idx_range.max, accel->cmds_idx_range.max + accel->cfgs.count);
    result.user_data = accel;
    result.idxed_expr_count = accel->cfgs.count + accel->cmds.count;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(top_level_cfg)
{
  E_LookupAccess result = {{&e_irnode_nil}};
  if(kind == E_ExprKind_ArrayIndex)
  {
    Temp scratch = scratch_begin(&arena, 1);
    RD_TopLevelCfgLookupAccel *accel = (RD_TopLevelCfgLookupAccel *)user_data;
    E_IRTreeAndType rhs_irtree = e_irtree_and_type_from_expr(scratch.arena, rhs);
    E_OpList rhs_oplist = e_oplist_from_irtree(scratch.arena, rhs_irtree.root);
    String8 rhs_bytecode = e_bytecode_from_oplist(scratch.arena, &rhs_oplist);
    E_Interpretation rhs_interp = e_interpret(rhs_bytecode);
    E_Value rhs_value = rhs_interp.value;
    if(rhs_value.u64 < accel->cfgs.count)
    {
      RD_Cfg *cfg = accel->cfgs.v[rhs_value.u64];
      E_Space cfg_space = rd_eval_space_from_cfg(cfg);
      String8 cfg_name = cfg->string;
      E_TypeKey cfg_type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, cfg_name);
      result.irtree_and_type.root      = e_irtree_set_space(arena, cfg_space, e_irtree_const_u(arena, 0));
      result.irtree_and_type.type_key  = cfg_type_key;
      result.irtree_and_type.mode      = E_Mode_Offset;
    }
    scratch_end(scratch);
  }
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(top_level_cfg)
{
  RD_TopLevelCfgLookupAccel *accel = (RD_TopLevelCfgLookupAccel *)user_data;
  Rng1U64 cmds_idx_range = accel->cmds_idx_range;
  Rng1U64 cfgs_idx_range = accel->cfgs_idx_range;
  U64 dst_idx = 0;
  E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(arena, lhs);
  
  // rjf: fill commands
  {
    Rng1U64 read_range = intersect_1u64(cmds_idx_range, idx_range);
    U64 read_count = dim_1u64(read_range);
    E_Expr *commands = e_parse_expr_from_text(arena, str8_lit("query:commands"));
    E_IRTreeAndType commands_irtree = e_irtree_and_type_from_expr(arena, commands);
    for(U64 idx = 0; idx < read_count; idx += 1, dst_idx += 1)
    {
      String8 cmd_name = accel->cmds.v[idx + read_range.min - cmds_idx_range.min];
      RD_CmdKind cmd_kind = rd_cmd_kind_from_string(cmd_name);
      exprs[dst_idx] = e_expr_irext_array_index(arena, commands, &commands_irtree, (U64)cmd_kind-1);
    }
  }
  
  // rjf: fill cfgs
  {
    Rng1U64 read_range = intersect_1u64(cfgs_idx_range, idx_range);
    U64 read_count = dim_1u64(read_range);
    for(U64 idx = 0; idx < read_count; idx += 1, dst_idx += 1)
    {
      exprs[dst_idx] = e_expr_irext_array_index(arena, lhs, &lhs_irtree, idx + read_range.min - cfgs_idx_range.min);
    }
  }
}

E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(top_level_cfg)
{
  U64 id = 0;
  RD_TopLevelCfgLookupAccel *accel = (RD_TopLevelCfgLookupAccel *)user_data;
  if(num != 0)
  {
    U64 idx = num-1;
    if(contains_1u64(accel->cfgs_idx_range, idx))
    {
      RD_Cfg *cfg = accel->cfgs.v[idx - accel->cfgs_idx_range.min];
      id = cfg->id;
    }
    else if(contains_1u64(accel->cmds_idx_range, idx))
    {
      id = num;
      id |= (1ull<<63);
    }
  }
  return id;
}

E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(top_level_cfg)
{
  U64 num = 0;
  RD_TopLevelCfgLookupAccel *accel = (RD_TopLevelCfgLookupAccel *)user_data;
  if(id != 0)
  {
    if(id & (1ull<<63))
    {
      num = id;
      num &= ~(1ull<<63);
    }
    else for EachIndex(idx, accel->cfgs.count)
    {
      if(accel->cfgs.v[idx]->id == id)
      {
        num = idx + accel->cfgs_idx_range.min + 1;
        break;
      }
    }
  }
  return num;
}

//- rjf: threads / callstacks

E_LOOKUP_INFO_FUNCTION_DEF(thread)
{
  E_LookupInfo result = E_LOOKUP_INFO_FUNCTION_NAME(default)(arena, lhs, filter);
  result.named_expr_count += 1;
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_LookupAccess result = E_LOOKUP_ACCESS_FUNCTION_NAME(default)(arena, kind, lhs, rhs, user_data);
  if(kind == E_ExprKind_MemberAccess && rhs->kind == E_ExprKind_LeafMember && str8_match(rhs->string, str8_lit("call_stack"), 0))
  {
    E_Eval eval = e_eval_from_expr(scratch.arena, lhs);
    CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(eval.space);
    result.irtree_and_type.root     = e_irtree_set_space(arena, e_space_make(RD_EvalSpaceKind_MetaCtrlEntity), e_irtree_leaf_u128(arena, u128_make(entity->handle.machine_id, entity->handle.dmn_handle.u64[0])));
    result.irtree_and_type.type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = str8_lit("call_stack"));
    result.irtree_and_type.mode     = E_Mode_Offset;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(thread)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_LOOKUP_RANGE_FUNCTION_NAME(default)(arena, lhs, idx_range, exprs, exprs_strings, user_data);
  E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, lhs);
  E_Type *lhs_type = e_type_from_key__cached(lhs_irtree.type_key);
  Rng1U64 extras_idx_range = r1u64(lhs_type->count, max_U64);
  Rng1U64 extras_read_range = intersect_1u64(extras_idx_range, idx_range);
  U64 extras_count = dim_1u64(extras_read_range);
  for(U64 extras_idx = 0; extras_idx < extras_count; extras_idx += 1)
  {
    U64 out_idx = extras_idx_range.min - idx_range.min + extras_idx;
    exprs[out_idx] = e_expr_irext_member_access(arena, lhs, &lhs_irtree, str8_lit("call_stack"));
  }
  scratch_end(scratch);
}

typedef struct RD_CallStackLookupAccel RD_CallStackLookupAccel;
struct RD_CallStackLookupAccel
{
  Arch arch;
  CTRL_Handle process;
  CTRL_CallStack call_stack;
};

E_LOOKUP_INFO_FUNCTION_DEF(call_stack)
{
  E_LookupInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    RD_CallStackLookupAccel *accel = push_array(arena, RD_CallStackLookupAccel, 1);
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, lhs->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interp = e_interpret(bytecode);
    U128 u128 = interp.value.u128;
    CTRL_Handle handle = {0};
    MemoryCopyStruct(&handle, &u128);
    CTRL_Entity *entity = ctrl_entity_from_handle(d_state->ctrl_entity_store, handle);
    if(entity->kind == CTRL_EntityKind_Thread)
    {
      CTRL_Entity *process = ctrl_process_from_entity(entity);
      CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(entity);
      accel->arch = entity->arch;
      accel->process = process->handle;
      accel->call_stack = ctrl_call_stack_from_unwind(arena, rd_state->frame_di_scope, process, &base_unwind);
      result.idxed_expr_count = accel->call_stack.count;
    }
    result.user_data = accel;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(call_stack)
{
  E_LookupAccess result = {{&e_irnode_nil}};
  if(kind == E_ExprKind_ArrayIndex)
  {
    Temp scratch = scratch_begin(&arena, 1);
    E_IRTreeAndType rhs_irtree = e_irtree_and_type_from_expr(scratch.arena, rhs);
    E_OpList rhs_oplist = e_oplist_from_irtree(scratch.arena, rhs_irtree.root);
    String8 rhs_bytecode = e_bytecode_from_oplist(scratch.arena, &rhs_oplist);
    E_Interpretation rhs_interp = e_interpret(rhs_bytecode);
    E_Value rhs_value = rhs_interp.value;
    RD_CallStackLookupAccel *accel = (RD_CallStackLookupAccel *)user_data;
    CTRL_CallStack *call_stack = &accel->call_stack;
    if(0 <= rhs_value.u64 && rhs_value.u64 < call_stack->count)
    {
      CTRL_Entity *process = ctrl_entity_from_handle(d_state->ctrl_entity_store, accel->process);
      CTRL_CallStackFrame *f = &call_stack->frames[rhs_value.u64];
      result.irtree_and_type.root = e_irtree_set_space(arena, rd_eval_space_from_ctrl_entity(process, RD_EvalSpaceKind_CtrlEntity), e_irtree_const_u(arena, regs_rip_from_arch_block(accel->arch, f->regs)));
      result.irtree_and_type.type_key = e_type_key_cons(.arch = process->arch, .kind = E_TypeKind_Ptr, .direct_key = e_type_key_basic(E_TypeKind_Function), .count = 1, .depth = f->inline_depth);
      result.irtree_and_type.mode = E_Mode_Value;
    }
    scratch_end(scratch);
  }
  return result;
}

//- rjf: targets / environment

E_LOOKUP_INFO_FUNCTION_DEF(target)
{
  E_LookupInfo result = E_LOOKUP_INFO_FUNCTION_NAME(default)(arena, lhs, filter);
  result.named_expr_count += 1;
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(target)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_LookupAccess result = E_LOOKUP_ACCESS_FUNCTION_NAME(default)(arena, kind, lhs, rhs, user_data);
  if(kind == E_ExprKind_MemberAccess && rhs->kind == E_ExprKind_LeafMember && str8_match(rhs->string, str8_lit("environment"), 0))
  {
    E_Eval eval = e_eval_from_expr(scratch.arena, lhs);
    RD_Cfg *cfg = rd_cfg_from_eval_space(eval.space);
    result.irtree_and_type.root     = e_irtree_set_space(arena, e_space_make(RD_EvalSpaceKind_MetaCfgCollection), e_irtree_const_u(arena, cfg->id));
    result.irtree_and_type.type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = str8_lit("environment"));
    result.irtree_and_type.mode     = E_Mode_Offset;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(target)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_LOOKUP_RANGE_FUNCTION_NAME(default)(arena, lhs, idx_range, exprs, exprs_strings, user_data);
  E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(scratch.arena, lhs);
  E_Type *lhs_type = e_type_from_key__cached(lhs_irtree.type_key);
  Rng1U64 extras_idx_range = r1u64(lhs_type->count, max_U64);
  Rng1U64 extras_read_range = intersect_1u64(extras_idx_range, idx_range);
  U64 extras_count = dim_1u64(extras_read_range);
  for(U64 extras_idx = 0; extras_idx < extras_count; extras_idx += 1)
  {
    U64 out_idx = extras_idx_range.min - idx_range.min + extras_idx;
    exprs[out_idx] = e_expr_irext_member_access(arena, lhs, &lhs_irtree, str8_lit("environment"));
  }
  scratch_end(scratch);
}

E_LOOKUP_INFO_FUNCTION_DEF(environment)
{
  E_LookupInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    E_OpList oplist = e_oplist_from_irtree(scratch.arena, lhs->root);
    String8 bytecode = e_bytecode_from_oplist(scratch.arena, &oplist);
    E_Interpretation interpret = e_interpret(bytecode);
    RD_CfgID id = interpret.value.u64;
    RD_Cfg *target = rd_cfg_from_id(id);
    RD_CfgList env_strings = {0};
    for(RD_Cfg *child = target->first; child != &rd_nil_cfg; child = child->next)
    {
      if(str8_match(child->string, str8_lit("environment"), 0))
      {
        rd_cfg_list_push(scratch.arena, &env_strings, child);
      }
    }
    RD_CfgArray *accel = push_array(arena, RD_CfgArray, 1);
    *accel = rd_cfg_array_from_list(arena, &env_strings);
    result.user_data = accel;
    result.idxed_expr_count = accel->count + 1;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(environment)
{
  E_LookupAccess result = {{&e_irnode_nil}};
  if(kind == E_ExprKind_ArrayIndex)
  {
    Temp scratch = scratch_begin(&arena, 1);
    RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
    E_IRTreeAndType rhs_irtree = e_irtree_and_type_from_expr(scratch.arena, rhs);
    E_OpList rhs_oplist = e_oplist_from_irtree(scratch.arena, rhs_irtree.root);
    String8 rhs_bytecode = e_bytecode_from_oplist(scratch.arena, &rhs_oplist);
    E_Interpretation rhs_interp = e_interpret(rhs_bytecode);
    E_Value rhs_value = rhs_interp.value;
    if(0 <= rhs_value.u64 && rhs_value.u64 < cfgs->count)
    {
      RD_Cfg *cfg = cfgs->v[rhs_value.u64];
      result.irtree_and_type.root      = e_irtree_set_space(arena, rd_eval_space_from_cfg(cfg), e_irtree_const_u(arena, 0));
      result.irtree_and_type.type_key  = e_type_key_cons_ptr(arch_from_context(), e_type_key_basic(E_TypeKind_U8), 1, E_TypeFlag_IsCodeText);
      result.irtree_and_type.mode      = E_Mode_Offset;
    }
    scratch_end(scratch);
  }
  return result;
}

E_LOOKUP_RANGE_FUNCTION_DEF(environment)
{
  RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
  E_IRTreeAndType lhs_irtree = e_irtree_and_type_from_expr(arena, lhs);
  Rng1U64 legal_idx_range = r1u64(0, cfgs->count);
  Rng1U64 read_range = intersect_1u64(idx_range, legal_idx_range);
  U64 read_range_count = dim_1u64(read_range);
  for(U64 idx = 0; idx < read_range_count; idx += 1)
  {
    U64 cfg_idx = read_range.min + idx;
    if(cfg_idx < cfgs->count)
    {
      exprs[idx] = e_expr_irext_array_index(arena, lhs, &lhs_irtree, cfg_idx);
    }
  }
}

E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(environment)
{
  U64 id = 0;
  RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
  if(1 <= num && num <= cfgs->count)
  {
    U64 idx = (num-1);
    id = cfgs->v[idx]->id;
  }
  else if(num == cfgs->count+1)
  {
    id = max_U64;
  }
  return id;
}

E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(environment)
{
  U64 num = 0;
  RD_CfgArray *cfgs = (RD_CfgArray *)user_data;
  if(id != 0 && id != max_U64)
  {
    for EachIndex(idx, cfgs->count)
    {
      if(cfgs->v[idx]->id == id)
      {
        num = idx+1;
        break;
      }
    }
  }
  else if(id == max_U64)
  {
    num = cfgs->count + 1;
  }
  return num;
}

//- rjf: control entities

E_LOOKUP_INFO_FUNCTION_DEF(ctrl_entities)
{
  E_LookupInfo result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  {
    E_TypeKey lhs_type_key = lhs->type_key;
    E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
    String8 name = rd_singular_from_code_name_plural(lhs_type->name);
    CTRL_EntityKind entity_kind = CTRL_EntityKind_Null;
    for EachNonZeroEnumVal(CTRL_EntityKind, k)
    {
      if(str8_match(name, ctrl_entity_kind_code_name_table[k], 0))
      {
        entity_kind = k;
        break;
      }
    }
    CTRL_EntityList list = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, entity_kind);
    CTRL_EntityList list__filtered = list;
    if(filter.size != 0)
    {
      MemoryZeroStruct(&list__filtered);
      for(CTRL_EntityNode *n = list.first; n != 0; n = n->next)
      {
        CTRL_Entity *entity = n->v;
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, entity->string);
        if(matches.count == matches.needle_part_count)
        {
          ctrl_entity_list_push(scratch.arena, &list__filtered, entity);
        }
      }
    }
    CTRL_EntityArray *array = push_array(arena, CTRL_EntityArray, 1);
    *array = ctrl_entity_array_from_list(arena, &list__filtered);
    result.user_data = array;
    result.idxed_expr_count = list.count;
  }
  scratch_end(scratch);
  return result;
}

E_LOOKUP_ACCESS_FUNCTION_DEF(ctrl_entities)
{
  E_LookupAccess result = {{&e_irnode_nil}};
  if(kind == E_ExprKind_ArrayIndex)
  {
    Temp scratch = scratch_begin(&arena, 1);
    CTRL_EntityArray *entities = (CTRL_EntityArray *)user_data;
    E_IRTreeAndType rhs_irtree = e_irtree_and_type_from_expr(scratch.arena, rhs);
    E_OpList rhs_oplist = e_oplist_from_irtree(scratch.arena, rhs_irtree.root);
    String8 rhs_bytecode = e_bytecode_from_oplist(scratch.arena, &rhs_oplist);
    E_Interpretation rhs_interp = e_interpret(rhs_bytecode);
    E_Value rhs_value = rhs_interp.value;
    if(0 <= rhs_value.u64 && rhs_value.u64 < entities->count)
    {
      CTRL_Entity *entity = entities->v[rhs_value.u64];
      E_Space entity_space = rd_eval_space_from_ctrl_entity(entity, RD_EvalSpaceKind_MetaCtrlEntity);
      String8 entity_name = ctrl_entity_kind_code_name_table[entity->kind];
      E_TypeKey entity_type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, entity_name);
      result.irtree_and_type.root      = e_irtree_set_space(arena, entity_space, e_irtree_const_u(arena, 0));
      result.irtree_and_type.type_key  = entity_type_key;
      result.irtree_and_type.mode      = E_Mode_Offset;
    }
    scratch_end(scratch);
  }
  return result;
}

//- rjf: debug info tables

typedef struct RD_DebugInfoTableLookupAccel RD_DebugInfoTableLookupAccel;
struct RD_DebugInfoTableLookupAccel
{
  RDI_SectionKind section;
  U64 rdis_count;
  RDI_Parsed **rdis;
  DI_SearchItemArray items;
};

E_LOOKUP_INFO_FUNCTION_DEF(debug_info_table)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  // rjf: determine which debug info section we're dealing with
  RDI_SectionKind section = RDI_SectionKind_NULL;
  {
    E_TypeKey lhs_type_key = lhs->type_key;
    E_Type *lhs_type = e_type_from_key__cached(lhs_type_key);
    if(0){}
    else if(str8_match(lhs_type->name, str8_lit("procedures"), 0))       {section = RDI_SectionKind_Procedures;}
    else if(str8_match(lhs_type->name, str8_lit("globals"), 0))          {section = RDI_SectionKind_GlobalVariables;}
    else if(str8_match(lhs_type->name, str8_lit("thread_locals"), 0))    {section = RDI_SectionKind_ThreadVariables;}
    else if(str8_match(lhs_type->name, str8_lit("types"), 0))            {section = RDI_SectionKind_UDTs;}
  }
  
  // rjf: gather debug info table items
  RD_DebugInfoTableLookupAccel *accel = push_array(arena, RD_DebugInfoTableLookupAccel, 1);
  if(section != RDI_SectionKind_NULL)
  {
    U64 endt_us = d_state->frame_eval_memread_endt_us;
    
    //- rjf: unpack context
    DI_KeyList dbgi_keys_list = d_push_active_dbgi_key_list(scratch.arena);
    DI_KeyArray dbgi_keys = di_key_array_from_list(scratch.arena, &dbgi_keys_list);
    U64 rdis_count = dbgi_keys.count;
    RDI_Parsed **rdis = push_array(arena, RDI_Parsed *, rdis_count);
    for(U64 idx = 0; idx < rdis_count; idx += 1)
    {
      rdis[idx] = di_rdi_from_key(rd_state->frame_di_scope, &dbgi_keys.v[idx], endt_us);
    }
    
    //- rjf: query all filtered items from dbgi searching system
    U128 fuzzy_search_key = {d_hash_from_string(str8_struct(&rd_regs()->view)), (U64)section};
    B32 items_stale = 0;
    DI_SearchParams params = {section, dbgi_keys};
    accel->section = section;
    accel->rdis_count = rdis_count;
    accel->rdis = rdis;
    accel->items = di_search_items_from_key_params_query(rd_state->frame_di_scope, fuzzy_search_key, &params, filter, endt_us, &items_stale);
    if(items_stale)
    {
      rd_request_frame();
    }
  }
  E_LookupInfo info = {accel, 0, accel->items.count};
  scratch_end(scratch);
  return info;
}

E_LOOKUP_RANGE_FUNCTION_DEF(debug_info_table)
{
  RD_DebugInfoTableLookupAccel *accel = (RD_DebugInfoTableLookupAccel *)user_data;
  U64 needed_row_count = dim_1u64(idx_range);
  for EachIndex(idx, needed_row_count)
  {
    // rjf: unpack row
    DI_SearchItem *item = &accel->items.v[idx_range.min + idx];
    
    // rjf: skip bad elements
    if(item->dbgi_idx >= accel->rdis_count)
    {
      continue;
    }
    
    // rjf: unpack row info
    RDI_Parsed *rdi = accel->rdis[item->dbgi_idx];
    E_Module *module = &e_parse_state->ctx->modules[item->dbgi_idx];
    
    // rjf: build expr
    E_Expr *item_expr = &e_expr_nil;
    {
      U64 element_idx = item->idx;
      switch(accel->section)
      {
        default:{}break;
        case RDI_SectionKind_Procedures:
        {
          RDI_Procedure *procedure = rdi_element_from_name_idx(module->rdi, Procedures, element_idx);
          RDI_Scope *scope = rdi_element_from_name_idx(module->rdi, Scopes, procedure->root_scope_idx);
          U64 voff = *rdi_element_from_name_idx(module->rdi, ScopeVOffData, scope->voff_range_first);
          E_OpList oplist = {0};
          e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU64, e_value_u64(module->vaddr_range.min + voff));
          String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
          U32 type_idx = procedure->type_idx;
          RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
          E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_state->ctx->modules));
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
          e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU64, e_value_u64(module->vaddr_range.min + voff));
          String8 bytecode = e_bytecode_from_oplist(arena, &oplist);
          U32 type_idx = gvar->type_idx;
          RDI_TypeNode *type_node = rdi_element_from_name_idx(module->rdi, TypeNodes, type_idx);
          E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_state->ctx->modules));
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
          E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), type_idx, (U32)(module - e_parse_state->ctx->modules));
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
          E_TypeKey type_key = e_type_key_ext(e_type_kind_from_rdi(type_node->kind), udt->self_type_idx, (U32)(module - e_parse_state->ctx->modules));
          item_expr = e_push_expr(arena, E_ExprKind_TypeIdent, 0);
          item_expr->type_key = type_key;
        }break;
      }
    }
    
    // rjf: fill
    exprs[idx] = item_expr;
  }
}

E_LOOKUP_ID_FROM_NUM_FUNCTION_DEF(debug_info_table)
{
  RD_DebugInfoTableLookupAccel *accel = (RD_DebugInfoTableLookupAccel *)user_data;
  U64 id = 0;
  if(0 < num && num <= accel->items.count)
  {
    id = accel->items.v[num-1].idx+1;
  }
  return id;
}

E_LOOKUP_NUM_FROM_ID_FUNCTION_DEF(debug_info_table)
{
  RD_DebugInfoTableLookupAccel *accel = (RD_DebugInfoTableLookupAccel *)user_data;
  U64 num = di_search_item_num_from_array_element_idx__linear_search(&accel->items, id-1);
  return num;
}

internal F32
rd_append_value_strings_from_eval(Arena *arena, EV_StringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, S32 depth, E_Expr *root_expr, E_Eval eval, String8List *out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  F32 space_taken = 0;
  
  //- rjf: unpack view rules
  U32 radix = default_radix;
  U32 min_digits = 0;
  B32 no_addr = 0;
  B32 no_string = 0;
  B32 has_array = 0;
  for(E_Expr *tag = root_expr->first_tag; tag != &e_expr_nil; tag = tag->next)
  {
    if(0){}
    else if(str8_match(tag->string, str8_lit("dec"), 0)) {radix = 10;}
    else if(str8_match(tag->string, str8_lit("hex"), 0)) {radix = 16;}
    else if(str8_match(tag->string, str8_lit("bin"), 0)) {radix = 2; }
    else if(str8_match(tag->string, str8_lit("oct"), 0)) {radix = 8; }
    else if(str8_match(tag->string, str8_lit("no_addr"), 0)) {no_addr = 1;}
    else if(str8_match(tag->string, str8_lit("no_string"), 0)) {no_string = 1;}
    else if(str8_match(tag->string, str8_lit("array"), 0)) {has_array = 1;}
    else if(str8_match(tag->string, str8_lit("digits"), 0))
    {
      E_Expr *num_expr = tag->first->next;
      E_Eval num_eval = e_eval_from_expr(scratch.arena, num_expr);
      E_Eval num_value_eval = e_value_eval_from_eval(num_eval);
      min_digits = (U32)num_value_eval.value.u64;
    }
  }
  if(eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity ||
     eval.space.kind == RD_EvalSpaceKind_MetaCfg)
  {
    E_TypeKind kind = e_type_kind_from_key(eval.type_key);
    if(kind != E_TypeKind_Ptr)
    {
      no_addr = 1;
    }
    else
    {
      E_Type *type = e_type_from_key__cached(eval.type_key);
      if(!(type->flags & E_TypeFlag_External))
      {
        no_addr = 1;
      }
    }
  }
  
  //- rjf: type evaluations -> display type string
  if(eval.mode == E_Mode_Null && !e_type_key_match(e_type_key_zero(), eval.type_key))
  {
    String8 string = e_type_string_from_key(arena, eval.type_key);
    str8_list_push(arena, out, string);
  }
  
  //- rjf: value/offset evaluations
  else if(max_size > 0)
  {
    E_TypeKey type_key = e_type_unwrap(eval.type_key);
    E_TypeKind kind = e_type_kind_from_key(type_key);
    switch(kind)
    {
      //- rjf: default - leaf cases
      default:
      {
        E_Eval value_eval = e_value_eval_from_eval(eval);
        String8 string = ev_string_from_simple_typed_eval(arena, flags, radix, min_digits, value_eval);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
        str8_list_push(arena, out, string);
      }break;
      
      //- rjf: pointers
      case E_TypeKind_Function:
      case E_TypeKind_Ptr:
      case E_TypeKind_LRef:
      case E_TypeKind_RRef:
      {
        // rjf: unpack type info
        E_TypeKey type_key = e_type_unwrap(eval.type_key);
        E_Type *type = e_type_from_key__cached(type_key);
        E_TypeKind type_kind = type->kind;
        E_TypeKey direct_type_key = e_type_unwrap(e_type_ptee_from_key(type_key));
        E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
        
        // rjf: unpack info about pointer destination
        E_Eval value_eval = e_value_eval_from_eval(eval);
        B32 ptee_has_content = (direct_type_kind != E_TypeKind_Null && direct_type_kind != E_TypeKind_Void);
        B32 ptee_has_string  = ((E_TypeKind_Char8 <= direct_type_kind && direct_type_kind <= E_TypeKind_UChar32) ||
                                direct_type_kind == E_TypeKind_S8 ||
                                direct_type_kind == E_TypeKind_U8);
        E_Space space = value_eval.space;
        CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
        CTRL_Entity *process = ctrl_process_from_entity(entity);
        if(process == &ctrl_entity_nil)
        {
          CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
          process = ctrl_process_from_entity(thread);
        }
        String8 symbol_name = d_symbol_name_from_process_vaddr(arena, process, value_eval.value.u64, type->depth, 1);
        
        // rjf: special case: push strings for textual string content
        B32 did_content = 0;
        B32 did_string = 0;
        if(!did_content && ptee_has_string && !no_string)
        {
          did_content = 1;
          did_string = 1;
          U64 string_memory_addr = value_eval.value.u64;
          U64 element_size = e_type_byte_size_from_key(direct_type_key);
          U64 string_buffer_size = 1024;
          U8 *string_buffer = push_array(arena, U8, string_buffer_size);
          for(U64 try_size = string_buffer_size; try_size >= 16; try_size /= 2)
          {
            B32 read_good = e_space_read(eval.space, string_buffer, r1u64(string_memory_addr, string_memory_addr+try_size));
            if(read_good)
            {
              break;
            }
          }
          string_buffer[string_buffer_size-1] = 0;
          String8 string = {0};
          switch(element_size)
          {
            default:{string = str8_cstring((char *)string_buffer);}break;
            case 2: {string = str8_from_16(arena, str16_cstring((U16 *)string_buffer));}break;
            case 4: {string = str8_from_32(arena, str32_cstring((U32 *)string_buffer));}break;
          }
          String8 string_escaped = string;
          if(!no_addr || depth > 0)
          {
            string_escaped = ev_escaped_from_raw_string(arena, string);
          }
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string_escaped).x;
          space_taken += 2*fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
          if(!no_addr || depth > 0)
          {
            str8_list_push(arena, out, str8_lit("\""));
          }
          str8_list_push(arena, out, string_escaped);
          if(!no_addr || depth > 0)
          {
            str8_list_push(arena, out, str8_lit("\""));
          }
        }
        
        // rjf: special case: push strings for symbols
        if(value_eval.value.u64 != 0 &&
           !did_content && symbol_name.size != 0 &&
           flags & EV_StringFlag_ReadOnlyDisplayRules &&
           ((type_kind == E_TypeKind_Ptr && direct_type_kind == E_TypeKind_Void) ||
            (type_kind == E_TypeKind_Ptr && direct_type_kind == E_TypeKind_Function) ||
            (type_kind == E_TypeKind_Function)))
        {
          did_content = 1;
          str8_list_push(arena, out, symbol_name);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, symbol_name).x;
        }
        
        // rjf: special case: need symbol name, don't have one
        if(value_eval.value.u64 != 0 &&
           !did_content && symbol_name.size == 0 &&
           flags & EV_StringFlag_ReadOnlyDisplayRules &&
           ((type_kind == E_TypeKind_Ptr && direct_type_kind == E_TypeKind_Function) ||
            (type_kind == E_TypeKind_Function)) &&
           (flags & EV_StringFlag_ReadOnlyDisplayRules))
        {
          did_content = 1;
          String8 string = str8_lit("???");
          str8_list_push(arena, out, string);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
        }
        
        // rjf: push pointer value
        B32 did_ptr_value = 0;
        if(!no_addr || value_eval.value.u64 == 0)
        {
          did_ptr_value = 1;
          if(did_content)
          {
            String8 left_paren = str8_lit(" (");
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, left_paren).x;
            str8_list_push(arena, out, left_paren);
          }
          String8 string = ev_string_from_simple_typed_eval(arena, flags, radix, min_digits, value_eval);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
          str8_list_push(arena, out, string);
          if(did_content)
          {
            String8 right_paren = str8_lit(")");
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, right_paren).x;
            str8_list_push(arena, out, right_paren);
          }
        }
        
        // rjf: descend for all other cases
        B32 did_arrow = 0;
        if(value_eval.value.u64 != 0 && !did_content && ptee_has_content && (flags & EV_StringFlag_ReadOnlyDisplayRules))
        {
          if(did_ptr_value && !did_arrow)
          {
            did_arrow = 1;
            String8 arrow = str8_lit(" -> ");
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, arrow).x;
            str8_list_push(arena, out, arrow);
          }
          did_content = 1;
          if(depth < 4)
          {
            if(type->count == 1)
            {
              E_Expr *deref_expr = e_expr_ref_deref(scratch.arena, eval.expr);
              E_Eval deref_eval = e_eval_from_expr(scratch.arena, deref_expr);
              space_taken += rd_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, root_expr, deref_eval, out);
            }
            else
            {
              String8 opener_string = str8_lit("[");
              String8 closer_string = str8_lit("]");
              
              // rjf: opener ({, [)
              {
                str8_list_push(arena, out, opener_string);
                space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, opener_string).x;
              }
              
              // rjf: contents
              {
                E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, eval.expr);
                E_LookupRuleTagPair lookup_rule_and_tag = e_lookup_rule_tag_pair_from_expr_irtree(eval.expr, &irtree);
                E_LookupRule *lookup_rule = lookup_rule_and_tag.rule;
                E_Expr *lookup_rule_tag = lookup_rule_and_tag.tag;
                E_LookupInfo lookup_info = lookup_rule->info(arena, &irtree, str8_zero());
                U64 total_possible_child_count = Max(lookup_info.idxed_expr_count, lookup_info.named_expr_count);
                B32 is_first = 1;
                for(U64 idx = 0; idx < total_possible_child_count && max_size > space_taken; idx += 1)
                {
                  E_Expr *expr = &e_expr_nil;
                  String8 expr_string = {0};
                  lookup_rule->range(scratch.arena, eval.expr, r1u64(idx, idx+1), &expr, &expr_string, lookup_info.user_data);
                  if(expr != &e_expr_nil)
                  {
                    if(!is_first)
                    {
                      String8 comma = str8_lit(", ");
                      space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
                      str8_list_push(arena, out, comma);
                    }
                    is_first = 0;
                    E_Eval child_eval = e_eval_from_expr(scratch.arena, expr);
                    space_taken += rd_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, root_expr, child_eval, out);
                    if(space_taken > max_size && idx+1 < total_possible_child_count)
                    {
                      String8 ellipses = str8_lit(", ...");
                      space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
                      str8_list_push(arena, out, ellipses);
                    }
                  }
                }
              }
              
              // rjf: closer (}, ])
              {
                str8_list_push(arena, out, closer_string);
                space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, closer_string).x;
              }
            }
          }
          else
          {
            String8 ellipses = str8_lit("...");
            str8_list_push(arena, out, ellipses);
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
          }
        }
      }break;
      
      //- rjf: arrays
      case E_TypeKind_Array:
      {
        // rjf: unpack type info
        E_Type *eval_type = e_type_from_key__cached(e_type_unwrap(eval.type_key));
        E_TypeKey direct_type_key = e_type_unwrap(eval_type->direct_type_key);
        E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
        U64 array_count = eval_type->count;
        
        // rjf: get pointed-at type
        B32 array_is_string = ((E_TypeKind_Char8 <= direct_type_kind && direct_type_kind <= E_TypeKind_UChar32) ||
                               direct_type_kind == E_TypeKind_S8 ||
                               direct_type_kind == E_TypeKind_U8);
        
        // rjf: special case: push strings for textual string content
        B32 did_content = 0;
        if(!did_content && array_is_string && !no_string)
        {
          U64 element_size = e_type_byte_size_from_key(direct_type_key);
          did_content = 1;
          U64 string_buffer_size = Clamp(1, array_count, 1024);
          U8 *string_buffer = push_array(arena, U8, string_buffer_size);
          switch(eval.mode)
          {
            default:{}break;
            case E_Mode_Offset:
            {
              U64 string_memory_addr = eval.value.u64;
              B32 read_good = e_space_read(eval.space, string_buffer, r1u64(string_memory_addr, string_memory_addr+string_buffer_size));
            }break;
            case E_Mode_Value:
            {
              MemoryCopy(string_buffer, &eval.value.u512[0], Min(string_buffer_size, sizeof(eval.value)));
            }break;
          }
          String8 string = {0};
          switch(element_size)
          {
            default:{string = str8_cstring_capped(string_buffer, string_buffer + string_buffer_size);}break;
            case 2: {string = str8_from_16(arena, str16_cstring_capped(string_buffer, string_buffer + string_buffer_size));}break;
            case 4: {string = str8_from_32(arena, str32_cstring((U32 *)string_buffer));}break;
          }
          String8 string_escaped = ev_escaped_from_raw_string(arena, string);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string_escaped).x;
          space_taken += 2*fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
          if(!no_addr || depth > 0)
          {
            str8_list_push(arena, out, str8_lit("\""));
          }
          str8_list_push(arena, out, string_escaped);
          if(!no_addr || depth > 0)
          {
            str8_list_push(arena, out, str8_lit("\""));
          }
        }
        
        // rjf: if we did not do any special content, then go to the regular arrays/structs/sets case
        if(!did_content)
        {
          goto arrays_and_sets_and_structs;
        }
      }break;
      
      //- rjf: non-string-arrays/structs/sets
      case E_TypeKind_Struct:
      case E_TypeKind_Union:
      case E_TypeKind_Class:
      case E_TypeKind_IncompleteStruct:
      case E_TypeKind_IncompleteUnion:
      case E_TypeKind_IncompleteClass:
      case E_TypeKind_Set:
      arrays_and_sets_and_structs:
      {
        String8 opener_string = str8_lit("{");
        String8 closer_string = str8_lit("}");
        if(kind == E_TypeKind_Array)
        {
          opener_string = str8_lit("[");
          closer_string = str8_lit("]");
        }
        
        // rjf: opener ({, [)
        {
          str8_list_push(arena, out, opener_string);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, opener_string).x;
        }
        
        // rjf: build contents
        if(depth < 4)
        {
          E_IRTreeAndType irtree = e_irtree_and_type_from_expr(scratch.arena, eval.expr);
          E_LookupRuleTagPair lookup_rule_and_tag = e_lookup_rule_tag_pair_from_expr_irtree(eval.expr, &irtree);
          E_LookupRule *lookup_rule = lookup_rule_and_tag.rule;
          E_Expr *lookup_rule_tag = lookup_rule_and_tag.tag;
          E_LookupInfo lookup_info = lookup_rule->info(arena, &irtree, str8_zero());
          U64 total_possible_child_count = Max(lookup_info.idxed_expr_count, lookup_info.named_expr_count);
          B32 is_first = 1;
          for(U64 idx = 0; idx < total_possible_child_count && max_size > space_taken; idx += 1)
          {
            E_Expr *expr = &e_expr_nil;
            String8 expr_string = {0};
            lookup_rule->range(scratch.arena, eval.expr, r1u64(idx, idx+1), &expr, &expr_string, lookup_info.user_data);
            if(expr != &e_expr_nil)
            {
              if(!is_first)
              {
                String8 comma = str8_lit(", ");
                space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
                str8_list_push(arena, out, comma);
              }
              is_first = 0;
              E_Eval child_eval = e_eval_from_expr(scratch.arena, expr);
              space_taken += rd_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, root_expr, child_eval, out);
              if(space_taken > max_size && idx+1 < total_possible_child_count)
              {
                String8 ellipses = str8_lit(", ...");
                space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
                str8_list_push(arena, out, ellipses);
              }
            }
          }
        }
        else
        {
          String8 ellipses = str8_lit("...");
          str8_list_push(arena, out, ellipses);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
        }
        
        // rjf: closer (}, ])
        {
          str8_list_push(arena, out, closer_string);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, closer_string).x;
        }
      }break;
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return space_taken;
}

internal String8
rd_value_string_from_eval(Arena *arena, EV_StringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, E_Eval eval)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  rd_append_value_strings_from_eval(scratch.arena, flags, default_radix, font, font_size, max_size, 0, eval.expr, eval, &strs);
  String8 result = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Hover Eval

internal void
rd_set_hover_eval(Vec2F32 pos, String8 file_path, TxtPt pt, U64 vaddr, String8 string)
{
  RD_Cfg *window_cfg = rd_cfg_from_id(rd_regs()->window);
  RD_WindowState *ws = rd_window_state_from_cfg(window_cfg);
  if(ws->hover_eval_last_frame_idx+1 < rd_state->frame_index &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Left), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Middle), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Right), ui_key_zero()))
  {
    B32 is_new_string = !str8_match(ws->hover_eval_string, string, 0);
    if(is_new_string)
    {
      ws->hover_eval_first_frame_idx = ws->hover_eval_last_frame_idx = rd_state->frame_index;
      arena_clear(ws->hover_eval_arena);
      ws->hover_eval_string = push_str8_copy(ws->hover_eval_arena, string);
      ws->hover_eval_file_path = push_str8_copy(ws->hover_eval_arena, file_path);
      ws->hover_eval_file_pt = pt;
      ws->hover_eval_vaddr = vaddr;
      ws->hover_eval_focused = 0;
    }
    ws->hover_eval_spawn_pos = pos;
    ws->hover_eval_last_frame_idx = rd_state->frame_index;
  }
}

////////////////////////////////
//~ rjf: Lister Functions

internal void
rd_lister_item_chunk_list_push(Arena *arena, RD_ListerItemChunkList *list, U64 cap, RD_ListerItem *item)
{
  RD_ListerItemChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = push_array(arena, RD_ListerItemChunkNode, 1);
    SLLQueuePush(list->first, list->last, n);
    n->cap = cap;
    n->v = push_array_no_zero(arena, RD_ListerItem, n->cap);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&n->v[n->count], item);
  n->count += 1;
  list->total_count += 1;
}

internal RD_ListerItemArray
rd_lister_item_array_from_chunk_list(Arena *arena, RD_ListerItemChunkList *list)
{
  RD_ListerItemArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, RD_ListerItem, array.count);
  U64 idx = 0;
  for(RD_ListerItemChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(RD_ListerItem)*n->count);
    idx += n->count;
  }
  return array;
}

internal int
rd_lister_item_qsort_compare(RD_ListerItem *a, RD_ListerItem *b)
{
  int result = 0;
  if(a->group < b->group)
  {
    result = -1;
  }
  else if(a->group > b->group)
  {
    result = +1;
  }
  else if(a->display_name__matches.count > b->display_name__matches.count)
  {
    result = -1;
  }
  else if(a->display_name__matches.count < b->display_name__matches.count)
  {
    result = +1;
  }
  else if(a->description__matches.count > b->description__matches.count)
  {
    result = -1;
  }
  else if(a->description__matches.count < b->description__matches.count)
  {
    result = +1;
  }
  else if(a->kind_name__matches.count > b->kind_name__matches.count)
  {
    result = -1;
  }
  else if(a->kind_name__matches.count < b->kind_name__matches.count)
  {
    result = +1;
  }
  else if(a->string.size < b->string.size)
  {
    result = -1;
  }
  else if(a->string.size > b->string.size)
  {
    result = +1;
  }
  else
  {
    result = strncmp((char *)a->string.str, (char *)b->string.str, Min(a->string.size, b->string.size));
  }
  return result;
}

internal void
rd_lister_item_array_sort__in_place(RD_ListerItemArray *array)
{
  quick_sort(array->v, array->count, sizeof(array->v[0]), rd_lister_item_qsort_compare);
}

internal RD_ListerItemArray
rd_lister_item_array_from_regs_needle_cursor_off(Arena *arena, RD_Regs *regs, String8 needle, U64 cursor_off)
{
  Temp scratch = scratch_begin(&arena, 1);
  DI_Scope *di_scope = di_scope_open();
  RD_ListerFlags flags = regs->lister_flags;
  String8 needle_path = rd_lister_query_path_from_input_string_off(needle, cursor_off);
  RD_ListerItemChunkList item_list = {0};
  DI_KeyList dbgi_keys_list = d_push_active_dbgi_key_list(scratch.arena);
  DI_KeyArray dbgi_keys = di_key_array_from_list(scratch.arena, &dbgi_keys_list);
  
#if 0 // TODO(rjf): @cfg
  
  //////////////////////////
  //- rjf: determine all ctx filters
  //
  String8List ctx_filter_strings = {0};
  {
    switch(regs->reg_slot)
    {
      default:{}break;
      case RD_RegSlot_Cursor:
      {
        if(!txt_pt_match(regs->cursor, regs->mark))
        {
          str8_list_pushf(scratch.arena, &ctx_filter_strings, "$text_rng,");
        }
        else
        {
          str8_list_pushf(scratch.arena, &ctx_filter_strings, "$text_pt,");
        }
      }break;
      case RD_RegSlot_Cfg:
      {
        RD_Cfg *cfg = rd_cfg_from_id(regs->cfg);
        str8_list_pushf(scratch.arena, &ctx_filter_strings, "$%S,", cfg->string);
      }break;
      case RD_RegSlot_View:
      {
        RD_Cfg *view = rd_cfg_from_id(regs->view);
        str8_list_pushf(scratch.arena, &ctx_filter_strings, "$tab,");
        str8_list_pushf(scratch.arena, &ctx_filter_strings, "$%S,", view->string);
        String8 view_expr = rd_expr_from_cfg(view);
        String8 view_file_path = rd_file_path_from_eval_string(scratch.arena, view_expr);
        if(view_file_path.size != 0)
        {
          str8_list_pushf(scratch.arena, &ctx_filter_strings, "$file,");
        }
      } // fallthrough;
      case RD_RegSlot_Panel:
      {
        str8_list_pushf(scratch.arena, &ctx_filter_strings, "$panel,");
      }break;
    }
  }
  
  //////////////////////////
  //- rjf: grab rdis
  //
  U64 rdis_count = dbgi_keys.count;
  RDI_Parsed **rdis = push_array(scratch.arena, RDI_Parsed *, rdis_count);
  {
    for(U64 idx = 0; idx < rdis_count; idx += 1)
    {
      RDI_Parsed *rdi = di_rdi_from_key(di_scope, &dbgi_keys.v[idx], 0);
      RDI_TopLevelInfo *tli = rdi_element_from_name_idx(rdi, TopLevelInfo, 0);
      rdis[idx] = rdi;
    }
  }
  
  //- rjf: gather locals
  if(flags & RD_ListerFlag_Locals)
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
    U64 thread_rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, rd_regs()->unwind_count);
    CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
    CTRL_Entity *module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
    U64 thread_rip_voff = ctrl_voff_from_vaddr(module, thread_rip_vaddr);
    DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
    E_String2NumMap *locals_map = d_query_cached_locals_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
    E_String2NumMap *member_map = d_query_cached_member_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
    for(E_String2NumMapNode *n = locals_map->first; n != 0; n = n->order_next)
    {
      FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, n->string);
      if(display_name__matches.count == display_name__matches.needle_part_count)
      {
        rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                           .string                = n->string,
                                           .kind_name             = str8_lit("Local"),
                                           .display_name          = n->string,
                                           .display_name__matches = display_name__matches);
      }
    }
    for(E_String2NumMapNode *n = member_map->first; n != 0; n = n->order_next)
    {
      FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, n->string);
      if(display_name__matches.count == display_name__matches.needle_part_count)
      {
        rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                           .string                = n->string,
                                           .kind_name             = str8_lit("Local (Member)"),
                                           .display_name          = n->string,
                                           .display_name__matches = display_name__matches);
      }
    }
  }
  
  //- rjf: gather registers
  if(flags & RD_ListerFlag_Registers)
  {
    CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
    Arch arch = thread->arch;
    U64 reg_names_count = regs_reg_code_count_from_arch(arch);
    U64 alias_names_count = regs_alias_code_count_from_arch(arch);
    String8 *reg_names = regs_reg_code_string_table_from_arch(arch);
    String8 *alias_names = regs_alias_code_string_table_from_arch(arch);
    for(U64 idx = 0; idx < reg_names_count; idx += 1)
    {
      if(reg_names[idx].size != 0)
      {
        String8 display_name = reg_names[idx];
        FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
        if(display_name__matches.count == display_name__matches.needle_part_count)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .string                = display_name,
                                             .kind_name             = str8_lit("Register"),
                                             .display_name          = display_name,
                                             .display_name__matches = display_name__matches);
        }
      }
    }
    for(U64 idx = 0; idx < alias_names_count; idx += 1)
    {
      if(alias_names[idx].size != 0)
      {
        String8 display_name = alias_names[idx];
        FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
        if(display_name__matches.count == display_name__matches.needle_part_count)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .string                = display_name,
                                             .kind_name             = str8_lit("Reg. Alias"),
                                             .display_name          = display_name,
                                             .display_name__matches = display_name__matches);
        }
      }
    }
  }
  
  //- rjf: gather view rules
  if(flags & RD_ListerFlag_ViewRules)
  {
    for(U64 slot_idx = 0; slot_idx < d_state->view_rule_spec_table_size; slot_idx += 1)
    {
      for(D_ViewRuleSpec *spec = d_state->view_rule_spec_table[slot_idx]; spec != 0 && spec != &d_nil_core_view_rule_spec; spec = spec->hash_next)
      {
        String8 display_name = spec->info.string;
        FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
        String8 description = spec->info.description;
        FuzzyMatchRangeList description__matches = fuzzy_match_find(arena, needle, description);
        if(display_name__matches.count == display_name__matches.needle_part_count ||
           description__matches.count == description__matches.needle_part_count)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .string                = display_name,
                                             .kind_name             = str8_lit("View Rule"),
                                             .display_name          = display_name,
                                             .display_name__matches = display_name__matches,
                                             .description           = description,
                                             .description__matches  = description__matches);
        }
      }
    }
  }
  
  //- rjf: gather members
  if(flags & RD_ListerFlag_Members)
  {
    // TODO(rjf)
  }
  
  //- rjf: gather globals
  if(flags & RD_ListerFlag_Globals && needle.size != 0)
  {
    U128 search_key = {d_hash_from_string(str8_lit("autocomp_globals_search_key")), d_hash_from_string(str8_lit("autocomp_globals_search_key"))};
    DI_SearchParams search_params =
    {
      RDI_SectionKind_GlobalVariables,
      dbgi_keys,
    };
    B32 is_stale = 0;
    DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, needle, 0, &is_stale);
    for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
    {
      // rjf: skip bad elements
      if(items.v[idx].dbgi_idx >= rdis_count)
      {
        continue;
      }
      
      // rjf: unpack info
      RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
      String8 display_name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
      FuzzyMatchRangeList display_name__matches = items.v[idx].match_ranges;
      
      // rjf: push item
      rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                         .string                = display_name,
                                         .kind_name             = str8_lit("Global"),
                                         .display_name          = display_name,
                                         .display_name__matches = display_name__matches,
                                         .group                 = 1);
    }
  }
  
  //- rjf: gather thread locals
  if(flags & RD_ListerFlag_ThreadLocals && needle.size != 0)
  {
    U128 search_key = {d_hash_from_string(str8_lit("autocomp_tvars_dis_key")), d_hash_from_string(str8_lit("autocomp_tvars_dis_key"))};
    DI_SearchParams search_params =
    {
      RDI_SectionKind_ThreadVariables,
      dbgi_keys,
    };
    B32 is_stale = 0;
    DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, needle, 0, &is_stale);
    for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
    {
      // rjf: skip bad elements
      if(items.v[idx].dbgi_idx >= rdis_count)
      {
        continue;
      }
      
      // rjf: unpack info
      RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
      String8 display_name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
      FuzzyMatchRangeList display_name__matches = items.v[idx].match_ranges;
      
      // rjf: push item
      rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                         .string                = display_name,
                                         .kind_name             = str8_lit("Thread Local"),
                                         .display_name          = display_name,
                                         .display_name__matches = display_name__matches,
                                         .group                 = 1);
    }
  }
  
  //- rjf: gather procedures
  if(flags & RD_ListerFlag_Procedures && needle.size != 0)
  {
    U128 search_key = {d_hash_from_string(str8_lit("autocomp_procedures_search_key")), d_hash_from_string(str8_lit("autocomp_procedures_search_key"))};
    DI_SearchParams search_params =
    {
      RDI_SectionKind_Procedures,
      dbgi_keys,
    };
    B32 is_stale = 0;
    DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, needle, 0, &is_stale);
    for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
    {
      // rjf: skip bad elements
      if(items.v[idx].dbgi_idx >= rdis_count)
      {
        continue;
      }
      
      // rjf: unpack info
      RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
      String8 display_name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
      FuzzyMatchRangeList display_name__matches = items.v[idx].match_ranges;
      
      // rjf: push item
      rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                         .string                = display_name,
                                         .kind_name             = str8_lit("Procedure"),
                                         .display_name          = display_name,
                                         .display_name__matches = display_name__matches,
                                         .group                 = 1);
    }
  }
  
  //- rjf: gather types
  if(flags & RD_ListerFlag_Types && needle.size != 0)
  {
    U128 search_key = {d_hash_from_string(str8_lit("autocomp_types_search_key")), d_hash_from_string(str8_lit("autocomp_types_search_key"))};
    DI_SearchParams search_params =
    {
      RDI_SectionKind_UDTs,
      dbgi_keys,
    };
    B32 is_stale = 0;
    DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, needle, 0, &is_stale);
    for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
    {
      // rjf: skip bad elements
      if(items.v[idx].dbgi_idx >= rdis_count)
      {
        continue;
      }
      
      // rjf: unpack info
      RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
      String8 display_name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
      FuzzyMatchRangeList display_name__matches = items.v[idx].match_ranges;
      
      // rjf: push item
      rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                         .string                = display_name,
                                         .kind_name             = str8_lit("Type"),
                                         .display_name          = display_name,
                                         .display_name__matches = display_name__matches,
                                         .group                 = 1);
    }
  }
  
  //- rjf: gather languages
  if(flags & RD_ListerFlag_Languages)
  {
    for EachNonZeroEnumVal(TXT_LangKind, lang)
    {
      String8 display_name = txt_extension_from_lang_kind(lang);
      if(display_name.size != 0)
      {
        FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
        if(display_name__matches.count == display_name__matches.needle_part_count)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .string                = display_name,
                                             .kind_name             = str8_lit("Language"),
                                             .display_name          = display_name,
                                             .display_name__matches = display_name__matches);
        }
      }
    }
  }
  
  //- rjf: gather architectures
  if(flags & RD_ListerFlag_Architectures)
  {
    for EachNonZeroEnumVal(Arch, arch)
    {
      String8 display_name = string_from_arch(arch);
      FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
      if(display_name__matches.count == display_name__matches.needle_part_count)
      {
        rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                           .string                = display_name,
                                           .kind_name             = str8_lit("Architecture"),
                                           .display_name          = display_name,
                                           .display_name__matches = display_name__matches);
      }
    }
  }
  
  //- rjf: gather tex2dformats
  if(flags & RD_ListerFlag_Tex2DFormats)
  {
    for EachEnumVal(R_Tex2DFormat, fmt)
    {
      String8 display_name = lower_from_str8(scratch.arena, r_tex2d_format_display_string_table[fmt]);
      FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
      if(display_name__matches.count == display_name__matches.needle_part_count)
      {
        rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                           .string                = display_name,
                                           .kind_name             = str8_lit("2D Texture Format"),
                                           .display_name          = display_name,
                                           .display_name__matches = display_name__matches);
      }
    }
  }
  
  //- rjf: gather view rule params
#if 0 // TODO(rjf): @cfg
  if(flags & RD_ListerFlag_ViewRuleParams)
  {
    for(String8Node *n = strings.first; n != 0; n = n->next)
    {
      String8 display_name = n->string;
      FuzzyMatchRangeList display_name__matches = fuzzy_match_find(arena, needle, display_name);
      if(display_name__matches.count == display_name__matches.needle_part_count)
      {
        rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                           .string                = display_name,
                                           .kind_name             = str8_lit("View Rule Parameter"),
                                           .display_name          = display_name,
                                           .display_name__matches = display_name__matches);
      }
    }
  }
#endif
  
  //- rjf: gather files
  if(flags & RD_ListerFlag_Files)
  {
    // rjf: find containing directory in needle_path
    String8 dir_str_in_input = {0};
    for(U64 i = 0; i < needle_path.size; i += 1)
    {
      String8 substr1 = str8_substr(needle_path, r1u64(i, i+1));
      String8 substr2 = str8_substr(needle_path, r1u64(i, i+2));
      String8 substr3 = str8_substr(needle_path, r1u64(i, i+3));
      if(str8_match(substr1, str8_lit("/"), StringMatchFlag_SlashInsensitive))
      {
        dir_str_in_input = str8_substr(needle_path, r1u64(i, needle_path.size));
      }
      else if(i != 0 && str8_match(substr2, str8_lit(":/"), StringMatchFlag_SlashInsensitive))
      {
        dir_str_in_input = str8_substr(needle_path, r1u64(i-1, needle_path.size));
      }
      else if(str8_match(substr2, str8_lit("./"), StringMatchFlag_SlashInsensitive))
      {
        dir_str_in_input = str8_substr(needle_path, r1u64(i, needle_path.size));
      }
      else if(str8_match(substr3, str8_lit("../"), StringMatchFlag_SlashInsensitive))
      {
        dir_str_in_input = str8_substr(needle_path, r1u64(i, needle_path.size));
      }
      if(dir_str_in_input.size != 0)
      {
        break;
      }
    }
    
    // rjf: use needle_path string to form various parts of search space
    String8 prefix = {0};
    String8 path = {0};
    String8 search = {0};
    if(dir_str_in_input.size != 0)
    {
      String8 dir = dir_str_in_input;
      U64 one_past_last_slash = dir.size;
      for(U64 i = 0; i < dir_str_in_input.size; i += 1)
      {
        if(dir_str_in_input.str[i] == '/' || dir_str_in_input.str[i] == '\\')
        {
          one_past_last_slash = i+1;
        }
      }
      dir.size = one_past_last_slash;
      path = dir;
      search = str8_substr(dir_str_in_input, r1u64(one_past_last_slash, dir_str_in_input.size));
      prefix = str8_substr(needle_path, r1u64(0, path.str - needle_path.str));
    }
    
    // rjf: get current files, filtered
    if(dir_str_in_input.size != 0)
    {
      B32 allow_dirs = 1;
      OS_FileIter *it = os_file_iter_begin(scratch.arena, path, 0);
      for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, it, &info);)
      {
        FuzzyMatchRangeList match_ranges = fuzzy_match_find(arena, search, info.name);
        B32 fits_search = (match_ranges.count == match_ranges.needle_part_count);
        B32 fits_dir_only = (allow_dirs || !(info.props.flags & FilePropertyFlag_IsFolder));
        if(fits_search && fits_dir_only)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .string                = info.name,
                                             .kind_name             = info.props.flags & FilePropertyFlag_IsFolder ? str8_lit("Folder") : str8_lit("File"),
                                             .display_name          = info.name,
                                             .icon_kind             = info.props.flags & FilePropertyFlag_IsFolder ? RD_IconKind_FolderClosedFilled : RD_IconKind_FileOutline,
                                             .display_name__matches = match_ranges,
                                             .flags                 = RD_ListerItemFlag_IsNonCode);
        }
      }
      os_file_iter_end(it);
    }
  }
  
  //- rjf: gather commands
  if(flags & RD_ListerFlag_Commands)
  {
    for EachNonZeroEnumVal(RD_CmdKind, k)
    {
      RD_CmdKindInfo *info = &rd_cmd_kind_info_table[k];
      B32 included_by_filter_string = 0;
      for(String8Node *n = ctx_filter_strings.first; n != 0; n = n->next)
      {
        B32 n_match = (str8_find_needle(info->ctx_filter, 0, n->string, 0) < info->ctx_filter.size);
        if(n_match)
        {
          included_by_filter_string = 1;
          break;
        }
      }
      if(ctx_filter_strings.node_count == 0 || included_by_filter_string)
      {
        String8 cmd_display_name = info->display_name;
        String8 cmd_desc = info->description;
        String8 cmd_tags = info->search_tags;
        FuzzyMatchRangeList name_matches = fuzzy_match_find(arena, needle, cmd_display_name);
        FuzzyMatchRangeList desc_matches = fuzzy_match_find(arena, needle, cmd_desc);
        FuzzyMatchRangeList tags_matches = fuzzy_match_find(arena, needle, cmd_tags);
        if(name_matches.count == name_matches.needle_part_count ||
           desc_matches.count == name_matches.needle_part_count ||
           tags_matches.count > 0 ||
           name_matches.needle_part_count == 0)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .icon_kind             = info->icon_kind,
                                             .string                = info->string,
                                             .kind_name             = str8_lit("Command"),
                                             .display_name          = cmd_display_name,
                                             .display_name__matches = name_matches,
                                             .description           = cmd_desc,
                                             .description__matches  = desc_matches,
                                             .flags                 = RD_ListerItemFlag_IsNonCode|RD_ListerItemFlag_Bindings);
        }
      }
    }
  }
  
  //- rjf: gather settings
  if(flags & RD_ListerFlag_Settings)
  {
#if 0 // TODO(rjf): @cfg
    String8List schema_strings = {0};
    
    // rjf: push schema for view
    {
      RD_Cfg *view = rd_cfg_from_id(regs->view);
      String8 view_name = view->string;
      RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(view_name);
      str8_list_push(scratch.arena, &schema_strings, view_rule_info->params_schema);
    }
    
    // rjf: for each schema, gather parameters
    for(String8Node *n = schema_strings.first; n != 0; n = n->next)
    {
      MD_Node *schema = md_tree_from_string(scratch.arena, n->string)->first;
      for MD_EachNode(param, schema->first)
      {
        RD_VocabInfo *param_vocab_info = rd_vocab_info_from_code_name(param->string);
        String8 name = param_vocab_info->display_name;
        RD_IconKind icon_kind = param_vocab_info->icon_kind;
        FuzzyMatchRangeList name_matches = fuzzy_match_find(arena, needle, name);
        if(name_matches.count == name_matches.needle_part_count)
        {
          rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                             .icon_kind             = icon_kind,
                                             .string                = param->string,
                                             .kind_name             = str8_lit("Setting"),
                                             .display_name          = name,
                                             .display_name__matches = name_matches,
                                             .flags                 = RD_ListerItemFlag_IsNonCode);
        }
      }
    }
#endif
  }
  
  //- rjf: gather system processes
  if(flags & RD_ListerFlag_SystemProcesses)
  {
    U32 this_process_pid = os_get_process_info()->pid;
    DMN_ProcessIter iter = {0};
    dmn_process_iter_begin(&iter);
    for(DMN_ProcessInfo info = {0}; dmn_process_iter_next(scratch.arena, &iter, &info);)
    {
      if(info.pid == this_process_pid)
      {
        continue;
      }
      String8 name = push_str8f(scratch.arena, "%S (PID: %i)", info.name, info.pid);
      FuzzyMatchRangeList name_matches = fuzzy_match_find(arena, needle, name);
      if(name_matches.count == name_matches.needle_part_count)
      {
        rd_lister_item_chunk_list_push_new(scratch.arena, &item_list, 256,
                                           .icon_kind             = RD_IconKind_Threads,
                                           .string                = name,
                                           .kind_name             = str8_lit("System Process"),
                                           .display_name          = name,
                                           .display_name__matches = name_matches,
                                           .flags                 = RD_ListerItemFlag_IsNonCode);
      }
    }
    dmn_process_iter_end(&iter);
  }
  
  //- rjf: lister item list -> sorted array
  RD_ListerItemArray item_array = rd_lister_item_array_from_chunk_list(arena, &item_list);
  rd_lister_item_array_sort__in_place(&item_array);
#endif
  RD_ListerItemArray item_array = {0};
  di_scope_close(di_scope);
  scratch_end(scratch);
  return item_array;
}

internal U64
rd_hash_from_lister_item(RD_ListerItem *item)
{
  U64 item_hash = 5381;
  item_hash = d_hash_from_seed_string(item_hash, item->string);
  item_hash = d_hash_from_seed_string(item_hash, item->kind_name);
  item_hash = d_hash_from_seed_string(item_hash, str8_struct(&item->group));
  return item_hash;
}

internal String8
rd_lister_query_word_from_input_string_off(String8 input, U64 cursor_off)
{
  U64 word_start_off = 0;
  for(U64 off = 0; off < input.size && off < cursor_off; off += 1)
  {
    if(!char_is_alpha(input.str[off]) && !char_is_digit(input.str[off], 10) && input.str[off] != '_')
    {
      word_start_off = off+1;
    }
  }
  String8 query = str8_skip(str8_prefix(input, cursor_off), word_start_off);
  return query;
}

internal String8
rd_lister_query_path_from_input_string_off(String8 input, U64 cursor_off)
{
  // rjf: find start of path
  U64 path_start_off = 0;
  {
    B32 single_quoted = 0;
    B32 double_quoted = 0;
    for(U64 off = 0; off < input.size && off < cursor_off; off += 1)
    {
      if(input.str[off] == '\'')
      {
        single_quoted ^= 1;
      }
      if(input.str[off] == '\"')
      {
        double_quoted ^= 1;
      }
      if(char_is_space(input.str[off]) && !single_quoted && !double_quoted)
      {
        path_start_off = off+1;
      }
    }
  }
  
  // rjf: form path
  String8 path = str8_skip(str8_prefix(input, cursor_off), path_start_off);
  if(path.size >= 1 && path.str[0] == '"')  { path = str8_skip(path, 1); }
  if(path.size >= 1 && path.str[0] == '\'') { path = str8_skip(path, 1); }
  if(path.size >= 1 && path.str[path.size-1] == '"')  { path = str8_chop(path, 1); }
  if(path.size >= 1 && path.str[path.size-1] == '\'') { path = str8_chop(path, 1); }
  return path;
}

#if 0 // TODO(rjf): @cfg_lister

internal RD_ListerParams
rd_view_rule_lister_params_from_input_cursor(Arena *arena, String8 string, U64 cursor_off)
{
  RD_ListerParams params = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    //- rjf: do partial parse of input
    MD_TokenizeResult input_tokenize = md_tokenize_from_text(scratch.arena, string);
    
    //- rjf: find descension steps to cursor
    typedef struct DescendStep DescendStep;
    struct DescendStep
    {
      DescendStep *next;
      DescendStep *prev;
      String8 string;
    };
    DescendStep *first_step = 0;
    DescendStep *last_step = 0;
    DescendStep *free_step = 0;
    S32 paren_nest = 0;
    S32 colon_nest = 0;
    String8 last_step_string = {0};
    for(U64 idx = 0; idx < input_tokenize.tokens.count; idx += 1)
    {
      MD_Token *token = &input_tokenize.tokens.v[idx];
      if(token->range.min >= cursor_off)
      {
        break;
      }
      String8 token_string = str8_substr(string, token->range);
      if(token->flags & (MD_TokenFlag_Identifier|MD_TokenFlag_StringLiteral))
      {
        last_step_string = token_string;
      }
      if(str8_match(token_string, str8_lit("("), 0) || str8_match(token_string, str8_lit("["), 0) || str8_match(token_string, str8_lit("{"), 0))
      {
        paren_nest += 1;
      }
      if(str8_match(token_string, str8_lit(")"), 0) || str8_match(token_string, str8_lit("]"), 0) || str8_match(token_string, str8_lit("}"), 0))
      {
        paren_nest -= 1;
        for(;colon_nest > paren_nest; colon_nest -= 1)
        {
          if(last_step != 0)
          {
            DescendStep *step = last_step;
            DLLRemove(first_step, last_step, step);
            SLLStackPush(free_step, step);
          }
        }
        if(paren_nest == 0 && last_step != 0)
        {
          DescendStep *step = last_step;
          DLLRemove(first_step, last_step, step);
          SLLStackPush(free_step, step);
        }
      }
      if(str8_match(token_string, str8_lit(":"), 0))
      {
        colon_nest += 1;
        if(last_step_string.size != 0)
        {
          DescendStep *step = free_step;
          if(step != 0)
          {
            SLLStackPop(free_step);
            MemoryZeroStruct(step);
          }
          else
          {
            step = push_array(scratch.arena, DescendStep, 1);
          }
          step->string = last_step_string;
          DLLPushBack(first_step, last_step, step);
        }
      }
      if(str8_match(token_string, str8_lit(";"), 0) || str8_match(token_string, str8_lit(","), 0))
      {
        for(;colon_nest > paren_nest; colon_nest -= 1)
        {
          if(last_step != 0)
          {
            DescendStep *step = last_step;
            DLLRemove(first_step, last_step, step);
            SLLStackPush(free_step, step);
          }
        }
      }
    }
    
    //- rjf: map view rule root to spec
    D_ViewRuleSpec *spec = d_view_rule_spec_from_string(first_step ? first_step->string : str8_zero());
    
    //- rjf: do parse of schema
    MD_TokenizeResult schema_tokenize = md_tokenize_from_text(scratch.arena, spec->info.schema);
    MD_ParseResult schema_parse = md_parse_from_text_tokens(scratch.arena, str8_zero(), spec->info.schema, schema_tokenize.tokens);
    MD_Node *schema_rule_root = md_child_from_string(schema_parse.root, str8_lit("x"), 0);
    
    //- rjf: follow schema according to descend steps, gather flags from schema node matching cursor descension steps
    if(first_step != 0)
    {
      MD_Node *schema_node = schema_rule_root;
      for(DescendStep *step = first_step->next;;)
      {
        if(step == 0)
        {
          for MD_EachNode(child, schema_node->first)
          {
            if(0){}
            else if(str8_match(child->string, str8_lit("expr"),           StringMatchFlag_CaseInsensitive)) {params.flags |= RD_ListerFlag_Locals;}
            else if(str8_match(child->string, str8_lit("member"),         StringMatchFlag_CaseInsensitive)) {params.flags |= RD_ListerFlag_Members;}
            else if(str8_match(child->string, str8_lit("lang"),           StringMatchFlag_CaseInsensitive)) {params.flags |= RD_ListerFlag_Languages;}
            else if(str8_match(child->string, str8_lit("arch"),           StringMatchFlag_CaseInsensitive)) {params.flags |= RD_ListerFlag_Architectures;}
            else if(str8_match(child->string, str8_lit("tex2dformat"),    StringMatchFlag_CaseInsensitive)) {params.flags |= RD_ListerFlag_Tex2DFormats;}
            else if(child->flags & (MD_NodeFlag_StringSingleQuote|MD_NodeFlag_StringDoubleQuote|MD_NodeFlag_StringTick))
            {
              str8_list_push(arena, &params.strings, child->string);
              params.flags |= RD_ListerFlag_ViewRuleParams;
            }
          }
          break;
        }
        if(step != 0)
        {
          MD_Node *next_node = md_child_from_string(schema_node, step->string, StringMatchFlag_CaseInsensitive);
          schema_node = next_node;
          step = step->next;
        }
        else
        {
          schema_node = schema_node->first;
        }
      }
    }
    
    scratch_end(scratch);
  }
  return params;
}

internal void
rd_set_autocomp_lister_query_(RD_ListerParams *params)
{
  RD_Cfg *window_cfg = rd_cfg_from_id(rd_regs()->window);
  RD_WindowState *ws = rd_window_state_from_cfg(window_cfg);
  arena_clear(ws->lister_arena);
  ws->lister_regs = rd_regs_copy(ws->lister_arena, rd_regs());
  MemoryCopyStruct(&ws->lister_params, params);
  ws->lister_params.strings = str8_list_copy(ws->lister_arena, &ws->lister_params.strings);
  if(!(params->flags & RD_ListerFlag_LineEdit))
  {
    ws->lister_input_size = Min(params->input.size, sizeof(ws->lister_input_buffer));
    MemoryCopy(ws->lister_input_buffer, params->input.str, ws->lister_input_size);
  }
  ws->lister_last_frame_idx = rd_state->frame_index;
}

#endif

internal void
rd_set_autocomp_lister_query_(RD_Regs *regs)
{
  RD_Cfg *window_cfg = rd_cfg_from_id(regs->window);
  RD_WindowState *ws = rd_window_state_from_cfg(window_cfg);
  if(ws->autocomp_lister == 0)
  {
    Arena *arena = arena_alloc();
    ws->autocomp_lister = push_array(arena, RD_Lister, 1);
    ws->autocomp_lister->arena = arena;
  }
  Arena *arena = ws->autocomp_lister->arena;
  arena_clear(arena);
  ws->autocomp_lister = push_array(arena, RD_Lister, 1);
  ws->autocomp_lister->arena = arena;
  ws->autocomp_lister->regs = rd_regs_copy(arena, regs);
  ws->autocomp_lister->input_string_size = Min(regs->string.size, sizeof(ws->autocomp_lister->input_buffer));
  MemoryCopy(ws->autocomp_lister->input_buffer, regs->string.str, ws->autocomp_lister->input_string_size);
  ws->autocomp_lister->input_cursor = regs->cursor;
  ws->autocomp_lister->input_mark = regs->mark;
  ws->autocomp_lister_last_frame_idx = rd_state->frame_index;
}

////////////////////////////////
//~ rjf: Search Strings

internal void
rd_set_search_string(String8 string)
{
  arena_clear(rd_state->string_search_arena);
  rd_state->string_search_string = push_str8_copy(rd_state->string_search_arena, string);
}

internal String8
rd_push_search_string(Arena *arena)
{
  String8 result = push_str8_copy(arena, rd_state->string_search_string);
  return result;
}

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: colors

internal Vec4F32
rd_rgba_from_theme_color(RD_ThemeColor color)
{
  return rd_state->theme->colors[color];
}

internal RD_ThemeColor
rd_theme_color_from_txt_token_kind(TXT_TokenKind kind)
{
  RD_ThemeColor color = RD_ThemeColor_CodeDefault;
  switch(kind)
  {
    default:break;
    case TXT_TokenKind_Keyword:{color = RD_ThemeColor_CodeKeyword;}break;
    case TXT_TokenKind_Numeric:{color = RD_ThemeColor_CodeNumeric;}break;
    case TXT_TokenKind_String: {color = RD_ThemeColor_CodeString;}break;
    case TXT_TokenKind_Meta:   {color = RD_ThemeColor_CodeMeta;}break;
    case TXT_TokenKind_Comment:{color = RD_ThemeColor_CodeComment;}break;
    case TXT_TokenKind_Symbol: {color = RD_ThemeColor_CodeDelimiterOperator;}break;
  }
  return color;
}

internal RD_ThemeColor
rd_theme_color_from_txt_token_kind_lookup_string(TXT_TokenKind kind, String8 string)
{
  RD_ThemeColor color = RD_ThemeColor_CodeDefault;
  if(kind == TXT_TokenKind_Identifier || kind == TXT_TokenKind_Keyword)
  {
    CTRL_Entity *module = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->module);
    DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
    B32 mapped = 0;
    
    // rjf: try to map as local
    if(!mapped && kind == TXT_TokenKind_Identifier)
    {
      U64 local_num = e_num_from_string(e_parse_state->ctx->locals_map, string);
      if(local_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeLocal;
      }
    }
    
    // rjf: try to map as member
    if(!mapped && kind == TXT_TokenKind_Identifier)
    {
      U64 member_num = e_num_from_string(e_parse_state->ctx->member_map, string);
      if(member_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeLocal;
      }
    }
    
    // rjf: try to map as register
    if(!mapped)
    {
      U64 reg_num = e_num_from_string(e_parse_state->ctx->regs_map, string);
      if(reg_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeRegister;
      }
    }
    
    // rjf: try to map as register alias
    if(!mapped)
    {
      U64 alias_num = e_num_from_string(e_parse_state->ctx->reg_alias_map, string);
      if(alias_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeRegister;
      }
    }
    
    // rjf: try to map using asynchronous matching system
    if(!mapped && kind == TXT_TokenKind_Identifier)
    {
      RDI_SectionKind section_kind = di_match_store_section_kind_from_name(rd_state->match_store, string, 0);
      mapped = 1;
      switch(section_kind)
      {
        default:{mapped = 0;}break;
        case RDI_SectionKind_Procedures:
        case RDI_SectionKind_GlobalVariables:
        case RDI_SectionKind_ThreadVariables:
        {
          color = RD_ThemeColor_CodeSymbol;
        }break;
        case RDI_SectionKind_TypeNodes:
        {
          color = RD_ThemeColor_CodeType;
        }break;
      }
    }
    
#if 0
    // rjf: try to map as symbol
    if(!mapped && kind == TXT_TokenKind_Identifier)
    {
      U64 voff = d_voff_from_dbgi_key_symbol_name(&dbgi_key, string);
      if(voff != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeSymbol;
      }
    }
    
    // rjf: try to map as type
    if(!mapped && kind == TXT_TokenKind_Identifier)
    {
      U64 type_num = d_type_num_from_dbgi_key_name(&dbgi_key, string);
      if(type_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeType;
      }
    }
#endif
  }
  return color;
}

//- rjf: code -> palette

internal UI_Palette *
rd_palette_from_code(RD_PaletteCode code)
{
  RD_Cfg *wcfg = rd_cfg_from_id(rd_regs()->window);
  RD_WindowState *ws = rd_window_state_from_cfg(wcfg);
  UI_Palette *result = &ws->cfg_palettes[code];
  return result;
}

//- rjf: fonts/sizes

internal FNT_Tag
rd_font_from_slot(RD_FontSlot slot)
{
  // rjf: determine key name for this font slot
  String8 key = {0};
  switch(slot)
  {
    default:{}break;
    case RD_FontSlot_Main:{key = str8_lit("main_font");}break;
    case RD_FontSlot_Code:{key = str8_lit("code_font");}break;
  }
  
  // rjf: determine font name
  String8 font_name = {0};
  if(key.size != 0)
  {
    RD_Cfg *seed_cfg = &rd_nil_cfg;
    if(seed_cfg == &rd_nil_cfg) { seed_cfg = rd_cfg_from_id(rd_regs()->view); }
    if(seed_cfg == &rd_nil_cfg) { seed_cfg = rd_cfg_from_id(rd_regs()->panel); }
    if(seed_cfg == &rd_nil_cfg) { seed_cfg = rd_cfg_from_id(rd_regs()->window); }
    for(RD_Cfg *cfg = seed_cfg; cfg != &rd_nil_cfg; cfg = cfg->parent)
    {
      RD_Cfg *font_root = rd_cfg_child_from_string(cfg, key);
      if(font_root != &rd_nil_cfg)
      {
        font_name = font_root->first->string;
        break;
      }
    }
  }
  
  // rjf: map name -> tag
  FNT_Tag result = {0};
  if(font_name.size == 0)
  {
    switch(slot)
    {
      default:
      case RD_FontSlot_Main: {result = fnt_tag_from_static_data_string(&rd_default_main_font_bytes);}break;
      case RD_FontSlot_Code: {result = fnt_tag_from_static_data_string(&rd_default_code_font_bytes);}break;
      case RD_FontSlot_Icons:{result = fnt_tag_from_static_data_string(&rd_icon_font_bytes);}break;
    }
  }
  else
  {
    // TODO(rjf): need to handle "system font names" here.
    result = fnt_tag_from_path(font_name);
  }
  return result;
}

internal F32
rd_font_size_from_slot(RD_FontSlot slot)
{
  F32 result = 11.f;
  
  // rjf: determine config key based on slot
  String8 key = {0};
  switch(slot)
  {
    default:{}break;
    case RD_FontSlot_Icons:
    case RD_FontSlot_Main:{key = str8_lit("main_font_size");}break;
    case RD_FontSlot_Code:{key = str8_lit("code_font_size");}break;
  }
  
  // rjf: given key, find setting string
  String8 setting_string = rd_setting_from_name(key);
  
  // rjf: if found, map setting string -> f64; otherwise use the window's monitor's DPI
  // based on some default size.
  if(setting_string.size)
  {
    result = (F32)f64_from_str8(setting_string);
  }
  else
  {
    RD_Cfg *window_cfg = rd_cfg_from_id(rd_regs()->window);
    RD_WindowState *ws = rd_window_state_from_cfg(window_cfg);
    F32 dpi = os_dpi_from_window(ws->os);
    result = 11.f * (dpi / 96.f);
  }
  
  return result;
}

internal FNT_RasterFlags
rd_raster_flags_from_slot(RD_FontSlot slot)
{
  FNT_RasterFlags flags = FNT_RasterFlag_Smooth|FNT_RasterFlag_Hinted;
  switch(slot)
  {
    default:{}break;
    case RD_FontSlot_Icons:{flags = FNT_RasterFlag_Smooth;}break;
    case RD_FontSlot_Main: {flags = (rd_setting_b32_from_name(str8_lit("smooth_main_text"))*FNT_RasterFlag_Smooth)|(rd_setting_b32_from_name(str8_lit("hint_main_text"))*FNT_RasterFlag_Hinted);}break;
    case RD_FontSlot_Code: {flags = (rd_setting_b32_from_name(str8_lit("smooth_code_text"))*FNT_RasterFlag_Smooth)|(rd_setting_b32_from_name(str8_lit("hint_code_text"))*FNT_RasterFlag_Hinted);}break;
  }
  return flags;
}

////////////////////////////////
//~ rjf: Process Control Info Stringification

internal String8
rd_string_from_exception_code(U32 code)
{
  String8 string = {0};
  for EachNonZeroEnumVal(CTRL_ExceptionCodeKind, k)
  {
    if(code == ctrl_exception_code_kind_code_table[k])
    {
      string = ctrl_exception_code_kind_display_string_table[k];
      break;
    }
  }
  return string;
}

internal DR_FStrList
rd_stop_explanation_fstrs_from_ctrl_event(Arena *arena, CTRL_Event *event)
{
  CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, event->entity);
  DR_FStrList thread_fstrs = rd_title_fstrs_from_ctrl_entity(arena, thread, ui_top_palette()->text, ui_top_font_size(), 0);
  DR_FStrList fstrs = {0};
  DR_FStrParams params = {ui_top_font(), ui_top_text_raster_flags(), ui_top_palette()->text, ui_top_font_size()};
  switch(event->cause)
  {
    default:{}break;
    
    //- rjf: finished operation; if active thread, completed thread, otherwise we're just stopped
    case CTRL_EventCause_Finished:
    {
      if(thread != &ctrl_entity_nil)
      {
        dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" completed step"));
      }
      else
      {
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("Stopped"));
      }
    }break;
    
    //- rjf: stopped at entry point
    case CTRL_EventCause_EntryPoint:
    {
      if(thread != &ctrl_entity_nil)
      {
        dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" stopped at entry point"));
      }
      else
      {
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("Stopped at entry point"));
      }
    }break;
    
    //- rjf: user breakpoint
    case CTRL_EventCause_UserBreakpoint:
    {
      if(thread != &ctrl_entity_nil)
      {
        dr_fstrs_push_new(arena, &fstrs, &params, rd_icon_kind_text_table[RD_IconKind_CircleFilled], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons));
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
        dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit a breakpoint"));
      }
    }break;
    
    //- rjf: exception
    case CTRL_EventCause_InterruptedByException:
    {
      if(thread != &ctrl_entity_nil)
      {
        dr_fstrs_push_new(arena, &fstrs, &params, rd_icon_kind_text_table[RD_IconKind_WarningBig], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons));
        switch(event->exception_kind)
        {
          default:
          {
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
            dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit an exception - "));
            String8 exception_code_string = str8_from_u64(arena, event->exception_code, 16, 0, 0);
            String8 exception_explanation_string = rd_string_from_exception_code(event->exception_code);
            String8 exception_info_string = push_str8f(arena, "%S%s%S%s",
                                                       exception_code_string,
                                                       exception_explanation_string.size != 0 ? " (" : "",
                                                       exception_explanation_string,
                                                       exception_explanation_string.size != 0 ? ")" : "");
            dr_fstrs_push_new(arena, &fstrs, &params, exception_info_string);
          }break;
          case CTRL_ExceptionKind_CppThrow:
          {
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
            dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit a C++ exception - "));
            String8 exception_code_string = str8_from_u64(arena, event->exception_code, 16, 0, 0);
            dr_fstrs_push_new(arena, &fstrs, &params, exception_code_string);
          }break;
          case CTRL_ExceptionKind_MemoryRead:
          {
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
            dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit an exception - "));
            String8 exception_code_string = str8_from_u64(arena, event->exception_code, 16, 0, 0);
            String8 exception_info_string = push_str8f(arena, "%S (Access violation reading 0x%I64x)", exception_code_string, event->vaddr_rng.min);
            dr_fstrs_push_new(arena, &fstrs, &params, exception_info_string);
          }break;
          case CTRL_ExceptionKind_MemoryWrite:
          {
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
            dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit an exception - "));
            String8 exception_code_string = str8_from_u64(arena, event->exception_code, 16, 0, 0);
            String8 exception_info_string = push_str8f(arena, "%S (Access violation writing 0x%I64x)", exception_code_string, event->vaddr_rng.min);
            dr_fstrs_push_new(arena, &fstrs, &params, exception_info_string);
          }break;
          case CTRL_ExceptionKind_MemoryExecute:
          {
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
            dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
            dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit an exception - "));
            String8 exception_code_string = str8_from_u64(arena, event->exception_code, 16, 0, 0);
            String8 exception_info_string = push_str8f(arena, "%S (Access violation executing 0x%I64x)", exception_code_string, event->vaddr_rng.min);
            dr_fstrs_push_new(arena, &fstrs, &params, exception_info_string);
          }break;
        }
      }
      else
      {
        dr_fstrs_push_new(arena, &fstrs, &params, rd_icon_kind_text_table[RD_IconKind_WarningBig], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons));
        dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("Hit an exception - "));
        String8 exception_code_string = str8_from_u64(arena, event->exception_code, 16, 0, 0);
        String8 exception_explanation_string = rd_string_from_exception_code(event->exception_code);
        String8 exception_info_string = push_str8f(arena, "%S%s%S%s",
                                                   exception_code_string,
                                                   exception_explanation_string.size != 0 ? " (" : "",
                                                   exception_explanation_string,
                                                   exception_explanation_string.size != 0 ? ")" : "");
        dr_fstrs_push_new(arena, &fstrs, &params, exception_info_string);
      }
    }break;
    
    //- rjf: trap
    case CTRL_EventCause_InterruptedByTrap:
    {
      dr_fstrs_push_new(arena, &fstrs, &params, rd_icon_kind_text_table[RD_IconKind_WarningBig], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons));
      dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("  "));
      dr_fstrs_concat_in_place(&fstrs, &thread_fstrs);
      dr_fstrs_push_new(arena, &fstrs, &params, str8_lit(" hit a trap"));
    }break;
    
    //- rjf: halt
    case CTRL_EventCause_InterruptedByHalt:
    {
      dr_fstrs_push_new(arena, &fstrs, &params, rd_icon_kind_text_table[RD_IconKind_Pause], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons));
      dr_fstrs_push_new(arena, &fstrs, &params, str8_lit("Halted"));
    }break;
  }
  return fstrs;
}

////////////////////////////////
//~ rjf: Vocab Info Lookups

internal RD_VocabInfo *
rd_vocab_info_from_code_name(String8 code_name)
{
  RD_VocabInfo *result = &rd_nil_vocab_info;
  if(code_name.size != 0)
  {
    U64 hash = d_hash_from_string(code_name);
    U64 slot_idx = hash%rd_state->vocab_info_map.single_slots_count;
    for(RD_VocabInfoMapNode *n = rd_state->vocab_info_map.single_slots[slot_idx].first;
        n != 0;
        n = n->single_next)
    {
      if(str8_match(n->v.code_name, code_name, 0))
      {
        result = &n->v;
        break;
      }
    }
  }
  return result;
}

internal RD_VocabInfo *
rd_vocab_info_from_code_name_plural(String8 code_name_plural)
{
  RD_VocabInfo *result = &rd_nil_vocab_info;
  if(code_name_plural.size != 0)
  {
    U64 hash = d_hash_from_string(code_name_plural);
    U64 slot_idx = hash%rd_state->vocab_info_map.plural_slots_count;
    for(RD_VocabInfoMapNode *n = rd_state->vocab_info_map.plural_slots[slot_idx].first;
        n != 0;
        n = n->plural_next)
    {
      if(str8_match(n->v.code_name_plural, code_name_plural, 0))
      {
        result = &n->v;
        break;
      }
    }
  }
  return result;
}

internal DR_FStrList
rd_title_fstrs_from_code_name(Arena *arena, String8 code_name, Vec4F32 secondary_color, F32 size)
{
  DR_FStrList result = {0};
  {
    RD_VocabInfo *info = rd_vocab_info_from_code_name(code_name);
    
    //- rjf: set up color/size for all parts of the title
    //
    // the "running" part implies that it changes as things are added - 
    // so if a primary title is pushed, we can make the rest of the title
    // more faded/smaller, but only after a primary title is pushed,
    // which could be caused by many different potential parts of a cfg.
    //
    DR_FStrParams params = {rd_font_from_slot(RD_FontSlot_Main), rd_raster_flags_from_slot(RD_FontSlot_Main), rd_rgba_from_theme_color(RD_ThemeColor_Text), size};
    B32 running_is_secondary = 0;
#define start_secondary() if(!running_is_secondary){running_is_secondary = 1; params.color = secondary_color; params.size = size*0.8f;}
    
    //- rjf: push icon
    if(info->icon_kind != RD_IconKind_Null)
    {
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
      dr_fstrs_push_new(arena, &result, &params, rd_icon_kind_text_table[info->icon_kind], .font = rd_font_from_slot(RD_FontSlot_Icons), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Icons), .color = secondary_color);
      dr_fstrs_push_new(arena, &result, &params, str8_lit("  "));
    }
    
    //- rjf: push display name
    if(info->display_name.size != 0)
    {
      dr_fstrs_push_new(arena, &result, &params, info->display_name);
    }
    
    //- rjf: push code name as a fallback
    else
    {
      dr_fstrs_push_new(arena, &result, &params, code_name, .font = rd_font_from_slot(RD_FontSlot_Code), .raster_flags = rd_raster_flags_from_slot(RD_FontSlot_Code));
    }
    
#undef start_secondary
  }
  return result;
}

////////////////////////////////
//~ rjf: Continuous Frame Requests

internal void
rd_request_frame(void)
{
  rd_state->num_frames_requested = 4;
}

////////////////////////////////
//~ rjf: Main State Accessors

//- rjf: per-frame arena

internal Arena *
rd_frame_arena(void)
{
  return rd_state->frame_arenas[rd_state->frame_index%ArrayCount(rd_state->frame_arenas)];
}

////////////////////////////////
//~ rjf: Registers

internal RD_Regs *
rd_regs(void)
{
  RD_Regs *regs = &rd_state->top_regs->v;
  return regs;
}

internal RD_Regs *
rd_base_regs(void)
{
  RD_Regs *regs = &rd_state->base_regs.v;
  return regs;
}

internal RD_Regs *
rd_push_regs_(RD_Regs *regs)
{
  RD_RegsNode *n = push_array(rd_frame_arena(), RD_RegsNode, 1);
  rd_regs_copy_contents(rd_frame_arena(), &n->v, regs);
  SLLStackPush(rd_state->top_regs, n);
  return &n->v;
}

internal RD_Regs *
rd_pop_regs(void)
{
  RD_Regs *regs = &rd_state->top_regs->v;
  SLLStackPop(rd_state->top_regs);
  if(rd_state->top_regs == 0)
  {
    rd_state->top_regs = &rd_state->base_regs;
  }
  return regs;
}

internal void
rd_regs_fill_slot_from_string(RD_RegSlot slot, String8 string)
{
  String8 error = {0};
  switch(slot)
  {
    default:
    case RD_RegSlot_String:
    case RD_RegSlot_FilePath:
    {
      String8TxtPtPair pair = str8_txt_pt_pair_from_string(string);
      if(pair.pt.line == 0 || slot == RD_RegSlot_String)
      {
        rd_regs()->string = push_str8_copy(rd_frame_arena(), string);
      }
      else
      {
        rd_regs()->file_path = push_str8_copy(rd_frame_arena(), pair.string);
        rd_regs()->cursor = pair.pt;
      }
    }break;
    case RD_RegSlot_Cursor:
    {
      U64 v = 0;
      if(try_u64_from_str8_c_rules(string, &v))
      {
        rd_regs()->cursor.column = 1;
        rd_regs()->cursor.line = v;
      }
      else
      {
        log_user_error(str8_lit("Couldn't interpret as a line number."));
      }
    }break;
    case RD_RegSlot_Vaddr: goto use_numeric_eval;
    case RD_RegSlot_Voff: goto use_numeric_eval;
    case RD_RegSlot_UnwindCount: goto use_numeric_eval;
    case RD_RegSlot_InlineDepth: goto use_numeric_eval;
    case RD_RegSlot_PID: goto use_numeric_eval;
    use_numeric_eval:
    {
      Temp scratch = scratch_begin(0, 0);
      E_Eval eval = e_eval_from_string(scratch.arena, string);
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
        switch(slot)
        {
          default:{}break;
          case RD_RegSlot_Vaddr:
          {
            rd_regs()->vaddr = u64;
          }break;
          case RD_RegSlot_Voff:
          {
            rd_regs()->voff = u64;
          }break;
          case RD_RegSlot_UnwindCount:
          {
            rd_regs()->unwind_count = u64;
          }break;
          case RD_RegSlot_InlineDepth:
          {
            rd_regs()->inline_depth = u64;
          }break;
          case RD_RegSlot_PID:
          {
            rd_regs()->pid = u64;
          }break;
        }
      }
      else
      {
        log_user_errorf("Couldn't evaluate \"%S\" as an address.", string);
      }
      scratch_end(scratch);
    }break;
  }
}

////////////////////////////////
//~ rjf: Commands

//- rjf: name -> info

internal RD_CmdKind
rd_cmd_kind_from_string(String8 string)
{
  RD_CmdKind result = RD_CmdKind_Null;
  for(U64 idx = 0; idx < ArrayCount(rd_cmd_kind_info_table); idx += 1)
  {
    if(str8_match(string, rd_cmd_kind_info_table[idx].string, 0))
    {
      result = (RD_CmdKind)idx;
      break;
    }
  }
  return result;
}

internal RD_CmdKindInfo *
rd_cmd_kind_info_from_string(String8 string)
{
  RD_CmdKindInfo *info = &rd_nil_cmd_kind_info;
  {
    // TODO(rjf): @dynamic_cmds extend this by looking up into dynamically-registered commands by views
    RD_CmdKind kind = rd_cmd_kind_from_string(string);
    if(kind != RD_CmdKind_Null)
    {
      info = &rd_cmd_kind_info_table[kind];
    }
  }
  return info;
}

//- rjf: pushing

internal void
rd_push_cmd(String8 name, RD_Regs *regs)
{
  rd_cmd_list_push_new(rd_state->cmds_arenas[0], &rd_state->cmds[0], name, regs);
}

//- rjf: iterating

internal B32
rd_next_cmd(RD_Cmd **cmd)
{
  U64 slot = rd_state->cmds_gen%ArrayCount(rd_state->cmds);
  RD_CmdNode *start_node = rd_state->cmds[slot].first;
  if(cmd[0] != 0)
  {
    start_node = CastFromMember(RD_CmdNode, cmd, cmd[0]);
    start_node = start_node->next;
  }
  cmd[0] = 0;
  if(start_node != 0)
  {
    cmd[0] = &start_node->cmd;
  }
  return !!cmd[0];
}

internal B32
rd_next_view_cmd(RD_Cmd **cmd)
{
  for(;rd_next_cmd(cmd);)
  {
    if(rd_regs()->view == cmd[0]->regs->view)
    {
      break;
    }
  }
  B32 result = !!cmd[0];
  return result;
}

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

#if !defined(STBI_INCLUDE_STB_IMAGE_H)
# define STB_IMAGE_IMPLEMENTATION
# define STBI_ONLY_PNG
# define STBI_ONLY_BMP
# include "third_party/stb/stb_image.h"
#endif

internal void
rd_init(CmdLine *cmdln)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  rd_state = push_array(arena, RD_State, 1);
  rd_state->arena = arena;
  rd_state->quit_after_success = (cmd_line_has_flag(cmdln, str8_lit("quit_after_success")) ||
                                  cmd_line_has_flag(cmdln, str8_lit("q")));
  rd_state->user_path_arena = arena_alloc();
  rd_state->project_path_arena = arena_alloc();
  for(U64 idx = 0; idx < ArrayCount(rd_state->frame_arenas); idx += 1)
  {
    rd_state->frame_arenas[idx] = arena_alloc();
  }
  rd_state->log = log_alloc();
  log_select(rd_state->log);
  {
    Temp scratch = scratch_begin(0, 0);
    String8 user_program_data_path = os_get_process_info()->user_program_data_path;
    String8 user_data_folder = push_str8f(scratch.arena, "%S/raddbg/logs", user_program_data_path);
    rd_state->log_path = push_str8f(rd_state->arena, "%S/ui_thread.raddbg_log", user_data_folder);
    os_make_directory(user_data_folder);
    os_write_data_to_file_path(rd_state->log_path, str8_zero());
    scratch_end(scratch);
  }
  rd_state->num_frames_requested = 2;
  rd_state->seconds_until_autosave = 0.5f;
  rd_state->match_store = di_match_store_alloc();
  for(U64 idx = 0; idx < ArrayCount(rd_state->cmds_arenas); idx += 1)
  {
    rd_state->cmds_arenas[idx] = arena_alloc();
  }
  rd_state->popup_arena = arena_alloc();
  rd_state->ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("top_level_ctx_menu"));
  rd_state->drop_completion_key = ui_key_from_string(ui_key_zero(), str8_lit("drop_completion_ctx_menu"));
  rd_state->string_search_arena = arena_alloc();
  rd_state->bind_change_arena = arena_alloc();
  rd_state->drag_drop_arena = arena_alloc();
  rd_state->drag_drop_regs = push_array(rd_state->drag_drop_arena, RD_Regs, 1);
  rd_state->top_regs = &rd_state->base_regs;
  
  // rjf: set up schemas
  {
    U64 schemas_count = ArrayCount(rd_name_schema_info_table);
    rd_state->schemas = push_array(rd_state->arena, MD_Node *, schemas_count);
    for EachIndex(idx, schemas_count)
    {
      rd_state->schemas[idx] = md_tree_from_string(rd_state->arena, rd_name_schema_info_table[idx].schema)->first;
    }
  }
  
  // rjf: set up vocab info map
  {
    rd_state->vocab_info_map.single_slots_count = 1024;
    rd_state->vocab_info_map.single_slots = push_array(rd_state->arena, RD_VocabInfoMapSlot, rd_state->vocab_info_map.single_slots_count);
    rd_state->vocab_info_map.plural_slots_count = 1024;
    rd_state->vocab_info_map.plural_slots = push_array(rd_state->arena, RD_VocabInfoMapSlot, rd_state->vocab_info_map.plural_slots_count);
    for EachElement(idx, rd_vocab_info_table)
    {
      RD_VocabInfoMapNode *n = push_array(rd_state->arena, RD_VocabInfoMapNode, 1);
      MemoryCopyStruct(&n->v, &rd_vocab_info_table[idx]);
      U64 single_hash = d_hash_from_string(n->v.code_name);
      U64 plural_hash = d_hash_from_string(n->v.code_name_plural);
      U64 single_slot_idx = single_hash%rd_state->vocab_info_map.single_slots_count;
      U64 plural_slot_idx = plural_hash%rd_state->vocab_info_map.plural_slots_count;
      if(n->v.code_name.size != 0)
      {
        SLLQueuePush_N(rd_state->vocab_info_map.single_slots[single_slot_idx].first, rd_state->vocab_info_map.single_slots[single_slot_idx].last, n, single_next);
      }
      if(n->v.code_name_plural.size != 0)
      {
        SLLQueuePush_N(rd_state->vocab_info_map.plural_slots[plural_slot_idx].first, rd_state->vocab_info_map.plural_slots[plural_slot_idx].last, n, plural_next);
      }
    }
  }
  
  // rjf: set up top-level config entity trees & tables
  {
    rd_state->cfg_id_slots_count = 1024;
    rd_state->cfg_id_slots = push_array(arena, RD_CfgSlot, rd_state->cfg_id_slots_count);
    rd_state->root_cfg = rd_cfg_alloc();
    RD_Cfg *user_tree         = rd_cfg_new(rd_state->root_cfg, str8_lit("user"));
    RD_Cfg *project_tree      = rd_cfg_new(rd_state->root_cfg, str8_lit("project"));
    RD_Cfg *command_line_tree = rd_cfg_new(rd_state->root_cfg, str8_lit("command_line"));
    RD_Cfg *transient         = rd_cfg_new(rd_state->root_cfg, str8_lit("transient"));
  }
  
  // rjf: set up window cache
  {
    rd_state->window_state_slots_count = 64;
    rd_state->window_state_slots = push_array(arena, RD_WindowStateSlot, rd_state->window_state_slots_count);
    rd_state->first_window_state = rd_state->last_window_state = &rd_nil_window_state;
  }
  
  // rjf: set up view cache
  {
    rd_state->view_state_slots_count = 4096;
    rd_state->view_state_slots = push_array(arena, RD_ViewStateSlot, rd_state->view_state_slots_count);
  }
  
  // rjf: set up user / project paths
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: unpack command line arguments
    String8 user_path = cmd_line_string(cmdln, str8_lit("user"));
    String8 project_path = cmd_line_string(cmdln, str8_lit("project"));
    {
      String8 user_program_data_path = os_get_process_info()->user_program_data_path;
      String8 user_data_folder = push_str8f(scratch.arena, "%S/%S", user_program_data_path, str8_lit("raddbg"));
      os_make_directory(user_data_folder);
      if(user_path.size == 0)
      {
        user_path = push_str8f(scratch.arena, "%S/default.raddbg_user", user_data_folder);
      }
      if(project_path.size == 0)
      {
        project_path = push_str8f(scratch.arena, "%S/default.raddbg_project", user_data_folder);
      }
    }
    
    // rjf: do initial load
    rd_cmd(RD_CmdKind_OpenUser,    .file_path = user_path);
    rd_cmd(RD_CmdKind_OpenProject, .file_path = project_path);
    
    scratch_end(scratch);
  }
  
  // rjf: unpack icon image data
  {
    Temp scratch = scratch_begin(0, 0);
    String8 data = rd_icon_file_bytes;
    U8 *ptr = data.str;
    U8 *opl = ptr+data.size;
    
    // rjf: read header
#pragma pack(push, 1)
    typedef struct ICO_Header ICO_Header;
    struct ICO_Header
    {
      U16 reserved_padding; // must be 0
      U16 image_type; // if 1 -> ICO, if 2 -> CUR
      U16 num_images;
    };
    typedef struct ICO_Entry ICO_Entry;
    struct ICO_Entry
    {
      U8 image_width_px;
      U8 image_height_px;
      U8 num_colors;
      U8 reserved_padding; // should be 0
      union
      {
        U16 ico_color_planes; // in ICO
        U16 cur_hotspot_x_px; // in CUR
      };
      union
      {
        U16 ico_bits_per_pixel; // in ICO
        U16 cur_hotspot_y_px;   // in CUR
      };
      U32 image_data_size;
      U32 image_data_off;
    };
#pragma pack(pop)
    ICO_Header hdr = {0};
    if(ptr+sizeof(hdr) < opl)
    {
      MemoryCopy(&hdr, ptr, sizeof(hdr));
      ptr += sizeof(hdr);
    }
    
    // rjf: read image entries
    U64 entries_count = hdr.num_images;
    ICO_Entry *entries = push_array(scratch.arena, ICO_Entry, hdr.num_images);
    {
      U64 bytes_to_read = sizeof(ICO_Entry)*entries_count;
      bytes_to_read = Min(bytes_to_read, opl-ptr);
      MemoryCopy(entries, ptr, bytes_to_read);
      ptr += bytes_to_read;
    }
    
    // rjf: find largest image
    ICO_Entry *best_entry = 0;
    U64 best_entry_area = 0;
    for(U64 idx = 0; idx < entries_count; idx += 1)
    {
      ICO_Entry *entry = &entries[idx];
      U64 width = entry->image_width_px;
      if(width == 0) { width = 256; }
      U64 height = entry->image_height_px;
      if(height == 0) { height = 256; }
      U64 entry_area = width*height;
      if(entry_area > best_entry_area)
      {
        best_entry = entry;
        best_entry_area = entry_area;
      }
    }
    
    // rjf: deserialize raw image data from best entry's offset
    U8 *image_data = 0;
    Vec2S32 image_dim = {0};
    if(best_entry != 0)
    {
      U8 *file_data_ptr = data.str + best_entry->image_data_off;
      U64 file_data_size = best_entry->image_data_size;
      int width = 0;
      int height = 0;
      int components = 0;
      image_data = stbi_load_from_memory(file_data_ptr, file_data_size, &width, &height, &components, 4);
      image_dim.x = width;
      image_dim.y = height;
    }
    
    // rjf: upload to gpu texture
    rd_state->icon_texture = r_tex2d_alloc(R_ResourceKind_Static, image_dim, R_Tex2DFormat_RGBA8, image_data);
    
    // rjf: release
    stbi_image_free(image_data);
    scratch_end(scratch);
  }
  
  // rjf: set up initial browse path
  {
    Temp scratch = scratch_begin(0, 0);
    String8 current_path = os_get_current_path(scratch.arena);
    String8 current_path_with_slash = push_str8f(scratch.arena, "%S/", current_path);
    rd_state->current_path_arena = arena_alloc();
    rd_state->current_path = push_str8_copy(rd_state->current_path_arena, current_path_with_slash);
    scratch_end(scratch);
  }
  
  ProfEnd();
}

internal void
rd_frame(void)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  local_persist S32 depth = 0;
  log_scope_begin();
  
  //- TODO(rjf): @cfg debugging: stringify the current cfg tree
  {
    String8 string = rd_string_from_cfg_tree(scratch.arena, rd_state->root_cfg);
    int x = 0;
  }
  
  //////////////////////////////
  //- rjf: do per-frame resets
  //
  arena_clear(rd_frame_arena());
  rd_state->top_regs = &rd_state->base_regs;
  rd_regs_copy_contents(rd_frame_arena(), &rd_state->top_regs->v, &rd_state->top_regs->v);
  if(rd_state->next_hover_regs != 0)
  {
    rd_state->hover_regs = rd_regs_copy(rd_frame_arena(), rd_state->next_hover_regs);
    rd_state->hover_regs_slot = rd_state->next_hover_regs_slot;
    rd_state->next_hover_regs = 0;
  }
  else
  {
    rd_state->hover_regs = push_array(rd_frame_arena(), RD_Regs, 1);
    rd_state->hover_regs_slot = RD_RegSlot_Null;
  }
  if(depth == 0)
  {
    rd_state->frame_di_scope = di_scope_open();
  }
  B32 allow_text_hotkeys = !rd_state->text_edit_mode;
  rd_state->text_edit_mode = 0;
  rd_state->cfg2evalblob_map = push_array(rd_frame_arena(), RD_Cfg2EvalBlobMap, 1);
  rd_state->cfg2evalblob_map->slots_count = 256;
  rd_state->cfg2evalblob_map->slots = push_array(rd_frame_arena(), RD_Cfg2EvalBlobSlot, rd_state->cfg2evalblob_map->slots_count);
  rd_state->entity2evalblob_map = push_array(rd_frame_arena(), RD_Entity2EvalBlobMap, 1);
  rd_state->entity2evalblob_map->slots_count = 256;
  rd_state->entity2evalblob_map->slots = push_array(rd_frame_arena(), RD_Entity2EvalBlobSlot, rd_state->entity2evalblob_map->slots_count);
  
  //////////////////////////////
  //- rjf: iterate all tabs, touch their view-states
  //
  if(depth == 0)
  {
    Temp scratch = scratch_begin(0, 0);
    RD_CfgList windows = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("window"));
    for(RD_CfgNode *n = windows.first; n != 0; n = n->next)
    {
      RD_Cfg *window = n->v;
      RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
      for(RD_PanelNode *p = panel_tree.root; p != &rd_nil_panel_node; p = rd_panel_node_rec__depth_first_pre(panel_tree.root, p).next)
      {
        for(RD_CfgNode *n = p->tabs.first; n != 0; n = n->next)
        {
          RD_Cfg *tab = n->v;
          if(rd_cfg_is_project_filtered(tab))
          {
            continue;
          }
          rd_view_state_from_cfg(tab);
        }
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: garbage collect untouched immediate cfg trees
  //
  if(depth == 0)
  {
    RD_Cfg *transient = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("transient"));
    for(RD_Cfg *tln = transient->first, *next = &rd_nil_cfg; tln != &rd_nil_cfg; tln = next)
    {
      next = tln->next;
      if(str8_match(tln->string, str8_lit("immediate"), 0))
      {
        if(rd_cfg_child_from_string(tln, str8_lit("hot")) == &rd_nil_cfg)
        {
          rd_cfg_release(tln);
        }
      }
    }
    for(RD_Cfg *tln = transient->first; tln != &rd_nil_cfg; tln = tln->next)
    {
      if(str8_match(tln->string, str8_lit("immediate"), 0))
      {
        rd_cfg_release(rd_cfg_child_from_string(tln, str8_lit("hot")));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: garbage collect untouched window states
  //
  if(depth == 0) DeferLoop(depth += 1, depth -= 1)
  {
    for EachIndex(slot_idx, rd_state->window_state_slots_count)
    {
      for(RD_WindowState *ws = rd_state->window_state_slots[slot_idx].first, *next; ws != 0; ws = next)
      {
        next = ws->hash_next;
        if(ws->last_frame_index_touched+2 < rd_state->frame_index)
        {
          ui_state_release(ws->ui);
          r_window_unequip(ws->os, ws->r);
          os_window_close(ws->os);
          if(ws->autocomp_lister != 0)
          {
            arena_release(ws->autocomp_lister->arena);
          }
          for(RD_Lister *lister = ws->top_query_lister; lister != 0; lister = lister->next)
          {
            arena_release(lister->arena);
          }
          arena_release(ws->ctx_menu_arena);
          arena_release(ws->drop_completion_arena);
          arena_release(ws->hover_eval_arena);
          arena_release(ws->arena);
          DLLRemove_NPZ(&rd_nil_window_state, rd_state->first_window_state, rd_state->last_window_state, ws, order_next, order_prev);
          DLLRemove_NP(rd_state->window_state_slots[slot_idx].first, rd_state->window_state_slots[slot_idx].last, ws, hash_next, hash_prev);
          SLLStackPush_N(rd_state->free_window_state, ws, order_next);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: garbage collect untouched view states
  //
  if(depth == 0)
  {
    for EachIndex(slot_idx, rd_state->view_state_slots_count)
    {
      for(RD_ViewState *vs = rd_state->view_state_slots[slot_idx].first, *next; vs != 0; vs = next)
      {
        next = vs->hash_next;
        if(vs->last_frame_index_touched+2 < rd_state->frame_index)
        {
          ev_view_release(vs->ev_view);
          for(RD_ArenaExt *ext = vs->first_arena_ext; ext != 0; ext = ext->next)
          {
            arena_release(ext->arena);
          }
          arena_release(vs->arena);
          DLLRemove_NP(rd_state->view_state_slots[slot_idx].first, rd_state->view_state_slots[slot_idx].last, vs, hash_next, hash_prev);
          SLLStackPush_N(rd_state->free_view_state, vs, hash_next);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: sync with di parsers
  //
  ProfScope("sync with di parsers")
  {
    DI_EventList events = di_p2u_pop_events(scratch.arena, 0);
    for(DI_EventNode *n = events.first; n != 0; n = n->next)
    {
      DI_Event *event = &n->v;
      switch(event->kind)
      {
        default:{}break;
        case DI_EventKind_ConversionStarted:
        {
          RD_Cfg *root = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("transient"));
          RD_Cfg *task = rd_cfg_new(root, str8_lit("conversion_task"));
          rd_cfg_new(task, event->string);
        }break;
        case DI_EventKind_ConversionEnded:
        {
          RD_Cfg *root = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("transient"));
          for(RD_Cfg *tln = root->first; tln != &rd_nil_cfg; tln = tln->next)
          {
            if(str8_match(tln->string, str8_lit("conversion_task"), 0) && str8_match(tln->first->string, event->string, 0))
            {
              rd_cfg_release(tln);
              break;
            }
          }
        }break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: animate all views
  //
  if(depth == 0)
  {
    F32 slow_rate = 1 - pow_f32(2, (-10.f * rd_state->frame_dt));
    F32 fast_rate = 1 - pow_f32(2, (-40.f * rd_state->frame_dt));
    for EachIndex(slot_idx, rd_state->view_state_slots_count)
    {
      for(RD_ViewState *vs = rd_state->view_state_slots[slot_idx].first;
          vs != 0;
          vs = vs->hash_next)
      {
        F32 scroll_x_diff = (-vs->scroll_pos.x.off);
        F32 scroll_y_diff = (-vs->scroll_pos.y.off);
        F32 loading_t_diff = (vs->loading_t_target - vs->loading_t);
        vs->scroll_pos.x.off += scroll_x_diff*fast_rate;
        vs->scroll_pos.y.off += scroll_y_diff*fast_rate;
        vs->loading_t += loading_t_diff * slow_rate;
        if(abs_f32(loading_t_diff) > 0.01f ||
           abs_f32(scroll_x_diff) > 0.01f ||
           abs_f32(scroll_y_diff) > 0.01f)
        {
          rd_request_frame();
        }
        if(abs_f32(scroll_x_diff) <= 0.01f)
        {
          vs->scroll_pos.x.off = 0;
        }
        if(abs_f32(scroll_y_diff) <= 0.01f)
        {
          vs->scroll_pos.y.off = 0;
        }
        RD_Cfg *vcfg = rd_cfg_from_id(vs->cfg_id);
        if(rd_cfg_child_from_string(vcfg, str8_lit("selected")) != &rd_nil_cfg)
        {
          if(vs->loading_t_target > 0.5f)
          {
            rd_request_frame();
          }
          vs->loading_t_target = 0;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: get events from the OS
  //
  OS_EventList events = {0};
  if(depth == 0) DeferLoop(depth += 1, depth -= 1)
  {
    events = os_get_events(scratch.arena, rd_state->num_frames_requested == 0);
  }
  
  //////////////////////////////
  //- rjf: pick target hz
  //
  // TODO(rjf): maximize target, given all windows and their monitors
  F32 target_hz = os_get_gfx_info()->default_refresh_rate;
  if(rd_state->frame_index > 32)
  {
    // rjf: calculate average frame time out of the last N
    U64 num_frames_in_history = Min(ArrayCount(rd_state->frame_time_us_history), rd_state->frame_index);
    U64 frame_time_history_sum_us = 0;
    for(U64 idx = 0; idx < num_frames_in_history; idx += 1)
    {
      frame_time_history_sum_us += rd_state->frame_time_us_history[idx];
    }
    U64 frame_time_history_avg_us = frame_time_history_sum_us/num_frames_in_history;
    
    // rjf: pick among a number of sensible targets to snap to, given how well
    // we've been performing
    F32 possible_alternate_hz_targets[] = {target_hz, 60.f, 120.f, 144.f, 240.f};
    F32 best_target_hz = target_hz;
    S64 best_target_hz_frame_time_us_diff = max_S64;
    for(U64 idx = 0; idx < ArrayCount(possible_alternate_hz_targets); idx += 1)
    {
      F32 candidate = possible_alternate_hz_targets[idx];
      if(candidate <= target_hz)
      {
        U64 candidate_frame_time_us = 1000000/(U64)candidate;
        S64 frame_time_us_diff = (S64)frame_time_history_avg_us - (S64)candidate_frame_time_us;
        if(abs_s64(frame_time_us_diff) < best_target_hz_frame_time_us_diff)
        {
          best_target_hz = candidate;
          best_target_hz_frame_time_us_diff = frame_time_us_diff;
        }
      }
    }
    target_hz = best_target_hz;
  }
  
  //////////////////////////////
  //- rjf: target Hz -> delta time
  //
  rd_state->frame_dt = 1.f/target_hz;
  
  //////////////////////////////
  //- rjf: begin measuring actual per-frame work
  //
  U64 begin_time_us = os_now_microseconds();
  
  //////////////////////////////
  //- rjf: bind change
  //
  if(!rd_state->popup_active && rd_state->bind_change_active)
  {
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Esc))
    {
      rd_request_frame();
      rd_state->bind_change_active = 0;
    }
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Delete))
    {
      rd_request_frame();
      // TODO(rjf): @cfg_bindchange rd_unbind_name(rd_state->bind_change_cmd_name, rd_state->bind_change_binding);
      rd_state->bind_change_active = 0;
    }
    for(OS_Event *event = events.first, *next = 0; event != 0; event = next)
    {
      if(event->kind == OS_EventKind_Press &&
         event->key != OS_Key_Esc &&
         event->key != OS_Key_Return &&
         event->key != OS_Key_Backspace &&
         event->key != OS_Key_Delete &&
         event->key != OS_Key_LeftMouseButton &&
         event->key != OS_Key_RightMouseButton &&
         event->key != OS_Key_MiddleMouseButton &&
         event->key != OS_Key_Ctrl &&
         event->key != OS_Key_Alt &&
         event->key != OS_Key_Shift)
      {
        rd_state->bind_change_active = 0;
        RD_Binding binding = zero_struct;
        {
          binding.key = event->key;
          binding.modifiers = event->modifiers;
        }
        // TODO(rjf): @cfg_bindchange rd_unbind_name(rd_state->bind_change_cmd_name, rd_state->bind_change_binding);
        // TODO(rjf): @cfg_bindchange rd_bind_name(rd_state->bind_change_cmd_name, binding);
        U32 codepoint = os_codepoint_from_modifiers_and_key(event->modifiers, event->key);
        os_text(&events, event->window, codepoint);
        os_eat_event(&events, event);
        rd_request_frame();
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build key map from config
  //
  ProfScope("build key map from config")
  {
    //- rjf: set up table
    rd_state->key_map = push_array(rd_frame_arena(), RD_KeyMap, 1);
    RD_KeyMap *key_map = rd_state->key_map;
    key_map->name_slots_count = 4096;
    key_map->name_slots = push_array(rd_frame_arena(), RD_KeyMapSlot, key_map->name_slots_count);
    key_map->binding_slots_count = 4096;
    key_map->binding_slots = push_array(rd_frame_arena(), RD_KeyMapSlot, key_map->binding_slots_count);
    
    //- rjf: gather & parse all explicitly stored keybinding sets
    RD_CfgList keybindings_cfg_list = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("keybindings"));
    for(RD_CfgNode *n = keybindings_cfg_list.first; n != 0; n = n->next)
    {
      RD_Cfg *keybindings_root = n->v;
      for(RD_Cfg *keybinding = keybindings_root->first; keybinding != &rd_nil_cfg; keybinding = keybinding->next)
      {
        String8 name = {0};
        RD_Binding binding = {0};
        for(RD_Cfg *child = keybinding->first; child != &rd_nil_cfg; child = child->next)
        {
          if(0){}
          else if(str8_match(child->string, str8_lit("ctrl"), 0))   { binding.modifiers |= OS_Modifier_Ctrl; }
          else if(str8_match(child->string, str8_lit("alt"), 0))    { binding.modifiers |= OS_Modifier_Alt; }
          else if(str8_match(child->string, str8_lit("shift"), 0))  { binding.modifiers |= OS_Modifier_Shift; }
          else
          {
            OS_Key key = OS_Key_Null;
            for EachEnumVal(OS_Key, k)
            {
              if(str8_match(child->string, os_g_key_cfg_string_table[k], StringMatchFlag_CaseInsensitive))
              {
                key = k;
                break;
              }
            }
            if(key != OS_Key_Null)
            {
              binding.key = key;
            }
            else
            {
              name = child->string;
              for(U64 idx = 0; idx < ArrayCount(rd_binding_version_remap_old_name_table); idx += 1)
              {
                if(str8_match(rd_binding_version_remap_old_name_table[idx], name, StringMatchFlag_CaseInsensitive))
                {
                  name = rd_binding_version_remap_new_name_table[idx];
                }
              }
            }
          }
        }
        if(name.size != 0)
        {
          U64 name_hash = d_hash_from_string(name);
          U64 binding_hash = d_hash_from_string(str8_struct(&binding));
          U64 name_slot_idx = name_hash%key_map->name_slots_count;
          U64 binding_slot_idx = binding_hash%key_map->binding_slots_count;
          RD_KeyMapNode *n = push_array(rd_frame_arena(), RD_KeyMapNode, 1);
          n->cfg = keybinding;
          n->name = push_str8_copy(rd_frame_arena(), name);
          n->binding = binding;
          SLLQueuePush_N(key_map->name_slots[name_slot_idx].first, key_map->name_slots[name_slot_idx].last, n, name_hash_next);
          SLLQueuePush_N(key_map->binding_slots[binding_slot_idx].first, key_map->binding_slots[binding_slot_idx].last, n, binding_hash_next);
        }
      }
    }
    
    //- rjf: iterate default bindings - if their commands are not found in the
    // map, then use the default binding.
    //
    // TODO(rjf): @dynamic_cmds
    //
    for EachElement(idx, rd_default_binding_table)
    {
      String8 name = rd_default_binding_table[idx].string;
      B32 name_was_mapped = 0;
      U64 name_hash = d_hash_from_string(name);
      U64 name_slot_idx = name_hash%key_map->name_slots_count;
      for(RD_KeyMapNode *n = key_map->name_slots[name_slot_idx].first; n != 0; n = n->name_hash_next)
      {
        if(str8_match(n->name, name, 0))
        {
          name_was_mapped = 1;
          break;
        }
      }
      if(!name_was_mapped)
      {
        RD_Binding binding = rd_default_binding_table[idx].binding;
        U64 binding_hash = d_hash_from_string(str8_struct(&binding));
        U64 binding_slot_idx = binding_hash%key_map->binding_slots_count;
        RD_KeyMapNode *n = push_array(rd_frame_arena(), RD_KeyMapNode, 1);
        n->cfg = &rd_nil_cfg;
        n->name = push_str8_copy(rd_frame_arena(), name);
        n->binding = binding;
        SLLQueuePush_N(key_map->name_slots[name_slot_idx].first, key_map->name_slots[name_slot_idx].last, n, name_hash_next);
        SLLQueuePush_N(key_map->binding_slots[binding_slot_idx].first, key_map->binding_slots[binding_slot_idx].last, n, binding_hash_next);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build theme from config
  //
  ProfScope("build theme from config")
  {
    rd_state->theme_target = push_array(rd_frame_arena(), RD_Theme, 1);
    RD_Theme *theme = rd_state->theme_target;
    
    //- rjf: gather globally-applying config options
    RD_CfgList preset_roots = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("color_preset"));
    RD_CfgList colors_roots = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("colors"));
    
    //- rjf: assume default-dark
    MemoryCopy(theme->colors, rd_theme_preset_colors_table[RD_ThemePreset_DefaultDark], sizeof(rd_theme_preset_colors__default_dark));
    
    //- rjf: apply explicitly-specified presets
    for(RD_CfgNode *n = preset_roots.first; n != 0; n = n->next)
    {
      RD_Cfg *preset = n->v;
      String8 preset_name = preset->first->string;
      RD_ThemePreset preset_kind = (RD_ThemePreset)0;
      B32 found_preset_kind = 0;
      for(RD_ThemePreset p = (RD_ThemePreset)0; p < RD_ThemePreset_COUNT; p = (RD_ThemePreset)(p+1))
      {
        if(str8_match(preset_name, rd_theme_preset_code_string_table[p], StringMatchFlag_CaseInsensitive))
        {
          found_preset_kind = 1;
          preset_kind = p;
          break;
        }
      }
      if(found_preset_kind)
      {
        MemoryCopy(theme->colors, rd_theme_preset_colors_table[preset_kind], sizeof(rd_theme_preset_colors__default_dark));
      }
    }
    
    //- rjf: apply explicitly-specified color codes
    for(RD_CfgNode *n = colors_roots.first; n != 0; n = n->next)
    {
      RD_Cfg *colors = n->v;
      for(RD_Cfg *color = colors->first; color != &rd_nil_cfg; color = color->next)
      {
        String8 name = color->string;
        RD_ThemeColor color_code = RD_ThemeColor_Null;
        for(RD_ThemeColor c = RD_ThemeColor_Null; c < RD_ThemeColor_COUNT; c = (RD_ThemeColor)(c+1))
        {
          if(str8_match(rd_theme_color_cfg_string_table[c], name, StringMatchFlag_CaseInsensitive))
          {
            color_code = c;
            break;
          }
        }
        if(color_code != RD_ThemeColor_Null)
        {
          U64 color_val = 0;
          if(try_u64_from_str8_c_rules(color->first->string, &color_val))
          {
            Vec4F32 color_rgba = rgba_from_u32((U32)color_val);
            theme->colors[color_code] = color_rgba;
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: animate theme
  //
  {
    Temp scratch = scratch_begin(0, 0);
    RD_Theme *last = push_array(scratch.arena, RD_Theme, 1);
    if(rd_state->theme != 0)
    {
      MemoryCopyStruct(last, rd_state->theme);
    }
    rd_state->theme = push_array(rd_frame_arena(), RD_Theme, 1);
    RD_Theme *current = rd_state->theme;
    RD_Theme *target = rd_state->theme_target;
    if(last)
    {
      MemoryCopyStruct(current, last);
    }
    if(rd_state->frame_index <= 2)
    {
      MemoryCopyStruct(current, target);
    }
    else
    {
      F32 rate = 1 - pow_f32(2, (-50.f * rd_state->frame_dt));
      for(RD_ThemeColor color = RD_ThemeColor_Null;
          color < RD_ThemeColor_COUNT;
          color = (RD_ThemeColor)(color+1))
      {
        if(abs_f32(target->colors[color].x - current->colors[color].x) > 0.01f ||
           abs_f32(target->colors[color].y - current->colors[color].y) > 0.01f ||
           abs_f32(target->colors[color].z - current->colors[color].z) > 0.01f ||
           abs_f32(target->colors[color].w - current->colors[color].w) > 0.01f)
        {
          rd_request_frame();
        }
        current->colors[color].x += (target->colors[color].x - current->colors[color].x) * rate;
        current->colors[color].y += (target->colors[color].y - current->colors[color].y) * rate;
        current->colors[color].z += (target->colors[color].z - current->colors[color].z) * rate;
        current->colors[color].w += (target->colors[color].w - current->colors[color].w) * rate;
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: consume events
  //
  ProfScope("consume events")
  {
    for(OS_Event *event = events.first, *next = 0;
        event != 0;
        event = next)
      RD_RegsScope()
    {
      next = event->next;
      RD_WindowState *ws = rd_window_state_from_os_handle(event->window);
      if(ws != 0 && ws != rd_window_state_from_cfg(rd_cfg_from_id(rd_regs()->window)))
      {
        Temp scratch = scratch_begin(0, 0);
        RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, rd_cfg_from_id(ws->cfg_id));
        rd_regs()->window = ws->cfg_id;
        rd_regs()->panel  = panel_tree.focused->cfg->id;
        rd_regs()->view   = panel_tree.focused->selected_tab->id;
        scratch_end(scratch);
      }
      B32 take = 0;
      
      //- rjf: try drag/drop drop-kickoff
      if(rd_drag_is_active() && event->kind == OS_EventKind_Release && event->key == OS_Key_LeftMouseButton)
      {
        rd_state->drag_drop_state = RD_DragDropState_Dropping;
      }
      
      //- rjf: try window close
      if(!take && event->kind == OS_EventKind_WindowClose && ws != 0)
      {
        take = 1;
        rd_cmd(RD_CmdKind_Exit);
      }
      
      //- rjf: try menu bar operations
      {
        if(!take && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->modifiers == 0 && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          ws->menu_bar_focused_on_press = ws->menu_bar_focused;
          ws->menu_bar_key_held = 1;
          ws->menu_bar_focus_press_started = 1;
        }
        if(!take && event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->modifiers == 0 && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          ws->menu_bar_key_held = 0;
        }
        if(ws->menu_bar_focused && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->modifiers == 0 && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          ws->menu_bar_focused = 0;
        }
        else if(ws->menu_bar_focus_press_started && !ws->menu_bar_focused && event->kind == OS_EventKind_Release && event->modifiers == 0 && event->key == OS_Key_Alt && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          ws->menu_bar_focused = !ws->menu_bar_focused_on_press;
          ws->menu_bar_focus_press_started = 0;
        }
        else if(event->kind == OS_EventKind_Press && event->key == OS_Key_Esc && ws->menu_bar_focused && !ui_any_ctx_menu_is_open())
        {
          take = 1;
          rd_request_frame();
          ws->menu_bar_focused = 0;
        }
      }
      
      //- rjf: try hotkey presses
      if(!take && event->kind == OS_EventKind_Press)
      {
        RD_Binding binding = {event->key, event->modifiers};
        RD_KeyMapNodePtrList key_map_nodes = rd_key_map_node_ptr_list_from_binding(scratch.arena, binding);
        if(key_map_nodes.first != 0)
        {
          U32 hit_char = os_codepoint_from_modifiers_and_key(event->modifiers, event->key);
          if(hit_char == 0 || allow_text_hotkeys)
          {
            rd_cmd(RD_CmdKind_RunCommand, .cmd_name = key_map_nodes.first->v->name);
            if(allow_text_hotkeys)
            {
              os_text(&events, event->window, hit_char);
              next = event->next;
            }
            take = 1;
            if(event->modifiers & OS_Modifier_Alt)
            {
              ws->menu_bar_focus_press_started = 0;
            }
          }
        }
        else if(OS_Key_F1 <= event->key && event->key <= OS_Key_F19)
        {
          ws->menu_bar_focus_press_started = 0;
        }
        rd_request_frame();
      }
      
      //- rjf: try text events
      if(!take && event->kind == OS_EventKind_Text)
      {
        String32 insertion32 = str32(&event->character, 1);
        String8 insertion8 = str8_from_32(scratch.arena, insertion32);
        rd_cmd(RD_CmdKind_InsertText, .string = insertion8);
        rd_request_frame();
        take = 1;
        if(event->modifiers & OS_Modifier_Alt)
        {
          ws->menu_bar_focus_press_started = 0;
        }
      }
      
      //- rjf: do fall-through
      if(!take)
      {
        take = 1;
        rd_cmd(RD_CmdKind_OSEvent, .os_event = event);
      }
      
      //- rjf: take
      if(take)
      {
        os_eat_event(&events, event);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: loop - consume events in core, tick engine, and repeat
  //
  CTRL_Handle find_thread_retry = {0};
  RD_Cmd *cmd = 0;
  ProfScope("loop - consume events in core, tick engine, and repeat") for(U64 cmd_process_loop_idx = 0; cmd_process_loop_idx < 3; cmd_process_loop_idx += 1)
  {
    ////////////////////////////
    //- rjf: unpack eval-dependent info
    //
    ProfBegin("unpack eval-dependent info");
    CTRL_Entity *process = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->process);
    CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
    Arch arch = thread->arch;
    U64 unwind_count = rd_regs()->unwind_count;
    U64 rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_count);
    CTRL_Unwind unwind = d_query_cached_unwind_from_thread(thread);
    CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
    U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
    U64 tls_root_vaddr = ctrl_query_cached_tls_root_vaddr_from_thread(d_state->ctrl_entity_store, thread->handle);
    CTRL_EntityList all_modules = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Module);
    U64 eval_modules_count = Max(1, all_modules.count);
    E_Module *eval_modules = push_array(scratch.arena, E_Module, eval_modules_count);
    E_Module *eval_modules_primary = &eval_modules[0];
    eval_modules_primary->rdi = &di_rdi_parsed_nil;
    eval_modules_primary->vaddr_range = r1u64(0, max_U64);
    DI_Key primary_dbgi_key = {0};
    ProfScope("produce all eval modules")
    {
      U64 eval_module_idx = 0;
      for(CTRL_EntityNode *n = all_modules.first; n != 0; n = n->next, eval_module_idx += 1)
      {
        CTRL_Entity *m = n->v;
        DI_Key dbgi_key = ctrl_dbgi_key_from_module(m);
        eval_modules[eval_module_idx].arch        = m->arch;
        eval_modules[eval_module_idx].rdi         = di_rdi_from_key(rd_state->frame_di_scope, &dbgi_key, 0);
        eval_modules[eval_module_idx].vaddr_range = m->vaddr_range;
        eval_modules[eval_module_idx].space       = rd_eval_space_from_ctrl_entity(ctrl_entity_ancestor_from_kind(m, CTRL_EntityKind_Process), RD_EvalSpaceKind_CtrlEntity);
        if(module == m)
        {
          eval_modules_primary = &eval_modules[eval_module_idx];
          primary_dbgi_key = dbgi_key;
        }
      }
    }
    ProfEnd();
    
    ////////////////////////////
    //- rjf: build eval type context
    //
    E_TypeCtx *type_ctx = push_array(scratch.arena, E_TypeCtx, 1);
    ProfScope("build eval type context")
    {
      E_TypeCtx *ctx = type_ctx;
      ctx->ip_vaddr          = rip_vaddr;
      ctx->ip_voff           = rip_voff;
      ctx->modules           = eval_modules;
      ctx->modules_count     = eval_modules_count;
      ctx->primary_module    = eval_modules_primary;
    }
    e_select_type_ctx(type_ctx);
    
    ////////////////////////////
    //- rjf: build eval parse context
    //
    E_ParseCtx *parse_ctx = push_array(scratch.arena, E_ParseCtx, 1);
    ProfScope("build eval parse context")
    {
      E_ParseCtx *ctx = parse_ctx;
      ctx->ip_vaddr          = rip_vaddr;
      ctx->ip_voff           = rip_voff;
      ctx->ip_thread_space   = rd_eval_space_from_ctrl_entity(thread, RD_EvalSpaceKind_CtrlEntity);
      ctx->modules           = eval_modules;
      ctx->modules_count     = eval_modules_count;
      ctx->primary_module    = eval_modules_primary;
      ctx->regs_map      = ctrl_string2reg_from_arch(ctx->primary_module->arch);
      ctx->reg_alias_map = ctrl_string2alias_from_arch(ctx->primary_module->arch);
      ctx->locals_map    = d_query_cached_locals_map_from_dbgi_key_voff(&primary_dbgi_key, rip_voff);
      ctx->member_map    = d_query_cached_member_map_from_dbgi_key_voff(&primary_dbgi_key, rip_voff);
    }
    e_select_parse_ctx(parse_ctx);
    
    ////////////////////////////
    //- rjf: build eval IR context
    //
    E_IRCtx *ir_ctx = push_array(scratch.arena, E_IRCtx, 1);
    if(e_ir_state != 0) { e_ir_state->ctx = 0; }
    {
      E_IRCtx *ctx = ir_ctx;
      ctx->macro_map    = push_array(scratch.arena, E_String2ExprMap, 1);
      ctx->macro_map[0] = e_string2expr_map_make(scratch.arena, 512);
      ctx->lookup_rule_map    = push_array(scratch.arena, E_LookupRuleMap, 1);
      ctx->lookup_rule_map[0] = e_lookup_rule_map_make(scratch.arena, 512);
      ctx->irgen_rule_map    = push_array(scratch.arena, E_IRGenRuleMap, 1);
      ctx->irgen_rule_map[0] = e_irgen_rule_map_make(scratch.arena, 512);
      ctx->auto_hook_map      = push_array(scratch.arena, E_AutoHookMap, 1);
      ctx->auto_hook_map[0]   = e_auto_hook_map_make(scratch.arena, 512);
      
      //- rjf: choose set of evallable meta names
      String8 evallable_meta_names[] =
      {
        str8_lit("breakpoint"),
        str8_lit("watch_pin"),
        str8_lit("target"),
        str8_lit("file_path_map"),
        str8_lit("auto_view_rule"),
        str8_lit("machine"),
        str8_lit("process"),
        str8_lit("thread"),
        str8_lit("module"),
      };
      
      //- rjf: build special member types for evallable meta types
      E_TypeKey bool_type_key = {0};
      E_TypeKey u64_type_key = {0};
      E_TypeKey vaddr_range_type_key = {0};
      E_TypeKey code_string_type_key = {0};
      E_TypeKey path_type_key = {0};
      E_TypeKey string_type_key = {0};
      E_TypeKey location_type_key = {0};
      {
        E_MemberList vaddr_range_members_list = {0};
        e_member_list_push_new(scratch.arena, &vaddr_range_members_list, .type_key = e_type_key_basic(E_TypeKind_U64), .name = str8_lit("min"), .off = 0);
        e_member_list_push_new(scratch.arena, &vaddr_range_members_list, .type_key = e_type_key_basic(E_TypeKind_U64), .name = str8_lit("max"), .off = 8);
        E_MemberArray vaddr_range_members = e_member_array_from_list(scratch.arena, &vaddr_range_members_list);
        bool_type_key        = e_type_key_basic(E_TypeKind_Bool);
        u64_type_key         = e_type_key_basic(E_TypeKind_U64);
        vaddr_range_type_key = e_type_key_cons(.kind = E_TypeKind_Struct, .name = str8_lit("vaddr_range"), .count = vaddr_range_members.count, .members = vaddr_range_members.v);
        code_string_type_key = e_type_key_cons_ptr(arch_from_context(), e_type_key_basic(E_TypeKind_U8), 1, E_TypeFlag_IsCodeText);
        path_type_key        = e_type_key_cons_ptr(arch_from_context(), e_type_key_basic(E_TypeKind_U8), 1, E_TypeFlag_IsPathText);
        string_type_key      = e_type_key_cons_ptr(arch_from_context(), e_type_key_basic(E_TypeKind_U8), 1, E_TypeFlag_IsPlainText);
        location_type_key    = e_type_key_cons_ptr(arch_from_context(), e_type_key_basic(E_TypeKind_U8), 1, E_TypeFlag_IsPlainText|E_TypeFlag_IsCodeText|E_TypeFlag_IsPathText);
      }
      
      //- rjf: build types for each evallable meta name
      struct
      {
        String8 schema_type_name;
        E_TypeKey type_key;
      }
      schema_type_name_key_map[] =
      {
        { str8_lit("bool"), bool_type_key },
        { str8_lit("u64"), u64_type_key },
        { str8_lit("vaddr_range"), vaddr_range_type_key },
        { str8_lit("code_string"), code_string_type_key },
        { str8_lit("path"), path_type_key },
        { str8_lit("string"), string_type_key },
        { str8_lit("location"), location_type_key },
      };
      E_TypeKey evallable_meta_types[ArrayCount(evallable_meta_names)] = {0};
      for EachElement(idx, evallable_meta_names)
      {
        String8 name = evallable_meta_names[idx];
        MD_Node *schema = rd_schema_from_name(scratch.arena, name);
        E_MemberList members_list = {0};
        U64 off = 0;
        for MD_EachNode(child, schema->first)
        {
          String8 member_name        = child->string;
          String8 member_pretty_name = rd_display_from_code_name(member_name);
          E_TypeKey member_type_key  = zero_struct;
          for EachElement(schema_type_name_idx, schema_type_name_key_map)
          {
            if(str8_match(child->first->string, schema_type_name_key_map[schema_type_name_idx].schema_type_name, 0))
            {
              member_type_key = schema_type_name_key_map[schema_type_name_idx].type_key;
              break;
            }
          }
          e_member_list_push_new(scratch.arena, &members_list,
                                 .type_key    = member_type_key,
                                 .name        = member_name,
                                 .pretty_name = member_pretty_name,
                                 .off         = off);
          off += e_type_byte_size_from_key(member_type_key);
        }
        E_MemberArray members = e_member_array_from_list(scratch.arena, &members_list);
        evallable_meta_types[idx] = e_type_key_cons(.name    = name,
                                                    .kind    = E_TypeKind_Struct,
                                                    .members = members.v,
                                                    .count   = members.count);
      }
      
      //- rjf: cache meta name -> type key correllation
      rd_state->meta_name2type_map = push_array(rd_frame_arena(), E_String2TypeKeyMap, 1);
      rd_state->meta_name2type_map[0] = e_string2typekey_map_make(rd_frame_arena(), 256);
      for EachElement(idx, evallable_meta_names)
      {
        String8 name = evallable_meta_names[idx];
        E_TypeKey type_key = evallable_meta_types[idx];
        e_string2typekey_map_insert(rd_frame_arena(), rd_state->meta_name2type_map, name, type_key);
      }
      
      //- rjf: add macros for evallable config trees
      struct
      {
        String8 name;
        E_LookupInfoFunctionType *info;
        E_LookupAccessFunctionType *access;
        E_LookupRangeFunctionType *range;
        E_LookupIDFromNumFunctionType *id_from_num;
        E_LookupNumFromIDFunctionType *num_from_id;
      }
      evallable_cfg_table[] =
      {
        { str8_lit("breakpoint") },
        { str8_lit("watch_pin") },
        { str8_lit("target"), .info = E_LOOKUP_INFO_FUNCTION_NAME(target), .access = E_LOOKUP_ACCESS_FUNCTION_NAME(target), .range = E_LOOKUP_RANGE_FUNCTION_NAME(target) },
        { str8_lit("file_path_map") },
        { str8_lit("auto_view_rule") },
      };
      for EachElement(idx, evallable_cfg_table)
      {
        String8 name = evallable_cfg_table[idx].name;
        E_TypeKey type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, name);
        if(evallable_cfg_table[idx].info != 0)
        {
          e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, name,
                                       .info        = evallable_cfg_table[idx].info,
                                       .access      = evallable_cfg_table[idx].access,
                                       .range       = evallable_cfg_table[idx].range,
                                       .id_from_num = evallable_cfg_table[idx].id_from_num,
                                       .num_from_id = evallable_cfg_table[idx].num_from_id);
          e_auto_hook_map_insert_new(scratch.arena, ctx->auto_hook_map, .type_key = type_key, .tag_expr_string = name);
        }
        RD_CfgList cfgs = rd_cfg_top_level_list_from_string(scratch.arena, name);
        for(RD_CfgNode *n = cfgs.first; n != 0; n = n->next)
        {
          RD_Cfg *cfg = n->v;
          String8 label = rd_cfg_child_from_string(cfg, str8_lit("label"))->first->string;
          String8 exe   = rd_cfg_child_from_string(cfg, str8_lit("executable"))->first->string;
          E_Space space = rd_eval_space_from_cfg(cfg);
          E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
          expr->space    = space;
          expr->mode     = E_Mode_Offset;
          expr->type_key = type_key;
          e_string2expr_map_insert(scratch.arena, ctx->macro_map, push_str8f(scratch.arena, "$%I64u", cfg->id), expr);
          if(exe.size != 0)
          {
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, str8_skip_last_slash(exe), expr);
          }
          if(label.size != 0)
          {
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, label, expr);
          }
        }
      }
      
      //- rjf: add macros for evallable control entities
      struct
      {
        String8 name;
        E_LookupInfoFunctionType *info;
        E_LookupAccessFunctionType *access;
        E_LookupRangeFunctionType *range;
        E_LookupIDFromNumFunctionType *id_from_num;
        E_LookupNumFromIDFunctionType *num_from_id;
      }
      evallable_ctrl_table[] =
      {
        { str8_lit("machine") },
        { str8_lit("process") },
        { str8_lit("thread"), .info = E_LOOKUP_INFO_FUNCTION_NAME(thread), .access = E_LOOKUP_ACCESS_FUNCTION_NAME(thread), .range = E_LOOKUP_RANGE_FUNCTION_NAME(thread) },
        { str8_lit("module") },
      };
      for EachElement(idx, evallable_ctrl_table)
      {
        String8 name = evallable_ctrl_table[idx].name;
        CTRL_EntityKind kind = ctrl_entity_kind_from_string(name);
        CTRL_EntityList list = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, kind);
        E_TypeKey type_key = e_string2typekey_map_lookup(rd_state->meta_name2type_map, name);
        if(evallable_ctrl_table[idx].info != 0)
        {
          e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, name,
                                       .info        = evallable_ctrl_table[idx].info,
                                       .access      = evallable_ctrl_table[idx].access,
                                       .range       = evallable_ctrl_table[idx].range,
                                       .id_from_num = evallable_ctrl_table[idx].id_from_num,
                                       .num_from_id = evallable_ctrl_table[idx].num_from_id);
          e_auto_hook_map_insert_new(scratch.arena, ctx->auto_hook_map, .type_key = type_key, .tag_expr_string = name);
        }
        for(CTRL_EntityNode *n = list.first; n != 0; n = n->next)
        {
          CTRL_Entity *entity = n->v;
          E_Space space = rd_eval_space_from_ctrl_entity(entity, RD_EvalSpaceKind_MetaCtrlEntity);
          E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
          expr->space    = space;
          expr->mode     = E_Mode_Offset;
          expr->type_key = type_key;
          if(entity->string.size != 0)
          {
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, entity->string, expr);
          }
          if(kind == CTRL_EntityKind_Thread && ctrl_handle_match(rd_base_regs()->thread, entity->handle))
          {
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, str8_lit("current_thread"), expr);
          }
          if(kind == CTRL_EntityKind_Process && ctrl_handle_match(rd_base_regs()->process, entity->handle))
          {
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, str8_lit("current_process"), expr);
          }
          if(kind == CTRL_EntityKind_Module && ctrl_handle_match(rd_base_regs()->module, entity->handle))
          {
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, str8_lit("current_module"), expr);
          }
        }
        e_auto_hook_map_insert_new(scratch.arena, ctx->auto_hook_map, .type_key = type_key, .tag_expr_string = name);
      }
      
      //- rjf: add macro for 'call_stack' -> 'current_thread.callstack'
      {
        E_Expr *expr = e_parse_expr_from_text(scratch.arena, str8_lit("current_thread.call_stack"));
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, str8_lit("call_stack"), expr);
      }
      
      //- rjf: add macro for watches group
      {
        String8 collection_name = str8_lit("watches");
        E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
        expr->type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = collection_name);
        expr->space = e_space_make(RD_EvalSpaceKind_MetaCfgCollection);
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, collection_name, expr);
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, collection_name,
                                     .info        = E_LOOKUP_INFO_FUNCTION_NAME(watches),
                                     .range       = E_LOOKUP_RANGE_FUNCTION_NAME(watches),
                                     .id_from_num = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(watches),
                                     .num_from_id = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(watches));
      }
      
      //- rjf: add lookup rules for queries
      {
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, str8_lit("environment"),
                                     .info        = E_LOOKUP_INFO_FUNCTION_NAME(environment),
                                     .access      = E_LOOKUP_ACCESS_FUNCTION_NAME(environment),
                                     .range       = E_LOOKUP_RANGE_FUNCTION_NAME(environment),
                                     .id_from_num = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(environment),
                                     .num_from_id = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(environment));
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, str8_lit("call_stack"),
                                     .info        = E_LOOKUP_INFO_FUNCTION_NAME(call_stack),
                                     .access      = E_LOOKUP_ACCESS_FUNCTION_NAME(call_stack));
      }
      
      //- rjf: add macro for collections with specific lookup rules (but no unique id rules)
      {
        struct
        {
          String8 name;
          E_LookupInfoFunctionType *lookup_info;
          E_LookupRangeFunctionType *lookup_range;
        }
        collection_infos[] =
        {
#define Collection(name) {str8_lit_comp(#name), E_LOOKUP_INFO_FUNCTION_NAME(name), E_LOOKUP_RANGE_FUNCTION_NAME(name)}
          Collection(locals),
          Collection(registers),
#undef Collection
        };
        for EachElement(idx, collection_infos)
        {
          String8 collection_name = collection_infos[idx].name;
          E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
          expr->type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = collection_name);
          e_string2expr_map_insert(scratch.arena, ctx->macro_map, collection_name, expr);
          e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, collection_name,
                                       .info   = collection_infos[idx].lookup_info,
                                       .range  = collection_infos[idx].lookup_range);
        }
      }
      
      //- rjf: add macros for debug info table collections
      String8 debug_info_table_collection_names[] =
      {
        str8_lit_comp("procedures"),
        str8_lit_comp("thread_locals"),
        str8_lit_comp("globals"),
        str8_lit_comp("types"),
      };
      for EachElement(idx, debug_info_table_collection_names)
      {
        String8 name = debug_info_table_collection_names[idx];
        E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
        expr->type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = name);
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, name, expr);
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, name,
                                     .info        = E_LOOKUP_INFO_FUNCTION_NAME(debug_info_table),
                                     .range       = E_LOOKUP_RANGE_FUNCTION_NAME(debug_info_table),
                                     .id_from_num = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(debug_info_table),
                                     .num_from_id = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(debug_info_table));
      }
      
      //- rjf: add macros for all config collections
      for EachElement(cfg_name_idx, evallable_cfg_table)
      {
        String8 cfg_name = evallable_cfg_table[cfg_name_idx].name;
        String8 collection_name = rd_plural_from_code_name(cfg_name);
        E_TypeKey collection_type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = collection_name);
        E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
        expr->type_key = collection_type_key;
        expr->space = e_space_make(RD_EvalSpaceKind_MetaCfgCollection);
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, collection_name, expr);
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, collection_name,
                                     .info        = E_LOOKUP_INFO_FUNCTION_NAME(top_level_cfg),
                                     .access      = E_LOOKUP_ACCESS_FUNCTION_NAME(top_level_cfg),
                                     .range       = E_LOOKUP_RANGE_FUNCTION_NAME(top_level_cfg),
                                     .id_from_num = E_LOOKUP_ID_FROM_NUM_FUNCTION_NAME(top_level_cfg),
                                     .num_from_id = E_LOOKUP_NUM_FROM_ID_FUNCTION_NAME(top_level_cfg));
      }
      
      //- rjf: add macros for all ctrl entity collections
      for EachElement(ctrl_name_idx, evallable_ctrl_table)
      {
        String8 kind_name = evallable_ctrl_table[ctrl_name_idx].name;
        String8 collection_name = rd_plural_from_code_name(kind_name);
        E_TypeKey collection_type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = collection_name);
        E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
        expr->type_key = collection_type_key;
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, collection_name, expr);
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, collection_name,
                                     .info   = E_LOOKUP_INFO_FUNCTION_NAME(ctrl_entities),
                                     .access = E_LOOKUP_ACCESS_FUNCTION_NAME(ctrl_entities));
      }
      
      //- rjf: add macro for commands
      {
        String8 name = str8_lit("commands");
        E_TypeKey type_key = e_type_key_cons(.kind = E_TypeKind_Set, .name = name);
        E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
        expr->type_key = type_key;
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, name, expr);
        e_lookup_rule_map_insert_new(scratch.arena, ctx->lookup_rule_map, name,
                                     .info        = E_LOOKUP_INFO_FUNCTION_NAME(commands),
                                     .access      = E_LOOKUP_ACCESS_FUNCTION_NAME(commands));
      }
      
      //- rjf: add macro for output log
      {
        HS_Scope *hs_scope = hs_scope_open();
        U128 key = d_state->output_log_key;
        U128 hash = hs_hash_from_key(key, 0);
        String8 data = hs_data_from_hash(hs_scope, hash);
        E_Space space = e_space_make(E_SpaceKind_HashStoreKey);
        E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
        space.u128 = key;
        expr->space    = space;
        expr->mode     = E_Mode_Offset;
        expr->type_key = e_type_key_cons_array(e_type_key_basic(E_TypeKind_U8), data.size);
        e_string2expr_map_insert(scratch.arena, ctx->macro_map, str8_lit("output"), expr);
        hs_scope_close(hs_scope);
      }
      
      //- rjf: add macros for all watches which define identifiers
      RD_CfgList watches = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("watch"));
      for(RD_CfgNode *n = watches.first; n != 0; n = n->next)
      {
        RD_Cfg *watch = n->v;
        String8 expr = rd_expr_from_cfg(watch);
        E_Parse parse = e_parse_expr_from_text__cached(expr);
        if(parse.msgs.max_kind == E_MsgKind_Null)
        {
          for(E_Expr *expr = parse.first_expr; expr != &e_expr_nil; expr = expr->next)
          {
            e_push_leaf_ident_exprs_from_expr__in_place(scratch.arena, ctx->macro_map, expr);
          }
        }
      }
      
      //- rjf: add auto-hook rules for auto-view-rules
      {
        RD_CfgList auto_view_rules = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("auto_view_rule"));
        for(RD_CfgNode *n = auto_view_rules.first; n != 0; n = n->next)
        {
          RD_Cfg *rule = n->v;
          String8 type_string      = rd_cfg_child_from_string(rule, str8_lit("source"))->first->string;
          String8 view_rule_string = rd_cfg_child_from_string(rule, str8_lit("dest"))->first->string;
          e_auto_hook_map_insert_new(scratch.arena, ctx->auto_hook_map, .type_pattern = type_string, .tag_expr_string = view_rule_string);
        }
      }
    }
    e_select_ir_ctx(ir_ctx);
    
    ////////////////////////////
    //- rjf: build eval interpretation context
    //
    E_InterpretCtx *interpret_ctx = push_array(scratch.arena, E_InterpretCtx, 1);
    {
      E_InterpretCtx *ctx = interpret_ctx;
      ctx->space_read        = rd_eval_space_read;
      ctx->space_write       = rd_eval_space_write;
      ctx->primary_space     = eval_modules_primary->space;
      ctx->reg_arch          = eval_modules_primary->arch;
      ctx->reg_space         = rd_eval_space_from_ctrl_entity(thread, RD_EvalSpaceKind_CtrlEntity);
      ctx->reg_unwind_count  = unwind_count;
      ctx->module_base       = push_array(scratch.arena, U64, 1);
      ctx->module_base[0]    = module->vaddr_range.min;
      ctx->tls_base          = push_array(scratch.arena, U64, 1);
      ctx->tls_base[0]       = d_query_cached_tls_base_vaddr_from_process_root_rip(process, tls_root_vaddr, rip_vaddr);
    }
    e_select_interpret_ctx(interpret_ctx);
    
    ////////////////////////////
    //- rjf: build eval expand rule table
    //
    EV_ExpandRuleTable *expand_rule_table = push_array(scratch.arena, EV_ExpandRuleTable, 1);
    ev_select_expand_rule_table(expand_rule_table);
    
    ////////////////////////////
    //- rjf: build view ui rule map
    //
    rd_state->view_ui_rule_map = rd_view_ui_rule_map_make(scratch.arena, 512);
    {
      // TODO(rjf): generate via metaprogram
      struct
      {
        String8 name;
        RD_ViewUIFunctionType *ui;
        EV_ExpandRuleInfoHookFunctionType *expand;
      }
      table[] =
      {
        {str8_lit("watch"),       RD_VIEW_UI_FUNCTION_NAME(watch),             EV_EXPAND_RULE_INFO_FUNCTION_NAME(watch)},
        {str8_lit("text"),        RD_VIEW_UI_FUNCTION_NAME(text),              EV_EXPAND_RULE_INFO_FUNCTION_NAME(text)},
        {str8_lit("disasm"),      RD_VIEW_UI_FUNCTION_NAME(disasm),            EV_EXPAND_RULE_INFO_FUNCTION_NAME(disasm)},
        {str8_lit("memory"),      RD_VIEW_UI_FUNCTION_NAME(memory),            EV_EXPAND_RULE_INFO_FUNCTION_NAME(memory)},
        {str8_lit("bitmap"),      RD_VIEW_UI_FUNCTION_NAME(bitmap),            EV_EXPAND_RULE_INFO_FUNCTION_NAME(bitmap)},
        {str8_lit("checkbox"),    RD_VIEW_UI_FUNCTION_NAME(checkbox),          0},
        {str8_lit("color_rgba"),  RD_VIEW_UI_FUNCTION_NAME(color_rgba),        EV_EXPAND_RULE_INFO_FUNCTION_NAME(color_rgba)},
        {str8_lit("geo3d"),       RD_VIEW_UI_FUNCTION_NAME(geo3d),             EV_EXPAND_RULE_INFO_FUNCTION_NAME(geo3d)},
      };
      for EachElement(idx, table)
      {
        rd_view_ui_rule_map_insert(scratch.arena, rd_state->view_ui_rule_map, table[idx].name, table[idx].ui);
        if(table[idx].expand != 0)
        {
          ev_expand_rule_table_push_new(scratch.arena, expand_rule_table, table[idx].name, table[idx].expand);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: autosave if needed
    //
    {
      rd_state->seconds_until_autosave -= rd_state->frame_dt;
      if(rd_state->seconds_until_autosave <= 0.f)
      {
        rd_cmd(RD_CmdKind_WriteUserData);
        rd_cmd(RD_CmdKind_WriteProjectData);
        rd_state->seconds_until_autosave = 5.f;
      }
    }
    
    ////////////////////////////
    //- rjf: sanitize the window/panel/tab tree structure
    //
    {
      // TODO(rjf): @cfg_panels in the past, we had a spot in the rd_window_frame,
      // which ensured to select tabs, if a panel had tabs but had none
      // selected, and if a panel had a selected tab but it was project-filtered.
      // this is effectively just fixing up unexpected malformations of the
      // panel tree - because we are adopting some number of new possibilities
      // of this via the cfg change, we can just actually do a single fixup
      // point here.
    }
    
    ////////////////////////////
    //- rjf: process top-level graphical commands
    //
    if(depth == 0)
    {
      for(;rd_next_cmd(&cmd);) RD_RegsScope()
      {
        // rjf: unpack command
        RD_CmdKind kind = rd_cmd_kind_from_string(cmd->name);
        rd_regs_copy_contents(rd_frame_arena(), rd_regs(), cmd->regs);
        
        // rjf: request frame
        rd_request_frame();
        
        // rjf: process command
        Dir2 split_dir = Dir2_Invalid;
        RD_Cfg *split_panel = &rd_nil_cfg;
        U64 panel_sib_off = 0;
        U64 panel_child_off = 0;
        Vec2S32 panel_change_dir = {0};
        switch(kind)
        {
          //- rjf: default cases
          case RD_CmdKind_Run:
          case RD_CmdKind_LaunchAndRun:
          case RD_CmdKind_LaunchAndInit:
          case RD_CmdKind_StepInto:
          case RD_CmdKind_StepOver:
          case RD_CmdKind_Restart:
          {
            CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
            if(processes.count == 0 || kind == RD_CmdKind_Restart)
            {
              RD_CfgList bps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("breakpoint"));
              for(RD_CfgNode *n = bps.first; n != 0; n = n->next)
              {
                RD_Cfg *hit_count = rd_cfg_child_from_string_or_alloc(n->v, str8_lit("hit_count"));
                rd_cfg_new_replace(hit_count, str8_lit("0"));
              }
            }
          } // fallthrough
          default:
          {
            // rjf: try to run engine command
            if(D_CmdKind_Null < (D_CmdKind)kind && (D_CmdKind)kind < D_CmdKind_COUNT)
            {
              D_CmdParams params = {0};
              params.machine       = rd_regs()->machine;
              params.process       = rd_regs()->process;
              params.thread        = rd_regs()->thread;
              params.entity        = rd_regs()->ctrl_entity;
              params.string        = rd_regs()->string;
              params.file_path     = rd_regs()->file_path;
              params.cursor        = rd_regs()->cursor;
              params.vaddr         = rd_regs()->vaddr;
              params.prefer_disasm = rd_regs()->prefer_disasm;
              params.pid           = rd_regs()->pid;
              params.targets.count = 1;
              params.targets.v = push_array(scratch.arena, D_Target, params.targets.count);
              params.targets.v[0] = rd_target_from_cfg(scratch.arena, rd_cfg_from_id(rd_regs()->cfg));
              d_push_cmd((D_CmdKind)kind, &params);
            }
            
            // rjf: try to open tabs for "view driver" commands
#if 0 // TODO(rjf): @cfg
            RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(cmd->name);
            if(view_rule_info != &rd_nil_view_rule_info)
            {
              rd_cmd(RD_CmdKind_OpenTab, .string = str8_zero(), .params_tree = md_tree_from_string(scratch.arena, cmd->name)->first);
            }
#endif
          }break;
          
          //- rjf: top-level lister
          case RD_CmdKind_OpenLister:
          {
            RD_ListerFlags lister_flags = (RD_ListerFlag_LineEdit|
                                           RD_ListerFlag_Descriptions|
                                           RD_ListerFlag_KindLabel|
                                           RD_ListerFlag_Procedures|
                                           RD_ListerFlag_Files|
                                           RD_ListerFlag_Commands|
                                           RD_ListerFlag_Settings|
                                           RD_ListerFlag_SystemProcesses);
            rd_cmd(RD_CmdKind_PushQuery, .lister_flags = lister_flags);
          }break;
          
          //- rjf: command fast path
          case RD_CmdKind_RunCommand:
          {
            RD_CmdKindInfo *info = rd_cmd_kind_info_from_string(cmd->regs->cmd_name);
            
            // rjf: command does not have a query - simply execute with the current registers
            if(!(info->query.flags & RD_QueryFlag_Required))
            {
              RD_RegsScope(.cmd_name = str8_zero()) rd_push_cmd(cmd->regs->cmd_name, rd_regs());
            }
            
            // rjf: command has required query -> prep query
            else
            {
              rd_cmd(RD_CmdKind_PushQuery, .lister_flags = rd_regs()->lister_flags|RD_ListerFlag_LineEdit|RD_ListerFlag_Commands|RD_ListerFlag_Descriptions);
            }
          }break;
          
          //- rjf: exiting
          case RD_CmdKind_Exit:
          {
            // rjf: if control processes are live, but this is not force-confirmed, then
            // get confirmation from user
            CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
            UI_Key key = ui_key_from_string(ui_key_zero(), str8_lit("lossy_exit_confirmation"));
            if(processes.count != 0 && !rd_regs()->force_confirm && !ui_key_match(rd_state->popup_key, key))
            {
              rd_state->popup_key = key;
              rd_state->popup_active = 1;
              arena_clear(rd_state->popup_arena);
              MemoryZeroStruct(&rd_state->popup_cmds);
              rd_state->popup_title = push_str8f(rd_state->popup_arena, "Are you sure you want to exit?");
              rd_state->popup_desc = push_str8f(rd_state->popup_arena, "The debugger is still attached to %slive process%s.",
                                                processes.count == 1 ? "a " : "",
                                                processes.count == 1 ? ""   : "es");
              RD_Regs *regs = rd_regs_copy(rd_frame_arena(), rd_regs());
              regs->force_confirm = 1;
              rd_cmd_list_push_new(rd_state->popup_arena, &rd_state->popup_cmds, rd_cmd_kind_info_table[RD_CmdKind_Exit].string, regs);
            }
            
            // rjf: otherwise, actually exit
            else
            {
              rd_cmd(RD_CmdKind_WriteUserData);
              rd_cmd(RD_CmdKind_WriteProjectData);
              rd_state->quit = 1;
            }
          }break;
          
          //- rjf: windows
          case RD_CmdKind_OpenWindow:
          {
            RD_Cfg *old_window = rd_cfg_from_id(rd_regs()->window);
            RD_Cfg *bucket = old_window->parent;
            if(bucket == &rd_nil_cfg)
            {
              bucket = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("user"));
            }
            RD_Cfg *new_window = rd_cfg_new(bucket, str8_lit("window"));
            RD_Cfg *size = rd_cfg_new(new_window, str8_lit("size"));
            rd_cfg_newf(size, "1280");
            rd_cfg_newf(size, "720");
            for(RD_Cfg *old_child = old_window->first; old_child != &rd_nil_cfg; old_child = old_child->next)
            {
              if(!str8_match(old_child->string, str8_lit("panels"), 0) &&
                 !str8_match(old_child->string, str8_lit("size"), 0))
              {
                RD_Cfg *new_child = rd_cfg_deep_copy(old_child);
                rd_cfg_insert_child(new_window, new_window->last, new_child);
              }
            }
            RD_Cfg *panels = rd_cfg_new(new_window, str8_lit("panels"));
            rd_cfg_new(panels, str8_lit("selected"));
          }break;
          case RD_CmdKind_CloseWindow:
          {
            RD_Cfg *wcfg = rd_cfg_from_id(rd_regs()->window);
            rd_cfg_release(wcfg);
          }break;
          case RD_CmdKind_ToggleFullscreen:
          {
            RD_Cfg *wcfg = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(wcfg);
            if(ws != &rd_nil_window_state)
            {
              os_window_set_fullscreen(ws->os, !os_window_is_fullscreen(ws->os));
            }
          }break;
          case RD_CmdKind_BringToFront:
          {
            RD_Cfg *last_focused_wcfg = rd_cfg_from_id(rd_state->last_focused_window);
            RD_WindowState *last_focused_ws = rd_window_state_from_cfg(last_focused_wcfg);
            if(last_focused_ws == &rd_nil_window_state)
            {
              last_focused_ws = rd_state->first_window_state;
            }
            if(last_focused_ws != &rd_nil_window_state)
            {
              os_window_set_minimized(last_focused_ws->os, 0);
              os_window_focus(last_focused_ws->os);
            }
          }break;
          
          //- rjf: confirmations
          case RD_CmdKind_PopupAccept:
          {
            rd_state->popup_active = 0;
            rd_state->popup_key = ui_key_zero();
            for(RD_CmdNode *n = rd_state->popup_cmds.first; n != 0; n = n->next)
            {
              rd_push_cmd(n->cmd.name, n->cmd.regs);
            }
          }break;
          case RD_CmdKind_PopupCancel:
          {
            rd_state->popup_active = 0;
            rd_state->popup_key = ui_key_zero();
          }break;
          
          //- rjf: config path saving/loading/applying
          case RD_CmdKind_OpenRecentProject:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            if(str8_match(cfg->string, str8_lit("recent_project"), 0))
            {
              rd_cmd(RD_CmdKind_OpenProject, .file_path = cfg->first->string);
            }
          }break;
          case RD_CmdKind_OpenUser:
          case RD_CmdKind_OpenProject:
          {
            String8 file_root_key = (kind == RD_CmdKind_OpenUser ? str8_lit("user") : str8_lit("project"));
            RD_Cfg *file_root = rd_cfg_child_from_string(rd_state->root_cfg, file_root_key);
            
            //- rjf: load the new file's data
            String8 file_path = rd_regs()->file_path;
            String8 file_data = os_data_from_file_path(scratch.arena, file_path);
            
            //- rjf: determine if the file is good
            B32 file_is_okay = 0;
            {
              FileProperties file_props = os_properties_from_file_path(file_path);
              file_is_okay = ((file_props.size == 0 && file_props.created == 0) ||
                              str8_match(str8_prefix(file_data, 9), str8_lit("// raddbg"), 0));
            }
            
            //- rjf: bad file -> alert user
            if(!file_is_okay)
            {
              log_user_errorf("\"%S\" appears to refer to an existing file which is not a RADDBG config file. This would overwrite the file.", file_path);
            }
            
            //- rjf: eliminate all old state under this file tree
            if(file_is_okay)
            {
              rd_cfg_release_all_children(file_root);
            }
            
            //- rjf: parse the new file, generate cfg entities for it
            RD_CfgList file_cfg_list = {0};
            if(file_is_okay)
            {
              file_cfg_list = rd_cfg_tree_list_from_string(scratch.arena, file_data);
            }
            
            //- rjf: store path
            if(file_is_okay)
            {
              switch(kind)
              {
                default:{}break;
                case RD_CmdKind_OpenUser:
                {
                  arena_clear(rd_state->user_path_arena);
                  rd_state->user_path = push_str8_copy(rd_state->user_path_arena, file_path);
                }break;
                case RD_CmdKind_OpenProject:
                {
                  arena_clear(rd_state->project_path_arena);
                  rd_state->project_path = push_str8_copy(rd_state->project_path_arena, file_path);
                }break;
              }
            }
            
            //- rjf: insert the new cfg entities into this file tree
            if(file_is_okay)
            {
              for(RD_CfgNode *n = file_cfg_list.first; n != 0; n = n->next)
              {
                rd_cfg_insert_child(file_root, file_root->last, n->v);
              }
            }
            
            //- rjf: if config did not open any windows for the user, then we need to open a sensible default
            if(file_is_okay && kind == RD_CmdKind_OpenUser)
            {
              RD_CfgList all_user_windows = rd_cfg_child_list_from_string(scratch.arena, file_root, str8_lit("window"));
              if(all_user_windows.count == 0)
              {
                OS_Handle monitor    = os_primary_monitor();
                String8 monitor_name = os_name_from_monitor(scratch.arena, monitor);
                Vec2F32 monitor_dim  = os_dim_from_monitor(monitor);
                F32 monitor_dpi      = os_dpi_from_monitor(monitor);
                Vec2F32 window_dim   = v2f32(monitor_dim.x*4/5, monitor_dim.y*4/5);
                RD_Cfg *new_window = rd_cfg_new(file_root, str8_lit("window"));
                RD_Cfg *size = rd_cfg_new(new_window, str8_lit("size"));
                rd_cfg_newf(size, "%f", window_dim.x);
                rd_cfg_newf(size, "%f", window_dim.y);
                F32 line_height_guess = 11.f * (monitor_dpi / 96.f);
                F32 num_lines_in_monitor_height = monitor_dim.y / line_height_guess;
                if(num_lines_in_monitor_height < 100)
                {
                  rd_cmd(RD_CmdKind_ResetToCompactPanels, .window = new_window->id);
                }
                else
                {
                  rd_cmd(RD_CmdKind_ResetToDefaultPanels, .window = new_window->id);
                }
              }
            }
            
            //- rjf: record recently-opened projects in the user
            if(file_is_okay && kind == RD_CmdKind_OpenProject)
            {
              RD_Cfg *user = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("user"));
              RD_CfgList recent_projects = rd_cfg_child_list_from_string(scratch.arena, user, str8_lit("recent_project"));
              RD_Cfg *recent_project = &rd_nil_cfg;
              for(RD_CfgNode *n = recent_projects.first; n != 0; n = n->next)
              {
                if(path_match_normalized(n->v->string, file_path))
                {
                  recent_project = n->v;
                  break;
                }
              }
              if(recent_project == &rd_nil_cfg)
              {
                recent_project = rd_cfg_new(user, str8_lit("recent_project"));
                rd_cfg_new(recent_project, path_normalized_from_string(scratch.arena, file_path));
              }
              rd_cfg_unhook(user, recent_project);
              rd_cfg_insert_child(user, &rd_nil_cfg, recent_project);
              recent_projects = rd_cfg_child_list_from_string(scratch.arena, user, str8_lit("recent_project"));
              if(recent_projects.count > 32)
              {
                rd_cfg_release(recent_projects.last->v);
              }
            }
            
            //- TODO(rjf): @cfg set up debugging config state
            if(kind == RD_CmdKind_OpenUser)
            {
              RD_Cfg *user = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("user"));
              {
                RD_Cfg *watch = rd_cfg_new(user, str8_lit("watch"));
                RD_Cfg *expr = rd_cfg_new(watch, str8_lit("expression"));
                rd_cfg_new(expr, str8_lit("current_thread"));
              }
              {
                RD_Cfg *watch = rd_cfg_new(user, str8_lit("watch"));
                RD_Cfg *expr = rd_cfg_new(watch, str8_lit("expression"));
                rd_cfg_new(expr, str8_lit("targets[0]"));
              }
              {
                RD_Cfg *watch = rd_cfg_new(user, str8_lit("watch"));
                RD_Cfg *expr = rd_cfg_new(watch, str8_lit("expression"));
                rd_cfg_new(expr, str8_lit("basics"));
              }
            }
          }break;
          
          //- rjf: writing config changes
          case RD_CmdKind_WriteUserData:
          case RD_CmdKind_WriteProjectData:
          {
#if 0 // TODO(rjf): @cfg
            RD_CfgSrc src = RD_CfgSrc_User;
            for(RD_CfgSrc s = (RD_CfgSrc)0; s < RD_CfgSrc_COUNT; s = (RD_CfgSrc)(s+1))
            {
              if(kind == rd_cfg_src_write_cmd_kind_table[s])
              {
                src = s;
                break;
              }
            }
            String8 path = rd_cfg_path_from_src(src);
            String8List rd_strs = rd_cfg_strings_from_gfx(scratch.arena, path, src);
            String8 header = push_str8f(scratch.arena, "// raddbg %s file\n\n", rd_cfg_src_string_table[src].str);
            String8List strs = {0};
            str8_list_push(scratch.arena, &strs, header);
            str8_list_concat_in_place(&strs, &rd_strs);
            String8 data = str8_list_join(scratch.arena, &strs, 0);
            String8 data_indented = indented_from_string(scratch.arena, data);
            os_write_data_to_file_path(path, data_indented);
#endif
          }break;
          
          //- rjf: code navigation
          case RD_CmdKind_FindTextForward:
          case RD_CmdKind_FindTextBackward:
          {
            rd_set_search_string(rd_regs()->string);
          }break;
          
          //- rjf: find next and find prev
          case RD_CmdKind_FindNext:
          {
            rd_cmd(RD_CmdKind_FindTextForward, .string = rd_push_search_string(scratch.arena));
          }break;
          case RD_CmdKind_FindPrev:
          {
            rd_cmd(RD_CmdKind_FindTextBackward, .string = rd_push_search_string(scratch.arena));
          }break;
          
          //- rjf: font sizes
          case RD_CmdKind_IncUIFontScale:
          {
            fnt_reset();
            F32 current_font_size = rd_font_size_from_slot(RD_FontSlot_Main);
            F32 new_font_size = clamp_1f32(r1f32(6, 72), current_font_size+1);
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_Cfg *main_font_size = rd_cfg_child_from_string_or_alloc(window, str8_lit("main_font_size"));
            rd_cfg_new_replacef(main_font_size, "%f", new_font_size);
          }break;
          case RD_CmdKind_DecUIFontScale:
          {
            fnt_reset();
            F32 current_font_size = rd_font_size_from_slot(RD_FontSlot_Main);
            F32 new_font_size = clamp_1f32(r1f32(6, 72), current_font_size-1);
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_Cfg *main_font_size = rd_cfg_child_from_string_or_alloc(window, str8_lit("main_font_size"));
            rd_cfg_new_replacef(main_font_size, "%f", new_font_size);
          }break;
          case RD_CmdKind_IncCodeFontScale:
          {
            fnt_reset();
            F32 current_font_size = rd_font_size_from_slot(RD_FontSlot_Code);
            F32 new_font_size = clamp_1f32(r1f32(6, 72), current_font_size+1);
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_Cfg *code_font_size = rd_cfg_child_from_string_or_alloc(window, str8_lit("code_font_size"));
            rd_cfg_new_replacef(code_font_size, "%f", new_font_size);
          }break;
          case RD_CmdKind_DecCodeFontScale:
          {
            fnt_reset();
            F32 current_font_size = rd_font_size_from_slot(RD_FontSlot_Code);
            F32 new_font_size = clamp_1f32(r1f32(6, 72), current_font_size-1);
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_Cfg *code_font_size = rd_cfg_child_from_string_or_alloc(window, str8_lit("code_font_size"));
            rd_cfg_new_replacef(code_font_size, "%f", new_font_size);
          }break;
          
          //- rjf: panel creation
          case RD_CmdKind_NewPanelLeft: {split_dir = Dir2_Left;}goto split;
          case RD_CmdKind_NewPanelUp:   {split_dir = Dir2_Up;}goto split;
          case RD_CmdKind_NewPanelRight:{split_dir = Dir2_Right;}goto split;
          case RD_CmdKind_NewPanelDown: {split_dir = Dir2_Down;}goto split;
          case RD_CmdKind_SplitPanel:
          {
            split_dir = rd_regs()->dir2;
            split_panel = rd_cfg_from_id(rd_regs()->dst_panel);
          }goto split;
          split:;
          if(split_dir != Dir2_Invalid)
          {
            // rjf: unpack
            Axis2 split_axis = axis2_from_dir2(split_dir);
            Side split_side = side_from_dir2(split_dir);
            RD_Cfg *new_panel_cfg = &rd_nil_cfg;
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, split_panel);
            RD_PanelNode *panel_root = panel_tree.root;
            RD_PanelNode *panel = rd_panel_node_from_tree_cfg(panel_root, split_panel);
            RD_PanelNode *parent = panel->parent;
            
            // rjf: splitting on same axis as parent -> insert new sibling on same axis, adjust sizes
            if(parent != &rd_nil_panel_node && parent->split_axis == split_axis)
            {
              RD_Cfg *parent_cfg = parent->cfg;
              RD_Cfg *panel_cfg = panel->cfg;
              RD_Cfg *new_cfg = rd_cfg_alloc();
              rd_cfg_insert_child(parent_cfg, split_side == Side_Max ? panel_cfg : panel_cfg->prev, new_cfg);
              rd_cfg_equip_stringf(new_cfg, "%f", 1.f/parent->child_count);
              for(RD_PanelNode *child = parent->first; child != &rd_nil_panel_node; child = child->next)
              {
                F32 old_pct = child->pct_of_parent;
                F32 new_pct = old_pct * ((F32)(parent->child_count) / (parent->child_count+1));
                rd_cfg_equip_stringf(child->cfg, "%f", new_pct);
              }
              new_panel_cfg = new_cfg;
            }
            
            // rjf: splitting on opposite axis as parent - need to create new replacement node, + new sibling
            else
            {
              RD_Cfg *split_panel_prev = panel->prev->cfg;
              RD_Cfg *new_parent = rd_cfg_alloc();
              RD_Cfg *new_sibling = rd_cfg_alloc();
              rd_cfg_equip_string(new_parent, split_panel->string);
              rd_cfg_equip_string(split_panel, str8_lit("0.5"));
              rd_cfg_equip_string(new_sibling, str8_lit("0.5"));
              if(parent->cfg != &rd_nil_cfg)
              {
                rd_cfg_unhook(parent->cfg, split_panel);
                rd_cfg_insert_child(parent->cfg, split_panel_prev, new_parent);
              }
              else
              {
                rd_cfg_equip_string(new_parent, str8_lit("panels"));
                RD_Cfg *window_cfg = rd_window_from_cfg(split_panel);
                rd_cfg_insert_child(window_cfg, window_cfg->last, new_parent);
              }
              RD_Cfg *min = split_panel;
              RD_Cfg *max = new_sibling;
              if(split_side == Side_Min)
              {
                Swap(RD_Cfg *, min, max);
              }
              rd_cfg_insert_child(new_parent, new_parent->last, min);
              rd_cfg_insert_child(new_parent, new_parent->last, max);
              new_panel_cfg = new_sibling;
            }
            
            // rjf: pre-emptively set up the animation rectangle, depending on where
            // the new panel was inserted (?)
#if 0 // TODO(rjf): @cfg
            if(!rd_panel_is_nil(new_panel->prev))
            {
              Rng2F32 prev_rect_pct = new_panel->prev->animated_rect_pct;
              new_panel->animated_rect_pct = prev_rect_pct;
              new_panel->animated_rect_pct.p0.v[split_axis] = new_panel->animated_rect_pct.p1.v[split_axis];
            }
            if(!rd_panel_is_nil(new_panel->next))
            {
              Rng2F32 next_rect_pct = new_panel->next->animated_rect_pct;
              new_panel->animated_rect_pct = next_rect_pct;
              new_panel->animated_rect_pct.p1.v[split_axis] = new_panel->animated_rect_pct.p0.v[split_axis];
            }
#endif
            
            // rjf: if this split was caused by drag/dropping a tab, and the originating panel
            // has no further tabs, then close the originating panel
            RD_Cfg *dragdrop_origin_panel_cfg = rd_cfg_from_id(rd_regs()->panel);
            RD_Cfg *dragdrop_tab = rd_cfg_from_id(rd_regs()->view);
            if(kind == RD_CmdKind_SplitPanel &&
               new_panel_cfg != &rd_nil_cfg && dragdrop_tab != &rd_nil_cfg && dragdrop_origin_panel_cfg != &rd_nil_cfg)
            {
              rd_cfg_unhook(dragdrop_origin_panel_cfg, dragdrop_tab);
              rd_cfg_insert_child(new_panel_cfg, new_panel_cfg->last, dragdrop_tab);
              RD_PanelTree origin_panel_tree = rd_panel_tree_from_cfg(scratch.arena, dragdrop_origin_panel_cfg);
              RD_PanelNode *origin_panel = rd_panel_node_from_tree_cfg(origin_panel_tree.root, dragdrop_origin_panel_cfg);
              if(origin_panel->tabs.count == 0)
              {
                rd_cmd(RD_CmdKind_ClosePanel);
              }
            }
            
            // rjf: focus new panel
            if(new_panel_cfg != &rd_nil_cfg)
            {
              rd_cmd(RD_CmdKind_FocusPanel, .panel = new_panel_cfg->id);
            }
          }break;
          
          //- rjf: panel rotation
          case RD_CmdKind_RotatePanelColumns:
          {
            RD_Cfg *panel_cfg = rd_cfg_from_id(rd_regs()->panel);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, panel_cfg);
            RD_PanelNode *panel = rd_panel_node_from_tree_cfg(panel_tree.root, panel_cfg);
            RD_PanelNode *parent = &rd_nil_panel_node;
            for(RD_PanelNode *p = panel->parent; p != &rd_nil_panel_node; p = p->parent)
            {
              if(p->split_axis == Axis2_X)
              {
                parent = p;
                break;
              }
            }
            if(parent != &rd_nil_panel_node && parent->child_count > 1)
            {
              RD_Cfg *rotated = parent->first->cfg;
              rd_cfg_unhook(parent->cfg, parent->first->cfg);
              rd_cfg_insert_child(parent->cfg, parent->last->cfg, rotated);
            }
          }break;
          
          //- rjf: panel focusing
          case RD_CmdKind_NextPanel: panel_sib_off = OffsetOf(RD_PanelNode, next); panel_child_off = OffsetOf(RD_PanelNode, first); goto cycle;
          case RD_CmdKind_PrevPanel: panel_sib_off = OffsetOf(RD_PanelNode, prev); panel_child_off = OffsetOf(RD_PanelNode, last); goto cycle;
          cycle:;
          {
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, rd_cfg_from_id(rd_regs()->window));
            RD_PanelNode *next_focused = &rd_nil_panel_node;
            for(RD_PanelNode *p = panel_tree.focused;
                p != &rd_nil_panel_node;
                p = rd_panel_node_rec__depth_first(panel_tree.root, p, panel_sib_off, panel_child_off).next)
            {
              if(p->first == &rd_nil_panel_node)
              {
                next_focused = p;
                break;
              }
            }
            if(next_focused == &rd_nil_panel_node)
            {
              for(RD_PanelNode *p = panel_tree.root;
                  p != &rd_nil_panel_node;
                  p = rd_panel_node_rec__depth_first(panel_tree.root, p, panel_sib_off, panel_child_off).next)
              {
                if(p->first == &rd_nil_panel_node)
                {
                  next_focused = p;
                }
              }
            }
            rd_cmd(RD_CmdKind_FocusPanel, .panel = next_focused->cfg->id);
          }break;
          case RD_CmdKind_FocusPanel:
          {
            RD_Cfg *panel = rd_cfg_from_id(rd_regs()->panel);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, panel);
            RD_Cfg *selection_cfg = &rd_nil_cfg;
            for(RD_PanelNode *p = panel_tree.root;
                p != &rd_nil_panel_node;
                p = rd_panel_node_rec__depth_first_pre(panel_tree.root, p).next)
            {
              RD_Cfg *p_cfg = p->cfg;
              RD_Cfg *p_selection = rd_cfg_child_from_string(p_cfg, str8_lit("selected"));
              if(selection_cfg == &rd_nil_cfg)
              {
                selection_cfg = p_selection;
              }
              else
              {
                rd_cfg_release(p_selection);
              }
            }
            if(selection_cfg == &rd_nil_cfg)
            {
              selection_cfg = rd_cfg_alloc();
              rd_cfg_equip_string(selection_cfg, str8_lit("selected"));
            }
            if(panel != &rd_nil_cfg)
            {
              rd_cfg_insert_child(panel, &rd_nil_cfg, selection_cfg);
              RD_Cfg *window = rd_window_from_cfg(panel);
              RD_WindowState *ws = rd_window_state_from_cfg(window);
              ws->menu_bar_focused = 0;
            }
          }break;
          
          //- rjf: directional panel focus changing
          case RD_CmdKind_FocusPanelRight: panel_change_dir = v2s32(+1, +0); goto focus_panel_dir;
          case RD_CmdKind_FocusPanelLeft:  panel_change_dir = v2s32(-1, +0); goto focus_panel_dir;
          case RD_CmdKind_FocusPanelUp:    panel_change_dir = v2s32(+0, -1); goto focus_panel_dir;
          case RD_CmdKind_FocusPanelDown:  panel_change_dir = v2s32(+0, +1); goto focus_panel_dir;
          focus_panel_dir:;
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
            RD_PanelNode *src_panel = panel_tree.focused;
            Rng2F32 src_panel_rect = rd_target_rect_from_panel_node(r2f32(v2f32(0, 0), v2f32(1000, 1000)), panel_tree.root, src_panel);
            Vec2F32 src_panel_center = center_2f32(src_panel_rect);
            Vec2F32 src_panel_half_dim = scale_2f32(dim_2f32(src_panel_rect), 0.5f);
            Vec2F32 travel_dim = add_2f32(src_panel_half_dim, v2f32(10.f, 10.f));
            Vec2F32 travel_dst = add_2f32(src_panel_center, mul_2f32(travel_dim, v2f32((F32)panel_change_dir.x, (F32)panel_change_dir.y)));
            RD_PanelNode *dst_root = &rd_nil_panel_node;
            for(RD_PanelNode *p = panel_tree.root; p != &rd_nil_panel_node; p = rd_panel_node_rec__depth_first_pre(panel_tree.root, p).next)
            {
              if(p == src_panel || p->first != &rd_nil_panel_node)
              {
                continue;
              }
              Rng2F32 p_rect = rd_target_rect_from_panel_node(r2f32(v2f32(0, 0), v2f32(1000, 1000)), panel_tree.root, p);
              if(contains_2f32(p_rect, travel_dst))
              {
                dst_root = p;
                break;
              }
            }
            if(dst_root != &rd_nil_panel_node)
            {
              RD_PanelNode *dst_panel = &rd_nil_panel_node;
              for(RD_PanelNode *p = dst_root; p != &rd_nil_panel_node; p = rd_panel_node_rec__depth_first_pre(dst_root, p).next)
              {
                if(p->first == &rd_nil_panel_node && p != src_panel)
                {
                  dst_panel = p;
                  break;
                }
              }
              rd_cmd(RD_CmdKind_FocusPanel, .panel = dst_panel->cfg->id);
            }
          }break;
          
          //- rjf: undo/redo
          case RD_CmdKind_Undo:{}break;
          case RD_CmdKind_Redo:{}break;
          
          //- rjf: focus history
          case RD_CmdKind_GoBack:{}break;
          case RD_CmdKind_GoForward:{}break;
          
          //- rjf: files
          case RD_CmdKind_SetCurrentPath:
          {
            arena_clear(rd_state->current_path_arena);
            rd_state->current_path = push_str8_copy(rd_state->current_path_arena, rd_regs()->file_path);
          }break;
          case RD_CmdKind_SetFileReplacementPath:
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
            String8 src_path = rd_regs()->string;
            String8 dst_path = rd_regs()->file_path;
            String8 src_path__normalized = path_normalized_from_string(scratch.arena, src_path);
            String8 dst_path__normalized = path_normalized_from_string(scratch.arena, dst_path);
            String8List src_path_parts = str8_split_path(scratch.arena, src_path__normalized);
            String8List dst_path_parts = str8_split_path(scratch.arena, dst_path__normalized);
            
            //- rjf: reverse path parts
            String8List src_path_parts__reversed = {0};
            String8List dst_path_parts__reversed = {0};
            for(String8Node *n = src_path_parts.first; n != 0; n = n->next)
            {
              str8_list_push_front(scratch.arena, &src_path_parts__reversed, n->string);
            }
            for(String8Node *n = dst_path_parts.first; n != 0; n = n->next)
            {
              str8_list_push_front(scratch.arena, &dst_path_parts__reversed, n->string);
            }
            
            //- rjf: trace from each path upwards, in lock-step, to find the first difference
            // between the paths
            String8Node *first_diff_src = src_path_parts__reversed.first;
            String8Node *first_diff_dst = dst_path_parts__reversed.first;
            for(;first_diff_src != 0 && first_diff_dst != 0;)
            {
              if(!str8_match(first_diff_src->string, first_diff_dst->string, StringMatchFlag_CaseInsensitive))
              {
                break;
              }
              first_diff_src = first_diff_src->next;
              first_diff_dst = first_diff_dst->next;
            }
            
            //- rjf: form final map paths
            String8List map_src_parts = {0};
            String8List map_dst_parts = {0};
            for(String8Node *n = first_diff_src; n != 0; n = n->next)
            {
              str8_list_push_front(scratch.arena, &map_src_parts, n->string);
            }
            for(String8Node *n = first_diff_dst; n != 0; n = n->next)
            {
              str8_list_push_front(scratch.arena, &map_dst_parts, n->string);
            }
            StringJoin map_join = {.sep = str8_lit("/")};
            String8 map_src = str8_list_join(scratch.arena, &map_src_parts, &map_join);
            String8 map_dst = str8_list_join(scratch.arena, &map_dst_parts, &map_join);
            
            //- rjf: store as file path map cfg
            RD_Cfg *user = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("user"));
            RD_Cfg *map = rd_cfg_new(user, str8_lit("file_path_map"));
            RD_Cfg *src = rd_cfg_new(map, str8_lit("source"));
            RD_Cfg *dst = rd_cfg_new(map, str8_lit("dest"));
            rd_cfg_new(src, map_src);
            rd_cfg_new(dst, map_dst);
          }break;
          
          //- rjf: panel removal
          case RD_CmdKind_ClosePanel:
          {
#if 0 // TODO(rjf): @cfg
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_Panel *parent = panel->parent;
            if(!rd_panel_is_nil(parent))
            {
              Axis2 split_axis = parent->split_axis;
              
              // NOTE(rjf): If we're removing all but the last child of this parent,
              // we should just remove both children.
              if(parent->child_count == 2)
              {
                RD_Panel *discard_child = panel;
                RD_Panel *keep_child = panel == parent->first ? parent->last : parent->first;
                RD_Panel *grandparent = parent->parent;
                RD_Panel *parent_prev = parent->prev;
                F32 pct_of_parent = parent->pct_of_parent;
                
                // rjf: unhook kept child
                rd_panel_remove(parent, keep_child);
                
                // rjf: unhook this subtree
                if(!rd_panel_is_nil(grandparent))
                {
                  rd_panel_remove(grandparent, parent);
                }
                
                // rjf: release the things we should discard
                {
                  rd_panel_release(ws, parent);
                  rd_panel_release(ws, discard_child);
                }
                
                // rjf: re-hook our kept child into the overall tree
                if(rd_panel_is_nil(grandparent))
                {
                  ws->root_panel = keep_child;
                }
                else
                {
                  rd_panel_insert(grandparent, parent_prev, keep_child);
                }
                keep_child->pct_of_parent = pct_of_parent;
                
                // rjf: reset focus, if needed
                if(ws->focused_panel == discard_child)
                {
                  ws->focused_panel = keep_child;
                  for(RD_Panel *grandchild = ws->focused_panel; !rd_panel_is_nil(grandchild); grandchild = grandchild->first)
                  {
                    ws->focused_panel = grandchild;
                  }
                }
                
                // rjf: keep-child split-axis == grandparent split-axis? bubble keep-child up into grandparent's children
                if(!rd_panel_is_nil(grandparent) && grandparent->split_axis == keep_child->split_axis && !rd_panel_is_nil(keep_child->first))
                {
                  rd_panel_remove(grandparent, keep_child);
                  RD_Panel *prev = parent_prev;
                  for(RD_Panel *child = keep_child->first, *next = 0; !rd_panel_is_nil(child); child = next)
                  {
                    next = child->next;
                    rd_panel_remove(keep_child, child);
                    rd_panel_insert(grandparent, prev, child);
                    prev = child;
                    child->pct_of_parent *= keep_child->pct_of_parent;
                  }
                  rd_panel_release(ws, keep_child);
                }
              }
              // NOTE(rjf): Otherwise we can just remove this child.
              else
              {
                RD_Panel *next = &rd_nil_panel;
                F32 removed_size_pct = panel->pct_of_parent;
                if(rd_panel_is_nil(next)) { next = panel->prev; }
                if(rd_panel_is_nil(next)) { next = panel->next; }
                rd_panel_remove(parent, panel);
                rd_panel_release(ws, panel);
                if(ws->focused_panel == panel)
                {
                  ws->focused_panel = next;
                  for(RD_Panel *grandchild = ws->focused_panel; !rd_panel_is_nil(grandchild); grandchild = grandchild->first)
                  {
                    ws->focused_panel = grandchild;
                  }
                }
                for(RD_Panel *child = parent->first; !rd_panel_is_nil(child); child = child->next)
                {
                  child->pct_of_parent /= 1.f-removed_size_pct;
                }
              }
            }
#endif
          }break;
          
          //- rjf: panel tab controls
          case RD_CmdKind_FocusTab:
          {
            RD_Cfg *tab = rd_cfg_from_id(rd_regs()->view);
            RD_Cfg *panel = tab->parent;
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, panel);
            RD_PanelNode *panel_node = rd_panel_node_from_tree_cfg(panel_tree.root, panel);
            RD_Cfg *selection_cfg = &rd_nil_cfg;
            for(RD_CfgNode *n = panel_node->tabs.first; n != 0; n = n->next)
            {
              RD_Cfg *tab_selection_cfg = rd_cfg_child_from_string(n->v, str8_lit("selected"));
              if(selection_cfg == &rd_nil_cfg)
              {
                selection_cfg = tab_selection_cfg;
              }
              else
              {
                rd_cfg_release(tab_selection_cfg);
              }
            }
            if(selection_cfg == &rd_nil_cfg)
            {
              selection_cfg = rd_cfg_alloc();
              rd_cfg_equip_string(selection_cfg, str8_lit("selected"));
            }
            rd_cfg_insert_child(tab, &rd_nil_cfg, selection_cfg);
          }break;
          case RD_CmdKind_NextTab:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
            RD_PanelNode *focused = panel_tree.focused;
            RD_CfgNode *selected_tab_n = 0;
            for(RD_CfgNode *n = focused->tabs.first; n != 0; n = n->next)
            {
              if(n->v == focused->selected_tab)
              {
                selected_tab_n = n;
                break;
              }
            }
            RD_Cfg *next_selected_tab = &rd_nil_cfg;
            U64 idx = 0;
            for(RD_CfgNode *tab_n = selected_tab_n;
                tab_n != 0 && (tab_n != selected_tab_n || idx == 0);
                ((tab_n->next == 0) ? (tab_n = focused->tabs.first) : (tab_n = tab_n->next)), idx += 1)
            {
              if(!rd_cfg_is_project_filtered(tab_n->v) && tab_n != selected_tab_n)
              {
                next_selected_tab = tab_n->v;
                break;
              }
            }
            if(next_selected_tab != &rd_nil_cfg)
            {
              rd_cmd(RD_CmdKind_FocusTab, .view = next_selected_tab->id);
            }
          }break;
          case RD_CmdKind_PrevTab:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
            RD_PanelNode *focused = panel_tree.focused;
            RD_CfgNode *selected_tab_n = 0;
            for(RD_CfgNode *n = focused->tabs.last; n != 0; n = n->prev)
            {
              if(n->v == focused->selected_tab)
              {
                selected_tab_n = n;
                break;
              }
            }
            RD_Cfg *next_selected_tab = &rd_nil_cfg;
            U64 idx = 0;
            for(RD_CfgNode *tab_n = selected_tab_n;
                tab_n != 0 && (tab_n != selected_tab_n || idx == 0);
                ((tab_n->prev == 0) ? (tab_n = focused->tabs.last) : (tab_n = tab_n->prev)), idx += 1)
            {
              if(!rd_cfg_is_project_filtered(tab_n->v) && tab_n != selected_tab_n)
              {
                next_selected_tab = tab_n->v;
                break;
              }
            }
            if(next_selected_tab != &rd_nil_cfg)
            {
              rd_cmd(RD_CmdKind_FocusTab, .view = next_selected_tab->id);
            }
          }break;
          case RD_CmdKind_MoveTabRight:
          case RD_CmdKind_MoveTabLeft:
          {
#if 0 // TODO(rjf): @cfg
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *panel = ws->focused_panel;
            RD_View *view = rd_selected_tab_from_panel(panel);
            RD_View *prev_view = (kind == RD_CmdKind_MoveTabRight ? view->order_next : view->order_prev->order_prev);
            if(!rd_view_is_nil(prev_view) || kind == RD_CmdKind_MoveTabLeft)
            {
              rd_cmd(RD_CmdKind_MoveTab,
                     .panel = rd_handle_from_panel(panel),
                     .dst_panel = rd_handle_from_panel(panel),
                     .view = rd_handle_from_view(view),
                     .prev_view = rd_handle_from_view(prev_view));
            }
#endif
          }break;
          case RD_CmdKind_OpenTab:
          {
#if 0 // TODO(rjf): @cfg
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *view = rd_view_alloc();
            String8 query = rd_regs()->string;
            RD_ViewRuleInfo *spec = rd_view_rule_info_from_string(rd_regs()->params_tree->string);
            rd_view_equip_spec(view, spec, query, rd_regs()->params_tree);
            rd_panel_insert_tab_view(panel, panel->last_tab_view, view);
#endif
          }break;
          case RD_CmdKind_CloseTab:
          {
            RD_Cfg *tab = rd_cfg_from_id(rd_regs()->view);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, tab);
            RD_PanelNode *panel = rd_panel_node_from_tree_cfg(panel_tree.root, tab->parent);
            B32 found_selected = 0;
            RD_Cfg *next_selected_tab = &rd_nil_cfg;
            for(RD_CfgNode *n = panel->tabs.first; n != 0; n = n->next)
            {
              if(n->v == panel->selected_tab)
              {
                found_selected = 1;
              }
              else if(!rd_cfg_is_project_filtered(n->v))
              {
                next_selected_tab = n->v;
                if(found_selected)
                {
                  break;
                }
              }
            }
            rd_cmd(RD_CmdKind_FocusTab, .view = next_selected_tab->id);
            rd_cfg_release(tab);
          }break;
          case RD_CmdKind_MoveTab:
          {
#if 0 // TODO(rjf): @cfg
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *src_panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            RD_Panel *dst_panel = rd_panel_from_handle(rd_regs()->dst_panel);
            RD_View *prev_view = rd_view_from_handle(rd_regs()->prev_view);
            if(!rd_panel_is_nil(src_panel) &&
               !rd_panel_is_nil(dst_panel) &&
               prev_view != view)
            {
              rd_panel_remove_tab_view(src_panel, view);
              rd_panel_insert_tab_view(dst_panel, prev_view, view);
              ws->focused_panel = dst_panel;
              B32 src_panel_is_empty = 1;
              for(RD_View *v = src_panel->first_tab_view; !rd_view_is_nil(v); v = v->order_next)
              {
                if(!rd_view_is_project_filtered(v))
                {
                  src_panel_is_empty = 0;
                  break;
                }
              }
              if(src_panel_is_empty && src_panel != ws->root_panel)
              {
                rd_cmd(RD_CmdKind_ClosePanel, .panel = rd_handle_from_panel(src_panel));
              }
            }
#endif
          }break;
          case RD_CmdKind_TabBarTop:
          {
            RD_Cfg *panel = rd_cfg_from_id(rd_regs()->panel);
            rd_cfg_release(rd_cfg_child_from_string(panel, str8_lit("tabs_on_bottom")));
          }break;
          case RD_CmdKind_TabBarBottom:
          {
            RD_Cfg *panel = rd_cfg_from_id(rd_regs()->panel);
            rd_cfg_child_from_string_or_alloc(panel, str8_lit("tabs_on_bottom"));
          }break;
          
          //- rjf: files
          case RD_CmdKind_Open:
          {
            String8 path = rd_regs()->file_path;
            FileProperties props = os_properties_from_file_path(path);
            if(props.created != 0)
            {
              rd_cmd(RD_CmdKind_RecordFileInProject);
#if 0 // TODO(rjf): @cfg
              rd_cmd(RD_CmdKind_OpenTab,
                     .string = rd_eval_string_from_file_path(scratch.arena, path),
                     .params_tree = md_tree_from_string(scratch.arena, rd_view_rule_kind_info_table[RD_ViewRuleKind_PendingFile].string)->first);
#endif
            }
            else
            {
              log_user_errorf("Couldn't open file at \"%S\".", path);
            }
          }break;
          case RD_CmdKind_Switch:
          {
#if 0 // TODO(rjf): @cfg
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *src_panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *src_view = rd_view_from_handle(rd_regs()->view);
            RD_ViewRuleKind src_view_kind = rd_view_rule_kind_from_string(src_view->spec->string);
            RD_Entity *recent_file = rd_entity_from_handle(rd_regs()->entity);
            if(!rd_entity_is_nil(recent_file))
            {
              String8 recent_file_path = recent_file->string;
              RD_Panel *existing_panel = &rd_nil_panel;
              RD_View *existing_view = &rd_nil_view;
              for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
              {
                if(!rd_panel_is_nil(panel->first))
                {
                  continue;
                }
                for(RD_View *v = panel->first_tab_view; !rd_view_is_nil(v); v = v->order_next)
                {
                  if(rd_view_is_project_filtered(v)) { continue; }
                  String8 v_path = rd_file_path_from_eval_string(scratch.arena, str8(v->query_buffer, v->query_string_size));
                  RD_ViewRuleKind v_kind = rd_view_rule_kind_from_string(v->spec->string);
                  if(str8_match(v_path, recent_file_path, StringMatchFlag_CaseInsensitive) && v_kind == src_view_kind)
                  {
                    existing_panel = panel;
                    existing_view = v;
                    goto done_existing_view_search__switch;
                  }
                }
              }
              done_existing_view_search__switch:;
              if(rd_view_is_nil(existing_view))
              {
                rd_cmd(RD_CmdKind_OpenTab,
                       .string = rd_eval_string_from_file_path(scratch.arena, recent_file_path),
                       .params_tree = md_tree_from_string(scratch.arena, rd_view_rule_kind_info_table[RD_ViewRuleKind_PendingFile].string)->first);
              }
              else
              {
                rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(existing_panel));
                existing_panel->selected_tab_view = rd_handle_from_view(existing_view);
              }
            }
#endif
          }break;
          case RD_CmdKind_SwitchToPartnerFile:
          {
#if 0 // TODO(rjf): @cfg
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *view = rd_selected_tab_from_panel(panel);
            {
              String8 file_path      = rd_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
              String8 file_full_path = path_normalized_from_string(scratch.arena, file_path);
              String8 file_folder    = str8_chop_last_slash(file_full_path);
              String8 file_name      = str8_skip_last_slash(str8_chop_last_dot(file_full_path));
              String8 file_ext       = str8_skip_last_dot(file_full_path);
              String8 partner_ext_candidates[] =
              {
                str8_lit_comp("h"),
                str8_lit_comp("hpp"),
                str8_lit_comp("hxx"),
                str8_lit_comp("c"),
                str8_lit_comp("cc"),
                str8_lit_comp("cxx"),
                str8_lit_comp("cpp"),
              };
              for(U64 idx = 0; idx < ArrayCount(partner_ext_candidates); idx += 1)
              {
                if(!str8_match(partner_ext_candidates[idx], file_ext, StringMatchFlag_CaseInsensitive))
                {
                  String8 candidate = push_str8f(scratch.arena, "%S.%S", file_name, partner_ext_candidates[idx]);
                  String8 candidate_path = push_str8f(scratch.arena, "%S/%S", file_folder, candidate);
                  FileProperties candidate_props = os_properties_from_file_path(candidate_path);
                  if(candidate_props.modified != 0)
                  {
                    RD_Entity *recent_file = rd_entity_from_name_and_kind(candidate_path, RD_EntityKind_RecentFile);
                    if(!rd_entity_is_nil(recent_file))
                    {
                      rd_cmd(RD_CmdKind_Switch, .entity = rd_handle_from_entity(recent_file));
                    }
                    else
                    {
                      rd_cmd(RD_CmdKind_RecordFileInProject, .file_path = candidate_path);
                      rd_cmd(RD_CmdKind_OpenTab, .string = rd_eval_string_from_file_path(scratch.arena, candidate_path), .params_tree = md_tree_from_string(scratch.arena, view->spec->string)->first);
                    }
                    break;
                  }
                }
              }
            }
#endif
          }break;
          case RD_CmdKind_RecordFileInProject:
          if(rd_regs()->file_path.size != 0)
          {
            String8 path = path_normalized_from_string(scratch.arena, rd_regs()->file_path);
            RD_Cfg *project = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("project"));
            RD_CfgList recent_files = rd_cfg_child_list_from_string(scratch.arena, project, str8_lit("recent_files"));
            RD_Cfg *recent_file = &rd_nil_cfg;
            for(RD_CfgNode *n = recent_files.first; n != 0; n = n->next)
            {
              if(path_match_normalized(n->v->string, path))
              {
                recent_file = n->v;
                break;
              }
            }
            if(recent_file == &rd_nil_cfg)
            {
              recent_file = rd_cfg_new(project, str8_lit("recent_file"));
              rd_cfg_new(recent_file, path);
            }
            rd_cfg_unhook(project, recent_file);
            rd_cfg_insert_child(project, &rd_nil_cfg, recent_file);
            recent_files = rd_cfg_child_list_from_string(scratch.arena, project, str8_lit("recent_files"));
            if(recent_files.count > 256)
            {
              rd_cfg_release(recent_files.last->v);
            }
          }break;
          case RD_CmdKind_ShowFileInExplorer:
          if(rd_regs()->file_path.size != 0)
          {
            String8 full_path = path_normalized_from_string(scratch.arena, rd_regs()->file_path);
            os_show_in_filesystem_ui(full_path);
          }break;
          
          //- rjf: source <-> disasm
          case RD_CmdKind_GoToDisassembly:
          {
            CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
            U64 vaddr = 0;
            for(D_LineNode *n = rd_regs()->lines.first; n != 0; n = n->next)
            {
              CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &n->v.dbgi_key);
              CTRL_Entity *module = ctrl_module_from_thread_candidates(d_state->ctrl_entity_store, thread, &modules);
              if(module != &ctrl_entity_nil)
              {
                vaddr = ctrl_vaddr_from_voff(module, n->v.voff_range.min);
                break;
              }
            }
            rd_cmd(RD_CmdKind_FindCodeLocation, .process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process)->handle, .vaddr = vaddr, .prefer_disasm = 1);
          }break;
          case RD_CmdKind_GoToSource:
          {
            if(rd_regs()->lines.first != 0)
            {
              rd_cmd(RD_CmdKind_FindCodeLocation,
                     .file_path = rd_regs()->lines.first->v.file_path,
                     .cursor    = rd_regs()->lines.first->v.pt,
                     .vaddr     = 0,
                     .process   = ctrl_handle_zero(),
                     .prefer_disasm = 0);
            }
          }break;
          
          //- rjf: panel built-in layout builds
          case RD_CmdKind_ResetToDefaultPanels:
          case RD_CmdKind_ResetToCompactPanels:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_Cfg *panels = rd_cfg_child_from_string(window, str8_lit("panels"));
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
            
            //- rjf: define all of the "fixed" tabs we care about
#define FixedTab_XList \
X(watches)\
X(locals)\
X(registers)\
X(globals)\
X(thread_locals)\
X(types)\
X(procedures)\
X(call_stack)\
X(breakpoints)\
X(watch_pins)\
X(targets)\
X(scheduler)\
X(modules)\
Y(output, text, "query:output")\
Y(disasm, disasm, "")\
Y(memory, memory, "")\
Z(getting_started)
#define X(name) RD_Cfg *name = &rd_nil_cfg;
#define Y(name, rule, expr) RD_Cfg *name = &rd_nil_cfg;
#define Z(name) RD_Cfg *name = &rd_nil_cfg;
            FixedTab_XList
#undef X
#undef Y
#undef Z
            
            //- rjf: find all the fixed tabs, and all text viewers
            RD_CfgList texts = {0};
            for(RD_PanelNode *panel = panel_tree.root;
                panel != &rd_nil_panel_node;
                panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
            {
              for(RD_CfgNode *n = panel->tabs.first; n != 0; n = n->next)
              {
                RD_Cfg *tab = n->v;
                B32 need_unhook = 1;
                if(0){}
#define X(name) else if(str8_match(tab->string, str8_lit("watch"), 0) && str8_match(rd_expr_from_cfg(tab), str8_lit("query:" #name), 0)) {name = tab;}
#define Y(name, rule, expr) else if(str8_match(tab->string, str8_lit(#rule), 0) && str8_match(rd_expr_from_cfg(tab), str8_lit(expr), 0)) {name = tab;}
#define Z(name) else if(str8_match(tab->string, str8_lit(#name), 0)) {name = tab;}
                FixedTab_XList
#undef X
#undef Y
#undef Z
                else if(str8_match(tab->string, str8_lit("text"), 0)) {rd_cfg_list_push(scratch.arena, &texts, tab);}
                else
                {
                  need_unhook = 0;
                }
                if(need_unhook)
                {
                  rd_cfg_unhook(panel->cfg, tab);
                }
              }
            }
            
            //- rjf: release the old panel tree
            rd_cfg_release(panels);
            
            //- rjf: allocate any missing tabs
#define X(name) if(name == &rd_nil_cfg) {name = rd_cfg_alloc(); rd_cfg_equip_string(name, str8_lit("watch")); RD_Cfg *expr_cfg = rd_cfg_new(name, str8_lit("expression")); rd_cfg_new(expr_cfg, str8_lit("query:" #name));}
#define Y(name, rule, expr) if(name == &rd_nil_cfg) {name = rd_cfg_alloc(); rd_cfg_equip_string(name, str8_lit(#rule)); RD_Cfg *expr_cfg = rd_cfg_new(name, str8_lit("expression")); rd_cfg_new(expr_cfg, str8_lit(expr));}
#define Z(name) if(name == &rd_nil_cfg) {name = rd_cfg_alloc(); rd_cfg_equip_string(name, str8_lit(#name));}
            FixedTab_XList
#undef X
#undef Y
#undef Z
            
            //- rjf: eliminate all tab selections
#define X(name) if(name != &rd_nil_cfg) {rd_cfg_release(rd_cfg_child_from_string(name, str8_lit("selected")));}
#define Y(name, rule, expr) if(name != &rd_nil_cfg) {rd_cfg_release(rd_cfg_child_from_string(name, str8_lit("selected")));}
#define Z(name) if(name != &rd_nil_cfg) {rd_cfg_release(rd_cfg_child_from_string(name, str8_lit("selected")));}
            FixedTab_XList
#undef X
#undef Y
#undef Z
            for(RD_CfgNode *n = texts.first; n != 0; n = n->next)
            {
              rd_cfg_release(rd_cfg_child_from_string(n->v, str8_lit("selected")));
            }
            
            //- rjf: create the panel root
            panels = rd_cfg_new(window, str8_lit("panels"));
            
            //- rjf: rebuild the new panel tree
            switch(kind)
            {
              default:{}break;
              
              //- rjf: (default layout)
              case RD_CmdKind_ResetToDefaultPanels:
              {
                // rjf: root split
                rd_cfg_child_from_string_or_alloc(window, str8_lit("split_x"));
                RD_Cfg *root_0 = rd_cfg_new(panels, str8_lit("0.85"));
                RD_Cfg *root_1 = rd_cfg_new(panels, str8_lit("0.15"));
                
                // rjf: root_0 split
                RD_Cfg *root_0_0 = rd_cfg_new(root_0, str8_lit("0.80"));
                RD_Cfg *root_0_1 = rd_cfg_new(root_0, str8_lit("0.20"));
                
                // rjf: root_1 split
                RD_Cfg *root_1_0 = rd_cfg_new(root_1, str8_lit("0.50"));
                RD_Cfg *root_1_1 = rd_cfg_new(root_1, str8_lit("0.50"));
                rd_cfg_insert_child(root_1_0, root_1_0->last, targets);
                rd_cfg_insert_child(root_1_1, root_1_1->last, scheduler);
                rd_cfg_new(targets, str8_lit("selected"));
                rd_cfg_new(scheduler, str8_lit("selected"));
                
                // rjf: root 0_0 split
                RD_Cfg *root_0_0_0 = rd_cfg_new(root_0_0, str8_lit("0.25"));
                RD_Cfg *root_0_0_1 = rd_cfg_new(root_0_0, str8_lit("0.75"));
                
                // rjf: root_0_0_0 split
                RD_Cfg *root_0_0_0_0 = rd_cfg_new(root_0_0_0, str8_lit("0.50"));
                RD_Cfg *root_0_0_0_1 = rd_cfg_new(root_0_0_0, str8_lit("0.50"));
                rd_cfg_insert_child(root_0_0_0_0, root_0_0_0_0->last, disasm);
                rd_cfg_new(disasm, str8_lit("selected"));
                rd_cfg_insert_child(root_0_0_0_1, root_0_0_0_1->last, breakpoints);
                rd_cfg_insert_child(root_0_0_0_1, root_0_0_0_1->last, watch_pins);
                rd_cfg_insert_child(root_0_0_0_1, root_0_0_0_1->last, output);
                rd_cfg_insert_child(root_0_0_0_1, root_0_0_0_1->last, memory);
                rd_cfg_new(output, str8_lit("selected"));
                
                // rjf: root_0_1 split
                RD_Cfg *root_0_1_0 = rd_cfg_new(root_0_1, str8_lit("0.60"));
                RD_Cfg *root_0_1_1 = rd_cfg_new(root_0_1, str8_lit("0.40"));
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, watches);
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, locals);
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, registers);
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, globals);
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, thread_locals);
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, types);
                rd_cfg_insert_child(root_0_1_0, root_0_1_0->last, procedures);
                rd_cfg_new(watches, str8_lit("selected"));
                rd_cfg_new(root_0_1_0, str8_lit("tabs_on_bottom"));
                rd_cfg_insert_child(root_0_1_1, root_0_1_1->last, call_stack);
                rd_cfg_insert_child(root_0_1_1, root_0_1_1->last, modules);
                rd_cfg_new(call_stack, str8_lit("selected"));
                rd_cfg_new(root_0_1_1, str8_lit("tabs_on_bottom"));
                
                // rjf: fill main panel with getting started, OR all collected code views
                RD_Cfg *main_panel = root_0_0_1;
                if(getting_started != &rd_nil_cfg)
                {
                  rd_cfg_insert_child(main_panel, main_panel->last, getting_started);
                  rd_cfg_new(getting_started, str8_lit("selected"));
                }
                else if(texts.first)
                {
                  rd_cfg_new(texts.first->v, str8_lit("selected"));
                }
                for(RD_CfgNode *n = texts.first; n != 0; n = n->next)
                {
                  rd_cfg_insert_child(main_panel, main_panel->last, n->v);
                }
                
                // rjf: set main panel as selected
                rd_cfg_new(main_panel, str8_lit("selected"));
              }break;
              
              //- rjf: (compact layout)
              case RD_CmdKind_ResetToCompactPanels:
              {
                // rjf: root split
                rd_cfg_child_from_string_or_alloc(window, str8_lit("split_x"));
                RD_Cfg *root_0 = rd_cfg_new(panels, str8_lit("0.25"));
                RD_Cfg *root_1 = rd_cfg_new(panels, str8_lit("0.75"));
                
                // rjf: root_0 split
                RD_Cfg *root_0_0 = rd_cfg_new(root_0, str8_lit("0.25"));
                RD_Cfg *root_0_1 = rd_cfg_new(root_0, str8_lit("0.25"));
                RD_Cfg *root_0_2 = rd_cfg_new(root_0, str8_lit("0.25"));
                RD_Cfg *root_0_3 = rd_cfg_new(root_0, str8_lit("0.25"));
                rd_cfg_insert_child(root_0_0, root_0_0->last, watches);
                rd_cfg_insert_child(root_0_0, root_0_0->last, types);
                rd_cfg_new(watches, str8_lit("selected"));
                rd_cfg_insert_child(root_0_1, root_0_1->last, scheduler);
                rd_cfg_insert_child(root_0_1, root_0_1->last, targets);
                rd_cfg_insert_child(root_0_1, root_0_1->last, breakpoints);
                rd_cfg_insert_child(root_0_1, root_0_1->last, watch_pins);
                rd_cfg_new(scheduler, str8_lit("selected"));
                rd_cfg_insert_child(root_0_2, root_0_2->last, disasm);
                rd_cfg_insert_child(root_0_2, root_0_2->last, output);
                rd_cfg_new(disasm, str8_lit("selected"));
                rd_cfg_insert_child(root_0_3, root_0_3->last, call_stack);
                rd_cfg_insert_child(root_0_3, root_0_3->last, modules);
                rd_cfg_new(call_stack, str8_lit("selected"));
                
                // rjf: fill main panel with getting started, OR all collected code views
                RD_Cfg *main_panel = root_1;
                if(getting_started != &rd_nil_cfg)
                {
                  rd_cfg_insert_child(main_panel, main_panel->last, getting_started);
                  rd_cfg_new(getting_started, str8_lit("selected"));
                }
                else if(texts.first)
                {
                  rd_cfg_new(texts.first->v, str8_lit("selected"));
                }
                for(RD_CfgNode *n = texts.first; n != 0; n = n->next)
                {
                  rd_cfg_insert_child(main_panel, main_panel->last, n->v);
                }
                
                // rjf: set main panel as selected
                rd_cfg_new(main_panel, str8_lit("selected"));
              }break;
            }
            
            //- rjf: release any unused views from the previous layout
#define X(name) if(name->parent == &rd_nil_cfg) {rd_cfg_release(name);}
#define Y(name, rule, expr) if(name->parent == &rd_nil_cfg) {rd_cfg_release(name);}
#define Z(name) if(name->parent == &rd_nil_cfg) {rd_cfg_release(name);}
            FixedTab_XList
#undef X
#undef Y
#undef Z
          }break;
          
          //- rjf: thread finding
          case RD_CmdKind_FindThread:
          for(RD_WindowState *ws = rd_state->first_window_state; ws != &rd_nil_window_state; ws = ws->order_next)
          {
            DI_Scope *scope = di_scope_open();
            CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
            U64 unwind_index = rd_regs()->unwind_count;
            U64 inline_depth = rd_regs()->inline_depth;
            if(thread->kind == CTRL_EntityKind_Thread)
            {
              // rjf: grab rip
              U64 rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_index);
              
              // rjf: extract thread/rip info
              CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
              CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
              DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
              RDI_Parsed *rdi = di_rdi_from_key(scope, &dbgi_key, 0);
              U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
              D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff);
              D_Line line = {0};
              {
                U64 idx = 0;
                for(D_LineNode *n = lines.first; n != 0; n = n->next, idx += 1)
                {
                  line = n->v;
                  if(idx == inline_depth)
                  {
                    break;
                  }
                }
              }
              
              // rjf: snap to resolved line
              B32 missing_rip   = (rip_vaddr == 0);
              B32 dbgi_missing  = (dbgi_key.min_timestamp == 0 || dbgi_key.path.size == 0);
              B32 dbgi_pending  = !dbgi_missing && rdi == &di_rdi_parsed_nil;
              B32 has_line_info = (line.voff_range.max != 0);
              B32 has_module    = (module != &ctrl_entity_nil);
              B32 has_dbg_info  = has_module && !dbgi_missing;
              if(!dbgi_pending && (has_line_info || has_module))
              {
                rd_cmd(RD_CmdKind_FindCodeLocation,
                       .file_path    = line.file_path,
                       .cursor       = line.pt,
                       .process      = process->handle,
                       .voff         = rip_voff,
                       .vaddr        = rip_vaddr,
                       .unwind_count = unwind_index,
                       .inline_depth = inline_depth);
              }
              
              // rjf: snap to resolved address w/o line info
              if(!missing_rip && !dbgi_pending && !has_line_info && !has_module)
              {
                rd_cmd(RD_CmdKind_FindCodeLocation,
                       .file_path    = str8_zero(),
                       .process      = process->handle,
                       .module       = module->handle,
                       .voff         = rip_voff,
                       .vaddr        = rip_vaddr,
                       .unwind_count = unwind_index,
                       .inline_depth = inline_depth);
              }
              
              // rjf: retry on stopped, pending debug info
              if(!d_ctrl_targets_running() && (dbgi_pending || missing_rip))
              {
                find_thread_retry = thread->handle;
              }
            }
            di_scope_close(scope);
          }break;
          case RD_CmdKind_FindSelectedThread:
          for(RD_WindowState *ws = rd_state->first_window_state; ws != &rd_nil_window_state; ws = ws->order_next)
          {
            CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
            rd_cmd(RD_CmdKind_FindThread,
                   .thread       = selected_thread->handle,
                   .unwind_count = rd_base_regs()->unwind_count,
                   .inline_depth = rd_base_regs()->inline_depth);
          }break;
          
          //- rjf: name finding
          case RD_CmdKind_GoToName:
          {
            String8 name = rd_regs()->string;
            if(name.size != 0)
            {
              B32 name_resolved = 0;
              
              // rjf: try to resolve name as a symbol
              U64 voff = 0;
              DI_Key voff_dbgi_key = {0};
              if(!name_resolved)
              {
                DI_KeyList keys = d_push_active_dbgi_key_list(scratch.arena);
                for(DI_KeyNode *n = keys.first; n != 0; n = n->next)
                {
                  U64 binary_voff = d_voff_from_dbgi_key_symbol_name(&n->v, name);
                  if(binary_voff != 0)
                  {
                    voff = binary_voff;
                    voff_dbgi_key = n->v;
                    name_resolved = 1;
                    break;
                  }
                }
              }
              
              // rjf: try to resolve name as a file
              String8 file_path = {0};
              if(!name_resolved)
              {
                // rjf: unpack quoted portion of string
                String8 file_part_of_name = name;
                U64 quote_pos = str8_find_needle(name, 0, str8_lit("\""), 0);
                if(quote_pos < name.size)
                {
                  file_part_of_name = str8_skip(name, quote_pos+1);
                  U64 ender_quote_pos = str8_find_needle(file_part_of_name, 0, str8_lit("\""), 0);
                  file_part_of_name = str8_prefix(file_part_of_name, ender_quote_pos);
                }
                String8List search_parts = str8_split_path(scratch.arena, file_part_of_name);
                
                // rjf: get source path
                RD_Cfg *src_view = rd_cfg_from_id(rd_regs()->view);
                String8 src_view_expr = rd_expr_from_cfg(src_view);
                String8 src_file_path = rd_file_path_from_eval_string(scratch.arena, src_view_expr);
                String8List src_file_parts = str8_split_path(scratch.arena, src_file_path);
                
                // rjf: search for actual file
                Temp temp = temp_begin(scratch.arena);
                for(String8Node *n = src_file_parts.first; n != 0; n = n->next)
                {
                  temp_end(temp);
                  String8List try_path_parts = {0};
                  for(String8Node *src_n = src_file_parts.first; src_n != n && src_n != 0; src_n = src_n->next)
                  {
                    str8_list_push(temp.arena, &try_path_parts, src_n->string);
                  }
                  for(String8Node *try_n = search_parts.first; try_n != 0; try_n = try_n->next)
                  {
                    str8_list_push(temp.arena, &try_path_parts, try_n->string);
                  }
                  String8 try_path = str8_list_join(temp.arena, &try_path_parts, &(StringJoin){.sep = str8_lit("/")});
                  FileProperties try_props = os_properties_from_file_path(try_path);
                  if(try_props.modified != 0)
                  {
                    name_resolved = 1;
                    file_path = try_path;
                    break;
                  }
                }
              }
              
              // rjf: process resolved info
              if(!name_resolved)
              {
                log_user_errorf("`%S` could not be found.", name);
              }
              
              // rjf: name resolved to voff * dbg info
              if(name_resolved && voff != 0)
              {
                D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &voff_dbgi_key, voff);
                if(lines.first != 0)
                {
                  CTRL_Entity *process = &ctrl_entity_nil;
                  U64 vaddr = 0;
                  if(voff_dbgi_key.path.size != 0)
                  {
                    CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &voff_dbgi_key);
                    CTRL_Entity *module = ctrl_entity_list_first(&modules);
                    process = ctrl_entity_ancestor_from_kind(module, CTRL_EntityKind_Process);
                    if(process != &ctrl_entity_nil)
                    {
                      vaddr = module->vaddr_range.min + lines.first->v.voff_range.min;
                    }
                  }
                  rd_cmd(RD_CmdKind_FindCodeLocation,
                         .file_path = lines.first->v.file_path,
                         .cursor    = lines.first->v.pt,
                         .process   = process->handle,
                         .module    = module->handle,
                         .vaddr     = module->vaddr_range.min + lines.first->v.voff_range.min);
                }
              }
              
              // rjf: name resolved to a file path
              if(name_resolved && file_path.size != 0)
              {
                rd_cmd(RD_CmdKind_FindCodeLocation, .file_path = file_path, .cursor = txt_pt(1, 1), .vaddr = 0);
              }
            }
          }break;
          
          //- rjf: snap-to-code-location
          case RD_CmdKind_FindCodeLocation:
          {
            // NOTE(rjf): This command is where a lot of high-level flow things
            // in the debugger come together. It's that codepath that runs any
            // time a source code location is clicked in the UI, when a thread
            // is selected, or when a thread causes a halt (hitting a breakpoint
            // or exception or something). This is the logic that manages the
            // flow of how views and panels are changed, opened, etc. when
            // something like that happens.
            //
            // The gist of the intended rule for textual source code locations
            // is the following:
            //
            // 1. Try to find a panel that's viewing the file (has it open in a
            //    tab, *and* that tab is selected).
            // 2. Try to find a panel that has the file open in a tab, but does not
            //    currently have that tab selected.
            // 3. Try to find a panel that has ANY source code open in any tab.
            // 4. If the above things fail, try to pick the biggest panel, which
            //    is generally a decent rule (because it matches the popular
            //    debugger usage UI paradigm).
            //
            // The reason why this is a little more complicated than you might
            // imagine is because this debugger frontend does not have any special
            // "code panels" or anything like that, unlike e.g. VS or Remedy. All
            // panels are identical in nature to allow for the user to organize
            // the interface how they want, but in cases like this, we have to
            // "fish out" the best option given the user's configuration. This
            // can't be what the user wants in 100% of cases (this program cannot
            // read anyone's mind), but it does provide expected behavior in
            // common cases.
            //
            // The gist of the intended rule for finding disassembly locations is
            // the following:
            //
            // 1. Try to find a panel that's viewing disassembly already - if so,
            //    snap it to the right address.
            // 2. If there is no disassembly tab open, then we need to open one
            //    ONLY if source code was not found.
            // 3. If we need to open a disassembly tab, we will first try to pick
            //    the biggest empty panel.
            // 4. If there is no empty panel, then we will pick the biggest
            //    panel.
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
            
            // rjf: grab things to find. path * point, process * address, etc.
            String8 file_path = {0};
            TxtPt point = {0};
            CTRL_Entity *thread = &ctrl_entity_nil;
            CTRL_Entity *process = &ctrl_entity_nil;
            U64 vaddr = 0;
            B32 require_disasm_snap = 0;
            {
              file_path = rd_mapped_from_file_path(scratch.arena, rd_regs()->file_path);
              point     = rd_regs()->cursor;
              thread    = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
              process   = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->process);
              vaddr     = rd_regs()->vaddr;
              if(file_path.size == 0)
              {
                require_disasm_snap = 1;
              }
            }
            
            // rjf: given a src code location, if no vaddr is specified,
            // try to map the src coordinates to a vaddr via line info
            if(vaddr == 0 && file_path.size != 0)
            {
              D_LineList lines = d_lines_from_file_path_line_num(scratch.arena, file_path, point.line);
              for(D_LineNode *n = lines.first; n != 0; n = n->next)
              {
                CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &n->v.dbgi_key);
                CTRL_Entity *module = ctrl_module_from_thread_candidates(d_state->ctrl_entity_store, thread, &modules);
                vaddr = ctrl_vaddr_from_voff(module, n->v.voff_range.min);
                break;
              }
            }
            
            // rjf: first, try to find panel/view pair that already has the src file open
            RD_PanelNode *panel_w_this_src_code = &rd_nil_panel_node;
            RD_Cfg *view_w_this_src_code = &rd_nil_cfg;
            for(RD_PanelNode *panel = panel_tree.root;
                panel != &rd_nil_panel_node;
                panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
            {
              if(panel->first != &rd_nil_panel_node)
              {
                continue;
              }
              for(RD_CfgNode *tab_n = panel->tabs.first; tab_n != 0; tab_n = tab_n->next)
              {
                RD_Cfg *tab = tab_n->v;
                if(rd_cfg_is_project_filtered(tab)) { continue; }
                RD_RegsScope(.view = tab->id)
                {
                  String8 tab_expr = rd_expr_from_cfg(tab);
                  String8 tab_file_path = rd_file_path_from_eval_string(scratch.arena, tab_expr);
                  if((str8_match(tab->string, str8_lit("text"), 0) || str8_match(tab->string, str8_lit("pending"), 0)) && 
                     path_match_normalized(tab_file_path, file_path))
                  {
                    panel_w_this_src_code = panel;
                    view_w_this_src_code = tab;
                    if(tab == panel->selected_tab)
                    {
                      break;
                    }
                  }
                }
              }
            }
            
            // rjf: find a panel that already has *any* code open (prioritize largest)
            RD_PanelNode *panel_w_any_src_code = &rd_nil_panel_node;
            {
              Rng2F32 root_rect = r2f32(v2f32(0, 0), v2f32(1000, 1000));
              F32 best_panel_area = 0;
              for(RD_PanelNode *panel = panel_tree.root;
                  panel != &rd_nil_panel_node;
                  panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
              {
                if(panel->first != &rd_nil_panel_node)
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel_node(root_rect, panel_tree.root, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 panel_area = panel_rect_dim.x*panel_rect_dim.y;
                for(RD_CfgNode *tab_n = panel->tabs.first; tab_n != 0; tab_n = tab_n->next)
                {
                  RD_Cfg *tab = tab_n->v;
                  if(rd_cfg_is_project_filtered(tab)) { continue; }
                  String8 view_expr = rd_expr_from_cfg(tab);
                  String8 file_path = rd_file_path_from_eval_string(scratch.arena, view_expr);
                  if(str8_match(tab->string, str8_lit("text"), 0) && file_path.size != 0 && panel_area > best_panel_area)
                  {
                    panel_w_any_src_code = panel;
                    best_panel_area = panel_area;
                    break;
                  }
                }
              }
            }
            
            // rjf: try to find panel/view pair that has disassembly open (prioritize largest)
            RD_PanelNode *panel_w_disasm = &rd_nil_panel_node;
            RD_Cfg *view_w_disasm = &rd_nil_cfg;
            {
              Rng2F32 root_rect = r2f32(v2f32(0, 0), v2f32(1000, 1000));
              F32 best_panel_area = 0;
              for(RD_PanelNode *panel = panel_tree.root;
                  panel != &rd_nil_panel_node;
                  panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
              {
                if(panel->first != &rd_nil_panel_node)
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel_node(root_rect, panel_tree.root, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 panel_area = panel_rect_dim.x*panel_rect_dim.y;
                for(RD_CfgNode *tab_n = panel->tabs.first; tab_n != 0; tab_n = tab_n->next)
                {
                  RD_Cfg *tab = tab_n->v;
                  if(rd_cfg_is_project_filtered(tab)) { continue; }
                  RD_RegsScope(.view = tab->id)
                  {
                    B32 tab_is_selected = (tab == panel->selected_tab);
                    String8 expr_string = rd_expr_from_cfg(tab);
                    if(str8_match(tab->string, str8_lit("disasm"), 0) && expr_string.size == 0 && panel_area > best_panel_area)
                    {
                      panel_w_disasm = panel;
                      view_w_disasm = tab;
                      best_panel_area = panel_area;
                      if(tab_is_selected)
                      {
                        break;
                      }
                    }
                  }
                }
              }
            }
            
            // rjf: find the biggest panel
            RD_PanelNode *biggest_panel = &rd_nil_panel_node;
            {
              Rng2F32 root_rect = r2f32(v2f32(0, 0), v2f32(1000, 1000));
              F32 best_panel_area = 0;
              for(RD_PanelNode *panel = panel_tree.root;
                  panel != &rd_nil_panel_node;
                  panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
              {
                if(panel->first != &rd_nil_panel_node)
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel_node(root_rect, panel_tree.root, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 panel_area = panel_rect_dim.x*panel_rect_dim.y;
                if((best_panel_area == 0 || panel_area > best_panel_area))
                {
                  best_panel_area = panel_area;
                  biggest_panel = panel;
                }
              }
            }
            
            // rjf: find the biggest empty panel
            RD_PanelNode *biggest_empty_panel = &rd_nil_panel_node;
            {
              Rng2F32 root_rect = r2f32(v2f32(0, 0), v2f32(1000, 1000));
              F32 best_panel_area = 0;
              for(RD_PanelNode *panel = panel_tree.root;
                  panel != &rd_nil_panel_node;
                  panel = rd_panel_node_rec__depth_first_pre(panel_tree.root, panel).next)
              {
                if(panel->first != &rd_nil_panel_node)
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel_node(root_rect, panel_tree.root, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 panel_area = panel_rect_dim.x*panel_rect_dim.y;
                B32 panel_is_empty = 1;
                for(RD_CfgNode *n = panel->tabs.first; n != 0; n = n->next)
                {
                  RD_Cfg *tab = n->v;
                  if(!rd_cfg_is_project_filtered(tab))
                  {
                    panel_is_empty = 0;
                    break;
                  }
                }
                if(panel_is_empty && (best_panel_area == 0 || panel_area > best_panel_area))
                {
                  best_panel_area = panel_area;
                  biggest_empty_panel = panel;
                }
              }
            }
            
            // rjf: choose panel for source code
            RD_PanelNode *src_code_dst_panel = &rd_nil_panel_node;
            {
              if(src_code_dst_panel == &rd_nil_panel_node) { src_code_dst_panel = panel_w_this_src_code; }
              if(src_code_dst_panel == &rd_nil_panel_node) { src_code_dst_panel = panel_w_any_src_code; }
              if(src_code_dst_panel == &rd_nil_panel_node) { src_code_dst_panel = biggest_empty_panel; }
              if(src_code_dst_panel == &rd_nil_panel_node) { src_code_dst_panel = biggest_panel; }
            }
            
            // rjf: choose panel for disassembly
            RD_PanelNode *disasm_dst_panel = &rd_nil_panel_node;
            {
              if(disasm_dst_panel == &rd_nil_panel_node) { disasm_dst_panel = panel_w_disasm; }
              if(disasm_dst_panel == &rd_nil_panel_node) { disasm_dst_panel = biggest_empty_panel; }
              if(disasm_dst_panel == &rd_nil_panel_node) { disasm_dst_panel = biggest_panel; }
            }
            
            // rjf: if disasm and source code match:
            //        if disasm preferred, cancel source
            //        if source preferred, cancel disasm
            if(disasm_dst_panel == src_code_dst_panel)
            {
              if(rd_regs()->prefer_disasm)
              {
                src_code_dst_panel = &rd_nil_panel_node;
              }
              else
              {
                disasm_dst_panel = &rd_nil_panel_node;
              }
            }
            
            // rjf: if disasm is not preferred, and we have no disassembly view
            // open at all, cancel disasm, so that it doesn't open if the user
            // doesn't want it.
            if(!rd_regs()->prefer_disasm && panel_w_disasm == &rd_nil_panel_node)
            {
              disasm_dst_panel = &rd_nil_panel_node;
            }
            
            // rjf: given the above, find source code location.
            if(file_path.size != 0 && src_code_dst_panel != &rd_nil_panel_node)
            {
              RD_PanelNode *dst_panel = src_code_dst_panel;
              
              // rjf: construct new view if needed
              RD_Cfg *dst_tab = view_w_this_src_code;
              if(dst_panel != &rd_nil_panel_node && dst_tab == &rd_nil_cfg)
              {
                dst_tab = rd_cfg_new(dst_panel->cfg, str8_lit("text"));
                RD_Cfg *expr = rd_cfg_new(dst_tab, str8_lit("expression"));
                rd_cfg_new(expr, rd_eval_string_from_file_path(scratch.arena, file_path));
              }
              
              // rjf: determine if we need a contain or center
              RD_CmdKind cursor_snap_kind = RD_CmdKind_CenterCursor;
              if(dst_panel != &rd_nil_panel_node && dst_tab == view_w_this_src_code && dst_panel->selected_tab == dst_tab)
              {
                cursor_snap_kind = RD_CmdKind_ContainCursor;
              }
              
              // rjf: move cursor & snap-to-cursor
              if(dst_panel != &rd_nil_panel_node) RD_RegsScope(.panel = dst_panel->cfg->id,
                                                               .view  = dst_tab->id)
              {
                rd_cmd(RD_CmdKind_FocusTab);
                rd_cmd(RD_CmdKind_GoToLine, .cursor = point);
                rd_cmd(cursor_snap_kind);
              }
              
              // rjf: record
              rd_cmd(RD_CmdKind_RecordFileInProject, .file_path = file_path);
            }
            
            // rjf: given the above, find disassembly location.
            if(process != &ctrl_entity_nil && vaddr != 0 && disasm_dst_panel != &rd_nil_panel_node)
            {
              RD_PanelNode *dst_panel = disasm_dst_panel;
              
              // rjf: construct new tab if needed
              RD_Cfg *dst_tab = view_w_disasm;
              if(dst_panel != &rd_nil_panel_node && view_w_disasm == &rd_nil_cfg)
              {
                dst_tab = rd_cfg_new(dst_panel->cfg, str8_lit("disasm"));
              }
              
              // rjf: determine if we need a contain or center
              RD_CmdKind cursor_snap_kind = RD_CmdKind_CenterCursor;
              if(dst_tab == view_w_disasm && dst_panel->selected_tab == dst_tab)
              {
                cursor_snap_kind = RD_CmdKind_ContainCursor;
              }
              
              // rjf: move cursor & snap-to-cursor
              if(dst_panel != &rd_nil_panel_node) RD_RegsScope(.panel = dst_panel->cfg->id,
                                                               .view  = dst_tab->id)
              {
                rd_cmd(RD_CmdKind_FocusTab);
                rd_cmd(RD_CmdKind_GoToAddress, .process = process->handle, .vaddr = vaddr);
                rd_cmd(cursor_snap_kind);
              }
            }
          }break;
          
          //- rjf: filtering
          case RD_CmdKind_Filter:
          {
#if 0 // TODO(rjf): @cfg
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            B32 view_is_tab = 0;
            for(RD_View *tab = panel->first_tab_view; !rd_view_is_nil(tab); tab = tab->order_next)
            {
              if(rd_view_is_project_filtered(tab)) { continue; }
              if(tab == view)
              {
                view_is_tab = 1;
                break;
              }
            }
            if(view_is_tab && view->spec->flags & RD_ViewRuleInfoFlag_CanFilter)
            {
              view->is_filtering ^= 1;
              view->query_cursor = txt_pt(1, 1+(S64)view->query_string_size);
              view->query_mark = txt_pt(1, 1);
            }
#endif
          }break;
          case RD_CmdKind_ClearFilter:
          {
#if 0 // TODO(rjf): @cfg
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            if(!rd_view_is_nil(view))
            {
              view->query_string_size = 0;
              view->is_filtering = 0;
              view->query_cursor = view->query_mark = txt_pt(1, 1);
            }
#endif
          }break;
          case RD_CmdKind_ApplyFilter:
          {
#if 0 // TODO(rjf): @cfg
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            if(!rd_view_is_nil(view))
            {
              view->is_filtering = 0;
            }
#endif
          }break;
          
          //- rjf: query stack
          case RD_CmdKind_PushQuery:
          {
            RD_Cfg *wcfg = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(wcfg);
            if(ws != &rd_nil_window_state)
            {
              Arena *arena = arena_alloc();
              RD_Lister *lister = push_array(arena, RD_Lister, 1);
              lister->arena = arena;
              lister->regs = rd_regs_copy(lister->arena, rd_regs());
              lister->input_string_size = Min(rd_regs()->string.size, sizeof(lister->input_buffer));
              lister->input_cursor = lister->input_mark = txt_pt(1, lister->input_string_size+1);
              MemoryCopy(lister->input_buffer, rd_regs()->string.str, lister->input_string_size);
              SLLStackPush(ws->top_query_lister, lister);
            }
          }break;
          case RD_CmdKind_CompleteQuery:
          {
#if 0 // TODO(rjf): @cfg
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            String8 query_cmd_name = ws->query_cmd_name;
            RD_CmdKindInfo *info = rd_cmd_kind_info_from_string(query_cmd_name);
            RD_RegSlot slot = info->query.slot;
            
            // rjf: compound command parameters
            if(slot != RD_RegSlot_Null && !(ws->query_cmd_regs_mask[slot/64] & (1ull<<(slot%64))))
            {
              RD_Regs *regs_copy = rd_regs_copy(ws->query_cmd_arena, rd_regs());
              Rng1U64 offset_range_in_regs = rd_reg_slot_range_table[slot];
              MemoryCopy((U8 *)(ws->query_cmd_regs) + offset_range_in_regs.min,
                         (U8 *)(regs_copy) + offset_range_in_regs.min,
                         dim_1u64(offset_range_in_regs));
              ws->query_cmd_regs_mask[slot/64] |= (1ull<<(slot%64));
            }
            
            // rjf: determine if command is ready to run
            B32 command_ready = 1;
            if(slot != RD_RegSlot_Null && !(ws->query_cmd_regs_mask[slot/64] & (1ull<<(slot%64))))
            {
              command_ready = 0;
            }
            
            // rjf: end this query
            if(!(info->query.flags & RD_QueryFlag_KeepOldInput))
            {
              rd_cmd(RD_CmdKind_CancelQuery);
            }
            
            // rjf: unset command register slot, if we keep old input (and thus need
            // to re-query user)
            if(info->query.flags & RD_QueryFlag_KeepOldInput)
            {
              ws->query_cmd_regs_mask[slot/64] &= ~(1ull<<(slot%64));
            }
            
            // rjf: push command if possible
            if(command_ready)
            {
              rd_push_cmd(ws->query_cmd_name, ws->query_cmd_regs);
            }
#endif
          }break;
          case RD_CmdKind_CancelQuery:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            RD_Lister *top_lister = ws->top_query_lister;
            if(top_lister != 0)
            {
              SLLStackPop(ws->top_query_lister);
              arena_release(top_lister->arena);
            }
          }break;
          
          //- rjf: developer commands
          case RD_CmdKind_ToggleDevMenu:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            ws->dev_menu_is_open ^= 1;
          }break;
          
          //- rjf: general entity operations
          case RD_CmdKind_SelectCfg:
          case RD_CmdKind_SelectTarget:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            RD_CfgList all_of_the_same_kind = rd_cfg_top_level_list_from_string(scratch.arena, cfg->string);
            B32 is_selected = rd_disabled_from_cfg(cfg);
            for(RD_CfgNode *n = all_of_the_same_kind.first; n != 0; n = n->next)
            {
              RD_Cfg *c = n->v;
              rd_cfg_child_from_string_or_alloc(c, str8_lit("disabled"));
            }
            if(!is_selected)
            {
              rd_cfg_release(rd_cfg_child_from_string(cfg, str8_lit("disabled")));
            }
          }break;
          case RD_CmdKind_EnableCfg:
          case RD_CmdKind_EnableBreakpoint:
          case RD_CmdKind_EnableTarget:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            rd_cfg_release(rd_cfg_child_from_string(cfg, str8_lit("disabled")));
          }break;
          case RD_CmdKind_DisableCfg:
          case RD_CmdKind_DisableBreakpoint:
          case RD_CmdKind_DisableTarget:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            RD_Cfg *disabled = rd_cfg_child_from_string_or_alloc(cfg, str8_lit("disabled"));
            rd_cfg_new(disabled, str8_lit("1"));
          }break;
          case RD_CmdKind_RemoveCfg:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            rd_cfg_release(cfg);
          }break;
          case RD_CmdKind_NameCfg:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            if(rd_regs()->string.size != 0)
            {
              RD_Cfg *label = rd_cfg_child_from_string_or_alloc(cfg, str8_lit("label"));
              rd_cfg_new(label, rd_regs()->string);
            }
            else
            {
              rd_cfg_release(rd_cfg_child_from_string(cfg, str8_lit("label")));
            }
          }break;
          case RD_CmdKind_ConditionCfg:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            if(rd_regs()->string.size != 0)
            {
              RD_Cfg *cnd = rd_cfg_child_from_string_or_alloc(cfg, str8_lit("condition"));
              rd_cfg_new(cnd, rd_regs()->string);
            }
            else
            {
              rd_cfg_release(rd_cfg_child_from_string(cfg, str8_lit("condition")));
            }
          }break;
          case RD_CmdKind_DuplicateCfg:
          {
            RD_Cfg *src = rd_cfg_from_id(rd_regs()->cfg);
            RD_Cfg *dst = rd_cfg_deep_copy(src);
            rd_cfg_insert_child(src->parent, src, dst);
          }break;
          case RD_CmdKind_RelocateCfg:
          {
            RD_Cfg *cfg = rd_cfg_from_id(rd_regs()->cfg);
            RD_Cfg *loc = rd_cfg_child_from_string_or_alloc(cfg, str8_lit("location"));
            for(RD_Cfg *child = loc->first, *next = &rd_nil_cfg; child != &rd_nil_cfg; child = next)
            {
              next = child->next;
              rd_cfg_release(child);
            }
            if(rd_regs()->cursor.line != 0)
            {
              RD_Cfg *file = rd_cfg_new(loc, rd_regs()->file_path);
              RD_Cfg *line = rd_cfg_newf(file, "%I64d", rd_regs()->cursor.line);
              rd_cfg_newf(line, "%I64d", rd_regs()->cursor.column);
            }
            else if(rd_regs()->vaddr != 0)
            {
              rd_cfg_newf(loc, "0x%I64x", rd_regs()->vaddr);
            }
            else if(rd_regs()->string.size != 0)
            {
              rd_cfg_new(loc, rd_regs()->string);
            }
          }break;
          
          //- rjf: breakpoints
          case RD_CmdKind_AddBreakpoint:
          case RD_CmdKind_ToggleBreakpoint:
          {
            String8 file_path = rd_regs()->file_path;
            TxtPt pt = rd_regs()->cursor;
            String8 string = rd_regs()->string;
            U64 vaddr = rd_regs()->vaddr;
            if(file_path.size != 0 || string.size != 0 || vaddr != 0)
            {
              B32 removed_already_existing = 0;
              if(kind == RD_CmdKind_ToggleBreakpoint)
              {
                RD_CfgList bps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("breakpoint"));
                for(RD_CfgNode *n = bps.first; n != 0; n = n->next)
                {
                  RD_Cfg *bp = n->v;
                  RD_Cfg *loc = rd_cfg_child_from_string(bp, str8_lit("location"));
                  S64 loc_line = 0;
                  U64 loc_vaddr = 0;
                  B32 loc_matches_file_pt = (file_path.size != 0 && path_match_normalized(loc->first->string, file_path) && try_s64_from_str8_c_rules(loc->first->first->string, &loc_line) && loc_line == pt.line);
                  B32 loc_matches_string  = (string.size != 0 && str8_match(loc->first->string, string, 0));
                  B32 loc_matches_vaddr   = (vaddr != 0 && try_u64_from_str8_c_rules(loc->first->string, &loc_vaddr) && loc_vaddr == vaddr);
                  if(loc_matches_file_pt || loc_matches_string || loc_matches_vaddr)
                  {
                    rd_cfg_release(bp);
                    removed_already_existing = 1;
                    break;
                  }
                }
              }
              if(!removed_already_existing)
              {
                RD_Cfg *project = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("project"));
                RD_Cfg *bp = rd_cfg_new(project, str8_lit("breakpoint"));
                RD_Cfg *loc = rd_cfg_new(bp, str8_lit("location"));
                if(vaddr != 0)
                {
                  rd_cfg_newf(loc, "0x%I64x", vaddr);
                }
                else if(string.size != 0)
                {
                  rd_cfg_new(loc, string);
                }
                else if(file_path.size != 0)
                {
                  RD_Cfg *file_path_cfg = rd_cfg_new(loc, file_path);
                  rd_cfg_newf(file_path_cfg, "%I64d", pt.line);
                }
              }
            }
          }break;
          case RD_CmdKind_AddAddressBreakpoint:
          {
            rd_cmd(RD_CmdKind_AddBreakpoint, .string = str8_zero());
          }break;
          case RD_CmdKind_AddFunctionBreakpoint:
          {
            rd_cmd(RD_CmdKind_AddBreakpoint, .vaddr = 0);
          }break;
          
          //- rjf: watch pins
          case RD_CmdKind_AddWatchPin:
          case RD_CmdKind_ToggleWatchPin:
          {
            String8 file_path = rd_regs()->file_path;
            TxtPt pt = rd_regs()->cursor;
            String8 string = rd_regs()->string;
            U64 vaddr = rd_regs()->vaddr;
            B32 removed_already_existing = 0;
            if(kind == RD_CmdKind_ToggleWatchPin)
            {
              RD_CfgList wps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("watch_pin"));
              for(RD_CfgNode *n = wps.first; n != 0; n = n->next)
              {
                RD_Cfg *wp = n->v;
                RD_Cfg *expr = rd_cfg_child_from_string(wp, str8_lit("expression"));
                RD_Cfg *loc = rd_cfg_child_from_string(wp, str8_lit("location"));
                S64 loc_line = 0;
                U64 loc_vaddr = 0;
                B32 loc_matches_file_pt = (file_path.size != 0 && path_match_normalized(loc->first->string, file_path) && try_s64_from_str8_c_rules(loc->first->first->string, &loc_line) && loc_line == pt.line);
                B32 loc_matches_vaddr   = (vaddr != 0 && try_u64_from_str8_c_rules(loc->first->string, &loc_vaddr) && loc_vaddr == vaddr);
                B32 loc_matches_expr    = (string.size != 0 && str8_match(expr->first->string, string, 0));
                if(loc_matches_expr && (loc_matches_file_pt || loc_matches_vaddr))
                {
                  rd_cfg_release(wp);
                  removed_already_existing = 1;
                  break;
                }
              }
            }
            if(!removed_already_existing)
            {
              RD_Cfg *project = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("project"));
              RD_Cfg *wp = rd_cfg_new(project, str8_lit("watch_pin"));
              RD_Cfg *expr = rd_cfg_new(wp, str8_lit("expression"));
              RD_Cfg *loc = rd_cfg_new(wp, str8_lit("location"));
              rd_cfg_new(expr, string);
              if(vaddr != 0)
              {
                rd_cfg_newf(loc, "0x%I64x", vaddr);
              }
              else if(file_path.size != 0)
              {
                RD_Cfg *file_path_cfg = rd_cfg_new(loc, file_path);
                rd_cfg_newf(file_path_cfg, "%I64d", pt.line);
              }
            }
          }break;
          
          //- rjf: watches
          case RD_CmdKind_ToggleWatchExpression:
          if(rd_regs()->string.size != 0)
          {
            RD_CfgList all_existing_watches = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("watch"));
            RD_Cfg *existing_watch = &rd_nil_cfg;
            for(RD_CfgNode *n = all_existing_watches.first; n != 0; n = n->next)
            {
              RD_Cfg *watch = n->v;
              String8 expr = rd_expr_from_cfg(watch);
              if(str8_match(expr, rd_regs()->string, 0))
              {
                existing_watch = watch;
                break;
              }
            }
            if(existing_watch == &rd_nil_cfg)
            {
              RD_Cfg *project = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("project"));
              RD_Cfg *watch = rd_cfg_new(project, str8_lit("watch"));
              RD_Cfg *expr = rd_cfg_new(watch, str8_lit("expression"));
              rd_cfg_new(expr, rd_regs()->string);
            }
            else
            {
              rd_cfg_release(existing_watch);
            }
          }break;
          
          //- rjf: cursor operations
          case RD_CmdKind_GoToNameAtCursor:
          case RD_CmdKind_ToggleWatchExpressionAtCursor:
          {
            HS_Scope *hs_scope = hs_scope_open();
            TXT_Scope *txt_scope = txt_scope_open();
            RD_Regs *regs = rd_regs();
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
            rd_cmd((kind == RD_CmdKind_GoToNameAtCursor ? RD_CmdKind_GoToName :
                    kind == RD_CmdKind_ToggleWatchExpressionAtCursor ? RD_CmdKind_ToggleWatchExpression :
                    RD_CmdKind_GoToName),
                   .string = expr);
            txt_scope_close(txt_scope);
            hs_scope_close(hs_scope);
          }break;
          case RD_CmdKind_SetNextStatement:
          {
            CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
            String8 file_path = rd_regs()->file_path;
            U64 new_rip_vaddr = rd_regs()->vaddr_range.min;
            if(file_path.size != 0)
            {
              D_LineList *lines = &rd_regs()->lines;
              for(D_LineNode *n = lines->first; n != 0; n = n->next)
              {
                CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &n->v.dbgi_key);
                CTRL_Entity *module = ctrl_module_from_thread_candidates(d_state->ctrl_entity_store, thread, &modules);
                if(module != &ctrl_entity_nil)
                {
                  new_rip_vaddr = ctrl_vaddr_from_voff(module, n->v.voff_range.min);
                  break;
                }
              }
            }
            rd_cmd(RD_CmdKind_SetThreadIP, .vaddr = new_rip_vaddr);
          }break;
          
          //- rjf: targets
          case RD_CmdKind_AddTarget:
          {
            String8 file_path = rd_regs()->file_path;
            RD_Cfg *project = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("project"));
            RD_Cfg *target = rd_cfg_new(project, str8_lit("target"));
            RD_Cfg *exe = rd_cfg_new(target, str8_lit("executable"));
            rd_cfg_new(exe, file_path);
            String8 working_directory = str8_chop_last_slash(file_path);
            if(working_directory.size != 0)
            {
              RD_Cfg *wdir = rd_cfg_new(target, str8_lit("working_directory"));
              rd_cfg_newf(wdir, "%S/", working_directory);
            }
            rd_cmd(RD_CmdKind_SelectCfg, .cfg = target->id);
          }break;
          
          //- rjf: jit-debugger registration
          case RD_CmdKind_RegisterAsJITDebugger:
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
              log_user_error(str8_lit("Could not register as the just-in-time debugger, access was denied; try running the debugger as administrator."));
            }
#else
            log_user_error(str8_lit("Registering as the just-in-time debugger is currently not supported on this system."));
#endif
          }break;
          
          //- rjf: developer commands
          case RD_CmdKind_LogMarker:
          {
            log_infof("\"#MARKER\"");
          }break;
          
          //- rjf: os event passthrough
          case RD_CmdKind_OSEvent:
          {
            OS_Event *os_event = rd_regs()->os_event;
            RD_WindowState *ws = rd_window_state_from_os_handle(os_event->window);
            if(os_event != 0 && ws != &rd_nil_window_state)
            {
              UI_Event ui_event = zero_struct;
              UI_EventKind kind = UI_EventKind_Null;
              {
                switch(os_event->kind)
                {
                  default:{}break;
                  case OS_EventKind_Press:     {kind = UI_EventKind_Press;}break;
                  case OS_EventKind_Release:   {kind = UI_EventKind_Release;}break;
                  case OS_EventKind_MouseMove: {kind = UI_EventKind_MouseMove;}break;
                  case OS_EventKind_Text:      {kind = UI_EventKind_Text;}break;
                  case OS_EventKind_Scroll:    {kind = UI_EventKind_Scroll;}break;
                  case OS_EventKind_FileDrop:  {kind = UI_EventKind_FileDrop;}break;
                }
              }
              ui_event.kind         = kind;
              ui_event.key          = os_event->key;
              ui_event.modifiers    = os_event->modifiers;
              ui_event.string       = os_event->character ? str8_from_32(ui_build_arena(), str32(&os_event->character, 1)) : str8_zero();
              ui_event.paths        = str8_list_copy(ui_build_arena(), &os_event->strings);
              ui_event.pos          = os_event->pos;
              ui_event.delta_2f32   = os_event->delta;
              ui_event.timestamp_us = os_event->timestamp_us;
              ui_event_list_push(scratch.arena, &ws->ui_events, &ui_event);
            }
          }break;
          
          //- rjf: debug control context management operations
          case RD_CmdKind_SelectThread:
          {
            CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
            CTRL_Entity *module = ctrl_module_from_process_vaddr(process, ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->handle));
            CTRL_Entity *machine = ctrl_entity_ancestor_from_kind(process, CTRL_EntityKind_Machine);
            rd_state->base_regs.v.unwind_count = 0;
            rd_state->base_regs.v.inline_depth = 0;
            rd_state->base_regs.v.thread  = thread->handle;
            rd_state->base_regs.v.module  = module->handle;
            rd_state->base_regs.v.process = process->handle;
            rd_state->base_regs.v.machine = machine->handle;
            rd_cmd(RD_CmdKind_FindThread, .thread = thread->handle, .unwind_count = 0, .inline_depth = 0);
          }break;
          case RD_CmdKind_SelectUnwind:
          {
            DI_Scope *di_scope = di_scope_open();
            CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
            CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(thread);
            CTRL_CallStack call_stack = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
            CTRL_CallStackFrame *frame = ctrl_call_stack_frame_from_unwind_and_inline_depth(&call_stack, rd_regs()->unwind_count, rd_regs()->inline_depth);
            if(frame == 0)
            {
              frame = ctrl_call_stack_frame_from_unwind_and_inline_depth(&call_stack, rd_regs()->unwind_count, 0);
            }
            if(frame)
            {
              rd_state->base_regs.v.unwind_count = rd_regs()->unwind_count;
              rd_state->base_regs.v.inline_depth = rd_regs()->inline_depth;
            }
            rd_cmd(RD_CmdKind_FindThread, .thread = thread->handle, .unwind_count = rd_state->base_regs.v.unwind_count, .inline_depth = rd_state->base_regs.v.inline_depth);
            di_scope_close(di_scope);
          }break;
          case RD_CmdKind_UpOneFrame:
          case RD_CmdKind_DownOneFrame:
          {
            DI_Scope *di_scope = di_scope_open();
            CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
            CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(thread);
            CTRL_CallStack call_stack = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
            CTRL_CallStackFrame *current_frame = ctrl_call_stack_frame_from_unwind_and_inline_depth(&call_stack, rd_regs()->unwind_count, rd_regs()->inline_depth);
            CTRL_CallStackFrame *next_frame = current_frame;
            if(current_frame != 0) switch(kind)
            {
              default:{}break;
              case RD_CmdKind_UpOneFrame:
              if(current_frame > call_stack.frames)
              {
                next_frame = current_frame-1;
              }break;
              case RD_CmdKind_DownOneFrame:
              if(current_frame+1 < call_stack.frames + call_stack.count)
              {
                next_frame = current_frame+1;
              }break;
            }
            if(next_frame != 0)
            {
              CTRL_CallStackFrame *next_base_frame = next_frame + next_frame->inline_depth;
              rd_cmd(RD_CmdKind_SelectUnwind,
                     .unwind_count = next_frame->unwind_count,
                     .inline_depth = next_frame->inline_depth);
            }
            di_scope_close(di_scope);
          }break;
          
          //- rjf: meta controls
          case RD_CmdKind_Edit:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Press;
            evt.slot       = UI_EventActionSlot_Edit;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Accept:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Press;
            evt.slot       = UI_EventActionSlot_Accept;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Cancel:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Press;
            evt.slot       = UI_EventActionSlot_Cancel;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          
          //- rjf: directional movement & text controls
          //
          // NOTE(rjf): These all get funneled into a separate intermediate that
          // can be used by the UI build phase for navigation and stuff, as well
          // as builder codepaths that want to use these controls to modify text.
          //
          case RD_CmdKind_MoveLeft:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRight:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUp:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDown:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveLeftSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRightSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveLeftChunk:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRightChunk:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpChunk:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownChunk:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpPage:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownPage:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpWhole:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownWhole:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveLeftChunkSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRightChunkSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpChunkSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownChunkSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpPageSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownPageSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpWholeSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownWholeSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpReorder:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_Reorder;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownReorder:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_Reorder;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveHome:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveEnd:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveHomeSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveEndSelect:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_SelectAll:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt1 = zero_struct;
            evt1.kind       = UI_EventKind_Navigate;
            evt1.delta_unit = UI_EventDeltaUnit_Whole;
            evt1.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt1);
            UI_Event evt2 = zero_struct;
            evt2.kind       = UI_EventKind_Navigate;
            evt2.flags      = UI_EventFlag_KeepMark;
            evt2.delta_unit = UI_EventDeltaUnit_Whole;
            evt2.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt2);
          }break;
          case RD_CmdKind_DeleteSingle:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_DeleteChunk:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_BackspaceSingle:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete|UI_EventFlag_ZeroDeltaOnSelect;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_BackspaceChunk:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Copy:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind  = UI_EventKind_Edit;
            evt.flags = UI_EventFlag_Copy|UI_EventFlag_KeepMark;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Cut:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind  = UI_EventKind_Edit;
            evt.flags = UI_EventFlag_Copy|UI_EventFlag_Delete;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Paste:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind   = UI_EventKind_Text;
            evt.string = os_get_clipboard_text(scratch.arena);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_InsertText:
          {
            RD_Cfg *window = rd_cfg_from_id(rd_regs()->window);
            RD_WindowState *ws = rd_window_state_from_cfg(window);
            UI_Event evt = zero_struct;
            evt.kind   = UI_EventKind_Text;
            evt.string = rd_regs()->string;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: gather targets
    //
    D_TargetArray targets = {0};
    ProfScope("gather targets")
    {
      RD_CfgList target_cfgs = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("target"));
      targets.count = target_cfgs.count;
      targets.v = push_array(scratch.arena, D_Target, targets.count);
      U64 idx = 0;
      for(RD_CfgNode *n = target_cfgs.first; n != 0; n = n->next)
      {
        RD_Cfg *src = n->v;
        B32 src_is_disabled = rd_disabled_from_cfg(src);
        if(src_is_disabled)
        {
          targets.count -= 1;
          continue;
        }
        targets.v[idx] = rd_target_from_cfg(scratch.arena, src);
        idx += 1;
      }
    }
    
    ////////////////////////////
    //- rjf: gather breakpoints & meta-evals (for the engine, meta-evals can only be referenced by breakpoints)
    //
    D_BreakpointArray breakpoints = {0};
    ProfScope("gather breakpoints & meta-evals")
    {
      RD_CfgList bp_cfgs = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("breakpoint"));
      breakpoints.count = bp_cfgs.count;
      breakpoints.v = push_array(scratch.arena, D_Breakpoint, breakpoints.count);
      U64 idx = 0;
      for(RD_CfgNode *n = bp_cfgs.first; n != 0; n = n->next)
      {
        RD_Cfg *src_bp = n->v;
        B32 src_bp_is_disabled = rd_disabled_from_cfg(src_bp);
        if(src_bp_is_disabled)
        {
          breakpoints.count -= 1;
          continue;
        }
        RD_Location src_bp_loc = rd_location_from_cfg(src_bp);
        String8 src_bp_cnd = rd_cfg_child_from_string(src_bp, str8_lit("condition"))->first->string;
        
        //- rjf: walk conditional breakpoint expression tree - for each leaf identifier,
        // determine if it resolves to a meta-evaluation. if it does, compute the meta
        // evaluation data & store.
        //
        // for many conditions, we can statically-disqualify the breakpoint, if it only
        // references frontend-controlled meta-evaluation state. in such cases, we just
        // want to never send the user breakpoint to the control thread, since it cannot
        // be hit anyways. so in this pass, we can also gather information about whether
        // or not it is 'static', w.r.t. the control thread.
        //
        B32 is_static_for_ctrl_thread = 0;
        if(src_bp_cnd.size != 0)
        {
          typedef struct ExprWalkTask ExprWalkTask;
          struct ExprWalkTask
          {
            ExprWalkTask *next;
            E_Expr *expr;
          };
          E_Expr *expr = e_parse_expr_from_text(scratch.arena, src_bp_cnd);
          ExprWalkTask start_task = {0, expr};
          ExprWalkTask *first_task = &start_task;
          for(ExprWalkTask *t = first_task; t != 0; t = t->next)
          {
            if(t->expr->kind == E_ExprKind_LeafIdent)
            {
              E_Expr *macro_expr = e_string2expr_lookup(e_ir_state->ctx->macro_map, t->expr->string);
              if(macro_expr != &e_expr_nil)
              {
                E_Eval eval = e_eval_from_expr(scratch.arena, macro_expr);
                switch(eval.space.kind)
                {
                  default:{is_static_for_ctrl_thread = 0;}break;
                  case E_SpaceKind_Null:
                  case RD_EvalSpaceKind_MetaCfg:
                  {
                    is_static_for_ctrl_thread = 1;
                  }break;
                }
              }
            }
            for(E_Expr *child = t->expr->first; child != &e_expr_nil; child = child->next)
            {
              ExprWalkTask *task = push_array(scratch.arena, ExprWalkTask, 1);
              task->expr = child;
              task->next = t->next;
              t->next = task;
            }
          }
        }
        
        //- rjf: if this breakpoint is conditioned & static for the control thread, then
        // we can evaluate this condition early, and decide whether or not to send this
        // breakpoint.
        B32 is_statically_disqualified = 0;
        if(is_static_for_ctrl_thread)
        {
          E_Eval eval = e_eval_from_string(scratch.arena, src_bp_cnd);
          E_Eval value_eval = e_value_eval_from_eval(eval);
          if(value_eval.value.u64 == 0)
          {
            is_statically_disqualified = 1;
          }
        }
        
        //- rjf: statically disqualified? -> skip
        if(is_statically_disqualified)
        {
          breakpoints.count -= 1;
          continue;
        }
        
        //- rjf: fill breakpoint
        D_Breakpoint *dst_bp = &breakpoints.v[idx];
        dst_bp->file_path   = src_bp_loc.file_path;
        dst_bp->pt          = src_bp_loc.pt;
        dst_bp->symbol_name = src_bp_loc.name;
        dst_bp->vaddr       = src_bp_loc.vaddr;
        dst_bp->condition   = src_bp_cnd;
        idx += 1;
      }
    }
    
    ////////////////////////////
    //- rjf: gather path maps
    //
    D_PathMapArray path_maps = {0};
    {
      RD_CfgList maps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("file_path_map"));
      path_maps.count = maps.count;
      path_maps.v = push_array(scratch.arena, D_PathMap, path_maps.count);
      U64 idx = 0;
      for(RD_CfgNode *n = maps.first; n != 0; n = n->next, idx += 1)
      {
        RD_Cfg *map = n->v;
        path_maps.v[idx].src = rd_cfg_child_from_string(map, str8_lit("source"))->first->string;
        path_maps.v[idx].dst = rd_cfg_child_from_string(map, str8_lit("dest"))->first->string;
      }
    }
    
    ////////////////////////////
    //- rjf: gather exception code filters
    //
    U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64] = {0};
    {
      Temp scratch = scratch_begin(0, 0);
      for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)0; k < CTRL_ExceptionCodeKind_COUNT; k = (CTRL_ExceptionCodeKind)(k+1))
      {
        if(ctrl_exception_code_kind_default_enable_table[k])
        {
          exception_code_filters[k/64] |= 1ull<<(k%64);
        }
      }
      RD_CfgList exception_code_filters_roots = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("exception_code_filters"));
      for(RD_CfgNode *n = exception_code_filters_roots.first; n != 0; n = n->next)
      {
        for(RD_Cfg *rule = n->v->first; rule != &rd_nil_cfg; rule = rule->next)
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
                exception_code_filters[kind/64] |= (1ull<<(kind%64));
              }
              else
              {
                exception_code_filters[kind/64] &= ~(1ull<<(kind%64));
              }
            }
          }
        }
      }
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: tick debug engine
    //
    U64 cmd_count_pre_tick = rd_state->cmds[0].count;
    D_EventList engine_events = d_tick(scratch.arena, &targets, &breakpoints, &path_maps, exception_code_filters);
    
    ////////////////////////////
    //- rjf: process debug engine events
    //
    for(D_EventNode *n = engine_events.first; n != 0; n = n->next)
    {
      D_Event *evt = &n->v;
      switch(evt->kind)
      {
        default:{}break;
        case D_EventKind_ProcessEnd:
        if(rd_state->quit_after_success)
        {
          CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
          if(evt->code == 0 && processes.count == 0)
          {
            rd_cmd(RD_CmdKind_Exit);
          }
          else if(evt->code != 0)
          {
            rd_state->quit_after_success = 0;
          }
        }break;
        case D_EventKind_Stop:
        {
          B32 need_refocus = (evt->cause != D_EventCause_SoftHalt);
          CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, evt->thread);
          U64 vaddr = evt->vaddr;
          CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
          CTRL_Entity *module = ctrl_module_from_process_vaddr(process, vaddr);
          DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
          U64 voff = ctrl_voff_from_vaddr(module, vaddr);
          U64 test_cached_vaddr = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->handle);
          
          // rjf: valid stop thread? -> select & snap
          if(need_refocus && thread != &ctrl_entity_nil && evt->cause != D_EventCause_Halt)
          {
            rd_cmd(RD_CmdKind_SelectThread, .thread = thread->handle);
          }
          
          // rjf: no stop-causing thread, but have selected thread? -> snap to selected
          CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
          if(need_refocus && (evt->cause == D_EventCause_Halt || thread == &ctrl_entity_nil) && selected_thread != &ctrl_entity_nil)
          {
            rd_cmd(RD_CmdKind_SelectThread, .thread = selected_thread->handle);
          }
          
          // rjf: no stop-causing thread, but don't have selected thread? -> snap to first available thread
          if(need_refocus && thread == &ctrl_entity_nil && selected_thread == &ctrl_entity_nil)
          {
            CTRL_EntityList threads = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Thread);
            CTRL_Entity *first_available_thread = ctrl_entity_list_first(&threads);
            rd_cmd(RD_CmdKind_SelectThread, .thread = first_available_thread->handle);
          }
          
          // rjf: increment breakpoint hit counts
          if(evt->cause == D_EventCause_UserBreakpoint)
          {
            RD_CfgList bps = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("breakpoint"));
            for(RD_CfgNode *n = bps.first; n != 0; n = n->next)
            {
              RD_Cfg *bp = n->v;
              RD_Cfg *hit_count_root = rd_cfg_child_from_string_or_alloc(bp, str8_lit("hit_count"));
              U64 hit_count = 0;
              try_u64_from_str8_c_rules(hit_count_root->first->string, &hit_count);
              RD_Location loc = rd_location_from_cfg(bp);
              D_LineList loc_lines = d_lines_from_file_path_line_num(scratch.arena, loc.file_path, loc.pt.line);
              if(loc_lines.first != 0)
              {
                for(D_LineNode *n = loc_lines.first; n != 0; n = n->next)
                {
                  if(contains_1u64(n->v.voff_range, voff))
                  {
                    hit_count += 1;
                    break;
                  }
                }
              }
              else if(loc.vaddr != 0 && vaddr == loc.vaddr)
              {
                hit_count += 1;
              }
              else if(loc.name.size != 0)
              {
                U64 symb_voff = d_voff_from_dbgi_key_symbol_name(&dbgi_key, loc.name);
                if(symb_voff == voff)
                {
                  hit_count += 1;
                }
              }
              rd_cfg_new_replacef(hit_count_root, "%I64u", hit_count);
            }
          }
          
          // rjf: focus window if none focused, and if we have a thread to snap to
          if(need_refocus && (selected_thread != &ctrl_entity_nil || thread != &ctrl_entity_nil))
          {
            B32 any_window_is_focused = 0;
            for(RD_WindowState *ws = rd_state->first_window_state; ws != &rd_nil_window_state; ws = ws->order_next)
            {
              if(os_window_is_focused(ws->os))
              {
                any_window_is_focused = 1;
                break;
              }
            }
            if(!any_window_is_focused)
            {
              RD_Cfg *last_focused_window = rd_cfg_from_id(rd_state->last_focused_window);
              RD_WindowState *ws = rd_window_state_from_cfg(last_focused_window);
              if(ws == &rd_nil_window_state)
              {
                ws = rd_state->first_window_state;
              }
              if(ws != &rd_nil_window_state)
              {
                os_window_set_minimized(ws->os, 0);
                os_window_bring_to_front(ws->os);
                os_window_focus(ws->os);
              }
            }
          }
        }break;
      }
    }
    
    ////////////////////////////
    //- rjf: early-out if no new commands
    //
    if(rd_state->cmds[0].count == cmd_count_pre_tick)
    {
      break;
    }
  }
  
  //////////////////////////////
  //- rjf: retry find-thread
  //
  if(!ctrl_handle_match(ctrl_handle_zero(), find_thread_retry))
  {
    rd_cmd(RD_CmdKind_FindThread, .thread = find_thread_retry);
  }
  
  //////////////////////////////
  //- rjf: animate confirmation
  //
  {
    F32 rate = rd_setting_b32_from_name(str8_lit("menu_animations")) ? 1 - pow_f32(2, (-10.f * rd_state->frame_dt)) : 1.f;
    B32 popup_open = rd_state->popup_active;
    rd_state->popup_t += rate * ((F32)!!popup_open-rd_state->popup_t);
    if(abs_f32(rd_state->popup_t - (F32)!!popup_open) > 0.005f)
    {
      rd_request_frame();
    }
  }
  
  ////////////////////////////
  //- rjf: rotate command slots, bump command gen counter
  //
  // in this step, we rotate the ring buffer of command batches (command
  // arenas & lists). when the cmds_gen (the position of the ring buffer)
  // is even, the command queue is in a "read/write" mode, and this is uniquely
  // usable by the core - this is done so that commands in the core can push
  // other commands, and have those other commands processed on the same frame.
  //
  // in view code, however, they can only use the current command queue in a
  // "read only" mode, because new commands pushed by those views must be
  // processed first by the core. so, before calling into view code, the
  // cmds_gen is incremented to be *odd*. this way, the views will *write*
  // commands into the 0 slot, but *read* from the 1 slot (which will contain
  // this frame's commands).
  //
  // after view code runs, the generation number is incremented back to even.
  // the commands pushed by the view will be in the queue, and the core can
  // treat that queue as r/w again.
  //
  if(depth == 0)
  {
    // rjf: rotate
    {
      Arena *first_arena = rd_state->cmds_arenas[0];
      RD_CmdList first_cmds = rd_state->cmds[0];
      MemoryCopy(rd_state->cmds_arenas,
                 rd_state->cmds_arenas+1,
                 sizeof(rd_state->cmds_arenas[0])*(ArrayCount(rd_state->cmds_arenas)-1));
      MemoryCopy(rd_state->cmds,
                 rd_state->cmds+1,
                 sizeof(rd_state->cmds[0])*(ArrayCount(rd_state->cmds)-1));
      rd_state->cmds_arenas[ArrayCount(rd_state->cmds_arenas)-1] = first_arena;
      rd_state->cmds[ArrayCount(rd_state->cmds_arenas)-1] = first_cmds;
    }
    
    // rjf: clear next batch
    {
      arena_clear(rd_state->cmds_arenas[0]);
      MemoryZeroStruct(&rd_state->cmds[0]);
    }
    
    // rjf: bump
    {
      rd_state->cmds_gen += 1;
    }
  }
  
  //////////////////////////////
  //- rjf: compute all ambiguous paths from view titles
  //
  ProfScope("compute all ambiguous paths from view titles")
  {
    Temp scratch = scratch_begin(0, 0);
    rd_state->ambiguous_path_slots_count = 512;
    rd_state->ambiguous_path_slots = push_array(rd_frame_arena(), RD_AmbiguousPathNode *, rd_state->ambiguous_path_slots_count);
    for(RD_WindowState *ws = rd_state->first_window_state; ws != &rd_nil_window_state; ws = ws->order_next)
    {
      RD_Cfg *window = rd_cfg_from_id(ws->cfg_id);
      RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, window);
      for(RD_PanelNode *p = panel_tree.root; p != &rd_nil_panel_node; p = rd_panel_node_rec__depth_first_pre(panel_tree.root, p).next)
      {
        for(RD_CfgNode *tab_n = p->tabs.first; tab_n != 0; tab_n = tab_n->next)
        {
          RD_Cfg *tab = tab_n->v;
          if(rd_cfg_is_project_filtered(tab))
          {
            continue;
          }
          RD_RegsScope(.view = tab->id)
          {
            String8 eval_string = rd_expr_from_cfg(tab);
            String8 file_path = rd_file_path_from_eval_string(scratch.arena, eval_string);
            if(file_path.size != 0)
            {
              String8 name = str8_skip_last_slash(file_path);
              U64 hash = d_hash_from_string__case_insensitive(name);
              U64 slot_idx = hash%rd_state->ambiguous_path_slots_count;
              RD_AmbiguousPathNode *node = 0;
              for(RD_AmbiguousPathNode *n = rd_state->ambiguous_path_slots[slot_idx];
                  n != 0;
                  n = n->next)
              {
                if(str8_match(n->name, name, StringMatchFlag_CaseInsensitive))
                {
                  node = n;
                  break;
                }
              }
              if(node == 0)
              {
                node = push_array(rd_frame_arena(), RD_AmbiguousPathNode, 1);
                SLLStackPush(rd_state->ambiguous_path_slots[slot_idx], node);
                node->name = push_str8_copy(rd_frame_arena(), name);
              }
              str8_list_push(rd_frame_arena(), &node->paths, push_str8_copy(rd_frame_arena(), file_path));
            }
          }
        }
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: set name matching parameters; begin matching
  //
  {
    DI_KeyList keys_list = d_push_active_dbgi_key_list(scratch.arena);
    DI_KeyArray keys = di_key_array_from_list(scratch.arena, &keys_list);
    di_match_store_begin(rd_state->match_store, keys);
  }
  
  //////////////////////////////
  //- rjf: update/render all windows
  //
  {
    dr_begin_frame();
    RD_CfgList windows = rd_cfg_top_level_list_from_string(scratch.arena, str8_lit("window"));
    for(RD_CfgNode *n = windows.first; n != 0; n = n->next)
    {
      RD_Cfg *window = n->v;
      RD_WindowState *w = rd_window_state_from_cfg(window);
      B32 window_is_focused = os_window_is_focused(w->os);
      if(window_is_focused)
      {
        rd_state->last_focused_window = w->cfg_id;
      }
      rd_push_regs();
      rd_regs()->window = w->cfg_id;
      rd_window_frame();
      MemoryZeroStruct(&w->ui_events);
      RD_Regs *window_regs = rd_pop_regs();
      if(rd_state->last_focused_window == w->cfg_id)
      {
        MemoryCopyStruct(rd_regs(), window_regs);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: simulate lag
  //
  if(DEV_simulate_lag)
  {
    os_sleep_milliseconds(300);
  }
  
  //////////////////////////////
  //- rjf: end drag/drop if needed
  //
  if(rd_state->drag_drop_state == RD_DragDropState_Dropping)
  {
    rd_state->drag_drop_state = RD_DragDropState_Null;
  }
  
  //////////////////////////////
  //- rjf: clear frame request state
  //
  if(rd_state->num_frames_requested > 0)
  {
    rd_state->num_frames_requested -= 1;
  }
  
  //////////////////////////////
  //- rjf: close scopes
  //
  if(depth == 0)
  {
    di_scope_close(rd_state->frame_di_scope);
  }
  
  //////////////////////////////
  //- rjf: submit rendering to all windows
  //
  ProfScope("submit rendering to all windows")
  {
    r_begin_frame();
    for(RD_WindowState *w = rd_state->first_window_state; w != &rd_nil_window_state; w = w->order_next)
    {
      r_window_begin_frame(w->os, w->r);
      dr_submit_bucket(w->os, w->r, w->draw_bucket);
      r_window_end_frame(w->os, w->r);
    }
    r_end_frame();
  }
  
  //////////////////////////////
  //- rjf: show windows after first frame
  //
  if(depth == 0)
  {
    RD_CfgIDList windows_to_show = {0};
    for(RD_WindowState *w = rd_state->first_window_state; w != &rd_nil_window_state; w = w->order_next)
    {
      if(w->frames_alive == 1)
      {
        rd_cfg_id_list_push(scratch.arena, &windows_to_show, w->cfg_id);
      }
    }
    for(RD_CfgIDNode *n = windows_to_show.first; n != 0; n = n->next)
    {
      RD_Cfg *window = rd_cfg_from_id(n->v);
      RD_WindowState *ws = rd_window_state_from_cfg(window);
      DeferLoop(depth += 1, depth -= 1) os_window_first_paint(ws->os);
    }
  }
  
  //////////////////////////////
  //- rjf: determine frame time, record into history
  //
  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  rd_state->frame_time_us_history[rd_state->frame_index%ArrayCount(rd_state->frame_time_us_history)] = frame_time_us;
  
  //////////////////////////////
  //- rjf: bump frame time counters
  //
  rd_state->frame_index += 1;
  rd_state->time_in_seconds += rd_state->frame_dt;
  
  //////////////////////////////
  //- rjf: bump command batch ring buffer generation
  //
  if(depth == 0)
  {
    rd_state->cmds_gen += 1;
  }
  
  //////////////////////////////
  //- rjf: collect logs
  //
  ProfScope("collect logs")
  {
    LogScopeResult log = log_scope_end(scratch.arena);
    os_append_data_to_file_path(rd_state->log_path, log.strings[LogMsgKind_Info]);
    if(log.strings[LogMsgKind_UserError].size != 0)
    {
      for(RD_WindowState *ws = rd_state->first_window_state; ws != &rd_nil_window_state; ws = ws->order_next)
      {
        ws->error_string_size = Min(sizeof(ws->error_buffer), log.strings[LogMsgKind_UserError].size);
        MemoryCopy(ws->error_buffer, log.strings[LogMsgKind_UserError].str, ws->error_string_size);
        ws->error_t = 1.f;
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}
