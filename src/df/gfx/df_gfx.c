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

internal B32
df_view_is_project_filtered(DF_View *view)
{
  B32 result = 0;
  DF_Entity *view_project = df_entity_from_handle(view->project);
  if(!df_entity_is_nil(view_project))
  {
    DF_Entity *current_project = df_entity_from_path(df_cfg_path_from_src(DF_CfgSrc_Project), 0);
    if(current_project != view_project)
    {
      result = 1;
    }
  }
  return result;
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
  DLLInsert_NPZ(&df_g_nil_view, panel->first_tab_view, panel->last_tab_view, prev_view, view, next, prev);
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
    panel->selected_tab_view = df_handle_zero();
    if(df_handle_match(df_handle_zero(), panel->selected_tab_view))
    {
      for(DF_View *v = view->next; !df_view_is_nil(v); v = v->next)
      {
        if(!df_view_is_project_filtered(v))
        {
          panel->selected_tab_view = df_handle_from_view(v);
          break;
        }
      }
    }
    if(df_handle_match(df_handle_zero(), panel->selected_tab_view))
    {
      for(DF_View *v = view->prev; !df_view_is_nil(v); v = v->prev)
      {
        if(!df_view_is_project_filtered(v))
        {
          panel->selected_tab_view = df_handle_from_view(v);
          break;
        }
      }
    }
  }
  DLLRemove_NPZ(&df_g_nil_view, panel->first_tab_view, panel->last_tab_view, view, next, prev);
  panel->tab_view_count -= 1;
}

internal DF_View *
df_selected_tab_from_panel(DF_Panel *panel)
{
  DF_View *view = df_view_from_handle(panel->selected_tab_view);
  if(df_view_is_project_filtered(view))
  {
    view = &df_g_nil_view;
  }
  return view;
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
    p.view   = df_handle_from_view(df_selected_tab_from_panel(window->focused_panel));
  }
  return p;
}

internal B32
df_prefer_dasm_from_window(DF_Window *window)
{
  DF_Panel *panel = window->focused_panel;
  DF_View *view = df_selected_tab_from_panel(panel);
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
      DF_View *p_view = df_selected_tab_from_panel(p);
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
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(window, df_selected_tab_from_panel(window->focused_panel));
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_PreferDisassembly);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_UnwindIndex);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_InlineDepth);
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(window->focused_panel);
  p.view   = df_handle_from_view(df_selected_tab_from_panel(window->focused_panel));
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = ctrl_ctx.thread;
  p.unwind_index = ctrl_ctx.unwind_count;
  p.inline_depth = ctrl_ctx.inline_depth;
  return p;
}

internal DF_CmdParams
df_cmd_params_from_panel(DF_Window *window, DF_Panel *panel)
{
  DF_CmdParams p = df_cmd_params_zero();
  DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(window, df_selected_tab_from_panel(panel));
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_PreferDisassembly);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_UnwindIndex);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_InlineDepth);
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(panel);
  p.view   = df_handle_from_view(df_selected_tab_from_panel(panel));
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = ctrl_ctx.thread;
  p.unwind_index = ctrl_ctx.unwind_count;
  p.inline_depth = ctrl_ctx.inline_depth;
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
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_UnwindIndex);
  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_InlineDepth);
  p.window = df_handle_from_window(window);
  p.panel  = df_handle_from_panel(panel);
  p.view   = df_handle_from_view(view);
  p.prefer_dasm = df_prefer_dasm_from_window(window);
  p.entity = ctrl_ctx.thread;
  p.unwind_index = ctrl_ctx.unwind_count;
  p.inline_depth = ctrl_ctx.inline_depth;
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
  if(src->cfg_node != 0) {dst.cfg_node = df_cfg_tree_copy(arena, src->cfg_node);}
  if(dst.cmd_spec == 0)  {dst.cmd_spec = &df_g_nil_cmd_spec;}
  if(dst.view_spec == 0) {dst.view_spec = &df_g_nil_view_spec;}
  if(dst.cfg_node == 0)  {dst.cfg_node = &df_g_nil_cfg_node;}
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
df_set_rich_hover_info(DF_RichHoverInfo *info)
{
  arena_clear(df_gfx_state->rich_hover_info_next_arena);
  MemoryCopyStruct(&df_gfx_state->rich_hover_info_next, info);
  df_gfx_state->rich_hover_info_next.dbgi_key = di_key_copy(df_gfx_state->rich_hover_info_next_arena, &info->dbgi_key);
}

