// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_WINDOW_MANAGER_H
#define LINUX_WINDOW_MANAGER_H

////////////////////////////////
//~ rjf: Includes

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/sync.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <poll.h>
#include <sys/eventfd.h>

////////////////////////////////
//~ rjf: Window State

typedef struct LNX_WM_Window LNX_WM_Window;
struct LNX_WM_Window
{
  LNX_WM_Window *next;
  LNX_WM_Window *prev;
  Window window;
  XIC xic;
  XID counter_xid;
  U64 counter_value;
};

////////////////////////////////
//~ rjf: State Bundle

typedef struct LNX_WM_State LNX_WM_State;
struct LNX_WM_State
{
  Arena *arena;
  Display *display;
  XIM xim;
  LNX_WM_Window *first_window;
  LNX_WM_Window *last_window;
  LNX_WM_Window *free_window;
  Atom wm_delete_window_atom;
  Atom wm_sync_request_atom;
  Atom wm_sync_request_counter_atom;
  Cursor cursors[WM_Cursor_COUNT];
  WM_Cursor last_set_cursor;
  WM_SystemInfo gfx_info;
  int wakeup_fd;
};

////////////////////////////////
//~ rjf: Globals

global LNX_WM_State *lnx_wm_state = 0;

////////////////////////////////
//~ rjf: Helpers

internal LNX_WM_Window *lnx_window_from_x11window(Window window);

#endif // LINUX_WINDOW_MANAGER_H
