////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_gfx_init(void)
{
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
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  OS_Handle handle = {1};
  return handle;
}

internal void
os_window_close(OS_Handle window)
{
}

internal void
os_window_first_paint(OS_Handle window)
{
}

internal void
os_window_equip_repaint(OS_Handle window, OS_WindowRepaintFunctionType *repaint, void *user_data)
{
}

internal void
os_window_focus(OS_Handle window)
{
}

internal B32
os_window_is_focused(OS_Handle window)
{
  return 0;
}

internal B32
os_window_is_fullscreen(OS_Handle window)
{
  return 0;
}

internal void
os_window_set_fullscreen(OS_Handle window, B32 fullscreen)
{
}

internal B32
os_window_is_maximized(OS_Handle window)
{
  return 0;
}

internal void
os_window_set_maximized(OS_Handle window, B32 maximized)
{
}

internal void
os_window_minimize(OS_Handle window)
{
}

internal void
os_window_bring_to_front(OS_Handle window)
{
}

internal void
os_window_set_monitor(OS_Handle window, OS_Handle monitor)
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
os_rect_from_window(OS_Handle window)
{
  Rng2F32 rect = r2f32(v2f32(0, 0), v2f32(500, 500));
  return rect;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle window)
{
  Rng2F32 rect = r2f32(v2f32(0, 0), v2f32(500, 500));
  return rect;
}

internal F32
os_dpi_from_window(OS_Handle window)
{
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  OS_HandleArray arr = {0};
  return arr;
}

internal OS_Handle
os_primary_monitor(void)
{
  OS_Handle handle = {1};
  return handle;
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  OS_Handle handle = {1};
  return handle;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  return str8_zero();
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  Vec2F32 v = v2f32(1000, 1000);
  return v;
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
  OS_EventList evts = {0};
  return evts;
}

internal OS_EventFlags
os_get_event_flags(void)
{
  OS_EventFlags f = 0;
  return f;
}

internal B32
os_key_is_down(OS_Key key)
{
  return 0;
}

internal Vec2F32
os_mouse_from_window(OS_Handle window)
{
  return v2f32(0, 0);
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
