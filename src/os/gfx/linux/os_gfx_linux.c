// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_gfx_init(void)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal OS_GfxInfo *
os_get_gfx_info(void)
{
  return 0;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{
  
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  
}

internal void
os_window_close(OS_Handle handle)
{
  
}

internal void
os_window_first_paint(OS_Handle window_handle)
{
  
}

internal void
os_window_equip_repaint(OS_Handle handle, OS_WindowRepaintFunctionType *repaint, void *user_data)
{
  
}

internal void
os_window_focus(OS_Handle handle)
{
  
}

internal B32
os_window_is_focused(OS_Handle handle)
{
  
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  
}

internal void
os_window_set_fullscreen(OS_Handle handle, B32 fullscreen)
{
  
}

internal B32
os_window_is_maximized(OS_Handle handle)
{
  
}

internal void
os_window_set_maximized(OS_Handle handle, B32 maximized)
{
  
}

internal void
os_window_minimize(OS_Handle handle)
{
  
}

internal void
os_window_bring_to_front(OS_Handle handle)
{
  
}

internal void
os_window_set_monitor(OS_Handle window_handle, OS_Handle monitor)
{
  
}

internal void
os_window_clear_custom_border_data(OS_Handle handle)
{
  
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  
}

internal void
os_window_push_custom_edges(OS_Handle handle, F32 thickness)
{
  
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  
}

internal Rng2F32
os_rect_from_window(OS_Handle handle)
{
  
}

internal Rng2F32
os_client_rect_from_window(OS_Handle handle)
{
  
}

internal F32
os_dpi_from_window(OS_Handle handle)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  
}

internal OS_Handle
os_primary_monitor(void)
{
  
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
os_send_wakeup_event(void)
{
  
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  
}

internal OS_EventFlags
os_get_event_flags(void)
{
  
}

internal B32
os_key_is_down(OS_Key key)
{
  
}

internal Vec2F32
os_mouse_from_window(OS_Handle handle)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
{
  
}
