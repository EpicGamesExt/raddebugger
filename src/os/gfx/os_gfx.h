// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GRAPHICAL_H
#define OS_GRAPHICAL_H

////////////////////////////////
//~ rjf: Graphics System Info

typedef struct OS_GfxInfo OS_GfxInfo;
struct OS_GfxInfo
{
  F32 double_click_time;
  F32 caret_blink_time;
  F32 default_refresh_rate;
};

////////////////////////////////
//~ rjf: Window Types

typedef U32 OS_WindowFlags;
enum
{
  OS_WindowFlag_CustomBorder = (1<<0),
};

typedef void OS_WindowRepaintFunctionType(OS_Handle window, void *user_data);

////////////////////////////////
//~ rjf: Cursor Types

typedef enum OS_Cursor
{
  OS_Cursor_Pointer,
  OS_Cursor_IBar,
  OS_Cursor_LeftRight,
  OS_Cursor_UpDown,
  OS_Cursor_DownRight,
  OS_Cursor_UpRight,
  OS_Cursor_UpDownLeftRight,
  OS_Cursor_HandPoint,
  OS_Cursor_Disabled,
  OS_Cursor_COUNT,
}
OS_Cursor;

////////////////////////////////
//~ rjf: Generated Code

#include "os/gfx/generated/os_gfx.meta.h"

////////////////////////////////
//~ rjf: Event Types

typedef enum OS_EventKind
{
  OS_EventKind_Null,
  OS_EventKind_Press,
  OS_EventKind_Release,
  OS_EventKind_MouseMove,
  OS_EventKind_Text,
  OS_EventKind_Scroll,
  OS_EventKind_WindowLoseFocus,
  OS_EventKind_WindowClose,
  OS_EventKind_FileDrop,
  OS_EventKind_Wakeup,
  OS_EventKind_COUNT
}
OS_EventKind;

typedef U32 OS_EventFlags;
enum
{
  OS_EventFlag_Ctrl  = (1<<0),
  OS_EventFlag_Shift = (1<<1),
  OS_EventFlag_Alt   = (1<<2),
};

typedef struct OS_Event OS_Event;
struct OS_Event
{
  OS_Event *next;
  OS_Event *prev;
  U64 timestamp_us;
  OS_Handle window;
  OS_EventKind kind;
  OS_EventFlags flags;
  OS_Key key;
  B32 is_repeat;
  B32 right_sided;
  U32 character;
  U32 repeat_count;
  Vec2F32 pos;
  Vec2F32 delta;
  String8List strings;
};

typedef struct OS_EventList OS_EventList;
struct OS_EventList
{
  U64 count;
  OS_Event *first;
  OS_Event *last;
};

////////////////////////////////
//~ rjf: Event Functions (Helpers, Implemented Once)

internal String8List os_string_list_from_event_flags(Arena *arena, OS_EventFlags flags);
internal U32 os_codepoint_from_event_flags_and_key(OS_EventFlags flags, OS_Key key);
internal void os_eat_event(OS_EventList *events, OS_Event *event);
internal B32  os_key_press(OS_EventList *events, OS_Handle window, OS_EventFlags flags, OS_Key key);
internal B32  os_key_release(OS_EventList *events, OS_Handle window, OS_EventFlags flags, OS_Key key);
internal B32  os_text(OS_EventList *events, OS_Handle window, U32 character);
internal OS_EventList os_event_list_copy(Arena *arena, OS_EventList *src);
internal void os_event_list_concat_in_place(OS_EventList *dst, OS_EventList *to_push);

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void os_gfx_init(void);

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal OS_GfxInfo *os_get_gfx_info(void);

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void    os_set_clipboard_text(String8 string);
internal String8 os_get_clipboard_text(Arena *arena);

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle      os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title);
internal void           os_window_close(OS_Handle window);
internal void           os_window_first_paint(OS_Handle window);
internal void           os_window_equip_repaint(OS_Handle window, OS_WindowRepaintFunctionType *repaint, void *user_data);
internal void           os_window_focus(OS_Handle window);
internal B32            os_window_is_focused(OS_Handle window);
internal B32            os_window_is_fullscreen(OS_Handle window);
internal void           os_window_set_fullscreen(OS_Handle window, B32 fullscreen);
internal B32            os_window_is_maximized(OS_Handle window);
internal void           os_window_set_maximized(OS_Handle window, B32 maximized);
internal void           os_window_minimize(OS_Handle window);
internal void           os_window_bring_to_front(OS_Handle window);
internal void           os_window_set_monitor(OS_Handle window, OS_Handle monitor);
internal void           os_window_clear_custom_border_data(OS_Handle handle);
internal void           os_window_push_custom_title_bar(OS_Handle handle, F32 thickness);
internal void           os_window_push_custom_edges(OS_Handle handle, F32 thickness);
internal void           os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect);
internal Rng2F32        os_rect_from_window(OS_Handle window);
internal Rng2F32        os_client_rect_from_window(OS_Handle window);
internal F32            os_dpi_from_window(OS_Handle window);

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray os_push_monitors_array(Arena *arena);
internal OS_Handle      os_primary_monitor(void);
internal OS_Handle      os_monitor_from_window(OS_Handle window);
internal String8        os_name_from_monitor(Arena *arena, OS_Handle monitor);
internal Vec2F32        os_dim_from_monitor(OS_Handle monitor);

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void           os_send_wakeup_event(void);
internal OS_EventList   os_get_events(Arena *arena, B32 wait);
internal OS_EventFlags  os_get_event_flags(void);
internal B32            os_key_is_down(OS_Key key);
internal Vec2F32        os_mouse_from_window(OS_Handle window);

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void           os_set_cursor(OS_Cursor cursor);

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void           os_graphical_message(B32 error, String8 title, String8 message);

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void           os_show_in_filesystem_ui(String8 path);

#endif // OS_GRAPHICAL_H
