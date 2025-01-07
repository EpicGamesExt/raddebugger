

typedef Display X11_Display;
typedef Window X11_Window;
// Forward Declares

global X11_Display* x11_server = NULL;
global X11_Window x11_window = 0;
global X11_Window x11_root_window = 0;
global Atom x11_atoms[100];
global Atom x11_atom_tail = 0;
global U32 x11_unhandled_events = 0;
global U32 x11_xdnd_supported_version = 5;
global RROutput* x11_monitors = 0;
global GFX_LinuxMonitor* x11_monitors2 = 0;
global U32 x11_monitors_size = 0;

B32
x11_monitor_update_properties(GFX_LinuxMonitor* out_monitor)
{
  RROutput xrandr_output = out_monitor->handle;
  XRRScreenResources* resources = XRRGetScreenResources(x11_server, x11_root_window);
  XRROutputInfo* props = XRRGetOutputInfo(x11_server, resources, xrandr_output);
  XRRCrtcInfo* x_info = NULL;
  B32 found_crtc = 0;
  for (int i=0; i< props->ncrtc; ++i)
  {
    x_info = XRRGetCrtcInfo(x11_server, resources, props->crtcs[i]);
    /* NOTE(mallchad): If output is set its probably means the output is using
       this config but I'm not always be sure its *this* output usin this crtc
       mode so this should probably be investigated later*/
    if (x_info->noutput > 0) { found_crtc = 1; break; }
    XRRFreeCrtcInfo(x_info);
  }
  if (found_crtc)
  {
    out_monitor->offset.x = x_info->x;
    out_monitor->offset.y = x_info->y;
    out_monitor->offset_mid.x = x_info->x + (x_info->width / 2);
    out_monitor->offset_mid.y = x_info->y + (x_info->height / 2);
    out_monitor->size_px.x = x_info->width;
    out_monitor->size_px.y = x_info->height;
    out_monitor->size_mid_px.x = (x_info->width / 2);
    out_monitor->size_mid_px.y = (x_info->height / 2);
    XRRFreeCrtcInfo(x_info);
  }
  else
  { return 0; }
  out_monitor->size_physical_mm.x = props->mm_width;
  out_monitor->size_physical_mm.y = props->mm_height;
  out_monitor->size_physical_mid_mm.x = (props->mm_width / 2) ;
  out_monitor->size_physical_mid_mm.y = (props->mm_height / 2) ;
  /* out_monitor->refresh_rate = mode->dotClock; */
  /* out_monitor->subpixel_order = */
  /* out_monitor->orientation; */
  out_monitor->landscape = (out_monitor->size_px.x > out_monitor->size_px.y);
  out_monitor->landscape_physical = (props->mm_width > props->mm_height);
  out_monitor->physical = 1;

  XRRFreeOutputInfo(props);
  XRRFreeScreenResources(resources);
  return 1;
}

/// Update window properties from X11
B32
x11_window_update_properties(GFX_LinuxWindow* out)
{
  XWindowAttributes props = {0};

  // Update base properties
  XGetWindowAttributes(x11_server, out->handle, &props);
  out->pos.x = props.x;
  out->pos.y = props.y;
  out->pos_mid.x = props.x + (props.width/2.f);
  out->pos_mid.y = props.y + (props.height/2.f);
  out->size.x = props.width;
  out->size.y = props.height;
  out->border_width = props.border_width;
  out->root_relative_depth = props.depth;

  // Calculate window's current monitor
  B32 horizontal_inside;
  B32 vertical_inside;

  // Monitor Measurements
  S32 right_extent;
  S32 left_extent;
  S32 up_extent;
  S32 down_extent;

  B32 found = 0;
  XRRMonitorInfo* monitor_list = NULL;
  S32 monitor_count = 0;
  monitor_list = XRRGetMonitors(x11_server, x11_root_window, 1, &monitor_count);
  XRRMonitorInfo* one_monitor = XRRGetMonitors(x11_server, out->handle, 1, &monitor_count);
  XRRMonitorInfo x_monitor;
  for (int i=0; i<x11_monitors_size; ++i)
  {
    x_monitor = monitor_list[i];
    left_extent = x_monitor.x;
    right_extent = (x_monitor.x + x_monitor.width);
    up_extent = x_monitor.y;
    down_extent = (x_monitor.y + x_monitor.height);
    horizontal_inside = (out->pos_mid.x > left_extent) && (out->pos_mid.x < right_extent);
    vertical_inside = (out->pos_mid.y > up_extent) && (out->pos_mid.y < down_extent);

    if (horizontal_inside && vertical_inside) { found = 1; break; }
  }
  if (found == 0)
  { out->monitor = NULL; }
  else
  {
    // small leak
    out->monitor = push_array(gfx_lnx_arena, GFX_LinuxMonitor, 1);
    out->monitor->handle = x_monitor.outputs[0];
    x11_monitor_update_properties(out->monitor);
  }
  return 1;
}

