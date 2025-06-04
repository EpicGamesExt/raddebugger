// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_LINUX_H
#define OS_GFX_LINUX_H

////////////////////////////////
//~ rjf: Includes

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/sync.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

////////////////////////////////
//~ rjf: Window State

typedef struct OS_LNX_Window OS_LNX_Window;
struct OS_LNX_Window
{
  OS_LNX_Window *next;
  OS_LNX_Window *prev;
  Window window;
  XIC xic;
  XID counter_xid;
  U64 counter_value;
  B32 custom_border;
  F32 custom_border_title_thickness;
  F32 custom_border_edge_thickness;
};

////////////////////////////////
//~ rjf: State Bundle

typedef struct OS_LNX_GfxState OS_LNX_GfxState;
struct OS_LNX_GfxState
{
  Arena *arena;
  Display *display;
  XIM xim;
  OS_LNX_Window *first_window;
  OS_LNX_Window *last_window;
  OS_LNX_Window *free_window;
  Atom wm_delete_window_atom;
  Atom wm_sync_request_atom;
  Atom wm_sync_request_counter_atom;
  Atom wm_motif_hints_atom;
  Atom wm_move_resize_atom;
  Atom wm_state_atom;
  Atom wm_state_hidden_atom;
  Atom wm_state_maximized_vert_atom, wm_state_maximized_horz_atom;
  Cursor cursors[OS_Cursor_COUNT];
  OS_Cursor last_set_cursor;
  OS_GfxInfo gfx_info;
};

////////////////////////////////
//~ rjf: Globals

global OS_LNX_GfxState *os_lnx_gfx_state = 0;

////////////////////////////////
//~ rjf: Helpers

internal OS_LNX_Window *os_lnx_window_from_x11window(Window window);
internal B32 os_lnx_check_x11window_atoms(Window window, Atom properties[], U64 num_properties);

#endif // OS_GFX_LINUX_H
