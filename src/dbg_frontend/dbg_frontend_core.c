// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef MARKUP_LAYER_COLOR
#define MARKUP_LAYER_COLOR 0.10f, 0.20f, 0.25f

////////////////////////////////
//~ rjf: Generated Code

#include "generated/dbg_frontend.meta.c"

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

////////////////////////////////
//~ rjf: Handle Type Functions

internal DF_Handle
df_handle_zero(void)
{
  DF_Handle h = {0};
  return h;
}

internal B32
df_handle_match(DF_Handle a, DF_Handle b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

internal void
df_handle_list_push(Arena *arena, DF_HandleList *list, DF_Handle v)
{
  DF_HandleNode *n = push_array(arena, DF_HandleNode, 1);
  n->v = v;
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
}

internal DF_HandleList
df_handle_list_copy(Arena *arena, DF_HandleList *src)
{
  DF_HandleList dst = {0};
  for(DF_HandleNode *n = src->first; n != 0; n = n->next)
  {
    df_handle_list_push(arena, &dst, n->v);
  }
  return dst;
}

////////////////////////////////
//~ rjf: Register Type Functions

internal void
df_regs_copy_contents(Arena *arena, DF_Regs *dst, DF_Regs *src)
{
  MemoryCopyStruct(dst, src);
  dst->cfg_tree_list = df_handle_list_copy(arena, &src->cfg_tree_list);
  dst->file_path     = push_str8_copy(arena, src->file_path);
  dst->lines         = d_line_list_copy(arena, &src->lines);
  dst->dbgi_key      = di_key_copy(arena, &src->dbgi_key);
  dst->string        = push_str8_copy(arena, src->string);
  dst->params_tree   = md_tree_copy(arena, src->params_tree);
  if(dst->cfg_tree_list.count == 0 && !df_handle_match(df_handle_zero(), dst->cfg_tree))
  {
    df_handle_list_push(arena, &dst->cfg_tree_list, dst->cfg_tree);
  }
}

internal DF_Regs *
df_regs_copy(Arena *arena, DF_Regs *src)
{
  DF_Regs *dst = push_array(arena, DF_Regs, 1);
  df_regs_copy_contents(arena, dst, src);
  return dst;
}

////////////////////////////////
//~ rjf: View Type Functions

internal B32
df_view_is_nil(DF_View *view)
{
  return (view == 0 || view == &df_nil_view);
}

internal B32
df_view_is_project_filtered(DF_View *view)
{
  B32 result = 0;
  String8 view_project = view->project_path;
  if(view_project.size != 0)
  {
    String8 current_project = df_state->cfg_slot_roots[DF_CfgSlot_Project]->string;
    result = !path_match_normalized(view_project, current_project);
  }
  return result;
}

internal D_Handle
df_handle_from_view(DF_View *view)
{
  D_Handle handle = d_handle_zero();
  if(!df_view_is_nil(view))
  {
    handle.u64[0] = (U64)view;
    handle.u64[1] = view->generation;
  }
  return handle;
}

internal DF_View *
df_view_from_handle(D_Handle handle)
{
  DF_View *result = (DF_View *)handle.u64[0];
  if(df_view_is_nil(result) || result->generation != handle.u64[1])
  {
    result = &df_nil_view;
  }
  return result;
}

////////////////////////////////
//~ rjf: View Spec Type Functions

internal DF_ViewKind
df_view_kind_from_string(String8 string)
{
  DF_ViewKind result = DF_ViewKind_Null;
  for(U64 idx = 0; idx < ArrayCount(df_g_gfx_view_kind_spec_info_table); idx += 1)
  {
    if(str8_match(string, df_g_gfx_view_kind_spec_info_table[idx].name, StringMatchFlag_CaseInsensitive))
    {
      result = (DF_ViewKind)idx;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Panel Type Functions

//- rjf: basic type functions

internal B32
df_panel_is_nil(DF_Panel *panel)
{
  return panel == 0 || panel == &df_nil_panel;
}

internal D_Handle
df_handle_from_panel(DF_Panel *panel)
{
  D_Handle h = {0};
  h.u64[0] = (U64)panel;
  h.u64[1] = panel->generation;
  return h;
}

internal DF_Panel *
df_panel_from_handle(D_Handle handle)
{
  DF_Panel *panel = (DF_Panel *)handle.u64[0];
  if(panel == 0 || panel->generation != handle.u64[1])
  {
    panel = &df_nil_panel;
  }
  return panel;
}

internal UI_Key
df_ui_key_from_panel(DF_Panel *panel)
{
  UI_Key panel_key = ui_key_from_stringf(ui_key_zero(), "panel_window_%p", panel);
  return panel_key;
}

//- rjf: tree construction

internal void
df_panel_insert(DF_Panel *parent, DF_Panel *prev_child, DF_Panel *new_child)
{
  DLLInsert_NPZ(&df_nil_panel, parent->first, parent->last, prev_child, new_child, next, prev);
  parent->child_count += 1;
  new_child->parent = parent;
}

internal void
df_panel_remove(DF_Panel *parent, DF_Panel *child)
{
  DLLRemove_NPZ(&df_nil_panel, parent->first, parent->last, child, next, prev);
  child->next = child->prev = child->parent = &df_nil_panel;
  parent->child_count -= 1;
}

//- rjf: tree walk

internal DF_PanelRec
df_panel_rec_df(DF_Panel *panel, U64 sib_off, U64 child_off)
{
  DF_PanelRec rec = {0};
  if(!df_panel_is_nil(*MemberFromOffset(DF_Panel **, panel, child_off)))
  {
    rec.next = *MemberFromOffset(DF_Panel **, panel, child_off);
    rec.push_count = 1;
  }
  else if(!df_panel_is_nil(*MemberFromOffset(DF_Panel **, panel, sib_off)))
  {
    rec.next = *MemberFromOffset(DF_Panel **, panel, sib_off);
  }
  else
  {
    DF_Panel *uncle = &df_nil_panel;
    for(DF_Panel *p = panel->parent; !df_panel_is_nil(p); p = p->parent)
    {
      rec.pop_count += 1;
      if(!df_panel_is_nil(*MemberFromOffset(DF_Panel **, p, sib_off)))
      {
        uncle = *MemberFromOffset(DF_Panel **, p, sib_off);
        break;
      }
    }
    rec.next = uncle;
  }
  return rec;
}

//- rjf: panel -> rect calculations

internal Rng2F32
df_target_rect_from_panel_child(Rng2F32 parent_rect, DF_Panel *parent, DF_Panel *panel)
{
  Rng2F32 rect = parent_rect;
  if(!df_panel_is_nil(parent))
  {
    Vec2F32 parent_rect_size = dim_2f32(parent_rect);
    Axis2 axis = parent->split_axis;
    rect.p1.v[axis] = rect.p0.v[axis];
    for(DF_Panel *child = parent->first; !df_panel_is_nil(child); child = child->next)
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
df_target_rect_from_panel(Rng2F32 root_rect, DF_Panel *root, DF_Panel *panel)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: count ancestors
  U64 ancestor_count = 0;
  for(DF_Panel *p = panel->parent; !df_panel_is_nil(p); p = p->parent)
  {
    ancestor_count += 1;
  }
  
  // rjf: gather ancestors
  DF_Panel **ancestors = push_array(scratch.arena, DF_Panel *, ancestor_count);
  {
    U64 ancestor_idx = 0;
    for(DF_Panel *p = panel->parent; !df_panel_is_nil(p); p = p->parent)
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
    DF_Panel *ancestor = ancestors[ancestor_idx];
    DF_Panel *parent = ancestor->parent;
    if(!df_panel_is_nil(parent))
    {
      parent_rect = df_target_rect_from_panel_child(parent_rect, parent, ancestor);
    }
  }
  
  // rjf: calculate final rect
  Rng2F32 rect = df_target_rect_from_panel_child(parent_rect, panel->parent, panel);
  
  scratch_end(scratch);
  return rect;
}

//- rjf: view ownership insertion/removal

internal void
df_panel_insert_tab_view(DF_Panel *panel, DF_View *prev_view, DF_View *view)
{
  DLLInsert_NPZ(&df_nil_view, panel->first_tab_view, panel->last_tab_view, prev_view, view, order_next, order_prev);
  panel->tab_view_count += 1;
  if(!df_view_is_project_filtered(view))
  {
    panel->selected_tab_view = df_handle_from_view(view);
  }
}

internal void
df_panel_remove_tab_view(DF_Panel *panel, DF_View *view)
{
  if(df_view_from_handle(panel->selected_tab_view) == view)
  {
    panel->selected_tab_view = d_handle_zero();
    if(d_handle_match(d_handle_zero(), panel->selected_tab_view))
    {
      for(DF_View *v = view->order_next; !df_view_is_nil(v); v = v->order_next)
      {
        if(!df_view_is_project_filtered(v))
        {
          panel->selected_tab_view = df_handle_from_view(v);
          break;
        }
      }
    }
    if(d_handle_match(d_handle_zero(), panel->selected_tab_view))
    {
      for(DF_View *v = view->order_prev; !df_view_is_nil(v); v = v->order_prev)
      {
        if(!df_view_is_project_filtered(v))
        {
          panel->selected_tab_view = df_handle_from_view(v);
          break;
        }
      }
    }
  }
  DLLRemove_NPZ(&df_nil_view, panel->first_tab_view, panel->last_tab_view, view, order_next, order_prev);
  panel->tab_view_count -= 1;
}

internal DF_View *
df_selected_tab_from_panel(DF_Panel *panel)
{
  DF_View *view = df_view_from_handle(panel->selected_tab_view);
  if(df_view_is_project_filtered(view))
  {
    view = &df_nil_view;
  }
  return view;
}

//- rjf: icons & display strings

internal DF_IconKind
df_icon_kind_from_view(DF_View *view)
{
  DF_IconKind result = view->spec->info.icon_kind;
  return result;
}

internal DR_FancyStringList
df_title_fstrs_from_view_spec_query(Arena *arena, DF_ViewSpec *spec, String8 query, Vec4F32 primary_color, Vec4F32 secondary_color, F32 size)
{
  DR_FancyStringList result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  String8 file_path = d_file_path_from_eval_string(scratch.arena, query);
  if(file_path.size != 0)
  {
    DR_FancyString fstr =
    {
      df_font_from_slot(DF_FontSlot_Main),
      push_str8_copy(arena, str8_skip_last_slash(file_path)),
      primary_color,
      size,
    };
    dr_fancy_string_list_push(arena, &result, &fstr);
  }
  else
  {
    DR_FancyString fstr1 =
    {
      df_font_from_slot(DF_FontSlot_Main),
      spec->info.display_string,
      primary_color,
      size,
    };
    dr_fancy_string_list_push(arena, &result, &fstr1);
    if(query.size != 0)
    {
      DR_FancyString fstr2 =
      {
        df_font_from_slot(DF_FontSlot_Code),
        str8_lit(" "),
        primary_color,
        size,
      };
      dr_fancy_string_list_push(arena, &result, &fstr2);
      DR_FancyString fstr3 =
      {
        df_font_from_slot(DF_FontSlot_Code),
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

internal D_Handle
df_handle_from_window(DF_Window *window)
{
  D_Handle handle = {0};
  if(window != 0)
  {
    handle.u64[0] = (U64)window;
    handle.u64[1] = window->gen;
  }
  return handle;
}

internal DF_Window *
df_window_from_handle(D_Handle handle)
{
  DF_Window *window = (DF_Window *)handle.u64[0];
  if(window != 0 && window->gen != handle.u64[1])
  {
    window = 0;
  }
  return window;
}

////////////////////////////////
//~ rjf: Command Parameters From Context

internal D_CmdParams
df_cmd_params_from_gfx(void)
{
  D_CmdParams p = d_cmd_params_zero();
  DF_Window *window = 0;
  for(DF_Window *w = df_state->first_window; w != 0; w = w->next)
  {
    if(os_window_is_focused(w->os))
    {
      window = w;
      break;
    }
  }
  if(window != 0)
  {
    p.window = df_handle_from_window(window);
    p.panel  = df_handle_from_panel(window->focused_panel);
    p.view   = df_handle_from_view(df_selected_tab_from_panel(window->focused_panel));
  }
  return p;
}

internal B32
df_prefer_dasm_from_window(DF_Window *window)
{
  DF_Panel *panel = window->focused_panel;
  DF_View *view = df_selected_tab_from_panel(panel);
  DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
  B32 result = 0;
  if(view_kind == DF_ViewKind_Disasm)
  {
    result = 1;
  }
  else if(view_kind == DF_ViewKind_Text)
  {
    result = 0;
  }
  else
  {
    B32 has_src = 0;
    B32 has_dasm = 0;
    for(DF_Panel *p = window->root_panel; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
    {
      DF_View *p_view = df_selected_tab_from_panel(p);
      DF_ViewKind p_view_kind = df_view_kind_from_string(p_view->spec->info.name);
      if(p_view_kind == DF_ViewKind_Text)
      {
        has_src = 1;
      }
      if(p_view_kind == DF_ViewKind_Disasm)
      {
        has_dasm = 1;
      }
    }
    if(has_src && !has_dasm) {result = 0;}
    if(has_dasm && !has_src) {result = 1;}
  }
  return result;
}

#if 0 // NOTE(rjf): @msgs
internal D_CmdParams
df_cmd_params_from_window(DF_Window *window)
{
  D_CmdParams p = d_cmd_params_zero();
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(window->focused_panel);
  p.view   = df_handle_from_view(df_selected_tab_from_panel(window->focused_panel));
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = d_regs()->thread;
  p.unwind_index = d_regs()->unwind_count;
  p.inline_depth = d_regs()->inline_depth;
  return p;
}

internal D_CmdParams
df_cmd_params_from_panel(DF_Window *window, DF_Panel *panel)
{
  D_CmdParams p = d_cmd_params_zero();
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(panel);
  p.view   = df_handle_from_view(df_selected_tab_from_panel(panel));
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = d_regs()->thread;
  p.unwind_index = d_regs()->unwind_count;
  p.inline_depth = d_regs()->inline_depth;
  return p;
}

internal D_CmdParams
df_cmd_params_from_view(DF_Window *window, DF_Panel *panel, DF_View *view)
{
  D_CmdParams p = d_cmd_params_zero();
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(panel);
  p.view   = df_handle_from_view(view);
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = d_regs()->thread;
  p.unwind_index = d_regs()->unwind_count;
  p.inline_depth = d_regs()->inline_depth;
  return p;
}

#endif

internal D_CmdParams
df_cmd_params_copy(Arena *arena, D_CmdParams *src)
{
  D_CmdParams dst = {0};
  MemoryCopyStruct(&dst, src);
  dst.entity_list = d_handle_list_copy(arena, src->entity_list);
  if(dst.entity_list.count == 0 && !d_handle_match(src->entity, d_handle_zero()))
  {
    d_handle_list_push(arena, &dst.entity_list, dst.entity);
  }
  dst.string = push_str8_copy(arena, src->string);
  dst.file_path = push_str8_copy(arena, src->file_path);
  if(src->params_tree != 0) {dst.params_tree = md_tree_copy(arena, src->params_tree);}
  if(dst.cmd_spec == 0)     {dst.cmd_spec = &d_nil_cmd_spec;}
  if(dst.view_spec == 0)    {dst.view_spec = &df_nil_view_spec;}
  if(dst.params_tree == 0)  {dst.params_tree= &md_nil_node;}
  return dst;
}

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32
df_drag_is_active(void)
{
  return ((df_state->drag_drop_state == DF_DragDropState_Dragging) ||
          (df_state->drag_drop_state == DF_DragDropState_Dropping));
}

internal void
df_drag_begin_(DF_Regs *regs)
{
  if(!df_drag_is_active())
  {
    arena_clear(df_state->drag_drop_arena);
    df_state->drag_drop_state = DF_DragDropState_Dragging;
    df_state->drag_drop_regs = df_regs_copy(df_state->drag_drop_arena, regs);
  }
}

internal B32
df_drag_drop(void)
{
  B32 result = 0;
  if(df_state->drag_drop_state == DF_DragDropState_Dropping)
  {
    result = 1;
    df_state->drag_drop_state = DF_DragDropState_Null;
  }
  return result;
}

internal void
df_drag_kill(void)
{
  df_state->drag_drop_state = DF_DragDropState_Null;
}

internal void
df_queue_drag_drop(void)
{
  df_state->drag_drop_state = DF_DragDropState_Dropping;
}

internal void
df_set_rich_hover_info(DF_RichHoverInfo *info)
{
  arena_clear(df_state->rich_hover_info_next_arena);
  MemoryCopyStruct(&df_state->rich_hover_info_next, info);
  df_state->rich_hover_info_next.dbgi_key = di_key_copy(df_state->rich_hover_info_next_arena, &info->dbgi_key);
}

internal DF_RichHoverInfo
df_get_rich_hover_info(void)
{
  DF_RichHoverInfo info = df_state->rich_hover_info_current;
  return info;
}

////////////////////////////////
//~ rjf: Context Menu Opening

internal void
df_ctx_menu_open_(UI_Box *box, DF_Regs *regs)
{
  MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
  DF_Window *window = df_window_from_cfg_tree(window_cfg);
  arena_clear(window->ctx_menu_arena);
  ui_ctx_menu_open(df_state->ctx_menu_key, box->key, v2f32(0, box->rect.y1 - box->rect.y0));
  window->ctx_menu_regs = df_regs_copy(window->ctx_menu_arena, regs);
}

////////////////////////////////
//~ rjf: View Spec State Functions

internal void
df_register_view_specs(DF_ViewSpecInfoArray specs)
{
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    DF_ViewSpecInfo *src_info = &specs.v[idx];
    U64 hash = d_hash_from_string(src_info->name);
    U64 slot_idx = hash%df_state->view_spec_table_size;
    DF_ViewSpec *spec = push_array(df_state->arena, DF_ViewSpec, 1);
    SLLStackPush_N(df_state->view_spec_table[slot_idx], spec, hash_next);
    MemoryCopyStruct(&spec->info, src_info);
    spec->info.name = push_str8_copy(df_state->arena, spec->info.name);
    spec->info.display_string = push_str8_copy(df_state->arena, spec->info.display_string);
  }
}

internal DF_ViewSpec *
df_view_spec_from_string(String8 string)
{
  DF_ViewSpec *spec = &df_nil_view_spec;
  U64 hash = d_hash_from_string(string);
  U64 slot_idx = hash%df_state->view_spec_table_size;
  for(DF_ViewSpec *s = df_state->view_spec_table[slot_idx];
      s != 0;
      s = s->hash_next)
  {
    if(str8_match(s->info.name, string, 0))
    {
      spec = s;
      break;
    }
  }
  return spec;
}

internal DF_ViewSpec *
df_view_spec_from_kind(DF_ViewKind kind)
{
  DF_ViewSpec *spec = df_view_spec_from_string(df_g_gfx_view_kind_spec_info_table[kind].name);
  return spec;
}

internal DF_ViewSpec *
df_view_spec_from_cmd_param_slot_spec(D_CmdParamSlot slot, D_CmdSpec *cmd_spec)
{
  DF_ViewSpec *spec = &df_nil_view_spec;
  for(D_CmdParamSlotViewSpecRuleNode *n = df_state->cmd_param_slot_view_spec_table[slot].first;
      n != 0;
      n = n->next)
  {
    if(cmd_spec == n->cmd_spec || d_cmd_spec_is_nil(n->cmd_spec))
    {
      spec = n->view_spec;
      if(!d_cmd_spec_is_nil(n->cmd_spec))
      {
        break;
      }
    }
  }
  return spec;
}

////////////////////////////////
//~ rjf: View Rule Spec State Functions

internal void
df_register_view_rule_specs(DF_ViewRuleSpecInfoArray specs)
{
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    // rjf: extract info from array slot
    DF_ViewRuleSpecInfo *info = &specs.v[idx];
    
    // rjf: skip empties
    if(info->string.size == 0)
    {
      continue;
    }
    
    // rjf: determine hash/slot
    U64 hash = d_hash_from_string(info->string);
    U64 slot_idx = hash%df_state->view_rule_spec_table_size;
    
    // rjf: allocate node & push
    DF_ViewRuleSpec *spec = push_array(df_state->arena, DF_ViewRuleSpec, 1);
    SLLStackPush_N(df_state->view_rule_spec_table[slot_idx], spec, hash_next);
    
    // rjf: fill node
    DF_ViewRuleSpecInfo *info_copy = &spec->info;
    MemoryCopyStruct(info_copy, info);
    info_copy->string         = push_str8_copy(df_state->arena, info->string);
  }
}

internal DF_ViewRuleSpec *
df_view_rule_spec_from_string(String8 string)
{
  DF_ViewRuleSpec *spec = &df_nil_view_rule_spec;
  {
    U64 hash = d_hash_from_string(string);
    U64 slot_idx = hash%df_state->view_rule_spec_table_size;
    for(DF_ViewRuleSpec *s = df_state->view_rule_spec_table[slot_idx]; s != 0; s = s->hash_next)
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
//~ rjf: View State Functions

//- rjf: cfg tree -> view

internal DF_View *
df_view_from_cfg_tree(MD_Node *view_cfg)
{
  DF_View *view = &df_nil_view;
  {
    DF_Handle handle = df_handle_from_cfg_tree(view_cfg);
    U64 hash = df_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash%df_state->view_slots_count;
    DF_ViewSlot *slot = &df_state->view_slots[slot_idx];
    DF_ViewNode *node = 0;
    for(DF_ViewNode *n = slot->first; n != 0; n = n->next)
    {
      if(df_handle_match(n->v.handle, handle))
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      Arena *arena = arena_alloc();
      node = push_array(arena, DF_ViewNode, 1);
      SLLQueuePush(slot->first, slot->last, node);
      DF_View *v = &node->v;
      v->arena = arena;
      v->handle = handle;
    }
    view = &node->v;
  }
  return view;
}

//- rjf: allocation/releasing

internal DF_View *
df_view_alloc(void)
{
  // rjf: allocate
  DF_View *view = df_state->free_view;
  {
    if(!df_view_is_nil(view))
    {
      df_state->free_view_count -= 1;
      SLLStackPop_N(df_state->free_view, alloc_next);
      U64 generation = view->generation;
      MemoryZeroStruct(view);
      view->generation = generation;
    }
    else
    {
      view = push_array(df_state->arena, DF_View, 1);
    }
    view->generation += 1;
  }
  
  // rjf: initialize
  view->arena = arena_alloc();
  view->spec = &df_nil_view_spec;
  view->project_path_arena = arena_alloc();
  view->project_path = str8_zero();
  for(U64 idx = 0; idx < ArrayCount(view->params_arenas); idx += 1)
  {
    view->params_arenas[idx] = arena_alloc();
    view->params_roots[idx] = &md_nil_node;
  }
  view->query_cursor = view->query_mark = txt_pt(1, 1);
  view->query_string_size = 0;
  df_state->allocated_view_count += 1;
  DLLPushBack_NPZ(&df_nil_view, df_state->first_view, df_state->last_view, view, alloc_next, alloc_prev);
  return view;
}

internal void
df_view_release(DF_View *view)
{
  DLLRemove_NPZ(&df_nil_view, df_state->first_view, df_state->last_view, view, alloc_next, alloc_prev);
  SLLStackPush_N(df_state->free_view, view, alloc_next);
  for(DF_View *tchild = view->first_transient, *next = 0; !df_view_is_nil(tchild); tchild = next)
  {
    next = tchild->order_next;
    df_view_release(tchild);
  }
  view->first_transient = view->last_transient = &df_nil_view;
  view->transient_view_slots_count = 0;
  view->transient_view_slots = 0;
  for(DF_ArenaExt *ext = view->first_arena_ext; ext != 0; ext = ext->next)
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
  df_state->allocated_view_count -= 1;
  df_state->free_view_count += 1;
}

//- rjf: equipment

internal void
df_view_equip_spec(DF_View *view, DF_ViewSpec *spec, String8 query, MD_Node *params)
{
  // rjf: fill params tree
  for(U64 idx = 0; idx < ArrayCount(view->params_arenas); idx += 1)
  {
    arena_clear(view->params_arenas[idx]);
  }
  view->params_roots[0] = md_tree_copy(view->params_arenas[0], params);
  view->params_write_gen = view->params_read_gen = 0;
  
  // rjf: fill query buffer
  df_view_equip_query(view, query);
  
  // rjf: initialize state for new view spec
  // NOTE(rjf): @msgs DF_ViewSetupFunctionType *view_setup = spec->info.setup_hook;
  {
    for(DF_ArenaExt *ext = view->first_arena_ext; ext != 0; ext = ext->next)
    {
      arena_release(ext->arena);
    }
    for(DF_View *tchild = view->first_transient, *next = 0; !df_view_is_nil(tchild); tchild = next)
    {
      next = tchild->order_next;
      df_view_release(tchild);
    }
    view->first_transient = view->last_transient = &df_nil_view;
    view->first_arena_ext = view->last_arena_ext = 0;
    view->transient_view_slots_count = 0;
    view->transient_view_slots = 0;
    arena_clear(view->arena);
    view->user_data = 0;
  }
  MemoryZeroStruct(&view->scroll_pos);
  view->spec = spec;
  if(spec->info.flags & DF_ViewSpecFlag_ProjectSpecific)
  {
    arena_clear(view->project_path_arena);
    view->project_path = push_str8_copy(view->project_path_arena, df_state->cfg_slot_roots[DF_CfgSlot_Project]->string);
  }
  else
  {
    MemoryZeroStruct(&view->project_path);
  }
  view->is_filtering = 0;
  view->is_filtering_t = 0;
  // NOTE(rjf): @msgs view_setup(view, view->params_roots[view->params_read_gen%ArrayCount(view->params_roots)], str8(view->query_buffer, view->query_string_size));
}

internal void
df_view_equip_query(DF_View *view, String8 query)
{
  view->query_string_size = Min(sizeof(view->query_buffer), query.size);
  MemoryCopy(view->query_buffer, query.str, view->query_string_size);
  view->query_cursor = view->query_mark = txt_pt(1, query.size+1);
}

internal void
df_view_equip_loading_info(DF_View *view, B32 is_loading, U64 progress_v, U64 progress_target)
{
  view->loading_t_target = (F32)!!is_loading;
  view->loading_progress_v = progress_v;
  view->loading_progress_v_target = progress_target;
}

//- rjf: user state extensions

internal void *
df_view_get_or_push_user_state(DF_View *view, U64 size)
{
  void *result = view->user_data;
  if(result == 0)
  {
    view->user_data = result = push_array(view->arena, U8, size);
  }
  return result;
}

internal Arena *
df_view_push_arena_ext(DF_View *view)
{
  DF_ArenaExt *ext = push_array(view->arena, DF_ArenaExt, 1);
  ext->arena = arena_alloc();
  SLLQueuePush(view->first_arena_ext, view->last_arena_ext, ext);
  return ext->arena;
}

//- rjf: param saving

internal void
df_view_store_param(DF_View *view, String8 key, String8 value)
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
  for(MD_EachNode(child, value_parse.root->first))
  {
    child->parent = key_node;
  }
  key_node->first = value_parse.root->first;
  key_node->last = value_parse.root->last;
}

internal void
df_view_store_paramf(DF_View *view, String8 key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  df_view_store_param(view, key, string);
  va_end(args);
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: Expand-Keyed Transient View Functions

internal DF_TransientViewNode *
df_transient_view_node_from_expand_key(DF_View *owner_view, D_ExpandKey key)
{
  if(owner_view->transient_view_slots_count == 0)
  {
    owner_view->transient_view_slots_count = 256;
    owner_view->transient_view_slots = push_array(owner_view->arena, DF_TransientViewSlot, owner_view->transient_view_slots_count);
  }
  U64 hash = df_hash_from_expand_key(key);
  U64 slot_idx = hash%owner_view->transient_view_slots_count;
  DF_TransientViewSlot *slot = &owner_view->transient_view_slots[slot_idx];
  DF_TransientViewNode *node = 0;
  for(DF_TransientViewNode *n = slot->first; n != 0; n = n->next)
  {
    if(d_expand_key_match(n->key, key))
    {
      node = n;
      n->last_frame_index_touched = d_frame_index();
      break;
    }
  }
  if(node == 0)
  {
    if(!owner_view->free_transient_view_node)
    {
      owner_view->free_transient_view_node = push_array(df_state->arena, DF_TransientViewNode, 1);
    }
    node = owner_view->free_transient_view_node;
    SLLStackPop(owner_view->free_transient_view_node);
    DLLPushBack(slot->first, slot->last, node);
    node->key = key;
    node->view = df_view_alloc();
    node->initial_params_arena = arena_alloc();
    node->first_frame_index_touched = node->last_frame_index_touched = d_frame_index();
    DLLPushBack_NPZ(&df_nil_view, owner_view->first_transient, owner_view->last_transient, node->view, order_next, order_prev);
  }
  return node;
}

////////////////////////////////
//~ rjf: Panel State Functions

internal DF_Panel *
df_panel_alloc(DF_Window *ws)
{
  DF_Panel *panel = ws->free_panel;
  if(!df_panel_is_nil(panel))
  {
    SLLStackPop(ws->free_panel);
    U64 generation = panel->generation;
    MemoryZeroStruct(panel);
    panel->generation = generation;
  }
  else
  {
    panel = push_array(ws->arena, DF_Panel, 1);
  }
  panel->first = panel->last = panel->next = panel->prev = panel->parent = &df_nil_panel;
  panel->first_tab_view = panel->last_tab_view = &df_nil_view;
  panel->generation += 1;
  MemoryZeroStruct(&panel->animated_rect_pct);
  return panel;
}

internal void
df_panel_release(DF_Window *ws, DF_Panel *panel)
{
  df_panel_release_all_views(panel);
  SLLStackPush(ws->free_panel, panel);
  panel->generation += 1;
}

internal void
df_panel_release_all_views(DF_Panel *panel)
{
  for(DF_View *view = panel->first_tab_view, *next = 0; !df_view_is_nil(view); view = next)
  {
    next = view->order_next;
    df_view_release(view);
  }
  panel->first_tab_view = panel->last_tab_view = &df_nil_view;
  panel->selected_tab_view = d_handle_zero();
  panel->tab_view_count = 0;
}

////////////////////////////////
//~ rjf: Window State Functions

internal DF_Window *
df_window_from_cfg_tree(MD_Node *window_cfg)
{
  DF_Window *result = &df_nil_window;
  if(!md_node_is_nil(window_cfg))
  {
    Temp scratch = scratch_begin(0, 0);
    
    // rjf: unpack key
    DF_Handle handle = df_handle_from_cfg_tree(window_cfg);
    U64 hash = df_hash_from_string(str8_struct(&handle));
    U64 slot_idx = hash%df_state->window_slots_count;
    DF_WindowSlot *slot = &df_state->window_slots[slot_idx];
    
    // rjf: key * slot -> node
    DF_WindowNode *node = 0;
    for(DF_WindowNode *n = slot->first; n != 0; n = n->next)
    {
      if(df_handle_match(n->v.handle, handle))
      {
        node = n;
        break;
      }
    }
    
    // rjf: need new node? -> allocate & open window
    if(node == 0)
    {
      // rjf: allocate & insert
      Arena *arena = arena_alloc();
      node = push_array(arena, DF_WindowNode, 1);
      DLLPushBack(slot->first, slot->last, node);
      
      // rjf: unpack size from cfg
      Vec2F32 size = v2f32(1280, 720);
      {
        MD_Node *size_cfg = md_child_from_string(window_cfg, str8_lit("size"), 0);
        MD_Node *size_x_cfg = size_cfg->first;
        MD_Node *size_y_cfg = size_x_cfg->next;
        U64 size_x = 0;
        U64 size_y = 0;
        if(try_u64_from_str8_c_rules(size_x_cfg->string, &size_x) &&
           try_u64_from_str8_c_rules(size_y_cfg->string, &size_y))
        {
          size = v2f32((F32)size_x, (F32)size_y);
        }
      }
      
      // rjf: unpack monitor from cfg
      OS_Handle preferred_monitor = os_primary_monitor();
      {
        OS_HandleArray monitors = os_push_monitors_array(scratch.arena);
        MD_Node *monitor_cfg = md_child_from_string(window_cfg, str8_lit("monitor"), 0);
        String8 monitor_name = monitor_cfg->first->string;
        for(U64 idx = 0; idx < monitors.count; idx += 1)
        {
          String8 name = os_name_from_monitor(scratch.arena, monitors.v[idx]);
          if(str8_match(monitor_name, name, StringMatchFlag_CaseInsensitive))
          {
            preferred_monitor = monitors.v[idx];
            break;
          }
        }
      }
      
      // rjf: fill
      DF_Window *w = &node->v;
      w->handle = handle;
      w->first_frame_touched = w->last_frame_touched = df_state->frame_index;
      w->arena = arena;
      w->os = os_window_open(size, OS_WindowFlag_CustomBorder, str8_lit(BUILD_TITLE_STRING_LITERAL));
      os_window_set_monitor(w->os, preferred_monitor);
      w->r = r_window_equip(w->os);
      w->ui = ui_state_alloc();
      w->code_ctx_menu_arena = arena_alloc();
      w->hover_eval_arena = arena_alloc();
      w->autocomp_lister_params_arena = arena_alloc();
      w->free_panel = &df_nil_panel;
      w->root_panel = df_panel_alloc(w);
      w->focused_panel = w->root_panel;
      w->query_cmd_arena = arena_alloc();
      w->query_cmd_spec = &d_nil_cmd_spec;
      w->query_view_stack_top = &df_nil_view;
      w->last_dpi = os_dpi_from_window(w->os);
      for(EachEnumVal(DF_SettingCode, code))
      {
        if(df_g_setting_code_default_is_per_window_table[code])
        {
          w->setting_vals[code] = df_g_setting_code_default_val_table[code];
        }
      }
      D_RegsScope
      {
        d_regs()->window = df_handle_from_window(w);
        DF_FontSlot english_font_slots[] = {DF_FontSlot_Main, DF_FontSlot_Code};
        DF_FontSlot icon_font_slot = DF_FontSlot_Icons;
        for(U64 idx = 0; idx < ArrayCount(english_font_slots); idx += 1)
        {
          Temp scratch = scratch_begin(0, 0);
          DF_FontSlot slot = english_font_slots[idx];
          String8 sample_text = str8_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#$%^&*()-_+=[{]}\\|;:'\",<.>/?");
          fnt_push_run_from_string(scratch.arena,
                                   df_font_from_slot(slot),
                                   df_font_size_from_slot(DF_FontSlot_Code),
                                   0, 0, 0,
                                   sample_text);
          fnt_push_run_from_string(scratch.arena,
                                   df_font_from_slot(slot),
                                   df_font_size_from_slot(DF_FontSlot_Main),
                                   0, 0, 0,
                                   sample_text);
          scratch_end(scratch);
        }
        for(DF_IconKind icon_kind = DF_IconKind_Null; icon_kind < DF_IconKind_COUNT; icon_kind = (DF_IconKind)(icon_kind+1))
        {
          Temp scratch = scratch_begin(0, 0);
          fnt_push_run_from_string(scratch.arena,
                                   df_font_from_slot(icon_font_slot),
                                   df_font_size_from_slot(icon_font_slot),
                                   0, 0, FNT_RasterFlag_Smooth,
                                   df_g_icon_kind_text_table[icon_kind]);
          fnt_push_run_from_string(scratch.arena,
                                   df_font_from_slot(icon_font_slot),
                                   df_font_size_from_slot(DF_FontSlot_Main),
                                   0, 0, FNT_RasterFlag_Smooth,
                                   df_g_icon_kind_text_table[icon_kind]);
          fnt_push_run_from_string(scratch.arena,
                                   df_font_from_slot(icon_font_slot),
                                   df_font_size_from_slot(DF_FontSlot_Code),
                                   0, 0, FNT_RasterFlag_Smooth,
                                   df_g_icon_kind_text_table[icon_kind]);
          scratch_end(scratch);
        }
      }
    }
    
    // rjf: obtain result
    if(node != 0)
    {
      result = &node->v;
    }
    
    scratch_end(scratch);
  }
  return result;
}

internal DF_Window *
df_window_open(Vec2F32 size, OS_Handle preferred_monitor, D_CfgSrc cfg_src)
{
  DF_Window *window = df_state->free_window;
  if(window != 0)
  {
    SLLStackPop(df_state->free_window);
    U64 gen = window->gen;
    MemoryZeroStruct(window);
    window->gen = gen;
  }
  else
  {
    window = push_array(df_state->arena, DF_Window, 1);
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
  window->code_ctx_menu_arena = arena_alloc();
  window->hover_eval_arena = arena_alloc();
  window->autocomp_lister_params_arena = arena_alloc();
  window->free_panel = &df_nil_panel;
  window->root_panel = df_panel_alloc(window);
  window->focused_panel = window->root_panel;
  window->query_cmd_arena = arena_alloc();
  window->query_cmd_spec = &d_nil_cmd_spec;
  window->query_view_stack_top = &df_nil_view;
  window->last_dpi = os_dpi_from_window(window->os);
  for(EachEnumVal(DF_SettingCode, code))
  {
    if(df_g_setting_code_default_is_per_window_table[code])
    {
      window->setting_vals[code] = df_g_setting_code_default_val_table[code];
    }
  }
  OS_Handle zero_monitor = {0};
  if(!os_handle_match(zero_monitor, preferred_monitor))
  {
    os_window_set_monitor(window->os, preferred_monitor);
  }
  if(df_state->first_window == 0) D_RegsScope
  {
    d_regs()->window = df_handle_from_window(window);
    DF_FontSlot english_font_slots[] = {DF_FontSlot_Main, DF_FontSlot_Code};
    DF_FontSlot icon_font_slot = DF_FontSlot_Icons;
    for(U64 idx = 0; idx < ArrayCount(english_font_slots); idx += 1)
    {
      Temp scratch = scratch_begin(0, 0);
      DF_FontSlot slot = english_font_slots[idx];
      String8 sample_text = str8_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#$%^&*()-_+=[{]}\\|;:'\",<.>/?");
      fnt_push_run_from_string(scratch.arena,
                               df_font_from_slot(slot),
                               df_font_size_from_slot(DF_FontSlot_Code),
                               0, 0, 0,
                               sample_text);
      fnt_push_run_from_string(scratch.arena,
                               df_font_from_slot(slot),
                               df_font_size_from_slot(DF_FontSlot_Main),
                               0, 0, 0,
                               sample_text);
      scratch_end(scratch);
    }
    for(DF_IconKind icon_kind = DF_IconKind_Null; icon_kind < DF_IconKind_COUNT; icon_kind = (DF_IconKind)(icon_kind+1))
    {
      Temp scratch = scratch_begin(0, 0);
      fnt_push_run_from_string(scratch.arena,
                               df_font_from_slot(icon_font_slot),
                               df_font_size_from_slot(icon_font_slot),
                               0, 0, FNT_RasterFlag_Smooth,
                               df_g_icon_kind_text_table[icon_kind]);
      fnt_push_run_from_string(scratch.arena,
                               df_font_from_slot(icon_font_slot),
                               df_font_size_from_slot(DF_FontSlot_Main),
                               0, 0, FNT_RasterFlag_Smooth,
                               df_g_icon_kind_text_table[icon_kind]);
      fnt_push_run_from_string(scratch.arena,
                               df_font_from_slot(icon_font_slot),
                               df_font_size_from_slot(DF_FontSlot_Code),
                               0, 0, FNT_RasterFlag_Smooth,
                               df_g_icon_kind_text_table[icon_kind]);
      scratch_end(scratch);
    }
  }
  DLLPushBack(df_state->first_window, df_state->last_window, window);
  return window;
}

internal DF_Window *
df_window_from_os_handle(OS_Handle os)
{
  DF_Window *result = 0;
  for(DF_Window *w = df_state->first_window; w != 0; w = w->next)
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
df_window_frame(MD_Node *window_cfg)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_Window *ws = df_window_from_cfg_tree(window_cfg);
  
  //////////////////////////////
  //- rjf: unpack context
  //
  B32 window_is_focused = os_window_is_focused(ws->os) || ws->window_temporarily_focused_ipc;
  B32 confirm_open = df_state->confirm_active;
  B32 query_is_open = !df_view_is_nil(ws->query_view_stack_top);
  B32 hover_eval_is_open = (!confirm_open &&
                            ws->hover_eval_string.size != 0 &&
                            ws->hover_eval_first_frame_idx+20 < ws->hover_eval_last_frame_idx &&
                            d_frame_index()-ws->hover_eval_last_frame_idx < 20);
  if(!window_is_focused || confirm_open)
  {
    ws->menu_bar_key_held = 0;
  }
  ws->window_temporarily_focused_ipc = 0;
  ui_select_state(ws->ui);
  
  ////////////////////////////
  //- rjf: unpack panel tree info
  //
  typedef struct PanelTask PanelTask;
  struct PanelTask
  {
    PanelTask *next;
    MD_Node *root;
    S32 depth;
    Axis2 split_axis;
  };
  MD_Node *focused_panel_cfg = &md_nil_node;
  PanelTask *first_panel_task = 0;
  ProfScope("gather all panel cfg trees")
  {
    Axis2 root_split_axis = md_node_is_nil(md_child_from_string(window_cfg, str8_lit("split_x"), 0)) ? Axis2_Y : Axis2_X;
    MD_Node *panels_cfg = df_panel_tree_from_window_cfg(window_cfg);
    if(!md_node_is_nil(panels_cfg))
    {
      PanelTask *start_task = push_array(scratch.arena, PanelTask, 1);
      first_panel_task = start_task;
      start_task->root = panels_cfg;
      start_task->split_axis = root_split_axis;
      for(PanelTask *t = first_panel_task; t != 0; t = t->next)
      {
        if(!md_node_is_nil(md_tag_from_string(t->root, str8_lit("selected"), 0)))
        {
          focused_panel_cfg = t->root;
        }
        for(MD_Node *child = md_node_from_chain_flags(t->root->first, &md_nil_node, MD_NodeFlag_Numeric);
            !md_node_is_nil(child);
            child = md_node_from_chain_flags(child->next, &md_nil_node, MD_NodeFlag_Numeric))
        {
          PanelTask *task = push_array(scratch.arena, PanelTask, 1);
          task->next = t->next;
          t->next = task;
          task->root = child;
          task->depth = t->depth+1;
          task->split_axis = (task->depth & 1) ? axis2_flip(root_split_axis) : root_split_axis;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: auto-close tabs which have parameter entities that've been deleted
  //
#if 0 // TODO(rjf): @msgs
  for(DF_Panel *panel = ws->root_panel;
      !df_panel_is_nil(panel);
      panel = df_panel_rec_df_pre(panel).next)
  {
    for(DF_View *view = panel->first_tab_view;
        !df_view_is_nil(view);
        view = view->order_next)
    {
      D_Entity *entity = d_entity_from_eval_string(str8(view->query_buffer, view->query_string_size));
      if(entity->flags & D_EntityFlag_MarkedForDeletion ||
         (d_entity_is_nil(entity) && view->spec->info.flags & DF_ViewSpecFlag_ParameterizedByEntity))
      {
        D_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        d_cmd_list_push(arena, cmds, &params, d_cmd_spec_from_kind(D_CmdKind_CloseTab));
      }
    }
  }
#endif
  
  //////////////////////////////
  //- rjf: do core-layer commands & batch up commands to be dispatched to views
  //
  UI_EventList events = {0};
#if 0 // TODO(rjf): @msgs
  ProfScope("do commands")
  {
    Temp scratch = scratch_begin(&arena, 1);
    for(D_CmdNode *cmd_node = cmds->first;
        cmd_node != 0;
        cmd_node = cmd_node->next)
    {
      temp_end(scratch);
      
      // rjf: get command info
      D_Cmd *cmd = &cmd_node->cmd;
      D_CmdParams params = cmd->params;
      D_CmdKind kind = d_cmd_kind_from_string(cmd->spec->info.string);
      
      // rjf: mismatched window => skip
      if(df_window_from_handle(params.window) != ws)
      {
        continue;
      }
      
      // rjf: set up data for cases
      Dir2 split_dir = Dir2_Invalid;
      DF_Panel *split_panel = ws->focused_panel;
      U64 panel_sib_off = 0;
      U64 panel_child_off = 0;
      Vec2S32 panel_change_dir = {0};
      
      // rjf: dispatch by core command kind
      switch(kind)
      {
        //- rjf: OS events
        case D_CmdKind_OSEvent:
        {
          OS_Event *os_event = params.os_event;
          if(os_event != 0 && os_handle_match(os_event->window, ws->os))
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
            ui_event.modifiers    = os_event->flags;
            ui_event.string       = os_event->character ? str8_from_32(ui_build_arena(), str32(&os_event->character, 1)) : str8_zero();
            ui_event.paths        = str8_list_copy(ui_build_arena(), &os_event->strings);
            ui_event.pos          = os_event->pos;
            ui_event.delta_2f32   = os_event->delta;
            ui_event.timestamp_us = os_event->timestamp_us;
            ui_event_list_push(ui_build_arena(), &events, &ui_event);
          }
        }break;
        
        //- rjf: meta controls
        case D_CmdKind_Edit:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Press;
          evt.slot       = UI_EventActionSlot_Edit;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_Accept:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Press;
          evt.slot       = UI_EventActionSlot_Accept;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_Cancel:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Press;
          evt.slot       = UI_EventActionSlot_Cancel;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        
        //- rjf: directional movement & text controls
        //
        // NOTE(rjf): These all get funneled into a separate intermediate that
        // can be used by the UI build phase for navigation and stuff, as well
        // as builder codepaths that want to use these controls to modify text.
        //
        case D_CmdKind_MoveLeft:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveRight:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUp:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDown:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveLeftSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveRightSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveLeftChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveRightChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpPage:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownPage:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpWhole:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownWhole:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveLeftChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveRightChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpPageSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownPageSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpWholeSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownWholeSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveUpReorder:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_Reorder;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveDownReorder:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_Reorder;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveHome:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveEnd:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveHomeSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_MoveEndSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_SelectAll:
        {
          UI_Event evt1 = zero_struct;
          evt1.kind       = UI_EventKind_Navigate;
          evt1.delta_unit = UI_EventDeltaUnit_Whole;
          evt1.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt1);
          UI_Event evt2 = zero_struct;
          evt2.kind       = UI_EventKind_Navigate;
          evt2.flags      = UI_EventFlag_KeepMark;
          evt2.delta_unit = UI_EventDeltaUnit_Whole;
          evt2.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt2);
        }break;
        case D_CmdKind_DeleteSingle:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_DeleteChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_BackspaceSingle:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete|UI_EventFlag_ZeroDeltaOnSelect;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_BackspaceChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_Copy:
        {
          UI_Event evt = zero_struct;
          evt.kind  = UI_EventKind_Edit;
          evt.flags = UI_EventFlag_Copy|UI_EventFlag_KeepMark;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_Cut:
        {
          UI_Event evt = zero_struct;
          evt.kind  = UI_EventKind_Edit;
          evt.flags = UI_EventFlag_Copy|UI_EventFlag_Delete;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_Paste:
        {
          UI_Event evt = zero_struct;
          evt.kind   = UI_EventKind_Text;
          evt.string = os_get_clipboard_text(ui_build_arena());
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case D_CmdKind_InsertText:
        {
          UI_Event evt = zero_struct;
          evt.kind   = UI_EventKind_Text;
          evt.string = params.string;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
      }
    }
    scratch_end(scratch);
  }
#endif
  
  //////////////////////////////
  //- rjf: panels with no selected tabs? -> select.
  // panels with selected tabs? -> ensure they have active tabs.
  //
  for(DF_Panel *panel = ws->root_panel;
      !df_panel_is_nil(panel);
      panel = df_panel_rec_df_pre(panel).next)
  {
    if(!df_panel_is_nil(panel->first))
    {
      continue;
    }
    DF_View *view = df_selected_tab_from_panel(panel);
    if(df_view_is_nil(view))
    {
      for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->order_next)
      {
        if(!df_view_is_project_filtered(tab))
        {
          panel->selected_tab_view = df_handle_from_view(tab);
          break;
        }
      }
    }
    if(!df_view_is_nil(view))
    {
      B32 found = 0;
      for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->order_next)
      {
        if(df_view_is_project_filtered(tab)) {continue;}
        if(tab == view)
        {
          found = 1;
        }
      }
      if(!found)
      {
        panel->selected_tab_view = d_handle_zero();
      }
    }
  }
  
  //////////////////////////////
  //- rjf: fill panel/view interaction registers
  //
  d_regs()->panel  = df_handle_from_panel(ws->focused_panel);
  d_regs()->view   = ws->focused_panel->selected_tab_view;
  
  //////////////////////////////
  //- rjf: process view-level commands on leaf panels
  //
#if 0 // TODO(rjf): @msgs
  ProfScope("dispatch view-level commands")
  {
    for(DF_Panel *panel = ws->root_panel;
        !df_panel_is_nil(panel);
        panel = df_panel_rec_df_pre(panel).next)
    {
      if(!df_panel_is_nil(panel->first))
      {
        continue;
      }
      DF_View *view = df_selected_tab_from_panel(panel);
      if(!df_view_is_nil(view))
      {
        d_push_regs();
        d_regs()->panel = df_handle_from_panel(panel);
        d_regs()->view  = df_handle_from_view(view);
        DF_ViewCmdFunctionType *do_view_cmds_function = view->spec->info.cmd_hook;
        do_view_cmds_function(view, view->params_roots[view->params_read_gen%ArrayCount(view->params_roots)], str8(view->query_buffer, view->query_string_size), cmds);
        D_Regs *view_regs = d_pop_regs();
        if(panel == ws->focused_panel)
        {
          MemoryCopyStruct(d_regs(), view_regs);
        }
      }
    }
  }
#endif
  
  //////////////////////////////
  //- rjf: compute ui palettes from theme
  //
  {
    DF_Theme *current = &df_state->cfg_theme;
    for(EachEnumVal(DF_PaletteCode, code))
    {
      ws->cfg_palettes[code].null       = v4f32(1, 0, 1, 1);
      ws->cfg_palettes[code].cursor     = current->colors[DF_ThemeColor_Cursor];
      ws->cfg_palettes[code].selection  = current->colors[DF_ThemeColor_SelectionOverlay];
    }
    ws->cfg_palettes[DF_PaletteCode_Base].background = current->colors[DF_ThemeColor_BaseBackground];
    ws->cfg_palettes[DF_PaletteCode_Base].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_Base].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_Base].border     = current->colors[DF_ThemeColor_BaseBorder];
    ws->cfg_palettes[DF_PaletteCode_MenuBar].background = current->colors[DF_ThemeColor_MenuBarBackground];
    ws->cfg_palettes[DF_PaletteCode_MenuBar].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_MenuBar].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_MenuBar].border     = current->colors[DF_ThemeColor_MenuBarBorder];
    ws->cfg_palettes[DF_PaletteCode_Floating].background = current->colors[DF_ThemeColor_FloatingBackground];
    ws->cfg_palettes[DF_PaletteCode_Floating].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_Floating].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_Floating].border     = current->colors[DF_ThemeColor_FloatingBorder];
    ws->cfg_palettes[DF_PaletteCode_ImplicitButton].background = current->colors[DF_ThemeColor_ImplicitButtonBackground];
    ws->cfg_palettes[DF_PaletteCode_ImplicitButton].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_ImplicitButton].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_ImplicitButton].border     = current->colors[DF_ThemeColor_ImplicitButtonBorder];
    ws->cfg_palettes[DF_PaletteCode_PlainButton].background = current->colors[DF_ThemeColor_PlainButtonBackground];
    ws->cfg_palettes[DF_PaletteCode_PlainButton].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_PlainButton].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_PlainButton].border     = current->colors[DF_ThemeColor_PlainButtonBorder];
    ws->cfg_palettes[DF_PaletteCode_PositivePopButton].background = current->colors[DF_ThemeColor_PositivePopButtonBackground];
    ws->cfg_palettes[DF_PaletteCode_PositivePopButton].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_PositivePopButton].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_PositivePopButton].border     = current->colors[DF_ThemeColor_PositivePopButtonBorder];
    ws->cfg_palettes[DF_PaletteCode_NegativePopButton].background = current->colors[DF_ThemeColor_NegativePopButtonBackground];
    ws->cfg_palettes[DF_PaletteCode_NegativePopButton].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_NegativePopButton].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_NegativePopButton].border     = current->colors[DF_ThemeColor_NegativePopButtonBorder];
    ws->cfg_palettes[DF_PaletteCode_NeutralPopButton].background = current->colors[DF_ThemeColor_NeutralPopButtonBackground];
    ws->cfg_palettes[DF_PaletteCode_NeutralPopButton].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_NeutralPopButton].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_NeutralPopButton].border     = current->colors[DF_ThemeColor_NeutralPopButtonBorder];
    ws->cfg_palettes[DF_PaletteCode_ScrollBarButton].background = current->colors[DF_ThemeColor_ScrollBarButtonBackground];
    ws->cfg_palettes[DF_PaletteCode_ScrollBarButton].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_ScrollBarButton].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_ScrollBarButton].border     = current->colors[DF_ThemeColor_ScrollBarButtonBorder];
    ws->cfg_palettes[DF_PaletteCode_Tab].background = current->colors[DF_ThemeColor_TabBackground];
    ws->cfg_palettes[DF_PaletteCode_Tab].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_Tab].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_Tab].border     = current->colors[DF_ThemeColor_TabBorder];
    ws->cfg_palettes[DF_PaletteCode_TabInactive].background = current->colors[DF_ThemeColor_TabBackgroundInactive];
    ws->cfg_palettes[DF_PaletteCode_TabInactive].text       = current->colors[DF_ThemeColor_Text];
    ws->cfg_palettes[DF_PaletteCode_TabInactive].text_weak  = current->colors[DF_ThemeColor_TextWeak];
    ws->cfg_palettes[DF_PaletteCode_TabInactive].border     = current->colors[DF_ThemeColor_TabBorderInactive];
    ws->cfg_palettes[DF_PaletteCode_DropSiteOverlay].background = current->colors[DF_ThemeColor_DropSiteOverlay];
    ws->cfg_palettes[DF_PaletteCode_DropSiteOverlay].text       = current->colors[DF_ThemeColor_DropSiteOverlay];
    ws->cfg_palettes[DF_PaletteCode_DropSiteOverlay].text_weak  = current->colors[DF_ThemeColor_DropSiteOverlay];
    ws->cfg_palettes[DF_PaletteCode_DropSiteOverlay].border     = current->colors[DF_ThemeColor_DropSiteOverlay];
    if(df_setting_val_from_code(DF_SettingCode_OpaqueBackgrounds).s32)
    {
      for(EachEnumVal(DF_PaletteCode, code))
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
  UI_Box *autocomp_box = &ui_g_nil_box;
  UI_Box *hover_eval_box = &ui_g_nil_box;
  ProfScope("build UI")
  {
    ////////////////////////////
    //- rjf: set up
    //
    {
      // rjf: gather font info
      FNT_Tag main_font = df_font_from_slot(DF_FontSlot_Main);
      F32 main_font_size = df_font_size_from_slot(DF_FontSlot_Main);
      FNT_Tag icon_font = df_font_from_slot(DF_FontSlot_Icons);
      
      // rjf: build icon info
      UI_IconInfo icon_info = {0};
      {
        icon_info.icon_font = icon_font;
        icon_info.icon_kind_text_map[UI_IconKind_RightArrow]     = df_g_icon_kind_text_table[DF_IconKind_RightScroll];
        icon_info.icon_kind_text_map[UI_IconKind_DownArrow]      = df_g_icon_kind_text_table[DF_IconKind_DownScroll];
        icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]      = df_g_icon_kind_text_table[DF_IconKind_LeftScroll];
        icon_info.icon_kind_text_map[UI_IconKind_UpArrow]        = df_g_icon_kind_text_table[DF_IconKind_UpScroll];
        icon_info.icon_kind_text_map[UI_IconKind_RightCaret]     = df_g_icon_kind_text_table[DF_IconKind_RightCaret];
        icon_info.icon_kind_text_map[UI_IconKind_DownCaret]      = df_g_icon_kind_text_table[DF_IconKind_DownCaret];
        icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]      = df_g_icon_kind_text_table[DF_IconKind_LeftCaret];
        icon_info.icon_kind_text_map[UI_IconKind_UpCaret]        = df_g_icon_kind_text_table[DF_IconKind_UpCaret];
        icon_info.icon_kind_text_map[UI_IconKind_CheckHollow]    = df_g_icon_kind_text_table[DF_IconKind_CheckHollow];
        icon_info.icon_kind_text_map[UI_IconKind_CheckFilled]    = df_g_icon_kind_text_table[DF_IconKind_CheckFilled];
      }
      
      // rjf: build widget palette info
      UI_WidgetPaletteInfo widget_palette_info = {0};
      {
        widget_palette_info.tooltip_palette   = df_palette_from_code(DF_PaletteCode_Floating);
        widget_palette_info.ctx_menu_palette  = df_palette_from_code(DF_PaletteCode_Floating);
        widget_palette_info.scrollbar_palette = df_palette_from_code(DF_PaletteCode_ScrollBarButton);
      }
      
      // rjf: build animation info
      UI_AnimationInfo animation_info = {0};
      {
        if(df_setting_val_from_code(DF_SettingCode_HoverAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_HotAnimations;}
        if(df_setting_val_from_code(DF_SettingCode_PressAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_ActiveAnimations;}
        if(df_setting_val_from_code(DF_SettingCode_FocusAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_FocusAnimations;}
        if(df_setting_val_from_code(DF_SettingCode_TooltipAnimations).s32)     {animation_info.flags |= UI_AnimationInfoFlag_TooltipAnimations;}
        if(df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32)        {animation_info.flags |= UI_AnimationInfoFlag_ContextMenuAnimations;}
        if(df_setting_val_from_code(DF_SettingCode_ScrollingAnimations).s32)   {animation_info.flags |= UI_AnimationInfoFlag_ScrollingAnimations;}
      }
      
      // rjf: begin & push initial stack values
      ui_begin_build(ws->os, &events, &icon_info, &widget_palette_info, &animation_info, d_dt(), d_dt());
      ui_push_font(main_font);
      ui_push_font_size(main_font_size);
      ui_push_text_padding(main_font_size*0.3f);
      ui_push_pref_width(ui_em(20.f, 1));
      ui_push_pref_height(ui_em(2.75f, 1.f));
      ui_push_palette(df_palette_from_code(DF_PaletteCode_Base));
      ui_push_blur_size(10.f);
      FNT_RasterFlags text_raster_flags = 0;
      if(df_setting_val_from_code(DF_SettingCode_SmoothUIText).s32) {text_raster_flags |= FNT_RasterFlag_Smooth;}
      if(df_setting_val_from_code(DF_SettingCode_HintUIText).s32) {text_raster_flags |= FNT_RasterFlag_Hinted;}
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
    Vec2F32 content_rect_dim = dim_2f32(content_rect);
    
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
    //- rjf: drag/drop visualization tooltips
    //
    B32 drag_active = df_drag_is_active();
    if(drag_active && window_is_focused)
    {
      Temp scratch = scratch_begin(0, 0);
      {
        //- rjf: tab dragging
        MD_Node *tab_cfg = df_cfg_tree_from_handle(df_state->drag_drop_regs->tab);
        if(!md_node_is_nil(tab_cfg)) DF_RegsScope(.tab = df_state->drag_drop_regs->tab)
        {
          MD_Node *expr_cfg = tab_cfg->first;
          MD_Node *spec_cfg = expr_cfg->first;
          DF_ViewSpec *spec = df_view_spec_from_string(spec_cfg->string);
          UI_Size main_width = ui_top_pref_width();
          UI_Size main_height = ui_top_pref_height();
          UI_TextAlign main_text_align = ui_top_text_alignment();
          DF_Palette(DF_PaletteCode_Tab)
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
                DF_IconKind icon_kind = spec->info.icon_kind;
                DR_FancyStringList fstrs = df_title_fstrs_from_view_spec_query(scratch.arena, spec, expr_cfg->string, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
                DF_Font(DF_FontSlot_Icons)
                  UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons))
                  UI_PrefWidth(ui_em(2.5f, 1.f))
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  ui_label(df_g_icon_kind_text_table[icon_kind]);
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
                spec->info.ui_hook(view_preview_container->rect);
              }
            }
          }
        }
        
        //- rjf: entity dragging
#if 0 // TODO(rjf): @msgs
        else if(!d_entity_is_nil(entity)) UI_Tooltip
        {
          ui_set_next_pref_width(ui_children_sum(1));
          UI_Row UI_HeightFill
          {
            String8 display_name = d_display_string_from_entity(scratch.arena, entity);
            DF_IconKind icon_kind = df_entity_kind_icon_kind_table[entity->kind];
            DF_Font(DF_FontSlot_Icons)
              UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons))
              UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
              ui_label(df_g_icon_kind_text_table[icon_kind]);
            ui_label(display_name);
          }
        }
#endif
      }
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: developer menu
    //
    if(ws->dev_menu_is_open) DF_Font(DF_FontSlot_Code)
    {
      ui_set_next_flags(UI_BoxFlag_ViewScrollY|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_ViewClamp);
      UI_PaneF(r2f32p(30, 30, 30+ui_top_font_size()*100, ui_top_font_size()*150), "###dev_ctx_menu")
      {
        //- rjf: toggles
        for(U64 idx = 0; idx < ArrayCount(DEV_toggle_table); idx += 1)
        {
          if(ui_clicked(df_icon_button(*DEV_toggle_table[idx].value_ptr ? DF_IconKind_CheckFilled : DF_IconKind_CheckHollow, 0, DEV_toggle_table[idx].name)))
          {
            *DEV_toggle_table[idx].value_ptr ^= 1;
          }
        }
        
        ui_divider(ui_em(1.f, 1.f));
        
        //- rjf: draw current interaction regs
        {
          D_Regs *regs = d_regs();
#define Handle(name) ui_labelf("%s: [0x%I64x, 0x%I64x]", #name, (regs->name).u64[0], (regs->name).u64[1])
          Handle(window);
          Handle(panel);
          Handle(view);
          Handle(module);
          Handle(process);
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
        for(DF_Window *window = df_state->first_window; window != 0; window = window->next)
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
          ui_labelf("Target Hz: %.2f", 1.f/d_dt());
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
#if 0 // TODO(rjf): @msgs
        D_EntityRec rec = {0};
        S32 indent = 0;
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("Entity Tree:");
        for(D_Entity *e = d_entity_root(); !d_entity_is_nil(e); e = rec.next)
        {
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_pref_height(ui_children_sum(1));
          UI_Row
          {
            ui_spacer(ui_em(2.f*indent, 1.f));
            D_Entity *dst = d_entity_from_handle(e->entity_handle);
            if(!d_entity_is_nil(dst))
            {
              ui_labelf("[link] %S -> %S", e->string, dst->string);
            }
            else
            {
              ui_labelf("%S: %S", d_entity_kind_display_string_table[e->kind], e->string);
            }
          }
          rec = d_entity_rec_depth_first_pre(e, d_entity_root());
          indent += rec.push_count;
          indent -= rec.pop_count;
        }
#endif
      }
    }
    
    ////////////////////////////
    //- rjf: universal ctx menus
    //
    DF_Palette(DF_PaletteCode_Floating)
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: auto-close entity ctx menu
      if(ui_ctx_menu_is_open(df_state->entity_ctx_menu_key))
      {
        D_Entity *entity = d_entity_from_handle(ws->entity_ctx_menu_entity);
        if(d_entity_is_nil(entity))
        {
          ui_ctx_menu_close();
        }
      }
      
      //- rjf: code ctx menu
      UI_CtxMenu(df_state->code_ctx_menu_key)
        UI_PrefWidth(ui_em(40.f, 1.f))
        DF_Palette(DF_PaletteCode_ImplicitButton)
      {
        TXT_Scope *txt_scope = txt_scope_open();
        HS_Scope *hs_scope = hs_scope_open();
        TxtRng range = ws->code_ctx_menu_range;
        D_LineList lines = ws->code_ctx_menu_lines;
        if(!txt_pt_match(range.min, range.max) && ui_clicked(df_cmd_spec_button(d_cmd_spec_from_kind(D_CmdKind_Copy))))
        {
          U128 hash = {0};
          TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, ws->code_ctx_menu_text_key, ws->code_ctx_menu_lang_kind, &hash);
          String8 data = hs_data_from_hash(hs_scope, hash);
          String8 copy_data = txt_string_from_info_data_txt_rng(&info, data, ws->code_ctx_menu_range);
          os_set_clipboard_text(copy_data);
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_RightArrow, 0, "Set Next Statement")))
        {
          CTRL_Entity *thread = ctrl_entity_from_machine_id_handle(d_state->ctrl_entity_store, d_regs()->machine_id, d_regs()->thread);
          U64 new_rip_vaddr = ws->code_ctx_menu_vaddr;
          if(ws->code_ctx_menu_file_path.size != 0)
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
          d_msg(D_MsgKind_SetThreadIP, .vaddr_range = r1u64(new_rip_vaddr, new_rip_vaddr));
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_Play, 0, "Run To Line")))
        {
          if(ws->code_ctx_menu_file_path.size != 0)
          {
            d_msg(D_MsgKind_RunToLine, .file_path = ws->code_ctx_menu_file_path, .cursor = range.min);
          }
          else
          {
            d_msg(D_MsgKind_RunToAddress, .vaddr_range = r1u64(ws->code_ctx_menu_vaddr, ws->code_ctx_menu_vaddr));
          }
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_Null, 0, "Go To Name")))
        {
          U128 hash = {0};
          TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, ws->code_ctx_menu_text_key, ws->code_ctx_menu_lang_kind, &hash);
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
          df_msg(DF_MsgKind_GoToName, .string = expr);
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_CircleFilled, 0, "Toggle Breakpoint")))
        {
          df_msg(DF_MsgKind_ToggleBreakpoint,
                 .file_path  = ws->code_ctx_menu_file_path,
                 .cursor     = range.min,
                 .vaddr_range= r1u64(ws->code_ctx_menu_vaddr, ws->code_ctx_menu_vaddr));
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_Binoculars, 0, "Toggle Watch Expression")))
        {
          U128 hash = {0};
          TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, ws->code_ctx_menu_text_key, ws->code_ctx_menu_lang_kind, &hash);
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
          df_msg(DF_MsgKind_ToggleWatchExpression, .string = expr);
          ui_ctx_menu_close();
        }
        if(ws->code_ctx_menu_file_path.size == 0 && range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Go To Source")))
        {
          if(lines.first != 0)
          {
            df_msg(DF_MsgKind_FindCodeLocation,
                   .file_path = lines.first->v.file_path,
                   .cursor = lines.first->v.pt);
          }
          ui_ctx_menu_close();
        }
        if(ws->code_ctx_menu_file_path.size != 0 && range.min.line == range.max.line && ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Go To Disassembly")))
        {
          CTRL_Entity *thread = ctrl_entity_from_machine_id_handle(d_state->ctrl_entity_store, d_regs()->machine_id, d_regs()->thread);
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
          df_msg(DF_MsgKind_FindCodeLocation, .vaddr_range = r1u64(vaddr, vaddr));
          ui_ctx_menu_close();
        }
        hs_scope_close(hs_scope);
        txt_scope_close(txt_scope);
      }
      
      //- rjf: entity menu
#if 0 // TODO(rjf): @msgs
      UI_CtxMenu(df_state->entity_ctx_menu_key)
        UI_PrefWidth(ui_em(40.f, 1.f))
        DF_Palette(DF_PaletteCode_ImplicitButton)
      {
        D_Entity *entity = d_entity_from_handle(ws->entity_ctx_menu_entity);
        DF_IconKind entity_icon = df_entity_kind_icon_kind_table[entity->kind];
        D_EntityKindFlags kind_flags = d_entity_kind_flags_table[entity->kind];
        String8 display_name = d_display_string_from_entity(scratch.arena, entity);
        
        // rjf: title
        UI_Row
        {
          ui_spacer(ui_em(1.f, 1.f));
          DF_Font(DF_FontSlot_Icons)
            UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons))
            UI_PrefWidth(ui_em(2.f, 1.f))
            UI_PrefHeight(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_Flags(UI_BoxFlag_DrawTextWeak)
            ui_label(df_g_icon_kind_text_table[entity_icon]);
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_Flags(UI_BoxFlag_DrawTextWeak)
            ui_label(d_entity_kind_display_string_table[entity->kind]);
          {
            UI_Palette *palette = ui_top_palette();
            if(entity->flags & D_EntityFlag_HasColor)
            {
              palette = ui_build_palette(ui_top_palette(), .text = d_rgba_from_entity(entity));
            }
            UI_Palette(palette)
              UI_PrefWidth(ui_text_dim(10, 1))
              DF_Font((kind_flags & D_EntityKindFlag_NameIsCode) ? DF_FontSlot_Code : DF_FontSlot_Main)
              ui_label(display_name);
          }
        }
        
        DF_Palette(DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
        
        // rjf: name editor
        if(kind_flags & D_EntityKindFlag_CanRename) UI_TextPadding(ui_top_font_size()*1.5f)
        {
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, entity->string, "%S###entity_name_edit_%p", d_entity_kind_name_label_table[entity->kind], entity);
          if(ui_committed(sig))
          {
            df_msg(DF_MsgKind_NameEntity,
                   .entity = d_handle_from_entity(entity),
                   .string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size));
          }
        }
        
        // rjf: condition editor
        if(kind_flags & D_EntityKindFlag_CanCondition)
          DF_Font(DF_FontSlot_Code)
          UI_TextPadding(ui_top_font_size()*1.5f)
        {
          D_Entity *condition = d_entity_child_from_kind(entity, D_EntityKind_Condition);
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border|DF_LineEditFlag_CodeContents, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, condition->string, "Condition###entity_cond_edit_%p", entity);
          if(ui_committed(sig))
          {
            String8 new_string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            if(new_string.size != 0)
            {
              if(d_entity_is_nil(condition))
              {
                condition = d_entity_alloc(entity, D_EntityKind_Condition);
              }
              df_msg(DF_MsgKind_NameEntity, .entity = d_handle_from_entity(condition), .string = new_string);
            }
            else if(!d_entity_is_nil(condition))
            {
              d_entity_mark_for_deletion(condition);
            }
          }
        }
        
        // rjf: exe editor
        if(entity->kind == D_EntityKind_Target) UI_TextPadding(ui_top_font_size()*1.5f)
        {
          D_Entity *exe = d_entity_child_from_kind(entity, D_EntityKind_Executable);
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, exe->string, "Executable###entity_exe_edit_%p", entity);
          if(ui_committed(sig))
          {
            String8 new_string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            if(new_string.size != 0)
            {
              if(d_entity_is_nil(exe))
              {
                exe = d_entity_alloc(entity, D_EntityKind_Executable);
              }
              df_msg(DF_MsgKind_NameEntity, .entity = d_handle_from_entity(exe), .string = new_string);
            }
            else if(!d_entity_is_nil(exe))
            {
              d_entity_mark_for_deletion(exe);
            }
          }
        }
        
        // rjf: arguments editors
        if(entity->kind == D_EntityKind_Target) UI_TextPadding(ui_top_font_size()*1.5f)
        {
          D_Entity *args = d_entity_child_from_kind(entity, D_EntityKind_Arguments);
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, args->string, "Arguments###entity_args_edit_%p", entity);
          if(ui_committed(sig))
          {
            String8 new_string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            if(new_string.size != 0)
            {
              if(d_entity_is_nil(args))
              {
                args = d_entity_alloc(entity, D_EntityKind_Arguments);
              }
              df_msg(DF_MsgKind_NameEntity, .entity = d_handle_from_entity(args), .string = new_string);
            }
            else if(!d_entity_is_nil(args))
            {
              d_entity_mark_for_deletion(args);
            }
          }
        }
        
        // rjf: copy name
        if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Name")))
        {
          os_set_clipboard_text(display_name);
          ui_ctx_menu_close();
        }
        
        // rjf: is command line only? -> make permanent
        if(entity->cfg_src == D_CfgSrc_CommandLine && ui_clicked(df_icon_buttonf(DF_IconKind_Save, 0, "Save To Project")))
        {
          d_entity_equip_cfg_src(entity, D_CfgSrc_Project);
        }
        
        // rjf: duplicate
        if(kind_flags & D_EntityKindFlag_CanDuplicate && ui_clicked(df_icon_buttonf(DF_IconKind_XSplit, 0, "Duplicate")))
        {
          df_msg(DF_MsgKind_DuplicateEntity, .entity = d_handle_from_entity(entity));
          ui_ctx_menu_close();
        }
        
        // rjf: edit
        if(kind_flags & D_EntityKindFlag_CanEdit && ui_clicked(df_icon_buttonf(DF_IconKind_Pencil, 0, "Edit")))
        {
          df_msg(DF_MsgKind_EditEntity, .entity = d_handle_from_entity(entity));
          ui_ctx_menu_close();
        }
        
        // rjf: deletion
        if(kind_flags & D_EntityKindFlag_CanDelete && ui_clicked(df_icon_buttonf(DF_IconKind_Trash, 0, "Delete")))
        {
          df_msg(DF_MsgKind_RemoveEntity, .entity = d_handle_from_entity(entity));
          ui_ctx_menu_close();
        }
        
        // rjf: enabling
        if(kind_flags & D_EntityKindFlag_CanEnable)
        {
          B32 is_enabled = !entity->disabled;
          if(!is_enabled && ui_clicked(df_icon_buttonf(DF_IconKind_CheckHollow, 0, "Enable###enabler")))
          {
            df_msg(DF_MsgKind_EnableEntity, .entity = d_handle_from_entity(entity));
          }
          if(is_enabled && ui_clicked(df_icon_buttonf(DF_IconKind_CheckFilled, 0, "Disable###enabler")))
          {
            df_msg(DF_MsgKind_DisableEntity, .entity = d_handle_from_entity(entity));
          }
        }
        
        // rjf: freezing
        if(kind_flags & D_EntityKindFlag_CanFreeze)
        {
          // TODO(rjf): @msgs
#if 0
          d_entity_is_frozen(entity);
          ui_set_next_palette(df_palette_from_code(is_frozen ? DF_PaletteCode_NegativePopButton : DF_PaletteCode_PositivePopButton));
          if(is_frozen && ui_clicked(df_icon_buttonf(DF_IconKind_Locked, 0, "Thaw###freeze_thaw")))
          {
            d_msg(D_MsgKind_ThawEntity, .entity = d_handle_from_entity(entity));
          }
          if(!is_frozen && ui_clicked(df_icon_buttonf(DF_IconKind_Unlocked, 0, "Freeze###freeze_thaw")))
          {
            d_msg(D_MsgKind_FreezeEntity, .entity = d_handle_from_entity(entity));
          }
#endif
        }
        
        // rjf: go-to-location
        {
          D_Entity *loc = d_entity_child_from_kind(entity, D_EntityKind_Location);
          if(!d_entity_is_nil(loc) && ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Go To Location")))
          {
            df_msg(DF_MsgKind_FindCodeLocation,
                   .file_path  = loc->string,
                   .text_point = loc->text_point,
                   .vaddr      = loc->vaddr);
            ui_ctx_menu_close();
          }
        }
        
        // rjf: entity-kind-specific options
        switch(entity->kind)
        {
          default:
          {
          }break;
          
          case D_EntityKind_Process:
          case D_EntityKind_Thread:
          {
            if(entity->kind == D_EntityKind_Thread)
            {
              B32 is_selected = d_handle_match(d_base_regs()->thread, d_handle_from_entity(entity));
              if(is_selected)
              {
                df_icon_buttonf(DF_IconKind_Thread, 0, "[Selected]###select_entity");
              }
              else if(ui_clicked(df_icon_buttonf(DF_IconKind_Thread, 0, "Select###select_entity")))
              {
                d_msg(D_MsgKind_SelectThread, .entity = d_handle_from_entity(entity));
                ui_ctx_menu_close();
              }
            }
            
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy ID")))
            {
              U32 ctrl_id = entity->ctrl_id;
              String8 string = push_str8f(scratch.arena, "%i", (int)ctrl_id);
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            
            if(entity->kind == D_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Instruction Pointer Address")))
              {
                U64 rip = d_query_cached_rip_from_thread(entity);
                String8 string = push_str8f(scratch.arena, "0x%I64x", rip);
                os_set_clipboard_text(string);
                ui_ctx_menu_close();
              }
            }
            
            if(entity->kind == D_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Call Stack")))
              {
                DI_Scope *di_scope = di_scope_open();
                D_Entity *process = d_entity_ancestor_from_kind(entity, D_EntityKind_Process);
                CTRL_Unwind base_unwind = d_query_cached_unwind_from_thread(entity);
                D_Unwind rich_unwind = d_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
                String8List lines = {0};
                for(U64 frame_idx = 0; frame_idx < rich_unwind.frames.concrete_frame_count; frame_idx += 1)
                {
                  D_UnwindFrame *concrete_frame = &rich_unwind.frames.v[frame_idx];
                  U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, concrete_frame->regs);
                  D_Entity *module = d_module_from_process_vaddr(process, rip_vaddr);
                  RDI_Parsed *rdi = concrete_frame->rdi;
                  RDI_Procedure *procedure = concrete_frame->procedure;
                  for(D_UnwindInlineFrame *inline_frame = concrete_frame->last_inline_frame;
                      inline_frame != 0;
                      inline_frame = inline_frame->prev)
                  {
                    RDI_InlineSite *inline_site = inline_frame->inline_site;
                    String8 name = {0};
                    name.str = rdi_string_from_idx(rdi, inline_site->name_string_idx, &name.size);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [inlined] \"%S\"%s%S", rip_vaddr, name, d_entity_is_nil(module) ? "" : " in ", module->string);
                  }
                  if(procedure != 0)
                  {
                    String8 name = {0};
                    name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: \"%S\"%s%S", rip_vaddr, name, d_entity_is_nil(module) ? "" : " in ", module->string);
                  }
                  else if(!d_entity_is_nil(module))
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
            
            if(entity->kind == D_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Find")))
              {
                df_msg(DF_MsgKind_FindThread, .entity = d_handle_from_entity(entity));
                ui_ctx_menu_close();
              }
            }
          }break;
          
          case D_EntityKind_Module:
          {
            UI_Signal copy_full_path_sig = df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Full Path");
            if(ui_clicked(copy_full_path_sig))
            {
              String8 string = entity->string;
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            if(ui_hovering(copy_full_path_sig)) UI_Tooltip
            {
              String8 string = entity->string;
              ui_label(string);
            }
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Base Address")))
            {
              Rng1U64 vaddr_rng = entity->vaddr_rng;
              String8 string = push_str8f(scratch.arena, "0x%I64x", vaddr_rng.min);
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Address Range Size")))
            {
              Rng1U64 vaddr_rng = entity->vaddr_rng;
              String8 string = push_str8f(scratch.arena, "0x%I64x", dim_1u64(vaddr_rng));
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
          }break;
          
          case D_EntityKind_Target:
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Play, 0, "Launch And Run")))
            {
              d_msg(D_MsgKind_LaunchAndRun, .entity = d_handle_from_entity(entity));
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_icon_buttonf(DF_IconKind_PlayStepForward, 0, "Launch And Initialize")))
            {
              d_msg(D_MsgKind_LaunchAndInit, .entity = d_handle_from_entity(entity));
              ui_ctx_menu_close();
            }
          }break;
        }
        
        DF_Palette(DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
        
        // rjf: color editor
        {
          B32 entity_has_color = entity->flags & D_EntityFlag_HasColor;
          if(entity_has_color)
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
              DF_Palette(DF_PaletteCode_Floating)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Trash, 0, "Remove Color###color_toggle")))
              {
                D_StateDeltaHistoryBatch(d_state_delta_history())
                {
                  d_state_delta_history_push_struct_delta(d_state_delta_history(), &entity->flags, .guard_entity = entity);
                  entity->flags &= ~D_EntityFlag_HasColor;
                }
              }
            }
            
            ui_spacer(ui_em(1.5f, 1.f));
          }
          if(!entity_has_color && ui_clicked(df_icon_buttonf(DF_IconKind_Palette, 0, "Apply Color###color_toggle")))
          {
            D_StateDeltaHistoryBatch(d_state_delta_history())
            {
              d_entity_equip_color_rgba(entity, v4f32(1, 1, 1, 1));
            }
          }
        }
      }
#endif
      
      //- rjf: auto-close tab ctx menu
      if(ui_ctx_menu_is_open(df_state->tab_ctx_menu_key))
      {
        MD_Node *tab_cfg_tree = df_cfg_tree_from_handle(ws->tab_ctx_menu_view);
        if(md_node_is_nil(tab_cfg_tree))
        {
          ui_ctx_menu_close();
        }
      }
      
      //- rjf: tab menu
      UI_CtxMenu(df_state->tab_ctx_menu_key) UI_PrefWidth(ui_em(40.f, 1.f)) UI_CornerRadius(0)
        DF_Palette(DF_PaletteCode_ImplicitButton)
      {
        DF_RegsScope(.tab = ws->tab_ctx_menu_view)
        {
          MD_Node *tab_cfg = df_cfg_tree_from_handle(df_regs()->tab);
          String8 query = tab_cfg->string;
          DF_ViewSpec *view_spec = df_view_spec_from_string(tab_cfg->first->string);
          DF_IconKind view_icon = view_spec->info.icon_kind;
          DR_FancyStringList fstrs = df_title_fstrs_from_view_spec_query(scratch.arena, view_spec, query, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
          String8 file_path = d_file_path_from_eval_string(scratch.arena, query);
          
          // rjf: title
          UI_Row
          {
            ui_spacer(ui_em(1.f, 1.f));
            DF_Font(DF_FontSlot_Icons)
              UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons))
              UI_PrefWidth(ui_em(2.f, 1.f))
              UI_PrefHeight(ui_pct(1, 0))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
              ui_label(df_g_icon_kind_text_table[view_icon]);
            UI_PrefWidth(ui_text_dim(10, 1))
            {
              UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
              ui_box_equip_display_fancy_strings(name_box, &fstrs);
            }
          }
          
          DF_Palette(DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
          
          // rjf: copy name
          if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Name")))
          {
            os_set_clipboard_text(dr_string_from_fancy_string_list(scratch.arena, &fstrs));
            ui_ctx_menu_close();
          }
          
          // rjf: copy full path
          if(file_path.size != 0)
          {
            UI_Signal copy_full_path_sig = df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Full Path");
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
            UI_Signal sig = df_icon_buttonf(DF_IconKind_FolderClosedFilled, 0, "Show In Explorer");
            if(ui_clicked(sig))
            {
              String8 full_path = path_normalized_from_string(scratch.arena, file_path);
              os_show_in_filesystem_ui(full_path);
              ui_ctx_menu_close();
            }
          }
          
          // rjf: filter controls
          if(view_spec->info.flags & DF_ViewSpecFlag_CanFilter)
          {
            if(ui_clicked(df_cmd_spec_button(d_cmd_spec_from_kind(D_CmdKind_Filter))))
            {
              df_msg(DF_MsgKind_Filter);
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_cmd_spec_button(d_cmd_spec_from_kind(D_CmdKind_ClearFilter))))
            {
              df_msg(DF_MsgKind_ClearFilter);
              ui_ctx_menu_close();
            }
          }
          
          // rjf: close tab
          if(ui_clicked(df_icon_buttonf(DF_IconKind_X, 0, "Close Tab")))
          {
            df_msg(DF_MsgKind_CloseTab);
            ui_ctx_menu_close();
          }
          
          // rjf: param tree editing
#if 0 // TODO(rjf): @msgs
          UI_TextPadding(ui_top_font_size()*1.5f) DF_Font(DF_FontSlot_Code)
          {
            Temp scratch = scratch_begin(0, 0);
            D_ViewRuleSpec *core_vr_spec = d_view_rule_spec_from_string(view->spec->info.name);
            String8 schema_string = core_vr_spec->info.schema;
            MD_TokenizeResult schema_tokenize = md_tokenize_from_text(scratch.arena, schema_string);
            MD_ParseResult schema_parse = md_parse_from_text_tokens(scratch.arena, str8_zero(), schema_string, schema_tokenize.tokens);
            MD_Node *schema_root = schema_parse.root->first;
            if(!md_node_is_nil(schema_root))
            {
              if(!md_node_is_nil(schema_root->first))
              {
                DF_Palette(DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
              }
              for(MD_EachNode(key, schema_root->first))
              {
                UI_Row
                {
                  MD_Node *params = view->params_roots[view->params_write_gen%ArrayCount(view->params_roots)];
                  MD_Node *param_tree = md_child_from_string(params, key->string, 0);
                  String8 pre_edit_value = md_string_from_children(scratch.arena, param_tree);
                  UI_PrefWidth(ui_em(10.f, 1.f)) ui_label(key->string);
                  UI_Signal sig = df_line_editf(DF_LineEditFlag_Border|DF_LineEditFlag_CodeContents, 0, 0, &ws->tab_ctx_menu_input_cursor, &ws->tab_ctx_menu_input_mark, ws->tab_ctx_menu_input_buffer, sizeof(ws->tab_ctx_menu_input_buffer), &ws->tab_ctx_menu_input_size, 0, pre_edit_value, "%S##view_param", key->string);
                  if(ui_committed(sig))
                  {
                    String8 new_string = str8(ws->tab_ctx_menu_input_buffer, ws->tab_ctx_menu_input_size);
                    df_view_store_param(view, key->string, new_string);
                  }
                }
              }
            }
            scratch_end(scratch);
          }
#endif
        }
      }
      
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: confirmation popup
    //
    {
      if(df_state->confirm_t > 0.005f) UI_TextAlignment(UI_TextAlign_Center) UI_Focus(df_state->confirm_active ? UI_FocusKind_Root : UI_FocusKind_Off)
      {
        Vec2F32 window_dim = dim_2f32(window_rect);
        UI_Box *bg_box = &ui_g_nil_box;
        UI_Palette *palette = ui_build_palette(df_palette_from_code(DF_PaletteCode_Floating));
        palette->background.w *= df_state->confirm_t;
        UI_Rect(window_rect)
          UI_ChildLayoutAxis(Axis2_X)
          UI_Focus(UI_FocusKind_On)
          UI_BlurSize(10*df_state->confirm_t)
          UI_Palette(palette)
        {
          bg_box = ui_build_box_from_stringf(UI_BoxFlag_FixedSize|
                                             UI_BoxFlag_Floating|
                                             UI_BoxFlag_Clickable|
                                             UI_BoxFlag_Scroll|
                                             UI_BoxFlag_DefaultFocusNav|
                                             UI_BoxFlag_DisableFocusOverlay|
                                             UI_BoxFlag_DrawBackgroundBlur|
                                             UI_BoxFlag_DrawBackground, "###confirm_popup_%p", ws);
        }
        if(df_state->confirm_active) UI_Parent(bg_box) UI_Transparency(1-df_state->confirm_t)
        {
          ui_ctx_menu_close();
          UI_WidthFill UI_PrefHeight(ui_children_sum(1.f)) UI_Column UI_Padding(ui_pct(1, 0))
          {
            UI_TextRasterFlags(df_raster_flags_from_slot(DF_FontSlot_Main)) UI_FontSize(ui_top_font_size()*2.f) UI_PrefHeight(ui_em(3.f, 1.f)) ui_label(df_state->confirm_title);
            UI_PrefHeight(ui_em(3.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_label(df_state->confirm_desc);
            ui_spacer(ui_em(1.5f, 1.f));
            UI_Row UI_Padding(ui_pct(1.f, 0.f)) UI_WidthFill UI_PrefHeight(ui_em(5.f, 1.f))
            {
              UI_CornerRadius00(ui_top_font_size()*0.25f)
                UI_CornerRadius01(ui_top_font_size()*0.25f)
                DF_Palette(DF_PaletteCode_NeutralPopButton)
                if(ui_clicked(ui_buttonf("OK")) || (ui_key_match(bg_box->default_nav_focus_hot_key, ui_key_zero()) && ui_slot_press(UI_EventActionSlot_Accept)))
              {
                df_msg(DF_MsgKind_ConfirmAccept);
              }
              UI_CornerRadius10(ui_top_font_size()*0.25f)
                UI_CornerRadius11(ui_top_font_size()*0.25f)
                if(ui_clicked(ui_buttonf("Cancel")) || ui_slot_press(UI_EventActionSlot_Cancel))
              {
                df_msg(DF_MsgKind_ConfirmCancel);
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
      if(!ws->autocomp_force_closed && !ui_key_match(ws->autocomp_root_key, ui_key_zero()) && ws->autocomp_last_frame_idx+1 >= d_frame_index())
    {
      String8 query = str8(ws->autocomp_lister_query_buffer, ws->autocomp_lister_query_size);
      UI_Box *autocomp_root_box = ui_box_from_key(ws->autocomp_root_key);
      if(!ui_box_is_nil(autocomp_root_box))
      {
        Temp scratch = scratch_begin(0, 0);
        
        //- rjf: unpack lister params
        CTRL_Entity *thread = d_regs_thread();
        U64 thread_rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, d_base_regs()->unwind_count);
        CTRL_Entity *process = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
        CTRL_Entity *module = ctrl_module_from_process_vaddr(process, thread_rip_vaddr);
        U64 thread_rip_voff = ctrl_voff_from_vaddr(module, thread_rip_vaddr);
        DI_Key dbgi_key = ctrl_dbgi_key_from_module(module);
        
        //- rjf: gather lister items
        DF_AutoCompListerItemChunkList item_list = {0};
        {
          //- rjf: gather locals
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Locals)
          {
            E_String2NumMap *locals_map = d_query_cached_locals_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
            E_String2NumMap *member_map = d_query_cached_member_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
            for(E_String2NumMapNode *n = locals_map->first; n != 0; n = n->order_next)
            {
              DF_AutoCompListerItem item = {0};
              {
                item.string      = n->string;
                item.kind_string = str8_lit("Local");
                item.matches     = fuzzy_match_find(scratch.arena, query, n->string);
              }
              if(query.size == 0 || item.matches.count != 0)
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
            for(E_String2NumMapNode *n = member_map->first; n != 0; n = n->order_next)
            {
              DF_AutoCompListerItem item = {0};
              {
                item.string      = n->string;
                item.kind_string = str8_lit("Local (Member)");
                item.matches     = fuzzy_match_find(scratch.arena, query, n->string);
              }
              if(query.size == 0 || item.matches.count != 0)
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather registers
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Registers)
          {
            CTRL_Entity *thread = d_regs_thread();
            Arch arch = thread->arch;
            U64 reg_names_count = regs_reg_code_count_from_arch(arch);
            U64 alias_names_count = regs_alias_code_count_from_arch(arch);
            String8 *reg_names = regs_reg_code_string_table_from_arch(arch);
            String8 *alias_names = regs_alias_code_string_table_from_arch(arch);
            for(U64 idx = 0; idx < reg_names_count; idx += 1)
            {
              if(reg_names[idx].size != 0)
              {
                DF_AutoCompListerItem item = {0};
                {
                  item.string      = reg_names[idx];
                  item.kind_string = str8_lit("Register");
                  item.matches     = fuzzy_match_find(scratch.arena, query, reg_names[idx]);
                }
                if(query.size == 0 || item.matches.count != 0)
                {
                  df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
                }
              }
            }
            for(U64 idx = 0; idx < alias_names_count; idx += 1)
            {
              if(alias_names[idx].size != 0)
              {
                DF_AutoCompListerItem item = {0};
                {
                  item.string      = alias_names[idx];
                  item.kind_string = str8_lit("Reg. Alias");
                  item.matches     = fuzzy_match_find(scratch.arena, query, alias_names[idx]);
                }
                if(query.size == 0 || item.matches.count != 0)
                {
                  df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
                }
              }
            }
          }
          
          //- rjf: gather view rules
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_ViewRules)
          {
            for(U64 slot_idx = 0; slot_idx < d_state->view_rule_spec_table_size; slot_idx += 1)
            {
              for(D_ViewRuleSpec *spec = d_state->view_rule_spec_table[slot_idx]; spec != 0 && spec != &d_nil_core_view_rule_spec; spec = spec->hash_next)
              {
                DF_AutoCompListerItem item = {0};
                {
                  item.string      = spec->info.string;
                  item.kind_string = str8_lit("View Rule");
                  item.matches     = fuzzy_match_find(scratch.arena, query, spec->info.string);
                }
                if(query.size == 0 || item.matches.count != 0)
                {
                  df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
                }
              }
            }
          }
          
          //- rjf: gather members
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Members)
          {
            
          }
          
          //- rjf: gather languages
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Languages)
          {
            for(EachNonZeroEnumVal(TXT_LangKind, lang))
            {
              DF_AutoCompListerItem item = {0};
              {
                item.string      = txt_extension_from_lang_kind(lang);
                item.kind_string = str8_lit("Language");
                item.matches     = fuzzy_match_find(scratch.arena, query, item.string);
              }
              if(item.string.size != 0 && (query.size == 0 || item.matches.count != 0))
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather architectures
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Architectures)
          {
            for(EachNonZeroEnumVal(Arch, arch))
            {
              DF_AutoCompListerItem item = {0};
              {
                item.string      = string_from_arch(arch);
                item.kind_string = str8_lit("Arch");
                item.matches     = fuzzy_match_find(scratch.arena, query, item.string);
              }
              if(query.size == 0 || item.matches.count != 0)
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather tex2dformats
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Tex2DFormats)
          {
            for(EachEnumVal(R_Tex2DFormat, fmt))
            {
              DF_AutoCompListerItem item = {0};
              {
                item.string      = lower_from_str8(scratch.arena, r_tex2d_format_display_string_table[fmt]);
                item.kind_string = str8_lit("Format");
                item.matches     = fuzzy_match_find(scratch.arena, query, item.string);
              }
              if(query.size == 0 || item.matches.count != 0)
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather view rule params
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_ViewRuleParams)
          {
            for(String8Node *n = ws->autocomp_lister_params.strings.first; n != 0; n = n->next)
            {
              String8 string = n->string;
              DF_AutoCompListerItem item = {0};
              {
                item.string      = string;
                item.kind_string = str8_lit("Parameter");
                item.matches     = fuzzy_match_find(scratch.arena, query, item.string);
              }
              if(query.size == 0 || item.matches.count != 0)
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
        }
        
        //- rjf: lister item list -> sorted array
        DF_AutoCompListerItemArray item_array = df_autocomp_lister_item_array_from_chunk_list(scratch.arena, &item_list);
        df_autocomp_lister_item_array_sort__in_place(&item_array);
        
        //- rjf: animate
        {
          // rjf: animate target # of rows
          {
            F32 rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? (1 - pow_f32(2, (-60.f * d_dt()))) : 1.f;
            F32 target = Min((F32)item_array.count, 16.f);
            if(abs_f32(target - ws->autocomp_num_visible_rows_t) > 0.01f)
            {
              df_request_frame();
            }
            ws->autocomp_num_visible_rows_t += (target - ws->autocomp_num_visible_rows_t) * rate;
            if(abs_f32(target - ws->autocomp_num_visible_rows_t) <= 0.02f)
            {
              ws->autocomp_num_visible_rows_t = target;
            }
          }
          
          // rjf: animate open
          {
            F32 rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * d_dt())) : 1.f;
            F32 diff = 1.f-ws->autocomp_open_t;
            ws->autocomp_open_t += diff*rate;
            if(abs_f32(diff) < 0.05f)
            {
              ws->autocomp_open_t = 1.f;
            }
            else
            {
              df_request_frame();
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
            DF_Palette(DF_PaletteCode_Floating)
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
            if(ws->autocomp_query_dirty)
            {
              ws->autocomp_query_dirty = 0;
              autocomp_box->default_nav_focus_hot_key = autocomp_box->default_nav_focus_active_key = autocomp_box->default_nav_focus_next_hot_key = autocomp_box->default_nav_focus_next_active_key = ui_key_zero();
            }
          }
          UI_Parent(autocomp_box)
            UI_WidthFill
            UI_PrefHeight(ui_px(row_height_px, 1.f))
            DF_Font(DF_FontSlot_Code)
            UI_HoverCursor(OS_Cursor_HandPoint)
            UI_Focus(UI_FocusKind_Null)
            DF_Palette(DF_PaletteCode_ImplicitButton)
            UI_Padding(ui_em(1.f, 1.f))
          {
            for(U64 idx = 0; idx < item_array.count; idx += 1)
            {
              DF_AutoCompListerItem *item = &item_array.v[idx];
              UI_Box *item_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects|UI_BoxFlag_MouseClickable, "autocomp_%I64x", idx);
              UI_Parent(item_box) UI_Padding(ui_em(1.f, 1.f))
              {
                UI_WidthFill
                {
                  UI_Box *box = ui_label(item->string).box;
                  ui_box_equip_fuzzy_match_ranges(box, &item->matches);
                }
                DF_Font(DF_FontSlot_Main)
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
                move_back_evt.delta_2s32.x = -(S32)query.size;
                ui_event_list_push(ui_build_arena(), &events, &move_back_evt);
                UI_Event paste_evt = zero_struct;
                paste_evt.kind = UI_EventKind_Text;
                paste_evt.string = item->string;
                ui_event_list_push(ui_build_arena(), &events, &paste_evt);
                autocomp_box->default_nav_focus_hot_key = autocomp_box->default_nav_focus_active_key = autocomp_box->default_nav_focus_next_hot_key = autocomp_box->default_nav_focus_next_active_key = ui_key_zero();
              }
              else if(item_box->flags & UI_BoxFlag_FocusHot && !(item_box->flags & UI_BoxFlag_FocusHotDisabled))
              {
                UI_Event evt = zero_struct;
                evt.kind   = UI_EventKind_AutocompleteHint;
                evt.string = item->string;
                ui_event_list_push(ui_build_arena(), &events, &evt);
              }
            }
          }
        }
        
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
      DF_Palette(DF_PaletteCode_MenuBar)
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
              R_Handle texture = df_state->icon_texture;
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
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(file_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              D_CmdKind cmds[] =
              {
                D_CmdKind_Open,
                D_CmdKind_OpenUser,
                D_CmdKind_OpenProject,
                D_CmdKind_OpenRecentProject,
                D_CmdKind_Exit,
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
              df_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: window menu
            UI_Key window_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_window_menu_key_"));
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(window_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              D_CmdKind cmds[] =
              {
                D_CmdKind_OpenWindow,
                D_CmdKind_CloseWindow,
                D_CmdKind_ToggleFullscreen,
              };
              U32 codepoints[] =
              {
                'w',
                'c',
                'f',
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              df_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: panel menu
            UI_Key panel_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_panel_menu_key_"));
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(panel_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              D_CmdKind cmds[] =
              {
                D_CmdKind_NewPanelRight,
                D_CmdKind_NewPanelDown,
                D_CmdKind_ClosePanel,
                D_CmdKind_RotatePanelColumns,
                D_CmdKind_NextPanel,
                D_CmdKind_PrevPanel,
                D_CmdKind_CloseTab,
                D_CmdKind_NextTab,
                D_CmdKind_PrevTab,
                D_CmdKind_TabBarTop,
                D_CmdKind_TabBarBottom,
                D_CmdKind_ResetToDefaultPanels,
                D_CmdKind_ResetToCompactPanels,
              };
              U32 codepoints[] =
              {
                'r',
                'd',
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
              df_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: view menu
            UI_Key view_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_view_menu_key_"));
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(view_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              D_CmdKind cmds[] =
              {
                D_CmdKind_Targets,
                D_CmdKind_Scheduler,
                D_CmdKind_CallStack,
                D_CmdKind_Modules,
                D_CmdKind_Output,
                D_CmdKind_Memory,
                D_CmdKind_Disassembly,
                D_CmdKind_Watch,
                D_CmdKind_Locals,
                D_CmdKind_Registers,
                D_CmdKind_Globals,
                D_CmdKind_ThreadLocals,
                D_CmdKind_Types,
                D_CmdKind_Procedures,
                D_CmdKind_Breakpoints,
                D_CmdKind_WatchPins,
                D_CmdKind_FilePathMap,
                D_CmdKind_Settings,
                D_CmdKind_ExceptionFilters,
                D_CmdKind_GettingStarted,
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
                'e',
                'g',
                0,
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              df_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: targets menu
            UI_Key targets_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_targets_menu_key_"));
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(targets_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              Temp scratch = scratch_begin(0, 0);
              D_CmdKind cmds[] =
              {
                D_CmdKind_AddTarget,
                D_CmdKind_EditTarget,
                D_CmdKind_RemoveTarget,
              };
              U32 codepoints[] =
              {
                'a',
                'e',
                'r',
              };
              Assert(ArrayCount(codepoints) == ArrayCount(cmds));
              df_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
              DF_Palette(DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
              for(EachEnumVal(DF_CfgSlot, slot))
              {
                for(MD_EachNode(tln, df_state->cfg_slot_roots[slot]->first))
                {
                  if(!str8_match(tln->string, str8_lit("target"), 0))
                  {
                    continue;
                  }
                  UI_Palette *palette = ui_top_palette();
                  U32 rgba_u32 = d_value_from_params_key(tln, str8_lit("rgba")).u32;
                  if(rgba_u32 != 0)
                  {
                    Vec4F32 rgba = rgba_from_u32(rgba_u32);
                    palette = ui_build_palette(ui_top_palette(), .text = rgba);
                  }
                  String8 target_name = md_child_from_string(tln, str8_lit("label"), 0)->first->string;
                  if(target_name.size == 0)
                  {
                    target_name = md_child_from_string(tln, str8_lit("executable"), 0)->first->string;
                    target_name = str8_skip_last_slash(target_name);
                  }
                  UI_Signal sig = {0};
                  UI_Palette(palette) sig = df_icon_buttonf(DF_IconKind_Target, 0, "%S##%p", target_name, tln);
                  if(ui_clicked(sig))
                  {
                    df_msg(DF_MsgKind_SelectTarget, .cfg_tree = df_handle_from_cfg_tree(tln));
                    ui_ctx_menu_close();
                    ws->menu_bar_focused = 0;
                  }
                }
              }
              scratch_end(scratch);
            }
            
            // rjf: ctrl menu
            UI_Key ctrl_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_ctrl_menu_key_"));
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(ctrl_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              D_CmdKind cmds[] =
              {
                D_CmdKind_Run,
                D_CmdKind_KillAll,
                D_CmdKind_Restart,
                D_CmdKind_Halt,
                D_CmdKind_SoftHaltRefresh,
                D_CmdKind_StepInto,
                D_CmdKind_StepOver,
                D_CmdKind_StepOut,
                D_CmdKind_Attach,
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
              df_cmd_list_menu_buttons(ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: help menu
            UI_Key help_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_help_menu_key_"));
            DF_Palette(DF_PaletteCode_Floating)
              UI_CtxMenu(help_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(DF_PaletteCode_ImplicitButton)
            {
              UI_Row UI_TextAlignment(UI_TextAlign_Center) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                ui_label(str8_lit(BUILD_TITLE_STRING_LITERAL));
              UI_PrefHeight(ui_children_sum(1)) UI_Row UI_Padding(ui_pct(1, 0))
              {
                R_Handle texture = df_state->icon_texture;
                Vec2S32 texture_dim = r_size_from_tex2d(texture);
                UI_PrefWidth(ui_px(ui_top_font_size()*10.f, 1.f))
                  UI_PrefHeight(ui_px(ui_top_font_size()*10.f, 1.f))
                  ui_image(texture, R_Tex2DSampleKind_Linear, r2f32p(0, 0, texture_dim.x, texture_dim.y), v4f32(1, 1, 1, 1), 0, str8_lit(""));
              }
              ui_spacer(ui_em(0.25f, 1.f));
              UI_Row
                UI_PrefWidth(ui_text_dim(10, 1))
                UI_TextAlignment(UI_TextAlign_Center)
                UI_Padding(ui_pct(1, 0))
              {
                ui_labelf("Search for commands by pressing ");
                D_CmdSpec *spec = d_cmd_spec_from_kind(D_CmdKind_RunCommand);
                UI_Flags(UI_BoxFlag_DrawBorder)
                  UI_TextAlignment(UI_TextAlign_Center)
                  df_cmd_binding_buttons(spec);
              }
              ui_spacer(ui_em(0.25f, 1.f));
              UI_Row UI_TextAlignment(UI_TextAlign_Center) ui_label(str8_lit("Submit issues to the GitHub at:"));
              UI_TextAlignment(UI_TextAlign_Center)
              {
                UI_Signal url_sig = ui_buttonf("github.com/EpicGames/raddebugger");
                if(ui_hovering(url_sig)) UI_Tooltip
                {
                  ui_labelf("Copy To Clipboard");
                }
                if(ui_clicked(url_sig))
                {
                  os_set_clipboard_text(str8_lit("https://github.com/EpicGames/raddebugger"));
                }
              }
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
                if(ui_key_press(OS_EventFlag_Alt, items[idx].key))
                {
                  alt_fastpath_key = 1;
                }
                if((ws->menu_bar_key_held || ws->menu_bar_focused) && !ui_any_ctx_menu_is_open())
                {
                  ui_set_next_flags(UI_BoxFlag_DrawTextFastpathCodepoint);
                }
                UI_Signal sig = df_menu_bar_button(items[idx].name);
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
#if 0 // TODO(rjf): @msgs
          UI_PrefWidth(ui_text_dim(10, 1)) UI_HeightFill
            DF_Palette(DF_PaletteCode_NeutralPopButton)
          {
            Temp scratch = scratch_begin(0, 0);
            D_EntityList tasks = d_query_cached_entity_list_with_kind(D_EntityKind_ConversionTask);
            for(D_EntityNode *n = tasks.first; n != 0; n = n->next)
            {
              D_Entity *task = n->entity;
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
#endif
        }
        
        //- rjf: center column
        UI_PrefWidth(ui_children_sum(1.f)) UI_Row
          UI_PrefWidth(ui_em(2.25f, 1))
          DF_Font(DF_FontSlot_Icons)
          UI_FontSize(ui_top_font_size()*0.85f)
        {
          Temp scratch = scratch_begin(0, 0);
          D_EntityList targets = d_push_active_target_list(scratch.arena);
          CTRL_EntityList processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
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
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextPositive)))
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Play]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_play)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: %s", have_targets ? "Targets are currently running" : "No active targets exist");
            }
            if(ui_hovering(sig) && can_play)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
              {
                if(can_stop)
                {
                  ui_labelf("Resume all processes");
                }
                else
                {
                  ui_labelf("Launch all active targets:");
                  for(D_EntityNode *n = targets.first; n != 0; n = n->next)
                  {
                    String8 target_display_name = d_display_string_from_entity(scratch.arena, n->entity);
                    ui_label(target_display_name);
                  }
                }
              }
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_Run);
            }
          }
          
          //- rjf: restart button
          else UI_TextAlignment(UI_TextAlign_Center)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextPositive)))
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Redo]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig))
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
              {
                ui_labelf("Restart all running targets:");
                {
                  D_EntityList processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
                  for(D_EntityNode *n = processes.first; n != 0; n = n->next)
                  {
                    D_Entity *process = n->entity;
                    D_Entity *target = d_entity_from_handle(process->entity_handle);
                    if(!d_entity_is_nil(target))
                    {
                      String8 target_display_name = d_display_string_from_entity(scratch.arena, target);
                      ui_label(target_display_name);
                    }
                  }
                }
              }
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_Restart);
            }
          }
          
          //- rjf: pause button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_pause ? 0 : UI_BoxFlag_Disabled)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextNeutral)))
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Pause]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_pause)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: Already halted");
            }
            if(ui_hovering(sig) && can_pause)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Halt all target processes");
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_Halt);
            }
          }
          
          //- rjf: stop button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_stop ? 0 : UI_BoxFlag_Disabled)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextNegative)))
          {
            UI_Signal sig = {0};
            {
              sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Stop]);
              os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            }
            if(ui_hovering(sig) && !can_stop)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_stop)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Kill all target processes");
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_Kill);
            }
          }
          
          //- rjf: step over button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_step ? 0 : UI_BoxFlag_Disabled)
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_StepOver]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_step && can_pause)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Step Over");
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_StepOver);
            }
          }
          
          //- rjf: step into button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_step ? 0 : UI_BoxFlag_Disabled)
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_StepInto]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_step && can_pause)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Step Into");
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_StepInto);
            }
          }
          
          //- rjf: step out button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_step ? 0 : UI_BoxFlag_Disabled)
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_StepOut]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_step && can_pause)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                DF_Font(DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                ui_labelf("Step Out");
            }
            if(ui_clicked(sig))
            {
              d_msg(D_MsgKind_StepOut);
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
          if(do_user_prof) DF_Palette(DF_PaletteCode_NeutralPopButton)
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
              String8 user_path = df_state->cfg_slot_roots[DF_CfgSlot_User]->string;
              user_path = str8_chop_last_dot(user_path);
              DF_Font(DF_FontSlot_Icons)
                UI_TextRasterFlags(df_raster_flags_from_slot(DF_FontSlot_Icons))
                ui_label(df_g_icon_kind_text_table[DF_IconKind_Person]);
              ui_label(str8_skip_last_slash(user_path));
            }
            UI_Signal user_sig = ui_signal_from_box(user_box);
            if(ui_clicked(user_sig))
            {
              df_msg(DF_MsgKind_RunCommand, .string = df_msg_kind_info_table[DF_MsgKind_LoadUser].name_lower);
            }
          }
          
          if(do_user_prof)
          {
            ui_spacer(ui_em(0.75f, 0));
          }
          
          // rjf: loaded project viz
          if(do_user_prof) DF_Palette(DF_PaletteCode_NeutralPopButton)
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
              String8 prof_path = df_state->cfg_slot_roots[DF_CfgSlot_Project]->string;
              prof_path = str8_chop_last_dot(prof_path);
              DF_Font(DF_FontSlot_Icons)
                ui_label(df_g_icon_kind_text_table[DF_IconKind_Briefcase]);
              ui_label(str8_skip_last_slash(prof_path));
            }
            UI_Signal prof_sig = ui_signal_from_box(prof_box);
            if(ui_clicked(prof_sig))
            {
              df_msg(DF_MsgKind_RunCommand, .string = df_msg_kind_info_table[DF_MsgKind_LoadProject].name_lower);
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
              min_sig = df_icon_buttonf(DF_IconKind_Minus,  0, "##minimize");
              max_sig = df_icon_buttonf(DF_IconKind_Window, 0, "##maximize");
            }
            UI_PrefWidth(ui_px(button_dim, 1.f))
              DF_Palette(DF_PaletteCode_NegativePopButton)
            {
              cls_sig = df_icon_buttonf(DF_IconKind_X,      0, "##close");
            }
            if(ui_clicked(min_sig))
            {
              os_window_minimize(ws->os);
            }
            if(ui_clicked(max_sig))
            {
              os_window_set_maximized(ws->os, !os_window_is_maximized(ws->os));
            }
            if(ui_clicked(cls_sig))
            {
              df_msg(DF_MsgKind_CloseWindow);
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
      F32 error_t = ClampBot(0, 1.f - (df_state->error_num_seconds_shown/df_state->error_num_seconds_to_show));
      CTRL_Event stop_event = d_ctrl_last_stop_event();
      UI_Palette *positive_scheme = df_palette_from_code(DF_PaletteCode_PositivePopButton);
      UI_Palette *running_scheme  = df_palette_from_code(DF_PaletteCode_NeutralPopButton);
      UI_Palette *negative_scheme = df_palette_from_code(DF_PaletteCode_NegativePopButton);
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
      if(error_t > 0.01f)
      {
        UI_Palette *blended_scheme = push_array(ui_build_arena(), UI_Palette, 1);
        MemoryCopyStruct(blended_scheme, palette);
        for(EachEnumVal(UI_ColorCode, code))
        {
          for(U64 idx = 0; idx < 4; idx += 1)
          {
            blended_scheme->colors[code].v[idx] += (negative_scheme->colors[code].v[idx] - blended_scheme->colors[code].v[idx]) * error_t;
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
          F32 animation_t = pow_f32(sin_f32(d_time_in_seconds()/2.f), 2.f);
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
            DF_IconKind icon = DF_IconKind_Null;
            String8 explanation = str8_lit("Not running");
            {
              String8 stop_explanation = df_stop_explanation_string_icon_from_ctrl_event(scratch.arena, &stop_event, &icon);
              if(stop_explanation.size != 0)
              {
                explanation = stop_explanation;
              }
            }
            if(icon != DF_IconKind_Null)
            {
              UI_PrefWidth(ui_em(2.25f, 1.f))
                DF_Font(DF_FontSlot_Icons)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons))
                ui_label(df_g_icon_kind_text_table[icon]);
            }
            UI_PrefWidth(ui_text_dim(10, 1)) ui_label(explanation);
            scratch_end(scratch);
          }
        }
        
        ui_spacer(ui_pct(1, 0));
        
        // rjf: bind change visualization
        if(df_state->bind_change_active)
        {
          MD_Node *bind_cfg = df_cfg_tree_from_handle(df_state->bind_change_bind_handle);
          DF_MsgKind msg_kind = df_msg_kind_from_string(bind_cfg->first->string);
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_Flags(UI_BoxFlag_DrawBackground)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_CornerRadius(4)
            DF_Palette(DF_PaletteCode_NeutralPopButton)
            ui_labelf("Currently rebinding \"%S\" hotkey", df_msg_kind_info_table[msg_kind].display_name);
        }
        
        // rjf: error visualization
        else if(df_state->error_num_seconds_shown < df_state->error_num_seconds_to_show)
        {
          df_state->error_num_seconds_shown += df_state->dt;
          df_request_frame();
          String8 error_string = df_state->error_string;
          if(error_string.size != 0)
          {
            ui_set_next_pref_width(ui_children_sum(1));
            UI_CornerRadius(4)
              UI_Row
              UI_PrefWidth(ui_text_dim(10, 1))
              UI_TextAlignment(UI_TextAlign_Center)
            {
              DF_Font(DF_FontSlot_Icons)
                UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons))
                ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
              df_label(error_string);
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: prepare query view stack for the in-progress command
    //
    if(!d_cmd_spec_is_nil(ws->query_cmd_spec))
    {
      D_CmdSpec *cmd_spec = ws->query_cmd_spec;
      D_CmdParamSlot first_missing_slot = cmd_spec->info.query.slot;
      DF_ViewSpec *view_spec = df_view_spec_from_cmd_param_slot_spec(first_missing_slot, cmd_spec);
      if(ws->query_view_stack_top->spec != view_spec ||
         df_view_is_nil(ws->query_view_stack_top))
      {
        Temp scratch = scratch_begin(0, 0);
        
        // rjf: clear existing query stack
        for(DF_View *query_view = ws->query_view_stack_top, *next = 0;
            !df_view_is_nil(query_view);
            query_view = next)
        {
          next = query_view->order_next;
          df_view_release(query_view);
        }
        
        // rjf: determine default query
        String8 default_query = {0};
        switch(first_missing_slot)
        {
          default:
          if(cmd_spec->info.query.flags & D_CmdQueryFlag_KeepOldInput)
          {
            default_query = df_push_search_string(scratch.arena);
          }break;
          case D_CmdParamSlot_FilePath:
          {
            default_query = path_normalized_from_string(scratch.arena, d_current_path());
            default_query = push_str8f(scratch.arena, "%S/", default_query);
          }break;
        }
        
        // rjf: construct & push new view
        DF_View *view = df_view_alloc();
        df_view_equip_spec(view, view_spec, default_query, &md_nil_node);
        if(cmd_spec->info.query.flags & D_CmdQueryFlag_SelectOldInput)
        {
          view->query_mark = txt_pt(1, 1);
        }
        ws->query_view_stack_top = view;
        ws->query_view_selected = 1;
        view->order_next = &df_nil_view;
        
        scratch_end(scratch);
      }
    }
    
    ////////////////////////////
    //- rjf: animate query info
    //
    {
      F32 rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * d_dt())) : 1.f;
      
      // rjf: animate query view selection transition
      {
        F32 target = (F32)!!ws->query_view_selected;
        F32 diff = abs_f32(target - ws->query_view_selected_t);
        if(diff > 0.005f)
        {
          df_request_frame();
          if(diff < 0.005f)
          {
            ws->query_view_selected_t = target;
          }
          ws->query_view_selected_t += (target - ws->query_view_selected_t) * rate;
        }
      }
      
      // rjf: animate query view open/close transition
      {
        F32 query_view_t_target = !df_view_is_nil(ws->query_view_stack_top);
        F32 diff = abs_f32(query_view_t_target - ws->query_view_t);
        if(diff > 0.005f)
        {
          df_request_frame();
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
    if(!df_view_is_nil(ws->query_view_stack_top))
      UI_Focus((window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && ws->query_view_selected) ? UI_FocusKind_On : UI_FocusKind_Off)
      DF_Palette(DF_PaletteCode_Floating)
    {
      DF_View *view = ws->query_view_stack_top;
      D_CmdSpec *cmd_spec = ws->query_cmd_spec;
      D_CmdQuery *query = &cmd_spec->info.query;
      
      //- rjf: calculate rectangles
      Vec2F32 window_center = center_2f32(window_rect);
      F32 query_container_width = dim_2f32(window_rect).x*0.5f;
      F32 query_container_margin = ui_top_font_size()*8.f;
      F32 query_line_edit_height = ui_top_font_size()*3.f;
      Rng2F32 query_container_rect = r2f32p(window_center.x - query_container_width/2 + (1-ws->query_view_t)*query_container_width/4,
                                            window_rect.y0 + query_container_margin,
                                            window_center.x + query_container_width/2 - (1-ws->query_view_t)*query_container_width/4,
                                            window_rect.y1 - query_container_margin);
      if(ws->query_view_stack_top->spec == &df_nil_view_spec)
      {
        query_container_rect.y1 = query_container_rect.y0 + query_line_edit_height;
      }
      query_container_rect.y1 = mix_1f32(query_container_rect.y0, query_container_rect.y1, ws->query_view_t);
      Rng2F32 query_container_content_rect = r2f32p(query_container_rect.x0,
                                                    query_container_rect.y0+query_line_edit_height,
                                                    query_container_rect.x1,
                                                    query_container_rect.y1);
      
      //- rjf: build floating query view container
      UI_Box *query_container_box = &ui_g_nil_box;
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
            D_CmdKind cmd_kind = d_cmd_kind_from_string(ws->query_cmd_spec->info.string);
            DF_IconKind icon_kind = df_cmd_kind_icon_kind_table[cmd_kind];
            if(icon_kind != DF_IconKind_Null)
            {
              DF_Font(DF_FontSlot_Icons) ui_label(df_g_icon_kind_text_table[icon_kind]);
            }
            ui_labelf("%S", ws->query_cmd_spec->info.display_name);
          }
          DF_Font((query->flags & D_CmdQueryFlag_CodeInput) ? DF_FontSlot_Code : DF_FontSlot_Main)
            UI_TextPadding(ui_top_font_size()*0.5f)
          {
            UI_Signal sig = df_line_edit(DF_LineEditFlag_Border|
                                         (DF_LineEditFlag_CodeContents * !!(query->flags & D_CmdQueryFlag_CodeInput)),
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
          UI_PrefWidth(ui_em(5.f, 1.f)) UI_Focus(UI_FocusKind_Off) DF_Palette(DF_PaletteCode_PositivePopButton)
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_RightArrow, 0, "##complete_query")))
            {
              query_completed = 1;
            }
          }
          UI_PrefWidth(ui_em(3.f, 1.f)) UI_Focus(UI_FocusKind_Off) DF_Palette(DF_PaletteCode_PlainButton)
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_X, 0, "##cancel_query")))
            {
              query_cancelled = 1;
            }
          }
        }
      }
      
      //- rjf: build query view
      UI_Parent(query_container_box) UI_WidthFill UI_Focus(UI_FocusKind_Null)
      {
        DF_ViewSpec *view_spec = view->spec;
        view_spec->info.ui_hook(query_container_content_rect);
      }
      
      //- rjf: query submission
      if(((ui_is_focus_active() || (window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && !ws->query_view_selected)) &&
          ui_slot_press(UI_EventActionSlot_Cancel)) || query_cancelled)
      {
        df_msg(DF_MsgKind_CancelQuery);
      }
      if((ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept)) || query_completed)
      {
        Temp scratch = scratch_begin(0, 0);
        DF_View *view = ws->query_view_stack_top;
        D_Regs *regs = ws->query_msg_regs;
        D_RegsScope
        {
          d_regs_copy_contents(scratch.arena, d_regs(), regs);
          df_regs_set_from_query_slot_string(ws->query_msg_query.slot, str8(view->query_buffer, view->query_string_size));
          df_msg(ws->query_msg_kind);
        }
#if 0 // NOTE(rjf): @msgs
        D_CmdParams params = df_cmd_params_from_window(ws);
        String8 error = d_cmd_params_apply_spec_query(scratch.arena, &params, ws->query_cmd_spec, str8(view->query_buffer, view->query_string_size));
        d_push_cmd(d_cmd_spec_from_kind(D_CmdKind_CompleteQuery), &params);
        if(error.size != 0)
        {
          d_error(error);
        }
#endif
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
      UI_Palette(ui_build_palette(0, .background = mix_4f32(df_rgba_from_theme_color(DF_ThemeColor_InactivePanelOverlay), v4f32(0, 0, 0, 0), 1-ws->query_view_selected_t)))
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
        for(DF_Panel *panel = ws->root_panel;
            !df_panel_is_nil(panel);
            panel = df_panel_rec_df_pre(panel).next)
        {
          if(!df_panel_is_nil(panel->first)) { continue; }
          Rng2F32 panel_rect = df_target_rect_from_panel(content_rect, ws->root_panel, panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          if(!df_view_is_nil(view) &&
             contains_2f32(panel_rect, ui_mouse()) &&
             (abs_f32(view->scroll_pos.x.off) > 0.01f ||
              abs_f32(view->scroll_pos.y.off) > 0.01f))
          {
            build_hover_eval = 0;
            ws->hover_eval_first_frame_idx = d_frame_index();
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
      if(ws->hover_eval_string.size != 0 && !hover_eval_is_open && ws->hover_eval_last_frame_idx < ws->hover_eval_first_frame_idx+20 && d_frame_index()-ws->hover_eval_last_frame_idx < 50)
      {
        df_request_frame();
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
        DF_Font(DF_FontSlot_Code)
        UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
        DF_Palette(DF_PaletteCode_Floating)
      {
        Temp scratch = scratch_begin(0, 0);
        DI_Scope *scope = di_scope_open();
        String8 expr = ws->hover_eval_string;
        E_Eval eval = e_eval_from_string(scratch.arena, expr);
        D_CfgTable top_level_cfg_table = {0};
        
        //- rjf: build if good
        if(!e_type_key_match(eval.type_key, e_type_key_zero()) && !ui_any_ctx_menu_is_open())
          UI_Focus((hover_eval_is_open && !ui_any_ctx_menu_is_open() && ws->hover_eval_focused && (!query_is_open || !ws->query_view_selected)) ? UI_FocusKind_Null : UI_FocusKind_Off)
        {
          //- rjf: eval -> viz artifacts
          F32 row_height = floor_f32(ui_top_font_size()*2.8f);
          D_CfgTable cfg_table = {0};
          U64 expr_hash = d_hash_from_string(expr);
          D_EvalViewKey eval_view_key = d_eval_view_key_from_stringf("eval_hover_%I64x", expr_hash);
          D_EvalView *eval_view = d_eval_view_from_key(eval_view_key);
          D_ExpandKey parent_key = d_expand_key_make(5381, 1);
          D_ExpandKey key = d_expand_key_make(df_hash_from_expand_key(parent_key), 1);
          D_EvalVizBlockList viz_blocks = d_eval_viz_block_list_from_eval_view_expr_keys(scratch.arena, eval_view, &top_level_cfg_table, expr, parent_key, key);
          CTRL_Entity *entity = d_entity_from_eval_space(eval.space);
          U32 default_radix = (entity->kind == D_EntityKind_Thread ? 16 : 10);
          D_EvalVizWindowedRowList viz_rows = d_eval_viz_windowed_row_list_from_viz_block_list(scratch.arena, eval_view, r1s64(0, 50), &viz_blocks);
          
          //- rjf: animate
          {
            // rjf: animate height
            {
              F32 fish_rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * d_dt())) : 1.f;
              F32 hover_eval_container_height_target = row_height * Min(30, viz_blocks.total_visual_row_count);
              ws->hover_eval_num_visible_rows_t += (hover_eval_container_height_target - ws->hover_eval_num_visible_rows_t) * fish_rate;
              if(abs_f32(hover_eval_container_height_target - ws->hover_eval_num_visible_rows_t) > 0.5f)
              {
                df_request_frame();
              }
              else
              {
                ws->hover_eval_num_visible_rows_t = hover_eval_container_height_target;
              }
            }
            
            // rjf: animate open
            {
              F32 fish_rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * d_dt())) : 1.f;
              F32 diff = 1.f - ws->hover_eval_open_t;
              ws->hover_eval_open_t += diff*fish_rate;
              if(abs_f32(diff) < 0.01f)
              {
                ws->hover_eval_open_t = 1.f;
              }
              else
              {
                df_request_frame();
              }
            }
          }
          
          //- rjf: calculate width
          F32 width_px = 40.f*ui_top_font_size();
          F32 expr_column_width_px = 10.f*ui_top_font_size();
          F32 value_column_width_px = 30.f*ui_top_font_size();
          if(viz_rows.first != 0)
          {
            D_EvalVizRow *row = viz_rows.first;
            E_Eval row_eval = e_eval_from_expr(scratch.arena, row->expr);
            String8 row_expr_string = d_expr_string_from_viz_row(scratch.arena, row);
            String8 row_display_value = df_value_string_from_eval(scratch.arena, D_EvalVizStringFlag_ReadOnlyDisplayRules, default_radix, ui_top_font(), ui_top_font_size(), 500.f, row_eval, row->member, row->cfg_table);
            expr_column_width_px = fnt_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, 0, row_expr_string).x + ui_top_font_size()*5.f;
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
            for(D_EvalVizRow *row = viz_rows.first; row != 0; row = row->next)
            {
              //- rjf: unpack row
              E_Eval row_eval = e_eval_from_expr(scratch.arena, row->expr);
              String8 row_expr_string = d_expr_string_from_viz_row(scratch.arena, row);
              String8 row_edit_value = df_value_string_from_eval(scratch.arena, 0, default_radix, ui_top_font(), ui_top_font_size(), 500.f, row_eval, row->member, row->cfg_table);
              String8 row_display_value = df_value_string_from_eval(scratch.arena, D_EvalVizStringFlag_ReadOnlyDisplayRules, default_radix, ui_top_font(), ui_top_font_size(), 500.f, row_eval, row->member, row->cfg_table);
              B32 row_is_editable = d_type_key_is_editable(row_eval.type_key);
              B32 row_is_expandable = d_type_key_is_expandable(row_eval.type_key);
              
              //- rjf: determine if row's data is fresh and/or bad
              B32 row_is_fresh = 0;
              B32 row_is_bad = 0;
              switch(row_eval.mode)
              {
                default:{}break;
                case E_Mode_Offset:
                {
                  CTRL_Entity *space_entity = d_entity_from_eval_space(row_eval.space);
                  if(space_entity->kind == CTRL_EntityKind_Process)
                  {
                    U64 size = e_type_byte_size_from_key(row_eval.type_key);
                    size = Min(size, 64);
                    Rng1U64 vaddr_rng = r1u64(row_eval.value.u64, row_eval.value.u64+size);
                    CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, space_entity->machine_id, space_entity->handle, vaddr_rng, 0);
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
                ui_spacer(ui_em(0.75f, 1.f));
                if(row->depth > 0)
                {
                  for(S32 indent = 0; indent < row->depth; indent += 1)
                  {
                    ui_spacer(ui_em(0.75f, 1.f));
                    UI_Flags(UI_BoxFlag_DrawSideLeft) ui_spacer(ui_em(1.5f, 1.f));
                  }
                }
                U64 row_hash = df_hash_from_expand_key(row->key);
                B32 row_is_expanded = d_expand_key_is_set(&eval_view->expand_tree_table, row->key);
                if(row_is_expandable)
                  UI_PrefWidth(ui_em(1.5f, 1)) 
                  if(ui_pressed(ui_expanderf(row_is_expanded, "###%I64x_%I64x_is_expanded", row->key.parent_hash, row->key.child_num)))
                {
                  d_expand_set_expansion(eval_view->arena, &eval_view->expand_tree_table, row->parent_key, row->key, !row_is_expanded);
                }
                if(!row_is_expandable)
                {
                  UI_PrefWidth(ui_em(1.5f, 1))
                    UI_Flags(UI_BoxFlag_DrawTextWeak)
                    DF_Font(DF_FontSlot_Icons)
                    ui_label(df_g_icon_kind_text_table[DF_IconKind_Dot]);
                }
                UI_WidthFill UI_TextRasterFlags(df_raster_flags_from_slot(DF_FontSlot_Code))
                {
                  UI_PrefWidth(ui_px(expr_column_width_px, 1.f)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), row_expr_string);
                  ui_spacer(ui_em(1.5f, 1.f));
                  if(row_is_editable)
                  {
                    if(row_is_fresh)
                    {
                      Vec4F32 rgba = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rgba));
                    }
                    UI_Signal sig = df_line_editf(DF_LineEditFlag_CodeContents|
                                                  DF_LineEditFlag_DisplayStringIsCode|
                                                  DF_LineEditFlag_PreferDisplayString|
                                                  DF_LineEditFlag_Border,
                                                  0, 0, &ws->hover_eval_txt_cursor, &ws->hover_eval_txt_mark, ws->hover_eval_txt_buffer, sizeof(ws->hover_eval_txt_buffer), &ws->hover_eval_txt_size, 0, row_edit_value, "%S###val_%I64x", row_display_value, row_hash);
                    if(ui_pressed(sig))
                    {
                      ws->hover_eval_focused = 1;
                    }
                    if(ui_committed(sig))
                    {
                      String8 commit_string = str8(ws->hover_eval_txt_buffer, ws->hover_eval_txt_size);
                      B32 success = d_commit_eval_value_string(row_eval, commit_string);
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
                      Vec4F32 rgba = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rgba));
                      ui_set_next_flags(UI_BoxFlag_DrawBackground);
                    }
                    df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), row_display_value);
                  }
                }
                if(row == viz_rows.first)
                {
                  UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_CornerRadius00(0)
                    UI_CornerRadius01(0)
                    UI_CornerRadius10(0)
                    UI_CornerRadius11(0)
                  {
                    UI_Signal watch_sig = df_icon_buttonf(DF_IconKind_List, 0, "###watch_hover_eval");
                    if(ui_hovering(watch_sig)) UI_Tooltip DF_Font(DF_FontSlot_Main) UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                    {
                      ui_labelf("Add the hovered expression to an opened watch view.");
                    }
                    if(ui_clicked(watch_sig))
                    {
                      df_msg(DF_MsgKind_ToggleWatchExpression, .string = expr);
                    }
                  }
                  if(ws->hover_eval_file_path.size != 0 || ws->hover_eval_vaddr != 0)
                    UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_CornerRadius10(corner_radius)
                    UI_CornerRadius11(corner_radius)
                  {
                    UI_Signal pin_sig = df_icon_buttonf(DF_IconKind_Pin, 0, "###pin_hover_eval");
                    if(ui_hovering(pin_sig)) UI_Tooltip DF_Font(DF_FontSlot_Main) UI_FontSize(df_font_size_from_slot(DF_FontSlot_Main))
                      UI_CornerRadius00(0)
                      UI_CornerRadius01(0)
                      UI_CornerRadius10(0)
                      UI_CornerRadius11(0)
                    {
                      ui_labelf("Pin the hovered expression to this code location.");
                    }
                    if(ui_clicked(pin_sig))
                    {
                      df_msg(DF_MsgKind_ToggleWatchPin,
                             .file_path  = ws->hover_eval_file_path,
                             .cursor     = ws->hover_eval_file_pt,
                             .vaddr_range= r1u64(ws->hover_eval_vaddr, ws->hover_eval_vaddr),
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
              ws->hover_eval_last_frame_idx = d_frame_index();
            }
            else if(ws->hover_eval_last_frame_idx+2 < d_frame_index())
            {
              df_request_frame();
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
    {
      for(PanelTask *t = first_panel_task; t != 0; t = t->next)
      {
        MD_Node *panel_cfg = t->root;
        Axis2 split_axis   = t->split_axis;
        
        //////////////////////////
        //- rjf: continue on leaf panels
        //
        MD_Node *first_child_panel = md_node_from_chain_flags(panel_cfg->first, &md_nil_node, MD_NodeFlag_Numeric);
        if(md_node_is_nil(first_child_panel))
        {
          continue;
        }
        
        //////////////////////////
        //- rjf: unpack panel info
        //
        Rng2F32 panel_rect = df_target_rect_from_panel_cfg(content_rect, panel_cfg);
        
        //////////////////////////
        //- rjf: boundary tab-drag/drop sites
        //
        {
          MD_Node *drag_cfg_tree = df_cfg_tree_from_handle(df_state->drag_drop_regs->tab);
          if(df_drag_is_active() && !md_node_is_nil(drag_cfg_tree))
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
            //
            if(t == first_panel_task) UI_CornerRadius(corner_radius)
            {
              Vec2F32 panel_rect_center = center_2f32(panel_rect);
              Axis2 axis = axis2_flip(ws->root_panel->split_axis);
              for(EachEnumVal(Side, side))
              {
                UI_Key key = ui_key_from_stringf(ui_key_zero(), "root_extra_split_%i", side);
                Rng2F32 site_rect = panel_rect;
                site_rect.p0.v[axis2_flip(axis)] = panel_rect_center.v[axis2_flip(axis)] - drop_site_major_dim_px/2;
                site_rect.p1.v[axis2_flip(axis)] = panel_rect_center.v[axis2_flip(axis)] + drop_site_major_dim_px/2;
                site_rect.p0.v[axis] = panel_rect.v[side].v[axis] - drop_site_minor_dim_px/2;
                site_rect.p1.v[axis] = panel_rect.v[side].v[axis] + drop_site_minor_dim_px/2;
                
                // rjf: build
                UI_Box *site_box = &ui_g_nil_box;
                {
                  UI_Rect(site_rect)
                  {
                    site_box = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
                    ui_signal_from_box(site_box);
                  }
                  UI_Box *site_box_viz = &ui_g_nil_box;
                  UI_Parent(site_box) UI_WidthFill UI_HeightFill
                    UI_Padding(ui_px(padding, 1.f))
                    UI_Column
                    UI_Padding(ui_px(padding, 1.f))
                  {
                    ui_set_next_child_layout_axis(axis2_flip(axis));
                    if(ui_key_match(key, ui_drop_hot_key()))
                    {
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = df_rgba_from_theme_color(DF_ThemeColor_Hover)));
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
                  UI_Rect(future_split_rect) DF_Palette(DF_PaletteCode_DropSiteOverlay)
                  {
                    ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
                  }
                }
                
                // rjf: drop
                if(ui_key_match(site_box->key, ui_drop_hot_key()) && df_drag_drop())
                {
                  Dir2 dir = (axis == Axis2_Y ? (side == Side_Min ? Dir2_Up : Dir2_Down) :
                              axis == Axis2_X ? (side == Side_Min ? Dir2_Left : Dir2_Right) :
                              Dir2_Invalid);
                  if(dir != Dir2_Invalid)
                  {
                    df_msg(DF_MsgKind_SplitPanel,
                           .panel = df_handle_from_cfg_tree(panel_cfg),
                           .tab   = df_state->drag_drop_regs->tab,
                           .dir2  = dir);
                  }
                }
              }
            }
            
            //- rjf: iterate all children, build boundary drop sites
            UI_CornerRadius(corner_radius) for(MD_Node *child = md_node_from_chain_flags(panel_cfg->first, &md_nil_node, MD_NodeFlag_Numeric);;
                                               child = md_node_from_chain_flags(child->next, &md_nil_node, MD_NodeFlag_Numeric))
            {
              // rjf: form rect
              Rng2F32 child_rect = df_target_rect_from_panel_child_cfg(panel_rect, split_axis, child);
              Vec2F32 child_rect_center = center_2f32(child_rect);
              UI_Key key = ui_key_from_stringf(ui_key_zero(), "drop_boundary_%p_%p", panel_cfg, child);
              Rng2F32 site_rect = r2f32(child_rect_center, child_rect_center);
              site_rect.p0.v[split_axis] = child_rect.p0.v[split_axis] - drop_site_minor_dim_px/2;
              site_rect.p1.v[split_axis] = child_rect.p0.v[split_axis] + drop_site_minor_dim_px/2;
              site_rect.p0.v[axis2_flip(split_axis)] -= drop_site_major_dim_px/2;
              site_rect.p1.v[axis2_flip(split_axis)] += drop_site_major_dim_px/2;
              
              // rjf: build
              UI_Box *site_box = &ui_g_nil_box;
              {
                UI_Rect(site_rect)
                {
                  site_box = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
                  ui_signal_from_box(site_box);
                }
                UI_Box *site_box_viz = &ui_g_nil_box;
                UI_Parent(site_box) UI_WidthFill UI_HeightFill
                  UI_Padding(ui_px(padding, 1.f))
                  UI_Column
                  UI_Padding(ui_px(padding, 1.f))
                {
                  ui_set_next_child_layout_axis(axis2_flip(split_axis));
                  if(ui_key_match(key, ui_drop_hot_key()))
                  {
                    ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = df_rgba_from_theme_color(DF_ThemeColor_Hover)));
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
                UI_Rect(future_split_rect) DF_Palette(DF_PaletteCode_DropSiteOverlay)
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
                }
              }
              
              // rjf: drop
              if(ui_key_match(site_box->key, ui_drop_hot_key()) && df_drag_drop())
              {
                Dir2 dir = (split_axis == Axis2_X ? Dir2_Left : Dir2_Up);
                MD_Node *split_panel = child;
                if(md_node_is_nil(split_panel))
                {
                  split_panel = panel_cfg->last;
                  dir = (split_axis == Axis2_X ? Dir2_Right : Dir2_Down);
                }
                df_msg(DF_MsgKind_SplitPanel,
                       .panel      = df_handle_from_cfg_tree(split_panel),
                       .tab        = df_state->drag_drop_regs->tab,
                       .dir2       = dir);
              }
              
              // rjf: exit on opl child
              if(md_node_is_nil(child))
              {
                break;
              }
            }
          }
        }
        
        //////////////////////////
        //- rjf: do UI for drag boundaries between all children
        //
        for(MD_Node *child = md_node_from_chain_flags(panel_cfg->first, &md_nil_node, MD_NodeFlag_Numeric), *next = &md_nil_node;
            !md_node_is_nil(child);
            child = next)
        {
          next = md_node_from_chain_flags(child->next, &md_nil_node, MD_NodeFlag_Numeric);
          if(md_node_is_nil(next))
          {
            break;
          }
          MD_Node *min_child = child;
          MD_Node *max_child = next;
          F32 min_child_pct = f32_from_str8(min_child->string);
          F32 max_child_pct = f32_from_str8(max_child->string);
          Rng2F32 min_child_rect = df_target_rect_from_panel_child_cfg(panel_rect, split_axis, min_child);
          Rng2F32 max_child_rect = df_target_rect_from_panel_child_cfg(panel_rect, split_axis, max_child);
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
              F32 sum_pct = min_child_pct + max_child_pct;
              df_cfg_tree_set_stringf(min_child, "%f", 0.5f*sum_pct);
              df_cfg_tree_set_stringf(max_child, "%f", 0.5f*sum_pct);
            }
            else if(ui_pressed(sig))
            {
              Vec2F32 v = {min_child_pct, max_child_pct};
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
              df_cfg_tree_set_stringf(min_child, "%f", min_pct__after);
              df_cfg_tree_set_stringf(max_child, "%f", max_pct__after);
              is_changing_panel_boundaries = 1;
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: animate panels
    //
#if 0 // TODO(rjf): @msgs
    {
      F32 rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-50.f * d_dt())) : 1.f;
      Vec2F32 content_rect_dim = dim_2f32(content_rect);
      for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
      {
        Rng2F32 target_rect_px = df_target_rect_from_panel(content_rect, ws->root_panel, panel);
        Rng2F32 target_rect_pct = r2f32p(target_rect_px.x0/content_rect_dim.x,
                                         target_rect_px.y0/content_rect_dim.y,
                                         target_rect_px.x1/content_rect_dim.x,
                                         target_rect_px.y1/content_rect_dim.y);
        if(abs_f32(target_rect_pct.x0 - panel->animated_rect_pct.x0) > 0.005f ||
           abs_f32(target_rect_pct.y0 - panel->animated_rect_pct.y0) > 0.005f ||
           abs_f32(target_rect_pct.x1 - panel->animated_rect_pct.x1) > 0.005f ||
           abs_f32(target_rect_pct.y1 - panel->animated_rect_pct.y1) > 0.005f)
        {
          df_request_frame();
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
#endif
    
    ////////////////////////////
    //- rjf: panel leaf UI
    //
    ProfScope("leaf panel UI")
    {
      for(PanelTask *t = first_panel_task; t != 0; t = t->next)
      {
        ////////////////////////
        //- rjf: unpack general panel info
        //
        MD_Node *panel_cfg = t->root;
        MD_Node *first_panel_child = md_node_from_chain_flags(panel_cfg->first, &md_nil_node, MD_NodeFlag_Numeric);
        Side tab_side = md_node_is_nil(md_child_from_string(panel_cfg, str8_lit("tabs_on_bottom"), 0)) ? Side_Min : Side_Max;
        B32 panel_is_focused = (window_is_focused &&
                                !ws->menu_bar_focused &&
                                (!query_is_open || !ws->query_view_selected) &&
                                !ui_any_ctx_menu_is_open() &&
                                !ws->hover_eval_focused &&
                                focused_panel_cfg == panel_cfg);
        
        ////////////////////////
        //- rjf: skip non-leaf panels
        //
        if(!md_node_is_nil(first_panel_child)) {continue;}
        
        ////////////////////////
        //- rjf: unpack leaf info
        //
        MD_Node *selected_tab_cfg = &md_nil_node;
        DF_ViewSpec *selected_tab_spec = df_view_spec_from_string(str8_lit("empty"));
        String8 tab_filter_string = {0};
        String8 tab_expr_string = {0};
        B32 tab_filter_open = 0;
        F32 tab_filtering_t = 0;
        {
          for(MD_EachNode(child, panel_cfg))
          {
            if(!md_node_is_nil(md_tag_from_string(child, str8_lit("selected"), 0)))
            {
              selected_tab_cfg = child;
              break;
            }
          }
          if(!md_node_is_nil(selected_tab_cfg))
          {
            selected_tab_spec = df_view_spec_from_string(selected_tab_cfg->first->first->string);
            tab_filter_string = raw_from_escaped_string(scratch.arena, md_tag_from_string(selected_tab_cfg, str8_lit("filter"), 0)->first->string);
            tab_expr_string = raw_from_escaped_string(scratch.arena, selected_tab_cfg->first->string);
            tab_filter_open = !md_node_is_nil(md_tag_from_string(selected_tab_cfg, str8_lit("filter_open"), 0));
            tab_filtering_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "tab_%p_filter_t", selected_tab_cfg),
                                      (F32)!!tab_filter_open);
          }
        }
        
        ////////////////////////
        //- rjf: build
        //
        UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
        {
          //////////////////////////
          //- rjf: push registers for this panel
          //
          df_push_regs(.panel     = df_handle_from_cfg_tree(panel_cfg),
                       .tab       = df_handle_from_cfg_tree(selected_tab_cfg),
                       .file_path = d_file_path_from_eval_string(scratch.arena, tab_expr_string));
          
          //////////////////////////
          //- rjf: calculate UI rectangles
          //
          Rng2F32 target_rect_px = df_target_rect_from_panel_cfg(content_rect, panel_cfg);
          Rng2F32 target_rect_pct = r2f32p(target_rect_px.x0/content_rect_dim.x,
                                           target_rect_px.y0/content_rect_dim.y,
                                           target_rect_px.x1/content_rect_dim.x,
                                           target_rect_px.y1/content_rect_dim.y);
          Rng2F32 panel_rect_pct =
          {
            ui_anim(ui_key_from_stringf(ui_key_zero(), "panel_%p_x0", panel_cfg), target_rect_pct.x0),
            ui_anim(ui_key_from_stringf(ui_key_zero(), "panel_%p_y0", panel_cfg), target_rect_pct.y0),
            ui_anim(ui_key_from_stringf(ui_key_zero(), "panel_%p_x1", panel_cfg), target_rect_pct.x1),
            ui_anim(ui_key_from_stringf(ui_key_zero(), "panel_%p_y1", panel_cfg), target_rect_pct.y1),
          };
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
          Rng2F32 panel_content_rect = r2f32p(panel_rect.x0, panel_rect.y0+tab_bar_vheight, panel_rect.x1, panel_rect.y1);
          Rng2F32 filter_rect = {0};
          if(tab_side == Side_Max)
          {
            tab_bar_rect.y0 = panel_rect.y1 - tab_bar_vheight;
            tab_bar_rect.y1 = panel_rect.y1;
            panel_content_rect.y0 = panel_rect.y0;
            panel_content_rect.y1 = panel_rect.y1 - tab_bar_vheight;
          }
          if(tab_filtering_t > 0.01f)
          {
            filter_rect.x0 = panel_content_rect.x0;
            filter_rect.y0 = panel_content_rect.y0;
            filter_rect.x1 = panel_content_rect.x1;
            panel_content_rect.y0 += filter_bar_height*tab_filtering_t;
            filter_rect.y1 = panel_content_rect.y0;
          }
          
          //////////////////////////
          //- rjf: build combined split+movetab drag/drop sites
          //
          {
            if(df_drag_is_active() &&
               !md_node_is_nil(df_cfg_tree_from_handle(df_state->drag_drop_regs->tab)) &&
               contains_2f32(panel_rect, ui_mouse()))
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
                  ui_key_from_stringf(ui_key_zero(), "drop_split_center_%p", panel_cfg),
                  Dir2_Invalid,
                  r2f32(sub_2f32(panel_center, drop_site_half_dim),
                        add_2f32(panel_center, drop_site_half_dim))
                },
                {
                  ui_key_from_stringf(ui_key_zero(), "drop_split_up_%p", panel_cfg),
                  Dir2_Up,
                  r2f32p(panel_center.x-drop_site_half_dim.x,
                         panel_center.y-drop_site_half_dim.y - drop_site_half_dim.y*2,
                         panel_center.x+drop_site_half_dim.x,
                         panel_center.y+drop_site_half_dim.y - drop_site_half_dim.y*2),
                },
                {
                  ui_key_from_stringf(ui_key_zero(), "drop_split_down_%p", panel_cfg),
                  Dir2_Down,
                  r2f32p(panel_center.x-drop_site_half_dim.x,
                         panel_center.y-drop_site_half_dim.y + drop_site_half_dim.y*2,
                         panel_center.x+drop_site_half_dim.x,
                         panel_center.y+drop_site_half_dim.y + drop_site_half_dim.y*2),
                },
                {
                  ui_key_from_stringf(ui_key_zero(), "drop_split_left_%p", panel_cfg),
                  Dir2_Left,
                  r2f32p(panel_center.x-drop_site_half_dim.x - drop_site_half_dim.x*2,
                         panel_center.y-drop_site_half_dim.y,
                         panel_center.x+drop_site_half_dim.x - drop_site_half_dim.x*2,
                         panel_center.y+drop_site_half_dim.y),
                },
                {
                  ui_key_from_stringf(ui_key_zero(), "drop_split_right_%p", panel_cfg),
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
                if(dir != Dir2_Invalid && split_axis != t->split_axis)
                {
                  continue;
                }
                UI_Box *site_box = &ui_g_nil_box;
                {
                  UI_Rect(rect)
                  {
                    site_box = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
                    ui_signal_from_box(site_box);
                  }
                  UI_Box *site_box_viz = &ui_g_nil_box;
                  UI_Parent(site_box) UI_WidthFill UI_HeightFill
                    UI_Padding(ui_px(padding, 1.f))
                    UI_Column
                    UI_Padding(ui_px(padding, 1.f))
                  {
                    ui_set_next_child_layout_axis(axis2_flip(split_axis));
                    if(ui_key_match(key, ui_drop_hot_key()))
                    {
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = df_rgba_from_theme_color(DF_ThemeColor_Hover)));
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
                        DF_Palette(DF_PaletteCode_DropSiteOverlay) ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                        ui_spacer(ui_px(padding, 1.f));
                        if(split_side == Side_Max) { ui_set_next_flags(UI_BoxFlag_DrawBackground); }
                        DF_Palette(DF_PaletteCode_DropSiteOverlay) ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                      }
                    }
                  }
                  else
                  {
                    UI_Parent(site_box_viz) UI_WidthFill UI_HeightFill UI_Padding(ui_px(padding, 1.f))
                    {
                      ui_set_next_child_layout_axis(split_axis);
                      UI_Box *row_or_column = ui_build_box_from_key(0, ui_key_zero());
                      UI_Parent(row_or_column) UI_Padding(ui_px(padding, 1.f)) DF_Palette(DF_PaletteCode_DropSiteOverlay)
                      {
                        ui_build_box_from_key(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, ui_key_zero());
                      }
                    }
                  }
                }
                if(ui_key_match(site_box->key, ui_drop_hot_key()) && df_drag_drop())
                {
                  if(dir != Dir2_Invalid)
                  {
                    df_msg(DF_MsgKind_SplitPanel,
                           .tab  = df_state->drag_drop_regs->tab,
                           .dir2 = dir);
                  }
                  else
                  {
                    df_msg(DF_MsgKind_MoveTab,
                           .tab            = df_state->drag_drop_regs->tab,
                           .prev_cfg_child = df_handle_from_cfg_tree(panel_cfg->last));
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
                  UI_Rect(future_split_rect) DF_Palette(DF_PaletteCode_DropSiteOverlay)
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
          if(df_drag_is_active() && ui_key_match(ui_key_zero(), ui_drop_hot_key()))
          {
            UI_Rect(panel_rect)
            {
              UI_Key key = ui_key_from_stringf(ui_key_zero(), "catchall_drop_site_%p", panel_cfg);
              UI_Box *catchall_drop_site = ui_build_box_from_key(UI_BoxFlag_DropSite, key);
              ui_signal_from_box(catchall_drop_site);
              catchall_drop_site_hovered = ui_key_match(key, ui_drop_hot_key());
            }
          }
          
          //////////////////////////
          //- rjf: build filtering box
          //
          {
            MD_Node *tab_cfg = selected_tab_cfg;
            DF_ViewSpec *tab_spec = selected_tab_spec;
            UI_Focus(UI_FocusKind_On)
            {
              if(tab_filter_open && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept))
              {
                df_msg(DF_MsgKind_ApplyFilter);
              }
              if(tab_filter_open || tab_filtering_t > 0.01f)
              {
                UI_Box *filter_box = &ui_g_nil_box;
                UI_Rect(filter_rect)
                {
                  ui_set_next_child_layout_axis(Axis2_X);
                  filter_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip|UI_BoxFlag_DrawBorder, "filter_box_%p", tab_cfg);
                }
                UI_Parent(filter_box) UI_WidthFill UI_HeightFill
                {
                  UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                    DF_Font(DF_FontSlot_Icons)
                    UI_TextAlignment(UI_TextAlign_Center)
                    ui_label(df_g_icon_kind_text_table[DF_IconKind_Find]);
                  UI_PrefWidth(ui_text_dim(10, 1))
                  {
                    ui_label(str8_lit("Filter"));
                  }
                  ui_spacer(ui_em(0.5f, 1.f));
                  DF_Font(tab_spec->info.flags & DF_ViewSpecFlag_FilterIsCode ? DF_FontSlot_Code : DF_FontSlot_Main)
                    UI_Focus(tab_filter_open ? UI_FocusKind_On : UI_FocusKind_Off)
                    UI_TextPadding(ui_top_font_size()*0.5f)
                  {
                    DF_View *view = df_view_from_cfg_tree(tab_cfg);
                    UI_Signal sig = df_line_edit(DF_LineEditFlag_CodeContents*!!(tab_spec->info.flags & DF_ViewSpecFlag_FilterIsCode),
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
                      df_msg(DF_MsgKind_FocusPanel);
                    }
                  }
                }
              }
            }
          }
          
          //////////////////////////
          //- rjf: panel not selected? -> darken
          //
          if(panel_cfg != focused_panel_cfg)
          {
            UI_Palette(ui_build_palette(0, .background = df_rgba_from_theme_color(DF_ThemeColor_InactivePanelOverlay)))
              UI_Rect(panel_content_rect)
            {
              ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
            }
          }
          
          //////////////////////////
          //- rjf: build panel container box
          //
          UI_Box *panel_box = &ui_g_nil_box;
          UI_Rect(panel_content_rect) UI_ChildLayoutAxis(Axis2_Y) UI_CornerRadius(0) UI_Focus(UI_FocusKind_On)
          {
            UI_Key panel_key = ui_key_from_stringf(ui_key_zero(), "panel_box_%p", panel_cfg);
            panel_box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                              UI_BoxFlag_Clip|
                                              UI_BoxFlag_DrawBorder|
                                              UI_BoxFlag_DisableFocusOverlay|
                                              ((focused_panel_cfg != panel_cfg)*UI_BoxFlag_DisableFocusBorder)|
                                              ((focused_panel_cfg != panel_cfg)*UI_BoxFlag_DrawOverlay),
                                              panel_key);
          }
          
          //////////////////////////
          //- rjf: build panel overlay box
          //
          UI_Box *panel_overlay_box = &ui_g_nil_box;
          UI_Rect(panel_content_rect)
          {
            panel_overlay_box = ui_build_box_from_key(0, ui_key_zero());
          }
          
          //////////////////////////
          //- rjf: build selected tab view
          //
          UI_Parent(panel_box)
            UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
            UI_WidthFill
          {
            MD_Node *tab_cfg = selected_tab_cfg;
            DF_ViewSpec *tab_spec = selected_tab_spec;
            UI_Box *view_container_box = &ui_g_nil_box;
            UI_FixedWidth(dim_2f32(panel_content_rect).x)
              UI_FixedHeight(dim_2f32(panel_content_rect).y)
              UI_ChildLayoutAxis(Axis2_Y)
            {
              view_container_box = ui_build_box_from_key(0, ui_key_zero());
            }
            UI_Parent(view_container_box)
            {
              tab_spec->info.ui_hook(panel_content_rect);
            }
          }
          
          //////////////////////////
          //- rjf: loading animation for stable view
          //
          UI_Parent(panel_overlay_box)
          {
            MD_Node *tab_cfg = selected_tab_cfg;
            DF_ViewSpec *tab_spec = selected_tab_spec;
            DF_View *tab_view = df_view_from_cfg_tree(tab_cfg);
            F32 loading_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "loading_t_%p", tab_cfg), (F32)!!tab_view->is_loading);
            df_loading_overlay(panel_content_rect, loading_t, tab_view->loading_progress_v, tab_view->loading_progress_v_target);
          }
          
          //////////////////////////
          //- rjf: take events to automatically start/end filtering, if applicable
          //
          UI_Focus(UI_FocusKind_On)
          {
            MD_Node *tab_cfg = selected_tab_cfg;
            DF_View *tab_view = df_view_from_cfg_tree(tab_cfg);
            DF_ViewSpec *tab_spec = selected_tab_spec;
            if(ui_is_focus_active() && tab_spec->info.flags & DF_ViewSpecFlag_TypingAutomaticallyFilters && !tab_filter_open)
            {
              for(UI_Event *evt = 0; ui_next_event(&evt);)
              {
                if(evt->flags & UI_EventFlag_Paste)
                {
                  ui_eat_event(evt);
                  df_msg(DF_MsgKind_Filter);
                  df_msg(DF_MsgKind_Paste);
                }
                else if(evt->string.size != 0 && evt->kind == UI_EventKind_Text)
                {
                  ui_eat_event(evt);
                  df_msg(DF_MsgKind_Filter);
                  df_msg(DF_MsgKind_InsertText, .string = evt->string);
                }
              }
            }
            if(tab_spec->info.flags & DF_ViewSpecFlag_CanFilter && (tab_view->query_string_size != 0 || tab_filter_open) && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Cancel))
            {
              df_msg(DF_MsgKind_ClearFilter);
            }
          }
          
          //////////////////////////
          //- rjf: consume panel fallthrough interaction events
          //
          UI_Signal panel_sig = ui_signal_from_box(panel_box);
          if(ui_pressed(panel_sig))
          {
            df_msg(DF_MsgKind_FocusPanel);
          }
          
          //////////////////////////
          //- rjf: build tab bar
          //
          UI_Focus(UI_FocusKind_Off)
          {
            Temp scratch = scratch_begin(0, 0);
            
            // rjf: types
            typedef struct TabTask TabTask;
            struct TabTask
            {
              TabTask *next;
              MD_Node *root;
            };
            typedef struct DropSite DropSite;
            struct DropSite
            {
              F32 p;
              MD_Node *prev_tab;
            };
            
            // rjf: gather tabs
            TabTask *first_tab_task = 0;
            TabTask *last_tab_task = 0;
            U64 tab_count = 0;
            for(MD_EachNode(child, panel_cfg->first))
            {
              if(child->flags & MD_NodeFlag_StringSingleQuote)
              {
                TabTask *t = push_array(scratch.arena, TabTask, 1);
                SLLQueuePush(first_tab_task, last_tab_task, t);
                t->root = child;
                tab_count += 1;
              }
            }
            
            // rjf: prep output data
            UI_Box *tab_bar_box = &ui_g_nil_box;
            U64 drop_site_count = tab_count+1;
            DropSite *drop_sites = push_array(scratch.arena, DropSite, drop_site_count);
            F32 drop_site_max_p = 0;
            
            // rjf: build
            UI_CornerRadius(0)
            {
              UI_Rect(tab_bar_rect) tab_bar_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_ViewClampX|UI_BoxFlag_ViewScrollX|UI_BoxFlag_Clickable, "tab_bar_%p", panel_cfg);
              if(tab_side == Side_Max)
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
              U64 tab_idx = 0;
              UI_PrefWidth(ui_em(18.f, 0.5f))
                UI_CornerRadius00(tab_side == Side_Min ? corner_radius : 0)
                UI_CornerRadius01(tab_side == Side_Min ? 0 : corner_radius)
                UI_CornerRadius10(tab_side == Side_Min ? corner_radius : 0)
                UI_CornerRadius11(tab_side == Side_Min ? 0 : corner_radius)
                for(TabTask *t = first_tab_task, *prev = 0;; (prev = t, t = t->next))
              {
                temp_end(scratch);
                
                // rjf: if before this tab is the prev-view of the current tab drag,
                // draw empty space
#if 0 // TODO(rjf): @msgs
                if(df_drag_is_active() && catchall_drop_site_hovered)
                {
                  DF_Panel *dst_panel = df_panel_from_handle(df_last_drag_drop_panel);
                  DF_View *drag_view = df_view_from_handle(df_drag_drop_payload.view);
                  DF_View *dst_prev_view = df_view_from_handle(df_last_drag_drop_prev_tab);
                  if(dst_panel == panel &&
                     ((!df_view_is_nil(view) && dst_prev_view == view->order_prev && drag_view != view && drag_view != view->order_prev) ||
                      (df_view_is_nil(view) && dst_prev_view == panel->last_tab_view && drag_view != panel->last_tab_view)))
                  {
                    UI_PrefWidth(ui_em(9.f, 0.2f)) UI_Column
                    {
                      ui_spacer(ui_em(0.2f, 1.f));
                      UI_CornerRadius00(corner_radius)
                        UI_CornerRadius10(corner_radius)
                        DF_Palette(DF_PaletteCode_DropSiteOverlay)
                      {
                        ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                      }
                    }
                  }
                }
#endif
                
                // rjf: end on opl
                if(t == 0)
                {
                  break;
                }
                
                // rjf: unpack tab info
                MD_Node *tab_cfg = t->root;
                if(df_tab_cfg_is_project_filtered(tab_cfg)) { continue; }
                String8 tab_expr = tab_cfg->string;
                DF_ViewSpec *tab_spec = df_view_spec_from_string(tab_cfg->first->string);
                
                // rjf: gather info for this tab
                B32 tab_is_selected = (tab_cfg == selected_tab_cfg);
                DF_IconKind icon_kind = tab_spec->info.icon_kind;
                DR_FancyStringList title_fstrs = df_title_fstrs_from_view_spec_query(scratch.arena, tab_spec, tab_expr, ui_top_palette()->text, ui_top_palette()->text_weak, ui_top_font_size());
                
                // rjf: begin vertical region for this tab
                ui_set_next_child_layout_axis(Axis2_Y);
                UI_Box *tab_column_box = ui_build_box_from_stringf(!is_changing_panel_boundaries*UI_BoxFlag_AnimatePosX, "tab_column_%p", tab_cfg);
                
                // rjf: build tab container box
                UI_Parent(tab_column_box) UI_PrefHeight(ui_px(tab_bar_vheight, 1)) DF_Palette(tab_is_selected ? DF_PaletteCode_Tab : DF_PaletteCode_TabInactive)
                {
                  if(tab_side == Side_Max)
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
                                                              (UI_BoxFlag_DrawDropShadow*tab_is_selected)|
                                                              UI_BoxFlag_Clickable,
                                                              "tab_%p", tab_cfg);
                  
                  // rjf: build tab contents
                  UI_Parent(tab_box)
                  {
                    UI_WidthFill UI_Row
                    {
                      ui_spacer(ui_em(0.5f, 1.f));
                      if(icon_kind != DF_IconKind_Null)
                      {
                        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                          DF_Font(DF_FontSlot_Icons)
                          UI_TextAlignment(UI_TextAlign_Center)
                          UI_PrefWidth(ui_em(1.75f, 1.f))
                          ui_label(df_g_icon_kind_text_table[icon_kind]);
                      }
                      UI_PrefWidth(ui_text_dim(10, 0))
                      {
                        UI_Box *name_box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                        ui_box_equip_display_fancy_strings(name_box, &title_fstrs);
                      }
                    }
                    UI_PrefWidth(ui_em(2.35f, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
                      DF_Font(DF_FontSlot_Icons)
                      UI_FontSize(df_font_size_from_slot(DF_FontSlot_Icons)*0.75f)
                      UI_Flags(UI_BoxFlag_DrawTextWeak)
                      UI_CornerRadius00(0)
                      UI_CornerRadius01(0)
                    {
                      UI_Palette *palette = ui_build_palette(ui_top_palette());
                      palette->background = v4f32(0, 0, 0, 0);
                      ui_set_next_palette(palette);
                      UI_Signal sig = ui_buttonf("%S###close_view_%p", df_g_icon_kind_text_table[DF_IconKind_X], tab_cfg);
                      if(ui_clicked(sig) || ui_middle_clicked(sig))
                      {
                        df_msg(DF_MsgKind_CloseTab, .tab = df_handle_from_cfg_tree(tab_cfg));
                      }
                    }
                  }
                  
                  // rjf: consume events for tab clicking
                  {
                    UI_Signal sig = ui_signal_from_box(tab_box);
                    if(ui_pressed(sig))
                    {
                      df_msg(DF_MsgKind_SelectTab, .tab = df_handle_from_cfg_tree(tab_cfg));
                      df_msg(DF_MsgKind_FocusPanel);
                    }
                    else if(ui_dragging(sig) && !df_drag_is_active() && length_2f32(ui_drag_delta()) > 10.f)
                    {
                      df_drag_begin(.tab = df_handle_from_cfg_tree(tab_cfg));
                    }
                    else if(ui_right_clicked(sig))
                    {
                      ui_ctx_menu_open(df_state->tab_ctx_menu_key, sig.box->key, v2f32(0, sig.box->rect.y1 - sig.box->rect.y0));
                      ws->tab_ctx_menu_view = df_handle_from_cfg_tree(tab_cfg);
                    }
                    else if(ui_middle_clicked(sig))
                    {
                      df_msg(DF_MsgKind_CloseTab, .tab = df_handle_from_cfg_tree(tab_cfg));
                    }
                  }
                }
                
                // rjf: space for next tab
                {
                  ui_spacer(ui_em(0.3f, 1.f));
                }
                
                // rjf: store off drop-site
                drop_sites[tab_idx].p = tab_column_box->rect.x0 - tab_spacing/2;
                drop_sites[tab_idx].prev_tab = prev ? prev->root : &md_nil_node;
                drop_site_max_p = Max(tab_column_box->rect.x1, drop_site_max_p);
              }
              
              // rjf: build add-new-tab button
              UI_TextAlignment(UI_TextAlign_Center)
                UI_PrefWidth(ui_px(tab_bar_vheight, 1.f))
                UI_PrefHeight(ui_px(tab_bar_vheight, 1.f))
                UI_Column
              {
                if(tab_side == Side_Max)
                {
                  ui_spacer(ui_px(tab_bar_rv_diff-1.f, 1.f));
                }
                else
                {
                  ui_spacer(ui_px(1.f, 1.f));
                }
                UI_CornerRadius00(tab_side == Side_Min ? corner_radius : 0)
                  UI_CornerRadius10(tab_side == Side_Min ? corner_radius : 0)
                  UI_CornerRadius01(tab_side == Side_Max ? corner_radius : 0)
                  UI_CornerRadius11(tab_side == Side_Max ? corner_radius : 0)
                  DF_Font(DF_FontSlot_Icons)
                  UI_FontSize(ui_top_font_size())
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  UI_HoverCursor(OS_Cursor_HandPoint)
                  DF_Palette(DF_PaletteCode_ImplicitButton)
                {
                  UI_Box *add_new_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|
                                                                  UI_BoxFlag_DrawText|
                                                                  UI_BoxFlag_DrawBorder|
                                                                  UI_BoxFlag_DrawHotEffects|
                                                                  UI_BoxFlag_DrawActiveEffects|
                                                                  UI_BoxFlag_Clickable|
                                                                  UI_BoxFlag_DisableTextTrunc,
                                                                  "%S##add_new_tab_button_%p",
                                                                  df_g_icon_kind_text_table[DF_IconKind_Add],
                                                                  panel_cfg);
                  UI_Signal sig = ui_signal_from_box(add_new_box);
                  if(ui_clicked(sig))
                  {
                    df_msg(DF_MsgKind_FocusPanel);
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
              drop_sites[drop_site_count-1].prev_tab = last_tab_task ? last_tab_task->root : &md_nil_node;
            }
            
            // rjf: more precise drop-sites on tab bar
#if 0 // TODO(rjf): @msgs
            {
              Vec2F32 mouse = ui_mouse();
              MD_Node *tab_cfg = df_cfg_tree_from_handle(df_state->drag_drop_regs->tab);
              if(df_drag_is_active() && window_is_focused && contains_2f32(panel_rect, mouse) && !md_node_is_nil(tab_cfg))
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
                  df_last_drag_drop_prev_tab = df_handle_from_view(active_drop_site->prev_view);
                }
                else
                {
                  df_last_drag_drop_prev_tab = d_handle_zero();
                }
                
                // rjf: vis
                DF_Panel *drag_panel = df_panel_from_handle(df_drag_drop_payload.panel);
                if(!df_view_is_nil(view) && active_drop_site != 0) 
                {
                  DF_Palette(DF_PaletteCode_DropSiteOverlay) UI_Rect(tab_bar_rect)
                    ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
                }
                
                // rjf: drop
                DF_DragDropPayload payload = df_drag_drop_payload;
                if(catchall_drop_site_hovered && (active_drop_site != 0 && df_drag_drop(&payload)))
                {
                  DF_View *view = df_view_from_handle(payload.view);
                  DF_Panel *src_panel = df_panel_from_handle(payload.panel);
                  if(!df_panel_is_nil(panel) && !df_view_is_nil(view))
                  {
                    df_msg(DF_MsgKind_MoveTab,
                           .panel = df_handle_from_panel(src_panel),
                           .dst_panel = df_handle_from_panel(panel),
                           .view = df_handle_from_view(view),
                           .prev_view = df_handle_from_view(active_drop_site->prev_view));
                  }
                }
              }
            }
#endif
            
            scratch_end(scratch);
          }
          
          //////////////////////////
          //- rjf: less granular panel for tabs & entities drop-site
          //
#if 0 // TODO(rjf): @msgs
          if(catchall_drop_site_hovered)
          {
            df_last_drag_drop_panel = df_handle_from_panel(panel);
            
            DF_DragDropPayload *payload = &df_drag_drop_payload;
            DF_View *dragged_view = df_view_from_handle(payload->view);
            B32 view_is_in_panel = 0;
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
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
                DF_Palette(DF_PaletteCode_DropSiteOverlay) UI_Rect(panel_content_rect)
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
              }
              
              // rjf: drop
              {
                DF_DragDropPayload payload = {0};
                if(df_drag_drop(&payload))
                {
                  DF_Panel *src_panel = df_panel_from_handle(payload.panel);
                  DF_View *view = df_view_from_handle(payload.view);
                  D_Entity *entity = d_entity_from_handle(payload.entity);
                  
                  // rjf: view drop
                  if(!df_view_is_nil(view))
                  {
                    df_msg(DF_MsgKind_MoveTab,
                           .prev_view = df_handle_from_view(panel->last_tab_view),
                           .panel = df_handle_from_panel(src_panel),
                           .dst_panel = df_handle_from_panel(panel),
                           .view = df_handle_from_view(view));
                  }
                  
                  // rjf: entity drop
                  if(!d_entity_is_nil(entity))
                  {
                    // TODO(rjf): @msgs
                    // df_msg(D_MsgKind_SpawnEntityView,
                    // .panel = df_handle_from_panel(panel),
                    // .text_point = payload.text_point,
                    // .entity = d_handle_from_entity(entity));
                  }
                }
              }
            }
          }
#endif
          
          //////////////////////////
          //- rjf: accept file drops
          //
          for(UI_Event *evt = 0; ui_next_event(&evt);)
          {
            if(evt->kind == UI_EventKind_FileDrop && contains_2f32(panel_content_rect, evt->pos))
            {
              for(String8Node *n = evt->paths.first; n != 0; n = n->next)
              {
                Temp scratch = scratch_begin(0, 0);
                df_msg(DF_MsgKind_Open, .file_path = path_normalized_from_string(scratch.arena, n->string));
                scratch_end(scratch);
              }
              ui_eat_event(evt);
            }
          }
          
          //////////////////////////
          //- rjf: commit this panel's registers to parent, if selected
          //
          {
            D_Regs *tab_regs = d_pop_regs();
            if(focused_panel_cfg == panel_cfg)
            {
              MemoryCopyStruct(d_regs(), tab_regs);
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
        DF_Panel *panel;
        DF_View *list_first;
        DF_View *transient_owner;
      };
      Task start_task = {0, &df_nil_panel, ws->query_view_stack_top};
      Task *first_task = &start_task;
      Task *last_task = first_task;
      F32 rate = 1 - pow_f32(2, (-10.f * d_dt()));
      F32 fast_rate = 1 - pow_f32(2, (-40.f * d_dt()));
      for(DF_Panel *panel = ws->root_panel;
          !df_panel_is_nil(panel);
          panel = df_panel_rec_df_pre(panel).next)
      {
        Task *t = push_array(scratch.arena, Task, 1);
        SLLQueuePush(first_task, last_task, t);
        t->panel = panel;
        t->list_first = panel->first_tab_view;
      }
      for(Task *t = first_task; t != 0; t = t->next)
      {
        DF_View *list_first = t->list_first;
        for(DF_View *view = list_first; !df_view_is_nil(view); view = view->order_next)
        {
          if(!df_view_is_nil(view->first_transient))
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
              df_request_frame();
            }
            if(view->loading_t_target != 0 && (view == df_selected_tab_from_panel(t->panel) ||
                                               t->transient_owner == df_selected_tab_from_panel(t->panel)))
            {
              df_request_frame();
            }
          }
          view->loading_t += (view->loading_t_target - view->loading_t) * rate;
          view->is_filtering_t += ((F32)!!view->is_filtering - view->is_filtering_t) * fast_rate;
          view->scroll_pos.x.off -= view->scroll_pos.x.off * (df_setting_val_from_code(DF_SettingCode_ScrollingAnimations).s32 ? fast_rate : 1.f);
          view->scroll_pos.y.off -= view->scroll_pos.y.off * (df_setting_val_from_code(DF_SettingCode_ScrollingAnimations).s32 ? fast_rate : 1.f);
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
          if(view == df_selected_tab_from_panel(t->panel) ||
             t->transient_owner == df_selected_tab_from_panel(t->panel))
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
    if(df_drag_is_active() && ui_slot_press(UI_EventActionSlot_Cancel))
    {
      df_drag_kill();
      ui_kill_action();
    }
    
    ////////////////////////////
    //- rjf: font size changing
    //
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      if(evt->kind == UI_EventKind_Scroll && evt->modifiers & OS_EventFlag_Ctrl)
      {
        ui_eat_event(evt);
        if(evt->delta_2f32.y < 0)
        {
          df_msg(DF_MsgKind_IncUIFontScale);
        }
        else if(evt->delta_2f32.y > 0)
        {
          df_msg(DF_MsgKind_DecUIFontScale);
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
  if(!ui_box_is_nil(autocomp_box) && ws->autocomp_last_frame_idx+1 >= d_frame_index()+1)
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
  else if(!ui_box_is_nil(autocomp_box) && ws->autocomp_last_frame_idx+1 < d_frame_index()+1)
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
    df_request_frame();
  }
  
  //////////////////////////////
  //- rjf: animate
  //
  if(ui_animating_from_state(ws->ui))
  {
    df_request_frame();
  }
  
  //////////////////////////////
  //- rjf: draw UI
  //
  ws->draw_bucket = dr_bucket_make();
  D_BucketScope(ws->draw_bucket)
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
      Vec4F32 bg_color = df_rgba_from_theme_color(DF_ThemeColor_BaseBackground);
      dr_rect(os_client_rect_from_window(ws->os), bg_color, 0, 0, 0);
    }
    
    //- rjf: draw window border
    {
      Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_BaseBorder);
      dr_rect(os_client_rect_from_window(ws->os), color, 0, 1.f, 0.5f);
    }
    
    //- rjf: recurse & draw
    U64 total_heatmap_sum_count = 0;
    for(UI_Box *box = ui_root_from_state(ws->ui); !ui_box_is_nil(box);)
    {
      // rjf: get recursion
      UI_BoxRec rec = ui_box_rec_df_post(box, &ui_g_nil_box);
      
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
        Vec4F32 drop_shadow_color = df_rgba_from_theme_color(DF_ThemeColor_DropShadow);
        dr_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
      }
      
      // rjf: blur background
      if(box->flags & UI_BoxFlag_DrawBackgroundBlur && df_setting_val_from_code(DF_SettingCode_BackgroundBlur).s32)
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
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Hover);
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
          Vec4F32 shadow_color = df_rgba_from_theme_color(DF_ThemeColor_Hover);
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
          Vec4F32 match_color = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
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
          
          // rjf: draw border
          if(b->flags & UI_BoxFlag_DrawBorder)
          {
            R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 1), b->palette->colors[UI_ColorCode_Border], 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
            
            // rjf: hover effect
            if(b->flags & UI_BoxFlag_DrawHotEffects)
            {
              Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Hover);
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
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Focus);
            color.w *= 0.2f*b->focus_hot_t;
            R_Rect2DInst *inst = dr_rect(b->rect, color, 0, 0, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw focus border
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Focus);
            color.w *= b->focus_active_t;
            R_Rect2DInst *inst = dr_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: disabled overlay
          if(b->disabled_t >= 0.005f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_DisabledOverlay);
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
    F32 error_t = ClampBot(0, 1.f - (df_state->error_num_seconds_shown/df_state->error_num_seconds_to_show));
    if(error_t > 0.01f)
    {
      Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_NegativePopButtonBackground);
      color.w *= error_t;
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
      FNT_Run trailer_run = fnt_push_run_from_string(scratch.arena, df_font_from_slot(DF_FontSlot_Main), 16.f, 0, 0, 0, str8_lit("..."));
      DR_FancyStringList strs = {0};
      DR_FancyString str = {df_font_from_slot(DF_FontSlot_Main), str8_lit("Shift + F5"), v4f32(1, 1, 1, 1), 72.f, 0.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str);
      DR_FancyRunList runs = dr_fancy_run_list_from_fancy_string_list(scratch.arena, 0, FNT_RasterFlag_Smooth, &strs);
      dr_truncated_fancy_run_list(p, &runs, 1000000.f, trailer_run);
      dr_rect(r2f32(p, add_2f32(p, runs.dim)), v4f32(1, 0, 0, 0.5f), 0, 1, 0);
      dr_rect(r2f32(sub_2f32(p, v2f32(4, 4)), add_2f32(p, v2f32(4, 4))), v4f32(1, 0, 1, 1), 0, 0, 0);
#else
      Vec2F32 p = add_2f32(os_mouse_from_window(ws->os), v2f32(30, 0));
      dr_rect(os_client_rect_from_window(ws->os), v4f32(0, 0, 0, 0.4f), 0, 0, 0);
      DR_FancyStringList strs = {0};
      DR_FancyString str1 = {df_font_from_slot(DF_FontSlot_Main), str8_lit("T"), v4f32(1, 1, 1, 1), 16.f, 4.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str1);
      DR_FancyString str2 = {df_font_from_slot(DF_FontSlot_Main), str8_lit("his is a test of some "), v4f32(1, 0.5f, 0.5f, 1), 14.f, 0.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str2);
      DR_FancyString str3 = {df_font_from_slot(DF_FontSlot_Code), str8_lit("very fancy text!"), v4f32(1, 0.8f, 0.4f, 1), 18.f, 4.f, 4.f};
      dr_fancy_string_list_push(scratch.arena, &strs, &str3);
      DR_FancyRunList runs = dr_fancy_run_list_from_fancy_string_list(scratch.arena, 0, 0, &strs);
      FNT_Run trailer_run = fnt_push_run_from_string(scratch.arena, df_font_from_slot(DF_FontSlot_Main), 16.f, 0, 0, 0, str8_lit("..."));
      F32 limit = 500.f + sin_f32(d_time_in_seconds()/10.f)*200.f;
      dr_truncated_fancy_run_list(p, &runs, limit, trailer_run);
      dr_rect(r2f32p(p.x+limit, 0, p.x+limit+2.f, 1000), v4f32(1, 0, 0, 1), 0, 0, 0);
      df_request_frame();
#endif
    }
    
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: increment per-window frame counter
  //
  ws->frames_alive += 1;
  
  scratch_end(scratch);
  ProfEnd();
}

#if COMPILER_MSVC && !BUILD_DEBUG
#pragma optimize("", on)
#endif

////////////////////////////////
//~ rjf: Eval Viz

internal F32
df_append_value_strings_from_eval(Arena *arena, D_EvalVizStringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, S32 depth, E_Eval eval, E_Member *member, D_CfgTable *cfg_table, String8List *out)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  F32 space_taken = 0;
  
  //- rjf: unpack view rules
  U32 radix = default_radix;
  D_CfgVal *dec_cfg = d_cfg_val_from_string(cfg_table, str8_lit("dec"));
  D_CfgVal *hex_cfg = d_cfg_val_from_string(cfg_table, str8_lit("hex"));
  D_CfgVal *bin_cfg = d_cfg_val_from_string(cfg_table, str8_lit("bin"));
  D_CfgVal *oct_cfg = d_cfg_val_from_string(cfg_table, str8_lit("oct"));
  U64 best_insertion_stamp = Max(dec_cfg->insertion_stamp, Max(hex_cfg->insertion_stamp, Max(bin_cfg->insertion_stamp, oct_cfg->insertion_stamp)));
  if(dec_cfg != &d_nil_cfg_val && dec_cfg->insertion_stamp == best_insertion_stamp) { radix = 10; }
  if(hex_cfg != &d_nil_cfg_val && hex_cfg->insertion_stamp == best_insertion_stamp) { radix = 16; }
  if(bin_cfg != &d_nil_cfg_val && bin_cfg->insertion_stamp == best_insertion_stamp) { radix = 2; }
  if(oct_cfg != &d_nil_cfg_val && oct_cfg->insertion_stamp == best_insertion_stamp) { radix = 8; }
  B32 no_addr = (d_cfg_val_from_string(cfg_table, str8_lit("no_addr")) != &d_nil_cfg_val) && (flags & D_EvalVizStringFlag_ReadOnlyDisplayRules);
  B32 has_array = (d_cfg_val_from_string(cfg_table, str8_lit("array")) != &d_nil_cfg_val);
  
  //- rjf: member evaluations -> display member info
  if(eval.mode == E_Mode_Null && !e_type_key_match(e_type_key_zero(), eval.type_key) && member != 0)
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
      String8 string = d_string_from_simple_typed_eval(arena, flags, radix, value_eval);
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
      B32 ptee_has_string  = (E_TypeKind_Char8 <= direct_type_kind && direct_type_kind <= E_TypeKind_UChar32);
      CTRL_Entity *thread = d_regs_thread();
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
        String8 string_escaped = d_escaped_from_raw_string(arena, string);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string_escaped).x;
        space_taken += 2*fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
        str8_list_push(arena, out, str8_lit("\""));
        str8_list_push(arena, out, string_escaped);
        str8_list_push(arena, out, str8_lit("\""));
      }
      
      // rjf: special case: push strings for symbols
      if(!did_content && symbol_name.size != 0 &&
         ((type_kind == E_TypeKind_Ptr && direct_type_kind == E_TypeKind_Void) ||
          (type_kind == E_TypeKind_Ptr && direct_type_kind == E_TypeKind_Function) ||
          (type_kind == E_TypeKind_Function)))
      {
        did_content = 1;
        str8_list_push(arena, out, symbol_name);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, symbol_name).x;
      }
      
      // rjf: special case: need symbol name, don't have one
      if(!did_content && symbol_name.size == 0 &&
         ((type_kind == E_TypeKind_Ptr && direct_type_kind == E_TypeKind_Function) ||
          (type_kind == E_TypeKind_Function)) &&
         (flags & D_EvalVizStringFlag_ReadOnlyDisplayRules))
      {
        did_content = 1;
        String8 string = str8_lit("???");
        str8_list_push(arena, out, string);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
      }
      
      // rjf: descend for all other cases
      if(!did_content && ptee_has_content && (flags & D_EvalVizStringFlag_ReadOnlyDisplayRules))
      {
        did_content = 1;
        if(depth < 4)
        {
          E_Expr *deref_expr = e_expr_ref_deref(scratch.arena, eval.expr);
          E_Eval deref_eval = e_eval_from_expr(scratch.arena, deref_expr);
          space_taken += df_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, deref_eval, 0, cfg_table, out);
        }
        else
        {
          String8 ellipses = str8_lit("...");
          str8_list_push(arena, out, ellipses);
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
        }
      }
      
      // rjf: push pointer value
      B32 did_ptr_value = 0;
      if((!no_addr || !did_content) && ((flags & D_EvalVizStringFlag_ReadOnlyDisplayRules) || !did_string))
      {
        did_ptr_value = 1;
        if(did_content)
        {
          String8 ptr_prefix = str8_lit(" (");
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, ptr_prefix).x;
          str8_list_push(arena, out, ptr_prefix);
        }
        String8 string = d_string_from_simple_typed_eval(arena, flags, radix, value_eval);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
        str8_list_push(arena, out, string);
        if(did_content)
        {
          String8 close = str8_lit(")");
          space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, close).x;
          str8_list_push(arena, out, close);
        }
      }
    }break;
    
    //- rjf: arrays
    case E_TypeKind_Array:
    {
      // rjf: unpack type info
      E_Type *eval_type = e_type_from_key(scratch.arena, e_type_unwrap(eval.type_key));
      E_TypeKey direct_type_key = eval_type->direct_type_key;
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
        U64 string_buffer_size = 1024;
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
        String8 string_escaped = d_escaped_from_raw_string(arena, string);
        space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, string_escaped).x;
        space_taken += 2*fnt_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
        str8_list_push(arena, out, str8_lit("\""));
        str8_list_push(arena, out, string_escaped);
        str8_list_push(arena, out, str8_lit("\""));
      }
      
      // rjf: descend in all other cases
      if(!did_content && (flags & D_EvalVizStringFlag_ReadOnlyDisplayRules))
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
            space_taken += df_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, element_eval, 0, cfg_table, out);
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
        E_MemberArray data_members = e_type_data_members_from_key(scratch.arena, e_type_unwrap(eval.type_key));
        E_MemberArray filtered_data_members = d_filtered_data_members_from_members_cfg_table(scratch.arena, data_members, cfg_table);
        for(U64 member_idx = 0; member_idx < filtered_data_members.count && max_size > space_taken; member_idx += 1)
        {
          E_Member *mem = &filtered_data_members.v[member_idx];
          E_Expr *dot_expr = e_expr_ref_member_access(scratch.arena, eval.expr, mem->name);
          E_Eval dot_eval = e_eval_from_expr(scratch.arena, dot_expr);
          space_taken += df_append_value_strings_from_eval(arena, flags, radix, font, font_size, max_size-space_taken, depth+1, dot_eval, 0, cfg_table, out);
          if(member_idx+1 < filtered_data_members.count)
          {
            String8 comma = str8_lit(", ");
            space_taken += fnt_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
            str8_list_push(arena, out, comma);
          }
          if(space_taken > max_size && member_idx+1 < filtered_data_members.count)
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
  }
  
  scratch_end(scratch);
  ProfEnd();
  return space_taken;
}

internal String8
df_value_string_from_eval(Arena *arena, D_EvalVizStringFlags flags, U32 default_radix, FNT_Tag font, F32 font_size, F32 max_size, E_Eval eval, E_Member *member, D_CfgTable *cfg_table)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  df_append_value_strings_from_eval(scratch.arena, flags, default_radix, font, font_size, max_size, 0, eval, member, cfg_table, &strs);
  String8 result = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Hover Eval

internal void
df_set_hover_eval(Vec2F32 pos, String8 file_path, TxtPt pt, U64 vaddr, String8 string)
{
  DF_Window *window = df_window_from_handle(d_regs()->window);
  if(window->hover_eval_last_frame_idx+1 < d_frame_index() &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Left), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Middle), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Right), ui_key_zero()))
  {
    B32 is_new_string = !str8_match(window->hover_eval_string, string, 0);
    if(is_new_string)
    {
      window->hover_eval_first_frame_idx = window->hover_eval_last_frame_idx = d_frame_index();
      arena_clear(window->hover_eval_arena);
      window->hover_eval_string = push_str8_copy(window->hover_eval_arena, string);
      window->hover_eval_file_path = push_str8_copy(window->hover_eval_arena, file_path);
      window->hover_eval_file_pt = pt;
      window->hover_eval_vaddr = vaddr;
      window->hover_eval_focused = 0;
    }
    window->hover_eval_spawn_pos = pos;
    window->hover_eval_last_frame_idx = d_frame_index();
  }
}

////////////////////////////////
//~ rjf: Auto-Complete Lister

internal void
df_autocomp_lister_item_chunk_list_push(Arena *arena, DF_AutoCompListerItemChunkList *list, U64 cap, DF_AutoCompListerItem *item)
{
  DF_AutoCompListerItemChunkNode *n = list->last;
  if(n == 0 || n->count >= n->cap)
  {
    n = push_array(arena, DF_AutoCompListerItemChunkNode, 1);
    SLLQueuePush(list->first, list->last, n);
    n->cap = cap;
    n->v = push_array_no_zero(arena, DF_AutoCompListerItem, n->cap);
    list->chunk_count += 1;
  }
  MemoryCopyStruct(&n->v[n->count], item);
  n->count += 1;
  list->total_count += 1;
}

internal DF_AutoCompListerItemArray
df_autocomp_lister_item_array_from_chunk_list(Arena *arena, DF_AutoCompListerItemChunkList *list)
{
  DF_AutoCompListerItemArray array = {0};
  array.count = list->total_count;
  array.v = push_array_no_zero(arena, DF_AutoCompListerItem, array.count);
  U64 idx = 0;
  for(DF_AutoCompListerItemChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(DF_AutoCompListerItem)*n->count);
    idx += n->count;
  }
  return array;
}

internal int
df_autocomp_lister_item_qsort_compare(DF_AutoCompListerItem *a, DF_AutoCompListerItem *b)
{
  int result = 0;
  if(a->matches.count > b->matches.count)
  {
    result = -1;
  }
  else if(a->matches.count < b->matches.count)
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
df_autocomp_lister_item_array_sort__in_place(DF_AutoCompListerItemArray *array)
{
  quick_sort(array->v, array->count, sizeof(array->v[0]), df_autocomp_lister_item_qsort_compare);
}

internal String8
df_autocomp_query_word_from_input_string_off(String8 input, U64 cursor_off)
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

internal DF_AutoCompListerParams
df_view_rule_autocomp_lister_params_from_input_cursor(Arena *arena, String8 string, U64 cursor_off)
{
  DF_AutoCompListerParams params = {0};
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
          for(MD_EachNode(child, schema_node->first))
          {
            if(0){}
            else if(str8_match(child->string, str8_lit("expr"),           StringMatchFlag_CaseInsensitive)) {params.flags |= DF_AutoCompListerFlag_Locals;}
            else if(str8_match(child->string, str8_lit("member"),         StringMatchFlag_CaseInsensitive)) {params.flags |= DF_AutoCompListerFlag_Members;}
            else if(str8_match(child->string, str8_lit("lang"),           StringMatchFlag_CaseInsensitive)) {params.flags |= DF_AutoCompListerFlag_Languages;}
            else if(str8_match(child->string, str8_lit("arch"),           StringMatchFlag_CaseInsensitive)) {params.flags |= DF_AutoCompListerFlag_Architectures;}
            else if(str8_match(child->string, str8_lit("tex2dformat"),    StringMatchFlag_CaseInsensitive)) {params.flags |= DF_AutoCompListerFlag_Tex2DFormats;}
            else if(child->flags & (MD_NodeFlag_StringSingleQuote|MD_NodeFlag_StringDoubleQuote|MD_NodeFlag_StringTick))
            {
              str8_list_push(arena, &params.strings, child->string);
              params.flags |= DF_AutoCompListerFlag_ViewRuleParams;
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
df_set_autocomp_lister_query(UI_Key root_key, DF_AutoCompListerParams *params, String8 input, U64 cursor_off)
{
  DF_Window *window = df_window_from_handle(d_regs()->window);
  String8 query = df_autocomp_query_word_from_input_string_off(input, cursor_off);
  String8 current_query = str8(window->autocomp_lister_query_buffer, window->autocomp_lister_query_size);
  if(cursor_off != window->autocomp_cursor_off)
  {
    window->autocomp_query_dirty = 1;
    window->autocomp_cursor_off = cursor_off;
  }
  if(!str8_match(query, current_query, 0))
  {
    window->autocomp_force_closed = 0;
  }
  if(!ui_key_match(window->autocomp_root_key, root_key))
  {
    window->autocomp_force_closed = 0;
    window->autocomp_num_visible_rows_t = 0;
    window->autocomp_open_t = 0;
  }
  if(window->autocomp_last_frame_idx+1 < d_frame_index())
  {
    window->autocomp_force_closed = 0;
    window->autocomp_num_visible_rows_t = 0;
    window->autocomp_open_t = 0;
  }
  window->autocomp_root_key = root_key;
  arena_clear(window->autocomp_lister_params_arena);
  MemoryCopyStruct(&window->autocomp_lister_params, params);
  window->autocomp_lister_params.strings = str8_list_copy(window->autocomp_lister_params_arena, &window->autocomp_lister_params.strings);
  window->autocomp_lister_query_size = Min(query.size, sizeof(window->autocomp_lister_query_buffer));
  MemoryCopy(window->autocomp_lister_query_buffer, query.str, window->autocomp_lister_query_size);
  window->autocomp_last_frame_idx = d_frame_index();
}

////////////////////////////////
//~ rjf: Search Strings

internal void
df_set_search_string(String8 string)
{
  arena_clear(df_state->string_search_arena);
  df_state->string_search_string = push_str8_copy(df_state->string_search_arena, string);
}

internal String8
df_push_search_string(Arena *arena)
{
  String8 result = push_str8_copy(arena, df_state->string_search_string);
  return result;
}

////////////////////////////////
//~ rjf: Main State Accessors

internal Arena *
df_frame_arena(void)
{
  Arena *arena = df_state->frame_arenas[df_state->frame_index%ArrayCount(df_state->frame_arenas)];
  return arena;
}

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: handle <-> cfg tree

internal DF_Handle
df_handle_from_cfg_tree(MD_Node *cfg)
{
  DF_Handle handle = {0};
  handle.u64[0] = (U64)cfg;
  handle.u64[1] = cfg->user_gen;
  return handle;
}

internal MD_Node *
df_cfg_tree_from_handle(DF_Handle handle)
{
  MD_Node *cfg_tree = (MD_Node *)handle.u64[0];
  if(cfg_tree->user_gen != handle.u64[1])
  {
    cfg_tree = &md_nil_node;
  }
  return cfg_tree;
}

//- rjf: cfg tree -> slot

internal DF_CfgSlot
df_cfg_slot_from_tree(MD_Node *node)
{
  DF_CfgSlot slot = DF_CfgSlot_User;
  for(MD_Node *n = node; !md_node_is_nil(n); n = n->parent)
  {
    for(EachEnumVal(DF_CfgSlot, s))
    {
      if(n == df_state->cfg_slot_roots[s])
      {
        slot = s;
        goto end;
      }
    }
  }
  end:;
  return slot;
}

//- rjf: cfg slot allocations

internal MD_Node *
df_cfg_node_alloc(void)
{
  MD_Node *node = df_state->cfg_free;
  if(!md_node_is_nil(node))
  {
    SLLStackPop(df_state->cfg_free);
    U64 gen = node->user_gen;
    MemoryZeroStruct(node);
    node->user_gen = gen+1;
  }
  else
  {
    node = push_array(df_state->cfg_arena, MD_Node, 1);
  }
  node->first = node->last = node->parent = node->next = node->prev = node->first_tag = node->last_tag = &md_nil_node;
  node->user_gen += 1;
  return node;
}

internal void
df_cfg_node_release(MD_Node *node)
{
  Temp scratch = scratch_begin(0, 0);
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    MD_Node *node;
  };
  Task start_task = {0, node};
  Task *first_task = &start_task;
  Task *last_task = first_task;
  for(Task *t = first_task; t != 0; t = t->next)
  {
    for(MD_EachNode(child, t->node->first))
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->node = child;
      SLLQueuePush(first_task, last_task, task);
    }
    for(MD_EachNode(child, t->node->first_tag))
    {
      Task *task = push_array(scratch.arena, Task, 1);
      task->node = child;
      SLLQueuePush(first_task, last_task, task);
    }
    SLLStackPush(df_state->cfg_free, t->node);
    if(t->node->string.size != 0)
    {
      df_cfg_string_release(t->node->string);
    }
    if(t->node->raw_string.size != 0)
    {
      df_cfg_string_release(t->node->raw_string);
    }
    t->node->user_gen += 1;
  }
  scratch_end(scratch);
}

internal U64
df_cfg_string_bucket_idx_from_string_size(U64 size)
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
    default:{bucket_idx = ArrayCount(df_state->cfg_free_string_chunks)-1;}break;
  }
  return bucket_idx;
}

internal String8
df_cfg_string_alloc(String8 string)
{
  if(string.size == 0) {return str8_zero();}
  U64 bucket_idx = df_cfg_string_bucket_idx_from_string_size(string.size);
  
  // rjf: loop -> find node, allocate if not there
  //
  // (we do a loop here so that all allocation logic goes through
  // the same path, such that we *always* pull off a free list,
  // rather than just using what was pushed onto an arena directly,
  // which is not undoable; the free lists we control, and are thus
  // trivially undoable)
  //
  DF_StringChunkNode *node = 0;
  for(;node == 0;)
  {
    node = df_state->cfg_free_string_chunks[bucket_idx];
    
    // rjf: pull from bucket free list
    if(node != 0)
    {
      if(bucket_idx == ArrayCount(df_state->cfg_free_string_chunks)-1)
      {
        node = 0;
        DF_StringChunkNode *prev = 0;
        for(DF_StringChunkNode *n = df_state->cfg_free_string_chunks[bucket_idx];
            n != 0;
            prev = n, n = n->next)
        {
          if(n->size >= string.size+1)
          {
            if(prev == 0)
            {
              df_state->cfg_free_string_chunks[bucket_idx] = n->next;
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
        SLLStackPop(df_state->cfg_free_string_chunks[bucket_idx]);
      }
    }
    
    // rjf: no found node -> allocate new, push onto associated free list
    if(node == 0)
    {
      U64 chunk_size = 0;
      if(bucket_idx < ArrayCount(df_state->cfg_free_string_chunks)-1)
      {
        chunk_size = 1<<(bucket_idx+4);
      }
      else
      {
        chunk_size = u64_up_to_pow2(string.size);
      }
      U8 *chunk_memory = push_array(df_state->cfg_arena, U8, chunk_size);
      DF_StringChunkNode *chunk = (DF_StringChunkNode *)chunk_memory;
      SLLStackPush(df_state->cfg_free_string_chunks[bucket_idx], chunk);
    }
  }
  
  // rjf: fill string & return
  String8 allocated_string = str8((U8 *)node, string.size);
  MemoryCopy((U8 *)node, string.str, string.size);
  return allocated_string;
}

internal void
df_cfg_string_release(String8 string)
{
  if(string.size == 0) {return;}
  U64 bucket_idx = df_cfg_string_bucket_idx_from_string_size(string.size);
  DF_StringChunkNode *node = (DF_StringChunkNode *)string.str;
  node->size = u64_up_to_pow2(string.size);
  SLLStackPush(df_state->cfg_free_string_chunks[bucket_idx], node);
}

//- rjf: tree -> cfg slot copying

internal MD_Node *
df_cfg_tree_copy(MD_Node *src)
{
  Temp scratch = scratch_begin(0, 0);
  typedef struct Task Task;
  struct Task
  {
    Task *next;
    MD_Node *dst_parent;
    MD_Node *first;
  };
  Task start_task = {0, &md_nil_node, src};
  Task *first_task = &start_task;
  Task *last_task = first_task;
  MD_Node *dst_root = &md_nil_node;
  for(Task *t = first_task; t != 0; t = t->next)
  {
    for(MD_EachNode(src, t->first))
    {
      MD_Node *dst = df_cfg_node_alloc();
      dst->kind       = src->kind;
      dst->flags      = src->flags;
      dst->string     = df_cfg_string_alloc(src->string);
      dst->raw_string = df_cfg_string_alloc(src->raw_string);
      dst->src_offset = src->src_offset;
      dst->user_gen   = src->user_gen;
      if(!md_node_is_nil(t->dst_parent))
      {
        if(dst->kind == MD_NodeKind_Tag)
        {
          md_node_push_tag(t->dst_parent, dst);
        }
        else
        {
          md_node_push_child(t->dst_parent, dst);
        }
      }
      else
      {
        dst_root = dst;
      }
      if(!md_node_is_nil(src->first_tag))
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->first = src->first_tag;
        task->dst_parent = dst;
        SLLQueuePush(first_task, last_task, task);
      }
      if(!md_node_is_nil(src->first))
      {
        Task *task = push_array(scratch.arena, Task, 1);
        task->first = src->first;
        task->dst_parent = dst;
        SLLQueuePush(first_task, last_task, task);
      }
    }
  }
  scratch_end(scratch);
  return dst_root;
}

//- rjf: string -> cfg tree helper

internal MD_Node *
df_file_cfg_tree_from_string(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *root = md_tree_from_string(scratch.arena, string);
  MD_Node *result = df_cfg_tree_copy(root);
  scratch_end(scratch);
  return result;
}

internal MD_Node *
df_single_cfg_tree_from_string(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *root = md_tree_from_string(scratch.arena, string)->first;
  MD_Node *result = df_cfg_tree_copy(root);
  scratch_end(scratch);
  return result;
}

internal MD_Node *
df_single_cfg_tree_from_stringf(char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  MD_Node *result = df_single_cfg_tree_from_string(string);
  va_end(args);
  scratch_end(scratch);
  return result;
}

//- rjf: cfg node string replacing helper

internal void
df_cfg_tree_set_string(MD_Node *node, String8 new_string)
{
  df_cfg_string_release(node->string);
  node->string = df_cfg_string_alloc(new_string);
}

internal void
df_cfg_tree_set_stringf(MD_Node *node, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  df_cfg_tree_set_string(node, string);
  va_end(args);
  scratch_end(scratch);
}

//- rjf: cfg subtree replacing helper

internal MD_Node *
df_cfg_tree_set_key(MD_Node *root, String8 key, String8 value)
{
  MD_Node *existing_node = md_child_from_string(root, key, 0);
  MD_Node *new_node = &md_nil_node;
  if(value.size != 0)
  {
    new_node = df_single_cfg_tree_from_string(value);
  }
  MD_Node *prev_node = existing_node->prev;
  if(!md_node_is_nil(existing_node))
  {
    md_unhook_child(existing_node);
  }
  if(!md_node_is_nil(new_node))
  {
    md_node_insert_child(root, prev_node, new_node);
  }
  return new_node;
}

internal MD_Node *
df_cfg_tree_set_keyf(MD_Node *root, String8 key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 value = push_str8fv(scratch.arena, fmt, args);
  MD_Node *result = df_cfg_tree_set_key(root, key, value);
  va_end(args);
  scratch_end(scratch);
  return result;
}

//- rjf: key string <-> cfg tree

internal MD_Node *
df_cfg_tree_from_key(String8 string)
{
  MD_Node *result = &md_nil_node;
  Temp scratch = scratch_begin(0, 0);
  {
    MD_Node *lookup_tree = md_tree_from_string(scratch.arena, string);
    MD_Node *lookup_root = &md_nil_node;
    for(MD_EachNode(op, lookup_tree->first))
    {
      // rjf: user -> look into user subtree
      if(op == lookup_tree->first && str8_match(op->string, str8_lit("user"), StringMatchFlag_CaseInsensitive))
      {
        lookup_root = df_state->cfg_slot_roots[DF_CfgSlot_User];
      }
      
      // rjf: project -> look into project subtree
      else if(op == lookup_tree->first && str8_match(op->string, str8_lit("project"), StringMatchFlag_CaseInsensitive))
      {
        lookup_root = df_state->cfg_slot_roots[DF_CfgSlot_Project];
      }
      
      // rjf: skip cases
      else if(op->flags & MD_NodeFlag_Symbol && str8_match(op->string, str8_lit("."), 0))
      {
        continue;
      }
      
      // rjf: look up `.name` or `.name[idx]` or `.name:"label"`
      else if(op->flags & MD_NodeFlag_Symbol && str8_match(op->string, str8_lit("."), 0) &&
              op->next->flags & MD_NodeFlag_Identifier)
      {
        String8 label_target = {0};
        if(op->next->first->flags & MD_NodeFlag_StringDoubleQuote)
        {
          label_target = op->next->first->string;
        }
        U64 idx_target = 0;
        if(op->next->next->flags & MD_NodeFlag_HasBracketLeft &&
           op->next->next->flags & MD_NodeFlag_HasBracketRight &&
           op->next->next->first->flags & MD_NodeFlag_Numeric &&
           op->next->next->first == op->next->last)
        {
          String8 idx_string = md_string_from_children(scratch.arena, op->next->next);
          E_Eval idx_eval = e_eval_from_string(scratch.arena, idx_string);
          E_Eval idx_eval_value = e_value_eval_from_eval(idx_eval);
          idx_target = idx_eval_value.value.u64;
        }
        U64 idx_search = 0;
        for(MD_EachNode(tln, lookup_root->first))
        {
          if(str8_match(tln->string, op->next->string, 0))
          {
            B32 label_target_matches = 1;
            if(label_target.size != 0)
            {
              MD_Node *label_child = md_child_from_string(tln, str8_lit("label"), 0);
              label_target_matches = str8_match(label_target, label_child->first->string, 0);
            }
            lookup_root = tln;
            if(idx_target == idx_search)
            {
              break;
            }
            idx_search += 1;
          }
        }
      }
    }
    result = lookup_root;
  }
  scratch_end(scratch);
  return result;
}

internal String8
df_key_from_cfg_tree(Arena *arena, MD_Node *node)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strings = {0};
  for(MD_Node *n = node; !md_node_is_nil(n); n = n->parent)
  {
    if(n == df_state->cfg_slot_roots[DF_CfgSlot_User])
    {
      str8_list_push_front(scratch.arena, &strings, str8_lit("user"));
      break;
    }
    else if(n == df_state->cfg_slot_roots[DF_CfgSlot_Project])
    {
      str8_list_push_front(scratch.arena, &strings, str8_lit("project"));
      break;
    }
    U64 index = 0;
    for(MD_Node *n2 = n->prev; !md_node_is_nil(n2); n2 = n2->prev)
    {
      if(str8_match(n2->string, n->string, 0))
      {
        index += 1;
      }
    }
    str8_list_push_frontf(scratch.arena, &strings, ".%S[%I64u]", n->string, index);
  }
  String8 result = str8_list_join(arena, &strings, 0);
  scratch_end(scratch);
  return result;
}

//- rjf: config tree lookups

internal MD_Node *
df_cfg_tree_last_from_key(MD_Node *root, String8 key)
{
  MD_Node *result = &md_nil_node;
  for(MD_Node *tln = root->last; !md_node_is_nil(tln); tln = tln->prev)
  {
    if(str8_match(tln->string, key, 0))
    {
      result = tln;
      break;
    }
  }
  return result;
}

internal Vec4F32
df_rgba_from_cfg_tree(MD_Node *cfg)
{
  Vec4F32 result = {0};
  MD_Node *child = md_child_from_string(cfg, str8_lit("color"), 0);
  if(!md_node_is_nil(child))
  {
    // rjf: hex-specified colors
    U64 hex_val = 0;
    if(child->first == child->last &&
       child->flags & MD_NodeFlag_Numeric &&
       try_u64_from_str8_c_rules(child->first->string, &hex_val))
    {
      result = rgba_from_u32((U32)hex_val);
    }
    
    // rjf: per-channel expressions
    else
    {
      Temp scratch = scratch_begin(0, 0);
      for(MD_EachNode(channel_cfg, child->first))
      {
        temp_end(scratch);
        U32 slot = 0;
        if(0){}
        else if(str8_match(channel_cfg->string, str8_lit("r"), StringMatchFlag_CaseInsensitive)) { slot = 0; }
        else if(str8_match(channel_cfg->string, str8_lit("g"), StringMatchFlag_CaseInsensitive)) { slot = 1; }
        else if(str8_match(channel_cfg->string, str8_lit("b"), StringMatchFlag_CaseInsensitive)) { slot = 2; }
        else if(str8_match(channel_cfg->string, str8_lit("a"), StringMatchFlag_CaseInsensitive)) { slot = 3; }
        String8 expr_string = md_string_from_children(scratch.arena, channel_cfg);
        E_Eval eval = e_eval_from_string(scratch.arena, expr_string);
        E_Eval value_eval = e_value_eval_from_eval(eval);
        result.v[slot] = e_f32_from_eval(value_eval);
      }
      scratch_end(scratch);
    }
  }
  return result;
}

internal Axis2
df_split_axis_from_panel_cfg(MD_Node *panel)
{
  Axis2 root_axis = Axis2_X;
  S32 num_panel_ancestors = 0;
  for(MD_Node *n = panel->parent; !md_node_is_nil(n); n = n->parent)
  {
    if(str8_match(n->string, str8_lit("window"), 0))
    {
      root_axis = md_node_is_nil(md_child_from_string(n, str8_lit("split_x"), 0)) ? Axis2_Y : Axis2_X;
      break;
    }
    num_panel_ancestors += 1;
  }
  Axis2 split_axis = (num_panel_ancestors & 1) ? axis2_flip(root_axis) : root_axis;
  return split_axis;
}

internal Rng2F32
df_target_rect_from_panel_child_cfg(Rng2F32 parent_rect, Axis2 parent_split_axis, MD_Node *panel)
{
  Rng2F32 rect = parent_rect;
  MD_Node *parent = panel->parent;
  if(!md_node_is_nil(parent))
  {
    Vec2F32 parent_rect_size = dim_2f32(parent_rect);
    Axis2 axis = parent_split_axis;
    rect.p1.v[axis] = rect.p0.v[axis];
    for(MD_Node *panel_child = md_node_from_chain_flags(panel->first, &md_nil_node, MD_NodeFlag_Numeric);
        !md_node_is_nil(panel_child);
        panel_child = md_node_from_chain_flags(panel_child->next, &md_nil_node, MD_NodeFlag_Numeric))
    {
      F32 pct_of_parent = f32_from_str8(panel_child->string);
      rect.p1.v[axis] += parent_rect_size.v[axis] * pct_of_parent;
      if(panel_child == panel)
      {
        break;
      }
      rect.p0.v[axis] = rect.p1.v[axis];
    }
  }
  rect.x0 = round_f32(rect.x0);
  rect.x1 = round_f32(rect.x1);
  rect.y0 = round_f32(rect.y0);
  rect.y1 = round_f32(rect.y1);
  return rect;
}

internal Rng2F32
df_target_rect_from_panel_cfg(Rng2F32 root_rect, MD_Node *panel)
{
  Temp scratch = scratch_begin(0, 0);
  
  // rjf: count ancestors
  MD_Node *root_panel_cfg = &md_nil_node;
  U64 ancestor_count = 0;
  for(MD_Node *p = panel->parent;
      !md_node_is_nil(p);
      p = p->parent)
  {
    if(!str8_match(p->string, str8_lit("window"), 0))
    {
      root_panel_cfg = md_child_from_string(p, str8_lit("panels"), 0);
      break;
    }
    ancestor_count += 1;
  }
  
  // rjf: gather ancestors
  MD_Node **ancestors = push_array(scratch.arena, MD_Node *, ancestor_count);
  {
    U64 ancestor_idx = 0;
    for(MD_Node *p = panel->parent;
        !md_node_is_nil(p) && p != root_panel_cfg->parent;
        p = p->parent)
    {
      ancestors[ancestor_idx] = p;
      ancestor_idx += 1;
    }
  }
  
  // rjf: go from highest ancestor => panel and calculate rect
  Rng2F32 parent_rect = root_rect;
  Axis2 split_axis = df_split_axis_from_panel_cfg(root_panel_cfg);
  for(S64 ancestor_idx = (S64)ancestor_count-1;
      0 <= ancestor_idx && ancestor_idx < ancestor_count;
      ancestor_idx -= 1)
  {
    MD_Node *ancestor = ancestors[ancestor_idx];
    MD_Node *parent = ancestor->parent;
    if(!md_node_is_nil(parent) && parent != root_panel_cfg->parent)
    {
      parent_rect = df_target_rect_from_panel_child_cfg(parent_rect, split_axis, ancestor);
      split_axis = axis2_flip(split_axis);
    }
  }
  
  // rjf: calculate final rect
  Rng2F32 rect = df_target_rect_from_panel_child_cfg(parent_rect, split_axis, panel);
  
  scratch_end(scratch);
  return rect;
}

internal B32
df_tab_cfg_is_project_filtered(MD_Node *cfg)
{
  Temp scratch = scratch_begin(0, 0);
  MD_Node *project_tag = md_tag_from_string(cfg, str8_lit("project"), 0);
  String8 required_project = raw_from_escaped_string(scratch.arena, project_tag->first->string);
  String8 current_project = df_state->cfg_slot_roots[DF_CfgSlot_Project]->string;
  B32 result = path_match_normalized(required_project, current_project);
  scratch_end(scratch);
  return result;
}

//- rjf: keybindings

internal void
df_clear_bindings(void)
{
  arena_clear(df_state->key_map_arena);
  df_state->key_map_table_size = 1024;
  df_state->key_map_table = push_array(df_state->key_map_arena, DF_KeyMapSlot, df_state->key_map_table_size);
  df_state->key_map_total_count = 0;
}

internal DF_BindingList
df_bindings_from_spec(Arena *arena, D_CmdSpec *spec)
{
  DF_BindingList result = {0};
  U64 hash = d_hash_from_string(spec->info.string);
  U64 slot = hash%df_state->key_map_table_size;
  for(DF_KeyMapNode *n = df_state->key_map_table[slot].first; n != 0; n = n->hash_next)
  {
    if(n->spec == spec)
    {
      DF_BindingNode *node = push_array(arena, DF_BindingNode, 1);
      node->binding = n->binding;
      SLLQueuePush(result.first, result.last, node);
      result.count += 1;
    }
  }
  return result;
}

internal void
df_bind_spec(D_CmdSpec *spec, DF_Binding binding)
{
  if(binding.key != OS_Key_Null)
  {
    U64 hash = d_hash_from_string(spec->info.string);
    U64 slot = hash%df_state->key_map_table_size;
    DF_KeyMapNode *existing_node = 0;
    for(DF_KeyMapNode *n = df_state->key_map_table[slot].first; n != 0; n = n->hash_next)
    {
      if(n->spec == spec && n->binding.key == binding.key && n->binding.flags == binding.flags)
      {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0)
    {
      DF_KeyMapNode *n = df_state->free_key_map_node;
      if(n == 0)
      {
        n = push_array(df_state->arena, DF_KeyMapNode, 1);
      }
      else
      {
        df_state->free_key_map_node = df_state->free_key_map_node->hash_next;
      }
      n->spec = spec;
      n->binding = binding;
      DLLPushBack_NP(df_state->key_map_table[slot].first, df_state->key_map_table[slot].last, n, hash_next, hash_prev);
      df_state->key_map_total_count += 1;
    }
  }
}

internal void
df_unbind_spec(D_CmdSpec *spec, DF_Binding binding)
{
  U64 hash = d_hash_from_string(spec->info.string);
  U64 slot = hash%df_state->key_map_table_size;
  for(DF_KeyMapNode *n = df_state->key_map_table[slot].first, *next = 0; n != 0; n = next)
  {
    next = n->hash_next;
    if(n->spec == spec && n->binding.key == binding.key && n->binding.flags == binding.flags)
    {
      DLLRemove_NP(df_state->key_map_table[slot].first, df_state->key_map_table[slot].last, n, hash_next, hash_prev);
      n->hash_next = df_state->free_key_map_node;
      df_state->free_key_map_node = n;
      df_state->key_map_total_count -= 1;
    }
  }
}

internal D_CmdSpecList
df_cmd_spec_list_from_binding(Arena *arena, DF_Binding binding)
{
  D_CmdSpecList result = {0};
  for(U64 idx = 0; idx < df_state->key_map_table_size; idx += 1)
  {
    for(DF_KeyMapNode *n = df_state->key_map_table[idx].first; n != 0; n = n->hash_next)
    {
      if(n->binding.key == binding.key && n->binding.flags == binding.flags)
      {
        d_cmd_spec_list_push(arena, &result, n->spec);
      }
    }
  }
  return result;
}

internal D_CmdSpecList
df_cmd_spec_list_from_event_flags(Arena *arena, OS_EventFlags flags)
{
  D_CmdSpecList result = {0};
  for(U64 idx = 0; idx < df_state->key_map_table_size; idx += 1)
  {
    for(DF_KeyMapNode *n = df_state->key_map_table[idx].first; n != 0; n = n->hash_next)
    {
      if(n->binding.flags == flags)
      {
        d_cmd_spec_list_push(arena, &result, n->spec);
      }
    }
  }
  return result;
}

//- rjf: colors

internal Vec4F32
df_rgba_from_theme_color(DF_ThemeColor color)
{
  return df_state->cfg_theme.colors[color];
}

internal DF_ThemeColor
df_theme_color_from_txt_token_kind(TXT_TokenKind kind)
{
  DF_ThemeColor color = DF_ThemeColor_CodeDefault;
  switch(kind)
  {
    default:break;
    case TXT_TokenKind_Keyword:{color = DF_ThemeColor_CodeKeyword;}break;
    case TXT_TokenKind_Numeric:{color = DF_ThemeColor_CodeNumeric;}break;
    case TXT_TokenKind_String: {color = DF_ThemeColor_CodeString;}break;
    case TXT_TokenKind_Meta:   {color = DF_ThemeColor_CodeMeta;}break;
    case TXT_TokenKind_Comment:{color = DF_ThemeColor_CodeComment;}break;
    case TXT_TokenKind_Symbol: {color = DF_ThemeColor_CodeDelimiterOperator;}break;
  }
  return color;
}

//- rjf: code -> palette

internal UI_Palette *
df_palette_from_code(DF_PaletteCode code)
{
  DF_Window *window = df_window_from_handle(d_regs()->window);
  UI_Palette *result = &window->cfg_palettes[code];
  return result;
}

//- rjf: fonts/sizes

internal FNT_Tag
df_font_from_slot(DF_FontSlot slot)
{
  FNT_Tag result = df_state->cfg_font_tags[slot];
  return result;
}

internal F32
df_font_size_from_slot(DF_FontSlot slot)
{
  F32 result = 0;
  DF_Window *ws = df_window_from_handle(d_regs()->window);
  F32 dpi = os_dpi_from_window(ws->os);
  if(dpi != ws->last_dpi)
  {
    F32 old_dpi = ws->last_dpi;
    F32 new_dpi = dpi;
    ws->last_dpi = dpi;
    S32 *pt_sizes[] =
    {
      &ws->setting_vals[DF_SettingCode_MainFontSize].s32,
      &ws->setting_vals[DF_SettingCode_CodeFontSize].s32,
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
    case DF_FontSlot_Code:
    {
      result = (F32)ws->setting_vals[DF_SettingCode_CodeFontSize].s32;
    }break;
    default:
    case DF_FontSlot_Main:
    case DF_FontSlot_Icons:
    {
      result = (F32)ws->setting_vals[DF_SettingCode_MainFontSize].s32;
    }break;
  }
  return result;
}

internal FNT_RasterFlags
df_raster_flags_from_slot(DF_FontSlot slot)
{
  FNT_RasterFlags flags = FNT_RasterFlag_Smooth|FNT_RasterFlag_Hinted;
  switch(slot)
  {
    default:{}break;
    case DF_FontSlot_Icons:{flags = FNT_RasterFlag_Smooth;}break;
    case DF_FontSlot_Main: {flags = (!!df_setting_val_from_code(DF_SettingCode_SmoothUIText).s32*FNT_RasterFlag_Smooth)|(!!df_setting_val_from_code(DF_SettingCode_HintUIText).s32*FNT_RasterFlag_Hinted);}break;
    case DF_FontSlot_Code: {flags = (!!df_setting_val_from_code(DF_SettingCode_SmoothCodeText).s32*FNT_RasterFlag_Smooth)|(!!df_setting_val_from_code(DF_SettingCode_HintCodeText).s32*FNT_RasterFlag_Hinted);}break;
  }
  return flags;
}

//- rjf: settings

internal DF_SettingVal
df_setting_val_from_code(DF_SettingCode code)
{
  DF_Window *window = df_window_from_handle(d_regs()->window);
  DF_SettingVal result = {0};
  if(window != 0)
  {
    result = window->setting_vals[code];
  }
  if(result.set == 0)
  {
    for(EachEnumVal(D_CfgSrc, src))
    {
      if(df_state->cfg_setting_vals[src][code].set)
      {
        result = df_state->cfg_setting_vals[src][code];
        break;
      }
    }
  }
  return result;
}

//- rjf: config serialization

#if 0 // TODO(rjf): @msgs
internal int
df_qsort_compare__cfg_string_bindings(DF_StringBindingPair *a, DF_StringBindingPair *b)
{
  return strncmp((char *)a->string.str, (char *)b->string.str, Min(a->string.size, b->string.size));
}

internal String8List
df_cfg_strings_from_state(Arena *arena, String8 root_path, D_CfgSrc source)
{
  ProfBeginFunction();
  String8List strs = {0};
  
  //- rjf: serialize windows
  {
    B32 first = 1;
    for(DF_Window *window = df_state->first_window; window != 0; window = window->next)
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
      DF_Panel *root_panel = window->root_panel;
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
      for(EachEnumVal(DF_SettingCode, code))
      {
        DF_SettingVal current = window->setting_vals[code];
        if(current.set)
        {
          str8_list_pushf(arena, &strs, "  %S: %i\n", df_g_setting_code_lower_string_table[code], current.s32);
        }
      }
      {
        DF_PanelRec rec = {0};
        S32 indentation = 2;
        String8 indent_str = str8_lit("                                                                                                   ");
        str8_list_pushf(arena, &strs, "  panels:\n");
        str8_list_pushf(arena, &strs, "  {\n");
        for(DF_Panel *p = root_panel; !df_panel_is_nil(p); p = rec.next)
        {
          // rjf: get recursion
          rec = df_panel_rec_df_pre(p);
          
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
          for(DF_View *view = p->first_tab_view; !df_view_is_nil(view); view = view->order_next)
          {
            String8 view_string = view->spec->info.name;
            
            // rjf: serialize views which can be serialized
            if(view->spec->info.flags & DF_ViewSpecFlag_CanSerialize)
            {
              str8_list_pushf(arena, &strs, "%.*s", indentation*2, indent_str.str);
              
              // rjf: serialize view string
              str8_list_push(arena, &strs, view_string);
              
              // rjf: serialize view parameterizations
              str8_list_push(arena, &strs, str8_lit(": {"));
              if(view == df_selected_tab_from_panel(p))
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
                  String8 query_file_path = d_file_path_from_eval_string(scratch.arena, query_raw);
                  if(query_file_path.size != 0)
                  {
                    query_file_path = path_relative_dst_from_absolute_dst_src(scratch.arena, query_file_path, root_path);
                    query_raw = push_str8f(scratch.arena, "file:\"%S\"", query_file_path);
                  }
                }
                String8 query_sanitized = escaped_from_raw_string(scratch.arena, query_raw);
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
            if(pop_idx == rec.pop_count-1 && rec.next == &df_nil_panel)
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
  if(source == D_CfgSrc_User)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 indent_str = str8_lit("                                                                                                             ");
    U64 string_binding_pair_count = 0;
    DF_StringBindingPair *string_binding_pairs = push_array(scratch.arena, DF_StringBindingPair, df_state->key_map_total_count);
    for(U64 idx = 0;
        idx < df_state->key_map_table_size && string_binding_pair_count < df_state->key_map_total_count;
        idx += 1)
    {
      for(DF_KeyMapNode *n = df_state->key_map_table[idx].first;
          n != 0 && string_binding_pair_count < df_state->key_map_total_count;
          n = n->hash_next)
      {
        DF_StringBindingPair *pair = string_binding_pairs + string_binding_pair_count;
        pair->string = n->spec->info.string;
        pair->binding = n->binding;
        string_binding_pair_count += 1;
      }
    }
    quick_sort(string_binding_pairs, string_binding_pair_count, sizeof(DF_StringBindingPair), df_qsort_compare__cfg_string_bindings);
    if(string_binding_pair_count != 0)
    {
      str8_list_push(arena, &strs, str8_lit("/// keybindings ///////////////////////////////////////////////////////////////\n"));
      str8_list_push(arena, &strs, str8_lit("\n"));
      str8_list_push(arena, &strs, str8_lit("keybindings:\n"));
      str8_list_push(arena, &strs, str8_lit("{\n"));
      for(U64 idx = 0; idx < string_binding_pair_count; idx += 1)
      {
        DF_StringBindingPair *pair = string_binding_pairs + idx;
        String8List event_flags_strings = os_string_list_from_event_flags(scratch.arena, pair->binding.flags);
        StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
        String8 event_flags_string = str8_list_join(scratch.arena, &event_flags_strings, &join);
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
  if(source == D_CfgSrc_User)
  {
    // rjf: determine if this theme matches an existing preset
    B32 is_preset = 0;
    DF_ThemePreset matching_preset = DF_ThemePreset_DefaultDark;
    {
      for(DF_ThemePreset p = (DF_ThemePreset)0; p < DF_ThemePreset_COUNT; p = (DF_ThemePreset)(p+1))
      {
        B32 matches_this_preset = 1;
        for(DF_ThemeColor c = (DF_ThemeColor)(DF_ThemeColor_Null+1); c < DF_ThemeColor_COUNT; c = (DF_ThemeColor)(c+1))
        {
          if(!MemoryMatchStruct(&df_state->cfg_theme_target.colors[c], &df_g_theme_preset_colors_table[p][c]))
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
      str8_list_pushf(arena, &strs, "color_preset: \"%S\"\n\n", df_g_theme_preset_code_string_table[matching_preset]);
    }
    
    // rjf: serialize non-preset theme
    if(!is_preset)
    {
      str8_list_push(arena, &strs, str8_lit("colors:\n"));
      str8_list_push(arena, &strs, str8_lit("{\n"));
      for(DF_ThemeColor color = (DF_ThemeColor)(DF_ThemeColor_Null+1);
          color < DF_ThemeColor_COUNT;
          color = (DF_ThemeColor)(color+1))
      {
        String8 color_name = df_g_theme_color_cfg_string_table[color];
        Vec4F32 color_rgba = df_state->cfg_theme_target.colors[color];
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
  if(source == D_CfgSrc_User)
  {
    String8 code_font_path_escaped = d_cfg_escaped_from_raw_string(arena, df_state->cfg_code_font_path);
    String8 main_font_path_escaped = d_cfg_escaped_from_raw_string(arena, df_state->cfg_main_font_path);
    str8_list_push(arena, &strs, str8_lit("/// fonts /////////////////////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    str8_list_pushf(arena, &strs, "code_font: \"%S\"\n", code_font_path_escaped);
    str8_list_pushf(arena, &strs, "main_font: \"%S\"\n", main_font_path_escaped);
    str8_list_push(arena, &strs, str8_lit("\n"));
  }
  
  //- rjf: serialize global settings
  {
    B32 first = 1;
    for(EachEnumVal(DF_SettingCode, code))
    {
      if(df_g_setting_code_default_is_per_window_table[code])
      {
        continue;
      }
      DF_SettingVal current = df_state->cfg_setting_vals[source][code];
      if(current.set)
      {
        if(first)
        {
          first = 0;
          str8_list_push(arena, &strs, str8_lit("/// global settings ///////////////////////////////////////////////////////////\n"));
          str8_list_push(arena, &strs, str8_lit("\n"));
        }
        str8_list_pushf(arena, &strs, "%S: %i\n", df_g_setting_code_lower_string_table[code], current.s32);
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
#endif

////////////////////////////////
//~ rjf: Process Control Info Stringification

internal String8
df_string_from_exception_code(U32 code)
{
  String8 string = {0};
  for(EachNonZeroEnumVal(CTRL_ExceptionCodeKind, k))
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
df_stop_explanation_string_icon_from_ctrl_event(Arena *arena, CTRL_Event *event, DF_IconKind *icon_out)
{
  DF_IconKind icon = DF_IconKind_Null;
  String8 explanation = {0};
  Temp scratch = scratch_begin(&arena, 1);
  D_Entity *thread = d_entity_from_ctrl_handle(event->machine_id, event->entity);
  String8 thread_display_string = d_display_string_from_entity(scratch.arena, thread);
  String8 process_thread_string = thread_display_string;
  D_Entity *process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
  if(process->kind == D_EntityKind_Process)
  {
    String8 process_display_string = d_display_string_from_entity(scratch.arena, process);
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
          if(!d_entity_is_nil(thread))
          {
            explanation = push_str8f(arena, "%S completed step", process_thread_string);
          }
          else
          {
            explanation = str8_lit("Stopped");
          }
        }break;
        case CTRL_EventCause_UserBreakpoint:
        {
          if(!d_entity_is_nil(thread))
          {
            icon = DF_IconKind_CircleFilled;
            explanation = push_str8f(arena, "%S hit a breakpoint", process_thread_string);
          }
        }break;
        case CTRL_EventCause_InterruptedByException:
        {
          if(!d_entity_is_nil(thread))
          {
            icon = DF_IconKind_WarningBig;
            switch(event->exception_kind)
            {
              default:
              {
                String8 exception_code_string = df_string_from_exception_code(event->exception_code);
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
            icon = DF_IconKind_Pause;
            explanation = str8_lit("Interrupted");
          }
        }break;
        case CTRL_EventCause_InterruptedByTrap:
        {
          icon = DF_IconKind_WarningBig;
          explanation = push_str8f(arena, "%S interrupted by trap - 0x%x", process_thread_string, event->exception_code);
        }break;
        case CTRL_EventCause_InterruptedByHalt:
        {
          icon = DF_IconKind_Pause;
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
df_request_frame(void)
{
  df_state->num_frames_requested = 4;
}

////////////////////////////////
//~ rjf: Registers Functions

internal DF_Regs *
df_regs(void)
{
  DF_Regs *regs = &df_state->top_regs->v;
  return regs;
}

internal DF_Regs *
df_base_regs(void)
{
  DF_Regs *regs = &df_state->base_regs.v;
  return regs;
}

internal DF_Regs *
df_push_regs_(DF_Regs *regs)
{
  DF_RegsNode *n = push_array(df_frame_arena(), DF_RegsNode, 1);
  MemoryCopyStruct(&n->v, regs);
  SLLStackPush(df_state->top_regs, n);
  return &n->v;
}

internal DF_Regs *
df_pop_regs(void)
{
  DF_Regs *regs = &df_state->top_regs->v;
  SLLStackPop(df_state->top_regs);
  if(df_state->top_regs == 0)
  {
    df_state->top_regs = &df_state->base_regs;
  }
  return regs;
}

////////////////////////////////
//~ rjf: Message Functions

internal DF_MsgKind
df_msg_kind_from_string(String8 string)
{
  DF_MsgKind result = DF_MsgKind_Null;
  for(EachNonZeroEnumVal(DF_MsgKind, k))
  {
    if(str8_match(string, df_msg_kind_info_table[k].name_lower, 0))
    {
      result = k;
      break;
    }
  }
  return result;
}

internal void
df_regs_set_window(MD_Node *cfg_tree)
{
  
}

internal void
df_regs_set_panel(MD_Node *cfg_tree)
{
  
}

internal void
df_regs_set_view(MD_Node *cfg_tree)
{
  
}

internal void
df_regs_set_from_query_slot_string(D_RegSlot slot, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  switch(slot)
  {
    default:
    case D_RegSlot_String:
    {
      df_regs()->string = push_str8_copy(d_frame_arena(), string);
    }break;
    case D_RegSlot_FilePath:
    {
      String8TxtPtPair pair = str8_txt_pt_pair_from_string(string);
      df_regs()->file_path = push_str8_copy(d_frame_arena(), pair.string);
      df_regs()->cursor = pair.pt;
    }break;
    case D_CmdParamSlot_TextPoint:
    {
      E_Eval eval = e_eval_from_string(scratch.arena, string);
      E_Eval value_eval = e_value_eval_from_eval(eval);
      df_regs()->cursor.column = 1;
      df_regs()->cursor.line   = Min(1, value_eval.value.s64);
    }break;
    case D_CmdParamSlot_VirtualAddr: goto use_numeric_eval;
    case D_CmdParamSlot_VirtualOff: goto use_numeric_eval;
    case D_CmdParamSlot_Index: goto use_numeric_eval;
    case D_CmdParamSlot_ID: goto use_numeric_eval;
    use_numeric_eval:
    {
      E_Eval eval = e_eval_from_string(scratch.arena, string);
      E_Eval value_eval = e_value_eval_from_eval(eval);
      switch(slot)
      {
        default:{}break;
        case D_RegSlot_VaddrRange:
        {
          df_regs()->vaddr_range = r1u64(value_eval.value.u64, value_eval.value.u64);
        }break;
        case D_RegSlot_VoffRange:
        {
          df_regs()->voff_range = r1u64(value_eval.value.u64, value_eval.value.u64);
        }break;
        case D_RegSlot_PID:
        {
          df_regs()->pid = value_eval.value.u64;
        }break;
        case D_RegSlot_UnwindCount:
        {
          df_regs()->unwind_count = value_eval.value.u64;
        }break;
        case D_RegSlot_InlineDepth:
        {
          df_regs()->inline_depth = value_eval.value.u64;
        }break;
      }
    }break;
  }
  scratch_end(scratch);
}

internal void
df_msg_(DF_MsgKind kind, DF_Regs *regs)
{
  DF_MsgNode *n = push_array(df_state->msgs_arena, DF_MsgNode, 1);
  SLLQueuePush(df_state->msgs.first, df_state->msgs.last, n);
  df_state->msgs.count += 1;
  n->v.kind = kind;
  n->v.regs = df_regs_copy(df_state->msgs_arena, regs);
}

//- rjf: message iteration

internal B32
df_next_msg(DF_Msg **msg)
{
  DF_MsgNode *start_node = df_state->msgs.first;
  if(msg[0] != 0)
  {
    start_node = CastFromMember(DF_MsgNode, v, msg[0]);
    start_node = start_node->next;
  }
  msg[0] = 0;
  if(start_node != 0)
  {
    msg[0] = &start_node->v;
  }
  return !!msg[0];
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
df_init(CmdLine *cmdln)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  df_state = push_array(arena, DF_State, 1);
  df_state->arena = arena;
  df_state->log = log_alloc();
  df_state->num_frames_requested = 2;
  df_state->cfg_arena = arena_alloc();
  df_state->cfg_free = &md_nil_node;
  for(EachEnumVal(DF_CfgSlot, slot))
  {
    df_state->cfg_slot_roots[slot] = &md_nil_node;
  }
  df_state->msgs_arena = arena_alloc();
  df_state->window_slots_count = 256;
  df_state->window_slots = push_array(arena, DF_WindowSlot, df_state->window_slots_count);
  df_state->key_map_arena = arena_alloc();
  df_state->confirm_arena = arena_alloc();
  df_state->view_spec_table_size = 256;
  df_state->view_spec_table = push_array(arena, DF_ViewSpec *, df_state->view_spec_table_size);
  df_state->view_rule_spec_table_size = 1024;
  df_state->view_rule_spec_table = push_array(arena, DF_ViewRuleSpec *, d_state->view_rule_spec_table_size);
  df_state->code_ctx_menu_key   = ui_key_from_string(ui_key_zero(), str8_lit("_code_ctx_menu_"));
  df_state->entity_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_entity_ctx_menu_"));
  df_state->tab_ctx_menu_key    = ui_key_from_string(ui_key_zero(), str8_lit("_tab_ctx_menu_"));
  df_state->error_arena = arena_alloc();
  df_state->string_search_arena = arena_alloc();
  df_state->cfg_main_font_path_arena = arena_alloc();
  df_state->cfg_code_font_path_arena = arena_alloc();
  df_state->rich_hover_info_next_arena = arena_alloc();
  df_state->rich_hover_info_current_arena = arena_alloc();
  df_clear_bindings();
  
  // rjf: do initial config load
  {
    Temp scratch = scratch_begin(0, 0);
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
    df_msg(DF_MsgKind_LoadUser, .file_path = user_cfg_path);
    df_msg(DF_MsgKind_LoadProject, .file_path = project_cfg_path);
    scratch_end(scratch);
  }
  
  // rjf: open ui thread log file
  {
    Temp scratch = scratch_begin(0, 0);
    String8 user_program_data_path = os_get_process_info()->user_program_data_path;
    String8 user_data_folder = push_str8f(scratch.arena, "%S/raddbg/logs", user_program_data_path);
    df_state->log_path = push_str8f(df_state->arena, "%S/ui_thread.raddbg_log", user_data_folder);
    os_make_directory(user_data_folder);
    os_write_data_to_file_path(df_state->log_path, str8_zero());
    scratch_end(scratch);
  }
  
  // rjf: register gfx layer views
  {
    DF_ViewSpecInfoArray array = {df_g_gfx_view_kind_spec_info_table, ArrayCount(df_g_gfx_view_kind_spec_info_table)};
    df_register_view_specs(array);
  }
  
  // rjf: register gfx layer view rules
  {
    DF_ViewRuleSpecInfoArray array = {df_g_gfx_view_rule_spec_info_table, ArrayCount(df_g_gfx_view_rule_spec_info_table)};
    df_register_view_rule_specs(array);
  }
  
  // rjf: register cmd param slot -> view specs
  {
    for(U64 idx = 0; idx < ArrayCount(df_g_cmd_param_slot_2_view_spec_src_map); idx += 1)
    {
      D_CmdParamSlot slot = df_g_cmd_param_slot_2_view_spec_src_map[idx];
      String8 view_spec_name = df_g_cmd_param_slot_2_view_spec_dst_map[idx];
      String8 cmd_spec_name = df_g_cmd_param_slot_2_view_spec_cmd_map[idx];
      DF_ViewSpec *view_spec = df_view_spec_from_string(view_spec_name);
      D_CmdSpec *cmd_spec = cmd_spec_name.size != 0 ? d_cmd_spec_from_string(cmd_spec_name) : &d_nil_cmd_spec;
      D_CmdParamSlotViewSpecRuleNode *n = push_array(df_state->arena, D_CmdParamSlotViewSpecRuleNode, 1);
      n->view_spec = view_spec;
      n->cmd_spec = cmd_spec;
      SLLQueuePush(df_state->cmd_param_slot_view_spec_table[slot].first, df_state->cmd_param_slot_view_spec_table[slot].last, n);
      df_state->cmd_param_slot_view_spec_table[slot].count += 1;
    }
  }
  
  // rjf: unpack icon image data
  {
    Temp scratch = scratch_begin(0, 0);
    String8 data = df_g_icon_file_bytes;
    U8 *ptr = data.str;
    U8 *opl = ptr+data.size;
    
    // rjf: read header
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
    df_state->icon_texture = r_tex2d_alloc(R_ResourceKind_Static, image_dim, R_Tex2DFormat_RGBA8, image_data);
    
    // rjf: release
    stbi_image_free(image_data);
    scratch_end(scratch);
  }
  
  ProfEnd();
}

internal void
df_frame(void)
{
  Temp scratch = scratch_begin(0, 0);
  DI_Scope *di_scope = di_scope_open();
  arena_clear(df_frame_arena());
  df_state->top_regs = &df_state->base_regs;
  df_regs_copy_contents(df_frame_arena(), &df_state->top_regs->v, &df_state->top_regs->v);
  local_persist S32 depth = 0;
  
  //////////////////////////////
  //- rjf: begin logging
  //
  log_select(df_state->log);
  log_scope_begin();
  
  //////////////////////////////
  //- rjf: get events from the OS
  //
  OS_EventList events = {0};
  if(depth == 0) DeferLoop(depth += 1, depth -= 1)
  {
    events = os_get_events(scratch.arena, df_state->num_frames_requested == 0);
  }
  
  //////////////////////////////
  //- rjf: apply new rich hover info
  //
  arena_clear(df_state->rich_hover_info_current_arena);
  MemoryCopyStruct(&df_state->rich_hover_info_current, &df_state->rich_hover_info_next);
  df_state->rich_hover_info_current.dbgi_key = di_key_copy(df_state->rich_hover_info_current_arena, &df_state->rich_hover_info_current.dbgi_key);
  arena_clear(df_state->rich_hover_info_next_arena);
  MemoryZeroStruct(&df_state->rich_hover_info_next);
  
  //////////////////////////////
  //- rjf: pick target hz
  //
  // TODO(rjf): maximize target, given all windows and their monitors
  F32 target_hz = os_get_gfx_info()->default_refresh_rate;
  if(df_state->frame_index > 32)
  {
    // rjf: calculate average frame time out of the last N
    U64 num_frames_in_history = Min(ArrayCount(df_state->frame_time_us_history), df_state->frame_index);
    U64 frame_time_history_sum_us = 0;
    for(U64 idx = 0; idx < num_frames_in_history; idx += 1)
    {
      frame_time_history_sum_us += df_state->frame_time_us_history[idx];
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
  F32 dt = 1.f/target_hz;
  
  //////////////////////////////
  //- rjf: begin measuring actual per-frame work
  //
  U64 begin_time_us = os_now_microseconds();
  
  //////////////////////////////
  //- rjf: consume OS events for bind change
  //
  if(!df_state->confirm_active && df_state->bind_change_active)
  {
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Esc))
    {
      df_request_frame();
      df_state->bind_change_active = 0;
    }
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Delete))
    {
      df_request_frame();
      df_msg(DF_MsgKind_RemoveEntity, .cfg_tree = df_state->bind_change_bind_handle);
      df_state->bind_change_active = 0;
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
        MD_Node *current_bind_cfg = df_cfg_tree_from_handle(df_state->bind_change_bind_handle);
        MD_Node *bindings_root_cfg = current_bind_cfg->parent;
        if(md_node_is_nil(bindings_root_cfg))
        {
          bindings_root_cfg = md_child_from_string(df_state->cfg_slot_roots[DF_CfgSlot_User], str8_lit("keybindings"), 0);
          if(md_node_is_nil(bindings_root_cfg))
          {
            bindings_root_cfg = df_single_cfg_tree_from_string(str8_lit("keybindings"));
            md_node_insert_child(df_state->cfg_slot_roots[DF_CfgSlot_User], df_state->cfg_slot_roots[DF_CfgSlot_User]->last, bindings_root_cfg);
          }
        }
        MD_Node *prev_bind_cfg = current_bind_cfg->prev;
        String8List modifiers_strings = os_string_list_from_event_flags(scratch.arena, event->flags);
        String8 modifiers_string = str8_list_join(scratch.arena, &modifiers_strings, &(StringJoin){.sep = str8_lit(" ")});
        String8 key_string = os_g_key_cfg_string_table[event->key];
        MD_Node *new_bind_cfg = df_single_cfg_tree_from_stringf("{\"%S\" %S %S}", df_state->bind_change_msg_name, modifiers_string, key_string);
        md_node_insert_child(bindings_root_cfg, prev_bind_cfg, new_bind_cfg);
#if 0 // NOTE(rjf): @msgs
        DF_Binding binding = zero_struct;
        {
          binding.key = event->key;
          binding.flags = event->flags;
        }
        df_unbind_spec(df_state->bind_change_cmd_spec, df_state->bind_change_binding);
        df_bind_spec(df_state->bind_change_cmd_spec, binding);
#endif
        U32 codepoint = os_codepoint_from_event_flags_and_key(event->flags, event->key);
        os_text(&events, os_handle_zero(), codepoint);
        os_eat_event(&events, event);
        df_request_frame();
        df_state->bind_change_active = 0;
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: consume OS events
  //
  {
    for(OS_Event *event = events.first, *next = 0;
        event != 0;
        event = next)
    {
      next = event->next;
      DF_Window *window = df_window_from_os_handle(event->window);
      MD_Node *window_cfg = df_cfg_tree_from_handle(window->handle);
      D_RegsScope
      {
        df_regs_set_window(window_cfg);
        B32 take = 0;
        
        //- rjf: try window close
        if(!take && event->kind == OS_EventKind_WindowClose && window != 0)
        {
          take = 1;
          df_msg(DF_MsgKind_CloseWindow);
        }
        
        //- rjf: try menu bar operations
        {
          if(!take && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
          {
            take = 1;
            df_request_frame();
            window->menu_bar_focused_on_press = window->menu_bar_focused;
            window->menu_bar_key_held = 1;
            window->menu_bar_focus_press_started = 1;
          }
          if(!take && event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
          {
            take = 1;
            df_request_frame();
            window->menu_bar_key_held = 0;
          }
          if(window->menu_bar_focused && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->flags == 0 && event->is_repeat == 0)
          {
            take = 1;
            df_request_frame();
            window->menu_bar_focused = 0;
          }
          else if(window->menu_bar_focus_press_started && !window->menu_bar_focused && event->kind == OS_EventKind_Release && event->flags == 0 && event->key == OS_Key_Alt && event->is_repeat == 0)
          {
            take = 1;
            df_request_frame();
            window->menu_bar_focused = !window->menu_bar_focused_on_press;
            window->menu_bar_focus_press_started = 0;
          }
          else if(event->kind == OS_EventKind_Press && event->key == OS_Key_Esc && window->menu_bar_focused && !ui_any_ctx_menu_is_open())
          {
            take = 1;
            df_request_frame();
            window->menu_bar_focused = 0;
          }
        }
        
        //- rjf: try hotkey presses
        if(!take && event->kind == OS_EventKind_Press)
        {
          // rjf: look into keymap; push run-command message to frontend if this event
          // matches a binding
          B32 taken_by_keybinding = 0;
          for(EachEnumVal(DF_CfgSlot, slot))
          {
            for(MD_EachNode(tln, df_state->cfg_slot_roots[slot]->first))
            {
              if(str8_match(tln->string, str8_lit("keybindings"), StringMatchFlag_CaseInsensitive))
              {
                for(MD_EachNode(map, tln->first))
                {
                  OS_Key map_key = OS_Key_Null;
                  OS_EventFlags map_flags = 0;
                  String8 map_msg_name = map->first->string;
                  for(MD_EachNode(child, map->first->next))
                  {
                    if(0){}
                    else if(str8_match(child->string, str8_lit("ctrl"), 0))  { map_flags |= OS_EventFlag_Ctrl; }
                    else if(str8_match(child->string, str8_lit("shift"), 0)) { map_flags |= OS_EventFlag_Shift; }
                    else if(str8_match(child->string, str8_lit("alt"), 0))   { map_flags |= OS_EventFlag_Alt; }
                    else
                    {
                      OS_Key key = os_key_from_string(child->string);
                      if(key != OS_Key_Null)
                      {
                        map_key = key;
                      }
                    }
                  }
                  if(map_key == event->key && map_flags == event->flags)
                  {
                    take = 1;
                    taken_by_keybinding = 1;
                    df_msg(DF_MsgKind_RunCommand, .string = map_msg_name);
                  }
                }
              }
            }
          }
          
          // rjf: not taken by keybinding -> try alternatives
          if(!taken_by_keybinding && (OS_Key_F1 <= event->key && event->key <= OS_Key_F19))
          {
            window->menu_bar_focus_press_started = 0;
          }
          
          df_request_frame();
        }
        
        //- rjf: try text events
        if(!take && event->kind == OS_EventKind_Text)
        {
          String32 insertion32 = str32(&event->character, 1);
          String8 insertion8 = str8_from_32(scratch.arena, insertion32);
          df_msg(DF_MsgKind_InsertText, .string = insertion8);
          df_request_frame();
          take = 1;
          if(event->flags & OS_EventFlag_Alt)
          {
            window->menu_bar_focus_press_started = 0;
          }
        }
        
        //- rjf: do fall-through
        if(!take)
        {
          take = 1;
          df_msg(DF_MsgKind_OSEvent, .os_event = event);
        }
        
        //- rjf: take
        if(take)
        {
          os_eat_event(&events, event);
        }
      }
    }
  }
  
#if 0 // NOTE(rjf): @msgs
  //////////////////////////////
  //- rjf: gather root-level commands
  //
  D_CmdList cmds = d_gather_root_cmds(scratch.arena);
#endif
  
  //////////////////////////////
  //- rjf: process messages
  //
  ProfScope("process messages") DF_RegsScope()
  {
    for(DF_Msg *msg = 0; df_next_msg(&msg);)
    {
      DF_Regs *regs = msg->regs;
      df_regs_copy_contents(scratch.arena, df_regs(), regs);
      Dir2 split_dir = Dir2_Invalid;
      U64 panel_sib_off = 0;
      U64 panel_child_off = 0;
      Vec2S32 panel_change_dir = {0};
      DF_CfgSlot cfg_slot = DF_CfgSlot_User;
      switch(msg->kind)
      {
        //- rjf: engine commands
        default:
        {
          D_MsgKind d_kind = (D_MsgKind)msg->kind;
          if(d_kind < D_MsgKind_COUNT)
          {
            // TODO(rjf): @msgs package regs into d-layer parameters
            d_msg(d_kind);
          }
        }break;
        
        //- rjf: meta
        case DF_MsgKind_Exit:
        {
          CTRL_EntityList running_processes = ctrl_entity_list_from_kind(d_state->ctrl_entity_store, CTRL_EntityKind_Process);
          
          // NOTE(rjf): if targets are running, push confirmation first, and
          // get user's OK
          UI_Key key = ui_key_from_string(ui_key_zero(), str8_lit("lossy_exit_confirmation"));
          if(!ui_key_match(key, df_state->confirm_key) && running_processes.count != 0 && !regs->force_confirm)
          {
            df_state->confirm_key = key;
            df_state->confirm_active = 1;
            arena_clear(df_state->confirm_arena);
            MemoryZeroStruct(&df_state->confirm_msg);
            df_state->confirm_title = push_str8f(df_state->confirm_arena, "Are you sure you want to exit?");
            df_state->confirm_desc = push_str8f(df_state->confirm_arena, "The debugger is still attached to %slive process%s.",
                                                running_processes.count == 1 ? "a " : "",
                                                running_processes.count == 1 ? ""   : "es");
            df_state->confirm_msg.kind = DF_MsgKind_Exit;
            df_state->confirm_msg.regs = df_regs_copy(df_state->confirm_arena, regs);
            df_state->confirm_msg.regs->force_confirm = 1;
          }
          
          // rjf: if no targets are running, or this is force-confirmed,
          // then save & exit
          else
          {
            df_msg(DF_MsgKind_SaveUser);
            df_msg(DF_MsgKind_SaveProject);
            df_state->quit = 1;
          }
        }break;
        case DF_MsgKind_RunCommand:
        {
          DF_MsgKind kind = df_msg_kind_from_string(regs->string);
          if(df_msg_kind_info_table[kind].query.flags & DF_MsgQueryFlag_Required)
          {
            MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
            DF_Window *window = df_window_from_cfg_tree(window_cfg);
            if(window != &df_nil_window)
            {
              arena_clear(window->query_msg_arena);
              window->query_msg_kind = kind;
              window->query_msg_query = df_msg_kind_info_table[kind].query;
              MemoryZeroArray(window->query_msg_regs_mask);
              window->query_view_selected = 1;
            }
          }
          else if(kind != DF_MsgKind_Null)
          {
            df_msg(kind);
          }
        }break;
        case DF_MsgKind_ToggleDevMenu:
        {
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          DF_Window *window = df_window_from_cfg_tree(window_cfg);
          window->dev_menu_is_open ^= 1;
        }break;
        case DF_MsgKind_RegisterAsJITDebugger:
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
            log_user_errorf("Could not register as the just-in-time debugger, access was denied; try running the debugger as administrator.");
          }
#else
          log_user_errorf("Registering as the just-in-time debugger is currently not supported on this system.");
#endif
        }break;
        case DF_MsgKind_LogMarker:
        {
          log_infof("\"#MARKER\"");
        }break;
        
        //- rjf: config loading/saving
        case DF_MsgKind_LoadUser:     cfg_slot = DF_CfgSlot_User; goto load_cfg_data;
        case DF_MsgKind_LoadProject:  cfg_slot = DF_CfgSlot_Project; goto load_cfg_data;
        load_cfg_data:;
        {
          String8 cfg_path = regs->file_path;
          String8 cfg_data = os_data_from_file_path(scratch.arena, cfg_path);
          MD_Node *cfg_tree = df_file_cfg_tree_from_string(cfg_data);
          df_state->cfg_slot_roots[cfg_slot] = cfg_tree;
        }break;
        case DF_MsgKind_SaveUser:    cfg_slot = DF_CfgSlot_User; goto save_cfg_data;
        case DF_MsgKind_SaveProject: cfg_slot = DF_CfgSlot_Project; goto save_cfg_data;
        save_cfg_data:;
        {
          // TODO(rjf): @msgs
        }break;
        
        //- rjf: general entity operations
        case DF_MsgKind_EnableEntity:
        case DF_MsgKind_EnableBreakpoint:
        case DF_MsgKind_EnableTarget:
        {
          MD_Node *cfg_tree = df_cfg_tree_from_handle(regs->cfg_tree);
          df_cfg_tree_set_keyf(cfg_tree, str8_lit("disabled"), "");
        }break;
        case DF_MsgKind_DisableEntity:
        case DF_MsgKind_DisableBreakpoint:
        case DF_MsgKind_DisableTarget:
        {
          MD_Node *cfg_tree = df_cfg_tree_from_handle(regs->cfg_tree);
          df_cfg_tree_set_keyf(cfg_tree, str8_lit("disabled"), "1");
        }break;
        case DF_MsgKind_RemoveEntity:
        case DF_MsgKind_RemoveBreakpoint:
        case DF_MsgKind_RemoveTarget:
        {
          MD_Node *cfg_tree = df_cfg_tree_from_handle(regs->cfg_tree);
          df_cfg_node_release(cfg_tree);
        }break;
        case DF_MsgKind_NameEntity:
        {
          MD_Node *cfg_tree = df_cfg_tree_from_handle(regs->cfg_tree);
          df_cfg_tree_set_key(cfg_tree, str8_lit("label"), regs->string);
        }break;
        case DF_MsgKind_DuplicateEntity:
        {
          MD_Node *cfg_tree = df_cfg_tree_from_handle(regs->cfg_tree);
          MD_Node *cfg_tree_dup = df_cfg_tree_copy(cfg_tree);
          md_node_insert_child(cfg_tree->parent, cfg_tree, cfg_tree_dup);
        }break;
        case DF_MsgKind_RelocateEntity:
        {
          MD_Node *cfg_tree = df_cfg_tree_from_handle(regs->cfg_tree);
          if(regs->file_path.size != 0)
          {
            df_cfg_tree_set_key(cfg_tree, str8_lit("file"), escaped_from_raw_string(scratch.arena, regs->file_path));
          }
          if(regs->cursor.line != 0)
          {
            df_cfg_tree_set_keyf(cfg_tree, str8_lit("pt"), "%I64d:%I64d", regs->cursor.line, regs->cursor.column);
          }
          if(regs->vaddr_range.min != 0)
          {
            df_cfg_tree_set_keyf(cfg_tree, str8_lit("vaddr"), "0x%I64x", regs->vaddr_range.min);
          }
          if(regs->string.size != 0)
          {
            df_cfg_tree_set_key(cfg_tree, str8_lit("symbol"), escaped_from_raw_string(scratch.arena, regs->string));
          }
        }break;
        
        //- rjf: breakpoints
        case DF_MsgKind_AddBreakpoint:
        case DF_MsgKind_ToggleBreakpoint:
        {
          // rjf: unpack
          String8 file_path = regs->file_path;
          TxtPt pt = regs->cursor;
          U64 vaddr = regs->vaddr_range.min;
          String8 symbol = regs->string;
          
          // rjf: remove if needed
          B32 removed_already_existing = 0;
          if(msg->kind == DF_MsgKind_ToggleBreakpoint)
          {
            for(EachEnumVal(DF_CfgSlot, slot))
            {
              for(MD_EachNode(tln, df_state->cfg_slot_roots[slot]))
              {
                if(str8_match(tln->string, str8_lit("breakpoint"), 0))
                {
                  MD_Node *tln_file_node  = md_child_from_string(tln, str8_lit("file"), 0);
                  MD_Node *tln_pt_node    = md_child_from_string(tln, str8_lit("pt"), 0);
                  MD_Node *tln_vaddr_node = md_child_from_string(tln, str8_lit("vaddr"), 0);
                  MD_Node *tln_symbol_node= md_child_from_string(tln, str8_lit("symbol"), 0);
                  String8 tln_file_path = raw_from_escaped_string(scratch.arena, tln_file_node->first->string);
                  TxtPt   tln_pt        = {0};
                  try_s64_from_str8_c_rules(tln_pt_node->first->string, &tln_pt.line);
                  try_s64_from_str8_c_rules(tln_pt_node->first->first->string, &tln_pt.column);
                  U64     tln_vaddr     = 0;
                  try_u64_from_str8_c_rules(tln_vaddr_node->first->string, &tln_vaddr);
                  String8 tln_symbol    = raw_from_escaped_string(scratch.arena, tln_symbol_node->first->string);
                  if((path_match_normalized(tln_file_path, file_path) &&
                      tln_pt.line == pt.line) ||
                     str8_match(tln_symbol, symbol, 0) ||
                     tln_vaddr == vaddr)
                  {
                    df_cfg_node_release(tln);
                    removed_already_existing = 1;
                    goto break_all_remove_existing;
                  }
                }
              }
            }
            break_all_remove_existing:;
          }
          
          // rjf: not removed? -> add new & locate
          if(!removed_already_existing)
          {
            MD_Node *root = df_state->cfg_slot_roots[DF_CfgSlot_Project];
            MD_Node *prev_bp = df_cfg_tree_last_from_key(root, str8_lit("breakpoint"));
            MD_Node *new_bp = df_single_cfg_tree_from_stringf("breakpoint");
            md_node_insert_child(root, prev_bp, new_bp);
            df_msg(DF_MsgKind_RelocateEntity, .cfg_tree = df_handle_from_cfg_tree(new_bp));
          }
        }break;
        case DF_MsgKind_AddAddressBreakpoint:
        case DF_MsgKind_AddFunctionBreakpoint:
        {
          df_msg(DF_MsgKind_AddBreakpoint);
        }break;
        
        //- rjf: watch pins
        case DF_MsgKind_AddWatchPin:
        case DF_MsgKind_ToggleWatchPin:
        {
          String8 file_path = regs->file_path;
          TxtPt pt = regs->cursor;
          String8 string = regs->string;
          U64 vaddr = regs->vaddr_range.min;
          B32 removed_already_existing = 0;
          if(msg->kind == DF_MsgKind_ToggleWatchPin)
          {
            D_EntityList wps = d_query_cached_entity_list_with_kind(D_EntityKind_WatchPin);
            for(D_EntityNode *n = wps.first; n != 0; n = n->next)
            {
              D_Entity *wp = n->entity;
              D_Entity *loc = d_entity_child_from_kind(wp, D_EntityKind_Location);
              if((loc->flags & D_EntityFlag_HasTextPoint && path_match_normalized(loc->string, file_path) && loc->text_point.line == pt.line) ||
                 (loc->flags & D_EntityFlag_HasVAddr && loc->vaddr == vaddr) ||
                 (!(loc->flags & D_EntityFlag_HasTextPoint) && str8_match(loc->string, string, 0)))
              {
                d_entity_mark_for_deletion(wp);
                removed_already_existing = 1;
                break;
              }
            }
          }
          if(!removed_already_existing)
          {
            D_Entity *wp = d_entity_alloc(d_entity_root(), D_EntityKind_WatchPin);
            d_entity_equip_name(wp, string);
            d_entity_equip_cfg_src(wp, D_CfgSrc_Project);
            D_Entity *loc = d_entity_alloc(wp, D_EntityKind_Location);
            if(file_path.size != 0 && pt.line != 0)
            {
              d_entity_equip_name(loc, file_path);
              d_entity_equip_txt_pt(loc, pt);
            }
            else if(vaddr != 0)
            {
              d_entity_equip_vaddr(loc, vaddr);
            }
          }
        }break;
        
        //- rjf: watch expressions
        case DF_MsgKind_ToggleWatchExpression:
        if(regs->string.size != 0)
        {
          D_Entity *existing_watch = d_entity_from_name_and_kind(regs->string, D_EntityKind_Watch);
          if(d_entity_is_nil(existing_watch))
          {
            D_Entity *watch = &d_nil_entity;
            D_StateDeltaHistoryBatch(d_state_delta_history())
            {
              watch = d_entity_alloc(d_entity_root(), D_EntityKind_Watch);
            }
            d_entity_equip_cfg_src(watch, D_CfgSrc_Project);
            d_entity_equip_name(watch, regs->string);
          }
          else
          {
            d_entity_mark_for_deletion(existing_watch);
          }
        }break;
        
        //- rjf: at-cursor operations
        case DF_MsgKind_ToggleBreakpointAtCursor:{df_msg(DF_MsgKind_ToggleBreakpoint);}break;
        case DF_MsgKind_ToggleWatchPinAtCursor:{df_msg(DF_MsgKind_ToggleWatchPin);}break;
        case DF_MsgKind_GoToNameAtCursor:
        case DF_MsgKind_ToggleWatchExpressionAtCursor:
        {
          // rjf: get expr
          String8 expr = {0};
          {
            HS_Scope *hs_scope = hs_scope_open();
            TXT_Scope *txt_scope = txt_scope_open();
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
            expr = push_str8_copy(scratch.arena, str8_substr(data, expr_off_range));
            txt_scope_close(txt_scope);
            hs_scope_close(hs_scope);
          }
          
          // rjf: push command for this expr
          df_msg(msg->kind == DF_MsgKind_GoToNameAtCursor ? DF_MsgKind_GoToName :
                 msg->kind == DF_MsgKind_ToggleWatchExpressionAtCursor ? DF_MsgKind_ToggleWatchExpression :
                 DF_MsgKind_GoToName,
                 .string = expr);
          // NOTE(rjf): @msgs
          // d_msg(msg->kind == DF_MsgKind_GoToNameAtCursor ? DF_MsgKind_GoToName :
          // msg->kind == DF_MsgKind_ToggleWatchExpressionAtCursor ? DF_MsgKind_ToggleWatchExpression :
          // DF_MsgKind_GoToName, .string = expr);
        }break;
        
        //- rjf: targets
        case DF_MsgKind_AddTarget:
        {
          DF_CfgSlot cfg_slot = DF_CfgSlot_Project;
          MD_Node *cfg_root = df_state->cfg_slot_roots[cfg_slot];
          String8 cfg_path = cfg_root->string;
          String8 cfg_folder = str8_chop_last_slash(cfg_path);
          String8 file_path = regs->file_path;
          String8 file_path_normalized = path_normalized_from_string(scratch.arena, file_path);
          String8 file_path_relative = path_relative_dst_from_absolute_dst_src(scratch.arena, file_path_normalized, cfg_folder);
          String8 file_path_escaped = escaped_from_raw_string(scratch.arena, file_path_relative);
          String8 working_dir_normalized = str8_chop_last_slash(file_path_normalized);
          String8 working_dir_relative = path_relative_dst_from_absolute_dst_src(scratch.arena, working_dir_normalized, cfg_folder);
          String8 working_dir_escaped = escaped_from_raw_string(scratch.arena, working_dir_relative);
          MD_Node *target_cfg = df_single_cfg_tree_from_stringf("target:{executable:\"%S\", working_directory:\"%S\"}",
                                                                file_path_escaped,
                                                                working_dir_escaped);
          MD_Node *last_target_cfg = df_cfg_tree_last_from_key(cfg_root, str8_lit("target"));
          md_node_insert_child(cfg_root, last_target_cfg, target_cfg);
          df_msg(DF_MsgKind_SelectTarget, .string = df_key_from_cfg_tree(scratch.arena, target_cfg));
        }break;
        case DF_MsgKind_SelectTarget:
        {
          MD_Node *target_cfg = df_cfg_tree_from_handle(regs->cfg_tree);
          DF_CfgSlot cfg_slot = df_cfg_slot_from_tree(target_cfg);
          if(!md_node_is_nil(target_cfg))
          {
            for(EachEnumVal(DF_CfgSlot, slot))
            {
              for(MD_EachNode(tln, df_state->cfg_slot_roots[slot]->first))
              {
                if(str8_match(target_cfg->string, tln->string, 0))
                {
                  df_cfg_tree_set_keyf(tln, str8_lit("disabled"), "%s", tln == target_cfg ? "" : "1");
                }
              }
            }
          }
          
#if 0 // NOTE(rjf): @msgs
          D_Entity *entity = d_entity_from_handle(regs->entity);
          if(entity->kind == D_EntityKind_Target)
          {
            D_EntityList all_targets = d_query_cached_entity_list_with_kind(D_EntityKind_Target);
            B32 is_selected = !entity->disabled;
            for(D_EntityNode *n = all_targets.first; n != 0; n = n->next)
            {
              D_Entity *target = n->entity;
              d_entity_equip_disabled(target, 1);
            }
            if(!is_selected)
            {
              d_entity_equip_disabled(entity, 0);
            }
          }
#endif
        }break;
        
        //- rjf: windows
        case DF_MsgKind_OpenWindow:
        {
          // TODO(rjf): @msgs
        }break;
        case DF_MsgKind_CloseWindow:
        {
          // TODO(rjf): coordinate with Exit msg
        }break;
        case DF_MsgKind_ToggleFullscreen:
        {
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          DF_Window *window = df_window_from_cfg_tree(window_cfg);
          if(window != &df_nil_window)
          {
            os_window_set_fullscreen(window->os, !os_window_is_fullscreen(window->os));
          }
        }break;
        
        //- rjf: confirmation
        case DF_MsgKind_ConfirmAccept:
        {
          df_state->confirm_active = 0;
          df_state->confirm_key = ui_key_zero();
          DF_RegsScope()
          {
            df_regs_copy_contents(scratch.arena, df_regs(), df_state->confirm_msg.regs);
            df_msg(df_state->confirm_msg.kind);
          }
        }break;
        case DF_MsgKind_ConfirmCancel:
        {
          df_state->confirm_active = 0;
          df_state->confirm_key = ui_key_zero();
        }break;
        
        //- rjf: queries
        case DF_MsgKind_CompleteQuery:
        {
          // TODO(rjf): @msgs
        }break;
        case DF_MsgKind_CancelQuery:
        {
          // TODO(rjf): @msgs
        }break;
        
        //- rjf: searching
        case DF_MsgKind_FindTextForward:
        case DF_MsgKind_FindTextBackward:
        {
          df_set_search_string(regs->string);
        }break;
        case DF_MsgKind_FindNext:{df_msg(DF_MsgKind_FindTextForward);}break;
        case DF_MsgKind_FindPrev:{df_msg(DF_MsgKind_FindTextBackward);}break;
        
        //- rjf: font sizes
        case DF_MsgKind_IncUIFontScale:
        {
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          DF_Window *window = df_window_from_cfg_tree(window_cfg);
          if(window != &df_nil_window)
          {
            window->setting_vals[DF_SettingCode_MainFontSize].set = 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 += 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_MainFontSize], window->setting_vals[DF_SettingCode_MainFontSize].s32);
          }
        }break;
        case DF_MsgKind_DecUIFontScale:
        {
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          DF_Window *window = df_window_from_cfg_tree(window_cfg);
          if(window != &df_nil_window)
          {
            window->setting_vals[DF_SettingCode_MainFontSize].set = 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 -= 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_MainFontSize], window->setting_vals[DF_SettingCode_MainFontSize].s32);
          }
        }break;
        case DF_MsgKind_IncCodeFontScale:
        {
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          DF_Window *window = df_window_from_cfg_tree(window_cfg);
          if(window != &df_nil_window)
          {
            window->setting_vals[DF_SettingCode_CodeFontSize].set = 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 += 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_CodeFontSize], window->setting_vals[DF_SettingCode_CodeFontSize].s32);
          }
        }break;
        case DF_MsgKind_DecCodeFontScale:
        {
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          DF_Window *window = df_window_from_cfg_tree(window_cfg);
          if(window != &df_nil_window)
          {
            window->setting_vals[DF_SettingCode_CodeFontSize].set = 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 -= 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_CodeFontSize], window->setting_vals[DF_SettingCode_CodeFontSize].s32);
          }
        }break;
        
        //- rjf: [panel creation/removal] splitting
        case DF_MsgKind_NewPanelLeft: {split_dir = Dir2_Left;}goto panel_split;
        case DF_MsgKind_NewPanelUp:   {split_dir = Dir2_Up;}goto panel_split;
        case DF_MsgKind_NewPanelRight:{split_dir = Dir2_Right;}goto panel_split;
        case DF_MsgKind_NewPanelDown: {split_dir = Dir2_Down;}goto panel_split;
        case DF_MsgKind_SplitPanel:
        {
          split_dir = regs->dir2;
        }goto panel_split;
        panel_split:;
        if(split_dir != Dir2_Invalid)
        {
          // rjf: unpack params
          MD_Node *window_cfg = df_cfg_tree_from_handle(regs->window);
          MD_Node *panel_cfg = df_cfg_tree_from_handle(regs->panel);
          MD_Node *parent_cfg = panel_cfg->parent;
          Axis2 parent_split_axis = axis2_flip(parent_cfg);
          Axis2 split_axis = axis2_from_dir2(split_dir);
          Side split_side = side_from_dir2(split_dir);
          
          // rjf: create new panel, fix up tree as needed
          MD_Node *new_panel_cfg = &md_nil_node;
          {
            // rjf: splitting along parent's axis? -> just add new panel
            if(parent_cfg != window_cfg && parent_split_axis == split_axis)
            {
              U64 child_count = 0;
              for(MD_EachNode(child, parent_cfg->first)) {child_count += !!(child->flags & MD_NodeFlag_Numeric);}
              F32 new_child_pct = 1.f/child_count;
              MD_Node *prev_child = split_side == Side_Max ? panel_cfg : panel_cfg->prev;
              MD_Node *next = df_single_cfg_tree_from_stringf("%f", new_child_pct);
              md_node_insert_child(parent_cfg, prev_child, next);
              for(MD_EachNode(child, parent_cfg->first))
              {
                if(child != next && child->flags & MD_NodeFlag_Numeric)
                {
                  F32 pct = f32_from_str8(child->string);
                  pct *= (F32)(child_count-1)/child_count;
                  df_cfg_tree_set_stringf(child, "%f", pct);
                }
              }
              new_panel_cfg = next;
            }
            
            // rjf: splitting along different axis? need to create new subtree
            else
            {
#if 0 // TODO(rjf): @msgs
              DF_Panel *pre_prev = panel->prev;
              DF_Panel *pre_parent = parent;
              DF_Panel *new_parent = df_panel_alloc(ws);
              new_parent->pct_of_parent = panel->pct_of_parent;
              if(!df_panel_is_nil(pre_parent))
              {
                df_panel_remove(pre_parent, panel);
                df_panel_insert(pre_parent, pre_prev, new_parent);
              }
              else
              {
                ws->root_panel = new_parent;
              }
              DF_Panel *left = panel;
              DF_Panel *right = df_panel_alloc(ws);
              new_panel = right;
              if(split_side == Side_Min)
              {
                Swap(DF_Panel *, left, right);
              }
              df_panel_insert(new_parent, &df_nil_panel, left);
              df_panel_insert(new_parent, left, right);
              new_parent->split_axis = split_axis;
              left->pct_of_parent = 0.5f;
              right->pct_of_parent = 0.5f;
              ws->focused_panel = new_panel;
#endif
            }
          }
          
          // rjf: set up new panel's animation rectangle to begin at edge where
          // it is spawning
#if 0 // TODO(rjf): @msgs
          if(!df_panel_is_nil(new_panel->prev))
          {
            Rng2F32 prev_rect_pct = new_panel->prev->animated_rect_pct;
            new_panel->animated_rect_pct = prev_rect_pct;
            new_panel->animated_rect_pct.p0.v[split_axis] = new_panel->animated_rect_pct.p1.v[split_axis];
          }
          if(!df_panel_is_nil(new_panel->next))
          {
            Rng2F32 next_rect_pct = new_panel->next->animated_rect_pct;
            new_panel->animated_rect_pct = next_rect_pct;
            new_panel->animated_rect_pct.p1.v[split_axis] = new_panel->animated_rect_pct.p0.v[split_axis];
          }
#endif
          
          // rjf: move tab, if doing combined move-tab-and-split option
          MD_Node *move_tab_cfg = df_cfg_tree_from_handle(regs->tab);
          if(!md_node_is_nil(new_panel_cfg) &&
             !md_node_is_nil(move_tab_cfg) &&
             msg->kind == DF_MsgKind_SplitPanel)
          {
            MD_Node *move_tab_panel_cfg = move_tab_cfg->parent;
            md_unhook_child(move_tab_cfg);
            md_node_insert_child(new_panel_cfg, new_panel_cfg->last, move_tab_cfg);
            df_msg(DF_MsgKind_SelectTab, .tab = df_handle_from_cfg_tree(move_tab_cfg));
            B32 move_tab_panel_is_empty = 1;
            for(MD_EachNode(child, move_tab_panel_cfg->first))
            {
              if(!df_tab_cfg_is_project_filtered(child))
              {
                move_tab_panel_is_empty = 0;
                break;
              }
            }
            if(move_tab_panel_is_empty && move_tab_panel_cfg != panel_cfg)
            {
              df_msg(DF_MsgKind_ClosePanel, .panel = df_handle_from_cfg_tree(move_tab_panel_cfg));
            }
          }
        }
        
        //- rjf: [panel creation/removal] removal
        case DF_MsgKind_ClosePanel:
        {
          // rjf: unpack context of the panel-to-close
          MD_Node *panel_cfg = df_cfg_tree_from_handle(regs->panel);
          DF_CfgSlot cfg_slot = df_cfg_slot_from_tree(panel_cfg);
          MD_Node *root_panel_cfg = &md_nil_node;
          for(MD_Node *p = panel_cfg; !md_node_is_nil(p); p = p->parent)
          {
            root_panel_cfg = p;
            if(str8_match(root_panel_cfg->string, str8_lit("panels"), 0))
            {
              break;
            }
          }
          MD_Node *window_cfg = root_panel_cfg->parent;
          MD_Node *parent_cfg = panel_cfg->parent;
          
          // rjf: close panel, if not root
          if(panel_cfg != root_panel_cfg)
          {
            // rjf: count children
            U64 child_count = 0;
            for(MD_EachNode(child, parent_cfg->first)) {child_count += !!(child->flags & MD_NodeFlag_Numeric);}
            
            // NOTE(rjf): if we are removing one of two remaining children of
            // any panel, then we need to remove this panel, but bubble the
            // remaining one upwards & merge with ancestors
            if(child_count == 2)
            {
              // rjf: gather nodes
              MD_Node *grandparent_cfg = (parent_cfg != root_panel_cfg) ? parent_cfg->parent : &md_nil_node;
              MD_Node *discard_child_cfg = panel_cfg;
              MD_Node *keep_child_cfg = &md_nil_node;
              F32 pct_of_parent = f32_from_str8(parent_cfg->string);
              for(MD_EachNode(child, parent_cfg->first))
              {
                if(child != discard_child_cfg && child->flags & MD_NodeFlag_Numeric)
                {
                  keep_child_cfg = child;
                  break;
                }
              }
              MD_Node *parent_prev_cfg = &md_nil_node;
              for(MD_EachNode(child, grandparent_cfg->first))
              {
                if(child == parent_cfg)
                {
                  break;
                }
                if(child->flags & MD_NodeFlag_Numeric)
                {
                  parent_prev_cfg = child;
                }
              }
              
              // rjf: unhook kept child
              md_unhook_child(keep_child_cfg);
              
              // rjf: unhook this subtree
              if(!md_node_is_nil(grandparent_cfg))
              {
                md_unhook_child(parent_cfg);
              }
              
              // rjf: release the things we should discard
              {
                df_cfg_node_release(parent_cfg);
                df_cfg_node_release(discard_child_cfg);
              }
              
              // rjf: re-hook our kept child into the overall tree
              if(md_node_is_nil(grandparent_cfg))
              {
                md_node_insert_child(window_cfg, window_cfg->last, keep_child_cfg);
                df_cfg_tree_set_stringf(keep_child_cfg, "panels");
              }
              else
              {
                md_node_insert_child(grandparent_cfg, parent_prev_cfg, keep_child_cfg);
                df_cfg_tree_set_stringf(keep_child_cfg, "%f", pct_of_parent);
              }
              
              // rjf: reset focus, if needed
              if(md_node_has_tag(discard_child_cfg, str8_lit("selected"), 0))
              {
                MD_Node *new_focused_panel_cfg = keep_child_cfg;
                for(MD_Node *descendant = new_focused_panel_cfg; !md_node_is_nil(descendant); descendant = descendant->first)
                {
                  new_focused_panel_cfg = descendant;
                }
                df_msg(DF_MsgKind_FocusPanel, .panel = df_handle_from_cfg_tree(new_focused_panel_cfg));
              }
              
              // rjf: keep-child has children? bubble keep-child children up into grandparent's children
              if(!md_node_is_nil(grandparent_cfg) && !md_node_is_nil(md_node_from_chain_flags(keep_child_cfg->first, &md_nil_node, MD_NodeFlag_Numeric)))
              {
                md_unhook_child(keep_child_cfg);
                MD_Node *prev_cfg = parent_prev_cfg;
                F32 keep_child_pct = f32_from_str8(keep_child_cfg->string);
                for(MD_Node *child_cfg = keep_child_cfg->first, *next = 0; !md_node_is_nil(child_cfg); child_cfg = next)
                {
                  next = child_cfg->next;
                  md_unhook_child(child_cfg);
                  md_node_insert_child(grandparent_cfg, prev_cfg, child_cfg);
                  prev_cfg = child_cfg;
                  F32 pct = f32_from_str8(child_cfg->string);
                  F32 new_pct = pct*keep_child_pct;
                  df_cfg_tree_set_stringf(child_cfg, "%f", new_pct);
                }
                df_cfg_node_release(keep_child_cfg);
              }
            }
            
            // rjf: if we are just removing one child of >2, then we just need
            // to remove this one from the list & give its space back to siblings
            else
            {
              F32 removed_size_pct = f32_from_str8(panel_cfg->string);
              MD_Node *next = &md_nil_node;
              if(!md_node_is_nil(next))
              {
                next = md_node_from_chain_flags(panel_cfg->next, &md_nil_node, MD_NodeFlag_Numeric);
              }
              if(!md_node_is_nil(next))
              {
                for(MD_EachNode(child, parent_cfg->first))
                {
                  if(child != panel_cfg && child->flags & MD_NodeFlag_Numeric)
                  {
                    next = child;
                  }
                }
              }
              B32 panel_is_focused = md_node_has_tag(panel_cfg, str8_lit("selected"), 0);
              md_unhook_child(panel_cfg);
              df_cfg_node_release(panel_cfg);
              if(panel_is_focused)
              {
                df_msg(DF_MsgKind_FocusPanel, .panel = df_handle_from_cfg_tree(next));
              }
              for(MD_EachNode(child, parent_cfg->first))
              {
                F32 pct = f32_from_str8(child->string);
                F32 new_pct = pct / (1.f - removed_size_pct);
                df_cfg_tree_set_stringf(child, "%f", new_pct);
              }
            }
          }
        }break;
        
        //- rjf: panel rearranging
#if 0 // TODO(rjf): @msgs
        case DF_MsgKind_RotatePanelColumns:
        {
          MD_Node *panel = df_cfg_tree_from_handle(regs->panel);
          MD_Node *parent = &md_nil_node;
          DF_Window *ws = df_window_from_handle(regs->window);
          DF_Panel *panel = ws->focused_panel;
          DF_Panel *parent = &df_nil_panel;
          for(DF_Panel *p = panel->parent; !df_panel_is_nil(p); p = p->parent)
          {
            if(p->split_axis == Axis2_X)
            {
              parent = p;
              break;
            }
          }
          if(!df_panel_is_nil(parent) && parent->child_count > 1)
          {
            DF_Panel *old_first = parent->first;
            DF_Panel *new_first = parent->first->next;
            old_first->next = &df_nil_panel;
            old_first->prev = parent->last;
            parent->last->next = old_first;
            new_first->prev = &df_nil_panel;
            parent->first = new_first;
            parent->last = old_first;
          }
        }break;
#endif
        
        //- rjf: panel focusing
        case DF_MsgKind_NextPanel: panel_sib_off = OffsetOf(DF_Panel, next); panel_child_off = OffsetOf(DF_Panel, first); goto panel_cycle;
        case DF_MsgKind_PrevPanel: panel_sib_off = OffsetOf(DF_Panel, prev); panel_child_off = OffsetOf(DF_Panel, last); goto panel_cycle;
        panel_cycle:;
        {
#if 0 // TODO(rjf): @msgs
          DF_Window *ws = df_window_from_handle(regs->window);
          for(DF_Panel *panel = ws->focused_panel; !df_panel_is_nil(panel);)
          {
            DF_PanelRec rec = df_panel_rec_df(panel, panel_sib_off, panel_child_off);
            panel = rec.next;
            if(df_panel_is_nil(panel))
            {
              panel = ws->root_panel;
            }
            if(df_panel_is_nil(panel->first))
            {
              df_msg(DF_MsgKind_FocusPanel, .panel = df_handle_from_panel(panel));
              break;
            }
          }
#endif
        }break;
        case DF_MsgKind_FocusPanel:
        {
#if 0 // TODO(rjf): @msgs
          DF_Window *ws = df_window_from_handle(regs->window);
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          if(!df_panel_is_nil(panel))
          {
            ws->focused_panel = panel;
            ws->menu_bar_focused = 0;
            ws->query_view_selected = 0;
          }
#endif
        }break;
        case DF_MsgKind_FocusPanelRight: panel_change_dir = v2s32(+1, +0); goto msg_focus_panel_dir;
        case DF_MsgKind_FocusPanelLeft:  panel_change_dir = v2s32(-1, +0); goto msg_focus_panel_dir;
        case DF_MsgKind_FocusPanelUp:    panel_change_dir = v2s32(+0, -1); goto msg_focus_panel_dir;
        case DF_MsgKind_FocusPanelDown:  panel_change_dir = v2s32(+0, +1); goto msg_focus_panel_dir;
        msg_focus_panel_dir:;
        {
#if 0 // TODO(rjf): @msgs
          DF_Window *ws = df_window_from_handle(regs->window);
          DF_Panel *src_panel = ws->focused_panel;
          Rng2F32 src_panel_rect = df_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, src_panel);
          Vec2F32 src_panel_center = center_2f32(src_panel_rect);
          Vec2F32 src_panel_half_dim = scale_2f32(dim_2f32(src_panel_rect), 0.5f);
          Vec2F32 travel_dim = add_2f32(src_panel_half_dim, v2f32(10.f, 10.f));
          Vec2F32 travel_dst = add_2f32(src_panel_center, mul_2f32(travel_dim, v2f32((F32)panel_change_dir.x, (F32)panel_change_dir.y)));
          DF_Panel *dst_root = &df_nil_panel;
          for(DF_Panel *p = ws->root_panel; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
          {
            if(p == src_panel || !df_panel_is_nil(p->first))
            {
              continue;
            }
            Rng2F32 p_rect = df_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, p);
            if(contains_2f32(p_rect, travel_dst))
            {
              dst_root = p;
              break;
            }
          }
          if(!df_panel_is_nil(dst_root))
          {
            DF_Panel *dst_panel = &df_nil_panel;
            for(DF_Panel *p = dst_root; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
            {
              if(df_panel_is_nil(p->first) && p != src_panel)
              {
                dst_panel = p;
                break;
              }
            }
            df_msg(DF_MsgKind_FocusPanel, .panel = df_handle_from_panel(dst_panel));
          }
#endif
        }break;
        
        //- rjf: view history navigation
        case DF_MsgKind_GoBack:{}break;
        case DF_MsgKind_GoForward:{}break;
        
        //- rjf: tab selection
        case DF_MsgKind_NextTab:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *next_view = view;
          for(DF_View *v = view;
              !df_view_is_nil(v);
              v = df_view_is_nil(v->order_next) ? panel->first_tab_view : v->order_next)
          {
            if(!df_view_is_project_filtered(v) && v != view)
            {
              next_view = v;
              break;
            }
          }
          view = next_view;
          panel->selected_tab_view = df_handle_from_view(view);
#endif
        }break;
        case DF_MsgKind_PrevTab:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *next_view = view;
          for(DF_View *v = view;
              !df_view_is_nil(v);
              v = df_view_is_nil(v->order_prev) ? panel->last_tab_view : v->order_prev)
          {
            if(!df_view_is_project_filtered(v) && v != view)
            {
              next_view = v;
              break;
            }
          }
          view = next_view;
          panel->selected_tab_view = df_handle_from_view(view);
#endif
        }break;
        
        //- rjf: tab rearranging
        case DF_MsgKind_MoveTabRight:
        case DF_MsgKind_MoveTabLeft:
        {
#if 0 // TODO(rjf): @msgs
          DF_Window *ws = df_window_from_handle(regs->window);
          DF_Panel *panel = ws->focused_panel;
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *prev_view = (msg->kind == DF_MsgKind_MoveTabRight ? view->order_next : view->order_prev->order_prev);
          if(!df_view_is_nil(prev_view) || msg->kind == DF_MsgKind_MoveTabLeft)
          {
            df_msg(DF_MsgKind_MoveTab,
                   .panel     = df_handle_from_panel(panel),
                   .dst_panel = df_handle_from_panel(panel),
                   .view      = df_handle_from_view(view),
                   .prev_view = df_handle_from_view(prev_view));
          }
#endif
        }break;
        case DF_MsgKind_MoveTab:
        {
#if 0 // TODO(rjf): @msgs
          DF_Window * ws        = df_window_from_handle(regs->window);
          DF_Panel *  src_panel = df_panel_from_handle(regs->panel);
          DF_View *   view      = df_view_from_handle(regs->view);
          DF_Panel *  dst_panel = df_panel_from_handle(regs->dst_panel);
          DF_View *prev_view = df_view_from_handle(regs->prev_view);
          if(!df_panel_is_nil(src_panel) &&
             !df_panel_is_nil(dst_panel) &&
             prev_view != view)
          {
            df_panel_remove_tab_view(src_panel, view);
            df_panel_insert_tab_view(dst_panel, prev_view, view);
            ws->focused_panel = dst_panel;
            B32 src_panel_is_empty = 1;
            for(DF_View *v = src_panel->first_tab_view; !df_view_is_nil(v); v = v->order_next)
            {
              if(!df_view_is_project_filtered(v))
              {
                src_panel_is_empty = 0;
                break;
              }
            }
            if(src_panel_is_empty && src_panel != ws->root_panel)
            {
              df_msg(DF_MsgKind_ClosePanel, .panel = df_handle_from_panel(src_panel));
            }
          }
#endif
        }break;
        
        //- rjf: tab creation/removal
        case DF_MsgKind_OpenTab:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          DF_View *view = df_view_alloc();
          df_view_equip_spec(view, &df_nil_view_spec, regs->string, regs->params_tree);
          DF_View *prev_view = panel->last_tab_view;
          if(!df_view_is_nil(df_view_from_handle(regs->prev_view)))
          {
            prev_view = df_view_from_handle(regs->prev_view);
          }
          df_panel_insert_tab_view(panel, prev_view, view);
#endif
        }break;
        case DF_MsgKind_CloseTab:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          DF_View *view = df_view_from_handle(regs->view);
          if(!df_view_is_nil(view))
          {
            df_panel_remove_tab_view(panel, view);
            df_view_release(view);
          }
#endif
        }break;
        
        //- rjf: panel tab settings
        case DF_MsgKind_TabBarTop:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          panel->tab_side = Side_Min;
#endif
        }break;
        case DF_MsgKind_TabBarBottom:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          panel->tab_side = Side_Max;
#endif
        }break;
        
        //- rjf: tab filters
        case DF_MsgKind_Filter:
        {
#if 0 // TODO(rjf): @msgs
          DF_View *view = df_view_from_handle(regs->view);
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          B32 view_is_tab = 0;
          for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->order_next)
          {
            if(df_view_is_project_filtered(tab)) { continue; }
            if(tab == view)
            {
              view_is_tab = 1;
              break;
            }
          }
          if(view_is_tab && view->spec->info.flags & DF_ViewSpecFlag_CanFilter)
          {
            view->is_filtering ^= 1;
            view->query_cursor = txt_pt(1, 1+(S64)view->query_string_size);
            view->query_mark = txt_pt(1, 1);
          }
#endif
        }break;
        case DF_MsgKind_ClearFilter:
        {
#if 0 // TODO(rjf): @msgs
          DF_View *view = df_view_from_handle(regs->view);
          if(!df_view_is_nil(view))
          {
            view->query_string_size = 0;
            view->is_filtering = 0;
            view->query_cursor = view->query_mark = txt_pt(1, 1);
          }
#endif
        }break;
        case DF_MsgKind_ApplyFilter:
        {
#if 0 // TODO(rjf): @msgs
          DF_View *view = df_view_from_handle(regs->view);
          if(!df_view_is_nil(view))
          {
            view->is_filtering = 0;
          }
#endif
        }break;
        
        //- rjf: default panel layouts
        case DF_MsgKind_ResetToDefaultPanels:
        case DF_MsgKind_ResetToCompactPanels:
        {
#if 0 // TODO(rjf): @msgs
          DF_Window *ws = df_window_from_handle(regs->window);
          
          typedef enum Layout
          {
            Layout_Default,
            Layout_Compact,
          }
          Layout;
          Layout layout = Layout_Default;
          switch(msg->kind)
          {
            default:{}break;
            case DF_MsgKind_ResetToDefaultPanels:{layout = Layout_Default;}break;
            case DF_MsgKind_ResetToCompactPanels:{layout = Layout_Compact;}break;
          }
          
          //- rjf: gather all panels in the panel tree - remove & gather views
          // we'd like to keep in the next layout
          D_HandleList panels_to_close = {0};
          D_HandleList views_to_close = {0};
          DF_View *watch = &df_nil_view;
          DF_View *locals = &df_nil_view;
          DF_View *regs = &df_nil_view;
          DF_View *globals = &df_nil_view;
          DF_View *tlocals = &df_nil_view;
          DF_View *types = &df_nil_view;
          DF_View *procs = &df_nil_view;
          DF_View *callstack = &df_nil_view;
          DF_View *breakpoints = &df_nil_view;
          DF_View *watch_pins = &df_nil_view;
          DF_View *output = &df_nil_view;
          DF_View *targets = &df_nil_view;
          DF_View *scheduler = &df_nil_view;
          DF_View *modules = &df_nil_view;
          DF_View *disasm = &df_nil_view;
          DF_View *memory = &df_nil_view;
          DF_View *getting_started = &df_nil_view;
          D_HandleList code_views = {0};
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            D_Handle handle = df_handle_from_panel(panel);
            d_handle_list_push(scratch.arena, &panels_to_close, handle);
            for(DF_View *view = panel->first_tab_view, *next = 0; !df_view_is_nil(view); view = next)
            {
              next = view->order_next;
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              B32 needs_delete = 1;
              switch(view_kind)
              {
                default:{}break;
                case DF_ViewKind_Watch:         {if(df_view_is_nil(watch))               { needs_delete = 0; watch = view;} }break;
                case DF_ViewKind_Locals:        {if(df_view_is_nil(locals))              { needs_delete = 0; locals = view;} }break;
                case DF_ViewKind_Registers:     {if(df_view_is_nil(regs))                { needs_delete = 0; regs = view;} }break;
                case DF_ViewKind_Globals:       {if(df_view_is_nil(globals))             { needs_delete = 0; globals = view;} }break;
                case DF_ViewKind_ThreadLocals:  {if(df_view_is_nil(tlocals))             { needs_delete = 0; tlocals = view;} }break;
                case DF_ViewKind_Types:         {if(df_view_is_nil(types))               { needs_delete = 0; types = view;} }break;
                case DF_ViewKind_Procedures:    {if(df_view_is_nil(procs))               { needs_delete = 0; procs = view;} }break;
                case DF_ViewKind_CallStack:     {if(df_view_is_nil(callstack))           { needs_delete = 0; callstack = view;} }break;
                case DF_ViewKind_Breakpoints:   {if(df_view_is_nil(breakpoints))         { needs_delete = 0; breakpoints = view;} }break;
                case DF_ViewKind_WatchPins:     {if(df_view_is_nil(watch_pins))          { needs_delete = 0; watch_pins = view;} }break;
                case DF_ViewKind_Output:        {if(df_view_is_nil(output))              { needs_delete = 0; output = view;} }break;
                case DF_ViewKind_Targets:       {if(df_view_is_nil(targets))             { needs_delete = 0; targets = view;} }break;
                case DF_ViewKind_Scheduler:     {if(df_view_is_nil(scheduler))           { needs_delete = 0; scheduler = view;} }break;
                case DF_ViewKind_Modules:       {if(df_view_is_nil(modules))             { needs_delete = 0; modules = view;} }break;
                case DF_ViewKind_Disasm:        {if(df_view_is_nil(disasm))              { needs_delete = 0; disasm = view;} }break;
                case DF_ViewKind_Memory:        {if(df_view_is_nil(memory))              { needs_delete = 0; memory = view;} }break;
                case DF_ViewKind_GettingStarted:{if(df_view_is_nil(getting_started))     { needs_delete = 0; getting_started = view;} }break;
                case DF_ViewKind_Text:
                {
                  needs_delete = 0;
                  d_handle_list_push(scratch.arena, &code_views, df_handle_from_view(view));
                }break;
              }
              if(!needs_delete)
              {
                df_panel_remove_tab_view(panel, view);
              }
            }
          }
          
          //- rjf: close all panels/views
          for(D_HandleNode *n = panels_to_close.first; n != 0; n = n->next)
          {
            DF_Panel *panel = df_panel_from_handle(n->handle);
            if(panel != ws->root_panel)
            {
              df_panel_release(ws, panel);
            }
            else
            {
              df_panel_release_all_views(panel);
              panel->first = panel->last = &df_nil_panel;
            }
          }
          
          //- rjf: allocate any missing views
          if(df_view_is_nil(watch))
          {
            watch = df_view_alloc();
            df_view_equip_spec(watch, df_view_spec_from_kind(DF_ViewKind_Watch), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(locals))
          {
            locals = df_view_alloc();
            df_view_equip_spec(locals, df_view_spec_from_kind(DF_ViewKind_Locals), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(regs))
          {
            regs = df_view_alloc();
            df_view_equip_spec(regs, df_view_spec_from_kind(DF_ViewKind_Registers), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(globals))
          {
            globals = df_view_alloc();
            df_view_equip_spec(globals, df_view_spec_from_kind(DF_ViewKind_Globals), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(tlocals))
          {
            tlocals = df_view_alloc();
            df_view_equip_spec(tlocals, df_view_spec_from_kind(DF_ViewKind_ThreadLocals), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(types))
          {
            types = df_view_alloc();
            df_view_equip_spec(types, df_view_spec_from_kind(DF_ViewKind_Types), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(procs))
          {
            procs = df_view_alloc();
            df_view_equip_spec(procs, df_view_spec_from_kind(DF_ViewKind_Procedures), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(callstack))
          {
            callstack = df_view_alloc();
            df_view_equip_spec(callstack, df_view_spec_from_kind(DF_ViewKind_CallStack), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(breakpoints))
          {
            breakpoints = df_view_alloc();
            df_view_equip_spec(breakpoints, df_view_spec_from_kind(DF_ViewKind_Breakpoints), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(watch_pins))
          {
            watch_pins = df_view_alloc();
            df_view_equip_spec(watch_pins, df_view_spec_from_kind(DF_ViewKind_WatchPins), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(output))
          {
            output = df_view_alloc();
            df_view_equip_spec(output, df_view_spec_from_kind(DF_ViewKind_Output), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(targets))
          {
            targets = df_view_alloc();
            df_view_equip_spec(targets, df_view_spec_from_kind(DF_ViewKind_Targets), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(scheduler))
          {
            scheduler = df_view_alloc();
            df_view_equip_spec(scheduler, df_view_spec_from_kind(DF_ViewKind_Scheduler), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(modules))
          {
            modules = df_view_alloc();
            df_view_equip_spec(modules, df_view_spec_from_kind(DF_ViewKind_Modules), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(disasm))
          {
            disasm = df_view_alloc();
            df_view_equip_spec(disasm, df_view_spec_from_kind(DF_ViewKind_Disasm), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(memory))
          {
            memory = df_view_alloc();
            df_view_equip_spec(memory, df_view_spec_from_kind(DF_ViewKind_Memory), str8_zero(), &md_nil_node);
          }
          if(code_views.count == 0 && df_view_is_nil(getting_started))
          {
            getting_started = df_view_alloc();
            df_view_equip_spec(getting_started, df_view_spec_from_kind(DF_ViewKind_GettingStarted), str8_zero(), &md_nil_node);
          }
          
          //- rjf: apply layout
          switch(layout)
          {
            //- rjf: default layout
            case Layout_Default:
            {
              // rjf: root split
              ws->root_panel->split_axis = Axis2_X;
              DF_Panel *root_0 = df_panel_alloc(ws);
              DF_Panel *root_1 = df_panel_alloc(ws);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
              root_0->pct_of_parent = 0.85f;
              root_1->pct_of_parent = 0.15f;
              
              // rjf: root_0 split
              root_0->split_axis = Axis2_Y;
              DF_Panel *root_0_0 = df_panel_alloc(ws);
              DF_Panel *root_0_1 = df_panel_alloc(ws);
              df_panel_insert(root_0, root_0->last, root_0_0);
              df_panel_insert(root_0, root_0->last, root_0_1);
              root_0_0->pct_of_parent = 0.80f;
              root_0_1->pct_of_parent = 0.20f;
              
              // rjf: root_1 split
              root_1->split_axis = Axis2_Y;
              DF_Panel *root_1_0 = df_panel_alloc(ws);
              DF_Panel *root_1_1 = df_panel_alloc(ws);
              df_panel_insert(root_1, root_1->last, root_1_0);
              df_panel_insert(root_1, root_1->last, root_1_1);
              root_1_0->pct_of_parent = 0.50f;
              root_1_1->pct_of_parent = 0.50f;
              df_panel_insert_tab_view(root_1_0, root_1_0->last_tab_view, targets);
              df_panel_insert_tab_view(root_1_1, root_1_1->last_tab_view, scheduler);
              root_1_0->selected_tab_view = df_handle_from_view(targets);
              root_1_1->selected_tab_view = df_handle_from_view(scheduler);
              root_1_1->tab_side = Side_Max;
              
              // rjf: root_0_0 split
              root_0_0->split_axis = Axis2_X;
              DF_Panel *root_0_0_0 = df_panel_alloc(ws);
              DF_Panel *root_0_0_1 = df_panel_alloc(ws);
              df_panel_insert(root_0_0, root_0_0->last, root_0_0_0);
              df_panel_insert(root_0_0, root_0_0->last, root_0_0_1);
              root_0_0_0->pct_of_parent = 0.25f;
              root_0_0_1->pct_of_parent = 0.75f;
              
              // rjf: root_0_0_0 split
              root_0_0_0->split_axis = Axis2_Y;
              DF_Panel *root_0_0_0_0 = df_panel_alloc(ws);
              DF_Panel *root_0_0_0_1 = df_panel_alloc(ws);
              df_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_0);
              df_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_1);
              root_0_0_0_0->pct_of_parent = 0.5f;
              root_0_0_0_1->pct_of_parent = 0.5f;
              df_panel_insert_tab_view(root_0_0_0_0, root_0_0_0_0->last_tab_view, disasm);
              root_0_0_0_0->selected_tab_view = df_handle_from_view(disasm);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, breakpoints);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, watch_pins);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, output);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, memory);
              root_0_0_0_1->selected_tab_view = df_handle_from_view(output);
              
              // rjf: root_0_1 split
              root_0_1->split_axis = Axis2_X;
              DF_Panel *root_0_1_0 = df_panel_alloc(ws);
              DF_Panel *root_0_1_1 = df_panel_alloc(ws);
              df_panel_insert(root_0_1, root_0_1->last, root_0_1_0);
              df_panel_insert(root_0_1, root_0_1->last, root_0_1_1);
              root_0_1_0->pct_of_parent = 0.60f;
              root_0_1_1->pct_of_parent = 0.40f;
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, watch);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, locals);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, regs);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, globals);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, tlocals);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, types);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, procs);
              root_0_1_0->selected_tab_view = df_handle_from_view(watch);
              root_0_1_0->tab_side = Side_Max;
              df_panel_insert_tab_view(root_0_1_1, root_0_1_1->last_tab_view, callstack);
              df_panel_insert_tab_view(root_0_1_1, root_0_1_1->last_tab_view, modules);
              root_0_1_1->selected_tab_view = df_handle_from_view(callstack);
              root_0_1_1->tab_side = Side_Max;
              
              // rjf: fill main panel with getting started, OR all collected code views
              if(!df_view_is_nil(getting_started))
              {
                df_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, getting_started);
              }
              for(D_HandleNode *n = code_views.first; n != 0; n = n->next)
              {
                DF_View *view = df_view_from_handle(n->handle);
                if(!df_view_is_nil(view))
                {
                  df_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, view);
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
              DF_Panel *root_0 = df_panel_alloc(ws);
              DF_Panel *root_1 = df_panel_alloc(ws);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
              root_0->pct_of_parent = 0.25f;
              root_1->pct_of_parent = 0.75f;
              
              // rjf: root_0 split
              root_0->split_axis = Axis2_Y;
              DF_Panel *root_0_0 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(watch)) { df_panel_insert_tab_view(root_0_0, root_0_0->last_tab_view, watch); }
                if(!df_view_is_nil(types)) { df_panel_insert_tab_view(root_0_0, root_0_0->last_tab_view, types); }
                root_0_0->selected_tab_view = df_handle_from_view(watch);
              }
              DF_Panel *root_0_1 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(scheduler))     { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, scheduler); }
                if(!df_view_is_nil(targets))       { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, targets); }
                if(!df_view_is_nil(breakpoints))   { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, breakpoints); }
                if(!df_view_is_nil(watch_pins))    { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, watch_pins); }
                root_0_1->selected_tab_view = df_handle_from_view(scheduler);
              }
              DF_Panel *root_0_2 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(disasm))    { df_panel_insert_tab_view(root_0_2, root_0_2->last_tab_view, disasm); }
                if(!df_view_is_nil(output))    { df_panel_insert_tab_view(root_0_2, root_0_2->last_tab_view, output); }
                root_0_2->selected_tab_view = df_handle_from_view(disasm);
              }
              DF_Panel *root_0_3 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(callstack))    { df_panel_insert_tab_view(root_0_3, root_0_3->last_tab_view, callstack); }
                if(!df_view_is_nil(modules))      { df_panel_insert_tab_view(root_0_3, root_0_3->last_tab_view, modules); }
                root_0_3->selected_tab_view = df_handle_from_view(callstack);
              }
              df_panel_insert(root_0, root_0->last, root_0_0);
              df_panel_insert(root_0, root_0->last, root_0_1);
              df_panel_insert(root_0, root_0->last, root_0_2);
              df_panel_insert(root_0, root_0->last, root_0_3);
              root_0_0->pct_of_parent = 0.25f;
              root_0_1->pct_of_parent = 0.25f;
              root_0_2->pct_of_parent = 0.25f;
              root_0_3->pct_of_parent = 0.25f;
              
              // rjf: fill main panel with getting started, OR all collected code views
              if(!df_view_is_nil(getting_started))
              {
                df_panel_insert_tab_view(root_1, root_1->last_tab_view, getting_started);
              }
              for(D_HandleNode *n = code_views.first; n != 0; n = n->next)
              {
                DF_View *view = df_view_from_handle(n->handle);
                if(!df_view_is_nil(view))
                {
                  df_panel_insert_tab_view(root_1, root_1->last_tab_view, view);
                }
              }
              
              // rjf: choose initial focused panel
              ws->focused_panel = root_1;
            }break;
          }
#endif
        }break;
        
        //- rjf: filesystem fast paths
        case DF_MsgKind_Open:
        {
#if 0 // TODO(rjf): @msgs
          String8 path = regs->file_path;
          FileProperties props = os_properties_from_file_path(path);
          if(props.created != 0)
          {
            df_msg(DF_MsgKind_OpenTab,
                   .string = d_eval_string_from_file_path(scratch.arena, path),
                   .params_tree = md_tree_from_string(scratch.arena, df_view_kind_name_lower_table[DF_ViewKind_PendingFile]));
          }
          else
          {
            log_user_errorf("Couldn't open file at \"%S\".", path);
          }
#endif
        }break;
        case DF_MsgKind_Switch:
        {
          // TODO(rjf): @msgs
        }break;
        case DF_MsgKind_SwitchToPartnerFile:
        {
#if 0 // TODO(rjf): @msgs
          DF_Panel *panel = df_panel_from_handle(regs->panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          String8 file_path      = d_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
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
                df_msg(DF_MsgKind_FindCodeLocation, .file_path = candidate_path, .cursor = txt_pt(0, 0));
                break;
              }
            }
          }
#endif
        }break;
        
        //- rjf: [snapping to code locations] thread finding
        case DF_MsgKind_FindThread:
        {
          for(DF_Window *ws = df_state->first_window; ws != 0; ws = ws->next)
          {
            DI_Scope *scope = di_scope_open();
            CTRL_Entity *thread = d_regs_thread();
            U64 unwind_index = regs->unwind_count;
            U64 inline_depth = regs->inline_depth;
            if(thread->kind == D_EntityKind_Thread)
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
                df_msg(DF_MsgKind_FindCodeLocation,
                       .file_path   = line.file_path,
                       .cursor      = line.pt,
                       .vaddr_range = r1u64(rip_vaddr, rip_vaddr),
                       .voff_range  = r1u64(rip_voff, rip_voff));
              }
              
              // rjf: snap to resolved address w/o line info
              if(!missing_rip && !dbgi_pending && !has_line_info && !has_module)
              {
                df_msg(DF_MsgKind_FindCodeLocation,
                       .vaddr_range = r1u64(rip_vaddr, rip_vaddr),
                       .voff_range  = r1u64(rip_voff, rip_voff));
              }
              
              // rjf: retry on stopped, pending debug info
              if(!d_ctrl_targets_running() && (dbgi_pending || missing_rip))
              {
                df_msg(DF_MsgKind_FindThread);
              }
            }
            di_scope_close(scope);
          }
        }break;
        case DF_MsgKind_FindSelectedThread:
        {
          df_msg(DF_MsgKind_FindThread, .thread = d_base_regs()->thread);
        }break;
        
        //- rjf: [snapping to code locations] name finding
        case DF_MsgKind_GoToName:
        {
          
        }break;
        
        //- rjf: [snapping to code locations] go to code location
        case DF_MsgKind_FindCodeLocation:
        {
#if 0 // TODO(rjf): @msgs
          
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
          DF_Window *ws = df_window_from_handle(regs->window);
          
          // rjf: grab things to find. path * point, process * address, etc.
          String8 file_path = {0};
          TxtPt point = {0};
          CTRL_Entity *thread = &ctrl_entity_nil;
          CTRL_Entity *process = &ctrl_entity_nil;
          U64 vaddr = 0;
          {
            file_path = regs->file_path;
            point     = regs->cursor;
            thread    = d_regs_thread();
            process   = ctrl_entity_ancestor_from_kind(thread, CTRL_EntityKind_Process);
            vaddr     = regs->vaddr_range.min;
          }
          
          // rjf: given a src code location, and a process, if no vaddr is specified,
          // try to map the src coordinates to a vaddr via line info
          if(vaddr == 0 && file_path.size != 0 && process != &ctrl_entity_nil)
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
          DF_Panel *panel_w_this_src_code = &df_nil_panel;
          DF_View *view_w_this_src_code = &df_nil_view;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
              String8 view_file_path = d_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              if((view_kind == DF_ViewKind_Text || view_kind == DF_ViewKind_PendingFile) &&
                 path_match_normalized(view_file_path, file_path))
              {
                panel_w_this_src_code = panel;
                view_w_this_src_code = view;
                if(view == df_selected_tab_from_panel(panel))
                {
                  break;
                }
              }
            }
          }
          
          // rjf: find a panel that already has *any* code open
          DF_Panel *panel_w_any_src_code = &df_nil_panel;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              if(view_kind == DF_ViewKind_Text)
              {
                panel_w_any_src_code = panel;
                break;
              }
            }
          }
          
          // rjf: try to find panel/view pair that has disassembly open
          DF_Panel *panel_w_disasm = &df_nil_panel;
          DF_View *view_w_disasm = &df_nil_view;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              if(view_kind == DF_ViewKind_Disasm && view->query_string_size == 0)
              {
                panel_w_disasm = panel;
                view_w_disasm = view;
                if(view == df_selected_tab_from_panel(panel))
                {
                  break;
                }
              }
            }
          }
          
          // rjf: find the biggest panel
          DF_Panel *biggest_panel = &df_nil_panel;
          {
            Rng2F32 root_rect = os_client_rect_from_window(ws->os);
            F32 best_panel_area = 0;
            for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
            {
              if(!df_panel_is_nil(panel->first))
              {
                continue;
              }
              Rng2F32 panel_rect = df_target_rect_from_panel(root_rect, ws->root_panel, panel);
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
          DF_Panel *biggest_empty_panel = &df_nil_panel;
          {
            Rng2F32 root_rect = os_client_rect_from_window(ws->os);
            F32 best_panel_area = 0;
            for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
            {
              if(!df_panel_is_nil(panel->first))
              {
                continue;
              }
              Rng2F32 panel_rect = df_target_rect_from_panel(root_rect, ws->root_panel, panel);
              Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
              F32 area = panel_rect_dim.x * panel_rect_dim.y;
              B32 panel_is_empty = 1;
              for(DF_View *v = panel->first_tab_view; !df_view_is_nil(v); v = v->order_next)
              {
                if(!df_view_is_project_filtered(v))
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
          DF_Panel *panel_used_for_src_code = &df_nil_panel;
          if(file_path.size != 0)
          {
            // rjf: determine which panel we will use to find the code loc
            DF_Panel *dst_panel = &df_nil_panel;
            {
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_this_src_code; }
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_any_src_code; }
              if(df_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
              if(df_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
            }
            
            // rjf: construct new view if needed
            DF_View *dst_view = view_w_this_src_code;
            if(!df_panel_is_nil(dst_panel) && df_view_is_nil(view_w_this_src_code))
            {
              DF_View *view = df_view_alloc();
              String8 file_path_query = d_eval_string_from_file_path(scratch.arena, file_path);
              df_view_equip_spec(view, df_view_spec_from_kind(DF_ViewKind_Text), file_path_query, &md_nil_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            DF_MsgKind cursor_snap_kind = DF_MsgKind_CenterCursor;
            if(!df_panel_is_nil(dst_panel) && dst_view == view_w_this_src_code && df_selected_tab_from_panel(dst_panel) == dst_view)
            {
              cursor_snap_kind = DF_MsgKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              disasm_view_prioritized = (df_selected_tab_from_panel(dst_panel) == view_w_disasm);
              dst_panel->selected_tab_view = df_handle_from_view(dst_view);
              if(point.line != 0)
              {
                df_msg(DF_MsgKind_GoToLine,
                       .panel  = df_handle_from_panel(dst_panel),
                       .view   = df_handle_from_view(dst_view),
                       .cursor = point);
                df_msg(cursor_snap_kind);
              }
              panel_used_for_src_code = dst_panel;
            }
          }
          
          // rjf: given the above, find disassembly location.
          if(process != &ctrl_entity_nil && vaddr != 0)
          {
            // rjf: determine which panel we will use to find the disasm loc -
            // we *cannot* use the same panel we used for source code, if any.
            DF_Panel *dst_panel = &df_nil_panel;
            {
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_disasm; }
              if(df_panel_is_nil(panel_used_for_src_code) && df_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
              if(df_panel_is_nil(panel_used_for_src_code) && df_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
              if(dst_panel == panel_used_for_src_code &&
                 !disasm_view_prioritized)
              {
                dst_panel = &df_nil_panel;
              }
            }
            
            // rjf: construct new view if needed
            DF_View *dst_view = view_w_disasm;
            if(!df_panel_is_nil(dst_panel) && df_view_is_nil(view_w_disasm))
            {
              DF_View *view = df_view_alloc();
              df_view_equip_spec(view, df_view_spec_from_kind(DF_ViewKind_Disasm), str8_zero(), &md_nil_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            DF_MsgKind cursor_snap_kind = DF_MsgKind_CenterCursor;
            if(dst_view == view_w_disasm && df_selected_tab_from_panel(dst_panel) == dst_view)
            {
              cursor_snap_kind = DF_MsgKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              dst_panel->selected_tab_view = df_handle_from_view(dst_view);
              df_msg(DF_MsgKind_GoToAddress,
                     .panel       = df_handle_from_panel(dst_panel),
                     .view        = df_handle_from_view(dst_view),
                     .machine_id  = process->machine_id,
                     .process     = process->handle,
                     .vaddr_range = r1u64(vaddr, vaddr));
              df_msg(cursor_snap_kind);
            }
          }
#endif
        }break;
      }
    }
    arena_clear(df_state->msgs_arena);
    MemoryZeroStruct(&df_state->msgs);
  }
  
  //////////////////////////////
  //- rjf: tick debug engine
  //
  d_tick(scratch.arena, di_scope, dt);
  
  //////////////////////////////
  //- rjf: animate confirmation
  //
  {
    F32 rate = df_setting_val_from_code(DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-10.f * d_dt())) : 1.f;
    B32 confirm_open = df_state->confirm_active;
    df_state->confirm_t += rate * ((F32)!!confirm_open-df_state->confirm_t);
    if(abs_f32(df_state->confirm_t - (F32)!!confirm_open) > 0.005f)
    {
      df_request_frame();
    }
  }
  
  //////////////////////////////
  //- rjf: animate theme
  //
  {
    DF_Theme *current = &df_state->cfg_theme;
    DF_Theme *target = &df_state->cfg_theme_target;
    F32 rate = 1 - pow_f32(2, (-50.f * d_dt()));
    for(DF_ThemeColor color = DF_ThemeColor_Null;
        color < DF_ThemeColor_COUNT;
        color = (DF_ThemeColor)(color+1))
    {
      if(abs_f32(target->colors[color].x - current->colors[color].x) > 0.01f ||
         abs_f32(target->colors[color].y - current->colors[color].y) > 0.01f ||
         abs_f32(target->colors[color].z - current->colors[color].z) > 0.01f ||
         abs_f32(target->colors[color].w - current->colors[color].w) > 0.01f)
      {
        df_request_frame();
      }
      current->colors[color].x += (target->colors[color].x - current->colors[color].x) * rate;
      current->colors[color].y += (target->colors[color].y - current->colors[color].y) * rate;
      current->colors[color].z += (target->colors[color].z - current->colors[color].z) * rate;
      current->colors[color].w += (target->colors[color].w - current->colors[color].w) * rate;
    }
  }
  
  //////////////////////////////
  //- rjf: animate alive-transitions for entities
  //
#if 0 // TODO(rjf): @msgs
  {
    F32 rate = 1.f - pow_f32(2.f, -20.f*d_dt());
    for(D_Entity *e = d_entity_root(); !d_entity_is_nil(e); e = d_entity_rec_depth_first_pre(e, d_entity_root()).next)
    {
      F32 diff = (1.f - e->alive_t);
      e->alive_t += diff * rate;
      if(diff >= 0.01f)
      {
        df_request_frame();
      }
    }
  }
#endif
  
  //////////////////////////////
  //- rjf: capture is active? -> keep rendering
  //
  if(ProfIsCapturing())
  {
    df_request_frame();
  }
  
  //////////////////////////////
  //- rjf: commit params changes for all views
  //
#if 0 // TODO(rjf): @msgs
  {
    for(DF_View *v = df_state->first_view; !df_view_is_nil(v); v = v->alloc_next)
    {
      if(v->params_write_gen == v->params_read_gen+1)
      {
        v->params_read_gen += 1;
      }
    }
  }
#endif
  
  //////////////////////////////
  //- rjf: process top-level graphical commands
  //
#if 0 // TODO(rjf): @msgs
  B32 panel_reset_done = 0;
  {
    B32 cfg_write_done[D_CfgSrc_COUNT] = {0};
    for(D_CmdNode *cmd_node = cmds.first;
        cmd_node != 0;
        cmd_node = cmd_node->next)
    {
      // rjf: unpack command
      D_Cmd *      cmd    = &cmd_node->cmd;
      D_CmdParams *params = &cmd->params;
      D_CmdKind    kind   = d_cmd_kind_from_string(cmd->spec->info.string);
      
      // rjf: request frame
      df_request_frame();
      
      // rjf: process command
      Dir2 split_dir = Dir2_Invalid;
      DF_Panel *split_panel = &df_nil_panel;
      U64 panel_sib_off = 0;
      U64 panel_child_off = 0;
      Vec2S32 panel_change_dir = {0};
      switch(kind)
      {
        //- rjf: default -> try to open tabs for "view driver" commands
        default:
        {
          String8 name = cmd->spec->info.string;
          DF_ViewSpec *view_spec = df_view_spec_from_string(name);
          if(view_spec != &df_nil_view_spec)
          {
            D_CmdParams p = *params;
            p.view_spec = view_spec;
            d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_OpenTab));
          }
        }break;
        
        //- rjf: command fast path
        case D_CmdKind_RunCommand:
        {
          D_CmdSpec *spec = params->cmd_spec;
          
          // rjf: command simply executes - just no-op in this layer
          if(!d_cmd_spec_is_nil(spec) && !(spec->info.query.flags & D_CmdQueryFlag_Required))
          {
          }
          
          // rjf: command has required query -> prep query
          else
          {
            DF_Window *window = df_window_from_handle(params->window);
            if(window != 0)
            {
              arena_clear(window->query_cmd_arena);
              window->query_cmd_spec   = d_cmd_spec_is_nil(spec) ? cmd->spec : spec;
              window->query_cmd_params = df_cmd_params_copy(window->query_cmd_arena, params);
              MemoryZeroArray(window->query_cmd_params_mask);
              window->query_view_selected = 1;
            }
          }
        }break;
        
        //- rjf: exiting
        case D_CmdKind_Exit:
        {
          // rjf: save
          {
            d_cmd(D_CmdKind_WriteUserData);
            d_cmd(D_CmdKind_WriteProjectData);
            df_state->last_window_queued_save = 1;
          }
          
          // rjf: close all windows
          for(DF_Window *window = df_state->first_window; window != 0; window = window->next)
          {
            d_cmd(D_CmdKind_CloseWindow, .window = df_handle_from_window(window));
          }
        }break;
        
        //- rjf: errors
        case D_CmdKind_Error:
        {
          for(DF_Window *w = df_state->first_window; w != 0; w = w->next)
          {
            String8 error_string = params->string;
            w->error_string_size = error_string.size;
            MemoryCopy(w->error_buffer, error_string.str, Min(sizeof(w->error_buffer), error_string.size));
            w->error_t = 1;
          }
        }break;
        
        //- rjf: windows
        case D_CmdKind_OpenWindow:
        {
          DF_Window *originating_window = df_window_from_handle(params->window);
          if(originating_window == 0)
          {
            originating_window = df_state->first_window;
          }
          OS_Handle preferred_monitor = {0};
          DF_Window *new_ws = df_window_open(v2f32(1280, 720), preferred_monitor, D_CfgSrc_User);
          if(originating_window)
          {
            MemoryCopy(new_ws->setting_vals, originating_window->setting_vals, sizeof(DF_SettingVal)*DF_SettingCode_COUNT);
          }
        }break;
        case D_CmdKind_CloseWindow:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          if(ws != 0)
          {
            D_EntityList running_processes = d_query_cached_entity_list_with_kind(D_EntityKind_Process);
            
            // NOTE(rjf): if this is the last window, and targets are running, but
            // this command is not force-confirmed, then we should query the user
            // to ensure they want to close the debugger before exiting
            UI_Key key = ui_key_from_string(ui_key_zero(), str8_lit("lossy_exit_confirmation"));
            if(!ui_key_match(key, df_state->confirm_key) && running_processes.count != 0 && ws == df_state->first_window && ws == df_state->last_window && !params->force_confirm)
            {
              df_state->confirm_key = key;
              df_state->confirm_active = 1;
              arena_clear(df_state->confirm_arena);
              MemoryZeroStruct(&df_state->confirm_cmds);
              df_state->confirm_title = push_str8f(df_state->confirm_arena, "Are you sure you want to exit?");
              df_state->confirm_desc = push_str8f(df_state->confirm_arena, "The debugger is still attached to %slive process%s.",
                                                  running_processes.count == 1 ? "a " : "",
                                                  running_processes.count == 1 ? ""   : "es");
              D_CmdParams p = df_cmd_params_from_window(ws);
              p.force_confirm = 1;
              d_cmd_list_push(df_state->confirm_arena, &df_state->confirm_cmds, &p, d_cmd_spec_from_kind(D_CmdKind_CloseWindow));
            }
            
            // NOTE(rjf): if this is the last window, and it is being closed, then
            // we need to auto-save, and provide one last chance to process saving
            // commands. after doing so, we can retry.
            else if(ws == df_state->first_window && ws == df_state->last_window && df_state->last_window_queued_save == 0)
            {
              df_state->last_window_queued_save = 1;
              d_cmd(D_CmdKind_WriteUserData);
              d_cmd(D_CmdKind_WriteProjectData);
              d_cmd(D_CmdKind_CloseWindow, .force_confirm = 1, .window = df_handle_from_window(ws));
            }
            
            // NOTE(rjf): if this is the last window and we've queued the final autosave,
            // or if it's not the last window, then we're free to release everything.
            else
            {
              // NOTE(rjf): we need to explicitly release all panel views, because views
              // are a global concept and otherwise would leak.
              for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
              {
                df_panel_release_all_views(panel);
              }
              
              ui_state_release(ws->ui);
              DLLRemove(df_state->first_window, df_state->last_window, ws);
              r_window_unequip(ws->os, ws->r);
              os_window_close(ws->os);
              arena_release(ws->query_cmd_arena);
              arena_release(ws->code_ctx_menu_arena);
              arena_release(ws->hover_eval_arena);
              arena_release(ws->autocomp_lister_params_arena);
              arena_release(ws->arena);
              SLLStackPush(df_state->free_window, ws);
              ws->gen += 1;
            }
          }
        }break;
        case D_CmdKind_ToggleFullscreen:
        {
          DF_Window *window = df_window_from_handle(params->window);
          if(window != 0)
          {
            os_window_set_fullscreen(window->os, !os_window_is_fullscreen(window->os));
          }
        }break;
        
        //- rjf: confirmations
        case D_CmdKind_ConfirmAccept:
        {
          df_state->confirm_active = 0;
          df_state->confirm_key = ui_key_zero();
          for(D_CmdNode *n = df_state->confirm_cmds.first; n != 0; n = n->next)
          {
            d_push_cmd(n->cmd.spec, &n->cmd.params);
          }
        }break;
        case D_CmdKind_ConfirmCancel:
        {
          df_state->confirm_active = 0;
          df_state->confirm_key = ui_key_zero();
        }break;
        
        //- rjf: debug control context management operations
        case D_CmdKind_SelectThread:goto thread_locator;
        case D_CmdKind_SelectUnwind:
        thread_locator:;
        {
          d_cmd_list_push(scratch.arena, &cmds, params, d_cmd_spec_from_kind(D_CmdKind_FindThread));
        }break;
        
        //- rjf: loading/applying stateful config changes
        case D_CmdKind_ApplyUserData:
        case D_CmdKind_ApplyProjectData:
        {
          D_CfgTable *table = d_cfg_table();
          OS_HandleArray monitors = os_push_monitors_array(scratch.arena);
          
          //- rjf: get src
          D_CfgSrc src = D_CfgSrc_User;
          for(D_CfgSrc s = (D_CfgSrc)0; s < D_CfgSrc_COUNT; s = (D_CfgSrc)(s+1))
          {
            if(kind == d_cfg_src_apply_cmd_kind_table[s])
            {
              src = s;
              break;
            }
          }
          
          //- rjf: get paths
          String8 cfg_path   = d_cfg_path_from_src(src);
          String8 cfg_folder = str8_chop_last_slash(cfg_path);
          
          //- rjf: eliminate all windows
          for(DF_Window *window = df_state->first_window; window != 0; window = window->next)
          {
            if(window->cfg_src != src)
            {
              continue;
            }
            d_cmd(D_CmdKind_CloseWindow, .window = df_handle_from_window(window));
          }
          
          //- rjf: apply fonts
          {
            FNT_Tag defaults[DF_FontSlot_COUNT] =
            {
              fnt_tag_from_static_data_string(&df_g_default_main_font_bytes),
              fnt_tag_from_static_data_string(&df_g_default_code_font_bytes),
              fnt_tag_from_static_data_string(&df_g_icon_font_bytes),
            };
            MemoryZeroArray(df_state->cfg_font_tags);
            {
              D_CfgVal *code_font_val = d_cfg_val_from_string(table, str8_lit("code_font"));
              D_CfgVal *main_font_val = d_cfg_val_from_string(table, str8_lit("main_font"));
              MD_Node *code_font_node = code_font_val->last->root;
              MD_Node *main_font_node = main_font_val->last->root;
              String8 code_font_relative_path = code_font_node->first->string;
              String8 main_font_relative_path = main_font_node->first->string;
              if(!md_node_is_nil(code_font_node))
              {
                arena_clear(df_state->cfg_code_font_path_arena);
                df_state->cfg_code_font_path = push_str8_copy(df_state->cfg_code_font_path_arena, code_font_relative_path);
              }
              if(!md_node_is_nil(main_font_node))
              {
                arena_clear(df_state->cfg_main_font_path_arena);
                df_state->cfg_main_font_path = push_str8_copy(df_state->cfg_main_font_path_arena, main_font_relative_path);
              }
              String8 code_font_path = path_absolute_dst_from_relative_dst_src(scratch.arena, code_font_relative_path, cfg_folder);
              String8 main_font_path = path_absolute_dst_from_relative_dst_src(scratch.arena, main_font_relative_path, cfg_folder);
              if(os_file_path_exists(code_font_path) && !md_node_is_nil(code_font_node) && code_font_relative_path.size != 0)
              {
                df_state->cfg_font_tags[DF_FontSlot_Code] = fnt_tag_from_path(code_font_path);
              }
              if(os_file_path_exists(main_font_path) && !md_node_is_nil(main_font_node) && main_font_relative_path.size != 0)
              {
                df_state->cfg_font_tags[DF_FontSlot_Main] = fnt_tag_from_path(main_font_path);
              }
            }
            for(DF_FontSlot slot = (DF_FontSlot)0; slot < DF_FontSlot_COUNT; slot = (DF_FontSlot)(slot+1))
            {
              if(fnt_tag_match(fnt_tag_zero(), df_state->cfg_font_tags[slot]))
              {
                df_state->cfg_font_tags[slot] = defaults[slot];
              }
            }
          }
          
          //- rjf: build windows & panel layouts
          D_CfgVal *windows = d_cfg_val_from_string(table, str8_lit("window"));
          for(D_CfgTree *window_tree = windows->first;
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
            DF_SettingVal setting_vals[DF_SettingCode_COUNT] = {0};
            {
              for(MD_EachNode(n, window_tree->root->first))
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
              for(EachEnumVal(DF_SettingCode, code))
              {
                MD_Node *code_node = md_child_from_string(window_tree->root, df_g_setting_code_lower_string_table[code], 0);
                if(!md_node_is_nil(code_node))
                {
                  S64 val_s64 = 0;
                  try_s64_from_str8_c_rules(code_node->first->string, &val_s64);
                  setting_vals[code].set = 1;
                  setting_vals[code].s32 = (S32)val_s64;
                  setting_vals[code].s32 = clamp_1s32(df_g_setting_code_s32_range_table[code], setting_vals[code].s32);
                }
              }
            }
            
            // rjf: open window
            DF_Window *ws = df_window_open(size, preferred_monitor, window_tree->source);
            if(dpi != 0.f) { ws->last_dpi = dpi; }
            for(EachEnumVal(DF_SettingCode, code))
            {
              if(setting_vals[code].set == 0 && df_g_setting_code_default_is_per_window_table[code])
              {
                setting_vals[code] = df_g_setting_code_default_val_table[code];
              }
            }
            MemoryCopy(ws->setting_vals, setting_vals, sizeof(setting_vals[0])*ArrayCount(setting_vals));
            
            // rjf: build panel tree
            MD_Node *panel_tree = md_child_from_string(window_tree->root, str8_lit("panels"), 0);
            DF_Panel *panel_parent = ws->root_panel;
            panel_parent->split_axis = top_level_split_axis;
            MD_NodeRec rec = {0};
            for(MD_Node *n = panel_tree, *next = &md_nil_node;
                !md_node_is_nil(n);
                n = next)
            {
              // rjf: assume we're just moving to the next one initially...
              next = n->next;
              
              // rjf: grab root panel
              DF_Panel *panel = &df_nil_panel;
              if(n == panel_tree)
              {
                panel = ws->root_panel;
                panel->pct_of_parent = 1.f;
              }
              
              // rjf: allocate & insert non-root panels - these will have a numeric string, determining
              // pct of parent
              if(n->flags & MD_NodeFlag_Numeric)
              {
                panel = df_panel_alloc(ws);
                df_panel_insert(panel_parent, panel_parent->last, panel);
                panel->split_axis = axis2_flip(panel_parent->split_axis);
                panel->pct_of_parent = (F32)f64_from_str8(n->string);
              }
              
              // rjf: do general per-panel work
              if(!df_panel_is_nil(panel))
              {
                // rjf: determine if this panel has panel children
                B32 has_panel_children = 0;
                for(MD_EachNode(child, n->first))
                {
                  if(child->flags & MD_NodeFlag_Numeric)
                  {
                    has_panel_children = 1;
                    break;
                  }
                }
                
                // rjf: apply panel options
                for(MD_EachNode(op, n->first))
                {
                  if(md_node_is_nil(op->first) && str8_match(op->string, str8_lit("tabs_on_bottom"), 0))
                  {
                    panel->tab_side = Side_Max;
                  }
                }
                
                // rjf: apply panel views/tabs/commands
                DF_View *selected_view = &df_nil_view;
                for(MD_EachNode(op, n->first))
                {
                  DF_ViewSpec *view_spec = df_view_spec_from_string(op->string);
                  if(view_spec == &df_nil_view_spec || has_panel_children != 0)
                  {
                    continue;
                  }
                  
                  // rjf: allocate view & apply view-specific parameterizations
                  DF_View *view = &df_nil_view;
                  B32 view_is_selected = 0;
                  DF_ViewSpecFlags view_spec_flags = view_spec->info.flags;
                  if(view_spec_flags & DF_ViewSpecFlag_CanSerialize)
                  {
                    // rjf: allocate view
                    view = df_view_alloc();
                    
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
                      view_query = d_cfg_raw_from_escaped_string(scratch.arena, escaped_query);
                    }
                    
                    // rjf: convert file queries from relative to absolute
                    {
                      String8 query_file_path = d_file_path_from_eval_string(scratch.arena, view_query);
                      if(query_file_path.size != 0)
                      {
                        query_file_path = path_absolute_dst_from_relative_dst_src(scratch.arena, query_file_path, cfg_folder);
                        view_query = push_str8f(scratch.arena, "file:\"%S\"", query_file_path);
                      }
                    }
                    
                    // rjf: set up view
                    df_view_equip_spec(view, view_spec, view_query, op);
                    if(project_path.size != 0)
                    {
                      arena_clear(view->project_path_arena);
                      view->project_path = push_str8_copy(view->project_path_arena, project_path);
                    }
                  }
                  
                  // rjf: insert
                  if(!df_view_is_nil(view))
                  {
                    df_panel_insert_tab_view(panel, panel->last_tab_view, view);
                    if(view_is_selected)
                    {
                      selected_view = view;
                    }
                  }
                }
                
                // rjf: select selected view
                if(!df_view_is_nil(selected_view))
                {
                  panel->selected_tab_view = df_handle_from_view(selected_view);
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
              DF_Panel *best_leaf_panel = &df_nil_panel;
              F32 best_leaf_panel_area = 0;
              Rng2F32 root_rect = r2f32p(0, 0, 1000, 1000); // NOTE(rjf): we can assume any size - just need proportions.
              for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
              {
                if(df_panel_is_nil(panel->first))
                {
                  Rng2F32 rect = df_target_rect_from_panel(root_rect, ws->root_panel, panel);
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
          if(src == D_CfgSrc_User)
          {
            df_clear_bindings();
          }
          D_CfgVal *keybindings = d_cfg_val_from_string(table, str8_lit("keybindings"));
          for(D_CfgTree *keybinding_set = keybindings->first;
              keybinding_set != &d_nil_cfg_tree;
              keybinding_set = keybinding_set->next)
          {
            for(MD_EachNode(keybind, keybinding_set->root->first))
            {
              D_CmdSpec *cmd_spec = &d_nil_cmd_spec;
              OS_Key key = OS_Key_Null;
              MD_Node *ctrl_node = &md_nil_node;
              MD_Node *shift_node = &md_nil_node;
              MD_Node *alt_node = &md_nil_node;
              for(MD_EachNode(child, keybind->first))
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
                  D_CmdSpec *spec = d_cmd_spec_from_string(child->string);
                  for(U64 idx = 0; idx < ArrayCount(df_g_binding_version_remap_old_name_table); idx += 1)
                  {
                    if(str8_match(df_g_binding_version_remap_old_name_table[idx], child->string, StringMatchFlag_CaseInsensitive))
                    {
                      String8 new_name = df_g_binding_version_remap_new_name_table[idx];
                      spec = d_cmd_spec_from_string(new_name);
                    }
                  }
                  
                  if(!d_cmd_spec_is_nil(spec))
                  {
                    cmd_spec = spec;
                  }
                  OS_Key k = os_key_from_string(child->string);
                  if(k != OS_Key_Null)
                  {
                    key = k;
                  }
                }
              }
              if(!d_cmd_spec_is_nil(cmd_spec) && key != OS_Key_Null)
              {
                OS_EventFlags flags = 0;
                if(!md_node_is_nil(ctrl_node))  { flags |= OS_EventFlag_Ctrl; }
                if(!md_node_is_nil(shift_node)) { flags |= OS_EventFlag_Shift; }
                if(!md_node_is_nil(alt_node))   { flags |= OS_EventFlag_Alt; }
                DF_Binding binding = {key, flags};
                df_bind_spec(cmd_spec, binding);
              }
            }
          }
          
          //- rjf: reset theme to default
          MemoryCopy(df_state->cfg_theme_target.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
          MemoryCopy(df_state->cfg_theme.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
          
          //- rjf: apply theme presets
          D_CfgVal *color_preset = d_cfg_val_from_string(table, str8_lit("color_preset"));
          B32 preset_applied = 0;
          if(color_preset != &d_nil_cfg_val)
          {
            String8 color_preset_name = color_preset->last->root->first->string;
            DF_ThemePreset preset = (DF_ThemePreset)0;
            B32 found_preset = 0;
            for(DF_ThemePreset p = (DF_ThemePreset)0; p < DF_ThemePreset_COUNT; p = (DF_ThemePreset)(p+1))
            {
              if(str8_match(color_preset_name, df_g_theme_preset_code_string_table[p], StringMatchFlag_CaseInsensitive))
              {
                found_preset = 1;
                preset = p;
                break;
              }
            }
            if(found_preset)
            {
              preset_applied = 1;
              MemoryCopy(df_state->cfg_theme_target.colors, df_g_theme_preset_colors_table[preset], sizeof(df_g_theme_preset_colors__default_dark));
              MemoryCopy(df_state->cfg_theme.colors, df_g_theme_preset_colors_table[preset], sizeof(df_g_theme_preset_colors__default_dark));
            }
          }
          
          //- rjf: apply individual theme colors
          B8 theme_color_hit[DF_ThemeColor_COUNT] = {0};
          D_CfgVal *colors = d_cfg_val_from_string(table, str8_lit("colors"));
          for(D_CfgTree *colors_set = colors->first;
              colors_set != &d_nil_cfg_tree;
              colors_set = colors_set->next)
          {
            for(MD_EachNode(color, colors_set->root->first))
            {
              String8 saved_color_name = color->string;
              String8List candidate_color_names = {0};
              str8_list_push(scratch.arena, &candidate_color_names, saved_color_name);
              for(U64 idx = 0; idx < ArrayCount(df_g_theme_color_version_remap_old_name_table); idx += 1)
              {
                if(str8_match(df_g_theme_color_version_remap_old_name_table[idx], saved_color_name, StringMatchFlag_CaseInsensitive))
                {
                  str8_list_push(scratch.arena, &candidate_color_names, df_g_theme_color_version_remap_new_name_table[idx]);
                }
              }
              for(String8Node *name_n = candidate_color_names.first; name_n != 0; name_n = name_n->next)
              {
                String8 name = name_n->string;
                DF_ThemeColor color_code = DF_ThemeColor_Null;
                for(DF_ThemeColor c = DF_ThemeColor_Null; c < DF_ThemeColor_COUNT; c = (DF_ThemeColor)(c+1))
                {
                  if(str8_match(df_g_theme_color_cfg_string_table[c], name, StringMatchFlag_CaseInsensitive))
                  {
                    color_code = c;
                    break;
                  }
                }
                if(color_code != DF_ThemeColor_Null)
                {
                  theme_color_hit[color_code] = 1;
                  MD_Node *hex_cfg = color->first;
                  String8 hex_string = hex_cfg->string;
                  U64 hex_val = 0;
                  try_u64_from_str8_c_rules(hex_string, &hex_val);
                  Vec4F32 color_rgba = rgba_from_u32((U32)hex_val);
                  df_state->cfg_theme_target.colors[color_code] = color_rgba;
                  if(d_frame_index() <= 2)
                  {
                    df_state->cfg_theme.colors[color_code] = color_rgba;
                  }
                }
              }
            }
          }
          
          //- rjf: no preset -> autofill all missing colors from the preset with the most similar background
          if(!preset_applied)
          {
            DF_ThemePreset closest_preset = DF_ThemePreset_DefaultDark;
            F32 closest_preset_bg_distance = 100000000;
            for(DF_ThemePreset p = (DF_ThemePreset)0; p < DF_ThemePreset_COUNT; p = (DF_ThemePreset)(p+1))
            {
              Vec4F32 cfg_bg = df_state->cfg_theme_target.colors[DF_ThemeColor_BaseBackground];
              Vec4F32 pre_bg = df_g_theme_preset_colors_table[p][DF_ThemeColor_BaseBackground];
              Vec4F32 diff = sub_4f32(cfg_bg, pre_bg);
              Vec3F32 diff3 = diff.xyz;
              F32 distance = length_3f32(diff3);
              if(distance < closest_preset_bg_distance)
              {
                closest_preset = p;
                closest_preset_bg_distance = distance;
              }
            }
            for(DF_ThemeColor c = (DF_ThemeColor)(DF_ThemeColor_Null+1);
                c < DF_ThemeColor_COUNT;
                c = (DF_ThemeColor)(c+1))
            {
              if(!theme_color_hit[c])
              {
                df_state->cfg_theme_target.colors[c] = df_state->cfg_theme.colors[c] = df_g_theme_preset_colors_table[closest_preset][c];
              }
            }
          }
          
          //- rjf: if theme colors are all zeroes, then set to default - config appears busted
          {
            B32 all_colors_are_zero = 1;
            Vec4F32 zero_color = {0};
            for(DF_ThemeColor c = (DF_ThemeColor)(DF_ThemeColor_Null+1); c < DF_ThemeColor_COUNT; c = (DF_ThemeColor)(c+1))
            {
              if(!MemoryMatchStruct(&df_state->cfg_theme_target.colors[c], &zero_color))
              {
                all_colors_are_zero = 0;
                break;
              }
            }
            if(all_colors_are_zero)
            {
              MemoryCopy(df_state->cfg_theme_target.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
              MemoryCopy(df_state->cfg_theme.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
            }
          }
          
          //- rjf: apply settings
          B8 setting_codes_hit[DF_SettingCode_COUNT] = {0};
          MemoryZero(&df_state->cfg_setting_vals[src][0], sizeof(DF_SettingVal)*DF_SettingCode_COUNT);
          for(EachEnumVal(DF_SettingCode, code))
          {
            String8 name = df_g_setting_code_lower_string_table[code];
            D_CfgVal *code_cfg_val = d_cfg_val_from_string(table, name);
            D_CfgTree *code_tree = code_cfg_val->last;
            if(code_tree->source == src)
            {
              MD_Node *val_node = code_tree->root->first;
              S64 val = 0;
              if(try_s64_from_str8_c_rules(val_node->string, &val))
              {
                df_state->cfg_setting_vals[src][code].set = 1;
                df_state->cfg_setting_vals[src][code].s32 = (S32)val;
              }
              setting_codes_hit[code] = !md_node_is_nil(val_node);
            }
          }
          
          //- rjf: if config applied 0 settings, we need to do some sensible default
          if(src == D_CfgSrc_User)
          {
            for(EachEnumVal(DF_SettingCode, code))
            {
              if(!setting_codes_hit[code])
              {
                df_state->cfg_setting_vals[src][code] = df_g_setting_code_default_val_table[code];
              }
            }
          }
          
          //- rjf: if config opened 0 windows, we need to do some sensible default
          if(src == D_CfgSrc_User && windows->first == &d_nil_cfg_tree)
          {
            OS_Handle preferred_monitor = os_primary_monitor();
            Vec2F32 monitor_dim = os_dim_from_monitor(preferred_monitor);
            Vec2F32 window_dim = v2f32(monitor_dim.x*4/5, monitor_dim.y*4/5);
            DF_Window *ws = df_window_open(window_dim, preferred_monitor, D_CfgSrc_User);
            D_CmdParams blank_params = df_cmd_params_from_window(ws);
            if(monitor_dim.x < 1920)
            {
              d_cmd_list_push(scratch.arena, &cmds, &blank_params, d_cmd_spec_from_kind(D_CmdKind_ResetToCompactPanels));
            }
            else
            {
              d_cmd_list_push(scratch.arena, &cmds, &blank_params, d_cmd_spec_from_kind(D_CmdKind_ResetToDefaultPanels));
            }
          }
          
          //- rjf: if config bound 0 keys, we need to do some sensible default
          if(src == D_CfgSrc_User && df_state->key_map_total_count == 0)
          {
            for(U64 idx = 0; idx < ArrayCount(df_g_default_binding_table); idx += 1)
            {
              DF_StringBindingPair *pair = &df_g_default_binding_table[idx];
              D_CmdSpec *cmd_spec = d_cmd_spec_from_string(pair->string);
              df_bind_spec(cmd_spec, pair->binding);
            }
          }
          
          //- rjf: always ensure that the meta controls have bindings
          if(src == D_CfgSrc_User)
          {
            struct
            {
              D_CmdSpec *spec;
              OS_Key fallback_key;
            }
            meta_ctrls[] =
            {
              { d_cmd_spec_from_kind(D_CmdKind_Edit), OS_Key_F2 },
              { d_cmd_spec_from_kind(D_CmdKind_Accept), OS_Key_Return },
              { d_cmd_spec_from_kind(D_CmdKind_Cancel), OS_Key_Esc },
            };
            for(U64 idx = 0; idx < ArrayCount(meta_ctrls); idx += 1)
            {
              DF_BindingList bindings = df_bindings_from_spec(scratch.arena, meta_ctrls[idx].spec);
              if(bindings.count == 0)
              {
                DF_Binding binding = {meta_ctrls[idx].fallback_key, 0};
                df_bind_spec(meta_ctrls[idx].spec, binding);
              }
            }
          }
        }break;
        
        //- rjf: writing config changes
        case D_CmdKind_WriteUserData:
        case D_CmdKind_WriteProjectData:
        {
          D_CfgSrc src = D_CfgSrc_User;
          for(D_CfgSrc s = (D_CfgSrc)0; s < D_CfgSrc_COUNT; s = (D_CfgSrc)(s+1))
          {
            if(kind == d_cfg_src_write_cmd_kind_table[s])
            {
              src = s;
              break;
            }
          }
          if(cfg_write_done[src] == 0)
          {
            cfg_write_done[src] = 1;
            String8 path = d_cfg_path_from_src(src);
            String8List strs = df_cfg_strings_from_state(scratch.arena, path, src);
            String8 data = str8_list_join(scratch.arena, &strs, 0);
            String8 data_indented = indented_from_string(scratch.arena, data);
            d_cfg_push_write_string(src, data_indented);
          }
        }break;
        
        //- rjf: code navigation
        case D_CmdKind_FindTextForward:
        case D_CmdKind_FindTextBackward:
        {
          df_set_search_string(params->string);
        }break;
        
        //- rjf: find next and find prev
        case D_CmdKind_FindNext:
        {
          D_CmdParams p = *params;
          p.string = df_push_search_string(scratch.arena);
          d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_FindTextForward));
        }break;
        case D_CmdKind_FindPrev:
        {
          D_CmdParams p = *params;
          p.string = df_push_search_string(scratch.arena);
          d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_FindTextBackward));
        }break;
        
        //- rjf: font sizes
        case D_CmdKind_IncUIFontScale:
        {
          DF_Window *window = df_window_from_handle(params->window);
          if(window != 0)
          {
            window->setting_vals[DF_SettingCode_MainFontSize].set = 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 += 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_MainFontSize], window->setting_vals[DF_SettingCode_MainFontSize].s32);
          }
        }break;
        case D_CmdKind_DecUIFontScale:
        {
          DF_Window *window = df_window_from_handle(params->window);
          if(window != 0)
          {
            window->setting_vals[DF_SettingCode_MainFontSize].set = 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 -= 1;
            window->setting_vals[DF_SettingCode_MainFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_MainFontSize], window->setting_vals[DF_SettingCode_MainFontSize].s32);
          }
        }break;
        case D_CmdKind_IncCodeFontScale:
        {
          DF_Window *window = df_window_from_handle(params->window);
          if(window != 0)
          {
            window->setting_vals[DF_SettingCode_CodeFontSize].set = 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 += 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_CodeFontSize], window->setting_vals[DF_SettingCode_CodeFontSize].s32);
          }
        }break;
        case D_CmdKind_DecCodeFontScale:
        {
          DF_Window *window = df_window_from_handle(params->window);
          if(window != 0)
          {
            window->setting_vals[DF_SettingCode_CodeFontSize].set = 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 -= 1;
            window->setting_vals[DF_SettingCode_CodeFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_CodeFontSize], window->setting_vals[DF_SettingCode_CodeFontSize].s32);
          }
        }break;
        
        //- rjf: panel creation
        case D_CmdKind_NewPanelLeft: {split_dir = Dir2_Left;}goto split;
        case D_CmdKind_NewPanelUp:   {split_dir = Dir2_Up;}goto split;
        case D_CmdKind_NewPanelRight:{split_dir = Dir2_Right;}goto split;
        case D_CmdKind_NewPanelDown: {split_dir = Dir2_Down;}goto split;
        case D_CmdKind_SplitPanel:
        {
          split_dir = params->dir2;
          split_panel = df_panel_from_handle(params->dest_panel);
        }goto split;
        split:;
        if(split_dir != Dir2_Invalid)
        {
          DF_Window *ws = df_window_from_handle(params->window);
          if(df_panel_is_nil(split_panel))
          {
            split_panel = ws->focused_panel;
          }
          DF_Panel *new_panel = &df_nil_panel;
          Axis2 split_axis = axis2_from_dir2(split_dir);
          Side split_side = side_from_dir2(split_dir);
          DF_Panel *panel = split_panel;
          DF_Panel *parent = panel->parent;
          if(!df_panel_is_nil(parent) && parent->split_axis == split_axis)
          {
            DF_Panel *next = df_panel_alloc(ws);
            df_panel_insert(parent, split_side == Side_Max ? panel : panel->prev, next);
            next->pct_of_parent = 1.f/parent->child_count;
            for(DF_Panel *child = parent->first; !df_panel_is_nil(child); child = child->next)
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
            DF_Panel *pre_prev = panel->prev;
            DF_Panel *pre_parent = parent;
            DF_Panel *new_parent = df_panel_alloc(ws);
            new_parent->pct_of_parent = panel->pct_of_parent;
            if(!df_panel_is_nil(pre_parent))
            {
              df_panel_remove(pre_parent, panel);
              df_panel_insert(pre_parent, pre_prev, new_parent);
            }
            else
            {
              ws->root_panel = new_parent;
            }
            DF_Panel *left = panel;
            DF_Panel *right = df_panel_alloc(ws);
            new_panel = right;
            if(split_side == Side_Min)
            {
              Swap(DF_Panel *, left, right);
            }
            df_panel_insert(new_parent, &df_nil_panel, left);
            df_panel_insert(new_parent, left, right);
            new_parent->split_axis = split_axis;
            left->pct_of_parent = 0.5f;
            right->pct_of_parent = 0.5f;
            ws->focused_panel = new_panel;
          }
          if(!df_panel_is_nil(new_panel->prev))
          {
            Rng2F32 prev_rect_pct = new_panel->prev->animated_rect_pct;
            new_panel->animated_rect_pct = prev_rect_pct;
            new_panel->animated_rect_pct.p0.v[split_axis] = new_panel->animated_rect_pct.p1.v[split_axis];
          }
          if(!df_panel_is_nil(new_panel->next))
          {
            Rng2F32 next_rect_pct = new_panel->next->animated_rect_pct;
            new_panel->animated_rect_pct = next_rect_pct;
            new_panel->animated_rect_pct.p1.v[split_axis] = new_panel->animated_rect_pct.p0.v[split_axis];
          }
          DF_Panel *move_tab_panel = df_panel_from_handle(params->panel);
          DF_View *move_tab = df_view_from_handle(params->view);
          if(!df_panel_is_nil(new_panel) && !df_view_is_nil(move_tab) && !df_panel_is_nil(move_tab_panel) &&
             kind == D_CmdKind_SplitPanel)
          {
            df_panel_remove_tab_view(move_tab_panel, move_tab);
            df_panel_insert_tab_view(new_panel, new_panel->last_tab_view, move_tab);
            new_panel->selected_tab_view = df_handle_from_view(move_tab);
            B32 move_tab_panel_is_empty = 1;
            for(DF_View *v = move_tab_panel->first_tab_view; !df_view_is_nil(v); v = v->order_next)
            {
              if(!df_view_is_project_filtered(v))
              {
                move_tab_panel_is_empty = 0;
                break;
              }
            }
            if(move_tab_panel_is_empty && move_tab_panel != ws->root_panel &&
               move_tab_panel != new_panel->prev && move_tab_panel != new_panel->next)
            {
              D_CmdParams p = df_cmd_params_from_panel(ws, move_tab_panel);
              d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_ClosePanel));
            }
          }
        }break;
        
        //- rjf: panel rotation
        case D_CmdKind_RotatePanelColumns:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *panel = ws->focused_panel;
          DF_Panel *parent = &df_nil_panel;
          for(DF_Panel *p = panel->parent; !df_panel_is_nil(p); p = p->parent)
          {
            if(p->split_axis == Axis2_X)
            {
              parent = p;
              break;
            }
          }
          if(!df_panel_is_nil(parent) && parent->child_count > 1)
          {
            DF_Panel *old_first = parent->first;
            DF_Panel *new_first = parent->first->next;
            old_first->next = &df_nil_panel;
            old_first->prev = parent->last;
            parent->last->next = old_first;
            new_first->prev = &df_nil_panel;
            parent->first = new_first;
            parent->last = old_first;
          }
        }break;
        
        //- rjf: panel focusing
        case D_CmdKind_NextPanel: panel_sib_off = OffsetOf(DF_Panel, next); panel_child_off = OffsetOf(DF_Panel, first); goto cycle;
        case D_CmdKind_PrevPanel: panel_sib_off = OffsetOf(DF_Panel, prev); panel_child_off = OffsetOf(DF_Panel, last); goto cycle;
        cycle:;
        {
          DF_Window *ws = df_window_from_handle(params->window);
          for(DF_Panel *panel = ws->focused_panel; !df_panel_is_nil(panel);)
          {
            DF_PanelRec rec = df_panel_rec_df(panel, panel_sib_off, panel_child_off);
            panel = rec.next;
            if(df_panel_is_nil(panel))
            {
              panel = ws->root_panel;
            }
            if(df_panel_is_nil(panel->first))
            {
              D_CmdParams p = df_cmd_params_from_window(ws);
              p.panel = df_handle_from_panel(panel);
              d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_FocusPanel));
              break;
            }
          }
        }break;
        case D_CmdKind_FocusPanel:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *panel = df_panel_from_handle(params->panel);
          if(!df_panel_is_nil(panel))
          {
            ws->focused_panel = panel;
            ws->menu_bar_focused = 0;
            ws->query_view_selected = 0;
          }
        }break;
        
        //- rjf: directional panel focus changing
        case D_CmdKind_FocusPanelRight: panel_change_dir = v2s32(+1, +0); goto focus_panel_dir;
        case D_CmdKind_FocusPanelLeft:  panel_change_dir = v2s32(-1, +0); goto focus_panel_dir;
        case D_CmdKind_FocusPanelUp:    panel_change_dir = v2s32(+0, -1); goto focus_panel_dir;
        case D_CmdKind_FocusPanelDown:  panel_change_dir = v2s32(+0, +1); goto focus_panel_dir;
        focus_panel_dir:;
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *src_panel = ws->focused_panel;
          Rng2F32 src_panel_rect = df_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, src_panel);
          Vec2F32 src_panel_center = center_2f32(src_panel_rect);
          Vec2F32 src_panel_half_dim = scale_2f32(dim_2f32(src_panel_rect), 0.5f);
          Vec2F32 travel_dim = add_2f32(src_panel_half_dim, v2f32(10.f, 10.f));
          Vec2F32 travel_dst = add_2f32(src_panel_center, mul_2f32(travel_dim, v2f32((F32)panel_change_dir.x, (F32)panel_change_dir.y)));
          DF_Panel *dst_root = &df_nil_panel;
          for(DF_Panel *p = ws->root_panel; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
          {
            if(p == src_panel || !df_panel_is_nil(p->first))
            {
              continue;
            }
            Rng2F32 p_rect = df_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, p);
            if(contains_2f32(p_rect, travel_dst))
            {
              dst_root = p;
              break;
            }
          }
          if(!df_panel_is_nil(dst_root))
          {
            DF_Panel *dst_panel = &df_nil_panel;
            for(DF_Panel *p = dst_root; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
            {
              if(df_panel_is_nil(p->first) && p != src_panel)
              {
                dst_panel = p;
                break;
              }
            }
            D_CmdParams p = df_cmd_params_from_window(ws);
            p.panel = df_handle_from_panel(dst_panel);
            d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_FocusPanel));
          }
        }break;
        
        //- rjf: focus history
        case D_CmdKind_GoBack:
        {
        }break;
        case D_CmdKind_GoForward:
        {
        }break;
        
        //- rjf: panel removal
        case D_CmdKind_ClosePanel:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *panel = df_panel_from_handle(params->panel);
          DF_Panel *parent = panel->parent;
          if(!df_panel_is_nil(parent))
          {
            Axis2 split_axis = parent->split_axis;
            
            // NOTE(rjf): If we're removing all but the last child of this parent,
            // we should just remove both children.
            if(parent->child_count == 2)
            {
              DF_Panel *discard_child = panel;
              DF_Panel *keep_child = panel == parent->first ? parent->last : parent->first;
              DF_Panel *grandparent = parent->parent;
              DF_Panel *parent_prev = parent->prev;
              F32 pct_of_parent = parent->pct_of_parent;
              
              // rjf: unhook kept child
              df_panel_remove(parent, keep_child);
              
              // rjf: unhook this subtree
              if(!df_panel_is_nil(grandparent))
              {
                df_panel_remove(grandparent, parent);
              }
              
              // rjf: release the things we should discard
              {
                df_panel_release(ws, parent);
                df_panel_release(ws, discard_child);
              }
              
              // rjf: re-hook our kept child into the overall tree
              if(df_panel_is_nil(grandparent))
              {
                ws->root_panel = keep_child;
              }
              else
              {
                df_panel_insert(grandparent, parent_prev, keep_child);
              }
              keep_child->pct_of_parent = pct_of_parent;
              
              // rjf: reset focus, if needed
              if(ws->focused_panel == discard_child)
              {
                ws->focused_panel = keep_child;
                for(DF_Panel *grandchild = ws->focused_panel; !df_panel_is_nil(grandchild); grandchild = grandchild->first)
                {
                  ws->focused_panel = grandchild;
                }
              }
              
              // rjf: keep-child split-axis == grandparent split-axis? bubble keep-child up into grandparent's children
              if(!df_panel_is_nil(grandparent) && grandparent->split_axis == keep_child->split_axis && !df_panel_is_nil(keep_child->first))
              {
                df_panel_remove(grandparent, keep_child);
                DF_Panel *prev = parent_prev;
                for(DF_Panel *child = keep_child->first, *next = 0; !df_panel_is_nil(child); child = next)
                {
                  next = child->next;
                  df_panel_remove(keep_child, child);
                  df_panel_insert(grandparent, prev, child);
                  prev = child;
                  child->pct_of_parent *= keep_child->pct_of_parent;
                }
                df_panel_release(ws, keep_child);
              }
            }
            // NOTE(rjf): Otherwise we can just remove this child.
            else
            {
              DF_Panel *next = &df_nil_panel;
              F32 removed_size_pct = panel->pct_of_parent;
              if(df_panel_is_nil(next)) { next = panel->prev; }
              if(df_panel_is_nil(next)) { next = panel->next; }
              df_panel_remove(parent, panel);
              df_panel_release(ws, panel);
              if(ws->focused_panel == panel)
              {
                ws->focused_panel = next;
              }
              for(DF_Panel *child = parent->first; !df_panel_is_nil(child); child = child->next)
              {
                child->pct_of_parent /= 1.f-removed_size_pct;
              }
            }
          }
        }break;
        
        //- rjf: panel tab controls
        case D_CmdKind_NextTab:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *next_view = view;
          for(DF_View *v = view; !df_view_is_nil(v); v = df_view_is_nil(v->order_next) ? panel->first_tab_view : v->order_next)
          {
            if(!df_view_is_project_filtered(v) && v != view)
            {
              next_view = v;
              break;
            }
          }
          view = next_view;
          panel->selected_tab_view = df_handle_from_view(view);
        }break;
        case D_CmdKind_PrevTab:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *next_view = view;
          for(DF_View *v = view; !df_view_is_nil(v); v = df_view_is_nil(v->order_prev) ? panel->last_tab_view : v->order_prev)
          {
            if(!df_view_is_project_filtered(v) && v != view)
            {
              next_view = v;
              break;
            }
          }
          view = next_view;
          panel->selected_tab_view = df_handle_from_view(view);
        }break;
        case D_CmdKind_MoveTabRight:
        case D_CmdKind_MoveTabLeft:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *panel = ws->focused_panel;
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *prev_view = (kind == D_CmdKind_MoveTabRight ? view->order_next : view->order_prev->order_prev);
          if(!df_view_is_nil(prev_view) || kind == D_CmdKind_MoveTabLeft)
          {
            D_CmdParams p = df_cmd_params_from_window(ws);
            p.panel = df_handle_from_panel(panel);
            p.dest_panel = df_handle_from_panel(panel);
            p.view = df_handle_from_view(view);
            p.prev_view = df_handle_from_view(prev_view);
            d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_MoveTab));
          }
        }break;
        case D_CmdKind_OpenTab:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          DF_ViewSpec *spec = params->view_spec;
          D_Entity *entity = &d_nil_entity;
          if(spec->info.flags & DF_ViewSpecFlag_ParameterizedByEntity)
          {
            entity = d_entity_from_handle(params->entity);
          }
          if(!df_panel_is_nil(panel) && spec != &df_nil_view_spec)
          {
            DF_View *view = df_view_alloc();
            String8 query = {0};
            if(!d_entity_is_nil(entity))
            {
              query = d_eval_string_from_entity(scratch.arena, entity);
            }
            else if(params->file_path.size != 0)
            {
              query = d_eval_string_from_file_path(scratch.arena, params->file_path);
            }
            else if(params->string.size != 0)
            {
              query = params->string;
            }
            df_view_equip_spec(view, spec, query, params->params_tree);
            df_panel_insert_tab_view(panel, panel->last_tab_view, view);
          }
        }break;
        case D_CmdKind_CloseTab:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          DF_View *view = df_view_from_handle(params->view);
          if(!df_view_is_nil(view))
          {
            df_panel_remove_tab_view(panel, view);
            df_view_release(view);
          }
        }break;
        case D_CmdKind_MoveTab:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *src_panel = df_panel_from_handle(params->panel);
          DF_View *view = df_view_from_handle(params->view);
          DF_Panel *dst_panel = df_panel_from_handle(params->dest_panel);
          DF_View *prev_view = df_view_from_handle(params->prev_view);
          if(!df_panel_is_nil(src_panel) &&
             !df_panel_is_nil(dst_panel) &&
             prev_view != view)
          {
            df_panel_remove_tab_view(src_panel, view);
            df_panel_insert_tab_view(dst_panel, prev_view, view);
            ws->focused_panel = dst_panel;
            B32 src_panel_is_empty = 1;
            for(DF_View *v = src_panel->first_tab_view; !df_view_is_nil(v); v = v->order_next)
            {
              if(!df_view_is_project_filtered(v))
              {
                src_panel_is_empty = 0;
                break;
              }
            }
            if(src_panel_is_empty && src_panel != ws->root_panel)
            {
              D_CmdParams p = df_cmd_params_from_panel(ws, src_panel);
              d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_ClosePanel));
            }
          }
        }break;
        case D_CmdKind_TabBarTop:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          panel->tab_side = Side_Min;
        }break;
        case D_CmdKind_TabBarBottom:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          panel->tab_side = Side_Max;
        }break;
        
        //- rjf: files
        case D_CmdKind_Open:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          String8 path = params->file_path;
          FileProperties props = os_properties_from_file_path(path);
          if(props.created != 0)
          {
            D_CmdParams p = *params;
            p.window    = df_handle_from_window(ws);
            p.panel     = df_handle_from_panel(ws->focused_panel);
            d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_PendingFile));
          }
          else
          {
            d_errorf("Couldn't open file at \"%S\".", path);
          }
        }break;
        case D_CmdKind_Switch:
        {
          // TODO(rjf): @msgs
#if 0
          B32 already_opened = 0;
          DF_Panel *panel = df_panel_from_handle(params->panel);
          for(DF_View *v = panel->first_tab_view; !df_view_is_nil(v); v = v->next)
          {
            if(df_view_is_project_filtered(v)) { continue; }
            D_Entity *v_param_entity = d_entity_from_handle(v->params_entity);
            if(v_param_entity == d_entity_from_handle(params->entity))
            {
              panel->selected_tab_view = df_handle_from_view(v);
              already_opened = 1;
              break;
            }
          }
          if(already_opened == 0)
          {
            D_CmdParams p = params;
            p.window = df_handle_from_window(ws);
            p.panel = df_handle_from_panel(ws->focused_panel);
            p.entity = params->entity;
            d_cmd_list_push(arena, cmds, &p, d_cmd_spec_from_kind(D_CmdKind_PendingFile));
          }
#endif
        }break;
        case D_CmdKind_SwitchToPartnerFile:
        {
          DF_Panel *panel = df_panel_from_handle(params->panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
          if(view_kind == DF_ViewKind_Text)
          {
            String8 file_path      = d_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
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
                  // TODO(rjf):
                  //D_CmdParams p = df_cmd_params_from_panel(ws, panel);
                  //p.entity = d_handle_from_entity(candidate);
                  //d_cmd_list_push(arena, cmds, &p, d_cmd_spec_from_kind(D_CmdKind_Switch));
                  break;
                }
              }
            }
          }
        }break;
        
        //- rjf: panel built-in layout builds
        case D_CmdKind_ResetToDefaultPanels:
        case D_CmdKind_ResetToCompactPanels:
        {
          panel_reset_done = 1;
          DF_Window *ws = df_window_from_handle(params->window);
          
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
            case D_CmdKind_ResetToDefaultPanels:{layout = Layout_Default;}break;
            case D_CmdKind_ResetToCompactPanels:{layout = Layout_Compact;}break;
          }
          
          //- rjf: gather all panels in the panel tree - remove & gather views
          // we'd like to keep in the next layout
          D_HandleList panels_to_close = {0};
          D_HandleList views_to_close = {0};
          DF_View *watch = &df_nil_view;
          DF_View *locals = &df_nil_view;
          DF_View *regs = &df_nil_view;
          DF_View *globals = &df_nil_view;
          DF_View *tlocals = &df_nil_view;
          DF_View *types = &df_nil_view;
          DF_View *procs = &df_nil_view;
          DF_View *callstack = &df_nil_view;
          DF_View *breakpoints = &df_nil_view;
          DF_View *watch_pins = &df_nil_view;
          DF_View *output = &df_nil_view;
          DF_View *targets = &df_nil_view;
          DF_View *scheduler = &df_nil_view;
          DF_View *modules = &df_nil_view;
          DF_View *disasm = &df_nil_view;
          DF_View *memory = &df_nil_view;
          DF_View *getting_started = &df_nil_view;
          D_HandleList code_views = {0};
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            D_Handle handle = df_handle_from_panel(panel);
            d_handle_list_push(scratch.arena, &panels_to_close, handle);
            for(DF_View *view = panel->first_tab_view, *next = 0; !df_view_is_nil(view); view = next)
            {
              next = view->order_next;
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              B32 needs_delete = 1;
              switch(view_kind)
              {
                default:{}break;
                case DF_ViewKind_Watch:         {if(df_view_is_nil(watch))               { needs_delete = 0; watch = view;} }break;
                case DF_ViewKind_Locals:        {if(df_view_is_nil(locals))              { needs_delete = 0; locals = view;} }break;
                case DF_ViewKind_Registers:     {if(df_view_is_nil(regs))                { needs_delete = 0; regs = view;} }break;
                case DF_ViewKind_Globals:       {if(df_view_is_nil(globals))             { needs_delete = 0; globals = view;} }break;
                case DF_ViewKind_ThreadLocals:  {if(df_view_is_nil(tlocals))             { needs_delete = 0; tlocals = view;} }break;
                case DF_ViewKind_Types:         {if(df_view_is_nil(types))               { needs_delete = 0; types = view;} }break;
                case DF_ViewKind_Procedures:    {if(df_view_is_nil(procs))               { needs_delete = 0; procs = view;} }break;
                case DF_ViewKind_CallStack:     {if(df_view_is_nil(callstack))           { needs_delete = 0; callstack = view;} }break;
                case DF_ViewKind_Breakpoints:   {if(df_view_is_nil(breakpoints))         { needs_delete = 0; breakpoints = view;} }break;
                case DF_ViewKind_WatchPins:     {if(df_view_is_nil(watch_pins))          { needs_delete = 0; watch_pins = view;} }break;
                case DF_ViewKind_Output:        {if(df_view_is_nil(output))              { needs_delete = 0; output = view;} }break;
                case DF_ViewKind_Targets:       {if(df_view_is_nil(targets))             { needs_delete = 0; targets = view;} }break;
                case DF_ViewKind_Scheduler:     {if(df_view_is_nil(scheduler))           { needs_delete = 0; scheduler = view;} }break;
                case DF_ViewKind_Modules:       {if(df_view_is_nil(modules))             { needs_delete = 0; modules = view;} }break;
                case DF_ViewKind_Disasm:        {if(df_view_is_nil(disasm))              { needs_delete = 0; disasm = view;} }break;
                case DF_ViewKind_Memory:        {if(df_view_is_nil(memory))              { needs_delete = 0; memory = view;} }break;
                case DF_ViewKind_GettingStarted:{if(df_view_is_nil(getting_started))     { needs_delete = 0; getting_started = view;} }break;
                case DF_ViewKind_Text:
                {
                  needs_delete = 0;
                  d_handle_list_push(scratch.arena, &code_views, df_handle_from_view(view));
                }break;
              }
              if(!needs_delete)
              {
                df_panel_remove_tab_view(panel, view);
              }
            }
          }
          
          //- rjf: close all panels/views
          for(D_HandleNode *n = panels_to_close.first; n != 0; n = n->next)
          {
            DF_Panel *panel = df_panel_from_handle(n->handle);
            if(panel != ws->root_panel)
            {
              df_panel_release(ws, panel);
            }
            else
            {
              df_panel_release_all_views(panel);
              panel->first = panel->last = &df_nil_panel;
            }
          }
          
          //- rjf: allocate any missing views
          D_CmdParams blank_params = df_cmd_params_from_window(ws);
          if(df_view_is_nil(watch))
          {
            watch = df_view_alloc();
            df_view_equip_spec(watch, df_view_spec_from_kind(DF_ViewKind_Watch), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(locals))
          {
            locals = df_view_alloc();
            df_view_equip_spec(locals, df_view_spec_from_kind(DF_ViewKind_Locals), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(regs))
          {
            regs = df_view_alloc();
            df_view_equip_spec(regs, df_view_spec_from_kind(DF_ViewKind_Registers), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(globals))
          {
            globals = df_view_alloc();
            df_view_equip_spec(globals, df_view_spec_from_kind(DF_ViewKind_Globals), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(tlocals))
          {
            tlocals = df_view_alloc();
            df_view_equip_spec(tlocals, df_view_spec_from_kind(DF_ViewKind_ThreadLocals), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(types))
          {
            types = df_view_alloc();
            df_view_equip_spec(types, df_view_spec_from_kind(DF_ViewKind_Types), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(procs))
          {
            procs = df_view_alloc();
            df_view_equip_spec(procs, df_view_spec_from_kind(DF_ViewKind_Procedures), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(callstack))
          {
            callstack = df_view_alloc();
            df_view_equip_spec(callstack, df_view_spec_from_kind(DF_ViewKind_CallStack), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(breakpoints))
          {
            breakpoints = df_view_alloc();
            df_view_equip_spec(breakpoints, df_view_spec_from_kind(DF_ViewKind_Breakpoints), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(watch_pins))
          {
            watch_pins = df_view_alloc();
            df_view_equip_spec(watch_pins, df_view_spec_from_kind(DF_ViewKind_WatchPins), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(output))
          {
            output = df_view_alloc();
            df_view_equip_spec(output, df_view_spec_from_kind(DF_ViewKind_Output), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(targets))
          {
            targets = df_view_alloc();
            df_view_equip_spec(targets, df_view_spec_from_kind(DF_ViewKind_Targets), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(scheduler))
          {
            scheduler = df_view_alloc();
            df_view_equip_spec(scheduler, df_view_spec_from_kind(DF_ViewKind_Scheduler), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(modules))
          {
            modules = df_view_alloc();
            df_view_equip_spec(modules, df_view_spec_from_kind(DF_ViewKind_Modules), str8_zero(), &md_nil_node);
          }
          if(df_view_is_nil(disasm))
          {
            disasm = df_view_alloc();
            df_view_equip_spec(disasm, df_view_spec_from_kind(DF_ViewKind_Disasm), str8_zero(), &md_nil_node);
          }
          if(layout == Layout_Default && df_view_is_nil(memory))
          {
            memory = df_view_alloc();
            df_view_equip_spec(memory, df_view_spec_from_kind(DF_ViewKind_Memory), str8_zero(), &md_nil_node);
          }
          if(code_views.count == 0 && df_view_is_nil(getting_started))
          {
            getting_started = df_view_alloc();
            df_view_equip_spec(getting_started, df_view_spec_from_kind(DF_ViewKind_GettingStarted), str8_zero(), &md_nil_node);
          }
          
          //- rjf: apply layout
          switch(layout)
          {
            //- rjf: default layout
            case Layout_Default:
            {
              // rjf: root split
              ws->root_panel->split_axis = Axis2_X;
              DF_Panel *root_0 = df_panel_alloc(ws);
              DF_Panel *root_1 = df_panel_alloc(ws);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
              root_0->pct_of_parent = 0.85f;
              root_1->pct_of_parent = 0.15f;
              
              // rjf: root_0 split
              root_0->split_axis = Axis2_Y;
              DF_Panel *root_0_0 = df_panel_alloc(ws);
              DF_Panel *root_0_1 = df_panel_alloc(ws);
              df_panel_insert(root_0, root_0->last, root_0_0);
              df_panel_insert(root_0, root_0->last, root_0_1);
              root_0_0->pct_of_parent = 0.80f;
              root_0_1->pct_of_parent = 0.20f;
              
              // rjf: root_1 split
              root_1->split_axis = Axis2_Y;
              DF_Panel *root_1_0 = df_panel_alloc(ws);
              DF_Panel *root_1_1 = df_panel_alloc(ws);
              df_panel_insert(root_1, root_1->last, root_1_0);
              df_panel_insert(root_1, root_1->last, root_1_1);
              root_1_0->pct_of_parent = 0.50f;
              root_1_1->pct_of_parent = 0.50f;
              df_panel_insert_tab_view(root_1_0, root_1_0->last_tab_view, targets);
              df_panel_insert_tab_view(root_1_1, root_1_1->last_tab_view, scheduler);
              root_1_0->selected_tab_view = df_handle_from_view(targets);
              root_1_1->selected_tab_view = df_handle_from_view(scheduler);
              root_1_1->tab_side = Side_Max;
              
              // rjf: root_0_0 split
              root_0_0->split_axis = Axis2_X;
              DF_Panel *root_0_0_0 = df_panel_alloc(ws);
              DF_Panel *root_0_0_1 = df_panel_alloc(ws);
              df_panel_insert(root_0_0, root_0_0->last, root_0_0_0);
              df_panel_insert(root_0_0, root_0_0->last, root_0_0_1);
              root_0_0_0->pct_of_parent = 0.25f;
              root_0_0_1->pct_of_parent = 0.75f;
              
              // rjf: root_0_0_0 split
              root_0_0_0->split_axis = Axis2_Y;
              DF_Panel *root_0_0_0_0 = df_panel_alloc(ws);
              DF_Panel *root_0_0_0_1 = df_panel_alloc(ws);
              df_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_0);
              df_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_1);
              root_0_0_0_0->pct_of_parent = 0.5f;
              root_0_0_0_1->pct_of_parent = 0.5f;
              df_panel_insert_tab_view(root_0_0_0_0, root_0_0_0_0->last_tab_view, disasm);
              root_0_0_0_0->selected_tab_view = df_handle_from_view(disasm);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, breakpoints);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, watch_pins);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, output);
              df_panel_insert_tab_view(root_0_0_0_1, root_0_0_0_1->last_tab_view, memory);
              root_0_0_0_1->selected_tab_view = df_handle_from_view(output);
              
              // rjf: root_0_1 split
              root_0_1->split_axis = Axis2_X;
              DF_Panel *root_0_1_0 = df_panel_alloc(ws);
              DF_Panel *root_0_1_1 = df_panel_alloc(ws);
              df_panel_insert(root_0_1, root_0_1->last, root_0_1_0);
              df_panel_insert(root_0_1, root_0_1->last, root_0_1_1);
              root_0_1_0->pct_of_parent = 0.60f;
              root_0_1_1->pct_of_parent = 0.40f;
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, watch);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, locals);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, regs);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, globals);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, tlocals);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, types);
              df_panel_insert_tab_view(root_0_1_0, root_0_1_0->last_tab_view, procs);
              root_0_1_0->selected_tab_view = df_handle_from_view(watch);
              root_0_1_0->tab_side = Side_Max;
              df_panel_insert_tab_view(root_0_1_1, root_0_1_1->last_tab_view, callstack);
              df_panel_insert_tab_view(root_0_1_1, root_0_1_1->last_tab_view, modules);
              root_0_1_1->selected_tab_view = df_handle_from_view(callstack);
              root_0_1_1->tab_side = Side_Max;
              
              // rjf: fill main panel with getting started, OR all collected code views
              if(!df_view_is_nil(getting_started))
              {
                df_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, getting_started);
              }
              for(D_HandleNode *n = code_views.first; n != 0; n = n->next)
              {
                DF_View *view = df_view_from_handle(n->handle);
                if(!df_view_is_nil(view))
                {
                  df_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, view);
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
              DF_Panel *root_0 = df_panel_alloc(ws);
              DF_Panel *root_1 = df_panel_alloc(ws);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
              df_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
              root_0->pct_of_parent = 0.25f;
              root_1->pct_of_parent = 0.75f;
              
              // rjf: root_0 split
              root_0->split_axis = Axis2_Y;
              DF_Panel *root_0_0 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(watch)) { df_panel_insert_tab_view(root_0_0, root_0_0->last_tab_view, watch); }
                if(!df_view_is_nil(types)) { df_panel_insert_tab_view(root_0_0, root_0_0->last_tab_view, types); }
                root_0_0->selected_tab_view = df_handle_from_view(watch);
              }
              DF_Panel *root_0_1 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(scheduler))     { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, scheduler); }
                if(!df_view_is_nil(targets))       { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, targets); }
                if(!df_view_is_nil(breakpoints))   { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, breakpoints); }
                if(!df_view_is_nil(watch_pins))    { df_panel_insert_tab_view(root_0_1, root_0_1->last_tab_view, watch_pins); }
                root_0_1->selected_tab_view = df_handle_from_view(scheduler);
              }
              DF_Panel *root_0_2 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(disasm))    { df_panel_insert_tab_view(root_0_2, root_0_2->last_tab_view, disasm); }
                if(!df_view_is_nil(output))    { df_panel_insert_tab_view(root_0_2, root_0_2->last_tab_view, output); }
                root_0_2->selected_tab_view = df_handle_from_view(disasm);
              }
              DF_Panel *root_0_3 = df_panel_alloc(ws);
              {
                if(!df_view_is_nil(callstack))    { df_panel_insert_tab_view(root_0_3, root_0_3->last_tab_view, callstack); }
                if(!df_view_is_nil(modules))      { df_panel_insert_tab_view(root_0_3, root_0_3->last_tab_view, modules); }
                root_0_3->selected_tab_view = df_handle_from_view(callstack);
              }
              df_panel_insert(root_0, root_0->last, root_0_0);
              df_panel_insert(root_0, root_0->last, root_0_1);
              df_panel_insert(root_0, root_0->last, root_0_2);
              df_panel_insert(root_0, root_0->last, root_0_3);
              root_0_0->pct_of_parent = 0.25f;
              root_0_1->pct_of_parent = 0.25f;
              root_0_2->pct_of_parent = 0.25f;
              root_0_3->pct_of_parent = 0.25f;
              
              // rjf: fill main panel with getting started, OR all collected code views
              if(!df_view_is_nil(getting_started))
              {
                df_panel_insert_tab_view(root_1, root_1->last_tab_view, getting_started);
              }
              for(D_HandleNode *n = code_views.first; n != 0; n = n->next)
              {
                DF_View *view = df_view_from_handle(n->handle);
                if(!df_view_is_nil(view))
                {
                  df_panel_insert_tab_view(root_1, root_1->last_tab_view, view);
                }
              }
              
              // rjf: choose initial focused panel
              ws->focused_panel = root_1;
            }break;
          }
          
          // rjf: dispatch cfg saves
          for(D_CfgSrc src = (D_CfgSrc)0; src < D_CfgSrc_COUNT; src = (D_CfgSrc)(src+1))
          {
            D_CmdKind write_cmd = d_cfg_src_write_cmd_kind_table[src];
            d_cmd(write_cmd, .file_path = d_cfg_path_from_src(src));
          }
        }break;
        
        
        //- rjf: thread finding
        case D_CmdKind_FindThread:
        for(DF_Window *ws = df_state->first_window; ws != 0; ws = ws->next)
        {
          DI_Scope *scope = di_scope_open();
          D_Entity *thread = d_entity_from_handle(params->entity);
          U64 unwind_index = params->unwind_index;
          U64 inline_depth = params->inline_depth;
          if(thread->kind == D_EntityKind_Thread)
          {
            // rjf: grab rip
            U64 rip_vaddr = d_query_cached_rip_from_thread_unwind(thread, unwind_index);
            
            // rjf: extract thread/rip info
            D_Entity *process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
            D_Entity *module = d_module_from_process_vaddr(process, rip_vaddr);
            DI_Key dbgi_key = d_dbgi_key_from_module(module);
            RDI_Parsed *rdi = di_rdi_from_key(scope, &dbgi_key, 0);
            U64 rip_voff = d_voff_from_vaddr(module, rip_vaddr);
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
            B32 missing_rip = (rip_vaddr == 0);
            B32 dbgi_missing = (dbgi_key.min_timestamp == 0 || dbgi_key.path.size == 0);
            B32 dbgi_pending = !dbgi_missing && rdi == &di_rdi_parsed_nil;
            B32 has_line_info = (line.voff_range.max != 0);
            B32 has_module = !d_entity_is_nil(module);
            B32 has_dbg_info = has_module && !dbgi_missing;
            if(!dbgi_pending && (has_line_info || has_module))
            {
              D_CmdParams params = df_cmd_params_from_window(ws);
              if(has_line_info)
              {
                params.file_path = line.file_path;
                params.text_point = line.pt;
              }
              params.entity = d_handle_from_entity(thread);
              params.voff = rip_voff;
              params.vaddr = rip_vaddr;
              params.unwind_index = unwind_index;
              params.inline_depth = inline_depth;
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(D_CmdKind_FindCodeLocation));
            }
            
            // rjf: snap to resolved address w/o line info
            if(!missing_rip && !dbgi_pending && !has_line_info && !has_module)
            {
              D_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = d_handle_from_entity(thread);
              params.voff = rip_voff;
              params.vaddr = rip_vaddr;
              params.unwind_index = unwind_index;
              params.inline_depth = inline_depth;
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(D_CmdKind_FindCodeLocation));
            }
            
            // rjf: retry on stopped, pending debug info
            if(!d_ctrl_targets_running() && (dbgi_pending || missing_rip))
            {
              d_cmd(D_CmdKind_FindThread, .entity = d_handle_from_entity(thread));
            }
          }
          di_scope_close(scope);
        }break;
        case D_CmdKind_FindSelectedThread:
        {
          // TODO(rjf): @msgs
#if 0
          for(DF_Window *ws = df_state->first_window; ws != 0; ws = ws->next)
          {
            D_Entity *selected_thread = d_entity_from_handle(d_base_regs()->thread);
            D_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = d_handle_from_entity(selected_thread);
            params.unwind_index = d_base_regs()->unwind_count;
            params.inline_depth = d_base_regs()->inline_depth;
            d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(D_CmdKind_FindThread));
          }
#endif
        }break;
        
        //- rjf: name finding
        case D_CmdKind_GoToName:
        {
          String8 name = params->string;
          if(name.size != 0)
          {
            B32 name_resolved = 0;
            
            // rjf: try to resolve name as a symbol
            U64 voff = 0;
            DI_Key voff_dbgi_key = {0};
            if(name_resolved == 0)
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
            D_Entity *file = &d_nil_entity;
            if(name_resolved == 0)
            {
              D_Entity *src_entity = d_entity_from_handle(params->entity);
              String8 file_part_of_name = name;
              U64 quote_pos = str8_find_needle(name, 0, str8_lit("\""), 0);
              if(quote_pos < name.size)
              {
                file_part_of_name = str8_skip(name, quote_pos+1);
                U64 ender_quote_pos = str8_find_needle(file_part_of_name, 0, str8_lit("\""), 0);
                file_part_of_name = str8_prefix(file_part_of_name, ender_quote_pos);
              }
              if(file_part_of_name.size != 0)
              {
                String8 folder_path = str8_chop_last_slash(file_part_of_name);
                String8 file_name = str8_skip_last_slash(file_part_of_name);
                String8List folders = str8_split_path(scratch.arena, folder_path);
                
                // rjf: some folders are specified
                if(folders.node_count != 0)
                {
                  String8 first_folder_name = folders.first->string;
                  D_Entity *root_folder = &d_nil_entity;
                  
                  // rjf: try to find root folder as if it's an absolute path
                  if(d_entity_is_nil(root_folder))
                  {
                    root_folder = d_entity_from_path(first_folder_name, D_EntityFromPathFlag_OpenAsNeeded);
                  }
                  
                  // rjf: try to find root folder as if it's a path we've already loaded
                  if(d_entity_is_nil(root_folder))
                  {
                    root_folder = d_entity_from_name_and_kind(first_folder_name, D_EntityKind_File);
                  }
                  
                  // rjf: try to find root folder as if it's inside of a path we've already loaded
                  if(d_entity_is_nil(root_folder))
                  {
                    D_EntityList all_files = d_query_cached_entity_list_with_kind(D_EntityKind_File);
                    for(D_EntityNode *n = all_files.first; n != 0; n = n->next)
                    {
                      if(n->entity->flags & D_EntityFlag_IsFolder)
                      {
                        String8 n_entity_path = d_full_path_from_entity(scratch.arena, n->entity);
                        String8 estimated_full_path = push_str8f(scratch.arena, "%S/%S", n_entity_path, first_folder_name);
                        root_folder = d_entity_from_path(estimated_full_path, D_EntityFromPathFlag_OpenAsNeeded);
                        if(!d_entity_is_nil(root_folder))
                        {
                          break;
                        }
                      }
                    }
                  }
                  
                  // rjf: has root folder -> descend downwards
                  if(!d_entity_is_nil(root_folder))
                  {
                    String8 root_folder_path = d_full_path_from_entity(scratch.arena, root_folder);
                    String8List full_file_path_parts = {0};
                    str8_list_push(scratch.arena, &full_file_path_parts, root_folder_path);
                    for(String8Node *n = folders.first->next; n != 0; n = n->next)
                    {
                      str8_list_push(scratch.arena, &full_file_path_parts, n->string);
                    }
                    str8_list_push(scratch.arena, &full_file_path_parts, file_name);
                    StringJoin join = {0};
                    join.sep = str8_lit("/");
                    String8 full_file_path = str8_list_join(scratch.arena, &full_file_path_parts, &join);
                    file = d_entity_from_path(full_file_path, D_EntityFromPathFlag_AllowOverrides|D_EntityFromPathFlag_OpenAsNeeded|D_EntityFromPathFlag_OpenMissing);
                  }
                }
                
                // rjf: no folders specified => just try the local folder, then try globally
                else if(src_entity->kind == D_EntityKind_File)
                {
                  file = d_entity_from_name_and_kind(file_name, D_EntityKind_File);
                  if(d_entity_is_nil(file))
                  {
                    String8 src_entity_full_path = d_full_path_from_entity(scratch.arena, src_entity);
                    String8 src_entity_folder = str8_chop_last_slash(src_entity_full_path);
                    String8 estimated_full_path = push_str8f(scratch.arena, "%S/%S", src_entity_folder, file_name);
                    file = d_entity_from_path(estimated_full_path, D_EntityFromPathFlag_All);
                  }
                }
              }
              name_resolved = !d_entity_is_nil(file) && !(file->flags & D_EntityFlag_IsMissing) && !(file->flags & D_EntityFlag_IsFolder);
            }
            
            // rjf: process resolved info
            if(name_resolved == 0)
            {
              d_errorf("`%S` could not be found.", name);
            }
            
            // rjf: name resolved to voff * dbg info
            if(name_resolved != 0 && voff != 0)
            {
              D_LineList lines = d_lines_from_dbgi_key_voff(scratch.arena, &voff_dbgi_key, voff);
              if(lines.first != 0)
              {
                D_CmdParams p = *params;
                {
                  p.file_path = lines.first->v.file_path;
                  p.text_point = lines.first->v.pt;
                  if(voff_dbgi_key.path.size != 0)
                  {
                    D_EntityList modules = d_modules_from_dbgi_key(scratch.arena, &voff_dbgi_key);
                    D_Entity *module = d_first_entity_from_list(&modules);
                    D_Entity *process = d_entity_ancestor_from_kind(module, D_EntityKind_Process);
                    if(!d_entity_is_nil(process))
                    {
                      p.entity = d_handle_from_entity(process);
                      p.vaddr = module->vaddr_rng.min + lines.first->v.voff_range.min;
                    }
                  }
                }
                d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_FindCodeLocation));
              }
            }
            
            // rjf: name resolved to a file
            if(name_resolved != 0 && !d_entity_is_nil(file))
            {
              String8 path = d_full_path_from_entity(scratch.arena, file);
              D_CmdParams p = *params;
              p.file_path = path;
              p.text_point = txt_pt(1, 1);
              d_cmd_list_push(scratch.arena, &cmds, &p, d_cmd_spec_from_kind(D_CmdKind_FindCodeLocation));
            }
          }
        }break;
        
        //- rjf: editors
        case D_CmdKind_EditEntity:
        {
          D_Entity *entity = d_entity_from_handle(params->entity);
          switch(entity->kind)
          {
            default: break;
            case D_EntityKind_Target:
            {
              d_cmd_list_push(scratch.arena, &cmds, params, d_cmd_spec_from_kind(D_CmdKind_EditTarget));
            }break;
          }
        }break;
        
        //- rjf: targets
        case D_CmdKind_EditTarget:
        {
          D_Entity *entity = d_entity_from_handle(params->entity);
          if(!d_entity_is_nil(entity) && entity->kind == D_EntityKind_Target)
          {
            d_cmd_list_push(scratch.arena, &cmds, params, d_cmd_spec_from_kind(D_CmdKind_Target));
          }
          else
          {
            d_errorf("Invalid target.");
          }
        }break;
        
        //- rjf: catchall general entity activation paths (drag/drop, clicking)
        case D_CmdKind_EntityRefFastPath:
        {
          D_Entity *entity = d_entity_from_handle(params->entity);
          switch(entity->kind)
          {
            default:
            {
              d_cmd(D_CmdKind_SpawnEntityView, .entity = d_handle_from_entity(entity));
            }break;
            case D_EntityKind_Thread:
            {
              d_cmd(D_CmdKind_SelectThread, .entity = d_handle_from_entity(entity));
            }break;
            case D_EntityKind_Target:
            {
              d_cmd(D_CmdKind_SelectTarget, .entity = d_handle_from_entity(entity));
            }break;
          }
        }break;
        case D_CmdKind_SpawnEntityView:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          DF_Panel *panel = df_panel_from_handle(params->panel);
          D_Entity *entity = d_entity_from_handle(params->entity);
          switch(entity->kind)
          {
            default:{}break;
            
            case D_EntityKind_Target:
            {
              D_CmdParams params = df_cmd_params_from_panel(ws, panel);
              params.entity = d_handle_from_entity(entity);
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(D_CmdKind_EditTarget));
            }break;
          }
        }break;
        case D_CmdKind_FindCodeLocation:
        {
#if 0 // TODO(rjf): @msgs
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
          DF_Window *ws = df_window_from_handle(params->window);
          
          // rjf: grab things to find. path * point, process * address, etc.
          String8 file_path = {0};
          TxtPt point = {0};
          D_Entity *thread = &d_nil_entity;
          D_Entity *process = &d_nil_entity;
          U64 vaddr = 0;
          {
            file_path = params->file_path;
            point = params->text_point;
            thread = d_entity_from_handle(d_regs()->thread);
            process = d_entity_ancestor_from_kind(thread, D_EntityKind_Process);
            vaddr = params->vaddr;
          }
          
          // rjf: given a src code location, and a process, if no vaddr is specified,
          // try to map the src coordinates to a vaddr via line info
          if(vaddr == 0 && file_path.size != 0 && !d_entity_is_nil(process))
          {
            D_LineList lines = d_lines_from_file_path_line_num(scratch.arena, file_path, point.line);
            for(D_LineNode *n = lines.first; n != 0; n = n->next)
            {
              D_EntityList modules = d_modules_from_dbgi_key(scratch.arena, &n->v.dbgi_key);
              D_Entity *module = d_module_from_thread_candidates(thread, &modules);
              vaddr = d_vaddr_from_voff(module, n->v.voff_range.min);
              break;
            }
          }
          
          // rjf: first, try to find panel/view pair that already has the src file open
          DF_Panel *panel_w_this_src_code = &df_nil_panel;
          DF_View *view_w_this_src_code = &df_nil_view;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
              String8 view_file_path = d_file_path_from_eval_string(scratch.arena, str8(view->query_buffer, view->query_string_size));
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              if((view_kind == DF_ViewKind_Text || view_kind == DF_ViewKind_PendingFile) &&
                 path_match_normalized(view_file_path, file_path))
              {
                panel_w_this_src_code = panel;
                view_w_this_src_code = view;
                if(view == df_selected_tab_from_panel(panel))
                {
                  break;
                }
              }
            }
          }
          
          // rjf: find a panel that already has *any* code open
          DF_Panel *panel_w_any_src_code = &df_nil_panel;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              if(view_kind == DF_ViewKind_Text)
              {
                panel_w_any_src_code = panel;
                break;
              }
            }
          }
          
          // rjf: try to find panel/view pair that has disassembly open
          DF_Panel *panel_w_disasm = &df_nil_panel;
          DF_View *view_w_disasm = &df_nil_view;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->order_next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
              DF_ViewKind view_kind = df_view_kind_from_string(view->spec->info.name);
              if(view_kind == DF_ViewKind_Disasm && view->query_string_size == 0)
              {
                panel_w_disasm = panel;
                view_w_disasm = view;
                if(view == df_selected_tab_from_panel(panel))
                {
                  break;
                }
              }
            }
          }
          
          // rjf: find the biggest panel
          DF_Panel *biggest_panel = &df_nil_panel;
          {
            Rng2F32 root_rect = os_client_rect_from_window(ws->os);
            F32 best_panel_area = 0;
            for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
            {
              if(!df_panel_is_nil(panel->first))
              {
                continue;
              }
              Rng2F32 panel_rect = df_target_rect_from_panel(root_rect, ws->root_panel, panel);
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
          DF_Panel *biggest_empty_panel = &df_nil_panel;
          {
            Rng2F32 root_rect = os_client_rect_from_window(ws->os);
            F32 best_panel_area = 0;
            for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
            {
              if(!df_panel_is_nil(panel->first))
              {
                continue;
              }
              Rng2F32 panel_rect = df_target_rect_from_panel(root_rect, ws->root_panel, panel);
              Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
              F32 area = panel_rect_dim.x * panel_rect_dim.y;
              B32 panel_is_empty = 1;
              for(DF_View *v = panel->first_tab_view; !df_view_is_nil(v); v = v->order_next)
              {
                if(!df_view_is_project_filtered(v))
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
          DF_Panel *panel_used_for_src_code = &df_nil_panel;
          if(file_path.size != 0)
          {
            // rjf: determine which panel we will use to find the code loc
            DF_Panel *dst_panel = &df_nil_panel;
            {
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_this_src_code; }
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_any_src_code; }
              if(df_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
              if(df_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
            }
            
            // rjf: construct new view if needed
            DF_View *dst_view = view_w_this_src_code;
            if(!df_panel_is_nil(dst_panel) && df_view_is_nil(view_w_this_src_code))
            {
              DF_View *view = df_view_alloc();
              String8 file_path_query = d_eval_string_from_file_path(scratch.arena, file_path);
              df_view_equip_spec(view, df_view_spec_from_kind(DF_ViewKind_Text), file_path_query, &md_nil_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            D_CmdKind cursor_snap_kind = D_CmdKind_CenterCursor;
            if(!df_panel_is_nil(dst_panel) && dst_view == view_w_this_src_code && df_selected_tab_from_panel(dst_panel) == dst_view)
            {
              cursor_snap_kind = D_CmdKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              disasm_view_prioritized = (df_selected_tab_from_panel(dst_panel) == view_w_disasm);
              dst_panel->selected_tab_view = df_handle_from_view(dst_view);
              D_CmdParams params = df_cmd_params_from_view(ws, dst_panel, dst_view);
              params.text_point = point;
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(D_CmdKind_GoToLine));
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(cursor_snap_kind));
              panel_used_for_src_code = dst_panel;
            }
          }
          
          // rjf: given the above, find disassembly location.
          if(!d_entity_is_nil(process) && vaddr != 0)
          {
            // rjf: determine which panel we will use to find the disasm loc -
            // we *cannot* use the same panel we used for source code, if any.
            DF_Panel *dst_panel = &df_nil_panel;
            {
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_disasm; }
              if(df_panel_is_nil(panel_used_for_src_code) && df_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
              if(df_panel_is_nil(panel_used_for_src_code) && df_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
              if(dst_panel == panel_used_for_src_code &&
                 !disasm_view_prioritized)
              {
                dst_panel = &df_nil_panel;
              }
            }
            
            // rjf: construct new view if needed
            DF_View *dst_view = view_w_disasm;
            if(!df_panel_is_nil(dst_panel) && df_view_is_nil(view_w_disasm))
            {
              DF_View *view = df_view_alloc();
              df_view_equip_spec(view, df_view_spec_from_kind(DF_ViewKind_Disasm), str8_zero(), &md_nil_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            D_CmdKind cursor_snap_kind = D_CmdKind_CenterCursor;
            if(dst_view == view_w_disasm && df_selected_tab_from_panel(dst_panel) == dst_view)
            {
              cursor_snap_kind = D_CmdKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              dst_panel->selected_tab_view = df_handle_from_view(dst_view);
              D_CmdParams params = df_cmd_params_from_view(ws, dst_panel, dst_view);
              params.entity = d_handle_from_entity(process);
              params.vaddr = vaddr;
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(D_CmdKind_GoToAddress));
              d_cmd_list_push(scratch.arena, &cmds, &params, d_cmd_spec_from_kind(cursor_snap_kind));
            }
          }
#endif
        }break;
        
        //- rjf: filtering
        case D_CmdKind_Filter:
        {
          DF_View *view = df_view_from_handle(params->view);
          DF_Panel *panel = df_panel_from_handle(params->panel);
          B32 view_is_tab = 0;
          for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->order_next)
          {
            if(df_view_is_project_filtered(tab)) { continue; }
            if(tab == view)
            {
              view_is_tab = 1;
              break;
            }
          }
          if(view_is_tab && view->spec->info.flags & DF_ViewSpecFlag_CanFilter)
          {
            view->is_filtering ^= 1;
            view->query_cursor = txt_pt(1, 1+(S64)view->query_string_size);
            view->query_mark = txt_pt(1, 1);
          }
        }break;
        case D_CmdKind_ClearFilter:
        {
          DF_View *view = df_view_from_handle(params->view);
          if(!df_view_is_nil(view))
          {
            view->query_string_size = 0;
            view->is_filtering = 0;
            view->query_cursor = view->query_mark = txt_pt(1, 1);
          }
        }break;
        case D_CmdKind_ApplyFilter:
        {
          DF_View *view = df_view_from_handle(params->view);
          if(!df_view_is_nil(view))
          {
            view->is_filtering = 0;
          }
        }break;
        
        //- rjf: query completion
        case D_CmdKind_CompleteQuery:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          D_CmdParamSlot slot = ws->query_cmd_spec->info.query.slot;
          
          // rjf: compound command parameters
          if(slot != D_CmdParamSlot_Null && !(ws->query_cmd_params_mask[slot/64] & (1ull<<(slot%64))))
          {
            D_CmdParams params_copy = df_cmd_params_copy(ws->query_cmd_arena, &ws->query_cmd_params);
            Rng1U64 offset_range_in_params = d_cmd_param_slot_range_table[ws->query_cmd_spec->info.query.slot];
            MemoryCopy((U8 *)(&ws->query_cmd_params) + offset_range_in_params.min,
                       (U8 *)(&params_copy) + offset_range_in_params.min,
                       dim_1u64(offset_range_in_params));
            ws->query_cmd_params_mask[slot/64] |= (1ull<<(slot%64));
          }
          
          // rjf: determine if command is ready to run
          B32 command_ready = 1;
          if(slot != D_CmdParamSlot_Null && !(ws->query_cmd_params_mask[slot/64] & (1ull<<(slot%64))))
          {
            command_ready = 0;
          }
          
          // rjf: end this query
          if(!(ws->query_cmd_spec->info.query.flags & D_CmdQueryFlag_KeepOldInput))
          {
            d_cmd(D_CmdKind_CancelQuery);
          }
          
          // rjf: push command if possible
          if(command_ready)
          {
            d_push_cmd(ws->query_cmd_spec, &ws->query_cmd_params);
          }
        }break;
        case D_CmdKind_CancelQuery:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          arena_clear(ws->query_cmd_arena);
          ws->query_cmd_spec = &d_nil_cmd_spec;
          MemoryZeroStruct(&ws->query_cmd_params);
          MemoryZeroArray(ws->query_cmd_params_mask);
          for(DF_View *v = ws->query_view_stack_top, *next = 0; !df_view_is_nil(v); v = next)
          {
            next = v->order_next;
            df_view_release(v);
          }
          ws->query_view_stack_top = &df_nil_view;
        }break;
        
        //- rjf: developer commands
        case D_CmdKind_ToggleDevMenu:
        {
          DF_Window *ws = df_window_from_handle(params->window);
          ws->dev_menu_is_open ^= 1;
        }break;
      }
    }
  }
#endif
  
  //////////////////////////////
  //- rjf: queue drag drop (TODO(rjf): @msgs)
  //
  B32 queue_drag_drop = 0;
  if(queue_drag_drop)
  {
    df_queue_drag_drop();
  }
  
  //////////////////////////////
  //- rjf: update/render all windows
  //
  {
    dr_begin_frame();
    for(EachEnumVal(DF_CfgSlot, slot))
    {
      for(MD_EachNode(tln, df_state->cfg_slot_roots[slot]->first))
      {
        if(str8_match(tln->string, str8_lit("window"), 0))
        {
          DF_Window *window = df_window_from_cfg_tree(tln);
          B32 window_is_focused = os_window_is_focused(window->os);
          if(window_is_focused)
          {
            df_state->last_focused_window = df_handle_from_cfg_tree(tln);
          }
          df_push_regs();
          df_regs()->window = df_handle_from_cfg_tree(tln);
          df_window_frame(tln);
          DF_Regs *window_regs = df_pop_regs();
          if(window_is_focused)
          {
            MemoryCopyStruct(df_regs(), window_regs);
          }
        }
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
  if(df_state->drag_drop_state == DF_DragDropState_Dropping)
  {
    df_state->drag_drop_state = DF_DragDropState_Null;
  }
  
  //////////////////////////////
  //- rjf: clear frame request state
  //
  if(df_state->num_frames_requested > 0)
  {
    df_state->num_frames_requested -= 1;
  }
  
  //////////////////////////////
  //- rjf: submit rendering to all windows
  //
  {
    r_begin_frame();
    for(U64 slot_idx = 0; slot_idx < df_state->window_slots_count; slot_idx += 1)
    {
      for(DF_WindowNode *n = df_state->window_slots[slot_idx].first; n != 0; n = n->next)
      {
        DF_Window *w = &n->v;
        r_window_begin_frame(w->os, w->r);
        dr_submit_bucket(w->os, w->r, w->draw_bucket);
        r_window_end_frame(w->os, w->r);
      }
    }
    r_end_frame();
  }
  
  //////////////////////////////
  //- rjf: show windows after first frame
  //
  if(depth == 0)
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      DF_Window *w;
    };
    Task *first_task = 0;
    Task *last_task = 0;
    for(U64 slot_idx = 0; slot_idx < df_state->window_slots_count; slot_idx += 1)
    {
      for(DF_WindowNode *n = df_state->window_slots[slot_idx].first; n != 0; n = n->next)
      {
        DF_Window *w = &n->v;
        if(w->first_frame_touched == w->last_frame_touched)
        {
          Task *t = push_array(scratch.arena, Task, 1);
          SLLQueuePush(first_task, last_task, t);
          t->w = w;
        }
      }
    }
    for(Task *t = first_task; t != 0; t = t->next)
    {
      DeferLoop(depth += 1, depth -= 1) os_window_first_paint(t->w->os);
    }
  }
  
  //////////////////////////////
  //- rjf: determine frame time, record into history
  //
  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  df_state->frame_time_us_history[df_state->frame_index%ArrayCount(df_state->frame_time_us_history)] = frame_time_us;
  
  //////////////////////////////
  //- rjf: tick frame counter
  //
  df_state->frame_index += 1;
  
  //////////////////////////////
  //- rjf: end logging
  //
  {
    LogScopeResult log = log_scope_end(scratch.arena);
    os_append_data_to_file_path(df_state->log_path, log.strings[LogMsgKind_Info]);
    if(log.strings[LogMsgKind_UserError].size != 0)
    {
      arena_clear(df_state->error_arena);
      df_state->error_string = push_str8_copy(df_state->error_arena, log.strings[LogMsgKind_UserError]);
      df_state->error_num_seconds_shown = 0.f;
      df_state->error_num_seconds_to_show = 10.f;
    }
  }
  
  di_scope_close(di_scope);
  scratch_end(scratch);
}