B32
x11_graphical_init(GFX_LinuxContext* out)
{
  // Initialize X11
  x11_server = XOpenDisplay(NULL);
  Assert(x11_server != NULL);
  if (x11_server == NULL) { return 0; }
  MemoryZeroArray(x11_atoms);

  out->native_server = (EGLNativeDisplayType)x11_server;

  // Setup atoms
  Atom test_atom = 0;
  for (int i=0; i<ArrayCount(x11_test_atoms); ++i)
  {
    char* x_atom = x11_test_atoms[i];
    test_atom = XInternAtom( x11_server, x_atom, 1 );
    if (test_atom != 0)
    { x11_atoms[ x11_atom_tail++ ] = test_atom; }
    else
    { Assert(0); } // Something is not going to work properly if an intern fails
  }

  // Initialize furthur internal things
  x11_monitors = (RROutput*)push_array_no_zero(gfx_lnx_arena, RROutput, gfx_lnx_max_monitors);
  S32 default_screen = XDefaultScreen(x11_server);
  x11_root_window = RootWindow(x11_server, default_screen);
  return 1;
}

B32
x11_window_open(GFX_LinuxContext* out, OS_Handle* out_handle,
                Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  GFX_LinuxWindow* result = push_array_no_zero(gfx_lnx_arena, GFX_LinuxWindow, 1);
  Assert(flags & OS_WindowFlag_CustomBorder == 0); // This isn't supported yet

  S32 default_screen = XDefaultScreen(x11_server);
  S32 default_depth = DefaultDepth(x11_server, default_screen);
  XVisualInfo visual_info = {0};
  XMatchVisualInfo(x11_server, default_screen, default_depth, TrueColor, &visual_info);
  XSetWindowAttributes window_attributes = {0};
  window_attributes.colormap = XCreateColormap( x11_server,
                                                x11_root_window,
                                                visual_info.visual, AllocNone );
  window_attributes.background_pixmap = None ;
  window_attributes.border_pixel      = 0;
  window_attributes.event_mask        = StructureNotifyMask;
  window_attributes.override_redirect = 1;
  X11_Window new_window = XCreateWindow(x11_server,
                                        x11_root_window,
                                        out->default_window_pos.x, out->default_window_pos.y,
                                        resolution.x, resolution.y, 0,
                                        visual_info.depth,
                                        InputOutput,
                                        visual_info.visual,
                                        CWBorderPixel|CWColormap|CWEventMask,
                                        &window_attributes);
  // Use default window name if none provided, no-name windows are super annoying.
  if (title.size)
  { XStoreName(x11_server, new_window, (char*)title.str); }
  else
  { XStoreName(x11_server, new_window, (char*)out->default_window_name.str); }

  // Subscribe to NET_WM_DELETE events and disable auto XDestroyWindow()
  Atom enabled_protocols[] = { x11_atoms[ X11_Atom_WM_DELETE_WINDOW ] };
  XSetWMProtocols( x11_server, new_window, enabled_protocols, ArrayCount(enabled_protocols) );

  // Subscribe to events
  U32 event_mask = (ClientMessage  | KeyPressMask | KeyReleaseMask | ButtonPressMask |
                    PointerMotionMask | StructureNotifyMask | FocusChangeMask |
                    EnterWindowMask | LeaveWindowMask);
  XSelectInput(x11_server, new_window, event_mask);

  // Make window viewable
  XMapWindow(x11_server, new_window);

  result->handle = (U64)new_window;
  result->name = push_str8_copy(gfx_lnx_arena, title);
  result->pos_target = out->default_window_pos;
  result->size_target = resolution;
  result->wayland_native = 0;
  x11_window_update_properties(result);
  *out_handle = gfx_handle_from_window(result);
  return 0;
}

OS_Key
x11_oskey_from_keycode(U32 keycode)
{
  U32 table_size = sizeof(x11_keysym);
  U32 x_keycode = 0;
  for (int i=0; i<table_size; ++i)
  {
    x_keycode =   x11_keysym[i];
    if (keycode == XKeysymToKeycode(x11_server, x_keycode)) { return i; }
  }
  return OS_Key_Null;
}

