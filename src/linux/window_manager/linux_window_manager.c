// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal LNX_WM_Window *
lnx_window_from_x11window(Window window)
{
  LNX_WM_Window *result = 0;
  for(LNX_WM_Window *w = lnx_wm_state->first_window; w != 0; w = w->next)
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
wm_init(void)
{
  //- rjf: initialize basics
  Arena *arena = arena_alloc();
  lnx_wm_state = push_array(arena, LNX_WM_State, 1);
  lnx_wm_state->arena = arena;
  lnx_wm_state->display = XOpenDisplay(0);
  
  //- rjf: calculate atoms
  lnx_wm_state->wm_delete_window_atom        = XInternAtom(lnx_wm_state->display, "WM_DELETE_WINDOW", 0);
  lnx_wm_state->wm_sync_request_atom         = XInternAtom(lnx_wm_state->display, "_NET_WM_SYNC_REQUEST", 0);
  lnx_wm_state->wm_sync_request_counter_atom = XInternAtom(lnx_wm_state->display, "_NET_WM_SYNC_REQUEST_COUNTER", 0);
  
  //- rjf: open im
  lnx_wm_state->xim = XOpenIM(lnx_wm_state->display, 0, 0, 0);
  
  //- rjf: fill out gfx info
  lnx_wm_state->gfx_info.double_click_time = 0.5f;
  lnx_wm_state->gfx_info.caret_blink_time = 0.5f;
  lnx_wm_state->gfx_info.default_refresh_rate = 60.f;
  
  //- rjf: fill out cursors
  {
    struct
    {
      WM_Cursor cursor;
      unsigned int id;
    }
    map[] =
    {
      {WM_Cursor_Pointer,         XC_left_ptr},
      {WM_Cursor_IBar,            XC_xterm},
      {WM_Cursor_LeftRight,       XC_sb_h_double_arrow},
      {WM_Cursor_UpDown,          XC_sb_v_double_arrow},
      {WM_Cursor_DownRight,       XC_bottom_right_corner},
      {WM_Cursor_UpRight,         XC_top_right_corner},
      {WM_Cursor_UpDownLeftRight, XC_fleur},
      {WM_Cursor_HandPoint,       XC_hand1},
      {WM_Cursor_Disabled,        XC_X_cursor},
    };
    for EachElement(idx, map)
    {
      lnx_wm_state->cursors[map[idx].cursor] = XCreateFontCursor(lnx_wm_state->display, map[idx].id);
    }
  }

  // create wakeup event for polling
  lnx_wm_state->wakeup_fd = eventfd(0, EFD_CLOEXEC);
  Assert(lnx_wm_state->wakeup_fd > 0);
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal WM_SystemInfo *
wm_get_system_info(void)
{
  return &lnx_wm_state->gfx_info;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
wm_set_clipboard_text(String8 string)
{
  
}

internal String8
wm_get_clipboard_text(Arena *arena)
{
  String8 result = {0};
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal WM_Window
wm_window_open(Rng2F32 rect, WM_WindowFlags flags, String8 title)
{
  Vec2F32 resolution = dim_2f32(rect);
  
  //- rjf: allocate window
  LNX_WM_Window *w = lnx_wm_state->free_window;
  if(w)
  {
    SLLStackPop(lnx_wm_state->free_window);
  }
  else
  {
    w = push_array_no_zero(lnx_wm_state->arena, LNX_WM_Window, 1);
  }
  MemoryZeroStruct(w);
  DLLPushBack(lnx_wm_state->first_window, lnx_wm_state->last_window, w);
  
  //- rjf: create window & equip with x11 info
  w->window = XCreateWindow(lnx_wm_state->display,
                            XDefaultRootWindow(lnx_wm_state->display),
                            0, 0, resolution.x, resolution.y,
                            0,
                            CopyFromParent,
                            InputOutput,
                            CopyFromParent,
                            0,
                            0);
  XSelectInput(lnx_wm_state->display, w->window,
               ExposureMask|
               PointerMotionMask|
               ButtonPressMask|
               ButtonReleaseMask|
               KeyPressMask|
               KeyReleaseMask|
               FocusChangeMask);
  Atom protocols[] =
  {
    lnx_wm_state->wm_delete_window_atom,
    lnx_wm_state->wm_sync_request_atom,
  };
  XSetWMProtocols(lnx_wm_state->display, w->window, protocols, ArrayCount(protocols));
  {
    XSyncValue initial_value;
    XSyncIntToValue(&initial_value, 0);
    w->counter_xid = XSyncCreateCounter(lnx_wm_state->display, initial_value);
  }
  XChangeProperty(lnx_wm_state->display, w->window, lnx_wm_state->wm_sync_request_counter_atom, XA_CARDINAL, 32, PropModeReplace, (U8 *)&w->counter_xid, 1);
  
  //- rjf: create xic
  w->xic = XCreateIC(lnx_wm_state->xim,
                     XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
                     XNClientWindow, w->window,
                     XNFocusWindow, w->window,
                     NULL);
  
  //- rjf: attach name
  Temp scratch = scratch_begin(0, 0);
  String8 title_copy = push_str8_copy(scratch.arena, title);
  XStoreName(lnx_wm_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);
  
  //- rjf: convert to handle & return
  WM_Window handle = {(U64)w};
  return handle;
}

internal void
wm_window_close(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  XDestroyWindow(lnx_wm_state->display, w->window);
}

internal void
wm_window_set_title(WM_Window handle, String8 title)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  Temp scratch = scratch_begin(0, 0);
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  String8 title_copy = push_str8_copy(scratch.arena, title);
  XStoreName(lnx_wm_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);
}

internal void
wm_window_first_paint(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  XMapWindow(lnx_wm_state->display, w->window);
}

internal void
wm_window_focus(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  // 
  // TODO: if the target is lightweight, the debugger may launch it even before the first frame is drawn;
  //       this is a problem because XSetInputFocus is now called before XMapWindow, we need a guard
  //       to prevent an X server error:
  //
  //         X Error of failed request:  BadMatch (invalid parameter attributes)
  //            Major opcode of failed request:  42 (X_SetInputFocus)
  //            Serial number of failed request:  373
  //            Current serial number in output stream:  374
  //        
  XSetInputFocus(lnx_wm_state->display, w->window, RevertToNone, CurrentTime);
}

internal B32
wm_window_is_focused(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return 0;}
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  Window focused_window = 0;
  int revert_to = 0;
  XGetInputFocus(lnx_wm_state->display, &focused_window, &revert_to);
  B32 result = (w->window == focused_window);
  return result;
}

internal B32
wm_window_is_fullscreen(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return 0;}
  // TODO(rjf)
  return 0;
}

internal void
wm_window_set_fullscreen(WM_Window handle, B32 fullscreen)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal B32
wm_window_is_maximized(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return 0;}
  // TODO(rjf)
  return 0;
}

