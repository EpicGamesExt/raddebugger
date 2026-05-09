// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

////////////////////////////////
//~ rjf: Graphics System Info

typedef struct WM_SystemInfo WM_SystemInfo;
struct WM_SystemInfo
{
  F32 double_click_time;
  F32 caret_blink_time;
  F32 default_refresh_rate;
};

////////////////////////////////
//~ rjf: Window Types

typedef U32 WM_WindowFlags;
enum
{
  WM_WindowFlag_CustomBorder       = (1<<0),
  WM_WindowFlag_UseDefaultPosition = (1<<1),
};

typedef struct WM_Window WM_Window;
struct WM_Window
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: External Window Types

typedef struct WM_ExtWindow WM_ExtWindow;
struct WM_ExtWindow
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Monitor Types

typedef struct WM_Monitor WM_Monitor;
struct WM_Monitor
{
  U64 u64[1];
};

typedef struct WM_MonitorArray WM_MonitorArray;
struct WM_MonitorArray
{
  WM_Monitor *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Cursor Types

typedef enum WM_Cursor
{
  WM_Cursor_Pointer,
  WM_Cursor_IBar,
  WM_Cursor_LeftRight,
  WM_Cursor_UpDown,
  WM_Cursor_DownRight,
  WM_Cursor_UpRight,
  WM_Cursor_UpDownLeftRight,
  WM_Cursor_HandPoint,
  WM_Cursor_Disabled,
  WM_Cursor_COUNT,
}
WM_Cursor;

////////////////////////////////
//~ rjf: Generated Code

#include "window_manager/generated/window_manager.meta.h"

////////////////////////////////
//~ rjf: Event Types

typedef enum WM_EventKind
{
  WM_EventKind_Null,
  WM_EventKind_Press,
  WM_EventKind_Release,
  WM_EventKind_MouseMove,
  WM_EventKind_Text,
  WM_EventKind_Scroll,
  WM_EventKind_WindowLoseFocus,
  WM_EventKind_WindowClose,
  WM_EventKind_FileDrop,
  WM_EventKind_Wakeup,
  WM_EventKind_COUNT
}
WM_EventKind;

typedef U32 WM_Modifiers;
enum
{
  WM_Modifier_Ctrl  = (1<<0),
  WM_Modifier_Shift = (1<<1),
  WM_Modifier_Alt   = (1<<2),
};

typedef struct WM_Event WM_Event;
struct WM_Event
{
  WM_Event *next;
  WM_Event *prev;
  U64 timestamp_us;
  WM_Window window;
  WM_EventKind kind;
  WM_Modifiers modifiers;
  WM_Key key;
  B32 is_repeat;
  B32 right_sided;
  U32 character;
  U32 repeat_count;
  Vec2F32 pos;
  Vec2F32 delta;
  String8List strings;
};

typedef struct WM_EventList WM_EventList;
struct WM_EventList
{
  U64 count;
  WM_Event *first;
  WM_Event *last;
};

////////////////////////////////
//~ rjf: Application-Defined Frame Hook Forward Declaration

internal B32 frame(void);

////////////////////////////////
//~ rjf: Handle Type Helpers

internal WM_Window wm_window_zero(void);
internal B32 wm_window_match(WM_Window a, WM_Window b);
internal B32 wm_monitor_match(WM_Monitor a, WM_Monitor b);

////////////////////////////////
//~ rjf: Event Functions (Helpers, Implemented Once)

internal String8 wm_string_from_event_kind(WM_EventKind kind);
internal String8List wm_string_list_from_modifiers(Arena *arena, WM_Modifiers flags);
internal String8 wm_string_from_modifiers_key(Arena *arena, WM_Modifiers modifiers, WM_Key key);
internal U32 wm_codepoint_from_modifiers_and_key(WM_Modifiers flags, WM_Key key);
internal void wm_eat_event(WM_EventList *events, WM_Event *event);
internal B32 wm_key_press(WM_EventList *events, WM_Window window, WM_Modifiers modifiers, WM_Key key);
internal B32 wm_key_release(WM_EventList *events, WM_Window window, WM_Modifiers modifiers, WM_Key key);
internal B32 wm_text(WM_EventList *events, WM_Window window, U32 character);
internal WM_EventList wm_event_list_copy(Arena *arena, WM_EventList *src);
internal void wm_event_list_concat_in_place(WM_EventList *dst, WM_EventList *to_push);
internal WM_Event *wm_event_list_push_new(Arena *arena, WM_EventList *evts, WM_EventKind kind);

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void wm_init(void);

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal WM_SystemInfo *wm_get_system_info(void);

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void    wm_set_clipboard_text(String8 string);
internal String8 wm_get_clipboard_text(Arena *arena);

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal WM_Window      wm_window_open(Rng2F32 rect, WM_WindowFlags flags, String8 title);
internal void           wm_window_close(WM_Window window);
internal void           wm_window_set_title(WM_Window window, String8 title);
internal void           wm_window_first_paint(WM_Window window);
internal void           wm_window_focus(WM_Window window);
internal B32            wm_window_is_focused(WM_Window window);
internal B32            wm_window_is_fullscreen(WM_Window window);
internal void           wm_window_set_fullscreen(WM_Window window, B32 fullscreen);
internal B32            wm_window_is_maximized(WM_Window window);
internal void           wm_window_set_maximized(WM_Window window, B32 maximized);
internal B32            wm_window_is_minimized(WM_Window window);
internal void           wm_window_set_minimized(WM_Window window, B32 minimized);
internal void           wm_window_bring_to_front(WM_Window window);
internal void           wm_window_set_monitor(WM_Window window, WM_Monitor monitor);
internal void           wm_window_clear_custom_border_data(WM_Window handle);
internal void           wm_window_push_custom_title_bar(WM_Window handle, F32 thickness);
internal void           wm_window_push_custom_edges(WM_Window handle, F32 thickness);
internal void           wm_window_push_custom_title_bar_client_area(WM_Window handle, Rng2F32 rect);
internal Rng2F32        wm_rect_from_window(WM_Window window);
internal Rng2F32        wm_client_rect_from_window(WM_Window window);
internal F32            wm_dpi_from_window(WM_Window window);

////////////////////////////////
//~ rjf: @os_hooks External Windows (Implemented Per-OS)

internal WM_ExtWindow wm_focused_external_window(void);
internal void         wm_focus_external_window(WM_ExtWindow ext_window);

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal WM_MonitorArray wm_push_monitors_array(Arena *arena);
internal WM_Monitor      wm_primary_monitor(void);
internal WM_Monitor      wm_monitor_from_window(WM_Window window);
internal String8         wm_name_from_monitor(Arena *arena, WM_Monitor monitor);
internal Vec2F32         wm_dim_from_monitor(WM_Monitor monitor);
internal F32             wm_dpi_from_monitor(WM_Monitor monitor);

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void           wm_send_wakeup_event(void);
internal WM_EventList   wm_get_events(Arena *arena, B32 wait);
internal WM_Modifiers   wm_get_modifiers(void);
internal B32            wm_key_is_down(WM_Key key);
internal Vec2F32        wm_mouse_from_window(WM_Window window);

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void           wm_set_cursor(WM_Cursor cursor);

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void           wm_graphical_message(B32 error, String8 title, String8 message);
internal String8        wm_graphical_pick_file(Arena *arena, String8 initial_path);

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void           wm_show_in_filesystem_ui(String8 path);
internal void           wm_open_in_browser(String8 url);

#endif // WINDOW_MANAGER_H