B32
x11_get_events(Arena *arena, B32 wait, OS_EventList* out)
{
  U32 pending_events = XPending(x11_server);
  // Force to wait for at least 1 event if 'wait'
  if (wait) { pending_events &= 1; }
  OS_Event* x_node;
  for (U32 i_events=0; i_events < pending_events; ++i_events)
  {
    x_node = push_array_no_zero(arena, OS_Event, 1);
    // Set links
    if (out->first == NULL) { out->first = x_node; };
    if (out->last != NULL)
    {
      out->last->next = x_node;
      x_node->prev = out->last;
    }
    out->last = x_node;
    ++(out->count);

    // Fill out Event
    XEvent event;
    XNextEvent(x11_server, &event);
    GFX_LinuxWindow x_window = {0};
    x_window.handle = event.xany.window;
    x_node->window = gfx_handle_from_window(&x_window);

    // NOTE(mallchad): Don't think wakeup is relevant here
    switch (event.type)
    {
      case KeyPress:
        x_node->timestamp_us = (event.xkey.time / 1000); // ms to us
        x_node->kind = OS_EventKind_Press;
        x_node->key = x11_oskey_from_keycode(event.xkey.keycode);
        // TODO(mallchad): Figure out modifiers, this is gonna be weird
        // x_node->flags = ;
        // TODO:(mallchad): Dunno what to do about this section right now
        x_node->is_repeat = 0;
        x_node->right_sided = 0;
        x_node->repeat_count = 0;
        break;

      case KeyRelease:
        x_node->timestamp_us = (event.xkey.time / 1000); // ms to us
        x_node->kind = OS_EventKind_Release;
        x_node->key = x11_oskey_from_keycode(event.xkey.keycode);
        // TODO(mallchad): Figure out modifiers, this is gonna be weird
        // x_node->flags = ;
        // TODO:(mallchad): Dunno what to do about this section right now
        x_node->is_repeat = 0;
        x_node->right_sided = 0;
        x_node->repeat_count = 0;
        break;

      case MotionNotify:
        /* PointerMotionMask - The client application receives MotionNotify
           events independent of the state of the pointer buttons.
           https://tronche.com/gui/x/xlib/events/keyboard-pointer/keyboard-pointer.html#XButtonEvent
           NOTE(mallchad): It should be constant update rate when using PointerMotionMask. */
        x_node->timestamp_us = (event.xmotion.time / 1000); // ms to us
        x_node->kind = OS_EventKind_MouseMove;
        x_node->pos = vec_2f32(event.xmotion.x_root, event.xmotion.y_root); // root window relative
        break;

      case ButtonPress:
        x_node->timestamp_us = (event.xmotion.time / 1000); // ms to us
        x_node->kind = OS_EventKind_Press;
        U32 button_code = event.xbutton.button;
        x_node->key = x11_mouse[button_code];
        B32 scroll_action = (button_code == X11_Mouse_ScrollUp || button_code == X11_Mouse_ScrollDown);
        if (scroll_action) {}
        /* NOTE(mallchad): Actually I don't know if this is sensible yet. X11
           doesn't support any kind of scroll even or touchpad but libinput
           does. I don't know what the raddebugger API is for it yet so. */
        break;

      case ClientMessage:
      {
        // NOTE(mallchad): Not doing OS_EventKind_Text event. Feels Windows specific
        // The rest of this section is just random 3rd-party spec events
        XClientMessageEvent message = event.xclient;
        Window source_window = message.data.l[0];
        U32 format = message.format;
        B32 format_list = (message.data.l[1] & 1);
        Window xdnd_version = (message.data.l[1] >> 24);

        if ((Atom)(message.data.l[0]) == x11_atoms[ X11_Atom_WM_DELETE_WINDOW ])
        { x_node->kind = OS_EventKind_WindowClose; }
        else if ((Atom)message.message_type == x11_atoms[ X11_Atom_XdndDrop ]
                 && xdnd_version <= x11_xdnd_supported_version)
        {
          x_node->kind = OS_EventKind_FileDrop;
          if (format_list)
          {
            /* https://tronche.com/gui/x/xlib/window-information/XGetWindowProperty.html
               https://www.codeproject.com/Tips/5387630/How-to-handle-X11-Drag-n-Drop-events
               TODO(mallchad): not finishing this because it's long and
               complicated and can't be tested properly for a really long
               time */
            U8* data;
            U32 format_count;
            Atom requested_type = 1; // 1 byte return format
            Atom return_type = 0;
            U32 return_format = 0;
            U64 return_count = 0; // Based on return type
            U64 bytes_read = 0;
            U64 bytes_unread = 0;
            XGetWindowProperty((Display*) x11_server,
                               source_window,
                               x11_atoms[ X11_Atom_XdndTypeList ],
                               0,
                               LONG_MAX,
                               False,
                               requested_type,
                               &return_type,
                               (S32*)&return_format,
                               &return_count,
                               &bytes_unread,
                               &data);
            bytes_read = (return_count * return_type);
            Trap();
            XFree(data);
          }
          if (message.format != 0)
          {
            // Get X11 updated time variable
            // TODO(mallchad): What the heck does the "format" atom mean?
            Time current_time = CurrentTime;
            XConvertSelection((Display*) x11_server,
                              x11_atoms[ X11_Atom_XdndSelection ],
                              format,
                              x11_atoms[ X11_Atom_XdndSelection ],
                              message.window,
                              current_time);
            // XSendEvent(); // Inform sending client the drag'n'drop was receive
          }
          NotImplemented;
        }
        break;
      }

      case FocusIn:
        x_node->kind = OS_EventKind_Null;
        break;

      case FocusOut:
        x_node->kind = OS_EventKind_WindowLoseFocus;
        break;

      default:
        ++x11_unhandled_events;
        x_node->kind = OS_EventKind_Null;
        break;

    }
  }
  return 1;
}