internal void
wm_window_set_maximized(WM_Window handle, B32 maximized)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal B32
wm_window_is_minimized(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return 0;}
  // TODO(rjf)
  return 0;
}

internal void
wm_window_set_minimized(WM_Window handle, B32 minimized)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal void
wm_window_bring_to_front(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal void
wm_window_set_monitor(WM_Window handle, WM_Monitor monitor)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal void
wm_window_clear_custom_border_data(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal void
wm_window_push_custom_title_bar(WM_Window handle, F32 thickness)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal void
wm_window_push_custom_edges(WM_Window handle, F32 thickness)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal void
wm_window_push_custom_title_bar_client_area(WM_Window handle, Rng2F32 rect)
{
  if(wm_window_match(handle, wm_window_zero())) {return;}
  // TODO(rjf)
}

internal Rng2F32
wm_rect_from_window(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return r2f32p(0, 0, 0, 0);}
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  XWindowAttributes atts = {0};
  Status s = XGetWindowAttributes(lnx_wm_state->display, w->window, &atts);
  Rng2F32 result = r2f32p((F32)atts.x, (F32)atts.y, (F32)atts.x + (F32)atts.width, (F32)atts.y + (F32)atts.height);
  return result;
}

internal Rng2F32
wm_client_rect_from_window(WM_Window handle)
{
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  XWindowAttributes atts = {0};
  Status s = XGetWindowAttributes(lnx_wm_state->display, w->window, &atts);
  Rng2F32 result = r2f32p(0, 0, (F32)atts.width, (F32)atts.height);
  return result;
}

internal F32
wm_dpi_from_window(WM_Window handle)
{
  // TODO(rjf)
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks External Windows (Implemented Per-OS)

internal WM_ExtWindow
wm_focused_external_window(void)
{
  WM_ExtWindow result = {0};
  // TODO(rjf)
  return result;
}

internal void
wm_focus_external_window(WM_ExtWindow handle)
{
  // TODO(rjf)
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal WM_MonitorArray
wm_push_monitors_array(Arena *arena)
{
  WM_MonitorArray result = {0};
  // TODO(rjf)
  return result;
}

internal WM_Monitor
wm_primary_monitor(void)
{
  WM_Monitor result = {0};
  // TODO(rjf)
  return result;
}

internal WM_Monitor
wm_monitor_from_window(WM_Window window)
{
  WM_Monitor result = {0};
  // TODO(rjf)
  return result;
}

internal String8
wm_name_from_monitor(Arena *arena, WM_Monitor monitor)
{
  // TODO(rjf)
  return str8_zero();
}

internal Vec2F32
wm_dim_from_monitor(WM_Monitor monitor)
{
  // TODO(rjf)
  return v2f32(0, 0);
}

internal F32
wm_dpi_from_monitor(WM_Monitor monitor)
{
  // TODO(rjf)
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
wm_send_wakeup_event(void)
{
  U64 dummy = 1;
  ssize_t size = LNX_RETRY_ON_EINTR(write(lnx_wm_state->wakeup_fd, &dummy, sizeof(dummy)));
  Assert(size == sizeof(dummy));
}

internal WM_EventList
wm_get_events(Arena *arena, B32 wait)
{
  WM_EventList evts = {0};
  for(;;)
  {
    if(XPending(lnx_wm_state->display) == 0)
    {
      struct pollfd poll_fds[2] =
      {
        { .fd = ConnectionNumber(lnx_wm_state->display), .events = POLLIN },
        { .fd = lnx_wm_state->wakeup_fd,                 .events = POLLIN },
      };
      int timeout = wait && evts.count == 0 ? -1 : 0;
      int poll_status = poll(poll_fds, ArrayCount(poll_fds), timeout);
      Assert(poll_status >= 0);
      if(poll_fds[1].revents & POLLIN)
      {
        U64 dummy = 0;
        read(lnx_wm_state->wakeup_fd, &dummy, sizeof(dummy));
        wait = 0;
      }
    }
    while(XPending(lnx_wm_state->display))
    {
      XEvent evt = {0};
      XNextEvent(lnx_wm_state->display, &evt);
      B32 set_mouse_cursor = 0;
      switch(evt.type)
      {
        default:{}break;
      
      //- rjf: key presses/releases
      case KeyPress:
      case KeyRelease:
      {
        // rjf: determine flags
        WM_Modifiers modifiers = 0;
        if(evt.xkey.state & ShiftMask)   { modifiers |= WM_Modifier_Shift; }
        if(evt.xkey.state & ControlMask) { modifiers |= WM_Modifier_Ctrl; }
        if(evt.xkey.state & Mod1Mask)    { modifiers |= WM_Modifier_Alt; }
        
        // rjf: map keycode -> keysym & codepoint
        LNX_WM_Window *window = lnx_window_from_x11window(evt.xkey.window);
        KeySym keysym = 0;
        U8 text[256] = {0};
        U64 text_size = Xutf8LookupString(window->xic, &evt.xkey, (char *)text, sizeof(text), &keysym, 0);
        
        // rjf: map keysym -> WM_Key
        B32 is_right_sided = 0;
        WM_Key key = WM_Key_Null;
        switch(keysym)
        {
          default:
          {
            if(0){}
            else if(XK_F1 <= keysym && keysym <= XK_F24) { key = (WM_Key)(WM_Key_F1 + (keysym - XK_F1)); }
            else if('0' <= keysym && keysym <= '9')      { key = WM_Key_0 + (keysym-'0'); }
          }break;
          case XK_Escape:{key = WM_Key_Esc;};break;
          case XK_BackSpace:{key = WM_Key_Backspace;}break;
          case XK_Delete:{key = WM_Key_Delete;}break;
          case XK_Return:{key = WM_Key_Return;}break;
          case XK_Pause:{key = WM_Key_Pause;}break;
          case XK_Tab:{key = WM_Key_Tab;}break;
          case XK_Left:{key = WM_Key_Left;}break;
          case XK_Right:{key = WM_Key_Right;}break;
          case XK_Up:{key = WM_Key_Up;}break;
          case XK_Down:{key = WM_Key_Down;}break;
          case XK_Home:{key = WM_Key_Home;}break;
          case XK_End:{key = WM_Key_End;}break;
          case XK_Page_Up:{key = WM_Key_PageUp;}break;
          case XK_Page_Down:{key = WM_Key_PageDown;}break;
          case XK_Alt_L:{ key = WM_Key_Alt; }break;
          case XK_Alt_R:{ key = WM_Key_Alt; is_right_sided = 1;}break;
          case XK_Shift_L:{ key = WM_Key_Shift; }break;
          case XK_Shift_R:{ key = WM_Key_Shift; is_right_sided = 1;}break;
          case XK_Control_L:{ key = WM_Key_Ctrl; }break;
          case XK_Control_R:{ key = WM_Key_Ctrl; is_right_sided = 1;}break;
          case '-':{key = WM_Key_Minus;}break;
          case '=':{key = WM_Key_Equal;}break;
          case '[':{key = WM_Key_LeftBracket;}break;
          case ']':{key = WM_Key_RightBracket;}break;
          case ';':{key = WM_Key_Semicolon;}break;
          case '\'':{key = WM_Key_Quote;}break;
          case '.':{key = WM_Key_Period;}break;
          case ',':{key = WM_Key_Comma;}break;
          case '/':{key = WM_Key_Slash;}break;
          case '\\':{key = WM_Key_BackSlash;}break;
          case '\t':{key = WM_Key_Tab;}break;
          case 'a':case 'A':{key = WM_Key_A;}break;
          case 'b':case 'B':{key = WM_Key_B;}break;
          case 'c':case 'C':{key = WM_Key_C;}break;
          case 'd':case 'D':{key = WM_Key_D;}break;
          case 'e':case 'E':{key = WM_Key_E;}break;
          case 'f':case 'F':{key = WM_Key_F;}break;
          case 'g':case 'G':{key = WM_Key_G;}break;
          case 'h':case 'H':{key = WM_Key_H;}break;
          case 'i':case 'I':{key = WM_Key_I;}break;
          case 'j':case 'J':{key = WM_Key_J;}break;
          case 'k':case 'K':{key = WM_Key_K;}break;
          case 'l':case 'L':{key = WM_Key_L;}break;
          case 'm':case 'M':{key = WM_Key_M;}break;
          case 'n':case 'N':{key = WM_Key_N;}break;
          case 'o':case 'O':{key = WM_Key_O;}break;
          case 'p':case 'P':{key = WM_Key_P;}break;
          case 'q':case 'Q':{key = WM_Key_Q;}break;
          case 'r':case 'R':{key = WM_Key_R;}break;
          case 's':case 'S':{key = WM_Key_S;}break;
          case 't':case 'T':{key = WM_Key_T;}break;
          case 'u':case 'U':{key = WM_Key_U;}break;
          case 'v':case 'V':{key = WM_Key_V;}break;
          case 'w':case 'W':{key = WM_Key_W;}break;
          case 'x':case 'X':{key = WM_Key_X;}break;
          case 'y':case 'Y':{key = WM_Key_Y;}break;
          case 'z':case 'Z':{key = WM_Key_Z;}break;
          case ' ':{key = WM_Key_Space;}break;
        }
        
        // rjf: push text event
        if(evt.type == KeyPress && text_size != 0)
        {
          for(U64 off = 0; off < text_size;)
          {
            UnicodeDecode decode = utf8_decode(text+off, text_size-off);
            if(decode.codepoint != 0 && (decode.codepoint >= 32 || decode.codepoint == '\t'))
            {
              WM_Event *e = wm_event_list_push_new(arena, &evts, WM_EventKind_Text);
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
          WM_Event *e = wm_event_list_push_new(arena, &evts, evt.type == KeyPress ? WM_EventKind_Press : WM_EventKind_Release);
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
        WM_Modifiers modifiers = 0;
        if(evt.xbutton.state & ShiftMask)   { modifiers |= WM_Modifier_Shift; }
        if(evt.xbutton.state & ControlMask) { modifiers |= WM_Modifier_Ctrl; }
        if(evt.xbutton.state & Mod1Mask)    { modifiers |= WM_Modifier_Alt; }
        
        // rjf: map button -> WM_Key
        WM_Key key = WM_Key_Null;
        switch(evt.xbutton.button)
        {
          default:{}break;
          case Button1:{key = WM_Key_LeftMouseButton;}break;
          case Button2:{key = WM_Key_MiddleMouseButton;}break;
          case Button3:{key = WM_Key_RightMouseButton;}break;
        }
        
        // rjf: push event
        LNX_WM_Window *window = lnx_window_from_x11window(evt.xbutton.window);
        if(key != WM_Key_Null)
        {
          WM_Event *e = wm_event_list_push_new(arena, &evts, evt.type == ButtonPress ? WM_EventKind_Press : WM_EventKind_Release);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->key = key;
          e->pos = v2f32((F32)evt.xbutton.x, (F32)evt.xbutton.y);
        }
        else if(evt.xbutton.button == Button4 ||
                evt.xbutton.button == Button5)
        {
          WM_Event *e = wm_event_list_push_new(arena, &evts, WM_EventKind_Scroll);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->delta = v2f32(0, evt.xbutton.button == Button4 ? -1.f : +1.f);
          e->pos = v2f32((F32)evt.xbutton.x, (F32)evt.xbutton.y);
        }
      }break;
      
      //- rjf: mouse motion
      case MotionNotify:
      {
        LNX_WM_Window *window = lnx_window_from_x11window(evt.xclient.window);
        WM_Event *e = wm_event_list_push_new(arena, &evts, WM_EventKind_MouseMove);
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
        LNX_WM_Window *window = lnx_window_from_x11window(evt.xfocus.window);
        WM_Event *e = wm_event_list_push_new(arena, &evts, WM_EventKind_WindowLoseFocus);
        e->window.u64[0] = (U64)window;
      }break;
      
      //- rjf: client messages
      case ClientMessage:
      {
        if((Atom)evt.xclient.data.l[0] == lnx_wm_state->wm_delete_window_atom)
        {
          LNX_WM_Window *window = lnx_window_from_x11window(evt.xclient.window);
          WM_Event *e = wm_event_list_push_new(arena, &evts, WM_EventKind_WindowClose);
          e->window.u64[0] = (U64)window;
        }
        else if((Atom)evt.xclient.data.l[0] == lnx_wm_state->wm_sync_request_atom)
        {
          LNX_WM_Window *window = lnx_window_from_x11window(evt.xclient.window);
          if(window != 0)
          {
            window->counter_value = 0;
            window->counter_value |= evt.xclient.data.l[2];
            window->counter_value |= (evt.xclient.data.l[3] << 32);
            XSyncValue value;
            XSyncIntToValue(&value, window->counter_value);
            XSyncSetCounter(lnx_wm_state->display, window->counter_xid, value);
          }
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
        if(XQueryPointer(lnx_wm_state->display, XDefaultRootWindow(lnx_wm_state->display), &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
        {
          XDefineCursor(lnx_wm_state->display, root_window, lnx_wm_state->cursors[lnx_wm_state->last_set_cursor]);
          XFlush(lnx_wm_state->display);
        }
      }
    }
    if(evts.count > 0 || (wait == 0 && evts.count == 0))
    {
      break;
    }
  }
  return evts;
}

internal WM_Modifiers
wm_get_modifiers(void)
{
  // TODO(rjf)
  return 0;
}

internal B32
wm_key_is_down(WM_Key key)
{
  // TODO(rjf)
  return 0;
}

internal Vec2F32
wm_mouse_from_window(WM_Window handle)
{
  if(wm_window_match(handle, wm_window_zero())) {return v2f32(0, 0);}
  LNX_WM_Window *w = (LNX_WM_Window *)handle.u64[0];
  Vec2F32 result = {0};
  {
    Window root_window = 0;
    Window child_window = 0;
    int root_rel_x = 0;
    int root_rel_y = 0;
    int child_rel_x = 0;
    int child_rel_y = 0;
    unsigned int mask = 0;
    if(XQueryPointer(lnx_wm_state->display, w->window, &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
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
wm_set_cursor(WM_Cursor cursor)
{
  lnx_wm_state->last_set_cursor = cursor;
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
wm_graphical_message(B32 error, String8 title, String8 message)
{
  if(error)
  {
    fprintf(stderr, "[X] ");
  }
  fprintf(stderr, "%.*s\n", str8_varg(title));
  fprintf(stderr, "%.*s\n\n", str8_varg(message));
}

internal String8
wm_graphical_pick_file(Arena *arena, String8 initial_path)
{
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
wm_show_in_filesystem_ui(String8 path)
{
  // TODO(rjf)
}

internal void
wm_open_in_browser(String8 url)
{
  // TODO(rjf)
}
