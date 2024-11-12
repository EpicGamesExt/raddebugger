// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef MARKUP_LAYER_COLOR
#define MARKUP_LAYER_COLOR 0.10f, 0.20f, 0.25f

////////////////////////////////
//~ rjf: Generated Code

#include "generated/raddbg.meta.c"

////////////////////////////////
//~ rjf: Handles

internal RD_Handle
rd_handle_zero(void)
{
  RD_Handle result = {0};
  return result;
}

internal B32
rd_handle_match(RD_Handle a, RD_Handle b)
{
  return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1]);
}

internal void
rd_handle_list_push_node(RD_HandleList *list, RD_HandleNode *node)
{
  DLLPushBack(list->first, list->last, node);
  list->count += 1;
}

internal void
rd_handle_list_push(Arena *arena, RD_HandleList *list, RD_Handle handle)
{
  RD_HandleNode *n = push_array(arena, RD_HandleNode, 1);
  n->handle = handle;
  rd_handle_list_push_node(list, n);
}

internal RD_HandleList
rd_handle_list_copy(Arena *arena, RD_HandleList list)
{
  RD_HandleList result = {0};
  for(RD_HandleNode *n = list.first; n != 0; n = n->next)
  {
    rd_handle_list_push(arena, &result, n->handle);
  }
  return result;
}

////////////////////////////////
//~ rjf: Config Type Functions

internal void
rd_cfg_table_push_unparsed_string(Arena *arena, RD_CfgTable *table, String8 string, RD_CfgSrc source)
{
  if(table->slot_count == 0)
  {
    table->slot_count = 64;
    table->slots = push_array(arena, RD_CfgSlot, table->slot_count);
  }
  MD_TokenizeResult tokenize = md_tokenize_from_text(arena, string);
  MD_ParseResult parse = md_parse_from_text_tokens(arena, str8_lit(""), string, tokenize.tokens);
  for MD_EachNode(tln, parse.root->first) if(tln->string.size != 0)
  {
    // rjf: map string -> hash*slot
    String8 string = str8(tln->string.str, tln->string.size);
    U64 hash = d_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % table->slot_count;
    RD_CfgSlot *slot = &table->slots[slot_idx];
    
    // rjf: find existing value for this string
    RD_CfgVal *val = 0;
    for(RD_CfgVal *v = slot->first; v != 0; v = v->hash_next)
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
      val = push_array(arena, RD_CfgVal, 1);
      val->string = push_str8_copy(arena, string);
      val->insertion_stamp = table->insertion_stamp_counter;
      SLLStackPush_N(slot->first, val, hash_next);
      SLLQueuePush_N(table->first_val, table->last_val, val, linear_next);
      table->insertion_stamp_counter += 1;
    }
    
    // rjf: create new node within this value
    RD_CfgTree *tree = push_array(arena, RD_CfgTree, 1);
    SLLQueuePush_NZ(&d_nil_cfg_tree, val->first, val->last, tree, next);
    tree->source = source;
    tree->root   = md_tree_copy(arena, tln);
  }
}

internal RD_CfgVal *
rd_cfg_val_from_string(RD_CfgTable *table, String8 string)
{
  RD_CfgVal *result = &d_nil_cfg_val;
  if(table->slot_count != 0)
  {
    U64 hash = d_hash_from_string__case_insensitive(string);
    U64 slot_idx = hash % table->slot_count;
    RD_CfgSlot *slot = &table->slots[slot_idx];
    for(RD_CfgVal *val = slot->first; val != 0; val = val->hash_next)
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
//~ rjf: Registers Type Functions

internal void
rd_regs_copy_contents(Arena *arena, RD_Regs *dst, RD_Regs *src)
{
  MemoryCopyStruct(dst, src);
  dst->entity_list = rd_handle_list_copy(arena, src->entity_list);
  dst->file_path   = push_str8_copy(arena, src->file_path);
  dst->lines       = d_line_list_copy(arena, &src->lines);
  dst->dbgi_key    = di_key_copy(arena, &src->dbgi_key);
  dst->string      = push_str8_copy(arena, src->string);
  dst->cmd_name    = push_str8_copy(arena, src->cmd_name);
  dst->params_tree = md_tree_copy(arena, src->params_tree);
  if(dst->entity_list.count == 0 && !rd_handle_match(rd_handle_zero(), dst->entity))
  {
    rd_handle_list_push(arena, &dst->entity_list, dst->entity);
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
//~ rjf: Entity Functions

//- rjf: nil

internal B32
rd_entity_is_nil(RD_Entity *entity)
{
  return (entity == 0 || entity == &d_nil_entity);
}

//- rjf: handle <-> entity conversions

internal U64
rd_index_from_entity(RD_Entity *entity)
{
  return (U64)(entity - rd_state->entities_base);
}

internal RD_Handle
rd_handle_from_entity(RD_Entity *entity)
{
  RD_Handle handle = rd_handle_zero();
  if(!rd_entity_is_nil(entity))
  {
    handle.u64[0] = rd_index_from_entity(entity);
    handle.u64[1] = entity->gen;
  }
  return handle;
}

internal RD_Entity *
rd_entity_from_handle(RD_Handle handle)
{
  RD_Entity *result = rd_state->entities_base + handle.u64[0];
  if(handle.u64[0] >= rd_state->entities_count || result->gen != handle.u64[1])
  {
    result = &d_nil_entity;
  }
  return result;
}

internal RD_HandleList
rd_handle_list_from_entity_list(Arena *arena, RD_EntityList entities)
{
  RD_HandleList result = {0};
  for(RD_EntityNode *n = entities.first; n != 0; n = n->next)
  {
    RD_Handle handle = rd_handle_from_entity(n->entity);
    rd_handle_list_push(arena, &result, handle);
  }
  return result;
}

//- rjf: entity recursion iterators

internal RD_EntityRec
rd_entity_rec_depth_first(RD_Entity *entity, RD_Entity *subtree_root, U64 sib_off, U64 child_off)
{
  RD_EntityRec result = {0};
  if(!rd_entity_is_nil(*MemberFromOffset(RD_Entity **, entity, child_off)))
  {
    result.next = *MemberFromOffset(RD_Entity **, entity, child_off);
    result.push_count = 1;
  }
  else for(RD_Entity *parent = entity; parent != subtree_root && !rd_entity_is_nil(parent); parent = parent->parent)
  {
    if(parent != subtree_root && !rd_entity_is_nil(*MemberFromOffset(RD_Entity **, parent, sib_off)))
    {
      result.next = *MemberFromOffset(RD_Entity **, parent, sib_off);
      break;
    }
    result.pop_count += 1;
  }
  return result;
}

//- rjf: ancestor/child introspection

internal RD_Entity *
rd_entity_child_from_kind(RD_Entity *entity, RD_EntityKind kind)
{
  RD_Entity *result = &d_nil_entity;
  for(RD_Entity *child = entity->first; !rd_entity_is_nil(child); child = child->next)
  {
    if(!(child->flags & RD_EntityFlag_MarkedForDeletion) && child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

internal RD_Entity *
rd_entity_ancestor_from_kind(RD_Entity *entity, RD_EntityKind kind)
{
  RD_Entity *result = &d_nil_entity;
  for(RD_Entity *p = entity->parent; !rd_entity_is_nil(p); p = p->parent)
  {
    if(p->kind == kind)
    {
      result = p;
      break;
    }
  }
  return result;
}

internal RD_EntityList
rd_push_entity_child_list_with_kind(Arena *arena, RD_Entity *entity, RD_EntityKind kind)
{
  RD_EntityList result = {0};
  for(RD_Entity *child = entity->first; !rd_entity_is_nil(child); child = child->next)
  {
    if(child->kind == kind)
    {
      rd_entity_list_push(arena, &result, child);
    }
  }
  return result;
}

internal RD_Entity *
rd_entity_child_from_string_and_kind(RD_Entity *parent, String8 string, RD_EntityKind kind)
{
  RD_Entity *result = &d_nil_entity;
  for(RD_Entity *child = parent->first; !rd_entity_is_nil(child); child = child->next)
  {
    if(str8_match(child->string, string, 0) && child->kind == kind)
    {
      result = child;
      break;
    }
  }
  return result;
}

//- rjf: entity list building

internal void
rd_entity_list_push(Arena *arena, RD_EntityList *list, RD_Entity *entity)
{
  RD_EntityNode *n = push_array(arena, RD_EntityNode, 1);
  n->entity = entity;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal RD_EntityArray
rd_entity_array_from_list(Arena *arena, RD_EntityList *list)
{
  RD_EntityArray result = {0};
  result.count = list->count;
  result.v = push_array(arena, RD_Entity *, result.count);
  U64 idx = 0;
  for(RD_EntityNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    result.v[idx] = n->entity;
  }
  return result;
}

//- rjf: entity fuzzy list building

internal RD_EntityFuzzyItemArray
rd_entity_fuzzy_item_array_from_entity_list_needle(Arena *arena, RD_EntityList *list, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  RD_EntityArray array = rd_entity_array_from_list(scratch.arena, list);
  RD_EntityFuzzyItemArray result = rd_entity_fuzzy_item_array_from_entity_array_needle(arena, &array, needle);
  return result;
}

internal RD_EntityFuzzyItemArray
rd_entity_fuzzy_item_array_from_entity_array_needle(Arena *arena, RD_EntityArray *array, String8 needle)
{
  Temp scratch = scratch_begin(&arena, 1);
  RD_EntityFuzzyItemArray result = {0};
  result.count = array->count;
  result.v = push_array(arena, RD_EntityFuzzyItem, result.count);
  U64 result_idx = 0;
  for(U64 src_idx = 0; src_idx < array->count; src_idx += 1)
  {
    RD_Entity *entity = array->v[src_idx];
    String8 display_string = rd_display_string_from_entity(scratch.arena, entity);
    FuzzyMatchRangeList matches = fuzzy_match_find(arena, needle, display_string);
    if(matches.count >= matches.needle_part_count)
    {
      result.v[result_idx].entity = entity;
      result.v[result_idx].matches = matches;
      result_idx += 1;
    }
    else
    {
      String8 search_tags = rd_search_tags_from_entity(scratch.arena, entity);
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
rd_full_path_from_entity(Arena *arena, RD_Entity *entity)
{
  String8 string = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List strs = {0};
    for(RD_Entity *e = entity; !rd_entity_is_nil(e); e = e->parent)
    {
      if(e->kind == RD_EntityKind_File)
      {
        str8_list_push_front(scratch.arena, &strs, e->string);
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
rd_display_string_from_entity(Arena *arena, RD_Entity *entity)
{
  String8 result = {0};
  switch(entity->kind)
  {
    default:
    {
      if(entity->string.size != 0)
      {
        result = push_str8_copy(arena, entity->string);
      }
      else
      {
        String8 kind_string = d_entity_kind_display_string_table[entity->kind];
        result = push_str8f(arena, "%S $%I64u", kind_string, entity->id);
      }
    }break;
    
    case RD_EntityKind_Target:
    {
      if(entity->string.size != 0)
      {
        result = push_str8_copy(arena, entity->string);
      }
      else
      {
        RD_Entity *exe = rd_entity_child_from_kind(entity, RD_EntityKind_Executable);
        result = push_str8_copy(arena, exe->string);
      }
    }break;
    
    case RD_EntityKind_Breakpoint:
    {
      if(entity->string.size != 0)
      {
        result = push_str8_copy(arena, entity->string);
      }
      else
      {
        RD_Entity *loc = rd_entity_child_from_kind(entity, RD_EntityKind_Location);
        if(loc->flags & RD_EntityFlag_HasTextPoint)
        {
          result = push_str8f(arena, "%S:%I64d:%I64d", str8_skip_last_slash(loc->string), loc->text_point.line, loc->text_point.column);
        }
        else if(loc->flags & RD_EntityFlag_HasVAddr)
        {
          result = str8_from_u64(arena, loc->vaddr, 16, 16, 0);
        }
        else if(loc->string.size != 0)
        {
          result = push_str8_copy(arena, loc->string);
        }
      }
    }break;
    
    case RD_EntityKind_Process:
    {
      RD_Entity *main_mod_child = rd_entity_child_from_kind(entity, RD_EntityKind_Module);
      String8 main_mod_name = str8_skip_last_slash(main_mod_child->string);
      result = push_str8f(arena, "%S%s%sPID: %i%s",
                          main_mod_name,
                          main_mod_name.size != 0 ? " " : "",
                          main_mod_name.size != 0 ? "(" : "",
                          entity->ctrl_id,
                          main_mod_name.size != 0 ? ")" : "");
    }break;
    
    case RD_EntityKind_Thread:
    {
      String8 name = entity->string;
      if(name.size == 0)
      {
        RD_Entity *process = rd_entity_ancestor_from_kind(entity, RD_EntityKind_Process);
        RD_Entity *first_thread = rd_entity_child_from_kind(process, RD_EntityKind_Thread);
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
    
    case RD_EntityKind_Module:
    {
      result = push_str8_copy(arena, str8_skip_last_slash(entity->string));
    }break;
    
    case RD_EntityKind_RecentProject:
    {
      result = push_str8_copy(arena, str8_skip_last_slash(entity->string));
    }break;
  }
  return result;
}

//- rjf: extra search tag strings for fuzzy filtering entities

internal String8
rd_search_tags_from_entity(Arena *arena, RD_Entity *entity)
{
  String8 result = {0};
  if(entity->kind == RD_EntityKind_Thread)
  {
    Temp scratch = scratch_begin(&arena, 1);
    CTRL_Entity *entity_ctrl = ctrl_entity_from_handle(d_state->ctrl_entity_store, entity->ctrl_handle);
    CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity_ctrl, CTRL_EntityKind_Process);
    CTRL_Unwind unwind = d_query_cached_unwind_from_thread(entity_ctrl);
    String8List strings = {0};
    for(U64 frame_num = unwind.frames.count; frame_num > 0; frame_num -= 1)
    {
      CTRL_UnwindFrame *f = &unwind.frames.v[frame_num-1];
      U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
      CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
      U64 rip_voff = ctrl_voff_from_vaddr(module, rip_vaddr);
      DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
      String8 procedure_name = d_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff, 0);
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
rd_hsva_from_entity(RD_Entity *entity)
{
  Vec4F32 result = {0};
  if(entity->flags & RD_EntityFlag_HasColor)
  {
    result = entity->color_hsva;
  }
  return result;
}

internal Vec4F32
rd_rgba_from_entity(RD_Entity *entity)
{
  Vec4F32 result = {0};
  if(entity->flags & RD_EntityFlag_HasColor)
  {
    Vec3F32 hsv = v3f32(entity->color_hsva.x, entity->color_hsva.y, entity->color_hsva.z);
    Vec3F32 rgb = rgb_from_hsv(hsv);
    result = v4f32(rgb.x, rgb.y, rgb.z, entity->color_hsva.w);
  }
  else switch(entity->kind)
  {
    default:{}break;
    case RD_EntityKind_Breakpoint:
    {
      result = rd_rgba_from_theme_color(RD_ThemeColor_Breakpoint);
    }break;
  }
  return result;
}

//- rjf: entity -> expansion tree keys

internal EV_Key
rd_ev_key_from_entity(RD_Entity *entity)
{
  EV_Key parent_key = rd_parent_ev_key_from_entity(entity);
  EV_Key key = ev_key_make(ev_hash_from_key(parent_key), (U64)entity);
  return key;
}

internal EV_Key
rd_parent_ev_key_from_entity(RD_Entity *entity)
{
  EV_Key parent_key = ev_key_make(5381, (U64)entity);
  return parent_key;
}

//- rjf: entity -> evaluation

internal RD_EntityEval *
rd_eval_from_entity(Arena *arena, RD_Entity *entity)
{
  RD_EntityEval *eval = push_array(arena, RD_EntityEval, 1);
  {
    RD_Entity *loc = rd_entity_child_from_kind(entity, RD_EntityKind_Location);
    RD_Entity *cnd = rd_entity_child_from_kind(entity, RD_EntityKind_Condition);
    String8 label_string = push_str8_copy(arena, entity->string);
    String8 loc_string = {0};
    if(loc->flags & RD_EntityFlag_HasTextPoint)
    {
      loc_string = push_str8f(arena, "%S:%I64u:%I64u", loc->string, loc->text_point.line, loc->text_point.column);
    }
    else if(loc->flags & RD_EntityFlag_HasVAddr)
    {
      loc_string = push_str8f(arena, "0x%I64x", loc->vaddr);
    }
    String8 cnd_string = push_str8_copy(arena, cnd->string);
    eval->enabled      = !entity->disabled;
    eval->hit_count    = entity->u64;
    eval->label_off    = (U64)((U8 *)label_string.str - (U8 *)eval);
    eval->location_off = (U64)((U8 *)loc_string.str - (U8 *)eval);
    eval->condition_off= (U64)((U8 *)cnd_string.str - (U8 *)eval);
  }
  return eval;
}

////////////////////////////////
//~ rjf: View Type Functions

internal B32
rd_view_is_nil(RD_View *view)
{
  return (view == 0 || view == &rd_nil_view);
}

internal B32
rd_view_is_project_filtered(RD_View *view)
{
  B32 result = 0;
  String8 view_project = view->project_path;
  if(view_project.size != 0)
  {
    RD_ViewRuleKind kind = rd_view_rule_kind_from_string(view->spec->string);
    // TODO(rjf): @hack hack hack - this should be completely determined if the view
    // is parameterized by an expression, but that is currently the same string as the
    // query, and so we can't rely on that. when query expressions are separated from
    // filter strings, we can rely on that here.
    if((kind == RD_ViewRuleKind_Text ||
        kind == RD_ViewRuleKind_Disasm ||
        kind == RD_ViewRuleKind_Memory ||
        kind == RD_ViewRuleKind_Bitmap ||
        kind == RD_ViewRuleKind_Geo3D) &&
       view->query_string_size != 0)
    {
      String8 current_project = rd_cfg_path_from_src(RD_CfgSrc_Project);
      result = !path_match_normalized(view_project, current_project);
    }
  }
  return result;
}

internal RD_Handle
rd_handle_from_view(RD_View *view)
{
  RD_Handle handle = rd_handle_zero();
  if(!rd_view_is_nil(view))
  {
    handle.u64[0] = (U64)view;
    handle.u64[1] = view->generation;
  }
  return handle;
}

internal RD_View *
rd_view_from_handle(RD_Handle handle)
{
  RD_View *result = (RD_View *)handle.u64[0];
  if(rd_view_is_nil(result) || result->generation != handle.u64[1])
  {
    result = &rd_nil_view;
  }
  return result;
}

////////////////////////////////
//~ rjf: View Spec Type Functions

internal RD_ViewRuleKind
rd_view_rule_kind_from_string(String8 string)
{
  RD_ViewRuleKind kind = RD_ViewRuleKind_Null;
  for EachEnumVal(RD_ViewRuleKind, k)
  {
    if(str8_match(string, rd_view_rule_kind_info_table[k].string, 0))
    {
      kind = k;
      break;
    }
  }
  return kind;
}

internal RD_ViewRuleInfo *
rd_view_rule_info_from_kind(RD_ViewRuleKind kind)
{
  return &rd_view_rule_kind_info_table[kind];
}

internal RD_ViewRuleInfo *
rd_view_rule_info_from_string(String8 string)
{
  RD_ViewRuleInfo *result = &rd_nil_view_rule_info;
  {
    RD_ViewRuleKind kind = rd_view_rule_kind_from_string(string);
    if(kind != RD_ViewRuleKind_Null)
    {
      result = &rd_view_rule_kind_info_table[kind];
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Panel Type Functions

//- rjf: basic type functions

internal B32
rd_panel_is_nil(RD_Panel *panel)
{
  return panel == 0 || panel == &rd_nil_panel;
}

internal RD_Handle
rd_handle_from_panel(RD_Panel *panel)
{
  RD_Handle h = {0};
  h.u64[0] = (U64)panel;
  h.u64[1] = panel->generation;
  return h;
}

internal RD_Panel *
rd_panel_from_handle(RD_Handle handle)
{
  RD_Panel *panel = (RD_Panel *)handle.u64[0];
  if(panel == 0 || panel->generation != handle.u64[1])
  {
    panel = &rd_nil_panel;
  }
  return panel;
}

internal UI_Key
rd_ui_key_from_panel(RD_Panel *panel)
{
  UI_Key panel_key = ui_key_from_stringf(ui_key_zero(), "panel_window_%p", panel);
  return panel_key;
}

//- rjf: tree construction

internal void
rd_panel_insert(RD_Panel *parent, RD_Panel *prev_child, RD_Panel *new_child)
{
  DLLInsert_NPZ(&rd_nil_panel, parent->first, parent->last, prev_child, new_child, next, prev);
  parent->child_count += 1;
  new_child->parent = parent;
}

internal void
rd_panel_remove(RD_Panel *parent, RD_Panel *child)
{
  DLLRemove_NPZ(&rd_nil_panel, parent->first, parent->last, child, next, prev);
  child->next = child->prev = child->parent = &rd_nil_panel;
  parent->child_count -= 1;
}

//- rjf: tree walk

internal RD_PanelRec
rd_panel_rec_depth_first(RD_Panel *panel, U64 sib_off, U64 child_off)
{
  RD_PanelRec rec = {0};
  if(!rd_panel_is_nil(*MemberFromOffset(RD_Panel **, panel, child_off)))
  {
    rec.next = *MemberFromOffset(RD_Panel **, panel, child_off);
    rec.push_count = 1;
  }
  else if(!rd_panel_is_nil(*MemberFromOffset(RD_Panel **, panel, sib_off)))
  {
    rec.next = *MemberFromOffset(RD_Panel **, panel, sib_off);
  }
  else
  {
    RD_Panel *uncle = &rd_nil_panel;
    for(RD_Panel *p = panel->parent; !rd_panel_is_nil(p); p = p->parent)
    {
      rec.pop_count += 1;
      if(!rd_panel_is_nil(*MemberFromOffset(RD_Panel **, p, sib_off)))
      {
        uncle = *MemberFromOffset(RD_Panel **, p, sib_off);
        break;
      }
    }
    rec.next = uncle;
  }
  return rec;
}

//- rjf: panel -> rect calculations

internal Rng2F32
rd_target_rect_from_panel_child(Rng2F32 parent_rect, RD_Panel *parent, RD_Panel *panel)
{
  Rng2F32 rect = parent_rect;
  if(!rd_panel_is_nil(parent))
  {
    Vec2F32 parent_rect_size = dim_2f32(parent_rect);
    Axis2 axis = parent->split_axis;
    rect.p1.v[axis] = rect.p0.v[axis];
    for(RD_Panel *child = parent->first; !rd_panel_is_nil(child); child = child->next)
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
rd_target_rect_from_panel(Rng2F32 root_rect, RD_Panel *root, RD_Panel *panel)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: count ancestors
  U64 ancestor_count = 0;
  for(RD_Panel *p = panel->parent; !rd_panel_is_nil(p); p = p->parent)
  {
    ancestor_count += 1;
  }
  
  // rjf: gather ancestors
  RD_Panel **ancestors = push_array(scratch.arena, RD_Panel *, ancestor_count);
  {
    U64 ancestor_idx = 0;
    for(RD_Panel *p = panel->parent; !rd_panel_is_nil(p); p = p->parent)
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
    RD_Panel *ancestor = ancestors[ancestor_idx];
    RD_Panel *parent = ancestor->parent;
    if(!rd_panel_is_nil(parent))
    {
      parent_rect = rd_target_rect_from_panel_child(parent_rect, parent, ancestor);
    }
  }
  
  // rjf: calculate final rect
  Rng2F32 rect = rd_target_rect_from_panel_child(parent_rect, panel->parent, panel);
  
  scratch_end(scratch);
  return rect;
}

//- rjf: view ownership insertion/removal

internal void
rd_panel_insert_tab_view(RD_Panel *panel, RD_View *prev_view, RD_View *view)
{
  DLLInsert_NPZ(&rd_nil_view, panel->first_tab_view, panel->last_tab_view, prev_view, view, order_next, order_prev);
  panel->tab_view_count += 1;
  if(!rd_view_is_project_filtered(view))
  {
    panel->selected_tab_view = rd_handle_from_view(view);
  }
}

internal void
rd_panel_remove_tab_view(RD_Panel *panel, RD_View *view)
{
  if(rd_view_from_handle(panel->selected_tab_view) == view)
  {
    panel->selected_tab_view = rd_handle_zero();
    if(rd_handle_match(rd_handle_zero(), panel->selected_tab_view))
    {
      for(RD_View *v = view->order_next; !rd_view_is_nil(v); v = v->order_next)
      {
        if(!rd_view_is_project_filtered(v))
        {
          panel->selected_tab_view = rd_handle_from_view(v);
          break;
        }
      }
    }
    if(rd_handle_match(rd_handle_zero(), panel->selected_tab_view))
    {
      for(RD_View *v = view->order_prev; !rd_view_is_nil(v); v = v->order_prev)
      {
        if(!rd_view_is_project_filtered(v))
        {
          panel->selected_tab_view = rd_handle_from_view(v);
          break;
        }
      }
    }
  }
  DLLRemove_NPZ(&rd_nil_view, panel->first_tab_view, panel->last_tab_view, view, order_next, order_prev);
  panel->tab_view_count -= 1;
}

internal RD_View *
rd_selected_tab_from_panel(RD_Panel *panel)
{
  RD_View *view = rd_view_from_handle(panel->selected_tab_view);
  if(rd_view_is_project_filtered(view))
  {
    view = &rd_nil_view;
  }
  return view;
}

//- rjf: icons & display strings

internal RD_IconKind
rd_icon_kind_from_view(RD_View *view)
{
  RD_IconKind result = view->spec->icon_kind;
  return result;
}

internal DR_FancyStringList
rd_title_fstrs_from_view(Arena *arena, RD_View *view, Vec4F32 primary_color, Vec4F32 secondary_color, F32 size)
{
  DR_FancyStringList result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  String8 query = str8(view->query_buffer, view->query_string_size);
  String8 file_path = rd_file_path_from_eval_string(scratch.arena, query);
  
  //- rjf: query is file path - do specific file name strings
  if(file_path.size != 0)
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
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size*0.95f, secondary_color, string);
    }
    
    // rjf: push file name
    DR_FancyString fstr =
    {
      rd_font_from_slot(RD_FontSlot_Main),
      push_str8_copy(arena, file_name),
      primary_color,
      size,
    };
    dr_fancy_string_list_push(arena, &result, &fstr);
  }
  
  //- rjf: query is not file path - do general case, for view rule & expression
  else
  {
    DR_FancyString fstr1 =
    {
      rd_font_from_slot(RD_FontSlot_Main),
      view->spec->display_name,
      primary_color,
      size,
    };
    dr_fancy_string_list_push(arena, &result, &fstr1);
    if(query.size != 0)
    {
      DR_FancyString fstr2 =
      {
        rd_font_from_slot(RD_FontSlot_Code),
        str8_lit(" "),
        primary_color,
        size,
      };
      dr_fancy_string_list_push(arena, &result, &fstr2);
      DR_FancyString fstr3 =
      {
        rd_font_from_slot(RD_FontSlot_Code),
        push_str8_copy(arena, query),
        secondary_color,
        size*0.8f,
      };
      dr_fancy_string_list_push(arena, &result, &fstr3);
    }
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Window Type Functions

internal RD_Handle
rd_handle_from_window(RD_Window *window)
{
  RD_Handle handle = {0};
  if(window != 0)
  {
    handle.u64[0] = (U64)window;
    handle.u64[1] = window->gen;
  }
  return handle;
}

internal RD_Window *
rd_window_from_handle(RD_Handle handle)
{
  RD_Window *window = (RD_Window *)handle.u64[0];
  if(window != 0 && window->gen != handle.u64[1])
  {
    window = 0;
  }
  return window;
}

////////////////////////////////
//~ rjf: Command Parameters From Context

internal B32
rd_prefer_dasm_from_window(RD_Window *window)
{
  RD_Panel *panel = window->focused_panel;
  RD_View *view = rd_selected_tab_from_panel(panel);
  RD_ViewRuleKind view_kind = rd_view_rule_kind_from_string(view->spec->string);
  B32 result = 0;
  if(view_kind == RD_ViewRuleKind_Disasm)
  {
    result = 1;
  }
  else if(view_kind == RD_ViewRuleKind_Text)
  {
    result = 0;
  }
  else
  {
    B32 has_src = 0;
    B32 has_dasm = 0;
    for(RD_Panel *p = window->root_panel; !rd_panel_is_nil(p); p = rd_panel_rec_depth_first_pre(p).next)
    {
      RD_View *p_view = rd_selected_tab_from_panel(p);
      RD_ViewRuleKind p_view_kind = rd_view_rule_kind_from_string(p_view->spec->string);
      if(p_view_kind == RD_ViewRuleKind_Text)
      {
        has_src = 1;
      }
      if(p_view_kind == RD_ViewRuleKind_Disasm)
      {
        has_dasm = 1;
      }
    }
    if(has_src && !has_dasm) {result = 0;}
    if(has_dasm && !has_src) {result = 1;}
  }
  return result;
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
  RD_Window *window = rd_window_from_handle(rd_regs()->window);
  if(window != 0)
  {
    ui_ctx_menu_open(rd_state->ctx_menu_key, anchor_box_key, anchor_box_off);
    arena_clear(window->ctx_menu_arena);
    window->ctx_menu_regs = rd_regs_copy(window->ctx_menu_arena, rd_regs());
    window->ctx_menu_regs_slot = slot;
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
          if(n->size >= string.size+1)
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
//~ rjf: Entity State Functions

//- rjf: entity allocation + tree forming

internal RD_Entity *
rd_entity_alloc(RD_Entity *parent, RD_EntityKind kind)
{
  B32 user_defined_lifetime = !!(rd_entity_kind_flags_table[kind] & RD_EntityKindFlag_UserDefinedLifetime);
  U64 free_list_idx = !!user_defined_lifetime;
  if(rd_entity_is_nil(parent)) { parent = rd_state->entities_root; }
  
  // rjf: empty free list -> push new
  if(!rd_state->entities_free[free_list_idx])
  {
    RD_Entity *entity = push_array(rd_state->entities_arena, RD_Entity, 1);
    rd_state->entities_count += 1;
    rd_state->entities_free_count += 1;
    SLLStackPush(rd_state->entities_free[free_list_idx], entity);
  }
  
  // rjf: pop new entity off free-list
  RD_Entity *entity = rd_state->entities_free[free_list_idx];
  SLLStackPop(rd_state->entities_free[free_list_idx]);
  rd_state->entities_free_count -= 1;
  rd_state->entities_active_count += 1;
  
  // rjf: zero entity
  {
    U64 gen = entity->gen;
    MemoryZeroStruct(entity);
    entity->gen = gen;
  }
  
  // rjf: set up alloc'd entity links
  entity->first = entity->last = entity->next = entity->prev = entity->parent = &d_nil_entity;
  entity->parent = parent;
  
  // rjf: stitch up parent links
  if(rd_entity_is_nil(parent))
  {
    rd_state->entities_root = entity;
  }
  else
  {
    DLLPushBack_NPZ(&d_nil_entity, parent->first, parent->last, entity, next, prev);
  }
  
  // rjf: fill out metadata
  entity->kind = kind;
  rd_state->entities_id_gen += 1;
  entity->id = rd_state->entities_id_gen;
  entity->gen += 1;
  entity->alloc_time_us = os_now_microseconds();
  entity->params_root = &md_nil_node;
  
  // rjf: initialize to deleted, record history, then "undelete" if this allocation can be undone
  if(user_defined_lifetime)
  {
    // TODO(rjf)
  }
  
  // rjf: dirtify caches
  rd_state->kind_alloc_gens[kind] += 1;
  
  // rjf: log
  LogInfoNamedBlockF("new_entity")
  {
    log_infof("kind: \"%S\"\n", d_entity_kind_display_string_table[kind]);
    log_infof("id: $0x%I64x\n", entity->id);
  }
  
  return entity;
}

internal void
rd_entity_mark_for_deletion(RD_Entity *entity)
{
  if(!rd_entity_is_nil(entity))
  {
    entity->flags |= RD_EntityFlag_MarkedForDeletion;
    rd_state->kind_alloc_gens[entity->kind] += 1;
  }
}

internal void
rd_entity_release(RD_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: unpack
  U64 free_list_idx = !!(rd_entity_kind_flags_table[entity->kind] & RD_EntityKindFlag_UserDefinedLifetime);
  
  // rjf: release whole tree
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    RD_Entity *e;
  };
  Task start_task = {0, entity};
  Task *first_task = &start_task;
  Task *last_task = &start_task;
  for(Task *task = first_task; task != 0; task = task->next)
  {
    for(RD_Entity *child = task->e->first; !rd_entity_is_nil(child); child = child->next)
    {
      Task *t = push_array(scratch.arena, Task, 1);
      t->e = child;
      SLLQueuePush(first_task, last_task, t);
    }
    LogInfoNamedBlockF("end_entity")
    {
      String8 name = rd_display_string_from_entity(scratch.arena, task->e);
      log_infof("kind: \"%S\"\n", d_entity_kind_display_string_table[task->e->kind]);
      log_infof("id: $0x%I64x\n", task->e->id);
      log_infof("display_string: \"%S\"\n", name);
    }
    SLLStackPush(rd_state->entities_free[free_list_idx], task->e);
    rd_state->entities_free_count += 1;
    rd_state->entities_active_count -= 1;
    task->e->gen += 1;
    if(task->e->string.size != 0)
    {
      rd_name_release(task->e->string);
    }
    if(task->e->params_arena != 0)
    {
      arena_release(task->e->params_arena);
    }
    rd_state->kind_alloc_gens[task->e->kind] += 1;
  }
  
  scratch_end(scratch);
}

internal void
rd_entity_change_parent(RD_Entity *entity, RD_Entity *old_parent, RD_Entity *new_parent, RD_Entity *prev_child)
{
  Assert(entity->parent == old_parent);
  Assert(prev_child->parent == old_parent || rd_entity_is_nil(prev_child));
  if(prev_child != entity)
  {
    // rjf: fix up links
    if(!rd_entity_is_nil(old_parent))
    {
      DLLRemove_NPZ(&d_nil_entity, old_parent->first, old_parent->last, entity, next, prev);
    }
    if(!rd_entity_is_nil(new_parent))
    {
      DLLInsert_NPZ(&d_nil_entity, new_parent->first, new_parent->last, prev_child, entity, next, prev);
    }
    entity->parent = new_parent;
    
    // rjf: notify
    rd_state->kind_alloc_gens[entity->kind] += 1;
  }
}

internal RD_Entity *
rd_entity_child_from_kind_or_alloc(RD_Entity *entity, RD_EntityKind kind)
{
  RD_Entity *child = rd_entity_child_from_kind(entity, kind);
  if(rd_entity_is_nil(child))
  {
    child = rd_entity_alloc(entity, kind);
  }
  return child;
}

//- rjf: entity simple equipment

internal void
rd_entity_equip_txt_pt(RD_Entity *entity, TxtPt point)
{
  rd_require_entity_nonnil(entity, return);
  entity->text_point = point;
  entity->flags |= RD_EntityFlag_HasTextPoint;
}

internal void
rd_entity_equip_entity_handle(RD_Entity *entity, RD_Handle handle)
{
  rd_require_entity_nonnil(entity, return);
  entity->entity_handle = handle;
  entity->flags |= RD_EntityFlag_HasEntityHandle;
}

internal void
rd_entity_equip_disabled(RD_Entity *entity, B32 value)
{
  rd_require_entity_nonnil(entity, return);
  entity->disabled = value;
}

internal void
rd_entity_equip_u64(RD_Entity *entity, U64 u64)
{
  rd_require_entity_nonnil(entity, return);
  entity->u64 = u64;
  entity->flags |= RD_EntityFlag_HasU64;
}

internal void
rd_entity_equip_color_rgba(RD_Entity *entity, Vec4F32 rgba)
{
  rd_require_entity_nonnil(entity, return);
  Vec3F32 rgb = v3f32(rgba.x, rgba.y, rgba.z);
  Vec3F32 hsv = hsv_from_rgb(rgb);
  Vec4F32 hsva = v4f32(hsv.x, hsv.y, hsv.z, rgba.w);
  rd_entity_equip_color_hsva(entity, hsva);
}

internal void
rd_entity_equip_color_hsva(RD_Entity *entity, Vec4F32 hsva)
{
  rd_require_entity_nonnil(entity, return);
  entity->color_hsva = hsva;
  entity->flags |= RD_EntityFlag_HasColor;
}

internal void
rd_entity_equip_cfg_src(RD_Entity *entity, RD_CfgSrc cfg_src)
{
  rd_require_entity_nonnil(entity, return);
  entity->cfg_src = cfg_src;
}

internal void
rd_entity_equip_timestamp(RD_Entity *entity, U64 timestamp)
{
  rd_require_entity_nonnil(entity, return);
  entity->timestamp = timestamp;
}

//- rjf: control layer correllation equipment

internal void
rd_entity_equip_ctrl_handle(RD_Entity *entity, CTRL_Handle handle)
{
  rd_require_entity_nonnil(entity, return);
  entity->ctrl_handle = handle;
  entity->flags |= RD_EntityFlag_HasCtrlHandle;
}

internal void
rd_entity_equip_arch(RD_Entity *entity, Arch arch)
{
  rd_require_entity_nonnil(entity, return);
  entity->arch = arch;
  entity->flags |= RD_EntityFlag_HasArch;
}

internal void
rd_entity_equip_ctrl_id(RD_Entity *entity, U32 id)
{
  rd_require_entity_nonnil(entity, return);
  entity->ctrl_id = id;
  entity->flags |= RD_EntityFlag_HasCtrlID;
}

internal void
rd_entity_equip_stack_base(RD_Entity *entity, U64 stack_base)
{
  rd_require_entity_nonnil(entity, return);
  entity->stack_base = stack_base;
  entity->flags |= RD_EntityFlag_HasStackBase;
}

internal void
rd_entity_equip_vaddr_rng(RD_Entity *entity, Rng1U64 range)
{
  rd_require_entity_nonnil(entity, return);
  entity->vaddr_rng = range;
  entity->flags |= RD_EntityFlag_HasVAddrRng;
}

internal void
rd_entity_equip_vaddr(RD_Entity *entity, U64 vaddr)
{
  rd_require_entity_nonnil(entity, return);
  entity->vaddr = vaddr;
  entity->flags |= RD_EntityFlag_HasVAddr;
}

//- rjf: name equipment

internal void
rd_entity_equip_name(RD_Entity *entity, String8 name)
{
  rd_require_entity_nonnil(entity, return);
  if(entity->string.size != 0)
  {
    rd_name_release(entity->string);
  }
  if(name.size != 0)
  {
    entity->string = rd_name_alloc(name);
  }
  else
  {
    entity->string = str8_zero();
  }
}

//- rjf: file path map override lookups

internal String8
rd_mapped_from_file_path(Arena *arena, String8 file_path)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8 result = file_path;
  if(file_path.size != 0)
  {
    String8 file_path__normalized = path_normalized_from_string(scratch.arena, file_path);
    String8List file_path_parts = str8_split_path(scratch.arena, file_path__normalized);
    RD_EntityList maps = rd_query_cached_entity_list_with_kind(RD_EntityKind_FilePathMap);
    String8 best_map_dst = {0};
    U64 best_map_match_length = max_U64;
    String8Node *best_map_remaining_suffix_first = 0;
    for(RD_EntityNode *n = maps.first; n != 0; n = n->next)
    {
      String8 map_src = rd_entity_child_from_kind(n->entity, RD_EntityKind_Source)->string;
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
        best_map_dst = rd_entity_child_from_kind(n->entity, RD_EntityKind_Dest)->string;
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
    RD_EntityList links = rd_query_cached_entity_list_with_kind(RD_EntityKind_FilePathMap);
    for(RD_EntityNode *n = links.first; n != 0; n = n->next)
    {
      //- rjf: unpack link
      RD_Entity *link = n->entity;
      RD_Entity *src = rd_entity_child_from_kind(link, RD_EntityKind_Source);
      RD_Entity *dst = rd_entity_child_from_kind(link, RD_EntityKind_Dest);
      PathStyle src_style = PathStyle_Relative;
      PathStyle dst_style = PathStyle_Relative;
      String8List src_parts = path_normalized_list_from_string(scratch.arena, src->string, &src_style);
      String8List dst_parts = path_normalized_list_from_string(scratch.arena, dst->string, &dst_style);
      
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

//- rjf: top-level state queries

internal RD_Entity *
rd_entity_root(void)
{
  return rd_state->entities_root;
}

internal RD_EntityList
rd_push_entity_list_with_kind(Arena *arena, RD_EntityKind kind)
{
  ProfBeginFunction();
  RD_EntityList result = {0};
  for(RD_Entity *entity = rd_state->entities_root;
      !rd_entity_is_nil(entity);
      entity = rd_entity_rec_depth_first_pre(entity, &d_nil_entity).next)
  {
    if(entity->kind == kind && !(entity->flags & RD_EntityFlag_MarkedForDeletion))
    {
      rd_entity_list_push(arena, &result, entity);
    }
  }
  ProfEnd();
  return result;
}

internal RD_Entity *
rd_entity_from_id(RD_EntityID id)
{
  RD_Entity *result = &d_nil_entity;
  for(RD_Entity *e = rd_entity_root();
      !rd_entity_is_nil(e);
      e = rd_entity_rec_depth_first_pre(e, &d_nil_entity).next)
  {
    if(e->id == id)
    {
      result = e;
      break;
    }
  }
  return result;
}

internal RD_Entity *
rd_machine_entity_from_machine_id(CTRL_MachineID machine_id)
{
  RD_Entity *result = &d_nil_entity;
  for(RD_Entity *e = rd_entity_root();
      !rd_entity_is_nil(e);
      e = rd_entity_rec_depth_first_pre(e, &d_nil_entity).next)
  {
    if(e->kind == RD_EntityKind_Machine && e->ctrl_handle.machine_id == machine_id)
    {
      result = e;
      break;
    }
  }
  if(rd_entity_is_nil(result))
  {
    result = rd_entity_alloc(rd_entity_root(), RD_EntityKind_Machine);
    rd_entity_equip_ctrl_handle(result, ctrl_handle_make(machine_id, dmn_handle_zero()));
  }
  return result;
}

internal RD_Entity *
rd_entity_from_ctrl_handle(CTRL_Handle handle)
{
  RD_Entity *result = &d_nil_entity;
  if(handle.machine_id != 0 || handle.dmn_handle.u64[0] != 0)
  {
    for(RD_Entity *e = rd_entity_root();
        !rd_entity_is_nil(e);
        e = rd_entity_rec_depth_first_pre(e, &d_nil_entity).next)
    {
      if(e->flags & RD_EntityFlag_HasCtrlHandle &&
         ctrl_handle_match(e->ctrl_handle, handle))
      {
        result = e;
        break;
      }
    }
  }
  return result;
}

internal RD_Entity *
rd_entity_from_ctrl_id(CTRL_MachineID machine_id, U32 id)
{
  RD_Entity *result = &d_nil_entity;
  if(id != 0)
  {
    for(RD_Entity *e = rd_entity_root();
        !rd_entity_is_nil(e);
        e = rd_entity_rec_depth_first_pre(e, &d_nil_entity).next)
    {
      if(e->flags & RD_EntityFlag_HasCtrlHandle &&
         e->flags & RD_EntityFlag_HasCtrlID &&
         e->ctrl_handle.machine_id == machine_id &&
         e->ctrl_id == id)
      {
        result = e;
        break;
      }
    }
  }
  return result;
}

internal RD_Entity *
rd_entity_from_name_and_kind(String8 string, RD_EntityKind kind)
{
  RD_Entity *result = &d_nil_entity;
  RD_EntityList all_of_this_kind = rd_query_cached_entity_list_with_kind(kind);
  for(RD_EntityNode *n = all_of_this_kind.first; n != 0; n = n->next)
  {
    if(str8_match(n->entity->string, string, 0))
    {
      result = n->entity;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Frontend Entity Info Extraction

internal D_Target
rd_d_target_from_entity(RD_Entity *entity)
{
  RD_Entity *src_target_exe   = rd_entity_child_from_kind(entity, RD_EntityKind_Executable);
  RD_Entity *src_target_args  = rd_entity_child_from_kind(entity, RD_EntityKind_Arguments);
  RD_Entity *src_target_wdir  = rd_entity_child_from_kind(entity, RD_EntityKind_WorkingDirectory);
  RD_Entity *src_target_stdo  = rd_entity_child_from_kind(entity, RD_EntityKind_StdoutPath);
  RD_Entity *src_target_stde  = rd_entity_child_from_kind(entity, RD_EntityKind_StderrPath);
  RD_Entity *src_target_stdi  = rd_entity_child_from_kind(entity, RD_EntityKind_StdinPath);
  RD_Entity *src_target_entry = rd_entity_child_from_kind(entity, RD_EntityKind_EntryPoint);
  D_Target target = {0};
  target.exe                     = src_target_exe->string;
  target.args                    = src_target_args->string;
  target.working_directory       = src_target_wdir->string;
  target.custom_entry_point_name = src_target_entry->string;
  target.stdout_path             = src_target_stdo->string;
  target.stderr_path             = src_target_stde->string;
  target.stdin_path              = src_target_stdi->string;
  return target;
}

internal DR_FancyStringList
rd_title_fstrs_from_entity(Arena *arena, RD_Entity *entity, Vec4F32 secondary_color, F32 size)
{
  DR_FancyStringList result = {0};
  RD_Entity *exe  = rd_entity_child_from_kind(entity, RD_EntityKind_Executable);
  RD_Entity *args = rd_entity_child_from_kind(entity, RD_EntityKind_Arguments);
  RD_Entity *loc  = rd_entity_child_from_kind(entity, RD_EntityKind_Location);
  RD_Entity *cnd  = rd_entity_child_from_kind(entity, RD_EntityKind_Condition);
  RD_Entity *src  = rd_entity_child_from_kind(entity, RD_EntityKind_Source);
  RD_Entity *dst  = rd_entity_child_from_kind(entity, RD_EntityKind_Dest);
  RD_IconKind icon_kind = rd_entity_kind_icon_kind_table[entity->kind];
  Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Text);
  if(icon_kind != RD_IconKind_Null)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Icons), size, secondary_color, rd_icon_kind_text_table[icon_kind]);
  }
  dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, secondary_color, str8_lit(" "));
  if(entity->kind == RD_EntityKind_Target && entity->cfg_src == RD_CfgSrc_CommandLine)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Icons), size, rd_rgba_from_theme_color(RD_ThemeColor_TextNegative), rd_icon_kind_text_table[RD_IconKind_Info]);
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, secondary_color, str8_lit(" "));
  }
  String8 name = entity->string;
  B32 name_is_code = 1;
  if(rd_entity_kind_flags_table[entity->kind] & RD_EntityKindFlag_NameIsPath)
  {
    name_is_code = 0;
  }
  String8 location = {0};
  B32 location_is_code = 0;
  String8 exe_name = str8_skip_last_slash(exe->string);
  String8 args_string = args->string;
  String8 cnd_string = cnd->string;
  if(!rd_entity_is_nil(loc))
  {
    if(loc->string.size != 0 && loc->flags & RD_EntityFlag_HasTextPoint)
    {
      location = push_str8f(arena, "%S:%I64d:%I64d", loc->string, loc->text_point.line, loc->text_point.column);
      location_is_code = 0;
    }
    else if(loc->string.size != 0)
    {
      location = loc->string;
      location_is_code = 1;
    }
    else if(loc->flags & RD_EntityFlag_HasVAddr)
    {
      location = push_str8f(arena, "0x%I64x", loc->vaddr);
      location_is_code = 1;
    }
  }
  B32 extra = 0;
  F32 size_extrafied = size;
  Vec4F32 color_extrafied = color;
  if(name.size != 0)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(name_is_code ? RD_FontSlot_Code : RD_FontSlot_Main), size, color, name);
    extra = 1;
    size_extrafied = size*0.95f;
    color_extrafied = secondary_color;
  }
  if(location.size != 0)
  {
    if(extra)
    {
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    }
    if(location_is_code)
    {
      DR_FancyStringList loc_fstrs = {0};
      RD_Font(RD_FontSlot_Code)
        loc_fstrs = rd_fancy_string_list_from_code_string(arena, 1.f, 0, color_extrafied, location);
      dr_fancy_string_list_concat_in_place(&result, &loc_fstrs);
    }
    else
    {
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size_extrafied, color_extrafied, location);
    }
    extra = 1;
    size_extrafied = size*0.95f;
    color_extrafied = secondary_color;
  }
  if(exe_name.size != 0)
  {
    if(extra)
    {
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    }
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size_extrafied, color_extrafied, exe_name);
    extra = 1;
    size_extrafied = size*0.95f;
    color_extrafied = secondary_color;
  }
  if(args_string.size != 0)
  {
    if(extra)
    {
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    }
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size_extrafied, color_extrafied, args_string);
    extra = 1;
    size_extrafied = size*0.95f;
    color_extrafied = secondary_color;
  }
  if(cnd_string.size != 0)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, color_extrafied, str8_lit(" if "));
    RD_Font(RD_FontSlot_Code) UI_FontSize(size_extrafied)
    {
      DR_FancyStringList cnd_fstrs = rd_fancy_string_list_from_code_string(arena, 1.f, 0.f, color_extrafied, cnd_string);
      dr_fancy_string_list_concat_in_place(&result, &cnd_fstrs);
    }
  }
  if(entity->kind == RD_EntityKind_FilePathMap)
  {
    String8 src_string = src->string;
    Vec4F32 src_color = color;
    String8 dst_string = dst->string;
    Vec4F32 dst_color = color;
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
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size, src_color, src_string);
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Icons), size, secondary_color, rd_icon_kind_text_table[RD_IconKind_RightArrow]);
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size, dst_color, dst_string);
  }
  if(entity->kind == RD_EntityKind_AutoViewRule)
  {
    String8 src_string = src->string;
    Vec4F32 src_color = color;
    String8 dst_string = dst->string;
    Vec4F32 dst_color = color;
    DR_FancyStringList src_fstrs = {0};
    DR_FancyStringList dst_fstrs = {0};
    if(src_string.size == 0)
    {
      src_string = str8_lit("(type)");
      src_color = secondary_color;
      dr_fancy_string_list_push_new(arena, &src_fstrs, rd_font_from_slot(RD_FontSlot_Main), size, src_color, src_string);
    }
    else RD_Font(RD_FontSlot_Code)
    {
      src_fstrs = rd_fancy_string_list_from_code_string(arena, 1.f, 0, src_color, src_string);
    }
    if(dst_string.size == 0)
    {
      dst_string = str8_lit("(view rule)");
      dst_color = secondary_color;
      dr_fancy_string_list_push_new(arena, &dst_fstrs, rd_font_from_slot(RD_FontSlot_Main), size, dst_color, dst_string);
    }
    else RD_Font(RD_FontSlot_Code)
    {
      dst_fstrs = rd_fancy_string_list_from_code_string(arena, 1.f, 0, dst_color, dst_string);
    }
    dr_fancy_string_list_concat_in_place(&result, &src_fstrs);
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Icons), size, secondary_color, rd_icon_kind_text_table[RD_IconKind_RightArrow]);
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    dr_fancy_string_list_concat_in_place(&result, &dst_fstrs);
  }
  if((entity->kind == RD_EntityKind_Target || entity->kind == RD_EntityKind_Breakpoint) && entity->disabled)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size*0.95f, secondary_color, str8_lit("(Disabled)"));
  }
  if(entity->kind == RD_EntityKind_Breakpoint)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, v4f32(0, 0, 0, 0), str8_lit(" "));
    String8 string = push_str8f(arena, "(%I64u hit%s)", entity->u64, entity->u64 == 1 ? "" : "s");
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size_extrafied, secondary_color, string);
  }
  return result;
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

internal DR_FancyStringList
rd_title_fstrs_from_ctrl_entity(Arena *arena, CTRL_Entity *entity, Vec4F32 secondary_color, F32 size, B32 include_extras)
{
  DR_FancyStringList result = {0};
  
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
  
  //- rjf: push icon
  if(icon_kind != RD_IconKind_Null)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Icons), size, secondary_color, rd_icon_kind_text_table[icon_kind]);
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, secondary_color, str8_lit(" "));
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
        dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), size, process_color, process_name);
        dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, secondary_color, str8_lit(" / "));
      }
    }
  }
  
  //- rjf: push name
  dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(name_is_code ? RD_FontSlot_Code : RD_FontSlot_Main), size, color, name);
  
  //- rjf: threads get callstack extras
  if(entity->kind == CTRL_EntityKind_Thread && include_extras)
  {
    dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, secondary_color, str8_lit(" "));
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
          dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), extras_size, rd_rgba_from_theme_color(RD_ThemeColor_CodeSymbol), name);
          if(idx+1 < unwind.frames.count)
          {
            dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), extras_size, secondary_color, str8_lit(" > "));
            if(idx+1 == limit)
            {
              dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), extras_size, secondary_color, str8_lit("..."));
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
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Code), size, secondary_color, str8_lit(" "));
      dr_fancy_string_list_push_new(arena, &result, rd_font_from_slot(RD_FontSlot_Main), extras_size, secondary_color, str8_lit("(Symbols not found)"));
    }
    di_scope_close(di_scope);
  }
  
  return result;
}

////////////////////////////////
//~ rjf: Evaluation Spaces

//- rjf: entity <-> eval space

internal RD_Entity *
rd_entity_from_eval_space(E_Space space)
{
  RD_Entity *entity = &d_nil_entity;
  if(space.kind == RD_EvalSpaceKind_MetaEntity)
  {
    RD_Handle handle = {space.u64s[0], space.u64s[1]};
    entity = rd_entity_from_handle(handle);
  }
  return entity;
}

internal E_Space
rd_eval_space_from_entity(RD_Entity *entity)
{
  E_Space space = e_space_make(RD_EvalSpaceKind_MetaEntity);
  RD_Handle handle = rd_handle_from_entity(entity);
  space.u64s[0] = handle.u64[0];
  space.u64s[1] = handle.u64[1];
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

//- rjf: entity -> meta eval

internal CTRL_MetaEval *
rd_ctrl_meta_eval_from_entity(Arena *arena, RD_Entity *entity)
{
  ProfBeginFunction();
  CTRL_MetaEval *meval = push_array(arena, CTRL_MetaEval, 1);
  RD_Entity *exe = rd_entity_child_from_kind(entity, RD_EntityKind_Executable);
  RD_Entity *args= rd_entity_child_from_kind(entity, RD_EntityKind_Arguments);
  RD_Entity *wdir= rd_entity_child_from_kind(entity, RD_EntityKind_WorkingDirectory);
  RD_Entity *entr= rd_entity_child_from_kind(entity, RD_EntityKind_EntryPoint);
  RD_Entity *stdo= rd_entity_child_from_kind(entity, RD_EntityKind_StdoutPath);
  RD_Entity *stde= rd_entity_child_from_kind(entity, RD_EntityKind_StderrPath);
  RD_Entity *stdi= rd_entity_child_from_kind(entity, RD_EntityKind_StdinPath);
  RD_Entity *loc = rd_entity_child_from_kind(entity, RD_EntityKind_Location);
  RD_Entity *cnd = rd_entity_child_from_kind(entity, RD_EntityKind_Condition);
  RD_Entity *src = rd_entity_child_from_kind(entity, RD_EntityKind_Source);
  RD_Entity *dst = rd_entity_child_from_kind(entity, RD_EntityKind_Dest);
  String8 label_string = push_str8_copy(arena, entity->string);
  String8 src_loc_string = {0};
  String8 vaddr_loc_string = {0};
  String8 function_loc_string = {0};
  if(loc->flags & RD_EntityFlag_HasTextPoint)
  {
    src_loc_string = push_str8f(arena, "%S:%I64u:%I64u", loc->string, loc->text_point.line, loc->text_point.column);
  }
  else if(loc->flags & RD_EntityFlag_HasVAddr)
  {
    vaddr_loc_string = push_str8f(arena, "0x%I64x", loc->vaddr);
  }
  else if(loc->string.size != 0)
  {
    function_loc_string = push_str8_copy(arena, loc->string);
  }
  String8 cnd_string = push_str8_copy(arena, cnd->string);
  meval->enabled   = !entity->disabled;
  meval->hit_count = entity->u64;
  meval->color     = u32_from_rgba(rd_rgba_from_entity(entity));
  meval->label     = label_string;
  meval->exe       = exe->string;
  meval->args      = args->string;
  meval->working_directory = wdir->string;
  meval->entry_point = entr->string;
  meval->stdout_path = stdo->string;
  meval->stderr_path = stde->string;
  meval->stdin_path  = stdi->string;
  meval->source_location = src_loc_string;
  meval->address_location = vaddr_loc_string;
  meval->function_location = function_loc_string;
  meval->condition = cnd_string;
  switch(entity->kind)
  {
    default:{}break;
    case RD_EntityKind_FilePathMap:
    {
      meval->source_path = src->string;
      meval->destination_path = dst->string;
    }break;
    case RD_EntityKind_AutoViewRule:
    {
      meval->type      = src->string;
      meval->view_rule = dst->string;
    }break;
  }
  ProfEnd();
  return meval;
}

internal CTRL_MetaEval *
rd_ctrl_meta_eval_from_ctrl_entity(Arena *arena, CTRL_Entity *entity)
{
  ProfBeginFunction();
  CTRL_MetaEval *meval = push_array(arena, CTRL_MetaEval, 1);
  meval->frozen      = entity->is_frozen;
  meval->vaddr_range = entity->vaddr_range;
  meval->color       = entity->rgba;
  meval->label       = entity->string;
  meval->id          = entity->id;
  if(entity->kind == CTRL_EntityKind_Thread)
  {
    DI_Scope *di_scope = di_scope_open();
    CTRL_Entity *process = ctrl_entity_ancestor_from_kind(entity, CTRL_EntityKind_Process);
    CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(entity);
    CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(arena, di_scope, process, &base_unwind);
    meval->callstack.count = rich_unwind.total_frame_count;
    meval->callstack.v = push_array(arena, CTRL_MetaEvalFrame, meval->callstack.count);
    U64 idx = 0;
    for(U64 base_idx = 0; base_idx < rich_unwind.concrete_frame_count; base_idx += 1)
    {
      U64 inline_idx = 0;
      for(CTRL_CallStackInlineFrame *f = rich_unwind.frames[base_idx].first_inline_frame; f != 0; f = f->next, inline_idx += 1)
      {
        meval->callstack.v[idx].vaddr = regs_rip_from_arch_block(entity->arch, rich_unwind.frames[base_idx].regs);
        meval->callstack.v[idx].inline_depth = inline_idx + 1;
        idx += 1;
      }
      meval->callstack.v[idx].vaddr = regs_rip_from_arch_block(entity->arch, rich_unwind.frames[base_idx].regs);
      idx += 1;
    }
    di_scope_close(di_scope);
  }
  if(entity->kind == CTRL_EntityKind_Module)
  {
    DI_Key dbgi_key = ctrl_dbgi_key_from_module(entity);
    meval->label = str8_skip_last_slash(entity->string);
    meval->exe = path_normalized_from_string(arena, entity->string);
    meval->dbg = path_normalized_from_string(arena, dbgi_key.path);
  }
  ProfEnd();
  return meval;
}

//- rjf: eval space reads/writes

internal B32
rd_eval_space_read(void *u, E_Space space, void *out, Rng1U64 range)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  CTRL_MetaEval *meval_read = 0;
  Rng1U64 meval_legal_range = {0};
  switch(space.kind)
  {
    //- rjf: filesystem reads
    case E_SpaceKind_FileSystem:
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
          Temp scratch = scratch_begin(0, 0);
          CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, entity->handle, range, d_state->frame_eval_memread_endt_us);
          String8 data = slice.data;
          if(data.size == dim_1u64(range))
          {
            result = 1;
            MemoryCopy(out, data.str, data.size);
          }
          scratch_end(scratch);
        }break;
        case CTRL_EntityKind_Thread:
        {
          Temp scratch = scratch_begin(0, 0);
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
          scratch_end(scratch);
        }break;
      }
    }break;
    
    //- rjf: meta reads (metadata about either control entities or debugger state)
    case RD_EvalSpaceKind_MetaCtrlEntity:
    {
      CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(space);
      U64 hash = ctrl_hash_from_handle(entity->handle);
      U64 slot_idx = hash%rd_state->ctrl_entity_meval_cache_slots_count;
      RD_CtrlEntityMetaEvalCacheSlot *slot = &rd_state->ctrl_entity_meval_cache_slots[slot_idx];
      RD_CtrlEntityMetaEvalCacheNode *node = 0;
      for(RD_CtrlEntityMetaEvalCacheNode *n = slot->first; n != 0; n = n->next)
      {
        if(ctrl_handle_match(n->handle, entity->handle))
        {
          node = n;
          break;
        }
      }
      if(!node)
      {
        CTRL_MetaEval *meval = rd_ctrl_meta_eval_from_ctrl_entity(scratch.arena, entity);
        String8 meval_srlzed = serialized_from_struct(scratch.arena, CTRL_MetaEval, meval);
        U64 pos_min = arena_pos(rd_frame_arena());
        arena_push(rd_frame_arena(), 0, 64);
        CTRL_MetaEval *meval_read = struct_from_serialized(rd_frame_arena(), CTRL_MetaEval, meval_srlzed);
        struct_rebase_ptrs(CTRL_MetaEval, meval_read, meval_read);
        U64 pos_opl = arena_pos(scratch.arena);
        node = push_array(rd_frame_arena(), RD_CtrlEntityMetaEvalCacheNode, 1);
        SLLQueuePush(slot->first, slot->last, node);
        node->handle = entity->handle;
        node->meval  = meval_read;
        node->range  = r1u64(0, pos_opl-pos_min);
      }
      meval_read = node->meval;
      meval_legal_range = node->range;
    }goto meta_eval;
    case RD_EvalSpaceKind_MetaEntity:
    {
      // rjf: calculate meta evaluation
      CTRL_MetaEval *meval = rd_ctrl_meta_eval_from_entity(scratch.arena, rd_entity_from_eval_space(space));
      
      // rjf: copy meta evaluation to scratch arena, to form range of legal reads
      arena_push(scratch.arena, 0, 64);
      String8 meval_srlzed = serialized_from_struct(scratch.arena, CTRL_MetaEval, meval);
      U64 pos_min = arena_pos(scratch.arena);
      meval_read = struct_from_serialized(scratch.arena, CTRL_MetaEval, meval_srlzed);
      U64 pos_opl = arena_pos(scratch.arena);
      
      // rjf: rebase all pointer values in meta evaluation to be relative to base pointer
      struct_rebase_ptrs(CTRL_MetaEval, meval_read, meval_read);
      
      // rjf: form legal range
      meval_legal_range = r1u64(0, pos_opl-pos_min);
    }goto meta_eval;
    meta_eval:;
    {
      if(contains_1u64(meval_legal_range, range.min))
      {
        result = 1;
        U64 range_dim = dim_1u64(range);
        U64 bytes_to_read = Min(range_dim, (meval_legal_range.max - range.min));
        MemoryCopy(out, ((U8 *)meval_read) + range.min, bytes_to_read);
        if(bytes_to_read < range_dim)
        {
          MemoryZero((U8 *)out + bytes_to_read, range_dim - bytes_to_read);
        }
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
    
    //- rjf: meta-entity writes
    case RD_EvalSpaceKind_MetaEntity:
    {
      Temp scratch = scratch_begin(0, 0);
      
      // rjf: get entity, produce meta-eval
      RD_Entity *entity = rd_entity_from_eval_space(space);
      CTRL_MetaEval *meval = rd_ctrl_meta_eval_from_entity(scratch.arena, entity);
      
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
      FlatMemberCase(enabled)             {result = 1; rd_entity_equip_disabled(entity, !!((U8 *)in)[0]);}
      StringMemberCase(label)             {result = 1; rd_entity_equip_name(entity, str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(exe)               {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Executable), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(args)              {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Arguments), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(working_directory) {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_WorkingDirectory), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(entry_point)       {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_EntryPoint), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(stdout_path)       {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_StdoutPath), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(stderr_path)       {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_StderrPath), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(stdin_path)        {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_StdinPath), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(source_path)       {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Source), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(destination_path)  {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Dest), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(type)              {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Source), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(view_rule)         {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Dest), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(condition)         {result = 1; rd_entity_equip_name(rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Condition), str8_cstring_capped(in, (U8 *)in + 4096));}
      StringMemberCase(source_location)
      {
        result = 1;
        String8TxtPtPair src_loc = str8_txt_pt_pair_from_string(str8_cstring_capped(in, (U8 *)in + 4096));
        RD_Entity *loc = rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Location);
        rd_entity_equip_name(loc, src_loc.string);
        rd_entity_equip_txt_pt(loc, src_loc.pt);
      }
      StringMemberCase(address_location)
      {
        U64 vaddr = 0;
        if(try_u64_from_str8_c_rules(str8_cstring_capped(in, (U8 *)in + 4096), &vaddr))
        {
          RD_Entity *loc = rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Location);
          rd_entity_equip_vaddr(loc, vaddr);
          rd_entity_equip_name(loc, str8_zero());
          loc->flags &= ~RD_EntityFlag_HasTextPoint;
          result = 1;
        }
      }
      StringMemberCase(function_location)
      {
        result = 1;
        RD_Entity *loc = rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Location);
        loc->flags &= ~RD_EntityFlag_HasTextPoint;
        rd_entity_equip_name(loc, str8_cstring_capped(in, (U8 *)in + 4096));
      }
#undef FlatMemberCase
#undef StringMemberCase
      scratch_end(scratch);
    }break;
    case RD_EvalSpaceKind_MetaCtrlEntity:
    {
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
    case E_SpaceKind_FileSystem:
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
    case E_SpaceKind_FileSystem:
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
      E_Eval src_eval = e_eval_from_string(scratch.arena, string);
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

internal U64
rd_base_offset_from_eval(E_Eval eval)
{
  if(e_type_kind_is_pointer_or_ref(e_type_kind_from_key(eval.type_key)))
  {
    eval = e_value_eval_from_eval(eval);
  }
  return eval.value.u64;
}

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
    E_Eval eval = e_eval_from_string(scratch.arena, string);
    if(eval.expr->kind == E_ExprKind_LeafFilePath)
    {
      result = raw_from_escaped_str8(arena, eval.expr->string);
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

////////////////////////////////
//~ rjf: View State Functions

//- rjf: allocation/releasing

internal RD_View *
rd_view_alloc(void)
{
  // rjf: allocate
  RD_View *view = rd_state->free_view;
  {
    if(!rd_view_is_nil(view))
    {
      rd_state->free_view_count -= 1;
      SLLStackPop_N(rd_state->free_view, alloc_next);
      U64 generation = view->generation;
      MemoryZeroStruct(view);
      view->generation = generation;
    }
    else
    {
      view = push_array(rd_state->arena, RD_View, 1);
    }
    view->generation += 1;
  }
  
  // rjf: initialize
  view->arena = arena_alloc();
  view->spec = &rd_nil_view_rule_info;
  view->project_path_arena = arena_alloc();
  view->project_path = str8_zero();
  for(U64 idx = 0; idx < ArrayCount(view->params_arenas); idx += 1)
  {
    view->params_arenas[idx] = arena_alloc();
    view->params_roots[idx] = &md_nil_node;
  }
  view->query_cursor = view->query_mark = txt_pt(1, 1);
  view->query_string_size = 0;
  rd_state->allocated_view_count += 1;
  DLLPushBack_NPZ(&rd_nil_view, rd_state->first_view, rd_state->last_view, view, alloc_next, alloc_prev);
  return view;
}

internal void
rd_view_release(RD_View *view)
{
  DLLRemove_NPZ(&rd_nil_view, rd_state->first_view, rd_state->last_view, view, alloc_next, alloc_prev);
  SLLStackPush_N(rd_state->free_view, view, alloc_next);
  for(RD_View *tchild = view->first_transient, *next = 0; !rd_view_is_nil(tchild); tchild = next)
  {
    next = tchild->order_next;
    rd_view_release(tchild);
  }
  view->first_transient = view->last_transient = &rd_nil_view;
  view->transient_view_slots_count = 0;
  view->transient_view_slots = 0;
  for(RD_ArenaExt *ext = view->first_arena_ext; ext != 0; ext = ext->next)
  {
    arena_release(ext->arena);
  }
  view->first_arena_ext = view->last_arena_ext = 0;
  arena_release(view->project_path_arena);
  for(U64 idx = 0; idx < ArrayCount(view->params_arenas); idx += 1)
  {
    arena_release(view->params_arenas[idx]);
  }
  arena_release(view->arena);
  view->generation += 1;
  rd_state->allocated_view_count -= 1;
  rd_state->free_view_count += 1;
}

//- rjf: equipment

internal void
rd_view_equip_spec(RD_View *view, RD_ViewRuleInfo *spec, String8 query, MD_Node *params)
{
  // rjf: fill params tree
  for(U64 idx = 0; idx < ArrayCount(view->params_arenas); idx += 1)
  {
    arena_clear(view->params_arenas[idx]);
  }
  view->params_roots[0] = md_tree_copy(view->params_arenas[0], params);
  view->params_write_gen = view->params_read_gen = 0;
  
  // rjf: fill query buffer
  rd_view_equip_query(view, query);
  
  // rjf: initialize state for new view spec
  {
    for(RD_ArenaExt *ext = view->first_arena_ext; ext != 0; ext = ext->next)
    {
      arena_release(ext->arena);
    }
    for(RD_View *tchild = view->first_transient, *next = 0; !rd_view_is_nil(tchild); tchild = next)
    {
      next = tchild->order_next;
      rd_view_release(tchild);
    }
    view->first_transient = view->last_transient = &rd_nil_view;
    view->first_arena_ext = view->last_arena_ext = 0;
    view->transient_view_slots_count = 0;
    view->transient_view_slots = 0;
    arena_clear(view->arena);
    view->user_data = 0;
  }
  MemoryZeroStruct(&view->scroll_pos);
  view->spec = spec;
  arena_clear(view->project_path_arena);
  view->project_path = push_str8_copy(view->project_path_arena, rd_cfg_path_from_src(RD_CfgSrc_Project));
  view->is_filtering = 0;
  view->is_filtering_t = 0;
}

internal void
rd_view_equip_query(RD_View *view, String8 query)
{
  view->query_string_size = Min(sizeof(view->query_buffer), query.size);
  MemoryCopy(view->query_buffer, query.str, view->query_string_size);
  view->query_cursor = view->query_mark = txt_pt(1, query.size+1);
}

internal void
rd_view_equip_loading_info(RD_View *view, B32 is_loading, U64 progress_v, U64 progress_target)
{
  view->loading_t_target = (F32)!!is_loading;
  view->loading_progress_v = progress_v;
  view->loading_progress_v_target = progress_target;
  if(is_loading)
  {
    view->loading_t = view->loading_t_target;
  }
}

//- rjf: user state extensions

internal void *
rd_view_get_or_push_user_state(RD_View *view, U64 size)
{
  void *result = view->user_data;
  if(result == 0)
  {
    view->user_data = result = push_array(view->arena, U8, size);
  }
  return result;
}

internal Arena *
rd_view_push_arena_ext(RD_View *view)
{
  RD_ArenaExt *ext = push_array(view->arena, RD_ArenaExt, 1);
  ext->arena = arena_alloc();
  SLLQueuePush(view->first_arena_ext, view->last_arena_ext, ext);
  return ext->arena;
}

//- rjf: param saving

internal void
rd_view_store_param(RD_View *view, String8 key, String8 value)
{
  B32 new_copy = 0;
  if(view->params_write_gen == view->params_read_gen)
  {
    view->params_write_gen += 1;
    new_copy = 1;
  }
  Arena *new_params_arena = view->params_arenas[view->params_write_gen%ArrayCount(view->params_arenas)];
  if(new_copy)
  {
    arena_clear(new_params_arena);
    view->params_roots[view->params_write_gen%ArrayCount(view->params_arenas)] = md_tree_copy(new_params_arena, view->params_roots[view->params_read_gen%ArrayCount(view->params_arenas)]);
  }
  MD_Node *new_params_root = view->params_roots[view->params_write_gen%ArrayCount(view->params_arenas)];
  if(md_node_is_nil(new_params_root))
  {
    new_params_root = view->params_roots[view->params_write_gen%ArrayCount(view->params_arenas)] = md_push_node(new_params_arena, MD_NodeKind_Main, 0, str8_zero(), str8_zero(), 0);
  }
  MD_Node *key_node = md_child_from_string(new_params_root, key, 0);
  if(md_node_is_nil(key_node))
  {
    String8 key_copy = push_str8_copy(new_params_arena, key);
    key_node = md_push_node(new_params_arena, MD_NodeKind_Main, MD_NodeFlag_Identifier, key_copy, key_copy, 0);
    md_node_push_child(new_params_root, key_node);
  }
  key_node->first = key_node->last = &md_nil_node;
  String8 value_copy = push_str8_copy(new_params_arena, value);
  MD_TokenizeResult value_tokenize = md_tokenize_from_text(new_params_arena, value_copy);
  MD_ParseResult value_parse = md_parse_from_text_tokens(new_params_arena, str8_zero(), value_copy, value_tokenize.tokens);
  for MD_EachNode(child, value_parse.root->first)
  {
    child->parent = key_node;
  }
  key_node->first = value_parse.root->first;
  key_node->last = value_parse.root->last;
}

internal void
rd_view_store_paramf(RD_View *view, String8 key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  rd_view_store_param(view, key, string);
  va_end(args);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: View Building API

//- rjf: view info extraction

internal Arena *
rd_view_arena(void)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  return view->arena;
}

internal UI_ScrollPt2
rd_view_scroll_pos(void)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  return view->scroll_pos;
}

internal String8
rd_view_expr_string(void)
{
  // TODO(rjf): @entity_simplification filter and expr string need to be different
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  String8 expr_string = str8(view->query_buffer, view->query_string_size);
  return expr_string;
}

internal String8
rd_view_filter(void)
{
  // TODO(rjf): @entity_simplification filter and expr string need to be different
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  String8 filter = str8(view->query_buffer, view->query_string_size);
  return filter;
}

//- rjf: pushing/attaching view resources

internal void *
rd_view_state_by_size(U64 size)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  void *result = rd_view_get_or_push_user_state(view, size);
  return result;
}

internal Arena *
rd_push_view_arena(void)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  Arena *result = rd_view_push_arena_ext(view);
  return result;
}

//- rjf: storing view-attached state

internal void
rd_store_view_expr_string(String8 string)
{
  // TODO(rjf): @entity_simplification filter and expr string need to be different
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  rd_view_equip_query(view, string);
}

internal void
rd_store_view_filter(String8 string)
{
  // TODO(rjf): @entity_simplification filter and expr string need to be different
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  rd_view_equip_query(view, string);
}

internal void
rd_store_view_loading_info(B32 is_loading, U64 progress_u64, U64 progress_u64_target)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  rd_view_equip_loading_info(view, is_loading, progress_u64, progress_u64_target);
}

internal void
rd_store_view_scroll_pos(UI_ScrollPt2 pos)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  view->scroll_pos = pos;
}

internal void
rd_store_view_param(String8 key, String8 value)
{
  RD_View *view = rd_view_from_handle(rd_regs()->view);
  rd_view_store_param(view, key, value);
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
//~ rjf: Expand-Keyed Transient View Functions

internal RD_TransientViewNode *
rd_transient_view_node_from_ev_key(RD_View *owner_view, EV_Key key)
{
  if(owner_view->transient_view_slots_count == 0)
  {
    owner_view->transient_view_slots_count = 256;
    owner_view->transient_view_slots = push_array(owner_view->arena, RD_TransientViewSlot, owner_view->transient_view_slots_count);
  }
  U64 hash = ev_hash_from_key(key);
  U64 slot_idx = hash%owner_view->transient_view_slots_count;
  RD_TransientViewSlot *slot = &owner_view->transient_view_slots[slot_idx];
  RD_TransientViewNode *node = 0;
  for(RD_TransientViewNode *n = slot->first; n != 0; n = n->next)
  {
    if(ev_key_match(n->key, key))
    {
      node = n;
      n->last_frame_index_touched = rd_state->frame_index;
      break;
    }
  }
  if(node == 0)
  {
    if(!owner_view->free_transient_view_node)
    {
      owner_view->free_transient_view_node = push_array(rd_state->arena, RD_TransientViewNode, 1);
    }
    node = owner_view->free_transient_view_node;
    SLLStackPop(owner_view->free_transient_view_node);
    DLLPushBack(slot->first, slot->last, node);
    node->key = key;
    node->view = rd_view_alloc();
    node->initial_params_arena = arena_alloc();
    node->first_frame_index_touched = node->last_frame_index_touched = rd_state->frame_index;
    DLLPushBack_NPZ(&rd_nil_view, owner_view->first_transient, owner_view->last_transient, node->view, order_next, order_prev);
  }
  return node;
}

////////////////////////////////
//~ rjf: Panel State Functions

internal RD_Panel *
rd_panel_alloc(RD_Window *ws)
{
  RD_Panel *panel = ws->free_panel;
  if(!rd_panel_is_nil(panel))
  {
    SLLStackPop(ws->free_panel);
    U64 generation = panel->generation;
    MemoryZeroStruct(panel);
    panel->generation = generation;
  }
  else
  {
    panel = push_array(ws->arena, RD_Panel, 1);
  }
  panel->first = panel->last = panel->next = panel->prev = panel->parent = &rd_nil_panel;
  panel->first_tab_view = panel->last_tab_view = &rd_nil_view;
  panel->generation += 1;
  MemoryZeroStruct(&panel->animated_rect_pct);
  return panel;
}

internal void
rd_panel_release(RD_Window *ws, RD_Panel *panel)
{
  rd_panel_release_all_views(panel);
  SLLStackPush(ws->free_panel, panel);
  panel->generation += 1;
}

internal void
rd_panel_release_all_views(RD_Panel *panel)
{
  for(RD_View *view = panel->first_tab_view, *next = 0; !rd_view_is_nil(view); view = next)
  {
    next = view->order_next;
    rd_view_release(view);
  }
  panel->first_tab_view = panel->last_tab_view = &rd_nil_view;
  panel->selected_tab_view = rd_handle_zero();
  panel->tab_view_count = 0;
}

////////////////////////////////
//~ rjf: Window State Functions

internal RD_Window *
rd_window_open(Vec2F32 size, OS_Handle preferred_monitor, RD_CfgSrc cfg_src)
{
  RD_Window *window = rd_state->free_window;
  if(window != 0)
  {
    SLLStackPop(rd_state->free_window);
    U64 gen = window->gen;
    MemoryZeroStruct(window);
    window->gen = gen;
  }
  else
  {
    window = push_array(rd_state->arena, RD_Window, 1);
  }
  window->gen += 1;
  window->frames_alive = 0;
  window->cfg_src = cfg_src;
  window->arena = arena_alloc();
  {
    String8 title = str8_lit_comp(BUILD_TITLE_STRING_LITERAL);
    window->os = os_window_open(size, OS_WindowFlag_CustomBorder, title);
  }
  window->r = r_window_equip(window->os);
  window->ui = ui_state_alloc();
  window->ctx_menu_arena = arena_alloc();
  window->ctx_menu_regs = push_array(window->ctx_menu_arena, RD_Regs, 1);
  window->ctx_menu_input_buffer_size = KB(4);
  window->ctx_menu_input_buffer = push_array(window->arena, U8, window->ctx_menu_input_buffer_size);
  window->drop_completion_arena = arena_alloc();
  window->hover_eval_arena = arena_alloc();
  window->autocomp_lister_params_arena = arena_alloc();
  window->free_panel = &rd_nil_panel;
  window->root_panel = rd_panel_alloc(window);
  window->focused_panel = window->root_panel;
  window->query_cmd_arena = arena_alloc();
  window->query_view_stack_top = &rd_nil_view;
  window->last_dpi = os_dpi_from_window(window->os);
  for EachEnumVal(RD_SettingCode, code)
  {
    if(rd_setting_code_default_is_per_window_table[code])
    {
      window->setting_vals[code] = rd_setting_code_default_val_table[code];
    }
  }
  OS_Handle zero_monitor = {0};
  if(!os_handle_match(zero_monitor, preferred_monitor))
  {
    os_window_set_monitor(window->os, preferred_monitor);
  }
  if(rd_state->first_window == 0) RD_RegsScope(.window = rd_handle_from_window(window))
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
  DLLPushBack(rd_state->first_window, rd_state->last_window, window);
  return window;
}

internal RD_Window *
rd_window_from_os_handle(OS_Handle os)
{
  RD_Window *result = 0;
  for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
  {
    if(os_handle_match(w->os, os))
    {
      result = w;
      break;
    }
  }
  return result;
}

#if COMPILER_MSVC && !BUILD_DEBUG
#pragma optimize("", off)
#endif

internal void
rd_window_frame(RD_Window *ws)
{
  ProfBeginFunction();
  
  //////////////////////////////
  //- rjf: unpack context
  //
  B32 window_is_focused = os_window_is_focused(ws->os) || ws->window_temporarily_focused_ipc;
  B32 popup_open = rd_state->popup_active;
  B32 query_is_open = !rd_view_is_nil(ws->query_view_stack_top);
  B32 hover_eval_is_open = (!popup_open &&
                            ws->hover_eval_string.size != 0 &&
                            ws->hover_eval_first_frame_idx+20 < ws->hover_eval_last_frame_idx &&
                            rd_state->frame_index-ws->hover_eval_last_frame_idx < 20);
  if(!window_is_focused || popup_open)
  {
    ws->menu_bar_key_held = 0;
  }
  ws->window_temporarily_focused_ipc = 0;
  ui_select_state(ws->ui);
  
  //////////////////////////////
  //- rjf: panels with no selected tabs? -> select.
  // panels with selected tabs? -> ensure they have active tabs.
  //
  for(RD_Panel *panel = ws->root_panel;
      !rd_panel_is_nil(panel);
      panel = rd_panel_rec_depth_first_pre(panel).next)
  {
    if(!rd_panel_is_nil(panel->first))
    {
      continue;
    }
    RD_View *view = rd_selected_tab_from_panel(panel);
    if(rd_view_is_nil(view))
    {
      for(RD_View *tab = panel->first_tab_view; !rd_view_is_nil(tab); tab = tab->order_next)
      {
        if(!rd_view_is_project_filtered(tab))
        {
          panel->selected_tab_view = rd_handle_from_view(tab);
          break;
        }
      }
    }
    if(!rd_view_is_nil(view))
    {
      B32 found = 0;
      for(RD_View *tab = panel->first_tab_view; !rd_view_is_nil(tab); tab = tab->order_next)
      {
        if(rd_view_is_project_filtered(tab)) {continue;}
        if(tab == view)
        {
          found = 1;
        }
      }
      if(!found)
      {
        panel->selected_tab_view = rd_handle_zero();
      }
    }
  }
  
  //////////////////////////////
  //- rjf: fill panel/view interaction registers
  //
  rd_regs()->panel  = rd_handle_from_panel(ws->focused_panel);
  rd_regs()->view   = ws->focused_panel->selected_tab_view;
  
  //////////////////////////////
  //- rjf: compute ui palettes from theme
  //
  {
    RD_Theme *current = &rd_state->cfg_theme;
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
    ws->cfg_palettes[RD_PaletteCode_DropSiteOverlay].border     = current->colors[RD_ThemeColor_DropSiteOverlay];
    if(rd_setting_val_from_code(RD_SettingCode_OpaqueBackgrounds).s32)
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
  //- rjf: build UI
  //
  UI_Box *autocomp_box = &ui_nil_box;
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
        if(rd_setting_val_from_code(RD_SettingCode_HoverAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_HotAnimations;}
        if(rd_setting_val_from_code(RD_SettingCode_PressAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_ActiveAnimations;}
        if(rd_setting_val_from_code(RD_SettingCode_FocusAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_FocusAnimations;}
        if(rd_setting_val_from_code(RD_SettingCode_TooltipAnimations).s32)     {animation_info.flags |= UI_AnimationInfoFlag_TooltipAnimations;}
        if(rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32)        {animation_info.flags |= UI_AnimationInfoFlag_ContextMenuAnimations;}
        if(rd_setting_val_from_code(RD_SettingCode_ScrollingAnimations).s32)   {animation_info.flags |= UI_AnimationInfoFlag_ScrollingAnimations;}
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
      if(rd_setting_val_from_code(RD_SettingCode_SmoothUIText).s32) {text_raster_flags |= FNT_RasterFlag_Smooth;}
      if(rd_setting_val_from_code(RD_SettingCode_HintUIText).s32) {text_raster_flags |= FNT_RasterFlag_Hinted;}
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
      String8 string = ui_string_hover_string(scratch.arena);
      DR_FancyRunList runs = ui_string_hover_runs(scratch.arena);
      UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
      ui_box_equip_display_string_fancy_runs(box, string, &runs);
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
        //- rjf: frontend entity tooltips
        //
        case RD_RegSlot_Entity:
        UI_Tooltip
        {
          // rjf: unpack
          RD_Entity *entity = rd_entity_from_handle(regs->entity);
          DR_FancyStringList fstrs = rd_title_fstrs_from_entity(scratch.arena, entity, rd_rgba_from_theme_color(RD_ThemeColor_TextWeak), ui_top_font_size());
          
          // rjf: title
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(5, 1))
          {
            UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
            ui_box_equip_display_fancy_strings(box, &fstrs);
          }
          
          // rjf: temporary target -> display
          if(entity->kind == RD_EntityKind_Target && entity->cfg_src == RD_CfgSrc_CommandLine)
          {
            UI_Flags(UI_BoxFlag_DrawTextWeak) ui_label(str8_lit("Specified on the command line; will not be saved."));
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
          DR_FancyStringList fstrs = rd_title_fstrs_from_ctrl_entity(scratch.arena, ctrl_entity,
                                                                     rd_rgba_from_theme_color(RD_ThemeColor_TextWeak),
                                                                     ui_top_font_size(), 0);
          
          // rjf: title
          UI_PrefWidth(ui_children_sum(1)) UI_Row UI_PrefWidth(ui_text_dim(5, 1))
          {
            UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
            ui_box_equip_display_fancy_strings(box, &fstrs);
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
            else
            {
              ui_labelf("Symbols not found at %S", dbgi_key.path);
            }
            di_scope_close(di_scope);
          }
          
          // rjf: unwind
          if(ctrl_entity->kind == CTRL_EntityKind_Thread)
          {
            CTRL_Entity *process = ctrl_entity_ancestor_from_kind(ctrl_entity, CTRL_EntityKind_Process);
            CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(ctrl_entity);
            CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
            if(rich_unwind.concrete_frame_count != 0)
            {
              ui_spacer(ui_em(1.5f, 1.f));
            }
            for(U64 idx = 0; idx < rich_unwind.concrete_frame_count; idx += 1)
            {
              CTRL_CallStackFrame *f = &rich_unwind.frames[idx];
              RDI_Parsed *rdi = f->rdi;
              RDI_Procedure *procedure = f->procedure;
              U64 rip_vaddr = regs_rip_from_arch_block(arch, f->regs);
              CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
              String8 module_name = module == &ctrl_entity_nil ? str8_lit("???") : str8_skip_last_slash(module->string);
              
              // rjf: inline frames
              for(CTRL_CallStackInlineFrame *fin = f->last_inline_frame; fin != 0; fin = fin->prev)
                UI_PrefWidth(ui_children_sum(1)) UI_Row
              {
                String8 name = {0};
                name.str = rdi_string_from_idx(rdi, fin->inline_site->name_string_idx, &name.size);
                name.size = Min(512, name.size);
                UI_TextAlignment(UI_TextAlign_Left) RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(12.f, 1)) ui_labelf("0x%I64x", rip_vaddr);
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
              }
              
              // rjf: concrete frame
              UI_PrefWidth(ui_children_sum(1)) UI_Row
              {
                String8 name = {0};
                name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
                name.size = Min(512, name.size);
                UI_TextAlignment(UI_TextAlign_Left) RD_Font(RD_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(12.f, 1)) ui_labelf("0x%I64x", rip_vaddr);
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
      RD_Panel *panel = rd_panel_from_handle(rd_state->drag_drop_regs->panel);
      RD_Entity *entity = rd_entity_from_handle(rd_state->drag_drop_regs->entity);
      RD_View *view = rd_view_from_handle(rd_state->drag_drop_regs->view);
      {
        //- rjf: tab dragging
        if(rd_state->drag_drop_regs_slot == RD_RegSlot_View && !rd_view_is_nil(view))
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
              UI_Row
              {
                RD_IconKind icon_kind = rd_icon_kind_from_view(view);
                DR_FancyStringList fstrs = rd_title_fstrs_from_view(scratch.arena, view, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
                RD_Font(RD_FontSlot_Icons)
                  UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
                  UI_PrefWidth(ui_em(2.5f, 1.f))
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  ui_label(rd_icon_kind_text_table[icon_kind]);
                UI_PrefWidth(ui_text_dim(10, 1))
                {
                  UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                  ui_box_equip_display_fancy_strings(name_box, &fstrs);
                }
              }
              ui_set_next_pref_width(ui_pct(1, 0));
              ui_set_next_pref_height(ui_pct(1, 0));
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *view_preview_container = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip, "###view_preview_container");
              UI_Parent(view_preview_container) UI_Focus(UI_FocusKind_Off) UI_WidthFill
              {
                RD_ViewRuleUIFunctionType *view_ui = view->spec->ui;
                view_ui(str8(view->query_buffer, view->query_string_size), view->params_roots[view->params_read_gen%ArrayCount(view->params_roots)], view_preview_container->rect);
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
#define Handle(name) ui_labelf("%s: [0x%I64x, 0x%I64x]", #name, (regs->name).u64[0], (regs->name).u64[1])
          Handle(window);
          Handle(panel);
          Handle(view);
#undef Handle
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
        for(RD_Window *window = rd_state->first_window; window != 0; window = window->next)
        {
          // rjf: calc ui hash chain length
          F64 avg_ui_hash_chain_length = 0;
          {
            F64 chain_count = 0;
            F64 chain_length_sum = 0;
            for(U64 idx = 0; idx < ws->ui->box_table_size; idx += 1)
            {
              F64 chain_length = 0;
              for(UI_Box *b = ws->ui->box_table[idx].hash_first; !ui_box_is_nil(b); b = b->hash_next)
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
          ui_labelf("Window %p", window);
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_pref_height(ui_children_sum(1));
          UI_Row
          {
            ui_spacer(ui_em(2.f, 1.f));
            ui_labelf("Box Count: %I64u", window->ui->last_build_box_count);
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
        
        //- rjf: draw entity tree
#if 0
        RD_EntityRec rec = {0};
        S32 indent = 0;
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("Entity Tree:");
        for(RD_Entity *e = rd_entity_root(); !rd_entity_is_nil(e); e = rec.next)
        {
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_pref_height(ui_children_sum(1));
          UI_Row
          {
            ui_spacer(ui_em(2.f*indent, 1.f));
            RD_Entity *dst = rd_entity_from_handle(e->entity_handle);
            if(!rd_entity_is_nil(dst))
            {
              ui_labelf("[link] %S -> %S", e->string, dst->string);
            }
            else
            {
              ui_labelf("%S: %S", d_entity_kind_display_string_table[e->kind], e->string);
            }
          }
          rec = rd_entity_rec_depth_first_pre(e, rd_entity_root());
          indent += rec.push_count;
          indent -= rec.pop_count;
        }
#endif
      }
    }
    
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
                rd_cmd(RD_CmdKind_RunToAddress, .vaddr = regs->vaddr);
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
            if(regs->file_path.size == 0 && range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_FileOutline, 0, "Go To Source")))
            {
              if(lines.first != 0)
              {
                rd_cmd(RD_CmdKind_FindCodeLocation,
                       .file_path = lines.first->v.file_path,
                       .cursor    = lines.first->v.pt,
                       .vaddr     = 0,
                       .process   = ctrl_handle_zero());
              }
              ui_ctx_menu_close();
            }
            if(regs->file_path.size != 0 && range.min.line == range.max.line && ui_clicked(rd_icon_buttonf(RD_IconKind_FileOutline, 0, "Go To Disassembly")))
            {
              CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
              U64 vaddr = 0;
              for(D_LineNode *n = lines.first; n != 0; n = n->next)
              {
                CTRL_EntityList modules = ctrl_modules_from_dbgi_key(scratch.arena, d_state->ctrl_entity_store, &n->v.dbgi_key);
                CTRL_Entity *module = ctrl_module_from_thread_candidates(d_state->ctrl_entity_store, thread, &modules);
                if(module != &ctrl_entity_nil)
                {
                  vaddr = ctrl_vaddr_from_voff(module, n->v.voff_range.min);
                  break;
                }
              }
              rd_cmd(RD_CmdKind_FindCodeLocation, .vaddr = vaddr);
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
            RD_Panel *panel = rd_panel_from_handle(regs->panel);
            RD_View *view = rd_view_from_handle(regs->view);
            RD_IconKind view_icon = rd_icon_kind_from_view(view);
            DR_FancyStringList fstrs = rd_title_fstrs_from_view(scratch.arena, view, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
            String8 file_path = rd_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
            
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
                ui_box_equip_display_fancy_strings(name_box, &fstrs);
              }
            }
            
            RD_Palette(RD_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
            
            // rjf: copy name
            if(ui_clicked(rd_icon_buttonf(RD_IconKind_Clipboard, 0, "Copy Name")))
            {
              os_set_clipboard_text(dr_string_from_fancy_string_list(scratch.arena, &fstrs));
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
            if(view->spec->flags & RD_ViewRuleInfoFlag_CanFilter)
            {
              if(ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_Filter].string)))
              {
                rd_cmd(RD_CmdKind_Filter, .panel = rd_handle_from_panel(panel), .view = rd_handle_from_view(view));
                ui_ctx_menu_close();
              }
              if(ui_clicked(rd_cmd_spec_button(rd_cmd_kind_info_table[RD_CmdKind_ClearFilter].string)))
              {
                rd_cmd(RD_CmdKind_ClearFilter, .panel = rd_handle_from_panel(panel), .view = rd_handle_from_view(view));
                ui_ctx_menu_close();
              }
            }
            
            // rjf: close tab
            if(ui_clicked(rd_icon_buttonf(RD_IconKind_X, 0, "Close Tab")))
            {
              rd_cmd(RD_CmdKind_CloseTab, .panel = rd_handle_from_panel(panel), .view = rd_handle_from_view(view));
              ui_ctx_menu_close();
            }
            
            // rjf: param tree editing
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
              DR_FancyStringList fstrs = rd_title_fstrs_from_ctrl_entity(scratch.arena, ctrl_entity, ui_top_palette()->text_weak, ui_top_font_size(), 0);
              UI_Box *title_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
              ui_box_equip_display_fancy_strings(title_box, &fstrs);
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
              UI_Signal sig = rd_line_editf(RD_LineEditFlag_Border|RD_LineEditFlag_CodeContents, 0, 0, &ws->ctx_menu_input_cursor, &ws->ctx_menu_input_mark, ws->ctx_menu_input_buffer, ws->ctx_menu_input_buffer_size, &ws->ctx_menu_input_string_size, 0, ctrl_entity->string, "Name###ctrl_entity_string_edit_%p", ctrl_entity);
              if(ui_committed(sig))
              {
                rd_cmd(RD_CmdKind_SetEntityName, .ctrl_entity = ctrl_entity->handle, .string = str8(ws->ctx_menu_input_buffer, ws->ctx_menu_input_string_size));
              }
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
            
            // rjf: copy call stack
            if(ctrl_entity->kind == CTRL_EntityKind_Thread)
            {
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_Clipboard, 0, "Copy Call Stack")))
              {
                DI_Scope *di_scope = di_scope_open();
                CTRL_Entity *process = ctrl_entity_ancestor_from_kind(ctrl_entity, CTRL_EntityKind_Process);
                CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(ctrl_entity);
                CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
                String8List lines = {0};
                for(U64 frame_idx = 0; frame_idx < rich_unwind.concrete_frame_count; frame_idx += 1)
                {
                  CTRL_CallStackFrame *concrete_frame = &rich_unwind.frames[frame_idx];
                  U64 rip_vaddr = regs_rip_from_arch_block(ctrl_entity->arch, concrete_frame->regs);
                  CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
                  RDI_Parsed *rdi = concrete_frame->rdi;
                  RDI_Procedure *procedure = concrete_frame->procedure;
                  for(CTRL_CallStackInlineFrame *inline_frame = concrete_frame->last_inline_frame;
                      inline_frame != 0;
                      inline_frame = inline_frame->prev)
                  {
                    RDI_InlineSite *inline_site = inline_frame->inline_site;
                    String8 name = {0};
                    name.str = rdi_string_from_idx(rdi, inline_site->name_string_idx, &name.size);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [inlined] \"%S\"%s%S", rip_vaddr, name, module == &ctrl_entity_nil ? "" : " in ", module->string);
                  }
                  if(procedure != 0)
                  {
                    String8 name = {0};
                    name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: \"%S\"%s%S", rip_vaddr, name, module == &ctrl_entity_nil ? "" : " in ", module->string);
                  }
                  else if(module != &ctrl_entity_nil)
                  {
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [??? in %S]", rip_vaddr, module->string);
                  }
                  else
                  {
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [??? in ???]", rip_vaddr);
                  }
                }
                StringJoin join = {0};
                join.sep = join.post = str8_lit("\n");
                String8 text = str8_list_join(scratch.arena, &lines, &join);
                os_set_clipboard_text(text);
                ui_ctx_menu_close();
                di_scope_close(di_scope);
              }
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
          
          //////////////////////
          //- rjf: frontend entities
          //
          case RD_RegSlot_Entity:
          {
            RD_Entity *entity = rd_entity_from_handle(regs->entity);
            RD_EntityKindFlags kind_flags = rd_entity_kind_flags_table[entity->kind];
            
            //- rjf: title
            UI_Row
              UI_PrefWidth(ui_text_dim(5, 1))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_TextPadding(ui_top_font_size()*1.5f)
            {
              DR_FancyStringList fstrs = rd_title_fstrs_from_entity(scratch.arena, entity, ui_top_palette()->text_weak, ui_top_font_size());
              UI_Box *title_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
              ui_box_equip_display_fancy_strings(title_box, &fstrs);
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
            if(kind_flags & RD_EntityKindFlag_CanRename) RD_Font(RD_FontSlot_Code) UI_TextPadding(ui_top_font_size()*1.5f)
            {
              UI_Signal sig = rd_line_editf(RD_LineEditFlag_Border|RD_LineEditFlag_CodeContents, 0, 0, &ws->ctx_menu_input_cursor, &ws->ctx_menu_input_mark, ws->ctx_menu_input_buffer, ws->ctx_menu_input_buffer_size, &ws->ctx_menu_input_string_size, 0, entity->string, "Name###entity_string_edit_%p", ctrl_entity);
              if(ui_committed(sig))
              {
                rd_cmd(RD_CmdKind_NameEntity, .entity = regs->entity, .string = str8(ws->ctx_menu_input_buffer, ws->ctx_menu_input_string_size));
              }
            }
            
            //- rjf: condition editor
            if(kind_flags & RD_EntityKindFlag_CanCondition) RD_Font(RD_FontSlot_Code) UI_TextPadding(ui_top_font_size()*1.5f)
            {
              UI_Signal sig = rd_line_editf(RD_LineEditFlag_Border|RD_LineEditFlag_CodeContents, 0, 0, &ws->ctx_menu_input_cursor, &ws->ctx_menu_input_mark, ws->ctx_menu_input_buffer, ws->ctx_menu_input_buffer_size, &ws->ctx_menu_input_string_size, 0, rd_entity_child_from_kind(entity, RD_EntityKind_Condition)->string, "Condition###entity_condition_edit_%p", entity);
              if(ui_committed(sig))
              {
                rd_cmd(RD_CmdKind_ConditionEntity, .entity = regs->entity, .string = str8(ws->ctx_menu_input_buffer, ws->ctx_menu_input_string_size));
              }
            }
            
            //- rjf: name editor
            if(entity->cfg_src == RD_CfgSrc_CommandLine)
            {
              if(ui_clicked(rd_icon_buttonf(RD_IconKind_Save, 0, "Save To Project")))
              {
                rd_entity_equip_cfg_src(entity, RD_CfgSrc_Project);
              }
            }
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
    //- rjf: build auto-complete lister
    //
    ProfScope("build autocomplete lister")
      if(!ui_key_match(ws->autocomp_root_key, ui_key_zero()) && ws->autocomp_last_frame_idx+1 >= rd_state->frame_index)
    {
      String8 input = str8(ws->autocomp_lister_input_buffer, ws->autocomp_lister_input_size);
      String8 query_word = rd_autocomp_query_word_from_input_string_off(input, ws->autocomp_cursor_off);
      String8 query_path = rd_autocomp_query_path_from_input_string_off(input, ws->autocomp_cursor_off);
      UI_Box *autocomp_root_box = ui_box_from_key(ws->autocomp_root_key);
      if(!ui_box_is_nil(autocomp_root_box))
      {
        Temp scratch = scratch_begin(0, 0);
        DI_Scope *di_scope = di_scope_open();
        DI_KeyList dbgi_keys_list = d_push_active_dbgi_key_list(scratch.arena);
        DI_KeyArray dbgi_keys = di_key_array_from_list(scratch.arena, &dbgi_keys_list);
        
        //- rjf: grab rdis
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
        
        //- rjf: unpack lister params
        CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
        U64 thread_rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, rd_base_regs()->unwind_count);
        CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
        CTRL_Entity *module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
        U64 thread_rip_voff = ctrl_voff_from_vaddr(module, thread_rip_vaddr);
        DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
        
        //- rjf: gather lister items
        RD_AutoCompListerItemChunkList item_list = {0};
        {
          //- rjf: gather locals
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Locals)
          {
            E_String2NumMap *locals_map = d_query_cached_locals_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
            E_String2NumMap *member_map = d_query_cached_member_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
            for(E_String2NumMapNode *n = locals_map->first; n != 0; n = n->order_next)
            {
              RD_AutoCompListerItem item = {0};
              {
                item.string      = n->string;
                item.kind_string = str8_lit("Local");
                item.matches     = fuzzy_match_find(scratch.arena, query_word, n->string);
              }
              if(query_word.size == 0 || item.matches.count != 0)
              {
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
            for(E_String2NumMapNode *n = member_map->first; n != 0; n = n->order_next)
            {
              RD_AutoCompListerItem item = {0};
              {
                item.string      = n->string;
                item.kind_string = str8_lit("Local (Member)");
                item.matches     = fuzzy_match_find(scratch.arena, query_word, n->string);
              }
              if(query_word.size == 0 || item.matches.count != 0)
              {
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather registers
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Registers)
          {
            Arch arch = thread->arch;
            U64 reg_names_count = regs_reg_code_count_from_arch(arch);
            U64 alias_names_count = regs_alias_code_count_from_arch(arch);
            String8 *reg_names = regs_reg_code_string_table_from_arch(arch);
            String8 *alias_names = regs_alias_code_string_table_from_arch(arch);
            for(U64 idx = 0; idx < reg_names_count; idx += 1)
            {
              if(reg_names[idx].size != 0)
              {
                RD_AutoCompListerItem item = {0};
                {
                  item.string      = reg_names[idx];
                  item.kind_string = str8_lit("Register");
                  item.matches     = fuzzy_match_find(scratch.arena, query_word, reg_names[idx]);
                }
                if(query_word.size == 0 || item.matches.count != 0)
                {
                  rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
                }
              }
            }
            for(U64 idx = 0; idx < alias_names_count; idx += 1)
            {
              if(alias_names[idx].size != 0)
              {
                RD_AutoCompListerItem item = {0};
                {
                  item.string      = alias_names[idx];
                  item.kind_string = str8_lit("Reg. Alias");
                  item.matches     = fuzzy_match_find(scratch.arena, query_word, alias_names[idx]);
                }
                if(query_word.size == 0 || item.matches.count != 0)
                {
                  rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
                }
              }
            }
          }
          
          //- rjf: gather view rules
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_ViewRules)
          {
            for(U64 slot_idx = 0; slot_idx < d_state->view_rule_spec_table_size; slot_idx += 1)
            {
              for(D_ViewRuleSpec *spec = d_state->view_rule_spec_table[slot_idx]; spec != 0 && spec != &d_nil_core_view_rule_spec; spec = spec->hash_next)
              {
                RD_AutoCompListerItem item = {0};
                {
                  item.string      = spec->info.string;
                  item.kind_string = str8_lit("View Rule");
                  item.matches     = fuzzy_match_find(scratch.arena, query_word, spec->info.string);
                }
                if(query_word.size == 0 || item.matches.count != 0)
                {
                  rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
                }
              }
            }
          }
          
          //- rjf: gather members
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Members)
          {
            // TODO(rjf)
          }
          
          //- rjf: gather globals
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Globals && query_word.size != 0)
          {
            U128 search_key = {d_hash_from_string(str8_lit("autocomp_globals_search_key")), d_hash_from_string(str8_lit("autocomp_globals_search_key"))};
            DI_SearchParams search_params =
            {
              RDI_SectionKind_GlobalVariables,
              dbgi_keys,
            };
            B32 is_stale = 0;
            DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, query_word, 0, &is_stale);
            for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
            {
              // rjf: unpack info
              RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
              String8 name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
              
              // rjf: push item
              RD_AutoCompListerItem item = {0};
              {
                item.string      = name;
                item.kind_string = str8_lit("Global");
                item.matches     = items.v[idx].match_ranges;
                item.group       = 1;
              }
              rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
            }
          }
          
          //- rjf: gather thread locals
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_ThreadLocals && query_word.size != 0)
          {
            U128 search_key = {d_hash_from_string(str8_lit("autocomp_tvars_dis_key")), d_hash_from_string(str8_lit("autocomp_tvars_dis_key"))};
            DI_SearchParams search_params =
            {
              RDI_SectionKind_ThreadVariables,
              dbgi_keys,
            };
            B32 is_stale = 0;
            DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, query_word, 0, &is_stale);
            for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
            {
              // rjf: unpack info
              RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
              String8 name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
              
              // rjf: push item
              RD_AutoCompListerItem item = {0};
              {
                item.string      = name;
                item.kind_string = str8_lit("Thread Local");
                item.matches     = items.v[idx].match_ranges;
                item.group       = 1;
              }
              rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
            }
          }
          
          //- rjf: gather procedures
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Procedures && query_word.size != 0)
          {
            U128 search_key = {d_hash_from_string(str8_lit("autocomp_procedures_search_key")), d_hash_from_string(str8_lit("autocomp_procedures_search_key"))};
            DI_SearchParams search_params =
            {
              RDI_SectionKind_Procedures,
              dbgi_keys,
            };
            B32 is_stale = 0;
            DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, query_word, 0, &is_stale);
            for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
            {
              // rjf: unpack info
              RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
              String8 name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
              
              // rjf: push item
              RD_AutoCompListerItem item = {0};
              {
                item.string      = name;
                item.kind_string = str8_lit("Procedure");
                item.matches     = items.v[idx].match_ranges;
                item.group       = 1;
              }
              rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
            }
          }
          
          //- rjf: gather types
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Types && query_word.size != 0)
          {
            U128 search_key = {d_hash_from_string(str8_lit("autocomp_types_search_key")), d_hash_from_string(str8_lit("autocomp_types_search_key"))};
            DI_SearchParams search_params =
            {
              RDI_SectionKind_UDTs,
              dbgi_keys,
            };
            B32 is_stale = 0;
            DI_SearchItemArray items = di_search_items_from_key_params_query(di_scope, search_key, &search_params, query_word, 0, &is_stale);
            for(U64 idx = 0; idx < 20 && idx < items.count; idx += 1)
            {
              // rjf: unpack info
              RDI_Parsed *rdi = rdis[items.v[idx].dbgi_idx];
              String8 name = di_search_item_string_from_rdi_target_element_idx(rdi, search_params.target, items.v[idx].idx);
              
              // rjf: push item
              RD_AutoCompListerItem item = {0};
              {
                item.string      = name;
                item.kind_string = str8_lit("Type");
                item.matches     = items.v[idx].match_ranges;
                item.group       = 1;
              }
              rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
            }
          }
          
          //- rjf: gather languages
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Languages)
          {
            for EachNonZeroEnumVal(TXT_LangKind, lang)
            {
              RD_AutoCompListerItem item = {0};
              {
                item.string      = txt_extension_from_lang_kind(lang);
                item.kind_string = str8_lit("Language");
                item.matches     = fuzzy_match_find(scratch.arena, query_word, item.string);
              }
              if(item.string.size != 0 && (query_word.size == 0 || item.matches.count != 0))
              {
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather architectures
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Architectures)
          {
            for EachNonZeroEnumVal(Arch, arch)
            {
              RD_AutoCompListerItem item = {0};
              {
                item.string      = string_from_arch(arch);
                item.kind_string = str8_lit("Arch");
                item.matches     = fuzzy_match_find(scratch.arena, query_word, item.string);
              }
              if(query_word.size == 0 || item.matches.count != 0)
              {
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather tex2dformats
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Tex2DFormats)
          {
            for EachEnumVal(R_Tex2DFormat, fmt)
            {
              RD_AutoCompListerItem item = {0};
              {
                item.string      = lower_from_str8(scratch.arena, r_tex2d_format_display_string_table[fmt]);
                item.kind_string = str8_lit("Format");
                item.matches     = fuzzy_match_find(scratch.arena, query_word, item.string);
              }
              if(query_word.size == 0 || item.matches.count != 0)
              {
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather view rule params
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_ViewRuleParams)
          {
            for(String8Node *n = ws->autocomp_lister_params.strings.first; n != 0; n = n->next)
            {
              String8 string = n->string;
              RD_AutoCompListerItem item = {0};
              {
                item.string      = string;
                item.kind_string = str8_lit("Parameter");
                item.matches     = fuzzy_match_find(scratch.arena, query_word, item.string);
              }
              if(query_word.size == 0 || item.matches.count != 0)
              {
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather files
          if(ws->autocomp_lister_params.flags & RD_AutoCompListerFlag_Files)
          {
            // rjf: find containing directory in query_path
            String8 dir_str_in_input = {0};
            for(U64 i = 0; i < query_path.size; i += 1)
            {
              String8 substr1 = str8_substr(query_path, r1u64(i, i+1));
              String8 substr2 = str8_substr(query_path, r1u64(i, i+2));
              String8 substr3 = str8_substr(query_path, r1u64(i, i+3));
              if(str8_match(substr1, str8_lit("/"), StringMatchFlag_SlashInsensitive))
              {
                dir_str_in_input = str8_substr(query_path, r1u64(i, query_path.size));
              }
              else if(i != 0 && str8_match(substr2, str8_lit(":/"), StringMatchFlag_SlashInsensitive))
              {
                dir_str_in_input = str8_substr(query_path, r1u64(i-1, query_path.size));
              }
              else if(str8_match(substr2, str8_lit("./"), StringMatchFlag_SlashInsensitive))
              {
                dir_str_in_input = str8_substr(query_path, r1u64(i, query_path.size));
              }
              else if(str8_match(substr3, str8_lit("../"), StringMatchFlag_SlashInsensitive))
              {
                dir_str_in_input = str8_substr(query_path, r1u64(i, query_path.size));
              }
              if(dir_str_in_input.size != 0)
              {
                break;
              }
            }
            
            // rjf: use query_path string to form various parts of search space
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
              prefix = str8_substr(query_path, r1u64(0, path.str - query_path.str));
            }
            
            // rjf: get current files, filtered
            B32 allow_dirs = 1;
            OS_FileIter *it = os_file_iter_begin(scratch.arena, path, 0);
            for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, it, &info);)
            {
              FuzzyMatchRangeList match_ranges = fuzzy_match_find(scratch.arena, search, info.name);
              B32 fits_search = (search.size == 0 || match_ranges.count == match_ranges.needle_part_count);
              B32 fits_dir_only = (allow_dirs || !(info.props.flags & FilePropertyFlag_IsFolder));
              if(fits_search && fits_dir_only)
              {
                RD_AutoCompListerItem item = {0};
                {
                  item.string      = info.props.flags & FilePropertyFlag_IsFolder ? push_str8f(scratch.arena, "%S/", info.name) : info.name;
                  item.kind_string = info.props.flags & FilePropertyFlag_IsFolder ? str8_lit("Folder") : str8_lit("File");
                  item.matches     = match_ranges;
                  item.is_non_code = 1;
                }
                rd_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
            os_file_iter_end(it);
          }
        }
        
        //- rjf: lister item list -> sorted array
        RD_AutoCompListerItemArray item_array = rd_autocomp_lister_item_array_from_chunk_list(scratch.arena, &item_list);
        rd_autocomp_lister_item_array_sort__in_place(&item_array);
        
        //- rjf: animate
        {
          // rjf: animate target # of rows
          {
            F32 rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? (1 - pow_f32(2, (-60.f * rd_state->frame_dt))) : 1.f;
            F32 target = Min((F32)item_array.count, 16.f);
            if(abs_f32(target - ws->autocomp_num_visible_rows_t) > 0.01f)
            {
              rd_request_frame();
            }
            ws->autocomp_num_visible_rows_t += (target - ws->autocomp_num_visible_rows_t) * rate;
            if(abs_f32(target - ws->autocomp_num_visible_rows_t) <= 0.02f)
            {
              ws->autocomp_num_visible_rows_t = target;
            }
          }
          
          // rjf: animate open
          {
            F32 rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * rd_state->frame_dt)) : 1.f;
            F32 diff = 1.f-ws->autocomp_open_t;
            ws->autocomp_open_t += diff*rate;
            if(abs_f32(diff) < 0.05f)
            {
              ws->autocomp_open_t = 1.f;
            }
            else
            {
              rd_request_frame();
            }
          }
        }
        
        //- rjf: build
        if(item_array.count != 0)
        {
          F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
          ui_set_next_fixed_x(autocomp_root_box->rect.x0);
          ui_set_next_fixed_y(autocomp_root_box->rect.y1);
          ui_set_next_pref_width(ui_em(30.f, 1.f));
          ui_set_next_pref_height(ui_px(row_height_px*ws->autocomp_num_visible_rows_t + ui_top_font_size()*2.f, 1.f));
          ui_set_next_child_layout_axis(Axis2_Y);
          ui_set_next_corner_radius_01(ui_top_font_size()*0.25f);
          ui_set_next_corner_radius_11(ui_top_font_size()*0.25f);
          ui_set_next_corner_radius_10(ui_top_font_size()*0.25f);
          UI_Focus(UI_FocusKind_On)
            UI_Squish(0.25f-0.25f*ws->autocomp_open_t)
            UI_Transparency(1.f-ws->autocomp_open_t)
            RD_Palette(RD_PaletteCode_Floating)
          {
            autocomp_box = ui_build_box_from_stringf(UI_BoxFlag_DefaultFocusNavY|
                                                     UI_BoxFlag_Clickable|
                                                     UI_BoxFlag_Clip|
                                                     UI_BoxFlag_RoundChildrenByParent|
                                                     UI_BoxFlag_DisableFocusOverlay|
                                                     UI_BoxFlag_DrawBorder|
                                                     UI_BoxFlag_DrawBackgroundBlur|
                                                     UI_BoxFlag_DrawDropShadow|
                                                     UI_BoxFlag_DrawBackground,
                                                     "autocomp_box");
            if(ws->autocomp_input_dirty)
            {
              ws->autocomp_input_dirty = 0;
              autocomp_box->default_nav_focus_hot_key = autocomp_box->default_nav_focus_active_key = autocomp_box->default_nav_focus_next_hot_key = autocomp_box->default_nav_focus_next_active_key = ui_key_zero();
            }
          }
          UI_Parent(autocomp_box)
            UI_WidthFill
            UI_PrefHeight(ui_px(row_height_px, 1.f))
            RD_Font(RD_FontSlot_Code)
            UI_HoverCursor(OS_Cursor_HandPoint)
            UI_Focus(UI_FocusKind_Null)
            RD_Palette(RD_PaletteCode_ImplicitButton)
            UI_Padding(ui_em(1.f, 1.f))
          {
            for(U64 idx = 0; idx < item_array.count; idx += 1)
            {
              RD_AutoCompListerItem *item = &item_array.v[idx];
              UI_Box *item_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects|UI_BoxFlag_MouseClickable, "autocomp_%I64x", idx);
              UI_Parent(item_box) UI_Padding(ui_em(1.f, 1.f))
              {
                UI_WidthFill RD_Font(item->is_non_code ? RD_FontSlot_Main : RD_FontSlot_Code)
                {
                  UI_Box *box = item->is_non_code ? ui_label(item->string).box : rd_code_label(1.f, 0, ui_top_palette()->text, item->string);
                  ui_box_equip_fuzzy_match_ranges(box, &item->matches);
                }
                RD_Font(RD_FontSlot_Main)
                  UI_PrefWidth(ui_text_dim(10, 1))
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  ui_label(item->kind_string);
              }
              UI_Signal item_sig = ui_signal_from_box(item_box);
              if(ui_clicked(item_sig))
              {
                UI_Event move_back_evt = zero_struct;
                move_back_evt.kind = UI_EventKind_Navigate;
                move_back_evt.flags = UI_EventFlag_KeepMark;
                move_back_evt.delta_2s32.x = -(S32)query_word.size;
                ui_event_list_push(ui_build_arena(), &ws->ui_events, &move_back_evt);
                UI_Event paste_evt = zero_struct;
                paste_evt.kind = UI_EventKind_Text;
                paste_evt.string = item->string;
                ui_event_list_push(ui_build_arena(), &ws->ui_events, &paste_evt);
                autocomp_box->default_nav_focus_hot_key = autocomp_box->default_nav_focus_active_key = autocomp_box->default_nav_focus_next_hot_key = autocomp_box->default_nav_focus_next_active_key = ui_key_zero();
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
        
        di_scope_close(di_scope);
        scratch_end(scratch);
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
                rd_cmd_kind_info_table[RD_CmdKind_ExceptionFilters].string,
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
                'e',
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
            RD_EntityList tasks = rd_query_cached_entity_list_with_kind(RD_EntityKind_ConversionTask);
            for(RD_EntityNode *n = tasks.first; n != 0; n = n->next)
            {
              RD_Entity *task = n->entity;
              if(task->alloc_time_us + 500000 < os_now_microseconds())
              {
                String8 rdi_path = task->string;
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
          RD_EntityList targets = rd_push_active_target_list(scratch.arena);
          RD_EntityList processes = rd_query_cached_entity_list_with_kind(RD_EntityKind_Process);
          B32 have_targets = targets.count != 0;
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
                  for(RD_EntityNode *n = targets.first; n != 0; n = n->next)
                  {
                    String8 target_display_name = rd_display_string_from_entity(scratch.arena, n->entity);
                    ui_label(target_display_name);
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
              String8 user_path = rd_cfg_path_from_src(RD_CfgSrc_User);
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
              String8 prof_path = rd_cfg_path_from_src(RD_CfgSrc_Project);
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
              rd_cmd(RD_CmdKind_CloseWindow, .window = rd_handle_from_window(ws));
            }
            os_window_push_custom_title_bar_client_area(ws->os, min_sig.box->rect);
            os_window_push_custom_title_bar_client_area(ws->os, max_sig.box->rect);
            os_window_push_custom_title_bar_client_area(ws->os, pad_2f32(cls_sig.box->rect, 2.f));
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
          if(is_running)
          {
            ui_label(str8_lit("Running"));
          }
          else
          {
            Temp scratch = scratch_begin(0, 0);
            RD_IconKind icon = RD_IconKind_Null;
            String8 explanation = str8_lit("Not running");
            {
              String8 stop_explanation = rd_stop_explanation_string_icon_from_ctrl_event(scratch.arena, &stop_event, &icon);
              if(stop_explanation.size != 0)
              {
                explanation = stop_explanation;
              }
            }
            if(icon != RD_IconKind_Null)
            {
              UI_PrefWidth(ui_em(2.25f, 1.f))
                RD_Font(RD_FontSlot_Icons)
                UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Icons))
                ui_label(rd_icon_kind_text_table[icon]);
            }
            UI_PrefWidth(ui_text_dim(10, 1)) ui_label(explanation);
            scratch_end(scratch);
          }
        }
        
        ui_spacer(ui_pct(1, 0));
        
        // rjf: bind change visualization
        if(rd_state->bind_change_active)
        {
          RD_CmdKindInfo *info = rd_cmd_kind_info_from_string(rd_state->bind_change_cmd_name);
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_Flags(UI_BoxFlag_DrawBackground)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_CornerRadius(4)
            RD_Palette(RD_PaletteCode_NeutralPopButton)
            ui_labelf("Currently rebinding \"%S\" hotkey", info->display_name);
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
    //- rjf: prepare query view stack for the in-progress command
    //
    if(ws->query_cmd_name.size != 0)
    {
      RD_CmdKindInfo *cmd_kind_info = rd_cmd_kind_info_from_string(ws->query_cmd_name);
      RD_RegSlot missing_slot = cmd_kind_info->query.slot;
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
      if(ws->query_view_stack_top->spec != view_spec ||
         rd_view_is_nil(ws->query_view_stack_top))
      {
        Temp scratch = scratch_begin(0, 0);
        
        // rjf: clear existing query stack
        for(RD_View *query_view = ws->query_view_stack_top, *next = 0;
            !rd_view_is_nil(query_view);
            query_view = next)
        {
          next = query_view->order_next;
          rd_view_release(query_view);
        }
        
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
        RD_View *view = rd_view_alloc();
        rd_view_equip_spec(view, view_spec, default_query, &md_nil_node);
        if(cmd_kind_info->query.flags & RD_QueryFlag_SelectOldInput)
        {
          view->query_mark = txt_pt(1, 1);
        }
        ws->query_view_stack_top = view;
        ws->query_view_selected = 1;
        view->order_next = &rd_nil_view;
        
        scratch_end(scratch);
      }
    }
    
    ////////////////////////////
    //- rjf: animate query info
    //
    {
      F32 rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * rd_state->frame_dt)) : 1.f;
      
      // rjf: animate query view selection transition
      {
        F32 target = (F32)!!ws->query_view_selected;
        F32 diff = abs_f32(target - ws->query_view_selected_t);
        if(diff > 0.005f)
        {
          rd_request_frame();
          if(diff < 0.005f)
          {
            ws->query_view_selected_t = target;
          }
          ws->query_view_selected_t += (target - ws->query_view_selected_t) * rate;
        }
      }
      
      // rjf: animate query view open/close transition
      {
        F32 query_view_t_target = !rd_view_is_nil(ws->query_view_stack_top);
        F32 diff = abs_f32(query_view_t_target - ws->query_view_t);
        if(diff > 0.005f)
        {
          rd_request_frame();
        }
        if(diff < 0.005f)
        {
          ws->query_view_t = query_view_t_target;
        }
        ws->query_view_t += (query_view_t_target - ws->query_view_t) * rate;
      }
    }
    
    ////////////////////////////
    //- rjf: build query
    //
    if(!rd_view_is_nil(ws->query_view_stack_top))
      UI_Focus((window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && ws->query_view_selected) ? UI_FocusKind_On : UI_FocusKind_Off)
      RD_Palette(RD_PaletteCode_Floating)
    {
      RD_View *view = ws->query_view_stack_top;
      String8 query_cmd_name = ws->query_cmd_name;
      RD_CmdKindInfo *query_cmd_info = rd_cmd_kind_info_from_string(query_cmd_name);
      RD_Query *query = &query_cmd_info->query;
      
      //- rjf: calculate rectangles
      Vec2F32 window_center = center_2f32(window_rect);
      F32 query_container_width = dim_2f32(window_rect).x*0.5f;
      F32 query_container_margin = ui_top_font_size()*8.f;
      F32 query_line_edit_height = ui_top_font_size()*3.f;
      Rng2F32 query_container_rect = r2f32p(window_center.x - query_container_width/2 + (1-ws->query_view_t)*query_container_width/4,
                                            window_rect.y0 + query_container_margin,
                                            window_center.x + query_container_width/2 - (1-ws->query_view_t)*query_container_width/4,
                                            window_rect.y1 - query_container_margin);
      if(ws->query_view_stack_top->spec == &rd_nil_view_rule_info)
      {
        query_container_rect.y1 = query_container_rect.y0 + query_line_edit_height;
      }
      query_container_rect.y1 = mix_1f32(query_container_rect.y0, query_container_rect.y1, ws->query_view_t);
      Rng2F32 query_container_content_rect = r2f32p(query_container_rect.x0,
                                                    query_container_rect.y0+query_line_edit_height,
                                                    query_container_rect.x1,
                                                    query_container_rect.y1);
      
      //- rjf: build floating query view container
      UI_Box *query_container_box = &ui_nil_box;
      UI_Rect(query_container_rect)
        UI_CornerRadius(ui_top_font_size()*0.2f)
        UI_ChildLayoutAxis(Axis2_Y)
        UI_Squish(0.25f-ws->query_view_t*0.25f)
        UI_Transparency(1-ws->query_view_t)
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
                                                        "panel_query_container");
      }
      
      //- rjf: build query text input
      B32 query_completed = 0;
      B32 query_cancelled = 0;
      UI_Parent(query_container_box)
        UI_WidthFill UI_PrefHeight(ui_px(query_line_edit_height, 1.f))
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
                                         &view->query_cursor,
                                         &view->query_mark,
                                         view->query_buffer,
                                         sizeof(view->query_buffer),
                                         &view->query_string_size,
                                         0,
                                         str8(view->query_buffer, view->query_string_size),
                                         str8_lit("###query_text_input"));
            if(ui_pressed(sig))
            {
              ws->query_view_selected = 1;
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
      
      //- rjf: build query view
      UI_Parent(query_container_box) UI_WidthFill UI_Focus(UI_FocusKind_Null)
        RD_RegsScope(.view = rd_handle_from_view(view))
      {
        RD_ViewRuleUIFunctionType *view_ui = view->spec->ui;
        view_ui(str8(view->query_buffer, view->query_string_size), view->params_roots[view->params_read_gen%ArrayCount(view->params_roots)], query_container_content_rect);
      }
      
      //- rjf: query submission
      if(((ui_is_focus_active() || (window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && !ws->query_view_selected)) &&
          ui_slot_press(UI_EventActionSlot_Cancel)) || query_cancelled)
      {
        rd_cmd(RD_CmdKind_CancelQuery);
      }
      if((ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept)) || query_completed)
      {
        Temp scratch = scratch_begin(0, 0);
        RD_View *view = ws->query_view_stack_top;
        RD_RegsScope()
        {
          rd_regs_fill_slot_from_string(query->slot, str8(view->query_buffer, view->query_string_size));
          rd_cmd(RD_CmdKind_CompleteQuery);
        }
        scratch_end(scratch);
      }
      
      //- rjf: take fallthrough interaction in query view
      {
        UI_Signal sig = ui_signal_from_box(query_container_box);
        if(ui_pressed(sig))
        {
          ws->query_view_selected = 1;
        }
      }
      
      //- rjf: build darkening overlay for rest of screen
      UI_Palette(ui_build_palette(0, .background = mix_4f32(rd_rgba_from_theme_color(RD_ThemeColor_InactivePanelOverlay), v4f32(0, 0, 0, 0), 1-ws->query_view_selected_t)))
        UI_Rect(window_rect)
      {
        ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
      }
    }
    else
    {
      ws->query_view_selected = 0;
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
        Temp scratch = scratch_begin(0, 0);
        DI_Scope *scope = di_scope_open();
        String8 expr = ws->hover_eval_string;
        E_Eval eval = e_eval_from_string(scratch.arena, expr);
        EV_ViewRuleList top_level_view_rules = {0};
        
        //- rjf: build if good
        if(!e_type_key_match(eval.type_key, e_type_key_zero()) && !ui_any_ctx_menu_is_open())
          UI_Focus((hover_eval_is_open && !ui_any_ctx_menu_is_open() && ws->hover_eval_focused && (!query_is_open || !ws->query_view_selected)) ? UI_FocusKind_Null : UI_FocusKind_Off)
        {
          //- rjf: eval -> viz artifacts
          F32 row_height = floor_f32(ui_top_font_size()*2.8f);
          RD_CfgTable cfg_table = {0};
          U64 expr_hash = d_hash_from_string(expr);
          String8 ev_view_key_string = push_str8f(scratch.arena, "eval_hover_%I64x", expr_hash);
          EV_View *ev_view = rd_ev_view_from_key(d_hash_from_string(ev_view_key_string));
          EV_Key parent_key = ev_key_make(5381, 1);
          EV_Key key = ev_key_make(ev_hash_from_key(parent_key), 1);
          EV_BlockTree block_tree = ev_block_tree_from_string(scratch.arena, ev_view, str8_zero(), expr, &top_level_view_rules);
          EV_BlockRangeList block_ranges = ev_block_range_list_from_tree(scratch.arena, &block_tree);
          // EV_BlockList viz_blocks = ev_block_list_from_view_expr_keys(scratch.arena, ev_view, str8_zero(), &top_level_view_rules, expr, parent_key, key, 0);
          CTRL_Entity *entity = rd_ctrl_entity_from_eval_space(eval.space);
          U32 default_radix = (entity->kind == CTRL_EntityKind_Thread ? 16 : 10);
          EV_WindowedRowList rows = ev_windowed_row_list_from_block_range_list(scratch.arena, ev_view, str8_zero(), &block_ranges, r1u64(0, 50));
          // EV_WindowedRowList viz_rows = ev_windowed_row_list_from_block_list(scratch.arena, ev_view, r1s64(0, 50), &viz_blocks);
          
          //- rjf: animate
          {
            // rjf: animate height
            {
              F32 fish_rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * rd_state->frame_dt)) : 1.f;
              F32 hover_eval_container_height_target = row_height * Min(30, block_tree.total_row_count);
              ws->hover_eval_num_visible_rows_t += (hover_eval_container_height_target - ws->hover_eval_num_visible_rows_t) * fish_rate;
              if(abs_f32(hover_eval_container_height_target - ws->hover_eval_num_visible_rows_t) > 0.5f)
              {
                rd_request_frame();
              }
              else
              {
                ws->hover_eval_num_visible_rows_t = hover_eval_container_height_target;
              }
            }
            
            // rjf: animate open
            {
              F32 fish_rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * rd_state->frame_dt)) : 1.f;
              F32 diff = 1.f - ws->hover_eval_open_t;
              ws->hover_eval_open_t += diff*fish_rate;
              if(abs_f32(diff) < 0.01f)
              {
                ws->hover_eval_open_t = 1.f;
              }
              else
              {
                rd_request_frame();
              }
            }
          }
          
          //- rjf: calculate width
          F32 width_px = 40.f*ui_top_font_size();
          F32 expr_column_width_px = 15.f*ui_top_font_size();
          F32 value_column_width_px = 25.f*ui_top_font_size();
          if(rows.first != 0)
          {
            EV_Row *row = rows.first;
            E_Eval row_eval = e_eval_from_expr(scratch.arena, row->expr);
            String8 row_expr_string = ev_expr_string_from_row(scratch.arena, row, 0);
            String8 row_display_value = rd_value_string_from_eval(scratch.arena, EV_StringFlag_ReadOnlyDisplayRules, default_radix, ui_top_font(), ui_top_font_size(), 500.f, row_eval, row->member, row->view_rules);
            expr_column_width_px = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, 0, row_expr_string).x + ui_top_font_size()*10.f;
            value_column_width_px = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, 0, row_display_value).x + ui_top_font_size()*5.f;
            F32 total_dim_px = (expr_column_width_px + value_column_width_px);
            width_px = Min(80.f*ui_top_font_size(), total_dim_px*1.5f);
          }
          
          //- rjf: build hover eval box
          F32 hover_eval_container_height = ws->hover_eval_num_visible_rows_t;
          F32 corner_radius = ui_top_font_size()*0.25f;
          ui_set_next_fixed_x(ws->hover_eval_spawn_pos.x);
          ui_set_next_fixed_y(ws->hover_eval_spawn_pos.y);
          ui_set_next_pref_width(ui_px(width_px, 1.f));
          ui_set_next_pref_height(ui_px(hover_eval_container_height, 1.f));
          ui_set_next_corner_radius_00(0);
          ui_set_next_corner_radius_01(corner_radius);
          ui_set_next_corner_radius_10(corner_radius);
          ui_set_next_corner_radius_11(corner_radius);
          ui_set_next_child_layout_axis(Axis2_Y);
          ui_set_next_squish(0.25f-0.25f*ws->hover_eval_open_t);
          ui_set_next_transparency(1.f-ws->hover_eval_open_t);
          UI_Focus(UI_FocusKind_On)
          {
            hover_eval_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_DrawBackground|
                                                       UI_BoxFlag_DrawBackgroundBlur|
                                                       UI_BoxFlag_DrawDropShadow|
                                                       UI_BoxFlag_DisableFocusOverlay|
                                                       UI_BoxFlag_Clip|
                                                       UI_BoxFlag_AllowOverflowY|
                                                       UI_BoxFlag_ViewScroll|
                                                       UI_BoxFlag_ViewClamp|
                                                       UI_BoxFlag_Floating|
                                                       UI_BoxFlag_AnimatePos|
                                                       UI_BoxFlag_Clickable|
                                                       UI_BoxFlag_DefaultFocusNav,
                                                       "###hover_eval");
          }
          
          //- rjf: build contents
          UI_Parent(hover_eval_box) UI_PrefHeight(ui_px(row_height, 1.f))
          {
            //- rjf: build rows
            for(EV_Row *row = rows.first; row != 0; row = row->next)
            {
              //- rjf: unpack row
              U64 row_depth = ev_depth_from_block(row->block);
              E_Eval row_eval = e_eval_from_expr(scratch.arena, row->expr);
              String8 row_expr_string = ev_expr_string_from_row(scratch.arena, row, 0);
              String8 row_edit_value = rd_value_string_from_eval(scratch.arena, 0, default_radix, ui_top_font(), ui_top_font_size(), 500.f, row_eval, row->member, row->view_rules);
              String8 row_display_value = rd_value_string_from_eval(scratch.arena, EV_StringFlag_ReadOnlyDisplayRules, default_radix, ui_top_font(), ui_top_font_size(), 500.f, row_eval, row->member, row->view_rules);
              B32 row_is_editable   = ev_row_is_editable(row);
              B32 row_is_expandable = ev_row_is_expandable(row);
              
              //- rjf: determine if row's data is fresh and/or bad
              B32 row_is_fresh = 0;
              B32 row_is_bad = 0;
              switch(row_eval.mode)
              {
                default:{}break;
                case E_Mode_Offset:
                {
                  CTRL_Entity *space_entity = rd_ctrl_entity_from_eval_space(row_eval.space);
                  if(space_entity->kind == CTRL_EntityKind_Process)
                  {
                    U64 size = e_type_byte_size_from_key(row_eval.type_key);
                    size = Min(size, 64);
                    Rng1U64 vaddr_rng = r1u64(row_eval.value.u64, row_eval.value.u64+size);
                    CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, space_entity->handle, vaddr_rng, 0);
                    for(U64 idx = 0; idx < (slice.data.size+63)/64; idx += 1)
                    {
                      if(slice.byte_changed_flags[idx] != 0)
                      {
                        row_is_fresh = 1;
                      }
                      if(slice.byte_bad_flags[idx] != 0)
                      {
                        row_is_bad = 1;
                      }
                    }
                  }
                }break;
              }
              
              //- rjf: build row
              UI_WidthFill UI_Row
              {
                ui_spacer(ui_em(0.5f, 1.f));
                if(row_depth > 0)
                {
                  for(U64 indent = 0; indent < row_depth; indent += 1)
                  {
                    ui_spacer(ui_em(0.5f, 1.f));
                    UI_Flags(UI_BoxFlag_DrawSideLeft) ui_spacer(ui_em(1.f, 1.f));
                  }
                }
                U64 row_hash = ev_hash_from_key(row->key);
                B32 row_is_expanded = ev_expansion_from_key(ev_view, row->key);
                if(row_is_expandable)
                  UI_PrefWidth(ui_em(1.f, 1)) 
                  if(ui_pressed(ui_expanderf(row->block->rows_default_expanded ? !row_is_expanded : row_is_expanded, "###%I64x_%I64x_is_expanded", row->key.parent_hash, row->key.child_id)))
                {
                  ev_key_set_expansion(ev_view, row->block->key, row->key, !row_is_expanded);
                }
                if(!row_is_expandable)
                {
                  UI_PrefWidth(ui_em(1.f, 1))
                    UI_Flags(UI_BoxFlag_DrawTextWeak)
                    RD_Font(RD_FontSlot_Icons)
                    ui_label(rd_icon_kind_text_table[RD_IconKind_Dot]);
                }
                UI_WidthFill UI_TextRasterFlags(rd_raster_flags_from_slot(RD_FontSlot_Code))
                {
                  UI_PrefWidth(ui_px(expr_column_width_px, 1.f)) rd_code_label(1.f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), row_expr_string);
                  ui_spacer(ui_em(1.5f, 1.f));
                  if(row_is_editable)
                  {
                    if(row_is_fresh)
                    {
                      Vec4F32 rgba = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlay);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rgba));
                    }
                    UI_Signal sig = rd_line_editf(RD_LineEditFlag_CodeContents|
                                                  RD_LineEditFlag_DisplayStringIsCode|
                                                  RD_LineEditFlag_PreferDisplayString|
                                                  RD_LineEditFlag_Border,
                                                  0, 0, &ws->hover_eval_txt_cursor, &ws->hover_eval_txt_mark, ws->hover_eval_txt_buffer, sizeof(ws->hover_eval_txt_buffer), &ws->hover_eval_txt_size, 0, row_edit_value, "%S###val_%I64x", row_display_value, row_hash);
                    if(ui_pressed(sig))
                    {
                      ws->hover_eval_focused = 1;
                    }
                    if(ui_committed(sig))
                    {
                      String8 commit_string = str8(ws->hover_eval_txt_buffer, ws->hover_eval_txt_size);
                      B32 success = rd_commit_eval_value_string(row_eval, commit_string, 1);
                      if(success == 0)
                      {
                        log_user_error(str8_lit("Could not commit value successfully."));
                      }
                    }
                  }
                  else
                  {
                    if(row_is_fresh)
                    {
                      Vec4F32 rgba = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlay);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rgba));
                      ui_set_next_flags(UI_BoxFlag_DrawBackground);
                    }
                    rd_code_label(1.f, 1, rd_rgba_from_theme_color(RD_ThemeColor_CodeDefault), row_display_value);
                  }
                }
                if(row == rows.first)
                {
                  UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_CornerRadius00(0)
                    UI_CornerRadius01(0)
                    UI_CornerRadius10(0)
                    UI_CornerRadius11(0)
                  {
                    UI_Signal watch_sig = rd_icon_buttonf(RD_IconKind_List, 0, "###watch_hover_eval");
                    if(ui_hovering(watch_sig)) UI_Tooltip RD_Font(RD_FontSlot_Main) UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                    {
                      ui_labelf("Add the hovered expression to an opened watch view.");
                    }
                    if(ui_clicked(watch_sig))
                    {
                      rd_cmd(RD_CmdKind_ToggleWatchExpression, .string = expr);
                    }
                  }
                  if(ws->hover_eval_file_path.size != 0 || ws->hover_eval_vaddr != 0)
                    UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_CornerRadius10(corner_radius)
                    UI_CornerRadius11(corner_radius)
                  {
                    UI_Signal pin_sig = rd_icon_buttonf(RD_IconKind_Pin, 0, "###pin_hover_eval");
                    if(ui_hovering(pin_sig)) UI_Tooltip RD_Font(RD_FontSlot_Main) UI_FontSize(rd_font_size_from_slot(RD_FontSlot_Main))
                      UI_CornerRadius00(0)
                      UI_CornerRadius01(0)
                      UI_CornerRadius10(0)
                      UI_CornerRadius11(0)
                    {
                      ui_labelf("Pin the hovered expression to this code location.");
                    }
                    if(ui_clicked(pin_sig))
                    {
                      rd_cmd(RD_CmdKind_ToggleWatchPin,
                             .file_path  = ws->hover_eval_file_path,
                             .cursor     = ws->hover_eval_file_pt,
                             .vaddr      = ws->hover_eval_vaddr,
                             .string     = expr);
                    }
                  }
                }
              }
            }
            UI_PrefWidth(ui_px(0, 0)) ui_spacer(ui_px(hover_eval_container_height-row_height, 1.f));
          }
          
          //- rjf: interact
          {
            UI_Signal hover_eval_sig = ui_signal_from_box(hover_eval_box);
            if(ui_mouse_over(hover_eval_sig))
            {
              ws->hover_eval_last_frame_idx = rd_state->frame_index;
            }
            else if(ws->hover_eval_last_frame_idx+2 < rd_state->frame_index)
            {
              rd_request_frame();
            }
            if(ui_pressed(hover_eval_sig))
            {
              ws->hover_eval_focused = 1;
            }
          }
        }
        
        di_scope_close(scope);
        scratch_end(scratch);
      }
    }
    
    ////////////////////////////
    //- rjf: panel non-leaf UI (drag boundaries, drag/drop sites)
    //
    B32 is_changing_panel_boundaries = 0;
    ProfScope("non-leaf panel UI")
      for(RD_Panel *panel = ws->root_panel;
          !rd_panel_is_nil(panel);
          panel = rd_panel_rec_depth_first_pre(panel).next)
    {
      //////////////////////////
      //- rjf: continue on leaf panels
      //
      if(rd_panel_is_nil(panel->first))
      {
        continue;
      }
      
      //////////////////////////
      //- rjf: grab info
      //
      Axis2 split_axis = panel->split_axis;
      Rng2F32 panel_rect = rd_target_rect_from_panel(content_rect, ws->root_panel, panel);
      
      //////////////////////////
      //- rjf: boundary tab-drag/drop sites
      //
      {
        RD_View *drag_view = rd_view_from_handle(rd_state->drag_drop_regs->view);
        if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && !rd_view_is_nil(drag_view))
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
          if(panel == ws->root_panel) UI_CornerRadius(corner_radius)
          {
            Vec2F32 panel_rect_center = center_2f32(panel_rect);
            Axis2 axis = axis2_flip(ws->root_panel->split_axis);
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
                UI_Rect(site_rect)
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
                Rng2F32 future_split_rect = site_rect;
                future_split_rect.p0.v[axis] -= drop_site_major_dim_px;
                future_split_rect.p1.v[axis] += drop_site_major_dim_px;
                future_split_rect.p0.v[axis2_flip(axis)] = panel_rect.p0.v[axis2_flip(axis)];
                future_split_rect.p1.v[axis2_flip(axis)] = panel_rect.p1.v[axis2_flip(axis)];
                UI_Rect(future_split_rect) RD_Palette(RD_PaletteCode_DropSiteOverlay)
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
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
                  RD_Panel *split_panel = panel;
                  rd_cmd(RD_CmdKind_SplitPanel,
                         .dst_panel  = rd_handle_from_panel(split_panel),
                         .panel      = rd_state->drag_drop_regs->panel,
                         .view       = rd_state->drag_drop_regs->view,
                         .dir2       = dir);
                }
              }
            }
          }
          
          //- rjf: iterate all children, build boundary drop sites
          Axis2 split_axis = panel->split_axis;
          UI_CornerRadius(corner_radius) for(RD_Panel *child = panel->first;; child = child->next)
          {
            // rjf: form rect
            Rng2F32 child_rect = rd_target_rect_from_panel_child(panel_rect, panel, child);
            Vec2F32 child_rect_center = center_2f32(child_rect);
            UI_Key key = ui_key_from_stringf(ui_key_zero(), "drop_boundary_%p_%p", panel, child);
            Rng2F32 site_rect = r2f32(child_rect_center, child_rect_center);
            site_rect.p0.v[split_axis] = child_rect.p0.v[split_axis] - drop_site_minor_dim_px/2;
            site_rect.p1.v[split_axis] = child_rect.p0.v[split_axis] + drop_site_minor_dim_px/2;
            site_rect.p0.v[axis2_flip(split_axis)] -= drop_site_major_dim_px/2;
            site_rect.p1.v[axis2_flip(split_axis)] += drop_site_major_dim_px/2;
            
            // rjf: build
            UI_Box *site_box = &ui_nil_box;
            {
              UI_Rect(site_rect)
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
              Rng2F32 future_split_rect = site_rect;
              future_split_rect.p0.v[split_axis] -= drop_site_major_dim_px;
              future_split_rect.p1.v[split_axis] += drop_site_major_dim_px;
              future_split_rect.p0.v[axis2_flip(split_axis)] = child_rect.p0.v[axis2_flip(split_axis)];
              future_split_rect.p1.v[axis2_flip(split_axis)] = child_rect.p1.v[axis2_flip(split_axis)];
              UI_Rect(future_split_rect) RD_Palette(RD_PaletteCode_DropSiteOverlay)
              {
                ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
              }
            }
            
            // rjf: drop
            if(ui_key_match(site_box->key, ui_drop_hot_key()) && rd_drag_drop())
            {
              Dir2 dir = (panel->split_axis == Axis2_X ? Dir2_Left : Dir2_Up);
              RD_Panel *split_panel = child;
              if(rd_panel_is_nil(split_panel))
              {
                split_panel = panel->last;
                dir = (panel->split_axis == Axis2_X ? Dir2_Right : Dir2_Down);
              }
              rd_cmd(RD_CmdKind_SplitPanel,
                     .dst_panel  = rd_handle_from_panel(split_panel),
                     .panel      = rd_state->drag_drop_regs->panel,
                     .view       = rd_state->drag_drop_regs->view,
                     .dir2       = dir);
            }
            
            // rjf: exit on opl child
            if(rd_panel_is_nil(child))
            {
              break;
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: do UI for drag boundaries between all children
      //
      for(RD_Panel *child = panel->first; !rd_panel_is_nil(child) && !rd_panel_is_nil(child->next); child = child->next)
      {
        RD_Panel *min_child = child;
        RD_Panel *max_child = min_child->next;
        Rng2F32 min_child_rect = rd_target_rect_from_panel_child(panel_rect, panel, min_child);
        Rng2F32 max_child_rect = rd_target_rect_from_panel_child(panel_rect, panel, max_child);
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
          UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_%p", min_child, max_child);
          UI_Signal sig = ui_signal_from_box(box);
          if(ui_double_clicked(sig))
          {
            ui_kill_action();
            F32 sum_pct = min_child->pct_of_parent + max_child->pct_of_parent;
            min_child->pct_of_parent = 0.5f * sum_pct;
            max_child->pct_of_parent = 0.5f * sum_pct;
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
            is_changing_panel_boundaries = 1;
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: animate panels
    //
    {
      F32 rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-50.f * rd_state->frame_dt)) : 1.f;
      Vec2F32 content_rect_dim = dim_2f32(content_rect);
      for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
      {
        Rng2F32 target_rect_px = rd_target_rect_from_panel(content_rect, ws->root_panel, panel);
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
    
    ////////////////////////////
    //- rjf: panel leaf UI
    //
    ProfScope("leaf panel UI")
      for(RD_Panel *panel = ws->root_panel;
          !rd_panel_is_nil(panel);
          panel = rd_panel_rec_depth_first_pre(panel).next)
    {
      if(!rd_panel_is_nil(panel->first)) {continue;}
      B32 panel_is_focused = (window_is_focused &&
                              !ws->menu_bar_focused &&
                              (!query_is_open || !ws->query_view_selected) &&
                              !ui_any_ctx_menu_is_open() &&
                              !ws->hover_eval_focused &&
                              ws->focused_panel == panel);
      UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
      {
        //////////////////////////
        //- rjf: calculate UI rectangles
        //
        Vec2F32 content_rect_dim = dim_2f32(content_rect);
        Rng2F32 panel_rect_pct = panel->animated_rect_pct;
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
        {
          RD_View *tab = rd_selected_tab_from_panel(panel);
          if(tab->is_filtering_t > 0.01f)
          {
            filter_rect.x0 = content_rect.x0;
            filter_rect.y0 = content_rect.y0;
            filter_rect.x1 = content_rect.x1;
            content_rect.y0 += filter_bar_height*tab->is_filtering_t;
            filter_rect.y1 = content_rect.y0;
          }
        }
        
        //////////////////////////
        //- rjf: build combined split+movetab drag/drop sites
        //
        {
          RD_View *view = rd_view_from_handle(rd_state->drag_drop_regs->view);
          if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && !rd_view_is_nil(view) && contains_2f32(panel_rect, ui_mouse()))
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
                ui_key_from_stringf(ui_key_zero(), "drop_split_center_%p", panel),
                Dir2_Invalid,
                r2f32(sub_2f32(panel_center, drop_site_half_dim),
                      add_2f32(panel_center, drop_site_half_dim))
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_up_%p", panel),
                Dir2_Up,
                r2f32p(panel_center.x-drop_site_half_dim.x,
                       panel_center.y-drop_site_half_dim.y - drop_site_half_dim.y*2,
                       panel_center.x+drop_site_half_dim.x,
                       panel_center.y+drop_site_half_dim.y - drop_site_half_dim.y*2),
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_down_%p", panel),
                Dir2_Down,
                r2f32p(panel_center.x-drop_site_half_dim.x,
                       panel_center.y-drop_site_half_dim.y + drop_site_half_dim.y*2,
                       panel_center.x+drop_site_half_dim.x,
                       panel_center.y+drop_site_half_dim.y + drop_site_half_dim.y*2),
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_left_%p", panel),
                Dir2_Left,
                r2f32p(panel_center.x-drop_site_half_dim.x - drop_site_half_dim.x*2,
                       panel_center.y-drop_site_half_dim.y,
                       panel_center.x+drop_site_half_dim.x - drop_site_half_dim.x*2,
                       panel_center.y+drop_site_half_dim.y),
              },
              {
                ui_key_from_stringf(ui_key_zero(), "drop_split_right_%p", panel),
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
                UI_Rect(rect)
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
                         .dst_panel = rd_handle_from_panel(panel),
                         .panel = rd_state->drag_drop_regs->panel,
                         .view = rd_state->drag_drop_regs->view,
                         .dir2 = dir);
                }
                else
                {
                  rd_cmd(RD_CmdKind_MoveTab,
                         .dst_panel = rd_handle_from_panel(panel),
                         .panel = rd_state->drag_drop_regs->panel,
                         .view = rd_state->drag_drop_regs->view,
                         .prev_view = rd_handle_from_view(panel->last_tab_view));
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
                Rng2F32 future_split_rect = panel_rect;
                if(sites[idx].split_dir != Dir2_Invalid)
                {
                  Vec2F32 panel_center = center_2f32(panel_rect);
                  future_split_rect.v[side_flip(split_side)].v[split_axis] = panel_center.v[split_axis];
                }
                UI_Rect(future_split_rect) RD_Palette(RD_PaletteCode_DropSiteOverlay)
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
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
            UI_Key key = ui_key_from_stringf(ui_key_zero(), "catchall_drop_site_%p", panel);
            UI_Box *catchall_drop_site = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
            ui_signal_from_box(catchall_drop_site);
            catchall_drop_site_hovered = ui_key_match(key, ui_drop_hot_key());
          }
        }
        
        //////////////////////////
        //- rjf: build filtering box
        //
        {
          RD_View *view = rd_selected_tab_from_panel(panel);
          UI_Focus(UI_FocusKind_On)
          {
            if(view->is_filtering && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept))
            {
              rd_cmd(RD_CmdKind_ApplyFilter, .view = rd_handle_from_view(view));
            }
            if(view->is_filtering || view->is_filtering_t > 0.01f)
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
                RD_Font(view->spec->flags & RD_ViewRuleInfoFlag_FilterIsCode ? RD_FontSlot_Code : RD_FontSlot_Main)
                  UI_Focus(view->is_filtering ? UI_FocusKind_On : UI_FocusKind_Off)
                  UI_TextPadding(ui_top_font_size()*0.5f)
                {
                  UI_Signal sig = rd_line_edit(RD_LineEditFlag_CodeContents*!!(view->spec->flags & RD_ViewRuleInfoFlag_FilterIsCode),
                                               0,
                                               0,
                                               &view->query_cursor,
                                               &view->query_mark,
                                               view->query_buffer,
                                               sizeof(view->query_buffer),
                                               &view->query_string_size,
                                               0,
                                               str8(view->query_buffer, view->query_string_size),
                                               str8_lit("###filter_text_input"));
                  if(ui_pressed(sig))
                  {
                    rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(panel));
                  }
                }
              }
            }
          }
        }
        
        //////////////////////////
        //- rjf: panel not selected? -> darken
        //
        if(panel != ws->focused_panel)
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
          UI_Key panel_key = rd_ui_key_from_panel(panel);
          panel_box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                            UI_BoxFlag_Clip|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DisableFocusOverlay|
                                            ((ws->focused_panel != panel)*UI_BoxFlag_DisableFocusBorder),
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
          rd_push_regs();
          {
            RD_View *view = rd_selected_tab_from_panel(panel);
            String8 view_file_path = rd_file_path_from_eval_string(rd_frame_arena(), str8(view->query_buffer, view->query_string_size));
            rd_regs()->panel = rd_handle_from_panel(panel);
            rd_regs()->view  = rd_handle_from_view(view);
            if(view_file_path.size != 0)
            {
              rd_regs()->file_path = view_file_path;
            }
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
          UI_Parent(view_container_box) if(rd_view_is_nil(rd_selected_tab_from_panel(panel)))
          {
            RD_VIEW_RULE_UI_FUNCTION_NAME(empty)(str8_zero(), &md_nil_node, content_rect);
          }
          
          //- rjf: build tab view
          UI_Parent(view_container_box) if(!rd_view_is_nil(rd_selected_tab_from_panel(panel)))
          {
            RD_View *view = rd_selected_tab_from_panel(panel);
            RD_ViewRuleUIFunctionType *view_ui = view->spec->ui;
            view_ui(str8(view->query_buffer, view->query_string_size), view->params_roots[view->params_read_gen%ArrayCount(view->params_roots)], content_rect);
          }
          
          //- rjf: pop interaction registers; commit if this is the selected view
          RD_Regs *view_regs = rd_pop_regs();
          if(ws->focused_panel == panel)
          {
            MemoryCopyStruct(rd_regs(), view_regs);
          }
        }
        
        ////////////////////////
        //- rjf: loading? -> fill loading overlay container
        //
        {
          RD_View *view = rd_selected_tab_from_panel(panel);
          if(view->loading_t > 0.01f) UI_Parent(loading_overlay_container)
          {
            rd_loading_overlay(panel_rect, view->loading_t, view->loading_progress_v, view->loading_progress_v_target);
          }
        }
        
        //////////////////////////
        //- rjf: take events to automatically start/end filtering, if applicable
        //
        UI_Focus(UI_FocusKind_On)
        {
          RD_View *view = rd_selected_tab_from_panel(panel);
          if(ui_is_focus_active() && view->spec->flags & RD_ViewRuleInfoFlag_TypingAutomaticallyFilters && !view->is_filtering)
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
          if(view->spec->flags & RD_ViewRuleInfoFlag_CanFilter && (view->query_string_size != 0 || view->is_filtering) && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Cancel))
          {
            rd_cmd(RD_CmdKind_ClearFilter, .view = rd_handle_from_view(view));
          }
        }
        
        //////////////////////////
        //- rjf: consume panel fallthrough interaction events
        //
        UI_Signal panel_sig = ui_signal_from_box(panel_box);
        if(ui_pressed(panel_sig))
        {
          rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(panel));
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
            RD_View *prev_view;
          };
          
          // rjf: prep output data
          RD_View *next_selected_tab_view = rd_selected_tab_from_panel(panel);
          UI_Box *tab_bar_box = &ui_nil_box;
          U64 drop_site_count = panel->tab_view_count+1;
          DropSite *drop_sites = push_array(scratch.arena, DropSite, drop_site_count);
          F32 drop_site_max_p = 0;
          U64 view_idx = 0;
          
          // rjf: build
          UI_CornerRadius(0)
          {
            UI_Rect(tab_bar_rect) tab_bar_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_ViewClampX|UI_BoxFlag_ViewScrollX|UI_BoxFlag_Clickable, "tab_bar_%p", panel);
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
            UI_PrefWidth(ui_em(18.f, 0.5f))
              UI_CornerRadius00(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius01(panel->tab_side == Side_Min ? 0 : corner_radius)
              UI_CornerRadius10(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius11(panel->tab_side == Side_Min ? 0 : corner_radius)
              for(RD_View *view = panel->first_tab_view;; view = view->order_next, view_idx += 1)
            {
              temp_end(scratch);
              if(rd_view_is_project_filtered(view)) { continue; }
              
              // rjf: if before this tab is the prev-view of the current tab drag,
              // draw empty space
              if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && catchall_drop_site_hovered)
              {
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
              }
              
              // rjf: end on nil view
              if(rd_view_is_nil(view))
              {
                break;
              }
              
              // rjf: gather info for this tab
              B32 view_is_selected = (view == rd_selected_tab_from_panel(panel));
              RD_IconKind icon_kind = rd_icon_kind_from_view(view);
              DR_FancyStringList title_fstrs = rd_title_fstrs_from_view(scratch.arena, view, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
              
              // rjf: begin vertical region for this tab
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *tab_column_box = ui_build_box_from_stringf(!is_changing_panel_boundaries*UI_BoxFlag_AnimatePosX, "tab_column_%p", view);
              
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
                                                            "tab_%p", view);
                
                // rjf: build tab contents
                UI_Parent(tab_box)
                {
                  UI_WidthFill UI_Row
                  {
                    ui_spacer(ui_em(0.5f, 1.f));
                    if(icon_kind != RD_IconKind_Null)
                    {
                      UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                        RD_Font(RD_FontSlot_Icons)
                        UI_TextAlignment(UI_TextAlign_Center)
                        UI_PrefWidth(ui_em(1.75f, 1.f))
                        ui_label(rd_icon_kind_text_table[icon_kind]);
                    }
                    UI_PrefWidth(ui_text_dim(10, 0))
                    {
                      UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                      ui_box_equip_display_fancy_strings(name_box, &title_fstrs);
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
                    UI_Signal sig = ui_buttonf("%S###close_view_%p", rd_icon_kind_text_table[RD_IconKind_X], view);
                    if(ui_clicked(sig) || ui_middle_clicked(sig))
                    {
                      rd_cmd(RD_CmdKind_CloseTab, .panel = rd_handle_from_panel(panel), .view = rd_handle_from_view(view));
                    }
                  }
                }
                
                // rjf: consume events for tab clicking
                {
                  UI_Signal sig = ui_signal_from_box(tab_box);
                  if(ui_pressed(sig))
                  {
                    next_selected_tab_view = view;
                    rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(panel));
                  }
                  else if(ui_dragging(sig) && !rd_drag_is_active() && length_2f32(ui_drag_delta()) > 10.f)
                  {
                    RD_RegsScope(.panel = rd_handle_from_panel(panel),
                                 .view = rd_handle_from_view(view))
                    {
                      rd_drag_begin(RD_RegSlot_View);
                    }
                  }
                  else if(ui_right_clicked(sig))
                  {
                    RD_RegsScope(.panel = rd_handle_from_panel(panel),
                                 .view = rd_handle_from_view(view))
                    {
                      rd_open_ctx_menu(sig.box->key, v2f32(0, sig.box->rect.y1 - sig.box->rect.y0), RD_RegSlot_View);
                    }
                  }
                  else if(ui_middle_clicked(sig))
                  {
                    rd_cmd(RD_CmdKind_CloseTab, .panel = rd_handle_from_panel(panel), .view = rd_handle_from_view(view));
                  }
                }
              }
              
              // rjf: space for next tab
              {
                ui_spacer(ui_em(0.3f, 1.f));
              }
              
              // rjf: store off drop-site
              drop_sites[view_idx].p = tab_column_box->rect.x0 - tab_spacing/2;
              drop_sites[view_idx].prev_view = view->order_prev;
              drop_site_max_p = Max(tab_column_box->rect.x1, drop_site_max_p);
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
                                                                panel);
                UI_Signal sig = ui_signal_from_box(add_new_box);
                if(ui_clicked(sig))
                {
                  rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(panel));
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
            drop_sites[drop_site_count-1].prev_view = panel->last_tab_view;
          }
          
          // rjf: more precise drop-sites on tab bar
          {
            Vec2F32 mouse = ui_mouse();
            RD_View *view = rd_view_from_handle(rd_state->drag_drop_regs->view);
            if(rd_drag_is_active() && rd_state->drag_drop_regs_slot == RD_RegSlot_View && window_is_focused && contains_2f32(panel_rect, mouse) && !rd_view_is_nil(view))
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
                rd_last_drag_drop_prev_tab = rd_handle_from_view(active_drop_site->prev_view);
              }
              else
              {
                rd_last_drag_drop_prev_tab = rd_handle_zero();
              }
              
              // rjf: vis
              RD_Panel *drag_panel = rd_panel_from_handle(rd_state->drag_drop_regs->panel);
              if(!rd_view_is_nil(view) && active_drop_site != 0) 
              {
                RD_Palette(RD_PaletteCode_DropSiteOverlay) UI_Rect(tab_bar_rect)
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
              }
              
              // rjf: drop
              if(catchall_drop_site_hovered && (active_drop_site != 0 && rd_drag_drop()))
              {
                RD_View *view = rd_view_from_handle(rd_state->drag_drop_regs->view);
                RD_Panel *src_panel = rd_panel_from_handle(rd_state->drag_drop_regs->panel);
                if(!rd_panel_is_nil(panel) && !rd_view_is_nil(view))
                {
                  rd_cmd(RD_CmdKind_MoveTab,
                         .panel = rd_handle_from_panel(src_panel),
                         .dst_panel = rd_handle_from_panel(panel),
                         .view = rd_handle_from_view(view),
                         .prev_view = rd_handle_from_view(active_drop_site->prev_view));
                }
              }
            }
          }
          
          // rjf: apply tab change
          {
            panel->selected_tab_view = rd_handle_from_view(next_selected_tab_view);
          }
          
          scratch_end(scratch);
        }
        
        //////////////////////////
        //- rjf: less granular panel-wide drop-site
        //
        if(catchall_drop_site_hovered)
        {
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
    //- rjf: animate views
    //
    {
      Temp scratch = scratch_begin(0, 0);
      typedef struct Task Task;
      struct Task
      {
        Task *next;
        RD_Panel *panel;
        RD_View *list_first;
        RD_View *transient_owner;
      };
      Task start_task = {0, &rd_nil_panel, ws->query_view_stack_top};
      Task *first_task = &start_task;
      Task *last_task = first_task;
      F32 rate = 1 - pow_f32(2, (-10.f * rd_state->frame_dt));
      F32 fast_rate = 1 - pow_f32(2, (-40.f * rd_state->frame_dt));
      for(RD_Panel *panel = ws->root_panel;
          !rd_panel_is_nil(panel);
          panel = rd_panel_rec_depth_first_pre(panel).next)
      {
        Task *t = push_array(scratch.arena, Task, 1);
        SLLQueuePush(first_task, last_task, t);
        t->panel = panel;
        t->list_first = panel->first_tab_view;
      }
      for(Task *t = first_task; t != 0; t = t->next)
      {
        RD_View *list_first = t->list_first;
        for(RD_View *view = list_first; !rd_view_is_nil(view); view = view->order_next)
        {
          if(!rd_view_is_nil(view->first_transient))
          {
            Task *task = push_array(scratch.arena, Task, 1);
            SLLQueuePush(first_task, last_task, task);
            task->panel = t->panel;
            task->list_first = view->first_transient;
            task->transient_owner = view;
          }
          if(window_is_focused)
          {
            if(abs_f32(view->loading_t_target - view->loading_t) > 0.01f ||
               abs_f32(view->scroll_pos.x.off) > 0.01f ||
               abs_f32(view->scroll_pos.y.off) > 0.01f ||
               abs_f32(view->is_filtering_t - (F32)!!view->is_filtering))
            {
              rd_request_frame();
            }
            if(view->loading_t_target != 0 && (view == rd_selected_tab_from_panel(t->panel) ||
                                               t->transient_owner == rd_selected_tab_from_panel(t->panel)))
            {
              rd_request_frame();
            }
          }
          view->loading_t += (view->loading_t_target - view->loading_t) * rate;
          view->is_filtering_t += ((F32)!!view->is_filtering - view->is_filtering_t) * fast_rate;
          view->scroll_pos.x.off -= view->scroll_pos.x.off * (rd_setting_val_from_code(RD_SettingCode_ScrollingAnimations).s32 ? fast_rate : 1.f);
          view->scroll_pos.y.off -= view->scroll_pos.y.off * (rd_setting_val_from_code(RD_SettingCode_ScrollingAnimations).s32 ? fast_rate : 1.f);
          if(abs_f32(view->scroll_pos.x.off) < 0.01f)
          {
            view->scroll_pos.x.off = 0;
          }
          if(abs_f32(view->scroll_pos.y.off) < 0.01f)
          {
            view->scroll_pos.y.off = 0;
          }
          if(abs_f32(view->is_filtering_t - (F32)!!view->is_filtering) < 0.01f)
          {
            view->is_filtering_t = (F32)!!view->is_filtering;
          }
          if(view == rd_selected_tab_from_panel(t->panel) ||
             t->transient_owner == rd_selected_tab_from_panel(t->panel))
          {
            view->loading_t_target = 0;
          }
        }
      }
      scratch_end(scratch);
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
          rd_cmd(RD_CmdKind_IncUIFontScale, .window = rd_handle_from_window(ws));
        }
        else if(evt->delta_2f32.y > 0)
        {
          rd_cmd(RD_CmdKind_DecUIFontScale, .window = rd_handle_from_window(ws));
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
  //- rjf: attach autocomp box to root, or hide if it has not been renewed
  //
  if(!ui_box_is_nil(autocomp_box) && ws->autocomp_last_frame_idx+1 >= rd_state->frame_index+1)
  {
    UI_Box *autocomp_root_box = ui_box_from_key(ws->autocomp_root_key);
    if(!ui_box_is_nil(autocomp_root_box))
    {
      Vec2F32 size = autocomp_box->fixed_size;
      autocomp_box->fixed_position = v2f32(autocomp_root_box->rect.x0, autocomp_root_box->rect.y1);
      autocomp_box->rect = r2f32(autocomp_box->fixed_position, add_2f32(autocomp_box->fixed_position, size));
      for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis + 1))
      {
        ui_calc_sizes_standalone__in_place_rec(autocomp_box, axis);
        ui_calc_sizes_upwards_dependent__in_place_rec(autocomp_box, axis);
        ui_calc_sizes_downwards_dependent__in_place_rec(autocomp_box, axis);
        ui_layout_enforce_constraints__in_place_rec(autocomp_box, axis);
        ui_layout_position__in_place_rec(autocomp_box, axis);
      }
    }
  }
  else if(!ui_box_is_nil(autocomp_box) && ws->autocomp_last_frame_idx+1 < rd_state->frame_index+1)
  {
    UI_Box *autocomp_root_box = ui_box_from_key(ws->autocomp_root_key);
    if(!ui_box_is_nil(autocomp_root_box))
    {
      Vec2F32 size = autocomp_box->fixed_size;
      Rng2F32 window_rect = os_client_rect_from_window(ws->os);
      autocomp_box->fixed_position = v2f32(window_rect.x1, window_rect.y1);
      autocomp_box->rect = r2f32(autocomp_box->fixed_position, add_2f32(autocomp_box->fixed_position, size));
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
      if(box->squish != 0)
      {
        Vec2F32 box_dim = dim_2f32(box->rect);
        Mat3x3F32 box2origin_xform = make_translate_3x3f32(v2f32(-box->rect.x0 - box_dim.x/8, -box->rect.y0));
        Mat3x3F32 scale_xform = make_scale_3x3f32(v2f32(1-box->squish, 1-box->squish));
        Mat3x3F32 origin2box_xform = make_translate_3x3f32(v2f32(box->rect.x0 + box_dim.x/8, box->rect.y0));
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
      if(box->flags & UI_BoxFlag_DrawBackgroundBlur && rd_setting_val_from_code(RD_SettingCode_BackgroundBlur).s32)
      {
        R_PassParams_Blur *params = dr_blur(box->rect, box->blur_size*(1-box->transparency), 0);
        MemoryCopyArray(params->corner_radii, box->corner_radii);
      }
      
      // rjf: draw background
      if(box->flags & UI_BoxFlag_DrawBackground)
      {
        // rjf: main rectangle
        {
          R_Rect2DInst *inst = dr_rect(pad_2f32(box->rect, 1), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
          MemoryCopyArray(inst->corner_radii, box->corner_radii);
        }
        
        // rjf: hot effect extension
        if(box->flags & UI_BoxFlag_DrawHotEffects)
        {
          F32 effective_active_t = box->active_t;
          if(!(box->flags & UI_BoxFlag_DrawActiveEffects))
          {
            effective_active_t = 0;
          }
          F32 t = box->hot_t*(1-effective_active_t);
          
          // rjf: brighten
          {
            R_Rect2DInst *inst = dr_rect(box->rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Hover);
            color.w *= t*0.2f;
            inst->colors[Corner_00] = color;
            inst->colors[Corner_01] = color;
            inst->colors[Corner_10] = color;
            inst->colors[Corner_11] = color;
            inst->colors[Corner_10].w *= t;
            inst->colors[Corner_11].w *= t;
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
          Vec4F32 shadow_color = rd_rgba_from_theme_color(RD_ThemeColor_Hover);
          shadow_color.x *= 0.3f;
          shadow_color.y *= 0.3f;
          shadow_color.z *= 0.3f;
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
          max_x = (box->rect.x1-text_position.x);
          ellipses_run = fnt_push_run_from_string(scratch.arena, box->font, box->font_size, 0, box->tab_size, 0, str8_lit("..."));
        }
        dr_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
        if(box->flags & UI_BoxFlag_HasFuzzyMatchRanges)
        {
          Vec4F32 match_color = rd_rgba_from_theme_color(RD_ThemeColor_HighlightOverlay);
          dr_truncated_fancy_run_fuzzy_matches(text_position, &box->display_string_runs, max_x, &box->fuzzy_match_ranges, match_color);
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
            R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1.f), b->palette->colors[UI_ColorCode_Border], 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
            
            // rjf: hover effect
            if(b->flags & UI_BoxFlag_DrawHotEffects)
            {
              Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Hover);
              color.w *= b->hot_t;
              R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1), color, 0, 1.f, 1.f);
              MemoryCopyArray(inst->corner_radii, b->corner_radii);
            }
          }
          
          // rjf: debug border rendering
          if(0)
          {
            R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1), v4f32(1, 0, 1, 0.25f), 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw sides
          {
            Rng2F32 r = b->rect;
            F32 half_thickness = 1.f;
            F32 softness = 0.5f;
            if(b->flags & UI_BoxFlag_DrawSideTop)
            {
              dr_rect(r2f32p(r.x0, r.y0-half_thickness, r.x1, r.y0+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideBottom)
            {
              dr_rect(r2f32p(r.x0, r.y1-half_thickness, r.x1, r.y1+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideLeft)
            {
              dr_rect(r2f32p(r.x0-half_thickness, r.y0, r.x0+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideRight)
            {
              dr_rect(r2f32p(r.x1-half_thickness, r.y0, r.x1+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
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
            Vec4F32 color = rd_rgba_from_theme_color(RD_ThemeColor_Focus);
            color.w *= b->focus_active_t;
            R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
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
          if(b->squish != 0)
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
    
    //- rjf: scratch debug mouse drawing
    if(DEV_scratch_mouse_draw)
    {
#if 1
      Vec2F32 p = add_2f32(os_mouse_from_window(ws->os), v2f32(30, 0));
      dr_rect(os_client_rect_from_window(ws->os), v4f32(0, 0, 0, 0.9f), 0, 0, 0);
      FNT_Run trailer_run = fnt_push_run_from_string(scratch.arena, rd_font_from_slot(RD_FontSlot_Main), 16.f, 0, 0, 0, str8_lit("..."));
      DR_FancyStringList strs = {0};
      DR_FancyString str = {rd_font_from_slot(RD_FontSlot_Main), str8_lit("Shift + F5"), v4f32(1, 1, 1, 1), 72.f, 0.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str);
      DR_FancyRunList runs = dr_fancy_run_list_from_fancy_string_list(scratch.arena, 0, FNT_RasterFlag_Smooth, &strs);
      dr_truncated_fancy_run_list(p, &runs, 1000000.f, trailer_run);
      dr_rect(r2f32(p, add_2f32(p, runs.dim)), v4f32(1, 0, 0, 0.5f), 0, 1, 0);
      dr_rect(r2f32(sub_2f32(p, v2f32(4, 4)), add_2f32(p, v2f32(4, 4))), v4f32(1, 0, 1, 1), 0, 0, 0);
#else
      Vec2F32 p = add_2f32(os_mouse_from_window(ws->os), v2f32(30, 0));
      dr_rect(os_client_rect_from_window(ws->os), v4f32(0, 0, 0, 0.4f), 0, 0, 0);
      DR_FancyStringList strs = {0};
      DR_FancyString str1 = {rd_font_from_slot(RD_FontSlot_Main), str8_lit("T"), v4f32(1, 1, 1, 1), 16.f, 4.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str1);
      DR_FancyString str2 = {rd_font_from_slot(RD_FontSlot_Main), str8_lit("his is a test of some "), v4f32(1, 0.5f, 0.5f, 1), 14.f, 0.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str2);
      DR_FancyString str3 = {rd_font_from_slot(RD_FontSlot_Code), str8_lit("very fancy text!"), v4f32(1, 0.8f, 0.4f, 1), 18.f, 4.f, 4.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str3);
      DR_FancyRunList runs = dr_fancy_run_list_from_fancy_string_list(scratch.arena, 0, 0, &strs);
      FNT_Run trailer_run = fnt_push_run_from_string(scratch.arena, rd_font_from_slot(RD_FontSlot_Main), 16.f, 0, 0, 0, str8_lit("..."));
      F32 limit = 500.f + sin_f32(rd_state->time_in_seconds/10.f)*200.f;
      dr_truncated_fancy_run_list(p, &runs, limit, trailer_run);
      dr_rect(r2f32p(p.x+limit, 0, p.x+limit+2.f, 1000), v4f32(1, 0, 0, 1), 0, 0, 0);
      rd_request_frame();
#endif
    }
    
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: increment per-window frame counter
  //
  ws->frames_alive += 1;
  
  ProfEnd();
}

#if COMPILER_MSVC && !BUILD_DEBUG
#pragma optimize("", on)
#endif

////////////////////////////////
//~ rjf: Eval Visualization

typedef struct RD_EntityExpandAccel RD_EntityExpandAccel;
struct RD_EntityExpandAccel
{
  RD_EntityArray entities;
};

typedef struct RD_CtrlEntityExpandAccel RD_CtrlEntityExpandAccel;
struct RD_CtrlEntityExpandAccel
{
  CTRL_EntityArray entities;
};

//- rjf: meta entities

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(watches)               { return rd_ev_view_rule_expr_expand_info__meta_entities(arena, view, filter, expr, params, RD_EntityKind_Watch); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(watches)         { return rd_ev_view_rule_expr_expand_range_info__meta_entities(arena, view, filter, expr, params, idx_range, user_data, RD_EntityKind_Watch, 0); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(watches)        { return rd_ev_view_rule_expr_id_from_num__meta_entities(num, user_data, RD_EntityKind_Watch, 0); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(watches)        { return rd_ev_view_rule_expr_num_from_id__meta_entities(id,  user_data, RD_EntityKind_Watch, 0); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(targets)               { return rd_ev_view_rule_expr_expand_info__meta_entities(arena, view, filter, expr, params, RD_EntityKind_Target); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(targets)         { return rd_ev_view_rule_expr_expand_range_info__meta_entities(arena, view, filter, expr, params, idx_range, user_data, RD_EntityKind_Target, 1); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(targets)        { return rd_ev_view_rule_expr_id_from_num__meta_entities(num, user_data, RD_EntityKind_Target, 1); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(targets)        { return rd_ev_view_rule_expr_num_from_id__meta_entities(id,  user_data, RD_EntityKind_Target, 1); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(breakpoints)           { return rd_ev_view_rule_expr_expand_info__meta_entities(arena, view, filter, expr, params, RD_EntityKind_Breakpoint); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(breakpoints)     { return rd_ev_view_rule_expr_expand_range_info__meta_entities(arena, view, filter, expr, params, idx_range, user_data, RD_EntityKind_Breakpoint, 1); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(breakpoints)    { return rd_ev_view_rule_expr_id_from_num__meta_entities(num, user_data, RD_EntityKind_Breakpoint, 1); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(breakpoints)    { return rd_ev_view_rule_expr_num_from_id__meta_entities(id,  user_data, RD_EntityKind_Breakpoint, 1); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(watch_pins)            { return rd_ev_view_rule_expr_expand_info__meta_entities(arena, view, filter, expr, params, RD_EntityKind_WatchPin); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(watch_pins)      { return rd_ev_view_rule_expr_expand_range_info__meta_entities(arena, view, filter, expr, params, idx_range, user_data, RD_EntityKind_WatchPin, 1); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(watch_pins)     { return rd_ev_view_rule_expr_id_from_num__meta_entities(num, user_data, RD_EntityKind_WatchPin, 1); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(watch_pins)     { return rd_ev_view_rule_expr_num_from_id__meta_entities(id,  user_data, RD_EntityKind_WatchPin, 1); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(file_path_maps)        { return rd_ev_view_rule_expr_expand_info__meta_entities(arena, view, filter, expr, params, RD_EntityKind_FilePathMap); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(file_path_maps)  { return rd_ev_view_rule_expr_expand_range_info__meta_entities(arena, view, filter, expr, params, idx_range, user_data, RD_EntityKind_FilePathMap, 1); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(file_path_maps) { return rd_ev_view_rule_expr_id_from_num__meta_entities(num, user_data, RD_EntityKind_FilePathMap, 1); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(file_path_maps) { return rd_ev_view_rule_expr_num_from_id__meta_entities(id,  user_data, RD_EntityKind_FilePathMap, 1); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(auto_view_rules)       { return rd_ev_view_rule_expr_expand_info__meta_entities(arena, view, filter, expr, params, RD_EntityKind_AutoViewRule); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(auto_view_rules) { return rd_ev_view_rule_expr_expand_range_info__meta_entities(arena, view, filter, expr, params, idx_range, user_data, RD_EntityKind_AutoViewRule, 1); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(auto_view_rules){ return rd_ev_view_rule_expr_id_from_num__meta_entities(num, user_data, RD_EntityKind_AutoViewRule, 1); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(auto_view_rules){ return rd_ev_view_rule_expr_num_from_id__meta_entities(id,  user_data, RD_EntityKind_AutoViewRule, 1); }

//- rjf: control entity groups

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(machines)          { return rd_ev_view_rule_expr_expand_info__meta_ctrl_entities(arena, view, str8_zero(), expr, params, CTRL_EntityKind_Machine); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(machines)    { return rd_ev_view_rule_expr_expand_range_info__meta_ctrl_entities(arena, view, str8_zero(), expr, params, idx_range, user_data, CTRL_EntityKind_Machine); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(machines)   { return rd_ev_view_rule_expr_id_from_num__meta_ctrl_entities(num, user_data, CTRL_EntityKind_Machine); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(machines)   { return rd_ev_view_rule_expr_num_from_id__meta_ctrl_entities(id, user_data, CTRL_EntityKind_Machine); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(processes)         { return rd_ev_view_rule_expr_expand_info__meta_ctrl_entities(arena, view, filter, expr, params, CTRL_EntityKind_Process); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(processes)   { return rd_ev_view_rule_expr_expand_range_info__meta_ctrl_entities(arena, view, filter, expr, params, idx_range, user_data, CTRL_EntityKind_Process); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(processes)  { return rd_ev_view_rule_expr_id_from_num__meta_ctrl_entities(num, user_data, CTRL_EntityKind_Process); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(processes)  { return rd_ev_view_rule_expr_num_from_id__meta_ctrl_entities(id, user_data, CTRL_EntityKind_Process); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(threads)           { return rd_ev_view_rule_expr_expand_info__meta_ctrl_entities(arena, view, filter, expr, params, CTRL_EntityKind_Thread); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(threads)     { return rd_ev_view_rule_expr_expand_range_info__meta_ctrl_entities(arena, view, filter, expr, params, idx_range, user_data, CTRL_EntityKind_Thread); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(threads)    { return rd_ev_view_rule_expr_id_from_num__meta_ctrl_entities(num, user_data, CTRL_EntityKind_Thread); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(threads)    { return rd_ev_view_rule_expr_num_from_id__meta_ctrl_entities(id, user_data, CTRL_EntityKind_Thread); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(modules)           { return rd_ev_view_rule_expr_expand_info__meta_ctrl_entities(arena, view, filter, expr, params, CTRL_EntityKind_Module); }
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(modules)     { return rd_ev_view_rule_expr_expand_range_info__meta_ctrl_entities(arena, view, filter, expr, params, idx_range, user_data, CTRL_EntityKind_Module); }
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(modules)    { return rd_ev_view_rule_expr_id_from_num__meta_ctrl_entities(num, user_data, CTRL_EntityKind_Module); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(modules)    { return rd_ev_view_rule_expr_num_from_id__meta_ctrl_entities(id, user_data, CTRL_EntityKind_Module); }

//- rjf: control entity hierarchies

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(scheduler_machine)
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
        info.row_strings[row_expr_idx] = process->string;
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

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(scheduler_process)
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
        expr->type_key = e_type_key_cons_base(type(CTRL_ThreadMetaEval));;
        info.row_strings[row_expr_idx] = push_str8f(arena, "thread:%S", thread->string);
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

//- rjf: locals

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(locals)
{
  Temp scratch = scratch_begin(&arena, 1);
  E_String2NumMapNodeArray nodes = e_string2num_map_node_array_from_map(scratch.arena, e_parse_ctx->locals_map);
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
  EV_ExpandInfo info = {accel, accel->count};
  scratch_end(scratch);
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(locals)
{
  String8Array *accel = (String8Array *)user_data;
  EV_ExpandRangeInfo result = {0};
  {
    U64 needed_row_count = dim_1u64(idx_range);
    result.row_exprs_count = Min(needed_row_count, accel->count);
    result.row_exprs       = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings     = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members     = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      result.row_exprs[row_expr_idx] = e_parse_expr_from_text(arena, accel->v[idx_range.min + row_expr_idx]);
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  return result;
}

//- rjf: registers

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(registers)
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
  EV_ExpandInfo info = {accel, accel->count};
  scratch_end(scratch);
  return info;
}

EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(registers)
{
  String8Array *accel = (String8Array *)user_data;
  EV_ExpandRangeInfo result = {0};
  {
    U64 needed_row_count = dim_1u64(idx_range);
    result.row_exprs_count = Min(needed_row_count, accel->count);
    result.row_exprs       = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings     = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members     = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      String8 string = push_str8f(arena, "reg:%S", accel->v[idx_range.min + row_expr_idx]);
      result.row_exprs[row_expr_idx] = e_parse_expr_from_text(arena, string);
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  return result;
}

//- rjf: debug info tables

EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(globals)               {return rd_ev_view_rule_expr_expand_info__debug_info_tables(arena, view, filter, expr, params, RDI_SectionKind_GlobalVariables);}
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(globals)         {return rd_ev_view_rule_expr_expand_range_info__debug_info_tables(arena, view, filter, expr, params, idx_range, user_data, RDI_SectionKind_GlobalVariables);}
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(globals)        {return rd_ev_view_rule_expr_id_from_num__debug_info_tables(num, user_data, RDI_SectionKind_GlobalVariables); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(globals)        {return rd_ev_view_rule_expr_num_from_id__debug_info_tables(id,  user_data, RDI_SectionKind_GlobalVariables); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(thread_locals)         {return rd_ev_view_rule_expr_expand_info__debug_info_tables(arena, view, filter, expr, params, RDI_SectionKind_ThreadVariables);}
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(thread_locals)   {return rd_ev_view_rule_expr_expand_range_info__debug_info_tables(arena, view, filter, expr, params, idx_range, user_data, RDI_SectionKind_ThreadVariables);}
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(thread_locals)  {return rd_ev_view_rule_expr_id_from_num__debug_info_tables(num, user_data, RDI_SectionKind_ThreadVariables); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(thread_locals)  {return rd_ev_view_rule_expr_num_from_id__debug_info_tables(id,  user_data, RDI_SectionKind_ThreadVariables); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(types)                 {return rd_ev_view_rule_expr_expand_info__debug_info_tables(arena, view, filter, expr, params, RDI_SectionKind_UDTs);}
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(types)           {return rd_ev_view_rule_expr_expand_range_info__debug_info_tables(arena, view, filter, expr, params, idx_range, user_data, RDI_SectionKind_UDTs);}
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(types)          {return rd_ev_view_rule_expr_id_from_num__debug_info_tables(num, user_data, RDI_SectionKind_UDTs); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(types)          {return rd_ev_view_rule_expr_num_from_id__debug_info_tables(id,  user_data, RDI_SectionKind_UDTs); }
EV_VIEW_RULE_EXPR_EXPAND_INFO_FUNCTION_DEF(procedures)            {return rd_ev_view_rule_expr_expand_info__debug_info_tables(arena, view, filter, expr, params, RDI_SectionKind_Procedures);}
EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_DEF(procedures)      {return rd_ev_view_rule_expr_expand_range_info__debug_info_tables(arena, view, filter, expr, params, idx_range, user_data, RDI_SectionKind_Procedures);}
EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_DEF(procedures)     {return rd_ev_view_rule_expr_id_from_num__debug_info_tables(num, user_data, RDI_SectionKind_Procedures); }
EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_DEF(procedures)     {return rd_ev_view_rule_expr_num_from_id__debug_info_tables(id,  user_data, RDI_SectionKind_Procedures); }

internal EV_ExpandInfo
rd_ev_view_rule_expr_expand_info__meta_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, RD_EntityKind kind)
{
  RD_EntityExpandAccel *accel = push_array(arena, RD_EntityExpandAccel, 1);
  Temp scratch = scratch_begin(&arena, 1);
  {
    RD_EntityList entities = rd_query_cached_entity_list_with_kind(kind);
    RD_EntityList entities_filtered = {0};
    for(RD_EntityNode *n = entities.first; n != 0; n = n->next)
    {
      RD_Entity *entity = n->entity;
      String8 entity_expr_string = entity->string;
      B32 is_collection = 0;
      for EachElement(idx, rd_collection_name_table)
      {
        if(str8_match(entity_expr_string, rd_collection_name_table[idx], 0))
        {
          is_collection = 1;
          break;
        }
      }
      B32 is_in_filter = 1;
      if(!is_collection && filter.size != 0)
      {
        RD_Entity *loc  = rd_entity_child_from_kind(entity, RD_EntityKind_Location);
        RD_Entity *exe  = rd_entity_child_from_kind(entity, RD_EntityKind_Executable);
        RD_Entity *args = rd_entity_child_from_kind(entity, RD_EntityKind_Arguments);
        FuzzyMatchRangeList expr_matches = fuzzy_match_find(scratch.arena, filter, entity_expr_string);
        FuzzyMatchRangeList loc_matches  = fuzzy_match_find(scratch.arena, filter, loc->string);
        FuzzyMatchRangeList exe_matches  = fuzzy_match_find(scratch.arena, filter, exe->string);
        FuzzyMatchRangeList args_matches = fuzzy_match_find(scratch.arena, filter, args->string);
        is_in_filter = (expr_matches.count == expr_matches.needle_part_count ||
                        loc_matches.count  == loc_matches.needle_part_count ||
                        exe_matches.count  == exe_matches.needle_part_count ||
                        args_matches.count == args_matches.needle_part_count);
      }
      if(is_collection || is_in_filter)
      {
        rd_entity_list_push(scratch.arena, &entities_filtered, entity);
      }
    }
    accel->entities = rd_entity_array_from_list(arena, &entities_filtered);
  }
  scratch_end(scratch);
  EV_ExpandInfo info = {accel, accel->entities.count + 1};
  info.add_new_row = 1;
  return info;
}

internal EV_ExpandRangeInfo
rd_ev_view_rule_expr_expand_range_info__meta_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data, RD_EntityKind kind, B32 add_new_at_top)
{
  RD_EntityExpandAccel *accel = (RD_EntityExpandAccel *)user_data;
  EV_ExpandRangeInfo result = {0};
  {
    U64 entities_base_idx = (U64)!!add_new_at_top;
    U64 needed_row_count = dim_1u64(idx_range);
    result.row_exprs_count = Min(needed_row_count, accel->entities.count+1);
    result.row_exprs       = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings     = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members     = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      U64 child_idx = idx_range.min + row_expr_idx;
      RD_Entity *entity = &d_nil_entity;
      if(entities_base_idx <= child_idx && child_idx < entities_base_idx+accel->entities.count)
      {
        entity = accel->entities.v[child_idx-entities_base_idx];
      }
      if(!rd_entity_is_nil(entity))
      {
        String8 entity_expr_string = (kind == RD_EntityKind_Watch ? entity->string : push_str8f(arena, "entity:$%I64u", entity->id));
        if(kind == RD_EntityKind_Watch)
        {
          result.row_strings[row_expr_idx] = entity_expr_string;
        }
        result.row_exprs[row_expr_idx] = e_parse_expr_from_text(arena, entity_expr_string);
        result.row_view_rules[row_expr_idx] = rd_entity_child_from_kind(entity, RD_EntityKind_ViewRule)->string;
      }
      else
      {
        result.row_exprs[row_expr_idx] = &e_expr_nil;
      }
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  return result;
}

internal U64
rd_ev_view_rule_expr_id_from_num__meta_entities(U64 num, void *user_data, RD_EntityKind kind, B32 add_new_at_top)
{
  U64 id = 0;
  RD_EntityExpandAccel *accel = (RD_EntityExpandAccel *)user_data;
  U64 entities_base_idx = (U64)!!add_new_at_top;
  if(entities_base_idx+1 <= num && num < entities_base_idx+accel->entities.count+1)
  {
    id = accel->entities.v[num-(entities_base_idx+1)]->id;
  }
  else
  {
    id = max_U64;
  }
  return id;
}

internal U64
rd_ev_view_rule_expr_num_from_id__meta_entities(U64 id, void *user_data, RD_EntityKind kind, B32 add_new_at_top)
{
  RD_EntityExpandAccel *accel = (RD_EntityExpandAccel *)user_data;
  U64 num = 0;
  U64 entities_base_idx = (U64)!!add_new_at_top;
  if(id == max_U64)
  {
    num = add_new_at_top ? 1 : (entities_base_idx + accel->entities.count + 1);
  }
  else for(U64 idx = 0; idx < accel->entities.count; idx += 1)
  {
    if(accel->entities.v[idx]->id == id)
    {
      num = entities_base_idx+idx+1;
      break;
    }
  }
  return num;
}

internal EV_ExpandInfo
rd_ev_view_rule_expr_expand_info__meta_ctrl_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, CTRL_EntityKind kind)
{
  RD_CtrlEntityExpandAccel *accel = push_array(arena, RD_CtrlEntityExpandAccel, 1);
  Temp scratch = scratch_begin(&arena, 1);
  {
    CTRL_EntityList entities = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, kind);
    CTRL_EntityList entities_filtered = {0};
    for(CTRL_EntityNode *n = entities.first; n != 0; n = n->next)
    {
      CTRL_Entity *entity = n->v;
      B32 is_in_filter = 1;
      if(filter.size != 0)
      {
        is_in_filter = 0;
        FuzzyMatchRangeList matches = fuzzy_match_find(scratch.arena, filter, entity->string);
        if(matches.count == matches.needle_part_count)
        {
          is_in_filter = 1;
        }
      }
      if(is_in_filter)
      {
        ctrl_entity_list_push(scratch.arena, &entities_filtered, entity);
      }
    }
    accel->entities = ctrl_entity_array_from_list(arena, &entities_filtered);
  }
  scratch_end(scratch);
  EV_ExpandInfo info = {accel, accel->entities.count};
  // TODO(rjf): hack
  if(kind == CTRL_EntityKind_Machine)
  {
    info.rows_default_expanded = 1;
  }
  return info;
}

internal EV_ExpandRangeInfo
rd_ev_view_rule_expr_expand_range_info__meta_ctrl_entities(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data, CTRL_EntityKind kind)
{
  RD_CtrlEntityExpandAccel *accel = (RD_CtrlEntityExpandAccel *)user_data;
  EV_ExpandRangeInfo result = {0};
  {
    U64 needed_row_count = dim_1u64(idx_range);
    result.row_exprs_count = Min(needed_row_count, accel->entities.count);
    result.row_exprs       = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings     = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members     = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      CTRL_Entity *entity = accel->entities.v[idx_range.min + row_expr_idx];
      String8 entity_expr_string = push_str8f(arena, "ctrl_entity:$_%I64x_%I64x", entity->handle.machine_id, entity->handle.dmn_handle.u64[0]);
      result.row_exprs[row_expr_idx] = e_parse_expr_from_text(arena, entity_expr_string);
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  return result;
}

internal U64
rd_ev_view_rule_expr_id_from_num__meta_ctrl_entities(U64 num, void *user_data, CTRL_EntityKind kind)
{
  RD_CtrlEntityExpandAccel *accel = (RD_CtrlEntityExpandAccel *)user_data;
  U64 id = 0;
  if(1 <= num && num <= accel->entities.count)
  {
    id = d_hash_from_string(str8_struct(&accel->entities.v[num-1]->handle));
  }
  return id;
}

internal U64
rd_ev_view_rule_expr_num_from_id__meta_ctrl_entities(U64 id, void *user_data, CTRL_EntityKind kind)
{
  RD_CtrlEntityExpandAccel *accel = (RD_CtrlEntityExpandAccel *)user_data;
  U64 num = 0;
  if(id != 0)
  {
    for EachIndex(idx, accel->entities.count)
    {
      U64 idx_id = d_hash_from_string(str8_struct(&accel->entities.v[idx]->handle));
      if(idx_id == id)
      {
        num = idx+1;
        break;
      }
    }
  }
  return num;
}

typedef struct RD_DebugInfoTableExpandAccel RD_DebugInfoTableExpandAccel;
struct RD_DebugInfoTableExpandAccel
{
  U64 rdis_count;
  RDI_Parsed **rdis;
  DI_SearchItemArray items;
};

internal EV_ExpandInfo
rd_ev_view_rule_expr_expand_info__debug_info_tables(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, RDI_SectionKind section)
{
  RD_DebugInfoTableExpandAccel *accel = push_array(arena, RD_DebugInfoTableExpandAccel, 1);
  if(section != RDI_SectionKind_NULL)
  {
    Temp scratch = scratch_begin(&arena, 1);
    U64 endt_us = os_now_microseconds()+200;
    
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
    U128 fuzzy_search_key = {(U64)view, (U64)section};
    B32 items_stale = 0;
    DI_SearchParams params = {section, dbgi_keys};
    accel->rdis_count = rdis_count;
    accel->rdis = rdis;
    accel->items = di_search_items_from_key_params_query(rd_state->frame_di_scope, fuzzy_search_key, &params, filter, endt_us, &items_stale);
    if(items_stale)
    {
      rd_request_frame();
    }
    
    scratch_end(scratch);
  }
  EV_ExpandInfo info = {accel, accel->items.count};
  return info;
}

internal EV_ExpandRangeInfo
rd_ev_view_rule_expr_expand_range_info__debug_info_tables(Arena *arena, EV_View *view, String8 filter, E_Expr *expr, MD_Node *params, Rng1U64 idx_range, void *user_data, RDI_SectionKind section)
{
  RD_DebugInfoTableExpandAccel *accel = (RD_DebugInfoTableExpandAccel *)user_data;
  EV_ExpandRangeInfo result = {0};
  {
    U64 needed_row_count = dim_1u64(idx_range);
    result.row_exprs_count = Min(needed_row_count, accel->items.count);
    result.row_exprs       = push_array(arena, E_Expr *, result.row_exprs_count);
    result.row_strings     = push_array(arena, String8, result.row_exprs_count);
    result.row_view_rules  = push_array(arena, String8, result.row_exprs_count);
    result.row_members     = push_array(arena, E_Member *, result.row_exprs_count);
    for EachIndex(row_expr_idx, result.row_exprs_count)
    {
      // rjf: unpack row info
      DI_SearchItem *item = &accel->items.v[idx_range.min + row_expr_idx];
      RDI_Parsed *rdi = accel->rdis[item->dbgi_idx];
      E_Module *module = &e_parse_ctx->modules[item->dbgi_idx];
      
      // rjf: build expr
      E_Expr *item_expr = &e_expr_nil;
      {
        U64 element_idx = item->idx;
        switch(section)
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
            e_oplist_push_op(arena, &oplist, RDI_EvalOp_ConstU64, e_value_u64(module->vaddr_range.min + voff));
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
      
      // rjf: fill
      result.row_exprs[row_expr_idx] = item_expr;
      result.row_members[row_expr_idx] = &e_member_nil;
    }
  }
  return result;
}

internal U64
rd_ev_view_rule_expr_id_from_num__debug_info_tables(U64 num, void *user_data, RDI_SectionKind section)
{
  RD_DebugInfoTableExpandAccel *accel = (RD_DebugInfoTableExpandAccel *)user_data;
  U64 id = 0;
  if(0 < num && num <= accel->items.count)
  {
    id = accel->items.v[num-1].idx+1;
  }
  return id;
}

internal U64
rd_ev_view_rule_expr_num_from_id__debug_info_tables(U64 id, void *user_data, RDI_SectionKind section)
{
  RD_DebugInfoTableExpandAccel *accel = (RD_DebugInfoTableExpandAccel *)user_data;
  U64 num = di_search_item_num_from_array_element_idx__linear_search(&accel->items, id-1);
  return num;
}

internal EV_View *
rd_ev_view_from_key(U64 key)
{
  U64 slot_idx = key % rd_state->eval_viz_view_cache_slots_count;
  RD_EvalVizViewCacheNode *node = 0;
  RD_EvalVizViewCacheSlot *slot = &rd_state->eval_viz_view_cache_slots[slot_idx];
  for(RD_EvalVizViewCacheNode *n = slot->first; n != 0; n = n->next)
  {
    if(n->key == key)
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = rd_state->eval_viz_view_cache_node_free;
    if(node)
    {
      SLLStackPop(rd_state->eval_viz_view_cache_node_free);
    }
    else
    {
      node = push_array(rd_state->arena, RD_EvalVizViewCacheNode, 1);
    }
    DLLPushBack(slot->first, slot->last, node);
    node->key = key;
    node->v = ev_view_alloc();
  }
  return node->v;
}

internal F32
rd_append_value_strings_from_eval(Arena *arena, EV_StringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, S32 depth, E_Eval eval, E_Member *member, EV_ViewRuleList *view_rules, String8List *out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  F32 space_taken = 0;
  
  //- rjf: unpack view rules
  U32 radix = default_radix;
  U32 min_digits = 0;
  B32 no_addr = 0;
  B32 has_array = 0;
  for(EV_ViewRuleNode *n = view_rules->first; n != 0; n = n->next)
  {
    if(0){}
    else if(str8_match(n->v.root->string, str8_lit("dec"), 0)) {radix = 10;}
    else if(str8_match(n->v.root->string, str8_lit("hex"), 0)) {radix = 16;}
    else if(str8_match(n->v.root->string, str8_lit("bin"), 0)) {radix = 2; }
    else if(str8_match(n->v.root->string, str8_lit("oct"), 0)) {radix = 8; }
    else if(str8_match(n->v.root->string, str8_lit("no_addr"), 0)) {no_addr = 1;}
    else if(str8_match(n->v.root->string, str8_lit("array"), 0)) {has_array = 1;}
    else if(str8_match(n->v.root->string, str8_lit("digits"), 0))
    {
      String8 expr = md_string_from_children(scratch.arena, n->v.root);
      E_Eval eval = e_eval_from_string(scratch.arena, expr);
      E_Eval value_eval = e_value_eval_from_eval(eval);
      min_digits = (U32)value_eval.value.u64;
    }
  }
  if(eval.space.kind == RD_EvalSpaceKind_MetaEntity ||
     eval.space.kind == RD_EvalSpaceKind_MetaCtrlEntity)
  {
    E_TypeKind kind = e_type_kind_from_key(eval.type_key);
    if(kind != E_TypeKind_Ptr)
    {
      no_addr = 1;
    }
    else
    {
      E_Type *type = e_type_from_key(scratch.arena, eval.type_key);
      if(!(type->flags & E_TypeFlag_External))
      {
        no_addr = 1;
      }
    }
  }
  
  //- rjf: member evaluations -> display member info
  if(eval.mode == E_Mode_Null && !e_type_key_match(e_type_key_zero(), eval.type_key) && member != &e_member_nil)
  {
    U64 member_byte_size = e_type_byte_size_from_key(eval.type_key);
    String8 offset_string = str8_from_u64(arena, member->off, radix, 0, 0);
    String8 size_string = str8_from_u64(arena, member_byte_size, radix, 0, 0);
    str8_list_pushf(arena, out, "member (%S offset, %S byte%s)", offset_string, size_string, member_byte_size == 1 ? "" : "s");
  }
  
  //- rjf: type evaluations -> display type basic information
  else if(eval.mode == E_Mode_Null && !e_type_key_match(e_type_key_zero(), eval.type_key) && eval.expr->kind != E_ExprKind_MemberAccess)
  {
    String8 basic_type_kind_string = e_kind_basic_string_table[e_type_kind_from_key(eval.type_key)];
    U64 byte_size = e_type_byte_size_from_key(eval.type_key);
    String8 size_string = str8_from_u64(arena, byte_size, radix, 0, 0);
    str8_list_pushf(arena, out, "%S (%S byte%s)", basic_type_kind_string, size_string, byte_size == 1 ? "" : "s");
  }
  
  //- rjf: value/offset evaluations
  else if(max_size > 0) switch(e_type_kind_from_key(e_type_unwrap(eval.type_key)))
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
      E_TypeKind type_kind = e_type_kind_from_key(e_type_unwrap(eval.type_key));
      E_TypeKey direct_type_key = e_type_unwrap(e_type_ptee_from_key(e_type_unwrap(eval.type_key)));
      E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
      
      // rjf: unpack info about pointer destination
      E_Eval value_eval = e_value_eval_from_eval(eval);
      B32 ptee_has_content = (direct_type_kind != E_TypeKind_Null && direct_type_kind != E_TypeKind_Void);
      B32 ptee_has_string  = ((E_TypeKind_Char8 <= direct_type_kind && direct_type_kind <= E_TypeKind_UChar32) ||
                              direct_type_kind == E_TypeKind_S8 ||
                              direct_type_kind == E_TypeKind_U8);
      CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_regs()->thread);
      CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
      String8 symbol_name = d_symbol_name_from_process_vaddr(arena, process, value_eval.value.u64, 1);
      
      // rjf: special case: push strings for textual string content
      B32 did_content = 0;
      B32 did_string = 0;
      if(!did_content && ptee_has_string && !has_array)
      {
        did_content = 1;
        did_string = 1;
        U64 string_memory_addr = value_eval.value.u64;
        U64 element_size = e_type_byte_size_from_key(direct_type_key);
        U64 string_buffer_size = 256;
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
        String8 string_escaped = ev_escaped_from_raw_string(arena, string);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string_escaped).x;
        space_taken += 2*fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
        str8_list_push(arena, out, str8_lit("\""));
        str8_list_push(arena, out, string_escaped);
        str8_list_push(arena, out, str8_lit("\""));
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
          E_Expr *deref_expr = e_expr_ref_deref(scratch.arena, eval.expr);
          E_Eval deref_eval = e_eval_from_expr(scratch.arena, deref_expr);
          space_taken += rd_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, deref_eval, 0, view_rules, out);
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
      E_Type *eval_type = e_type_from_key(scratch.arena, e_type_unwrap(eval.type_key));
      E_TypeKey direct_type_key = e_type_unwrap(eval_type->direct_type_key);
      E_TypeKind direct_type_kind = e_type_kind_from_key(direct_type_key);
      U64 array_count = eval_type->count;
      
      // rjf: get pointed-at type
      B32 array_is_string = ((E_TypeKind_Char8 <= direct_type_kind && direct_type_kind <= E_TypeKind_UChar32) ||
                             direct_type_kind == E_TypeKind_S8 ||
                             direct_type_kind == E_TypeKind_U8);
      
      // rjf: special case: push strings for textual string content
      B32 did_content = 0;
      if(!did_content && array_is_string && !has_array && (member == 0 || member->kind != E_MemberKind_Padding))
      {
        U64 element_size = e_type_byte_size_from_key(direct_type_key);
        did_content = 1;
        U64 string_buffer_size = 256;
        U8 *string_buffer = push_array(arena, U8, string_buffer_size);
        switch(eval.mode)
        {
          default:{}break;
          case E_Mode_Offset:
          {
            U64 string_memory_addr = eval.value.u64;
            for(U64 try_size = string_buffer_size; try_size >= 16; try_size /= 2)
            {
              B32 read_good = e_space_read(eval.space, string_buffer, r1u64(string_memory_addr, string_memory_addr+try_size));
              if(read_good)
              {
                break;
              }
            }
          }break;
          case E_Mode_Value:
          {
            MemoryCopy(string_buffer, &eval.value.u512[0], Min(string_buffer_size, sizeof(eval.value)));
          }break;
        }
        string_buffer[string_buffer_size-1] = 0;
        String8 string = {0};
        switch(element_size)
        {
          default:{string = str8_cstring((char *)string_buffer);}break;
          case 2: {string = str8_from_16(arena, str16_cstring((U16 *)string_buffer));}break;
          case 4: {string = str8_from_32(arena, str32_cstring((U32 *)string_buffer));}break;
        }
        String8 string_escaped = ev_escaped_from_raw_string(arena, string);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string_escaped).x;
        space_taken += 2*fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
        str8_list_push(arena, out, str8_lit("\""));
        str8_list_push(arena, out, string_escaped);
        str8_list_push(arena, out, str8_lit("\""));
      }
      
      // rjf: descend in all other cases
      if(!did_content && (flags & EV_StringFlag_ReadOnlyDisplayRules))
      {
        did_content = 1;
        
        // rjf: [
        {
          String8 bracket = str8_lit("[");
          str8_list_push(arena, out, bracket);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, bracket).x;
        }
        
        // rjf: build contents
        if(depth < 4)
        {
          for(U64 idx = 0; idx < array_count && max_size > space_taken; idx += 1)
          {
            E_Expr *element_expr = e_expr_ref_array_index(scratch.arena, eval.expr, idx);
            E_Eval element_eval = e_eval_from_expr(scratch.arena, element_expr);
            space_taken += rd_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, element_eval, 0, view_rules, out);
            if(idx+1 < array_count)
            {
              String8 comma = str8_lit(", ");
              space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
              str8_list_push(arena, out, comma);
            }
            if(space_taken > max_size && idx+1 < array_count)
            {
              String8 ellipses = str8_lit("...");
              space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
              str8_list_push(arena, out, ellipses);
            }
          }
        }
        else
        {
          String8 ellipses = str8_lit("...");
          str8_list_push(arena, out, ellipses);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
        }
        
        // rjf: ]
        {
          String8 bracket = str8_lit("]");
          str8_list_push(arena, out, bracket);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, bracket).x;
        }
      }
    }break;
    
    //- rjf: structs
    case E_TypeKind_Struct:
    case E_TypeKind_Union:
    case E_TypeKind_Class:
    case E_TypeKind_IncompleteStruct:
    case E_TypeKind_IncompleteUnion:
    case E_TypeKind_IncompleteClass:
    {
      // rjf: open brace
      {
        String8 brace = str8_lit("{");
        str8_list_push(arena, out, brace);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, brace).x;
      }
      
      // rjf: content
      if(depth < 4)
      {
        E_MemberArray data_members = e_type_data_members_from_key__cached(e_type_unwrap(eval.type_key));
        for(U64 member_idx = 0; member_idx < data_members.count && max_size > space_taken; member_idx += 1)
        {
          E_Member *mem = &data_members.v[member_idx];
          E_Expr *dot_expr = e_expr_ref_member_access(scratch.arena, eval.expr, mem->name);
          E_Expr *dot_expr_resolved = ev_resolved_from_expr(scratch.arena, dot_expr, view_rules);
          E_Eval dot_eval = e_eval_from_expr(scratch.arena, dot_expr_resolved);
          space_taken += rd_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, dot_eval, 0, view_rules, out);
          if(member_idx+1 < data_members.count)
          {
            String8 comma = str8_lit(", ");
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
            str8_list_push(arena, out, comma);
          }
          if(space_taken > max_size && member_idx+1 < data_members.count)
          {
            String8 ellipses = str8_lit("...");
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
            str8_list_push(arena, out, ellipses);
          }
        }
      }
      else
      {
        String8 ellipses = str8_lit("...");
        str8_list_push(arena, out, ellipses);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
      }
      
      // rjf: close brace
      {
        String8 brace = str8_lit("}");
        str8_list_push(arena, out, brace);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, brace).x;
      }
    }break;
    
    //- rjf: collections
    case E_TypeKind_Collection:
    {
      String8 placeholder = str8_lit("{...}");
      str8_list_push(arena, out, placeholder);
      space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, placeholder).x;
    }break;
  }
  
  scratch_end(scratch);
  ProfEnd();
  return space_taken;
}

internal String8
rd_value_string_from_eval(Arena *arena, EV_StringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, E_Eval eval, E_Member *member, EV_ViewRuleList *view_rules)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  rd_append_value_strings_from_eval(scratch.arena, flags, default_radix, font, font_size, max_size, 0, eval, member, view_rules, &strs);
  String8 result = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Hover Eval

internal void
rd_set_hover_eval(Vec2F32 pos, String8 file_path, TxtPt pt, U64 vaddr, String8 string)
{
  RD_Window *window = rd_window_from_handle(rd_regs()->window);
  if(window->hover_eval_last_frame_idx+1 < rd_state->frame_index &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Left), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Middle), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Right), ui_key_zero()))
  {
    B32 is_new_string = !str8_match(window->hover_eval_string, string, 0);
    if(is_new_string)
    {
      window->hover_eval_first_frame_idx = window->hover_eval_last_frame_idx = rd_state->frame_index;
      arena_clear(window->hover_eval_arena);
      window->hover_eval_string = push_str8_copy(window->hover_eval_arena, string);
      window->hover_eval_file_path = push_str8_copy(window->hover_eval_arena, file_path);
      window->hover_eval_file_pt = pt;
      window->hover_eval_vaddr = vaddr;
      window->hover_eval_focused = 0;
    }
    window->hover_eval_spawn_pos = pos;
    window->hover_eval_last_frame_idx = rd_state->frame_index;
  }
}

////////////////////////////////
//~ rjf: Auto-Complete Lister

internal void
rd_autocomp_lister_item_chunk_list_push(Arena *arena, RD_AutoCompListerItemChunkList *list, U64 cap, RD_AutoCompListerItem *item)
{
  RD_AutoCompListerItemChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = push_array(arena, RD_AutoCompListerItemChunkNode, 1);
    SLLQueuePush(list->first, list->last, n);
    n->cap = cap;
    n->v = push_array_no_zero(arena, RD_AutoCompListerItem, n->cap);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&n->v[n->count], item);
  n->count += 1;
  list->total_count += 1;
}

internal RD_AutoCompListerItemArray
rd_autocomp_lister_item_array_from_chunk_list(Arena *arena, RD_AutoCompListerItemChunkList *list)
{
  RD_AutoCompListerItemArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, RD_AutoCompListerItem, array.count);
  U64 idx = 0;
  for(RD_AutoCompListerItemChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(RD_AutoCompListerItem)*n->count);
    idx += n->count;
  }
  return array;
}

internal int
rd_autocomp_lister_item_qsort_compare(RD_AutoCompListerItem *a, RD_AutoCompListerItem *b)
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
  else if(a->matches.count > b->matches.count)
  {
    result = -1;
  }
  else if(a->matches.count < b->matches.count)
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
rd_autocomp_lister_item_array_sort__in_place(RD_AutoCompListerItemArray *array)
{
  quick_sort(array->v, array->count, sizeof(array->v[0]), rd_autocomp_lister_item_qsort_compare);
}

internal String8
rd_autocomp_query_word_from_input_string_off(String8 input, U64 cursor_off)
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
rd_autocomp_query_path_from_input_string_off(String8 input, U64 cursor_off)
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

internal RD_AutoCompListerParams
rd_view_rule_autocomp_lister_params_from_input_cursor(Arena *arena, String8 string, U64 cursor_off)
{
  RD_AutoCompListerParams params = {0};
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
            else if(str8_match(child->string, str8_lit("expr"),           StringMatchFlag_CaseInsensitive)) {params.flags |= RD_AutoCompListerFlag_Locals;}
            else if(str8_match(child->string, str8_lit("member"),         StringMatchFlag_CaseInsensitive)) {params.flags |= RD_AutoCompListerFlag_Members;}
            else if(str8_match(child->string, str8_lit("lang"),           StringMatchFlag_CaseInsensitive)) {params.flags |= RD_AutoCompListerFlag_Languages;}
            else if(str8_match(child->string, str8_lit("arch"),           StringMatchFlag_CaseInsensitive)) {params.flags |= RD_AutoCompListerFlag_Architectures;}
            else if(str8_match(child->string, str8_lit("tex2dformat"),    StringMatchFlag_CaseInsensitive)) {params.flags |= RD_AutoCompListerFlag_Tex2DFormats;}
            else if(child->flags & (MD_NodeFlag_StringSingleQuote|MD_NodeFlag_StringDoubleQuote|MD_NodeFlag_StringTick))
            {
              str8_list_push(arena, &params.strings, child->string);
              params.flags |= RD_AutoCompListerFlag_ViewRuleParams;
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
rd_set_autocomp_lister_query(UI_Key root_key, RD_AutoCompListerParams *params, String8 input, U64 cursor_off)
{
  RD_Window *window = rd_window_from_handle(rd_regs()->window);
  if(cursor_off != window->autocomp_cursor_off)
  {
    window->autocomp_input_dirty = 1;
    window->autocomp_cursor_off = cursor_off;
  }
  if(!ui_key_match(window->autocomp_root_key, root_key))
  {
    window->autocomp_num_visible_rows_t = 0;
    window->autocomp_open_t = 0;
  }
  if(window->autocomp_last_frame_idx+1 < rd_state->frame_index)
  {
    window->autocomp_num_visible_rows_t = 0;
    window->autocomp_open_t = 0;
  }
  window->autocomp_root_key = root_key;
  arena_clear(window->autocomp_lister_params_arena);
  MemoryCopyStruct(&window->autocomp_lister_params, params);
  window->autocomp_lister_params.strings = str8_list_copy(window->autocomp_lister_params_arena, &window->autocomp_lister_params.strings);
  window->autocomp_lister_input_size = Min(input.size, sizeof(window->autocomp_lister_input_buffer));
  MemoryCopy(window->autocomp_lister_input_buffer, input.str, window->autocomp_lister_input_size);
  window->autocomp_last_frame_idx = rd_state->frame_index;
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

//- rjf: keybindings

internal OS_Key
rd_os_key_from_cfg_string(String8 string)
{
  OS_Key result = OS_Key_Null;
  {
    for(OS_Key key = OS_Key_Null; key < OS_Key_COUNT; key = (OS_Key)(key+1))
    {
      if(str8_match(string, os_g_key_cfg_string_table[key], StringMatchFlag_CaseInsensitive))
      {
        result = key;
        break;
      }
    }
  }
  return result;
}

internal void
rd_clear_bindings(void)
{
  arena_clear(rd_state->key_map_arena);
  rd_state->key_map_table_size = 1024;
  rd_state->key_map_table = push_array(rd_state->key_map_arena, RD_KeyMapSlot, rd_state->key_map_table_size);
  rd_state->key_map_total_count = 0;
}

internal RD_BindingList
rd_bindings_from_name(Arena *arena, String8 name)
{
  RD_BindingList result = {0};
  U64 hash = d_hash_from_string(name);
  U64 slot = hash%rd_state->key_map_table_size;
  for(RD_KeyMapNode *n = rd_state->key_map_table[slot].first; n != 0; n = n->hash_next)
  {
    if(str8_match(n->name, name, 0))
    {
      RD_BindingNode *node = push_array(arena, RD_BindingNode, 1);
      node->binding = n->binding;
      SLLQueuePush(result.first, result.last, node);
      result.count += 1;
    }
  }
  return result;
}

internal void
rd_bind_name(String8 name, RD_Binding binding)
{
  if(binding.key != OS_Key_Null)
  {
    U64 hash = d_hash_from_string(name);
    U64 slot = hash%rd_state->key_map_table_size;
    RD_KeyMapNode *existing_node = 0;
    for(RD_KeyMapNode *n = rd_state->key_map_table[slot].first; n != 0; n = n->hash_next)
    {
      if(str8_match(n->name, name, 0) && n->binding.key == binding.key && n->binding.modifiers == binding.modifiers)
      {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0)
    {
      RD_KeyMapNode *n = rd_state->free_key_map_node;
      if(n == 0)
      {
        n = push_array(rd_state->arena, RD_KeyMapNode, 1);
      }
      else
      {
        rd_state->free_key_map_node = rd_state->free_key_map_node->hash_next;
      }
      n->name = push_str8_copy(rd_state->arena, name);
      n->binding = binding;
      DLLPushBack_NP(rd_state->key_map_table[slot].first, rd_state->key_map_table[slot].last, n, hash_next, hash_prev);
      rd_state->key_map_total_count += 1;
    }
  }
}

internal void
rd_unbind_name(String8 name, RD_Binding binding)
{
  U64 hash = d_hash_from_string(name);
  U64 slot = hash%rd_state->key_map_table_size;
  for(RD_KeyMapNode *n = rd_state->key_map_table[slot].first, *next = 0; n != 0; n = next)
  {
    next = n->hash_next;
    if(str8_match(n->name, name, 0) && n->binding.key == binding.key && n->binding.modifiers == binding.modifiers)
    {
      DLLRemove_NP(rd_state->key_map_table[slot].first, rd_state->key_map_table[slot].last, n, hash_next, hash_prev);
      n->hash_next = rd_state->free_key_map_node;
      rd_state->free_key_map_node = n;
      rd_state->key_map_total_count -= 1;
    }
  }
}

internal String8List
rd_cmd_name_list_from_binding(Arena *arena, RD_Binding binding)
{
  String8List result = {0};
  for(U64 idx = 0; idx < rd_state->key_map_table_size; idx += 1)
  {
    for(RD_KeyMapNode *n = rd_state->key_map_table[idx].first; n != 0; n = n->hash_next)
    {
      if(n->binding.key == binding.key && n->binding.modifiers == binding.modifiers)
      {
        str8_list_push(arena, &result, n->name);
      }
    }
  }
  return result;
}

//- rjf: colors

internal Vec4F32
rd_rgba_from_theme_color(RD_ThemeColor color)
{
  return rd_state->cfg_theme.colors[color];
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
      U64 local_num = e_num_from_string(e_parse_ctx->locals_map, string);
      if(local_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeLocal;
      }
    }
    
    // rjf: try to map as member
    if(!mapped && kind == TXT_TokenKind_Identifier)
    {
      U64 member_num = e_num_from_string(e_parse_ctx->member_map, string);
      if(member_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeLocal;
      }
    }
    
    // rjf: try to map as register
    if(!mapped)
    {
      U64 reg_num = e_num_from_string(e_parse_ctx->regs_map, string);
      if(reg_num != 0)
      {
        mapped = 1;
        color = RD_ThemeColor_CodeRegister;
      }
    }
    
    // rjf: try to map as register alias
    if(!mapped)
    {
      U64 alias_num = e_num_from_string(e_parse_ctx->reg_alias_map, string);
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
  RD_Window *window = rd_window_from_handle(rd_regs()->window);
  UI_Palette *result = &window->cfg_palettes[code];
  return result;
}

//- rjf: fonts/sizes

internal FNT_Tag
rd_font_from_slot(RD_FontSlot slot)
{
  FNT_Tag result = rd_state->cfg_font_tags[slot];
  return result;
}

internal F32
rd_font_size_from_slot(RD_FontSlot slot)
{
  F32 result = 0;
  RD_Window *ws = rd_window_from_handle(rd_regs()->window);
  F32 dpi = os_dpi_from_window(ws->os);
  if(dpi != ws->last_dpi)
  {
    F32 old_dpi = ws->last_dpi;
    F32 new_dpi = dpi;
    ws->last_dpi = dpi;
    S32 *pt_sizes[] =
    {
      &ws->setting_vals[RD_SettingCode_MainFontSize].s32,
      &ws->setting_vals[RD_SettingCode_CodeFontSize].s32,
    };
    for(U64 idx = 0; idx < ArrayCount(pt_sizes); idx += 1)
    {
      F32 ratio = pt_sizes[idx][0] / old_dpi;
      F32 new_pt_size = ratio*new_dpi;
      pt_sizes[idx][0] = (S32)new_pt_size;
    }
  }
  switch(slot)
  {
    case RD_FontSlot_Code:
    {
      result = (F32)ws->setting_vals[RD_SettingCode_CodeFontSize].s32;
    }break;
    default:
    case RD_FontSlot_Main:
    case RD_FontSlot_Icons:
    {
      result = (F32)ws->setting_vals[RD_SettingCode_MainFontSize].s32;
    }break;
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
    case RD_FontSlot_Main: {flags = (!!rd_setting_val_from_code(RD_SettingCode_SmoothUIText).s32*FNT_RasterFlag_Smooth)|(!!rd_setting_val_from_code(RD_SettingCode_HintUIText).s32*FNT_RasterFlag_Hinted);}break;
    case RD_FontSlot_Code: {flags = (!!rd_setting_val_from_code(RD_SettingCode_SmoothCodeText).s32*FNT_RasterFlag_Smooth)|(!!rd_setting_val_from_code(RD_SettingCode_HintCodeText).s32*FNT_RasterFlag_Hinted);}break;
  }
  return flags;
}

//- rjf: settings

internal RD_SettingVal
rd_setting_val_from_code(RD_SettingCode code)
{
  RD_Window *window = rd_window_from_handle(rd_regs()->window);
  RD_SettingVal result = {0};
  if(window != 0)
  {
    result = window->setting_vals[code];
  }
  if(result.set == 0)
  {
    for EachEnumVal(RD_CfgSrc, src)
    {
      if(rd_state->cfg_setting_vals[src][code].set)
      {
        result = rd_state->cfg_setting_vals[src][code];
        break;
      }
    }
  }
  return result;
}

//- rjf: config serialization

internal int
rd_qsort_compare__cfg_string_bindings(RD_StringBindingPair *a, RD_StringBindingPair *b)
{
  return strncmp((char *)a->string.str, (char *)b->string.str, Min(a->string.size, b->string.size));
}

internal String8List
rd_cfg_strings_from_gfx(Arena *arena, String8 root_path, RD_CfgSrc source)
{
  ProfBeginFunction();
  local_persist char *spaces = "                                                                                ";
  local_persist char *slashes= "////////////////////////////////////////////////////////////////////////////////";
  String8List strs = {0};
  
  //- rjf: write all entities
  {
    for EachEnumVal(RD_EntityKind, k)
    {
      RD_EntityKindFlags k_flags = rd_entity_kind_flags_table[k];
      if(!(k_flags & RD_EntityKindFlag_IsSerializedToConfig))
      {
        continue;
      }
      B32 first = 1;
      RD_EntityList entities = rd_query_cached_entity_list_with_kind(k);
      for(RD_EntityNode *n = entities.first; n != 0; n = n->next)
      {
        RD_Entity *entity = n->entity;
        if(entity->cfg_src != source)
        {
          continue;
        }
        if(first)
        {
          first = 0;
          String8 title_name = d_entity_kind_name_lower_plural_table[k];
          str8_list_pushf(arena, &strs, "/// %S %.*s\n\n",
                          title_name,
                          (int)Max(0, 79 - (title_name.size + 5)),
                          slashes);
        }
        RD_EntityRec rec = {0};
        S64 depth = 0;
        for(RD_Entity *e = entity; !rd_entity_is_nil(e); e = rec.next)
        {
          //- rjf: get next iteration
          rec = rd_entity_rec_depth_first_pre(e, entity);
          
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
          String8 entity_name_escaped = e->string;
          // TODO(rjf): @hack - hardcoding in the "EntityKind_Location" here - this is because
          // i am assuming an entity *kind* can 'know' about the 'pathness' of a string. this is
          // not the case. post-0.9.12 i need to fix this.
          if(rd_entity_kind_flags_table[e->kind] & RD_EntityKindFlag_NameIsPath &&
             (e->kind != RD_EntityKind_Location || e->flags & RD_EntityFlag_HasTextPoint))
          {
            Temp scratch = scratch_begin(&arena, 1);
            String8 path_normalized = path_normalized_from_string(scratch.arena, e->string);
            entity_name_escaped = path_relative_dst_from_absolute_dst_src(arena, path_normalized, root_path);
            scratch_end(scratch);
          }
          else
          {
            entity_name_escaped = escaped_from_raw_str8(arena, e->string);
          }
          EntityInfoFlags info_flags = 0;
          if(entity_name_escaped.size != 0)         { info_flags |= EntityInfoFlag_HasName; }
          if(!!e->disabled)                         { info_flags |= EntityInfoFlag_HasDisabled; }
          if(e->flags & RD_EntityFlag_HasTextPoint) { info_flags |= EntityInfoFlag_HasTxtPt; }
          if(e->flags & RD_EntityFlag_HasVAddr)     { info_flags |= EntityInfoFlag_HasVAddr; }
          if(e->flags & RD_EntityFlag_HasColor)     { info_flags |= EntityInfoFlag_HasColor; }
          if(!rd_entity_is_nil(e->first))           { info_flags |= EntityInfoFlag_HasChildren; }
          
          //- rjf: write entity info
          B32 opened_brace = 0;
          switch(info_flags)
          {
            //- rjf: default path -> entity has lots of stuff, so write all info generically
            default:
            {
              opened_brace = 1;
              
              // rjf: write entity title
              str8_list_pushf(arena, &strs, "%S:\n{\n", d_entity_kind_name_lower_table[e->kind]);
              
              // rjf: write this entity's info
              if(entity_name_escaped.size != 0)
              {
                str8_list_pushf(arena, &strs, "name: \"%S\"\n", entity_name_escaped);
              }
              if(e->disabled)
              {
                str8_list_pushf(arena, &strs, "disabled: 1\n");
              }
              if(e->flags & RD_EntityFlag_HasColor)
              {
                Vec4F32 hsva = rd_hsva_from_entity(e);
                Vec4F32 rgba = rgba_from_hsva(hsva);
                U32 rgba_hex = u32_from_rgba(rgba);
                str8_list_pushf(arena, &strs, "color: 0x%x\n", rgba_hex);
              }
              if(e->flags & RD_EntityFlag_HasTextPoint)
              {
                str8_list_pushf(arena, &strs, "line: %I64d\n", e->text_point.line);
              }
              if(e->flags & RD_EntityFlag_HasVAddr)
              {
                str8_list_pushf(arena, &strs, "vaddr: (0x%I64x)\n", e->vaddr);
              }
            }break;
            
            //- rjf: single-line fast-paths
            case EntityInfoFlag_HasName:
            {str8_list_pushf(arena, &strs, "%S: \"%S\"\n", d_entity_kind_name_lower_table[e->kind], entity_name_escaped);}break;
            case EntityInfoFlag_HasName|EntityInfoFlag_HasTxtPt:
            {str8_list_pushf(arena, &strs, "%S: (\"%S\":%I64d)\n", d_entity_kind_name_lower_table[e->kind], entity_name_escaped, e->text_point.line);}break;
            case EntityInfoFlag_HasVAddr:
            {str8_list_pushf(arena, &strs, "%S: (0x%I64x)\n", d_entity_kind_name_lower_table[e->kind], e->vaddr);}break;
            
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
          if(rd_entity_is_nil(rec.next) && (rec.pop_count != 0 || n->next == 0))
          {
            str8_list_pushf(arena, &strs, "\n");
          }
        }
      }
    }
  }
  
  //- rjf: write exception code filters
  if(source == RD_CfgSrc_Project)
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
      B32 value = !!(rd_state->ctrl_exception_code_filters[k/64] & (1ull<<(k%64)));
      str8_list_pushf(arena, &strs, "  %S: %i\n", name, value);
    }
    str8_list_push(arena, &strs, str8_lit("}\n\n"));
  }
  
  //- rjf: serialize windows
  {
    B32 first = 1;
    for(RD_Window *window = rd_state->first_window; window != 0; window = window->next)
    {
      if(window->cfg_src != source)
      {
        continue;
      }
      if(first)
      {
        first = 0;
        str8_list_push(arena, &strs, str8_lit("/// windows ///////////////////////////////////////////////////////////////////\n"));
        str8_list_push(arena, &strs, str8_lit("\n"));
      }
      OS_Handle monitor = os_monitor_from_window(window->os);
      String8 monitor_name = os_name_from_monitor(arena, monitor);
      RD_Panel *root_panel = window->root_panel;
      Rng2F32 rect = os_rect_from_window(window->os);
      Vec2F32 size = dim_2f32(rect);
      str8_list_push (arena, &strs,  str8_lit("window:\n"));
      str8_list_push (arena, &strs,  str8_lit("{\n"));
      str8_list_pushf(arena, &strs,           "  %s%s%s\n",
                      root_panel->split_axis == Axis2_X ? "split_x" : "split_y",
                      os_window_is_fullscreen(window->os) ? " fullscreen" : "",
                      os_window_is_maximized(window->os) ? " maximized" : "");
      str8_list_pushf(arena, &strs, "  monitor: \"%S\"\n", monitor_name);
      str8_list_pushf(arena, &strs, "  size: (%i %i)\n", (int)size.x, (int)size.y);
      str8_list_pushf(arena, &strs, "  dpi: %f\n", os_dpi_from_window(window->os));
      for EachEnumVal(RD_SettingCode, code)
      {
        RD_SettingVal current = window->setting_vals[code];
        if(current.set)
        {
          str8_list_pushf(arena, &strs, "  %S: %i\n", rd_setting_code_lower_string_table[code], current.s32);
        }
      }
      {
        RD_PanelRec rec = {0};
        S32 indentation = 2;
        String8 indent_str = str8_lit("                                                                                                   ");
        str8_list_pushf(arena, &strs, "  panels:\n");
        str8_list_pushf(arena, &strs, "  {\n");
        for(RD_Panel *p = root_panel; !rd_panel_is_nil(p); p = rec.next)
        {
          // rjf: get recursion
          rec = rd_panel_rec_depth_first_pre(p);
          
          // rjf: non-root needs pct node
          if(p != root_panel)
          {
            str8_list_pushf(arena, &strs, "%.*s%g:\n", indentation*2, indent_str.str, p->pct_of_parent);
            str8_list_pushf(arena, &strs, "%.*s{\n", indentation*2, indent_str.str);
            indentation += 1;
          }
          
          // rjf: per-panel options
          struct { String8 key; B32 value; } options[] =
          {
            {str8_lit_comp("tabs_on_bottom"),   p->tab_side == Side_Max},
          };
          B32 has_options = 0;
          for(U64 op_idx = 0; op_idx < ArrayCount(options); op_idx += 1)
          {
            if(options[op_idx].value)
            {
              if(has_options == 0)
              {
                str8_list_pushf(arena, &strs, "%.*s", indentation*2, indent_str.str);
              }
              else
              {
                str8_list_pushf(arena, &strs, " ");
              }
              has_options = 1;
              str8_list_push(arena, &strs, options[op_idx].key);
            }
          }
          if(has_options)
          {
            str8_list_pushf(arena, &strs, "\n");
          }
          
          // rjf: views
          for(RD_View *view = p->first_tab_view; !rd_view_is_nil(view); view = view->order_next)
          {
            String8 view_string = view->spec->string;
            
            // rjf: serialize views
            {
              str8_list_pushf(arena, &strs, "%.*s", indentation*2, indent_str.str);
              
              // rjf: serialize view string
              str8_list_push(arena, &strs, view_string);
              
              // rjf: serialize view parameterizations
              str8_list_push(arena, &strs, str8_lit(": {"));
              if(view == rd_selected_tab_from_panel(p))
              {
                str8_list_push(arena, &strs, str8_lit("selected "));
              }
              {
                if(view->project_path.size != 0)
                {
                  Temp scratch = scratch_begin(&arena, 1);
                  String8 project_path_absolute = path_normalized_from_string(scratch.arena, view->project_path);
                  String8 project_path_relative = path_relative_dst_from_absolute_dst_src(scratch.arena, project_path_absolute, root_path);
                  str8_list_pushf(arena, &strs, "project:{\"%S\"} ", project_path_relative);
                  scratch_end(scratch);
                }
              }
              if(view->query_string_size != 0)
              {
                Temp scratch = scratch_begin(&arena, 1);
                String8 query_raw = str8(view->query_buffer, view->query_string_size);
                {
                  String8 query_file_path = rd_file_path_from_eval_string(scratch.arena, query_raw);
                  if(query_file_path.size != 0)
                  {
                    query_file_path = path_relative_dst_from_absolute_dst_src(scratch.arena, query_file_path, root_path);
                    query_raw = push_str8f(scratch.arena, "file:\"%S\"", query_file_path);
                  }
                }
                String8 query_sanitized = escaped_from_raw_str8(scratch.arena, query_raw);
                str8_list_pushf(arena, &strs, "query:{\"%S\"} ", query_sanitized);
                scratch_end(scratch);
              }
              {
                String8 reserved_keys[] =
                {
                  str8_lit("project"),
                  str8_lit("query"),
                  str8_lit("selected"),
                };
                MD_NodeRec rec = {0};
                MD_Node *params_root = view->params_roots[view->params_read_gen%ArrayCount(view->params_roots)];
                for(MD_Node *n = params_root;
                    !md_node_is_nil(n);
                    n = rec.next)
                {
                  rec = md_node_rec_depth_first_pre(n, params_root);
                  B32 is_reserved_key = 0;
                  for(U64 idx = 0; idx < ArrayCount(reserved_keys); idx += 1)
                  {
                    if(str8_match(n->string, reserved_keys[idx], 0))
                    {
                      is_reserved_key = 1;
                      break;
                    }
                  }
                  if(is_reserved_key)
                  {
                    rec = md_node_rec_depth_first(n, params_root, OffsetOf(MD_Node, next), OffsetOf(MD_Node, next));
                  }
                  if(!is_reserved_key && n != params_root)
                  {
                    str8_list_pushf(arena, &strs, "%S", n->string);
                    if(n->first != &md_nil_node)
                    {
                      str8_list_pushf(arena, &strs, ":{");
                    }
                    for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
                    {
                      if(pop_idx == rec.pop_count-1 && rec.next == &md_nil_node)
                      {
                        break;
                      }
                      str8_list_pushf(arena, &strs, "}");
                    }
                    if(rec.pop_count != 0 || n->next != &md_nil_node)
                    {
                      str8_list_pushf(arena, &strs, " ");
                    }
                  }
                }
              }
              str8_list_push(arena, &strs, str8_lit("}\n"));
            }
          }
          
          // rjf: non-roots need closer
          if(p != root_panel && rec.push_count == 0)
          {
            indentation -= 1;
            str8_list_pushf(arena, &strs, "%.*s}\n", indentation*2, indent_str.str);
          }
          
          // rjf: pop
          for(S32 pop_idx = 0; pop_idx < rec.pop_count; pop_idx += 1)
          {
            indentation -= 1;
            if(pop_idx == rec.pop_count-1 && rec.next == &rd_nil_panel)
            {
              break;
            }
            str8_list_pushf(arena, &strs, "%.*s}\n", indentation*2, indent_str.str);
          }
        }
        str8_list_pushf(arena, &strs, "  }\n");
      }
      str8_list_push (arena, &strs,  str8_lit("}\n"));
      str8_list_push (arena, &strs,  str8_lit("\n"));
    }
  }
  
  //- rjf: serialize keybindings
  if(source == RD_CfgSrc_User)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 indent_str = str8_lit("                                                                                                             ");
    U64 string_binding_pair_count = 0;
    RD_StringBindingPair *string_binding_pairs = push_array(scratch.arena, RD_StringBindingPair, rd_state->key_map_total_count);
    for(U64 idx = 0;
        idx < rd_state->key_map_table_size && string_binding_pair_count < rd_state->key_map_total_count;
        idx += 1)
    {
      for(RD_KeyMapNode *n = rd_state->key_map_table[idx].first;
          n != 0 && string_binding_pair_count < rd_state->key_map_total_count;
          n = n->hash_next)
      {
        RD_StringBindingPair *pair = string_binding_pairs + string_binding_pair_count;
        pair->string = n->name;
        pair->binding = n->binding;
        string_binding_pair_count += 1;
      }
    }
    quick_sort(string_binding_pairs, string_binding_pair_count, sizeof(RD_StringBindingPair), rd_qsort_compare__cfg_string_bindings);
    if(string_binding_pair_count != 0)
    {
      str8_list_push(arena, &strs, str8_lit("/// keybindings ///////////////////////////////////////////////////////////////\n"));
      str8_list_push(arena, &strs, str8_lit("\n"));
      str8_list_push(arena, &strs, str8_lit("keybindings:\n"));
      str8_list_push(arena, &strs, str8_lit("{\n"));
      for(U64 idx = 0; idx < string_binding_pair_count; idx += 1)
      {
        RD_StringBindingPair *pair = string_binding_pairs + idx;
        String8List modifiers_strings = os_string_list_from_modifiers(scratch.arena, pair->binding.modifiers);
        StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
        String8 event_flags_string = str8_list_join(scratch.arena, &modifiers_strings, &join);
        String8 key_string = push_str8_copy(scratch.arena, os_g_key_cfg_string_table[pair->binding.key]);
        for(U64 i = 0; i < event_flags_string.size; i += 1)
        {
          event_flags_string.str[i] = char_to_lower(event_flags_string.str[i]);
        }
        String8 binding_string = push_str8f(scratch.arena, "%S%s%S",
                                            event_flags_string,
                                            event_flags_string.size > 0 ? " " : "",
                                            key_string);
        str8_list_pushf(arena, &strs, "  {\"%S\"%.*s%S%.*s}\n",
                        pair->string,
                        40 > pair->string.size ? ((int)(40 - pair->string.size)) : 0, indent_str.str,
                        binding_string,
                        20 > binding_string.size ? ((int)(20 - binding_string.size)) : 0, indent_str.str);
      }
      str8_list_push(arena, &strs, str8_lit("}\n\n"));
    }
    scratch_end(scratch);
  }
  
  //- rjf: serialize theme colors
  if(source == RD_CfgSrc_User)
  {
    // rjf: determine if this theme matches an existing preset
    B32 is_preset = 0;
    RD_ThemePreset matching_preset = RD_ThemePreset_DefaultDark;
    {
      for(RD_ThemePreset p = (RD_ThemePreset)0; p < RD_ThemePreset_COUNT; p = (RD_ThemePreset)(p+1))
      {
        B32 matches_this_preset = 1;
        for(RD_ThemeColor c = (RD_ThemeColor)(RD_ThemeColor_Null+1); c < RD_ThemeColor_COUNT; c = (RD_ThemeColor)(c+1))
        {
          if(!MemoryMatchStruct(&rd_state->cfg_theme_target.colors[c], &rd_theme_preset_colors_table[p][c]))
          {
            matches_this_preset = 0;
            break;
          }
        }
        if(matches_this_preset)
        {
          is_preset = 1;
          matching_preset = p;
          break;
        }
      }
    }
    
    // rjf: serialize header
    String8 indent_str = str8_lit("                                                                                                             ");
    str8_list_push(arena, &strs, str8_lit("/// colors ////////////////////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    
    // rjf: serialize preset theme
    if(is_preset)
    {
      str8_list_pushf(arena, &strs, "color_preset: \"%S\"\n\n", rd_theme_preset_code_string_table[matching_preset]);
    }
    
    // rjf: serialize non-preset theme
    if(!is_preset)
    {
      str8_list_push(arena, &strs, str8_lit("colors:\n"));
      str8_list_push(arena, &strs, str8_lit("{\n"));
      for(RD_ThemeColor color = (RD_ThemeColor)(RD_ThemeColor_Null+1);
          color < RD_ThemeColor_COUNT;
          color = (RD_ThemeColor)(color+1))
      {
        String8 color_name = rd_theme_color_cfg_string_table[color];
        Vec4F32 color_rgba = rd_state->cfg_theme_target.colors[color];
        String8 color_hex  = hex_string_from_rgba_4f32(arena, color_rgba);
        str8_list_pushf(arena, &strs, "  %S:%.*s0x%S\n",
                        color_name,
                        30 > color_name.size ? ((int)(30 - color_name.size)) : 0, indent_str.str,
                        color_hex);
      }
      str8_list_push(arena, &strs, str8_lit("}\n\n"));
    }
  }
  
  //- rjf: serialize fonts
  if(source == RD_CfgSrc_User)
  {
    String8 code_font_path_escaped = escaped_from_raw_str8(arena, rd_state->cfg_code_font_path);
    String8 main_font_path_escaped = escaped_from_raw_str8(arena, rd_state->cfg_main_font_path);
    str8_list_push(arena, &strs, str8_lit("/// fonts /////////////////////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    str8_list_pushf(arena, &strs, "code_font: \"%S\"\n", code_font_path_escaped);
    str8_list_pushf(arena, &strs, "main_font: \"%S\"\n", main_font_path_escaped);
    str8_list_push(arena, &strs, str8_lit("\n"));
  }
  
  //- rjf: serialize global settings
  {
    B32 first = 1;
    for EachEnumVal(RD_SettingCode, code)
    {
      if(rd_setting_code_default_is_per_window_table[code])
      {
        continue;
      }
      RD_SettingVal current = rd_state->cfg_setting_vals[source][code];
      if(current.set)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// global settings ///////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        str8_list_pushf(arena, &strs, "%S: %i\n", rd_setting_code_lower_string_table[code], current.s32);
      }
    }
    if(!first)
    {
      str8_list_push(arena, &strs, str8_lit("\n"));
    }
  }
  
  ProfEnd();
  return strs;
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

internal String8
rd_stop_explanation_string_icon_from_ctrl_event(Arena *arena, CTRL_Event *event, RD_IconKind *icon_out)
{
  RD_IconKind icon = RD_IconKind_Null;
  String8 explanation = {0};
  Temp scratch = scratch_begin(&arena, 1);
  RD_Entity *thread = rd_entity_from_ctrl_handle(event->entity);
  String8 thread_display_string = rd_display_string_from_entity(scratch.arena, thread);
  String8 process_thread_string = thread_display_string;
  RD_Entity *process = rd_entity_ancestor_from_kind(thread, RD_EntityKind_Process);
  if(process->kind == RD_EntityKind_Process)
  {
    String8 process_display_string = rd_display_string_from_entity(scratch.arena, process);
    process_thread_string = push_str8f(scratch.arena, "%S: %S", process_display_string, thread_display_string);
  }
  switch(event->kind)
  {
    default:
    {
      switch(event->cause)
      {
        default:{}break;
        case CTRL_EventCause_Finished:
        {
          if(!rd_entity_is_nil(thread))
          {
            explanation = push_str8f(arena, "%S completed step", process_thread_string);
          }
          else
          {
            explanation = str8_lit("Stopped");
          }
        }break;
        case CTRL_EventCause_EntryPoint:
        {
          explanation = str8_lit("Stopped at entry point");
        }break;
        case CTRL_EventCause_UserBreakpoint:
        {
          if(!rd_entity_is_nil(thread))
          {
            icon = RD_IconKind_CircleFilled;
            explanation = push_str8f(arena, "%S hit a breakpoint", process_thread_string);
          }
        }break;
        case CTRL_EventCause_InterruptedByException:
        {
          if(!rd_entity_is_nil(thread))
          {
            icon = RD_IconKind_WarningBig;
            switch(event->exception_kind)
            {
              default:
              {
                String8 exception_code_string = rd_string_from_exception_code(event->exception_code);
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x%s%S", process_thread_string, event->exception_code, exception_code_string.size > 0 ? ": " : "", exception_code_string);
              }break;
              case CTRL_ExceptionKind_CppThrow:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: C++ exception", process_thread_string, event->exception_code);
              }break;
              case CTRL_ExceptionKind_MemoryRead:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: Access violation reading 0x%I64x",
                                         process_thread_string,
                                         event->exception_code,
                                         event->vaddr_rng.min);
              }break;
              case CTRL_ExceptionKind_MemoryWrite:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: Access violation writing 0x%I64x",
                                         process_thread_string,
                                         event->exception_code,
                                         event->vaddr_rng.min);
              }break;
              case CTRL_ExceptionKind_MemoryExecute:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: Access violation executing 0x%I64x",
                                         process_thread_string,
                                         event->exception_code,
                                         event->vaddr_rng.min);
              }break;
            }
          }
          else
          {
            icon = RD_IconKind_Pause;
            explanation = str8_lit("Interrupted");
          }
        }break;
        case CTRL_EventCause_InterruptedByTrap:
        {
          icon = RD_IconKind_WarningBig;
          explanation = push_str8f(arena, "%S interrupted by trap - 0x%x", process_thread_string, event->exception_code);
        }break;
        case CTRL_EventCause_InterruptedByHalt:
        {
          icon = RD_IconKind_Pause;
          explanation = str8_lit("Halted");
        }break;
      }
    }break;
  }
  scratch_end(scratch);
  if(icon_out)
  {
    *icon_out = icon;
  }
  return explanation;
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

//- rjf: config paths

internal String8
rd_cfg_path_from_src(RD_CfgSrc src)
{
  return rd_state->cfg_paths[src];
}

//- rjf: entity cache queries

internal RD_EntityList
rd_query_cached_entity_list_with_kind(RD_EntityKind kind)
{
  ProfBeginFunction();
  RD_EntityListCache *cache = &rd_state->kind_caches[kind];
  
  // rjf: build cached list if we're out-of-date
  if(cache->alloc_gen != rd_state->kind_alloc_gens[kind])
  {
    cache->alloc_gen = rd_state->kind_alloc_gens[kind];
    if(cache->arena == 0)
    {
      cache->arena = arena_alloc();
    }
    arena_clear(cache->arena);
    cache->list = rd_push_entity_list_with_kind(cache->arena, kind);
  }
  
  // rjf: grab & return cached list
  RD_EntityList result = cache->list;
  ProfEnd();
  return result;
}

internal RD_EntityList
rd_push_active_target_list(Arena *arena)
{
  RD_EntityList active_targets = {0};
  RD_EntityList all_targets = rd_query_cached_entity_list_with_kind(RD_EntityKind_Target);
  for(RD_EntityNode *n = all_targets.first; n != 0; n = n->next)
  {
    if(!n->entity->disabled)
    {
      rd_entity_list_push(arena, &active_targets, n->entity);
    }
  }
  return active_targets;
}

internal RD_Entity *
rd_entity_from_ev_key_and_kind(EV_Key key, RD_EntityKind kind)
{
  RD_Entity *result = &d_nil_entity;
  RD_EntityList list = rd_query_cached_entity_list_with_kind(kind);
  for(RD_EntityNode *n = list.first; n != 0; n = n->next)
  {
    RD_Entity *entity = n->entity;
    if(ev_key_match(rd_ev_key_from_entity(entity), key))
    {
      result = entity;
      break;
    }
  }
  return result;
}

//- rjf: config state

internal RD_CfgTable *
rd_cfg_table(void)
{
  return &rd_state->cfg_table;
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
    // TODO(rjf): extend this by looking up into dynamically-registered commands by views
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
  rd_state->entities_arena = arena_alloc(.reserve_size = GB(64), .commit_size = KB(64));
  rd_state->entities_root = &d_nil_entity;
  rd_state->entities_base = push_array(rd_state->entities_arena, RD_Entity, 0);
  rd_state->entities_count = 0;
  rd_state->entities_root = rd_entity_alloc(&d_nil_entity, RD_EntityKind_Root);
  rd_state->key_map_arena = arena_alloc();
  rd_state->popup_arena = arena_alloc();
  rd_state->ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("top_level_ctx_menu"));
  rd_state->drop_completion_key = ui_key_from_string(ui_key_zero(), str8_lit("drop_completion_ctx_menu"));
  rd_state->string_search_arena = arena_alloc();
  rd_state->eval_viz_view_cache_slots_count = 1024;
  rd_state->eval_viz_view_cache_slots = push_array(arena, RD_EvalVizViewCacheSlot, rd_state->eval_viz_view_cache_slots_count);
  rd_state->cfg_main_font_path_arena = arena_alloc();
  rd_state->cfg_code_font_path_arena = arena_alloc();
  rd_state->bind_change_arena = arena_alloc();
  rd_state->drag_drop_arena = arena_alloc();
  rd_state->drag_drop_regs = push_array(rd_state->drag_drop_arena, RD_Regs, 1);
  rd_state->top_regs = &rd_state->base_regs;
  rd_clear_bindings();
  
  // rjf: set up initial entities
  {
    RD_Entity *local_machine = rd_entity_alloc(rd_state->entities_root, RD_EntityKind_Machine);
    rd_entity_equip_ctrl_handle(local_machine, ctrl_handle_make(CTRL_MachineID_Local, dmn_handle_zero()));
    rd_entity_equip_name(local_machine, str8_lit("This PC"));
  }
  
  // rjf: set up user / project paths
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
    String8 cfg_src_paths[RD_CfgSrc_COUNT] = {user_cfg_path, project_cfg_path};
    for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
    {
      rd_state->cfg_path_arenas[src] = arena_alloc();
      rd_cmd(rd_cfg_src_load_cmd_kind_table[src], .file_path = path_normalized_from_string(scratch.arena, cfg_src_paths[src]));
    }
    
    // rjf: set up config table arena
    rd_state->cfg_arena = arena_alloc();
    scratch_end(scratch);
  }
  
  // rjf: set up initial exception filtering rules
  for(CTRL_ExceptionCodeKind k = (CTRL_ExceptionCodeKind)0; k < CTRL_ExceptionCodeKind_COUNT; k = (CTRL_ExceptionCodeKind)(k+1))
  {
    if(ctrl_exception_code_kind_default_enable_table[k])
    {
      rd_state->ctrl_exception_code_filters[k/64] |= 1ull<<(k%64);
    }
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
  Temp scratch = scratch_begin(0, 0);
  local_persist S32 depth = 0;
  log_scope_begin();
  
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
  rd_state->ctrl_entity_meval_cache_slots_count = 1024;
  rd_state->ctrl_entity_meval_cache_slots = push_array(rd_frame_arena(), RD_CtrlEntityMetaEvalCacheSlot, rd_state->ctrl_entity_meval_cache_slots_count);
  
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
      rd_unbind_name(rd_state->bind_change_cmd_name, rd_state->bind_change_binding);
      rd_state->bind_change_active = 0;
      rd_cmd(rd_cfg_src_write_cmd_kind_table[RD_CfgSrc_User]);
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
        rd_unbind_name(rd_state->bind_change_cmd_name, rd_state->bind_change_binding);
        rd_bind_name(rd_state->bind_change_cmd_name, binding);
        U32 codepoint = os_codepoint_from_modifiers_and_key(event->modifiers, event->key);
        os_text(&events, event->window, codepoint);
        os_eat_event(&events, event);
        rd_cmd(rd_cfg_src_write_cmd_kind_table[RD_CfgSrc_User]);
        rd_request_frame();
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: consume events
  //
  {
    for(OS_Event *event = events.first, *next = 0;
        event != 0;
        event = next)
      RD_RegsScope()
    {
      next = event->next;
      RD_Window *window = rd_window_from_os_handle(event->window);
      if(window != 0 && window != rd_window_from_handle(rd_regs()->window))
      {
        rd_regs()->window = rd_handle_from_window(window);
        rd_regs()->panel  = rd_handle_from_panel(window->focused_panel);
        rd_regs()->view   = window->focused_panel->selected_tab_view;
      }
      B32 take = 0;
      
      //- rjf: try drag/drop drop-kickoff
      if(rd_drag_is_active() && event->kind == OS_EventKind_Release && event->key == OS_Key_LeftMouseButton)
      {
        rd_state->drag_drop_state = RD_DragDropState_Dropping;
      }
      
      //- rjf: try window close
      if(!take && event->kind == OS_EventKind_WindowClose && window != 0)
      {
        take = 1;
        rd_cmd(RD_CmdKind_CloseWindow, .window = rd_handle_from_window(window));
      }
      
      //- rjf: try menu bar operations
      {
        if(!take && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->modifiers == 0 && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          window->menu_bar_focused_on_press = window->menu_bar_focused;
          window->menu_bar_key_held = 1;
          window->menu_bar_focus_press_started = 1;
        }
        if(!take && event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->modifiers == 0 && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          window->menu_bar_key_held = 0;
        }
        if(window->menu_bar_focused && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->modifiers == 0 && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          window->menu_bar_focused = 0;
        }
        else if(window->menu_bar_focus_press_started && !window->menu_bar_focused && event->kind == OS_EventKind_Release && event->modifiers == 0 && event->key == OS_Key_Alt && event->is_repeat == 0)
        {
          take = 1;
          rd_request_frame();
          window->menu_bar_focused = !window->menu_bar_focused_on_press;
          window->menu_bar_focus_press_started = 0;
        }
        else if(event->kind == OS_EventKind_Press && event->key == OS_Key_Esc && window->menu_bar_focused && !ui_any_ctx_menu_is_open())
        {
          take = 1;
          rd_request_frame();
          window->menu_bar_focused = 0;
        }
      }
      
      //- rjf: try hotkey presses
      if(!take && event->kind == OS_EventKind_Press)
      {
        RD_Binding binding = {event->key, event->modifiers};
        String8List spec_candidates = rd_cmd_name_list_from_binding(scratch.arena, binding);
        if(spec_candidates.first != 0)
        {
          U32 hit_char = os_codepoint_from_modifiers_and_key(event->modifiers, event->key);
          if(hit_char == 0 || allow_text_hotkeys)
          {
            rd_cmd(RD_CmdKind_RunCommand, .cmd_name = spec_candidates.first->string);
            if(allow_text_hotkeys)
            {
              os_text(&events, event->window, hit_char);
              next = event->next;
            }
            take = 1;
            if(event->modifiers & OS_Modifier_Alt)
            {
              window->menu_bar_focus_press_started = 0;
            }
          }
        }
        else if(OS_Key_F1 <= event->key && event->key <= OS_Key_F19)
        {
          window->menu_bar_focus_press_started = 0;
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
          window->menu_bar_focus_press_started = 0;
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
  for(U64 cmd_process_loop_idx = 0; cmd_process_loop_idx < 3; cmd_process_loop_idx += 1)
  {
    ////////////////////////////
    //- rjf: unpack eval-dependent info
    //
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
        }
      }
    }
    U64 rdis_count = Max(1, all_modules.count);
    RDI_Parsed **rdis = push_array(scratch.arena, RDI_Parsed *, rdis_count);
    rdis[0] = &di_rdi_parsed_nil;
    U64 rdis_primary_idx = 0;
    Rng1U64 *rdis_vaddr_ranges = push_array(scratch.arena, Rng1U64, rdis_count);
    {
      U64 idx = 0;
      for(CTRL_EntityNode *n = all_modules.first; n != 0; n = n->next, idx += 1)
      {
        DI_Key dbgi_key = ctrl_dbgi_key_from_module(n->v);
        rdis[idx] = di_rdi_from_key(rd_state->frame_di_scope, &dbgi_key, 0);
        rdis_vaddr_ranges[idx] = n->v->vaddr_range;
        if(n->v == module)
        {
          primary_dbgi_key = dbgi_key;
          rdis_primary_idx = idx;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: build eval type context
    //
    E_TypeCtx *type_ctx = push_array(scratch.arena, E_TypeCtx, 1);
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
    //- rjf: create names/type-info for debugger collections
    //
    E_TypeKey collection_type_keys[ArrayCount(rd_collection_name_table)] = {0};
    for EachElement(idx, rd_collection_name_table)
    {
      collection_type_keys[idx] = e_type_key_cons(.kind = E_TypeKind_Collection, .name = rd_collection_name_table[idx]);
    }
    
    ////////////////////////////
    //- rjf: build eval IR context
    //
    E_TypeKey meta_eval_type_key = e_type_key_cons_base(type(CTRL_MetaEval));
    E_IRCtx *ir_ctx = push_array(scratch.arena, E_IRCtx, 1);
    {
      E_IRCtx *ctx = ir_ctx;
      ctx->macro_map     = push_array(scratch.arena, E_String2ExprMap, 1);
      ctx->macro_map[0]  = e_string2expr_map_make(scratch.arena, 512);
      
      //- rjf: add macros for collections
      {
        for EachElement(idx, rd_collection_name_table)
        {
          E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
          expr->space    = e_space_make(RD_EvalSpaceKind_MetaCollection);
          expr->mode     = E_Mode_Null;
          expr->type_key = collection_type_keys[idx];
          e_string2expr_map_insert(scratch.arena, ctx->macro_map, rd_collection_name_table[idx], expr);
        }
      }
      
      //- rjf: add macros for all evallable debugger frontend entities
      {
        RD_EntityKind evallable_kinds[] =
        {
          RD_EntityKind_Breakpoint,
          RD_EntityKind_WatchPin,
          RD_EntityKind_Target,
          RD_EntityKind_FilePathMap,
          RD_EntityKind_AutoViewRule,
        };
        E_TypeKey evallable_kind_types[] =
        {
          e_type_key_cons_base(type(CTRL_BreakpointMetaEval)),
          e_type_key_cons_base(type(CTRL_PinMetaEval)),
          e_type_key_cons_base(type(CTRL_TargetMetaEval)),
          e_type_key_cons_base(type(CTRL_FilePathMapMetaEval)),
          e_type_key_cons_base(type(CTRL_AutoViewRuleMetaEval)),
        };
        for EachElement(idx, evallable_kinds)
        {
          RD_EntityList list = rd_query_cached_entity_list_with_kind(evallable_kinds[idx]);
          for(RD_EntityNode *n = list.first; n != 0; n = n->next)
          {
            RD_Entity *entity = n->entity;
            E_Space space = rd_eval_space_from_entity(entity);
            E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
            expr->space    = space;
            expr->mode     = E_Mode_Offset;
            expr->type_key = evallable_kind_types[idx];
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, push_str8f(scratch.arena, "$%I64u", entity->id), expr);
            if(entity->string.size != 0 && entity->kind != RD_EntityKind_WatchPin)
            {
              e_string2expr_map_insert(scratch.arena, ctx->macro_map, entity->string, expr);
            }
          }
        }
      }
      
      //- rjf: add macros for all evallable control entities
      {
        CTRL_EntityKind evallable_kinds[] =
        {
          CTRL_EntityKind_Machine,
          CTRL_EntityKind_Process,
          CTRL_EntityKind_Thread,
          CTRL_EntityKind_Module,
        };
        E_TypeKey evallable_kind_types[] =
        {
          e_type_key_cons_base(type(CTRL_MachineMetaEval)),
          e_type_key_cons_base(type(CTRL_ProcessMetaEval)),
          e_type_key_cons_base(type(CTRL_ThreadMetaEval)),
          e_type_key_cons_base(type(CTRL_ModuleMetaEval)),
        };
        for EachElement(idx, evallable_kinds)
        {
          CTRL_EntityKind kind = evallable_kinds[idx];
          CTRL_EntityList list = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, kind);
          for(CTRL_EntityNode *n = list.first; n != 0; n = n->next)
          {
            CTRL_Entity *entity = n->v;
            E_Space space = rd_eval_space_from_ctrl_entity(entity, RD_EvalSpaceKind_MetaCtrlEntity);
            E_Expr *expr = e_push_expr(scratch.arena, E_ExprKind_LeafOffset, 0);
            expr->space    = space;
            expr->mode     = E_Mode_Offset;
            expr->type_key = evallable_kind_types[idx];
            e_string2expr_map_insert(scratch.arena, ctx->macro_map, push_str8f(scratch.arena, "$_%I64x_%I64x", entity->handle.machine_id, entity->handle.dmn_handle.u64[0]), expr);
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
        }
      }
      
      //- rjf: add macros for all watches which define identifiers
      RD_EntityList watches = rd_query_cached_entity_list_with_kind(RD_EntityKind_Watch);
      for(RD_EntityNode *n = watches.first; n != 0; n = n->next)
      {
        RD_Entity *watch = n->entity;
        String8 expr = watch->string;
        E_TokenArray tokens   = e_token_array_from_text(scratch.arena, expr);
        E_Parse      parse    = e_parse_expr_from_text_tokens(scratch.arena, expr, &tokens);
        if(parse.msgs.max_kind == E_MsgKind_Null)
        {
          e_push_leaf_ident_exprs_from_expr__in_place(scratch.arena, ctx->macro_map, parse.expr);
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
    //- rjf: build eval visualization view rule table
    //
    EV_ViewRuleInfoTable *view_rule_info_table = push_array(scratch.arena, EV_ViewRuleInfoTable, 1);
    {
      ev_view_rule_info_table_push_builtins(scratch.arena, view_rule_info_table);
      
      // rjf: collection view rules
      for EachElement(idx, rd_collection_name_table)
      {
        EV_ViewRuleInfo info = {0};
        info.string                  = rd_collection_name_table[idx];
        info.flags                   = EV_ViewRuleInfoFlag_Expandable;
        info.expr_resolution         = EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(identity);
        info.expr_expand_info        = rd_collection_expr_expand_info_hook_function_table[idx];
        info.expr_expand_range_info  = rd_collection_expr_expand_range_info_hook_function_table[idx];
        info.expr_expand_id_from_num = rd_collection_expr_expand_id_from_num_hook_function_table[idx];
        info.expr_expand_num_from_id = rd_collection_expr_expand_num_from_id_hook_function_table[idx];
        ev_view_rule_info_table_push(scratch.arena, view_rule_info_table, &info);
      }
      
      // rjf: visualizer view rules
      for EachEnumVal(RD_ViewRuleKind, k)
      {
        RD_ViewRuleInfo *src_info = &rd_view_rule_kind_info_table[k];
        if(src_info->flags & RD_ViewRuleInfoFlag_CanUseInWatchTable)
        {
          EV_ViewRuleInfo dst_info = {0};
          dst_info.string                  = src_info->string;
          dst_info.flags                   = src_info->flags & RD_ViewRuleInfoFlag_CanExpand ? EV_ViewRuleInfoFlag_Expandable : 0;
          dst_info.expr_resolution         = EV_VIEW_RULE_EXPR_RESOLUTION_FUNCTION_NAME(identity);
          dst_info.expr_expand_info        = src_info->expr_expand_info;
          dst_info.expr_expand_range_info  = EV_VIEW_RULE_EXPR_EXPAND_RANGE_INFO_FUNCTION_NAME(nil);
          dst_info.expr_expand_id_from_num = EV_VIEW_RULE_EXPR_EXPAND_ID_FROM_NUM_FUNCTION_NAME(identity);
          dst_info.expr_expand_num_from_id = EV_VIEW_RULE_EXPR_EXPAND_NUM_FROM_ID_FUNCTION_NAME(identity);
          ev_view_rule_info_table_push(scratch.arena, view_rule_info_table, &dst_info);
        }
      }
    }
    ev_select_view_rule_info_table(view_rule_info_table);
    
    ////////////////////////////
    //- rjf: build eval visualization auto-view-rule table
    //
    EV_AutoViewRuleTable *auto_view_rule_table = push_array(scratch.arena, EV_AutoViewRuleTable, 1);
    {
      ev_auto_view_rule_table_push_new(scratch.arena, auto_view_rule_table, e_type_key_cons_base(type(CTRL_MetaEvalFrameArray)), str8_lit("slice"), 1);
      ev_auto_view_rule_table_push_new(scratch.arena, auto_view_rule_table, e_type_key_cons_base(type(CTRL_MachineMetaEval)),    str8_lit("scheduler_machine"), 1);
      ev_auto_view_rule_table_push_new(scratch.arena, auto_view_rule_table, e_type_key_cons_base(type(CTRL_ProcessMetaEval)),    str8_lit("scheduler_process"), 1);
      for EachElement(idx, rd_collection_name_table)
      {
        ev_auto_view_rule_table_push_new(scratch.arena, auto_view_rule_table, collection_type_keys[idx], rd_collection_name_table[idx], 1);
      }
      RD_EntityList auto_view_rules = rd_query_cached_entity_list_with_kind(RD_EntityKind_AutoViewRule);
      for(RD_EntityNode *n = auto_view_rules.first; n != 0; n = n->next)
      {
        RD_Entity *rule = n->entity;
        RD_Entity *src = rd_entity_child_from_kind(rule, RD_EntityKind_Source);
        RD_Entity *dst = rd_entity_child_from_kind(rule, RD_EntityKind_Dest);
        String8 type_string = src->string;
        String8 view_rule_string = dst->string;
        E_TokenArray tokens = e_token_array_from_text(scratch.arena, type_string);
        E_Parse type_parse = e_parse_type_from_text_tokens(scratch.arena, type_string, &tokens);
        E_TypeKey type_key = e_type_from_expr(type_parse.expr);
        if(!e_type_key_match(e_type_key_zero(), type_key))
        {
          ev_auto_view_rule_table_push_new(scratch.arena, auto_view_rule_table, type_key, view_rule_string, 0);
        }
      }
    }
    ev_select_auto_view_rule_table(auto_view_rule_table);
    
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
    //- rjf: process top-level graphical commands
    //
    B32 panel_reset_done = 0;
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
        RD_Panel *split_panel = &rd_nil_panel;
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
          {
            CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
            if(processes.count == 0)
            {
              RD_EntityList bps = rd_query_cached_entity_list_with_kind(RD_EntityKind_Breakpoint);
              for(RD_EntityNode *n = bps.first; n != 0; n = n->next)
              {
                n->entity->u64 = 0;
              }
            }
          } // fallthrough
          default:
          {
            // rjf: try to run engine command
            if(D_CmdKind_Null < (D_CmdKind)kind && (D_CmdKind)kind < D_CmdKind_COUNT)
            {
              RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
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
              if(entity->kind == RD_EntityKind_Target)
              {
                params.targets.count = 1;
                params.targets.v = push_array(scratch.arena, D_Target, params.targets.count);
                params.targets.v[0] = rd_d_target_from_entity(entity);
              }
              d_push_cmd((D_CmdKind)kind, &params);
            }
            
            // rjf: try to open tabs for "view driver" commands
            RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(cmd->name);
            if(view_rule_info != &rd_nil_view_rule_info)
            {
              rd_cmd(RD_CmdKind_OpenTab, .string = str8_zero(), .params_tree = md_tree_from_string(scratch.arena, cmd->name)->first);
            }
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
              RD_Window *window = rd_window_from_handle(rd_regs()->window);
              if(window != 0)
              {
                arena_clear(window->query_cmd_arena);
                window->query_cmd_name = push_str8_copy(window->query_cmd_arena, cmd->regs->cmd_name);
                window->query_cmd_regs = rd_regs_copy(window->query_cmd_arena, rd_regs());
                MemoryZeroArray(window->query_cmd_regs_mask);
                window->query_view_selected = 1;
              }
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
            RD_Window *originating_window = rd_window_from_handle(rd_regs()->window);
            if(originating_window == 0)
            {
              originating_window = rd_state->first_window;
            }
            OS_Handle preferred_monitor = {0};
            RD_Window *new_ws = rd_window_open(v2f32(1280, 720), preferred_monitor, RD_CfgSrc_User);
            if(originating_window)
            {
              MemoryCopy(new_ws->setting_vals, originating_window->setting_vals, sizeof(RD_SettingVal)*RD_SettingCode_COUNT);
            }
          }break;
          case RD_CmdKind_CloseWindow:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            if(ws != 0)
            {
              // rjf: is this the last window? -> exit
              if(rd_state->first_window == rd_state->last_window && rd_state->first_window == ws)
              {
                rd_cmd(RD_CmdKind_Exit);
              }
              
              // rjf: not the last window? -> just release this window
              else
              {
                // NOTE(rjf): we need to explicitly release all panel views, because views
                // are a global concept and otherwise would leak.
                for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
                {
                  rd_panel_release_all_views(panel);
                }
                
                ui_state_release(ws->ui);
                DLLRemove(rd_state->first_window, rd_state->last_window, ws);
                r_window_unequip(ws->os, ws->r);
                os_window_close(ws->os);
                arena_release(ws->query_cmd_arena);
                arena_release(ws->ctx_menu_arena);
                arena_release(ws->drop_completion_arena);
                arena_release(ws->hover_eval_arena);
                arena_release(ws->autocomp_lister_params_arena);
                arena_release(ws->arena);
                SLLStackPush(rd_state->free_window, ws);
                ws->gen += 1;
              }
            }
          }break;
          case RD_CmdKind_ToggleFullscreen:
          {
            RD_Window *window = rd_window_from_handle(rd_regs()->window);
            if(window != 0)
            {
              os_window_set_fullscreen(window->os, !os_window_is_fullscreen(window->os));
            }
          }break;
          case RD_CmdKind_BringToFront:
          {
            RD_Window *last_focused_window = rd_window_from_handle(rd_state->last_focused_window);
            for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
            {
              os_window_set_minimized(w->os, 0);
              os_window_focus(last_focused_window->os);
            }
            if(last_focused_window != 0)
            {
              os_window_focus(last_focused_window->os);
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
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            if(entity->kind == RD_EntityKind_RecentProject)
            {
              rd_cmd(RD_CmdKind_OpenProject, .file_path = entity->string);
            }
          }break;
          case RD_CmdKind_OpenUser:
          case RD_CmdKind_OpenProject:
          {
            // TODO(rjf): dear lord this is so overcomplicated, this needs to be collapsed down & simplified ASAP
            
            B32 load_cfg[RD_CfgSrc_COUNT] = {0};
            RD_CfgSrc cfg_src = (RD_CfgSrc)0;
            for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
            {
              load_cfg[src] = (kind == rd_cfg_src_load_cmd_kind_table[src]);
              if(load_cfg[src])
              {
                cfg_src = src;
              }
            }
            
            //- rjf: normalize path
            String8 new_path = path_normalized_from_string(scratch.arena, rd_regs()->file_path);
            
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
            if(props.modified != 0 && data.size != 0 && !str8_match(str8_prefix(data, 9), str8_lit("// raddbg"), 0) && rd_state->cfg_cached_timestamp[cfg_src] != 0)
            {
              file_is_okay = 0;
            }
            
            //- rjf: set new config paths
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                if(load_cfg[src])
                {
                  arena_clear(rd_state->cfg_path_arenas[src]);
                  rd_state->cfg_paths[src] = push_str8_copy(rd_state->cfg_path_arenas[src], new_path);
                }
              }
            }
            
            //- rjf: get config file properties
            FileProperties cfg_props[RD_CfgSrc_COUNT] = {0};
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                String8 path = rd_cfg_path_from_src(src);
                cfg_props[src] = os_properties_from_file_path(path);
              }
            }
            
            //- rjf: load files
            String8 cfg_data[RD_CfgSrc_COUNT] = {0};
            U64 cfg_timestamps[RD_CfgSrc_COUNT] = {0};
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                String8 path = rd_cfg_path_from_src(src);
                OS_Handle file = os_file_open(OS_AccessFlag_ShareRead|OS_AccessFlag_Read, path);
                FileProperties props = os_properties_from_file(file);
                String8 data = os_string_from_file_range(scratch.arena, file, r1u64(0, props.size));
                if(props.modified != 0)
                {
                  cfg_data[src] = data;
                  cfg_timestamps[src] = props.modified;
                }
                os_file_close(file);
              }
            }
            
            //- rjf: determine if we need to save config
            B32 cfg_save[RD_CfgSrc_COUNT] = {0};
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                cfg_save[src] = (load_cfg[src] && cfg_props[src].created == 0);
              }
            }
            
            //- rjf: determine if we need to reload config
            B32 cfg_load[RD_CfgSrc_COUNT] = {0};
            B32 cfg_load_any = 0;
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                cfg_load[src] = (load_cfg[src] && ((cfg_save[src] == 0 && rd_state->cfg_cached_timestamp[src] != cfg_timestamps[src]) || cfg_props[src].created == 0));
                cfg_load_any = cfg_load_any || cfg_load[src];
              }
            }
            
            //- rjf: load => build new config table
            if(cfg_load_any)
            {
              arena_clear(rd_state->cfg_arena);
              MemoryZeroStruct(&rd_state->cfg_table);
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                rd_cfg_table_push_unparsed_string(rd_state->cfg_arena, &rd_state->cfg_table, cfg_data[src], src);
              }
            }
            
            //- rjf: load => dispatch apply
            //
            // NOTE(rjf): must happen before `save`. we need to create a default before saving, which
            // occurs in the 'apply' path.
            //
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                if(cfg_load[src])
                {
                  RD_CmdKind cmd_kind = rd_cfg_src_apply_cmd_kind_table[src];
                  rd_cmd(cmd_kind);
                  rd_state->cfg_cached_timestamp[src] = cfg_timestamps[src];
                }
              }
            }
            
            //- rjf: save => dispatch write
            if(file_is_okay)
            {
              for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
              {
                if(cfg_save[src])
                {
                  RD_CmdKind cmd_kind = rd_cfg_src_write_cmd_kind_table[src];
                  rd_cmd(cmd_kind);
                }
              }
            }
            
            //- rjf: bad file -> alert user
            if(!file_is_okay)
            {
              log_user_errorf("\"%S\" appears to refer to an existing file which is not a RADDBG config file. This would overwrite the file.", new_path);
            }
          }break;
          
          //- rjf: loading/applying stateful config changes
          case RD_CmdKind_ApplyUserData:
          case RD_CmdKind_ApplyProjectData:
          {
            RD_CfgTable *table = rd_cfg_table();
            OS_HandleArray monitors = os_push_monitors_array(scratch.arena);
            
            //- rjf: get config source
            RD_CfgSrc src = RD_CfgSrc_User;
            for(RD_CfgSrc s = (RD_CfgSrc)0; s < RD_CfgSrc_COUNT; s = (RD_CfgSrc)(s+1))
            {
              if(kind == rd_cfg_src_apply_cmd_kind_table[s])
              {
                src = s;
                break;
              }
            }
            
            //- rjf: get paths
            String8 cfg_path   = rd_cfg_path_from_src(src);
            String8 cfg_folder = str8_chop_last_slash(cfg_path);
            
            //- rjf: keep track of recent projects
            if(src == RD_CfgSrc_Project)
            {
              RD_EntityList recent_projects = rd_query_cached_entity_list_with_kind(RD_EntityKind_RecentProject);
              RD_Entity *recent_project = &d_nil_entity;
              for(RD_EntityNode *n = recent_projects.first; n != 0; n = n->next)
              {
                if(path_match_normalized(cfg_path, n->entity->string))
                {
                  recent_project = n->entity;
                  break;
                }
              }
              if(rd_entity_is_nil(recent_project))
              {
                recent_project = rd_entity_alloc(rd_entity_root(), RD_EntityKind_RecentProject);
                rd_entity_equip_name(recent_project, path_normalized_from_string(scratch.arena, cfg_path));
                rd_entity_equip_cfg_src(recent_project, RD_CfgSrc_User);
              }
            }
            
            //- rjf: eliminate all existing entities which are derived from config
            {
              for EachEnumVal(RD_EntityKind, k)
              {
                RD_EntityKindFlags k_flags = rd_entity_kind_flags_table[k];
                if(k_flags & RD_EntityKindFlag_IsSerializedToConfig)
                {
                  RD_EntityList entities = rd_query_cached_entity_list_with_kind(k);
                  for(RD_EntityNode *n = entities.first; n != 0; n = n->next)
                  {
                    if(n->entity->cfg_src == src)
                    {
                      rd_entity_mark_for_deletion(n->entity);
                    }
                  }
                }
              }
            }
            
            //- rjf: apply all entities
            {
              for EachEnumVal(RD_EntityKind, k)
              {
                RD_EntityKindFlags k_flags = rd_entity_kind_flags_table[k];
                if(k_flags & RD_EntityKindFlag_IsSerializedToConfig)
                {
                  RD_CfgVal *k_val = rd_cfg_val_from_string(table, d_entity_kind_name_lower_table[k]);
                  for(RD_CfgTree *k_tree = k_val->first;
                      k_tree != &d_nil_cfg_tree;
                      k_tree = k_tree->next)
                  {
                    if(k_tree->source != src)
                    {
                      continue;
                    }
                    RD_Entity *entity = rd_entity_alloc(rd_entity_root(), k);
                    rd_entity_equip_cfg_src(entity, k_tree->source);
                    
                    // rjf: iterate config tree
                    typedef struct Task Task;
                    struct Task
                    {
                      Task *next;
                      RD_Entity *entity;
                      MD_Node *n;
                    };
                    Task start_task = {0, entity, k_tree->root};
                    Task *first_task = &start_task;
                    Task *last_task = first_task;
                    for(Task *t = first_task; t != 0; t = t->next)
                    {
                      MD_Node *node = t->n;
                      for MD_EachNode(child, node->first)
                      {
                        // rjf: standalone string literals under an entity -> name
                        if(child->flags & MD_NodeFlag_StringLiteral && child->first == &md_nil_node)
                        {
                          String8 string = raw_from_escaped_str8(scratch.arena, child->string);
                          // TODO(rjf): @hack - hardcoding in the "EntityKind_Location" here - this is because
                          // i am assuming an entity *kind* can 'know' about the 'pathness' of a string. this is
                          // not the case. post-0.9.12 i need to fix this.
                          if(rd_entity_kind_flags_table[t->entity->kind] & RD_EntityKindFlag_NameIsPath &&
                             t->entity->kind != RD_EntityKind_Location)
                          {
                            string = path_absolute_dst_from_relative_dst_src(scratch.arena, string, cfg_folder);
                          }
                          rd_entity_equip_name(t->entity, string);
                        }
                        
                        // rjf: standalone string literals under an entity, with a numeric child -> name & text location
                        if(child->flags & MD_NodeFlag_StringLiteral && child->first->flags & MD_NodeFlag_Numeric && child->first->first == &md_nil_node)
                        {
                          String8 string = raw_from_escaped_str8(scratch.arena, child->string);
                          if(rd_entity_kind_flags_table[t->entity->kind] & RD_EntityKindFlag_NameIsPath)
                          {
                            string = path_absolute_dst_from_relative_dst_src(scratch.arena, string, cfg_folder);
                          }
                          rd_entity_equip_name(t->entity, string);
                          S64 line = 0;
                          try_s64_from_str8_c_rules(child->first->string, &line);
                          TxtPt pt = txt_pt(line, 1);
                          rd_entity_equip_txt_pt(t->entity, pt);
                        }
                        
                        // rjf: standalone hex literals under an entity -> vaddr
                        if(child->flags & MD_NodeFlag_Numeric && child->first == &md_nil_node && str8_match(str8_substr(child->string, r1u64(0, 2)), str8_lit("0x"), 0))
                        {
                          U64 vaddr = 0;
                          try_u64_from_str8_c_rules(child->string, &vaddr);
                          rd_entity_equip_vaddr(t->entity, vaddr);
                        }
                        
                        // rjf: specifically named entity equipment
                        if((str8_match(child->string, str8_lit("name"), StringMatchFlag_CaseInsensitive) ||
                            str8_match(child->string, str8_lit("label"), StringMatchFlag_CaseInsensitive)) &&
                           child->first != &md_nil_node)
                        {
                          String8 string = raw_from_escaped_str8(scratch.arena, child->first->string);
                          // TODO(rjf): @hack - hardcoding in the "EntityKind_Location" here - this is because
                          // i am assuming an entity *kind* can 'know' about the 'pathness' of a string. this is
                          // not the case. post-0.9.12 i need to fix this.
                          if(rd_entity_kind_flags_table[t->entity->kind] & RD_EntityKindFlag_NameIsPath &&
                             (t->entity->kind != RD_EntityKind_Location || !md_node_is_nil(md_child_from_string(node, str8_lit("line"), 0))))
                          {
                            string = path_absolute_dst_from_relative_dst_src(scratch.arena, string, cfg_folder);
                          }
                          rd_entity_equip_name(t->entity, string);
                        }
                        if((str8_match(child->string, str8_lit("active"), StringMatchFlag_CaseInsensitive) ||
                            str8_match(child->string, str8_lit("enabled"), StringMatchFlag_CaseInsensitive)) &&
                           child->first != &md_nil_node)
                        {
                          rd_entity_equip_disabled(t->entity, !str8_match(child->first->string, str8_lit("1"), 0));
                        }
                        if(str8_match(child->string, str8_lit("disabled"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                        {
                          rd_entity_equip_disabled(t->entity, str8_match(child->first->string, str8_lit("1"), 0));
                        }
                        if(str8_match(child->string, str8_lit("hsva"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                        {
                          Vec4F32 hsva = {0};
                          hsva.x = (F32)f64_from_str8(child->first->string);
                          hsva.y = (F32)f64_from_str8(child->first->next->string);
                          hsva.z = (F32)f64_from_str8(child->first->next->next->string);
                          hsva.w = (F32)f64_from_str8(child->first->next->next->next->string);
                          rd_entity_equip_color_hsva(t->entity, hsva);
                        }
                        if(str8_match(child->string, str8_lit("color"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                        {
                          Vec4F32 rgba = rgba_from_hex_string_4f32(child->first->string);
                          Vec4F32 hsva = hsva_from_rgba(rgba);
                          rd_entity_equip_color_hsva(t->entity, hsva);
                        }
                        if(str8_match(child->string, str8_lit("line"), StringMatchFlag_CaseInsensitive) && child->first != &md_nil_node)
                        {
                          S64 line = 0;
                          try_s64_from_str8_c_rules(child->first->string, &line);
                          TxtPt pt = txt_pt(line, 1);
                          rd_entity_equip_txt_pt(t->entity, pt);
                        }
                        if((str8_match(child->string, str8_lit("vaddr"), StringMatchFlag_CaseInsensitive) ||
                            str8_match(child->string, str8_lit("addr"), StringMatchFlag_CaseInsensitive)) &&
                           child->first != &md_nil_node)
                        {
                          U64 vaddr = 0;
                          try_u64_from_str8_c_rules(child->first->string, &vaddr);
                          rd_entity_equip_vaddr(t->entity, vaddr);
                        }
                        
                        // rjf: sub-entity -> create new task
                        RD_EntityKind sub_entity_kind = RD_EntityKind_Nil;
                        for EachEnumVal(RD_EntityKind, k2)
                        {
                          if(child->flags & MD_NodeFlag_Identifier && child->first != &md_nil_node &&
                             (str8_match(child->string, d_entity_kind_name_lower_table[k2], StringMatchFlag_CaseInsensitive) ||
                              (k2 == RD_EntityKind_Executable && str8_match(child->string, str8_lit("exe"), StringMatchFlag_CaseInsensitive))))
                          {
                            Task *task = push_array(scratch.arena, Task, 1);
                            task->next = t->next;
                            task->entity = rd_entity_alloc(t->entity, k2);
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
            RD_CfgVal *filter_tables = rd_cfg_val_from_string(table, str8_lit("exception_code_filters"));
            for(RD_CfgTree *table = filter_tables->first;
                table != &d_nil_cfg_tree;
                table = table->next)
            {
              for MD_EachNode(rule, table->root->first)
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
                      rd_state->ctrl_exception_code_filters[kind/64] |= (1ull<<(kind%64));
                    }
                    else
                    {
                      rd_state->ctrl_exception_code_filters[kind/64] &= ~(1ull<<(kind%64));
                    }
                  }
                }
              }
            }
            
            //- rjf: eliminate all windows
            for(RD_Window *window = rd_state->first_window; window != 0; window = window->next)
            {
              if(window->cfg_src != src)
              {
                continue;
              }
              rd_cmd(RD_CmdKind_CloseWindow, .window = rd_handle_from_window(window));
            }
            
            //- rjf: apply fonts
            {
              FNT_Tag defaults[RD_FontSlot_COUNT] =
              {
                fnt_tag_from_static_data_string(&rd_default_main_font_bytes),
                fnt_tag_from_static_data_string(&rd_default_code_font_bytes),
                fnt_tag_from_static_data_string(&rd_icon_font_bytes),
              };
              MemoryZeroArray(rd_state->cfg_font_tags);
              {
                RD_CfgVal *code_font_val = rd_cfg_val_from_string(table, str8_lit("code_font"));
                RD_CfgVal *main_font_val = rd_cfg_val_from_string(table, str8_lit("main_font"));
                MD_Node *code_font_node = code_font_val->last->root;
                MD_Node *main_font_node = main_font_val->last->root;
                String8 code_font_relative_path = code_font_node->first->string;
                String8 main_font_relative_path = main_font_node->first->string;
                if(!md_node_is_nil(code_font_node))
                {
                  arena_clear(rd_state->cfg_code_font_path_arena);
                  rd_state->cfg_code_font_path = raw_from_escaped_str8(rd_state->cfg_code_font_path_arena, code_font_relative_path);
                }
                if(!md_node_is_nil(main_font_node))
                {
                  arena_clear(rd_state->cfg_main_font_path_arena);
                  rd_state->cfg_main_font_path = raw_from_escaped_str8(rd_state->cfg_main_font_path_arena, main_font_relative_path);
                }
                String8 code_font_path = path_absolute_dst_from_relative_dst_src(scratch.arena, code_font_relative_path, cfg_folder);
                String8 main_font_path = path_absolute_dst_from_relative_dst_src(scratch.arena, main_font_relative_path, cfg_folder);
                if(os_file_path_exists(code_font_path) && !md_node_is_nil(code_font_node) && code_font_relative_path.size != 0)
                {
                  rd_state->cfg_font_tags[RD_FontSlot_Code] = fnt_tag_from_path(code_font_path);
                }
                if(os_file_path_exists(main_font_path) && !md_node_is_nil(main_font_node) && main_font_relative_path.size != 0)
                {
                  rd_state->cfg_font_tags[RD_FontSlot_Main] = fnt_tag_from_path(main_font_path);
                }
              }
              for(RD_FontSlot slot = (RD_FontSlot)0; slot < RD_FontSlot_COUNT; slot = (RD_FontSlot)(slot+1))
              {
                if(fnt_tag_match(fnt_tag_zero(), rd_state->cfg_font_tags[slot]))
                {
                  rd_state->cfg_font_tags[slot] = defaults[slot];
                }
              }
            }
            
            //- rjf: build windows & panel layouts
            RD_CfgVal *windows = rd_cfg_val_from_string(table, str8_lit("window"));
            for(RD_CfgTree *window_tree = windows->first;
                window_tree != &d_nil_cfg_tree;
                window_tree = window_tree->next)
            {
              // rjf: skip wrong source
              if(window_tree->source != src)
              {
                continue;
              }
              
              // rjf: grab metadata
              B32 is_fullscreen = 0;
              B32 is_maximized = 0;
              Axis2 top_level_split_axis = Axis2_X;
              OS_Handle preferred_monitor = os_primary_monitor();
              Vec2F32 size = {0};
              F32 dpi = 0.f;
              RD_SettingVal setting_vals[RD_SettingCode_COUNT] = {0};
              {
                for MD_EachNode(n, window_tree->root->first)
                {
                  if(n->flags & MD_NodeFlag_Identifier &&
                     md_node_is_nil(n->first) &&
                     str8_match(n->string, str8_lit("split_x"), StringMatchFlag_CaseInsensitive))
                  {
                    top_level_split_axis = Axis2_X;
                  }
                  if(n->flags & MD_NodeFlag_Identifier &&
                     md_node_is_nil(n->first) &&
                     str8_match(n->string, str8_lit("split_y"), StringMatchFlag_CaseInsensitive))
                  {
                    top_level_split_axis = Axis2_Y;
                  }
                  if(n->flags & MD_NodeFlag_Identifier &&
                     md_node_is_nil(n->first) &&
                     str8_match(n->string, str8_lit("fullscreen"), StringMatchFlag_CaseInsensitive))
                  {
                    is_fullscreen = 1;
                  }
                  if(n->flags & MD_NodeFlag_Identifier &&
                     md_node_is_nil(n->first) &&
                     str8_match(n->string, str8_lit("maximized"), StringMatchFlag_CaseInsensitive))
                  {
                    is_maximized = 1;
                  }
                }
                MD_Node *monitor_node = md_child_from_string(window_tree->root, str8_lit("monitor"), 0);
                String8 preferred_monitor_name = monitor_node->first->string;
                for(U64 idx = 0; idx < monitors.count; idx += 1)
                {
                  String8 monitor_name = os_name_from_monitor(scratch.arena, monitors.v[idx]);
                  if(str8_match(monitor_name, preferred_monitor_name, StringMatchFlag_CaseInsensitive))
                  {
                    preferred_monitor = monitors.v[idx];
                    break;
                  }
                }
                Vec2F32 preferred_monitor_size = os_dim_from_monitor(preferred_monitor);
                MD_Node *size_node = md_child_from_string(window_tree->root, str8_lit("size"), 0);
                {
                  String8 x_string = size_node->first->string;
                  String8 y_string = size_node->first->next->string;
                  U64 x_u64 = 0;
                  U64 y_u64 = 0;
                  if(!try_u64_from_str8_c_rules(x_string, &x_u64))
                  {
                    x_u64 = (U64)(preferred_monitor_size.x*2/3);
                  }
                  if(!try_u64_from_str8_c_rules(y_string, &y_u64))
                  {
                    y_u64 = (U64)(preferred_monitor_size.y*2/3);
                  }
                  size.x = (F32)x_u64;
                  size.y = (F32)y_u64;
                }
                MD_Node *dpi_node = md_child_from_string(window_tree->root, str8_lit("dpi"), 0);
                String8 dpi_string = md_string_from_children(scratch.arena, dpi_node);
                dpi = f64_from_str8(dpi_string);
                for EachEnumVal(RD_SettingCode, code)
                {
                  MD_Node *code_node = md_child_from_string(window_tree->root, rd_setting_code_lower_string_table[code], 0);
                  if(!md_node_is_nil(code_node))
                  {
                    S64 val_s64 = 0;
                    try_s64_from_str8_c_rules(code_node->first->string, &val_s64);
                    setting_vals[code].set = 1;
                    setting_vals[code].s32 = (S32)val_s64;
                    setting_vals[code].s32 = clamp_1s32(rd_setting_code_s32_range_table[code], setting_vals[code].s32);
                  }
                }
              }
              
              // rjf: open window
              RD_Window *ws = rd_window_open(size, preferred_monitor, window_tree->source);
              if(dpi != 0.f) { ws->last_dpi = dpi; }
              for EachEnumVal(RD_SettingCode, code)
              {
                if(setting_vals[code].set == 0 && rd_setting_code_default_is_per_window_table[code])
                {
                  setting_vals[code] = rd_setting_code_default_val_table[code];
                }
              }
              MemoryCopy(ws->setting_vals, setting_vals, sizeof(setting_vals[0])*ArrayCount(setting_vals));
              
              // rjf: build panel tree
              MD_Node *panel_tree = md_child_from_string(window_tree->root, str8_lit("panels"), 0);
              RD_Panel *panel_parent = ws->root_panel;
              panel_parent->split_axis = top_level_split_axis;
              MD_NodeRec rec = {0};
              for(MD_Node *n = panel_tree, *next = &md_nil_node;
                  !md_node_is_nil(n);
                  n = next)
              {
                // rjf: assume we're just moving to the next one initially...
                next = n->next;
                
                // rjf: grab root panel
                RD_Panel *panel = &rd_nil_panel;
                if(n == panel_tree)
                {
                  panel = ws->root_panel;
                  panel->pct_of_parent = 1.f;
                }
                
                // rjf: allocate & insert non-root panels - these will have a numeric string, determining
                // pct of parent
                if(n->flags & MD_NodeFlag_Numeric)
                {
                  panel = rd_panel_alloc(ws);
                  rd_panel_insert(panel_parent, panel_parent->last, panel);
                  panel->split_axis = axis2_flip(panel_parent->split_axis);
                  panel->pct_of_parent = (F32)f64_from_str8(n->string);
                }
                
                // rjf: do general per-panel work
                if(!rd_panel_is_nil(panel))
                {
                  // rjf: determine if this panel has panel children
                  B32 has_panel_children = 0;
                  for MD_EachNode(child, n->first)
                  {
                    if(child->flags & MD_NodeFlag_Numeric)
                    {
                      has_panel_children = 1;
                      break;
                    }
                  }
                  
                  // rjf: apply panel options
                  for MD_EachNode(op, n->first)
                  {
                    if(md_node_is_nil(op->first) && str8_match(op->string, str8_lit("tabs_on_bottom"), 0))
                    {
                      panel->tab_side = Side_Max;
                    }
                  }
                  
                  // rjf: apply panel views/tabs/commands
                  RD_View *selected_view = &rd_nil_view;
                  for MD_EachNode(op, n->first)
                  {
                    RD_ViewRuleInfo *view_rule_info = rd_view_rule_info_from_string(op->string);
                    if(view_rule_info == &rd_nil_view_rule_info || has_panel_children != 0)
                    {
                      continue;
                    }
                    
                    // rjf: allocate view & apply view-specific parameterizations
                    RD_View *view = &rd_nil_view;
                    B32 view_is_selected = 0;
                    RD_ViewRuleInfoFlags view_rule_info_flags = view_rule_info->flags;
                    {
                      // rjf: allocate view
                      view = rd_view_alloc();
                      
                      // rjf: check if this view is selected
                      view_is_selected = !md_node_is_nil(md_child_from_string(op, str8_lit("selected"), 0));
                      
                      // rjf: read project path
                      String8 project_path = str8_lit("");
                      {
                        MD_Node *project_node = md_child_from_string(op, str8_lit("project"), 0);
                        if(!md_node_is_nil(project_node))
                        {
                          project_path = path_absolute_dst_from_relative_dst_src(scratch.arena, project_node->first->string, cfg_folder);
                        }
                      }
                      
                      // rjf: read view query string
                      String8 view_query = str8_lit("");
                      {
                        String8 escaped_query = md_child_from_string(op, str8_lit("query"), 0)->first->string;
                        view_query = raw_from_escaped_str8(scratch.arena, escaped_query);
                      }
                      
                      // rjf: convert file queries from relative to absolute
                      {
                        String8 query_file_path = rd_file_path_from_eval_string(scratch.arena, view_query);
                        if(query_file_path.size != 0)
                        {
                          query_file_path = path_absolute_dst_from_relative_dst_src(scratch.arena, query_file_path, cfg_folder);
                          view_query = push_str8f(scratch.arena, "file:\"%S\"", query_file_path);
                        }
                      }
                      
                      // rjf: set up view
                      rd_view_equip_spec(view, view_rule_info, view_query, op);
                      if(project_path.size != 0)
                      {
                        arena_clear(view->project_path_arena);
                        view->project_path = push_str8_copy(view->project_path_arena, project_path);
                      }
                    }
                    
                    // rjf: insert
                    if(!rd_view_is_nil(view))
                    {
                      rd_panel_insert_tab_view(panel, panel->last_tab_view, view);
                      if(view_is_selected)
                      {
                        selected_view = view;
                      }
                    }
                  }
                  
                  // rjf: select selected view
                  if(!rd_view_is_nil(selected_view))
                  {
                    panel->selected_tab_view = rd_handle_from_view(selected_view);
                  }
                  
                  // rjf: recurse from this panel
                  if(has_panel_children)
                  {
                    next = n->first;
                    panel_parent = panel;
                  }
                  else for(MD_Node *p = n;
                           p != &md_nil_node && p != panel_tree;
                           p = p->parent, panel_parent = panel_parent->parent)
                  {
                    if(p->next != &md_nil_node)
                    {
                      next = p->next;
                      break;
                    }
                  }
                }
              }
              
              // rjf: initiate fullscreen
              if(is_fullscreen)
              {
                os_window_set_fullscreen(ws->os, 1);
              }
              
              // rjf: initiate maximize
              if(is_maximized)
              {
                os_window_set_maximized(ws->os, 1);
              }
              
              // rjf: focus the biggest panel
              {
                RD_Panel *best_leaf_panel = &rd_nil_panel;
                F32 best_leaf_panel_area = 0;
                Rng2F32 root_rect = r2f32p(0, 0, 1000, 1000); // NOTE(rjf): we can assume any size - just need proportions.
                for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
                {
                  if(rd_panel_is_nil(panel->first))
                  {
                    Rng2F32 rect = rd_target_rect_from_panel(root_rect, ws->root_panel, panel);
                    Vec2F32 dim = dim_2f32(rect);
                    F32 area = dim.x*dim.y;
                    if(best_leaf_panel_area == 0 || area > best_leaf_panel_area)
                    {
                      best_leaf_panel_area = area;
                      best_leaf_panel = panel;
                    }
                  }
                }
                ws->focused_panel = best_leaf_panel;
              }
            }
            
            //- rjf: apply keybindings
            if(src == RD_CfgSrc_User)
            {
              rd_clear_bindings();
            }
            RD_CfgVal *keybindings = rd_cfg_val_from_string(table, str8_lit("keybindings"));
            for(RD_CfgTree *keybinding_set = keybindings->first;
                keybinding_set != &d_nil_cfg_tree;
                keybinding_set = keybinding_set->next)
            {
              for MD_EachNode(keybind, keybinding_set->root->first)
              {
                String8 cmd_name = {0};
                OS_Key key = OS_Key_Null;
                MD_Node *ctrl_node = &md_nil_node;
                MD_Node *shift_node = &md_nil_node;
                MD_Node *alt_node = &md_nil_node;
                for MD_EachNode(child, keybind->first)
                {
                  if(str8_match(child->string, str8_lit("ctrl"), 0))
                  {
                    ctrl_node = child;
                  }
                  else if(str8_match(child->string, str8_lit("shift"), 0))
                  {
                    shift_node = child;
                  }
                  else if(str8_match(child->string, str8_lit("alt"), 0))
                  {
                    alt_node = child;
                  }
                  else
                  {
                    OS_Key k = rd_os_key_from_cfg_string(child->string);
                    if(k != OS_Key_Null)
                    {
                      key = k;
                    }
                    else
                    {
                      cmd_name = child->string;
                      for(U64 idx = 0; idx < ArrayCount(rd_binding_version_remap_old_name_table); idx += 1)
                      {
                        if(str8_match(rd_binding_version_remap_old_name_table[idx], child->string, StringMatchFlag_CaseInsensitive))
                        {
                          String8 new_name = rd_binding_version_remap_new_name_table[idx];
                          cmd_name = new_name;
                        }
                      }
                    }
                  }
                }
                if(cmd_name.size != 0 && key != OS_Key_Null)
                {
                  OS_Modifiers flags = 0;
                  if(!md_node_is_nil(ctrl_node))  { flags |= OS_Modifier_Ctrl; }
                  if(!md_node_is_nil(shift_node)) { flags |= OS_Modifier_Shift; }
                  if(!md_node_is_nil(alt_node))   { flags |= OS_Modifier_Alt; }
                  RD_Binding binding = {key, flags};
                  rd_bind_name(cmd_name, binding);
                }
              }
            }
            
            //- rjf: reset theme to default
            MemoryCopy(rd_state->cfg_theme_target.colors, rd_theme_preset_colors__default_dark, sizeof(rd_theme_preset_colors__default_dark));
            MemoryCopy(rd_state->cfg_theme.colors, rd_theme_preset_colors__default_dark, sizeof(rd_theme_preset_colors__default_dark));
            
            //- rjf: apply theme presets
            RD_CfgVal *color_preset = rd_cfg_val_from_string(table, str8_lit("color_preset"));
            B32 preset_applied = 0;
            if(color_preset != &d_nil_cfg_val)
            {
              String8 color_preset_name = color_preset->last->root->first->string;
              RD_ThemePreset preset = (RD_ThemePreset)0;
              B32 found_preset = 0;
              for(RD_ThemePreset p = (RD_ThemePreset)0; p < RD_ThemePreset_COUNT; p = (RD_ThemePreset)(p+1))
              {
                if(str8_match(color_preset_name, rd_theme_preset_code_string_table[p], StringMatchFlag_CaseInsensitive))
                {
                  found_preset = 1;
                  preset = p;
                  break;
                }
              }
              if(found_preset)
              {
                preset_applied = 1;
                MemoryCopy(rd_state->cfg_theme_target.colors, rd_theme_preset_colors_table[preset], sizeof(rd_theme_preset_colors__default_dark));
                MemoryCopy(rd_state->cfg_theme.colors, rd_theme_preset_colors_table[preset], sizeof(rd_theme_preset_colors__default_dark));
              }
            }
            
            //- rjf: apply individual theme colors
            B8 theme_color_hit[RD_ThemeColor_COUNT] = {0};
            RD_CfgVal *colors = rd_cfg_val_from_string(table, str8_lit("colors"));
            for(RD_CfgTree *colors_set = colors->first;
                colors_set != &d_nil_cfg_tree;
                colors_set = colors_set->next)
            {
              for MD_EachNode(color, colors_set->root->first)
              {
                String8 saved_color_name = color->string;
                String8List candidate_color_names = {0};
                str8_list_push(scratch.arena, &candidate_color_names, saved_color_name);
                for(U64 idx = 0; idx < ArrayCount(rd_theme_color_version_remap_old_name_table); idx += 1)
                {
                  if(str8_match(rd_theme_color_version_remap_old_name_table[idx], saved_color_name, StringMatchFlag_CaseInsensitive))
                  {
                    str8_list_push(scratch.arena, &candidate_color_names, rd_theme_color_version_remap_new_name_table[idx]);
                  }
                }
                for(String8Node *name_n = candidate_color_names.first; name_n != 0; name_n = name_n->next)
                {
                  String8 name = name_n->string;
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
                    theme_color_hit[color_code] = 1;
                    MD_Node *hex_cfg = color->first;
                    String8 hex_string = hex_cfg->string;
                    U64 hex_val = 0;
                    try_u64_from_str8_c_rules(hex_string, &hex_val);
                    Vec4F32 color_rgba = rgba_from_u32((U32)hex_val);
                    rd_state->cfg_theme_target.colors[color_code] = color_rgba;
                    if(rd_state->frame_index <= 2)
                    {
                      rd_state->cfg_theme.colors[color_code] = color_rgba;
                    }
                  }
                }
              }
            }
            
            //- rjf: no preset -> autofill all missing colors from the preset with the most similar background
            if(!preset_applied)
            {
              RD_ThemePreset closest_preset = RD_ThemePreset_DefaultDark;
              F32 closest_preset_bg_distance = 100000000;
              for(RD_ThemePreset p = (RD_ThemePreset)0; p < RD_ThemePreset_COUNT; p = (RD_ThemePreset)(p+1))
              {
                Vec4F32 cfg_bg = rd_state->cfg_theme_target.colors[RD_ThemeColor_BaseBackground];
                Vec4F32 pre_bg = rd_theme_preset_colors_table[p][RD_ThemeColor_BaseBackground];
                Vec4F32 diff = sub_4f32(cfg_bg, pre_bg);
                Vec3F32 diff3 = diff.xyz;
                F32 distance = length_3f32(diff3);
                if(distance < closest_preset_bg_distance)
                {
                  closest_preset = p;
                  closest_preset_bg_distance = distance;
                }
              }
              for(RD_ThemeColor c = (RD_ThemeColor)(RD_ThemeColor_Null+1);
                  c < RD_ThemeColor_COUNT;
                  c = (RD_ThemeColor)(c+1))
              {
                if(!theme_color_hit[c])
                {
                  rd_state->cfg_theme_target.colors[c] = rd_state->cfg_theme.colors[c] = rd_theme_preset_colors_table[closest_preset][c];
                }
              }
            }
            
            //- rjf: if theme colors are all zeroes, then set to default - config appears busted
            {
              B32 all_colors_are_zero = 1;
              Vec4F32 zero_color = {0};
              for(RD_ThemeColor c = (RD_ThemeColor)(RD_ThemeColor_Null+1); c < RD_ThemeColor_COUNT; c = (RD_ThemeColor)(c+1))
              {
                if(!MemoryMatchStruct(&rd_state->cfg_theme_target.colors[c], &zero_color))
                {
                  all_colors_are_zero = 0;
                  break;
                }
              }
              if(all_colors_are_zero)
              {
                MemoryCopy(rd_state->cfg_theme_target.colors, rd_theme_preset_colors__default_dark, sizeof(rd_theme_preset_colors__default_dark));
                MemoryCopy(rd_state->cfg_theme.colors, rd_theme_preset_colors__default_dark, sizeof(rd_theme_preset_colors__default_dark));
              }
            }
            
            //- rjf: apply settings
            B8 setting_codes_hit[RD_SettingCode_COUNT] = {0};
            MemoryZero(&rd_state->cfg_setting_vals[src][0], sizeof(RD_SettingVal)*RD_SettingCode_COUNT);
            for EachEnumVal(RD_SettingCode, code)
            {
              String8 name = rd_setting_code_lower_string_table[code];
              RD_CfgVal *code_cfg_val = rd_cfg_val_from_string(table, name);
              RD_CfgTree *code_tree = code_cfg_val->last;
              if(code_tree->source == src)
              {
                MD_Node *val_node = code_tree->root->first;
                S64 val = 0;
                if(try_s64_from_str8_c_rules(val_node->string, &val))
                {
                  rd_state->cfg_setting_vals[src][code].set = 1;
                  rd_state->cfg_setting_vals[src][code].s32 = (S32)val;
                }
                setting_codes_hit[code] = !md_node_is_nil(val_node);
              }
            }
            
            //- rjf: if config applied 0 settings, we need to do some sensible default
            if(src == RD_CfgSrc_User)
            {
              for EachEnumVal(RD_SettingCode, code)
              {
                if(!setting_codes_hit[code])
                {
                  rd_state->cfg_setting_vals[src][code] = rd_setting_code_default_val_table[code];
                }
              }
            }
            
            //- rjf: if config opened 0 windows, we need to do some sensible default
            if(src == RD_CfgSrc_User && windows->first == &d_nil_cfg_tree)
            {
              OS_Handle preferred_monitor = os_primary_monitor();
              Vec2F32 monitor_dim = os_dim_from_monitor(preferred_monitor);
              Vec2F32 window_dim = v2f32(monitor_dim.x*4/5, monitor_dim.y*4/5);
              RD_Window *ws = rd_window_open(window_dim, preferred_monitor, RD_CfgSrc_User);
              if(monitor_dim.x < 1920)
              {
                rd_cmd(RD_CmdKind_ResetToCompactPanels, .window = rd_handle_from_window(ws));
              }
              else
              {
                rd_cmd(RD_CmdKind_ResetToDefaultPanels, .window = rd_handle_from_window(ws));
              }
            }
            
            //- rjf: if config bound 0 keys, we need to do some sensible default
            if(src == RD_CfgSrc_User && rd_state->key_map_total_count == 0)
            {
              for(U64 idx = 0; idx < ArrayCount(rd_default_binding_table); idx += 1)
              {
                RD_StringBindingPair *pair = &rd_default_binding_table[idx];
                rd_bind_name(pair->string, pair->binding);
              }
            }
            
            //- rjf: always ensure that the meta controls have bindings
            if(src == RD_CfgSrc_User)
            {
              struct
              {
                String8 name;
                OS_Key fallback_key;
              }
              meta_ctrls[] =
              {
                { rd_cmd_kind_info_table[RD_CmdKind_Edit].string, OS_Key_F2 },
                { rd_cmd_kind_info_table[RD_CmdKind_Accept].string, OS_Key_Return },
                { rd_cmd_kind_info_table[RD_CmdKind_Cancel].string, OS_Key_Esc },
              };
              for(U64 idx = 0; idx < ArrayCount(meta_ctrls); idx += 1)
              {
                RD_BindingList bindings = rd_bindings_from_name(scratch.arena, meta_ctrls[idx].name);
                if(bindings.count == 0)
                {
                  RD_Binding binding = {meta_ctrls[idx].fallback_key, 0};
                  rd_bind_name(meta_ctrls[idx].name, binding);
                }
              }
            }
          }break;
          
          //- rjf: writing config changes
          case RD_CmdKind_WriteUserData:
          case RD_CmdKind_WriteProjectData:
          {
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
            RD_Window *window = rd_window_from_handle(rd_regs()->window);
            if(window != 0)
            {
              window->setting_vals[RD_SettingCode_MainFontSize].set = 1;
              window->setting_vals[RD_SettingCode_MainFontSize].s32 += 1;
              window->setting_vals[RD_SettingCode_MainFontSize].s32 = clamp_1s32(rd_setting_code_s32_range_table[RD_SettingCode_MainFontSize], window->setting_vals[RD_SettingCode_MainFontSize].s32);
            }
          }break;
          case RD_CmdKind_DecUIFontScale:
          {
            RD_Window *window = rd_window_from_handle(rd_regs()->window);
            if(window != 0)
            {
              window->setting_vals[RD_SettingCode_MainFontSize].set = 1;
              window->setting_vals[RD_SettingCode_MainFontSize].s32 -= 1;
              window->setting_vals[RD_SettingCode_MainFontSize].s32 = clamp_1s32(rd_setting_code_s32_range_table[RD_SettingCode_MainFontSize], window->setting_vals[RD_SettingCode_MainFontSize].s32);
            }
          }break;
          case RD_CmdKind_IncCodeFontScale:
          {
            RD_Window *window = rd_window_from_handle(rd_regs()->window);
            if(window != 0)
            {
              window->setting_vals[RD_SettingCode_CodeFontSize].set = 1;
              window->setting_vals[RD_SettingCode_CodeFontSize].s32 += 1;
              window->setting_vals[RD_SettingCode_CodeFontSize].s32 = clamp_1s32(rd_setting_code_s32_range_table[RD_SettingCode_CodeFontSize], window->setting_vals[RD_SettingCode_CodeFontSize].s32);
            }
          }break;
          case RD_CmdKind_DecCodeFontScale:
          {
            RD_Window *window = rd_window_from_handle(rd_regs()->window);
            if(window != 0)
            {
              window->setting_vals[RD_SettingCode_CodeFontSize].set = 1;
              window->setting_vals[RD_SettingCode_CodeFontSize].s32 -= 1;
              window->setting_vals[RD_SettingCode_CodeFontSize].s32 = clamp_1s32(rd_setting_code_s32_range_table[RD_SettingCode_CodeFontSize], window->setting_vals[RD_SettingCode_CodeFontSize].s32);
            }
          }break;
          
          //- rjf: panel creation
          case RD_CmdKind_NewPanelLeft: {split_dir = Dir2_Left;}goto split;
          case RD_CmdKind_NewPanelUp:   {split_dir = Dir2_Up;}goto split;
          case RD_CmdKind_NewPanelRight:{split_dir = Dir2_Right;}goto split;
          case RD_CmdKind_NewPanelDown: {split_dir = Dir2_Down;}goto split;
          case RD_CmdKind_SplitPanel:
          {
            split_dir = rd_regs()->dir2;
            split_panel = rd_panel_from_handle(rd_regs()->dst_panel);
          }goto split;
          split:;
          if(split_dir != Dir2_Invalid)
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            if(rd_panel_is_nil(split_panel))
            {
              split_panel = ws->focused_panel;
            }
            RD_Panel *new_panel = &rd_nil_panel;
            Axis2 split_axis = axis2_from_dir2(split_dir);
            Side split_side = side_from_dir2(split_dir);
            RD_Panel *panel = split_panel;
            RD_Panel *parent = panel->parent;
            if(!rd_panel_is_nil(parent) && parent->split_axis == split_axis)
            {
              RD_Panel *next = rd_panel_alloc(ws);
              rd_panel_insert(parent, split_side == Side_Max ? panel : panel->prev, next);
              next->pct_of_parent = 1.f/parent->child_count;
              for(RD_Panel *child = parent->first; !rd_panel_is_nil(child); child = child->next)
              {
                if(child != next)
                {
                  child->pct_of_parent *= (F32)(parent->child_count-1) / (parent->child_count);
                }
              }
              ws->focused_panel = next;
              new_panel = next;
            }
            else
            {
              RD_Panel *pre_prev = panel->prev;
              RD_Panel *pre_parent = parent;
              RD_Panel *new_parent = rd_panel_alloc(ws);
              new_parent->pct_of_parent = panel->pct_of_parent;
              if(!rd_panel_is_nil(pre_parent))
              {
                rd_panel_remove(pre_parent, panel);
                rd_panel_insert(pre_parent, pre_prev, new_parent);
              }
              else
              {
                ws->root_panel = new_parent;
              }
              RD_Panel *left = panel;
              RD_Panel *right = rd_panel_alloc(ws);
              new_panel = right;
              if(split_side == Side_Min)
              {
                Swap(RD_Panel *, left, right);
              }
              rd_panel_insert(new_parent, &rd_nil_panel, left);
              rd_panel_insert(new_parent, left, right);
              new_parent->split_axis = split_axis;
              left->pct_of_parent = 0.5f;
              right->pct_of_parent = 0.5f;
              ws->focused_panel = new_panel;
            }
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
            RD_Panel *move_tab_panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *move_tab = rd_view_from_handle(rd_regs()->view);
            if(!rd_panel_is_nil(new_panel) && !rd_view_is_nil(move_tab) && !rd_panel_is_nil(move_tab_panel) &&
               kind == RD_CmdKind_SplitPanel)
            {
              rd_panel_remove_tab_view(move_tab_panel, move_tab);
              rd_panel_insert_tab_view(new_panel, new_panel->last_tab_view, move_tab);
              new_panel->selected_tab_view = rd_handle_from_view(move_tab);
              B32 move_tab_panel_is_empty = 1;
              for(RD_View *v = move_tab_panel->first_tab_view; !rd_view_is_nil(v); v = v->order_next)
              {
                if(!rd_view_is_project_filtered(v))
                {
                  move_tab_panel_is_empty = 0;
                  break;
                }
              }
              if(move_tab_panel_is_empty && move_tab_panel != ws->root_panel &&
                 move_tab_panel != new_panel->prev && move_tab_panel != new_panel->next)
              {
                rd_cmd(RD_CmdKind_ClosePanel, .panel = rd_handle_from_panel(move_tab_panel));
              }
            }
          }break;
          
          //- rjf: panel rotation
          case RD_CmdKind_RotatePanelColumns:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *panel = ws->focused_panel;
            RD_Panel *parent = &rd_nil_panel;
            for(RD_Panel *p = panel->parent; !rd_panel_is_nil(p); p = p->parent)
            {
              if(p->split_axis == Axis2_X)
              {
                parent = p;
                break;
              }
            }
            if(!rd_panel_is_nil(parent) && parent->child_count > 1)
            {
              RD_Panel *old_first = parent->first;
              RD_Panel *new_first = parent->first->next;
              old_first->next = &rd_nil_panel;
              old_first->prev = parent->last;
              parent->last->next = old_first;
              new_first->prev = &rd_nil_panel;
              parent->first = new_first;
              parent->last = old_first;
            }
          }break;
          
          //- rjf: panel focusing
          case RD_CmdKind_NextPanel: panel_sib_off = OffsetOf(RD_Panel, next); panel_child_off = OffsetOf(RD_Panel, first); goto cycle;
          case RD_CmdKind_PrevPanel: panel_sib_off = OffsetOf(RD_Panel, prev); panel_child_off = OffsetOf(RD_Panel, last); goto cycle;
          cycle:;
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            for(RD_Panel *panel = ws->focused_panel; !rd_panel_is_nil(panel);)
            {
              RD_PanelRec rec = rd_panel_rec_depth_first(panel, panel_sib_off, panel_child_off);
              panel = rec.next;
              if(rd_panel_is_nil(panel))
              {
                panel = ws->root_panel;
              }
              if(rd_panel_is_nil(panel->first))
              {
                rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(panel));
                break;
              }
            }
          }break;
          case RD_CmdKind_FocusPanel:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            if(!rd_panel_is_nil(panel))
            {
              ws->focused_panel = panel;
              ws->menu_bar_focused = 0;
              ws->query_view_selected = 0;
            }
          }break;
          
          //- rjf: directional panel focus changing
          case RD_CmdKind_FocusPanelRight: panel_change_dir = v2s32(+1, +0); goto focus_panel_dir;
          case RD_CmdKind_FocusPanelLeft:  panel_change_dir = v2s32(-1, +0); goto focus_panel_dir;
          case RD_CmdKind_FocusPanelUp:    panel_change_dir = v2s32(+0, -1); goto focus_panel_dir;
          case RD_CmdKind_FocusPanelDown:  panel_change_dir = v2s32(+0, +1); goto focus_panel_dir;
          focus_panel_dir:;
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            RD_Panel *src_panel = ws->focused_panel;
            Rng2F32 src_panel_rect = rd_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, src_panel);
            Vec2F32 src_panel_center = center_2f32(src_panel_rect);
            Vec2F32 src_panel_half_dim = scale_2f32(dim_2f32(src_panel_rect), 0.5f);
            Vec2F32 travel_dim = add_2f32(src_panel_half_dim, v2f32(10.f, 10.f));
            Vec2F32 travel_dst = add_2f32(src_panel_center, mul_2f32(travel_dim, v2f32((F32)panel_change_dir.x, (F32)panel_change_dir.y)));
            RD_Panel *dst_root = &rd_nil_panel;
            for(RD_Panel *p = ws->root_panel; !rd_panel_is_nil(p); p = rd_panel_rec_depth_first_pre(p).next)
            {
              if(p == src_panel || !rd_panel_is_nil(p->first))
              {
                continue;
              }
              Rng2F32 p_rect = rd_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, p);
              if(contains_2f32(p_rect, travel_dst))
              {
                dst_root = p;
                break;
              }
            }
            if(!rd_panel_is_nil(dst_root))
            {
              RD_Panel *dst_panel = &rd_nil_panel;
              for(RD_Panel *p = dst_root; !rd_panel_is_nil(p); p = rd_panel_rec_depth_first_pre(p).next)
              {
                if(rd_panel_is_nil(p->first) && p != src_panel)
                {
                  dst_panel = p;
                  break;
                }
              }
              rd_cmd(RD_CmdKind_FocusPanel, .panel = rd_handle_from_panel(dst_panel));
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
            
            //- rjf: store as file path map entity
            RD_Entity *map = rd_entity_alloc(rd_entity_root(), RD_EntityKind_FilePathMap);
            RD_Entity *src = rd_entity_alloc(map, RD_EntityKind_Source);
            RD_Entity *dst = rd_entity_alloc(map, RD_EntityKind_Dest);
            rd_entity_equip_name(src, map_src);
            rd_entity_equip_name(dst, map_dst);
          }break;
          
          //- rjf: panel removal
          case RD_CmdKind_ClosePanel:
          {
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
          }break;
          
          //- rjf: panel tab controls
          case RD_CmdKind_NextTab:
          {
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *start_view = rd_selected_tab_from_panel(panel);
            RD_View *next_view = start_view;
            U64 idx = 0;
            for(RD_View *v = start_view; !rd_view_is_nil(v); v = (rd_view_is_nil(v->order_next) ? panel->first_tab_view : v->order_next), idx += 1)
            {
              if(v == start_view && idx != 0)
              {
                break;
              }
              if(!rd_view_is_project_filtered(v) && v != start_view)
              {
                next_view = v;
                break;
              }
            }
            panel->selected_tab_view = rd_handle_from_view(next_view);
          }break;
          case RD_CmdKind_PrevTab:
          {
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *start_view = rd_selected_tab_from_panel(panel);
            RD_View *next_view = start_view;
            U64 idx = 0;
            for(RD_View *v = start_view; !rd_view_is_nil(v); v = (rd_view_is_nil(v->order_prev) ? panel->last_tab_view : v->order_prev), idx += 1)
            {
              if(v == start_view && idx != 0)
              {
                break;
              }
              if(!rd_view_is_project_filtered(v) && v != start_view)
              {
                next_view = v;
                break;
              }
            }
            panel->selected_tab_view = rd_handle_from_view(next_view);
          }break;
          case RD_CmdKind_MoveTabRight:
          case RD_CmdKind_MoveTabLeft:
          {
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
          }break;
          case RD_CmdKind_OpenTab:
          {
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *view = rd_view_alloc();
            String8 query = rd_regs()->string;
            RD_ViewRuleInfo *spec = rd_view_rule_info_from_string(rd_regs()->params_tree->string);
            rd_view_equip_spec(view, spec, query, rd_regs()->params_tree);
            rd_panel_insert_tab_view(panel, panel->last_tab_view, view);
          }break;
          case RD_CmdKind_CloseTab:
          {
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            if(!rd_view_is_nil(view))
            {
              rd_panel_remove_tab_view(panel, view);
              rd_view_release(view);
            }
          }break;
          case RD_CmdKind_MoveTab:
          {
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
          }break;
          case RD_CmdKind_TabBarTop:
          {
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            panel->tab_side = Side_Min;
          }break;
          case RD_CmdKind_TabBarBottom:
          {
            RD_Panel *panel = rd_panel_from_handle(rd_regs()->panel);
            panel->tab_side = Side_Max;
          }break;
          
          //- rjf: files
          case RD_CmdKind_Open:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            String8 path = rd_regs()->file_path;
            FileProperties props = os_properties_from_file_path(path);
            if(props.created != 0)
            {
              rd_cmd(RD_CmdKind_RecordFileInProject);
              rd_cmd(RD_CmdKind_OpenTab,
                     .string = rd_eval_string_from_file_path(scratch.arena, path),
                     .params_tree = md_tree_from_string(scratch.arena, rd_view_rule_kind_info_table[RD_ViewRuleKind_PendingFile].string)->first);
            }
            else
            {
              log_user_errorf("Couldn't open file at \"%S\".", path);
            }
          }break;
          case RD_CmdKind_Switch:
          {
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
          }break;
          case RD_CmdKind_SwitchToPartnerFile:
          {
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
          }break;
          case RD_CmdKind_RecordFileInProject:
          if(rd_regs()->file_path.size != 0)
          {
            String8 path = path_normalized_from_string(scratch.arena, rd_regs()->file_path);
            RD_EntityList recent_files = rd_query_cached_entity_list_with_kind(RD_EntityKind_RecentFile);
            if(recent_files.count >= 256)
            {
              rd_entity_mark_for_deletion(recent_files.first->entity);
            }
            RD_Entity *existing_recent_file = &d_nil_entity;
            for(RD_EntityNode *n = recent_files.first; n != 0; n = n->next)
            {
              if(str8_match(n->entity->string, path, StringMatchFlag_CaseInsensitive))
              {
                existing_recent_file = n->entity;
                break;
              }
            }
            if(rd_entity_is_nil(existing_recent_file))
            {
              RD_Entity *recent_file = rd_entity_alloc(rd_entity_root(), RD_EntityKind_RecentFile);
              rd_entity_equip_name(recent_file, path);
              rd_entity_equip_cfg_src(recent_file, RD_CfgSrc_Project);
            }
            else
            {
              rd_entity_change_parent(existing_recent_file, rd_entity_root(), rd_entity_root(), rd_entity_root()->last);
            }
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
            rd_cmd(RD_CmdKind_FindCodeLocation, .vaddr = vaddr);
          }break;
          case RD_CmdKind_GoToSource:
          {
            if(rd_regs()->lines.first != 0)
            {
              rd_cmd(RD_CmdKind_FindCodeLocation,
                     .file_path = rd_regs()->lines.first->v.file_path,
                     .cursor    = rd_regs()->lines.first->v.pt,
                     .vaddr     = 0,
                     .process   = ctrl_handle_zero());
            }
          }break;
          
          //- rjf: panel built-in layout builds
          case RD_CmdKind_ResetToDefaultPanels:
          case RD_CmdKind_ResetToCompactPanels:
          {
            panel_reset_done = 1;
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            
            typedef enum Layout
            {
              Layout_Default,
              Layout_Compact,
            }
            Layout;
            Layout layout = Layout_Default;
            switch(kind)
            {
              default:{}break;
              case RD_CmdKind_ResetToDefaultPanels:{layout = Layout_Default;}break;
              case RD_CmdKind_ResetToCompactPanels:{layout = Layout_Compact;}break;
            }
            
            //- rjf: gather all panels in the panel tree - remove & gather views
            // we'd like to keep in the next layout
            RD_HandleList panels_to_close = {0};
            RD_HandleList views_to_close = {0};
            RD_View *watch = &rd_nil_view;
            RD_View *locals = &rd_nil_view;
            RD_View *regs = &rd_nil_view;
            RD_View *globals = &rd_nil_view;
            RD_View *tlocals = &rd_nil_view;
            RD_View *types = &rd_nil_view;
            RD_View *procs = &rd_nil_view;
            RD_View *callstack = &rd_nil_view;
            RD_View *breakpoints = &rd_nil_view;
            RD_View *watch_pins = &rd_nil_view;
            RD_View *output = &rd_nil_view;
            RD_View *targets = &rd_nil_view;
            RD_View *scheduler = &rd_nil_view;
            RD_View *modules = &rd_nil_view;
            RD_View *disasm = &rd_nil_view;
            RD_View *memory = &rd_nil_view;
            RD_View *getting_started = &rd_nil_view;
            RD_HandleList code_views = {0};
            for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
            {
              RD_Handle handle = rd_handle_from_panel(panel);
              rd_handle_list_push(scratch.arena, &panels_to_close, handle);
              for(RD_View *view = panel->first_tab_view, *next = 0; !rd_view_is_nil(view); view = next)
              {
                next = view->order_next;
                RD_ViewRuleKind view_rule_kind = rd_view_rule_kind_from_string(view->spec->string);
                B32 needs_delete = 1;
                switch(view_rule_kind)
                {
                  default:{}break;
                  case RD_ViewRuleKind_Watch:         {if(rd_view_is_nil(watch))               { needs_delete = 0; watch = view;} }break;
                  case RD_ViewRuleKind_Locals:        {if(rd_view_is_nil(locals))              { needs_delete = 0; locals = view;} }break;
                  case RD_ViewRuleKind_Registers:     {if(rd_view_is_nil(regs))                { needs_delete = 0; regs = view;} }break;
                  case RD_ViewRuleKind_Globals:       {if(rd_view_is_nil(globals))             { needs_delete = 0; globals = view;} }break;
                  case RD_ViewRuleKind_ThreadLocals:  {if(rd_view_is_nil(tlocals))             { needs_delete = 0; tlocals = view;} }break;
                  case RD_ViewRuleKind_Types:         {if(rd_view_is_nil(types))               { needs_delete = 0; types = view;} }break;
                  case RD_ViewRuleKind_Procedures:    {if(rd_view_is_nil(procs))               { needs_delete = 0; procs = view;} }break;
                  case RD_ViewRuleKind_CallStack:     {if(rd_view_is_nil(callstack))           { needs_delete = 0; callstack = view;} }break;
                  case RD_ViewRuleKind_Breakpoints:   {if(rd_view_is_nil(breakpoints))         { needs_delete = 0; breakpoints = view;} }break;
                  case RD_ViewRuleKind_WatchPins:     {if(rd_view_is_nil(watch_pins))          { needs_delete = 0; watch_pins = view;} }break;
                  case RD_ViewRuleKind_Output:        {if(rd_view_is_nil(output))              { needs_delete = 0; output = view;} }break;
                  case RD_ViewRuleKind_Targets:       {if(rd_view_is_nil(targets))             { needs_delete = 0; targets = view;} }break;
                  case RD_ViewRuleKind_Scheduler:     {if(rd_view_is_nil(scheduler))           { needs_delete = 0; scheduler = view;} }break;
                  case RD_ViewRuleKind_Modules:       {if(rd_view_is_nil(modules))             { needs_delete = 0; modules = view;} }break;
                  case RD_ViewRuleKind_Disasm:        {if(rd_view_is_nil(disasm))              { needs_delete = 0; disasm = view;} }break;
                  case RD_ViewRuleKind_Memory:        {if(rd_view_is_nil(memory))              { needs_delete = 0; memory = view;} }break;
                  case RD_ViewRuleKind_GettingStarted:{if(rd_view_is_nil(getting_started))     { needs_delete = 0; getting_started = view;} }break;
                  case RD_ViewRuleKind_Text:
                  {
                    needs_delete = 0;
                    rd_handle_list_push(scratch.arena, &code_views, rd_handle_from_view(view));
                  }break;
                }
                if(!needs_delete)
                {
                  rd_panel_remove_tab_view(panel, view);
                }
              }
            }
            
            //- rjf: close all panels/views
            for(RD_HandleNode *n = panels_to_close.first; n != 0; n = n->next)
            {
              RD_Panel *panel = rd_panel_from_handle(n->handle);
              if(panel != ws->root_panel)
              {
                rd_panel_release(ws, panel);
              }
              else
              {
                rd_panel_release_all_views(panel);
                panel->first = panel->last = &rd_nil_panel;
              }
            }
            
            //- rjf: allocate any missing views
            if(rd_view_is_nil(watch))
            {
              watch = rd_view_alloc();
              rd_view_equip_spec(watch, rd_view_rule_info_from_kind(RD_ViewRuleKind_Watch), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(locals))
            {
              locals = rd_view_alloc();
              rd_view_equip_spec(locals, rd_view_rule_info_from_kind(RD_ViewRuleKind_Locals), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(regs))
            {
              regs = rd_view_alloc();
              rd_view_equip_spec(regs, rd_view_rule_info_from_kind(RD_ViewRuleKind_Registers), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(globals))
            {
              globals = rd_view_alloc();
              rd_view_equip_spec(globals, rd_view_rule_info_from_kind(RD_ViewRuleKind_Globals), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(tlocals))
            {
              tlocals = rd_view_alloc();
              rd_view_equip_spec(tlocals, rd_view_rule_info_from_kind(RD_ViewRuleKind_ThreadLocals), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(types))
            {
              types = rd_view_alloc();
              rd_view_equip_spec(types, rd_view_rule_info_from_kind(RD_ViewRuleKind_Types), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(procs))
            {
              procs = rd_view_alloc();
              rd_view_equip_spec(procs, rd_view_rule_info_from_kind(RD_ViewRuleKind_Procedures), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(callstack))
            {
              callstack = rd_view_alloc();
              rd_view_equip_spec(callstack, rd_view_rule_info_from_kind(RD_ViewRuleKind_CallStack), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(breakpoints))
            {
              breakpoints = rd_view_alloc();
              rd_view_equip_spec(breakpoints, rd_view_rule_info_from_kind(RD_ViewRuleKind_Breakpoints), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(watch_pins))
            {
              watch_pins = rd_view_alloc();
              rd_view_equip_spec(watch_pins, rd_view_rule_info_from_kind(RD_ViewRuleKind_WatchPins), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(output))
            {
              output = rd_view_alloc();
              rd_view_equip_spec(output, rd_view_rule_info_from_kind(RD_ViewRuleKind_Output), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(targets))
            {
              targets = rd_view_alloc();
              rd_view_equip_spec(targets, rd_view_rule_info_from_kind(RD_ViewRuleKind_Targets), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(scheduler))
            {
              scheduler = rd_view_alloc();
              rd_view_equip_spec(scheduler, rd_view_rule_info_from_kind(RD_ViewRuleKind_Scheduler), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(modules))
            {
              modules = rd_view_alloc();
              rd_view_equip_spec(modules, rd_view_rule_info_from_kind(RD_ViewRuleKind_Modules), str8_zero(), &md_nil_node);
            }
            if(rd_view_is_nil(disasm))
            {
              disasm = rd_view_alloc();
              rd_view_equip_spec(disasm, rd_view_rule_info_from_kind(RD_ViewRuleKind_Disasm), str8_zero(), &md_nil_node);
            }
            if(layout == Layout_Default && rd_view_is_nil(memory))
            {
              memory = rd_view_alloc();
              rd_view_equip_spec(memory, rd_view_rule_info_from_kind(RD_ViewRuleKind_Memory), str8_zero(), &md_nil_node);
            }
            if(code_views.count == 0 && rd_view_is_nil(getting_started))
            {
              getting_started = rd_view_alloc();
              rd_view_equip_spec(getting_started, rd_view_rule_info_from_kind(RD_ViewRuleKind_GettingStarted), str8_zero(), &md_nil_node);
            }
            
            //- rjf: apply layout
            switch(layout)
            {
              //- rjf: default layout
              case Layout_Default:
              {
                // rjf: root split
                ws->root_panel->split_axis = Axis2_X;
                RD_Panel *root_0 = rd_panel_alloc(ws);
                RD_Panel *root_1 = rd_panel_alloc(ws);
                rd_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
                rd_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
                root_0->pct_of_parent = 0.85f;
                root_1->pct_of_parent = 0.15f;
                
                // rjf: root_0 split
                root_0->split_axis = Axis2_Y;
                RD_Panel *root_0_0 = rd_panel_alloc(ws);
                RD_Panel *root_0_1 = rd_panel_alloc(ws);
                rd_panel_insert(root_0, root_0->last, root_0_0);
                rd_panel_insert(root_0, root_0->last, root_0_1);
                root_0_0->pct_of_parent = 0.80f;
                root_0_1->pct_of_parent = 0.20f;
                
                // rjf: root_1 split
                root_1->split_axis = Axis2_Y;
                RD_Panel *root_1_0 = rd_panel_alloc(ws);
                RD_Panel *root_1_1 = rd_panel_alloc(ws);
                rd_panel_insert(root_1, root_1->last, root_1_0);
                rd_panel_insert(root_1, root_1->last, root_1_1);
                root_1_0->pct_of_parent = 0.50f;
                root_1_1->pct_of_parent = 0.50f;
                rd_panel_insert_tab_view(root_1_0, root_1_0->last_tab_view, targets);
                rd_panel_insert_tab_view(root_1_1, root_1_1->last_tab_view, scheduler);
                root_1_0->selected_tab_view = rd_handle_from_view(targets);
                root_1_1->selected_tab_view = rd_handle_from_view(scheduler);
                root_1_1->tab_side = Side_Max;
                
                // rjf: root_0_0 split
                root_0_0->split_axis = Axis2_X;
                RD_Panel *root_0_0_0 = rd_panel_alloc(ws);
                RD_Panel *root_0_0_1 = rd_panel_alloc(ws);
                rd_panel_insert(root_0_0, root_0_0->last, root_0_0_0);
                rd_panel_insert(root_0_0, root_0_0->last, root_0_0_1);
                root_0_0_0->pct_of_parent = 0.25f;
                root_0_0_1->pct_of_parent = 0.75f;
                
                // rjf: root_0_0_0 split
                root_0_0_0->split_axis = Axis2_Y;
                RD_Panel *root_0_0_0_0 = rd_panel_alloc(ws);
                RD_Panel *root_0_0_0_1 = rd_panel_alloc(ws);
                rd_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_0);
                rd_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_1);
                root_0_0_0_0->pct_of_parent = 0.5f;
                root_0_0_0_1->pct_of_parent = 0.5f;
                rd_panel_insert_tab_view(root_0_0_0_0, root_0_0_0_0->last_tab_view, disasm);
                root_0_0_0_0->selected_tab_view = rd_handle_from_view(disasm);
                rd_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, breakpoints);
                rd_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, watch_pins);
                rd_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, output);
                rd_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, memory);
                root_0_0_0_1->selected_tab_view = rd_handle_from_view(output);
                
                // rjf: root_0_1 split
                root_0_1->split_axis = Axis2_X;
                RD_Panel *root_0_1_0 = rd_panel_alloc(ws);
                RD_Panel *root_0_1_1 = rd_panel_alloc(ws);
                rd_panel_insert(root_0_1, root_0_1->last, root_0_1_0);
                rd_panel_insert(root_0_1, root_0_1->last, root_0_1_1);
                root_0_1_0->pct_of_parent = 0.60f;
                root_0_1_1->pct_of_parent = 0.40f;
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, watch);
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, locals);
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, regs);
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, globals);
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, tlocals);
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, types);
                rd_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, procs);
                root_0_1_0->selected_tab_view = rd_handle_from_view(watch);
                root_0_1_0->tab_side = Side_Max;
                rd_panel_insert_tab_view(root_0_1_1, root_0_1_1->last_tab_view, callstack);
                rd_panel_insert_tab_view(root_0_1_1, root_0_1_1->last_tab_view, modules);
                root_0_1_1->selected_tab_view = rd_handle_from_view(callstack);
                root_0_1_1->tab_side = Side_Max;
                
                // rjf: fill main panel with getting started, OR all collected code views
                if(!rd_view_is_nil(getting_started))
                {
                  rd_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, getting_started);
                }
                for(RD_HandleNode *n = code_views.first; n != 0; n = n->next)
                {
                  RD_View *view = rd_view_from_handle(n->handle);
                  if(!rd_view_is_nil(view))
                  {
                    rd_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, view);
                  }
                }
                
                // rjf: choose initial focused panel
                ws->focused_panel = root_0_0_1;
              }break;
              
              //- rjf: compact layout:
              case Layout_Compact:
              {
                // rjf: root split
                ws->root_panel->split_axis = Axis2_X;
                RD_Panel *root_0 = rd_panel_alloc(ws);
                RD_Panel *root_1 = rd_panel_alloc(ws);
                rd_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
                rd_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
                root_0->pct_of_parent = 0.25f;
                root_1->pct_of_parent = 0.75f;
                
                // rjf: root_0 split
                root_0->split_axis = Axis2_Y;
                RD_Panel *root_0_0 = rd_panel_alloc(ws);
                {
                  if(!rd_view_is_nil(watch)) { rd_panel_insert_tab_view(root_0_0, root_0_0->last_tab_view, watch); }
                  if(!rd_view_is_nil(types)) { rd_panel_insert_tab_view(root_0_0, root_0_0->last_tab_view, types); }
                  root_0_0->selected_tab_view = rd_handle_from_view(watch);
                }
                RD_Panel *root_0_1 = rd_panel_alloc(ws);
                {
                  if(!rd_view_is_nil(scheduler))     { rd_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, scheduler); }
                  if(!rd_view_is_nil(targets))       { rd_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, targets); }
                  if(!rd_view_is_nil(breakpoints))   { rd_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, breakpoints); }
                  if(!rd_view_is_nil(watch_pins))    { rd_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, watch_pins); }
                  root_0_1->selected_tab_view = rd_handle_from_view(scheduler);
                }
                RD_Panel *root_0_2 = rd_panel_alloc(ws);
                {
                  if(!rd_view_is_nil(disasm))    { rd_panel_insert_tab_view(root_0_2, root_0_2->last_tab_view, disasm); }
                  if(!rd_view_is_nil(output))    { rd_panel_insert_tab_view(root_0_2, root_0_2->last_tab_view, output); }
                  root_0_2->selected_tab_view = rd_handle_from_view(disasm);
                }
                RD_Panel *root_0_3 = rd_panel_alloc(ws);
                {
                  if(!rd_view_is_nil(callstack))    { rd_panel_insert_tab_view(root_0_3, root_0_3->last_tab_view, callstack); }
                  if(!rd_view_is_nil(modules))      { rd_panel_insert_tab_view(root_0_3, root_0_3->last_tab_view, modules); }
                  root_0_3->selected_tab_view = rd_handle_from_view(callstack);
                }
                rd_panel_insert(root_0, root_0->last, root_0_0);
                rd_panel_insert(root_0, root_0->last, root_0_1);
                rd_panel_insert(root_0, root_0->last, root_0_2);
                rd_panel_insert(root_0, root_0->last, root_0_3);
                root_0_0->pct_of_parent = 0.25f;
                root_0_1->pct_of_parent = 0.25f;
                root_0_2->pct_of_parent = 0.25f;
                root_0_3->pct_of_parent = 0.25f;
                
                // rjf: fill main panel with getting started, OR all collected code views
                if(!rd_view_is_nil(getting_started))
                {
                  rd_panel_insert_tab_view(root_1, root_1->last_tab_view, getting_started);
                }
                for(RD_HandleNode *n = code_views.first; n != 0; n = n->next)
                {
                  RD_View *view = rd_view_from_handle(n->handle);
                  if(!rd_view_is_nil(view))
                  {
                    rd_panel_insert_tab_view(root_1, root_1->last_tab_view, view);
                  }
                }
                
                // rjf: choose initial focused panel
                ws->focused_panel = root_1;
              }break;
            }
            
            // rjf: dispatch cfg saves
            for(RD_CfgSrc src = (RD_CfgSrc)0; src < RD_CfgSrc_COUNT; src = (RD_CfgSrc)(src+1))
            {
              RD_CmdKind write_cmd = rd_cfg_src_write_cmd_kind_table[src];
              rd_cmd(write_cmd, .file_path = rd_cfg_path_from_src(src));
            }
          }break;
          
          //- rjf: thread finding
          case RD_CmdKind_FindThread:
          for(RD_Window *ws = rd_state->first_window; ws != 0; ws = ws->next)
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
          for(RD_Window *ws = rd_state->first_window; ws != 0; ws = ws->next)
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
                RD_View *src_view = rd_view_from_handle(rd_regs()->view);
                String8 src_file_path = rd_file_path_from_eval_string(scratch.arena, str8(src_view->query_buffer, src_view->query_string_size));
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
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            
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
            RD_Panel *panel_w_this_src_code = &rd_nil_panel;
            RD_View *view_w_this_src_code = &rd_nil_view;
            for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
            {
              if(!rd_panel_is_nil(panel->first))
              {
                continue;
              }
              for(RD_View *view = panel->first_tab_view; !rd_view_is_nil(view); view = view->order_next)
              {
                if(rd_view_is_project_filtered(view)) { continue; }
                String8 view_file_path = rd_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
                RD_ViewRuleKind view_kind = rd_view_rule_kind_from_string(view->spec->string);
                if((view_kind == RD_ViewRuleKind_Text || view_kind == RD_ViewRuleKind_PendingFile) &&
                   path_match_normalized(view_file_path, file_path))
                {
                  panel_w_this_src_code = panel;
                  view_w_this_src_code = view;
                  if(view == rd_selected_tab_from_panel(panel))
                  {
                    break;
                  }
                }
              }
            }
            
            // rjf: find a panel that already has *any* code open (prioritize largest)
            RD_Panel *panel_w_any_src_code = &rd_nil_panel;
            {
              Rng2F32 root_rect = os_client_rect_from_window(ws->os);
              F32 best_panel_area = 0;
              for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
              {
                if(!rd_panel_is_nil(panel->first))
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel(root_rect, ws->root_panel, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 panel_area = panel_rect_dim.x*panel_rect_dim.y;
                for(RD_View *view = panel->first_tab_view; !rd_view_is_nil(view); view = view->order_next)
                {
                  if(rd_view_is_project_filtered(view)) { continue; }
                  RD_ViewRuleKind view_kind = rd_view_rule_kind_from_string(view->spec->string);
                  if(view_kind == RD_ViewRuleKind_Text && panel_area > best_panel_area)
                  {
                    panel_w_any_src_code = panel;
                    best_panel_area = panel_area;
                    break;
                  }
                }
              }
            }
            
            // rjf: try to find panel/view pair that has disassembly open (prioritize largest)
            RD_Panel *panel_w_disasm = &rd_nil_panel;
            RD_View *view_w_disasm = &rd_nil_view;
            {
              Rng2F32 root_rect = os_client_rect_from_window(ws->os);
              F32 best_panel_area = 0;
              for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
              {
                if(!rd_panel_is_nil(panel->first))
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel(root_rect, ws->root_panel, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 panel_area = panel_rect_dim.x*panel_rect_dim.y;
                RD_View *panel_selected_tab = rd_selected_tab_from_panel(panel);
                for(RD_View *view = panel->first_tab_view; !rd_view_is_nil(view); view = view->order_next)
                {
                  if(rd_view_is_project_filtered(view)) { continue; }
                  RD_ViewRuleKind view_kind = rd_view_rule_kind_from_string(view->spec->string);
                  B32 view_is_selected = (view == panel_selected_tab);
                  if(view_kind == RD_ViewRuleKind_Disasm && view->query_string_size == 0 && panel_area > best_panel_area &&
                     (view_is_selected || require_disasm_snap))
                  {
                    panel_w_disasm = panel;
                    view_w_disasm = view;
                    best_panel_area = panel_area;
                    if(view_is_selected)
                    {
                      break;
                    }
                  }
                }
              }
            }
            
            // rjf: find the biggest panel
            RD_Panel *biggest_panel = &rd_nil_panel;
            {
              Rng2F32 root_rect = os_client_rect_from_window(ws->os);
              F32 best_panel_area = 0;
              for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
              {
                if(!rd_panel_is_nil(panel->first))
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel(root_rect, ws->root_panel, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 area = panel_rect_dim.x * panel_rect_dim.y;
                if((best_panel_area == 0 || area > best_panel_area))
                {
                  best_panel_area = area;
                  biggest_panel = panel;
                }
              }
            }
            
            // rjf: find the biggest empty panel
            RD_Panel *biggest_empty_panel = &rd_nil_panel;
            {
              Rng2F32 root_rect = os_client_rect_from_window(ws->os);
              F32 best_panel_area = 0;
              for(RD_Panel *panel = ws->root_panel; !rd_panel_is_nil(panel); panel = rd_panel_rec_depth_first_pre(panel).next)
              {
                if(!rd_panel_is_nil(panel->first))
                {
                  continue;
                }
                Rng2F32 panel_rect = rd_target_rect_from_panel(root_rect, ws->root_panel, panel);
                Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
                F32 area = panel_rect_dim.x * panel_rect_dim.y;
                B32 panel_is_empty = 1;
                for(RD_View *v = panel->first_tab_view; !rd_view_is_nil(v); v = v->order_next)
                {
                  if(!rd_view_is_project_filtered(v))
                  {
                    panel_is_empty = 0;
                    break;
                  }
                }
                if(panel_is_empty && (best_panel_area == 0 || area > best_panel_area))
                {
                  best_panel_area = area;
                  biggest_empty_panel = panel;
                }
              }
            }
            
            // rjf: given the above, find source code location.
            B32 disasm_view_prioritized = 0;
            RD_Panel *panel_used_for_src_code = &rd_nil_panel;
            if(file_path.size != 0)
            {
              // rjf: determine which panel we will use to find the code loc
              RD_Panel *dst_panel = &rd_nil_panel;
              {
                if(rd_panel_is_nil(dst_panel)) { dst_panel = panel_w_this_src_code; }
                if(rd_panel_is_nil(dst_panel)) { dst_panel = panel_w_any_src_code; }
                if(rd_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
                if(rd_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
              }
              
              // rjf: construct new view if needed
              RD_View *dst_view = view_w_this_src_code;
              if(!rd_panel_is_nil(dst_panel) && rd_view_is_nil(view_w_this_src_code))
              {
                RD_View *view = rd_view_alloc();
                String8 file_path_query = rd_eval_string_from_file_path(scratch.arena, file_path);
                rd_view_equip_spec(view, rd_view_rule_info_from_kind(RD_ViewRuleKind_Text), file_path_query, &md_nil_node);
                rd_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
                dst_view = view;
              }
              
              // rjf: determine if we need a contain or center
              RD_CmdKind cursor_snap_kind = RD_CmdKind_CenterCursor;
              if(!rd_panel_is_nil(dst_panel) && dst_view == view_w_this_src_code && rd_selected_tab_from_panel(dst_panel) == dst_view)
              {
                cursor_snap_kind = RD_CmdKind_ContainCursor;
              }
              
              // rjf: move cursor & snap-to-cursor
              if(!rd_panel_is_nil(dst_panel))
              {
                disasm_view_prioritized = (rd_selected_tab_from_panel(dst_panel) == view_w_disasm);
                dst_panel->selected_tab_view = rd_handle_from_view(dst_view);
                rd_cmd(RD_CmdKind_GoToLine,
                       .panel = rd_handle_from_panel(dst_panel),
                       .view = rd_handle_from_view(dst_view),
                       .cursor = point);
                rd_cmd(cursor_snap_kind,
                       .panel = rd_handle_from_panel(dst_panel),
                       .view = rd_handle_from_view(dst_view));
                panel_used_for_src_code = dst_panel;
              }
              
              // rjf: record
              rd_cmd(RD_CmdKind_RecordFileInProject, .file_path = file_path);
            }
            
            // rjf: given the above, find disassembly location.
            if(process != &ctrl_entity_nil && vaddr != 0)
            {
              // rjf: determine which panel we will use to find the disasm loc -
              // we *cannot* use the same panel we used for source code, if any.
              RD_Panel *dst_panel = &rd_nil_panel;
              {
                if(rd_panel_is_nil(dst_panel)) { dst_panel = panel_w_disasm; }
                if(rd_panel_is_nil(panel_used_for_src_code) && rd_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
                if(rd_panel_is_nil(panel_used_for_src_code) && rd_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
                if(dst_panel == panel_used_for_src_code &&
                   !disasm_view_prioritized)
                {
                  dst_panel = &rd_nil_panel;
                }
              }
              
              // rjf: construct new view if needed
              RD_View *dst_view = view_w_disasm;
              if(!rd_panel_is_nil(dst_panel) && rd_view_is_nil(view_w_disasm))
              {
                RD_View *view = rd_view_alloc();
                rd_view_equip_spec(view, rd_view_rule_info_from_kind(RD_ViewRuleKind_Disasm), str8_zero(), &md_nil_node);
                rd_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
                dst_view = view;
              }
              
              // rjf: determine if we need a contain or center
              RD_CmdKind cursor_snap_kind = RD_CmdKind_CenterCursor;
              if(dst_view == view_w_disasm && rd_selected_tab_from_panel(dst_panel) == dst_view)
              {
                cursor_snap_kind = RD_CmdKind_ContainCursor;
              }
              
              // rjf: move cursor & snap-to-cursor
              if(!rd_panel_is_nil(dst_panel))
              {
                dst_panel->selected_tab_view = rd_handle_from_view(dst_view);
                rd_cmd(RD_CmdKind_GoToAddress,
                       .process = process->handle, .vaddr = vaddr,
                       .panel = rd_handle_from_panel(dst_panel),
                       .view = rd_handle_from_view(dst_view));
                rd_cmd(cursor_snap_kind);
              }
            }
          }break;
          
          //- rjf: filtering
          case RD_CmdKind_Filter:
          {
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
          }break;
          case RD_CmdKind_ClearFilter:
          {
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            if(!rd_view_is_nil(view))
            {
              view->query_string_size = 0;
              view->is_filtering = 0;
              view->query_cursor = view->query_mark = txt_pt(1, 1);
            }
          }break;
          case RD_CmdKind_ApplyFilter:
          {
            RD_View *view = rd_view_from_handle(rd_regs()->view);
            if(!rd_view_is_nil(view))
            {
              view->is_filtering = 0;
            }
          }break;
          
          //- rjf: query completion
          case RD_CmdKind_CompleteQuery:
          {
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
          }break;
          case RD_CmdKind_CancelQuery:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            arena_clear(ws->query_cmd_arena);
            MemoryZeroStruct(&ws->query_cmd_name);
            ws->query_cmd_regs = 0;
            MemoryZeroArray(ws->query_cmd_regs_mask);
            for(RD_View *v = ws->query_view_stack_top, *next = 0; !rd_view_is_nil(v); v = next)
            {
              next = v->order_next;
              rd_view_release(v);
            }
            ws->query_view_stack_top = &rd_nil_view;
          }break;
          
          //- rjf: developer commands
          case RD_CmdKind_ToggleDevMenu:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            ws->dev_menu_is_open ^= 1;
          }break;
          
          //- rjf: general entity operations
          case RD_CmdKind_SelectEntity:
          case RD_CmdKind_SelectTarget:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            RD_EntityList all_of_the_same_kind = rd_query_cached_entity_list_with_kind(entity->kind);
            B32 is_selected = !entity->disabled;
            for(RD_EntityNode *n = all_of_the_same_kind.first; n != 0; n = n->next)
            {
              RD_Entity *e = n->entity;
              rd_entity_equip_disabled(e, 1);
            }
            if(!is_selected)
            {
              rd_entity_equip_disabled(entity, 0);
            }
          }break;
          case RD_CmdKind_EnableEntity:
          case RD_CmdKind_EnableBreakpoint:
          case RD_CmdKind_EnableTarget:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            rd_entity_equip_disabled(entity, 0);
          }break;
          case RD_CmdKind_DisableEntity:
          case RD_CmdKind_DisableBreakpoint:
          case RD_CmdKind_DisableTarget:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            rd_entity_equip_disabled(entity, 1);
          }break;
          case RD_CmdKind_RemoveEntity:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            RD_EntityKindFlags kind_flags = rd_entity_kind_flags_table[entity->kind];
            if(kind_flags & RD_EntityKindFlag_CanDelete)
            {
              rd_entity_mark_for_deletion(entity);
            }
          }break;
          case RD_CmdKind_NameEntity:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            String8 string = rd_regs()->string;
            rd_entity_equip_name(entity, string);
          }break;
          case RD_CmdKind_ConditionEntity:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            String8 string = rd_regs()->string;
            if(string.size != 0)
            {
              RD_Entity *child = rd_entity_child_from_kind_or_alloc(entity, RD_EntityKind_Condition);
              rd_entity_equip_name(child, string);
            }
            else
            {
              RD_Entity *child = rd_entity_child_from_kind(entity, RD_EntityKind_Condition);
              rd_entity_mark_for_deletion(child);
            }
          }break;
          case RD_CmdKind_DuplicateEntity:
          {
            RD_Entity *src = rd_entity_from_handle(rd_regs()->entity);
            if(!rd_entity_is_nil(src))
            {
              typedef struct Task Task;
              struct Task
              {
                Task *next;
                RD_Entity *src_n;
                RD_Entity *dst_parent;
              };
              Task starter_task = {0, src, src->parent};
              Task *first_task = &starter_task;
              Task *last_task = &starter_task;
              for(Task *task = first_task; task != 0; task = task->next)
              {
                RD_Entity *src_n = task->src_n;
                RD_Entity *dst_n = rd_entity_alloc(task->dst_parent, task->src_n->kind);
                if(src_n->flags & RD_EntityFlag_HasTextPoint)    {rd_entity_equip_txt_pt(dst_n, src_n->text_point);}
                if(src_n->flags & RD_EntityFlag_HasU64)          {rd_entity_equip_u64(dst_n, src_n->u64);}
                if(src_n->flags & RD_EntityFlag_HasColor)        {rd_entity_equip_color_hsva(dst_n, rd_hsva_from_entity(src_n));}
                if(src_n->flags & RD_EntityFlag_HasVAddrRng)     {rd_entity_equip_vaddr_rng(dst_n, src_n->vaddr_rng);}
                if(src_n->flags & RD_EntityFlag_HasVAddr)        {rd_entity_equip_vaddr(dst_n, src_n->vaddr);}
                if(src_n->disabled)                             {rd_entity_equip_disabled(dst_n, 1);}
                if(src_n->string.size != 0)                     {rd_entity_equip_name(dst_n, src_n->string);}
                dst_n->cfg_src = src_n->cfg_src;
                for(RD_Entity *src_child = task->src_n->first; !rd_entity_is_nil(src_child); src_child = src_child->next)
                {
                  Task *child_task = push_array(scratch.arena, Task, 1);
                  child_task->src_n = src_child;
                  child_task->dst_parent = dst_n;
                  SLLQueuePush(first_task, last_task, child_task);
                }
              }
            }
          }break;
          case RD_CmdKind_RelocateEntity:
          {
            RD_Entity *entity = rd_entity_from_handle(rd_regs()->entity);
            RD_Entity *location = rd_entity_child_from_kind(entity, RD_EntityKind_Location);
            if(rd_entity_is_nil(location))
            {
              location = rd_entity_alloc(entity, RD_EntityKind_Location);
            }
            location->flags &= ~RD_EntityFlag_HasTextPoint;
            location->flags &= ~RD_EntityFlag_HasVAddr;
            if(rd_regs()->cursor.line != 0)
            {
              rd_entity_equip_txt_pt(location, rd_regs()->cursor);
            }
            if(rd_regs()->vaddr != 0)
            {
              rd_entity_equip_vaddr(location, rd_regs()->vaddr);
              rd_entity_equip_name(location, str8_zero());
            }
            if(rd_regs()->file_path.size != 0)
            {
              rd_entity_equip_name(location, rd_regs()->file_path);
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
                RD_EntityList bps = rd_query_cached_entity_list_with_kind(RD_EntityKind_Breakpoint);
                for(RD_EntityNode *n = bps.first; n != 0; n = n->next)
                {
                  RD_Entity *bp = n->entity;
                  RD_Entity *loc = rd_entity_child_from_kind(bp, RD_EntityKind_Location);
                  if((loc->flags & RD_EntityFlag_HasTextPoint && path_match_normalized(loc->string, file_path) && loc->text_point.line == pt.line) ||
                     (loc->flags & RD_EntityFlag_HasVAddr && loc->vaddr == vaddr) ||
                     (!(loc->flags & RD_EntityFlag_HasTextPoint) && str8_match(loc->string, string, 0)))
                  {
                    rd_entity_mark_for_deletion(bp);
                    removed_already_existing = 1;
                    break;
                  }
                }
              }
              if(!removed_already_existing)
              {
                RD_Entity *bp = rd_entity_alloc(rd_entity_root(), RD_EntityKind_Breakpoint);
                rd_entity_equip_cfg_src(bp, RD_CfgSrc_Project);
                RD_Entity *loc = rd_entity_alloc(bp, RD_EntityKind_Location);
                if(vaddr != 0)
                {
                  rd_entity_equip_vaddr(loc, vaddr);
                }
                else if(string.size != 0)
                {
                  rd_entity_equip_name(loc, string);
                }
                else if(file_path.size != 0 && pt.line != 0)
                {
                  rd_entity_equip_name(loc, file_path);
                  rd_entity_equip_txt_pt(loc, pt);
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
              RD_EntityList wps = rd_query_cached_entity_list_with_kind(RD_EntityKind_WatchPin);
              for(RD_EntityNode *n = wps.first; n != 0; n = n->next)
              {
                RD_Entity *wp = n->entity;
                RD_Entity *loc = rd_entity_child_from_kind(wp, RD_EntityKind_Location);
                if(str8_match(wp->string, string, 0) &&
                   ((loc->flags & RD_EntityFlag_HasTextPoint && path_match_normalized(loc->string, file_path) && loc->text_point.line == pt.line) ||
                    (loc->flags & RD_EntityFlag_HasVAddr && loc->vaddr == vaddr)))
                {
                  rd_entity_mark_for_deletion(wp);
                  removed_already_existing = 1;
                  break;
                }
              }
            }
            if(!removed_already_existing)
            {
              RD_Entity *wp = rd_entity_alloc(rd_entity_root(), RD_EntityKind_WatchPin);
              rd_entity_equip_name(wp, string);
              rd_entity_equip_cfg_src(wp, RD_CfgSrc_Project);
              RD_Entity *loc = rd_entity_alloc(wp, RD_EntityKind_Location);
              if(file_path.size != 0 && pt.line != 0)
              {
                rd_entity_equip_name(loc, file_path);
                rd_entity_equip_txt_pt(loc, pt);
              }
              else if(vaddr != 0)
              {
                rd_entity_equip_vaddr(loc, vaddr);
              }
            }
          }break;
          
          //- rjf: watches
          case RD_CmdKind_ToggleWatchExpression:
          if(rd_regs()->string.size != 0)
          {
            RD_Entity *existing_watch = rd_entity_from_name_and_kind(rd_regs()->string, RD_EntityKind_Watch);
            if(rd_entity_is_nil(existing_watch))
            {
              RD_Entity *watch = &d_nil_entity;
              watch = rd_entity_alloc(rd_entity_root(), RD_EntityKind_Watch);
              rd_entity_equip_cfg_src(watch, RD_CfgSrc_Project);
              rd_entity_equip_name(watch, rd_regs()->string);
            }
            else
            {
              rd_entity_mark_for_deletion(existing_watch);
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
          case RD_CmdKind_RunToCursor:
          {
            if(rd_regs()->file_path.size != 0)
            {
              rd_cmd(RD_CmdKind_RunToLine);
            }
            else
            {
              rd_cmd(RD_CmdKind_RunToAddress);
            }
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
            // rjf: build target
            RD_Entity *entity = &d_nil_entity;
            entity = rd_entity_alloc(rd_entity_root(), RD_EntityKind_Target);
            rd_entity_equip_disabled(entity, 1);
            rd_entity_equip_cfg_src(entity, RD_CfgSrc_Project);
            RD_Entity *exe = rd_entity_alloc(entity, RD_EntityKind_Executable);
            rd_entity_equip_name(exe, rd_regs()->file_path);
            String8 working_dir = str8_chop_last_slash(rd_regs()->file_path);
            if(working_dir.size != 0)
            {
              String8 working_dir_path = push_str8f(scratch.arena, "%S/", working_dir);
              RD_Entity *execution_path = rd_entity_alloc(entity, RD_EntityKind_WorkingDirectory);
              rd_entity_equip_name(execution_path, working_dir_path);
            }
            rd_cmd(RD_CmdKind_SelectTarget, .entity = rd_handle_from_entity(entity));
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
            RD_Window *ws = rd_window_from_os_handle(os_event->window);
            if(os_event != 0 && ws != 0)
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
            CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
            if(rd_regs()->unwind_count < rich_unwind.concrete_frame_count)
            {
              CTRL_CallStackFrame *frame = &rich_unwind.frames[rd_regs()->unwind_count];
              U64 rip_vaddr = regs_rip_from_arch_block(thread->arch, frame->regs);
              CTRL_Entity *module = ctrl_module_from_process_vaddr(process, rip_vaddr);
              rd_state->base_regs.v.module = module->handle;
              rd_state->base_regs.v.unwind_count = rd_regs()->unwind_count;
              rd_state->base_regs.v.inline_depth = 0;
              if(rd_regs()->inline_depth <= frame->inline_frame_count)
              {
                rd_state->base_regs.v.inline_depth = rd_regs()->inline_depth;
              }
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
            CTRL_CallStack rich_unwind = ctrl_call_stack_from_unwind(scratch.arena, di_scope, process, &base_unwind);
            U64 crnt_unwind_idx = rd_state->base_regs.v.unwind_count;
            U64 crnt_inline_dpt = rd_state->base_regs.v.inline_depth;
            U64 next_unwind_idx = crnt_unwind_idx;
            U64 next_inline_dpt = crnt_inline_dpt;
            if(crnt_unwind_idx < rich_unwind.concrete_frame_count)
            {
              CTRL_CallStackFrame *f = &rich_unwind.frames[crnt_unwind_idx];
              switch(kind)
              {
                default:{}break;
                case RD_CmdKind_UpOneFrame:
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
                case RD_CmdKind_DownOneFrame:
                {
                  if(crnt_inline_dpt > 0)
                  {
                    next_inline_dpt -= 1;
                  }
                  else if(crnt_unwind_idx < rich_unwind.concrete_frame_count)
                  {
                    next_unwind_idx += 1;
                    next_inline_dpt = (f+1)->inline_frame_count;
                  }
                }break;
              }
            }
            rd_cmd(RD_CmdKind_SelectUnwind,
                   .unwind_count = next_unwind_idx,
                   .inline_depth = next_inline_dpt);
            di_scope_close(di_scope);
          }break;
          
          //- rjf: meta controls
          case RD_CmdKind_Edit:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Press;
            evt.slot       = UI_EventActionSlot_Edit;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Accept:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Press;
            evt.slot       = UI_EventActionSlot_Accept;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Cancel:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
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
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRight:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUp:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDown:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveLeftSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRightSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveLeftChunk:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRightChunk:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpChunk:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownChunk:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpPage:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownPage:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpWhole:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownWhole:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveLeftChunkSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveRightChunkSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpChunkSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownChunkSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpPageSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownPageSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Page;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpWholeSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownWholeSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Whole;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveUpReorder:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_Reorder;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, -1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveDownReorder:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_Reorder;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+0, +1);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveHome:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveEnd:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveHomeSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_MoveEndSelect:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Navigate;
            evt.flags      = UI_EventFlag_KeepMark;
            evt.delta_unit = UI_EventDeltaUnit_Line;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_SelectAll:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
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
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_DeleteChunk:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(+1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_BackspaceSingle:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete|UI_EventFlag_ZeroDeltaOnSelect;
            evt.delta_unit = UI_EventDeltaUnit_Char;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_BackspaceChunk:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind       = UI_EventKind_Edit;
            evt.flags      = UI_EventFlag_Delete;
            evt.delta_unit = UI_EventDeltaUnit_Word;
            evt.delta_2s32 = v2s32(-1, +0);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Copy:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind  = UI_EventKind_Edit;
            evt.flags = UI_EventFlag_Copy|UI_EventFlag_KeepMark;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Cut:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind  = UI_EventKind_Edit;
            evt.flags = UI_EventFlag_Copy|UI_EventFlag_Delete;
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_Paste:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
            UI_Event evt = zero_struct;
            evt.kind   = UI_EventKind_Text;
            evt.string = os_get_clipboard_text(scratch.arena);
            ui_event_list_push(scratch.arena, &ws->ui_events, &evt);
          }break;
          case RD_CmdKind_InsertText:
          {
            RD_Window *ws = rd_window_from_handle(rd_regs()->window);
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
      RD_EntityList target_entities = rd_query_cached_entity_list_with_kind(RD_EntityKind_Target);
      targets.count = target_entities.count;
      targets.v = push_array(scratch.arena, D_Target, targets.count);
      U64 idx = 0;
      for(RD_EntityNode *n = target_entities.first; n != 0; n = n->next)
      {
        RD_Entity *src_target = n->entity;
        if(src_target->disabled)
        {
          targets.count -= 1;
          continue;
        }
        targets.v[idx] = rd_d_target_from_entity(src_target);
        idx += 1;
      }
    }
    
    ////////////////////////////
    //- rjf: gather breakpoints & meta-evals (for the engine, meta-evals can only be referenced by breakpoints)
    //
    D_BreakpointArray breakpoints = {0};
    CTRL_MetaEvalArray meta_evals = {0};
    ProfScope("gather breakpoints & meta-evals")
    {
      typedef struct MetaEvalNode MetaEvalNode;
      struct MetaEvalNode
      {
        MetaEvalNode *next;
        CTRL_MetaEval *meval;
      };
      U64 meval_count = 0;
      MetaEvalNode *first_meval = 0;
      MetaEvalNode *last_meval = 0;
      RD_EntityList bp_entities = rd_query_cached_entity_list_with_kind(RD_EntityKind_Breakpoint);
      breakpoints.count = bp_entities.count;
      breakpoints.v = push_array(scratch.arena, D_Breakpoint, breakpoints.count);
      U64 idx = 0;
      for(RD_EntityNode *n = bp_entities.first; n != 0; n = n->next)
      {
        RD_Entity *src_bp = n->entity;
        if(src_bp->disabled)
        {
          breakpoints.count -= 1;
          continue;
        }
        RD_Entity *src_bp_loc = rd_entity_child_from_kind(src_bp, RD_EntityKind_Location);
        RD_Entity *src_bp_cnd = rd_entity_child_from_kind(src_bp, RD_EntityKind_Condition);
        
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
        if(src_bp_cnd->string.size != 0)
        {
          typedef struct ExprWalkTask ExprWalkTask;
          struct ExprWalkTask
          {
            ExprWalkTask *next;
            E_Expr *expr;
          };
          E_Expr *expr = e_parse_expr_from_text(scratch.arena, src_bp_cnd->string);
          ExprWalkTask start_task = {0, expr};
          ExprWalkTask *first_task = &start_task;
          for(ExprWalkTask *t = first_task; t != 0; t = t->next)
          {
            if(t->expr->kind == E_ExprKind_LeafIdent)
            {
              E_Expr *macro_expr = e_string2expr_lookup(e_ir_ctx->macro_map, t->expr->string);
              if(macro_expr != &e_expr_nil)
              {
                E_Eval eval = e_eval_from_expr(scratch.arena, macro_expr);
                switch(eval.space.kind)
                {
                  default:{is_static_for_ctrl_thread = 0;}break;
                  case E_SpaceKind_Null:
                  case RD_EvalSpaceKind_MetaEntity:
                  {
                    is_static_for_ctrl_thread = 1;
                    RD_Entity *entity = rd_entity_from_eval_space(eval.space);
                    if(!rd_entity_is_nil(entity))
                    {
                      MetaEvalNode *meval_node = push_array(scratch.arena, MetaEvalNode, 1);
                      meval_node->meval = rd_ctrl_meta_eval_from_entity(scratch.arena, entity);
                      SLLQueuePush(first_meval, last_meval, meval_node);
                      meval_count += 1;
                    }
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
          E_Eval eval = e_eval_from_string(scratch.arena, src_bp_cnd->string);
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
        dst_bp->file_path   = src_bp_loc->string;
        dst_bp->pt          = src_bp_loc->text_point;
        dst_bp->symbol_name = src_bp_loc->string;
        dst_bp->vaddr       = src_bp_loc->vaddr;
        dst_bp->condition   = src_bp_cnd->string;
        idx += 1;
      }
      
      //- rjf: meta-eval list -> array
      meta_evals.count = meval_count;
      meta_evals.v = push_array(scratch.arena, CTRL_MetaEval, meta_evals.count);
      {
        U64 idx = 0;
        for(MetaEvalNode *n = first_meval; n != 0; n = n->next)
        {
          MemoryCopyStruct(&meta_evals.v[idx], n->meval);
          idx += 1;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: gather path maps
    //
    D_PathMapArray path_maps = {0};
    {
      RD_EntityList maps = rd_query_cached_entity_list_with_kind(RD_EntityKind_FilePathMap);
      path_maps.count = maps.count;
      path_maps.v = push_array(scratch.arena, D_PathMap, path_maps.count);
      U64 idx = 0;
      for(RD_EntityNode *n = maps.first; n != 0; n = n->next, idx += 1)
      {
        RD_Entity *map = n->entity;
        path_maps.v[idx].src = rd_entity_child_from_kind(map, RD_EntityKind_Source)->string;
        path_maps.v[idx].dst = rd_entity_child_from_kind(map, RD_EntityKind_Dest)->string;
      }
    }
    
    ////////////////////////////
    //- rjf: gather exception code filters
    //
    U64 exception_code_filters[(CTRL_ExceptionCodeKind_COUNT+63)/64] = {0};
    {
      MemoryCopyArray(exception_code_filters, rd_state->ctrl_exception_code_filters);
    }
    
    ////////////////////////////
    //- rjf: tick debug engine
    //
    U64 cmd_count_pre_tick = rd_state->cmds[0].count;
    D_EventList engine_events = d_tick(scratch.arena, &targets, &breakpoints, &path_maps, exception_code_filters, &meta_evals);
    
    ////////////////////////////
    //- rjf: no selected thread? -> try to snap to any existing thread
    //
    if(!d_ctrl_targets_running() && ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread) == &ctrl_entity_nil)
    {
      CTRL_Entity *process = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->process);
      if(process == &ctrl_entity_nil)
      {
        CTRL_EntityList all_processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
        if(all_processes.count != 0)
        {
          process = all_processes.first->v;
        }
      }
      CTRL_Entity *new_thread = ctrl_entity_child_from_kind(process, CTRL_EntityKind_Thread);
      if(new_thread != &ctrl_entity_nil)
      {
        rd_cmd(RD_CmdKind_SelectThread, .thread = new_thread->handle);
      }
    }
    
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
          CTRL_Entity *thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, evt->thread);
          U64 vaddr = evt->vaddr;
          CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
          CTRL_Entity *module = ctrl_module_from_process_vaddr(process, vaddr);
          DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
          U64 voff = ctrl_voff_from_vaddr(module, vaddr);
          U64 test_cached_vaddr = ctrl_query_cached_rip_from_thread(d_state->ctrl_entity_store, thread->handle);
          
          // rjf: valid stop thread? -> select & snap
          if(thread != &ctrl_entity_nil && evt->cause != D_EventCause_Halt)
          {
            rd_cmd(RD_CmdKind_SelectThread, .thread = thread->handle);
          }
          
          // rjf: no stop-causing thread, but have selected thread? -> snap to selected
          CTRL_Entity *selected_thread = ctrl_entity_from_handle(d_state->ctrl_entity_store, rd_base_regs()->thread);
          if((evt->cause == D_EventCause_Halt || thread == &ctrl_entity_nil) && selected_thread != &ctrl_entity_nil)
          {
            rd_cmd(RD_CmdKind_SelectThread, .thread = selected_thread->handle);
          }
          
          // rjf: no stop-causing thread, but don't have selected thread? -> snap to first available thread
          if(thread == &ctrl_entity_nil && selected_thread == &ctrl_entity_nil)
          {
            CTRL_EntityList threads = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Thread);
            CTRL_Entity *first_available_thread = ctrl_entity_list_first(&threads);
            rd_cmd(RD_CmdKind_SelectThread, .thread = first_available_thread->handle);
          }
          
          // rjf: increment breakpoint hit counts
          if(evt->cause == D_EventCause_UserBreakpoint)
          {
            RD_EntityList user_bps = rd_query_cached_entity_list_with_kind(RD_EntityKind_Breakpoint);
            for(RD_EntityNode *n = user_bps.first; n != 0; n = n->next)
            {
              RD_Entity *bp = n->entity;
              RD_Entity *loc = rd_entity_child_from_kind(bp, RD_EntityKind_Location);
              D_LineList loc_lines = d_lines_from_file_path_line_num(scratch.arena, loc->string, loc->text_point.line);
              if(loc_lines.first != 0)
              {
                for(D_LineNode *n = loc_lines.first; n != 0; n = n->next)
                {
                  if(contains_1u64(n->v.voff_range, voff))
                  {
                    bp->u64 += 1;
                    break;
                  }
                }
              }
              else if(loc->flags & RD_EntityFlag_HasVAddr && vaddr == loc->vaddr)
              {
                bp->u64 += 1;
              }
              else if(loc->string.size != 0)
              {
                U64 symb_voff = d_voff_from_dbgi_key_symbol_name(&dbgi_key, loc->string);
                if(symb_voff == voff)
                {
                  bp->u64 += 1;
                }
              }
            }
          }
          
          // rjf: focus window if none focused
          B32 any_window_is_focused = 0;
          for(RD_Window *window = rd_state->first_window; window != 0; window = window->next)
          {
            if(os_window_is_focused(window->os))
            {
              any_window_is_focused = 1;
              break;
            }
          }
          if(!any_window_is_focused)
          {
            RD_Window *window = rd_window_from_handle(rd_state->last_focused_window);
            if(window == 0)
            {
              window = rd_state->first_window;
            }
            if(window != 0)
            {
              os_window_set_minimized(window->os, 0);
              os_window_bring_to_front(window->os);
              os_window_focus(window->os);
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
    F32 rate = rd_setting_val_from_code(RD_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-10.f * rd_state->frame_dt)) : 1.f;
    B32 popup_open = rd_state->popup_active;
    rd_state->popup_t += rate * ((F32)!!popup_open-rd_state->popup_t);
    if(abs_f32(rd_state->popup_t - (F32)!!popup_open) > 0.005f)
    {
      rd_request_frame();
    }
  }
  
  //////////////////////////////
  //- rjf: animate theme
  //
  {
    RD_Theme *current = &rd_state->cfg_theme;
    RD_Theme *target = &rd_state->cfg_theme_target;
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
  
  //////////////////////////////
  //- rjf: capture is active? -> keep rendering
  //
  if(ProfIsCapturing())
  {
    rd_request_frame();
  }
  
  //////////////////////////////
  //- rjf: commit params changes for all views
  //
  {
    for(RD_View *v = rd_state->first_view; !rd_view_is_nil(v); v = v->alloc_next)
    {
      if(v->params_write_gen == v->params_read_gen+1)
      {
        v->params_read_gen += 1;
      }
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
  {
    Temp scratch = scratch_begin(0, 0);
    rd_state->ambiguous_path_slots_count = 512;
    rd_state->ambiguous_path_slots = push_array(rd_frame_arena(), RD_AmbiguousPathNode *, rd_state->ambiguous_path_slots_count);
    for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
    {
      for(RD_Panel *p = w->root_panel; !rd_panel_is_nil(p); p = rd_panel_rec_depth_first_pre(p).next)
      {
        for(RD_View *v = p->first_tab_view; !rd_view_is_nil(v); v = v->order_next)
        {
          if(rd_view_is_project_filtered(v))
          {
            continue;
          }
          String8 eval_string = str8(v->query_buffer, v->query_string_size);
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
    for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
    {
      B32 window_is_focused = os_window_is_focused(w->os);
      if(window_is_focused)
      {
        rd_state->last_focused_window = rd_handle_from_window(w);
      }
      rd_push_regs();
      rd_regs()->window = rd_handle_from_window(w);
      rd_window_frame(w);
      MemoryZeroStruct(&w->ui_events);
      RD_Regs *window_regs = rd_pop_regs();
      if(rd_window_from_handle(rd_state->last_focused_window) == w)
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
  {
    r_begin_frame();
    for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
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
    RD_HandleList windows_to_show = {0};
    for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
    {
      if(w->frames_alive == 1)
      {
        rd_handle_list_push(scratch.arena, &windows_to_show, rd_handle_from_window(w));
      }
    }
    for(RD_HandleNode *n = windows_to_show.first; n != 0; n = n->next)
    {
      RD_Window *window = rd_window_from_handle(n->handle);
      DeferLoop(depth += 1, depth -= 1) os_window_first_paint(window->os);
    }
  }
  
  //////////////////////////////
  //- rjf: eliminate entities that are marked for deletion
  //
  ProfScope("eliminate deleted entities")
  {
    for(RD_Entity *entity = rd_entity_root(), *next = 0; !rd_entity_is_nil(entity); entity = next)
    {
      next = rd_entity_rec_depth_first_pre(entity, &d_nil_entity).next;
      if(entity->flags & RD_EntityFlag_MarkedForDeletion)
      {
        B32 undoable = (rd_entity_kind_flags_table[entity->kind] & RD_EntityKindFlag_UserDefinedLifetime);
        
        // rjf: fixup next entity to iterate to
        next = rd_entity_rec_depth_first(entity, &d_nil_entity, OffsetOf(RD_Entity, next), OffsetOf(RD_Entity, next)).next;
        
        // rjf: eliminate root entity if we're freeing it
        if(entity == rd_state->entities_root)
        {
          rd_state->entities_root = &d_nil_entity;
        }
        
        // rjf: unhook & release this entity tree
        rd_entity_change_parent(entity, entity->parent, &d_nil_entity, &d_nil_entity);
        rd_entity_release(entity);
      }
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
  {
    LogScopeResult log = log_scope_end(scratch.arena);
    os_append_data_to_file_path(rd_state->log_path, log.strings[LogMsgKind_Info]);
    if(log.strings[LogMsgKind_UserError].size != 0)
    {
      for(RD_Window *w = rd_state->first_window; w != 0; w = w->next)
      {
        w->error_string_size = Min(sizeof(w->error_buffer), log.strings[LogMsgKind_UserError].size);
        MemoryCopy(w->error_buffer, log.strings[LogMsgKind_UserError].str, w->error_string_size);
        w->error_t = 1.f;
      }
    }
  }
  
  scratch_end(scratch);
}