internal DF_RichHoverInfo
df_get_rich_hover_info(void)
{
  DF_RichHoverInfo info = df_gfx_state->rich_hover_info_current;
  return info;
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

internal DF_ViewSpec *
df_tab_view_spec_from_gfx_view_rule_spec(DF_GfxViewRuleSpec *spec)
{
  DF_ViewSpec *result = &df_g_nil_view_spec;
  if(spec->info.tab_view_spec_name.size != 0)
  {
    result = df_view_spec_from_string(spec->info.tab_view_spec_name);
  }
  return result;
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
df_view_equip_spec(DF_Window *window, DF_View *view, DF_ViewSpec *spec, DF_Entity *entity, String8 default_query, DF_CfgNode *cfg_root)
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
    if(spec->info.flags & DF_ViewSpecFlag_ProjectSpecific)
    {
      view->project = df_handle_from_entity(df_entity_from_path(df_cfg_path_from_src(DF_CfgSrc_Project), DF_EntityFromPathFlag_OpenMissing|DF_EntityFromPathFlag_OpenAsNeeded));
    }
    else
    {
      MemoryZeroStruct(&view->project);
    }
    view->is_filtering = 0;
    view->is_filtering_t = 0;
    view_setup(window, view, cfg_root);
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
    next = view->next;
    df_view_release(view);
  }
  panel->first_tab_view = panel->last_tab_view = &df_g_nil_view;
  panel->selected_tab_view = df_handle_zero();
  panel->tab_view_count = 0;
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
    window->os = os_window_open(size, OS_WindowFlag_CustomBorder, title);
  }
  window->r = r_window_equip(window->os);
  window->ui = ui_state_alloc();
  window->view_state_hist = df_state_delta_history_alloc();
  window->code_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_code_ctx_menu_"));
  window->entity_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_entity_ctx_menu_"));
  window->tab_ctx_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_tab_ctx_menu_"));
  window->code_ctx_menu_arena = arena_alloc();
  window->hover_eval_arena = arena_alloc();
  window->autocomp_lister_params_arena = arena_alloc();
  window->free_panel = &df_g_nil_panel;
  window->root_panel = df_panel_alloc(window);
  window->focused_panel = window->root_panel;
  window->query_cmd_arena = arena_alloc();
  window->query_cmd_spec = &df_g_nil_cmd_spec;
  window->query_view_stack_top = &df_g_nil_view;
  window->last_dpi = os_dpi_from_window(window->os);
  for(EachEnumVal(DF_SettingCode, code))
  {
    if(df_g_setting_code_default_is_per_window_table[code])
    {
      window->setting_vals[code] = df_g_setting_code_default_val_table[code];
    }
  }
  if(df_gfx_state->first_window == 0)
  {
    DF_FontSlot english_font_slots[] = {DF_FontSlot_Main, DF_FontSlot_Code};
    DF_FontSlot icon_font_slot = DF_FontSlot_Icons;
    for(U64 idx = 0; idx < ArrayCount(english_font_slots); idx += 1)
    {
      Temp scratch = scratch_begin(0, 0);
      DF_FontSlot slot = english_font_slots[idx];
      String8 sample_text = str8_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#$%^&*()-_+=[{]}\\|;:'\",<.>/?");
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(slot),
                             df_font_size_from_slot(window, DF_FontSlot_Code),
                             0, 0, 0,
                             sample_text);
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(slot),
                             df_font_size_from_slot(window, DF_FontSlot_Main),
                             0, 0, 0,
                             sample_text);
      scratch_end(scratch);
    }
    for(DF_IconKind icon_kind = DF_IconKind_Null; icon_kind < DF_IconKind_COUNT; icon_kind = (DF_IconKind)(icon_kind+1))
    {
      Temp scratch = scratch_begin(0, 0);
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(icon_font_slot),
                             df_font_size_from_slot(window, icon_font_slot),
                             0, 0, F_RasterFlag_Smooth,
                             df_g_icon_kind_text_table[icon_kind]);
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(icon_font_slot),
                             df_font_size_from_slot(window, DF_FontSlot_Main),
                             0, 0, F_RasterFlag_Smooth,
                             df_g_icon_kind_text_table[icon_kind]);
      f_push_run_from_string(scratch.arena,
                             df_font_from_slot(icon_font_slot),
                             df_font_size_from_slot(window, DF_FontSlot_Code),
                             0, 0, F_RasterFlag_Smooth,
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
df_window_update_and_render(Arena *arena, DF_Window *ws, DF_CmdList *cmds)
{
  ProfBeginFunction();
  
  //////////////////////////////
  //- rjf: unpack context
  //
  B32 window_is_focused = os_window_is_focused(ws->os) || ws->window_temporarily_focused_ipc;
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
  ws->window_temporarily_focused_ipc = 0;
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
  UI_EventList events = {0};
  B32 panel_reset_done = 0;
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
      Dir2 split_dir = Dir2_Invalid;
      DF_Panel *split_panel = ws->focused_panel;
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
        
        //- rjf: OS events
        case DF_CoreCmdKind_OSEvent:
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
              }
            }
            ui_event.kind         = kind;
            ui_event.key          = os_event->key;
            ui_event.modifiers    = os_event->flags;
            ui_event.string       = os_event->character ? str8_from_32(ui_build_arena(), str32(&os_event->character, 1)) : str8_zero();
            ui_event.pos          = os_event->pos;
            ui_event.delta_2f32   = os_event->delta;
            ui_event.timestamp_us = os_event->timestamp_us;
            ui_event_list_push(ui_build_arena(), &events, &ui_event);
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
          MemoryZeroStruct(&ws->ctrl_ctx_overrides);
          ws->ctrl_ctx_overrides.thread = params.entity;
        }goto thread_locator;
        case DF_CoreCmdKind_SelectThreadView:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_view_from_handle(params.view);
          if(df_view_is_nil(view) && !df_panel_is_nil(panel))
          {
            view = df_selected_tab_from_panel(panel);
          }
          if(!df_view_is_nil(view))
          {
            MemoryZeroStruct(&view->ctrl_ctx_overrides);
            view->ctrl_ctx_overrides.thread = params.entity;
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
          ws->setting_vals[DF_SettingCode_MainFontSize].set = 1;
          ws->setting_vals[DF_SettingCode_MainFontSize].s32 += 1;
          ws->setting_vals[DF_SettingCode_MainFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_MainFontSize], ws->setting_vals[DF_SettingCode_MainFontSize].s32);
        }break;
        case DF_CoreCmdKind_DecUIFontScale:
        {
          ws->setting_vals[DF_SettingCode_MainFontSize].set = 1;
          ws->setting_vals[DF_SettingCode_MainFontSize].s32 -= 1;
          ws->setting_vals[DF_SettingCode_MainFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_MainFontSize], ws->setting_vals[DF_SettingCode_MainFontSize].s32);
        }break;
        case DF_CoreCmdKind_IncCodeFontScale:
        {
          ws->setting_vals[DF_SettingCode_CodeFontSize].set = 1;
          ws->setting_vals[DF_SettingCode_CodeFontSize].s32 += 1;
          ws->setting_vals[DF_SettingCode_CodeFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_CodeFontSize], ws->setting_vals[DF_SettingCode_CodeFontSize].s32);
        }break;
        case DF_CoreCmdKind_DecCodeFontScale:
        {
          ws->setting_vals[DF_SettingCode_CodeFontSize].set = 1;
          ws->setting_vals[DF_SettingCode_CodeFontSize].s32 -= 1;
          ws->setting_vals[DF_SettingCode_CodeFontSize].s32 = clamp_1s32(df_g_setting_code_s32_range_table[DF_SettingCode_CodeFontSize], ws->setting_vals[DF_SettingCode_CodeFontSize].s32);
        }break;
        
        //- rjf: panel creation
        case DF_CoreCmdKind_NewPanelLeft: {split_dir = Dir2_Left;}goto split;
        case DF_CoreCmdKind_NewPanelUp:   {split_dir = Dir2_Up;}goto split;
        case DF_CoreCmdKind_NewPanelRight:{split_dir = Dir2_Right;}goto split;
        case DF_CoreCmdKind_NewPanelDown: {split_dir = Dir2_Down;}goto split;
        case DF_CoreCmdKind_SplitPanel:
        {
          split_dir = params.dir2;
          split_panel = df_panel_from_handle(params.dest_panel);
        }goto split;
        split:;
        if(split_dir != Dir2_Invalid && !df_panel_is_nil(split_panel))
        {
          DF_Panel *new_panel = &df_g_nil_panel;
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
            df_panel_insert(new_parent, &df_g_nil_panel, left);
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
          DF_Panel *move_tab_panel = df_panel_from_handle(params.panel);
          DF_View *move_tab = df_view_from_handle(params.view);
          if(!df_panel_is_nil(new_panel) && !df_view_is_nil(move_tab) && !df_panel_is_nil(move_tab_panel) &&
             core_cmd_kind == DF_CoreCmdKind_SplitPanel)
          {
            df_panel_remove_tab_view(move_tab_panel, move_tab);
            df_panel_insert_tab_view(new_panel, new_panel->last_tab_view, move_tab);
            new_panel->selected_tab_view = df_handle_from_view(move_tab);
            B32 move_tab_panel_is_empty = 1;
            for(DF_View *v = move_tab_panel->first_tab_view; !df_view_is_nil(v); v = v->next)
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
              DF_CmdParams p = df_cmd_params_from_panel(ws, move_tab_panel);
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClosePanel));
            }
          }
          df_panel_notify_mutation(ws, panel);
        }break;
        case DF_CoreCmdKind_ResetToDefaultPanels:
        case DF_CoreCmdKind_ResetToCompactPanels:
        {
          panel_reset_done = 1;
          
          typedef enum Layout
          {
            Layout_Default,
            Layout_Compact,
          }
          Layout;
          Layout layout = Layout_Default;
          switch(core_cmd_kind)
          {
            default:{}break;
            case DF_CoreCmdKind_ResetToDefaultPanels:{layout = Layout_Default;}break;
            case DF_CoreCmdKind_ResetToCompactPanels:{layout = Layout_Compact;}break;
          }
          
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
          DF_View *getting_started = &df_g_nil_view;
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
                case DF_GfxViewKind_Watch:         {if(df_view_is_nil(watch))               { needs_delete = 0; watch = view;} }break;
                case DF_GfxViewKind_Locals:        {if(df_view_is_nil(locals))              { needs_delete = 0; locals = view;} }break;
                case DF_GfxViewKind_Registers:     {if(df_view_is_nil(regs))                { needs_delete = 0; regs = view;} }break;
                case DF_GfxViewKind_Globals:       {if(df_view_is_nil(globals))             { needs_delete = 0; globals = view;} }break;
                case DF_GfxViewKind_ThreadLocals:  {if(df_view_is_nil(tlocals))             { needs_delete = 0; tlocals = view;} }break;
                case DF_GfxViewKind_Types:         {if(df_view_is_nil(types))               { needs_delete = 0; types = view;} }break;
                case DF_GfxViewKind_Procedures:    {if(df_view_is_nil(procs))               { needs_delete = 0; procs = view;} }break;
                case DF_GfxViewKind_CallStack:     {if(df_view_is_nil(callstack))           { needs_delete = 0; callstack = view;} }break;
                case DF_GfxViewKind_Breakpoints:   {if(df_view_is_nil(breakpoints))         { needs_delete = 0; breakpoints = view;} }break;
                case DF_GfxViewKind_WatchPins:     {if(df_view_is_nil(watch_pins))          { needs_delete = 0; watch_pins = view;} }break;
                case DF_GfxViewKind_Output:        {if(df_view_is_nil(output))              { needs_delete = 0; output = view;} }break;
                case DF_GfxViewKind_Targets:       {if(df_view_is_nil(targets))             { needs_delete = 0; targets = view;} }break;
                case DF_GfxViewKind_Scheduler:     {if(df_view_is_nil(scheduler))           { needs_delete = 0; scheduler = view;} }break;
                case DF_GfxViewKind_Modules:       {if(df_view_is_nil(modules))             { needs_delete = 0; modules = view;} }break;
                case DF_GfxViewKind_Disassembly:   {if(df_view_is_nil(disasm))              { needs_delete = 0; disasm = view;} }break;
                case DF_GfxViewKind_Memory:        {if(df_view_is_nil(memory))              { needs_delete = 0; memory = view;} }break;
                case DF_GfxViewKind_GettingStarted:{if(df_view_is_nil(getting_started))     { needs_delete = 0; getting_started = view;} }break;
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
            df_view_equip_spec(ws, watch, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Watch), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(locals))
          {
            locals = df_view_alloc();
            df_view_equip_spec(ws, locals, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Locals), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(regs))
          {
            regs = df_view_alloc();
            df_view_equip_spec(ws, regs, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Registers), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(globals))
          {
            globals = df_view_alloc();
            df_view_equip_spec(ws, globals, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Globals), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(tlocals))
          {
            tlocals = df_view_alloc();
            df_view_equip_spec(ws, tlocals, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_ThreadLocals), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(types))
          {
            types = df_view_alloc();
            df_view_equip_spec(ws, types, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Types), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(procs))
          {
            procs = df_view_alloc();
            df_view_equip_spec(ws, procs, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Procedures), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(callstack))
          {
            callstack = df_view_alloc();
            df_view_equip_spec(ws, callstack, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_CallStack), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(breakpoints))
          {
            breakpoints = df_view_alloc();
            df_view_equip_spec(ws, breakpoints, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Breakpoints), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(watch_pins))
          {
            watch_pins = df_view_alloc();
            df_view_equip_spec(ws, watch_pins, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_WatchPins), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(output))
          {
            output = df_view_alloc();
            df_view_equip_spec(ws, output, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Output), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(targets))
          {
            targets = df_view_alloc();
            df_view_equip_spec(ws, targets, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Targets), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(scheduler))
          {
            scheduler = df_view_alloc();
            df_view_equip_spec(ws, scheduler, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Scheduler), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(modules))
          {
            modules = df_view_alloc();
            df_view_equip_spec(ws, modules, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Modules), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(df_view_is_nil(disasm))
          {
            disasm = df_view_alloc();
            df_view_equip_spec(ws, disasm, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Disassembly), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(layout == Layout_Default && df_view_is_nil(memory))
          {
            memory = df_view_alloc();
            df_view_equip_spec(ws, memory, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Memory), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
          }
          if(code_views.count == 0 && df_view_is_nil(getting_started))
          {
            getting_started = df_view_alloc();
            df_view_equip_spec(ws, getting_started, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_GettingStarted), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
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
              for(DF_HandleNode *n = code_views.first; n != 0; n = n->next)
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
          Rng2F32 src_panel_rect = df_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, src_panel);
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
            Rng2F32 p_rect = df_target_rect_from_panel(r2f32(v2f32(0, 0), v2f32(1000, 1000)), ws->root_panel, p);
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
              DF_Panel *next = &df_g_nil_panel;
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
        case DF_CoreCmdKind_NextTab:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *next_view = view;
          for(DF_View *v = view; !df_view_is_nil(v); v = df_view_is_nil(v->next) ? panel->first_tab_view : v->next)
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
        case DF_CoreCmdKind_PrevTab:
        {
          DF_Panel *panel = df_panel_from_handle(params.panel);
          DF_View *view = df_selected_tab_from_panel(panel);
          DF_View *next_view = view;
          for(DF_View *v = view; !df_view_is_nil(v); v = df_view_is_nil(v->prev) ? panel->last_tab_view : v->prev)
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
        case DF_CoreCmdKind_MoveTabRight:
        case DF_CoreCmdKind_MoveTabLeft:
        {
          DF_Panel *panel = ws->focused_panel;
          DF_View *view = df_selected_tab_from_panel(panel);
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
            df_view_equip_spec(ws, view, spec, entity, params.string, params.cfg_node);
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
            B32 src_panel_is_empty = 1;
            for(DF_View *v = src_panel->first_tab_view; !df_view_is_nil(v); v = v->next)
            {
              if(!df_view_is_project_filtered(v))
              {
                src_panel_is_empty = 0;
                break;
              }
            }
            if(src_panel_is_empty && src_panel != ws->root_panel)
            {
              DF_CmdParams p = df_cmd_params_from_panel(ws, src_panel);
              df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClosePanel));
            }
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
        case DF_CoreCmdKind_Switch:
        {
          B32 already_opened = 0;
          DF_Panel *panel = df_panel_from_handle(params.panel);
          for(DF_View *v = panel->first_tab_view; !df_view_is_nil(v); v = v->next)
          {
            if(df_view_is_project_filtered(v)) { continue; }
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
          DF_View *view = df_selected_tab_from_panel(panel);
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
        
        //- rjf: meta controls
        case DF_CoreCmdKind_Edit:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Press;
          evt.slot       = UI_EventActionSlot_Edit;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_Accept:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Press;
          evt.slot       = UI_EventActionSlot_Accept;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_Cancel:
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
        case DF_CoreCmdKind_MoveLeft:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveRight:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_PickSelectSide|UI_EventFlag_ZeroDeltaOnSelect|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUp:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDown:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveLeftSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveRightSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveLeftChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveRightChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpPage:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownPage:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpWhole:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownWhole:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveLeftChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveRightChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownChunkSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark|UI_EventFlag_ExplicitDirectional;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpPageSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownPageSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Page;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpWholeSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownWholeSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Whole;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveUpReorder:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_Reorder;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, -1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveDownReorder:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_Reorder;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+0, +1);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveHome:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveEnd:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveHomeSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_MoveEndSelect:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Navigate;
          evt.flags      = UI_EventFlag_KeepMark;
          evt.delta_unit = UI_EventDeltaUnit_Line;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_SelectAll:
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
        case DF_CoreCmdKind_DeleteSingle:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_DeleteChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(+1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_BackspaceSingle:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete|UI_EventFlag_ZeroDeltaOnSelect;
          evt.delta_unit = UI_EventDeltaUnit_Char;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_BackspaceChunk:
        {
          UI_Event evt = zero_struct;
          evt.kind       = UI_EventKind_Edit;
          evt.flags      = UI_EventFlag_Delete;
          evt.delta_unit = UI_EventDeltaUnit_Word;
          evt.delta_2s32 = v2s32(-1, +0);
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_Copy:
        {
          UI_Event evt = zero_struct;
          evt.kind  = UI_EventKind_Edit;
          evt.flags = UI_EventFlag_Copy|UI_EventFlag_KeepMark;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_Cut:
        {
          UI_Event evt = zero_struct;
          evt.kind  = UI_EventKind_Edit;
          evt.flags = UI_EventFlag_Copy|UI_EventFlag_Delete;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_Paste:
        {
          UI_Event evt = zero_struct;
          evt.kind   = UI_EventKind_Text;
          evt.string = os_get_clipboard_text(ui_build_arena());
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        case DF_CoreCmdKind_InsertText:
        {
          UI_Event evt = zero_struct;
          evt.kind   = UI_EventKind_Text;
          evt.string = params.string;
          ui_event_list_push(ui_build_arena(), &events, &evt);
        }break;
        
        //- rjf: address finding
        case DF_CoreCmdKind_GoToAddress:
        {
          U64 vaddr = params.vaddr;
        }break;
        
        //- rjf: thread finding
        case DF_CoreCmdKind_FindThread:
        {
          DI_Scope *scope = di_scope_open();
          DF_Entity *thread = df_entity_from_handle(params.entity);
          U64 unwind_index = params.unwind_index;
          U64 inline_depth = params.inline_depth;
          if(thread->kind == DF_EntityKind_Thread)
          {
            // rjf: grab rip
            U64 rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_index);
            
            // rjf: extract thread/rip info
            DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
            DI_Key dbgi_key = df_dbgi_key_from_module(module);
            RDI_Parsed *rdi = di_rdi_from_key(scope, &dbgi_key, 0);
            U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
            DF_LineList lines = df_lines_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff);
            DF_Line line = {0};
            {
              U64 idx = 0;
              for(DF_LineNode *n = lines.first; n != 0; n = n->next, idx += 1)
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
            B32 has_module = !df_entity_is_nil(module);
            B32 has_dbg_info = has_module && !dbgi_missing;
            if(!dbgi_pending && (has_line_info || has_module))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              if(has_line_info)
              {
                params.file_path = df_full_path_from_entity(scratch.arena, df_entity_from_handle(line.file));
                params.text_point = line.pt;
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
              }
              params.entity = df_handle_from_entity(thread);
              params.voff = rip_voff;
              params.vaddr = rip_vaddr;
              params.unwind_index = unwind_index;
              params.inline_depth = inline_depth;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualOff);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_UnwindIndex);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_InlineDepth);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            }
            
            // rjf: snap to resolved address w/o line info
            if(!missing_rip && !dbgi_pending && !has_line_info && !has_module)
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(thread);
              params.voff = rip_voff;
              params.vaddr = rip_vaddr;
              params.unwind_index = unwind_index;
              params.inline_depth = inline_depth;
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualOff);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_UnwindIndex);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_InlineDepth);
              df_cmd_list_push(arena, cmds, &params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
            }
            
            // rjf: retry on stopped, pending debug info
            if(!df_ctrl_targets_running() && (dbgi_pending || missing_rip))
            {
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindThread));
            }
          }
          di_scope_close(scope);
        }break;
        case DF_CoreCmdKind_FindSelectedThread:
        {
          DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(ws);
          DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(selected_thread);
          params.unwind_index = ctrl_ctx.unwind_count;
          params.inline_depth = ctrl_ctx.inline_depth;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_UnwindIndex);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_InlineDepth);
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
            DI_Key voff_dbgi_key = {0};
            if(name_resolved == 0)
            {
              DI_KeyList keys = df_push_active_dbgi_key_list(scratch.arena);
              for(DI_KeyNode *n = keys.first; n != 0; n = n->next)
              {
                U64 binary_voff = df_voff_from_dbgi_key_symbol_name(&n->v, name);
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
              DF_LineList lines = df_lines_from_dbgi_key_voff(scratch.arena, &voff_dbgi_key, voff);
              if(lines.first != 0)
              {
                DF_CmdParams p = params;
                {
                  p.file_path = df_full_path_from_entity(scratch.arena, df_entity_from_handle(lines.first->v.file));
                  p.text_point = lines.first->v.pt;
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_FilePath);
                  df_cmd_params_mark_slot(&p, DF_CmdParamSlot_TextPoint);
                  if(voff_dbgi_key.path.size != 0)
                  {
                    DF_EntityList modules = df_modules_from_dbgi_key(scratch.arena, &voff_dbgi_key);
                    DF_Entity *module = df_first_entity_from_list(&modules);
                    DF_Entity *process = df_entity_ancestor_from_kind(module, DF_EntityKind_Process);
                    if(!df_entity_is_nil(process))
                    {
                      p.entity = df_handle_from_entity(process);
                      p.vaddr = module->vaddr_rng.min + lines.first->v.voff_range.min;
                      df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Entity);
                      df_cmd_params_mark_slot(&p, DF_CmdParamSlot_VirtualAddr);
                    }
                  }
                }
                df_cmd_list_push(arena, cmds, &p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
              }
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
            case DF_EntityKind_Target:
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectTarget));
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
                DF_EntityList modules = df_modules_from_dbgi_key(scratch.arena, &n->v.dbgi_key);
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
              if(df_view_is_project_filtered(view)) { continue; }
              DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
              DF_Entity *viewed_entity = df_entity_from_handle(view->entity);
              if((view_kind == DF_GfxViewKind_Code || view_kind == DF_GfxViewKind_PendingEntity) && viewed_entity == src_code)
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
          DF_Panel *panel_w_any_src_code = &df_g_nil_panel;
          for(DF_Panel *panel = ws->root_panel; !df_panel_is_nil(panel); panel = df_panel_rec_df_pre(panel).next)
          {
            if(!df_panel_is_nil(panel->first))
            {
              continue;
            }
            for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
            {
              if(df_view_is_project_filtered(view)) { continue; }
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
              if(df_view_is_project_filtered(view)) { continue; }
              DF_GfxViewKind view_kind = df_gfx_view_kind_from_string(view->spec->info.name);
              DF_Entity *viewed_entity = df_entity_from_handle(view->entity);
              if(view_kind == DF_GfxViewKind_Disassembly)
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
              Rng2F32 panel_rect = df_target_rect_from_panel(root_rect, ws->root_panel, panel);
              Vec2F32 panel_rect_dim = dim_2f32(panel_rect);
              F32 area = panel_rect_dim.x * panel_rect_dim.y;
              B32 panel_is_empty = 1;
              for(DF_View *v = panel->first_tab_view; !df_view_is_nil(v); v = v->next)
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
              df_view_equip_spec(ws, view, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Code), src_code, str8_lit(""), &df_g_nil_cfg_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            DF_CoreCmdKind cursor_snap_kind = DF_CoreCmdKind_CenterCursor;
            if(!df_panel_is_nil(dst_panel) && dst_view == view_w_this_src_code && df_selected_tab_from_panel(dst_panel) == dst_view)
            {
              cursor_snap_kind = DF_CoreCmdKind_ContainCursor;
            }
            
            // rjf: move cursor & snap-to-cursor
            if(!df_panel_is_nil(dst_panel))
            {
              disasm_view_prioritized = (df_selected_tab_from_panel(dst_panel) == view_w_disasm);
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
              df_view_equip_spec(ws, view, df_view_spec_from_gfx_view_kind(DF_GfxViewKind_Disassembly), &df_g_nil_entity, str8_lit(""), &df_g_nil_cfg_node);
              df_panel_insert_tab_view(dst_panel, dst_panel->last_tab_view, view);
              dst_view = view;
            }
            
            // rjf: determine if we need a contain or center
            DF_CoreCmdKind cursor_snap_kind = DF_CoreCmdKind_CenterCursor;
            if(dst_view == view_w_disasm && df_selected_tab_from_panel(dst_panel) == dst_view)
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
      for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->next)
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
      for(DF_View *tab = panel->first_tab_view; !df_view_is_nil(tab); tab = tab->next)
      {
        if(df_view_is_project_filtered(tab)) {continue;}
        if(tab == view)
        {
          found = 1;
        }
      }
      if(!found)
      {
        panel->selected_tab_view = df_handle_zero();
      }
    }
  }
  
  //////////////////////////////
  //- rjf: fill window/panel/view interaction registers
  //
  df_interact_regs()->window = df_handle_from_window(ws);
  df_interact_regs()->panel  = df_handle_from_panel(ws->focused_panel);
  df_interact_regs()->view   = ws->focused_panel->selected_tab_view;
  
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
      DF_View *view = df_selected_tab_from_panel(panel);
      if(!df_view_is_nil(view))
      {
        df_push_interact_regs();
        DF_ViewCmdFunctionType *do_view_cmds_function = view->spec->info.cmd_hook;
        do_view_cmds_function(ws, panel, view, cmds);
        DF_InteractRegs *view_regs = df_pop_interact_regs();
        if(panel == ws->focused_panel)
        {
          MemoryCopyStruct(df_interact_regs(), view_regs);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: compute ui palettes from theme
  //
  {
    DF_Theme *current = &df_gfx_state->cfg_theme;
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
    if(df_setting_val_from_code(0, DF_SettingCode_OpaqueBackgrounds).s32)
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
      
      // rjf: build widget palette info
      UI_WidgetPaletteInfo widget_palette_info = {0};
      {
        widget_palette_info.tooltip_palette = df_palette_from_code(ws, DF_PaletteCode_Floating);
        widget_palette_info.ctx_menu_palette = df_palette_from_code(ws, DF_PaletteCode_Floating);
        widget_palette_info.scrollbar_palette = df_palette_from_code(ws, DF_PaletteCode_ScrollBarButton);
      }
      
      // rjf: build animation info
      UI_AnimationInfo animation_info = {0};
      {
        if(df_setting_val_from_code(ws, DF_SettingCode_HoverAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_HotAnimations;}
        if(df_setting_val_from_code(ws, DF_SettingCode_PressAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_ActiveAnimations;}
        if(df_setting_val_from_code(ws, DF_SettingCode_FocusAnimations).s32)       {animation_info.flags |= UI_AnimationInfoFlag_FocusAnimations;}
        if(df_setting_val_from_code(ws, DF_SettingCode_TooltipAnimations).s32)     {animation_info.flags |= UI_AnimationInfoFlag_TooltipAnimations;}
        if(df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32)        {animation_info.flags |= UI_AnimationInfoFlag_ContextMenuAnimations;}
        if(df_setting_val_from_code(ws, DF_SettingCode_ScrollingAnimations).s32)   {animation_info.flags |= UI_AnimationInfoFlag_ScrollingAnimations;}
      }
      
      // rjf: begin & push initial stack values
      ui_begin_build(ws->os, &events, &icon_info, &widget_palette_info, &animation_info, df_dt(), df_dt());
      ui_push_font(main_font);
      ui_push_font_size(main_font_size);
      ui_push_text_padding(main_font_size*0.3f);
      ui_push_pref_width(ui_em(20.f, 1));
      ui_push_pref_height(ui_em(2.75f, 1.f));
      ui_push_palette(df_palette_from_code(ws, DF_PaletteCode_Base));
      ui_push_blur_size(10.f);
      F_RasterFlags text_raster_flags = 0;
      if(df_setting_val_from_code(ws, DF_SettingCode_SmoothUIText).s32) {text_raster_flags |= F_RasterFlag_Smooth;}
      if(df_setting_val_from_code(ws, DF_SettingCode_HintUIText).s32) {text_raster_flags |= F_RasterFlag_Hinted;}
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
      {
        //- rjf: tab dragging
        if(!df_view_is_nil(view))
        {
          UI_Size main_width = ui_top_pref_width();
          UI_Size main_height = ui_top_pref_height();
          UI_TextAlign main_text_align = ui_top_text_alignment();
          DF_Palette(ws, DF_PaletteCode_Tab)
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
                DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
                String8 display_name = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
                DF_IconKind icon_kind = df_icon_kind_from_view(view);
                DF_Font(ws, DF_FontSlot_Icons)
                  UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
                  UI_PrefWidth(ui_em(2.5f, 1.f))
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  ui_label(df_g_icon_kind_text_table[icon_kind]);
                ui_label(display_name);
              }
              ui_set_next_pref_width(ui_pct(1, 0));
              ui_set_next_pref_height(ui_pct(1, 0));
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *view_preview_container = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clip, "###view_preview_container");
              UI_Parent(view_preview_container) UI_Focus(UI_FocusKind_Off) UI_WidthFill
              {
                DF_ViewSpec *view_spec = view->spec;
                DF_ViewUIFunctionType *build_view_ui_function = view_spec->info.ui_hook;
                build_view_ui_function(ws, &df_g_nil_panel, view, view_preview_container->rect);
              }
            }
          }
        }
        
        //- rjf: entity dragging
        else if(!df_entity_is_nil(entity)) UI_Tooltip
        {
          ui_set_next_pref_width(ui_children_sum(1));
          UI_Row UI_HeightFill
          {
            String8 display_name = df_display_string_from_entity(scratch.arena, entity);
            DF_IconKind icon_kind = df_g_entity_kind_icon_kind_table[entity->kind];
            DF_Font(ws, DF_FontSlot_Icons)
              UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
              UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
              ui_label(df_g_icon_kind_text_table[icon_kind]);
            ui_label(display_name);
          }
        }
      }
      scratch_end(scratch);
    }
    
    ////////////////////////////
    //- rjf: developer menu
    //
    if(ws->dev_menu_is_open) DF_Font(ws, DF_FontSlot_Code)
    {
      ui_set_next_flags(UI_BoxFlag_ViewScrollY|UI_BoxFlag_AllowOverflowY|UI_BoxFlag_ViewClamp);
      UI_PaneF(r2f32p(30, 30, 30+ui_top_font_size()*100, ui_top_font_size()*150), "###dev_ctx_menu")
      {
        //- rjf: toggles
        for(U64 idx = 0; idx < ArrayCount(DEV_toggle_table); idx += 1)
        {
          if(ui_clicked(df_icon_button(ws, *DEV_toggle_table[idx].value_ptr ? DF_IconKind_CheckFilled : DF_IconKind_CheckHollow, 0, DEV_toggle_table[idx].name)))
          {
            *DEV_toggle_table[idx].value_ptr ^= 1;
          }
        }
        
        ui_divider(ui_em(1.f, 1.f));
        
        //- rjf: draw current interaction regs
        {
          DF_InteractRegs *regs = df_interact_regs();
#define Handle(name) ui_labelf("%s: [0x%I64x, 0x%I64x]", #name, (regs->name).u64[0], (regs->name).u64[1])
          Handle(window);
          Handle(panel);
          Handle(view);
          Handle(module);
          Handle(process);
          Handle(thread);
          Handle(file);
#undef Handle
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
        
        ui_divider(ui_em(1.f, 1.f));
        
        //- rjf: draw entity tree
        DF_EntityRec rec = {0};
        S32 indent = 0;
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("Entity Tree:");
        for(DF_Entity *e = df_entity_root(); !df_entity_is_nil(e); e = rec.next)
        {
          ui_set_next_pref_width(ui_children_sum(1));
          ui_set_next_pref_height(ui_children_sum(1));
          UI_Row
          {
            ui_spacer(ui_em(2.f*indent, 1.f));
            if(e->kind == DF_EntityKind_OverrideFileLink)
            {
              DF_Entity *dst = df_entity_from_handle(e->entity_handle);
              ui_labelf("[link] %S -> %S", e->name, dst->name);
            }
            else
            {
              ui_labelf("%S: %S", df_g_entity_kind_display_string_table[e->kind], e->name);
            }
          }
          rec = df_entity_rec_df_pre(e, df_entity_root());
          indent += rec.push_count;
          indent -= rec.pop_count;
        }
      }
    }
    
    ////////////////////////////
    //- rjf: universal ctx menus
    //
    DF_Palette(ws, DF_PaletteCode_Floating)
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
      
      //- rjf: code ctx menu
      UI_CtxMenu(ws->code_ctx_menu_key)
        UI_PrefWidth(ui_em(40.f, 1.f))
        DF_Palette(ws, DF_PaletteCode_ImplicitButton)
      {
        TXT_Scope *txt_scope = txt_scope_open();
        HS_Scope *hs_scope = hs_scope_open();
        DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(ws);
        TxtRng range = ws->code_ctx_menu_range;
        DF_LineList lines = ws->code_ctx_menu_lines;
        if(!txt_pt_match(range.min, range.max) && ui_clicked(df_cmd_spec_button(ws, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Copy))))
        {
          U128 hash = {0};
          TXT_TextInfo info = txt_text_info_from_key_lang(txt_scope, ws->code_ctx_menu_text_key, ws->code_ctx_menu_lang_kind, &hash);
          String8 data = hs_data_from_hash(hs_scope, hash);
          String8 copy_data = txt_string_from_info_data_txt_rng(&info, data, ws->code_ctx_menu_range);
          os_set_clipboard_text(copy_data);
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_RightArrow, 0, "Set Next Statement")))
        {
          DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
          U64 new_rip_vaddr = ws->code_ctx_menu_vaddr;
          if(!df_entity_is_nil(df_entity_from_handle(ws->code_ctx_menu_file)))
          {
            for(DF_LineNode *n = lines.first; n != 0; n = n->next)
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
          DF_CmdParams p = df_cmd_params_from_window(ws);
          p.entity = df_handle_from_entity(thread);
          p.vaddr = new_rip_vaddr;
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Play, 0, "Run To Line")))
        {
          if(!df_entity_is_nil(df_entity_from_handle(ws->code_ctx_menu_file)))
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.entity = ws->code_ctx_menu_file;
            p.text_point = range.min;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToLine));
          }
          else
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.vaddr = ws->code_ctx_menu_vaddr;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunToAddress));
          }
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Null, 0, "Go To Name")))
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
          DF_CmdParams p = df_cmd_params_from_window(ws);
          p.string = expr;
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_GoToName));
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_CircleFilled, 0, "Toggle Breakpoint")))
        {
          if(ws->code_ctx_menu_vaddr != 0)
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.vaddr = ws->code_ctx_menu_vaddr;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddressBreakpoint));
          }
          else
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.entity = ws->code_ctx_menu_file;
            p.text_point = range.min;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_TextBreakpoint));
          }
          ui_ctx_menu_close();
        }
        if(range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Binoculars, 0, "Toggle Watch Expression")))
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
          DF_CmdParams p = df_cmd_params_from_window(ws);
          p.string = expr;
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ToggleWatchExpression));
          ui_ctx_menu_close();
        }
        if(df_entity_is_nil(df_entity_from_handle(ws->code_ctx_menu_file)) && range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_FileOutline, 0, "Go To Source")))
        {
          if(lines.first != 0)
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.file_path = df_full_path_from_entity(scratch.arena, df_entity_from_handle(lines.first->v.file));
            params.text_point = lines.first->v.pt;
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_FilePath);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_TextPoint);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
          }
          ui_ctx_menu_close();
        }
        if(!df_entity_is_nil(df_entity_from_handle(ws->code_ctx_menu_file)) && range.min.line == range.max.line && ui_clicked(df_icon_buttonf(ws, DF_IconKind_FileOutline, 0, "Go To Disassembly")))
        {
          DF_Entity *thread = df_entity_from_handle(ctrl_ctx.thread);
          U64 vaddr = 0;
          for(DF_LineNode *n = lines.first; n != 0; n = n->next)
          {
            DF_EntityList modules = df_modules_from_dbgi_key(scratch.arena, &n->v.dbgi_key);
            DF_Entity *module = df_module_from_thread_candidates(thread, &modules);
            if(!df_entity_is_nil(module))
            {
              vaddr = df_vaddr_from_voff(module, n->v.voff_range.min);
              break;
            }
          }
          DF_CmdParams params = df_cmd_params_from_window(ws);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_VirtualAddr);
          params.entity = df_handle_from_entity(thread);
          params.vaddr = vaddr;
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FindCodeLocation));
          ui_ctx_menu_close();
        }
        hs_scope_close(hs_scope);
        txt_scope_close(txt_scope);
      }
      
      //- rjf: entity menu
      UI_CtxMenu(ws->entity_ctx_menu_key)
        UI_PrefWidth(ui_em(40.f, 1.f))
        DF_Palette(ws, DF_PaletteCode_ImplicitButton)
      {
        DF_Entity *entity = df_entity_from_handle(ws->entity_ctx_menu_entity);
        DF_IconKind entity_icon = df_g_entity_kind_icon_kind_table[entity->kind];
        DF_EntityKindFlags kind_flags = df_g_entity_kind_flags_table[entity->kind];
        DF_EntityOpFlags op_flags = df_g_entity_kind_op_flags_table[entity->kind];
        String8 display_name = df_display_string_from_entity(scratch.arena, entity);
        
        // rjf: title
        UI_Row
        {
          ui_spacer(ui_em(1.f, 1.f));
          DF_Font(ws, DF_FontSlot_Icons)
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_PrefWidth(ui_em(2.f, 1.f))
            UI_PrefHeight(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_Flags(UI_BoxFlag_DrawTextWeak)
            ui_label(df_g_icon_kind_text_table[entity_icon]);
          UI_PrefWidth(ui_text_dim(10, 1))
            UI_Flags(UI_BoxFlag_DrawTextWeak)
            ui_label(df_g_entity_kind_display_string_table[entity->kind]);
          {
            UI_Palette *palette = ui_top_palette();
            if(entity->flags & DF_EntityFlag_HasColor)
            {
              palette = ui_build_palette(ui_top_palette(), .text = df_rgba_from_entity(entity));
            }
            UI_Palette(palette)
              UI_PrefWidth(ui_text_dim(10, 1))
              DF_Font(ws, (kind_flags & DF_EntityKindFlag_NameIsCode) ? DF_FontSlot_Code : DF_FontSlot_Main)
              ui_label(display_name);
          }
        }
        
        DF_Palette(ws, DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
        
        // rjf: name editor
        if(op_flags & DF_EntityOpFlag_Rename) UI_TextPadding(ui_top_font_size()*1.5f)
        {
          UI_Signal sig = df_line_editf(ws, DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, entity->name, "%S###entity_name_edit_%p", df_g_entity_kind_name_label_table[entity->kind], entity);
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
        if(op_flags & DF_EntityOpFlag_Condition)
          DF_Font(ws, DF_FontSlot_Code)
          UI_TextPadding(ui_top_font_size()*1.5f)
        {
          DF_Entity *condition = df_entity_child_from_kind(entity, DF_EntityKind_Condition);
          UI_Signal sig = df_line_editf(ws, DF_LineEditFlag_Border|DF_LineEditFlag_CodeContents, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, condition->name, "Condition###entity_cond_edit_%p", entity);
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
        if(entity->kind == DF_EntityKind_Target) UI_TextPadding(ui_top_font_size()*1.5f)
        {
          DF_Entity *exe = df_entity_child_from_kind(entity, DF_EntityKind_Executable);
          UI_Signal sig = df_line_editf(ws, DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, exe->name, "Executable###entity_exe_edit_%p", entity);
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
        if(entity->kind == DF_EntityKind_Target) UI_TextPadding(ui_top_font_size()*1.5f)
        {
          DF_Entity *args = df_entity_child_from_kind(entity, DF_EntityKind_Arguments);
          UI_Signal sig = df_line_editf(ws, DF_LineEditFlag_Border, 0, 0, &ws->entity_ctx_menu_input_cursor, &ws->entity_ctx_menu_input_mark, ws->entity_ctx_menu_input_buffer, sizeof(ws->entity_ctx_menu_input_buffer), &ws->entity_ctx_menu_input_size, 0, args->name, "Arguments###entity_args_edit_%p", entity);
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
        if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Name")))
        {
          os_set_clipboard_text(display_name);
          ui_ctx_menu_close();
        }
        
        // rjf: is command line only? -> make permanent
        if(entity->cfg_src == DF_CfgSrc_CommandLine && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Save, 0, "Save To Project")))
        {
          df_entity_equip_cfg_src(entity, DF_CfgSrc_Project);
        }
        
        // rjf: duplicate
        if(op_flags & DF_EntityOpFlag_Duplicate && ui_clicked(df_icon_buttonf(ws, DF_IconKind_XSplit, 0, "Duplicate")))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_DuplicateEntity));
          ui_ctx_menu_close();
        }
        
        // rjf: edit
        if(op_flags & DF_EntityOpFlag_Edit && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Pencil, 0, "Edit")))
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          params.entity = df_handle_from_entity(entity);
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EditEntity));
          ui_ctx_menu_close();
        }
        
        // rjf: deletion
        if(op_flags & DF_EntityOpFlag_Delete && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Trash, 0, "Delete")))
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
          if(!is_enabled && ui_clicked(df_icon_buttonf(ws, DF_IconKind_CheckHollow, 0, "Enable###enabler")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_EnableEntity));
          }
          if(is_enabled && ui_clicked(df_icon_buttonf(ws, DF_IconKind_CheckFilled, 0, "Disable###enabler")))
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
          ui_set_next_palette(df_palette_from_code(ws, is_frozen ? DF_PaletteCode_NegativePopButton : DF_PaletteCode_PositivePopButton));
          if(is_frozen && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Locked, 0, "Thaw###freeze_thaw")))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            params.entity = df_handle_from_entity(entity);
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ThawEntity));
          }
          if(!is_frozen && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Unlocked, 0, "Freeze###freeze_thaw")))
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
          if(!df_entity_is_nil(file_ancestor) && ui_clicked(df_icon_buttonf(ws, DF_IconKind_FileOutline, 0, "Go To Location")))
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
          if(entity->vaddr != 0 && !df_entity_is_nil(thread) && ui_clicked(df_icon_buttonf(ws, DF_IconKind_FileOutline, 0, "Go To Location")))
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
               ui_clicked(df_icon_buttonf(ws, DF_IconKind_FolderOpenOutline, 0, "Open File In Folder")))
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
               ui_clicked(df_icon_buttonf(ws, DF_IconKind_FileOutline, 0, "Go To File")))
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
                df_icon_buttonf(ws, DF_IconKind_Thread, 0, "[Selected]###select_entity");
              }
              else if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Thread, 0, "Select###select_entity")))
              {
                DF_CmdParams params = df_cmd_params_from_window(ws);
                params.entity = df_handle_from_entity(entity);
                df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
                ui_ctx_menu_close();
              }
            }
            
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy ID")))
            {
              U32 ctrl_id = entity->ctrl_id;
              String8 string = push_str8f(scratch.arena, "%i", (int)ctrl_id);
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            
            if(entity->kind == DF_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Instruction Pointer Address")))
              {
                U64 rip = df_query_cached_rip_from_thread(entity);
                String8 string = push_str8f(scratch.arena, "0x%I64x", rip);
                os_set_clipboard_text(string);
                ui_ctx_menu_close();
              }
            }
            
            if(entity->kind == DF_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Call Stack")))
              {
                DI_Scope *di_scope = di_scope_open();
                DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
                CTRL_Unwind base_unwind = df_query_cached_unwind_from_thread(entity);
                DF_Unwind rich_unwind = df_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
                String8List lines = {0};
                for(U64 frame_idx = 0; frame_idx < rich_unwind.frames.concrete_frame_count; frame_idx += 1)
                {
                  DF_UnwindFrame *concrete_frame = &rich_unwind.frames.v[frame_idx];
                  U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, concrete_frame->regs);
                  DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
                  RDI_Parsed *rdi = concrete_frame->rdi;
                  RDI_Procedure *procedure = concrete_frame->procedure;
                  for(DF_UnwindInlineFrame *inline_frame = concrete_frame->last_inline_frame;
                      inline_frame != 0;
                      inline_frame = inline_frame->prev)
                  {
                    RDI_InlineSite *inline_site = inline_frame->inline_site;
                    String8 name = {0};
                    name.str = rdi_string_from_idx(rdi, inline_site->name_string_idx, &name.size);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [inlined] \"%S\"%s%S", rip_vaddr, name, df_entity_is_nil(module) ? "" : " in ", module->name);
                  }
                  if(procedure != 0)
                  {
                    String8 name = {0};
                    name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: \"%S\"%s%S", rip_vaddr, name, df_entity_is_nil(module) ? "" : " in ", module->name);
                  }
                  else if(!df_entity_is_nil(module))
                  {
                    str8_list_pushf(scratch.arena, &lines, "0x%I64x: [??? in %S]", rip_vaddr, module->name);
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
            
            if(entity->kind == DF_EntityKind_Thread)
            {
              if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_FileOutline, 0, "Find")))
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
            UI_Signal copy_full_path_sig = df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Full Path");
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
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Base Address")))
            {
              Rng1U64 vaddr_rng = entity->vaddr_rng;
              String8 string = push_str8f(scratch.arena, "0x%I64x", vaddr_rng.min);
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Address Range Size")))
            {
              Rng1U64 vaddr_rng = entity->vaddr_rng;
              String8 string = push_str8f(scratch.arena, "0x%I64x", dim_1u64(vaddr_rng));
              os_set_clipboard_text(string);
              ui_ctx_menu_close();
            }
          }break;
          
          case DF_EntityKind_Target:
          {
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Play, 0, "Launch And Run")))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
              ui_ctx_menu_close();
            }
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_PlayStepForward, 0, "Launch And Initialize")))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(entity);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndInit));
              ui_ctx_menu_close();
            }
          }break;
        }
        
        DF_Palette(ws, DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
        
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
              DF_Palette(ws, DF_PaletteCode_Floating)
            {
              if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Trash, 0, "Remove Color###color_toggle")))
              {
                entity->flags &= ~DF_EntityFlag_HasColor;
              }
            }
            
            ui_spacer(ui_em(1.5f, 1.f));
          }
          if(!entity_has_color && ui_clicked(df_icon_buttonf(ws, DF_IconKind_Palette, 0, "Apply Color###color_toggle")))
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
      UI_CtxMenu(ws->tab_ctx_menu_key) UI_PrefWidth(ui_em(40.f, 1.f)) UI_CornerRadius(0)
        DF_Palette(ws, DF_PaletteCode_ImplicitButton)
      {
        DF_Panel *panel = df_panel_from_handle(ws->tab_ctx_menu_panel);
        DF_View *view = df_view_from_handle(ws->tab_ctx_menu_view);
        DF_IconKind view_icon = df_icon_kind_from_view(view);
        DF_Entity *entity = df_entity_from_handle(view->entity);
        DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
        String8 display_name = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
        
        // rjf: title
        UI_Row
        {
          ui_spacer(ui_em(1.f, 1.f));
          DF_Font(ws, DF_FontSlot_Icons)
            UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
            UI_PrefWidth(ui_em(2.f, 1.f))
            UI_PrefHeight(ui_pct(1, 0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
            ui_label(df_g_icon_kind_text_table[view_icon]);
          UI_PrefWidth(ui_text_dim(10, 1)) ui_label(display_name);
        }
        
        DF_Palette(ws, DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
        
        // rjf: copy name
        if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Name")))
        {
          os_set_clipboard_text(display_name);
          ui_ctx_menu_close();
        }
        
        // rjf: copy full path
        if(entity->kind == DF_EntityKind_File)
        {
          UI_Signal copy_full_path_sig = df_icon_buttonf(ws, DF_IconKind_Clipboard, 0, "Copy Full Path");
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
        
        // rjf: show in explorer
        if(entity->kind == DF_EntityKind_File)
        {
          UI_Signal sig = df_icon_buttonf(ws, DF_IconKind_FolderClosedFilled, 0, "Show In Explorer");
          if(ui_clicked(sig))
          {
            String8 full_path = df_full_path_from_entity(scratch.arena, entity);
            os_show_in_filesystem_ui(full_path);
            ui_ctx_menu_close();
          }
        }
        
        // rjf: filter controls
        if(view->spec->info.flags & DF_ViewSpecFlag_CanFilter)
        {
          if(ui_clicked(df_cmd_spec_button(ws, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter))))
          {
            DF_CmdParams params = df_cmd_params_from_window(ws);
            {
              params.view = df_handle_from_view(view);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_View);
            }
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter));
            ui_ctx_menu_close();
          }
          if(ui_clicked(df_cmd_spec_button(ws, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ClearFilter))))
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
        if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_X, 0, "Close Tab")))
        {
          DF_CmdParams params = df_cmd_params_from_view(ws, panel, view);
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
      if(df_gfx_state->confirm_t > 0.005f) UI_TextAlignment(UI_TextAlign_Center) UI_Focus(df_gfx_state->confirm_active ? UI_FocusKind_Root : UI_FocusKind_Off)
      {
        Vec2F32 window_dim = dim_2f32(window_rect);
        UI_Box *bg_box = &ui_g_nil_box;
        UI_Palette *palette = ui_build_palette(df_palette_from_code(ws, DF_PaletteCode_Floating));
        palette->background.w *= df_gfx_state->confirm_t;
        UI_Rect(window_rect)
          UI_ChildLayoutAxis(Axis2_X)
          UI_Focus(UI_FocusKind_On)
          UI_BlurSize(10*df_gfx_state->confirm_t)
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
        if(df_gfx_state->confirm_active) UI_Parent(bg_box) UI_Transparency(1-df_gfx_state->confirm_t)
        {
          ui_ctx_menu_close();
          UI_WidthFill UI_PrefHeight(ui_children_sum(1.f)) UI_Column UI_Padding(ui_pct(1, 0))
          {
            UI_TextRasterFlags(df_raster_flags_from_slot(ws, DF_FontSlot_Main)) UI_FontSize(ui_top_font_size()*2.f) UI_PrefHeight(ui_em(3.f, 1.f)) ui_label(df_gfx_state->confirm_title);
            UI_PrefHeight(ui_em(3.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_label(df_gfx_state->confirm_msg);
            ui_spacer(ui_em(1.5f, 1.f));
            UI_Row UI_Padding(ui_pct(1.f, 0.f)) UI_WidthFill UI_PrefHeight(ui_em(5.f, 1.f))
            {
              UI_CornerRadius00(ui_top_font_size()*0.25f)
                UI_CornerRadius01(ui_top_font_size()*0.25f)
                DF_Palette(ws, DF_PaletteCode_NeutralPopButton)
                if(ui_clicked(ui_buttonf("OK")) || (ui_key_match(bg_box->default_nav_focus_hot_key, ui_key_zero()) && ui_slot_press(UI_EventActionSlot_Accept)))
              {
                DF_CmdParams p = df_cmd_params_zero();
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ConfirmAccept));
              }
              UI_CornerRadius10(ui_top_font_size()*0.25f)
                UI_CornerRadius11(ui_top_font_size()*0.25f)
                if(ui_clicked(ui_buttonf("Cancel")) || ui_slot_press(UI_EventActionSlot_Cancel))
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
        DI_Key dbgi_key = df_dbgi_key_from_module(module);
        
        //- rjf: gather lister items
        DF_AutoCompListerItemChunkList item_list = {0};
        {
          //- rjf: gather locals
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Locals)
          {
            EVAL_String2NumMap *locals_map = df_query_cached_locals_map_from_dbgi_key_voff(&dbgi_key, thread_rip_voff);
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
          
          //- rjf: gather registers
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Registers)
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
          
          //- rjf: gather view rules
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_ViewRules)
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
              if(query.size == 0 || item.matches.count != 0)
              {
                df_autocomp_lister_item_chunk_list_push(scratch.arena, &item_list, 256, &item);
              }
            }
          }
          
          //- rjf: gather architectures
          if(ws->autocomp_lister_params.flags & DF_AutoCompListerFlag_Architectures)
          {
            for(EachNonZeroEnumVal(Architecture, arch))
            {
              DF_AutoCompListerItem item = {0};
              {
                item.string      = string_from_architecture(arch);
                item.kind_string = str8_lit("Architecture");
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
            F32 rate = df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32 ? (1 - pow_f32(2, (-60.f * df_dt()))) : 1.f;
            F32 target = Min((F32)item_array.count, 16.f);
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
            F32 rate = df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * df_dt())) : 1.f;
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
          ui_set_next_pref_width(ui_em(30.f, 1.f));
          ui_set_next_pref_height(ui_px(row_height_px*ws->autocomp_num_visible_rows_t + ui_top_font_size()*2.f, 1.f));
          ui_set_next_child_layout_axis(Axis2_Y);
          ui_set_next_corner_radius_01(ui_top_font_size()*0.25f);
          ui_set_next_corner_radius_11(ui_top_font_size()*0.25f);
          ui_set_next_corner_radius_10(ui_top_font_size()*0.25f);
          UI_Focus(UI_FocusKind_On)
            UI_Squish(0.25f-0.25f*ws->autocomp_open_t)
            UI_Transparency(1.f-ws->autocomp_open_t)
            DF_Palette(ws, DF_PaletteCode_Floating)
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
            DF_Font(ws, DF_FontSlot_Code)
            UI_HoverCursor(OS_Cursor_HandPoint)
            UI_Focus(UI_FocusKind_Null)
            DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
                DF_Font(ws, DF_FontSlot_Main)
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
      DF_Palette(ws, DF_PaletteCode_MenuBar)
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
              R_Handle texture = df_gfx_state->icon_texture;
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
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(file_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
            {
              DF_CoreCmdKind cmds[] =
              {
                DF_CoreCmdKind_Open,
                DF_CoreCmdKind_OpenUser,
                DF_CoreCmdKind_OpenProject,
                DF_CoreCmdKind_OpenRecentProject,
                DF_CoreCmdKind_Exit,
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
              df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: window menu
            UI_Key window_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_window_menu_key_"));
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(window_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(panel_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
                DF_CoreCmdKind_ResetToCompactPanels,
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
              df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: view menu
            UI_Key view_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_view_menu_key_"));
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(view_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
                DF_CoreCmdKind_Settings,
                DF_CoreCmdKind_ExceptionFilters,
                DF_CoreCmdKind_GettingStarted,
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
              df_cmd_list_menu_buttons(ws, ArrayCount(cmds), cmds, codepoints);
            }
            
            // rjf: targets menu
            UI_Key targets_menu_key = ui_key_from_string(ui_key_zero(), str8_lit("_targets_menu_key_"));
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(targets_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
              DF_Palette(ws, DF_PaletteCode_Floating) ui_divider(ui_em(1.f, 1.f));
              DF_EntityList targets_list = df_query_cached_entity_list_with_kind(DF_EntityKind_Target);
              for(DF_EntityNode *n = targets_list.first; n != 0; n = n->next)
              {
                DF_Entity *target = n->entity;
                UI_Palette *palette = ui_top_palette();
                if(target->flags & DF_EntityFlag_HasColor)
                {
                  palette = ui_build_palette(ui_top_palette(), .text = df_rgba_from_entity(target));
                }
                String8 target_name = df_display_string_from_entity(scratch.arena, target);
                UI_Signal sig = {0};
                UI_Palette(palette) sig = df_icon_buttonf(ws, DF_IconKind_Target, 0, "%S##%p", target_name, target);
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
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(ctrl_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
            DF_Palette(ws, DF_PaletteCode_Floating)
              UI_CtxMenu(help_menu_key)
              UI_PrefWidth(ui_em(50.f, 1.f))
              DF_Palette(ws, DF_PaletteCode_ImplicitButton)
            {
              UI_Row UI_TextAlignment(UI_TextAlign_Center) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                ui_label(str8_lit(BUILD_TITLE_STRING_LITERAL));
              UI_PrefHeight(ui_children_sum(1)) UI_Row UI_Padding(ui_pct(1, 0))
              {
                R_Handle texture = df_gfx_state->icon_texture;
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
                DF_CmdSpec *spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand);
                UI_Flags(UI_BoxFlag_DrawBorder)
                  UI_TextAlignment(UI_TextAlign_Center)
                  df_cmd_binding_buttons(ws, spec);
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
                UI_EventList *events = ui_events();
                for(UI_EventNode *n = events->first, *next = 0;
                    n != 0;
                    n = next)
                {
                  next = n->next;
                  UI_Event *evt = &n->v;
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
                    ui_eat_event(events, n);
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
          UI_PrefWidth(ui_text_dim(10, 1)) UI_HeightFill
            DF_Palette(ws, DF_PaletteCode_NeutralPopButton)
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
          DF_Font(ws, DF_FontSlot_Icons)
          UI_FontSize(ui_top_font_size()*0.85f)
        {
          Temp scratch = scratch_begin(&arena, 1);
          DF_EntityList targets = df_push_active_target_list(scratch.arena);
          DF_EntityList processes = df_query_cached_entity_list_with_kind(DF_EntityKind_Process);
          B32 have_targets = targets.count != 0;
          B32 can_send_signal = !df_ctrl_targets_running();
          B32 can_play  = (have_targets && (can_send_signal || df_ctrl_last_run_frame_idx()+4 > df_frame_index()));
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
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: %s", have_targets ? "Targets are currently running" : "No active targets exist");
            }
            if(ui_hovering(sig) && can_play)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
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
                    String8 target_display_name = df_display_string_from_entity(scratch.arena, n->entity);
                    ui_label(target_display_name);
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
          
          //- rjf: restart button
          else UI_TextAlignment(UI_TextAlign_Center)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextPositive)))
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Redo]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig))
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
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
                      String8 target_display_name = df_display_string_from_entity(scratch.arena, target);
                      ui_label(target_display_name);
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
          
          //- rjf: pause button
          UI_TextAlignment(UI_TextAlign_Center) UI_Flags(can_pause ? 0 : UI_BoxFlag_Disabled)
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextNeutral)))
          {
            UI_Signal sig = ui_button(df_g_icon_kind_text_table[DF_IconKind_Pause]);
            os_window_push_custom_title_bar_client_area(ws->os, sig.box->rect);
            if(ui_hovering(sig) && !can_pause)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: Already halted");
            }
            if(ui_hovering(sig) && can_pause)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Halt all target processes");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Halt));
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
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_stop)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Kill all target processes");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Kill));
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
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Step Over");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_StepOver));
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
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Step Into");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_StepInto));
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
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: Running");
            }
            if(ui_hovering(sig) && !can_step && !can_stop)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Disabled: No processes are running");
            }
            if(ui_hovering(sig) && can_step)
            {
              UI_Tooltip
                DF_Font(ws, DF_FontSlot_Main)
                UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
                ui_labelf("Step Out");
            }
            if(ui_clicked(sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_StepOut));
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
          if(do_user_prof) DF_Palette(ws, DF_PaletteCode_NeutralPopButton)
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
              String8 user_path = df_cfg_path_from_src(DF_CfgSrc_User);
              user_path = str8_chop_last_dot(user_path);
              DF_Font(ws, DF_FontSlot_Icons)
                UI_TextRasterFlags(df_raster_flags_from_slot(ws, DF_FontSlot_Icons))
                ui_label(df_g_icon_kind_text_table[DF_IconKind_Person]);
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
          
          if(do_user_prof)
          {
            ui_spacer(ui_em(0.75f, 0));
          }
          
          // rjf: loaded project viz
          if(do_user_prof) DF_Palette(ws, DF_PaletteCode_NeutralPopButton)
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
              String8 prof_path = df_cfg_path_from_src(DF_CfgSrc_Project);
              prof_path = str8_chop_last_dot(prof_path);
              DF_Font(ws, DF_FontSlot_Icons)
                ui_label(df_g_icon_kind_text_table[DF_IconKind_Briefcase]);
              ui_label(str8_skip_last_slash(prof_path));
            }
            UI_Signal prof_sig = ui_signal_from_box(prof_box);
            if(ui_clicked(prof_sig))
            {
              DF_CmdParams p = df_cmd_params_from_window(ws);
              p.cmd_spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_OpenProject);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_CmdSpec);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_RunCommand));
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
              min_sig = df_icon_buttonf(ws, DF_IconKind_Minus,  0, "##minimize");
              max_sig = df_icon_buttonf(ws, DF_IconKind_Window, 0, "##maximize");
            }
            UI_PrefWidth(ui_px(button_dim, 1.f))
              DF_Palette(ws, DF_PaletteCode_NegativePopButton)
            {
              cls_sig = df_icon_buttonf(ws, DF_IconKind_X,      0, "##close");
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
              DF_CmdParams p = df_cmd_params_from_window(ws);
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
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
      B32 is_running = df_ctrl_targets_running() && df_ctrl_last_run_frame_idx() < df_frame_index();
      CTRL_Event stop_event = df_ctrl_last_stop_event();
      UI_Palette *positive_scheme = df_palette_from_code(ws, DF_PaletteCode_PositivePopButton);
      UI_Palette *running_scheme = df_palette_from_code(ws, DF_PaletteCode_NeutralPopButton);
      UI_Palette *negative_scheme = df_palette_from_code(ws, DF_PaletteCode_NegativePopButton);
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
        for(EachEnumVal(UI_ColorCode, code))
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
                DF_Font(ws, DF_FontSlot_Icons)
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
            DF_Palette(ws, DF_PaletteCode_NeutralPopButton)
            ui_labelf("Currently rebinding \"%S\" hotkey", df_gfx_state->bind_change_cmd_spec->info.display_name);
        }
        
        // rjf: error visualization
        else if(ws->error_t >= 0.01f)
        {
          ws->error_t -= df_dt()/8.f;
          df_gfx_request_frame();
          String8 error_string = str8(ws->error_buffer, ws->error_string_size);
          if(error_string.size != 0)
          {
            ui_set_next_pref_width(ui_children_sum(1));
            UI_CornerRadius(4)
              UI_Row
              UI_PrefWidth(ui_text_dim(10, 1))
              UI_TextAlignment(UI_TextAlign_Center)
            {
              DF_Font(ws, DF_FontSlot_Icons)
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
        df_view_equip_spec(ws, view, view_spec, &df_g_nil_entity, default_query, &df_g_nil_cfg_node);
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
      F32 rate = df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * df_dt())) : 1.f;
      
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
      DF_Palette(ws, DF_PaletteCode_Floating)
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
            DF_IconKind icon_kind = ws->query_cmd_spec->info.canonical_icon_kind;
            if(icon_kind != DF_IconKind_Null)
            {
              DF_Font(ws, DF_FontSlot_Icons) ui_label(df_g_icon_kind_text_table[icon_kind]);
            }
            ui_labelf("%S", ws->query_cmd_spec->info.display_name);
          }
          DF_Font(ws, (query->flags & DF_CmdQueryFlag_CodeInput) ? DF_FontSlot_Code : DF_FontSlot_Main)
            UI_TextPadding(ui_top_font_size()*0.5f)
          {
            UI_Signal sig = df_line_edit(ws, DF_LineEditFlag_Border|
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
          UI_PrefWidth(ui_em(5.f, 1.f)) UI_Focus(UI_FocusKind_Off) DF_Palette(ws, DF_PaletteCode_PositivePopButton)
          {
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_RightArrow, 0, "##complete_query")))
            {
              query_completed = 1;
            }
          }
          UI_PrefWidth(ui_em(3.f, 1.f)) UI_Focus(UI_FocusKind_Off) DF_Palette(ws, DF_PaletteCode_PlainButton)
          {
            if(ui_clicked(df_icon_buttonf(ws, DF_IconKind_X, 0, "##cancel_query")))
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
        DF_ViewUIFunctionType *build_view_ui_function = view_spec->info.ui_hook;
        build_view_ui_function(ws, &df_g_nil_panel, view, query_container_content_rect);
      }
      
      //- rjf: query submission
      if(((ui_is_focus_active() || (window_is_focused && !ui_any_ctx_menu_is_open() && !ws->menu_bar_focused && !ws->query_view_selected)) &&
          ui_slot_press(UI_EventActionSlot_Cancel)) || query_cancelled)
      {
        DF_CmdParams params = df_cmd_params_from_window(ws);
        df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CancelQuery));
      }
      if((ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept)) || query_completed)
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
        DF_Font(ws, DF_FontSlot_Code)
        UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
        DF_Palette(ws, DF_PaletteCode_Floating)
      {
        Temp scratch = scratch_begin(&arena, 1);
        DI_Scope *scope = di_scope_open();
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
          UI_Focus((hover_eval_is_open && !ui_any_ctx_menu_is_open() && ws->hover_eval_focused && (!query_is_open || !ws->query_view_selected)) ? UI_FocusKind_Null : UI_FocusKind_Off)
        {
          //- rjf: eval -> viz artifacts
          F32 row_height = floor_f32(ui_top_font_size()*2.5f);
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
              F32 fish_rate = df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * df_dt())) : 1.f;
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
              F32 fish_rate = df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-60.f * df_dt())) : 1.f;
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
            F32 expr_column_width_px = 0;
            
            //- rjf: build rows
            for(DF_EvalVizRow *row = viz_rows.first; row != 0; row = row->next)
            {
              //- rjf: calculate width of exp row
              if(row == viz_rows.first)
              {
                expr_column_width_px = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), row->display_expr).x + ui_top_font_size()*2.5f;
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
                    UI_Flags(UI_BoxFlag_DrawSideLeft*(row->depth>0) | UI_BoxFlag_DrawTextWeak)
                    DF_Font(ws, DF_FontSlot_Icons)
                    ui_label(df_g_icon_kind_text_table[DF_IconKind_Dot]);
                }
                UI_WidthFill UI_TextRasterFlags(df_raster_flags_from_slot(ws, DF_FontSlot_Code))
                {
                  UI_PrefWidth(ui_px(expr_column_width_px, 1.f)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), row->display_expr);
                  ui_spacer(ui_em(1.5f, 1.f));
                  if(row->flags & DF_EvalVizRowFlag_CanEditValue)
                  {
                    if(row_is_fresh)
                    {
                      Vec4F32 rgba = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rgba));
                    }
                    UI_Signal sig = df_line_editf(ws, DF_LineEditFlag_CodeContents|
                                                  DF_LineEditFlag_DisplayStringIsCode|
                                                  DF_LineEditFlag_PreferDisplayString|
                                                  DF_LineEditFlag_Border,
                                                  0, 0, &ws->hover_eval_txt_cursor, &ws->hover_eval_txt_mark, ws->hover_eval_txt_buffer, sizeof(ws->hover_eval_txt_buffer), &ws->hover_eval_txt_size, 0, row->edit_value, "%S###val_%I64x", row->display_value, row_hash);
                    if(ui_pressed(sig))
                    {
                      ws->hover_eval_focused = 1;
                    }
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
                      Vec4F32 rgba = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
                      ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rgba));
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
                    UI_Signal watch_sig = df_icon_buttonf(ws, DF_IconKind_List, 0, "###watch_hover_eval");
                    if(ui_hovering(watch_sig)) UI_Tooltip DF_Font(ws, DF_FontSlot_Main) UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
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
                  if(!df_entity_is_nil(df_entity_from_handle(ws->hover_eval_file)) || ws->hover_eval_vaddr != 0)
                    UI_TextAlignment(UI_TextAlign_Center) UI_PrefWidth(ui_em(3.f, 1.f))
                    UI_CornerRadius10(corner_radius)
                    UI_CornerRadius11(corner_radius)
                  {
                    UI_Signal pin_sig = df_icon_buttonf(ws, DF_IconKind_Pin, 0, "###pin_hover_eval");
                    if(ui_hovering(pin_sig)) UI_Tooltip DF_Font(ws, DF_FontSlot_Main) UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Main))
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
      for(DF_Panel *panel = ws->root_panel;
          !df_panel_is_nil(panel);
          panel = df_panel_rec_df_pre(panel).next)
    {
      //////////////////////////
      //- rjf: continue on leaf panels
      //
      if(df_panel_is_nil(panel->first))
      {
        continue;
      }
      
      //////////////////////////
      //- rjf: grab info
      //
      Axis2 split_axis = panel->split_axis;
      Rng2F32 panel_rect = df_target_rect_from_panel(content_rect, ws->root_panel, panel);
      
      //////////////////////////
      //- rjf: boundary tab-drag/drop sites
      //
      {
        DF_View *drag_view = df_view_from_handle(df_g_drag_drop_payload.view);
        if(df_drag_is_active() && !df_view_is_nil(drag_view))
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
                UI_Rect(future_split_rect) DF_Palette(ws, DF_PaletteCode_DropSiteOverlay)
                {
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
                }
              }
              
              // rjf: drop
              DF_DragDropPayload payload = {0};
              if(ui_key_match(site_box->key, ui_drop_hot_key()) && df_drag_drop(&payload))
              {
                Dir2 dir = (axis == Axis2_Y ? (side == Side_Min ? Dir2_Up : Dir2_Down) :
                            axis == Axis2_X ? (side == Side_Min ? Dir2_Left : Dir2_Right) :
                            Dir2_Invalid);
                if(dir != Dir2_Invalid)
                {
                  DF_Panel *split_panel = panel;
                  DF_CmdParams p = df_cmd_params_from_window(ws);
                  p.dest_panel = df_handle_from_panel(split_panel);
                  p.panel = payload.panel;
                  p.view = payload.view;
                  p.dir2 = dir;
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SplitPanel));
                }
              }
            }
          }
          
          //- rjf: iterate all children, build boundary drop sites
          Axis2 split_axis = panel->split_axis;
          UI_CornerRadius(corner_radius) for(DF_Panel *child = panel->first;; child = child->next)
          {
            // rjf: form rect
            Rng2F32 child_rect = df_target_rect_from_panel_child(panel_rect, panel, child);
            Vec2F32 child_rect_center = center_2f32(child_rect);
            UI_Key key = ui_key_from_stringf(ui_key_zero(), "drop_boundary_%p_%p", panel, child);
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
              UI_Rect(future_split_rect) DF_Palette(ws, DF_PaletteCode_DropSiteOverlay)
              {
                ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
              }
            }
            
            // rjf: drop
            DF_DragDropPayload payload = {0};
            if(ui_key_match(site_box->key, ui_drop_hot_key()) && df_drag_drop(&payload))
            {
              Dir2 dir = (panel->split_axis == Axis2_X ? Dir2_Left : Dir2_Up);
              DF_Panel *split_panel = child;
              if(df_panel_is_nil(split_panel))
              {
                split_panel = panel->last;
                dir = (panel->split_axis == Axis2_X ? Dir2_Right : Dir2_Down);
              }
              DF_CmdParams p = df_cmd_params_from_window(ws);
              p.dest_panel = df_handle_from_panel(split_panel);
              p.panel = payload.panel;
              p.view = payload.view;
              p.dir2 = dir;
              df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SplitPanel));
            }
            
            // rjf: exit on opl child
            if(df_panel_is_nil(child))
            {
              break;
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: do UI for drag boundaries between all children
      //
      for(DF_Panel *child = panel->first; !df_panel_is_nil(child) && !df_panel_is_nil(child->next); child = child->next)
      {
        DF_Panel *min_child = child;
        DF_Panel *max_child = min_child->next;
        Rng2F32 min_child_rect = df_target_rect_from_panel_child(panel_rect, panel, min_child);
        Rng2F32 max_child_rect = df_target_rect_from_panel_child(panel_rect, panel, max_child);
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
          if(ui_released(sig) || ui_double_clicked(sig))
          {
            df_panel_notify_mutation(ws, min_child);
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: animate panels
    //
    {
      F32 rate = df_setting_val_from_code(ws, DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-50.f * df_dt())) : 1.f;
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
          df_gfx_request_frame();
        }
        panel->animated_rect_pct.x0 += rate * (target_rect_pct.x0 - panel->animated_rect_pct.x0);
        panel->animated_rect_pct.y0 += rate * (target_rect_pct.y0 - panel->animated_rect_pct.y0);
        panel->animated_rect_pct.x1 += rate * (target_rect_pct.x1 - panel->animated_rect_pct.x1);
        panel->animated_rect_pct.y1 += rate * (target_rect_pct.y1 - panel->animated_rect_pct.y1);
        if(ws->frames_alive < 5 || is_changing_panel_boundaries || panel_reset_done)
        {
          panel->animated_rect_pct = target_rect_pct;
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
          DF_View *tab = df_selected_tab_from_panel(panel);
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
          DF_View *view = df_view_from_handle(df_g_drag_drop_payload.view);
          if(df_drag_is_active() && !df_view_is_nil(view) && contains_2f32(panel_rect, ui_mouse()))
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
                      DF_Palette(ws, DF_PaletteCode_DropSiteOverlay) ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                      ui_spacer(ui_px(padding, 1.f));
                      if(split_side == Side_Max) { ui_set_next_flags(UI_BoxFlag_DrawBackground); }
                      DF_Palette(ws, DF_PaletteCode_DropSiteOverlay) ui_build_box_from_key(UI_BoxFlag_DrawBorder, ui_key_zero());
                    }
                  }
                }
                else
                {
                  UI_Parent(site_box_viz) UI_WidthFill UI_HeightFill UI_Padding(ui_px(padding, 1.f))
                  {
                    ui_set_next_child_layout_axis(split_axis);
                    UI_Box *row_or_column = ui_build_box_from_key(0, ui_key_zero());
                    UI_Parent(row_or_column) UI_Padding(ui_px(padding, 1.f)) DF_Palette(ws, DF_PaletteCode_DropSiteOverlay)
                    {
                      ui_build_box_from_key(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground, ui_key_zero());
                    }
                  }
                }
              }
              DF_DragDropPayload payload = {0};
              if(ui_key_match(site_box->key, ui_drop_hot_key()) && df_drag_drop(&payload))
              {
                if(dir != Dir2_Invalid)
                {
                  DF_CmdParams p = df_cmd_params_from_window(ws);
                  p.dest_panel = df_handle_from_panel(panel);
                  p.panel = payload.panel;
                  p.view = payload.view;
                  p.dir2 = dir;
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SplitPanel));
                }
                else
                {
                  DF_CmdParams p = df_cmd_params_from_window(ws);
                  p.dest_panel = df_handle_from_panel(panel);
                  p.panel = payload.panel;
                  p.view = payload.view;
                  p.prev_view = df_handle_from_view(panel->last_tab_view);
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_MoveTab));
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
                UI_Rect(future_split_rect) DF_Palette(ws, DF_PaletteCode_DropSiteOverlay)
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
          DF_View *view = df_selected_tab_from_panel(panel);
          UI_Focus(UI_FocusKind_On)
          {
            if(view->is_filtering && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Accept))
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
                UI_PrefWidth(ui_em(3.f, 1.f))
                  UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                  DF_Font(ws, DF_FontSlot_Icons)
                  UI_TextAlignment(UI_TextAlign_Center)
                  ui_label(df_g_icon_kind_text_table[DF_IconKind_Find]);
                UI_PrefWidth(ui_text_dim(10, 1))
                {
                  ui_label(str8_lit("Filter"));
                }
                ui_spacer(ui_em(0.5f, 1.f));
                DF_Font(ws, view->spec->info.flags & DF_ViewSpecFlag_FilterIsCode ? DF_FontSlot_Code : DF_FontSlot_Main)
                  UI_Focus(view->is_filtering ? UI_FocusKind_On : UI_FocusKind_Off)
                  UI_TextPadding(ui_top_font_size()*0.5f)
                {
                  UI_Signal sig = df_line_edit(ws,
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
        //- rjf: panel not selected? -> darken
        //
        if(panel != ws->focused_panel)
        {
          UI_Palette(ui_build_palette(0, .background = df_rgba_from_theme_color(DF_ThemeColor_InactivePanelOverlay)))
            UI_Rect(content_rect)
          {
            ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
          }
        }
        
        //////////////////////////
        //- rjf: build panel container box
        //
        UI_Box *panel_box = &ui_g_nil_box;
        UI_Rect(content_rect) UI_ChildLayoutAxis(Axis2_Y) UI_CornerRadius(0) UI_Focus(UI_FocusKind_On)
        {
          UI_Key panel_key = df_ui_key_from_panel(panel);
          panel_box = ui_build_box_from_key(UI_BoxFlag_MouseClickable|
                                            UI_BoxFlag_Clip|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DisableFocusOverlay|
                                            ((ws->focused_panel != panel)*UI_BoxFlag_DisableFocusBorder)|
                                            ((ws->focused_panel != panel)*UI_BoxFlag_DrawOverlay),
                                            panel_key);
        }
        
        //////////////////////////
        //- rjf: loading animation for stable view
        //
        UI_Parent(panel_box)
        {
          DF_View *view = df_selected_tab_from_panel(panel);
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
            Vec4F32 bg_color = df_rgba_from_theme_color(DF_ThemeColor_FloatingBackground);
            Vec4F32 bd_color = df_rgba_from_theme_color(DF_ThemeColor_FloatingBorder);
            Vec4F32 hl_color = df_rgba_from_theme_color(DF_ThemeColor_TextNeutral);
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
                ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = v4f32(1, 1, 1, 0.2f*view->loading_t)));
                ui_set_next_fixed_x(indicator_region_rect.x0);
                ui_set_next_fixed_y(indicator_region_rect.y0);
                ui_set_next_fixed_width(dim_2f32(indicator_region_rect).x*pct_done);
                ui_set_next_fixed_height(dim_2f32(indicator_region_rect).y);
                ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
              }
              
              // rjf: fill
              ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = hl_color));
              ui_set_next_fixed_x(indicator_rect.x0);
              ui_set_next_fixed_y(indicator_rect.y0);
              ui_set_next_fixed_width(dim_2f32(indicator_rect).x);
              ui_set_next_fixed_height(dim_2f32(indicator_rect).y);
              ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_FloatingX|UI_BoxFlag_FloatingY, ui_key_zero());
              
              // rjf: animated bar
              ui_set_next_palette(ui_build_palette(ui_top_palette(), .border = bd_color, .background = bg_color));
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
              ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = bg_color));
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
          //- rjf: push interaction registers, fill with per-view states
          df_push_interact_regs();
          {
            DF_View *view = df_selected_tab_from_panel(panel);
            DF_Entity *entity = df_entity_from_handle(view->entity);
            df_interact_regs()->cursor = view->cursor;
            df_interact_regs()->mark = view->mark;
            df_interact_regs()->file = df_handle_zero();
            switch(entity->kind)
            {
              default:{}break;
              case DF_EntityKind_File:{df_interact_regs()->file = view->entity;}break;
            }
          }
          
          //- rjf: build view container
          UI_Box *view_container_box = &ui_g_nil_box;
          UI_FixedWidth(dim_2f32(content_rect).x)
            UI_FixedHeight(dim_2f32(content_rect).y)
            UI_ChildLayoutAxis(Axis2_Y)
          {
            view_container_box = ui_build_box_from_key(0, ui_key_zero());
          }
          
          //- rjf: build empty view
          UI_Parent(view_container_box) if(df_view_is_nil(df_selected_tab_from_panel(panel)))
          {
            DF_VIEW_UI_FUNCTION_NAME(Empty)(ws, panel, &df_g_nil_view, content_rect);
          }
          
          //- rjf: build tab view
          UI_Parent(view_container_box) if(!df_view_is_nil(df_selected_tab_from_panel(panel)))
          {
            DF_View *view = df_selected_tab_from_panel(panel);
            DF_ViewUIFunctionType *build_view_ui_function = view->spec->info.ui_hook;
            build_view_ui_function(ws, panel, view, content_rect);
          }
          
          //- rjf: fill with per-view states, after the view has a chance to run
          {
            DF_View *view = df_selected_tab_from_panel(panel);
            df_interact_regs()->cursor = view->cursor;
            df_interact_regs()->mark = view->mark;
          }
          
          //- rjf: pop interaction registers; commit if this is the selected view
          DF_InteractRegs *view_regs = df_pop_interact_regs();
          if(ws->focused_panel == panel)
          {
            MemoryCopyStruct(df_interact_regs(), view_regs);
          }
        }
        
        //////////////////////////
        //- rjf: take events to automatically start/end filtering, if applicable
        //
        UI_Focus(UI_FocusKind_On)
        {
          DF_View *view = df_selected_tab_from_panel(panel);
          if(ui_is_focus_active() && view->spec->info.flags & DF_ViewSpecFlag_TypingAutomaticallyFilters && !view->is_filtering)
          {
            DF_CmdParams p = df_cmd_params_from_view(ws, panel, view);
            UI_EventList *events = ui_events();
            for(UI_EventNode *n = events->first, *next = 0; n != 0; n = next)
            {
              next = n->next;
              if(n->v.flags & UI_EventFlag_Paste)
              {
                ui_eat_event(events, n);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter));
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Paste));
              }
              else if(n->v.string.size != 0 && n->v.kind == UI_EventKind_Text)
              {
                ui_eat_event(events, n);
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Filter));
                p.string = n->v.string;
                df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_InsertText));
              }
            }
          }
          if((view->query_string_size != 0 || view->is_filtering) && ui_is_focus_active() && ui_slot_press(UI_EventActionSlot_Cancel))
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
          DF_View *next_selected_tab_view = df_selected_tab_from_panel(panel);
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
            
            // rjf: build tabs
            UI_PrefWidth(ui_em(18.f, 0.5f))
              UI_CornerRadius00(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius01(panel->tab_side == Side_Min ? 0 : corner_radius)
              UI_CornerRadius10(panel->tab_side == Side_Min ? corner_radius : 0)
              UI_CornerRadius11(panel->tab_side == Side_Min ? 0 : corner_radius)
              for(DF_View *view = panel->first_tab_view;; view = view->next, view_idx += 1)
            {
              temp_end(scratch);
              if(df_view_is_project_filtered(view)) { continue; }
              
              // rjf: if before this tab is the prev-view of the current tab drag,
              // draw empty space
              if(df_drag_is_active() && catchall_drop_site_hovered)
              {
                DF_Panel *dst_panel = df_panel_from_handle(df_g_last_drag_drop_panel);
                DF_View *drag_view = df_view_from_handle(df_g_drag_drop_payload.view);
                DF_View *dst_prev_view = df_view_from_handle(df_g_last_drag_drop_prev_tab);
                if(dst_panel == panel &&
                   ((!df_view_is_nil(view) && dst_prev_view == view->prev && drag_view != view && drag_view != view->prev) ||
                    (df_view_is_nil(view) && dst_prev_view == panel->last_tab_view && drag_view != panel->last_tab_view)))
                {
                  UI_PrefWidth(ui_em(9.f, 0.2f)) UI_Column
                  {
                    ui_spacer(ui_em(0.2f, 1.f));
                    UI_CornerRadius00(corner_radius)
                      UI_CornerRadius10(corner_radius)
                      DF_Palette(ws, DF_PaletteCode_DropSiteOverlay)
                    {
                      ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                    }
                  }
                }
              }
              
              // rjf: end on nil view
              if(df_view_is_nil(view))
              {
                break;
              }
              
              // rjf: gather info for this tab
              B32 view_is_selected = (view == df_selected_tab_from_panel(panel));
              DF_IconKind icon_kind = df_icon_kind_from_view(view);
              DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_view(ws, view);
              String8 label = df_display_string_from_view(scratch.arena, ctrl_ctx, view);
              
              // rjf: begin vertical region for this tab
              ui_set_next_child_layout_axis(Axis2_Y);
              UI_Box *tab_column_box = ui_build_box_from_stringf(!is_changing_panel_boundaries*UI_BoxFlag_AnimatePosX, "tab_column_%p", view);
              
              // rjf: build tab container box
              UI_Parent(tab_column_box) UI_PrefHeight(ui_px(tab_bar_vheight, 1)) DF_Palette(ws, view_is_selected ? DF_PaletteCode_Tab : DF_PaletteCode_TabInactive)
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
                    if(icon_kind != DF_IconKind_Null)
                    {
                      UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                        DF_Font(ws, DF_FontSlot_Icons)
                        UI_TextAlignment(UI_TextAlign_Center)
                        UI_PrefWidth(ui_em(1.75f, 1.f))
                        ui_label(df_g_icon_kind_text_table[icon_kind]);
                    }
                    if(view->query_string_size != 0)
                    {
                      UI_PrefWidth(ui_text_dim(10, 0))
                      {
                        Temp scratch = scratch_begin(0, 0);
                        D_FancyStringList fstrs = {0};
                        {
                          D_FancyString view_title =
                          {
                            df_font_from_slot(DF_FontSlot_Main),
                            label,
                            ui_top_palette()->colors[UI_ColorCode_Text],
                            ui_top_font_size(),
                          };
                          d_fancy_string_list_push(scratch.arena, &fstrs, &view_title);
                        }
                        {
                          D_FancyString space =
                          {
                            df_font_from_slot(DF_FontSlot_Code),
                            str8_lit(" "),
                            v4f32(0, 0, 0, 0),
                            ui_top_font_size(),
                          };
                          d_fancy_string_list_push(scratch.arena, &fstrs, &space);
                        }
                        {
                          D_FancyString query =
                          {
                            view->spec->info.flags & DF_ViewSpecFlag_FilterIsCode ? df_font_from_slot(DF_FontSlot_Code) : df_font_from_slot(DF_FontSlot_Main),
                            str8(view->query_buffer, view->query_string_size),
                            ui_top_palette()->colors[UI_ColorCode_TextWeak],
                            ui_top_font_size(),
                          };
                          d_fancy_string_list_push(scratch.arena, &fstrs, &query);
                        }
                        UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawText, ui_key_zero());
                        ui_box_equip_display_fancy_strings(box, &fstrs);
                        scratch_end(scratch);
                      }
                    }
                    else
                    {
                      UI_PrefWidth(ui_text_dim(10, 0)) ui_label(label);
                    }
                  }
                  UI_PrefWidth(ui_em(2.35f, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
                    DF_Font(ws, DF_FontSlot_Icons)
                    UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons)*0.75f)
                    UI_Flags(UI_BoxFlag_DrawTextWeak)
                    UI_CornerRadius00(0)
                    UI_CornerRadius01(0)
                  {
                    UI_Palette *palette = ui_build_palette(ui_top_palette());
                    palette->background = v4f32(0, 0, 0, 0);
                    ui_set_next_palette(palette);
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
                    ws->tab_ctx_menu_panel = df_handle_from_panel(panel);
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
              {
                ui_spacer(ui_em(0.3f, 1.f));
              }
              
              // rjf: store off drop-site
              drop_sites[view_idx].p = tab_column_box->rect.x0 - tab_spacing/2;
              drop_sites[view_idx].prev_view = view->prev;
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
                DF_Font(ws, DF_FontSlot_Icons)
                UI_FontSize(ui_top_font_size())
                UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
                UI_HoverCursor(OS_Cursor_HandPoint)
                DF_Palette(ws, DF_PaletteCode_ImplicitButton)
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
                                                                panel);
                UI_Signal sig = ui_signal_from_box(add_new_box);
                if(ui_clicked(sig))
                {
                  DF_CmdParams p = df_cmd_params_from_panel(ws, panel);
                  df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_FocusPanel));
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
            DF_View *view = df_view_from_handle(df_g_drag_drop_payload.view);
            if(df_drag_is_active() && window_is_focused && contains_2f32(panel_rect, mouse) && !df_view_is_nil(view))
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
                df_g_last_drag_drop_prev_tab = df_handle_from_view(active_drop_site->prev_view);
              }
              else
              {
                df_g_last_drag_drop_prev_tab = df_handle_zero();
              }
              
              // rjf: vis
              DF_Panel *drag_panel = df_panel_from_handle(df_g_drag_drop_payload.panel);
              if(!df_view_is_nil(view) && active_drop_site != 0) 
              {
                DF_Palette(ws, DF_PaletteCode_DropSiteOverlay) UI_Rect(tab_bar_rect)
                  ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
              }
              
              // rjf: drop
              DF_DragDropPayload payload = df_g_drag_drop_payload;
              if(catchall_drop_site_hovered && (active_drop_site != 0 && df_drag_drop(&payload)))
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
        if(catchall_drop_site_hovered)
        {
          df_g_last_drag_drop_panel = df_handle_from_panel(panel);
          
          DF_DragDropPayload *payload = &df_g_drag_drop_payload;
          DF_View *dragged_view = df_view_from_handle(payload->view);
          B32 view_is_in_panel = 0;
          for(DF_View *view = panel->first_tab_view; !df_view_is_nil(view); view = view->next)
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
              DF_Palette(ws, DF_PaletteCode_DropSiteOverlay) UI_Rect(content_rect)
                ui_build_box_from_key(UI_BoxFlag_DrawBackground, ui_key_zero());
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
                }
              }
            }
          }
        }
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
            if(window_is_focused)
            {
              if(abs_f32(view->loading_t_target - view->loading_t) > 0.01f ||
                 abs_f32(view->scroll_pos.x.off) > 0.01f ||
                 abs_f32(view->scroll_pos.y.off) > 0.01f ||
                 abs_f32(view->is_filtering_t - (F32)!!view->is_filtering))
              {
                df_gfx_request_frame();
              }
              if(view->loading_t_target != 0 && view == df_selected_tab_from_panel(panel))
              {
                df_gfx_request_frame();
              }
            }
            view->loading_t += (view->loading_t_target - view->loading_t) * rate;
            view->is_filtering_t += ((F32)!!view->is_filtering - view->is_filtering_t) * fast_rate;
            view->scroll_pos.x.off -= view->scroll_pos.x.off * (df_setting_val_from_code(ws, DF_SettingCode_ScrollingAnimations).s32 ? fast_rate : 1.f);
            view->scroll_pos.y.off -= view->scroll_pos.y.off * (df_setting_val_from_code(ws, DF_SettingCode_ScrollingAnimations).s32 ? fast_rate : 1.f);
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
            if(view == df_selected_tab_from_panel(panel))
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
    if(df_drag_is_active() && ui_slot_press(UI_EventActionSlot_Cancel))
    {
      df_drag_kill();
      ui_kill_action();
    }
    
    ////////////////////////////
    //- rjf: font size changing
    //
    for(UI_EventNode *n = events.first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      UI_Event *event = &n->v;
      if(event->kind == UI_EventKind_Scroll && event->modifiers & OS_EventFlag_Ctrl)
      {
        ui_eat_event(&events, n);
        if(event->delta_2f32.y < 0)
        {
          DF_CmdParams params = df_cmd_params_from_window(ws);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_IncUIFontScale));
        }
        else if(event->delta_2f32.y > 0)
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
  //- rjf: attach autocomp box to root, or hide if it has not been renewed
  //
  if(!ui_box_is_nil(autocomp_box) && ws->autocomp_last_frame_idx+1 >= df_frame_index()+1)
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
  else if(!ui_box_is_nil(autocomp_box) && ws->autocomp_last_frame_idx+1 < df_frame_index()+1)
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
      Vec4F32 bg_color = df_rgba_from_theme_color(DF_ThemeColor_BaseBackground);
      d_rect(os_client_rect_from_window(ws->os), bg_color, 0, 0, 0);
    }
    
    //- rjf: draw window border
    {
      Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_BaseBorder);
      d_rect(os_client_rect_from_window(ws->os), color, 0, 1.f, 0.5f);
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
        d_push_tex2d_sample_kind(R_Tex2DSampleKind_Linear);
      }
      
      // rjf: draw drop shadow
      if(box->flags & UI_BoxFlag_DrawDropShadow)
      {
        Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
        Vec4F32 drop_shadow_color = df_rgba_from_theme_color(DF_ThemeColor_DropShadow);
        d_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
      }
      
      // rjf: blur background
      if(box->flags & UI_BoxFlag_DrawBackgroundBlur && df_setting_val_from_code(ws, DF_SettingCode_BackgroundBlur).s32)
      {
        R_PassParams_Blur *params = d_blur(box->rect, box->blur_size*(1-box->transparency), 0);
        MemoryCopyArray(params->corner_radii, box->corner_radii);
      }
      
      // rjf: draw background
      if(box->flags & UI_BoxFlag_DrawBackground)
      {
        // rjf: main rectangle
        {
          R_Rect2DInst *inst = d_rect(pad_2f32(box->rect, 1), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
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
            R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
            inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
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
            inst->colors[Corner_00] = shadow_color;
            inst->colors[Corner_01] = shadow_color;
            MemoryCopyArray(inst->corner_radii, box->corner_radii);
          }
          
          // rjf: right -> left dark effect
          {
            R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x1 - shadow_size.x, box->rect.y0, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
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
          d_rect(r2f32p(text_position.x-4, text_position.y-4, text_position.x+4, text_position.y+4),
                 v4f32(1, 0, 1, 1), 1, 0, 1);
        }
        F32 max_x = 100000.f;
        F_Run ellipses_run = {0};
        if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
        {
          max_x = (box->rect.x1-text_position.x);
          ellipses_run = f_push_run_from_string(scratch.arena, box->font, box->font_size, 0, box->tab_size, 0, str8_lit("..."));
        }
        d_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
        if(box->flags & UI_BoxFlag_HasFuzzyMatchRanges)
        {
          Vec4F32 match_color = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
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
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), b->palette->colors[UI_ColorCode_Border], 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
            
            // rjf: hover effect
            if(b->flags & UI_BoxFlag_DrawHotEffects)
            {
              Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Hover);
              color.w *= b->hot_t;
              R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), color, 0, 1.f, 1.f);
              MemoryCopyArray(inst->corner_radii, b->corner_radii);
            }
          }
          
          // rjf: debug border rendering
          if(0)
          {
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), v4f32(1, 0, 1, 0.25f), 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw sides
          {
            Rng2F32 r = b->rect;
            F32 half_thickness = 1.f;
            F32 softness = 0.5f;
            if(b->flags & UI_BoxFlag_DrawSideTop)
            {
              d_rect(r2f32p(r.x0, r.y0-half_thickness, r.x1, r.y0+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideBottom)
            {
              d_rect(r2f32p(r.x0, r.y1-half_thickness, r.x1, r.y1+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideLeft)
            {
              d_rect(r2f32p(r.x0-half_thickness, r.y0, r.x0+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
            if(b->flags & UI_BoxFlag_DrawSideRight)
            {
              d_rect(r2f32p(r.x1-half_thickness, r.y0, r.x1+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
            }
          }
          
          // rjf: draw focus overlay
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusOverlay) && b->focus_hot_t > 0.01f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Focus);
            color.w *= 0.2f*b->focus_hot_t;
            R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: draw focus border
          if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Focus);
            color.w *= b->focus_active_t;
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: disabled overlay
          if(b->disabled_t >= 0.005f)
          {
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_DisabledOverlay);
            color.w *= b->disabled_t;
            R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 1);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);
          }
          
          // rjf: pop squish
          if(b->squish != 0)
          {
            d_pop_xform2d();
            d_pop_tex2d_sample_kind();
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
      Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_NegativePopButtonBackground);
      color.w *= ws->error_t;
      Rng2F32 rect = os_client_rect_from_window(ws->os);
      d_rect(pad_2f32(rect, 24.f), color, 0, 16.f, 12.f);
      d_rect(rect, v4f32(color.x, color.y, color.z, color.w*0.05f), 0, 0, 0);
    }
    
    //- rjf: scratch debug mouse drawing
    if(DEV_scratch_mouse_draw)
    {
#if 1
      Vec2F32 p = add_2f32(os_mouse_from_window(ws->os), v2f32(30, 0));
      d_rect(os_client_rect_from_window(ws->os), v4f32(0, 0, 0, 0.9f), 0, 0, 0);
      F_Run trailer_run = f_push_run_from_string(scratch.arena, df_font_from_slot(DF_FontSlot_Main), 16.f, 0, 0, 0, str8_lit("..."));
      D_FancyStringList strs = {0};
      D_FancyString str = {df_font_from_slot(DF_FontSlot_Main), str8_lit("Shift + F5"), v4f32(1, 1, 1, 1), 72.f, 0.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str);
      D_FancyRunList runs = d_fancy_run_list_from_fancy_string_list(scratch.arena, 0, F_RasterFlag_Smooth, &strs);
      d_truncated_fancy_run_list(p, &runs, 1000000.f, trailer_run);
      d_rect(r2f32(p, add_2f32(p, runs.dim)), v4f32(1, 0, 0, 0.5f), 0, 1, 0);
      d_rect(r2f32(sub_2f32(p, v2f32(4, 4)), add_2f32(p, v2f32(4, 4))), v4f32(1, 0, 1, 1), 0, 0, 0);
#else
      Vec2F32 p = add_2f32(os_mouse_from_window(ws->os), v2f32(30, 0));
      d_rect(os_client_rect_from_window(ws->os), v4f32(0, 0, 0, 0.4f), 0, 0, 0);
      D_FancyStringList strs = {0};
      D_FancyString str1 = {df_font_from_slot(DF_FontSlot_Main), str8_lit("T"), v4f32(1, 1, 1, 1), 16.f, 4.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str1);
      D_FancyString str2 = {df_font_from_slot(DF_FontSlot_Main), str8_lit("his is a test of some "), v4f32(1, 0.5f, 0.5f, 1), 14.f, 0.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str2);
      D_FancyString str3 = {df_font_from_slot(DF_FontSlot_Code), str8_lit("very fancy text!"), v4f32(1, 0.8f, 0.4f, 1), 18.f, 4.f, 4.f};
      d_fancy_string_list_push(scratch.arena, &strs, &str3);
      D_FancyRunList runs = d_fancy_run_list_from_fancy_string_list(scratch.arena, 0, 0, &strs);
      F_Run trailer_run = f_push_run_from_string(scratch.arena, df_font_from_slot(DF_FontSlot_Main), 16.f, 0, 0, 0, str8_lit("..."));
      F32 limit = 500.f + sin_f32(df_time_in_seconds()/10.f)*200.f;
      d_truncated_fancy_run_list(p, &runs, limit, trailer_run);
      d_rect(r2f32p(p.x+limit, 0, p.x+limit+2.f, 1000), v4f32(1, 0, 0, 1), 0, 0, 0);
      df_gfx_request_frame();
#endif
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
        space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
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
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, string).x;
          str8_list_push(arena, &list, string);
        }
        
        // rjf: arrow
        if(did_ptr_value && (direct_type_has_content || symbol_name.size != 0) && (flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules))
        {
          String8 arrow = str8_lit(" -> ");
          str8_list_push(arena, &list, arrow);
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, arrow).x;
        }
        
        // rjf: special-case: strings
        if(!has_array && direct_type_is_string && (flags & DF_EvalVizStringFlag_ReadOnlyDisplayRules) && eval.mode == EVAL_EvalMode_Addr)
        {
          U64 string_memory_addr = value_eval.imm_u64;
          U64 element_size = tg_byte_size_from_graph_rdi_key(graph, rdi, direct_type_key);
          CTRL_ProcessMemorySlice text_slice = ctrl_query_cached_zero_terminated_data_from_process_vaddr_limit(arena, process->ctrl_machine_id, process->ctrl_handle, string_memory_addr, 256, element_size, 0);
          String8 raw_text = {0};
          switch(element_size)
          {
            default:{raw_text = text_slice.data;}break;
            case 2: {raw_text = str8_from_16(arena, str16((U16 *)text_slice.data.str, text_slice.data.size/sizeof(U16)));}break;
            case 4: {raw_text = str8_from_32(arena, str32((U32 *)text_slice.data.str, text_slice.data.size/sizeof(U32)));}break;
          }
          String8 text = df_eval_escaped_from_raw_string(arena, raw_text);
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, text).x;
          space_taken += 2*f_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
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
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, symbol_name).x;
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
              space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, unknown).x;
            }
            else
            {
              space_taken += f_dim_from_tag_size_string_list(font, font_size, 0, 0, pted_strs).x;
              str8_list_concat_in_place(&list, &pted_strs);
            }
          }
          else
          {
            String8 ellipses = str8_lit("...");
            str8_list_push(arena, &list, ellipses);
            space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
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
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, text).x;
          space_taken += 2*f_dim_from_tag_size_string(font, font_size, 0, 0, str8_lit("\"")).x;
          str8_list_push(arena, &list, str8_lit("\""));
          str8_list_push(arena, &list, text);
          str8_list_push(arena, &list, str8_lit("\""));
        }
        
        // rjf: open brace
        if(!special_case)
        {
          String8 brace = str8_lit("[");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, brace).x;
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
              space_taken += f_dim_from_tag_size_string_list(font, font_size, 0, 0, element_strs).x;
              str8_list_concat_in_place(&list, &element_strs);
              if(idx+1 < array_count)
              {
                String8 comma = str8_lit(", ");
                space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
                str8_list_push(arena, &list, comma);
              }
            }
          }
          else
          {
            String8 ellipses = str8_lit("...");
            str8_list_push(arena, &list, ellipses);
            space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
          }
        }
        
        // rjf: close brace
        if(!special_case)
        {
          String8 brace = str8_lit("]");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, brace).x;
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
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, brace).x;
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
            space_taken += f_dim_from_tag_size_string_list(font, font_size, 0, 0, member_strs).x;
            str8_list_concat_in_place(&list, &member_strs);
            if(member_idx+1 < filtered_data_members.count)
            {
              String8 comma = str8_lit(", ");
              space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, comma).x;
              str8_list_push(arena, &list, comma);
            }
          }
          scratch_end(scratch);
        }
        else
        {
          String8 ellipses = str8_lit("...");
          str8_list_push(arena, &list, ellipses);
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, ellipses).x;
        }
        
        // rjf: close brace
        {
          String8 brace = str8_lit("}");
          str8_list_push(arena, &list, brace);
          space_taken += f_dim_from_tag_size_string(font, font_size, 0, 0, brace).x;
        }
        
      }break;
    }
  }
  ProfEnd();
  return list;
}

