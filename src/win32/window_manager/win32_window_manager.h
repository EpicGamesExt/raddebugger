// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_WINDOW_MANAGER_H
#define WIN32_WINDOW_MANAGER_H

////////////////////////////////
//~ rjf: Includes / Libraries

#include <uxtheme.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#pragma comment(lib, "gdi32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "UxTheme")
#pragma comment(lib, "comdlg32")
#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

////////////////////////////////
//~ rjf: Windows

typedef struct W32_WM_TitleBarClientArea W32_WM_TitleBarClientArea;
struct W32_WM_TitleBarClientArea
{
  W32_WM_TitleBarClientArea *next;
  Rng2F32 rect;
};

typedef struct W32_WM_Window W32_WM_Window;
struct W32_WM_Window
{
  W32_WM_Window *next;
  W32_WM_Window *prev;
  HWND hwnd;
  HDC hdc;
  WINDOWPLACEMENT last_window_placement;
  F32 dpi;
  B32 first_paint_done;
  B32 maximized;
  B32 custom_border;
  F32 custom_border_title_thickness;
  F32 custom_border_edge_thickness;
  B32 custom_border_composition_enabled;
  Arena *paint_arena;
  W32_WM_TitleBarClientArea *first_title_bar_client_area;
  W32_WM_TitleBarClientArea *last_title_bar_client_area;
};

////////////////////////////////
//~ rjf: Monitor Gathering Bundle

typedef struct W32_WM_MonitorGatherNode W32_WM_MonitorGatherNode;
struct W32_WM_MonitorGatherNode
{
  W32_WM_MonitorGatherNode *next;
  WM_Monitor v;
};

typedef struct W32_WM_MonitorGatherBundle W32_WM_MonitorGatherBundle;
struct W32_WM_MonitorGatherBundle
{
  Arena *arena;
  W32_WM_MonitorGatherNode *first_monitor;
  W32_WM_MonitorGatherNode *last_monitor;
  U64 monitor_count;
};

////////////////////////////////
//~ rjf: Global State

typedef struct W32_WM_State W32_WM_State;
struct W32_WM_State
{
  Arena *arena;
  U32 gfx_thread_tid;
  HINSTANCE hInstance;
  HCURSOR hCursor;
  WM_SystemInfo gfx_info;
  W32_WM_Window *first_window;
  W32_WM_Window *last_window;
  W32_WM_Window *free_window;
  WM_Key key_from_vkey_table[256];
};

////////////////////////////////
//~ rjf: Globals

global W32_WM_State *w32_wm_state = 0;
global WM_EventList w32_wm_event_list = {0};
global Arena *w32_wm_event_arena = 0;
global B32 w32_wm_resizing = 0;
global B32 w32_wm_new_window_custom_border = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal Rng2F32 w32_wm_rng2f32_from_rect(RECT rect);

////////////////////////////////
//~ rjf: Windows

internal WM_Window       w32_wm_handle_from_window(W32_WM_Window *window);
internal W32_WM_Window * w32_wm_window_from_handle(WM_Window window);
internal W32_WM_Window * w32_wm_window_from_hwnd(HWND hwnd);
internal HWND            w32_wm_hwnd_from_window(W32_WM_Window *window);
internal W32_WM_Window * w32_wm_window_alloc(void);
internal void            w32_wm_window_release(W32_WM_Window *window);
internal WM_Event *      w32_wm_push_event(WM_EventKind kind, W32_WM_Window *window);
internal WM_Key          w32_wm_os_key_from_vkey(WPARAM vkey);
internal WPARAM          w32_wm_vkey_from_os_key(WM_Key key);
internal LRESULT         w32_wm_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

////////////////////////////////
//~ rjf: Monitors

internal BOOL w32_wm_monitor_gather_enum_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM bundle_ptr);

#endif // WIN32_WINDOW_MANAGER_H
