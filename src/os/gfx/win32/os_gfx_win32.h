// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_GRAPHICAL_H
#define WIN32_GRAPHICAL_H

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

////////////////////////////////
//~ rjf: Windows

typedef struct W32_TitleBarClientArea W32_TitleBarClientArea;
struct W32_TitleBarClientArea
{
  W32_TitleBarClientArea *next;
  Rng2F32 rect;
};

typedef struct W32_Window W32_Window;
struct W32_Window
{
  W32_Window *next;
  W32_Window *prev;
  HWND hwnd;
  WINDOWPLACEMENT last_window_placement;
  OS_WindowRepaintFunctionType *repaint;
  void *repaint_user_data;
  F32 dpi;
  B32 first_paint_done;
  B32 maximized;
  B32 custom_border;
  F32 custom_border_title_thickness;
  F32 custom_border_edge_thickness;
  B32 custom_border_composition_enabled;
  Arena *paint_arena;
  W32_TitleBarClientArea *first_title_bar_client_area;
  W32_TitleBarClientArea *last_title_bar_client_area;
};

////////////////////////////////
//~ rjf: Monitor Gathering Bundle

typedef struct W32_MonitorGatherBundle W32_MonitorGatherBundle;
struct W32_MonitorGatherBundle
{
  Arena *arena;
  OS_HandleList *list;
};

////////////////////////////////
//~ rjf: Basic Helpers

internal Rng2F32         w32_base_rect_from_win32_rect(RECT rect);

////////////////////////////////
//~ rjf: Windows

internal OS_Handle       os_window_from_w32_window(W32_Window *window);
internal W32_Window *    w32_window_from_os_window(OS_Handle window);
internal W32_Window *    w32_window_from_hwnd(HWND hwnd);
internal HWND            w32_hwnd_from_window(W32_Window *window);
internal W32_Window *    w32_allocate_window(void);
internal void            w32_free_window(W32_Window *window);
internal OS_Event *      w32_push_event(OS_EventKind kind, W32_Window *window);
internal OS_Key          w32_os_key_from_vkey(WPARAM vkey);
internal WPARAM          w32_vkey_from_os_key(OS_Key key);
internal LRESULT         w32_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

////////////////////////////////
//~ rjf: Monitors

internal BOOL w32_monitor_gather_enum_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM bundle_ptr);

#endif // WIN32_GRAPHICAL_H
