// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

#include <X11/Xlib.h>
internal OS_LNX_Window *
os_lnx_window_from_x11window(Window window)
{
  OS_LNX_Window *result = 0;
  for(OS_LNX_Window *w = os_lnx_gfx_state->first_window; w != 0; w = w->next)
  {
    if(w->window == window)
    {
      result = w;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_gfx_init(void)
{
  //- rjf: initialize basics
  Arena *arena = arena_alloc();
  os_lnx_gfx_state = push_array(arena, OS_LNX_GfxState, 1);
  os_lnx_gfx_state->arena = arena;
  os_lnx_gfx_state->display = XOpenDisplay(0);
  os_lnx_gfx_state->clipboard = str8_zero();
  
  //- rjf: calculate atoms
  os_lnx_gfx_state->wm_delete_window_atom        = XInternAtom(os_lnx_gfx_state->display, "WM_DELETE_WINDOW", 0);
  os_lnx_gfx_state->wm_sync_request_atom         = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_SYNC_REQUEST", 0);
  os_lnx_gfx_state->wm_sync_request_counter_atom = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_SYNC_REQUEST_COUNTER", 0);
  
  //- rjf: open im
  os_lnx_gfx_state->xim = XOpenIM(os_lnx_gfx_state->display, 0, 0, 0);
  
  //- rjf: fill out gfx info
  os_lnx_gfx_state->gfx_info.double_click_time = 0.5f;
  os_lnx_gfx_state->gfx_info.caret_blink_time = 0.5f;
  os_lnx_gfx_state->gfx_info.default_refresh_rate = 60.f;
  
  //- rjf: fill out cursors
  {
    struct
    {
      OS_Cursor cursor;
      unsigned int id;
    }
    map[] =
    {
      {OS_Cursor_Pointer,         XC_left_ptr},
      {OS_Cursor_IBar,            XC_xterm},
      {OS_Cursor_LeftRight,       XC_sb_h_double_arrow},
      {OS_Cursor_UpDown,          XC_sb_v_double_arrow},
      {OS_Cursor_DownRight,       XC_bottom_right_corner},
      {OS_Cursor_UpRight,         XC_top_right_corner},
      {OS_Cursor_UpDownLeftRight, XC_fleur},
      {OS_Cursor_HandPoint,       XC_hand1},
      {OS_Cursor_Disabled,        XC_X_cursor},
    };
    for EachElement(idx, map)
    {
      os_lnx_gfx_state->cursors[map[idx].cursor] = XCreateFontCursor(os_lnx_gfx_state->display, map[idx].id);
    }
  }
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal OS_GfxInfo *
os_get_gfx_info(void)
{
  return &os_lnx_gfx_state->gfx_info;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{
  os_lnx_gfx_state->clipboard = str8(malloc(string.size + 1), string.size);
  MemoryCopy(os_lnx_gfx_state->clipboard.str, string.str, string.size);
  os_lnx_gfx_state->clipboard.str[string.size] = '\0';

  Atom sel = XInternAtom(os_lnx_gfx_state->display, "CLIPBOARD", False);
  Atom utf8 = XInternAtom(os_lnx_gfx_state->display, "UTF8_STRING", False);
  XSetSelectionOwner(os_lnx_gfx_state->display, sel, os_lnx_gfx_state->first_window->window, CurrentTime);
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  String8 result = str8_zero();

  // Return if we already have something in clipboard
  if (os_lnx_gfx_state->clipboard.str) {
    result = push_str8_copy(arena, os_lnx_gfx_state->clipboard);
    return result;
  }

  int screen = DefaultScreen(os_lnx_gfx_state->display);
  Window root = RootWindow(os_lnx_gfx_state->display, screen);

  Atom sel = XInternAtom(os_lnx_gfx_state->display, "CLIPBOARD", False);
  Atom utf8 = XInternAtom(os_lnx_gfx_state->display, "UTF8_STRING", False);

  Window owner = XGetSelectionOwner(os_lnx_gfx_state->display, sel);
  if (owner == None) {
    return result;
  }

  // Dummy Window
  Window tmpWin = XCreateSimpleWindow(os_lnx_gfx_state->display, root, -10, -10, 1, 1, 0, 0, 0);
  // Create A New Window Property
  Atom atomProperty = XInternAtom(os_lnx_gfx_state->display, "RADDBG_CLIPBOARD", False);

  // Request The Clipboard Owner To Convert The Clipboard To UTF-8
  XConvertSelection(os_lnx_gfx_state->display, sel, utf8, atomProperty, tmpWin, CurrentTime);

  XEvent ev;
  XSelectionEvent *sev = NULL;
  for (;;) {
    XNextEvent(os_lnx_gfx_state->display, &ev);
    switch (ev.type) {
      case SelectionNotify: {
        sev = (XSelectionEvent*)&ev.xselection;
        // If Property IS None, Then Conversion Failed
        if (sev->property != None) {
          Atom incr, actual_type_return;
          int actual_format_return;
          unsigned long clipboard_sz, nitems_return;
          unsigned char *prop_ret = NULL;

          // Get Size Of Clipboard String
          XGetWindowProperty(os_lnx_gfx_state->display, tmpWin, atomProperty, 0, 0, False, AnyPropertyType, &actual_type_return, &actual_format_return, &nitems_return, &clipboard_sz, &prop_ret);
          XFree(prop_ret);

          incr = XInternAtom(os_lnx_gfx_state->display, "INCR", False);
          // TODO(pegvin) - Implement INCR Protocol To Allow Large Data Transfers
          if (actual_type_return != incr) {
            XGetWindowProperty(os_lnx_gfx_state->display, tmpWin, atomProperty, 0, clipboard_sz, False, AnyPropertyType, &actual_type_return, &actual_format_return, &nitems_return, &clipboard_sz, &prop_ret);
            String8 tmp = str8_cstring((char*)prop_ret);
            result = push_str8_copy(arena, tmp);
            XFree(prop_ret);
          }
        }
        goto end;
      }break;
    }
  }

end:
  XDeleteProperty(os_lnx_gfx_state->display, tmpWin, atomProperty);
  XDestroyWindow(os_lnx_gfx_state->display, tmpWin);
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Rng2F32 rect, OS_WindowFlags flags, String8 title)
{
  Vec2F32 resolution = dim_2f32(rect);
  
  //- rjf: allocate window
  OS_LNX_Window *w = os_lnx_gfx_state->free_window;
  if(w)
  {
    SLLStackPop(os_lnx_gfx_state->free_window);
  }
  else
  {
    w = push_array_no_zero(os_lnx_gfx_state->arena, OS_LNX_Window, 1);
  }
  MemoryZeroStruct(w);
  DLLPushBack(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
  
  //- rjf: create window & equip with x11 info
  w->window = XCreateWindow(os_lnx_gfx_state->display,
                            XDefaultRootWindow(os_lnx_gfx_state->display),
                            0, 0, resolution.x, resolution.y,
                            0,
                            CopyFromParent,
                            InputOutput,
                            CopyFromParent,
                            0,
                            0);
  XSelectInput(os_lnx_gfx_state->display, w->window,
               ExposureMask|
               PointerMotionMask|
               ButtonPressMask|
               ButtonReleaseMask|
               KeyPressMask|
               KeyReleaseMask|
               FocusChangeMask);
  Atom protocols[] =
  {
    os_lnx_gfx_state->wm_delete_window_atom,
    os_lnx_gfx_state->wm_sync_request_atom,
  };
  XSetWMProtocols(os_lnx_gfx_state->display, w->window, protocols, ArrayCount(protocols));
  {
    XSyncValue initial_value;
    XSyncIntToValue(&initial_value, 0);
    w->counter_xid = XSyncCreateCounter(os_lnx_gfx_state->display, initial_value);
  }
  XChangeProperty(os_lnx_gfx_state->display, w->window, os_lnx_gfx_state->wm_sync_request_counter_atom, XA_CARDINAL, 32, PropModeReplace, (U8 *)&w->counter_xid, 1);
  
  //- rjf: create xic
  w->xic = XCreateIC(os_lnx_gfx_state->xim,
                     XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
                     XNClientWindow, w->window,
                     XNFocusWindow, w->window,
                     NULL);
  
  //- rjf: attach name
  Temp scratch = scratch_begin(0, 0);
  String8 title_copy = push_str8_copy(scratch.arena, title);
  XStoreName(os_lnx_gfx_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);
  
  //- rjf: convert to handle & return
  OS_Handle handle = {(U64)w};
  return handle;
}

internal void
os_window_close(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XDestroyWindow(os_lnx_gfx_state->display, w->window);
}

internal void
os_window_set_title(OS_Handle handle, String8 title)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  Temp scratch = scratch_begin(0, 0);
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  String8 title_copy = push_str8_copy(scratch.arena, title);
  XStoreName(os_lnx_gfx_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);
}

internal void
os_window_first_paint(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XMapWindow(os_lnx_gfx_state->display, w->window);
}

internal void
os_window_focus(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XSetInputFocus(os_lnx_gfx_state->display, w->window, RevertToNone, CurrentTime);
}

internal B32
os_window_is_focused(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  Window focused_window = 0;
  int revert_to = 0;
  XGetInputFocus(os_lnx_gfx_state->display, &focused_window, &revert_to);
  B32 result = (w->window == focused_window);
  return result;
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  // TODO(rjf)
  return 0;
}

internal void
os_window_set_fullscreen(OS_Handle handle, B32 fullscreen)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal B32
os_window_is_maximized(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  // TODO(rjf)
  return 0;
}

internal void
os_window_set_maximized(OS_Handle handle, B32 maximized)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal B32
os_window_is_minimized(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  // TODO(rjf)
  return 0;
}

internal void
os_window_set_minimized(OS_Handle handle, B32 minimized)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal void
os_window_bring_to_front(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal void
os_window_set_monitor(OS_Handle handle, OS_Handle monitor)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal void
os_window_clear_custom_border_data(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal void
os_window_push_custom_edges(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  // TODO(rjf)
}

internal Rng2F32
os_rect_from_window(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return r2f32p(0, 0, 0, 0);}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XWindowAttributes atts = {0};
  Status s = XGetWindowAttributes(os_lnx_gfx_state->display, w->window, &atts);
  Rng2F32 result = r2f32p((F32)atts.x, (F32)atts.y, (F32)atts.x + (F32)atts.width, (F32)atts.y + (F32)atts.height);
  return result;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle handle)
{
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XWindowAttributes atts = {0};
  Status s = XGetWindowAttributes(os_lnx_gfx_state->display, w->window, &atts);
  Rng2F32 result = r2f32p(0, 0, (F32)atts.width, (F32)atts.height);
  return result;
}

internal F32
os_dpi_from_window(OS_Handle handle)
{
  // TODO(rjf)
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  OS_HandleArray result = {0};
  // TODO(rjf)
  return result;
}

internal OS_Handle
os_primary_monitor(void)
{
  OS_Handle result = {0};
  // TODO(rjf)
  return result;
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  OS_Handle result = {0};
  // TODO(rjf)
  return result;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  // TODO(rjf)
  return str8_zero();
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  // TODO(rjf)
  return v2f32(0, 0);
}

internal F32
os_dpi_from_monitor(OS_Handle monitor)
{
  // TODO(rjf)
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
os_send_wakeup_event(void)
{
  // TODO(rjf)
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  OS_EventList evts = {0};
  for(;XPending(os_lnx_gfx_state->display) > 0 || (wait && evts.count == 0);)
  {
    XEvent evt = {0};
    XNextEvent(os_lnx_gfx_state->display, &evt);
    B32 set_mouse_cursor = 0;
    switch(evt.type)
    {
      default:{}break;
      
      //- rjf: key presses/releases
      case KeyPress:
      case KeyRelease:
      {
        // rjf: determine flags
        OS_Modifiers modifiers = 0;
        if(evt.xkey.state & ShiftMask)   { modifiers |= OS_Modifier_Shift; }
        if(evt.xkey.state & ControlMask) { modifiers |= OS_Modifier_Ctrl; }
        if(evt.xkey.state & Mod1Mask)    { modifiers |= OS_Modifier_Alt; }
        
        // rjf: map keycode -> keysym & codepoint
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xkey.window);
        KeySym keysym = 0;
        U8 text[256] = {0};
        U64 text_size = Xutf8LookupString(window->xic, &evt.xkey, (char *)text, sizeof(text), &keysym, 0);
        
        // rjf: map keysym -> OS_Key
        B32 is_right_sided = 0;
        OS_Key key = OS_Key_Null;
        switch(keysym)
        {
          default:
          {
            if(0){}
            else if(XK_F1 <= keysym && keysym <= XK_F24) { key = (OS_Key)(OS_Key_F1 + (keysym - XK_F1)); }
            else if('0' <= keysym && keysym <= '9')      { key = OS_Key_0 + (keysym-'0'); }
          }break;
          case XK_Escape:{key = OS_Key_Esc;};break;
          case XK_BackSpace:{key = OS_Key_Backspace;}break;
          case XK_Delete:{key = OS_Key_Delete;}break;
          case XK_Return:{key = OS_Key_Return;}break;
          case XK_Pause:{key = OS_Key_Pause;}break;
          case XK_Tab:{key = OS_Key_Tab;}break;
          case XK_Left:{key = OS_Key_Left;}break;
          case XK_Right:{key = OS_Key_Right;}break;
          case XK_Up:{key = OS_Key_Up;}break;
          case XK_Down:{key = OS_Key_Down;}break;
          case XK_Home:{key = OS_Key_Home;}break;
          case XK_End:{key = OS_Key_End;}break;
          case XK_Page_Up:{key = OS_Key_PageUp;}break;
          case XK_Page_Down:{key = OS_Key_PageDown;}break;
          case XK_Alt_L:{ key = OS_Key_Alt; }break;
          case XK_Alt_R:{ key = OS_Key_Alt; is_right_sided = 1;}break;
          case XK_Shift_L:{ key = OS_Key_Shift; }break;
          case XK_Shift_R:{ key = OS_Key_Shift; is_right_sided = 1;}break;
          case XK_Control_L:{ key = OS_Key_Ctrl; }break;
          case XK_Control_R:{ key = OS_Key_Ctrl; is_right_sided = 1;}break;
          case '-':{key = OS_Key_Minus;}break;
          case '=':{key = OS_Key_Equal;}break;
          case '[':{key = OS_Key_LeftBracket;}break;
          case ']':{key = OS_Key_RightBracket;}break;
          case ';':{key = OS_Key_Semicolon;}break;
          case '\'':{key = OS_Key_Quote;}break;
          case '.':{key = OS_Key_Period;}break;
          case ',':{key = OS_Key_Comma;}break;
          case '/':{key = OS_Key_Slash;}break;
          case '\\':{key = OS_Key_BackSlash;}break;
          case '\t':{key = OS_Key_Tab;}break;
          case 'a':case 'A':{key = OS_Key_A;}break;
          case 'b':case 'B':{key = OS_Key_B;}break;
          case 'c':case 'C':{key = OS_Key_C;}break;
          case 'd':case 'D':{key = OS_Key_D;}break;
          case 'e':case 'E':{key = OS_Key_E;}break;
          case 'f':case 'F':{key = OS_Key_F;}break;
          case 'g':case 'G':{key = OS_Key_G;}break;
          case 'h':case 'H':{key = OS_Key_H;}break;
          case 'i':case 'I':{key = OS_Key_I;}break;
          case 'j':case 'J':{key = OS_Key_J;}break;
          case 'k':case 'K':{key = OS_Key_K;}break;
          case 'l':case 'L':{key = OS_Key_L;}break;
          case 'm':case 'M':{key = OS_Key_M;}break;
          case 'n':case 'N':{key = OS_Key_N;}break;
          case 'o':case 'O':{key = OS_Key_O;}break;
          case 'p':case 'P':{key = OS_Key_P;}break;
          case 'q':case 'Q':{key = OS_Key_Q;}break;
          case 'r':case 'R':{key = OS_Key_R;}break;
          case 's':case 'S':{key = OS_Key_S;}break;
          case 't':case 'T':{key = OS_Key_T;}break;
          case 'u':case 'U':{key = OS_Key_U;}break;
          case 'v':case 'V':{key = OS_Key_V;}break;
          case 'w':case 'W':{key = OS_Key_W;}break;
          case 'x':case 'X':{key = OS_Key_X;}break;
          case 'y':case 'Y':{key = OS_Key_Y;}break;
          case 'z':case 'Z':{key = OS_Key_Z;}break;
          case ' ':{key = OS_Key_Space;}break;
        }
        
        // rjf: push text event
        if(evt.type == KeyPress && text_size != 0)
        {
          for(U64 off = 0; off < text_size;)
          {
            UnicodeDecode decode = utf8_decode(text+off, text_size-off);
            if(decode.codepoint != 0 && (decode.codepoint >= 32 || decode.codepoint == '\t'))
            {
              OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_Text);
              e->window.u64[0] = (U64)window;
              e->character = decode.codepoint;
            }
            if(decode.inc == 0)
            {
              break;
            }
            off += decode.inc;
          }
        }
        
        // rjf: push key event
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, evt.type == KeyPress ? OS_EventKind_Press : OS_EventKind_Release);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->key = key;
          e->right_sided = is_right_sided;
        }
      }break;
      
      //- rjf: mouse button presses/releases
      case ButtonPress:
      case ButtonRelease:
      {
        // rjf: determine flags
        OS_Modifiers modifiers = 0;
        if(evt.xbutton.state & ShiftMask)   { modifiers |= OS_Modifier_Shift; }
        if(evt.xbutton.state & ControlMask) { modifiers |= OS_Modifier_Ctrl; }
        if(evt.xbutton.state & Mod1Mask)    { modifiers |= OS_Modifier_Alt; }
        
        // rjf: map button -> OS_Key
        OS_Key key = OS_Key_Null;
        switch(evt.xbutton.button)
        {
          default:{}break;
          case Button1:{key = OS_Key_LeftMouseButton;}break;
          case Button2:{key = OS_Key_MiddleMouseButton;}break;
          case Button3:{key = OS_Key_RightMouseButton;}break;
        }
        
        // rjf: push event
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xbutton.window);
        if(key != OS_Key_Null)
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, evt.type == ButtonPress ? OS_EventKind_Press : OS_EventKind_Release);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->key = key;
          e->pos = v2f32((F32)evt.xbutton.x, (F32)evt.xbutton.y);
        }
        else if(evt.xbutton.button == Button4 ||
                evt.xbutton.button == Button5)
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_Scroll);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->delta = v2f32(0, evt.xbutton.button == Button4 ? -1.f : +1.f);
          e->pos = v2f32((F32)evt.xbutton.x, (F32)evt.xbutton.y);
        }
      }break;
      
      //- rjf: mouse motion
      case MotionNotify:
      {
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
        OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_MouseMove);
        e->window.u64[0] = (U64)window;
        e->pos.x = (F32)evt.xmotion.x;
        e->pos.y = (F32)evt.xmotion.y;
        set_mouse_cursor = 1;
      }break;
      
      //- rjf: window focus/unfocus
      case FocusIn:
      {
      }break;
      case FocusOut:
      {
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xfocus.window);
        OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_WindowLoseFocus);
        e->window.u64[0] = (U64)window;
      }break;
      
      //- rjf: client messages
      case ClientMessage:
      {
        if((Atom)evt.xclient.data.l[0] == os_lnx_gfx_state->wm_delete_window_atom)
        {
          OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
          OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_WindowClose);
          e->window.u64[0] = (U64)window;
        }
        else if((Atom)evt.xclient.data.l[0] == os_lnx_gfx_state->wm_sync_request_atom)
        {
          OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
          if(window != 0)
          {
            window->counter_value = 0;
            window->counter_value |= evt.xclient.data.l[2];
            window->counter_value |= (evt.xclient.data.l[3] << 32);
            XSyncValue value;
            XSyncIntToValue(&value, window->counter_value);
            XSyncSetCounter(os_lnx_gfx_state->display, window->counter_xid, value);
          }
        }
      }break;

      //- pegvin: clipboard events
      case SelectionClear:
      {
        free(os_lnx_gfx_state->clipboard.str);
        os_lnx_gfx_state->clipboard = str8_zero();
      }break;
      case SelectionRequest:
      {
        Atom sel = XInternAtom(os_lnx_gfx_state->display, "CLIPBOARD", False);
        Atom utf8 = XInternAtom(os_lnx_gfx_state->display, "UTF8_STRING", False);
        XSelectionRequestEvent *sev = (XSelectionRequestEvent*)&evt.xselectionrequest;
        if (sev->target != utf8 || sev->property == None) { // Might be set to None by "obsolete" clients
          XSelectionEvent reply;
          char *an = XGetAtomName(os_lnx_gfx_state->display, sev->target);
          if (an) {
            XFree(an);
          }

          reply.type = SelectionNotify;
          reply.requestor = sev->requestor;
          reply.selection = sev->selection;
          reply.target = sev->target;
          reply.property = None;
          reply.time = sev->time;

          XSendEvent(os_lnx_gfx_state->display, sev->requestor, True, NoEventMask, (XEvent *)&reply);
        } else {
          // send_utf8(os_lnx_gfx_state->display, sev, utf8);
          XSelectionEvent reply;
          time_t now_tm = time(NULL);
          // char *now = ctime(&now_tm);
          char *an = XGetAtomName(os_lnx_gfx_state->display, sev->property);
          if (an) {
            XFree(an);
          }

          XChangeProperty(os_lnx_gfx_state->display, sev->requestor, sev->property, utf8, 8, PropModeReplace, (unsigned char *)os_lnx_gfx_state->clipboard.str, os_lnx_gfx_state->clipboard.size);

          reply.type = SelectionNotify;
          reply.requestor = sev->requestor;
          reply.selection = sev->selection;
          reply.target = sev->target;
          reply.property = sev->property;
          reply.time = sev->time;

          XSendEvent(os_lnx_gfx_state->display, sev->requestor, True, NoEventMask, (XEvent *)&reply);
        }
      }break;
    }
    if(set_mouse_cursor)
    {
      Window root_window = 0;
      Window child_window = 0;
      int root_rel_x = 0;
      int root_rel_y = 0;
      int child_rel_x = 0;
      int child_rel_y = 0;
      unsigned int mask = 0;
      if(XQueryPointer(os_lnx_gfx_state->display, XDefaultRootWindow(os_lnx_gfx_state->display), &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
      {
        XDefineCursor(os_lnx_gfx_state->display, root_window, os_lnx_gfx_state->cursors[os_lnx_gfx_state->last_set_cursor]);
        XFlush(os_lnx_gfx_state->display);
      }
    }
  }
  return evts;
}

internal OS_Modifiers
os_get_modifiers(void)
{
  // TODO(rjf)
  return 0;
}

internal B32
os_key_is_down(OS_Key key)
{
  // TODO(rjf)
  return 0;
}

internal Vec2F32
os_mouse_from_window(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return v2f32(0, 0);}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  Vec2F32 result = {0};
  {
    Window root_window = 0;
    Window child_window = 0;
    int root_rel_x = 0;
    int root_rel_y = 0;
    int child_rel_x = 0;
    int child_rel_y = 0;
    unsigned int mask = 0;
    if(XQueryPointer(os_lnx_gfx_state->display, w->window, &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
    {
      result.x = child_rel_x;
      result.y = child_rel_y;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  os_lnx_gfx_state->last_set_cursor = cursor;
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  if(error)
  {
    fprintf(stderr, "[X] ");
  }
  fprintf(stderr, "%.*s\n", str8_varg(title));
  fprintf(stderr, "%.*s\n\n", str8_varg(message));
}

internal String8
os_graphical_pick_file(Arena *arena, String8 initial_path)
{
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
{
  // TODO(rjf)
}

internal void
os_open_in_browser(String8 url)
{
  // TODO(rjf)
}