B32
x11_push_monitors_array(Arena* arena, OS_HandleArray* monitor)
{
  (void)monitor;                // NOTE(mallchad): Only really need one instance for now
  RROutput x_monitor = {0};
  S32 x_monitor_count = 0;
  XRRMonitorInfo* monitor_list = NULL;
  MemoryZeroTyped(x11_monitors, gfx_lnx_max_monitors);

  // Get active monitors
  monitor_list = XRRGetMonitors(x11_server, x11_root_window, 1, &x_monitor_count);
  if (monitor_list == NULL) { return 0; }
  x11_monitors_size = x_monitor_count;
  if (x_monitor_count > gfx_lnx_max_monitors)
  {
    gfx_lnx_max_monitors = (2* x_monitor_count); // Greedy allocation to reduce leakage
    x11_monitors = push_array_no_zero(gfx_lnx_arena, RROutput, gfx_lnx_max_monitors);
  }
  GFX_LinuxMonitor* x_monitor2 = NULL;
  for (int i=0; i<x11_monitors_size; ++i)
  {
    x11_monitors2 = push_array(gfx_lnx_arena, GFX_LinuxMonitor, 1);
    x11_monitors[i] = monitor_list[i].outputs[0];
  }
  // Free data
  XRRFreeMonitors(monitor_list);
  return 1;
}

B32
x11_primary_monitor(OS_Handle* monitor)
{
  // small leak
  GFX_LinuxMonitor* _monitor = push_array_no_zero(gfx_lnx_arena, GFX_LinuxMonitor, 1);
  S32 default_screen = XDefaultScreen(x11_server);
  RROutput primary_output = XRRGetOutputPrimary(x11_server, x11_root_window);
  _monitor->handle = primary_output;
  x11_monitor_update_properties(_monitor);
  *monitor = gfx_handle_from_monitor(_monitor);
  return 1;
}

B32
x11_monitor_from_window(OS_Handle window, OS_Handle* out_monitor)
{
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  if (_window->monitor != NULL)
  { *out_monitor = gfx_handle_from_monitor(_window->monitor); }
  else
  { return 0; }
  return 1;
}

B32
x11_name_from_monitor(Arena* arena, OS_Handle monitor, String8* out_name)
{
  GFX_LinuxMonitor* _monitor = gfx_monitor_from_handle(monitor);
  RROutput xrandr_output = _monitor->handle;
  XRRScreenResources* resources = XRRGetScreenResources(x11_server, x11_root_window);
  XRROutputInfo* props = XRRGetOutputInfo(x11_server, resources, xrandr_output);
  String8 monitor_name = str8_cstring(props->name);
  *out_name = push_str8_copy(arena, monitor_name);

  XRRFreeOutputInfo(props);
  XRRFreeScreenResources(resources);
  return 1;
}

B32
x11_dim_from_monitor(OS_Handle monitor, Vec2F32* out_v2)
{
  GFX_LinuxMonitor* _monitor = gfx_monitor_from_handle(monitor);
  if ( x11_monitor_update_properties(_monitor) )
  { *out_v2 = _monitor->size_px; return 0; }
  else
  { return 0; }
}

B32
x11_window_set_monitor(OS_Handle window, OS_Handle monitor)
{
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  GFX_LinuxMonitor* _monitor = gfx_monitor_from_handle(monitor);
  F32 monitor_relative_x = (_window->pos.x - _window->monitor->offset.x  + _monitor->offset.x);
  F32 monitor_relative_y = (_window->pos.y - _window->monitor->offset.y  + _monitor->offset.y);
  XMoveResizeWindow(x11_server, _window->handle,
                    monitor_relative_x,
                    monitor_relative_y,
                    _window->size.y,
                    _window->size.y);
  x11_window_update_properties(_window);
  return 1;
}
