// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef RADDBG_LAYER_COLOR
#define RADDBG_LAYER_COLOR 0.10f, 0.20f, 0.25f

////////////////////////////////
//~ rjf: Generated Code

#include "generated/df_gfx.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal DF_PathQuery
df_path_query_from_string(String8 string)
{
  String8 dir_str_in_input = {0};
  for(U64 i = 0; i < string.size; i += 1)
  {
    String8 substr1 = str8_substr(string, r1u64(i, i+1));
    String8 substr2 = str8_substr(string, r1u64(i, i+2));
    String8 substr3 = str8_substr(string, r1u64(i, i+3));
    if(str8_match(substr1, str8_lit("/"), StringMatchFlag_SlashInsensitive))
    {
      dir_str_in_input = str8_substr(string, r1u64(i, string.size));
    }
    else if(i != 0 && str8_match(substr2, str8_lit(":/"), StringMatchFlag_SlashInsensitive))
    {
      dir_str_in_input = str8_substr(string, r1u64(i-1, string.size));
    }
    else if(str8_match(substr2, str8_lit("./"), StringMatchFlag_SlashInsensitive))
    {
      dir_str_in_input = str8_substr(string, r1u64(i, string.size));
    }
    else if(str8_match(substr3, str8_lit("../"), StringMatchFlag_SlashInsensitive))
    {
      dir_str_in_input = str8_substr(string, r1u64(i, string.size));
    }
    if(dir_str_in_input.size != 0)
    {
      break;
    }
  }
  
  DF_PathQuery path_query = {0};
  if(dir_str_in_input.size != 0)
  {
    String8 dir = dir_str_in_input;
    String8 search = {0};
    U64 one_past_last_slash = dir.size;
    for(U64 i = 0; i < dir_str_in_input.size; i += 1)
    {
      if(dir_str_in_input.str[i] == '/' || dir_str_in_input.str[i] == '\\')
      {
        one_past_last_slash = i+1;
      }
    }
    dir.size = one_past_last_slash;
    search = str8_substr(dir_str_in_input, r1u64(one_past_last_slash, dir_str_in_input.size));
    path_query.path = dir;
    path_query.search = search;
    path_query.prefix = str8_substr(string, r1u64(0, path_query.path.str - string.str));
  }
  return path_query;
}

////////////////////////////////
//~ rjf: View Type Functions

internal B32
df_view_is_nil(DF_View *view)
{
  return (view == 0 || view == &df_g_nil_view);
}

internal DF_Handle
df_handle_from_view(DF_View *view)
{
  DF_Handle handle = df_handle_zero();
  if(!df_view_is_nil(view))
  {
    handle.u64[0] = (U64)view;
    handle.u64[1] = view->generation;
  }
  return handle;
}

internal DF_View *
df_view_from_handle(DF_Handle handle)
{
  DF_View *result = (DF_View *)handle.u64[0];
  if(df_view_is_nil(result) || result->generation != handle.u64[1])
  {
    result = &df_g_nil_view;
  }
  return result;
}

////////////////////////////////
//~ rjf: View Spec Type Functions

internal DF_GfxViewKind
df_gfx_view_kind_from_string(String8 string)
{
  DF_GfxViewKind result = DF_GfxViewKind_Null;
  for(U64 idx = 0; idx < ArrayCount(df_g_gfx_view_kind_spec_info_table); idx += 1)
  {
    if(str8_match(string, df_g_gfx_view_kind_spec_info_table[idx].name, StringMatchFlag_CaseInsensitive))
    {
      result = (DF_GfxViewKind)idx;
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
  return panel == 0 || panel == &df_g_nil_panel;
}

internal DF_Handle
df_handle_from_panel(DF_Panel *panel)
{
  DF_Handle h = {0};
  h.u64[0] = (U64)panel;
  h.u64[1] = panel->generation;
  return h;
}

internal DF_Panel *
df_panel_from_handle(DF_Handle handle)
{
  DF_Panel *panel = (DF_Panel *)handle.u64[0];
  if(panel == 0 || panel->generation != handle.u64[1])
  {
    panel = &df_g_nil_panel;
  }
  return panel;
}

internal UI_Key
df_ui_key_from_panel(DF_Panel *panel)
{
  UI_Key panel_key = ui_key_from_stringf(ui_key_zero(), "panel_window_%p", panel);
  return panel_key;
}

//- rjf: panel tree mutation notification

internal void
df_panel_notify_mutation(DF_Window *window, DF_Panel *panel)
{
  DF_CmdParams p = df_cmd_params_from_panel(window, panel);
  DF_CfgSrc src = window->cfg_src;
  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(df_g_cfg_src_write_cmd_kind_table[src]));
}

//- rjf: tree construction

internal void
df_panel_insert(DF_Panel *parent, DF_Panel *prev_child, DF_Panel *new_child)
{
  DLLInsert_NPZ(&df_g_nil_panel, parent->first, parent->last, prev_child, new_child, next, prev);
  parent->child_count += 1;
  new_child->parent = parent;
}

internal void
df_panel_remove(DF_Panel *parent, DF_Panel *child)
{
  DLLRemove_NPZ(&df_g_nil_panel, parent->first, parent->last, child, next, prev);
  child->next = child->prev = child->parent = &df_g_nil_panel;
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
    DF_Panel *uncle = &df_g_nil_panel;
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
df_rect_from_panel_child(Rng2F32 parent_rect, DF_Panel *parent, DF_Panel *panel)
{
  Rng2F32 rect = parent_rect;
  if(!df_panel_is_nil(parent))
  {
    Vec2F32 parent_rect_size = dim_2f32(parent_rect);
    Axis2 axis = parent->split_axis;
    rect.p1.v[axis] = rect.p0.v[axis];
    for(DF_Panel *child = parent->first; !df_panel_is_nil(child); child = child->next)
    {
      rect.p1.v[axis] += parent_rect_size.v[axis] * child->size_pct_of_parent.v[axis];
      rect.p1.v[axis2_flip(axis)] = rect.p0.v[axis2_flip(axis)] + parent_rect_size.v[axis2_flip(axis)] * child->size_pct_of_parent.v[axis2_flip(axis)];
      if(child == panel)
      {
        break;
      }
      rect.p0.v[axis] = rect.p1.v[axis];
    }
    rect.p0.v[axis] += parent_rect_size.v[axis] * panel->off_pct_of_parent.v[axis];
    rect.p0.v[axis2_flip(axis)] += parent_rect_size.v[axis2_flip(axis)] * panel->off_pct_of_parent.v[axis2_flip(axis)];
  }
  rect.x0 = roundf(rect.x0);
  rect.x1 = roundf(rect.x1);
  rect.y0 = roundf(rect.y0);
  rect.y1 = roundf(rect.y1);
  return rect;
}

internal Rng2F32
df_rect_from_panel(Rng2F32 root_rect, DF_Panel *root, DF_Panel *panel)
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
      parent_rect = df_rect_from_panel_child(parent_rect, parent, ancestor);
    }
  }
  
  // rjf: calculate final rect
  Rng2F32 rect = df_rect_from_panel_child(parent_rect, panel->parent, panel);
  
  scratch_end(scratch);
  return rect;
}

//- rjf: view ownership insertion/removal

internal void
df_panel_insert_tab_view(DF_Panel *panel, DF_View *prev_view, DF_View *view)
{
  DLLInsert_NPZ(&df_g_nil_view, panel->first_tab_view, panel->last_tab_view, prev_view, view, next, prev);
  panel->tab_view_count += 1;
  panel->selected_tab_view = df_handle_from_view(view);
}

internal void
df_panel_remove_tab_view(DF_Panel *panel, DF_View *view)
{
  DLLRemove_NPZ(&df_g_nil_view, panel->first_tab_view, panel->last_tab_view, view, next, prev);
  if(df_view_from_handle(panel->selected_tab_view) == view)
  {
    panel->selected_tab_view = df_handle_from_view(!df_view_is_nil(view->prev) ? view->prev : view->next);
  }
  panel->tab_view_count -= 1;
}

//- rjf: icons & display strings

internal String8
df_display_string_from_view(Arena *arena, DF_CtrlCtx ctrl_ctx, DF_View *view)
{
  String8 result = {0};
  switch(view->spec->info.name_kind)
  {
    default:
    case DF_NameKind_Null:
    {
      result = view->spec->info.display_string;
    }break;
    case DF_NameKind_EntityName:
    {
      Temp scratch = scratch_begin(&arena, 1);
      DF_Entity *entity = df_entity_from_handle(view->entity);
      String8 display_string = df_display_string_from_entity(scratch.arena, entity);
      if(display_string.size != 0)
      {
        result = push_str8_copy(arena, display_string);
      }
      else if(df_entity_is_nil(entity))
      {
        result = str8_lit("Invalid");
      }
      else
      {
        String8 kind_string = df_g_entity_kind_display_string_table[entity->kind];
        result = push_str8f(arena, "Untitled %S", kind_string);
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal DF_IconKind
df_icon_kind_from_view(DF_View *view)
{
  DF_IconKind result = view->spec->info.icon_kind;
  return result;
}

////////////////////////////////
//~ rjf: Window Type Functions

internal DF_Handle
df_handle_from_window(DF_Window *window)
{
  DF_Handle handle = {0};
  if(window != 0)
  {
    handle.u64[0] = (U64)window;
    handle.u64[1] = window->gen;
  }
  return handle;
}

internal DF_Window *
df_window_from_handle(DF_Handle handle)
{
  DF_Window *window = (DF_Window *)handle.u64[0];
  if(window != 0 && window->gen != handle.u64[1])
  {
    window = 0;
  }
  return window;
}

////////////////////////////////
//~ rjf: Control Context

internal DF_CtrlCtx
df_ctrl_ctx_from_window(DF_Window *ws)
{
  DF_CtrlCtx ctx = df_ctrl_ctx();
  df_ctrl_ctx_apply_overrides(&ctx, &ws->ctrl_ctx_overrides);
  return ctx;
}

internal DF_CtrlCtx
df_ctrl_ctx_from_view(DF_Window *ws, DF_View *view)
{
  DF_CtrlCtx ctx = df_ctrl_ctx_from_window(ws);
  df_ctrl_ctx_apply_overrides(&ctx, &view->ctrl_ctx_overrides);
  return ctx;
}

////////////////////////////////
//~ rjf: Command Parameters From Context

internal DF_CmdParams
df_cmd_params_from_gfx(void)
{
  DF_CmdParams p = df_cmd_params_zero();
  DF_Window *window = 0;
  for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
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
    p.view   = window->focused_panel->selected_tab_view;
  }
  return p;
}

internal B32
df_prefer_dasm_from_window(DF_Window *window)
{
  DF_Panel *panel = window->focused_panel;
  DF_View *view = df_view_from_handle(panel->selected_tab_view);
  DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
  B32 result = 0;
  if(view_kind == DF_GfxViewKind_Disassembly)
  {
    result = 1;
  }
  else if(view_kind == DF_GfxViewKind_Code)
  {
    result = 0;
  }
  else
  {
    B32 has_src = 0;
    B32 has_dasm = 0;
    for(DF_Panel *p = window->root_panel; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
    {
      DF_View *p_view = df_view_from_handle(p->selected_tab_view);
      DF_GfxViewKind p_view_kind = df_gfx_view_kind_from_string(p_view->spec->info.name);
      if(p_view_kind == DF_GfxViewKind_Code)
      {
        has_src = 1;
      }
      if(p_view_kind == DF_GfxViewKind_Disassembly)
      {
        has_dasm = 1;
      }
    }
    if(has_src && !has_dasm) {result = 0;}
    if(has_dasm && !has_src) {result = 1;}
  }
  return result;
}

internal DF_CmdParams
df_cmd_params_from_window(DF_Window *window)
{
  DF_CmdParams p = df_cmd_params_zero();
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(window, df_view_from_handle(window->focused_panel->selected_tab_view));
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_PreferDisassembly);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Index);
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(window->focused_panel);
  p.view   = window->focused_panel->selected_tab_view;
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = ctrl_ctx.thread;
  p.index  = ctrl_ctx.unwind_count;
  return p;
}

internal DF_CmdParams
df_cmd_params_from_panel(DF_Window *window, DF_Panel *panel)
{
  DF_CmdParams p = df_cmd_params_zero();
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(window, df_view_from_handle(panel->selected_tab_view));
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_PreferDisassembly);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Index);
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(panel);
  p.view   = panel->selected_tab_view;
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = ctrl_ctx.thread;
  p.index  = ctrl_ctx.unwind_count;
  return p;
}

internal DF_CmdParams
df_cmd_params_from_view(DF_Window *window, DF_Panel *panel, DF_View *view)
{
  DF_CmdParams p = df_cmd_params_zero();
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(window, view);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_PreferDisassembly);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Index);
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(panel);
  p.view   = df_handle_from_view(view);
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = ctrl_ctx.thread;
  p.index  = ctrl_ctx.unwind_count;
  return p;
}

internal DF_CmdParams
df_cmd_params_copy(Arena *arena, DF_CmdParams *src)
{
  DF_CmdParams dst = {0};
  MemoryCopyStruct(&dst, src);
  dst.entity_list = df_push_handle_list_copy(arena, src->entity_list);
  dst.string = push_str8_copy(arena, src->string);
  dst.file_path = push_str8_copy(arena, src->file_path);
  if(dst.cmd_spec == 0)  {dst.cmd_spec = &df_g_nil_cmd_spec;}
  if(dst.view_spec == 0) {dst.view_spec = &df_g_nil_view_spec;}
  return dst;
}

////////////////////////////////
//~ rjf: Global Cross-Window UI Interaction State Functions

internal B32
df_drag_is_active(void)
{
  return ((df_gfx_state->drag_drop_state == DF_DragDropState_Dragging) ||
          (df_gfx_state->drag_drop_state == DF_DragDropState_Dropping));
}

internal void
df_drag_begin(DF_DragDropPayload *payload)
{
  if(!df_drag_is_active())
  {
    df_gfx_state->drag_drop_state = DF_DragDropState_Dragging;
    MemoryCopyStruct(&df_g_drag_drop_payload, payload);
  }
}

internal B32
df_drag_drop(DF_DragDropPayload *out_payload)
{
  B32 result = 0;
  if(df_gfx_state->drag_drop_state == DF_DragDropState_Dropping)
  {
    result = 1;
    df_gfx_state->drag_drop_state = DF_DragDropState_Null;
    MemoryCopyStruct(out_payload, &df_g_drag_drop_payload);
    MemoryZeroStruct(&df_g_drag_drop_payload);
  }
  return result;
}

internal void
df_drag_kill(void)
{
  df_gfx_state->drag_drop_state = DF_DragDropState_Null;
  MemoryZeroStruct(&df_g_drag_drop_payload);
}

internal void
df_queue_drag_drop(void)
{
  df_gfx_state->drag_drop_state = DF_DragDropState_Dropping;
}

internal void
df_set_hovered_line_info(DF_Entity *binary, U64 voff)
{
  df_gfx_state->hover_line_binary = df_handle_from_entity(binary);
  df_gfx_state->hover_line_voff = voff;
  df_gfx_state->hover_line_set_this_frame = 1;
}

internal DF_Entity *
df_get_hovered_line_info_binary(void)
{
  return df_entity_from_handle(df_gfx_state->hover_line_binary);
}

internal U64
df_get_hovered_line_info_voff(void)
{
  return df_gfx_state->hover_line_voff;
}

////////////////////////////////
//~ rjf: View Spec State Functions

internal void
df_register_view_specs(DF_ViewSpecInfoArray specs)
{
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    DF_ViewSpecInfo *src_info = &specs.v[idx];
    U64 hash = df_hash_from_string(src_info->name);
    U64 slot_idx = hash%df_gfx_state->view_spec_table_size;
    DF_ViewSpec *spec = push_array(df_gfx_state->arena, DF_ViewSpec, 1);
    SLLStackPush_N(df_gfx_state->view_spec_table[slot_idx], spec, hash_next);
    MemoryCopyStruct(&spec->info, src_info);
    spec->info.name = push_str8_copy(df_gfx_state->arena, spec->info.name);
    spec->info.display_string = push_str8_copy(df_gfx_state->arena, spec->info.display_string);
  }
}

internal DF_ViewSpec *
df_view_spec_from_string(String8 string)
{
  DF_ViewSpec *spec = &df_g_nil_view_spec;
  U64 hash = df_hash_from_string(string);
  U64 slot_idx = hash%df_gfx_state->view_spec_table_size;
  for(DF_ViewSpec *s = df_gfx_state->view_spec_table[slot_idx];
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
df_view_spec_from_gfx_view_kind(DF_GfxViewKind gfx_view_kind)
{
  DF_ViewSpec *spec = df_view_spec_from_string(df_g_gfx_view_kind_spec_info_table[gfx_view_kind].name);
  return spec;
}

internal DF_ViewSpec *
df_view_spec_from_cmd_param_slot_spec(DF_CmdParamSlot slot, DF_CmdSpec *cmd_spec)
{
  DF_ViewSpec *spec = &df_g_nil_view_spec;
  for(DF_CmdParamSlotViewSpecRuleNode *n = df_gfx_state->cmd_param_slot_view_spec_table[slot].first;
      n != 0;
      n = n->next)
  {
    if(cmd_spec == n->cmd_spec || df_cmd_spec_is_nil(n->cmd_spec))
    {
      spec = n->view_spec;
      if(!df_cmd_spec_is_nil(n->cmd_spec))
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
df_register_gfx_view_rule_specs(DF_GfxViewRuleSpecInfoArray specs)
{
  for(U64 idx = 0; idx < specs.count; idx += 1)
  {
    // rjf: extract info from array slot
    DF_GfxViewRuleSpecInfo *info = &specs.v[idx];
    
    // rjf: skip empties
    if(info->string.size == 0)
    {
      continue;
    }
    
    // rjf: determine hash/slot
    U64 hash = df_hash_from_string(info->string);
    U64 slot_idx = hash%df_gfx_state->view_rule_spec_table_size;
    
    // rjf: allocate node & push
    DF_GfxViewRuleSpec *spec = push_array(df_gfx_state->arena, DF_GfxViewRuleSpec, 1);
    SLLStackPush_N(df_gfx_state->view_rule_spec_table[slot_idx], spec, hash_next);
    
    // rjf: fill node
    DF_GfxViewRuleSpecInfo *info_copy = &spec->info;
    MemoryCopyStruct(info_copy, info);
    info_copy->string         = push_str8_copy(df_gfx_state->arena, info->string);
  }
}

internal DF_GfxViewRuleSpec *
df_gfx_view_rule_spec_from_string(String8 string)
{
  DF_GfxViewRuleSpec *spec = &df_g_nil_gfx_view_rule_spec;
  {
    U64 hash = df_hash_from_string(string);
    U64 slot_idx = hash%df_gfx_state->view_rule_spec_table_size;
    for(DF_GfxViewRuleSpec *s = df_gfx_state->view_rule_spec_table[slot_idx]; s != 0; s = s->hash_next)
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

internal DF_View *
df_view_alloc(void)
{
  // rjf: allocate
  DF_View *view = df_gfx_state->free_view;
  {
    if(!df_view_is_nil(view))
    {
      df_gfx_state->free_view_count -= 1;
      SLLStackPop(df_gfx_state->free_view);
      U64 generation = view->generation;
      MemoryZeroStruct(view);
      view->generation = generation;
    }
    else
    {
      view = push_array(df_gfx_state->arena, DF_View, 1);
    }
    view->generation += 1;
  }
  
  // rjf: initialize
  view->arena = arena_alloc();
  view->spec = &df_g_nil_view_spec;
  view->entity = df_handle_zero();
  view->query_cursor = view->query_mark = txt_pt(1, 1);
  view->query_string_size = 0;
  df_gfx_state->allocated_view_count += 1;
  return view;
}

internal void
df_view_release(DF_View *view)
{
  SLLStackPush(df_gfx_state->free_view, view);
  for(DF_ArenaExt *ext = view->first_arena_ext; ext != 0; ext = ext->next)
  {
    arena_release(ext->arena);
  }
  view->first_arena_ext = view->last_arena_ext = 0;
  arena_release(view->arena);
  view->generation += 1;
  
  df_gfx_state->allocated_view_count -= 1;
  df_gfx_state->free_view_count += 1;
}

internal void
df_view_equip_spec(DF_View *view, DF_ViewSpec *spec, DF_Entity *entity, String8 default_query, DF_CfgNode *cfg_root)
{
  // rjf: fill arguments buffer
  view->query_string_size = Min(sizeof(view->query_buffer), default_query.size);
  MemoryCopy(view->query_buffer, default_query.str, view->query_string_size);
  view->query_cursor = view->query_mark = txt_pt(1, default_query.size+1);
  
  // rjf: initialize state for new view spec, if needed
  if(view->spec != spec || spec == &df_g_nil_view_spec)
  {
    DF_ViewSetupFunctionType *view_setup = spec->info.setup_hook;
    df_view_clear_user_state(view);
    MemoryZeroStruct(&view->scroll_pos);
    view->spec = spec;
    view->entity = df_handle_from_entity(entity);
    view->is_filtering = 0;
    view->is_filtering_t = 0;
    view_setup(view, cfg_root);
  }
}

internal void
df_view_equip_loading_info(DF_View *view, B32 is_loading, U64 progress_v, U64 progress_target)
{
  view->loading_t_target = (F32)!!is_loading;
  view->loading_progress_v = progress_v;
  view->loading_progress_v_target = progress_target;
}

internal void
df_view_clear_user_state(DF_View *view)
{
  for(DF_ArenaExt *ext = view->first_arena_ext; ext != 0; ext = ext->next)
  {
    arena_release(ext->arena);
  }
  view->first_arena_ext = view->last_arena_ext = 0;
  arena_clear(view->arena);
  view->user_data = 0;
}

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

////////////////////////////////
//~ rjf: View Rule Instance State Functions

internal void *
df_view_rule_block_get_or_push_user_state(DF_ExpandKey key, U64 size)
{
  U64 hash = df_hash_from_expand_key(key);
  U64 slot_idx = hash%df_gfx_state->view_rule_block_slots_count;
  DF_ViewRuleBlockSlot *slot = &df_gfx_state->view_rule_block_slots[slot_idx];
  DF_ViewRuleBlockNode *node = 0;
  for(DF_ViewRuleBlockNode *n = slot->first; n != 0; n = n->next)
  {
    if(df_expand_key_match(n->key, key))
    {
      node = n;
      break;
    }
  }
  if(node == 0)
  {
    node = df_gfx_state->free_view_rule_block_node;
    if(node != 0)
    {
      SLLStackPop(df_gfx_state->free_view_rule_block_node);
    }
    else
    {
      node = push_array(df_gfx_state->arena, DF_ViewRuleBlockNode, 1);
    }
    node->key = key;
    node->user_state_arena = arena_alloc();
    SLLQueuePush(slot->first, slot->last, node);
  }
  void *user_state = node->user_state;
  if(user_state == 0 || node->user_state_size != size)
  {
    arena_clear(node->user_state_arena);
    user_state = node->user_state = push_array(node->user_state_arena, U8, size);
    node->user_state_size = size;
  }
  return user_state;
}

internal Arena *
df_view_rule_block_push_arena_ext(DF_ExpandKey key)
{
  // TODO(rjf)
  return 0;
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
  panel->first = panel->last = panel->next = panel->prev = panel->parent = &df_g_nil_panel;
  panel->first_tab_view = panel->last_tab_view = &df_g_nil_view;
  panel->generation += 1;
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
    next = view->next;
    df_view_release(view);
  }
  panel->first_tab_view = panel->last_tab_view = &df_g_nil_view;
  panel->selected_tab_view = df_handle_zero();
}

////////////////////////////////
//~ rjf: Window State Functions

internal DF_Window *
df_window_open(Vec2F32 size, OS_Handle preferred_monitor, DF_CfgSrc cfg_src)
{
  DF_Window *window = df_gfx_state->free_window;
  if(window != 0)
  {
    SLLStackPop(df_gfx_state->free_window);
    U64 gen = window->gen;
    MemoryZeroStruct(window);
    window->gen = gen;
  }
  else
  {
    window = push_array(df_gfx_state->arena, DF_Window, 1);
  }
  window->gen += 1;
  window->cfg_src = cfg_src;
  window->arena = arena_alloc();
  {
    String8 title = str8_lit_comp(BUILD_TITLE_STRING_LITERAL);
    window->os = os_window_open(size, title);
  }
  window->r = r_window_equip(window->os);
  window->ui = ui_state_alloc();
  window->view_state_hist = df_state_delta_history_alloc();
  window->drop_completion_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_drop_complete_ctx_menu_"));
  window->entity_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_entity_ctx_menu_"));
  window->tab_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_tab_ctx_menu_"));
  window->hover_eval_arena = arena_alloc();
  window->free_panel = &df_g_nil_panel;
  window->root_panel = df_panel_alloc(window);
  window->focused_panel = window->root_panel;
  window->query_cmd_arena = arena_alloc();
  window->query_cmd_spec = &df_g_nil_cmd_spec;
  window->query_view_stack_top = &df_g_nil_view;
  if(df_gfx_state->first_window == 0)
  {
    DF_FontSlot english_font_slots[] = {DF_FontSlot_Main, DF_FontSlot_Code};
    DF_FontSlot icon_font_slot = DF_FontSlot_Icons;
    for(U64 idx = 0; idx < ArrayCount(english_font_slots); idx += 1)
    {
      Temp scratch = scratch_begin(0, 0);
      DF_FontSlot slot = english_font_slots[idx];
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(slot),
                             df_font_size_from_slot(window, slot),
                             0,
                             str8_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#$%^&*()-_+=[{]}\\|;:'\",<.>/?"));
      scratch_end(scratch);
    }
    for(DF_IconKind icon_kind = DF_IconKind_Null; icon_kind < DF_IconKind_COUNT; icon_kind = (DF_IconKind)(icon_kind+1))
    {
      Temp scratch = scratch_begin(0, 0);
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(icon_font_slot),
                             df_font_size_from_slot(window, icon_font_slot),
                             0,
                             df_g_icon_kind_text_table[icon_kind]);
      scratch_end(scratch);
    }
  }
  OS_Handle zero_monitor = {0};
  if(!os_handle_match(zero_monitor, preferred_monitor))
  {
    os_window_set_monitor(window->os, preferred_monitor);
  }
  os_window_equip_repaint(window->os, df_gfx_state->repaint_hook, window);
  DLLPushBack(df_gfx_state->first_window, df_gfx_state->last_window, window);
  return window;
}

internal DF_Window *
df_window_from_os_handle(OS_Handle os)
{
  DF_Window *result = 0;
  for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
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
df_window_update_and_render(Arena *arena, OS_EventList *events, DF_Window *ws, DF_CmdList *cmds)
{
  ProfBeginFunction();
  
  //////////////////////////////
  //- rjf: unpack context
  //
  B32 window_is_focused = os_window_is_focused(ws->os);
  B32 confirm_open = df_gfx_state->confirm_active;
  B32 query_is_open = !df_view_is_nil(ws->query_view_stack_top);
  B32 hover_eval_is_open = (!confirm_open &&
                            ws->hover_eval_string.size != 0 &&
                            ws->hover_eval_first_frame_idx+20 < ws->hover_eval_last_frame_idx &&
                            df_frame_index()-ws->hover_eval_last_frame_idx < 20);
  if(!window_is_focused || confirm_open)
  {
    ws->menu_bar_key_held = 0;
  }
  ui_select_state(ws->ui);
  
  //////////////////////////////
  //- rjf: auto-close tabs which have parameter entities that've been deleted
  //
  for(DF_Panel *panel = ws->root_panel;
      !df_panel_is_nil(panel);
      panel = df_panel_rec_df_pre(panel).next)
  {
    for(DF_View *view = panel->first_tab_view;
        !df_view_is_nil(view);
        view = view->next)
    {
      DF_Entity *entity = df_entity_from_handle(view->entity);
      if(entity->flags & DF_EntityFlag_MarkedForDeletion || (df_entity_is_nil(entity) && !df_handle_match(df_handle_zero(), view->entity)))
      {
        DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
        df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseTab));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: do core-layer commands & batch up commands to be dispatched to views
  //
  UI_NavActionList nav_actions = {0};
  ProfScope("do commands")
  {
    Temp scratch = scratch_begin(&arena, 1);
    for(DF_CmdNode *cmd_node = cmds->first;
        cmd_node != 0;
        cmd_node = cmd_node->next)
    {
      temp_end(scratch);
      
      // rjf: get command info
      DF_Cmd *cmd = &cmd_node->cmd;
      DF_CmdParams params = cmd->params;
      DF_CoreCmdKind core_cmd_kind = df_core_cmd_kind_from_string(cmd->spec->info.string);
      
      // rjf: mismatched window => skip
      if(df_window_from_handle(params.window) != ws)
      {
        continue;
      }
      
      // rjf: set up data for cases
      Axis2 split_axis = Axis2_X;
      U64 panel_sib_off = 0;
      U64 panel_child_off = 0;
      Vec2S32 panel_change_dir = {0};
      
      // rjf: dispatch by core command kind
      switch(core_cmd_kind)
      {
        //- rjf: default -> try to open tabs for "view driver" commands
        default:
        {
          String8 name = cmd->spec->info.string;
          DF_ViewSpec *view_spec = df_view_spec_from_string(name);
          if(view_spec != &df_g_nil_view_spec)
          {
            DF_CmdParams p = params;
            p.view_spec = view_spec;
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_ViewSpec);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_OpenTab));
          }
        }break;
        
        //- rjf: command fast path
        case DF_CoreCmdKind_RunCommand:
        {
          DF_CmdSpec *spec = params.cmd_spec;
          
          // rjf: command simply executes - just no-op in this layer
          if(!(spec->info.query.flags & DF_CmdQueryFlag_Required) &&
             !df_cmd_spec_is_nil(spec) &&
             (spec->info.query.slot == DF_CmdParamSlot_Null || df_cmd_params_has_slot(&params, spec->info.query.slot)))
          {
          }
          
          // rjf: command is missing arguments -> prep query
          else
          {
            arena_clear(ws->query_cmd_arena);
            ws->query_cmd_spec   = df_cmd_spec_is_nil(spec) ? cmd->spec : spec;
            ws->query_cmd_params = df_cmd_params_copy(ws->query_cmd_arena, &params);
            ws->query_view_selected = 1;
          }
        }break;
        
        //- rjf: notifications
        case DF_CoreCmdKind_Error:
        {
          String8 error_string = params.string;
          ws->error_string_size = error_string.size;
          MemoryCopy(ws->error_buffer, error_string.str, Min(sizeof(ws->error_buffer), error_string.size));
          ws->error_t = 1;
        }break;
        
        //- rjf: debug control context management operations
        case DF_CoreCmdKind_SelectThread:goto thread_locator;
        case DF_CoreCmdKind_SelectThreadWindow:
        {
          ws->ctrl_ctx_overrides.thread = params.entity;
          ws->ctrl_ctx_overrides.unwind_count = 0;
        }goto thread_locator;
        case DF_CoreCmdKind_SelectThreadView:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(params.view);
          if(df_view_is_nil(view) && !df_panel_is_nil(panel))
          {
            view = df_view_from_handle(panel->selected_tab_view);
          }
          if(!df_view_is_nil(view))
          {
            view->ctrl_ctx_overrides.thread = params.entity;
            view->ctrl_ctx_overrides.unwind_count = 0;
          }
        }goto thread_locator;
        case DF_CoreCmdKind_SelectUnwind:
        thread_locator:;
        {
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindThread));
        }break;
        
        //- rjf: font sizes
        case DF_CoreCmdKind_IncUIFontScale:
        {
          ws->main_font_size_delta += 1/200.f;
          ws->main_font_size_delta = ClampTop(ws->main_font_size_delta, +0.3f);
        }break;
        case DF_CoreCmdKind_DecUIFontScale:
        {
          ws->main_font_size_delta -= 1/200.f;
          ws->main_font_size_delta = ClampBot(ws->main_font_size_delta, -0.075f);
        }break;
        case DF_CoreCmdKind_IncCodeFontScale:
        {
          ws->code_font_size_delta += 1/200.f;
          ws->code_font_size_delta = ClampTop(ws->code_font_size_delta, +0.3f);
        }break;
        case DF_CoreCmdKind_DecCodeFontScale:
        {
          ws->code_font_size_delta -= 1/200.f;
          ws->code_font_size_delta = ClampBot(ws->code_font_size_delta, -0.075f);
        }break;
        
        //- rjf: panel creation
        case DF_CoreCmdKind_NewPanelRight: split_axis = Axis2_X; goto split;
        case DF_CoreCmdKind_NewPanelDown:  split_axis = Axis2_Y; goto split;
        split:;
        {
          DF_Panel *panel = ws->focused_panel;
          DF_Panel *parent = panel->parent;
          if(!df_panel_is_nil(parent) && parent->split_axis == split_axis)
          {
            DF_Panel *next = df_panel_alloc(ws);
            df_panel_insert(parent, panel, next);
            next->size_pct_of_parent_target.v[split_axis] = 1.f / parent->child_count;
            next->size_pct_of_parent.v[axis2_flip(split_axis)] = next->size_pct_of_parent_target.v[axis2_flip(split_axis)] = 1.f;
            for(DF_Panel *child = parent->first; !df_panel_is_nil(child); child = child->next)
            {
              if(child != next)
              {
                child->size_pct_of_parent_target.v[split_axis] *= (F32)(parent->child_count-1) / (parent->child_count);
              }
            }
            ws->focused_panel = next;
          }
          else
          {
            DF_Panel *pre_prev = panel->prev;
            DF_Panel *pre_parent = parent;
            DF_Panel *new_parent = df_panel_alloc(ws);
            new_parent->size_pct_of_parent.v[split_axis] = panel->size_pct_of_parent.v[split_axis];
            new_parent->size_pct_of_parent_target.v[split_axis] = panel->size_pct_of_parent_target.v[split_axis];
            new_parent->size_pct_of_parent.v[axis2_flip(split_axis)] = new_parent->size_pct_of_parent_target.v[axis2_flip(split_axis)] = panel->size_pct_of_parent_target.v[axis2_flip(split_axis)];
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
            df_panel_insert(new_parent, &df_g_nil_panel, left);
            df_panel_insert(new_parent, left, right);
            new_parent->split_axis = split_axis;
            left->size_pct_of_parent.v[split_axis] = 1.f;
            left->size_pct_of_parent_target.v[split_axis] = 0.5f;
            right->size_pct_of_parent.v[split_axis] = 0.f;
            right->size_pct_of_parent_target.v[split_axis] = 0.5f;
            left->size_pct_of_parent.v[axis2_flip(split_axis)] = 1.f;
            left->size_pct_of_parent_target.v[axis2_flip(split_axis)] = 1.f;
            right->size_pct_of_parent.v[axis2_flip(split_axis)] = 1.f;
            right->size_pct_of_parent_target.v[axis2_flip(split_axis)] = 1.f;
            ws->focused_panel = right;
          }
          df_panel_notify_mutation(ws, panel);
        }break;
        case DF_CoreCmdKind_ResetToDefaultPanels:
        {
          //- rjf: gather all panels in the panel tree - remove & gather views
          // we'd like to keep in the next layout
          DF_HandleList panels_to_close = {0};
          DF_HandleList views_to_close = {0};
          DF_View *watch = &df_g_nil_view;
          DF_View *locals = &df_g_nil_view;
          DF_View *regs = &df_g_nil_view;
          DF_View *globals = &df_g_nil_view;
          DF_View *tlocals = &df_g_nil_view;
          DF_View *types = &df_g_nil_view;
          DF_View *procs = &df_g_nil_view;
          DF_View *callstack = &df_g_nil_view;
          DF_View *breakpoints = &df_g_nil_view;
          DF_View *watch_pins = &df_g_nil_view;
          DF_View *output = &df_g_nil_view;
          DF_View *targets = &df_g_nil_view;
          DF_View *scheduler = &df_g_nil_view;
          DF_View *modules = &df_g_nil_view;
          DF_View *disasm = &df_g_nil_view;
          DF_View *memory = &df_g_nil_view;
          DF_HandleList code_views = {0};
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            DF_Handle handle = df_handle_from_panel(panel);
            df_handle_list_push(scratch.arena, &panels_to_close, handle);
            for(DF_View *view = panel->first_tab_view, *next = 0; !df_view_is_nil(view); view = next)
            {
              next = view->next;
              DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
              B32 needs_delete = 1;
              switch(view_kind)
              {
                default:{}break;
                case DF_GfxViewKind_Watch:       {if(df_view_is_nil(watch))               { needs_delete = 0; watch = view;} }break;
                case DF_GfxViewKind_Locals:      {if(df_view_is_nil(locals))              { needs_delete = 0; locals = view;} }break;
                case DF_GfxViewKind_Registers:   {if(df_view_is_nil(regs))                { needs_delete = 0; regs = view;} }break;
                case DF_GfxViewKind_Globals:     {if(df_view_is_nil(globals))             { needs_delete = 0; globals = view;} }break;
                case DF_GfxViewKind_ThreadLocals:{if(df_view_is_nil(tlocals))             { needs_delete = 0; tlocals = view;} }break;
                case DF_GfxViewKind_Types:       {if(df_view_is_nil(types))               { needs_delete = 0; types = view;} }break;
                case DF_GfxViewKind_Procedures:  {if(df_view_is_nil(procs))               { needs_delete = 0; procs = view;} }break;
                case DF_GfxViewKind_CallStack:   {if(df_view_is_nil(callstack))           { needs_delete = 0; callstack = view;} }break;
                case DF_GfxViewKind_Breakpoints: {if(df_view_is_nil(breakpoints))         { needs_delete = 0; breakpoints = view;} }break;
                case DF_GfxViewKind_WatchPins:   {if(df_view_is_nil(watch_pins))          { needs_delete = 0; watch_pins = view;} }break;
                case DF_GfxViewKind_Output:      {if(df_view_is_nil(output))              { needs_delete = 0; output = view;} }break;
                case DF_GfxViewKind_Targets:     {if(df_view_is_nil(targets))             { needs_delete = 0; targets = view;} }break;
                case DF_GfxViewKind_Scheduler:   {if(df_view_is_nil(scheduler))           { needs_delete = 0; scheduler = view;} }break;
                case DF_GfxViewKind_Modules:     {if(df_view_is_nil(modules))             { needs_delete = 0; modules = view;} }break;
                case DF_GfxViewKind_Disassembly: {if(df_view_is_nil(disasm))              { needs_delete = 0; disasm = view;} }break;
                case DF_GfxViewKind_Memory:      {if(df_view_is_nil(memory))              { needs_delete = 0; memory = view;} }break;
                case DF_GfxViewKind_Code:
                {
                  needs_delete = 0;
                  df_handle_list_push(scratch.arena, &code_views, df_handle_from_view(view));
                }break;
              }
              if(!needs_delete)
              {
                df_panel_remove_tab_view(panel, view);
              }
            }
          }
          
          //- rjf: close all panels/views
          for(DF_HandleNode *n = panels_to_close.first; n != 0; n = n->next)
          {
            DF_Panel *panel = df_panel_from_handle(n->handle);
            if(panel != ws->root_panel)
            {
              df_panel_release(ws, panel);
            }
            else
            {
              df_panel_release_all_views(panel);
              panel->first = panel->last = &df_g_nil_panel;
            }
          }
          
          //- rjf: allocate any missing views
          DF_CmdParams blank_params = df_cmd_params_from_window(ws);
          if(df_view_is_nil(watch))
          {
            watch = df_view_alloc();
            df_view_equip_spec(watch, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Watch), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(locals))
          {
            locals = df_view_alloc();
            df_view_equip_spec(locals, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Locals), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(regs))
          {
            regs = df_view_alloc();
            df_view_equip_spec(regs, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Registers), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(globals))
          {
            globals = df_view_alloc();
            df_view_equip_spec(globals, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Globals), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(tlocals))
          {
            tlocals = df_view_alloc();
            df_view_equip_spec(tlocals, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_ThreadLocals), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(types))
          {
            types = df_view_alloc();
            df_view_equip_spec(types, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Types), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(procs))
          {
            procs = df_view_alloc();
            df_view_equip_spec(procs, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Procedures), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(callstack))
          {
            callstack = df_view_alloc();
            df_view_equip_spec(callstack, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_CallStack), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(breakpoints))
          {
            breakpoints = df_view_alloc();
            df_view_equip_spec(breakpoints, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Breakpoints), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(watch_pins))
          {
            watch_pins = df_view_alloc();
            df_view_equip_spec(watch_pins, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_WatchPins), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(output))
          {
            output = df_view_alloc();
            df_view_equip_spec(output, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Output), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(targets))
          {
            targets = df_view_alloc();
            df_view_equip_spec(targets, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Targets), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(scheduler))
          {
            scheduler = df_view_alloc();
            df_view_equip_spec(scheduler, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Scheduler), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(modules))
          {
            modules = df_view_alloc();
            df_view_equip_spec(modules, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Modules), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(disasm))
          {
            disasm = df_view_alloc();
            df_view_equip_spec(disasm, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Disassembly), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(memory))
          {
            memory = df_view_alloc();
            df_view_equip_spec(memory, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Memory), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          
          // rjf: root split
          ws->root_panel->split_axis = Axis2_X;
          DF_Panel *root_0 = df_panel_alloc(ws);
          DF_Panel *root_1 = df_panel_alloc(ws);
          df_panel_insert(ws->root_panel, ws->root_panel->last, root_0);
          df_panel_insert(ws->root_panel, ws->root_panel->last, root_1);
          root_0->size_pct_of_parent = root_0->size_pct_of_parent_target = v2f32(0.85f, 1.f);
          root_1->size_pct_of_parent = root_1->size_pct_of_parent_target = v2f32(0.15f, 1.f);
          
          // rjf: root_0 split
          root_0->split_axis = Axis2_Y;
          DF_Panel *root_0_0 = df_panel_alloc(ws);
          DF_Panel *root_0_1 = df_panel_alloc(ws);
          df_panel_insert(root_0, root_0->last, root_0_0);
          df_panel_insert(root_0, root_0->last, root_0_1);
          root_0_0->size_pct_of_parent = root_0_0->size_pct_of_parent_target = v2f32(1.f, 0.80f);
          root_0_1->size_pct_of_parent = root_0_1->size_pct_of_parent_target = v2f32(1.f, 0.20f);
          
          // rjf: root_1 split
          root_1->split_axis = Axis2_Y;
          DF_Panel *root_1_0 = df_panel_alloc(ws);
          DF_Panel *root_1_1 = df_panel_alloc(ws);
          df_panel_insert(root_1, root_1->last, root_1_0);
          df_panel_insert(root_1, root_1->last, root_1_1);
          root_1_0->size_pct_of_parent = root_1_0->size_pct_of_parent_target = v2f32(1.f, 0.50f);
          root_1_1->size_pct_of_parent = root_1_1->size_pct_of_parent_target = v2f32(1.f, 0.50f);
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
          root_0_0_0->size_pct_of_parent = root_0_0_0->size_pct_of_parent_target = v2f32(0.25f, 1.f);
          root_0_0_1->size_pct_of_parent = root_0_0_1->size_pct_of_parent_target = v2f32(0.75f, 1.f);
          
          // rjf: root_0_0_0 split
          root_0_0_0->split_axis = Axis2_Y;
          DF_Panel *root_0_0_0_0 = df_panel_alloc(ws);
          DF_Panel *root_0_0_0_1 = df_panel_alloc(ws);
          df_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_0);
          df_panel_insert(root_0_0_0, root_0_0_0->last, root_0_0_0_1);
          root_0_0_0_0->size_pct_of_parent = root_0_0_0_0->size_pct_of_parent_target = v2f32(1.f, 0.5f);
          root_0_0_0_1->size_pct_of_parent = root_0_0_0_1->size_pct_of_parent_target = v2f32(1.f, 0.5f);
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
          root_0_1_0->size_pct_of_parent = root_0_1_0->size_pct_of_parent_target = v2f32(0.60f, 1.f);
          root_0_1_1->size_pct_of_parent = root_0_1_1->size_pct_of_parent_target = v2f32(0.40f, 1.f);
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
          
          // rjf: fill main panel with all collected code views
          for(DF_HandleNode *n = code_views.first; n != 0; n = n->next)
          {
            DF_View *view = df_view_from_handle(n->handle);
            if(!df_view_is_nil(view))
            {
              df_panel_insert_tab_view(root_0_0_1, root_0_0_1->last_tab_view, view);
            }
          }
          
          // rjf: choose initial focused panel
          ws->focused_panel = root_0_0_1;
          
          // rjf: dispatch cfg saves
          for(DF_CfgSrc src = (DF_CfgSrc)0; src < DF_CfgSrc_COUNT; src = (DF_CfgSrc)(src+1))
          {
            DF_CoreCmdKind write_cmd = df_g_cfg_src_write_cmd_kind_table[src];
            DF_CmdParams p = df_cmd_params_zero();
            p.file_path = df_cfg_path_from_src(src);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(write_cmd));
          }
        }break;
        
        //- rjf: panel rotation
        case DF_CoreCmdKind_RotatePanelColumns:
        {
          DF_Panel *panel = ws->focused_panel;
          DF_Panel *parent = &df_g_nil_panel;
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
            old_first->next = &df_g_nil_panel;
            old_first->prev = parent->last;
            parent->last->next = old_first;
            new_first->prev = &df_g_nil_panel;
            parent->first = new_first;
            parent->last = old_first;
          }
          df_panel_notify_mutation(ws, panel);
        }break;
        
        //- rjf: focused panel cycling
        case DF_CoreCmdKind_NextPanel: panel_sib_off = OffsetOf(DF_Panel, next); panel_child_off = OffsetOf(DF_Panel, first); goto cycle;
        case DF_CoreCmdKind_PrevPanel: panel_sib_off = OffsetOf(DF_Panel, prev); panel_child_off = OffsetOf(DF_Panel, last); goto cycle;
        cycle:;
        {
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
              DF_CmdParams p = df_cmd_params_from_window(ws);
              p.panel = df_handle_from_panel(panel);
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
              break;
            }
          }
        }break;
        case DF_CoreCmdKind_FocusPanel:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          if(!df_panel_is_nil(panel))
          {
            ws->focused_panel = panel;
            ws->menu_bar_focused = 0;
            ws->query_view_selected = 0;
          }
        }break;
        
        //- rjf: directional panel focus changing
        case DF_CoreCmdKind_FocusPanelRight: panel_change_dir = v2s32(+1, +0); goto focus_panel_dir;
        case DF_CoreCmdKind_FocusPanelLeft:  panel_change_dir = v2s32(-1, +0); goto focus_panel_dir;
        case DF_CoreCmdKind_FocusPanelUp:    panel_change_dir = v2s32(+0, -1); goto focus_panel_dir;
        case DF_CoreCmdKind_FocusPanelDown:  panel_change_dir = v2s32(+0, +1); goto focus_panel_dir;
        focus_panel_dir:;
        {
          DF_Panel *src_panel = ws->focused_panel;
          Rng2F32 src_panel_rect = df_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, src_panel);
          Vec2F32 src_panel_center = center_2f32(src_panel_rect);
          Vec2F32 src_panel_half_dim = scale_2f32(dim_2f32(src_panel_rect), 0.5f);
          Vec2F32 travel_dim = add_2f32(src_panel_half_dim, v2f32(10.f, 10.f));
          Vec2F32 travel_dst = add_2f32(src_panel_center, mul_2f32(travel_dim, v2f32((F32)panel_change_dir.x, (F32)panel_change_dir.y)));
          DF_Panel *dst_root = &df_g_nil_panel;
          for(DF_Panel *p = ws->root_panel; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
          {
            if(p == src_panel || !df_panel_is_nil(p->first))
            {
              continue;
            }
            Rng2F32 p_rect = df_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, p);
            if(contains_2f32(p_rect, travel_dst))
            {
              dst_root = p;
              break;
            }
          }
          if(!df_panel_is_nil(dst_root))
          {
            DF_Panel *dst_panel = &df_g_nil_panel;
            for(DF_Panel *p = dst_root; !df_panel_is_nil(p); p = df_panel_rec_df_pre(p).next)
            {
              if(df_panel_is_nil(p->first) && p != src_panel)
              {
                dst_panel = p;
                break;
              }
            }
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.panel = df_handle_from_panel(dst_panel);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
          }
        }break;
        
        //- rjf: focus history
        case DF_CoreCmdKind_GoBack:
        {
          df_state_delta_history_wind(ws->view_state_hist, Side_Min);
        }break;
        case DF_CoreCmdKind_GoForward:
        {
          df_state_delta_history_wind(ws->view_state_hist, Side_Max);
        }break;
        
        //- rjf: panel removal
        case DF_CoreCmdKind_ClosePanel:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_Panel *parent = panel->parent;
          if(!df_panel_is_nil(parent))
          {
            df_panel_notify_mutation(ws, panel);
            Axis2 split_axis = parent->split_axis;
            
            // NOTE(rjf): If we're removing all but the last child of this parent,
            // we should just remove both children.
            if(parent->child_count == 2)
            {
              DF_Panel *discard_child = panel;
              DF_Panel *keep_child = panel == parent->first ? parent->last : parent->first;
              DF_Panel *grandparent = parent->parent;
              DF_Panel *parent_prev = parent->prev;
              Vec2F32 size_pct_of_parent = parent->size_pct_of_parent_target;
              
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
              keep_child->size_pct_of_parent_target = size_pct_of_parent;
              keep_child->size_pct_of_parent.v[split_axis] *= size_pct_of_parent.v[split_axis];
              keep_child->size_pct_of_parent.v[axis2_flip(split_axis)] *= size_pct_of_parent.v[axis2_flip(split_axis)];
              
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
                  child->size_pct_of_parent_target.v[keep_child->split_axis] *= keep_child->size_pct_of_parent_target.v[grandparent->split_axis];
                  child->size_pct_of_parent.v[keep_child->split_axis]        *= keep_child->size_pct_of_parent_target.v[grandparent->split_axis];
                }
                df_panel_release(ws, keep_child);
              }
            }
            // NOTE(rjf): Otherwise we can just remove this child.
            else
            {
              DF_Panel *next = &df_g_nil_panel;
              F32 removed_size_pct = panel->size_pct_of_parent_target.v[parent->split_axis];
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
                child->size_pct_of_parent_target.v[parent->split_axis] /= 1.f-removed_size_pct;
              }
            }
          }
        }break;
        
        //- rjf: panel tab controls
        case DF_CoreCmdKind_NextTab:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          view = view->next;
          if(df_view_is_nil(view))
          {
            view = panel->first_tab_view;
          }
          panel->selected_tab_view = df_handle_from_view(view);
        }break;
        case DF_CoreCmdKind_PrevTab:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          view = view->prev;
          if(df_view_is_nil(view))
          {
            view = panel->last_tab_view;
          }
          panel->selected_tab_view = df_handle_from_view(view);
        }break;
        case DF_CoreCmdKind_MoveTabRight:
        case DF_CoreCmdKind_MoveTabLeft:
        {
          DF_Panel *panel = ws->focused_panel;
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          DF_View *prev_view = core_cmd_kind == DF_CoreCmdKind_MoveTabRight ? view->next : view->prev->prev;
          if(!df_view_is_nil(prev_view) || core_cmd_kind == DF_CoreCmdKind_MoveTabLeft)
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.panel = df_handle_from_panel(panel);
            p.dest_panel = df_handle_from_panel(panel);
            p.view = df_handle_from_view(view);
            p.prev_view = df_handle_from_view(prev_view);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_MoveTab));
          }
        }break;
        case DF_CoreCmdKind_OpenTab:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_ViewSpec *spec = params.view_spec;
          DF_Entity *entity = &df_g_nil_entity;
          if(spec->info.flags & DF_ViewSpecFlag_ParameterizedByEntity)
          {
            entity = df_entity_from_handle(params.entity);
          }
          if(!df_panel_is_nil(panel) && spec != &df_g_nil_view_spec)
          {
            DF_View *view = df_view_alloc();
            df_view_equip_spec(view, spec, entity, str8_lit(""), &df_g_nil_cfg_node);
            df_panel_insert_tab_view(panel, panel->last_tab_view, view);
            df_panel_notify_mutation(ws, panel);
          }
        }break;
        case DF_CoreCmdKind_CloseTab:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(params.view);
          if(!df_view_is_nil(view))
          {
            df_panel_remove_tab_view(panel, view);
            df_view_release(view);
            df_panel_notify_mutation(ws, panel);
          }
        }break;
        case DF_CoreCmdKind_MoveTab:
        {
          DF_Panel *src_panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(params.view);
          DF_Panel *dst_panel = df_panel_from_handle(params.dest_panel);
          DF_View *prev_view = df_view_from_handle(params.prev_view);
          if(!df_panel_is_nil(src_panel) &&
             !df_panel_is_nil(dst_panel) &&
             prev_view != view)
          {
            df_panel_remove_tab_view(src_panel, view);
            df_panel_insert_tab_view(dst_panel, prev_view, view);
            ws->focused_panel = dst_panel;
            df_panel_notify_mutation(ws, dst_panel);
          }
        }break;
        case DF_CoreCmdKind_TabBarTop:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          panel->tab_side = Side_Min;
          df_panel_notify_mutation(ws, panel);
        }break;
        case DF_CoreCmdKind_TabBarBottom:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          panel->tab_side = Side_Max;
          df_panel_notify_mutation(ws, panel);
        }break;
        
        //- rjf: files
        case DF_CoreCmdKind_Open:
        {
          DF_Entity *entity = df_entity_from_path(params.file_path, DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
          if(!(entity->flags & DF_EntityFlag_IsMissing) && !(entity->flags & DF_EntityFlag_IsFolder))
          {
            DF_CmdParams p = params;
            p.window = df_handle_from_window(ws);
            p.panel = df_handle_from_panel(ws->focused_panel);
            p.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PendingEntity));
          }
        }break;
        case DF_CoreCmdKind_Reload:
        {
          DF_Entity *file = df_entity_from_handle(params.entity);
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            DF_View *view = df_view_from_handle(panel->selected_tab_view);
            DF_Entity *view_entity = df_entity_from_handle(view->entity);
            if(view_entity == file)
            {
              view->flash_t = 1.f;
            }
          }
        }break;
        case DF_CoreCmdKind_ReloadActive:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          DF_Entity *entity = df_entity_from_handle(view->entity);
          if(entity->kind == DF_EntityKind_File)
          {
            DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
            p.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Reload));
          }
        }break;
        case DF_CoreCmdKind_Switch:
        {
          B32 already_opened = 0;
          DF_Panel *panel = df_panel_from_handle(params.panel);
          for(DF_View *v = panel->first_tab_view; !df_view_is_nil(v); v = v->next)
          {
            DF_Entity *v_param_entity = df_entity_from_handle(v->entity);
            if(v_param_entity == df_entity_from_handle(params.entity))
            {
              panel->selected_tab_view = df_handle_from_view(v);
              already_opened = 1;
              break;
            }
          }
          if(already_opened == 0)
          {
            DF_CmdParams p = params;
            p.window = df_handle_from_window(ws);
            p.panel = df_handle_from_panel(ws->focused_panel);
            p.entity = params.entity;
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
            df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PendingEntity));
          }
        }break;
        case DF_CoreCmdKind_SwitchToPartnerFile:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          DF_Entity *entity = df_entity_from_handle(view->entity);
          DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
          if(view_kind == DF_GfxViewKind_Code && entity->kind == DF_EntityKind_File)
          {
            String8 file_full_path = df_full_path_from_entity(scratch.arena, entity);
            String8 file_folder = str8_chop_last_slash(file_full_path);
            String8 file_name = str8_chop_last_dot(entity->name);
            String8 file_ext = str8_skip_last_dot(entity->name);
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
                  DF_Entity *candidate = df_entity_from_path(candidate_path, DF_EntityFromPathFlag_OpenAsNeeded);
                  DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                  p.entity = df_handle_from_entity(candidate);
                  df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Switch));
                  break;
                }
              }
            }
          }
        }break;
        
        //- rjf: directional movement & text controls
        //
        // NOTE(rjf): These all get funneled into a separate intermediate that
        // can be used by the UI build phase for navigation and stuff, as well
        // as builder codepaths that want to use these controls to modify text.
        //
        case DF_CoreCmdKind_MoveLeft:
        {
          UI_NavAction action = {UI_NavActionFlag_PickSelectSide|UI_NavActionFlag_ZeroDeltaOnSelect|UI_NavActionFlag_ExplicitDirectional, {-1, +0}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveRight:
        {
          UI_NavAction action = {UI_NavActionFlag_PickSelectSide|UI_NavActionFlag_ZeroDeltaOnSelect|UI_NavActionFlag_ExplicitDirectional, {+1, +0}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUp:
        {
          UI_NavAction action = {UI_NavActionFlag_ExplicitDirectional, {+0, -1}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDown:
        {
          UI_NavAction action = {UI_NavActionFlag_ExplicitDirectional, {+0, +1}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveLeftSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {-1, +0}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveRightSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+1, +0}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+0, -1}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+0, +1}};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveLeftChunk:
        {
          UI_NavAction action = {UI_NavActionFlag_ExplicitDirectional, {-1, +0}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveRightChunk:
        {
          UI_NavAction action = {UI_NavActionFlag_ExplicitDirectional, {+1, +0}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpChunk:
        {
          UI_NavAction action = {UI_NavActionFlag_ExplicitDirectional, {+0, -1}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownChunk:
        {
          UI_NavAction action = {UI_NavActionFlag_ExplicitDirectional, {+0, +1}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpPage:
        {
          UI_NavAction action = {0, {+0, -1}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownPage:
        {
          UI_NavAction action = {0, {+0, +1}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpWhole:
        {
          UI_NavAction action = {0, {+0, -1}, UI_NavDeltaUnit_EndPoint};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownWhole:
        {
          UI_NavAction action = {0, {+0, +1}, UI_NavDeltaUnit_EndPoint};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveLeftChunkSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {-1, +0}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveRightChunkSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+1, +0}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpChunkSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+0, -1}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownChunkSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+0, +1}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpPageSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+0, -1}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownPageSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark|UI_NavActionFlag_ExplicitDirectional, {+0, +1}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveUpWholeSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark, {+0, -1}, UI_NavDeltaUnit_EndPoint};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveDownWholeSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark, {+0, +1}, UI_NavDeltaUnit_EndPoint};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveHome:
        {
          UI_NavAction action = {0, {-1, +0}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveEnd:
        {
          UI_NavAction action = {0, {+1, +0}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveHomeSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark, {-1, +0}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_MoveEndSelect:
        {
          UI_NavAction action = {UI_NavActionFlag_KeepMark, {+1, +0}, UI_NavDeltaUnit_Whole};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_SelectAll:
        {
          UI_NavAction action1 = {0, {-1, +0}, UI_NavDeltaUnit_EndPoint};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action1);
          UI_NavAction action2 = {UI_NavActionFlag_KeepMark, {+1, +0}, UI_NavDeltaUnit_EndPoint};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action2);
        }break;
        case DF_CoreCmdKind_DeleteSingle:
        {
          UI_NavAction action = {UI_NavActionFlag_Delete, {+1, +0}, UI_NavDeltaUnit_Element};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_DeleteChunk:
        {
          UI_NavAction action = {UI_NavActionFlag_Delete, {+1, +0}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_BackspaceSingle:
        {
          UI_NavAction action = {UI_NavActionFlag_Delete|UI_NavActionFlag_ZeroDeltaOnSelect, {-1, +0}, UI_NavDeltaUnit_Element};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_BackspaceChunk:
        {
          UI_NavAction action = {UI_NavActionFlag_Delete|UI_NavActionFlag_ZeroDeltaOnSelect, {-1, +0}, UI_NavDeltaUnit_Chunk};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_Copy:
        {
          UI_NavAction action = {UI_NavActionFlag_Copy|UI_NavActionFlag_KeepMark};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_Cut:
        {
          UI_NavAction action = {UI_NavActionFlag_Copy|UI_NavActionFlag_Delete};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_Paste:
        {
          UI_NavAction action = {UI_NavActionFlag_Paste};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        case DF_CoreCmdKind_InsertText:
        {
          String8 insertion = params.string;
          UI_NavAction action = {0, {0}, (UI_NavDeltaUnit)0, push_str8_copy(ui_build_arena(), insertion)};
          ui_nav_action_list_push(ui_build_arena(), &nav_actions, action);
        }break;
        
        //- rjf: address finding
        case DF_CoreCmdKind_GoToAddress:
        {
          U64 vaddr = params.vaddr;
        }break;
        
        //- rjf: thread finding
        case DF_CoreCmdKind_FindThread:
        {
          DBGI_Scope *scope = dbgi_scope_open();
          DF_Entity *thread = df_entity_from_handle(params.entity);
          U64 unwind_count = params.index;
          if(thread->kind == DF_EntityKind_Thread)
          {
            // rjf: grab rip
            U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
            
            // rjf: extract thread/rip info
            DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
            DF_Entity *binary = df_binary_file_from_module(module);
            U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
            DBGI_Parse *dbgi = df_dbgi_parse_from_binary_file(scope, binary);
            DF_TextLineDasm2SrcInfo line_info = df_text_line_dasm2src_info_from_binary_voff(binary, rip_voff);
            
            // rjf: snap to resolved line
            B32 missing_rip = (rip_vaddr == 0);
            B32 binary_missing = (binary->flags & DF_EntityFlag_IsMissing);
            B32 dbg_info_pending = !binary_missing && dbgi == &dbgi_parse_nil;
            B32 has_line_info = (line_info.voff_range.max != line_info.voff_range.min);
            B32 has_module = !df_entity_is_nil(module);
            B32 has_dbg_info = has_module && !binary_missing;
            if(!dbg_info_pending && (has_line_info || has_module))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              if(has_line_info)
              {
                params.file_path = df_full_path_from_entity(scratch.arena, line_info.file);
                params.text_point = line_info.pt;
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
              }
              params.entity = df_handle_from_entity(thread);
              params.voff = rip_voff;
              params.vaddr = rip_vaddr;
              params.index = unwind_count;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualOff);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Index);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            }
            
            // rjf: retry on stopped, pending debug info
            if(!df_ctrl_targets_running() && (dbg_info_pending || missing_rip))
            {
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindThread));
            }
          }
          dbgi_scope_close(scope);
        }break;
        case DF_CoreCmdKind_FindSelectedThread:
        {
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(ws);
          DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(selected_thread);
          params.index = ctrl_ctx.unwind_count;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Index);
          df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindThread));
        }break;
        
        //- rjf: name finding
        case DF_CoreCmdKind_GoToName:
        {
          String8 name = params.string;
          if(name.size != 0)
          {
            B32 name_resolved = 0;
            
            // rjf: try to resolve name as a symbol
            U64 voff = 0;
            DF_Entity *voff_binary = &df_g_nil_entity;
            if(name_resolved == 0)
            {
              DF_EntityList binaries = df_push_active_binary_list(scratch.arena);
              for(DF_EntityNode *n = binaries.first; n != 0; n = n->next)
              {
                U64 binary_voff = df_voff_from_binary_symbol_name(n->entity, name);
                if(binary_voff != 0)
                {
                  voff = binary_voff;
                  voff_binary = n->entity;
                  name_resolved = 1;
                  break;
                }
              }
            }
            
            // rjf: try to resolve name as a file
            DF_Entity *file = &df_g_nil_entity;
            if(name_resolved == 0)
            {
              DF_Entity *src_entity = df_entity_from_handle(params.entity);
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
                  DF_Entity *root_folder = &df_g_nil_entity;
                  
                  // rjf: try to find root folder as if it's an absolute path
                  if(df_entity_is_nil(root_folder))
                  {
                    root_folder = df_entity_from_path(first_folder_name, DF_EntityFromPathFlag_OpenAsNeeded);
                  }
                  
                  // rjf: try to find root folder as if it's a path we've already loaded
                  if(df_entity_is_nil(root_folder))
                  {
                    root_folder = df_entity_from_name_and_kind(first_folder_name, DF_EntityKind_File);
                  }
                  
                  // rjf: try to find root folder as if it's inside of a path we've already loaded
                  if(df_entity_is_nil(root_folder))
                  {
                    DF_EntityList all_files = df_query_cached_entity_list_with_kind(DF_EntityKind_File);
                    for(DF_EntityNode *n = all_files.first; n != 0; n = n->next)
                    {
                      if(n->entity->flags & DF_EntityFlag_IsFolder)
                      {
                        String8 n_entity_path = df_full_path_from_entity(scratch.arena, n->entity);
                        String8 estimated_full_path = push_str8f(scratch.arena, "%S/%S", n_entity_path, first_folder_name);
                        root_folder = df_entity_from_path(estimated_full_path, DF_EntityFromPathFlag_OpenAsNeeded);
                        if(!df_entity_is_nil(root_folder))
                        {
                          break;
                        }
                      }
                    }
                  }
                  
                  // rjf: has root folder -> descend downwards
                  if(!df_entity_is_nil(root_folder))
                  {
                    String8 root_folder_path = df_full_path_from_entity(scratch.arena, root_folder);
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
                    file = df_entity_from_path(full_file_path, DF_EntityFromPathFlag_AllowOverrides|DF_EntityFromPathFlag_OpenAsNeeded|DF_EntityFromPathFlag_OpenMissing);
                  }
                }
                
                // rjf: no folders specified => just try the local folder, then try globally
                else if(src_entity->kind == DF_EntityKind_File)
                {
                  file = df_entity_from_name_and_kind(file_name, DF_EntityKind_File);
                  if(df_entity_is_nil(file))
                  {
                    String8 src_entity_full_path = df_full_path_from_entity(scratch.arena, src_entity);
                    String8 src_entity_folder = str8_chop_last_slash(src_entity_full_path);
                    String8 estimated_full_path = push_str8f(scratch.arena, "%S/%S", src_entity_folder, file_name);
                    file = df_entity_from_path(estimated_full_path, DF_EntityFromPathFlag_All);
                  }
                }
              }
              name_resolved = !df_entity_is_nil(file) && !(file->flags & DF_EntityFlag_IsMissing) && !(file->flags & DF_EntityFlag_IsFolder);
            }
            
            // rjf: process resolved info
            if(name_resolved == 0)
            {
              DF_CmdParams p = params;
              p.string = push_str8f(scratch.arena, "\"%S\" could not be found.", name);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
            }
            
            // rjf: name resolved to voff * dbg info
            if(name_resolved != 0 && voff != 0)
            {
              DF_TextLineDasm2SrcInfo dasm2src_info = df_text_line_dasm2src_info_from_binary_voff(voff_binary, voff);
              DF_CmdParams p = params;
              {
                p.file_path = df_full_path_from_entity(scratch.arena, dasm2src_info.file);
                p.text_point = dasm2src_info.pt;
                df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
                df_cmd_params_mark_slot(&p, DF_CmdParamSlot_TextPoint);
                if(!df_entity_is_nil(voff_binary))
                {
                  p.entity = df_handle_from_entity(voff_binary);
                  p.voff = dasm2src_info.voff_range.min;
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_VirtualOff);
                }
              }
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            }
            
            // rjf: name resolved to a file
            if(name_resolved != 0 && !df_entity_is_nil(file))
            {
              String8 path = df_full_path_from_entity(scratch.arena, file);
              DF_CmdParams p = params;
              p.file_path = path;
              p.text_point = txt_pt(1, 1);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_TextPoint);
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            }
          }
        }break;
        
        //- rjf: editors
        case DF_CoreCmdKind_EditEntity:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          switch(entity->kind)
          {
            default: break;
            case DF_EntityKind_Target:
            {
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EditTarget));
            }break;
          }
        }break;
        
        //- rjf: targets
        case DF_CoreCmdKind_EditTarget:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          if(!df_entity_is_nil(entity) && entity->kind == DF_EntityKind_Target)
          {
            df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Target));
          }
          else
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.string = str8_lit("Invalid target.");
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
        }break;
        
        //- rjf: catchall general entity activation paths (drag/drop, clicking)
        case DF_CoreCmdKind_EntityRefFastPath:
        {
          DF_Entity *entity = df_entity_from_handle(params.entity);
          switch(entity->kind)
          {
            default:
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SpawnEntityView));
            }break;
            case DF_EntityKind_File:
            {
              String8 path = df_full_path_from_entity(scratch.arena, entity);
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.file_path = path;
              params.text_point = txt_pt(1, 1);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            }break;
            case DF_EntityKind_Thread:
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
            }break;
          }
        }break;
        case DF_CoreCmdKind_SpawnEntityView:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_Entity *entity = df_entity_from_handle(params.entity);
          switch(entity->kind)
          {
            default:{}break;
            
            case DF_EntityKind_File:
            {
              if(entity->flags & DF_EntityFlag_IsFolder)
              {
                String8 full_path = df_full_path_from_entity(scratch.arena, entity);
                String8 full_path_w_slash = push_str8f(scratch.arena, "%S/", full_path);
                
                // rjf: set current path
                {
                  DF_CmdParams p = df_cmd_params_zero();
                  p.file_path = full_path_w_slash;
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetCurrentPath));
                }
                
                // rjf: do fast path for open
                {
                  DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                  p.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Open);
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_CmdSpec);
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
                }
              }
              else
              {
                DF_CmdParams params = df_cmd_params_from_panel(ws, panel);
                params.entity = df_handle_from_entity(entity);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
                df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PendingEntity));
              }
            }break;
            
            case DF_EntityKind_Target:
            {
              DF_CmdParams params = df_cmd_params_from_panel(ws, panel);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EditTarget));
            }break;
          }
        }break;
        case DF_CoreCmdKind_FindCodeLocation:
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
          
          // rjf: grab things to find. file * text, process * address, etc.
          DF_Entity *src_code = &df_g_nil_entity;
          TxtPt point = {0};
          DF_Entity *thread = &df_g_nil_entity;
          DF_Entity *process = &df_g_nil_entity;
          U64 vaddr = 0;
          {
            DF_Entity *param_entity = df_entity_from_handle(params.entity);
            if(params.file_path.size != 0)
            {
              src_code = df_entity_from_path(params.file_path, DF_EntityFromPathFlag_All);
            }
            if(param_entity->kind == DF_EntityKind_Thread)
            {
              thread = param_entity;
            }
            if(param_entity->kind == DF_EntityKind_Thread ||
               param_entity->kind == DF_EntityKind_Module)
            {
              process = df_entity_ancestor_from_kind(param_entity, DF_EntityKind_Process);
              if(param_entity->kind == DF_EntityKind_Module)
              {
                thread = df_entity_child_from_kind(process, DF_EntityKind_Thread);
              }
            }
            if(param_entity->kind == DF_EntityKind_Process)
            {
              process = param_entity;
              thread = df_entity_child_from_kind(process, DF_EntityKind_Thread);
            }
            point = params.text_point;
            vaddr = params.vaddr;
          }
          
          // rjf: given a src code location, and a process, if no vaddr is specified,
          // try to map the src coordinates to a vaddr via line info
          if(vaddr == 0 && !df_entity_is_nil(src_code) && !df_entity_is_nil(process))
          {
            DF_TextLineSrc2DasmInfoListArray src2dasm = df_text_line_src2dasm_info_list_array_from_src_line_range(scratch.arena, src_code, r1s64(point.line, point.line));
            for(U64 src2dasm_idx = 0; src2dasm_idx < src2dasm.count; src2dasm_idx += 1)
            {
              for(DF_TextLineSrc2DasmInfoNode *n = src2dasm.v[src2dasm_idx].first; n != 0; n = n->next)
              {
                DF_EntityList modules = df_modules_from_binary_file(scratch.arena, n->v.binary);
                DF_Entity *module = df_module_from_thread_candidates(thread, &modules);
                vaddr = df_vaddr_from_voff(module, n->v.voff_range.min);
                goto end_lookup;
              }
            }
            end_lookup:;
          }
          
          // rjf: first, try to find panel/view pair that already has the src file open
          DF_Panel *panel_w_this_src_code = &df_g_nil_panel;
          DF_View *view_w_this_src_code = &df_g_nil_view;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
            {
              DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
              DF_Entity *viewed_entity = df_entity_from_handle(view->entity);
              if((view_kind == DF_GfxViewKind_Code || view_kind == DF_GfxViewKind_PendingEntity) && viewed_entity == src_code)
              {
                panel_w_this_src_code = panel;
                view_w_this_src_code = view;
                if(view == df_view_from_handle(panel->selected_tab_view))
                {
                  break;
                }
              }
            }
          }
          
          // rjf: find a panel that already has *any* code open
          DF_Panel *panel_w_any_src_code = &df_g_nil_panel;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
            {
              DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
              if(view_kind == DF_GfxViewKind_Code)
              {
                panel_w_any_src_code = panel;
                break;
              }
            }
          }
          
          // rjf: try to find panel/view pair that has disassembly open
          DF_Panel *panel_w_disasm = &df_g_nil_panel;
          DF_View *view_w_disasm = &df_g_nil_view;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
            {
              DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
              DF_Entity *viewed_entity = df_entity_from_handle(view->entity);
              if(view_kind == DF_GfxViewKind_Disassembly)
              {
                panel_w_disasm = panel;
                view_w_disasm = view;
                if(view == df_view_from_handle(panel->selected_tab_view))
                {
                  break;
                }
              }
            }
          }
          
          // rjf: find the biggest panel
          DF_Panel *biggest_panel = &df_g_nil_panel;
          {
            Rng2F32 root_rect = os_client_rect_from_window(ws->os);
            F32 best_panel_area = 0;
            for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
            {
              if(!df_panel_is_nil(panel->first))
              {
                continue;
              }
              Rng2F32 panel_rect = df_rect_from_panel(root_rect, ws->root_panel, panel);
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
          DF_Panel *biggest_empty_panel = &df_g_nil_panel;
          {
            Rng2F32 root_rect = os_client_rect_from_window(ws->os);
            F32 best_panel_area = 0;
            for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
            {
              if(!df_panel_is_nil(panel->first))
              {
                continue;
              }
              Rng2F32 panel_rect = df_rect_from_panel(root_rect, ws->root_panel, panel);
              Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
              F32 area = panel_rect_dim.x * panel_rect_dim.y;
              if(df_view_is_nil(panel->first_tab_view) && (best_panel_area == 0 || area > best_panel_area))
              {
                best_panel_area = area;
                biggest_empty_panel = panel;
              }
            }
          }
          
          // rjf: given the above, find source code location.
          B32 disasm_view_prioritized = 0;
          DF_Panel *panel_used_for_src_code = &df_g_nil_panel;
          if(!df_entity_is_nil(src_code))
          {
            // rjf: determine which panel we will use to find the code loc
            DF_Panel *dst_panel = &df_g_nil_panel;
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
              df_view_equip_spec(view, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Code), src_code, str8_lit(""), &df_g_nil_cfg_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            DF_CoreCmdKind cursor_snap_kind = DF_CoreCmdKind_CenterCursor;
            if(!df_panel_is_nil(dst_panel) && dst_view == view_w_this_src_code && df_view_from_handle(dst_panel->selected_tab_view) == dst_view)
            {
              cursor_snap_kind = DF_CoreCmdKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              disasm_view_prioritized = (df_view_from_handle(dst_panel->selected_tab_view) == view_w_disasm);
              dst_panel->selected_tab_view = df_handle_from_view(dst_view);
              DF_CmdParams params = df_cmd_params_from_view(ws, dst_panel, dst_view);
              params.text_point = point;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_GoToLine));
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(cursor_snap_kind));
              panel_used_for_src_code = dst_panel;
            }
          }
          
          // rjf: given the above, find disassembly location.
          if(!df_entity_is_nil(process) && vaddr != 0)
          {
            // rjf: determine which panel we will use to find the disasm loc -
            // we *cannot* use the same panel we used for source code, if any.
            DF_Panel *dst_panel = &df_g_nil_panel;
            {
              if(df_panel_is_nil(dst_panel)) { dst_panel = panel_w_disasm; }
              if(df_panel_is_nil(panel_used_for_src_code) && df_panel_is_nil(dst_panel)) { dst_panel = biggest_empty_panel; }
              if(df_panel_is_nil(panel_used_for_src_code) && df_panel_is_nil(dst_panel)) { dst_panel = biggest_panel; }
              if(dst_panel == panel_used_for_src_code &&
                 !disasm_view_prioritized)
              {
                dst_panel = &df_g_nil_panel;
              }
            }
            
            // rjf: construct new view if needed
            DF_View *dst_view = view_w_disasm;
            if(!df_panel_is_nil(dst_panel) && df_view_is_nil(view_w_disasm))
            {
              DF_View *view = df_view_alloc();
              df_view_equip_spec(view, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Disassembly), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            DF_CoreCmdKind cursor_snap_kind = DF_CoreCmdKind_CenterCursor;
            if(dst_view == view_w_disasm && df_view_from_handle(dst_panel->selected_tab_view) == dst_view)
            {
              cursor_snap_kind = DF_CoreCmdKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              dst_panel->selected_tab_view = df_handle_from_view(dst_view);
              DF_CmdParams params = df_cmd_params_from_view(ws, dst_panel, dst_view);
              params.entity = df_handle_from_entity(process);
              params.vaddr = vaddr;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_GoToAddress));
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(cursor_snap_kind));
            }
          }
        }break;
        
        //- rjf: filtering
        case DF_CoreCmdKind_Filter:
        {
          DF_View *view = df_view_from_handle(params.view);
          DF_Panel *panel = df_panel_from_handle(params.panel);
          B32 view_is_tab = 0;
          for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->next)
          {
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
        case DF_CoreCmdKind_ClearFilter:
        {
          DF_View *view = df_view_from_handle(params.view);
          if(!df_view_is_nil(view))
          {
            view->query_string_size = 0;
            view->is_filtering = 0;
            view->query_cursor = view->query_mark = txt_pt(1, 1);
          }
        }break;
        case DF_CoreCmdKind_ApplyFilter:
        {
          DF_View *view = df_view_from_handle(params.view);
          if(!df_view_is_nil(view))
          {
            view->is_filtering = 0;
          }
        }break;
        
        //- rjf: query completion
        case DF_CoreCmdKind_CompleteQuery:
        {
          // rjf: compound command parameters
          if(ws->query_cmd_spec->info.query.slot != DF_CmdParamSlot_Null &&
             df_cmd_params_has_slot(&params, ws->query_cmd_spec->info.query.slot))
          {
            DF_CmdParams params_copy = df_cmd_params_copy(ws->query_cmd_arena, &params);
            Rng1U64 offset_range_in_params = df_g_cmd_param_slot_range_table[ws->query_cmd_spec->info.query.slot];
            MemoryCopy((U8 *)(&ws->query_cmd_params) + offset_range_in_params.min,
                       (U8 *)(&params_copy) + offset_range_in_params.min,
                       dim_1u64(offset_range_in_params));
            df_cmd_params_mark_slot(&ws->query_cmd_params, ws->query_cmd_spec->info.query.slot);
          }
          
          // rjf: determine if command is ready to run
          B32 command_ready = 1;
          if(ws->query_cmd_spec->info.query.slot != DF_CmdParamSlot_Null &&
             !df_cmd_params_has_slot(&ws->query_cmd_params, ws->query_cmd_spec->info.query.slot))
          {
            command_ready = 0;
          }
          
          // rjf: end this query
          if(!(ws->query_cmd_spec->info.query.flags & DF_CmdQueryFlag_KeepOldInput))
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CancelQuery));
          }
          
          // rjf: push command if possible
          if(command_ready)
          {
            df_push_cmd__root(&ws->query_cmd_params, ws->query_cmd_spec);
          }
        }break;
        case DF_CoreCmdKind_CancelQuery:
        {
          arena_clear(ws->query_cmd_arena);
          ws->query_cmd_spec = &df_g_nil_cmd_spec;
          MemoryZeroStruct(&ws->query_cmd_params);
          for(DF_View *v = ws->query_view_stack_top, *next = 0; !df_view_is_nil(v); v = next)
          {
            next = v->next;
            df_view_release(v);
          }
          ws->query_view_stack_top = &df_g_nil_view;
        }break;
        
        //- rjf: developer commands
        case DF_CoreCmdKind_ToggleDevMenu:
        {
          ws->dev_menu_is_open ^= 1;
        }break;
      }
    }
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: process view-level commands on leaf panels
  //
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
      DF_View *view = df_view_from_handle(panel->selected_tab_view);
      if(!df_view_is_nil(view))
      {
        DF_ViewCmdFunctionType *do_view_cmds_function = view->spec->info.cmd_hook;
        do_view_cmds_function(ws, panel, view, cmds);
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
      F_Tag main_font = df_font_from_slot(DF_FontSlot_Main);
      F32 main_font_size = df_font_size_from_slot(ws, DF_FontSlot_Main);
      F_Tag icon_font = df_font_from_slot(DF_FontSlot_Icons);
      
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
      
      // rjf: begin & push initial stack values
      ui_begin_build(events, ws->os, &nav_actions, &icon_info, df_dt(), df_dt());
      ui_push_font(main_font);
      ui_push_font_size(main_font_size);
      ui_push_pref_width(ui_em(20.f, 1));
      ui_push_pref_height(ui_em(2.5f, 1.f));
      ui_push_background_color(df_rgba_from_theme_color(DF_ThemeColor_PlainBackground));
      ui_push_text_color(df_rgba_from_theme_color(DF_ThemeColor_PlainText));
      ui_push_border_color(df_rgba_from_theme_color(DF_ThemeColor_PlainBorder));
      ui_push_text_select_color(df_rgba_from_theme_color(DF_ThemeColor_TextSelection));
      ui_push_text_cursor_color(df_rgba_from_theme_color(DF_ThemeColor_Cursor));
      ui_push_blur_size(10.f);
    }
    
    ////////////////////////////
    //- rjf: calculate top-level rectangles
    //
    Rng2F32 window_rect = os_client_rect_from_window(ws->os);
    Vec2F32 window_rect_dim = dim_2f32(window_rect);
    Rng2F32 top_bar_rect = r2f32p(window_rect.x0, window_rect.y0, window_rect.x0+window_rect_dim.x, window_rect.y0+ui_top_pref_height().value);
    Rng2F32 bottom_bar_rect = r2f32p(window_rect.x0, window_rect_dim.y - ui_top_pref_height().value, window_rect.x0+window_rect_dim.x, window_rect.y0+window_rect_dim.y);
    Rng2F32 content_rect = r2f32p(window_rect.x0, top_bar_rect.y1, window_rect.x0+window_rect_dim.x, bottom_bar_rect.y0);
    
    ////////////////////////////
    //- rjf: truncated string hover
    //
    if(ui_string_hover_active()) UI_Tooltip
    {
      Temp scratch = scratch_begin(&arena, 1);
      String8 string = ui_string_hover_string(scratch.arena);
      D_FancyRunList runs = ui_string_hover_runs(scratch.arena);
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
      Temp scratch = scratch_begin(&arena, 1);
      DF_DragDropPayload *payload = &df_g_drag_drop_payload;
      DF_Panel *panel = df_panel_from_handle(payload->panel);
      DF_Entity *entity = df_entity_from_handle(payload->entity);
      DF_View *view = df_view_from_handle(payload->view);
      UI_Tooltip
      {
        UI_Box *tooltip = ui_top_parent();
        if(!df_view_is_nil(view))
        {
          ui_set_next_pref_width(ui_children_sum(1));
          UI_Row UI_HeightFill
          {
            DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
            String8 display_name = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
            DF_IconKind icon_kind = df_icon_kind_from_view(view);
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
              UI_Font(df_font_from_slot(DF_FontSlot_Icons))
              UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
              ui_label(df_g_icon_kind_text_table[icon_kind]);
            ui_label(display_name);
            tooltip->background_color = df_rgba_from_theme_color(DF_ThemeColor_TabActive);
          }
        }
        if(!df_entity_is_nil(entity))
        {
          ui_set_next_pref_width(ui_children_sum(1));
          UI_Row UI_HeightFill
          {
            String8 display_name = df_display_string_from_entity(scratch.arena, entity);
            DF_IconKind icon_kind = df_g_entity_kind_icon_kind_table[entity->kind];
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
              UI_Font(df_font_from_slot(DF_FontSlot_Icons))
              UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
              ui_label(df_g_icon_kind_text_table[icon_kind]);
            ui_label(display_name);
            tooltip->background_color = df_rgba_from_theme_color(DF_ThemeColor_EntityBackground);
          }
        }
      }
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: entity drop completion ctx menu
    //
    {
      UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_AltBackground))
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_AltText))
        UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_AltBorder))
        UI_CtxMenu(ws->drop_completion_ctx_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
      {
        DF_Entity *entity = df_entity_from_handle(ws->drop_completion_entity);
        DF_Panel *panel = df_panel_from_handle(ws->drop_completion_panel);
        if(df_entity_is_nil(entity))
        {
          ui_ctx_menu_close();
        }
        switch(entity->kind)
        {
          default:{}break;
          
          case DF_EntityKind_Module:
          {
            DF_Entity *bin_file = df_binary_file_from_module(entity);
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Module, 0, "Inspect Binary File Memory")))
            {
              DF_CmdParams params = df_cmd_params_from_panel(ws, panel);
              params.entity = df_handle_from_entity(bin_file);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PendingEntity));
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Module, 0, "View Binary File Disassembly")))
            {
              DF_CmdParams params = df_cmd_params_from_panel(ws, panel);
              params.entity = df_handle_from_entity(bin_file);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_PendingEntity));
              ui_ctx_menu_close();
            }
          }break;
          case DF_EntityKind_Process:
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Open Process Log")))
            {
              DF_Entity *log = df_log_from_entity(entity);
              DF_CmdParams params = df_cmd_params_from_panel(ws, panel);
              params.entity = df_handle_from_entity(log);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Code));
              ui_ctx_menu_close();
            }
          }break;
          case DF_EntityKind_Thread:
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Open Thread Log")))
            {
              DF_Entity *log = df_log_from_entity(entity);
              DF_CmdParams params = df_cmd_params_from_panel(ws, panel);
              params.entity = df_handle_from_entity(log);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Code));
              ui_ctx_menu_close();
            }
          }break;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: developer menu
    //
    if(ws->dev_menu_is_open)
      UI_Font(df_font_from_slot(DF_FontSlot_Code))
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
      
      //- rjf: stats & info
      {
        //- rjf: draw per-window stats
        for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
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
          ui_labelf("Target Hz: %.2f", 1.f/df_dt());
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
        
        //- rjf: draw entity file tree
#if 0
        DF_EntityRec rec = {0};
        S32 indent = 0;
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("Entity File Tree:");
        for(DF_Entity *e = df_entity_root(); !df_entity_is_nil(e); e = rec.next)
        {
          switch(e->kind)
          {
            default:{}break;
            case DF_EntityKind_File:
            case DF_EntityKind_OverrideFileLink:
            {
              ui_set_next_pref_width(ui_children_sum(1));
              ui_set_next_pref_height(ui_children_sum(1));
              UI_Row
              {
                ui_spacer(ui_em(2.f*indent, 1.f));
                if(e->kind == DF_EntityKind_File)
                {
                  ui_label(e->name);
                }
                if(e->kind == DF_EntityKind_OverrideFileLink)
                {
                  DF_Entity *dst = df_entity_from_handle(e->entity_handle);
                  ui_labelf("[link] %S -> %S", e->name, dst->name);
                }
              }
            }break;
          }
          rec = df_entity_rec_df_pre(e, df_entity_root());
          indent += rec.push_count;
          indent -= rec.pop_count;
        }
#endif
      }
    }
    
    ////////////////////////////
    //- rjf: universal ctx menus
    //
    UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_AltBackground))
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_AltText))
      UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_AltBorder))
    {
      Temp scratch = scratch_begin(&arena, 1);
      
      //- rjf: auto-close entity ctx menu
      if(ui_ctx_menu_is_open(ws->entity_ctx_menu_key))
      {
        DF_Entity *entity = df_entity_from_handle(ws->entity_ctx_menu_entity);
        if(df_entity_is_nil(entity))
        {
          ui_ctx_menu_close();
        }
      }
      
      //- rjf: entity menu
      UI_CtxMenu(ws->entity_ctx_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
      {
        DF_Entity *entity = df_entity_from_handle(ws->entity_ctx_menu_entity);
        DF_IconKind entity_icon = df_g_entity_kind_icon_kind_table[entity->kind];
        DF_EntityKindFlags kind_flags = df_g_entity_kind_flags_table[entity->kind];
        DF_EntityOpFlags op_flags = df_g_entity_kind_op_flags_table[entity->kind];
        String8 display_name = df_display_string_from_entity(scratch.arena, entity);
        
        // rjf: title
        UI_Row
        {
          UI_Font(df_font_from_slot(DF_FontSlot_Icons))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_PrefWidth(ui_em(2.f*1.5f, 1.f))
            UI_PrefHeight(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            ui_label(df_g_icon_kind_text_table[entity_icon]);
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            ui_label(df_g_entity_kind_display_string_table[entity->kind]);
          {
            Vec4F32 entity_color = ui_top_text_color();
            if(entity->flags & DF_EntityFlag_HasColor)
            {
              entity_color = df_rgba_from_entity(entity);
            }
            UI_TextColor(entity_color)
              UI_PrefWidth(ui_text_dim(10, 1))
              UI_Font((kind_flags & DF_EntityKindFlag_NameIsCode) ? df_font_from_slot(DF_FontSlot_Code) : ui_top_font())
              ui_label(display_name);
          }
        }
        
        // rjf: name editor
        if(op_flags & DF_EntityOpFlag_Rename)
        {
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, entity->name, "%S###entity_name_edit_%p", df_g_entity_kind_name_label_table[entity->kind], entity);
          if(ui_committed(sig))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            params.string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_NameEntity));
          }
        }
        
        // rjf: condition editor
        if(op_flags & DF_EntityOpFlag_Condition) UI_Font(df_font_from_slot(DF_FontSlot_Code))
        {
          DF_Entity *condition = df_entity_child_from_kind(entity, DF_EntityKind_Condition);
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border|DF_LineEditFlag_CodeContents, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, condition->name, "Condition###entity_cond_edit_%p", entity);
          if(ui_committed(sig))
          {
            String8 new_string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            if(new_string.size != 0)
            {
              if(df_entity_is_nil(condition))
              {
                df_state_delta_history_push_batch(df_state_delta_history(), 0);
                condition = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_Condition);
              }
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(condition);
              params.string = new_string;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_NameEntity));
            }
            else if(!df_entity_is_nil(condition))
            {
              df_entity_mark_for_deletion(condition);
            }
          }
        }
        
        // rjf: exe editor
        if(entity->kind == DF_EntityKind_Target)
        {
          DF_Entity *exe = df_entity_child_from_kind(entity, DF_EntityKind_Executable);
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, exe->name, "Executable###entity_exe_edit_%p", entity);
          if(ui_committed(sig))
          {
            String8 new_string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            if(new_string.size != 0)
            {
              if(df_entity_is_nil(exe))
              {
                df_state_delta_history_push_batch(df_state_delta_history(), 0);
                exe = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_Executable);
              }
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(exe);
              params.string = new_string;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_NameEntity));
            }
            else if(!df_entity_is_nil(exe))
            {
              df_entity_mark_for_deletion(exe);
            }
          }
        }
        
        // rjf: arguments editors
        if(entity->kind == DF_EntityKind_Target)
        {
          DF_Entity *args = df_entity_child_from_kind(entity, DF_EntityKind_Arguments);
          UI_Signal sig = df_line_editf(DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, args->name, "Arguments###entity_args_edit_%p", entity);
          if(ui_committed(sig))
          {
            String8 new_string = str8(ws->entity_ctx_menu_input_buffer, ws->entity_ctx_menu_input_size);
            if(new_string.size != 0)
            {
              if(df_entity_is_nil(args))
              {
                df_state_delta_history_push_batch(df_state_delta_history(), 0);
                args = df_entity_alloc(df_state_delta_history(), entity, DF_EntityKind_Arguments);
              }
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(args);
              params.string = new_string;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_NameEntity));
            }
            else if(!df_entity_is_nil(args))
            {
              df_entity_mark_for_deletion(args);
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
        if(entity->cfg_src == DF_CfgSrc_CommandLine && ui_clicked(df_icon_buttonf(DF_IconKind_Save, 0, "Save To Profile")))
        {
          df_entity_equip_cfg_src(entity, DF_CfgSrc_Profile);
        }
        
        // rjf: duplicate
        if(op_flags & DF_EntityOpFlag_Duplicate && ui_clicked(df_icon_buttonf(DF_IconKind_XSplit, 0, "Duplicate")))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_DuplicateEntity));
          ui_ctx_menu_close();
        }
        
        // rjf: edit
        if(op_flags & DF_EntityOpFlag_Edit && ui_clicked(df_icon_buttonf(DF_IconKind_Pencil, 0, "Edit")))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EditEntity));
          ui_ctx_menu_close();
        }
        
        // rjf: deletion
        if(op_flags & DF_EntityOpFlag_Delete && ui_clicked(df_icon_buttonf(DF_IconKind_Trash, 0, "Delete")))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RemoveEntity));
          ui_ctx_menu_close();
        }
        
        // rjf: enabling
        if(op_flags & DF_EntityOpFlag_Enable)
        {
          B32 is_enabled = entity->b32;
          if(!is_enabled && ui_clicked(df_icon_buttonf(DF_IconKind_CheckHollow, 0, "Enable###enabler")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EnableEntity));
          }
          if(is_enabled && ui_clicked(df_icon_buttonf(DF_IconKind_CheckFilled, 0, "Disable###enabler")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_DisableEntity));
          }
        }
        
        // rjf: freezing
        if(op_flags & DF_EntityOpFlag_Freeze)
        {
          B32 is_frozen = df_entity_is_frozen(entity);
          Vec4F32 color = df_rgba_from_theme_color(is_frozen ? DF_ThemeColor_FailureBackground : DF_ThemeColor_SuccessBackground);
          color.x *= 0.7f;
          color.y *= 0.7f;
          color.z *= 0.7f;
          ui_set_next_background_color(color);
          if(is_frozen && ui_clicked(df_icon_buttonf(DF_IconKind_Locked, 0, "Thaw###freeze_thaw")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ThawEntity));
          }
          if(!is_frozen && ui_clicked(df_icon_buttonf(DF_IconKind_Unlocked, 0, "Freeze###freeze_thaw")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FreezeEntity));
          }
        }
        
        // rjf: go-to-text-location
        if(entity->flags & DF_EntityFlag_HasTextPoint)
        {
          DF_Entity *file_ancestor = df_entity_ancestor_from_kind(entity, DF_EntityKind_File);
          if(!df_entity_is_nil(file_ancestor) && ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Go To Location")))
          {
            Temp scratch = scratch_begin(&arena, 1);
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.file_path = df_full_path_from_entity(scratch.arena, file_ancestor);
            params.text_point = entity->text_point;
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            ui_ctx_menu_close();
            scratch_end(scratch);
          }
        }
        
        // rjf: go-to-vaddr-location
        if(entity->flags & DF_EntityFlag_HasVAddr)
        {
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx();
          DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
          if(entity->vaddr != 0 && !df_entity_is_nil(thread) && ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Go To Location")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(df_entity_ancestor_from_kind(thread, DF_EntityKind_Process));
            params.vaddr = entity->vaddr;
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            ui_ctx_menu_close();
          }
        }
        
        // rjf: entity-kind-specific options
        switch(entity->kind)
        {
          default:
          {
          }break;
          
          case DF_EntityKind_File:
          {
            if(entity->flags & DF_EntityFlag_IsFolder &&
               ui_clicked(df_icon_buttonf(DF_IconKind_FolderOpenOutline, 0, "Open File In Folder")))
            {
              String8 path = df_full_path_from_entity(scratch.arena, entity);
              String8 path_w_slash = push_str8f(scratch.arena, "%S/", path);
              {
                DF_CmdParams p = df_cmd_params_zero();
                p.file_path = path_w_slash;
                df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetCurrentPath));
              }
              {
                DF_CmdParams p = df_cmd_params_from_window(ws);
                p.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Open);
                df_cmd_params_mark_slot(&p, DF_CmdParamSlot_CmdSpec);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
              }
              ui_ctx_menu_close();
            }
            if(!(entity->flags & DF_EntityFlag_IsFolder) &&
               !(entity->flags & DF_EntityFlag_IsMissing) &&
               ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Go To File")))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.file_path = df_full_path_from_entity(scratch.arena, entity);
              params.text_point = txt_pt(1, 1);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
              ui_ctx_menu_close();
            }
          }break;
          
          case DF_EntityKind_Process:
          case DF_EntityKind_Thread:
          {
            if(entity->kind == DF_EntityKind_Thread)
            {
              DF_CtrlCtx ctrl_ctx = df_ctrl_ctx();
              B32 is_selected = df_handle_match(ctrl_ctx.thread, df_handle_from_entity(entity));
              if(is_selected)
              {
                df_icon_buttonf(DF_IconKind_Thread, 0, "[Selected]###select_entity");
              }
              else if(ui_clicked(df_icon_buttonf(DF_IconKind_Thread, 0, "Select###select_entity")))
              {
                DF_CmdParams params = df_cmd_params_from_window(ws);
                params.entity = df_handle_from_entity(entity);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
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
            
            if(entity->kind == DF_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Instruction Pointer Address")))
              {
                U64 rip = df_query_cached_rip_from_thread(entity);
                String8 string = push_str8f(scratch.arena, "0x%I64x", rip);
                os_set_clipboard_text(string);
                ui_ctx_menu_close();
              }
            }
            
            if(entity->kind == DF_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Call Stack")))
              {
                DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
                CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
                String8List lines = {0};
                for(CTRL_UnwindFrame *frame = unwind.first; frame != 0; frame = frame->next)
                {
                  U64 rip_vaddr = frame->rip;
                  DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
                  DF_Entity *binary = df_binary_file_from_module(module);
                  U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
                  String8 symbol = df_symbol_name_from_binary_voff(scratch.arena, binary, rip_voff);
                  if(symbol.size != 0)
                  {
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: %S", rip_vaddr, symbol);
                  }
                  else
                  {
                    String8 module_filename = str8_skip_last_slash(module->name);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [??? in %S]", rip_vaddr, module_filename);
                  }
                }
                StringJoin join = {0};
                join.sep = join.post = str8_lit("\n");
                String8 text = str8_list_join(scratch.arena, &lines, &join);
                os_set_clipboard_text(text);
                ui_ctx_menu_close();
              }
            }
            
            if(entity->kind == DF_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_FileOutline, 0, "Find")))
              {
                DF_CmdParams params = df_cmd_params_from_window(ws);
                params.entity = df_handle_from_entity(entity);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindThread));
                ui_ctx_menu_close();
              }
            }
          }break;
          
          case DF_EntityKind_Module:
          {
            UI_Signal copy_full_path_sig = df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Full Path");
            if(ui_clicked(copy_full_path_sig))
            {
              String8 string = entity->name;
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            if(ui_hovering(copy_full_path_sig)) UI_Tooltip
            {
              String8 string = entity->name;
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
          
          case DF_EntityKind_Target:
          {
            if(ui_clicked(df_icon_buttonf(DF_IconKind_Play, 0, "Launch And Run")))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_icon_buttonf(DF_IconKind_PlayStepForward, 0, "Launch And Initialize")))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndInit));
              ui_ctx_menu_close();
            }
          }break;
        }
        
        // rjf: color editor
        {
          B32 entity_has_color = entity->flags & DF_EntityFlag_HasColor;
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
                    ui_set_next_background_color(presets[preset_idx]);
                    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
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
            
            UI_Row UI_Padding(ui_pct(1, 0)) UI_PrefWidth(ui_em(12.f, 1.f)) UI_CornerRadius(8.f)
            {
              if(ui_clicked(df_icon_buttonf(DF_IconKind_Trash, 0, "Remove Color###color_toggle")))
              {
                entity->flags &= ~DF_EntityFlag_HasColor;
              }
            }
            
            ui_spacer(ui_em(1.5f, 1.f));
          }
          if(!entity_has_color && ui_clicked(df_icon_buttonf(DF_IconKind_Palette, 0, "Apply Color###color_toggle")))
          {
            df_entity_equip_color_rgba(entity, v4f32(1, 1, 1, 1));
          }
        }
      }
      
      //- rjf: auto-close tab ctx menu
      if(ui_ctx_menu_is_open(ws->tab_ctx_menu_key))
      {
        DF_View *tab = df_view_from_handle(ws->tab_ctx_menu_view);
        if(df_view_is_nil(tab))
        {
          ui_ctx_menu_close();
        }
      }
      
      //- rjf: tab menu
      UI_CtxMenu(ws->tab_ctx_menu_key) UI_PrefWidth(ui_em(25.f, 1.f)) UI_CornerRadius(0)
      {
        DF_View *view = df_view_from_handle(ws->tab_ctx_menu_view);
        DF_IconKind view_icon = df_icon_kind_from_view(view);
        DF_Entity *entity = df_entity_from_handle(view->entity);
        DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
        String8 display_name = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
        
        // rjf: title
        UI_Row
        {
          UI_Font(df_font_from_slot(DF_FontSlot_Icons))
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_PrefWidth(ui_em(3.f, 1.f))
            UI_PrefHeight(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
            ui_label(df_g_icon_kind_text_table[view_icon]);
          UI_PrefWidth(ui_text_dim(10, 1)) ui_label(display_name);
        }
        
        // rjf: copy name
        if(ui_clicked(df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Name")))
        {
          os_set_clipboard_text(display_name);
          ui_ctx_menu_close();
        }
        
        // rjf: copy full path
        if(entity->kind == DF_EntityKind_File)
        {
          UI_Signal copy_full_path_sig = df_icon_buttonf(DF_IconKind_Clipboard, 0, "Copy Full Path");
          String8 full_path = df_full_path_from_entity(scratch.arena, entity);
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
        
        // rjf: filter controls
        if(view->spec->info.flags & DF_ViewSpecFlag_CanFilter)
        {
          if(ui_clicked(df_cmd_spec_button(df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter))))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            {
              params.view = df_handle_from_view(view);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_View);
            }
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter));
            ui_ctx_menu_close();
          }
          if(ui_clicked(df_cmd_spec_button(df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClearFilter))))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            {
              params.view = df_handle_from_view(view);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_View);
            }
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClearFilter));
            ui_ctx_menu_close();
          }
        }
        
        // rjf: close tab
        if(ui_clicked(df_icon_buttonf(DF_IconKind_X, 0, "Close Tab")))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          {
            params.view = df_handle_from_view(view);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_View);
          }
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseTab));
          ui_ctx_menu_close();
        }
        
      }
      
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: confirmation popup
    //
    {
      if(df_gfx_state->confirm_t > 0.005f) UI_TextAlignment(UI_TextAlign_Center)
      {
        Vec2F32 window_dim = dim_2f32(window_rect);
        UI_Box *bg_box = &ui_g_nil_box;
        UI_Rect(window_rect) UI_ChildLayoutAxis(Axis2_X)
        {
          Vec4F32 bg_color = ui_top_background_color();
          bg_color.w *= df_gfx_state->confirm_t;
          ui_set_next_blur_size(10*df_gfx_state->confirm_t);
          ui_set_next_background_color(bg_color);
          bg_box = ui_build_box_from_stringf(UI_BoxFlag_FixedSize|UI_BoxFlag_Floating|UI_BoxFlag_Clickable|UI_BoxFlag_Scroll|UI_BoxFlag_DefaultFocusNav|UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_DrawBackground, "###confirm_popup_%p", ws);
        }
        if(df_gfx_state->confirm_active) UI_Parent(bg_box)
        {
          ui_ctx_menu_close();
          UI_WidthFill UI_PrefHeight(ui_children_sum(1.f)) UI_Column UI_Padding(ui_pct(1, 0))
          {
            UI_FontSize(ui_top_font_size()*2.f) UI_PrefHeight(ui_em(3.f, 1.f)) ui_label(df_gfx_state->confirm_title);
            UI_PrefHeight(ui_em(3.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_label(df_gfx_state->confirm_msg);
            ui_spacer(ui_em(1.5f, 1.f));
            UI_Row UI_Padding(ui_pct(1.f, 0.f)) UI_WidthFill UI_PrefHeight(ui_em(5.f, 1.f))
            {
              UI_CornerRadius00(ui_top_font_size()*0.25f)
                UI_CornerRadius01(ui_top_font_size()*0.25f)
                UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBackground))
                UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_ActionText))
                UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_ActionBorder))
                if(ui_clicked(ui_buttonf("OK")) || os_key_press(ui_events(), ui_window(), 0, OS_Key_Return))
              {
                DF_CmdParams p = df_cmd_params_zero();
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ConfirmAccept));
              }
              UI_CornerRadius10(ui_top_font_size()*0.25f)
                UI_CornerRadius11(ui_top_font_size()*0.25f)
                if(ui_clicked(ui_buttonf("Cancel")) || os_key_press(ui_events(), ui_window(), 0, OS_Key_Esc))
              {
                DF_CmdParams p = df_cmd_params_zero();
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ConfirmCancel));
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
      if(!ws->autocomp_force_closed && !ui_key_match(ws->autocomp_root_key, ui_key_zero()) && ws->autocomp_last_frame_idx+1 >= df_frame_index())
    {
      String8 query = str8(ws->autocomp_lister_query_buffer, ws->autocomp_lister_query_size);
      UI_Box *autocomp_root_box = ui_box_from_key(ws->autocomp_root_key);
      if(!ui_box_is_nil(autocomp_root_box))
      {
        Temp scratch = scratch_begin(&arena, 1);
        
        //- rjf: unpack lister params
        DF_CtrlCtx ctrl_ctx = ws->autocomp_ctrl_ctx;
        DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
        U64 thread_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
        DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
        DF_Entity *module = df_module_from_process_vaddr(process, thread_rip_vaddr);
        U64 thread_rip_voff = df_voff_from_vaddr(module, thread_rip_vaddr);
        DF_Entity *binary = df_binary_file_from_module(module);
        
        //- rjf: gather lister items
        DF_AutoCompListerItemChunkList item_list = {0};
        {
          if(ws->autocomp_lister_flags & DF_AutoCompListerFlag_Locals)
          {
            EVAL_String2NumMap *locals_map = df_query_cached_locals_map_from_binary_voff(binary, thread_rip_voff);
            for(EVAL_String2NumMapNode *n = locals_map->first; n != 0; n = n->order_next)
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
          }
          if(ws->autocomp_lister_flags & DF_AutoCompListerFlag_Registers)
          {
            Architecture arch = df_architecture_from_entity(thread);
            U64 reg_names_count = regs_reg_code_count_from_architecture(arch);
            U64 alias_names_count = regs_alias_code_count_from_architecture(arch);
            String8 *reg_names = regs_reg_code_string_table_from_architecture(arch);
            String8 *alias_names = regs_alias_code_string_table_from_architecture(arch);
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
          if(ws->autocomp_lister_flags & DF_AutoCompListerFlag_ViewRules)
          {
            for(U64 slot_idx = 0; slot_idx < df_state->view_rule_spec_table_size; slot_idx += 1)
            {
              for(DF_CoreViewRuleSpec *spec = df_state->view_rule_spec_table[slot_idx]; spec != 0 && spec != &df_g_nil_core_view_rule_spec; spec = spec->hash_next)
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
        }
        
        //- rjf: lister item list -> sorted array
        DF_AutoCompListerItemArray item_array = df_autocomp_lister_item_array_from_chunk_list(scratch.arena, &item_list);
        df_autocomp_lister_item_array_sort__in_place(&item_array);
        
        //- rjf: animate
        {
          // rjf: animate target # of rows
          {
            F32 rate = 1 - pow_f32(2, (-60.f * df_dt()));
            F32 target = Min((F32)item_array.count, 8.f);
            if(abs_f32(target - ws->autocomp_num_visible_rows_t) > 0.01f)
            {
              df_gfx_request_frame();
            }
            ws->autocomp_num_visible_rows_t += (target - ws->autocomp_num_visible_rows_t) * rate;
            if(abs_f32(target - ws->autocomp_num_visible_rows_t) <= 0.02f)
            {
              ws->autocomp_num_visible_rows_t = target;
            }
          }
          
          // rjf: animate open
          {
            F32 rate = 1 - pow_f32(2, (-60.f * df_dt()));
            F32 diff = 1.f-ws->autocomp_open_t;
            ws->autocomp_open_t += diff*rate;
            if(abs_f32(diff) < 0.05f)
            {
              ws->autocomp_open_t = 1.f;
            }
            else
            {
              df_gfx_request_frame();
            }
          }
        }
        
        //- rjf: build
        if(item_array.count != 0)
        {
          F32 row_height_px = floor_f32(ui_top_font_size()*2.5f);
          ui_set_next_fixed_x(autocomp_root_box->rect.x0);
          ui_set_next_fixed_y(autocomp_root_box->rect.y1);
          ui_set_next_pref_width(ui_em(25.f, 1.f));
          ui_set_next_pref_height(ui_px(row_height_px*ws->autocomp_num_visible_rows_t, 1.f));
          ui_set_next_child_layout_axis(Axis2_Y);
          ui_set_next_corner_radius_01(ui_top_font_size()*0.25f);
          ui_set_next_corner_radius_11(ui_top_font_size()*0.25f);
          ui_set_next_corner_radius_10(ui_top_font_size()*0.25f);
          UI_Focus(UI_FocusKind_On)
            UI_Squish(0.25f-0.25f*ws->autocomp_open_t)
            UI_Transparency(1.f-ws->autocomp_open_t)
          {
            autocomp_box = ui_build_box_from_stringf(UI_BoxFlag_DefaultFocusNavY|UI_BoxFlag_Clip|UI_BoxFlag_RoundChildrenByParent|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBackground, "autocomp_box");
          }
          UI_Parent(autocomp_box) UI_WidthFill UI_PrefHeight(ui_px(row_height_px, 1.f)) UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_HoverCursor(OS_Cursor_HandPoint)
            UI_Focus(UI_FocusKind_Null)
          {
            for(U64 idx = 0; idx < item_array.count; idx += 1)
            {
              DF_AutoCompListerItem *item = &item_array.v[idx];
              UI_Box *item_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects|UI_BoxFlag_Clickable, "autocomp_%I64x", idx);
              UI_Parent(item_box)
              {
                UI_WidthFill ui_label(item->string);
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                  UI_PrefWidth(ui_text_dim(10, 1))
                  UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                  ui_label(item->kind_string);
              }
              UI_Signal item_sig = ui_signal_from_box(item_box);
              if(ui_clicked(item_sig))
              {
                UI_NavAction autocomp_action = {UI_NavActionFlag_ReplaceAndCommit, {0}, (UI_NavDeltaUnit)0, push_str8_copy(ui_build_arena(), item->string)};
                ui_nav_action_list_push(ui_build_arena(), ui_nav_actions(), autocomp_action);
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
      ui_set_next_flags(UI_BoxFlag_DefaultFocusNav);
      UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_AltBackground))
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_AltText))
        UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_AltBorder))
        UI_Focus((ws->menu_bar_focused && window_is_focused && !ui_any_ctx_menu_is_open() && !hover_eval_is_open) ? UI_FocusKind_On : UI_FocusKind_Null)
        UI_Pane(top_bar_rect, str8_lit("###top_bar"))
        UI_WidthFill UI_Row
        UI_Focus(UI_FocusKind_Null)
      {
        MemoryZeroArray(ui_top_parent()->parent->corner_radii);
        
        // rjf: menu items
        UI_PrefWidth(ui_text_dim(20, 1))
        {
          // rjf: file menu
          UI_Key file_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_file_menu_key_"));
          UI_CtxMenu(file_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
          {
            DF_CoreCmdKind cmds[] =
            {
              DF_CoreCmdKind_Open,
              DF_CoreCmdKind_OpenUser,
              DF_CoreCmdKind_OpenProfile,
              DF_CoreCmdKind_Exit,
            };
            U32 codepoints[] =
            {
              'o',
              'u',
              'p',
              'x',
            };
            Assert(ArrayCount(codepoints) == ArrayCount(cmds));
            df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
          }
          
          // rjf: window menu
          UI_Key window_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_window_menu_key_"));
          UI_CtxMenu(window_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
          {
            DF_CoreCmdKind cmds[] =
            {
              DF_CoreCmdKind_OpenWindow,
              DF_CoreCmdKind_CloseWindow,
              DF_CoreCmdKind_ToggleFullscreen,
            };
            U32 codepoints[] =
            {
              'w',
              'c',
              'f',
            };
            Assert(ArrayCount(codepoints) == ArrayCount(cmds));
            df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
          }
          
          // rjf: panel menu
          UI_Key panel_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_panel_menu_key_"));
          UI_CtxMenu(panel_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
          {
            DF_CoreCmdKind cmds[] =
            {
              DF_CoreCmdKind_NewPanelRight,
              DF_CoreCmdKind_NewPanelDown,
              DF_CoreCmdKind_ClosePanel,
              DF_CoreCmdKind_RotatePanelColumns,
              DF_CoreCmdKind_NextPanel,
              DF_CoreCmdKind_PrevPanel,
              DF_CoreCmdKind_CloseTab,
              DF_CoreCmdKind_NextTab,
              DF_CoreCmdKind_PrevTab,
              DF_CoreCmdKind_TabBarTop,
              DF_CoreCmdKind_TabBarBottom,
              DF_CoreCmdKind_ResetToDefaultPanels,
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
            };
            Assert(ArrayCount(codepoints) == ArrayCount(cmds));
            df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
          }
          
          // rjf: view menu
          UI_Key view_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_view_menu_key_"));
          UI_CtxMenu(view_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
          {
            DF_CoreCmdKind cmds[] =
            {
              DF_CoreCmdKind_Targets,
              DF_CoreCmdKind_Scheduler,
              DF_CoreCmdKind_CallStack,
              DF_CoreCmdKind_Modules,
              DF_CoreCmdKind_Output,
              DF_CoreCmdKind_Memory,
              DF_CoreCmdKind_Disassembly,
              DF_CoreCmdKind_Watch,
              DF_CoreCmdKind_Locals,
              DF_CoreCmdKind_Registers,
              DF_CoreCmdKind_Globals,
              DF_CoreCmdKind_ThreadLocals,
              DF_CoreCmdKind_Types,
              DF_CoreCmdKind_Procedures,
              DF_CoreCmdKind_Breakpoints,
              DF_CoreCmdKind_WatchPins,
              DF_CoreCmdKind_FilePathMap,
              DF_CoreCmdKind_Theme,
              DF_CoreCmdKind_ExceptionFilters,
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
            };
            Assert(ArrayCount(codepoints) == ArrayCount(cmds));
            df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
          }
          
          // rjf: targets menu
          UI_Key targets_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_targets_menu_key_"));
          UI_CtxMenu(targets_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
          {
            Temp scratch = scratch_begin(&arena, 1);
            DF_CoreCmdKind cmds[] =
            {
              DF_CoreCmdKind_AddTarget,
              DF_CoreCmdKind_EditTarget,
              DF_CoreCmdKind_RemoveTarget,
            };
            U32 codepoints[] =
            {
              'a',
              'e',
              'r',
            };
            Assert(ArrayCount(codepoints) == ArrayCount(cmds));
            df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
            DF_EntityList targets_list = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
            for(DF_EntityNode *n = targets_list.first; n != 0; n = n->next)
            {
              DF_Entity *target = n->entity;
              Vec4F32 color = ui_top_text_color();
              if(target->flags & DF_EntityFlag_HasColor)
              {
                color = df_rgba_from_entity(target);
              }
              String8 target_name = df_display_string_from_entity(scratch.arena, target);
              UI_Signal sig = {0};
              UI_TextColor(color)
                sig = df_icon_buttonf(DF_IconKind_Target, 0, "%S##%p", target_name, target);
              if(ui_clicked(sig))
              {
                DF_CmdParams params = df_cmd_params_from_window(ws);
                params.entity = df_handle_from_entity(target);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EditTarget));
                ui_ctx_menu_close();
                ws->menu_bar_focused = 0;
              }
            }
            scratch_end(scratch);
          }
          
          // rjf: ctrl menu
          UI_Key ctrl_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_ctrl_menu_key_"));
          UI_CtxMenu(ctrl_menu_key) UI_PrefWidth(ui_em(30.f, 1.f))
          {
            DF_CoreCmdKind cmds[] =
            {
              DF_CoreCmdKind_Run,
              DF_CoreCmdKind_KillAll,
              DF_CoreCmdKind_Restart,
              DF_CoreCmdKind_Halt,
              DF_CoreCmdKind_SoftHaltRefresh,
              DF_CoreCmdKind_StepInto,
              DF_CoreCmdKind_StepOver,
              DF_CoreCmdKind_StepOut,
              DF_CoreCmdKind_Attach,
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
            df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
          }
          
          // rjf: help menu
          UI_Key help_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_help_menu_key_"));
          UI_CtxMenu(help_menu_key) UI_PrefWidth(ui_em(40.f, 1.f))
          {
            UI_Row UI_TextAlignment(UI_TextAlign_Center) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
              ui_label(str8_lit(BUILD_TITLE_STRING_LITERAL));
            ui_spacer(ui_em(0.25f, 1.f));
            UI_Row
              UI_PrefWidth(ui_text_dim(10, 1))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_Padding(ui_pct(1, 0))
            {
              ui_labelf("Search for commands by pressing ");
              DF_CmdSpec *spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand);
              UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_PlainText))
                UI_Flags(UI_BoxFlag_DrawBorder)
                UI_TextAlignment(UI_TextAlign_Center)
                df_cmd_binding_button(spec);
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
              UI_NavActionList *nav_actions = ui_nav_actions();
              for(UI_NavActionNode *n = nav_actions->first, *next = 0;
                  n != 0;
                  n = next)
              {
                next = n->next;
                UI_NavAction *action = &n->v;
                B32 taken = 0;
                if(action->delta.x > 0)
                {
                  taken = 1;
                  open_menu_idx_prime += 1;
                  open_menu_idx_prime = open_menu_idx_prime%ArrayCount(items);
                }
                if(action->delta.x < 0)
                {
                  taken = 1;
                  open_menu_idx_prime = open_menu_idx_prime > 0 ? open_menu_idx_prime-1 : (ArrayCount(items)-1);
                }
                if(taken)
                {
                  ui_nav_eat_action_node(nav_actions, n);
                }
              }
            }
            
            // rjf: make ui
            for(U64 idx = 0; idx < ArrayCount(items); idx += 1)
            {
              ui_set_next_fastpath_codepoint(items[idx].codepoint);
              B32 alt_fastpath_key = 0;
              if(os_key_press(ui_events(), ui_window(), OS_EventFlag_Alt, items[idx].key))
              {
                alt_fastpath_key = 1;
              }
              if((ws->menu_bar_key_held || ws->menu_bar_focused) && !ui_any_ctx_menu_is_open())
              {
                ui_set_next_flags(UI_BoxFlag_DrawTextFastpathCodepoint);
              }
              UI_Signal sig = df_menu_bar_button(items[idx].name);
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
        UI_PrefWidth(ui_text_dim(10, 1)) UI_HeightFill UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_Highlight1))
        {
          Temp scratch = scratch_begin(&arena, 1);
          DF_EntityList tasks = df_query_cached_entity_list_with_kind(DF_EntityKind_ConversionTask);
          for(DF_EntityNode *n = tasks.first; n != 0; n = n->next)
          {
            DF_Entity *task = n->entity;
            if(task->alloc_time_us + 500000 < os_now_microseconds())
            {
              String8 rdi_path = task->name;
              String8 rdi_name = str8_skip_last_slash(rdi_path);
              String8 task_text = push_str8f(scratch.arena, "Creating %S...", rdi_name);
              UI_Key key = ui_key_from_stringf(ui_key_zero(), "task_%p", task);
              UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable, key);
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
        
        ui_spacer(ui_pct(1, 0));
        
        // rjf: loaded user viz
        {
          ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_Highlight1));
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_child_layout_axis(Axis2_X);
          ui_set_next_hover_cursor(OS_Cursor_HandPoint);
          UI_Box *user_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_DrawBackground|
                                                       UI_BoxFlag_DrawHotEffects|
                                                       UI_BoxFlag_DrawActiveEffects,
                                                       "###loaded_user_button");
          UI_Parent(user_box) UI_PrefWidth(ui_text_dim(10, 1)) UI_TextAlignment(UI_TextAlign_Center)
          {
            String8 user_path = df_cfg_path_from_src(DF_CfgSrc_User);
            UI_Font(ui_icon_font()) ui_label(df_g_icon_kind_text_table[DF_IconKind_Person]);
            ui_label(str8_skip_last_slash(user_path));
          }
          UI_Signal user_sig = ui_signal_from_box(user_box);
          if(ui_clicked(user_sig))
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_OpenUser);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_CmdSpec);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
          }
        }
        
        ui_spacer(ui_em(0.75f, 1));
        
        // rjf: loaded profile viz
        {
          ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_Highlight0));
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_child_layout_axis(Axis2_X);
          ui_set_next_hover_cursor(OS_Cursor_HandPoint);
          UI_Box *prof_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_DrawBackground|
                                                       UI_BoxFlag_DrawHotEffects|
                                                       UI_BoxFlag_DrawActiveEffects,
                                                       "###loaded_profile_button");
          UI_Parent(prof_box) UI_PrefWidth(ui_text_dim(10, 1)) UI_TextAlignment(UI_TextAlign_Center)
          {
            String8 prof_path = df_cfg_path_from_src(DF_CfgSrc_Profile);
            UI_Font(ui_icon_font()) ui_label(df_g_icon_kind_text_table[DF_IconKind_Briefcase]);
            ui_label(str8_skip_last_slash(prof_path));
          }
          UI_Signal prof_sig = ui_signal_from_box(prof_box);
          if(ui_clicked(prof_sig))
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_OpenProfile);
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_CmdSpec);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
          }
        }
        
        ui_spacer(ui_em(0.75f, 1));
        
        // rjf: fast-paths
        UI_PrefWidth(ui_em(2.25f, 1))
          UI_Font(df_font_from_slot(DF_FontSlot_Icons))
          UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
        {
          Temp scratch = scratch_begin(&arena, 1);
          DF_EntityList targets = df_push_active_target_list(scratch.arena);
          DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          B32 have_targets = targets.count != 0;
          B32 can_send_signal = !df_ctrl_targets_running();
          B32 can_play  = (have_targets && can_send_signal);
          B32 can_pause = (!can_send_signal);
          B32 can_stop  = (processes.count != 0);
          
          if(can_play || !have_targets) UI_TextAlignment(UI_TextAlign_Center) UI_Flags((can_play ? 0 : UI_BoxFlag_Disabled))
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Play]);
            if(ui_hovering(sig) && !can_play)
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: %s", have_targets ? "Targets are currently running" : "No active targets exist");
            }
            if(ui_hovering(sig) && can_play)
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
              {
                if(can_stop)
                {
                  ui_labelf("Resume all processes");
                }
                else
                {
                  ui_labelf("Launch all active targets:");
                  for(DF_EntityNode *n = targets.first; n != 0; n = n->next)
                  {
                    ui_label(n->entity->name);
                  }
                }
              }
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Run));
            }
          }
          
          if(!can_play && have_targets && !can_send_signal) UI_TextAlignment(UI_TextAlign_Center)
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Redo]);
            if(ui_hovering(sig))
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
              {
                ui_labelf("Restart all running targets:");
                {
                  DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
                  for(DF_EntityNode *n = processes.first; n != 0; n = n->next)
                  {
                    DF_Entity *process = n->entity;
                    DF_Entity *target = df_entity_from_handle(process->entity_handle);
                    if(!df_entity_is_nil(target))
                    {
                      ui_label(target->name);
                    }
                  }
                }
              }
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Restart));
            }
          }
          
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_pause ? 0 : UI_BoxFlag_Disabled)
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Pause]);
            if(ui_hovering(sig) && !can_pause)
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: Already halted");
            }
            if(ui_hovering(sig) && can_pause)
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Halt all target processes");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Halt));
            }
          }
          
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_stop ? 0 : UI_BoxFlag_Disabled)
          {
            UI_Signal sig = {0};
            {
              sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Stop]);
            }
            if(ui_hovering(sig) && !can_stop)
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_stop)
            {
              UI_Tooltip
                UI_Font(df_font_from_slot(DF_FontSlot_Main))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Kill all target processes");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Kill));
            }
          }
          scratch_end(scratch);
        }
      }
    }
    
    ////////////////////////////
    //- rjf: bottom bar
    //
    ProfScope("build bottom bar")
    {
      B32 is_running = df_ctrl_targets_running() && df_ctrl_last_run_frame_idx() < df_frame_index();
      CTRL_Event stop_event = df_ctrl_last_stop_event();
      Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
      if(!is_running)
      {
        switch(stop_event.cause)
        {
          default:
          case CTRL_EventCause_Finished:
          {
            color = df_rgba_from_theme_color(DF_ThemeColor_Highlight1);
          }break;
          case CTRL_EventCause_UserBreakpoint:
          case CTRL_EventCause_InterruptedByException:
          case CTRL_EventCause_InterruptedByTrap:
          case CTRL_EventCause_InterruptedByHalt:
          {
            color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
          }break;
        }
      }
      if(ws->error_t > 0.01f)
      {
        Vec4F32 failure_bg = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
        color.x += (failure_bg.x-color.x)*ws->error_t;
        color.y += (failure_bg.y-color.y)*ws->error_t;
        color.z += (failure_bg.z-color.z)*ws->error_t;
        color.w += (failure_bg.w-color.w)*ws->error_t;
      }
      UI_Flags(UI_BoxFlag_DrawBackground) UI_BackgroundColor(color) UI_CornerRadius(0)
        UI_Pane(bottom_bar_rect, str8_lit("###bottom_bar")) UI_WidthFill UI_Row
        UI_Flags(0)
      {
        // rjf: developer frame-time indicator
        if(DEV_updating_indicator)
        {
          F32 animation_t = pow_f32(sin_f32(df_time_in_seconds()/2.f), 2.f);
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
            Temp scratch = scratch_begin(&arena, 1);
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
                UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
                ui_label(df_g_icon_kind_text_table[icon]);
            }
            UI_PrefWidth(ui_text_dim(10, 1)) ui_label(explanation);
            scratch_end(scratch);
          }
        }
        
        ui_spacer(ui_pct(1, 0));
        
        // rjf: bind change visualization
        if(df_gfx_state->bind_change_active)
        {
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_Flags(UI_BoxFlag_DrawBackground)
            UI_TextAlignment(UI_TextAlign_Center)
            UI_CornerRadius(4)
            UI_BackgroundColor(df_rgba_from_theme_color(DF_ThemeColor_Highlight0))
            ui_labelf("Currently rebinding \"%S\" hotkey", df_gfx_state->bind_change_cmd_spec->info.display_name);
        }
        
        // rjf: error visualization
        else if(ws->error_t >= 0.01f)
        {
          ws->error_t -= df_dt()/8.f;
          df_gfx_request_frame();
          Vec4F32 tx_color = df_rgba_from_theme_color(DF_ThemeColor_FailureText);
          F32 alpha_factor = Max(ws->error_t, 0.2f);
          tx_color.w *= alpha_factor;
          String8 error_string = str8(ws->error_buffer, ws->error_string_size);
          if(error_string.size != 0)
          {
            ui_set_next_text_color(tx_color);
            ui_set_next_pref_width(ui_children_sum(1));
            UI_CornerRadius(4)
              UI_Row
              UI_PrefWidth(ui_text_dim(10, 1))
              UI_TextAlignment(UI_TextAlign_Center)
            {
              UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
                ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
              ui_label(error_string);
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: prepare query view stack for the in-progress command
    //
    if(!df_cmd_spec_is_nil(ws->query_cmd_spec))
    {
      DF_CmdSpec *cmd_spec = ws->query_cmd_spec;
      DF_CmdParamSlot first_missing_slot = cmd_spec->info.query.slot;
      DF_ViewSpec *view_spec = df_view_spec_from_cmd_param_slot_spec(first_missing_slot, cmd_spec);
      if(ws->query_view_stack_top->spec != view_spec ||
         df_view_is_nil(ws->query_view_stack_top))
      {
        Temp scratch = scratch_begin(&arena, 1);
        
        // rjf: clear existing query stack
        for(DF_View *query_view = ws->query_view_stack_top, *next = 0;
            !df_view_is_nil(query_view);
            query_view = next)
        {
          next = query_view->next;
          df_view_release(query_view);
        }
        
        // rjf: determine default query
        String8 default_query = {0};
        switch(first_missing_slot)
        {
          default:
          if(cmd_spec->info.query.flags & DF_CmdQueryFlag_KeepOldInput)
          {
            default_query = df_push_search_string(scratch.arena);
          }break;
          case DF_CmdParamSlot_FilePath:
          {
            default_query = path_normalized_from_string(scratch.arena, df_current_path());
            default_query = push_str8f(scratch.arena, "%S/", default_query);
          }break;
        }
        
        // rjf: construct & push new view
        DF_View *view = df_view_alloc();
        df_view_equip_spec(view, view_spec, &df_g_nil_entity, default_query, &df_g_nil_cfg_node);
        if(cmd_spec->info.query.flags & DF_CmdQueryFlag_SelectOldInput)
        {
          view->query_mark = txt_pt(1, 1);
        }
        ws->query_view_stack_top = view;
        ws->query_view_selected = 1;
        view->next = &df_g_nil_view;
        
        scratch_end(scratch);
      }
    }
    
    ////////////////////////////
    //- rjf: animate query info
    //
    {
      F32 rate = 1 - pow_f32(2, (-60.f * df_dt()));
      
      // rjf: animate query view selection transition
      {
        F32 target = (F32)!!ws->query_view_selected;
        F32 diff = abs_f32(target - ws->query_view_selected_t);
        if(diff > 0.005f)
        {
          df_gfx_request_frame();
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
          df_gfx_request_frame();
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
    {
      DF_View *view = ws->query_view_stack_top;
      DF_CmdSpec *cmd_spec = ws->query_cmd_spec;
      DF_CmdQuery *query = &cmd_spec->info.query;
      
      //- rjf: calculate rectangles
      Vec2F32 window_center = center_2f32(window_rect);
      F32 query_container_width = dim_2f32(window_rect).x*0.5f;
      F32 query_container_margin = ui_top_font_size()*8.f;
      F32 query_line_edit_height = ui_top_font_size()*3.f;
      Rng2F32 query_container_rect = r2f32p(window_center.x - query_container_width/2 + (1-ws->query_view_t)*query_container_width/4,
                                            window_rect.y0 + query_container_margin,
                                            window_center.x + query_container_width/2 - (1-ws->query_view_t)*query_container_width/4,
                                            window_rect.y1 - query_container_margin);
      if(ws->query_view_stack_top->spec == &df_g_nil_view_spec)
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
                                                        UI_BoxFlag_DrawBorder|
                                                        UI_BoxFlag_DrawBackground|
                                                        UI_BoxFlag_DrawBackgroundBlur|
                                                        UI_BoxFlag_DrawDropShadow,
                                                        "panel_query_container");
      }
      
      //- rjf: build query text input
      UI_Parent(query_container_box)
        UI_WidthFill UI_PrefHeight(ui_px(query_line_edit_height, 1.f))
        UI_Focus(UI_FocusKind_On)
      {
        ui_set_next_flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder);
        UI_Row
        {
          UI_PrefWidth(ui_text_dim(10.f, 1.f))
          {
            ui_spacer(ui_em(0.5f, 1.f));
            DF_IconKind icon_kind = ws->query_cmd_spec->info.canonical_icon_kind;
            if(icon_kind != DF_IconKind_Null)
            {
              UI_Font(df_font_from_slot(DF_FontSlot_Icons)) ui_label(df_g_icon_kind_text_table[icon_kind]);
            }
            ui_labelf("%S", ws->query_cmd_spec->info.display_name);
            ui_spacer(ui_em(0.5f, 1.f));
          }
          UI_Font((query->flags & DF_CmdQueryFlag_CodeInput) ? df_font_from_slot(DF_FontSlot_Code) : ui_top_font())
          {
            UI_Signal sig = df_line_edit(DF_LineEditFlag_Border|
                                         (DF_LineEditFlag_CodeContents * !!(query->flags & DF_CmdQueryFlag_CodeInput)),
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
        }
      }
      
      //- rjf: build query view
      UI_Parent(query_container_box) UI_WidthFill UI_Focus(UI_FocusKind_Null)
      {
        DF_ViewSpec *view_spec = view->spec;
        DF_ViewUIFunctionType *build_view_ui_function = view_spec->info.ui_hook;
        build_view_ui_function(ws, &df_g_nil_panel, view, query_container_content_rect);
      }
      
      //- rjf: query submission
      if((ui_is_focus_active() || (window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && !ws->query_view_selected)) &&
         os_key_press(events, ws->os, 0, OS_Key_Esc))
      {
        DF_CmdParams params = df_cmd_params_from_window(ws);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CancelQuery));
      }
      if(ui_is_focus_active())
      {
        if(os_key_press(events, ws->os, 0, OS_Key_Return))
        {
          Temp scratch = scratch_begin(&arena, 1);
          DF_View *view = ws->query_view_stack_top;
          DF_CmdParams params = df_cmd_params_from_window(ws);
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
          String8 error = df_cmd_params_apply_spec_query(scratch.arena, &ctrl_ctx, &params, ws->query_cmd_spec, str8(view->query_buffer, view->query_string_size));
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CompleteQuery));
          if(error.size != 0)
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.string = error;
            df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
          }
          scratch_end(scratch);
        }
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
      UI_BackgroundColor(mix_4f32(df_rgba_from_theme_color(DF_ThemeColor_InactivePanelOverlay), v4f32(0, 0, 0, 0), 1-ws->query_view_selected_t))
        UI_Rect(window_rect)
      {
        ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
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
        for(DF_Panel *panel = ws->root_panel;
            !df_panel_is_nil(panel);
            panel = df_panel_rec_df_pre(panel).next)
        {
          if(!df_panel_is_nil(panel->first)) { continue; }
          Rng2F32 panel_rect = df_rect_from_panel(content_rect, ws->root_panel, panel);
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          if(!df_view_is_nil(view) &&
             contains_2f32(panel_rect, ui_mouse()) &&
             (abs_f32(view->scroll_pos.x.off) > 0.01f ||
              abs_f32(view->scroll_pos.y.off) > 0.01f))
          {
            build_hover_eval = 0;
            ws->hover_eval_first_frame_idx = df_frame_index();
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
      if(ws->hover_eval_string.size != 0 && !hover_eval_is_open && ws->hover_eval_last_frame_idx < ws->hover_eval_first_frame_idx+20 && df_frame_index()-ws->hover_eval_last_frame_idx < 50)
      {
        df_gfx_request_frame();
        ws->hover_eval_num_visible_rows_t = 0;
        ws->hover_eval_open_t = 0;
      }
      
      // rjf: build hover eval
      if(build_hover_eval && ws->hover_eval_string.size != 0 && hover_eval_is_open)
        UI_Font(df_font_from_slot(DF_FontSlot_Code))
        UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
      {
        Temp scratch = scratch_begin(&arena, 1);
        DBGI_Scope *scope = dbgi_scope_open();
        DF_CtrlCtx ctrl_ctx = ws->hover_eval_ctrl_ctx;
        DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
        DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
        U64 thread_unwind_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, ctrl_ctx.unwind_count);
        EVAL_ParseCtx parse_ctx = df_eval_parse_ctx_from_process_vaddr(scope, process, thread_unwind_rip_vaddr);
        EVAL_String2ExprMap *macro_map = &eval_string2expr_map_nil;
        String8 expr = ws->hover_eval_string;
        DF_Eval eval = df_eval_from_string(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, expr);
        
        //- rjf: build if good
        if(!tg_key_match(eval.type_key, tg_key_zero()) && !ui_any_ctx_menu_is_open())
          UI_Focus((hover_eval_is_open && !ui_any_ctx_menu_is_open() && (!query_is_open || !ws->query_view_selected)) ? UI_FocusKind_Null : UI_FocusKind_Off)
        {
          //- rjf: eval -> viz artifacts
          F32 row_height = ui_top_font_size()*2.25f;
          DF_CfgTable cfg_table = {0};
          U64 expr_hash = df_hash_from_string(expr);
          DF_EvalViewKey eval_view_key = df_eval_view_key_from_stringf("eval_hover_%I64x", expr_hash);
          DF_EvalView *eval_view = df_eval_view_from_key(eval_view_key);
          DF_ExpandKey parent_key = df_expand_key_make(5381, 1);
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(parent_key), 1);
          DF_EvalVizBlockList viz_blocks = df_eval_viz_block_list_from_eval_view_expr_keys(scratch.arena, scope, &ctrl_ctx, &parse_ctx, macro_map, eval_view, expr, parent_key, key);
          U32 default_radix = (eval.mode == EVAL_EvalMode_Reg ? 16 : 10);
          DF_EvalVizWindowedRowList viz_rows = df_eval_viz_windowed_row_list_from_viz_block_list(scratch.arena, scope, &ctrl_ctx, &parse_ctx, macro_map, eval_view, default_radix, ui_top_font(), ui_top_font_size(), r1s64(0, 50), &viz_blocks);
          
          //- rjf: animate
          {
            // rjf: animate height
            {
              F32 fish_rate = 1 - pow_f32(2, (-60.f * df_dt()));
              F32 hover_eval_container_height_target = row_height * Min(30, viz_blocks.total_visual_row_count);
              ws->hover_eval_num_visible_rows_t += (hover_eval_container_height_target - ws->hover_eval_num_visible_rows_t) * fish_rate;
              if(abs_f32(hover_eval_container_height_target - ws->hover_eval_num_visible_rows_t) > 0.5f)
              {
                df_gfx_request_frame();
              }
              else
              {
                ws->hover_eval_num_visible_rows_t = hover_eval_container_height_target;
              }
            }
            
            // rjf: animate open
            {
              F32 fish_rate = 1 - pow_f32(2, (-60.f * df_dt()));
              F32 diff = 1.f - ws->hover_eval_open_t;
              ws->hover_eval_open_t += diff*fish_rate;
              if(abs_f32(diff) < 0.01f)
              {
                ws->hover_eval_open_t = 1.f;
              }
              else
              {
                df_gfx_request_frame();
              }
            }
          }
          
          //- rjf: build hover eval box
          F32 hover_eval_container_height = ws->hover_eval_num_visible_rows_t;
          F32 corner_radius = ui_top_font_size()*0.25f;
          ui_set_next_fixed_x(ws->hover_eval_spawn_pos.x);
          ui_set_next_fixed_y(ws->hover_eval_spawn_pos.y);
          ui_set_next_pref_width(ui_em(80.f, 1.f));
          ui_set_next_pref_height(ui_px(hover_eval_container_height, 1.f));
          ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_AltBackground));
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
            F32 expr_column_width_px = 0;
            
            //- rjf: build rows
            for(DF_EvalVizRow *row = viz_rows.first; row != 0; row = row->next)
            {
              //- rjf: calculate width of exp row
              if(row == viz_rows.first)
              {
                expr_column_width_px = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), row->expr).x + ui_top_font_size()*0.5f;
                expr_column_width_px = Max(expr_column_width_px, ui_top_font_size()*10.f);
              }
              
              //- rjf: determine if row's data is fresh
              B32 row_is_fresh = 0;
              switch(row->eval.mode)
              {
                default:{}break;
                case EVAL_EvalMode_Addr:
                {
                  U64 size = tg_byte_size_from_graph_rdi_key(parse_ctx.type_graph, parse_ctx.rdi, row->eval.type_key);
                  size = Min(size, 64);
                  Rng1U64 vaddr_rng = r1u64(row->eval.offset, row->eval.offset+size);
                  CTRL_ProcessMemorySlice slice = ctrl_query_cached_data_from_process_vaddr_range(scratch.arena, process->ctrl_machine_id, process->ctrl_handle, vaddr_rng, 0);
                  for(U64 idx = 0; idx < (slice.data.size+63)/64; idx += 1)
                  {
                    if(slice.byte_changed_flags[idx] != 0)
                    {
                      row_is_fresh = 1;
                      break;
                    }
                  }
                }break;
              }
              
              //- rjf: build row
              UI_WidthFill UI_Row
              {
                ui_spacer(ui_em(0.75f, 1.f));
                ui_spacer(ui_em(1.5f*row->depth, 1.f));
                U64 row_hash = df_hash_from_expand_key(row->key);
                B32 row_is_expanded = df_expand_key_is_set(&eval_view->expand_tree_table, row->key);
                if(row->flags & DF_EvalVizRowFlag_CanExpand)
                  UI_PrefWidth(ui_em(1.5f, 1)) UI_Flags(UI_BoxFlag_DrawSideLeft*(row->depth>0))
                  if(ui_pressed(ui_expanderf(row_is_expanded, "###%I64x_%I64x_is_expanded", row->key.parent_hash, row->key.child_num)))
                {
                  df_expand_set_expansion(eval_view->arena, &eval_view->expand_tree_table, row->parent_key, row->key, !row_is_expanded);
                }
                if(!(row->flags & DF_EvalVizRowFlag_CanExpand))
                {
                  UI_PrefWidth(ui_em(1.5f, 1))
                    UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                    UI_Flags(UI_BoxFlag_DrawSideLeft*(row->depth>0))
                    UI_Font(ui_icon_font())
                    ui_label(df_g_icon_kind_text_table[DF_IconKind_Dot]);
                }
                UI_WidthFill
                {
                  UI_PrefWidth(ui_px(expr_column_width_px, 1.f)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), row->expr);
                  ui_spacer(ui_em(1.5f, 1.f));
                  if(row->flags & DF_EvalVizRowFlag_CanEditValue)
                  {
                    if(row_is_fresh)
                    {
                      Vec4F32 rgba = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
                      rgba.w *= 0.2f;
                      ui_set_next_background_color(rgba);
                    }
                    UI_Signal sig = df_line_editf(DF_LineEditFlag_CodeContents|
                                                  DF_LineEditFlag_DisplayStringIsCode|
                                                  DF_LineEditFlag_PreferDisplayString|
                                                  DF_LineEditFlag_Border,
                                                  0, 0, &ws->hover_eval_txt_cursor, &ws->hover_eval_txt_mark, ws->hover_eval_txt_buffer, sizeof(ws->hover_eval_txt_buffer), &ws->hover_eval_txt_size, 0, row->edit_value, "%S###val_%I64x", row->display_value, row_hash);
                    if(ui_committed(sig))
                    {
                      String8 commit_string = str8(ws->hover_eval_txt_buffer, ws->hover_eval_txt_size);
                      DF_Eval write_eval = df_eval_from_string(scratch.arena, scope, &ctrl_ctx, &parse_ctx, &eval_string2expr_map_nil, commit_string);
                      B32 success = df_commit_eval_value(parse_ctx.type_graph, parse_ctx.rdi, &ctrl_ctx, row->eval, write_eval);
                      if(success == 0)
                      {
                        DF_CmdParams params = df_cmd_params_from_window(ws);
                        params.string = str8_lit("Could not commit value successfully.");
                        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                      }
                    }
                  }
                  else
                  {
                    if(row_is_fresh)
                    {
                      Vec4F32 rgba = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
                      rgba.w *= 0.2f;
                      ui_set_next_background_color(rgba);
                      ui_set_next_flags(UI_BoxFlag_DrawBackground);
                    }
                    df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), row->display_value);
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
                    if(ui_hovering(watch_sig)) UI_Tooltip UI_Font(df_font_from_slot(DF_FontSlot_Main)) UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                    {
                      ui_labelf("Add the hovered expression to an opened watch view.");
                    }
                    if(ui_clicked(watch_sig))
                    {
                      DF_CmdParams params = df_cmd_params_from_window(ws);
                      params.string = expr;
                      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpression));
                    }
                  }
                  if(!df_entity_is_nil(df_entity_from_handle(ws->hover_eval_file)))
                    UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_CornerRadius10(corner_radius)
                    UI_CornerRadius11(corner_radius)
                  {
                    UI_Signal pin_sig = df_icon_buttonf(DF_IconKind_Pin, 0, "###pin_hover_eval");
                    if(ui_hovering(pin_sig)) UI_Tooltip UI_Font(df_font_from_slot(DF_FontSlot_Main)) UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                      UI_CornerRadius00(0)
                      UI_CornerRadius01(0)
                      UI_CornerRadius10(0)
                      UI_CornerRadius11(0)
                    {
                      ui_labelf("Pin the hovered expression to this code location.");
                    }
                    if(ui_clicked(pin_sig))
                    {
                      DF_CmdParams params = df_cmd_params_from_window(ws);
                      if(ws->hover_eval_vaddr != 0)
                      {
                        params.vaddr = ws->hover_eval_vaddr;
                        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
                      }
                      else
                      {
                        params.entity = ws->hover_eval_file;
                        params.text_point = ws->hover_eval_file_pt;
                        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
                      }
                      params.string = expr;
                      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchPin));
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
              ws->hover_eval_last_frame_idx = df_frame_index();
            }
            else if(ws->hover_eval_last_frame_idx+2 < df_frame_index())
            {
              df_gfx_request_frame();
            }
          }
        }
        
        dbgi_scope_close(scope);
        scratch_end(scratch);
      }
    }
    
    ////////////////////////////
    //- rjf: panel non-leaf UI (drag boundaries)
    //
    B32 is_changing_panel_boundaries = 0;
    ProfScope("non-leaf panel UI")
      for(DF_Panel *panel = ws->root_panel;
          !df_panel_is_nil(panel);
          panel = df_panel_rec_df_pre(panel).next)
    {
      //- rjf: continue on leaf panels
      if(df_panel_is_nil(panel->first))
      {
        continue;
      }
      
      //- rjf: grab info
      Axis2 split_axis = panel->split_axis;
      Rng2F32 panel_rect = df_rect_from_panel(content_rect, ws->root_panel, panel);
      
      //- rjf: do UI for boundaries between all children
      for(DF_Panel *child = panel->first; !df_panel_is_nil(child) && !df_panel_is_nil(child->next); child = child->next)
      {
        DF_Panel *min_child = child;
        DF_Panel *max_child = min_child->next;
        Rng2F32 min_child_rect = df_rect_from_panel_child(panel_rect, panel, min_child);
        Rng2F32 max_child_rect = df_rect_from_panel_child(panel_rect, panel, max_child);
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
            F32 sum_pct = min_child->size_pct_of_parent_target.v[split_axis] + max_child->size_pct_of_parent_target.v[split_axis];
            min_child->size_pct_of_parent_target.v[split_axis] = 0.5f * sum_pct;
            max_child->size_pct_of_parent_target.v[split_axis] = 0.5f * sum_pct;
          }
          else if(ui_pressed(sig))
          {
            Vec2F32 v = {min_child->size_pct_of_parent_target.v[split_axis], max_child->size_pct_of_parent_target.v[split_axis]};
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
            min_child->size_pct_of_parent.v[split_axis] = min_child->size_pct_of_parent_target.v[split_axis] = min_pct__after;
            max_child->size_pct_of_parent.v[split_axis] = max_child->size_pct_of_parent_target.v[split_axis] = max_pct__after;
            is_changing_panel_boundaries = 1;
          }
          if(ui_released(sig) || ui_double_clicked(sig))
          {
            df_panel_notify_mutation(ws, min_child);
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: panel leaf UI
    //
    ProfScope("leaf panel UI")
      for(DF_Panel *panel = ws->root_panel;
          !df_panel_is_nil(panel);
          panel = df_panel_rec_df_pre(panel).next)
    {
      if(!df_panel_is_nil(panel->first)) {continue;}
      B32 panel_is_focused = (window_is_focused &&
                              !ws->menu_bar_focused &&
                              (!query_is_open || !ws->query_view_selected) &&
                              !ui_any_ctx_menu_is_open() &&
                              !hover_eval_is_open &&
                              ws->focused_panel == panel);
      UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
      {
        //////////////////////////
        //- rjf: calculate UI rectangles
        //
        Rng2F32 panel_rect = df_rect_from_panel(content_rect, ws->root_panel, panel);
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
          DF_View *tab = df_view_from_handle(panel->selected_tab_view);
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
        //- rjf: build filtering box
        //
        {
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          UI_Focus(UI_FocusKind_On)
          {
            if(view->is_filtering && ui_is_focus_active() && os_key_press(ui_events(), ui_window(), 0, OS_Key_Return))
            {
              DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ApplyFilter));
            }
            if(view->is_filtering || view->is_filtering_t > 0.01f)
            {
              UI_Box *filter_box = &ui_g_nil_box;
              UI_Rect(filter_rect)
              {
                ui_set_next_child_layout_axis(Axis2_X);
                filter_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip|UI_BoxFlag_DrawBorder, "filter_box_%p", view);
              }
              UI_Parent(filter_box) UI_WidthFill UI_HeightFill
              {
                UI_PrefWidth(ui_em(2.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                  UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                  ui_label(df_g_icon_kind_text_table[DF_IconKind_Find]);
                UI_PrefWidth(ui_text_dim(10, 1))
                {
                  ui_label(str8_lit("Filter"));
                }
                ui_spacer(ui_em(0.5f, 1.f));
                UI_Font(view->spec->info.flags & DF_ViewSpecFlag_FilterIsCode ? df_font_from_slot(DF_FontSlot_Code) : df_font_from_slot(DF_FontSlot_Main)) UI_Focus(view->is_filtering ? UI_FocusKind_On : UI_FocusKind_Off)
                {
                  UI_Signal sig = df_line_edit(DF_LineEditFlag_Border|
                                               DF_LineEditFlag_CodeContents*!!(view->spec->info.flags & DF_ViewSpecFlag_FilterIsCode),
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
                    DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                    df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
                  }
                }
              }
            }
          }
        }
        
        //////////////////////////
        //- rjf: build panel container box
        //
        UI_Box *panel_box = &ui_g_nil_box;
        UI_Rect(content_rect) UI_ChildLayoutAxis(Axis2_Y) UI_CornerRadius(0) UI_Focus(UI_FocusKind_On)
        {
          UI_Key panel_key = df_ui_key_from_panel(panel);
          if(ws->focused_panel != panel)
          {
            ui_set_next_overlay_color(df_rgba_from_theme_color(DF_ThemeColor_InactivePanelOverlay));
          }
          panel_box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                            UI_BoxFlag_Clip|
                                            UI_BoxFlag_DrawBorder|
                                            ((ws->focused_panel != panel)*UI_BoxFlag_DisableFocusViz)|
                                            ((ws->focused_panel != panel)*UI_BoxFlag_DrawOverlay),
                                            panel_key);
        }
        
        //////////////////////////
        //- rjf: flash animation for stable view
        //
        UI_Parent(panel_box)
        {
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          if(view->flash_t >= 0.001f)
          {
            UI_Box *panel_box = ui_top_parent();
            Rng2F32 panel_rect = panel_box->rect;
            Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
            color.w *= view->flash_t;
            color.w *= 0.35f;
            ui_set_next_fixed_x(0);
            ui_set_next_fixed_y(0);
            ui_set_next_fixed_width(panel_rect_dim.x);
            ui_set_next_fixed_height(panel_rect_dim.y);
            ui_set_next_background_color(color);
            ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
          }
        }
        
        //////////////////////////
        //- rjf: loading animation for stable view
        //
        UI_Parent(panel_box)
        {
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          if(view->loading_t >= 0.001f)
          {
            // rjf: set up dimensions
            F32 edge_padding = 30.f;
            F32 width = ui_top_font_size() * 10;
            F32 height = ui_top_font_size() * 1.f;
            F32 min_thickness = ui_top_font_size()/2;
            F32 trail = ui_top_font_size() * 4;
            F32 t = pow_f32(sin_f32((F32)df_time_in_seconds() / 1.8f), 2.f);
            F64 v = 1.f - abs_f32(0.5f - t);
            Rng2F32 panel_rect = panel_box->rect;
            
            // rjf: colors
            Vec4F32 bg_color = v4f32(0.1f, 0.1f, 0.1f, 1);
            Vec4F32 bd_color = df_rgba_from_theme_color(DF_ThemeColor_PlainBorder);
            Vec4F32 hl_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
            bg_color.w *= view->loading_t;
            bd_color.w *= view->loading_t;
            hl_color.w *= view->loading_t;
            
            // rjf: grab animation params
            F32 bg_work_indicator_t = 1.f;
            
            // rjf: build indicator
            UI_CornerRadius(height/3.f)
            {
              // rjf: rects
              Rng2F32 indicator_region_rect =
                r2f32p((panel_rect.x0 + panel_rect.x1)/2 - width/2  - panel_rect.x0,
                       (panel_rect.y0 + panel_rect.y1)/2 - height/2 - panel_rect.y0,
                       (panel_rect.x0 + panel_rect.x1)/2 + width/2  - panel_rect.x0,
                       (panel_rect.y0 + panel_rect.y1)/2 + height/2 - panel_rect.y0);
              Rng2F32 indicator_rect =
                r2f32p(indicator_region_rect.x0 + width*t - min_thickness/2 - trail*v,
                       indicator_region_rect.y0,
                       indicator_region_rect.x0 + width*t + min_thickness/2 + trail*v,
                       indicator_region_rect.y1);
              indicator_rect.x0 = Clamp(indicator_region_rect.x0, indicator_rect.x0, indicator_region_rect.x1);
              indicator_rect.x1 = Clamp(indicator_region_rect.x0, indicator_rect.x1, indicator_region_rect.x1);
              indicator_rect = pad_2f32(indicator_rect, -1.f);
              
              // rjf: does the view have loading *progress* info? -> draw extra progress layer
              if(view->loading_progress_v != view->loading_progress_v_target)
              {
                F64 pct_done_f64 = ((F64)view->loading_progress_v/(F64)view->loading_progress_v_target);
                F32 pct_done = (F32)pct_done_f64;
                ui_set_next_background_color(v4f32(1, 1, 1, 0.2f*view->loading_t));
                ui_set_next_fixed_x(indicator_region_rect.x0);
                ui_set_next_fixed_y(indicator_region_rect.y0);
                ui_set_next_fixed_width(dim_2f32(indicator_region_rect).x*pct_done);
                ui_set_next_fixed_height(dim_2f32(indicator_region_rect).y);
                ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
              }
              
              // rjf: fill
              ui_set_next_background_color(hl_color);
              ui_set_next_fixed_x(indicator_rect.x0);
              ui_set_next_fixed_y(indicator_rect.y0);
              ui_set_next_fixed_width(dim_2f32(indicator_rect).x);
              ui_set_next_fixed_height(dim_2f32(indicator_rect).y);
              ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
              
              // rjf: animated bar
              ui_set_next_background_color(bg_color);
              ui_set_next_border_color(bd_color);
              ui_set_next_fixed_x(indicator_region_rect.x0);
              ui_set_next_fixed_y(indicator_region_rect.y0);
              ui_set_next_fixed_width(dim_2f32(indicator_region_rect).x);
              ui_set_next_fixed_height(dim_2f32(indicator_region_rect).y);
              UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY|UI_BoxFlag_Clickable, "bg_system_status");
              UI_Signal sig = ui_signal_from_box(box);
            }
            
            // rjf: build background
            UI_WidthFill UI_HeightFill
            {
              ui_set_next_background_color(bg_color);
              ui_set_next_blur_size(10.f*view->loading_t);
              ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
            }
          }
        }
        
        //////////////////////////
        //- rjf: build selected tab view
        //
        UI_Parent(panel_box)
          UI_Focus(panel_is_focused ? UI_FocusKind_Null : UI_FocusKind_Off)
          UI_WidthFill
        {
          //- rjf: build view container
          UI_Box *view_container_box = &ui_g_nil_box;
          UI_FixedWidth(dim_2f32(content_rect).x)
            UI_FixedHeight(dim_2f32(content_rect).y)
            UI_ChildLayoutAxis(Axis2_Y)
          {
            view_container_box = ui_build_box_from_key(0, ui_key_zero());
          }
          
          //- rjf: build empty view
          UI_Parent(view_container_box) if(df_view_is_nil(df_view_from_handle(panel->selected_tab_view)))
          {
            DF_VIEW_UI_FUNCTION_NAME(Empty)(ws, panel, &df_g_nil_view, content_rect);
          }
          
          //- rjf: build tab view
          UI_Parent(view_container_box) if(!df_view_is_nil(df_view_from_handle(panel->selected_tab_view)))
          {
            DF_View *view = df_view_from_handle(panel->selected_tab_view);
            DF_ViewUIFunctionType *build_view_ui_function = view->spec->info.ui_hook;
            build_view_ui_function(ws, panel, view, content_rect);
          }
        }
        
        //////////////////////////
        //- rjf: take events to automatically start/end filtering, if applicable
        //
        UI_Focus(UI_FocusKind_On)
        {
          DF_View *view = df_view_from_handle(panel->selected_tab_view);
          if(ui_is_focus_active() && view->spec->info.flags & DF_ViewSpecFlag_TypingAutomaticallyFilters && !view->is_filtering)
          {
            DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
            for(UI_NavActionNode *n = ui_nav_actions()->first, *next = 0; n != 0; n = next)
            {
              next = n->next;
              if(n->v.flags & UI_NavActionFlag_Paste)
              {
                ui_nav_eat_action_node(ui_nav_actions(), n);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter));
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Paste));
              }
              else if(n->v.insertion.size != 0)
              {
                ui_nav_eat_action_node(ui_nav_actions(), n);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter));
                p.string = n->v.insertion;
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_InsertText));
              }
            }
          }
          if((view->query_string_size != 0 || view->is_filtering) && ui_is_focus_active() && os_key_press(ui_events(), ui_window(), 0, OS_Key_Esc))
          {
            DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClearFilter));
          }
        }
        
        //////////////////////////
        //- rjf: consume panel fallthrough interaction events
        //
        UI_Signal panel_sig = ui_signal_from_box(panel_box);
        if(ui_pressed(panel_sig))
        {
          DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
        }
        
        //////////////////////////
        //- rjf: build tab bar
        //
        UI_Focus(UI_FocusKind_Off)
        {
          Temp scratch = scratch_begin(&arena, 1);
          
          // rjf: types
          typedef struct DropSite DropSite;
          struct DropSite
          {
            F32 p;
            DF_View *prev_view;
          };
          
          // rjf: prep output data
          DF_View *next_selected_tab_view = df_view_from_handle(panel->selected_tab_view);
          UI_Box *tab_bar_box = &ui_g_nil_box;
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
            Temp scratch = scratch_begin(&arena, 1);
            F32 corner_radius = ui_em(0.6f, 1.f).value;
            ui_spacer(ui_px(1.f, 1.f));
            
            // rjf: build tab list ctx menu
            UI_Key tab_list_ctx_menu_key = ui_key_from_stringf(ui_key_zero(), "###tab_list_ctx_menu_%p", panel);
            UI_CtxMenu(tab_list_ctx_menu_key) UI_PrefWidth(ui_em(22.f, 1.f)) UI_PrefHeight(ui_em(2.25f, 1.f))
            {
              for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
              {
                B32 view_is_selected = (view == df_view_from_handle(panel->selected_tab_view));
                DF_IconKind icon_kind = df_icon_kind_from_view(view);
                DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
                String8 label = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
                if(view_is_selected)
                {
                  ui_set_next_background_color(df_rgba_from_theme_color(DF_ThemeColor_TabActive));
                }
                ui_set_next_hover_cursor(OS_Cursor_HandPoint);
                UI_Box *tab_list_item_box = ui_build_box_from_stringf(UI_BoxFlag_DrawHotEffects|
                                                                      UI_BoxFlag_DrawActiveEffects|
                                                                      UI_BoxFlag_DrawBorder|
                                                                      UI_BoxFlag_DrawBackground|
                                                                      UI_BoxFlag_Clickable,
                                                                      "###tab_list_item_box_%p", view);
                UI_Parent(tab_list_item_box)
                {
                  UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                    UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                    UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_TextAlignment(UI_TextAlign_Center)
                    ui_label(df_g_icon_kind_text_table[icon_kind]);
                  UI_PrefWidth(ui_text_dim(10.f, 1.f))
                    ui_label(label);
                }
                UI_Signal sig = ui_signal_from_box(tab_list_item_box);
                if(ui_clicked(sig))
                {
                  next_selected_tab_view = view;
                  DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
                }
              }
            }
            
            // rjf: build tab list button
            if(panel->tab_view_count > 5) UI_PrefWidth(ui_em(2.25f, 1.f)) UI_PrefHeight(ui_px(tab_bar_vheight, 1))
            {
              UI_Signal sig = df_icon_buttonf(DF_IconKind_List, 0, "###tab_list_%p", panel);
              if(ui_clicked(sig))
              {
                if(ui_ctx_menu_is_open(tab_list_ctx_menu_key))
                {
                  ui_ctx_menu_close();
                }
                else
                {
                  ui_ctx_menu_open(tab_list_ctx_menu_key, sig.box->key, v2f32(0, dim_2f32(sig.box->rect).y));
                }
              }
            }
            
            // rjf: build tabs
            UI_PrefWidth(ui_em(18.f, 0.5f))
              UI_CornerRadius00(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius01(panel->tab_side == Side_Min ? 0 : corner_radius)
              UI_CornerRadius10(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius11(panel->tab_side == Side_Min ? 0 : corner_radius)
              for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next, view_idx += 1)
            {
              temp_end(scratch);
              
              // rjf: gather info for this tab
              B32 view_is_selected = (view == df_view_from_handle(panel->selected_tab_view));
              DF_IconKind icon_kind = df_icon_kind_from_view(view);
              DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
              String8 label = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
              
              // rjf: begin vertical region for this tab
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *tab_column_box = ui_build_box_from_stringf(!is_changing_panel_boundaries*UI_BoxFlag_AnimatePosX, "tab_column_%p", view);
              
              // rjf: build tab container box
              UI_Parent(tab_column_box) UI_PrefHeight(ui_px(tab_bar_vheight, 1))
              {
                if((!view_is_selected && panel->tab_side == Side_Min) ||
                   (view_is_selected && panel->tab_side == Side_Max))
                {
                  ui_spacer(ui_px(tab_bar_rv_diff, 1.f));
                }
                else
                {
                  ui_spacer(ui_px(1.f, 1.f));
                }
                Vec4F32 bg_color = df_rgba_from_theme_color(view_is_selected ? DF_ThemeColor_TabActive : DF_ThemeColor_TabInactive);
                if(view_is_selected && panel != ws->focused_panel)
                {
                  bg_color.w *= 0.5f;
                }
                ui_set_next_hover_cursor(OS_Cursor_HandPoint);
                ui_set_next_background_color(bg_color);
                ui_set_next_border_color(mix_4f32(v4f32(bg_color.x, bg_color.y, bg_color.z, bg_color.w), v4f32(1, 1, 1, 0.2f), 0.5f));
                UI_Box *tab_box = ui_build_box_from_stringf(UI_BoxFlag_DrawHotEffects|
                                                            UI_BoxFlag_DrawBackground|
                                                            UI_BoxFlag_DrawBorder|
                                                            (UI_BoxFlag_DrawDropShadow*view_is_selected)|
                                                            UI_BoxFlag_AnimatePosY|
                                                            UI_BoxFlag_Clickable,
                                                            "tab_%p", view);
                
                // rjf: build tab contents
                UI_Parent(tab_box)
                {
                  if(icon_kind != DF_IconKind_Null)
                  {
                    UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
                      UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                      UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
                      UI_TextAlignment(UI_TextAlign_Center)
                      UI_PrefWidth(ui_em(2.25f, 1.f))
                      ui_label(df_g_icon_kind_text_table[icon_kind]);
                  }
                  UI_TextColor(df_rgba_from_theme_color(view_is_selected ? DF_ThemeColor_PlainText : DF_ThemeColor_WeakText))
                    UI_WidthFill
                    ui_label(label);
                  UI_PrefWidth(ui_em(2.35f, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
                    UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                    UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons)*0.75f)
                    UI_BackgroundColor(v4f32(0, 0, 0, 0))
                    UI_CornerRadius00(0)
                    UI_CornerRadius01(0)
                  {
                    UI_Signal sig = ui_buttonf("%S###close_view_%p", df_g_icon_kind_text_table[DF_IconKind_X], view);
                    if(ui_clicked(sig) || ui_middle_clicked(sig))
                    {
                      DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseTab));
                    }
                  }
                }
                
                // rjf: consume events for tab clicking
                {
                  UI_Signal sig = ui_signal_from_box(tab_box);
                  if(ui_pressed(sig))
                  {
                    next_selected_tab_view = view;
                    DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                    df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
                  }
                  else if(ui_dragging(sig) && !df_drag_is_active() && length_2f32(ui_drag_delta()) > 10.f)
                  {
                    DF_DragDropPayload payload = {0};
                    {
                      payload.key = sig.box->key;
                      payload.panel = df_handle_from_panel(panel);
                      payload.view = df_handle_from_view(view);
                    }
                    df_drag_begin(&payload);
                  }
                  else if(ui_right_clicked(sig))
                  {
                    ui_ctx_menu_open(ws->tab_ctx_menu_key, sig.box->key, v2f32(0, sig.box->rect.y1 - sig.box->rect.y0));
                    ws->tab_ctx_menu_view = df_handle_from_view(view);
                  }
                  else if(ui_middle_clicked(sig))
                  {
                    DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
                    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseTab));
                  }
                  if(ui_released(sig))
                  {
                    df_panel_notify_mutation(ws, panel);
                  }
                }
              }
              
              // rjf: space for next tab
              if(!df_view_is_nil(view->next))
              {
                ui_spacer(ui_em(0.15f, 1.f));
              }
              
              // rjf: store off drop-site
              drop_sites[view_idx].p = tab_column_box->rect.x0 - tab_spacing/2;
              drop_sites[view_idx].prev_view = view->prev;
              drop_site_max_p = Max(tab_column_box->rect.x1, drop_site_max_p);
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
            DF_View *view = df_view_from_handle(df_g_drag_drop_payload.view);
            if(df_drag_is_active() && window_is_focused && contains_2f32(panel_rect, mouse) && !df_view_is_nil(view))
            {
              // rjf: mouse => hovered drop site
              F32 min_distance = 0;
              DropSite *active_drop_site = 0;
              for(U64 drop_site_idx = 0; drop_site_idx < drop_site_count; drop_site_idx += 1)
              {
                F32 distance = abs_f32(drop_sites[drop_site_idx].p - mouse.x);
                if(drop_site_idx == 0 || distance < min_distance)
                {
                  active_drop_site = &drop_sites[drop_site_idx];
                  min_distance = distance;
                }
              }
              
              // rjf: vis
              DF_Panel *drag_panel = df_panel_from_handle(df_g_drag_drop_payload.panel);
              if(!df_view_is_nil(view) &&
                 active_drop_site != 0 &&
                 (panel != drag_panel))
              {
                tab_bar_box->flags |= UI_BoxFlag_DrawOverlay;
                tab_bar_box->overlay_color = df_rgba_from_theme_color(DF_ThemeColor_DropSiteOverlay);
                
                if(panel->tab_view_count != 0)
                {
                  D_Bucket *bucket = d_bucket_make();
                  D_BucketScope(bucket)
                  {
                    d_rect(r2f32p(active_drop_site->p - tab_spacing/2,
                                  tab_bar_box->rect.y0,
                                  active_drop_site->p + tab_spacing/2,
                                  tab_bar_box->rect.y1),
                           v4f32(1, 1, 1, 1),
                           2.f, 0, 1.f);
                  }
                  ui_box_equip_draw_bucket(tab_bar_box, bucket);
                }
              }
              
              // rjf: drop
              DF_DragDropPayload payload = df_g_drag_drop_payload;
              if((active_drop_site != 0 && df_drag_drop(&payload)) || df_panel_from_handle(payload.panel) == panel)
              {
                DF_View *view = df_view_from_handle(payload.view);
                DF_Panel *src_panel = df_panel_from_handle(payload.panel);
                if(!df_panel_is_nil(panel) && !df_view_is_nil(view))
                {
                  DF_CmdParams params = df_cmd_params_from_window(ws);
                  params.panel = df_handle_from_panel(src_panel);
                  params.dest_panel = df_handle_from_panel(panel);
                  params.view = df_handle_from_view(view);
                  params.prev_view = df_handle_from_view(active_drop_site->prev_view);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Panel);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_DestPanel);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_View);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_PrevView);
                  df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_MoveTab));
                }
              }
            }
          }
          
          // rjf: apply tab change
          {
            panel->selected_tab_view = df_handle_from_view(next_selected_tab_view);
          }
          
          scratch_end(scratch);
        }
        
        //////////////////////////
        //- rjf: less granular panel for tabs & entities drop-site
        //
        if(df_drag_is_active() && window_is_focused && contains_2f32(panel_rect, ui_mouse()))
        {
          DF_DragDropPayload *payload = &df_g_drag_drop_payload;
          DF_View *dragged_view = df_view_from_handle(payload->view);
          B32 view_is_in_panel = 0;
          for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
          {
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
              panel_box->flags |= UI_BoxFlag_DrawOverlay;
              panel_box->overlay_color = df_rgba_from_theme_color(DF_ThemeColor_DropSiteOverlay);
            }
            
            // rjf: drop
            {
              DF_DragDropPayload payload = {0};
              if(df_drag_drop(&payload))
              {
                DF_Panel *src_panel = df_panel_from_handle(payload.panel);
                DF_View *view = df_view_from_handle(payload.view);
                DF_Entity *entity = df_entity_from_handle(payload.entity);
                
                // rjf: view drop
                if(!df_view_is_nil(view))
                {
                  DF_CmdParams params = df_cmd_params_from_window(ws);
                  params.prev_view = df_handle_from_view(panel->last_tab_view);
                  params.panel = df_handle_from_panel(src_panel);
                  params.dest_panel = df_handle_from_panel(panel);
                  params.view = df_handle_from_view(view);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_PrevView);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Panel);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_DestPanel);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_View);
                  df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_MoveTab));
                  df_panel_notify_mutation(ws, panel);
                }
                
                // rjf: entity drop
                if(!df_entity_is_nil(entity))
                {
                  DF_CmdParams params = df_cmd_params_from_window(ws);
                  params.panel = df_handle_from_panel(panel);
                  params.text_point = payload.text_point;
                  params.entity = df_handle_from_entity(entity);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Panel);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
                  df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                  df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SpawnEntityView));
                  ui_ctx_menu_open(ws->drop_completion_ctx_menu_key, ui_key_zero(), sub_2f32(ui_mouse(), v2f32(2, 2)));
                  ws->drop_completion_entity = df_handle_from_entity(entity);
                  ws->drop_completion_panel = df_handle_from_panel(panel);
                }
              }
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: animate panel pcts
    //
    {
      F32 rate = 1 - pow_f32(2, (-50.f * df_dt()));
      for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
      {
        if(abs_f32(panel->off_pct_of_parent.x) > 0.005f ||
           abs_f32(panel->off_pct_of_parent.y) > 0.005f ||
           abs_f32(panel->size_pct_of_parent_target.x - panel->size_pct_of_parent.x) > 0.005f ||
           abs_f32(panel->size_pct_of_parent_target.y - panel->size_pct_of_parent.y) > 0.005f)
        {
          df_gfx_request_frame();
        }
        panel->off_pct_of_parent.x += (-panel->off_pct_of_parent.x) * rate;
        panel->off_pct_of_parent.y += (-panel->off_pct_of_parent.y) * rate;
        panel->size_pct_of_parent.x += (panel->size_pct_of_parent_target.x - panel->size_pct_of_parent.x) * rate;
        panel->size_pct_of_parent.y += (panel->size_pct_of_parent_target.y - panel->size_pct_of_parent.y) * rate;
      }
    }
    
    ////////////////////////////
    //- rjf: animate views
    //
    {
      F32 rate = 1 - pow_f32(2, (-10.f * df_dt()));
      F32 fast_rate = 1 - pow_f32(2, (-40.f * df_dt()));
      for(DF_Panel *panel = ws->root_panel;
          !df_panel_is_nil(panel);
          panel = df_panel_rec_df_pre(panel).next)
      {
        U64 list_firsts_count = 1 + !!(panel == ws->root_panel);
        DF_View *list_firsts[2] = {panel->first_tab_view, ws->query_view_stack_top};
        for(U64 idx = 0; idx < list_firsts_count; idx += 1)
        {
          DF_View *list_first = list_firsts[idx];
          for(DF_View *view = list_first; !df_view_is_nil(view); view = view->next)
          {
            if(abs_f32(view->loading_t_target - view->loading_t) > 0.01f ||
               abs_f32(0 - view->flash_t) > 0.01f ||
               abs_f32(view->scroll_pos.x.off) > 0.01f ||
               abs_f32(view->scroll_pos.y.off) > 0.01f ||
               abs_f32(view->is_filtering_t - (F32)!!view->is_filtering))
            {
              df_gfx_request_frame();
            }
            if(view->loading_t_target != 0 && view == df_view_from_handle(panel->selected_tab_view))
            {
              df_gfx_request_frame();
            }
            view->loading_t += (view->loading_t_target - view->loading_t) * rate;
            view->flash_t += (0 - view->flash_t) * rate;
            view->is_filtering_t += ((F32)!!view->is_filtering - view->is_filtering_t) * fast_rate;
            view->scroll_pos.x.off -= view->scroll_pos.x.off*fast_rate;
            view->scroll_pos.y.off -= view->scroll_pos.y.off*fast_rate;
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
            if(view == df_view_from_handle(panel->selected_tab_view))
            {
              view->loading_t_target = 0;
            }
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: drag/drop cancelling
    //
    if(df_drag_is_active() && os_key_press(events, ws->os, 0, OS_Key_Esc))
    {
      df_drag_kill();
      ui_kill_action();
    }
    
    ////////////////////////////
    //- rjf: font size changing
    //
    for(OS_Event *event = events->first; event != 0; event = event->next)
    {
      if(os_handle_match(event->window, ws->os) && event->kind == OS_EventKind_Scroll && event->flags & OS_EventFlag_Ctrl)
      {
        os_eat_event(ui_events(), event);
        if(event->delta.y < 0)
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_IncUIFontScale));
        }
        else if(event->delta.y > 0)
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_DecUIFontScale));
        }
      }
    }
  }
  ui_end_build();
  
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
  //- rjf: attach autocomp box to root
  //
  if(!ui_box_is_nil(autocomp_box))
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
  
  //////////////////////////////
  //- rjf: hover eval cancelling
  //
  if(ws->hover_eval_string.size != 0 && os_key_press(events, ws->os, 0, OS_Key_Esc))
  {
    MemoryZeroStruct(&ws->hover_eval_string);
    arena_clear(ws->hover_eval_arena);
    df_gfx_request_frame();
  }
  
  //////////////////////////////
  //- rjf: animate
  //
  if(ui_animating_from_state(ws->ui))
  {
    df_gfx_request_frame();
  }
  
  //////////////////////////////
  //- rjf: draw UI
  //
  ws->draw_bucket = d_bucket_make();
  D_BucketScope(ws->draw_bucket)
    ProfScope("draw UI")
  {
    Temp scratch = scratch_begin(&arena, 1);
    
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
      Vec4F32 bg_color = df_rgba_from_theme_color(DF_ThemeColor_PlainBackground);
      d_rect(os_client_rect_from_window(ws->os), bg_color, 0, 0, 0);
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
        d_push_transparency(box->transparency);
      }
      
      // rjf: push squish
      if(box->squish != 0)
      {
        Vec2F32 box_dim = dim_2f32(box->rect);
        Mat3x3F32 box2origin_xform = make_translate_3x3f32(v2f32(-box->rect.x0 - box_dim.x/8, -box->rect.y0));
        Mat3x3F32 scale_xform = make_scale_3x3f32(v2f32(1-box->squish, 1-box->squish));
        Mat3x3F32 origin2box_xform = make_translate_3x3f32(v2f32(box->rect.x0 + box_dim.x/8, box->rect.y0));
        Mat3x3F32 xform = mul_3x3f32(origin2box_xform, mul_3x3f32(scale_xform, box2origin_xform));
        d_push_xform2d(xform);
      }
      
      // rjf: draw drop shadow
      if(box->flags & UI_BoxFlag_DrawDropShadow)
      {
        Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
        Vec4F32 drop_shadow_color = df_rgba_from_theme_color(DF_ThemeColor_DropShadow);
        d_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
      }
      
      // rjf: blur background
      if(box->flags & UI_BoxFlag_DrawBackgroundBlur)
      {
        R_PassParams_Blur *params = d_blur(box->rect, box->blur_size*(1-box->transparency), 0);
        MemoryCopyArray(params->corner_radii, box->corner_radii);
      }
      
      // rjf: draw background
      if(box->flags & UI_BoxFlag_DrawBackground)
      {
        // rjf: main rectangle
        {
          R_Rect2DInst *inst = d_rect(pad_2f32(box->rect, 1.5f), box->background_color, 0, 0, 1.f);
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
            R_Rect2DInst *inst = d_rect(box->rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = v4f32(1.f, 0.9f, 0.7f, 0.1f*t);
            inst->colors[Corner_01] = v4f32(1.f, 0.9f, 0.7f, 0.1f*t);
            inst->colors[Corner_10] = v4f32(1.f, 0.9f, 0.7f, 0.1f*t);
            inst->colors[Corner_11] = v4f32(1.f, 0.9f, 0.7f, 0.1f*t);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: slight emboss fadeoff
          {
            Rng2F32 rect = r2f32p(box->rect.x0,
                                  box->rect.y0,
                                  box->rect.x1,
                                  box->rect.y1);
            R_Rect2DInst *inst = d_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
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
          Vec2F32 shadow_size =
          {
            (box->rect.x1 - box->rect.x0)*0.60f*box->active_t,
            (box->rect.y1 - box->rect.y0)*0.60f*box->active_t,
          };
          shadow_size.x = Clamp(0, shadow_size.x, box->font_size*2.f);
          shadow_size.y = Clamp(0, shadow_size.y, box->font_size*2.f);
          
          // rjf: top -> bottom dark effect
          {
            R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0.f, 0.f, 0.f, 0.8f*box->active_t);
            inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: bottom -> top light effect
          {
            R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y1 - shadow_size.y, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
            inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.4f, 0.4f, 0.4f, 0.4f*box->active_t);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: left -> right dark effect
          {
            R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x0 + shadow_size.x, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.f);
            inst->colors[Corner_00] = v4f32(0.f, 0.f, 0.f, 0.8f*box->active_t);
            inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.4f*box->active_t);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: right -> left dark effect
          {
            R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x1 - shadow_size.x, box->rect.y0, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.f);
            inst->colors[Corner_10] = v4f32(0.f, 0.f, 0.f, 0.8f*box->active_t);
            inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.4f*box->active_t);
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
          d_rect(r2f32p(text_position.x-4, text_position.y-4, text_position.x+4, text_position.y+4),
                 v4f32(1, 0, 1, 1), 1, 0, 1);
        }
        F32 max_x = 100000.f;
        F_Run ellipses_run = {0};
        if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
        {
          max_x = (box->rect.x1-text_position.x);
          ellipses_run = f_push_run_from_string(scratch.arena, box->font, box->font_size, 0, str8_lit("..."));
        }
        d_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
        if(box->flags & UI_BoxFlag_HasFuzzyMatchRanges)
        {
          Vec4F32 match_color = df_rgba_from_theme_color(DF_ThemeColor_Cursor);
          match_color.w *= 0.25f;
          d_truncated_fancy_run_fuzzy_matches(text_position, &box->display_string_runs, max_x, &box->fuzzy_match_ranges, match_color);
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
          d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), color, 2, 0, 1);
          d_rect(box->rect, color, 2, 2, 1);
        }
        if(box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive))
        {
          if(box->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
          {
            d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(1, 0, 0, 0.2f), 2, 0, 1);
          }
          else
          {
            d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(0, 1, 0, 0.2f), 2, 0, 1);
          }
        }
      }
      
      // rjf: push clip
      if(box->flags & UI_BoxFlag_Clip)
      {
        Rng2F32 top_clip = d_top_clip();
        Rng2F32 new_clip = pad_2f32(box->rect, -1);
        if(top_clip.x1 != 0 || top_clip.y1 != 0)
        {
          new_clip = intersect_2f32(new_clip, top_clip);
        }
        d_push_clip(new_clip);
      }
      
      // rjf: custom draw list
      if(box->flags & UI_BoxFlag_DrawBucket)
      {
        Mat3x3F32 xform = make_translate_3x3f32(box->position_delta);
        D_XForm2DScope(xform)
        {
          d_sub_bucket(box->draw_bucket);
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
            d_pop_clip();
          }
          
          // rjf: draw border
          if(b->flags & UI_BoxFlag_DrawBorder)
          {
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), b->border_color, 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
            
            // rjf: hover effect
            if(b->flags & UI_BoxFlag_DrawHotEffects)
            {
              R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1.f), v4f32(1, 1, 1, 0.5f*b->hot_t), 0, 1.f, 1.f);
              MemoryCopyArray(inst->corner_radii, b->corner_radii);
            }
          }
          
          // rjf: draw sides
          {
            Rng2F32 r = b->rect;
            F32 half_thickness = 1.f;
            F32 softness = 0.5f;
            if(b->flags & UI_BoxFlag_DrawSideTop)
            {
              d_rect(r2f32p(r.x0, r.y0-half_thickness, r.x1, r.y0+half_thickness), b->border_color, 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideBottom)
            {
              d_rect(r2f32p(r.x0, r.y1-half_thickness, r.x1, r.y1+half_thickness), b->border_color, 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideLeft)
            {
              d_rect(r2f32p(r.x0-half_thickness, r.y0, r.x0+half_thickness, r.y1), b->border_color, 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideRight)
            {
              d_rect(r2f32p(r.x1-half_thickness, r.y0, r.x1+half_thickness, r.y1), b->border_color, 0, 0, softness);
            }
          }
          
          // rjf: draw focus hot vis
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusViz) && b->focus_hot_t > 0.01f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
            F32 size_factor = 1 - Clamp(0, dim_2f32(b->rect).y / 100.f, 1);
            if(b->flags & UI_BoxFlag_RequireFocusBackground)
            {
              color.w *= 0.2f + b->focus_hot_t * 0.3f * size_factor;
            }
            else
            {
              color.w *= b->focus_hot_t * 0.5f * size_factor;
            }
            R_Rect2DInst *inst = d_rect(pad_2f32(r2f32p(b->rect.x0,
                                                        b->rect.y0,
                                                        b->rect.x0 + (b->rect.x1 - b->rect.x0) * b->focus_hot_t,
                                                        b->rect.y1),
                                                 6.f),
                                        color, 4.f, 0, 4.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(color.x, color.y, color.z, color.w/3.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw focus active vis
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusViz) && b->focus_active_t > 0.01f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
            color.w *= b->focus_active_t;
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1.f), color, 0, 2.f, 1.f);
            inst->colors[Corner_10] = inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(color.x, color.y, color.z, color.w/3.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw focus active disabled vis
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusViz) && b->focus_active_disabled_t > 0.01f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
            color.w *= b->focus_active_disabled_t;
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1.f), color, 0, 2.f, 1.f);
            inst->colors[Corner_10] = inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(color.x, color.y, color.z, color.w/3.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw overlay
          if(b->flags & UI_BoxFlag_DrawOverlay && b->overlay_color.w > 0.05f)
          {
            R_Rect2DInst *inst = d_rect(b->rect, b->overlay_color, 0, 0, 1);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: disabled overlay
          if(b->disabled_t >= 0.005f)
          {
            // rhp: disabled overlay color blends from plain background and inactive panel overlay
            Vec4F32 bg = df_rgba_from_theme_color(DF_ThemeColor_PlainBackground);
            Vec4F32 ov = df_rgba_from_theme_color(DF_ThemeColor_InactivePanelOverlay);
            Vec4F32 color = {};
            color.x = bg.x * bg.w + ov.x * ov.w * (1.0f - bg.w);
            color.y = bg.y * bg.w + ov.y * ov.w * (1.0f - bg.w);
            color.z = bg.z * bg.w + ov.z * ov.w * (1.0f - bg.w);
            color.w = bg.w + ov.w * (1.0f - bg.w);
            if (1.0f - color.w < 0.01f)
            {
              color.x *= color.w;
              color.y *= color.w;
              color.z *= color.w;
            }
            color.w = 0.60f * b->disabled_t;
            R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 1);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: pop squish
          if(b->squish != 0)
          {
            d_pop_xform2d();
          }
          
          // rjf: pop transparency
          if(b->transparency != 0)
          {
            d_pop_transparency();
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
        d_rect(rect, v4f32(rgb.x, rgb.y, rgb.z, 0.3f), 0, 0, 0);
      }
    }
    
    //- rjf: draw border/overlay color to signify error
    if(ws->error_t > 0.01f)
    {
      Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
      color.w *= ws->error_t;
      Rng2F32 rect = os_client_rect_from_window(ws->os);
      d_rect(pad_2f32(rect, 24.f), color, 0, 16.f, 12.f);
      d_rect(rect, v4f32(color.x, color.y, color.z, color.w*0.05f), 0, 0, 0);
    }
    
    //- rjf: scratch debug mouse drawing
    if(DEV_scratch_mouse_draw)
    {
      Vec2F32 p = add_2f32(os_mouse_from_window(ws->os), v2f32(30, 0));
      d_rect(os_client_rect_from_window(ws->os), v4f32(0, 0, 0, 0.4f), 0, 0, 0);
      D_FancyStringList strs = {0};
      D_FancyString str1 = {df_font_from_slot(DF_FontSlot_Main), str8_lit("T"), v4f32(1, 1, 1, 1), 16.f, 4.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str1);
      D_FancyString str2 = {df_font_from_slot(DF_FontSlot_Main), str8_lit("his is a test of some "), v4f32(1, 0.5f, 0.5f, 1), 14.f, 0.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str2);
      D_FancyString str3 = {df_font_from_slot(DF_FontSlot_Code), str8_lit("very fancy text!"), v4f32(1, 0.8f, 0.4f, 1), 18.f, 4.f, 4.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str3);
      D_FancyRunList runs = d_fancy_run_list_from_fancy_string_list(scratch.arena, &strs);
      F_Run trailer_run = f_push_run_from_string(scratch.arena, df_font_from_slot(DF_FontSlot_Main), 16.f, 0, str8_lit("..."));
      F32 limit = 500.f + sin_f32(df_time_in_seconds()/10.f)*200.f;
      d_truncated_fancy_run_list(p, &runs, limit, trailer_run);
      d_rect(r2f32p(p.x+limit, 0, p.x+limit+2.f, 1000), v4f32(1, 0, 0, 1), 0, 0, 0);
      df_gfx_request_frame();
    }
    
    scratch_end(scratch);
  }
  
  //////////////////////////////
  //- rjf: show window after first frame
  //
  {
    if(ws->frames_alive == 0)
    {
      os_window_first_paint(ws->os);
    }
    ws->frames_alive += 1;
  }
  
  ProfEnd();
}

#if COMPILER_MSVC && !BUILD_DEBUG
#pragma optimize("", on)
#endif

////////////////////////////////
//~ rjf: Eval Viz

internal String8
df_eval_escaped_from_raw_string(Arena *arena, String8 raw)
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

internal String8List
df_single_line_eval_value_strings_from_eval(Arena *arena, DF_EvalVizStringFlags flags, TG_Graph *graph, RDI_Parsed *rdi, DF_CtrlCtx *ctrl_ctx, U32 default_radix, F_Tag font, F32 font_size, F32 max_size, S32 depth, DF_Eval eval, TG_Member *opt_member, DF_CfgTable *cfg_table)
{
  ProfBeginFunction();
  String8List list = {0};
  F32 space_taken = 0;
  
  //- rjf: type path -> empty
  if(eval.mode == EVAL_EvalMode_NULL && !tg_key_match(tg_key_zero(), eval.type_key))
  {
    if(opt_member != 0)
    {
      U64 member_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, opt_member->type_key);
      str8_list_pushf(arena, &list, "member (%I64u offset, %I64u byte%s)", opt_member->off, member_byte_size, member_byte_size == 1 ? "s" : "");
    }
    else
    {
      String8 basic_type_kind_string = tg_kind_basic_string_table[tg_kind_from_key(eval.type_key)];
      U64 byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, eval.type_key);
      str8_list_pushf(arena, &list, "%S (%I64u byte%s)", basic_type_kind_string, byte_size, byte_size == 1 ? "s" : "");
    }
  }
  
  //- rjf: non-type path: descend recursively & produce single-line value strings
  else if(max_size > 0)
  {
    TG_Kind eval_type_kind = tg_kind_from_key(tg_unwrapped_from_graph_rdi_key(graph, rdi, eval.type_key));
    U32 radix = default_radix;
    DF_CfgVal *dec_cfg = df_cfg_val_from_string(cfg_table, str8_lit("dec"));
    DF_CfgVal *hex_cfg = df_cfg_val_from_string(cfg_table, str8_lit("hex"));
    DF_CfgVal *bin_cfg = df_cfg_val_from_string(cfg_table, str8_lit("bin"));
    DF_CfgVal *oct_cfg = df_cfg_val_from_string(cfg_table, str8_lit("oct"));
    U64 best_insertion_stamp = Max(dec_cfg->insertion_stamp, Max(hex_cfg->insertion_stamp, Max(bin_cfg->insertion_stamp, oct_cfg->insertion_stamp)));
    if(dec_cfg != &df_g_nil_cfg_val && dec_cfg->insertion_stamp == best_insertion_stamp) { radix = 10; }
    if(hex_cfg != &df_g_nil_cfg_val && hex_cfg->insertion_stamp == best_insertion_stamp) { radix = 16; }
    if(bin_cfg != &df_g_nil_cfg_val && bin_cfg->insertion_stamp == best_insertion_stamp) { radix = 2; }
    if(oct_cfg != &df_g_nil_cfg_val && oct_cfg->insertion_stamp == best_insertion_stamp) { radix = 8; }
    switch(eval_type_kind)
    {
      //- rjf: default - leaf cases
      default:
      {
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, eval);
        String8 string = df_string_from_simple_typed_eval(arena, graph, rdi, flags, radix, value_eval);
        space_taken += f_dim_from_tag_size_string(font, font_size, string).x;
        str8_list_push(arena, &list, string);
      }break;
      
      //- rjf: pointers
      case TG_Kind_Function:
      case TG_Kind_Ptr:
      case TG_Kind_LRef:
      case TG_Kind_RRef:
      {
        // rjf: determine ptr value omission
        DF_CfgVal *noaddr_cfg = df_cfg_val_from_string(cfg_table, str8_lit("no_addr"));
        B32 no_addr = (noaddr_cfg != &df_g_nil_cfg_val) && (flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules);
        
        // rjf: determine presence of array view rule -> omit c-string fastpath
        DF_CfgVal *array_cfg = df_cfg_val_from_string(cfg_table, str8_lit("array"));
        B32 has_array = (array_cfg != &df_g_nil_cfg_val);
        
        // rjf: get ptr value
        DF_Eval value_eval = df_value_mode_eval_from_eval(graph, rdi, ctrl_ctx, eval);
        
        // rjf: get pointed-at info
        TG_Kind type_kind = tg_kind_from_key(eval.type_key);
        TG_Key direct_type_key = tg_ptee_from_graph_rdi_key(graph, rdi, eval.type_key);
        TG_Kind direct_type_kind = tg_kind_from_key(direct_type_key);
        B32 direct_type_has_content = (direct_type_kind != TG_Kind_Null && direct_type_kind != TG_Kind_Void && value_eval.imm_u64 != 0);
        B32 direct_type_is_string = (direct_type_kind != TG_Kind_Null && value_eval.imm_u64 != 0 &&
                                     ((TG_Kind_Char8 <= direct_type_kind && direct_type_kind <= TG_Kind_UChar32) ||
                                      direct_type_kind == TG_Kind_Char8 ||
                                      direct_type_kind == TG_Kind_UChar8));
        DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
        DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
        String8 symbol_name = df_symbol_name_from_process_vaddr(arena, process, value_eval.imm_u64);
        
        // rjf: display ptr value
        B32 did_ptr_value = 0;
        if(!no_addr || (direct_type_has_content == 0 && direct_type_is_string == 0))
        {
          did_ptr_value = 1;
          String8 string = df_string_from_simple_typed_eval(arena, graph, rdi, flags, radix, value_eval);
          space_taken += f_dim_from_tag_size_string(font, font_size, string).x;
          str8_list_push(arena, &list, string);
        }
        
        // rjf: arrow
        if(did_ptr_value && (direct_type_has_content || symbol_name.size != 0) && (flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules))
        {
          String8 arrow = str8_lit(" -> ");
          str8_list_push(arena, &list, arrow);
          space_taken += f_dim_from_tag_size_string(font, font_size, arrow).x;
        }
        
        // rjf: special-case: strings
        if(!has_array && direct_type_is_string && (flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules) && eval.mode == EVAL_EvalMode_Addr)
        {
          U64 string_memory_addr = value_eval.imm_u64;
          U64 element_size = tg_byte_size_from_graph_rdi_key(graph, rdi, eval.type_key);
          CTRL_ProcessMemorySlice text_slice = ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(arena, process->ctrl_machine_id, process->ctrl_handle, eval.offset, 256, element_size, 0);
          String8 raw_text = {0};
          switch(element_size)
          {
            default:{raw_text = text_slice.data;}break;
            case 2: {raw_text = str8_from_16(arena, str16((U16 *)text_slice.data.str, text_slice.data.size/sizeof(U16)));}break;
            case 4: {raw_text = str8_from_32(arena, str32((U32 *)text_slice.data.str, text_slice.data.size/sizeof(U32)));}break;
          }
          String8 text = df_eval_escaped_from_raw_string(arena, raw_text);
          space_taken += f_dim_from_tag_size_string(font, font_size, text).x;
          space_taken += 2*f_dim_from_tag_size_string(font, font_size, str8_lit("\"")).x;
          str8_list_push(arena, &list, str8_lit("\""));
          str8_list_push(arena, &list, text);
          str8_list_push(arena, &list, str8_lit("\""));
        }
        
        // rjf: special-case: symbols
        else if((flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules) && symbol_name.size != 0 &&
                (type_kind == TG_Kind_Function ||
                 direct_type_kind == TG_Kind_Function ||
                 direct_type_kind == TG_Kind_Void))
        {
          str8_list_push(arena, &list, symbol_name);
          space_taken += f_dim_from_tag_size_string(font, font_size, symbol_name).x;
        }
        
        // rjf: descend to pointed-at thing
        else if(direct_type_has_content && (flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules))
        {
          if(depth < 3)
          {
            DF_Eval pted_eval = zero_struct;
            pted_eval.type_key = direct_type_key;
            pted_eval.mode     = EVAL_EvalMode_Addr;
            pted_eval.offset   = value_eval.imm_u64;
            String8List pted_strs = df_single_line_eval_value_strings_from_eval(arena, flags, graph, rdi, ctrl_ctx, default_radix, font, font_size, max_size-space_taken, depth+1, pted_eval, opt_member, cfg_table);
            if(pted_strs.total_size == 0)
            {
              String8 unknown = str8_lit("???");
              str8_list_push(arena, &list, unknown);
              space_taken += f_dim_from_tag_size_string(font, font_size, unknown).x;
            }
            else
            {
              space_taken += f_dim_from_tag_size_string_list(font, font_size, pted_strs).x;
              str8_list_concat_in_place(&list, &pted_strs);
            }
          }
          else
          {
            String8 ellipses = str8_lit("...");
            str8_list_push(arena, &list, ellipses);
            space_taken += f_dim_from_tag_size_string(font, font_size, ellipses).x;
          }
        }
      }break;
      
      //- rjf: arrays
      case TG_Kind_Array:
      {
        Temp scratch = scratch_begin(&arena, 1);
        TG_Type *eval_type = tg_type_from_graph_rdi_key(scratch.arena, graph, rdi, eval.type_key);
        TG_Key direct_type_key = eval_type->direct_type_key;
        TG_Kind direct_type_kind = tg_kind_from_key(direct_type_key);
        U64 array_count = eval_type->count;
        
        // rjf: determine presence of array view rule -> omit c-string fastpath
        DF_CfgVal *array_cfg = df_cfg_val_from_string(cfg_table, str8_lit("array"));
        B32 has_array = (array_cfg != &df_g_nil_cfg_val);
        
        // rjf: get pointed-at type
        B32 direct_type_is_string = (direct_type_kind != TG_Kind_Null &&
                                     ((TG_Kind_Char8 <= direct_type_kind && direct_type_kind <= TG_Kind_UChar32) ||
                                      direct_type_kind == TG_Kind_S8 ||
                                      direct_type_kind == TG_Kind_U8));
        B32 special_case = 0;
        
        // rjf: special-case: strings
        if(!has_array && direct_type_is_string && eval.mode == EVAL_EvalMode_Addr)
        {
          special_case = 1;
          DF_Entity *thread = df_entity_from_handle(ctrl_ctx->thread);
          DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
          U64 element_size = tg_byte_size_from_graph_rdi_key(graph, rdi, eval_type->direct_type_key);
          CTRL_ProcessMemorySlice text_slice = ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(arena, process->ctrl_machine_id, process->ctrl_handle, eval.offset, 256, element_size, 0);
          String8 raw_text = {0};
          switch(element_size)
          {
            default:{raw_text = text_slice.data;}break;
            case 2: {raw_text = str8_from_16(arena, str16((U16 *)text_slice.data.str, text_slice.data.size/sizeof(U16)));}break;
            case 4: {raw_text = str8_from_32(arena, str32((U32 *)text_slice.data.str, text_slice.data.size/sizeof(U32)));}break;
          }
          String8 text = df_eval_escaped_from_raw_string(arena, raw_text);
          space_taken += f_dim_from_tag_size_string(font, font_size, text).x;
          space_taken += 2*f_dim_from_tag_size_string(font, font_size, str8_lit("\"")).x;
          str8_list_push(arena, &list, str8_lit("\""));
          str8_list_push(arena, &list, text);
          str8_list_push(arena, &list, str8_lit("\""));
        }
        
        // rjf: open brace
        if(!special_case)
        {
          String8 brace = str8_lit("[");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, brace).x;
        }
        
        // rjf: content
        if(!special_case)
        {
          if(depth < 3)
          {
            U64 direct_type_byte_size = tg_byte_size_from_graph_rdi_key(graph, rdi, direct_type_key);
            for(U64 idx = 0; idx < array_count && max_size > space_taken; idx += 1)
            {
              DF_Eval element_eval = zero_struct;
              element_eval.type_key = direct_type_key;
              element_eval.mode     = eval.mode;
              element_eval.offset   = eval.offset + direct_type_byte_size*idx;
              MemoryCopyArray(element_eval.imm_u128, eval.imm_u128);
              String8List element_strs = df_single_line_eval_value_strings_from_eval(arena, flags, graph, rdi, ctrl_ctx, default_radix, font, font_size, max_size-space_taken, depth+1, element_eval, opt_member, cfg_table);
              space_taken += f_dim_from_tag_size_string_list(font, font_size, element_strs).x;
              str8_list_concat_in_place(&list, &element_strs);
              if(idx+1 < array_count)
              {
                String8 comma = str8_lit(", ");
                space_taken += f_dim_from_tag_size_string(font, font_size, comma).x;
                str8_list_push(arena, &list, comma);
              }
            }
          }
          else
          {
            String8 ellipses = str8_lit("...");
            str8_list_push(arena, &list, ellipses);
            space_taken += f_dim_from_tag_size_string(font, font_size, ellipses).x;
          }
        }
        
        // rjf: close brace
        if(!special_case)
        {
          String8 brace = str8_lit("]");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, brace).x;
        }
        scratch_end(scratch);
      }break;
      
      //- rjf: structs
      case TG_Kind_Struct:
      case TG_Kind_Union:
      case TG_Kind_Class:
      case TG_Kind_IncompleteStruct:
      case TG_Kind_IncompleteUnion:
      case TG_Kind_IncompleteClass:
      {
        // rjf: open brace
        {
          String8 brace = str8_lit("{");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, brace).x;
        }
        
        // rjf: content
        if(depth < 4)
        {
          Temp scratch = scratch_begin(&arena, 1);
          TG_MemberArray data_members = tg_data_members_from_graph_rdi_key(scratch.arena, graph, rdi, eval.type_key);
          TG_MemberArray filtered_data_members = df_filtered_data_members_from_members_cfg_table(scratch.arena, data_members, cfg_table);
          for(U64 member_idx = 0; member_idx < filtered_data_members.count && max_size > space_taken; member_idx += 1)
          {
            TG_Member *mem = &filtered_data_members.v[member_idx];
            DF_Eval member_eval = zero_struct;
            member_eval.type_key = mem->type_key;
            member_eval.mode = eval.mode;
            member_eval.offset = eval.offset + mem->off;
            MemoryCopyArray(member_eval.imm_u128, eval.imm_u128);
            String8List member_strs = df_single_line_eval_value_strings_from_eval(arena, flags, graph, rdi, ctrl_ctx, default_radix, font, font_size, max_size-space_taken, depth+1, member_eval, opt_member, cfg_table);
            space_taken += f_dim_from_tag_size_string_list(font, font_size, member_strs).x;
            str8_list_concat_in_place(&list, &member_strs);
            if(member_idx+1 < filtered_data_members.count)
            {
              String8 comma = str8_lit(", ");
              space_taken += f_dim_from_tag_size_string(font, font_size, comma).x;
              str8_list_push(arena, &list, comma);
            }
          }
          scratch_end(scratch);
        }
        else
        {
          String8 ellipses = str8_lit("...");
          str8_list_push(arena, &list, ellipses);
          space_taken += f_dim_from_tag_size_string(font, font_size, ellipses).x;
        }
        
        // rjf: close brace
        {
          String8 brace = str8_lit("}");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, brace).x;
        }
        
      }break;
    }
  }
  ProfEnd();
  return list;
}

internal DF_EvalVizWindowedRowList
df_eval_viz_windowed_row_list_from_viz_block_list(Arena *arena, DBGI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_EvalView *eval_view, U32 default_radix, F_Tag font, F32 font_size, Rng1S64 visible_range, DF_EvalVizBlockList *blocks)
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
    TG_Kind block_type_kind = tg_kind_from_key(block->eval.type_key);
    
    //////////////////////////////
    //- rjf: determine if view rules force expandability
    //
    B32 expandability_required = 0;
    for(DF_CfgVal *val = block->cfg_table.first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
    {
      DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(val->string);
      if(spec->info.flags & DF_CoreViewRuleSpecInfoFlag_Expandable)
      {
        expandability_required = 1;
        break;
      }
    }
    
    //////////////////////////////
    //- rjf: grab default row ui view rule to use for this block
    //
    DF_GfxViewRuleSpec *value_ui_rule_spec = &df_g_nil_gfx_view_rule_spec;
    DF_CfgNode *value_ui_rule_node= &df_g_nil_cfg_node;
    for(DF_CfgVal *val = block->cfg_table.first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
    {
      DF_GfxViewRuleSpec *spec = df_gfx_view_rule_spec_from_string(val->string);
      if(spec->info.flags & DF_GfxViewRuleSpecInfoFlag_RowUI)
      {
        value_ui_rule_spec = spec;
        value_ui_rule_node = val->last;
        break;
      }
    }
    
    //////////////////////////////
    //- rjf: grab expand ui view rule to use for this block's rows
    //
    DF_GfxViewRuleSpec *expand_ui_rule_spec = &df_g_nil_gfx_view_rule_spec;
    DF_CfgNode *expand_ui_rule_node = &df_g_nil_cfg_node;
    for(DF_CfgVal *val = block->cfg_table.first_val; val != 0 && val != &df_g_nil_cfg_val; val = val->linear_next)
    {
      DF_GfxViewRuleSpec *spec = df_gfx_view_rule_spec_from_string(val->string);
      if(spec->info.flags & DF_GfxViewRuleSpecInfoFlag_BlockUI)
      {
        expand_ui_rule_spec = spec;
        expand_ui_rule_node = val->last;
        break;
      }
    }
    
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
    switch(block->kind)
    {
      default:{}break;
      
      //////////////////////////////
      //- rjf: null -> empty row
      //
      case DF_EvalVizBlockKind_Null:
      if(visible_idx_range.max > visible_idx_range.min)
      {
        DF_Eval eval = zero_struct;
        df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, block->key, eval);
      }break;
      
      //////////////////////////////
      //- rjf: root -> just a single row. possibly expandable.
      //
      case DF_EvalVizBlockKind_Root:
      if(visible_idx_range.max > visible_idx_range.min)
      {
        String8List display_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, block->eval, block->member, &block->cfg_table);
        String8List edit_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, 0, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, block->eval, block->member, &block->cfg_table);
        DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, block->key, block->eval);
        row->expr                = block->string;
        row->display_value       = str8_list_join(arena, &display_strings, 0);
        row->edit_value          = str8_list_join(arena, &edit_strings, 0);
        row->value_ui_rule_node  = value_ui_rule_node;
        row->value_ui_rule_spec  = value_ui_rule_spec;
        row->expand_ui_rule_node = expand_ui_rule_node;
        row->expand_ui_rule_spec = expand_ui_rule_spec;
        if(block->member && block->member->kind == TG_MemberKind_Padding)
        {
          row->flags |= DF_EvalVizRowFlag_ExprIsSpecial;
        }
        if(expandability_required)
        {
          row->flags |= DF_EvalVizRowFlag_CanExpand;
        }
      }break;
      
      //////////////////////////////
      //- rjf: members -> produce rows for the visible range of members.
      //
      case DF_EvalVizBlockKind_Members:
      if(block_type_kind != TG_Kind_Null)
      {
        TG_MemberArray data_members = tg_data_members_from_graph_rdi_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, block->eval.type_key);
        TG_MemberArray filtered_data_members = df_filtered_data_members_from_members_cfg_table(scratch.arena, data_members, &block->cfg_table);
        for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max && idx < filtered_data_members.count; idx += 1)
        {
          TG_Member *member = &filtered_data_members.v[idx];
          
          // rjf: get keys for this row
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), idx+1);
          
          // rjf: get member eval
          DF_Eval member_eval = zero_struct;
          {
            member_eval.type_key = member->type_key;
            member_eval.mode     = block->eval.mode;
            member_eval.offset   = block->eval.offset + member->off;
            MemoryCopyArray(member_eval.imm_u128, block->eval.imm_u128);
          }
          
          // rjf: get view rules
          String8 view_rule_string = df_eval_view_rule_from_key(eval_view, key);
          DF_CfgTable view_rule_table = df_cfg_table_from_inheritance(scratch.arena, &block->cfg_table);
          df_cfg_table_push_unparsed_string(scratch.arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
          
          // rjf: apply view rules to eval
          {
            member_eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, member_eval);
            member_eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, macro_map, member_eval, &view_rule_table);
          }
          
          // rjf: build & push row
          String8List display_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, member_eval, member, &view_rule_table);
          String8List edit_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, 0, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, member_eval, member, &view_rule_table);
          DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, key, member_eval);
          if(member->kind == TG_MemberKind_Padding)
          {
            row->flags |= DF_EvalVizRowFlag_ExprIsSpecial;
          }
          row->expr = push_str8_copy(arena, member->name);
          row->display_value       = str8_list_join(arena, &display_strings, 0);
          row->edit_value          = str8_list_join(arena, &edit_strings, 0);
          row->value_ui_rule_node  = value_ui_rule_node;
          row->value_ui_rule_spec  = value_ui_rule_spec;
          row->expand_ui_rule_node = expand_ui_rule_node;
          row->expand_ui_rule_spec = expand_ui_rule_spec;
          row->inherited_type_key_chain = tg_key_list_copy(arena, &member->inheritance_key_chain);
          if(expandability_required)
          {
            row->flags |= DF_EvalVizRowFlag_CanExpand;
          }
        }
      }break;
      
      //////////////////////////////
      //- rjf: enum members -> produce rows for the visible range of enum members.
      //
      case DF_EvalVizBlockKind_EnumMembers:
      if(block_type_kind == TG_Kind_Enum)
      {
        TG_Type *type = tg_type_from_graph_rdi_key(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, block->eval.type_key);
        for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max && idx < type->count; idx += 1)
        {
          TG_EnumVal *enum_val = &type->enum_vals[idx];
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), idx+1);
          
          // rjf: produce eval for this enum member
          DF_Eval eval = zero_struct;
          {
            eval.type_key = block->eval.type_key;
            eval.mode     = EVAL_EvalMode_Value;
            eval.imm_u64  = enum_val->val;
          }
          
          // rjf: get view rules
          String8 view_rule_string = df_eval_view_rule_from_key(eval_view, key);
          DF_CfgTable view_rule_table = df_cfg_table_from_inheritance(scratch.arena, &block->cfg_table);
          df_cfg_table_push_unparsed_string(scratch.arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
          
          // rjf: apply view rules to eval
          {
            eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
            eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval, &view_rule_table);
          }
          
          // rjf: build & push row
          String8List display_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, eval, 0, &view_rule_table);
          String8List edit_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, 0, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, eval, 0, &view_rule_table);
          DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, key, eval);
          row->expr                = push_str8_copy(arena, enum_val->name);
          row->display_value       = str8_list_join(arena, &display_strings, 0);
          row->edit_value          = str8_list_join(arena, &edit_strings, 0);
          row->value_ui_rule_node  = value_ui_rule_node;
          row->value_ui_rule_spec  = value_ui_rule_spec;
          row->expand_ui_rule_node = expand_ui_rule_node;
          row->expand_ui_rule_spec = expand_ui_rule_spec;
        }
      }break;
      
      //////////////////////////////
      //- rjf: elements -> produce rows for the visible range of elements.
      //
      case DF_EvalVizBlockKind_Elements:
      {
        TG_Key direct_type_key = tg_unwrapped_direct_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, block->eval.type_key);
        TG_Kind direct_type_kind = tg_kind_from_key(direct_type_key);
        U64 direct_type_key_byte_size = tg_byte_size_from_graph_rdi_key(parse_ctx->type_graph, parse_ctx->rdi, direct_type_key);
        for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
        {
          // rjf: get keys for this row
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), idx+1);
          
          // rjf: get eval for this element
          DF_Eval elem_eval = zero_struct;
          {
            elem_eval.type_key = direct_type_key;
            elem_eval.mode     = block->eval.mode;
            elem_eval.offset   = block->eval.offset + idx*direct_type_key_byte_size;
            MemoryCopyArray(elem_eval.imm_u128, block->eval.imm_u128);
          }
          
          // rjf: get view rules
          String8 view_rule_string = df_eval_view_rule_from_key(eval_view, key);
          DF_CfgTable view_rule_table = df_cfg_table_from_inheritance(scratch.arena, &block->cfg_table);
          df_cfg_table_push_unparsed_string(scratch.arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
          
          // rjf: apply view rules to eval
          {
            elem_eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, elem_eval);
            elem_eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, macro_map, elem_eval, &view_rule_table);
          }
          
          // rjf: build row
          String8List display_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, elem_eval, 0, &view_rule_table);
          String8List edit_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, 0, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, elem_eval, 0, &view_rule_table);
          DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, key, elem_eval);
          row->expr                = push_str8f(arena, "[%I64u]", idx);
          row->display_value       = str8_list_join(arena, &display_strings, 0);
          row->edit_value          = str8_list_join(arena, &edit_strings, 0);
          row->value_ui_rule_node  = value_ui_rule_node;
          row->value_ui_rule_spec  = value_ui_rule_spec;
          row->expand_ui_rule_node = expand_ui_rule_node;
          row->expand_ui_rule_spec = expand_ui_rule_spec;
          if(expandability_required)
          {
            row->flags |= DF_EvalVizRowFlag_CanExpand;
          }
        }
      }break;
      
      //////////////////////////////
      //- rjf: links -> produce rows for the visible range of links in the linked-list chain.
      //
      case DF_EvalVizBlockKind_Links:
      {
        DF_EvalLinkBaseChunkList link_base_chunks = df_eval_link_base_chunk_list_from_eval(scratch.arena, parse_ctx->type_graph, parse_ctx->rdi, block->link_member_type_key, block->link_member_off, ctrl_ctx, block->eval, 512);
        DF_EvalLinkBaseArray link_bases = df_eval_link_base_array_from_chunk_list(scratch.arena, &link_base_chunks);
        for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
        {
          // rjf: get key for this row
          DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), idx+1);
          
          // rjf: get link base
          DF_EvalLinkBase *link_base = &link_bases.v[idx];
          
          // rjf: get eval for this link
          DF_Eval link_eval = zero_struct;
          {
            link_eval.type_key = block->eval.type_key;
            link_eval.mode     = link_base->mode;
            link_eval.offset   = link_base->offset;
          }
          
          // rjf: get view rules
          String8 view_rule_string = df_eval_view_rule_from_key(eval_view, key);
          DF_CfgTable view_rule_table = df_cfg_table_from_inheritance(scratch.arena, &block->cfg_table);
          df_cfg_table_push_unparsed_string(scratch.arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
          
          // rjf: apply view rules to eval
          link_eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, link_eval);
          link_eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, macro_map, link_eval, &view_rule_table);
          TG_Kind link_type_kind = tg_kind_from_key(link_eval.type_key);
          
          // rjf: build row
          String8List display_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, link_eval, 0, &view_rule_table);
          String8List edit_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, 0, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, link_eval, 0, &view_rule_table);
          DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, key, link_eval);
          row->expr                = push_str8f(arena, "[%I64u]", idx);
          row->display_value       = str8_list_join(arena, &display_strings, 0);
          row->edit_value          = str8_list_join(arena, &edit_strings, 0);
          row->value_ui_rule_node  = value_ui_rule_node;
          row->value_ui_rule_spec  = value_ui_rule_spec;
          row->expand_ui_rule_node = expand_ui_rule_node;
          row->expand_ui_rule_spec = expand_ui_rule_spec;
          if(expandability_required)
          {
            row->flags |= DF_EvalVizRowFlag_CanExpand;
          }
        }
      }break;
      
      //////////////////////////////
      //- rjf: canvas -> produce blank row, sized by the idx range specified in the block
      //
      case DF_EvalVizBlockKind_Canvas:
      if(num_skipped_visual < block_num_visual_rows)
      {
        DF_ExpandKey key = df_expand_key_make(df_hash_from_expand_key(block->parent_key), 1);
        DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, key, block->eval);
        row->flags               = DF_EvalVizRowFlag_Canvas;
        row->size_in_rows        = dim_1u64(intersect_1u64(visible_idx_range, r1u64(0, dim_1u64(block->visual_idx_range))));
        row->skipped_size_in_rows= (visible_idx_range.min > block->visual_idx_range.min) ? visible_idx_range.min - block->visual_idx_range.min : 0;
        row->chopped_size_in_rows= (visible_idx_range.max < block->visual_idx_range.max) ? block->visual_idx_range.max - visible_idx_range.max : 0;
        row->expand_ui_rule_node = expand_ui_rule_node;
        row->expand_ui_rule_spec = expand_ui_rule_spec;
      }break;
      
      //////////////////////////////
      //- rjf: all types -> produce rows for visible range
      //
      case DF_EvalVizBlockKind_DebugInfoTable:
      for(U64 idx = visible_idx_range.min; idx < visible_idx_range.max; idx += 1)
      {
        // rjf: unpack info about this row
        String8 name = dbgi_fuzzy_item_string_from_rdi_target_element_idx(parse_ctx->rdi, block->dbgi_target, block->backing_search_items.v[idx].idx);
        
        // rjf: get keys for this row
        DF_ExpandKey parent_key = block->parent_key;
        DF_ExpandKey key = block->key;
        key.child_num = block->backing_search_items.v[idx].idx;
        
        // rjf: get eval for this row
        DF_Eval eval = df_eval_from_string(arena, scope, ctrl_ctx, parse_ctx, macro_map, name);
        
        // rjf: get view rules
        String8 view_rule_string = df_eval_view_rule_from_key(eval_view, key);
        DF_CfgTable view_rule_table = df_cfg_table_from_inheritance(scratch.arena, &block->cfg_table);
        df_cfg_table_push_unparsed_string(scratch.arena, &view_rule_table, view_rule_string, DF_CfgSrc_User);
        
        // rjf: apply view rules to eval
        {
          eval = df_dynamically_typed_eval_from_eval(parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, eval);
          eval = df_eval_from_eval_cfg_table(arena, scope, ctrl_ctx, parse_ctx, macro_map, eval, &view_rule_table);
        }
        
        // rjf: build row
        String8List display_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, eval, 0, &view_rule_table);
        String8List edit_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, 0, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, default_radix, font, font_size, 500, 0, eval, 0, &view_rule_table);
        DF_EvalVizRow *row = df_eval_viz_row_list_push_new(arena, parse_ctx, &list, block, key, eval);
        row->expr = name;
        row->display_value = str8_list_join(arena, &display_strings, 0);
        row->edit_value = str8_list_join(arena, &edit_strings, 0);
        row->value_ui_rule_node = value_ui_rule_node;
        row->value_ui_rule_spec = value_ui_rule_spec;
        row->expand_ui_rule_node = expand_ui_rule_node;
        row->expand_ui_rule_spec = expand_ui_rule_spec;
      }break;
    }
  }
  scratch_end(scratch);
  ProfEnd();
  return list;
}

////////////////////////////////
//~ rjf: Hover Eval

internal void
df_set_hover_eval(DF_Window *ws, Vec2F32 pos, DF_CtrlCtx ctrl_ctx, DF_Entity *file, TxtPt pt, U64 vaddr, String8 string)
{
  if(ws->hover_eval_last_frame_idx+1 < df_frame_index() &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Left), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Middle), ui_key_zero()) &&
     ui_key_match(ui_active_key(UI_MouseButtonKind_Right), ui_key_zero()))
  {
    B32 is_new_string = !str8_match(ws->hover_eval_string, string, 0);
    if(is_new_string)
    {
      ws->hover_eval_first_frame_idx = ws->hover_eval_last_frame_idx = df_frame_index();
      arena_clear(ws->hover_eval_arena);
      ws->hover_eval_string = push_str8_copy(ws->hover_eval_arena, string);
      ws->hover_eval_file = df_handle_from_entity(file);
      ws->hover_eval_file_pt = pt;
      ws->hover_eval_vaddr = vaddr;
    }
    ws->hover_eval_ctrl_ctx = ctrl_ctx;
    ws->hover_eval_spawn_pos = pos;
    ws->hover_eval_last_frame_idx = df_frame_index();
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
  qsort(array->v, array->count, sizeof(array->v[0]), (int (*)(const void*, const void*))df_autocomp_lister_item_qsort_compare);
}

internal void
df_set_autocomp_lister_query(DF_Window *ws, UI_Key root_key, DF_CtrlCtx ctrl_ctx, DF_AutoCompListerFlags flags, String8 query)
{
  String8 current_query = str8(ws->autocomp_lister_query_buffer, ws->autocomp_lister_query_size);
  if(!str8_match(query, current_query, 0))
  {
    ws->autocomp_force_closed = 0;
  }
  if(!ui_key_match(ws->autocomp_root_key, root_key))
  {
    ws->autocomp_force_closed = 0;
    ws->autocomp_num_visible_rows_t = 0;
    ws->autocomp_open_t = 0;
  }
  if(ws->autocomp_last_frame_idx+1 < df_frame_index())
  {
    ws->autocomp_force_closed = 0;
    ws->autocomp_num_visible_rows_t = 0;
    ws->autocomp_open_t = 0;
  }
  ws->autocomp_ctrl_ctx = ctrl_ctx;
  ws->autocomp_root_key = root_key;
  ws->autocomp_lister_flags = flags;
  ws->autocomp_lister_query_size = Min(query.size, sizeof(ws->autocomp_lister_query_buffer));
  MemoryCopy(ws->autocomp_lister_query_buffer, query.str, ws->autocomp_lister_query_size);
  ws->autocomp_last_frame_idx = df_frame_index();
}

////////////////////////////////
//~ rjf: Search Strings

internal void
df_set_search_string(String8 string)
{
  arena_clear(df_gfx_state->string_search_arena);
  df_gfx_state->string_search_string = push_str8_copy(df_gfx_state->string_search_arena, string);
}

internal String8
df_push_search_string(Arena *arena)
{
  String8 result = push_str8_copy(arena, df_gfx_state->string_search_string);
  return result;
}

////////////////////////////////
//~ rjf: Background Text Searching Thread

internal void
df_text_search_match_chunk_list_push(Arena *arena, DF_TextSearchMatchChunkList *list, U64 cap, DF_TextSearchMatch *match)
{
  DF_TextSearchMatchChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, DF_TextSearchMatchChunkNode, 1);
    node->cap = cap;
    node->v = push_array_no_zero(arena, DF_TextSearchMatch, node->cap);
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  node->v[node->count] = *match;
  node->count += 1;
  list->total_count += 1;
}

internal DF_TextSearchMatchArray
df_text_search_match_array_from_chunk_list(Arena *arena, DF_TextSearchMatchChunkList *chunks)
{
  DF_TextSearchMatchArray array = {0};
  array.count = chunks->total_count;
  array.v = push_array_no_zero(arena, DF_TextSearchMatch, array.count);
  U64 idx = 0;
  for(DF_TextSearchMatchChunkNode *node = chunks->first; node != 0; node = node->next)
  {
    MemoryCopy(array.v+idx, node->v, node->count * sizeof(DF_TextSearchMatch));
    idx += node->count;
  }
  return array;
}

internal U64
df_text_search_little_hash_from_hash(U128 hash)
{
  // TODO(rjf): [ ] @de2ctrl df_text_search_little_hash_from_hash
  U64 little_hash = 0;
  MemoryCopy(&little_hash, &hash, sizeof(little_hash));
  return little_hash;
}

internal void
df_text_search_thread_entry_point(void *p)
{
#if 0
  // TODO(rjf): [ ] @de2ctrl text searcher -- wound up in DE_Hash
  
  //- rjf: types
  typedef enum WorkKind
  {
    WorkKind_Search,
    WorkKind_GarbageCollect,
    WorkKind_COUNT
  }
  WorkKind;
  typedef struct WorkNode WorkNode;
  struct WorkNode
  {
    WorkNode *next;
    WorkKind kind;
    U128 hash;
    String8 needle;
    DF_TextSliceFlags flags;
    TxtPt start_pt;
  };
  
  //- rjf: set up local debug engine map
  Arena *local_map_arena = arena_alloc();
  DE_ContentMap local_map = {0};
  DE_PipelineHint hint = zero_struct;
  
  //- rjf: loop over work
  for(;;)
  {
    //- rjf: begin
    Temp scratch = scratch_begin(0, 0);
    DE_Session *session = de_session_begin();
    
    //- rjf: wait for changes
    os_mutex_take(df_gfx_state->tsrch_wakeup_mutex);
    os_condition_variable_wait(df_gfx_state->tsrch_wakeup_cv, df_gfx_state->tsrch_wakeup_mutex, os_now_microseconds()+1000000);
    os_mutex_drop(df_gfx_state->tsrch_wakeup_mutex);
    
    //- rjf: gather all searches to complete
    WorkNode *first_work_node = 0;
    WorkNode *last_work_node = 0;
    for(U64 slot_idx = 0; slot_idx < df_gfx_state->tsrch_slot_count; slot_idx += 1)
    {
      //- rjf: slot idx -> slot * stripe
      DF_TextSearchCacheSlot *slot = &df_gfx_state->tsrch_slots[slot_idx];
      U64 stripe_idx = slot_idx%df_gfx_state->tsrch_stripe_count;
      OS_Handle stripe_rw_mutex = df_gfx_state->tsrch_stripe_rw_mutexes[stripe_idx];
      
      //- rjf: gather nodes in this slot
      os_rw_mutex_take_r(stripe_rw_mutex);
      {
        for(DF_TextSearchCacheNode *n = slot->first; n != 0; n = n->next)
        {
          B32 not_done = (n->good == 0);
          B32 expired = (os_now_microseconds() >= n->last_time_touched_us + 10000000);
          if(not_done || expired)
          {
            WorkNode *work = push_array(scratch.arena, WorkNode, 1);
            work->kind     = not_done ? WorkKind_Search : WorkKind_GarbageCollect;
            work->hash     = n->hash;
            work->needle   = push_str8_copy(scratch.arena, n->needle);
            work->flags    = n->flags;
            work->start_pt = n->start_pt;
            SLLQueuePush(first_work_node, last_work_node, work);
          }
        }
      }
      os_rw_mutex_drop_r(stripe_rw_mutex);
    }
    
    //- rjf: perform all searches
    for(WorkNode *work_node = first_work_node; work_node != 0; work_node = work_node->next)
    {
      //- rjf: unpack work node
      WorkKind kind           = work_node->kind;
      DE_Hash hash            = work_node->hash;
      String8 needle          = work_node->needle;
      DF_TextSliceFlags flags = work_node->flags;
      TxtPt start_pt          = work_node->start_pt;
      
      //- rjf: work params -> slot/stripe info
      U64 little_hash              = df_text_search_little_hash_from_hash(hash);
      U64 slot_idx                 = little_hash%df_gfx_state->tsrch_slot_count;
      DF_TextSearchCacheSlot *slot = &df_gfx_state->tsrch_slots[slot_idx];
      U64 stripe_idx               = slot_idx%df_gfx_state->tsrch_stripe_count;
      OS_Handle stripe_rw_mutex    = df_gfx_state->tsrch_stripe_rw_mutexes[stripe_idx];
      
      //- rjf: do work
      switch(kind)
      {
        //- rjf: search
        default:
        case WorkKind_Search:
        {
          //- rjf: hash -> artifacts
          DE_Key hash2data_key             = de_key_hash(DE_KeyFunc_DataFromHash, &hash);
          DE_Val *hash2data_val            = de_shared_chained_lookup(local_map_arena, &local_map, de_shared, &hint, &hash2data_key);
          DE_ContentBlock *hash2data_block = de_session_node_access_via_val(session, hash2data_val);
          String8 data                     = hash2data_block->data;
          DE_Key hash2txti_key             = de_key_hash(DE_KeyFunc_TxtiFromHash, &hash);
          DE_Val *hash2txti_val            = de_shared_chained_lookup(local_map_arena, &local_map, de_shared, &hint, &hash2txti_key);
          DE_ContentBlock *hash2txti_block = de_session_node_access_via_val(session, hash2txti_val);
          DE_InfoTxt *txt                  = hash2txti_block->txt;
          
          //- rjf: start pt -> search start offset
          U64 start_off = 0;
          if(1 <= start_pt.line && start_pt.line <= txt->line_count)
          {
            start_off = txt->line_ranges[start_pt.line-1].min;
            if(1 <= start_pt.column && start_pt.column <= dim_1u64(txt->line_ranges[start_pt.line-1]))
            {
              start_off += (start_pt.column-1);
            }
          }
          
          //- rjf: search for all needle occurrences
          U8 *byte_first = data.str;
          U8 *byte_opl = data.str+data.size;
          U8 *byte_start = byte_first + start_off;
          U64 num_bytes_traversed = 0;
          for(U8 *byte = byte_start; num_bytes_traversed < data.size;)
          {
            String8 rest_of_data = str8(byte, byte_opl-byte);
            String8 next_needle_size = str8_prefix(rest_of_data, needle.size);
            B32 found_match = str8_match(next_needle_size, needle, StringMatchFlag_CaseInsensitive);
            
            // rjf: record match
            if(found_match)
            {
              U64 match_off = (U64)(byte-byte_first);
              TxtPt match_pt = de_txt_pt_from_txti_off(txt, match_off);
              DF_TextSearchMatch match = {match_pt};
              os_rw_mutex_take_w(stripe_rw_mutex);
              {
                DF_TextSearchCacheNode *node = 0;
                for(DF_TextSearchCacheNode *n = slot->first; n != 0; n = n->next)
                {
                  if(MemoryMatchStruct(&hash, &n->hash) &&
                     str8_match(needle, n->needle, 0) &&
                     flags == n->flags)
                  {
                    node = n;
                  }
                }
                df_text_search_match_chunk_list_push(node->arena, &node->search_matches, 256, &match);
                node->good = 1;
              }
              os_rw_mutex_drop_w(stripe_rw_mutex);
            }
            
            // rjf: increment
            byte += 1;
            num_bytes_traversed += 1;
            if(byte >= byte_opl)
            {
              byte = byte_first;
            }
          }
          
        }break;
        
        //- rjf: garbage collect
        case WorkKind_GarbageCollect:
        {
          os_rw_mutex_take_w(stripe_rw_mutex);
          {
            DF_TextSearchCacheNode *node = 0;
            for(DF_TextSearchCacheNode *n = slot->first; n != 0; n = n->next)
            {
              if(MemoryMatchStruct(&hash, &n->hash) &&
                 str8_match(needle, n->needle, 0) &&
                 flags == n->flags)
              {
                node = n;
              }
            }
            if(node != 0)
            {
              DLLRemove(slot->first, slot->last, node);
              arena_release(node->arena);
            }
          }
          os_rw_mutex_drop_w(stripe_rw_mutex);
        }break;
      }
    }
    
    //- rjf: end
    de_session_end(session);
    scratch_end(scratch);
  }
#endif
}

internal int
df_text_search_match_array_qsort_compare(TxtPt *a, TxtPt *b)
{
  int result = 0;
  if(txt_pt_less_than(*a, *b))
  {
    result = -1;
  }
  else if(txt_pt_less_than(*b, *a))
  {
    result = +1;
  }
  return result;
}

internal void
df_text_search_match_array_sort_in_place(DF_TextSearchMatchArray *array)
{
  qsort(array->v, array->count, sizeof(DF_TextSearchMatch), (int (*)(const void *, const void *))df_text_search_match_array_qsort_compare);
}

internal DF_TextSearchMatch
df_text_search_match_array_find_nearest__linear_scan(DF_TextSearchMatchArray *array, TxtPt pt, Side side)
{
  ProfBeginFunction();
  DF_TextSearchMatch result = {0};
  if(array->count != 0)
  {
    S64 best_line_distance = max_S64;
    S64 best_column_distance = max_S64;
    B32 best_matches_side = 0;
    for(U64 idx = 0; idx < array->count; idx += 1)
    {
      S64 line_distance = abs_s64(array->v[idx].pt.line - pt.line);
      S64 column_distance = abs_s64(array->v[idx].pt.column - pt.column);
      B32 matches_side = (side == Side_Max ? txt_pt_less_than(pt, array->v[idx].pt) :
                          side == Side_Min ? txt_pt_less_than(array->v[idx].pt, pt) :
                          1);
      if(matches_side >= best_matches_side && line_distance == 0 && column_distance < best_column_distance)
      {
        best_matches_side = matches_side;
        best_line_distance = 0;
        best_column_distance = column_distance;
        result = array->v[idx];
      }
      else if(matches_side >= best_matches_side && line_distance < best_line_distance)
      {
        best_matches_side = matches_side;
        best_line_distance = line_distance;
        result = array->v[idx];
      }
    }
  }
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Colors, Fonts, Config

//- rjf: keybindings

internal OS_Key
df_os_key_from_cfg_string(String8 string)
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
df_clear_bindings(void)
{
  arena_clear(df_gfx_state->key_map_arena);
  df_gfx_state->key_map_table_size = 1024;
  df_gfx_state->key_map_table = push_array(df_gfx_state->key_map_arena, DF_KeyMapSlot, df_gfx_state->key_map_table_size);
  df_gfx_state->key_map_total_count = 0;
}

internal DF_BindingList
df_bindings_from_spec(Arena *arena, DF_CmdSpec *spec)
{
  DF_BindingList result = {0};
  U64 hash = df_hash_from_string(spec->info.string);
  U64 slot = hash%df_gfx_state->key_map_table_size;
  for(DF_KeyMapNode *n = df_gfx_state->key_map_table[slot].first; n != 0; n = n->hash_next)
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
df_bind_spec(DF_CmdSpec *spec, DF_Binding binding)
{
  if(binding.key != OS_Key_Null)
  {
    U64 hash = df_hash_from_string(spec->info.string);
    U64 slot = hash%df_gfx_state->key_map_table_size;
    DF_KeyMapNode *existing_node = 0;
    for(DF_KeyMapNode *n = df_gfx_state->key_map_table[slot].first; n != 0; n = n->hash_next)
    {
      if(n->spec == spec && n->binding.key == binding.key && n->binding.flags == binding.flags)
      {
        existing_node = n;
        break;
      }
    }
    if(existing_node == 0)
    {
      DF_KeyMapNode *n = df_gfx_state->free_key_map_node;
      if(n == 0)
      {
        n = push_array(df_gfx_state->arena, DF_KeyMapNode, 1);
      }
      else
      {
        df_gfx_state->free_key_map_node = df_gfx_state->free_key_map_node->hash_next;
      }
      n->spec = spec;
      n->binding = binding;
      DLLPushBack_NP(df_gfx_state->key_map_table[slot].first, df_gfx_state->key_map_table[slot].last, n, hash_next, hash_prev);
      df_gfx_state->key_map_total_count += 1;
    }
  }
}

internal void
df_unbind_spec(DF_CmdSpec *spec, DF_Binding binding)
{
  U64 hash = df_hash_from_string(spec->info.string);
  U64 slot = hash%df_gfx_state->key_map_table_size;
  for(DF_KeyMapNode *n = df_gfx_state->key_map_table[slot].first, *next = 0; n != 0; n = next)
  {
    next = n->hash_next;
    if(n->spec == spec && n->binding.key == binding.key && n->binding.flags == binding.flags)
    {
      DLLRemove_NP(df_gfx_state->key_map_table[slot].first, df_gfx_state->key_map_table[slot].last, n, hash_next, hash_prev);
      n->hash_next = df_gfx_state->free_key_map_node;
      df_gfx_state->free_key_map_node = n;
      df_gfx_state->key_map_total_count -= 1;
    }
  }
}

internal DF_CmdSpecList
df_cmd_spec_list_from_binding(Arena *arena, DF_Binding binding)
{
  DF_CmdSpecList result = {0};
  for(U64 idx = 0; idx < df_gfx_state->key_map_table_size; idx += 1)
  {
    for(DF_KeyMapNode *n = df_gfx_state->key_map_table[idx].first; n != 0; n = n->hash_next)
    {
      if(n->binding.key == binding.key && n->binding.flags == binding.flags)
      {
        df_cmd_spec_list_push(arena, &result, n->spec);
      }
    }
  }
  return result;
}

internal DF_CmdSpecList
df_cmd_spec_list_from_event_flags(Arena *arena, OS_EventFlags flags)
{
  DF_CmdSpecList result = {0};
  for(U64 idx = 0; idx < df_gfx_state->key_map_table_size; idx += 1)
  {
    for(DF_KeyMapNode *n = df_gfx_state->key_map_table[idx].first; n != 0; n = n->hash_next)
    {
      if(n->binding.flags == flags)
      {
        df_cmd_spec_list_push(arena, &result, n->spec);
      }
    }
  }
  return result;
}

//- rjf: colors

internal Vec4F32
df_rgba_from_theme_color(DF_ThemeColor color)
{
  return df_gfx_state->cfg_theme.colors[color];
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
    case TXT_TokenKind_Symbol: {color = DF_ThemeColor_CodeSymbol;}break;
  }
  return color;
}

//- rjf: fonts/sizes

internal F_Tag
df_font_from_slot(DF_FontSlot slot)
{
  F_Tag result = df_gfx_state->cfg_font_tags[slot];
  return result;
}

internal F32
df_font_size_from_slot(DF_Window *ws, DF_FontSlot slot)
{
  F32 result = 0;
  F32 dpi = os_dpi_from_window(ws->os);
  switch(slot)
  {
    default:
    case DF_FontSlot_Main:
    {
      F32 size_at_96dpi = 9.f;
      result = dpi * ((size_at_96dpi / 96.f) + ws->main_font_size_delta);
    }break;
    case DF_FontSlot_Code:
    {
      F32 size_at_96dpi = 9.f;
      result = dpi * ((size_at_96dpi / 96.f) + ws->code_font_size_delta);
    }break;
    case DF_FontSlot_Icons:
    {
      F32 size_at_96dpi = 10.f;
      result = dpi * ((size_at_96dpi / 96.f) + ws->main_font_size_delta);
    }break;
  }
  return result;
}

//- rjf: config serialization

internal int
df_qsort_compare__cfg_string_bindings(DF_StringBindingPair *a, DF_StringBindingPair *b)
{
  return strncmp((char *)a->string.str, (char *)b->string.str, Min(a->string.size, b->string.size));
}

internal String8List
df_cfg_strings_from_gfx(Arena *arena, String8 root_path, DF_CfgSrc source)
{
  ProfBeginFunction();
  String8List strs = {0};
  
  //- rjf: serialize windows
  {
    B32 first = 1;
    for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
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
      str8_list_pushf(arena, &strs, "  size:    (%i %i)\n", (int)size.x, (int)size.y);
      str8_list_pushf(arena, &strs, "  code_font_size_delta: %.5f\n", window->code_font_size_delta);
      str8_list_pushf(arena, &strs, "  main_font_size_delta: %.5f\n", window->main_font_size_delta);
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
            str8_list_pushf(arena, &strs, "%.*s%g:\n", indentation*2, indent_str.str, p->size_pct_of_parent_target.v[p->parent->split_axis]);
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
          for(DF_View *view = p->first_tab_view; !df_view_is_nil(view); view = view->next)
          {
            String8 view_string = view->spec->info.name;
            DF_Entity *view_entity = df_entity_from_handle(view->entity);
            
            // rjf: serialize views which can be serialized
            if(view->spec->info.flags & DF_ViewSpecFlag_CanSerialize)
            {
              str8_list_pushf(arena, &strs, "%.*s", indentation*2, indent_str.str);
              
              // rjf: serialize view string
              str8_list_push(arena, &strs, view_string);
              
              // rjf: serialize view parameterizations
              str8_list_push(arena, &strs, str8_lit(": {"));
              if(view == df_view_from_handle(p->selected_tab_view))
              {
                str8_list_push(arena, &strs, str8_lit("selected "));
              }
              if(view->spec->info.flags & DF_ViewSpecFlag_CanSerializeEntityPath)
              {
                if(view_entity->kind == DF_EntityKind_File)
                {
                  String8 profile_path = root_path;
                  String8 entity_path = df_full_path_from_entity(arena, view_entity);
                  String8 entity_path_rel = path_relative_dst_from_absolute_dst_src(arena, entity_path, profile_path);
                  str8_list_pushf(arena, &strs, "\"%S\"", entity_path_rel);
                }
              }
              String8 view_state_string = view->spec->info.string_from_state_hook(arena, view);
              str8_list_push(arena, &strs, view_state_string);
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
            if(pop_idx == rec.pop_count-1 && rec.next == &df_g_nil_panel)
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
  if(source == DF_CfgSrc_User)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8 indent_str = str8_lit("                                                                                                             ");
    U64 string_binding_pair_count = 0;
    DF_StringBindingPair *string_binding_pairs = push_array(scratch.arena, DF_StringBindingPair, df_gfx_state->key_map_total_count);
    for(U64 idx = 0;
        idx < df_gfx_state->key_map_table_size && string_binding_pair_count < df_gfx_state->key_map_total_count;
        idx += 1)
    {
      for(DF_KeyMapNode *n = df_gfx_state->key_map_table[idx].first;
          n != 0 && string_binding_pair_count < df_gfx_state->key_map_total_count;
          n = n->hash_next)
      {
        DF_StringBindingPair *pair = string_binding_pairs + string_binding_pair_count;
        pair->string = n->spec->info.string;
        pair->binding = n->binding;
        string_binding_pair_count += 1;
      }
    }
    qsort(string_binding_pairs, string_binding_pair_count, sizeof(DF_StringBindingPair), (int (*)(const void *, const void *))df_qsort_compare__cfg_string_bindings);
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
  if(source == DF_CfgSrc_User)
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
          if(!MemoryMatchStruct(&df_gfx_state->cfg_theme_target.colors[c], &df_g_theme_preset_colors_table[p][c]))
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
        Vec4F32 color_rgba = df_gfx_state->cfg_theme_target.colors[color];
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
  if(source == DF_CfgSrc_User)
  {
    String8 code_font_path_escaped = df_cfg_escaped_from_raw_string(arena, df_gfx_state->cfg_code_font_path);
    String8 main_font_path_escaped = df_cfg_escaped_from_raw_string(arena, df_gfx_state->cfg_main_font_path);
    str8_list_push(arena, &strs, str8_lit("/// fonts /////////////////////////////////////////////////////////////////////\n"));
    str8_list_push(arena, &strs, str8_lit("\n"));
    str8_list_pushf(arena, &strs, "code_font: \"%S\"\n", code_font_path_escaped);
    str8_list_pushf(arena, &strs, "main_font: \"%S\"\n", main_font_path_escaped);
    str8_list_push(arena, &strs, str8_lit("\n"));
  }
  
  ProfEnd();
  return strs;
}

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
  DF_Entity *thread = df_entity_from_ctrl_handle(event->machine_id, event->entity);
  String8 thread_display_string = df_display_string_from_entity(scratch.arena, thread);
  switch(event->kind)
  {
    default:
    {
      switch(event->cause)
      {
        default:{}break;
        case CTRL_EventCause_Finished:
        {
          if(!df_entity_is_nil(thread))
          {
            explanation = push_str8f(arena, "%S completed step", thread_display_string);
          }
          else
          {
            explanation = str8_lit("Stopped");
          }
        }break;
        case CTRL_EventCause_UserBreakpoint:
        {
          if(!df_entity_is_nil(thread))
          {
            icon = DF_IconKind_CircleFilled;
            explanation = push_str8f(arena, "%S hit a breakpoint", thread_display_string);
          }
        }break;
        case CTRL_EventCause_InterruptedByException:
        {
          if(!df_entity_is_nil(thread))
          {
            icon = DF_IconKind_WarningBig;
            switch(event->exception_kind)
            {
              default:
              {
                String8 exception_code_string = df_string_from_exception_code(event->exception_code);
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x%s%S", thread_display_string, event->exception_code, exception_code_string.size > 0 ? ": " : "", exception_code_string);
              }break;
              case CTRL_ExceptionKind_CppThrow:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: C++ exception", thread_display_string, event->exception_code);
              }break;
              case CTRL_ExceptionKind_MemoryRead:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: Access violation reading 0x%I64x",
                                         thread_display_string,
                                         event->exception_code,
                                         event->vaddr_rng.min);
              }break;
              case CTRL_ExceptionKind_MemoryWrite:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: Access violation writing 0x%I64x",
                                         thread_display_string,
                                         event->exception_code,
                                         event->vaddr_rng.min);
              }break;
              case CTRL_ExceptionKind_MemoryExecute:
              {
                explanation = push_str8f(arena, "Exception thrown by %S - 0x%x: Access violation executing 0x%I64x",
                                         thread_display_string,
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
          explanation = push_str8f(arena, "%S interrupted by trap - 0x%x", thread_display_string, event->exception_code);
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
//~ rjf: UI Widgets: Fancy Buttons

internal void
df_cmd_binding_button(DF_CmdSpec *spec)
{
  Temp scratch = scratch_begin(0, 0);
  DF_BindingList bindings = df_bindings_from_spec(scratch.arena, spec);
  DF_Binding binding = zero_struct;
  if(bindings.first != 0)
  {
    binding = bindings.first->binding;
  }
  
  //- rjf: grab all conflicts
  DF_CmdSpecList specs_with_binding = df_cmd_spec_list_from_binding(scratch.arena, binding);
  B32 has_conflicts = 0;
  for(DF_CmdSpecNode *n = specs_with_binding.first; n != 0; n = n->next)
  {
    if(n->spec != spec)
    {
      has_conflicts = 1;
      break;
    }
  }
  
  //- rjf: form binding string
  String8 keybinding_str = {0};
  {
    if(binding.key != OS_Key_Null)
    {
      String8List mods = os_string_list_from_event_flags(scratch.arena, binding.flags);
      String8 key = os_g_key_display_string_table[binding.key];
      str8_list_push(scratch.arena, &mods, key);
      StringJoin join = {0};
      join.sep = str8_lit(" + ");
      keybinding_str = str8_list_join(scratch.arena, &mods, &join);
    }
    else
    {
      keybinding_str = str8_lit("- no binding -");
    }
  }
  
  //- rjf: build box
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_text_alignment(UI_TextAlign_Center);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                          UI_BoxFlag_Clickable|
                                          UI_BoxFlag_DrawActiveEffects,
                                          "%S###bind_btn_%p", keybinding_str, spec);
  
  //- rjf: has conflicts => red
  if(has_conflicts)
  {
    box->text_color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
  }
  
  //- rjf: interaction
  UI_Signal sig = ui_signal_from_box(box);
  {
    // rjf: hover => visualize clickability
    if(ui_hovering(sig))
    {
      box->flags |= UI_BoxFlag_DrawBorder;
      box->flags |= UI_BoxFlag_DrawBackground;
    }
    
    // rjf: click => toggle activity
    if(!df_gfx_state->bind_change_active && ui_clicked(sig))
    {
      df_gfx_state->bind_change_active = 1;
      df_gfx_state->bind_change_cmd_spec = spec;
      df_gfx_state->bind_change_binding = binding;
    }
    else if(df_gfx_state->bind_change_active && ui_clicked(sig))
    {
      df_gfx_state->bind_change_active = 0;
    }
    
    // rjf: hover w/ conflicts => show conflicts
    if(ui_hovering(sig) && has_conflicts) UI_Tooltip
    {
      ui_labelf("This binding conflicts with others:");
      for(DF_CmdSpecNode *n = specs_with_binding.first; n != 0; n = n->next)
      {
        if(n->spec != spec)
        {
          ui_labelf("%S", n->spec->info.display_name);
        }
      }
    }
  }
  
  //- rjf: activity vis
  if(df_gfx_state->bind_change_active && df_gfx_state->bind_change_cmd_spec == spec)
  {
    box->flags |= UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground;
    box->border_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight1);
    Vec4F32 bg_color = df_rgba_from_theme_color(DF_ThemeColor_Highlight1);
    bg_color.w *= 0.25f;
    box->background_color = bg_color;
  }
  
  scratch_end(scratch);
}

internal UI_Signal
df_menu_bar_button(String8 string)
{
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable|UI_BoxFlag_DrawHotEffects, string);
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal UI_Signal
df_cmd_spec_button(DF_CmdSpec *spec)
{
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects|
                                          UI_BoxFlag_Clickable,
                                          "###cmd_%p", spec);
  UI_Parent(box) UI_HeightFill
  {
    DF_IconKind canonical_icon = spec->info.canonical_icon_kind;
    if(canonical_icon != DF_IconKind_Null)
    {
      UI_Font(df_font_from_slot(DF_FontSlot_Icons))
        UI_PrefWidth(ui_em(2.f, 1.f))
        UI_TextAlignment(UI_TextAlign_Center)
        UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      {
        ui_label(df_g_icon_kind_text_table[canonical_icon]);
      }
    }
    UI_PrefWidth(ui_text_dim(10, 1.f))
    {
      UI_Flags(UI_BoxFlag_DrawTextFastpathCodepoint)
        UI_FastpathCodepoint(box->fastpath_codepoint)
        ui_label(spec->info.display_name);
      ui_spacer(ui_pct(1, 0));
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
        UI_FastpathCodepoint(0)
      {
        df_cmd_binding_button(spec);
      }
    }
  }
  UI_Signal sig = ui_signal_from_box(box);
  return sig;
}

internal void
df_cmd_list_menu_buttons(DF_Window *ws, U64 count, DF_CoreCmdKind *cmds, U32 *fastpath_codepoints)
{
  Temp scratch = scratch_begin(0, 0);
  for(U64 idx = 0; idx < count; idx += 1)
  {
    DF_CmdSpec *spec = df_cmd_spec_from_core_cmd_kind(cmds[idx]);
    ui_set_next_fastpath_codepoint(fastpath_codepoints[idx]);
    UI_Signal sig = df_cmd_spec_button(spec);
    if(ui_clicked(sig))
    {
      DF_CmdParams params = df_cmd_params_from_window(ws);
      params.cmd_spec = spec;
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
      ui_ctx_menu_close();
      ws->menu_bar_focused = 0;
    }
  }
  scratch_end(scratch);
}

internal UI_Signal
df_icon_button(DF_IconKind kind, FuzzyMatchRangeList *matches, String8 string)
{
  String8 display_string = ui_display_part_from_key_string(string);
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|
                                         UI_BoxFlag_DrawBorder|
                                         UI_BoxFlag_DrawBackground|
                                         UI_BoxFlag_DrawHotEffects|
                                         UI_BoxFlag_DrawActiveEffects,
                                         string);
  UI_Parent(box)
  {
    if(display_string.size == 0)
    {
      ui_spacer(ui_pct(1, 0));
    }
    UI_TextAlignment(UI_TextAlign_Center)
      UI_Flags(UI_BoxFlag_DisableTextTrunc)
      UI_Font(df_font_from_slot(DF_FontSlot_Icons))
      UI_PrefWidth(ui_em(2.f, 1.f))
      UI_PrefHeight(ui_pct(1, 0))
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      ui_label(df_g_icon_kind_text_table[kind]);
    if(display_string.size != 0)
    {
      UI_PrefWidth(ui_pct(1.f, 0.f))
      {
        UI_Box *box = ui_label(display_string).box;
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
    }
    if(display_string.size == 0)
    {
      ui_spacer(ui_pct(1, 0));
    }
  }
  UI_Signal result = ui_signal_from_box(box);
  return result;
}

internal UI_Signal
df_icon_buttonf(DF_IconKind kind, FuzzyMatchRangeList *matches, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = df_icon_button(kind, matches, string);
  scratch_end(scratch);
  return sig;
}

internal void
df_entity_tooltips(DF_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  switch(entity->kind)
  {
    default:break;
    case DF_EntityKind_File:
    UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      String8 full_path = df_full_path_from_entity(scratch.arena, entity);
      ui_label(full_path);
    }break;
    case DF_EntityKind_Thread: UI_Flags(0)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      String8 display_string = df_display_string_from_entity(scratch.arena, entity);
      U64 rip_vaddr = df_query_cached_rip_from_thread(entity);
      Architecture arch = df_architecture_from_entity(entity);
      String8 arch_str = string_from_architecture(arch);
      U32 pid_or_tid = entity->ctrl_id;
      if(display_string.size != 0) UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        if(entity->flags & DF_EntityFlag_HasColor)
        {
          Vec4F32 color = df_rgba_from_entity(entity);
          ui_set_next_text_color(color);
        }
        UI_PrefWidth(ui_text_dim(10, 1)) ui_label(display_string);
      }
      {
        CTRL_Event stop_event = df_ctrl_last_stop_event();
        DF_Entity *stopper_thread = df_entity_from_ctrl_handle(stop_event.machine_id, stop_event.entity);
        if(stopper_thread == entity)
        {
          ui_spacer(ui_em(1.5f, 1.f));
          DF_IconKind icon_kind = DF_IconKind_Null;
          String8 explanation = df_stop_explanation_string_icon_from_ctrl_event(scratch.arena, &stop_event, &icon_kind);
          if(explanation.size != 0)
          {
            UI_PrefWidth(ui_children_sum(1)) UI_Row UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
            {
              UI_PrefWidth(ui_em(1.5f, 1.f)) UI_Font(df_font_from_slot(DF_FontSlot_Icons)) ui_label(df_g_icon_kind_text_table[icon_kind]);
              UI_PrefWidth(ui_text_dim(10, 1)) ui_label(explanation);
            }
          }
        }
      }
      ui_spacer(ui_em(1.5f, 1.f));
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        UI_PrefWidth(ui_em(18.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_labelf("TID: ");
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("%i", pid_or_tid);
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        UI_PrefWidth(ui_em(18.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_labelf("Architecture: ");
        UI_PrefWidth(ui_text_dim(10, 1)) ui_label(arch_str);
      }
      ui_spacer(ui_em(1.5f, 1.f));
      DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
      CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
      for(CTRL_UnwindFrame *frame = unwind.first; frame != 0; frame = frame->next)
      {
        U64 rip_vaddr = frame->rip;
        DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
        DF_Entity *binary = df_binary_file_from_module(module);
        U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
        String8 symbol = df_symbol_name_from_binary_voff(scratch.arena, binary, rip_voff);
        UI_PrefWidth(ui_children_sum(1)) UI_Row
        {
          UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_PrefWidth(ui_em(18.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_labelf("0x%I64x", rip_vaddr);
          if(symbol.size != 0)
          {
            UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_CodeFunction)) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("%S", symbol);
          }
          else
          {
            String8 module_filename = str8_skip_last_slash(module->name);
            UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_filename);
          }
        }
      }
    }break;
    case DF_EntityKind_Breakpoint: UI_Flags(0)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      if(entity->flags & DF_EntityFlag_HasColor)
      {
        Vec4F32 color = df_rgba_from_entity(entity);
        ui_set_next_text_color(color);
      }
      String8 display_string = df_display_string_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_text_dim(10, 1)) ui_label(display_string);
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        String8 stop_condition = df_entity_child_from_kind(entity, DF_EntityKind_Condition)->name;
        if(stop_condition.size == 0)
        {
          stop_condition = str8_lit("true");
        }
        UI_PrefWidth(ui_em(12.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_labelf("Stop Condition: ");
        UI_PrefWidth(ui_text_dim(10, 1)) UI_Font(df_font_from_slot(DF_FontSlot_Code)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), stop_condition);
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        U64 hit_count = entity->u64;
        String8 hit_count_text = str8_from_u64(scratch.arena, hit_count, 10, 0, 0);
        UI_PrefWidth(ui_em(12.f, 1.f)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) ui_labelf("Hit Count: ");
        UI_PrefWidth(ui_text_dim(10, 1)) UI_Font(df_font_from_slot(DF_FontSlot_Code)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), hit_count_text);
      }
    }break;
    case DF_EntityKind_WatchPin:
    UI_Font(df_font_from_slot(DF_FontSlot_Code))
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      if(entity->flags & DF_EntityFlag_HasColor)
      {
        Vec4F32 color = df_rgba_from_entity(entity);
        ui_set_next_text_color(color);
      }
      String8 display_string = df_display_string_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_text_dim(10, 1)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), display_string);
    }break;
  }
  scratch_end(scratch);
}

internal void
df_entity_desc_button(DF_Window *ws, DF_Entity *entity, FuzzyMatchRangeList *name_matches, String8 fuzzy_query)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  if(entity->kind == DF_EntityKind_Thread)
  {
    DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(ws);
    CTRL_Event stop_event = df_ctrl_last_stop_event();
    DF_Entity *stopped_thread = df_entity_from_ctrl_handle(stop_event.machine_id, stop_event.entity);
    DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
    B32 do_special_color = 0;
    Vec4F32 special_color = {0};
    if(selected_thread == entity)
    {
      Vec4F32 color = ui_top_background_color();
      Vec4F32 highlight_color = df_rgba_from_theme_color(DF_ThemeColor_SuccessBackground);
      color.x += (highlight_color.x - color.x) * 0.5f;
      color.y += (highlight_color.y - color.y) * 0.5f;
      color.z += (highlight_color.z - color.z) * 0.5f;
      color.w += (highlight_color.w - color.w) * 0.5f;
      special_color = color;
      do_special_color = 1;
    }
    if(stopped_thread == entity &&
       (stop_event.cause == CTRL_EventCause_UserBreakpoint ||
        stop_event.cause == CTRL_EventCause_InterruptedByException ||
        stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
        stop_event.cause == CTRL_EventCause_InterruptedByHalt))
    {
      Vec4F32 color = ui_top_background_color();
      Vec4F32 highlight_color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
      color.x += (highlight_color.x - color.x) * 0.5f;
      color.y += (highlight_color.y - color.y) * 0.5f;
      color.z += (highlight_color.z - color.z) * 0.5f;
      color.w += (highlight_color.w - color.w) * 0.5f;
      special_color = color;
      do_special_color = 1;
    }
    if(do_special_color)
    {
      ui_set_next_background_color(special_color);
    }
  }
  if(entity->cfg_src == DF_CfgSrc_CommandLine)
  {
    Vec4F32 bg_color = mix_4f32(ui_top_background_color(), df_rgba_from_theme_color(DF_ThemeColor_Highlight0), 0.25f);
    ui_set_next_background_color(bg_color);
  }
  else if(entity->kind == DF_EntityKind_Target && entity->b32 != 0)
  {
    Vec4F32 bg_color = mix_4f32(ui_top_background_color(), df_rgba_from_theme_color(DF_ThemeColor_Highlight1), 0.25f);
    ui_set_next_background_color(bg_color);
  }
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Key key = ui_key_from_stringf(ui_top_parent()->key, "entity_ref_button_%p", entity);
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_Clickable|
                                      UI_BoxFlag_DrawBorder|
                                      UI_BoxFlag_DrawBackground|
                                      UI_BoxFlag_DrawHotEffects|
                                      UI_BoxFlag_DrawActiveEffects,
                                      key);
  
  //- rjf: build contents
  UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 0))
  {
    DF_EntityKindFlags kind_flags = df_g_entity_kind_flags_table[entity->kind];
    DF_EntityOpFlags op_flags = df_g_entity_kind_op_flags_table[entity->kind];
    DF_IconKind icon = df_g_entity_kind_icon_kind_table[entity->kind];
    Vec4F32 entity_color = df_rgba_from_theme_color(DF_ThemeColor_PlainText);
    Vec4F32 entity_color_weak = df_rgba_from_theme_color(DF_ThemeColor_WeakText);
    if(entity->flags & DF_EntityFlag_HasColor)
    {
      entity_color = df_rgba_from_entity(entity);
      entity_color_weak = entity_color;
      entity_color_weak.w *= 0.5f;
    }
    UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      UI_TextAlignment(UI_TextAlign_Center)
      UI_Font(df_font_from_slot(DF_FontSlot_Icons))
      UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
      UI_PrefWidth(ui_em(1.875f, 1.f))
      ui_label(df_g_icon_kind_text_table[icon]);
    if(entity->cfg_src == DF_CfgSrc_CommandLine)
    {
      UI_TextColor(entity_color_weak)
        UI_TextAlignment(UI_TextAlign_Center)
        UI_PrefWidth(ui_em(1.875f, 1.f))
      {
        UI_Box *info_box = &ui_g_nil_box;
        UI_Font(df_font_from_slot(DF_FontSlot_Icons))
          UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
          UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_Highlight0))
        {
          info_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_Clickable, "%S###%p_temp_info", df_g_icon_kind_text_table[DF_IconKind_Info], entity);
        }
        UI_Signal info_sig = ui_signal_from_box(info_box);
        if(ui_hovering(info_sig)) UI_Tooltip
        {
          ui_labelf("Specified via command line; not saved in profile.");
        }
      }
    }
    String8 label = df_display_string_from_entity(scratch.arena, entity);
    UI_TextColor(entity_color)
      UI_Font(kind_flags&DF_EntityKindFlag_NameIsCode ? df_font_from_slot(DF_FontSlot_Code) : ui_top_font())
    {
      UI_Signal label_sig = ui_label(label);
      if(name_matches != 0)
      {
        ui_box_equip_fuzzy_match_ranges(label_sig.box, name_matches);
      }
    }
    if(entity->kind == DF_EntityKind_Target) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_FontSize(ui_top_font_size()*0.95f)
    {
      DF_Entity *args = df_entity_child_from_kind(entity, DF_EntityKind_Arguments);
      ui_label(args->name);
    }
    if(op_flags & DF_EntityOpFlag_Enable && entity->b32 == 0) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_FontSize(ui_top_font_size()*0.95f) UI_HeightFill
    {
      ui_label(str8_lit("(Disabled)"));
    }
    if(entity->kind == DF_EntityKind_Thread) UI_FontSize(ui_top_font_size()*0.75f) UI_Font(df_font_from_slot(DF_FontSlot_Code)) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_CodeFunction))
    {
      CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
      DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
      U64 idx = 0;
      U64 limit = 3;
      ui_spacer(ui_em(1.f, 1.f));
      for(CTRL_UnwindFrame *f = unwind.last; f != 0 && idx < limit; f = f->prev)
      {
        U64 rip_vaddr = f->rip;
        DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
        U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
        DF_Entity *binary = df_binary_file_from_module(module);
        String8 procedure_name = df_symbol_name_from_binary_voff(scratch.arena, binary, rip_voff);
        if(procedure_name.size != 0)
        {
          FuzzyMatchRangeList fuzzy_matches = {0};
          if(fuzzy_query.size != 0)
          {
            fuzzy_matches = fuzzy_match_find(scratch.arena, fuzzy_query, procedure_name);
          }
          if(idx != 0)
          {
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_em(2.f, 1.f)) ui_label(str8_lit(">"));
          }
          UI_PrefWidth(ui_text_dim(10.f, 0.f))
          {
            UI_Box *label_box = ui_label(procedure_name).box;
            ui_box_equip_fuzzy_match_ranges(label_box, &fuzzy_matches);
          }
          idx += 1;
          if(idx == limit)
          {
            UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText)) UI_PrefWidth(ui_text_dim(10.f, 1.f)) ui_label(str8_lit("> ..."));
          }
        }
      }
    }
  }
  
  //- rjf: do interaction on main box
  {
    UI_Signal sig = ui_signal_from_box(box);
    if(ui_key_match(box->key, ui_hot_key()))
    {
      df_entity_tooltips(entity);
    }
    
    // rjf: click => fastpath or dropdown for this entity
    if(ui_clicked(sig))
    {
      DF_CmdParams params = df_cmd_params_from_window(ws);
      params.entity = df_handle_from_entity(entity);
      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EntityRefFastPath));
    }
    
    // rjf: right-click => context menu for this entity
    else if(ui_right_clicked(sig))
    {
      DF_Handle handle = df_handle_from_entity(entity);
      if(ui_ctx_menu_is_open(ws->entity_ctx_menu_key) && df_handle_match(ws->entity_ctx_menu_entity, handle))
      {
        ui_ctx_menu_close();
      }
      else
      {
        ui_ctx_menu_open(ws->entity_ctx_menu_key, sig.box->key, v2f32(0, sig.box->rect.y1 - sig.box->rect.y0));
        ws->entity_ctx_menu_entity = handle;
      }
    }
    
    // rjf: drag+drop
    else if(ui_dragging(sig) && !contains_2f32(box->rect, ui_mouse()))
    {
      DF_DragDropPayload payload = {0};
      payload.key = box->key;
      payload.entity = df_handle_from_entity(entity);
      df_drag_begin(&payload);
    }
  }
  scratch_end(scratch);
  ProfEnd();
}

internal void
df_entity_src_loc_button(DF_Window *ws, DF_Entity *entity, TxtPt point)
{
  Temp scratch = scratch_begin(0, 0);
  String8 full_path = df_full_path_from_entity(scratch.arena, entity);
  String8 filename = str8_skip_last_slash(full_path);
  
  // rjf: build main box
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                          UI_BoxFlag_DrawBorder|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects,
                                          "entity_file_ref_button_%p", entity);
  UI_Signal sig = ui_signal_from_box(box);
  
  // rjf: build contents
  UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 0))
  {
    DF_IconKind icon = df_g_entity_kind_icon_kind_table[entity->kind];
    UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      UI_TextAlignment(UI_TextAlign_Center)
      UI_Font(df_font_from_slot(DF_FontSlot_Icons))
      UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
      ui_label(df_g_icon_kind_text_table[icon]);
    ui_labelf("%S:%I64d:%I64d", filename, point.line, point.column);
  }
  
  // rjf: click => find code location
  if(ui_clicked(sig))
  {
    DF_CmdParams params = df_cmd_params_from_window(ws);
    params.file_path = full_path;
    params.text_point = point;
    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
    df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
    df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
  }
  
  // rjf: drag+drop
  else if(ui_dragging(sig) && !contains_2f32(box->rect, ui_mouse()))
  {
    DF_DragDropPayload payload = {0};
    payload.key = box->key;
    payload.entity = df_handle_from_entity(entity);
    payload.text_point = point;
    df_drag_begin(&payload);
  }
  
  // rjf: hover => show full path
  else if(ui_hovering(sig) && !ui_dragging(sig)) UI_Tooltip
  {
    ui_labelf("%S:%I64d:%I64d", full_path, point.line, point.column);
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: UI Widgets: Text View

typedef struct DF_ThreadBoxDrawExtData DF_ThreadBoxDrawExtData;
struct DF_ThreadBoxDrawExtData
{
  Vec4F32 thread_color;
  F32 progress_t;
  F32 alive_t;
  B32 is_selected;
  B32 is_frozen;
};

internal UI_BOX_CUSTOM_DRAW(df_thread_box_draw_extensions)
{
  DF_ThreadBoxDrawExtData *u = (DF_ThreadBoxDrawExtData *)box->custom_draw_user_data;
  
  // rjf: draw line before next-to-execute line
  {
    R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0,
                                       box->parent->rect.y0 - box->font_size*0.125f,
                                       box->rect.x0 + box->font_size*260*u->alive_t,
                                       box->parent->rect.y0 + box->font_size*0.125f),
                                v4f32(u->thread_color.x, u->thread_color.y, u->thread_color.z, 0),
                                0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = u->thread_color;
  }
  
  // rjf: draw 'progress bar', showing thread's progress through the line's address range
  if(u->progress_t > 0)
  {
    Vec4F32 weak_thread_color = u->thread_color;
    weak_thread_color.w *= 0.4f;
    d_rect(r2f32p(box->rect.x0,
                  box->rect.y0,
                  box->rect.x1,
                  box->rect.y0 + (box->rect.y1-box->rect.y0)*u->progress_t),
           weak_thread_color,
           0, 0, 1);
  }
  
  // rjf: draw slight fill on selected thread
  if(u->is_selected)
  {
    Vec4F32 weak_thread_color = u->thread_color;
    weak_thread_color.w *= 0.3f;
    R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0,
                                       box->parent->rect.y0,
                                       box->rect.x0 + ui_top_font_size()*22.f*u->alive_t,
                                       box->parent->rect.y1),
                                v4f32(0, 0, 0, 0),
                                0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: locked icon on frozen threads
  if(u->is_frozen)
  {
    F32 lock_icon_off = ui_top_font_size()*0.2f;
    Vec4F32 lock_icon_color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
    lock_icon_color.x += (1 - lock_icon_color.x) * 0.3f;
    lock_icon_color.y += (1 - lock_icon_color.y) * 0.3f;
    lock_icon_color.z += (1 - lock_icon_color.z) * 0.3f;
    d_text(ui_icon_font(),
           box->font_size,
           v2f32((box->rect.x0 + box->rect.x1)/2 + lock_icon_off/2,
                 box->rect.y0 + lock_icon_off/2),
           lock_icon_color,
           df_g_icon_kind_text_table[DF_IconKind_Locked]);
  }
}

typedef struct DF_BreakpointBoxDrawExtData DF_BreakpointBoxDrawExtData;
struct DF_BreakpointBoxDrawExtData
{
  Vec4F32 color;
  F32 alive_t;
  F32 remap_px_delta;
};

internal UI_BOX_CUSTOM_DRAW(df_bp_box_draw_extensions)
{
  DF_BreakpointBoxDrawExtData *u = (DF_BreakpointBoxDrawExtData *)box->custom_draw_user_data;
  
  // rjf: draw line before next-to-execute line
  {
    R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0,
                                       box->parent->rect.y0 - box->font_size*0.125f,
                                       box->rect.x0 + ui_top_font_size()*250.f*u->alive_t,
                                       box->parent->rect.y0 + box->font_size*0.125f),
                                v4f32(u->color.x, u->color.y, u->color.z, 0),
                                0, 0, 1.f);
    inst->colors[Corner_00] = inst->colors[Corner_01] = u->color;
  }
  
  // rjf: draw slight fill
  {
    Vec4F32 weak_thread_color = u->color;
    weak_thread_color.w *= 0.3f;
    R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0,
                                       box->parent->rect.y0,
                                       box->rect.x0 + ui_top_font_size()*22.f*u->alive_t,
                                       box->parent->rect.y1),
                                v4f32(0, 0, 0, 0),
                                0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: draw remaps
  if(u->remap_px_delta != 0)
  {
    F32 remap_px_delta = u->remap_px_delta;
    F32 circle_advance = f_dim_from_tag_size_string(box->font, box->font_size, df_g_icon_kind_text_table[DF_IconKind_CircleFilled]).x;
    Vec2F32 bp_text_pos = ui_box_text_position(box);
    Vec2F32 bp_center = v2f32(bp_text_pos.x + circle_advance/2 + circle_advance/8.f, bp_text_pos.y);
    F_Metrics icon_font_metrics = f_metrics_from_tag_size(box->font, box->font_size);
    F32 icon_font_line_height = f_line_height_from_metrics(&icon_font_metrics);
    F32 remap_bar_thickness = 0.3f*ui_top_font_size();
    Vec4F32 remap_color = u->color;
    remap_color.w *= 0.3f;
    R_Rect2DInst *inst = d_rect(r2f32p(bp_center.x - remap_bar_thickness,
                                       bp_center.y + ClampTop(remap_px_delta, 0) - remap_bar_thickness,
                                       bp_center.x + remap_bar_thickness,
                                       bp_center.y + ClampBot(remap_px_delta, 0) + remap_bar_thickness),
                                remap_color, 2.f, 0, 1.f);
    d_text(box->font, box->font_size,
           v2f32(bp_text_pos.x,
                 bp_center.y + remap_px_delta),
           remap_color,
           df_g_icon_kind_text_table[DF_IconKind_CircleFilled]);
  }
}

internal DF_CodeSliceSignal
df_code_slice(DF_Window *ws, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, String8 string)
{
  DF_CodeSliceSignal result = {0};
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx->thread);
  DF_Entity *selected_thread_process = df_entity_ancestor_from_kind(selected_thread, DF_EntityKind_Process);
  U64 selected_thread_rip_unwind_vaddr = df_query_cached_rip_from_thread_unwind(selected_thread, ctrl_ctx->unwind_count);
  DF_Entity *selected_thread_module = df_module_from_process_vaddr(selected_thread_process, selected_thread_rip_unwind_vaddr);
  CTRL_Event stop_event = df_ctrl_last_stop_event();
  DF_Entity *stopper_thread = df_entity_from_ctrl_handle(stop_event.machine_id, stop_event.entity);
  B32 is_focused = ui_is_focus_active();
  B32 ctrlified = (os_get_event_flags() & OS_EventFlag_Ctrl);
  Vec4F32 code_line_bgs[] =
  {
    df_rgba_from_theme_color(DF_ThemeColor_LineInfo0),
    df_rgba_from_theme_color(DF_ThemeColor_LineInfo1),
    df_rgba_from_theme_color(DF_ThemeColor_LineInfo2),
    df_rgba_from_theme_color(DF_ThemeColor_LineInfo3),
  };
  
  //////////////////////////////
  //- rjf: build top-level container
  //
  UI_Box *top_container_box = &ui_g_nil_box;
  Rng2F32 clipped_top_container_rect = {0};
  {
    ui_set_next_child_layout_axis(Axis2_X);
    ui_set_next_pref_width(ui_px(params->line_text_max_width_px, 1));
    ui_set_next_pref_height(ui_children_sum(1));
    top_container_box = ui_build_box_from_string(UI_BoxFlag_DisableFocusViz|UI_BoxFlag_DrawBorder, string);
    clipped_top_container_rect = top_container_box->rect;
    for(UI_Box *b = top_container_box; !ui_box_is_nil(b); b = b->parent)
    {
      if(b->flags & UI_BoxFlag_Clip)
      {
        clipped_top_container_rect = intersect_2f32(b->rect, clipped_top_container_rect);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build per-line background colors
  //
  Vec4F32 *line_bg_colors = push_array(scratch.arena, Vec4F32, dim_1s64(params->line_num_range)+1);
  {
    //- rjf: color line with stopper-thread red
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      DF_EntityList threads = params->line_ips[line_idx];
      for(DF_EntityNode *n = threads.first; n != 0; n = n->next)
      {
        if(n->entity == stopper_thread && (stop_event.cause == CTRL_EventCause_InterruptedByTrap || stop_event.cause == CTRL_EventCause_InterruptedByException))
        {
          line_bg_colors[line_idx] = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
          line_bg_colors[line_idx].w *= 0.25f;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build per-line context menus
  //
  UI_Key *ctx_menu_keys = push_array(scratch.arena, UI_Key, dim_1s64(params->line_num_range)+1);
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      ctx_menu_keys[line_idx] = ui_key_from_stringf(top_container_box->key, "line_ctx_%I64d", line_num);
      UI_CtxMenu(ctx_menu_keys[line_idx]) UI_PrefWidth(ui_em(37.f, 1.f))
      {
        DF_TextLineSrc2DasmInfoList *line_src2dasm_list = &params->line_src2dasm[line_idx];
        DF_TextLineDasm2SrcInfoList *line_dasm2src_list = &params->line_dasm2src[line_idx];
        
        //- rjf: copy selection
        if(!txt_pt_match(*cursor, *mark) && ui_clicked(df_cmd_spec_button(df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Copy))))
        {
          result.copy_range = txt_rng(*cursor, *mark);
          ui_ctx_menu_close();
        }
        
        //- rjf: watch selection
        if(ui_clicked(df_cmd_spec_button(df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpressionAtCursor))))
        {
          result.toggle_cursor_watch = 1;
          ui_ctx_menu_close();
        }
        
        //- rjf: set-next-statement
        if(ui_clicked(df_cmd_spec_button(df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetNextStatement))))
        {
          result.set_next_statement_line_num = line_num;
          ui_ctx_menu_close();
        }
        
        //- rjf: run-to-line
        if(ui_clicked(df_icon_buttonf(DF_IconKind_Play, 0, "Run To Line")))
        {
          result.run_to_line_num = line_num;
          ui_ctx_menu_close();
        }
        
        //- rjf: breakpoint placing
        if((params->line_bps[line_idx].count == 0 &&
            ui_clicked(df_icon_buttonf(DF_IconKind_CircleFilled, 0, "Place Breakpoint"))) ||
           (params->line_bps[line_idx].count != 0 &&
            ui_clicked(df_icon_buttonf(DF_IconKind_CircleFilled, 0, "Remove Breakpoint"))))
        {
          result.clicked_margin_line_num = line_num;
          ui_ctx_menu_close();
        }
        
        //- rjf: go from src -> disasm
        if(line_src2dasm_list->first != 0 &&
           ui_clicked(df_icon_buttonf(DF_IconKind_Find, 0, "Go To Disassembly")))
        {
          result.goto_disasm_line_num = line_num;
          ui_ctx_menu_close();
        }
        
        //- rjf: go from disasm -> src
        if(line_dasm2src_list->first != 0 &&
           ui_clicked(df_icon_buttonf(DF_IconKind_Find, 0, "Go To Source")))
        {
          result.goto_src_line_num = line_num;
          ui_ctx_menu_close();
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build margins
  //
  UI_Box *margin_container_box = &ui_g_nil_box;
  if(params->flags & DF_CodeSliceFlag_Margin) UI_Focus(UI_FocusKind_Off) UI_Parent(top_container_box) ProfScope("build margins")
  {
    ui_set_next_pref_width(ui_px(params->margin_width_px, 1));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_child_layout_axis(Axis2_Y);
    margin_container_box = ui_build_box_from_string(UI_BoxFlag_Clickable, str8_lit("margin_container"));
    UI_Parent(margin_container_box) UI_PrefHeight(ui_px(params->line_height_px, 1.f))
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        DF_EntityList line_ips  = params->line_ips[line_idx];
        DF_EntityList line_bps  = params->line_bps[line_idx];
        DF_EntityList line_pins = params->line_pins[line_idx];
        ui_set_next_hover_cursor(OS_Cursor_HandPoint);
        ui_set_next_background_color(v4f32(0, 0, 0, 0));
        UI_Box *line_margin_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawActiveEffects, "line_margin_%I64x", line_num);
        UI_Parent(line_margin_box)
        {
          //- rjf: build margin thread ip ui
          for(DF_EntityNode *n = line_ips.first; n != 0; n = n->next)
          {
            // rjf: unpack thread
            DF_Entity *thread = n->entity;
            U64 unwind_count = (thread == selected_thread) ? ctrl_ctx->unwind_count : 0;
            U64 thread_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
            DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, thread_rip_vaddr);
            DF_Entity *binary = df_binary_file_from_module(module);
            U64 thread_rip_voff = df_voff_from_vaddr(module, thread_rip_vaddr);
            
            // rjf: thread info => color
            Vec4F32 color = v4f32(1, 1, 1, 1);
            {
              if(unwind_count != 0)
              {
                color = df_rgba_from_theme_color(DF_ThemeColor_ThreadUnwound);
              }
              else if(thread == stopper_thread &&
                      (stop_event.cause == CTRL_EventCause_InterruptedByHalt ||
                       stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
                       stop_event.cause == CTRL_EventCause_InterruptedByException))
              {
                color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
              }
              else if(thread->flags & DF_EntityFlag_HasColor)
              {
                color = df_rgba_from_entity(thread);
              }
              if(df_ctrl_targets_running() && df_ctrl_last_run_frame_idx() < df_frame_index())
              {
                color.w *= 0.5f;
              }
              if(thread != selected_thread)
              {
                color.w *= 0.8f;
              }
            }
            
            // rjf: build thread box
            ui_set_next_hover_cursor(OS_Cursor_UpDownLeftRight);
            ui_set_next_font(ui_icon_font());
            ui_set_next_font_size(params->font_size);
            ui_set_next_pref_width(ui_pct(1, 0));
            ui_set_next_pref_height(ui_pct(1, 0));
            ui_set_next_text_color(color);
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Key thread_box_key = ui_key_from_stringf(top_container_box->key, "###ip_%p", thread);
            UI_Box *thread_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|
                                                       UI_BoxFlag_Clickable|
                                                       UI_BoxFlag_AnimatePosX|
                                                       UI_BoxFlag_DrawText,
                                                       thread_box_key);
            ui_box_equip_display_string(thread_box, df_g_icon_kind_text_table[DF_IconKind_RightArrow]);
            UI_Signal thread_sig = ui_signal_from_box(thread_box);
            
            // rjf: custom draw
            {
              DF_ThreadBoxDrawExtData *u = push_array(ui_build_arena(), DF_ThreadBoxDrawExtData, 1);
              u->thread_color = color;
              u->alive_t = thread->alive_t;
              u->is_selected = (thread == selected_thread);
              u->is_frozen = df_entity_is_frozen(thread);
              ui_box_equip_custom_draw(thread_box, df_thread_box_draw_extensions, u);
              
              // rjf: fill out progress t (progress into range of current line's
              // voff range)
              if(params->line_src2dasm[line_idx].first != 0)
              {
                DF_TextLineSrc2DasmInfoList *line_info_list = &params->line_src2dasm[line_idx];
                DF_TextLineSrc2DasmInfo *line_info = 0;
                for(DF_TextLineSrc2DasmInfoNode *n = line_info_list->first;
                    n != 0;
                    n = n->next)
                {
                  if(n->v.binary == binary)
                  {
                    line_info = &n->v;
                    break;
                  }
                }
                if(line_info != 0)
                {
                  Rng1U64 line_voff_rng = line_info->voff_range;
                  Vec4F32 weak_thread_color = color;
                  weak_thread_color.w *= 0.4f;
                  F32 progress_t = (line_voff_rng.max != line_voff_rng.min) ? ((F32)(thread_rip_voff - line_voff_rng.min) / (F32)(line_voff_rng.max - line_voff_rng.min)) : 0;
                  progress_t = Clamp(0, progress_t, 1);
                  u->progress_t = progress_t;
                }
              }
            }
            
            // rjf: hover tooltips
            if(ui_hovering(thread_sig))
            {
              df_entity_tooltips(thread);
            }
            
            // rjf: ip right-click menu
            if(ui_right_clicked(thread_sig))
            {
              DF_Handle handle = df_handle_from_entity(thread);
              if(ui_ctx_menu_is_open(ws->entity_ctx_menu_key) && df_handle_match(ws->entity_ctx_menu_entity, handle))
              {
                ui_ctx_menu_close();
              }
              else
              {
                ui_ctx_menu_open(ws->entity_ctx_menu_key, thread_box->key, v2f32(0, thread_box->rect.y1-thread_box->rect.y0));
                ws->entity_ctx_menu_entity = handle;
              }
            }
            
            // rjf: double click => select
            if(ui_double_clicked(thread_sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(thread);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
            }
            
            // rjf: drag start
            if(ui_dragging(thread_sig) && !contains_2f32(thread_box->rect, ui_mouse()))
            {
              DF_DragDropPayload payload = {0};
              payload.key = thread_box->key;
              payload.entity = df_handle_from_entity(thread);
              df_drag_begin(&payload);
            }
          }
          
          //- rjf: build margin breakpoint ui
          for(DF_EntityNode *n = line_bps.first; n != 0; n = n->next)
          {
            DF_Entity *bp = n->entity;
            Vec4F32 bp_color = df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
            if(bp->flags & DF_EntityFlag_HasColor)
            {
              bp_color = df_rgba_from_entity(bp);
            }
            if(bp->b32 == 0)
            {
              bp_color = v4f32(bp_color.x * 0.6f, bp_color.y * 0.6f, bp_color.z * 0.6f, bp_color.w * 0.6f);
            }
            
            // rjf: prep custom rendering data
            DF_BreakpointBoxDrawExtData *bp_draw = push_array(ui_build_arena(), DF_BreakpointBoxDrawExtData, 1);
            {
              bp_draw->color = bp_color;
              DF_TextLineSrc2DasmInfoList *src2dasm_list = &params->line_src2dasm[line_idx];
              for(DF_TextLineSrc2DasmInfoNode *n = src2dasm_list->first; n != 0; n = n->next)
              {
                S64 remap_line = (S64)n->v.remap_line;
                if(remap_line != line_num)
                {
                  bp_draw->remap_px_delta = (remap_line - line_num) * params->line_height_px;
                  break;
                }
              }
              bp_draw->alive_t = bp->alive_t;
            }
            
            // rjf: build box for breakpoint
            ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
            ui_set_next_font_size(params->font_size * 1.f);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            ui_set_next_text_color(bp_color);
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Box *bp_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                                       UI_BoxFlag_DrawActiveEffects|
                                                       UI_BoxFlag_DrawHotEffects|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_AnimatePosX|
                                                       UI_BoxFlag_Clickable|
                                                       UI_BoxFlag_DisableTextTrunc,
                                                       "%S##bp_%p",
                                                       df_g_icon_kind_text_table[DF_IconKind_CircleFilled],
                                                       bp);
            ui_box_equip_custom_draw(bp_box, df_bp_box_draw_extensions, bp_draw);
            UI_Signal bp_sig = ui_signal_from_box(bp_box);
            
            // rjf: bp hovering
            if(ui_hovering(bp_sig))
            {
              df_entity_tooltips(bp);
            }
            
            // rjf: click => remove breakpoint
            if(ui_clicked(bp_sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(bp);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RemoveBreakpoint));
            }
            
            // rjf: drag start
            if(ui_dragging(bp_sig) && !contains_2f32(bp_box->rect, ui_mouse()))
            {
              DF_DragDropPayload payload = {0};
              payload.entity = df_handle_from_entity(bp);
              df_drag_begin(&payload);
            }
            
            // rjf: bp right-click menu
            if(ui_right_clicked(bp_sig))
            {
              DF_Handle handle = df_handle_from_entity(bp);
              if(ui_ctx_menu_is_open(ws->entity_ctx_menu_key) && df_handle_match(ws->entity_ctx_menu_entity, handle))
              {
                ui_ctx_menu_close();
              }
              else
              {
                ui_ctx_menu_open(ws->entity_ctx_menu_key, bp_box->key, v2f32(0, bp_box->rect.y1-bp_box->rect.y0));
                ws->entity_ctx_menu_entity = handle;
              }
            }
          }
          
          //- rjf: build margin watch pin ui
          for(DF_EntityNode *n = line_pins.first; n != 0; n = n->next)
          {
            DF_Entity *pin = n->entity;
            Vec4F32 color = v4f32(1, 1, 1, 1);
            if(pin->flags & DF_EntityFlag_HasColor)
            {
              color = df_rgba_from_entity(pin);
            }
            
            // rjf: build box for watch
            ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
            ui_set_next_font_size(params->font_size * 1.f);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            ui_set_next_text_color(color);
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Box *pin_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                                        UI_BoxFlag_DrawActiveEffects|
                                                        UI_BoxFlag_DrawHotEffects|
                                                        UI_BoxFlag_DrawBorder|
                                                        UI_BoxFlag_Clickable|
                                                        UI_BoxFlag_AnimatePosX|
                                                        UI_BoxFlag_DisableTextTrunc,
                                                        "%S##watch_%p",
                                                        df_g_icon_kind_text_table[DF_IconKind_Pin],
                                                        pin);
            UI_Signal pin_sig = ui_signal_from_box(pin_box);
            
            // rjf: watch hovering
            if(ui_hovering(pin_sig))
            {
              df_entity_tooltips(pin);
            }
            
            // rjf: click => remove pin
            if(ui_clicked(pin_sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(pin);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RemoveEntity));
            }
            
            // rjf: drag start
            if(ui_dragging(pin_sig) && !contains_2f32(pin_box->rect, ui_mouse()))
            {
              DF_DragDropPayload payload = {0};
              payload.entity = df_handle_from_entity(pin);
              df_drag_begin(&payload);
            }
            
            // rjf: watch right-click menu
            if(ui_right_clicked(pin_sig))
            {
              DF_Handle handle = df_handle_from_entity(pin);
              if(ui_ctx_menu_is_open(ws->entity_ctx_menu_key) && df_handle_match(ws->entity_ctx_menu_entity, handle))
              {
                ui_ctx_menu_close();
              }
              else
              {
                ui_ctx_menu_open(ws->entity_ctx_menu_key, pin_box->key, v2f32(0, pin_box->rect.y1-pin_box->rect.y0));
                ws->entity_ctx_menu_entity = handle;
              }
            }
          }
        }
        UI_Signal line_margin_sig = ui_signal_from_box(line_margin_box);
        if(ui_clicked(line_margin_sig))
        {
          result.clicked_margin_line_num = line_num;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build main text container box, for mouse interaction on both lines & line numbers
  //
  UI_Box *text_container_box = &ui_g_nil_box;
  UI_Parent(top_container_box) UI_Focus(UI_FocusKind_Off)
  {
    ui_set_next_hover_cursor(ctrlified ? OS_Cursor_HandPoint : OS_Cursor_IBar);
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    text_container_box = ui_build_box_from_string(UI_BoxFlag_Clickable, str8_lit("text_container"));
  }
  
  //////////////////////////////
  //- rjf: determine starting offset for each at line, at which we can begin placing extra info to the right
  //
  F32 *line_extras_off = push_array(scratch.arena, F32, dim_1s64(params->line_num_range)+1);
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      String8 line_text = params->line_text[line_idx];
      F32 line_text_dim = f_dim_from_tag_size_string(params->font, params->font_size, line_text).x + params->line_num_width_px;
      line_extras_off[line_idx] = Max(line_text_dim, params->font_size*50);
    }
  }
  
  //////////////////////////////
  //- rjf: produce per-line extra annotation containers
  //
  UI_Box **line_extras_boxes = push_array(scratch.arena, UI_Box *, dim_1s64(params->line_num_range)+1);
  UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_px(params->line_height_px, 1.f)) UI_Parent(text_container_box) UI_Focus(UI_FocusKind_Off)
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      ui_set_next_fixed_x(line_extras_off[line_idx]);
      ui_set_next_fixed_y(line_idx*params->line_height_px);
      line_extras_boxes[line_idx] = ui_build_box_from_stringf(0, "###extras_%I64x", line_idx);
    }
  }
  
  //////////////////////////////
  //- rjf: build exception annotations
  //
  UI_Focus(UI_FocusKind_Off)
  {
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      DF_EntityList threads = params->line_ips[line_idx];
      for(DF_EntityNode *n = threads.first; n != 0; n = n->next)
      {
        DF_Entity *thread = n->entity;
        if(thread == stopper_thread &&
           (stop_event.cause == CTRL_EventCause_InterruptedByException ||
            stop_event.cause == CTRL_EventCause_InterruptedByTrap))
        {
          DF_IconKind icon = DF_IconKind_WarningBig;
          String8 explanation = df_stop_explanation_string_icon_from_ctrl_event(scratch.arena, &stop_event, &icon);
          UI_Parent(line_extras_boxes[line_idx]) UI_PrefWidth(ui_children_sum(1)) UI_PrefHeight(ui_px(params->line_height_px, 1.f))
          {
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_Clickable, "###exception_info");
            UI_Parent(box)
            {
              UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
                UI_Font(df_font_from_slot(DF_FontSlot_Icons))
                UI_PrefWidth(ui_text_dim(10, 1))
              {
                ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
              }
              UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
                UI_PrefWidth(ui_text_dim(10, 1))
              {
                ui_label(explanation);
              }
            }
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build watch pin annotations
  //
  UI_Focus(UI_FocusKind_Off)
  {
    DBGI_Scope *scope = dbgi_scope_open();
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      DF_EntityList pins = params->line_pins[line_idx];
      if(pins.count != 0) UI_Parent(line_extras_boxes[line_idx]) UI_Font(params->font) UI_FontSize(params->font_size) UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      {
        for(DF_EntityNode *n = pins.first; n != 0; n = n->next)
        {
          DF_Entity *pin = n->entity;
          String8 pin_expr = pin->name;
          DF_Eval eval = df_eval_from_string(scratch.arena, scope, ctrl_ctx, parse_ctx, &eval_string2expr_map_nil, pin_expr);
          String8 eval_string = {0};
          if(!tg_key_match(tg_key_zero(), eval.type_key))
          {
            DF_CfgTable cfg_table = {0};
            String8List eval_strings = df_single_line_eval_value_strings_from_eval(scratch.arena, DF_EvalVizStringFlag_ReadOnlyDisplayRules, parse_ctx->type_graph, parse_ctx->rdi, ctrl_ctx, 10, params->font, params->font_size, params->font_size*60.f, 0, eval, 0, &cfg_table);
            eval_string = str8_list_join(scratch.arena, &eval_strings, 0);
          }
          ui_spacer(ui_em(1.5f, 1.f));
          ui_set_next_pref_width(ui_children_sum(1));
          UI_Key pin_box_key = ui_key_from_stringf(ui_key_zero(), "###pin_%p", pin);
          UI_Box *pin_box = ui_build_box_from_key(UI_BoxFlag_AnimatePos|UI_BoxFlag_Clickable|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawBorder, pin_box_key);
          UI_Parent(pin_box) UI_PrefWidth(ui_text_dim(10, 1))
          {
            Vec4F32 pin_color = df_rgba_from_theme_color(DF_ThemeColor_WeakText);
            if(pin->flags & DF_EntityFlag_HasColor)
            {
              pin_color = df_rgba_from_entity(pin);
            }
            UI_PrefWidth(ui_em(1.5f, 1.f))
              UI_Font(df_font_from_slot(DF_FontSlot_Icons))
              UI_TextColor(pin_color)
              UI_TextAlignment(UI_TextAlign_Center)
              UI_Flags(UI_BoxFlag_DisableTextTrunc)
            {
              UI_Signal sig = ui_buttonf("%S###pin_nub", df_g_icon_kind_text_table[DF_IconKind_Pin]);
              if(ui_dragging(sig) && !contains_2f32(sig.box->rect, ui_mouse()))
              {
                DF_DragDropPayload payload = {0};
                payload.entity = df_handle_from_entity(pin);
                df_drag_begin(&payload);
              }
              if(ui_clicked(sig) || ui_right_clicked(sig))
              {
                ui_ctx_menu_open(ws->entity_ctx_menu_key, sig.box->key, v2f32(0, sig.box->rect.y1-sig.box->rect.y0));
                ws->entity_ctx_menu_entity = df_handle_from_entity(pin);
              }
            }
            df_code_label(0.8f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), pin_expr);
            df_code_label(0.6f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), eval_string);
          }
          UI_Signal pin_sig = ui_signal_from_box(pin_box);
          if(ui_key_match(pin_box_key, ui_hot_key()))
          {
            df_set_hover_eval(ws, v2f32(pin_box->rect.x0, pin_box->rect.y1-2.f), *ctrl_ctx, &df_g_nil_entity, txt_pt(1, 1), 0, pin_expr);
          }
        }
      }
    }
    dbgi_scope_close(scope);
  }
  
  //////////////////////////////
  //- rjf: mouse -> text coordinates
  //
  TxtPt mouse_pt = {0};
  ProfScope("mouse -> text coordinates")
  {
    Vec2F32 mouse = ui_mouse();
    
    // rjf: mouse y => index
    U64 mouse_y_line_idx = (U64)((mouse.y - text_container_box->rect.y0) / params->line_height_px);
    
    // rjf: index => line num
    S64 line_num = (params->line_num_range.min + mouse_y_line_idx);
    String8 line_string = (params->line_num_range.min <= line_num && line_num <= params->line_num_range.max) ? (params->line_text[mouse_y_line_idx]) : str8_zero();
    
    // rjf: mouse x * string => column
    S64 column = f_char_pos_from_tag_size_string_p(params->font, params->font_size, line_string, mouse.x-text_container_box->rect.x0-params->line_num_width_px)+1;
    
    // rjf: bundle
    mouse_pt = txt_pt(line_num, column);
    
    // rjf: clamp
    if(dim_1s64(params->line_num_range) > 0)
    {
      U64 last_line_size = params->line_text[dim_1s64(params->line_num_range)-1].size;
      TxtRng legal_pt_rng = txt_rng(txt_pt(params->line_num_range.min, 1),
                                    txt_pt(params->line_num_range.max, last_line_size+1));
      if(txt_pt_less_than(mouse_pt, legal_pt_rng.min))
      {
        mouse_pt = legal_pt_rng.min;
      }
      if(txt_pt_less_than(legal_pt_rng.max, mouse_pt))
      {
        mouse_pt = legal_pt_rng.max;
      }
    }
    else
    {
      mouse_pt = txt_pt(1, 1);
    }
    result.mouse_pt = mouse_pt;
  }
  
  //////////////////////////////
  //- rjf: mouse point -> mouse token range, mouse line range
  //
  TxtRng mouse_token_rng = txt_rng(mouse_pt, mouse_pt);
  TxtRng mouse_line_rng = txt_rng(mouse_pt, mouse_pt);
  if(contains_1s64(params->line_num_range, mouse_pt.line))
  {
    TXT_TokenArray *line_tokens = &params->line_tokens[mouse_pt.line-params->line_num_range.min];
    Rng1U64 line_range = params->line_ranges[mouse_pt.line-params->line_num_range.min];
    U64 mouse_pt_off = (mouse_pt.column-1) + line_range.min;
    for(U64 line_token_idx = 0; line_token_idx < line_tokens->count; line_token_idx += 1)
    {
      TXT_Token *line_token = &line_tokens->v[line_token_idx];
      if(contains_1u64(line_token->range, mouse_pt_off))
      {
        mouse_token_rng = txt_rng(txt_pt(mouse_pt.line, 1+line_token->range.min-line_range.min), txt_pt(mouse_pt.line, 1+line_token->range.max-line_range.min));
        break;
      }
    }
    mouse_line_rng = txt_rng(txt_pt(mouse_pt.line, 1), txt_pt(mouse_pt.line, 1+line_range.max));
  }
  
  //////////////////////////////
  //- rjf: interact with margin box & text box
  //
  UI_Signal margin_container_sig = ui_signal_from_box(margin_container_box);
  UI_Signal text_container_sig = ui_signal_from_box(text_container_box);
  DF_Entity *line_drag_entity = &df_g_nil_entity;
  {
    //- rjf: determine mouse drag range
    TxtRng mouse_drag_rng = txt_rng(mouse_pt, mouse_pt);
    if(text_container_sig.f & UI_SignalFlag_LeftTripleDragging)
    {
      mouse_drag_rng = mouse_line_rng;
    }
    else if(text_container_sig.f & UI_SignalFlag_LeftDoubleDragging)
    {
      mouse_drag_rng = mouse_token_rng;
    }
    
    //- rjf: clicking/dragging over the text container
    if(!ctrlified && ui_dragging(text_container_sig))
    {
      if(mouse_pt.line == 0)
      {
        mouse_pt.column = 1;
        if(ui_mouse().y <= top_container_box->rect.y0)
        {
          mouse_pt.line = params->line_num_range.min - 2;
        }
        else if(ui_mouse().y >= top_container_box->rect.y1)
        {
          mouse_pt.line = params->line_num_range.max + 2;
        }
      }
      if(ui_pressed(text_container_sig))
      {
        *cursor = mouse_drag_rng.max;
        *mark = mouse_drag_rng.min;
      }
      if(txt_pt_less_than(mouse_pt, *mark))
      {
        *cursor = mouse_drag_rng.min;
      }
      else
      {
        *cursor = mouse_drag_rng.max;
      }
      *preferred_column = cursor->column;
    }
    
    //- rjf: right-click => active context menu for line
    if(ui_right_clicked(text_container_sig))
    {
      S64 line_idx = mouse_pt.line-params->line_num_range.min;
      if(0 <= line_idx && line_idx < dim_1s64(params->line_num_range))
      {
        ui_ctx_menu_open(ctx_menu_keys[line_idx], ui_key_zero(), sub_2f32(ui_mouse(), v2f32(2, 2)));
        if(txt_pt_match(*cursor, *mark))
        {
          *cursor = *mark = mouse_pt;
        }
      }
    }
    
    //- rjf: hovering text container & ctrl+scroll -> change font size
    if(ui_hovering(text_container_sig))
    {
      for(OS_Event *event = ui_events()->first; event != 0; event = event->next)
      {
        if(os_handle_match(event->window, ui_window()) && event->kind == OS_EventKind_Scroll && event->flags & OS_EventFlag_Ctrl)
        {
          os_eat_event(ui_events(), event);
          if(event->delta.y < 0)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_IncCodeFontScale));
          }
          else if(event->delta.y > 0)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_DecCodeFontScale));
          }
        }
      }
    }
    
    //- rjf: dragging threads, breakpoints, or watch pins over this slice ->
    // drop target
    if(df_drag_is_active() && contains_2f32(clipped_top_container_rect, ui_mouse()))
    {
      DF_DragDropPayload *payload = &df_g_drag_drop_payload;
      DF_Entity *entity = df_entity_from_handle(payload->entity);
      if(entity->kind == DF_EntityKind_Thread ||
         entity->kind == DF_EntityKind_WatchPin ||
         entity->kind == DF_EntityKind_Breakpoint)
      {
        line_drag_entity = entity;
      }
    }
    
    //- rjf: drop target is dropped -> process
    {
      DF_DragDropPayload payload = {0};
      if(!df_entity_is_nil(line_drag_entity) && df_drag_drop(&payload))
      {
        result.dropped_entity = line_drag_entity;
        result.dropped_entity_line_num = mouse_pt.line;
      }
    }
    
    //- rjf: commit text container signal to main output
    result.base = text_container_sig;
  }
  
  //////////////////////////////
  //- rjf: mouse -> expression range info
  //
  if(ui_hovering(text_container_sig) && contains_1s64(params->line_num_range, mouse_pt.line)) ProfScope("mouse -> expression range")
  {
    TxtRng selected_rng = txt_rng(*cursor, *mark);
    if(!txt_pt_match(*cursor, *mark) && cursor->line == mark->line &&
       ((txt_pt_less_than(selected_rng.min, mouse_pt) || txt_pt_match(selected_rng.min, mouse_pt)) &&
        txt_pt_less_than(mouse_pt, selected_rng.max)))
    {
      U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
      String8 line_text = params->line_text[line_slice_idx];
      F32 expr_hoff_px = params->line_num_width_px + f_dim_from_tag_size_string(params->font, params->font_size, str8_prefix(line_text, selected_rng.min.column-1)).x;
      result.mouse_expr_rng = selected_rng;
      result.mouse_expr_baseline_pos = v2f32(text_container_box->rect.x0+expr_hoff_px,
                                             text_container_box->rect.y0+line_slice_idx*params->line_height_px + params->line_height_px*0.85f);
    }
    else
    {
      U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
      String8 line_text = params->line_text[line_slice_idx];
      TXT_TokenArray line_tokens = params->line_tokens[line_slice_idx];
      Rng1U64 line_range = params->line_ranges[line_slice_idx];
      U64 mouse_pt_off = line_range.min + (mouse_pt.column-1);
      Rng1U64 expr_off_rng = txti_expr_range_from_line_off_range_string_tokens(mouse_pt_off, line_range, line_text, &line_tokens);
      if(expr_off_rng.max != expr_off_rng.min)
      {
        F32 expr_hoff_px = params->line_num_width_px + f_dim_from_tag_size_string(params->font, params->font_size, str8_prefix(line_text, expr_off_rng.min-line_range.min)).x;
        result.mouse_expr_rng = txt_rng(txt_pt(mouse_pt.line, 1+(expr_off_rng.min-line_range.min)), txt_pt(mouse_pt.line, 1+(expr_off_rng.max-line_range.min)));
        result.mouse_expr_baseline_pos = v2f32(text_container_box->rect.x0+expr_hoff_px,
                                               text_container_box->rect.y0+line_slice_idx*params->line_height_px + params->line_height_px*0.85f);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: mouse -> set global frontend hovered line info
  //
  if(ui_hovering(text_container_sig) && contains_1s64(params->line_num_range, mouse_pt.line))
  {
    U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
    if(params->line_src2dasm[line_slice_idx].first != 0 &&
       params->line_src2dasm[line_slice_idx].first->v.remap_line == mouse_pt.line)
    {
      df_set_hovered_line_info(params->line_src2dasm[line_slice_idx].first->v.binary, params->line_src2dasm[line_slice_idx].first->v.voff_range.min);
    }
    if(params->line_dasm2src[line_slice_idx].first != 0)
    {
      df_set_hovered_line_info(params->line_dasm2src[line_slice_idx].first->v.binary, params->line_dasm2src[line_slice_idx].first->v.voff_range.min);
    }
  }
  
  //////////////////////////////
  //- rjf: dragging entity which applies to lines over this slice -> visualize
  //
  if(!df_entity_is_nil(line_drag_entity) && contains_2f32(clipped_top_container_rect, ui_mouse()))
  {
    Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_DropSiteOverlay);
    if(line_drag_entity->flags & DF_EntityFlag_HasColor)
    {
      color = df_rgba_from_entity(line_drag_entity);
      color.w /= 2;
    }
    D_Bucket *bucket = d_bucket_make();
    D_BucketScope(bucket)
    {
      Rng2F32 drop_line_rect = r2f32p(top_container_box->rect.x0,
                                      top_container_box->rect.y0 + (mouse_pt.line - params->line_num_range.min) * params->line_height_px,
                                      top_container_box->rect.x1,
                                      top_container_box->rect.y0 + (mouse_pt.line - params->line_num_range.min + 1) * params->line_height_px);
      R_Rect2DInst *inst = d_rect(pad_2f32(drop_line_rect, 8.f), color, 0, 0, 4.f);
      inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(color.x, color.y, color.z, 0);
    }
    ui_box_equip_draw_bucket(text_container_box, bucket);
  }
  
  //////////////////////////////
  //- rjf: (cursor*mark*list(flash_range)) -> list(text_range*color)
  //
  typedef struct TxtRngColorPairNode TxtRngColorPairNode;
  struct TxtRngColorPairNode
  {
    TxtRngColorPairNode *next;
    TxtRng rng;
    Vec4F32 color;
  };
  TxtRngColorPairNode *first_txt_rng_color_pair = 0;
  TxtRngColorPairNode *last_txt_rng_color_pair = 0;
  {
    // rjf: push initial for cursor/mark
    {
      TxtRngColorPairNode *n = push_array(scratch.arena, TxtRngColorPairNode, 1);
      n->rng = txt_rng(*cursor, *mark);
      n->color = ui_top_text_select_color();
      SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, n);
    }
    
    // rjf: push for flash ranges
    for(DF_EntityNode *n = params->flash_ranges.first; n != 0; n = n->next)
    {
      DF_Entity *flash_range = n->entity;
      if(flash_range->flags & DF_EntityFlag_HasTextPoint &&
         flash_range->flags & DF_EntityFlag_HasTextPointAlt)
      {
        TxtRngColorPairNode *pair = push_array(scratch.arena, TxtRngColorPairNode, 1);
        pair->rng = txt_rng(flash_range->text_point, flash_range->text_point_alt);
        pair->color = df_rgba_from_entity(flash_range);
        pair->color.w *= ClampTop(flash_range->life_left, 1.f);
        SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, pair);
      }
    }
    
    // rjf: push for ctrlified mouse expr
    if(ctrlified && !txt_pt_match(result.mouse_expr_rng.max, result.mouse_expr_rng.min))
    {
      TxtRngColorPairNode *n = push_array(scratch.arena, TxtRngColorPairNode, 1);
      n->rng = result.mouse_expr_rng;
      n->color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
      n->color.w *= 0.3f;
      SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, n);
    }
  }
  
  //////////////////////////////
  //- rjf: build line numbers
  //
  if(params->flags & DF_CodeSliceFlag_LineNums) UI_Parent(text_container_box) ProfScope("build line numbers") UI_Focus(UI_FocusKind_Off)
  {
    TxtRng select_rng = txt_rng(*cursor, *mark);
    Vec4F32 inactive_color = df_rgba_from_theme_color(DF_ThemeColor_WeakText);
    Vec4F32 active_color = df_rgba_from_theme_color(DF_ThemeColor_PlainText);
    ui_set_next_pref_width(ui_px(params->line_num_width_px, 1.f));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_flags(UI_BoxFlag_DrawSideRight|UI_BoxFlag_DrawSideLeft);
    UI_Column
      UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      UI_Font(params->font)
      UI_FontSize(params->font_size)
      UI_CornerRadius(0)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        Vec4F32 text_color = (select_rng.min.line <= line_num && line_num <= select_rng.max.line) ? active_color : inactive_color;
        Vec4F32 bg_color = v4f32(0, 0, 0, 0);
        
        // rjf: line info on this line -> adjust bg color to visualize
        B32 has_line_info = 0;
        {
          S64 line_info_line_num = 0;
          F32 line_info_t = 0;
          DF_TextLineSrc2DasmInfoList *src2dasm_list = &params->line_src2dasm[line_idx];
          DF_TextLineDasm2SrcInfoList *dasm2src_list = &params->line_dasm2src[line_idx];
          if(src2dasm_list->first != 0)
          {
            has_line_info = (src2dasm_list->first->v.remap_line == line_num);
            line_info_line_num = line_num;
            line_info_t = selected_thread_module->alive_t;
          }
          if(dasm2src_list->first != 0)
          {
            DF_TextLineDasm2SrcInfo *dasm2src_info = 0;
            U64 best_stamp = 0;
            for(DF_TextLineDasm2SrcInfoNode *n = dasm2src_list->first; n != 0; n = n->next)
            {
              if(n->v.file->timestamp > best_stamp)
              {
                dasm2src_info = &n->v;
                best_stamp = n->v.file->timestamp;
              }
            }
            if(dasm2src_info != 0)
            {
              DF_Entity *binary = dasm2src_info->binary;
              has_line_info = 1;
              line_info_line_num = dasm2src_info->pt.line;
              line_info_t = selected_thread_module->alive_t;
            }
          }
          if(has_line_info)
          {
            Vec4F32 color = code_line_bgs[line_info_line_num % ArrayCount(code_line_bgs)];
            color.w *= line_info_t;
            bg_color = color;
          }
        }
        
        // rjf: build line num box
        ui_set_next_text_color(text_color);
        ui_set_next_background_color(bg_color);
        ui_build_box_from_stringf(UI_BoxFlag_DrawText|(UI_BoxFlag_DrawBackground*!!has_line_info), "%I64u##line_num", line_num);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build line text
  //
  UI_Parent(text_container_box) ProfScope("build line text") UI_Focus(UI_FocusKind_Off)
  {
    DF_Entity *hovered_line_binary = df_get_hovered_line_info_binary();
    U64 hovered_line_voff = df_get_hovered_line_info_voff();
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    UI_WidthFill
      UI_Column
      UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      UI_Font(params->font)
      UI_FontSize(params->font_size)
      UI_CornerRadius(0)
      UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_CodeDefault))
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max; line_num += 1, line_idx += 1)
      {
        String8 line_string = params->line_text[line_idx];
        Rng1U64 line_range = params->line_ranges[line_idx];
        TXT_TokenArray *line_tokens = &params->line_tokens[line_idx];
        ui_set_next_text_padding(-2);
        UI_Key line_key = ui_key_from_stringf(top_container_box->key, "ln_%I64x", line_num);
        Vec4F32 line_bg_color = line_bg_colors[line_idx];
        if(line_bg_color.w != 0)
        {
          ui_set_next_background_color(line_bg_color);
          ui_set_next_flags(UI_BoxFlag_DrawBackground);
        }
        UI_Box *line_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|UI_BoxFlag_DrawText|UI_BoxFlag_DisableIDString, line_key);
        D_Bucket *line_bucket = d_bucket_make();
        d_push_bucket(line_bucket);
        
        // rjf: string * tokens -> fancy string list
        D_FancyStringList line_fancy_strings = {0};
        {
          if(line_tokens->count == 0)
          {
            D_FancyString fstr =
            {
              params->font,
              line_string,
              df_rgba_from_theme_color(DF_ThemeColor_CodeDefault),
              params->font_size,
              0,
              0,
            };
            d_fancy_string_list_push(scratch.arena, &line_fancy_strings, &fstr);
          }
          else
          {
            TXT_Token *line_tokens_first = line_tokens->v;
            TXT_Token *line_tokens_opl = line_tokens->v + line_tokens->count;
            for(TXT_Token *token = line_tokens_first; token < line_tokens_opl; token += 1)
            {
              // rjf: token -> token string
              String8 token_string = {0};
              {
                Rng1U64 token_range = r1u64(0, line_string.size);
                if(token->range.min > line_range.min)
                {
                  token_range.min += token->range.min-line_range.min;
                }
                if(token->range.max < line_range.max)
                {
                  token_range.max = token->range.max-line_range.min;
                }
                token_string = str8_substr(line_string, token_range);
              }
              
              // rjf: token -> token color
              Vec4F32 token_color = df_rgba_from_theme_color(DF_ThemeColor_CodeDefault);
              {
                DF_ThemeColor new_color_kind = df_theme_color_from_txt_token_kind(token->kind);
                F32 mix_t = 1.f;
                if(token->kind == TXT_TokenKind_Identifier || token->kind == TXT_TokenKind_Keyword)
                {
                  B32 mapped_special = 0;
                  for(DF_EntityNode *n = params->relevant_binaries.first; n != 0; n = n->next)
                  {
                    DF_Entity *binary = n->entity;
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 voff = df_voff_from_binary_symbol_name(binary, token_string);
                      if(voff != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = DF_ThemeColor_CodeFunction;
                        mix_t = selected_thread_module->alive_t;
                      }
                    }
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 type_num = df_type_num_from_binary_name(binary, token_string);
                      if(type_num != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = DF_ThemeColor_CodeType;
                        mix_t = selected_thread_module->alive_t;
                      }
                    }
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 local_num = eval_num_from_string(parse_ctx->locals_map, token_string);
                      if(local_num != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = DF_ThemeColor_CodeLocal;
                        mix_t = selected_thread_module->alive_t;
                      }
                    }
                    if(!mapped_special)
                    {
                      U64 reg_num = eval_num_from_string(parse_ctx->regs_map, token_string);
                      if(reg_num != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = DF_ThemeColor_CodeRegister;
                        mix_t = selected_thread_module->alive_t;
                      }
                    }
                    if(!mapped_special)
                    {
                      U64 alias_num = eval_num_from_string(parse_ctx->reg_alias_map, token_string);
                      if(alias_num != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = DF_ThemeColor_CodeRegister;
                        mix_t = selected_thread_module->alive_t;
                      }
                    }
                    break;
                  }
                }
                if(new_color_kind != DF_ThemeColor_Null)
                {
                  Vec4F32 t_color = df_rgba_from_theme_color(new_color_kind);
                  token_color.x += (t_color.x - token_color.x) * mix_t;
                  token_color.y += (t_color.y - token_color.y) * mix_t;
                  token_color.z += (t_color.z - token_color.z) * mix_t;
                  token_color.w += (t_color.w - token_color.w) * mix_t;
                }
              }
              
              // rjf: push fancy string
              D_FancyString fstr =
              {
                params->font,
                token_string,
                token_color,
                params->font_size,
                0,
                0,
              };
              d_fancy_string_list_push(scratch.arena, &line_fancy_strings, &fstr);
            }
          }
        }
        
        // rjf: equip fancy strings to line box
        ui_box_equip_display_fancy_strings(line_box, &line_fancy_strings);
        
        // rjf: extra rendering for strings that are currently being searched for
        if(params->search_query.size != 0)
        {
          for(U64 needle_pos = 0; needle_pos < line_string.size;)
          {
            needle_pos = str8_find_needle(line_string, needle_pos, params->search_query, StringMatchFlag_CaseInsensitive);
            if(needle_pos < line_string.size)
            {
              Rng1U64 match_range = r1u64(needle_pos, needle_pos+params->search_query.size);
              Rng1F32 match_column_pixel_off_range =
              {
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, str8_prefix(line_string, match_range.min)).x,
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, str8_prefix(line_string, match_range.max)).x,
              };
              Rng2F32 match_rect =
              {
                line_box->rect.x0+match_column_pixel_off_range.min,
                line_box->rect.y0,
                line_box->rect.x0+match_column_pixel_off_range.max+2.f,
                line_box->rect.y1,
              };
              Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Highlight0);
              color.w *= 0.8f;
              if(cursor->line == line_num && needle_pos+1 <= cursor->column && cursor->column < needle_pos+params->search_query.size+1)
              {
                color.x += (1.f - color.x) * 0.5f;
                color.y += (1.f - color.y) * 0.5f;
                color.z += (1.f - color.z) * 0.5f;
                color.w += (1.f - color.w) * 0.5f;
              }
              if(!is_focused)
              {
                color.w *= 0.5f;
              }
              d_rect(match_rect, color, 4.f, 0, 1.f);
              needle_pos += 1;
            }
          }
        }
        
        // rjf: extra rendering for list(text_range*color)
        {
          U64 prev_line_size = (line_idx > 0) ? params->line_text[line_idx-1].size : 0;
          U64 next_line_size = (line_idx+1 < dim_1s64(params->line_num_range)) ? params->line_text[line_idx+1].size : 0;
          for(TxtRngColorPairNode *n = first_txt_rng_color_pair; n != 0; n = n->next)
          {
            TxtRng select_range = n->rng;
            TxtRng line_range = txt_rng(txt_pt(line_num, 1), txt_pt(line_num, line_string.size+1));
            TxtRng select_range_in_line = txt_rng_intersect(select_range, line_range);
            if(!txt_pt_match(select_range_in_line.min, select_range_in_line.max) &&
               txt_pt_less_than(select_range_in_line.min, select_range_in_line.max))
            {
              TxtRng prev_line_range = txt_rng(txt_pt(line_num-1, 1), txt_pt(line_num-1, prev_line_size+1));
              TxtRng next_line_range = txt_rng(txt_pt(line_num+1, 1), txt_pt(line_num+1, next_line_size+1));
              TxtRng select_range_in_prev_line = txt_rng_intersect(prev_line_range, select_range);
              TxtRng select_range_in_next_line = txt_rng_intersect(next_line_range, select_range);
              B32 prev_line_good = (!txt_pt_match(select_range_in_prev_line.min, select_range_in_prev_line.max) &&
                                    txt_pt_less_than(select_range_in_prev_line.min, select_range_in_prev_line.max));
              B32 next_line_good = (!txt_pt_match(select_range_in_next_line.min, select_range_in_next_line.max) &&
                                    txt_pt_less_than(select_range_in_next_line.min, select_range_in_next_line.max));
              Rng1S64 select_column_range_in_line =
              {
                (select_range.min.line == line_num) ? select_range.min.column : 1,
                (select_range.max.line == line_num) ? select_range.max.column : (S64)(line_string.size+1),
              };
              Rng1F32 select_column_pixel_off_range =
              {
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, str8_prefix(line_string, select_column_range_in_line.min-1)).x,
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, str8_prefix(line_string, select_column_range_in_line.max-1)).x,
              };
              Rng2F32 select_rect =
              {
                line_box->rect.x0+select_column_pixel_off_range.min,
                floorf(line_box->rect.y0) - 1.f,
                line_box->rect.x0+select_column_pixel_off_range.max+2.f,
                ceilf(line_box->rect.y1) + 1.f,
              };
              Vec4F32 color = n->color;
              if(!is_focused)
              {
                color.w *= 0.5f;
              }
              F32 rounded_radius = params->font_size*0.4f;
              R_Rect2DInst *inst = d_rect(select_rect, color, rounded_radius, 0, 1);
              inst->corner_radii[Corner_00] = !prev_line_good || select_range_in_prev_line.min.column > select_range_in_line.min.column ? rounded_radius : 0.f;
              inst->corner_radii[Corner_10] = (!prev_line_good || select_range_in_line.max.column > select_range_in_prev_line.max.column || select_range_in_line.max.column < select_range_in_prev_line.min.column) ? rounded_radius : 0.f;
              inst->corner_radii[Corner_01] = (!next_line_good || select_range_in_next_line.min.column > select_range_in_line.min.column || select_range_in_next_line.max.column < select_range_in_line.min.column) ? rounded_radius : 0.f;
              inst->corner_radii[Corner_11] = !next_line_good || select_range_in_line.max.column > select_range_in_next_line.max.column ? rounded_radius : 0.f;
            }
          }
        }
        
        // rjf: extra rendering for cursor position
        if(cursor->line == line_num)
        {
          S64 column = cursor->column;
          Vec2F32 advance = f_dim_from_tag_size_string(line_box->font, line_box->font_size, str8_prefix(line_string, column-1));
          F32 cursor_off_pixels = advance.x;
          F32 cursor_thickness = ClampBot(4.f, line_box->font_size/6.f);
          Rng2F32 cursor_rect =
          {
            ui_box_text_position(line_box).x+cursor_off_pixels,
            line_box->rect.y0-params->font_size*0.25f,
            ui_box_text_position(line_box).x+cursor_off_pixels+cursor_thickness,
            line_box->rect.y1+params->font_size*0.25f,
          };
          Vec4F32 color = is_focused ? ui_top_text_cursor_color() : df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
          d_rect(cursor_rect, color, 1.f, 0, 1.f);
        }
        
        // rjf: extra rendering for lines with line-info that match the hovered
        {
          B32 matches = 0;
          S64 line_info_line_num = 0;
          DF_TextLineSrc2DasmInfoList *src2dasm_list = &params->line_src2dasm[line_idx];
          DF_TextLineDasm2SrcInfoList *dasm2src_list = &params->line_dasm2src[line_idx];
          
          // rjf: check src2dasm
          if(src2dasm_list->first != 0)
          {
            for(DF_TextLineSrc2DasmInfoNode *n = src2dasm_list->first; n != 0; n = n->next)
            {
              if(n->v.remap_line == line_num &&
                 n->v.binary == hovered_line_binary &&
                 n->v.voff_range.min <= hovered_line_voff && hovered_line_voff < n->v.voff_range.max)
              {
                matches = 1;
                line_info_line_num = line_num;
                break;
              }
            }
          }
          
          // rjf: check dasm2src
          if(dasm2src_list->first != 0)
          {
            DF_Entity *binary = dasm2src_list->first->v.binary;
            if(binary == hovered_line_binary)
            {
              for(DF_TextLineDasm2SrcInfoNode *n = dasm2src_list->first; n != 0; n = n->next)
              {
                if(n->v.voff_range.min <= hovered_line_voff && hovered_line_voff < n->v.voff_range.max)
                {
                  line_info_line_num = n->v.pt.line;
                  matches = 1;
                  break;
                }
              }
            }
          }
          
          // rjf: matches => highlight background
          if(matches)
          {
            Vec4F32 highlight_color = code_line_bgs[line_info_line_num % ArrayCount(code_line_bgs)];
            highlight_color.w *= 0.25f;
            d_rect(line_box->rect, highlight_color, 0, 0, 0);
          }
        }
        
        // rjf: equip bucket
        if(line_bucket->passes.count != 0)
        {
          ui_box_equip_draw_bucket(line_box, line_bucket);
        }
        
        d_pop_bucket();
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
  return result;
}

internal DF_CodeSliceSignal
df_code_slicef(DF_Window *ws, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, DF_CodeSliceParams *params, TxtPt *cursor, TxtPt *mark, S64 *preferred_column, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  DF_CodeSliceSignal sig = df_code_slice(ws, ctrl_ctx, parse_ctx, params, cursor, mark, preferred_column, string);
  va_end(args);
  scratch_end(scratch);
  return sig;
}

internal B32
df_do_txt_controls(TXT_TextInfo *info, String8 data, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column)
{
  Temp scratch = scratch_begin(0, 0);
  B32 change = 0;
  UI_NavActionList *nav_actions = ui_nav_actions();
  for(UI_NavActionNode *n = nav_actions->first, *next = 0; n != 0; n = next)
  {
    next = n->next;
    B32 taken = 0;
    
    String8 line = txt_string_from_info_data_line_num(info, data, cursor->line);
    UI_NavTxtOp single_line_op = ui_nav_single_line_txt_op_from_action(scratch.arena, n->v, line, *cursor, *mark);
    
    //- rjf: invalid single-line op or endpoint units => try multiline
    if(n->v.delta_unit == UI_NavDeltaUnit_EndPoint || single_line_op.flags & UI_NavTxtOpFlag_Invalid)
    {
      U64 line_count = info->lines_count;
      String8 prev_line = txt_string_from_info_data_line_num(info, data, cursor->line-1);
      String8 next_line = txt_string_from_info_data_line_num(info, data, cursor->line+1);
      Vec2S32 delta = n->v.delta;
      
      //- rjf: wrap lines right
      if(n->v.delta_unit != UI_NavDeltaUnit_EndPoint && delta.x > 0 && cursor->column == line.size+1 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = 1;
        *preferred_column = 1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: wrap lines left
      if(n->v.delta_unit != UI_NavDeltaUnit_EndPoint && delta.x < 0 && cursor->column == 1 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = prev_line.size+1;
        *preferred_column = prev_line.size+1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (plain)
      if(n->v.delta_unit == UI_NavDeltaUnit_Element && delta.y > 0 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = Min(*preferred_column, next_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (plain)
      if(n->v.delta_unit == UI_NavDeltaUnit_Element && delta.y < 0 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = Min(*preferred_column, prev_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (chunk)
      if(n->v.delta_unit == UI_NavDeltaUnit_Chunk && delta.y > 0 && cursor->line+1 <= line_count)
      {
        for(S64 line_num = cursor->line+1; line_num <= line_count; line_num += 1)
        {
          String8 line = txt_string_from_info_data_line_num(info, data, line_num);
          U64 line_size = line.size;
          if(line_size == 0)
          {
            cursor->line = line_num;
            cursor->column = 1;
            break;
          }
          else if(line_num == line_count)
          {
            cursor->line = line_num;
            cursor->column = line_size+1;
          }
        }
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (chunk)
      if(n->v.delta_unit == UI_NavDeltaUnit_Chunk && delta.y < 0 && cursor->line-1 >= 1)
      {
        for(S64 line_num = cursor->line-1; line_num > 0; line_num -= 1)
        {
          String8 line = txt_string_from_info_data_line_num(info, data, line_num);
          U64 line_size = line.size;
          if(line_size == 0)
          {
            cursor->line = line_num;
            cursor->column = 1;
            break;
          }
          else if(line_num == 1)
          {
            cursor->line = line_num;
            cursor->column = 1;
          }
        }
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (page)
      if(n->v.delta_unit == UI_NavDeltaUnit_Whole && delta.y > 0)
      {
        cursor->line += line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (page)
      if(n->v.delta_unit == UI_NavDeltaUnit_Whole && delta.y < 0)
      {
        cursor->line -= line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (+)
      if(n->v.delta_unit == UI_NavDeltaUnit_EndPoint && (delta.y > 0 || delta.x > 0))
      {
        *cursor = txt_pt(line_count, info->lines_count ? dim_1u64(info->lines_ranges[info->lines_count-1])+1 : 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (-)
      if(n->v.delta_unit == UI_NavDeltaUnit_EndPoint && (delta.y < 0 || delta.x < 0))
      {
        *cursor = txt_pt(1, 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: stick mark to cursor, when we don't want to keep it in the same spot
      if(!(n->v.flags & UI_NavActionFlag_KeepMark))
      {
        *mark = *cursor;
      }
    }
    
    //- rjf: valid single-line op => do single-line op
    else
    {
      *cursor = single_line_op.cursor;
      *mark = single_line_op.mark;
      *preferred_column = cursor->column;
      change = 1;
      taken = 1;
    }
    
    //- rjf: copy
    if(n->v.flags & UI_NavActionFlag_Copy)
    {
      String8 text = txt_string_from_info_data_txt_rng(info, data, txt_rng(*cursor, *mark));
      os_set_clipboard_text(text);
    }
    
    //- rjf: consume
    if(taken)
    {
      ui_nav_eat_action_node(nav_actions, n);
    }
  }
  
  scratch_end(scratch);
  return change;
}

internal B32
df_do_txti_controls(TXTI_Handle handle, U64 line_count_per_page, TxtPt *cursor, TxtPt *mark, S64 *preferred_column)
{
  Temp scratch = scratch_begin(0, 0);
  B32 change = 0;
  UI_NavActionList *nav_actions = ui_nav_actions();
  TXTI_BufferInfo buffer_info = txti_buffer_info_from_handle(scratch.arena, handle);
  for(UI_NavActionNode *n = nav_actions->first, *next = 0; n != 0; n = next)
  {
    next = n->next;
    B32 taken = 0;
    
    String8 line = txti_string_from_handle_line_num(scratch.arena, handle, cursor->line);
    UI_NavTxtOp single_line_op = ui_nav_single_line_txt_op_from_action(scratch.arena, n->v, line, *cursor, *mark);
    
    //- rjf: invalid single-line op or endpoint units => try multiline
    if(n->v.delta_unit == UI_NavDeltaUnit_EndPoint || single_line_op.flags & UI_NavTxtOpFlag_Invalid)
    {
      U64 line_count = buffer_info.total_line_count;
      String8 prev_line = txti_string_from_handle_line_num(scratch.arena, handle, cursor->line-1);
      String8 next_line = txti_string_from_handle_line_num(scratch.arena, handle, cursor->line+1);
      Vec2S32 delta = n->v.delta;
      
      //- rjf: wrap lines right
      if(n->v.delta_unit != UI_NavDeltaUnit_EndPoint && delta.x > 0 && cursor->column == line.size+1 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = 1;
        *preferred_column = 1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: wrap lines left
      if(n->v.delta_unit != UI_NavDeltaUnit_EndPoint && delta.x < 0 && cursor->column == 1 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = prev_line.size+1;
        *preferred_column = prev_line.size+1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (plain)
      if(n->v.delta_unit == UI_NavDeltaUnit_Element && delta.y > 0 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = Min(*preferred_column, next_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (plain)
      if(n->v.delta_unit == UI_NavDeltaUnit_Element && delta.y < 0 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = Min(*preferred_column, prev_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (chunk)
      if(n->v.delta_unit == UI_NavDeltaUnit_Chunk && delta.y > 0 && cursor->line+1 <= line_count)
      {
        for(S64 line_num = cursor->line+1; line_num <= line_count; line_num += 1)
        {
          String8 line = txti_string_from_handle_line_num(scratch.arena, handle, line_num);
          U64 line_size = line.size;
          if(line_size == 0)
          {
            cursor->line = line_num;
            cursor->column = 1;
            break;
          }
          else if(line_num == line_count)
          {
            cursor->line = line_num;
            cursor->column = line_size+1;
          }
        }
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (chunk)
      if(n->v.delta_unit == UI_NavDeltaUnit_Chunk && delta.y < 0 && cursor->line-1 >= 1)
      {
        for(S64 line_num = cursor->line-1; line_num > 0; line_num -= 1)
        {
          String8 line = txti_string_from_handle_line_num(scratch.arena, handle, line_num);
          U64 line_size = line.size;
          if(line_size == 0)
          {
            cursor->line = line_num;
            cursor->column = 1;
            break;
          }
          else if(line_num == 1)
          {
            cursor->line = line_num;
            cursor->column = 1;
          }
        }
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (page)
      if(n->v.delta_unit == UI_NavDeltaUnit_Whole && delta.y > 0)
      {
        cursor->line += line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (page)
      if(n->v.delta_unit == UI_NavDeltaUnit_Whole && delta.y < 0)
      {
        cursor->line -= line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (+)
      if(n->v.delta_unit == UI_NavDeltaUnit_EndPoint && (delta.y > 0 || delta.x > 0))
      {
        *cursor = txt_pt(line_count, buffer_info.last_line_size);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (-)
      if(n->v.delta_unit == UI_NavDeltaUnit_EndPoint && (delta.y < 0 || delta.x < 0))
      {
        *cursor = txt_pt(1, 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: stick mark to cursor, when we don't want to keep it in the same spot
      if(!(n->v.flags & UI_NavActionFlag_KeepMark))
      {
        *mark = *cursor;
      }
    }
    
    //- rjf: valid single-line op => do single-line op
    else
    {
      *cursor = single_line_op.cursor;
      *mark = single_line_op.mark;
      *preferred_column = cursor->column;
      change = 1;
      taken = 1;
    }
    
    //- rjf: copy
    if(n->v.flags & UI_NavActionFlag_Copy)
    {
      String8 text = txti_string_from_handle_txt_rng(scratch.arena, handle, txt_rng(*cursor, *mark));
      os_set_clipboard_text(text);
    }
    
    //- rjf: consume
    if(taken)
    {
      ui_nav_eat_action_node(nav_actions, n);
    }
  }
  
  scratch_end(scratch);
  return change;
}

////////////////////////////////
//~ rjf: UI Widgets: Fancy Labels

internal UI_Signal
df_error_label(String8 string)
{
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%S_error_label", string);
  UI_Signal sig = ui_signal_from_box(box);
  UI_Parent(box) UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_FailureBackground))
  {
    ui_set_next_font(ui_icon_font());
    ui_set_next_text_alignment(UI_TextAlign_Center);
    UI_PrefWidth(ui_em(2.25f, 1.f)) ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
    UI_PrefWidth(ui_text_dim(10, 0)) ui_label(string);
  }
  return sig;
}

internal B32
df_help_label(String8 string)
{
  B32 result = 0;
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%S_help_label", string);
  UI_Signal sig = ui_signal_from_box(box);
  UI_Parent(box)
  {
    UI_PrefWidth(ui_pct(1, 0)) ui_label(string);
    if(ui_hovering(sig)) UI_PrefWidth(ui_em(2.25f, 1))
    {
      result = 1;
      ui_set_next_font(ui_icon_font());
      ui_set_next_text_alignment(UI_TextAlign_Center);
      UI_Box *help_hoverer = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawHotEffects, "###help_hoverer_%S", string);
      ui_box_equip_display_string(help_hoverer, df_g_icon_kind_text_table[DF_IconKind_QuestionMark]);
      if(!contains_2f32(help_hoverer->rect, ui_mouse()))
      {
        result = 0;
      }
    }
  }
  return result;
}

internal D_FancyStringList
df_fancy_string_list_from_code_string(Arena *arena, F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string)
{
  Temp scratch = scratch_begin(&arena, 1);
  D_FancyStringList fancy_strings = {0};
  TXT_TokenArray tokens = txt_token_array_from_string__c_cpp(scratch.arena, 0, string);
  TXT_Token *tokens_opl = tokens.v+tokens.count;
  S32 indirection_counter = 0;
  for(TXT_Token *token = tokens.v; token < tokens_opl; token += 1)
  {
    DF_ThemeColor token_color = df_theme_color_from_txt_token_kind(token->kind);
    Vec4F32 token_color_rgba = df_rgba_from_theme_color(token_color);
    token_color_rgba.w *= alpha;
    String8 token_string = str8_substr(string, token->range);
    if(str8_match(token_string, str8_lit("{"), 0)) { indirection_counter += 1; }
    if(str8_match(token_string, str8_lit("["), 0)) { indirection_counter += 1; }
    indirection_counter = ClampBot(0, indirection_counter);
    switch(token->kind)
    {
      default:
      {
        D_FancyString fancy_string =
        {
          ui_top_font(),
          token_string,
          token_color_rgba,
          ui_top_font_size() * (1.f - !!indirection_size_change*(indirection_counter/10.f)),
        };
        d_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
      }break;
      case TXT_TokenKind_Identifier:
      {
        D_FancyString fancy_string =
        {
          ui_top_font(),
          token_string,
          base_color,
          ui_top_font_size() * (1.f - !!indirection_size_change*(indirection_counter/10.f)),
        };
        d_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
      }break;
      case TXT_TokenKind_Numeric:
      {
        Vec4F32 token_color_rgba_alt = token_color_rgba;
        token_color_rgba_alt.x *= 0.7f;
        token_color_rgba_alt.y *= 0.7f;
        token_color_rgba_alt.z *= 0.7f;
        F32 font_size = ui_top_font_size() * (1.f - !!indirection_size_change*(indirection_counter/10.f));
        
        // rjf: unpack string
        U32 base = 10;
        U64 prefix_skip = 0;
        U64 digit_group_size = 3;
        if(str8_match(str8_prefix(token_string, 2), str8_lit("0x"), StringMatchFlag_CaseInsensitive))
        {
          base = 16;
          prefix_skip = 2;
          digit_group_size = 4;
        }
        else if(str8_match(str8_prefix(token_string, 2), str8_lit("0b"), StringMatchFlag_CaseInsensitive))
        {
          base = 2;
          prefix_skip = 2;
          digit_group_size = 8;
        }
        else if(str8_match(str8_prefix(token_string, 2), str8_lit("0o"), StringMatchFlag_CaseInsensitive))
        {
          base = 8;
          prefix_skip = 2;
          digit_group_size = 2;
        }
        
        // rjf: grab string parts
        U64 dot_pos = str8_find_needle(token_string, 0, str8_lit("."), 0);
        String8 prefix = str8_prefix(token_string, prefix_skip);
        String8 whole = str8_substr(token_string, r1u64(prefix_skip, dot_pos));
        String8 decimal = str8_skip(token_string, dot_pos);
        
        // rjf: determine # of digits
        U64 num_digits = 0;
        for(U64 idx = 0; idx < whole.size; idx += 1)
        {
          num_digits += char_is_digit(whole.str[idx], base);
        }
        
        // rjf: push prefix
        {
          D_FancyString fancy_string =
          {
            ui_top_font(),
            prefix,
            token_color_rgba,
            font_size,
          };
          d_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
        }
        
        // rjf: push digit groups
        {
          B32 odd = 0;
          U64 start_idx = 0;
          U64 num_digits_passed = digit_group_size - num_digits%digit_group_size;
          for(U64 idx = 0; idx <= whole.size; idx += 1)
          {
            U8 byte = idx < whole.size ? whole.str[idx] : 0;
            if(num_digits_passed >= digit_group_size || idx == whole.size)
            {
              num_digits_passed = 0;
              if(start_idx < idx)
              {
                D_FancyString fancy_string =
                {
                  ui_top_font(),
                  str8_substr(whole, r1u64(start_idx, idx)),
                  odd ? token_color_rgba_alt : token_color_rgba,
                  font_size,
                };
                d_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
                start_idx = idx;
                odd ^= 1;
              }
            }
            if(char_is_digit(byte, base))
            {
              num_digits_passed += 1;
            }
          }
        }
        
        // rjf: push decimal
        {
          D_FancyString fancy_string =
          {
            ui_top_font(),
            decimal,
            token_color_rgba,
            font_size,
          };
          d_fancy_string_list_push(arena, &fancy_strings, &fancy_string);
        }
        
      }break;
    }
    if(str8_match(token_string, str8_lit("}"), 0)) { indirection_counter -= 1; }
    if(str8_match(token_string, str8_lit("]"), 0)) { indirection_counter -= 1; }
    indirection_counter = ClampBot(0, indirection_counter);
  }
  scratch_end(scratch);
  return fancy_strings;
}

internal UI_Box *
df_code_label(F32 alpha, B32 indirection_size_change, Vec4F32 base_color, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  D_FancyStringList fancy_strings = df_fancy_string_list_from_code_string(scratch.arena, alpha, indirection_size_change, base_color, string);
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
  ui_box_equip_display_fancy_strings(box, &fancy_strings);
  scratch_end(scratch);
  return box;
}

////////////////////////////////
//~ rjf: UI Widgets: Line Edit

internal UI_Signal
df_line_edit(DF_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, String8 string)
{
  //- rjf: unpack visual metrics
  F32 expander_size_px = ui_top_font_size()*1.3f;
  
  //- rjf: make key
  UI_Key key = ui_key_from_string(ui_active_seed_key(), string);
  
  //- rjf: calculate & push focus
  B32 is_auto_focus_hot = ui_is_key_auto_focus_hot(key);
  B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
  if(is_auto_focus_hot) { ui_push_focus_hot(UI_FocusKind_On); }
  if(is_auto_focus_active) { ui_push_focus_active(UI_FocusKind_On); }
  B32 is_focus_hot    = ui_is_focus_hot();
  B32 is_focus_active = ui_is_focus_active();
  B32 is_focus_hot_disabled = (!is_focus_hot && ui_top_focus_hot() == UI_FocusKind_On);
  B32 is_focus_active_disabled = (!is_focus_active && ui_top_focus_active() == UI_FocusKind_On);
  
  //- rjf: build top-level box
  if(is_focus_active || is_focus_active_disabled)
  {
    ui_set_next_hover_cursor(OS_Cursor_IBar);
  }
  UI_Box *box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                      UI_BoxFlag_ClickToFocus|
                                      UI_BoxFlag_DrawHotEffects|
                                      (!(flags & DF_LineEditFlag_NoBackground)*UI_BoxFlag_DrawBackground)|
                                      (!!(flags & DF_LineEditFlag_Border)*UI_BoxFlag_DrawBorder)|
                                      ((is_auto_focus_hot || is_auto_focus_active)*UI_BoxFlag_KeyboardClickable)|
                                      (is_focus_active || is_focus_active_disabled)*(UI_BoxFlag_Clip),
                                      key);
  
  //- rjf: build indent
  if(depth != 0) UI_Parent(box)
  {
    ui_spacer(ui_em(1.5f*depth, 1.f));
  }
  
  //- rjf: build expander
  if(flags & DF_LineEditFlag_Expander) UI_PrefWidth(ui_px(expander_size_px, 1.f)) UI_Parent(box)
    UI_Flags(UI_BoxFlag_DrawSideLeft)
    UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
    UI_Focus(UI_FocusKind_Off)
  {
    UI_Signal expander_sig = ui_expanderf(*expanded_out, "expander");
    if(ui_pressed(expander_sig))
    {
      *expanded_out ^= 1;
    }
  }
  
  //- rjf: build expander placeholder
  else if(flags & DF_LineEditFlag_ExpanderPlaceholder) UI_Parent(box) UI_PrefWidth(ui_px(expander_size_px, 1.f)) UI_Focus(UI_FocusKind_Off)
  {
    UI_TextColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      UI_Flags(UI_BoxFlag_DrawSideLeft)
      UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      UI_Font(ui_icon_font())
      ui_label(df_g_icon_kind_text_table[DF_IconKind_Dot]);
  }
  
  //- rjf: build expander space
  else if(flags & DF_LineEditFlag_ExpanderSpace) UI_Parent(box) UI_Focus(UI_FocusKind_Off)
  {
    UI_Flags(UI_BoxFlag_DrawSideLeft)
      UI_BorderColor(df_rgba_from_theme_color(DF_ThemeColor_WeakText))
      ui_spacer(ui_px(expander_size_px, 1.f));
  }
  
  //- rjf: build scrollable container box
  UI_Box *scrollable_box = &ui_g_nil_box;
  UI_Parent(box) UI_PrefWidth(ui_children_sum(0))
  {
    scrollable_box = ui_build_box_from_stringf(is_focus_active*(UI_BoxFlag_AllowOverflowX), "scroll_box_%p", edit_buffer);
  }
  
  //- rjf: do non-textual edits (delete, copy, cut)
  B32 commit = 0;
  if(!is_focus_active && is_focus_hot)
  {
    UI_NavActionList *nav_actions = ui_nav_actions();
    for(UI_NavActionNode *n = nav_actions->first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      UI_NavAction *action = &n->v;
      if(action->flags & UI_NavActionFlag_Copy)
      {
        os_set_clipboard_text(pre_edit_value);
      }
      if(action->flags & UI_NavActionFlag_Delete)
      {
        commit = 1;
        edit_string_size_out[0] = 0;
      }
    }
  }
  
  //- rjf: get signal
  UI_Signal sig = ui_signal_from_box(box);
  if(commit)
  {
    sig.f |= UI_SignalFlag_Commit;
  }
  
  //- rjf: do start/end editing interaction
  B32 focus_started = 0;
  if(!is_focus_active)
  {
    B32 start_editing_via_sig = (ui_double_clicked(sig) || sig.f&UI_SignalFlag_KeyboardPressed);
    B32 start_editing_via_typing = 0;
    if(is_focus_hot)
    {
      UI_NavActionList *nav_actions = ui_nav_actions();
      for(UI_NavActionNode *n = nav_actions->first; n != 0; n = n->next)
      {
        if(n->v.insertion.size != 0 || n->v.flags & UI_NavActionFlag_Paste)
        {
          start_editing_via_typing = 1;
          break;
        }
      }
    }
    if(is_focus_hot && os_key_press(ui_events(), ui_window(), 0, OS_Key_F2))
    {
      start_editing_via_typing = 1;
    }
    if(start_editing_via_sig || start_editing_via_typing)
    {
      String8 edit_string = pre_edit_value;
      edit_string.size = Min(edit_buffer_size, pre_edit_value.size);
      MemoryCopy(edit_buffer, edit_string.str, edit_string.size);
      edit_string_size_out[0] = edit_string.size;
      ui_set_auto_focus_active_key(key);
      ui_kill_action();
      *cursor = txt_pt(1, edit_string.size+1);
      *mark = txt_pt(1, 1);
      focus_started = 1;
    }
  }
  else if(is_focus_active && sig.f&UI_SignalFlag_KeyboardPressed)
  {
    ui_set_auto_focus_active_key(ui_key_zero());
    sig.f |= UI_SignalFlag_Commit;
  }
  
  //- rjf: take navigation actions for editing
  B32 changes_made = 0;
  if(!(flags & DF_LineEditFlag_DisableEdit) && (is_focus_active || focus_started))
  {
    Temp scratch = scratch_begin(0, 0);
    UI_NavActionList *nav_actions = ui_nav_actions();
    for(UI_NavActionNode *n = nav_actions->first, *next = 0; n != 0; n = next)
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      next = n->next;
      
      // rjf: do not consume anything that doesn't fit a single-line's operations
      if(n->v.delta.y != 0)
      {
        continue;
      }
      
      // rjf: map this action to an op
      UI_NavTxtOp op = ui_nav_single_line_txt_op_from_action(scratch.arena, n->v, edit_string, *cursor, *mark);
      
      // rjf: perform replace range
      if(!txt_pt_match(op.range.min, op.range.max) || op.replace.size != 0)
      {
        String8 new_string = ui_nav_push_string_replace_range(scratch.arena, edit_string, r1s64(op.range.min.column, op.range.max.column), op.replace);
        new_string.size = Min(edit_buffer_size, new_string.size);
        MemoryCopy(edit_buffer, new_string.str, new_string.size);
        edit_string_size_out[0] = new_string.size;
      }
      
      // rjf: perform copy
      if(op.flags & UI_NavTxtOpFlag_Copy)
      {
        os_set_clipboard_text(op.copy);
      }
      
      // rjf: commit op's changed cursor & mark to caller-provided state
      *cursor = op.cursor;
      *mark = op.mark;
      
      // rjf: consume event
      {
        ui_nav_eat_action_node(nav_actions, n);
        changes_made = 1;
      }
    }
    scratch_end(scratch);
  }
  
  //- rjf: build scrolled contents
  TxtPt mouse_pt = {0};
  F32 cursor_off = 0;
  UI_Parent(scrollable_box)
  {
    if(!is_focus_active && !is_focus_active_disabled && flags & DF_LineEditFlag_CodeContents)
    {
      String8 display_string = ui_display_part_from_key_string(string);
      if(!(flags & DF_LineEditFlag_PreferDisplayString) && pre_edit_value.size != 0)
      {
        display_string = pre_edit_value;
        UI_Box *box = df_code_label(1.f, 1, ui_top_text_color(), display_string);
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
      else if(flags & DF_LineEditFlag_DisplayStringIsCode)
      {
        UI_Box *box = df_code_label(1.f, 1, ui_top_text_color(), display_string);
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
      else
      {
        ui_set_next_text_color(df_rgba_from_theme_color(DF_ThemeColor_WeakText));
        UI_Box *box = ui_label(display_string).box;
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
    }
    else if(!is_focus_active && !is_focus_active_disabled && !(flags & DF_LineEditFlag_CodeContents))
    {
      String8 display_string = ui_display_part_from_key_string(string);
      if(!(flags & DF_LineEditFlag_PreferDisplayString) && pre_edit_value.size != 0)
      {
        display_string = pre_edit_value;
      }
      else
      {
        ui_set_next_text_color(df_rgba_from_theme_color(DF_ThemeColor_WeakText));
      }
      UI_Box *box = ui_label(display_string).box;
      if(matches != 0)
      {
        ui_box_equip_fuzzy_match_ranges(box, matches);
      }
    }
    else if((is_focus_active || is_focus_active_disabled) && flags & DF_LineEditFlag_CodeContents)
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      Temp scratch = scratch_begin(0, 0);
      F32 total_text_width = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), edit_string).x;
      F32 total_editstr_width = total_text_width - !!(flags & (DF_LineEditFlag_Expander|DF_LineEditFlag_ExpanderSpace|DF_LineEditFlag_ExpanderPlaceholder)) * expander_size_px;
      ui_set_next_pref_width(ui_px(total_editstr_width+ui_top_font_size()*2, 0.f));
      UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc, "###editstr");
      D_FancyStringList code_fancy_strings = df_fancy_string_list_from_code_string(scratch.arena, 1.f, 0, ui_top_text_color(), edit_string);
      ui_box_equip_display_fancy_strings(editstr_box, &code_fancy_strings);
      UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
      draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
      draw_data->cursor = *cursor;
      draw_data->mark = *mark;
      draw_data->cursor_color = is_focus_active ? ui_top_text_cursor_color() : df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
      draw_data->select_color = ui_top_text_select_color();
      ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
      mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
      cursor_off = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), str8_prefix(edit_string, cursor->column-1)).x;
      scratch_end(scratch);
    }
    else if((is_focus_active || is_focus_active_disabled) && !(flags & DF_LineEditFlag_CodeContents))
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      F32 total_text_width = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), edit_string).x;
      F32 total_editstr_width = total_text_width - !!(flags & (DF_LineEditFlag_Expander|DF_LineEditFlag_ExpanderSpace|DF_LineEditFlag_ExpanderPlaceholder)) * expander_size_px;
      ui_set_next_pref_width(ui_px(total_editstr_width+ui_top_font_size()*2, 0.f));
      UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc, "###editstr");
      UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
      draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
      draw_data->cursor = *cursor;
      draw_data->mark = *mark;
      draw_data->cursor_color = is_focus_active ? ui_top_text_cursor_color() : df_rgba_from_theme_color(DF_ThemeColor_FailureBackground);
      draw_data->select_color = ui_top_text_select_color();
      ui_box_equip_display_string(editstr_box, edit_string);
      ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
      mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
      cursor_off = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), str8_prefix(edit_string, cursor->column-1)).x;
    }
  }
  
  //- rjf: click+drag
  if(is_focus_active && ui_dragging(sig))
  {
    if(ui_pressed(sig))
    {
      *mark = mouse_pt;
    }
    *cursor = mouse_pt;
  }
  if(!is_focus_active && is_focus_active_disabled && ui_pressed(sig))
  {
    *cursor = *mark = mouse_pt;
  }
  
  //- rjf: focus cursor
  {
    F32 visible_dim_px = dim_2f32(box->rect).x;
    if(visible_dim_px != 0)
    {
      Rng1F32 cursor_range_px  = r1f32(cursor_off-ui_top_font_size()*2.f, cursor_off+ui_top_font_size()*2.f);
      Rng1F32 visible_range_px = r1f32(scrollable_box->view_off_target.x, scrollable_box->view_off_target.x + visible_dim_px);
      cursor_range_px.min = ClampBot(0, cursor_range_px.min);
      cursor_range_px.max = ClampBot(0, cursor_range_px.max);
      F32 min_delta = cursor_range_px.min-visible_range_px.min;
      F32 max_delta = cursor_range_px.max-visible_range_px.max;
      min_delta = Min(min_delta, 0);
      max_delta = Max(max_delta, 0);
      scrollable_box->view_off_target.x += min_delta;
      scrollable_box->view_off_target.x += max_delta;
    }
    if(!is_focus_active && !is_focus_active_disabled)
    {
      scrollable_box->view_off_target.x = scrollable_box->view_off.x = 0;
    }
  }
  
  //- rjf: pop focus
  if(is_auto_focus_hot) { ui_pop_focus_hot(); }
  if(is_auto_focus_active) { ui_pop_focus_active(); }
  
  return sig;
}

internal UI_Signal
df_line_editf(DF_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = df_line_edit(flags, depth, matches, cursor, mark, edit_buffer, edit_buffer_size, edit_string_size_out, expanded_out, pre_edit_value, string);
  scratch_end(scratch);
  return sig;
}

////////////////////////////////
//~ rjf: Continuous Frame Requests

internal void
df_gfx_request_frame(void)
{
  df_gfx_state->num_frames_requested = 4;
}

////////////////////////////////
//~ rjf: Main Layer Top-Level Calls

internal void
df_gfx_init(OS_WindowRepaintFunctionType *window_repaint_entry_point, DF_StateDeltaHistory *hist)
{
  ProfBeginFunction();
  Arena *arena = arena_alloc();
  df_gfx_state = push_array(arena, DF_GfxState, 1);
  df_gfx_state->arena = arena;
  df_gfx_state->num_frames_requested = 2;
  df_gfx_state->hist = hist;
  df_gfx_state->key_map_arena = arena_alloc();
  df_gfx_state->confirm_arena = arena_alloc();
  df_gfx_state->view_spec_table_size = 256;
  df_gfx_state->view_spec_table = push_array(arena, DF_ViewSpec *, df_gfx_state->view_spec_table_size);
  df_gfx_state->view_rule_spec_table_size = 1024;
  df_gfx_state->view_rule_spec_table = push_array(arena, DF_GfxViewRuleSpec *, df_state->view_rule_spec_table_size);
  df_gfx_state->view_rule_block_slots_count = 1024;
  df_gfx_state->view_rule_block_slots = push_array(arena, DF_ViewRuleBlockSlot, df_gfx_state->view_rule_block_slots_count);
  df_gfx_state->string_search_arena = arena_alloc();
  df_gfx_state->repaint_hook = window_repaint_entry_point;
  df_gfx_state->cfg_main_font_path_arena = arena_alloc();
  df_gfx_state->cfg_code_font_path_arena = arena_alloc();
  df_clear_bindings();
  
  // rjf: register gfx layer views
  {
    DF_ViewSpecInfoArray array = {df_g_gfx_view_kind_spec_info_table, ArrayCount(df_g_gfx_view_kind_spec_info_table)};
    df_register_view_specs(array);
  }
  
  // rjf: register gfx layer view rules
  {
    DF_GfxViewRuleSpecInfoArray array = {df_g_gfx_view_rule_spec_info_table, ArrayCount(df_g_gfx_view_rule_spec_info_table)};
    df_register_gfx_view_rule_specs(array);
  }
  
  // rjf: register cmd param slot -> view specs
  {
    for(U64 idx = 0; idx < ArrayCount(df_g_cmd_param_slot_2_view_spec_src_map); idx += 1)
    {
      DF_CmdParamSlot slot = df_g_cmd_param_slot_2_view_spec_src_map[idx];
      String8 view_spec_name = df_g_cmd_param_slot_2_view_spec_dst_map[idx];
      String8 cmd_spec_name = df_g_cmd_param_slot_2_view_spec_cmd_map[idx];
      DF_ViewSpec *view_spec = df_view_spec_from_string(view_spec_name);
      DF_CmdSpec *cmd_spec = cmd_spec_name.size != 0 ? df_cmd_spec_from_string(cmd_spec_name) : &df_g_nil_cmd_spec;
      DF_CmdParamSlotViewSpecRuleNode *n = push_array(df_gfx_state->arena, DF_CmdParamSlotViewSpecRuleNode, 1);
      n->view_spec = view_spec;
      n->cmd_spec = cmd_spec;
      SLLQueuePush(df_gfx_state->cmd_param_slot_view_spec_table[slot].first, df_gfx_state->cmd_param_slot_view_spec_table[slot].last, n);
      df_gfx_state->cmd_param_slot_view_spec_table[slot].count += 1;
    }
  }
  
  ProfEnd();
}

internal void
df_gfx_begin_frame(Arena *arena, DF_CmdList *cmds)
{
  ProfBeginFunction();
  df_gfx_state->hover_line_set_this_frame = 0;
  
  //- rjf: animate confirmation
  {
    F32 rate = 1 - pow_f32(2, (-10.f * df_dt()));
    B32 confirm_open = df_gfx_state->confirm_active;
    df_gfx_state->confirm_t += rate * ((F32)!!confirm_open-df_gfx_state->confirm_t);
    if(abs_f32(df_gfx_state->confirm_t - (F32)!!confirm_open) > 0.005f)
    {
      df_gfx_request_frame();
    }
  }
  
  //- rjf: capture is active? -> keep rendering
  if(ProfIsCapturing())
  {
    df_gfx_request_frame();
  }
  
  //- rjf: process top-level graphical commands
  {
    B32 cfg_write_done[DF_CfgSrc_COUNT] = {0};
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
      
      // rjf: request frame
      df_gfx_request_frame();
      
      // rjf: process command
      DF_CfgSrc cfg_src = (DF_CfgSrc)0;
      switch(core_cmd_kind)
      {
        default:{}break;
        
        //- rjf: exiting
        case DF_CoreCmdKind_Exit:
        {
          // rjf: save
          {
            DF_CmdParams params = df_cmd_params_zero();
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteUserData));
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProfileData));
            df_gfx_state->last_window_queued_save = 1;
          }
          
          // rjf: close all windows
          for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
          {
            DF_CmdParams params = df_cmd_params_from_window(window);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
          }
        }break;
        
        //- rjf: errors
        case DF_CoreCmdKind_Error:
        {
          DF_Window *window = df_window_from_handle(params.window);
          if(window == 0)
          {
            for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
            {
              DF_CmdParams p = df_cmd_params_from_window(w);
              p.string = push_str8_copy(arena, params.string);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
            }
          }
        }break;
        
        //- rjf: windows
        case DF_CoreCmdKind_OpenWindow:
        {
          OS_Handle preferred_monitor = {0};
          df_window_open(v2f32(1280, 720), preferred_monitor, DF_CfgSrc_User);
        }break;
        case DF_CoreCmdKind_CloseWindow:
        {
          DF_Window *ws = df_window_from_handle(params.window);
          if(ws != 0)
          {
            DF_EntityList running_processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
            
            // NOTE(rjf): if this is the last window, and targets are running, but
            // this command is not force-confirmed, then we should query the user
            // to ensure they want to close the debugger before exiting
            UI_Key key = ui_key_from_string(ui_key_zero(), str8_lit("lossy_exit_confirmation"));
            if(!ui_key_match(key, df_gfx_state->confirm_key) && running_processes.count != 0 && ws == df_gfx_state->first_window && ws == df_gfx_state->last_window && !params.force_confirm)
            {
              df_gfx_state->confirm_key = key;
              df_gfx_state->confirm_active = 1;
              arena_clear(df_gfx_state->confirm_arena);
              MemoryZeroStruct(&df_gfx_state->confirm_cmds);
              df_gfx_state->confirm_title = push_str8f(df_gfx_state->confirm_arena, "Are you sure you want to exit?");
              df_gfx_state->confirm_msg = push_str8f(df_gfx_state->confirm_arena, "The debugger is still attached to %slive process%s.",
                                                     running_processes.count == 1 ? "a " : "",
                                                     running_processes.count == 1 ? ""   : "es");
              DF_CmdParams p = df_cmd_params_from_window(ws);
              p.force_confirm = 1;
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_ForceConfirm);
              df_cmd_list_push(df_gfx_state->confirm_arena, &df_gfx_state->confirm_cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
            }
            
            // NOTE(rjf): if this is the last window, and it is being closed, then
            // we need to auto-save, and provide one last chance to process saving
            // commands. after doing so, we can retry.
            else if(ws == df_gfx_state->first_window && ws == df_gfx_state->last_window && df_gfx_state->last_window_queued_save == 0)
            {
              df_gfx_state->last_window_queued_save = 1;
              {
                DF_CmdParams params = df_cmd_params_zero();
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteUserData));
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProfileData));
              }
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
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
              
              df_state_delta_history_release(ws->view_state_hist);
              ui_state_release(ws->ui);
              DLLRemove(df_gfx_state->first_window, df_gfx_state->last_window, ws);
              r_window_unequip(ws->os, ws->r);
              os_window_close(ws->os);
              arena_release(ws->query_cmd_arena);
              arena_release(ws->hover_eval_arena);
              arena_release(ws->arena);
              SLLStackPush(df_gfx_state->free_window, ws);
              ws->gen += 1;
            }
          }
        }break;
        case DF_CoreCmdKind_ToggleFullscreen:
        {
          DF_Window *window = df_window_from_handle(params.window);
          if(window != 0)
          {
            os_window_set_fullscreen(window->os, !os_window_is_fullscreen(window->os));
          }
        }break;
        
        //- rjf: confirmations
        case DF_CoreCmdKind_ConfirmAccept:
        {
          df_gfx_state->confirm_active = 0;
          df_gfx_state->confirm_key = ui_key_zero();
          for(DF_CmdNode *n = df_gfx_state->confirm_cmds.first; n != 0; n = n->next)
          {
            df_push_cmd__root(&n->cmd.params, n->cmd.spec);
          }
        }break;
        case DF_CoreCmdKind_ConfirmCancel:
        {
          df_gfx_state->confirm_active = 0;
          df_gfx_state->confirm_key = ui_key_zero();
        }break;
        
        //- rjf: commands with implications for graphical systems, but generated
        // without context needed - pick selected window & dispatch
        case DF_CoreCmdKind_SelectThread:
        case DF_CoreCmdKind_SelectThreadView:
        case DF_CoreCmdKind_SelectThreadWindow:
        case DF_CoreCmdKind_FindThread:
        {
          DF_Window *window = df_window_from_handle(params.window);
          if(window == 0)
          {
            window = df_gfx_state->first_window;
            for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
            {
              if(os_window_is_focused(w->os))
              {
                window = w;
              }
            }
            if(window != 0)
            {
              os_window_bring_to_front(window->os);
              os_window_focus(window->os);
              DF_CmdParams p = params;
              p.window = df_handle_from_window(window);
              p.panel = df_handle_from_panel(window->focused_panel);
              p.view = df_handle_from_view(df_view_from_handle(window->focused_panel->selected_tab_view));
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
              df_cmd_list_push(arena, cmds, &p, cmd->spec);
            }
          }
        }break;
        
        //- rjf: loading/applying stateful config changes
        case DF_CoreCmdKind_ApplyUserData:
        case DF_CoreCmdKind_ApplyProfileData:
        {
          DF_CfgTable *table = df_cfg_table();
          OS_HandleArray monitors = os_push_monitors_array(scratch.arena);
          
          //- rjf: get src
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
          
          //- rjf: eliminate all windows
          for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
          {
            if(window->cfg_src != src)
            {
              continue;
            }
            DF_CmdParams params = df_cmd_params_from_window(window);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
          }
          
          //- rjf: apply fonts
          {
            F_Tag defaults[DF_FontSlot_COUNT] =
            {
              f_tag_from_static_data_string(&df_g_default_main_font_bytes),
              f_tag_from_static_data_string(&df_g_default_code_font_bytes),
              f_tag_from_static_data_string(&df_g_icon_font_bytes),
            };
            MemoryZeroArray(df_gfx_state->cfg_font_tags);
            {
              DF_CfgVal *code_font_val = df_cfg_val_from_string(table, str8_lit("code_font"));
              DF_CfgVal *main_font_val = df_cfg_val_from_string(table, str8_lit("main_font"));
              DF_CfgNode *code_font_cfg = code_font_val->last;
              DF_CfgNode *main_font_cfg = main_font_val->last;
              String8 code_font_relative_path = code_font_cfg->first->string;
              String8 main_font_relative_path = main_font_cfg->first->string;
              if(code_font_cfg != &df_g_nil_cfg_node)
              {
                arena_clear(df_gfx_state->cfg_code_font_path_arena);
                df_gfx_state->cfg_code_font_path = push_str8_copy(df_gfx_state->cfg_code_font_path_arena, code_font_relative_path);
              }
              if(main_font_cfg != &df_g_nil_cfg_node)
              {
                arena_clear(df_gfx_state->cfg_main_font_path_arena);
                df_gfx_state->cfg_main_font_path = push_str8_copy(df_gfx_state->cfg_main_font_path_arena, main_font_relative_path);
              }
              String8 code_font_path = path_absolute_dst_from_relative_dst_src(scratch.arena, code_font_relative_path, cfg_folder);
              String8 main_font_path = path_absolute_dst_from_relative_dst_src(scratch.arena, main_font_relative_path, cfg_folder);
              if(os_file_path_exists(code_font_path) && code_font_cfg != &df_g_nil_cfg_node && code_font_relative_path.size != 0)
              {
                df_gfx_state->cfg_font_tags[DF_FontSlot_Code] = f_tag_from_path(code_font_path);
              }
              if(os_file_path_exists(main_font_path) && main_font_cfg != &df_g_nil_cfg_node && main_font_relative_path.size != 0)
              {
                df_gfx_state->cfg_font_tags[DF_FontSlot_Main] = f_tag_from_path(main_font_path);
              }
            }
            for(DF_FontSlot slot = (DF_FontSlot)0; slot < DF_FontSlot_COUNT; slot = (DF_FontSlot)(slot+1))
            {
              if(f_tag_match(f_tag_zero(), df_gfx_state->cfg_font_tags[slot]))
              {
                df_gfx_state->cfg_font_tags[slot] = defaults[slot];
              }
            }
          }
          
          //- rjf: build windows & panel layouts
          DF_CfgVal *windows = df_cfg_val_from_string(table, str8_lit("window"));
          for(DF_CfgNode *window_node = windows->first;
              window_node != &df_g_nil_cfg_node;
              window_node = window_node->next)
          {
            // rjf: skip wrong source
            if(window_node->source != src)
            {
              continue;
            }
            
            // rjf: grab metadata
            B32 is_fullscreen = 0;
            B32 is_maximized = 0;
            Axis2 top_level_split_axis = Axis2_X;
            OS_Handle preferred_monitor = os_primary_monitor();
            Vec2F32 size = {0};
            F32 code_font_size_delta = 0.f;
            F32 main_font_size_delta = 0.f;
            {
              for(DF_CfgNode *n = window_node->first; n != &df_g_nil_cfg_node; n = n->next)
              {
                if(n->flags & DF_CfgNodeFlag_Identifier &&
                   n->first == &df_g_nil_cfg_node &&
                   str8_match(n->string, str8_lit("split_x"), StringMatchFlag_CaseInsensitive))
                {
                  top_level_split_axis = Axis2_X;
                }
                if(n->flags & DF_CfgNodeFlag_Identifier &&
                   n->first == &df_g_nil_cfg_node &&
                   str8_match(n->string, str8_lit("split_y"), StringMatchFlag_CaseInsensitive))
                {
                  top_level_split_axis = Axis2_Y;
                }
                if(n->flags & DF_CfgNodeFlag_Identifier &&
                   n->first == &df_g_nil_cfg_node &&
                   str8_match(n->string, str8_lit("fullscreen"), StringMatchFlag_CaseInsensitive))
                {
                  is_fullscreen = 1;
                }
                if(n->flags & DF_CfgNodeFlag_Identifier &&
                   n->first == &df_g_nil_cfg_node &&
                   str8_match(n->string, str8_lit("maximized"), StringMatchFlag_CaseInsensitive))
                {
                  is_maximized = 1;
                }
              }
              DF_CfgNode *monitor_cfg = df_cfg_node_child_from_string(window_node, str8_lit("monitor"), StringMatchFlag_CaseInsensitive);
              String8 preferred_monitor_name = monitor_cfg->first->string;
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
              DF_CfgNode *size_cfg = df_cfg_node_child_from_string(window_node, str8_lit("size"), StringMatchFlag_CaseInsensitive);
              {
                String8 x_string = size_cfg->first->string;
                String8 y_string = size_cfg->first->next->string;
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
              DF_CfgNode *code_font_size_delta_cfg = df_cfg_node_child_from_string(window_node, str8_lit("code_font_size_delta"), StringMatchFlag_CaseInsensitive);
              DF_CfgNode *main_font_size_delta_cfg = df_cfg_node_child_from_string(window_node, str8_lit("main_font_size_delta"), StringMatchFlag_CaseInsensitive);
              String8 code_font_size_delta_cfg_string = df_string_from_cfg_node_children(scratch.arena, code_font_size_delta_cfg);
              String8 main_font_size_delta_cfg_string = df_string_from_cfg_node_children(scratch.arena, main_font_size_delta_cfg);
              code_font_size_delta = (F32)f64_from_str8(code_font_size_delta_cfg_string);
              main_font_size_delta = (F32)f64_from_str8(main_font_size_delta_cfg_string);
            }
            
            // rjf: open window
            DF_Window *ws = df_window_open(size, preferred_monitor, window_node->source);
            ws->code_font_size_delta = code_font_size_delta;
            ws->main_font_size_delta = main_font_size_delta;
            
            // rjf: build panel tree
            DF_CfgNode *cfg_panels = df_cfg_node_child_from_string(window_node, str8_lit("panels"), StringMatchFlag_CaseInsensitive);
            DF_Panel *panel_parent = ws->root_panel;
            panel_parent->split_axis = top_level_split_axis;
            DF_CfgNodeRec rec = {0};
            for(DF_CfgNode *n = cfg_panels, *next = &df_g_nil_cfg_node;
                n != &df_g_nil_cfg_node;
                n = next)
            {
              // rjf: assume we're just moving to the next one initially...
              next = n->next;
              
              // rjf: grab root panel
              DF_Panel *panel = &df_g_nil_panel;
              if(n == cfg_panels)
              {
                panel = ws->root_panel;
                panel->size_pct_of_parent.v[panel_parent->split_axis] = panel->size_pct_of_parent_target.v[panel_parent->split_axis] = 1.f;
                panel->size_pct_of_parent.v[axis2_flip(panel_parent->split_axis)] = panel->size_pct_of_parent_target.v[axis2_flip(panel_parent->split_axis)] = 1.f;
              }
              
              // rjf: allocate & insert non-root panels - these will have a numeric string, determining
              // pct of parent
              if(n->flags & DF_CfgNodeFlag_Numeric)
              {
                panel = df_panel_alloc(ws);
                df_panel_insert(panel_parent, panel_parent->last, panel);
                panel->split_axis = axis2_flip(panel_parent->split_axis);
                panel->size_pct_of_parent.v[panel_parent->split_axis] = panel->size_pct_of_parent_target.v[panel_parent->split_axis] = (F32)f64_from_str8(n->string);
                panel->size_pct_of_parent.v[axis2_flip(panel_parent->split_axis)] = panel->size_pct_of_parent_target.v[axis2_flip(panel_parent->split_axis)] = 1.f;
              }
              
              // rjf: do general per-panel work
              if(!df_panel_is_nil(panel))
              {
                // rjf: determine if this panel has panel children
                B32 has_panel_children = 0;
                for(DF_CfgNode *child = n->first; child != &df_g_nil_cfg_node; child = child->next)
                {
                  if(child->flags & DF_CfgNodeFlag_Numeric)
                  {
                    has_panel_children = 1;
                    break;
                  }
                }
                
                // rjf: apply panel options
                for(DF_CfgNode *op = n->first; op != &df_g_nil_cfg_node; op = op->next)
                {
                  if(op->first == &df_g_nil_cfg_node && str8_match(op->string, str8_lit("tabs_on_bottom"), StringMatchFlag_CaseInsensitive))
                  {
                    panel->tab_side = Side_Max;
                  }
                }
                
                // rjf: apply panel views/tabs/commands
                DF_View *selected_view = &df_g_nil_view;
                for(DF_CfgNode *op = n->first; op != &df_g_nil_cfg_node; op = op->next)
                {
                  DF_ViewSpec *view_spec = df_view_spec_from_string(op->string);
                  if(view_spec == &df_g_nil_view_spec || has_panel_children != 0)
                  {
                    continue;
                  }
                  
                  // rjf: allocate view & apply view-specific parameterizations
                  DF_View *view = &df_g_nil_view;
                  B32 view_is_selected = 0;
                  DF_ViewSpecFlags view_spec_flags = view_spec->info.flags;
                  if(view_spec_flags & DF_ViewSpecFlag_CanSerialize)
                  {
                    // rjf: allocate view
                    view = df_view_alloc();
                    
                    // rjf: check if this view is selected
                    view_is_selected = df_cfg_node_child_from_string(op, str8_lit("selected"), StringMatchFlag_CaseInsensitive) != &df_g_nil_cfg_node;
                    
                    // rjf: read entity path
                    DF_Entity *entity = &df_g_nil_entity;
                    if(view_spec_flags & DF_ViewSpecFlag_CanSerializeEntityPath)
                    {
                      String8 saved_path = df_first_cfg_node_child_from_flags(op, DF_CfgNodeFlag_StringLiteral)->string;
                      String8 saved_path_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, saved_path, cfg_folder);
                      entity = df_entity_from_path(saved_path_absolute, DF_EntityFromPathFlag_All);
                    }
                    
                    // rjf: set up view
                    df_view_equip_spec(view, view_spec, entity, str8_lit(""), op);
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
                else for(DF_CfgNode *p = n;
                         p != &df_g_nil_cfg_node && p != cfg_panels;
                         p = p->parent, panel_parent = panel_parent->parent)
                {
                  if(p->next != &df_g_nil_cfg_node)
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
              DF_Panel *best_leaf_panel = &df_g_nil_panel;
              F32 best_leaf_panel_area = 0;
              Rng2F32 root_rect = r2f32p(0, 0, 1000, 1000); // NOTE(rjf): we can assume any size - just need proportions.
              for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
              {
                if(df_panel_is_nil(panel->first))
                {
                  Rng2F32 rect = df_rect_from_panel(root_rect, ws->root_panel, panel);
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
          if(src == DF_CfgSrc_User)
          {
            df_clear_bindings();
          }
          DF_CfgVal *keybindings = df_cfg_val_from_string(table, str8_lit("keybindings"));
          for(DF_CfgNode *keybinding_set = keybindings->first;
              keybinding_set != &df_g_nil_cfg_node;
              keybinding_set = keybinding_set->next)
          {
            for(DF_CfgNode *keybind = keybinding_set->first;
                keybind != &df_g_nil_cfg_node;
                keybind = keybind->next)
            {
              DF_CmdSpec *cmd_spec = &df_g_nil_cmd_spec;
              OS_Key key = OS_Key_Null;
              DF_CfgNode *ctrl_cfg = &df_g_nil_cfg_node;
              DF_CfgNode *shift_cfg = &df_g_nil_cfg_node;
              DF_CfgNode *alt_cfg = &df_g_nil_cfg_node;
              for(DF_CfgNode *child = keybind->first;
                  child != &df_g_nil_cfg_node;
                  child = child->next)
              {
                if(str8_match(child->string, str8_lit("ctrl"), StringMatchFlag_CaseInsensitive))
                {
                  ctrl_cfg = child;
                }
                else if(str8_match(child->string, str8_lit("shift"), StringMatchFlag_CaseInsensitive))
                {
                  shift_cfg = child;
                }
                else if(str8_match(child->string, str8_lit("alt"), StringMatchFlag_CaseInsensitive))
                {
                  alt_cfg = child;
                }
                else
                {
                  DF_CmdSpec *spec = df_cmd_spec_from_string(child->string);
                  for(U64 idx = 0; idx < ArrayCount(df_g_binding_version_remap_old_name_table); idx += 1)
                  {
                    if(str8_match(df_g_binding_version_remap_old_name_table[idx], child->string, StringMatchFlag_CaseInsensitive))
                    {
                      String8 new_name = df_g_binding_version_remap_new_name_table[idx];
                      spec = df_cmd_spec_from_string(new_name);
                    }
                  }
                  if(!df_cmd_spec_is_nil(spec))
                  {
                    cmd_spec = spec;
                  }
                  OS_Key k = df_os_key_from_cfg_string(child->string);
                  if(k != OS_Key_Null)
                  {
                    key = k;
                  }
                }
              }
              if(!df_cmd_spec_is_nil(cmd_spec) && key != OS_Key_Null)
              {
                OS_EventFlags flags = 0;
                if(ctrl_cfg != &df_g_nil_cfg_node) { flags |= OS_EventFlag_Ctrl; }
                if(shift_cfg != &df_g_nil_cfg_node) { flags |= OS_EventFlag_Shift; }
                if(alt_cfg != &df_g_nil_cfg_node) { flags |= OS_EventFlag_Alt; }
                DF_Binding binding = {key, flags};
                df_bind_spec(cmd_spec, binding);
              }
            }
          }
          
          //- rjf: apply theme colors
          B8 theme_color_hit[DF_ThemeColor_COUNT] = {0};
          MemoryCopy(df_gfx_state->cfg_theme_target.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
          MemoryCopy(df_gfx_state->cfg_theme.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
          DF_CfgVal *colors = df_cfg_val_from_string(table, str8_lit("colors"));
          for(DF_CfgNode *colors_set = colors->first;
              colors_set != &df_g_nil_cfg_node;
              colors_set = colors_set->next)
          {
            for(DF_CfgNode *color = colors_set->first;
                color != &df_g_nil_cfg_node;
                color = color->next)
            {
              String8 color_name = color->string;
              DF_ThemeColor color_code = DF_ThemeColor_Null;
              for(DF_ThemeColor c = DF_ThemeColor_Null; c < DF_ThemeColor_COUNT; c = (DF_ThemeColor)(c+1))
              {
                if(str8_match(df_g_theme_color_cfg_string_table[c], color_name, StringMatchFlag_CaseInsensitive))
                {
                  color_code = c;
                  break;
                }
              }
              if(color_code != DF_ThemeColor_Null)
              {
                theme_color_hit[color_code] = 1;
                DF_CfgNode *hex_cfg = color->first;
                String8 hex_string = hex_cfg->string;
                U64 hex_val = 0;
                try_u64_from_str8_c_rules(hex_string, &hex_val);
                Vec4F32 color_rgba = rgba_from_u32((U32)hex_val);
                df_gfx_state->cfg_theme_target.colors[color_code] = color_rgba;
                if(df_frame_index() <= 2)
                {
                  df_gfx_state->cfg_theme.colors[color_code] = color_rgba;
                }
              }
            }
          }
          
          //- rjf: apply theme presets
          DF_CfgVal *color_preset = df_cfg_val_from_string(table, str8_lit("color_preset"));
          B32 preset_applied = 0;
          if(color_preset != &df_g_nil_cfg_val)
          {
            String8 color_preset_name = color_preset->last->first->string;
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
              MemoryCopy(df_gfx_state->cfg_theme_target.colors, df_g_theme_preset_colors_table[preset], sizeof(df_g_theme_preset_colors__default_dark));
              MemoryCopy(df_gfx_state->cfg_theme.colors, df_g_theme_preset_colors_table[preset], sizeof(df_g_theme_preset_colors__default_dark));
            }
          }
          
          //- rjf: no preset -> autofill all missing colors from the preset with the most similar background
          if(!preset_applied)
          {
            DF_ThemePreset closest_preset = DF_ThemePreset_DefaultDark;
            F32 closest_preset_bg_distance = 100000000;
            for(DF_ThemePreset p = (DF_ThemePreset)0; p < DF_ThemePreset_COUNT; p = (DF_ThemePreset)(p+1))
            {
              Vec4F32 cfg_bg = df_gfx_state->cfg_theme_target.colors[DF_ThemeColor_PlainBackground];
              Vec4F32 pre_bg = df_g_theme_preset_colors_table[p][DF_ThemeColor_PlainBackground];
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
                df_gfx_state->cfg_theme_target.colors[c] = df_gfx_state->cfg_theme.colors[c] = df_g_theme_preset_colors_table[closest_preset][c];
              }
            }
          }
          
          //- rjf: if theme colors are all zeroes, then set to default - config appears busted
          {
            B32 all_colors_are_zero = 1;
            Vec4F32 zero_color = {0};
            for(DF_ThemeColor c = (DF_ThemeColor)(DF_ThemeColor_Null+1); c < DF_ThemeColor_COUNT; c = (DF_ThemeColor)(c+1))
            {
              if(!MemoryMatchStruct(&df_gfx_state->cfg_theme_target.colors[c], &zero_color))
              {
                all_colors_are_zero = 0;
                break;
              }
            }
            if(all_colors_are_zero)
            {
              MemoryCopy(df_gfx_state->cfg_theme_target.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
              MemoryCopy(df_gfx_state->cfg_theme.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
            }
          }
          
          //- rjf: if config opened 0 windows, we need to do some sensible default
          if(src == DF_CfgSrc_User && windows->first == &df_g_nil_cfg_node)
          {
            OS_Handle preferred_monitor = os_primary_monitor();
            Vec2F32 monitor_dim = os_dim_from_monitor(preferred_monitor);
            Vec2F32 window_dim = v2f32(monitor_dim.x*4/5, monitor_dim.y*4/5);
            DF_Window *ws = df_window_open(window_dim, preferred_monitor, DF_CfgSrc_User);
            DF_CmdParams blank_params = df_cmd_params_from_window(ws);
            df_cmd_list_push(arena, cmds, &blank_params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ResetToDefaultPanels));
          }
          
          //- rjf: if config bound 0 keys, we need to do some sensible default
          if(src == DF_CfgSrc_User && df_gfx_state->key_map_total_count == 0)
          {
            for(U64 idx = 0; idx < ArrayCount(df_g_default_binding_table); idx += 1)
            {
              DF_StringBindingPair *pair = &df_g_default_binding_table[idx];
              DF_CmdSpec *cmd_spec = df_cmd_spec_from_string(pair->string);
              df_bind_spec(cmd_spec, pair->binding);
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
          if(cfg_write_done[src] == 0)
          {
            cfg_write_done[src] = 1;
            String8 path = df_cfg_path_from_src(src);
            String8List strs = df_cfg_strings_from_gfx(scratch.arena, path, src);
            String8 data = str8_list_join(scratch.arena, &strs, 0);
            df_cfg_push_write_string(src, data);
          }
        }break;
        
        //- rjf: code navigation
        case DF_CoreCmdKind_FindTextForward:
        case DF_CoreCmdKind_FindTextBackward:
        {
          df_set_search_string(params.string);
        }break;
        
        //- rjf: find next and find prev
        case DF_CoreCmdKind_FindNext:
        {
          DF_CmdParams p = params;
          p.string = df_push_search_string(scratch.arena);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindTextForward));
        }break;
        case DF_CoreCmdKind_FindPrev:
        {
          DF_CmdParams p = params;
          p.string = df_push_search_string(scratch.arena);
          df_cmd_params_mark_slot(&p, DF_CmdParamSlot_String);
          df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindTextBackward));
        }break;
      }
    }
    scratch_end(scratch);
  }
  
  //- rjf: animate theme
  {
    DF_Theme *current = &df_gfx_state->cfg_theme;
    DF_Theme *target = &df_gfx_state->cfg_theme_target;
    F32 rate = 1 - pow_f32(2, (-50.f * df_dt()));
    for(DF_ThemeColor color = DF_ThemeColor_Null;
        color < DF_ThemeColor_COUNT;
        color = (DF_ThemeColor)(color+1))
    {
      if(abs_f32(target->colors[color].x - current->colors[color].x) > 0.01f ||
         abs_f32(target->colors[color].y - current->colors[color].y) > 0.01f ||
         abs_f32(target->colors[color].z - current->colors[color].z) > 0.01f ||
         abs_f32(target->colors[color].w - current->colors[color].w) > 0.01f)
      {
        df_gfx_request_frame();
      }
      current->colors[color].x += (target->colors[color].x - current->colors[color].x) * rate;
      current->colors[color].y += (target->colors[color].y - current->colors[color].y) * rate;
      current->colors[color].z += (target->colors[color].z - current->colors[color].z) * rate;
      current->colors[color].w += (target->colors[color].w - current->colors[color].w) * rate;
    }
  }
  
  //- rjf: animate alive-transitions for entities
  {
    F32 rate = 1.f - pow_f32(2.f, -20.f*df_dt());
    for(DF_Entity *e = df_entity_root(); !df_entity_is_nil(e); e = df_entity_rec_df_pre(e, df_entity_root()).next)
    {
      F32 diff = (1.f - e->alive_t);
      e->alive_t += diff * rate;
      if(diff >= 0.01f)
      {
        df_gfx_request_frame();
      }
    }
  }
  
  ProfEnd();
}

internal void
df_gfx_end_frame(void)
{
  ProfBeginFunction();
  
  //- rjf: simulate lag
  if(DEV_simulate_lag)
  {
    Sleep(300);
  }
  
  //- rjf: entities with a death timer -> keep animating
  for(DF_Entity *entity = df_entity_root(), *next = 0; !df_entity_is_nil(entity); entity = next)
  {
    next = df_entity_rec_df_pre(entity, &df_g_nil_entity).next;
    if(entity->flags & DF_EntityFlag_DiesWithTime)
    {
      df_gfx_request_frame();
    }
  }
  
  //- rjf: end drag/drop if needed
  if(df_gfx_state->drag_drop_state == DF_DragDropState_Dropping)
  {
    df_gfx_state->drag_drop_state = DF_DragDropState_Null;
    MemoryZeroStruct(&df_g_drag_drop_payload);
  }
  
  //- rjf: clear hover line info
  if(df_gfx_state->hover_line_set_this_frame == 0)
  {
    df_gfx_state->hover_line_binary = df_handle_zero();
    df_gfx_state->hover_line_voff = 0;
  }
  
  //- rjf: clear frame request state
  if(df_gfx_state->num_frames_requested > 0)
  {
    df_gfx_state->num_frames_requested -= 1;
  }
  
  ProfEnd();
}
