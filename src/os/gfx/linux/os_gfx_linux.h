// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_LINUX_H
#define OS_GFX_LINUX_H

////////////////////////////////
//~ dan: Includes

// Corrected Order: GLEW first
#include <GL/glew.h>       // For OpenGL extensions
#include <GL/glx.h>        // For GLX types
#include <GL/glxext.h>     // For GLX extensions

// Add typedef for function pointer
typedef GLXContext (*PFNGLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/sync.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/cursorfont.h>
#include <X11/Xresource.h>
////////////////////////////////
//~ dan: Window State

typedef struct OS_LNX_Window OS_LNX_Window;
struct OS_LNX_Window
{
  OS_LNX_Window *next;
  OS_LNX_Window *prev;
  Window window;
  XID counter_xid;
  U64 counter_value;
  B32 has_focus;
  F32 dpi;
  GLXContext gl_context;
};

////////////////////////////////
//~ dan: State Bundle

typedef struct OS_LNX_GfxState OS_LNX_GfxState;
struct OS_LNX_GfxState
{
  Arena *arena;
  Display *display;
  OS_LNX_Window *first_window;
  OS_LNX_Window *last_window;
  OS_LNX_Window *free_window;
  Atom wm_delete_window_atom;
  Atom wm_sync_request_atom;
  Atom wm_sync_request_counter_atom;
  Atom clipboard_atom;
  Atom targets_atom;
  Atom utf8_string_atom;
  Atom wakeup_atom;
  String8 clipboard_buffer;
  Window clipboard_window;
  Atom clipboard_target_prop_atom;
  String8 last_retrieved_clipboard_text;
  Arena *clipboard_arena;
  B8 key_is_down[OS_Key_COUNT];
  KeyCode keycode_from_oskey[OS_Key_COUNT];
  KeyCode keycode_lshift; KeyCode keycode_rshift;
  KeyCode keycode_lctrl;  KeyCode keycode_rctrl;
  KeyCode keycode_lalt;   KeyCode keycode_ralt;
  Cursor cursors[OS_Cursor_COUNT];
  Cursor current_cursor;
  OS_GfxInfo gfx_info;
  GLXFBConfig fb_config;
  B32 glew_initialized;
  
  // EWMH Atoms
  Atom net_wm_state_atom;
  Atom net_wm_state_maximized_vert_atom;
  Atom net_wm_state_maximized_horz_atom;
  Atom net_wm_state_hidden_atom;
  Atom net_wm_state_fullscreen_atom;
};

////////////////////////////////
//~ dan: Globals

global OS_LNX_GfxState *os_lnx_gfx_state = 0;

#endif // OS_GFX_LINUX_H
