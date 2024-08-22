// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_WIN32_H
#define OS_GFX_WIN32_H

////////////////////////////////
//~ rjf: Includes / Libraries

#include <uxtheme.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#pragma comment(lib, "gdi32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "UxTheme")
#pragma comment(lib, "ole32")
#pragma comment(lib, "user32")
#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

////////////////////////////////
//~ rjf: Windows

typedef struct OS_W32_TitleBarClientArea OS_W32_TitleBarClientArea;
struct OS_W32_TitleBarClientArea
{
  OS_W32_TitleBarClientArea *next;
  Rng2F32 rect;
};

typedef struct OS_W32_Window OS_W32_Window;
struct OS_W32_Window
{
  OS_W32_Window *next;
  OS_W32_Window *prev;
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
  OS_W32_TitleBarClientArea *first_title_bar_client_area;
  OS_W32_TitleBarClientArea *last_title_bar_client_area;
};

////////////////////////////////
//~ rjf: Monitor Gathering Bundle

typedef struct OS_W32_MonitorGatherBundle OS_W32_MonitorGatherBundle;
struct OS_W32_MonitorGatherBundle
{
  Arena *arena;
  OS_HandleList *list;
};

////////////////////////////////
//~ rjf: Global State

typedef struct OS_W32_GfxState OS_W32_GfxState;
struct OS_W32_GfxState
{
  Arena *arena;
  U32 gfx_thread_tid;
  HINSTANCE hInstance;
  HCURSOR hCursor;
  OS_GfxInfo gfx_info;
  OS_W32_Window *first_window;
  OS_W32_Window *last_window;
  OS_W32_Window *free_window;
  OS_Key key_from_vkey_table[256];
};

////////////////////////////////
//~ rjf: Globals

global OS_W32_GfxState *os_w32_gfx_state = 0;
global OS_EventList os_w32_event_list = {0};
global Arena *os_w32_event_arena = 0;
B32 os_w32_resizing = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal Rng2F32 os_w32_rng2f32_from_rect(RECT rect);

////////////////////////////////
//~ rjf: Windows

internal OS_Handle       os_w32_handle_from_window(OS_W32_Window *window);
internal OS_W32_Window * os_w32_window_from_handle(OS_Handle window);
internal OS_W32_Window * os_w32_window_from_hwnd(HWND hwnd);
internal HWND            os_w32_hwnd_from_window(OS_W32_Window *window);
internal OS_W32_Window * os_w32_window_alloc(void);
internal void            os_w32_window_release(OS_W32_Window *window);
internal OS_Event *      os_w32_push_event(OS_EventKind kind, OS_W32_Window *window);
internal OS_Key          os_w32_os_key_from_vkey(WPARAM vkey);
internal WPARAM          os_w32_vkey_from_os_key(OS_Key key);
internal LRESULT         os_w32_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

////////////////////////////////
//~ rjf: Monitors

internal BOOL os_w32_monitor_gather_enum_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM bundle_ptr);

#endif // OS_GFX_WIN32_H