internal DF_EvalVizWindowedRowList
df_eval_viz_windowed_row_list_from_viz_block_list(Arena *arena, DI_Scope *scope, DF_CtrlCtx *ctrl_ctx, EVAL_ParseCtx *parse_ctx, EVAL_String2ExprMap *macro_map, DF_EvalView *eval_view, U32 default_radix, F_Tag font, F32 font_size, Rng1S64 visible_range, DF_EvalVizBlockList *blocks)
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
        row->display_expr        = block->string;
        row->edit_expr           = block->string;
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
          row->display_expr        = push_str8_copy(arena, member->name);
          row->edit_expr           = push_str8f(arena, "%S.%S", block->string, member->name);
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
          row->display_expr        = push_str8_copy(arena, enum_val->name);
          row->edit_expr           = row->display_expr;
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
          row->display_expr        = push_str8f(arena, "[%I64u]", idx);
          row->edit_expr           = push_str8f(arena, "%S[%I64u]", block->string, idx);
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
        String8 node_type_string = tg_string_from_key(arena, parse_ctx->type_graph, parse_ctx->rdi, block->eval.type_key);
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
          row->display_expr        = push_str8f(arena, "[%I64u]", idx);
          row->edit_expr           = push_str8f(arena, "(%S *)0xI64x", node_type_string, link_eval.offset);
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
        row->edit_expr           = block->string;
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
        String8 name = fzy_item_string_from_rdi_target_element_idx(parse_ctx->rdi, block->fzy_target, block->fzy_backing_items.v[idx].idx);
        
        // rjf: get keys for this row
        DF_ExpandKey parent_key = block->parent_key;
        DF_ExpandKey key = block->key;
        key.child_num = block->fzy_backing_items.v[idx].idx;
        
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
        row->display_expr = name;
        row->edit_expr = name;
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
      ws->hover_eval_focused = 0;
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
    DF_CoreViewRuleSpec *spec = df_core_view_rule_spec_from_string(first_step ? first_step->string : str8_zero());
    
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
df_set_autocomp_lister_query(DF_Window *ws, UI_Key root_key, DF_CtrlCtx ctrl_ctx, DF_AutoCompListerParams *params, String8 input, U64 cursor_off)
{
  String8 query = df_autocomp_query_word_from_input_string_off(input, cursor_off);
  String8 current_query = str8(ws->autocomp_lister_query_buffer, ws->autocomp_lister_query_size);
  if(cursor_off != ws->autocomp_cursor_off)
  {
    ws->autocomp_query_dirty = 1;
    ws->autocomp_cursor_off = cursor_off;
  }
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
  arena_clear(ws->autocomp_lister_params_arena);
  MemoryCopyStruct(&ws->autocomp_lister_params, params);
  ws->autocomp_lister_params.strings = str8_list_copy(ws->autocomp_lister_params_arena, &ws->autocomp_lister_params.strings);
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
    case TXT_TokenKind_Symbol: {color = DF_ThemeColor_CodeDelimiterOperator;}break;
  }
  return color;
}

