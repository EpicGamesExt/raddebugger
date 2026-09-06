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
//~ rjf: X definitions

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9   /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10   /* move via keyboard */
#define _NET_WM_MOVERESIZE_CANCEL           11   /* cancel operation */

////////////////////////////////
//~ rjf: Window State

typedef struct LNX_WM_WindowClientArea LNX_WM_WindowClientArea;
struct LNX_WM_WindowClientArea
{
  LNX_WM_WindowClientArea *next;
  Rng2F32 rect;
};

typedef struct LNX_WM_Window LNX_WM_Window;
struct LNX_WM_Window
{
  LNX_WM_Window *next;
  LNX_WM_Window *prev;
  Window window;
  XIC xic;
  XID counter_xid;
  U64 counter_value;
  B32 resize_draw;
  Rng2F32 last_synced_rect;
  B32 pending_focus;
  B32 waiting_for_resize;
  B32 custom_border;
  F32 custom_title_bar_thickness;
  F32 custom_edge_thickness;
  LNX_WM_WindowClientArea *first_client_area;
  LNX_WM_WindowClientArea *last_client_area;
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
  Visual *window_visual;
  int window_depth;
  Colormap window_colormap;
  LNX_WM_WindowClientArea *free_client_area;
  long *icon_image_data;
  U64 icon_image_data_count;
};

////////////////////////////////
//~ rjf: Generated Code

#include "generated/linux_window_manager.meta.h"

////////////////////////////////
//~ rjf: Globals

global LNX_WM_State *lnx_wm_state = 0;

////////////////////////////////
//~ rjf: Helpers

internal LNX_WM_Window *lnx_window_from_x11window(Window window);
internal int lnx_moveresize_code_from_pos(LNX_WM_Window *window, Vec2F32 pos, B32 *out_is_in_client_area);
internal KeySym lnx_wm_keysym_from_key(WM_Key key);
internal WM_Key lnx_wm_key_from_keysym(KeySym ks, B32 *out_is_right_sided);

#endif // LINUX_WINDOW_MANAGER_H
