////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
wm_init(void)
{
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal WM_SystemInfo *
wm_get_system_info(void)
{
  local_persist WM_SystemInfo g = {0};
#if defined(WM_STUB_DEFAULT_REFRESH_RATE)
  g.default_refresh_rate = WM_STUB_DEFAULT_REFRESH_RATE;
#endif
  return &g;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
wm_set_clipboard_text(String8 string)
{
}

internal String8
wm_get_clipboard_text(Arena *arena)
{
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal WM_Window
wm_window_open(Rng2F32 rect, WM_WindowFlags flags, String8 title)
{
  WM_Window handle = {1};
  return handle;
}

internal void
wm_window_close(WM_Window window)
{
}

internal void
wm_window_set_title(WM_Window window, String8 title)
{
}

internal void
wm_window_first_paint(WM_Window window)
{
}

internal void
wm_window_focus(WM_Window window)
{
}

internal B32
wm_window_is_focused(WM_Window window)
{
  return 0;
}

internal B32
wm_window_is_fullscreen(WM_Window window)
{
  return 0;
}

internal void
wm_window_set_fullscreen(WM_Window window, B32 fullscreen)
{
}

internal B32
wm_window_is_maximized(WM_Window window)
{
  return 0;
}

internal void
wm_window_set_maximized(WM_Window window, B32 maximized)
{
}

internal B32
wm_window_is_minimized(WM_Window window)
{
  return 0;
}

internal void
wm_window_set_minimized(WM_Window window, B32 minimized)
{
}

internal void
wm_window_bring_to_front(WM_Window window)
{
}

internal void
wm_window_set_monitor(WM_Window window, WM_Monitor monitor)
{
}

internal void
wm_window_clear_custom_border_data(WM_Window handle)
{
}

internal void
wm_window_push_custom_title_bar(WM_Window handle, F32 thickness)
{
}

internal void
wm_window_push_custom_edges(WM_Window handle, F32 thickness)
{
}

internal void
wm_window_push_custom_title_bar_client_area(WM_Window handle, Rng2F32 rect)
{
}

internal Rng2F32
wm_rect_from_window(WM_Window window)
{
  Rng2F32 rect = r2f32(v2f32(0, 0), v2f32(500, 500));
  return rect;
}

internal Rng2F32
wm_client_rect_from_window(WM_Window window)
{
  Rng2F32 rect = r2f32(v2f32(0, 0), v2f32(500, 500));
  return rect;
}

internal F32
wm_dpi_from_window(WM_Window window)
{
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks External Windows (Implemented Per-OS)

internal WM_ExtWindow
wm_focused_external_window(void)
{
  WM_ExtWindow result = {0};
  return result;
}

internal void
wm_focus_external_window(WM_ExtWindow handle)
{
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal WM_MonitorArray
wm_push_monitors_array(Arena *arena)
{
  WM_MonitorArray arr = {0};
  return arr;
}

internal WM_Monitor
wm_primary_monitor(void)
{
  WM_Monitor handle = {1};
  return handle;
}

internal WM_Monitor
wm_monitor_from_window(WM_Window window)
{
  WM_Monitor handle = {1};
  return handle;
}

internal String8
wm_name_from_monitor(Arena *arena, WM_Monitor monitor)
{
  return str8_zero();
}

internal Vec2F32
wm_dim_from_monitor(WM_Monitor monitor)
{
  Vec2F32 v = v2f32(1000, 1000);
  return v;
}

internal F32
wm_dpi_from_monitor(WM_Monitor monitor)
{
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
wm_send_wakeup_event(void)
{
}

internal WM_EventList
wm_get_events(Arena *arena, B32 wait)
{
  WM_EventList evts = {0};
  return evts;
}

internal WM_Modifiers
wm_get_modifiers(void)
{
  WM_Modifiers f = 0;
  return f;
}

internal B32
wm_key_is_down(WM_Key key)
{
  return 0;
}

internal Vec2F32
wm_mouse_from_window(WM_Window window)
{
  return v2f32(0, 0);
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
wm_set_cursor(WM_Cursor cursor)
{
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
wm_graphical_message(B32 error, String8 title, String8 message)
{
}

internal String8
wm_graphical_pick_file(Arena *arena, String8 initial_path)
{
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
wm_show_in_filesystem_ui(String8 path)
{
}

internal void
wm_open_in_browser(String8 url)
{
}