//- rjf: code -> palette

internal UI_Palette *
df_palette_from_code(DF_Window *ws, DF_PaletteCode code)
{
  UI_Palette *result = &ws->cfg_palettes[code];
  return result;
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

internal F_RasterFlags
df_raster_flags_from_slot(DF_Window *ws, DF_FontSlot slot)
{
  F_RasterFlags flags = F_RasterFlag_Smooth|F_RasterFlag_Hinted;
  switch(slot)
  {
    default:{}break;
    case DF_FontSlot_Icons:{flags = F_RasterFlag_Smooth;}break;
    case DF_FontSlot_Main: {flags = (!!df_setting_val_from_code(ws, DF_SettingCode_SmoothUIText).s32*F_RasterFlag_Smooth)|(!!df_setting_val_from_code(ws, DF_SettingCode_HintUIText).s32*F_RasterFlag_Hinted);}break;
    case DF_FontSlot_Code: {flags = (!!df_setting_val_from_code(ws, DF_SettingCode_SmoothCodeText).s32*F_RasterFlag_Smooth)|(!!df_setting_val_from_code(ws, DF_SettingCode_HintCodeText).s32*F_RasterFlag_Hinted);}break;
  }
  return flags;
}

//- rjf: settings

internal DF_SettingVal
df_setting_val_from_code(DF_Window *optional_window, DF_SettingCode code)
{
  DF_SettingVal result = {0};
  if(optional_window != 0)
  {
    result = optional_window->setting_vals[code];
  }
  if(result.set == 0)
  {
    for(EachEnumVal(DF_CfgSrc, src))
    {
      if(df_gfx_state->cfg_setting_vals[src][code].set)
      {
        result = df_gfx_state->cfg_setting_vals[src][code];
        break;
      }
    }
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
              if(view == df_selected_tab_from_panel(p))
              {
                str8_list_push(arena, &strs, str8_lit("selected "));
              }
              {
                DF_Entity *project = df_entity_from_handle(view->project);
                if(!df_entity_is_nil(project))
                {
                  Temp scratch = scratch_begin(&arena, 1);
                  String8 project_path_absolute = df_full_path_from_entity(scratch.arena, project);
                  String8 project_path_relative = path_relative_dst_from_absolute_dst_src(scratch.arena, project_path_absolute, root_path);
                  str8_list_pushf(arena, &strs, "project:{\"%S\"} ", project_path_relative);
                  scratch_end(scratch);
                }
              }
              if(view->query_string_size != 0 && view->spec->info.flags & DF_ViewSpecFlag_CanSerializeQuery)
              {
                Temp scratch = scratch_begin(&arena, 1);
                String8 query_raw = str8(view->query_buffer, view->query_string_size);
                String8 query_sanitized = df_cfg_escaped_from_raw_string(scratch.arena, query_raw);
                str8_list_pushf(arena, &strs, "query:{\"%S\"} ", query_sanitized);
                scratch_end(scratch);
              }
              if(view->spec->info.flags & DF_ViewSpecFlag_CanSerializeEntityPath)
              {
                if(view_entity->kind == DF_EntityKind_File)
                {
                  String8 project_path = root_path;
                  String8 entity_path = df_full_path_from_entity(arena, view_entity);
                  String8 entity_path_rel = path_relative_dst_from_absolute_dst_src(arena, entity_path, project_path);
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
  
  //- rjf: serialize global settings
  {
    B32 first = 1;
    for(EachEnumVal(DF_SettingCode, code))
    {
      if(df_g_setting_code_default_is_per_window_table[code])
      {
        continue;
      }
      DF_SettingVal current = df_gfx_state->cfg_setting_vals[source][code];
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
  String8 process_thread_string = thread_display_string;
  DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
  if(process->kind == DF_EntityKind_Process)
  {
    String8 process_display_string = df_display_string_from_entity(scratch.arena, process);
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
          if(!df_entity_is_nil(thread))
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
          if(!df_entity_is_nil(thread))
          {
            icon = DF_IconKind_CircleFilled;
            explanation = push_str8f(arena, "%S hit a breakpoint", process_thread_string);
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
//~ rjf: UI Widgets: Fancy Buttons

internal void
df_cmd_binding_buttons(DF_Window *ws, DF_CmdSpec *spec)
{
  Temp scratch = scratch_begin(0, 0);
  DF_BindingList bindings = df_bindings_from_spec(scratch.arena, spec);
  
  //- rjf: build buttons for each binding
  for(DF_BindingNode *n = bindings.first; n != 0; n = n->next)
  {
    DF_Binding binding = n->binding;
    B32 rebinding_active_for_this_binding = (df_gfx_state->bind_change_active &&
                                             df_gfx_state->bind_change_cmd_spec == spec &&
                                             df_gfx_state->bind_change_binding.key == binding.key &&
                                             df_gfx_state->bind_change_binding.flags == binding.flags);
    
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
    
    //- rjf: form color palette
    UI_Palette *palette = ui_top_palette();
    if(has_conflicts || rebinding_active_for_this_binding)
    {
      palette = push_array(ui_build_arena(), UI_Palette, 1);
      MemoryCopyStruct(palette, ui_top_palette());
      if(has_conflicts)
      {
        palette->colors[UI_ColorCode_Text] = df_rgba_from_theme_color(DF_ThemeColor_TextNegative);
        palette->colors[UI_ColorCode_TextWeak] = df_rgba_from_theme_color(DF_ThemeColor_TextNegative);
      }
      if(rebinding_active_for_this_binding)
      {
        palette->colors[UI_ColorCode_Border] = df_rgba_from_theme_color(DF_ThemeColor_Focus);
        palette->colors[UI_ColorCode_Background] = df_rgba_from_theme_color(DF_ThemeColor_Focus);
        palette->colors[UI_ColorCode_Background].w *= 0.25f;
      }
    }
    
    //- rjf: build box
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_palette(palette);
    ui_set_next_group_key(ui_key_zero());
    ui_set_next_pref_width(ui_text_dim(ui_top_font_size()*1.f, 1));
    UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                            UI_BoxFlag_Clickable|
                                            UI_BoxFlag_DrawActiveEffects|
                                            UI_BoxFlag_DrawHotEffects|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DrawBackground,
                                            "%S###bind_btn_%p_%x_%x", keybinding_str, spec, binding.key, binding.flags);
    
    //- rjf: interaction
    UI_Signal sig = ui_signal_from_box(box);
    {
      // rjf: click => toggle activity
      if(!df_gfx_state->bind_change_active && ui_clicked(sig))
      {
        if((binding.key == OS_Key_Esc || binding.key == OS_Key_Delete) && binding.flags == 0)
        {
          DF_CmdParams p = df_cmd_params_zero();
          p.string = str8_lit("Cannot rebind; this command uses a reserved keybinding.");
          df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
        }
        else
        {
          df_gfx_state->bind_change_active = 1;
          df_gfx_state->bind_change_cmd_spec = spec;
          df_gfx_state->bind_change_binding = binding;
        }
      }
      else if(df_gfx_state->bind_change_active && ui_clicked(sig))
      {
        df_gfx_state->bind_change_active = 0;
      }
      
      // rjf: hover w/ conflicts => show conflicts
      if(ui_hovering(sig) && has_conflicts) UI_Tooltip
      {
        UI_PrefWidth(ui_children_sum(1)) df_error_label(str8_lit("This binding conflicts with those for:"));
        for(DF_CmdSpecNode *n = specs_with_binding.first; n != 0; n = n->next)
        {
          if(n->spec != spec)
          {
            ui_labelf("%S", n->spec->info.display_name);
          }
        }
      }
    }
    
    //- rjf: delete button
    if(rebinding_active_for_this_binding)
      UI_PrefWidth(ui_em(2.5f, 1.f))
      UI_Palette(ui_build_palette(ui_top_palette(),
                                  .background = df_rgba_from_theme_color(DF_ThemeColor_NegativePopButtonBackground),
                                  .border = df_rgba_from_theme_color(DF_ThemeColor_NegativePopButtonBorder),
                                  .text = df_rgba_from_theme_color(DF_ThemeColor_Text)))
    {
      ui_set_next_group_key(ui_key_zero());
      UI_Signal sig = df_icon_button(ws, DF_IconKind_X, 0, str8_lit("###delete_binding"));
      if(ui_clicked(sig))
      {
        df_unbind_spec(spec, binding);
        df_gfx_state->bind_change_active = 0;
      }
    }
    
    //- rjf: space
    ui_spacer(ui_em(1.f, 1.f));
  }
  
  //- rjf: build "add new binding" button
  DF_Font(ws, DF_FontSlot_Icons)
  {
    UI_Palette *palette = ui_top_palette();
    B32 adding_new_binding = (df_gfx_state->bind_change_active &&
                              df_gfx_state->bind_change_cmd_spec == spec &&
                              df_gfx_state->bind_change_binding.key == OS_Key_Null &&
                              df_gfx_state->bind_change_binding.flags == 0);
    if(adding_new_binding)
    {
      palette = ui_build_palette(ui_top_palette());
      palette->colors[UI_ColorCode_Border] = df_rgba_from_theme_color(DF_ThemeColor_Focus);
      palette->colors[UI_ColorCode_Background] = df_rgba_from_theme_color(DF_ThemeColor_Focus);
      palette->colors[UI_ColorCode_Background].w *= 0.25f;
    }
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_group_key(ui_key_zero());
    ui_set_next_pref_width(ui_text_dim(ui_top_font_size()*1.f, 1));
    ui_set_next_palette(palette);
    UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                            UI_BoxFlag_Clickable|
                                            UI_BoxFlag_DrawActiveEffects|
                                            UI_BoxFlag_DrawHotEffects|
                                            UI_BoxFlag_DrawBorder|
                                            UI_BoxFlag_DrawBackground,
                                            "%S###add_binding", df_g_icon_kind_text_table[DF_IconKind_Add]);
    UI_Signal sig = ui_signal_from_box(box);
    if(ui_clicked(sig))
    {
      if(!df_gfx_state->bind_change_active && ui_clicked(sig))
      {
        df_gfx_state->bind_change_active = 1;
        df_gfx_state->bind_change_cmd_spec = spec;
        MemoryZeroStruct(&df_gfx_state->bind_change_binding);
      }
      else if(df_gfx_state->bind_change_active && ui_clicked(sig))
      {
        df_gfx_state->bind_change_active = 0;
      }
    }
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
df_cmd_spec_button(DF_Window *ws, DF_CmdSpec *spec)
{
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects|
                                          UI_BoxFlag_Clickable,
                                          "###cmd_%p", spec);
  UI_Parent(box) UI_HeightFill UI_Padding(ui_em(1.f, 1.f))
  {
    DF_IconKind canonical_icon = spec->info.canonical_icon_kind;
    if(canonical_icon != DF_IconKind_Null)
    {
      DF_Font(ws, DF_FontSlot_Icons)
        UI_PrefWidth(ui_em(2.f, 1.f))
        UI_TextAlignment(UI_TextAlign_Center)
        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
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
      ui_set_next_flags(UI_BoxFlag_Clickable);
      ui_set_next_group_key(ui_key_zero());
      UI_PrefWidth(ui_children_sum(1))
        UI_NamedRow(str8_lit("###bindings"))
        UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
        UI_FastpathCodepoint(0)
      {
        df_cmd_binding_buttons(ws, spec);
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
    UI_Signal sig = df_cmd_spec_button(ws, spec);
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
df_icon_button(DF_Window *ws, DF_IconKind kind, FuzzyMatchRangeList *matches, String8 string)
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
    else
    {
      ui_spacer(ui_em(1.f, 1.f));
    }
    UI_TextAlignment(UI_TextAlign_Center)
      DF_Font(ws, DF_FontSlot_Icons)
      UI_PrefWidth(ui_em(2.f, 1.f))
      UI_PrefHeight(ui_pct(1, 0))
      UI_FlagsAdd(UI_BoxFlag_DisableTextTrunc|UI_BoxFlag_DrawTextWeak)
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
    else
    {
      ui_spacer(ui_em(1.f, 1.f));
    }
  }
  UI_Signal result = ui_signal_from_box(box);
  return result;
}

internal UI_Signal
df_icon_buttonf(DF_Window *ws, DF_IconKind kind, FuzzyMatchRangeList *matches, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = df_icon_button(ws, kind, matches, string);
  scratch_end(scratch);
  return sig;
}

internal void
df_entity_tooltips(DF_Window *ws, DF_Entity *entity)
{
  Temp scratch = scratch_begin(0, 0);
  DF_Palette(ws, DF_PaletteCode_Floating) switch(entity->kind)
  {
    default:{}break;
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
          ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_entity(entity)));
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
            UI_Palette *palette = ui_top_palette();
            if(stop_event.cause == CTRL_EventCause_Error ||
               stop_event.cause == CTRL_EventCause_InterruptedByException ||
               stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
               stop_event.cause == CTRL_EventCause_UserBreakpoint)
            {
              palette = ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextNegative));
            }
            UI_PrefWidth(ui_children_sum(1)) UI_Row UI_Palette(palette)
            {
              UI_PrefWidth(ui_em(1.5f, 1.f))
                DF_Font(ws, DF_FontSlot_Icons)
                ui_label(df_g_icon_kind_text_table[icon_kind]);
              UI_PrefWidth(ui_text_dim(10, 1)) ui_label(explanation);
            }
          }
        }
      }
      ui_spacer(ui_em(1.5f, 1.f));
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("TID: ");
        UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("%i", pid_or_tid);
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("Architecture: ");
        UI_PrefWidth(ui_text_dim(10, 1)) ui_label(arch_str);
      }
      ui_spacer(ui_em(1.5f, 1.f));
      DI_Scope *di_scope = di_scope_open();
      DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
      CTRL_Unwind base_unwind = df_query_cached_unwind_from_thread(entity);
      DF_Unwind rich_unwind = df_unwind_from_ctrl_unwind(scratch.arena, di_scope, process, &base_unwind);
      for(U64 idx = 0; idx < rich_unwind.frames.concrete_frame_count; idx += 1)
      {
        DF_UnwindFrame *f = &rich_unwind.frames.v[idx];
        RDI_Parsed *rdi = f->rdi;
        RDI_Procedure *procedure = f->procedure;
        U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
        DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
        String8 module_name = df_entity_is_nil(module) ? str8_lit("???") : str8_skip_last_slash(module->name);
        
        // rjf: inline frames
        for(DF_UnwindInlineFrame *fin = f->last_inline_frame; fin != 0; fin = fin->prev)
          UI_PrefWidth(ui_children_sum(1)) UI_Row
        {
          String8 name = {0};
          name.str = rdi_string_from_idx(rdi, fin->inline_site->name_string_idx, &name.size);
          DF_Font(ws, DF_FontSlot_Code) UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("0x%I64x", rip_vaddr);
          DF_Font(ws, DF_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_label(str8_lit("[inlined]"));
          if(name.size != 0)
          {
            DF_Font(ws, DF_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
            {
              df_code_label(1.f, 0, df_rgba_from_theme_color(DF_ThemeColor_CodeSymbol), name);
            }
          }
          else
          {
            DF_Font(ws, DF_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
          }
        }
        
        // rjf: concrete frame
        UI_PrefWidth(ui_children_sum(1)) UI_Row
        {
          String8 name = {0};
          name.str = rdi_name_from_procedure(rdi, procedure, &name.size);
          DF_Font(ws, DF_FontSlot_Code) UI_PrefWidth(ui_em(18.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("0x%I64x", rip_vaddr);
          if(name.size != 0)
          {
            DF_Font(ws, DF_FontSlot_Code) UI_PrefWidth(ui_text_dim(10, 1))
            {
              df_code_label(1.f, 0, df_rgba_from_theme_color(DF_ThemeColor_CodeSymbol), name);
            }
          }
          else
          {
            DF_Font(ws, DF_FontSlot_Code) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10, 1)) ui_labelf("[??? in %S]", module_name);
          }
        }
      }
      di_scope_close(di_scope);
    }break;
    case DF_EntityKind_Breakpoint: UI_Flags(0)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      if(entity->flags & DF_EntityFlag_HasColor)
      {
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_entity(entity)));
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
        UI_PrefWidth(ui_em(12.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("Stop Condition: ");
        UI_PrefWidth(ui_text_dim(10, 1)) DF_Font(ws, DF_FontSlot_Code) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), stop_condition);
      }
      UI_PrefWidth(ui_children_sum(1)) UI_Row
      {
        U64 hit_count = entity->u64;
        String8 hit_count_text = str8_from_u64(scratch.arena, hit_count, 10, 0, 0);
        UI_PrefWidth(ui_em(12.f, 1.f)) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) ui_labelf("Hit Count: ");
        UI_PrefWidth(ui_text_dim(10, 1)) DF_Font(ws, DF_FontSlot_Code) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), hit_count_text);
      }
    }break;
    case DF_EntityKind_WatchPin:
    DF_Font(ws, DF_FontSlot_Code)
      UI_Tooltip UI_PrefWidth(ui_text_dim(10, 1))
    {
      if(entity->flags & DF_EntityFlag_HasColor)
      {
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_entity(entity)));
      }
      String8 display_string = df_display_string_from_entity(scratch.arena, entity);
      UI_PrefWidth(ui_text_dim(10, 1)) df_code_label(1.f, 1, df_rgba_from_theme_color(DF_ThemeColor_CodeDefault), display_string);
    }break;
  }
  scratch_end(scratch);
}

internal UI_Signal
df_entity_desc_button(DF_Window *ws, DF_Entity *entity, FuzzyMatchRangeList *name_matches, String8 fuzzy_query, B32 is_implicit)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  UI_Palette *palette = ui_top_palette();
  if(entity->kind == DF_EntityKind_Thread)
  {
    DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(ws);
    CTRL_Event stop_event = df_ctrl_last_stop_event();
    DF_Entity *stopped_thread = df_entity_from_ctrl_handle(stop_event.machine_id, stop_event.entity);
    DF_Entity *selected_thread = df_entity_from_handle(ctrl_ctx.thread);
    if(selected_thread == entity)
    {
      palette = df_palette_from_code(ws, DF_PaletteCode_NeutralPopButton);
    }
    if(stopped_thread == entity &&
       (stop_event.cause == CTRL_EventCause_UserBreakpoint ||
        stop_event.cause == CTRL_EventCause_InterruptedByException ||
        stop_event.cause == CTRL_EventCause_InterruptedByTrap ||
        stop_event.cause == CTRL_EventCause_InterruptedByHalt))
    {
      palette = df_palette_from_code(ws, DF_PaletteCode_NegativePopButton);
    }
  }
  if(entity->cfg_src == DF_CfgSrc_CommandLine)
  {
    palette = df_palette_from_code(ws, DF_PaletteCode_NeutralPopButton);
  }
  else if(entity->kind == DF_EntityKind_Target && entity->b32 != 0)
  {
    palette = df_palette_from_code(ws, DF_PaletteCode_NeutralPopButton);
  }
  ui_set_next_palette(palette);
  ui_set_next_hover_cursor(OS_Cursor_HandPoint);
  UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                          (!is_implicit*UI_BoxFlag_DrawBorder)|
                                          UI_BoxFlag_DrawBackground|
                                          UI_BoxFlag_DrawHotEffects|
                                          UI_BoxFlag_DrawActiveEffects,
                                          "entity_ref_button_%p", entity);
  
  //- rjf: build contents
  UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 0)) UI_Padding(ui_em(1.f, 1.f))
  {
    DF_EntityKindFlags kind_flags = df_g_entity_kind_flags_table[entity->kind];
    DF_EntityOpFlags op_flags = df_g_entity_kind_op_flags_table[entity->kind];
    DF_IconKind icon = df_g_entity_kind_icon_kind_table[entity->kind];
    Vec4F32 entity_color = palette->colors[UI_ColorCode_Text];
    Vec4F32 entity_color_weak = palette->colors[UI_ColorCode_TextWeak];
    if(entity->flags & DF_EntityFlag_HasColor)
    {
      entity_color = df_rgba_from_entity(entity);
      entity_color_weak = entity_color;
      entity_color_weak.w *= 0.5f;
    }
    UI_TextAlignment(UI_TextAlign_Center)
      DF_Font(ws, DF_FontSlot_Icons)
      UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
      UI_PrefWidth(ui_em(1.875f, 1.f))
      UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      ui_label(df_g_icon_kind_text_table[icon]);
    if(entity->cfg_src == DF_CfgSrc_CommandLine)
    {
      UI_TextAlignment(UI_TextAlign_Center)
        UI_PrefWidth(ui_em(1.875f, 1.f))
      {
        UI_Box *info_box = &ui_g_nil_box;
        DF_Font(ws, DF_FontSlot_Icons)
          UI_FontSize(df_font_size_from_slot(ws, DF_FontSlot_Icons))
        {
          info_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DrawTextWeak|UI_BoxFlag_Clickable, "%S###%p_temp_info", df_g_icon_kind_text_table[DF_IconKind_Info], entity);
        }
        UI_Signal info_sig = ui_signal_from_box(info_box);
        if(ui_hovering(info_sig)) UI_Tooltip
        {
          ui_labelf("Specified via command line; not saved in project.");
        }
      }
    }
    String8 label = df_display_string_from_entity(scratch.arena, entity);
    UI_Palette(ui_build_palette(ui_top_palette(), .text = entity_color))
      DF_Font(ws, kind_flags&DF_EntityKindFlag_NameIsCode ? DF_FontSlot_Code : DF_FontSlot_Main)
      UI_Flags((entity->kind == DF_EntityKind_Thread ||
                entity->kind == DF_EntityKind_Breakpoint ||
                entity->kind == DF_EntityKind_WatchPin)
               ? UI_BoxFlag_DisableTruncatedHover
               : 0)
    {
      UI_Signal label_sig = ui_label(label);
      if(name_matches != 0)
      {
        ui_box_equip_fuzzy_match_ranges(label_sig.box, name_matches);
      }
    }
    if(entity->kind == DF_EntityKind_Target) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_FontSize(ui_top_font_size()*0.95f)
    {
      DF_Entity *args = df_entity_child_from_kind(entity, DF_EntityKind_Arguments);
      ui_label(args->name);
    }
    if(op_flags & DF_EntityOpFlag_Enable && entity->b32 == 0) UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_FontSize(ui_top_font_size()*0.95f) UI_HeightFill
    {
      ui_label(str8_lit("(Disabled)"));
    }
    if(entity->kind == DF_EntityKind_Thread)
      UI_FontSize(ui_top_font_size()*0.75f)
      DF_Font(ws, DF_FontSlot_Code)
      UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_CodeSymbol)))
      UI_Flags(UI_BoxFlag_DisableTruncatedHover)
    {
      CTRL_Unwind unwind = df_query_cached_unwind_from_thread(entity);
      DF_Entity *process = df_entity_ancestor_from_kind(entity, DF_EntityKind_Process);
      U64 idx = 0;
      U64 limit = 3;
      ui_spacer(ui_em(1.f, 1.f));
      for(U64 num = unwind.frames.count; num > 0; num -= 1)
      {
        CTRL_UnwindFrame *f = &unwind.frames.v[num-1];
        U64 rip_vaddr = regs_rip_from_arch_block(entity->arch, f->regs);
        DF_Entity *module = df_module_from_process_vaddr(process, rip_vaddr);
        U64 rip_voff = df_voff_from_vaddr(module, rip_vaddr);
        DI_Key dbgi_key = df_dbgi_key_from_module(module);
        String8 procedure_name = df_symbol_name_from_dbgi_key_voff(scratch.arena, &dbgi_key, rip_voff);
        if(procedure_name.size != 0)
        {
          FuzzyMatchRangeList fuzzy_matches = {0};
          if(fuzzy_query.size != 0)
          {
            fuzzy_matches = fuzzy_match_find(scratch.arena, fuzzy_query, procedure_name);
          }
          if(idx != 0)
          {
            UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_em(2.f, 1.f)) ui_label(str8_lit(">"));
          }
          UI_PrefWidth(ui_text_dim(10.f, 0.f))
          {
            UI_Box *label_box = ui_label(procedure_name).box;
            ui_box_equip_fuzzy_match_ranges(label_box, &fuzzy_matches);
          }
          idx += 1;
          if(idx == limit)
          {
            UI_FlagsAdd(UI_BoxFlag_DrawTextWeak) UI_PrefWidth(ui_text_dim(10.f, 1.f)) ui_label(str8_lit("> ..."));
          }
        }
      }
    }
  }
  
  //- rjf: do interaction on main box
  UI_Signal sig = ui_signal_from_box(box);
  {
    if(ui_hovering(sig) && !df_drag_is_active())
    {
      df_entity_tooltips(ws, entity);
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
      ui_ctx_menu_open(ws->entity_ctx_menu_key, sig.box->key, v2f32(0, sig.box->rect.y1 - sig.box->rect.y0));
      ws->entity_ctx_menu_entity = handle;
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
  return sig;
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
    UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      UI_TextAlignment(UI_TextAlign_Center)
      DF_Font(ws, DF_FontSlot_Icons)
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
  B32 do_lines;
  B32 do_glow;
};

internal UI_BOX_CUSTOM_DRAW(df_thread_box_draw_extensions)
{
  DF_ThreadBoxDrawExtData *u = (DF_ThreadBoxDrawExtData *)box->custom_draw_user_data;
  
  // rjf: draw line before next-to-execute line
  if(u->do_lines)
  {
    R_Rect2DInst *inst = d_rect(r2f32p(box->parent->parent->parent->rect.x0,
                                       box->parent->rect.y0 - box->font_size*0.125f,
                                       box->parent->parent->parent->rect.x0 + box->font_size*260*u->alive_t,
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
  if(u->is_selected && u->do_glow)
  {
    Vec4F32 weak_thread_color = u->thread_color;
    weak_thread_color.w *= 0.3f;
    R_Rect2DInst *inst = d_rect(r2f32p(box->parent->parent->parent->rect.x0,
                                       box->parent->rect.y0,
                                       box->parent->parent->parent->rect.x0 + ui_top_font_size()*22.f*u->alive_t,
                                       box->parent->rect.y1),
                                v4f32(0, 0, 0, 0),
                                0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: locked icon on frozen threads
  if(u->is_frozen)
  {
    F32 lock_icon_off = ui_top_font_size()*0.2f;
    Vec4F32 lock_icon_color = df_rgba_from_theme_color(DF_ThemeColor_TextNegative);
    d_text(df_font_from_slot(DF_FontSlot_Icons),
           box->font_size, 0, 0, F_RasterFlag_Smooth,
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
  B32 do_lines;
  B32 do_glow;
};

internal UI_BOX_CUSTOM_DRAW(df_bp_box_draw_extensions)
{
  DF_BreakpointBoxDrawExtData *u = (DF_BreakpointBoxDrawExtData *)box->custom_draw_user_data;
  
  // rjf: draw line before next-to-execute line
  if(u->do_lines)
  {
    R_Rect2DInst *inst = d_rect(r2f32p(box->parent->parent->parent->rect.x0,
                                       box->parent->rect.y0 - box->font_size*0.125f,
                                       box->parent->parent->parent->rect.x0 + ui_top_font_size()*250.f*u->alive_t,
                                       box->parent->rect.y0 + box->font_size*0.125f),
                                v4f32(u->color.x, u->color.y, u->color.z, 0),
                                0, 0, 1.f);
    inst->colors[Corner_00] = inst->colors[Corner_01] = u->color;
  }
  
  // rjf: draw slight fill
  if(u->do_glow)
  {
    Vec4F32 weak_thread_color = u->color;
    weak_thread_color.w *= 0.3f;
    R_Rect2DInst *inst = d_rect(r2f32p(box->parent->parent->parent->rect.x0,
                                       box->parent->rect.y0,
                                       box->parent->parent->parent->rect.x0 + ui_top_font_size()*22.f*u->alive_t,
                                       box->parent->rect.y1),
                                v4f32(0, 0, 0, 0),
                                0, 0, 1);
    inst->colors[Corner_00] = inst->colors[Corner_01] = weak_thread_color;
  }
  
  // rjf: draw remaps
  if(u->remap_px_delta != 0)
  {
    F32 remap_px_delta = u->remap_px_delta;
    F32 circle_advance = f_dim_from_tag_size_string(box->font, box->font_size, 0, 0, df_g_icon_kind_text_table[DF_IconKind_CircleFilled]).x;
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
    d_text(box->font, box->font_size, 0, 0, F_RasterFlag_Smooth,
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
    df_rgba_from_theme_color(DF_ThemeColor_LineInfoBackground0),
    df_rgba_from_theme_color(DF_ThemeColor_LineInfoBackground1),
    df_rgba_from_theme_color(DF_ThemeColor_LineInfoBackground2),
    df_rgba_from_theme_color(DF_ThemeColor_LineInfoBackground3),
  };
  UI_Palette *margin_palette = df_palette_from_code(ws, DF_PaletteCode_Floating);
  UI_Palette *margin_contents_palette = ui_build_palette(df_palette_from_code(ws, DF_PaletteCode_Floating));
  margin_contents_palette->background = v4f32(0, 0, 0, 0);
  F32 line_num_padding_px = ui_top_font_size()*1.f;
  
  //////////////////////////////
  //- rjf: build top-level container
  //
  UI_Box *top_container_box = &ui_g_nil_box;
  Rng2F32 clipped_top_container_rect = {0};
  {
    ui_set_next_child_layout_axis(Axis2_X);
    ui_set_next_pref_width(ui_px(params->line_text_max_width_px, 1));
    ui_set_next_pref_height(ui_children_sum(1));
    top_container_box = ui_build_box_from_string(UI_BoxFlag_DisableFocusEffects|UI_BoxFlag_DrawBorder, string);
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
          line_bg_colors[line_idx] = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlayError);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build priority margin
  //
  UI_Box *priority_margin_container_box = &ui_g_nil_box;
  if(params->flags & DF_CodeSliceFlag_PriorityMargin) UI_Focus(UI_FocusKind_Off) UI_Parent(top_container_box) UI_Palette(margin_palette) ProfScope("build priority margins")
  {
    if(params->margin_float_off_px != 0)
    {
      ui_set_next_pref_width(ui_px(params->priority_margin_width_px, 1));
      ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
      ui_build_box_from_key(0, ui_key_zero());
      ui_set_next_fixed_x(floor_f32(params->margin_float_off_px));
    }
    ui_set_next_pref_width(ui_px(params->priority_margin_width_px, 1));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_child_layout_axis(Axis2_Y);
    priority_margin_container_box = ui_build_box_from_string(UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable), str8_lit("priority_margin_container"));
    UI_Parent(priority_margin_container_box) UI_PrefHeight(ui_px(params->line_height_px, 1.f)) UI_Palette(margin_contents_palette)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max;
          line_num += 1, line_idx += 1)
      {
        DF_EntityList line_ips  = params->line_ips[line_idx];
        ui_set_next_hover_cursor(OS_Cursor_HandPoint);
        UI_Box *line_margin_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawActiveEffects, "line_margin_%I64x", line_num);
        UI_Parent(line_margin_box)
        {
          //- rjf: build margin thread ip ui
          for(DF_EntityNode *n = line_ips.first; n != 0; n = n->next)
          {
            // rjf: unpack thread
            DF_Entity *thread = n->entity;
            if(thread != selected_thread)
            {
              continue;
            }
            U64 unwind_count = (thread == selected_thread) ? ctrl_ctx->unwind_count : 0;
            U64 thread_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
            DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, thread_rip_vaddr);
            DI_Key dbgi_key = df_dbgi_key_from_module(module);
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
                color = df_rgba_from_theme_color(DF_ThemeColor_ThreadError);
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
            ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
            ui_set_next_font_size(params->font_size);
            ui_set_next_text_raster_flags(F_RasterFlag_Smooth);
            ui_set_next_pref_width(ui_pct(1, 0));
            ui_set_next_pref_height(ui_pct(1, 0));
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Key thread_box_key = ui_key_from_stringf(top_container_box->key, "###ip_%I64x_%p", line_num, thread);
            UI_Box *thread_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|
                                                       UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|
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
              u->do_lines = df_setting_val_from_code(ws, DF_SettingCode_ThreadLines).s32;
              u->do_glow = df_setting_val_from_code(ws, DF_SettingCode_ThreadGlow).s32;
              ui_box_equip_custom_draw(thread_box, df_thread_box_draw_extensions, u);
              
              // rjf: fill out progress t (progress into range of current line's
              // voff range)
              if(params->line_infos[line_idx].first != 0)
              {
                DF_LineList *lines = &params->line_infos[line_idx];
                DF_Line *line = 0;
                for(DF_LineNode *n = lines->first; n != 0; n = n->next)
                {
                  if(di_key_match(&n->v.dbgi_key, &dbgi_key))
                  {
                    line = &n->v;
                    break;
                  }
                }
                if(line != 0)
                {
                  Rng1U64 line_voff_rng = line->voff_range;
                  Vec4F32 weak_thread_color = color;
                  weak_thread_color.w *= 0.4f;
                  F32 progress_t = (line_voff_rng.max != line_voff_rng.min) ? ((F32)(thread_rip_voff - line_voff_rng.min) / (F32)(line_voff_rng.max - line_voff_rng.min)) : 0;
                  progress_t = Clamp(0, progress_t, 1);
                  u->progress_t = progress_t;
                }
              }
            }
            
            // rjf: hover tooltips
            if(ui_hovering(thread_sig) && !df_drag_is_active())
            {
              df_entity_tooltips(ws, thread);
            }
            
            // rjf: ip right-click menu
            if(ui_right_clicked(thread_sig))
            {
              DF_Handle handle = df_handle_from_entity(thread);
              ui_ctx_menu_open(ws->entity_ctx_menu_key, thread_box->key, v2f32(0, thread_box->rect.y1-thread_box->rect.y0));
              ws->entity_ctx_menu_entity = handle;
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
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build catchall margin
  //
  UI_Box *catchall_margin_container_box = &ui_g_nil_box;
  if(params->flags & DF_CodeSliceFlag_CatchallMargin) UI_Focus(UI_FocusKind_Off) UI_Palette(margin_palette) UI_Parent(top_container_box) ProfScope("build catchall margins")
  {
    if(params->margin_float_off_px != 0)
    {
      ui_set_next_pref_width(ui_px(params->catchall_margin_width_px, 1));
      ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
      ui_build_box_from_key(0, ui_key_zero());
      ui_set_next_fixed_x(floor_f32(params->margin_float_off_px + params->priority_margin_width_px));
    }
    ui_set_next_pref_width(ui_px(params->catchall_margin_width_px, 1));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_child_layout_axis(Axis2_Y);
    catchall_margin_container_box = ui_build_box_from_string(UI_BoxFlag_DrawSideLeft|UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable), str8_lit("catchall_margin_container"));
    UI_Parent(catchall_margin_container_box) UI_PrefHeight(ui_px(params->line_height_px, 1.f)) UI_Palette(margin_contents_palette)
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
        UI_Box *line_margin_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawActiveEffects, "line_margin_%I64x", line_num);
        UI_Parent(line_margin_box)
        {
          //- rjf: build margin thread ip ui
          for(DF_EntityNode *n = line_ips.first; n != 0; n = n->next)
          {
            // rjf: unpack thread
            DF_Entity *thread = n->entity;
            if(thread == selected_thread)
            {
              continue;
            }
            U64 unwind_count = (thread == selected_thread) ? ctrl_ctx->unwind_count : 0;
            U64 thread_rip_vaddr = df_query_cached_rip_from_thread_unwind(thread, unwind_count);
            DF_Entity *process = df_entity_ancestor_from_kind(thread, DF_EntityKind_Process);
            DF_Entity *module = df_module_from_process_vaddr(process, thread_rip_vaddr);
            DI_Key dbgi_key = df_dbgi_key_from_module(module);
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
                color = df_rgba_from_theme_color(DF_ThemeColor_ThreadError);
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
            ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
            ui_set_next_font_size(params->font_size);
            ui_set_next_text_raster_flags(F_RasterFlag_Smooth);
            ui_set_next_pref_width(ui_pct(1, 0));
            ui_set_next_pref_height(ui_pct(1, 0));
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Key thread_box_key = ui_key_from_stringf(top_container_box->key, "###ip_%I64x_catchall_%p", line_num, thread);
            UI_Box *thread_box = ui_build_box_from_key(UI_BoxFlag_DisableTextTrunc|
                                                       UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|
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
              if(!df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)) && params->line_infos[line_idx].first != 0)
              {
                DF_LineList *lines = &params->line_infos[line_idx];
                DF_Line *line = 0;
                for(DF_LineNode *n = lines->first; n != 0; n = n->next)
                {
                  if(di_key_match(&n->v.dbgi_key, &dbgi_key))
                  {
                    line = &n->v;
                    break;
                  }
                }
                if(line != 0)
                {
                  Rng1U64 line_voff_rng = line->voff_range;
                  Vec4F32 weak_thread_color = color;
                  weak_thread_color.w *= 0.4f;
                  F32 progress_t = (line_voff_rng.max != line_voff_rng.min) ? ((F32)(thread_rip_voff - line_voff_rng.min) / (F32)(line_voff_rng.max - line_voff_rng.min)) : 0;
                  progress_t = Clamp(0, progress_t, 1);
                  u->progress_t = progress_t;
                }
              }
            }
            
            // rjf: hover tooltips
            if(ui_hovering(thread_sig) && !df_drag_is_active())
            {
              df_entity_tooltips(ws, thread);
            }
            
            // rjf: ip right-click menu
            if(ui_right_clicked(thread_sig))
            {
              DF_Handle handle = df_handle_from_entity(thread);
              ui_ctx_menu_open(ws->entity_ctx_menu_key, thread_box->key, v2f32(0, thread_box->rect.y1-thread_box->rect.y0));
              ws->entity_ctx_menu_entity = handle;
            }
            
            // rjf: double click => select
            if(ui_double_clicked(thread_sig))
            {
              DF_CmdParams params = df_cmd_params_from_window(ws);
              params.entity = df_handle_from_entity(thread);
              df_cmd_params_mark_slot(&params, DF_CmdParamSlot_Entity);
              df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SelectThread));
              ui_kill_action();
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
            Vec4F32 bp_color = df_rgba_from_theme_color(DF_ThemeColor_Breakpoint);
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
              bp_draw->alive_t = bp->alive_t;
              bp_draw->do_lines = df_setting_val_from_code(ws, DF_SettingCode_BreakpointLines).s32;
              bp_draw->do_glow = df_setting_val_from_code(ws, DF_SettingCode_BreakpointGlow).s32;
              if(!df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)))
              {
                DF_LineList *lines = &params->line_infos[line_idx];
                for(DF_LineNode *n = lines->first; n != 0; n = n->next)
                {
                  S64 remap_line = n->v.pt.line;
                  if(remap_line != line_num)
                  {
                    bp_draw->remap_px_delta = (remap_line - line_num) * params->line_height_px;
                    break;
                  }
                }
              }
            }
            
            // rjf: build box for breakpoint
            ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
            ui_set_next_font_size(params->font_size * 1.f);
            ui_set_next_text_raster_flags(F_RasterFlag_Smooth);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = bp_color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Box *bp_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                                       UI_BoxFlag_DrawActiveEffects|
                                                       UI_BoxFlag_DrawHotEffects|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|
                                                       UI_BoxFlag_DisableTextTrunc,
                                                       "%S##bp_%p",
                                                       df_g_icon_kind_text_table[DF_IconKind_CircleFilled],
                                                       bp);
            ui_box_equip_custom_draw(bp_box, df_bp_box_draw_extensions, bp_draw);
            UI_Signal bp_sig = ui_signal_from_box(bp_box);
            
            // rjf: bp hovering
            if(ui_hovering(bp_sig) && !df_drag_is_active())
            {
              df_entity_tooltips(ws, bp);
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
              ui_ctx_menu_open(ws->entity_ctx_menu_key, bp_box->key, v2f32(0, bp_box->rect.y1-bp_box->rect.y0));
              ws->entity_ctx_menu_entity = handle;
            }
          }
          
          //- rjf: build margin watch pin ui
          for(DF_EntityNode *n = line_pins.first; n != 0; n = n->next)
          {
            DF_Entity *pin = n->entity;
            Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_Text);
            if(pin->flags & DF_EntityFlag_HasColor)
            {
              color = df_rgba_from_entity(pin);
            }
            
            // rjf: build box for watch
            ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
            ui_set_next_font_size(params->font_size * 1.f);
            ui_set_next_text_raster_flags(F_RasterFlag_Smooth);
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = color));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            UI_Box *pin_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|
                                                        UI_BoxFlag_DrawActiveEffects|
                                                        UI_BoxFlag_DrawHotEffects|
                                                        UI_BoxFlag_DrawBorder|
                                                        UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|
                                                        UI_BoxFlag_DisableTextTrunc,
                                                        "%S##watch_%p",
                                                        df_g_icon_kind_text_table[DF_IconKind_Pin],
                                                        pin);
            UI_Signal pin_sig = ui_signal_from_box(pin_box);
            
            // rjf: watch hovering
            if(ui_hovering(pin_sig) && !df_drag_is_active())
            {
              df_entity_tooltips(ws, pin);
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
              ui_ctx_menu_open(ws->entity_ctx_menu_key, pin_box->key, v2f32(0, pin_box->rect.y1-pin_box->rect.y0));
              ws->entity_ctx_menu_entity = handle;
            }
          }
        }
        
        // rjf: empty margin interaction
        UI_Signal line_margin_sig = ui_signal_from_box(line_margin_box);
        if(ui_clicked(line_margin_sig))
        {
          if(!df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)))
          {
            TxtPt pt = txt_pt(line_num, 1);
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.entity = df_interact_regs()->file;
            p.text_point = pt;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_TextBreakpoint));
          }
          else if(params->line_vaddrs[line_idx] != 0)
          {
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.vaddr = params->line_vaddrs[line_idx];
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_AddressBreakpoint));
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build line numbers
  //
  if(params->flags & DF_CodeSliceFlag_LineNums) UI_Parent(top_container_box) ProfScope("build line numbers") UI_Focus(UI_FocusKind_Off)
  {
    TxtRng select_rng = txt_rng(*cursor, *mark);
    Vec4F32 active_color = df_rgba_from_theme_color(DF_ThemeColor_CodeLineNumbersSelected);
    Vec4F32 inactive_color = df_rgba_from_theme_color(DF_ThemeColor_CodeLineNumbers);
    ui_set_next_fixed_x(floor_f32(params->margin_float_off_px + params->priority_margin_width_px + params->catchall_margin_width_px));
    ui_set_next_pref_width(ui_px(params->line_num_width_px, 1.f));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_set_next_flags(UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight);
    UI_Column
      UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      DF_Font(ws, DF_FontSlot_Code)
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
          U64 best_stamp = 0;
          S64 line_info_line_num = 0;
          F32 line_info_t = selected_thread_module->alive_t;
          DF_LineList *lines = &params->line_infos[line_idx];
          for(DF_LineNode *n = lines->first; n != 0; n = n->next)
          {
            if(n->v.dbgi_key.min_timestamp >= best_stamp)
            {
              has_line_info = (n->v.pt.line == line_num || df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)));
              line_info_line_num = n->v.pt.line;
              best_stamp = n->v.dbgi_key.min_timestamp;
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
        ui_set_next_palette(ui_build_palette(ui_top_palette(), .text = text_color, .background = bg_color));
        ui_build_box_from_stringf(UI_BoxFlag_DrawText|(UI_BoxFlag_DrawBackground*!!has_line_info), "%I64u##line_num", line_num);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: build background for line numbers & margins
  //
  {
    UI_Parent(top_container_box) DF_Palette(ws, DF_PaletteCode_Floating)
    {
      ui_set_next_pref_width(ui_px(params->priority_margin_width_px + params->catchall_margin_width_px + params->line_num_width_px, 1));
      ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
      ui_set_next_fixed_x(floor_f32(params->margin_float_off_px));
      ui_build_box_from_key(UI_BoxFlag_DrawBackgroundBlur|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, ui_key_zero());
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
    text_container_box = ui_build_box_from_string(UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable), str8_lit("text_container"));
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
      F32 line_text_dim = f_dim_from_tag_size_string(params->font, params->font_size, 0, params->tab_size, line_text).x + params->line_num_width_px;
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
            UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextNegative)))
          {
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder, "###exception_info");
            UI_Parent(box) UI_PrefWidth(ui_text_dim(10, 1))
            {
              DF_Font(ws, DF_FontSlot_Icons) ui_label(df_g_icon_kind_text_table[DF_IconKind_WarningBig]);
              ui_label(explanation);
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
    DI_Scope *scope = di_scope_open();
    U64 line_idx = 0;
    for(S64 line_num = params->line_num_range.min;
        line_num < params->line_num_range.max;
        line_num += 1, line_idx += 1)
    {
      DF_EntityList pins = params->line_pins[line_idx];
      if(pins.count != 0) UI_Parent(line_extras_boxes[line_idx])
        DF_Font(ws, DF_FontSlot_Code)
        UI_FontSize(params->font_size)
        UI_PrefHeight(ui_px(params->line_height_px, 1.f))
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
          UI_Box *pin_box = ui_build_box_from_key(UI_BoxFlag_AnimatePos|
                                                  UI_BoxFlag_Clickable*!!(params->flags & DF_CodeSliceFlag_Clickable)|
                                                  UI_BoxFlag_DrawHotEffects|
                                                  UI_BoxFlag_DrawBorder, pin_box_key);
          UI_Parent(pin_box) UI_PrefWidth(ui_text_dim(10, 1))
          {
            Vec4F32 pin_color = df_rgba_from_theme_color(DF_ThemeColor_CodeDefault);
            if(pin->flags & DF_EntityFlag_HasColor)
            {
              pin_color = df_rgba_from_entity(pin);
            }
            UI_PrefWidth(ui_em(1.5f, 1.f))
              DF_Font(ws, DF_FontSlot_Icons)
              UI_Palette(ui_build_palette(ui_top_palette(), .text = pin_color))
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
              if(ui_right_clicked(sig))
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
    di_scope_close(scope);
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
    S64 column = f_char_pos_from_tag_size_string_p(params->font, params->font_size, 0, params->tab_size, line_string, mouse.x-text_container_box->rect.x0-params->line_num_width_px-line_num_padding_px)+1;
    
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
  UI_Signal priority_margin_container_sig = ui_signal_from_box(priority_margin_container_box);
  UI_Signal catchall_margin_container_sig = ui_signal_from_box(catchall_margin_container_box);
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
    
    //- rjf: right-click => code context menu
    if(ui_right_clicked(text_container_sig))
    {
      if(txt_pt_match(*cursor, *mark))
      {
        *cursor = *mark = mouse_pt;
      }
      ui_ctx_menu_open(ws->code_ctx_menu_key, ui_key_zero(), sub_2f32(ui_mouse(), v2f32(2, 2)));
      arena_clear(ws->code_ctx_menu_arena);
      ws->code_ctx_menu_file      = df_interact_regs()->file;
      ws->code_ctx_menu_text_key  = df_interact_regs()->text_key;
      ws->code_ctx_menu_lang_kind = df_interact_regs()->lang_kind;
      ws->code_ctx_menu_range     = txt_rng(*cursor, *mark);
      if(params->line_num_range.min <= cursor->line && cursor->line < params->line_num_range.max)
      {
        ws->code_ctx_menu_vaddr = params->line_vaddrs[cursor->line - params->line_num_range.min];
      }
      if(params->line_num_range.min <= cursor->line && cursor->line < params->line_num_range.max)
      {
        ws->code_ctx_menu_lines = df_line_list_copy(ws->code_ctx_menu_arena, &params->line_infos[cursor->line - params->line_num_range.min]);
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
      if(!df_entity_is_nil(line_drag_entity) && df_drag_drop(&payload) && contains_1s64(params->line_num_range, mouse_pt.line))
      {
        DF_Entity *dropped_entity = line_drag_entity;
        S64 line_num = mouse_pt.line;
        U64 line_idx = line_num - params->line_num_range.min;
        U64 line_vaddr = params->line_vaddrs[line_idx];
        switch(dropped_entity->kind)
        {
          default:{}break;
          case DF_EntityKind_Breakpoint:
          case DF_EntityKind_WatchPin:
          {
            if(!df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)))
            {
              df_entity_change_parent(0, dropped_entity, dropped_entity->parent, df_entity_from_handle(df_interact_regs()->file));
              df_entity_equip_txt_pt(dropped_entity, txt_pt(line_num, 1));
              if(dropped_entity->flags & DF_EntityFlag_HasVAddr)
              {
                dropped_entity->flags &= ~DF_EntityFlag_HasVAddr;
              }
            }
            else if(line_vaddr != 0)
            {
              df_entity_change_parent(0, dropped_entity, dropped_entity->parent, df_entity_root());
              df_entity_equip_vaddr(dropped_entity, line_vaddr);
            }
          }break;
          case DF_EntityKind_Thread:
          {
            U64 new_rip_vaddr = line_vaddr;
            if(!df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)))
            {
              DF_LineList *lines = &params->line_infos[line_idx];
              for(DF_LineNode *n = lines->first; n != 0; n = n->next)
              {
                DF_EntityList modules = df_modules_from_dbgi_key(scratch.arena, &n->v.dbgi_key);
                DF_Entity *module = df_module_from_thread_candidates(dropped_entity, &modules);
                if(!df_entity_is_nil(module))
                {
                  new_rip_vaddr = df_vaddr_from_voff(module, n->v.voff_range.min);
                  break;
                }
              }
            }
            DF_CmdParams p = df_cmd_params_from_window(ws);
            p.entity = df_handle_from_entity(dropped_entity);
            p.vaddr = new_rip_vaddr;
            df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_SetThreadIP));
          }break;
        }
      }
    }
    
    //- rjf: commit text container signal to main output
    result.base = text_container_sig;
  }
  
  //////////////////////////////
  //- rjf: mouse -> expression range info
  //
  TxtRng mouse_expr_rng = {0};
  Vec2F32 mouse_expr_baseline_pos = {0};
  String8 mouse_expr = {0};
  if(ui_hovering(text_container_sig) && contains_1s64(params->line_num_range, mouse_pt.line)) ProfScope("mouse -> expression range")
  {
    TxtRng selected_rng = txt_rng(*cursor, *mark);
    if(!txt_pt_match(*cursor, *mark) && cursor->line == mark->line &&
       ((txt_pt_less_than(selected_rng.min, mouse_pt) || txt_pt_match(selected_rng.min, mouse_pt)) &&
        txt_pt_less_than(mouse_pt, selected_rng.max)))
    {
      U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
      String8 line_text = params->line_text[line_slice_idx];
      F32 expr_hoff_px = params->line_num_width_px + f_dim_from_tag_size_string(params->font, params->font_size, 0, params->tab_size, str8_prefix(line_text, selected_rng.min.column-1)).x;
      result.mouse_expr_rng = mouse_expr_rng = selected_rng;
      result.mouse_expr_baseline_pos = mouse_expr_baseline_pos = v2f32(text_container_box->rect.x0+expr_hoff_px,
                                                                       text_container_box->rect.y0+line_slice_idx*params->line_height_px + params->line_height_px*0.85f);
      mouse_expr = str8_substr(line_text, r1u64(selected_rng.min.column-1, selected_rng.max.column-1));
    }
    else
    {
      U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
      String8 line_text = params->line_text[line_slice_idx];
      TXT_TokenArray line_tokens = params->line_tokens[line_slice_idx];
      Rng1U64 line_range = params->line_ranges[line_slice_idx];
      U64 mouse_pt_off = line_range.min + (mouse_pt.column-1);
      Rng1U64 expr_off_rng = txt_expr_off_range_from_line_off_range_string_tokens(mouse_pt_off, line_range, line_text, &line_tokens);
      if(expr_off_rng.max != expr_off_rng.min)
      {
        F32 expr_hoff_px = params->line_num_width_px + f_dim_from_tag_size_string(params->font, params->font_size, 0, params->tab_size, str8_prefix(line_text, expr_off_rng.min-line_range.min)).x;
        result.mouse_expr_rng = mouse_expr_rng = txt_rng(txt_pt(mouse_pt.line, 1+(expr_off_rng.min-line_range.min)), txt_pt(mouse_pt.line, 1+(expr_off_rng.max-line_range.min)));
        result.mouse_expr_baseline_pos = mouse_expr_baseline_pos = v2f32(text_container_box->rect.x0+expr_hoff_px,
                                                                         text_container_box->rect.y0+line_slice_idx*params->line_height_px + params->line_height_px*0.85f);
        mouse_expr = str8_substr(line_text, r1u64(expr_off_rng.min-line_range.min, expr_off_rng.max-line_range.min));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: mouse -> set global frontend hovered line info
  //
  if(ui_hovering(text_container_sig) && contains_1s64(params->line_num_range, mouse_pt.line) && (ui_mouse().x - text_container_box->rect.x0 < params->line_num_width_px + line_num_padding_px))
  {
    U64 line_slice_idx = mouse_pt.line-params->line_num_range.min;
    DF_LineList *lines = &params->line_infos[line_slice_idx];
    if(lines->first != 0 && (df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file)) || lines->first->v.pt.line == mouse_pt.line))
    {
      DF_RichHoverInfo info = {0};
      info.process      = df_handle_from_entity(selected_thread_process);
      info.vaddr_range  = df_vaddr_range_from_voff_range(selected_thread_module, lines->first->v.voff_range);
      info.module       = df_handle_from_entity(selected_thread_module);
      info.dbgi_key     = lines->first->v.dbgi_key;
      info.voff_range   = lines->first->v.voff_range;
      df_set_rich_hover_info(&info);
    }
  }
  
  //////////////////////////////
  //- rjf: hover eval
  //
  if(!ui_dragging(text_container_sig) && text_container_sig.event_flags == 0 && mouse_expr.size != 0)
  {
    DI_Scope *di_scope = di_scope_open();
    DF_Eval eval = df_eval_from_string(scratch.arena, di_scope, ctrl_ctx, parse_ctx, &eval_string2expr_map_nil, mouse_expr);
    if(eval.mode != EVAL_EvalMode_NULL)
    {
      U64 line_vaddr = 0;
      if(contains_1s64(params->line_num_range, mouse_pt.line))
      {
        U64 line_idx = mouse_pt.line-params->line_num_range.min;
        line_vaddr = params->line_vaddrs[line_idx];
      }
      df_set_hover_eval(ws, mouse_expr_baseline_pos, *ctrl_ctx, df_entity_from_handle(df_interact_regs()->file), mouse_pt, line_vaddr, mouse_expr);
    }
    di_scope_close(di_scope);
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
      n->color = ui_top_palette()->colors[UI_ColorCode_Selection];
      SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, n);
    }
    
    // rjf: push for ctrlified mouse expr
    if(ctrlified && !txt_pt_match(result.mouse_expr_rng.max, result.mouse_expr_rng.min))
    {
      TxtRngColorPairNode *n = push_array(scratch.arena, TxtRngColorPairNode, 1);
      n->rng = result.mouse_expr_rng;
      n->color = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
      SLLQueuePush(first_txt_rng_color_pair, last_txt_rng_color_pair, n);
    }
  }
  
  //////////////////////////////
  //- rjf: build line numbers region (line number interaction should be basically identical to lines)
  //
  if(params->flags & DF_CodeSliceFlag_LineNums) UI_Parent(text_container_box) ProfScope("build line number interaction box") UI_Focus(UI_FocusKind_Off)
  {
    ui_set_next_pref_width(ui_px(params->line_num_width_px, 1.f));
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    ui_build_box_from_key(0, ui_key_zero());
  }
  
  //////////////////////////////
  //- rjf: build line text
  //
  UI_Parent(text_container_box) ProfScope("build line text") UI_Focus(UI_FocusKind_Off)
  {
    DF_RichHoverInfo rich_hover = df_get_rich_hover_info();
    Rng1U64 rich_hover_voff_range = rich_hover.voff_range;
    if(rich_hover_voff_range.min == 0 && rich_hover_voff_range.max == 0)
    {
      DF_Entity *module = df_entity_from_handle(rich_hover.module);
      rich_hover_voff_range = df_voff_range_from_vaddr_range(module, rich_hover.vaddr_range);
    }
    ui_set_next_pref_height(ui_px(params->line_height_px*(dim_1s64(params->line_num_range)+1), 1.f));
    UI_WidthFill
      UI_Column
      UI_PrefHeight(ui_px(params->line_height_px, 1.f))
      DF_Font(ws, DF_FontSlot_Code)
      UI_FontSize(params->font_size)
      UI_CornerRadius(0)
    {
      U64 line_idx = 0;
      for(S64 line_num = params->line_num_range.min;
          line_num <= params->line_num_range.max; line_num += 1, line_idx += 1)
      {
        String8 line_string = params->line_text[line_idx];
        Rng1U64 line_range = params->line_ranges[line_idx];
        TXT_TokenArray *line_tokens = &params->line_tokens[line_idx];
        ui_set_next_text_padding(line_num_padding_px);
        UI_Key line_key = ui_key_from_stringf(top_container_box->key, "ln_%I64x", line_num);
        Vec4F32 line_bg_color = line_bg_colors[line_idx];
        if(line_bg_color.w != 0)
        {
          ui_set_next_flags(UI_BoxFlag_DrawBackground);
          ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = line_bg_color));
        }
        ui_set_next_tab_size(params->tab_size);
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
                  for(DI_KeyNode *n = params->relevant_dbgi_keys.first; n != 0; n = n->next)
                  {
                    DI_Key dbgi_key = n->v;
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 voff = df_voff_from_dbgi_key_symbol_name(&dbgi_key, token_string);
                      if(voff != 0)
                      {
                        mapped_special = 1;
                        new_color_kind = DF_ThemeColor_CodeSymbol;
                        mix_t = selected_thread_module->alive_t;
                      }
                    }
                    if(!mapped_special && token->kind == TXT_TokenKind_Identifier)
                    {
                      U64 type_num = df_type_num_from_dbgi_key_name(&dbgi_key, token_string);
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
                    break;
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
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, match_range.min)).x,
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, match_range.max)).x,
              };
              Rng2F32 match_rect =
              {
                line_box->rect.x0+line_num_padding_px+match_column_pixel_off_range.min,
                line_box->rect.y0,
                line_box->rect.x0+line_num_padding_px+match_column_pixel_off_range.max+2.f,
                line_box->rect.y1,
              };
              Vec4F32 color = df_rgba_from_theme_color(DF_ThemeColor_HighlightOverlay);
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
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, select_column_range_in_line.min-1)).x,
                f_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, select_column_range_in_line.max-1)).x,
              };
              Rng2F32 select_rect =
              {
                line_box->rect.x0+line_num_padding_px+select_column_pixel_off_range.min-2.f,
                floor_f32(line_box->rect.y0) - 1.f,
                line_box->rect.x0+line_num_padding_px+select_column_pixel_off_range.max+2.f,
                ceil_f32(line_box->rect.y1) + 1.f,
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
          Vec2F32 advance = f_dim_from_tag_size_string(line_box->font, line_box->font_size, 0, params->tab_size, str8_prefix(line_string, column-1));
          F32 cursor_off_pixels = advance.x;
          F32 cursor_thickness = ClampBot(4.f, line_box->font_size/6.f);
          Rng2F32 cursor_rect =
          {
            ui_box_text_position(line_box).x+cursor_off_pixels-cursor_thickness/2.f,
            line_box->rect.y0-params->font_size*0.25f,
            ui_box_text_position(line_box).x+cursor_off_pixels+cursor_thickness/2.f,
            line_box->rect.y1+params->font_size*0.25f,
          };
          d_rect(cursor_rect, df_rgba_from_theme_color(is_focused ? DF_ThemeColor_Cursor : DF_ThemeColor_CursorInactive), 1.f, 0, 1.f);
        }
        
        // rjf: extra rendering for lines with line-info that match the hovered
        {
          B32 matches = 0;
          S64 line_info_line_num = 0;
          DF_LineList *lines = &params->line_infos[line_idx];
          for(DF_LineNode *n = lines->first; n != 0; n = n->next)
          {
            if((n->v.pt.line == line_num || df_entity_is_nil(df_entity_from_handle(df_interact_regs()->file))) &&
               ((di_key_match(&n->v.dbgi_key, &rich_hover.dbgi_key) &&
                 n->v.voff_range.min <= rich_hover_voff_range.min && rich_hover_voff_range.min < n->v.voff_range.max) ||
                (params->line_vaddrs[line_idx] == rich_hover.vaddr_range.min && rich_hover.vaddr_range.min != 0)))
            {
              matches = 1;
              line_info_line_num = n->v.pt.line;
              break;
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
  UI_EventList *events = ui_events();
  for(UI_EventNode *n = events->first, *next = 0; n != 0; n = next)
  {
    next = n->next;
    if(n->v.kind != UI_EventKind_Navigate && n->v.kind != UI_EventKind_Edit)
    {
      continue;
    }
    B32 taken = 0;
    String8 line = txt_string_from_info_data_line_num(info, data, cursor->line);
    UI_TxtOp single_line_op = ui_single_line_txt_op_from_event(scratch.arena, &n->v, line, *cursor, *mark);
    
    //- rjf: invalid single-line op or endpoint units => try multiline
    if(n->v.delta_unit == UI_EventDeltaUnit_Whole || single_line_op.flags & UI_TxtOpFlag_Invalid)
    {
      U64 line_count = info->lines_count;
      String8 prev_line = txt_string_from_info_data_line_num(info, data, cursor->line-1);
      String8 next_line = txt_string_from_info_data_line_num(info, data, cursor->line+1);
      Vec2S32 delta = n->v.delta_2s32;
      
      //- rjf: wrap lines right
      if(n->v.delta_unit != UI_EventDeltaUnit_Whole && delta.x > 0 && cursor->column == line.size+1 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = 1;
        *preferred_column = 1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: wrap lines left
      if(n->v.delta_unit != UI_EventDeltaUnit_Whole && delta.x < 0 && cursor->column == 1 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = prev_line.size+1;
        *preferred_column = prev_line.size+1;
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (plain)
      if(n->v.delta_unit == UI_EventDeltaUnit_Char && delta.y > 0 && cursor->line+1 <= line_count)
      {
        cursor->line += 1;
        cursor->column = Min(*preferred_column, next_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (plain)
      if(n->v.delta_unit == UI_EventDeltaUnit_Char && delta.y < 0 && cursor->line-1 >= 1)
      {
        cursor->line -= 1;
        cursor->column = Min(*preferred_column, prev_line.size+1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement down (chunk)
      if(n->v.delta_unit == UI_EventDeltaUnit_Word && delta.y > 0 && cursor->line+1 <= line_count)
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
      if(n->v.delta_unit == UI_EventDeltaUnit_Word && delta.y < 0 && cursor->line-1 >= 1)
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
      if(n->v.delta_unit == UI_EventDeltaUnit_Page && delta.y > 0)
      {
        cursor->line += line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement up (page)
      if(n->v.delta_unit == UI_EventDeltaUnit_Page && delta.y < 0)
      {
        cursor->line -= line_count_per_page;
        cursor->column = 1;
        cursor->line = Clamp(1, cursor->line, line_count);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (+)
      if(n->v.delta_unit == UI_EventDeltaUnit_Whole && (delta.y > 0 || delta.x > 0))
      {
        *cursor = txt_pt(line_count, info->lines_count ? dim_1u64(info->lines_ranges[info->lines_count-1])+1 : 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: movement to endpoint (-)
      if(n->v.delta_unit == UI_EventDeltaUnit_Whole && (delta.y < 0 || delta.x < 0))
      {
        *cursor = txt_pt(1, 1);
        change = 1;
        taken = 1;
      }
      
      //- rjf: stick mark to cursor, when we don't want to keep it in the same spot
      if(!(n->v.flags & UI_EventFlag_KeepMark))
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
    if(n->v.flags & UI_EventFlag_Copy)
    {
      String8 text = txt_string_from_info_data_txt_rng(info, data, txt_rng(*cursor, *mark));
      os_set_clipboard_text(text);
      taken = 1;
    }
    
    //- rjf: consume
    if(taken)
    {
      ui_eat_event(events, n);
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
  UI_Parent(box) UI_Palette(ui_build_palette(ui_top_palette(), .text = df_rgba_from_theme_color(DF_ThemeColor_TextNegative), .text_weak = df_rgba_from_theme_color(DF_ThemeColor_TextNegative)))
  {
    ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
    ui_set_next_text_raster_flags(F_RasterFlag_Smooth);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    ui_set_next_flags(UI_BoxFlag_DrawTextWeak);
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
      ui_set_next_font(df_font_from_slot(DF_FontSlot_Icons));
      ui_set_next_text_raster_flags(F_RasterFlag_Smooth);
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
        Vec4F32 token_color_rgba_alt = df_rgba_from_theme_color(DF_ThemeColor_CodeNumericAltDigitGroup);
        token_color_rgba_alt.w *= alpha;
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
df_line_edit(DF_Window *ws, DF_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, String8 string)
{
  //- rjf: unpack visual metrics
  F32 expander_size_px = ui_top_font_size()*1.5f;
  
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
    UI_FlagsAdd(UI_BoxFlag_DrawTextWeak)
      UI_Flags(UI_BoxFlag_DrawSideLeft)
      DF_Font(ws, DF_FontSlot_Icons)
      UI_TextAlignment(UI_TextAlign_Center)
      ui_label(df_g_icon_kind_text_table[DF_IconKind_Dot]);
  }
  
  //- rjf: build expander space
  else if(flags & DF_LineEditFlag_ExpanderSpace) UI_Parent(box) UI_Focus(UI_FocusKind_Off)
  {
    UI_Flags(UI_BoxFlag_DrawSideLeft) ui_spacer(ui_px(expander_size_px, 1.f));
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
    UI_EventList *events = ui_events();
    for(UI_EventNode *n = events->first, *next = 0; n != 0; n = next)
    {
      next = n->next;
      UI_Event *evt = &n->v;
      if(evt->flags & UI_EventFlag_Copy)
      {
        os_set_clipboard_text(pre_edit_value);
      }
      if(evt->flags & UI_EventFlag_Delete)
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
      UI_EventList *events = ui_events();
      for(UI_EventNode *n = events->first; n != 0; n = n->next)
      {
        if(n->v.string.size != 0 || n->v.flags & UI_EventFlag_Paste)
        {
          start_editing_via_typing = 1;
          break;
        }
      }
    }
    if(is_focus_hot && ui_slot_press(UI_EventActionSlot_Edit))
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
  
  //- rjf: determine autocompletion string
  String8 autocomplete_hint_string = {0};
  {
    UI_EventList *events = ui_events();
    for(UI_EventNode *n = events->first; n != 0; n = n->next)
    {
      if(n->v.kind == UI_EventKind_AutocompleteHint)
      {
        autocomplete_hint_string = n->v.string;
      }
    }
  }
  
  //- rjf: take navigation actions for editing
  B32 changes_made = 0;
  if(!(flags & DF_LineEditFlag_DisableEdit) && (is_focus_active || focus_started))
  {
    Temp scratch = scratch_begin(0, 0);
    UI_EventList *events = ui_events();
    for(UI_EventNode *n = events->first, *next = 0; n != 0; n = next)
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      next = n->next;
      
      // rjf: do not consume anything that doesn't fit a single-line's operations
      if((n->v.kind != UI_EventKind_Edit && n->v.kind != UI_EventKind_Navigate && n->v.kind != UI_EventKind_Text) || n->v.delta_2s32.y != 0)
      {
        continue;
      }
      
      // rjf: map this action to an op
      UI_TxtOp op = ui_single_line_txt_op_from_event(scratch.arena, &n->v, edit_string, *cursor, *mark);
      
      // rjf: any valid op & autocomplete hint? -> perform autocomplete first, then re-compute op
      if(autocomplete_hint_string.size != 0)
      {
        String8 word_query = df_autocomp_query_word_from_input_string_off(edit_string, cursor->column-1);
        U64 word_off = (U64)(word_query.str - edit_string.str);
        String8 new_string = ui_push_string_replace_range(scratch.arena, edit_string, r1s64(word_off+1, word_off+1+word_query.size), autocomplete_hint_string);
        new_string.size = Min(edit_buffer_size, new_string.size);
        MemoryCopy(edit_buffer, new_string.str, new_string.size);
        edit_string_size_out[0] = new_string.size;
        *cursor = *mark = txt_pt(1, word_off+1+autocomplete_hint_string.size);
        edit_string = str8(edit_buffer, edit_string_size_out[0]);
        op = ui_single_line_txt_op_from_event(scratch.arena, &n->v, edit_string, *cursor, *mark);
        MemoryZeroStruct(&autocomplete_hint_string);
      }
      
      // rjf: perform replace range
      if(!txt_pt_match(op.range.min, op.range.max) || op.replace.size != 0)
      {
        String8 new_string = ui_push_string_replace_range(scratch.arena, edit_string, r1s64(op.range.min.column, op.range.max.column), op.replace);
        new_string.size = Min(edit_buffer_size, new_string.size);
        MemoryCopy(edit_buffer, new_string.str, new_string.size);
        edit_string_size_out[0] = new_string.size;
      }
      
      // rjf: perform copy
      if(op.flags & UI_TxtOpFlag_Copy)
      {
        os_set_clipboard_text(op.copy);
      }
      
      // rjf: commit op's changed cursor & mark to caller-provided state
      *cursor = op.cursor;
      *mark = op.mark;
      
      // rjf: consume event
      {
        ui_eat_event(events, n);
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
        UI_Box *box = df_code_label(1.f, 1, ui_top_palette()->text, display_string);
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
      else if(flags & DF_LineEditFlag_DisplayStringIsCode)
      {
        UI_Box *box = df_code_label(1.f, 1, ui_top_palette()->text, display_string);
        if(matches != 0)
        {
          ui_box_equip_fuzzy_match_ranges(box, matches);
        }
      }
      else
      {
        ui_set_next_flags(UI_BoxFlag_DrawTextWeak);
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
        ui_set_next_flags(UI_BoxFlag_DrawTextWeak);
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
      F32 total_text_width = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), edit_string).x;
      F32 total_editstr_width = total_text_width - !!(flags & (DF_LineEditFlag_Expander|DF_LineEditFlag_ExpanderSpace|DF_LineEditFlag_ExpanderPlaceholder)) * expander_size_px;
      ui_set_next_pref_width(ui_px(total_editstr_width+ui_top_font_size()*2, 0.f));
      UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc, "###editstr");
      D_FancyStringList code_fancy_strings = df_fancy_string_list_from_code_string(scratch.arena, 1.f, 0, ui_top_palette()->text, edit_string);
      if(autocomplete_hint_string.size != 0)
      {
        String8 query_word = df_autocomp_query_word_from_input_string_off(edit_string, cursor->column-1);
        String8 autocomplete_append_string = str8_skip(autocomplete_hint_string, query_word.size);
        U64 off = 0;
        U64 cursor_off = cursor->column-1;
        D_FancyStringNode *prev_n = 0;
        for(D_FancyStringNode *n = code_fancy_strings.first; n != 0; n = n->next)
        {
          if(off <= cursor_off && cursor_off <= off+n->v.string.size)
          {
            prev_n = n;
            break;
          }
          off += n->v.string.size;
        }
        {
          D_FancyStringNode *autocomp_fstr_n = push_array(scratch.arena, D_FancyStringNode, 1);
          D_FancyString *fstr = &autocomp_fstr_n->v;
          fstr->font = ui_top_font();
          fstr->string = autocomplete_append_string;
          fstr->color = ui_top_palette()->text;
          fstr->color.w *= 0.5f;
          fstr->size = ui_top_font_size();
          autocomp_fstr_n->next = prev_n ? prev_n->next : 0;
          if(prev_n != 0)
          {
            prev_n->next = autocomp_fstr_n;
          }
          if(prev_n == 0)
          {
            code_fancy_strings.first = code_fancy_strings.last = autocomp_fstr_n;
          }
          if(prev_n != 0 && prev_n->next == 0)
          {
            code_fancy_strings.last = autocomp_fstr_n;
          }
          code_fancy_strings.node_count += 1;
          code_fancy_strings.total_size += autocomplete_hint_string.size;
          if(prev_n != 0 && cursor_off - off < prev_n->v.string.size)
          {
            String8 full_string = prev_n->v.string;
            U64 chop_amt = full_string.size - (cursor_off - off);
            prev_n->v.string = str8_chop(full_string, chop_amt);
            code_fancy_strings.total_size -= chop_amt;
            if(chop_amt != 0)
            {
              String8 post_cursor = str8_skip(full_string, cursor_off - off);
              D_FancyStringNode *post_fstr_n = push_array(scratch.arena, D_FancyStringNode, 1);
              D_FancyString *post_fstr = &post_fstr_n->v;
              MemoryCopyStruct(post_fstr, &prev_n->v);
              post_fstr->string   = post_cursor;
              if(autocomp_fstr_n->next == 0)
              {
                code_fancy_strings.last = post_fstr_n;
              }
              post_fstr_n->next = autocomp_fstr_n->next;
              autocomp_fstr_n->next = post_fstr_n;
              code_fancy_strings.node_count += 1;
              code_fancy_strings.total_size += post_cursor.size;
            }
          }
        }
      }
      ui_box_equip_display_fancy_strings(editstr_box, &code_fancy_strings);
      UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
      draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
      draw_data->cursor = *cursor;
      draw_data->mark = *mark;
      ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
      mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
      cursor_off = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), str8_prefix(edit_string, cursor->column-1)).x;
      scratch_end(scratch);
    }
    else if((is_focus_active || is_focus_active_disabled) && !(flags & DF_LineEditFlag_CodeContents))
    {
      String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
      F32 total_text_width = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), edit_string).x;
      F32 total_editstr_width = total_text_width - !!(flags & (DF_LineEditFlag_Expander|DF_LineEditFlag_ExpanderSpace|DF_LineEditFlag_ExpanderPlaceholder)) * expander_size_px;
      ui_set_next_pref_width(ui_px(total_editstr_width+ui_top_font_size()*2, 0.f));
      UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc, "###editstr");
      UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
      draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
      draw_data->cursor = *cursor;
      draw_data->mark = *mark;
      ui_box_equip_display_string(editstr_box, edit_string);
      ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
      mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
      cursor_off = f_dim_from_tag_size_string(ui_top_font(), ui_top_font_size(), 0, ui_top_tab_size(), str8_prefix(edit_string, cursor->column-1)).x;
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
df_line_editf(DF_Window *ws, DF_LineEditFlags flags, S32 depth, FuzzyMatchRangeList *matches, TxtPt *cursor, TxtPt *mark, U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out, B32 *expanded_out, String8 pre_edit_value, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Signal sig = df_line_edit(ws, flags, depth, matches, cursor, mark, edit_buffer, edit_buffer_size, edit_string_size_out, expanded_out, pre_edit_value, string);
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

#if !defined(STBI_INCLUDE_STB_IMAGE_H)
# define STB_IMAGE_IMPLEMENTATION
# define STBI_ONLY_PNG
# define STBI_ONLY_BMP
# include "third_party/stb/stb_image.h"
#endif

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
  df_gfx_state->rich_hover_info_next_arena = arena_alloc();
  df_gfx_state->rich_hover_info_current_arena = arena_alloc();
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
    DF_ViewSpecInfoArray tab_view_specs_array = {df_g_gfx_view_rule_tab_view_spec_info_table, ArrayCount(df_g_gfx_view_rule_tab_view_spec_info_table)};
    df_register_view_specs(tab_view_specs_array);
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
    df_gfx_state->icon_texture = r_tex2d_alloc(R_ResourceKind_Static, image_dim, R_Tex2DFormat_RGBA8, image_data);
    
    // rjf: release
    stbi_image_free(image_data);
    scratch_end(scratch);
  }
  
  ProfEnd();
}

internal void
df_gfx_begin_frame(Arena *arena, DF_CmdList *cmds)
{
  ProfBeginFunction();
  arena_clear(df_gfx_state->rich_hover_info_current_arena);
  MemoryCopyStruct(&df_gfx_state->rich_hover_info_current, &df_gfx_state->rich_hover_info_next);
  df_gfx_state->rich_hover_info_current.dbgi_key = di_key_copy(df_gfx_state->rich_hover_info_current_arena, &df_gfx_state->rich_hover_info_current.dbgi_key);
  arena_clear(df_gfx_state->rich_hover_info_next_arena);
  MemoryZeroStruct(&df_gfx_state->rich_hover_info_next);
  
  //- rjf: animate confirmation
  {
    F32 rate = df_setting_val_from_code(0, DF_SettingCode_MenuAnimations).s32 ? 1 - pow_f32(2, (-10.f * df_dt())) : 1.f;
    B32 confirm_open = df_gfx_state->confirm_active;
    df_gfx_state->confirm_t += rate * ((F32)!!confirm_open-df_gfx_state->confirm_t);
    if(abs_f32(df_gfx_state->confirm_t - (F32)!!confirm_open) > 0.005f)
    {
      df_gfx_request_frame();
    }
  }
  
  //- rjf: capture is active? -> keep rendering
  if(ProfIsCapturing() || DEV_telemetry_capture)
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
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProjectData));
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
          DF_Window *originating_window = df_window_from_handle(params.window);
          if(originating_window == 0)
          {
            originating_window = df_gfx_state->first_window;
          }
          OS_Handle preferred_monitor = {0};
          DF_Window *new_ws = df_window_open(v2f32(1280, 720), preferred_monitor, DF_CfgSrc_User);
          if(originating_window)
          {
            MemoryCopy(new_ws->setting_vals, originating_window->setting_vals, sizeof(DF_SettingVal)*DF_SettingCode_COUNT);
          }
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
                df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_WriteProjectData));
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
              arena_release(ws->code_ctx_menu_arena);
              arena_release(ws->hover_eval_arena);
              arena_release(ws->autocomp_lister_params_arena);
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
              p.view = df_handle_from_view(df_selected_tab_from_panel(window->focused_panel));
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Window);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_Panel);
              df_cmd_params_mark_slot(&p, DF_CmdParamSlot_View);
              df_cmd_list_push(arena, cmds, &p, cmd->spec);
            }
          }
        }break;
        
        //- rjf: loading/applying stateful config changes
        case DF_CoreCmdKind_ApplyUserData:
        case DF_CoreCmdKind_ApplyProjectData:
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
            F32 dpi = 0.f;
            DF_SettingVal setting_vals[DF_SettingCode_COUNT] = {0};
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
              DF_CfgNode *dpi_cfg = df_cfg_node_child_from_string(window_node, str8_lit("dpi"), StringMatchFlag_CaseInsensitive);
              String8 dpi_cfg_string = df_string_from_cfg_node_children(scratch.arena, dpi_cfg);
              dpi = f64_from_str8(dpi_cfg_string);
              for(EachEnumVal(DF_SettingCode, code))
              {
                DF_CfgNode *cfg = df_cfg_node_child_from_string(window_node, df_g_setting_code_lower_string_table[code], StringMatchFlag_CaseInsensitive);
                if(cfg != &df_g_nil_cfg_node)
                {
                  S64 val_s64 = 0;
                  try_s64_from_str8_c_rules(cfg->first->string, &val_s64);
                  setting_vals[code].set = 1;
                  setting_vals[code].s32 = (S32)val_s64;
                  setting_vals[code].s32 = clamp_1s32(df_g_setting_code_s32_range_table[code], setting_vals[code].s32);
                }
              }
            }
            
            // rjf: open window
            DF_Window *ws = df_window_open(size, preferred_monitor, window_node->source);
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
                panel->pct_of_parent = 1.f;
              }
              
              // rjf: allocate & insert non-root panels - these will have a numeric string, determining
              // pct of parent
              if(n->flags & DF_CfgNodeFlag_Numeric)
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
                    
                    // rjf: read project path
                    String8 project_path = str8_lit("");
                    {
                      DF_CfgNode *project_cfg_node = df_cfg_node_child_from_string(op, str8_lit("project"), StringMatchFlag_CaseInsensitive);
                      if(project_cfg_node != &df_g_nil_cfg_node)
                      {
                        project_path = path_absolute_dst_from_relative_dst_src(scratch.arena, project_cfg_node->first->string, cfg_folder);
                      }
                    }
                    
                    // rjf: read view query string
                    String8 view_query = str8_lit("");
                    if(view_spec_flags & DF_ViewSpecFlag_CanSerializeQuery)
                    {
                      String8 escaped_query = df_cfg_node_child_from_string(op, str8_lit("query"), StringMatchFlag_CaseInsensitive)->first->string;
                      view_query = df_cfg_raw_from_escaped_string(scratch.arena, escaped_query);
                    }
                    
                    // rjf: read entity path
                    DF_Entity *entity = &df_g_nil_entity;
                    if(view_spec_flags & DF_ViewSpecFlag_CanSerializeEntityPath)
                    {
                      String8 saved_path = df_first_cfg_node_child_from_flags(op, DF_CfgNodeFlag_StringLiteral)->string;
                      String8 saved_path_absolute = path_absolute_dst_from_relative_dst_src(scratch.arena, saved_path, cfg_folder);
                      entity = df_entity_from_path(saved_path_absolute, DF_EntityFromPathFlag_All);
                    }
                    
                    // rjf: set up view
                    df_view_equip_spec(ws, view, view_spec, entity, view_query, op);
                    if(project_path.size != 0)
                    {
                      view->project = df_handle_from_entity(df_entity_from_path(project_path, DF_EntityFromPathFlag_OpenMissing|DF_EntityFromPathFlag_OpenAsNeeded));
                    }
                  }
                  
                  // rjf: insert
                  if(!df_view_is_nil(view))
                  {
                    DF_Entity *current_project = df_entity_from_path(df_cfg_path_from_src(DF_CfgSrc_Project), DF_EntityFromPathFlag_OpenMissing|DF_EntityFromPathFlag_OpenAsNeeded);
                    DF_Entity *view_project = df_entity_from_handle(view->project);
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
          
          //- rjf: reset theme to default
          MemoryCopy(df_gfx_state->cfg_theme_target.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
          MemoryCopy(df_gfx_state->cfg_theme.colors, df_g_theme_preset_colors__default_dark, sizeof(df_g_theme_preset_colors__default_dark));
          
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
          
          //- rjf: apply individual theme colors
          B8 theme_color_hit[DF_ThemeColor_COUNT] = {0};
          DF_CfgVal *colors = df_cfg_val_from_string(table, str8_lit("colors"));
          for(DF_CfgNode *colors_set = colors->first;
              colors_set != &df_g_nil_cfg_node;
              colors_set = colors_set->next)
          {
            for(DF_CfgNode *color = colors_set->first;
                color != &df_g_nil_cfg_node;
                color = color->next)
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
          }
          
          //- rjf: no preset -> autofill all missing colors from the preset with the most similar background
          if(!preset_applied)
          {
            DF_ThemePreset closest_preset = DF_ThemePreset_DefaultDark;
            F32 closest_preset_bg_distance = 100000000;
            for(DF_ThemePreset p = (DF_ThemePreset)0; p < DF_ThemePreset_COUNT; p = (DF_ThemePreset)(p+1))
            {
              Vec4F32 cfg_bg = df_gfx_state->cfg_theme_target.colors[DF_ThemeColor_BaseBackground];
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
          
          //- rjf: apply settings
          B8 setting_codes_hit[DF_SettingCode_COUNT] = {0};
          MemoryZero(&df_gfx_state->cfg_setting_vals[src][0], sizeof(DF_SettingVal)*DF_SettingCode_COUNT);
          for(EachEnumVal(DF_SettingCode, code))
          {
            String8 name = df_g_setting_code_lower_string_table[code];
            DF_CfgVal *code_cfg_val = df_cfg_val_from_string(table, name);
            DF_CfgNode *root_node = code_cfg_val->last;
            if(root_node->source == src)
            {
              DF_CfgNode *val_node = root_node->first;
              S64 val = 0;
              if(try_s64_from_str8_c_rules(val_node->string, &val))
              {
                df_gfx_state->cfg_setting_vals[src][code].set = 1;
                df_gfx_state->cfg_setting_vals[src][code].s32 = (S32)val;
              }
              if(val_node != &df_g_nil_cfg_node)
              {
                setting_codes_hit[code] = 1;
              }
            }
          }
          
          //- rjf: if config applied 0 settings, we need to do some sensible default
          if(src == DF_CfgSrc_User)
          {
            for(EachEnumVal(DF_SettingCode, code))
            {
              if(!setting_codes_hit[code])
              {
                df_gfx_state->cfg_setting_vals[src][code] = df_g_setting_code_default_val_table[code];
              }
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
            if(monitor_dim.x < 1920)
            {
              df_cmd_list_push(arena, cmds, &blank_params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ResetToCompactPanels));
            }
            else
            {
              df_cmd_list_push(arena, cmds, &blank_params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_ResetToDefaultPanels));
            }
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
          
          //- rjf: always ensure that the meta controls have bindings
          if(src == DF_CfgSrc_User)
          {
            struct
            {
              DF_CmdSpec *spec;
              OS_Key fallback_key;
            }
            meta_ctrls[] =
            {
              { df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Edit), OS_Key_F2 },
              { df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Accept), OS_Key_Return },
              { df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Cancel), OS_Key_Esc },
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
    os_sleep_milliseconds(300);
  }
  
  //- rjf: end drag/drop if needed
  if(df_gfx_state->drag_drop_state == DF_DragDropState_Dropping)
  {
    df_gfx_state->drag_drop_state = DF_DragDropState_Null;
    MemoryZeroStruct(&df_g_drag_drop_payload);
  }
  
  //- rjf: clear frame request state
  if(df_gfx_state->num_frames_requested > 0)
  {
    df_gfx_state->num_frames_requested -= 1;
  }
  
  ProfEnd();
}
