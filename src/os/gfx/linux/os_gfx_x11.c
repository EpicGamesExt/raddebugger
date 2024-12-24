
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <GL/gl.h>

typedef Display X11_Display;
typedef Window X11_Window;
// Forward Declares

global X11_Display* x11_server = NULL;
global X11_Window x11_window = 0;

B32
x11_graphical_init(GFX_LinuxContext* out)
{
  // Initialize X11
  x11_server = XOpenDisplay(NULL);
  Assert(x11_server != NULL);
  if (x11_server == NULL) { return 0; }

  out->native_server = (EGLNativeDisplayType)x11_server;
  return 1;
}

B32
x11_window_open(GFX_LinuxContext* out, OS_Handle* out_handle,
                Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  GFX_LinuxWindow* result = push_array_no_zero(gfx_lnx_arena, GFX_LinuxWindow, 1);

  S32 default_screen = XDefaultScreen(x11_server);
  S32 default_depth = DefaultDepth(x11_server, default_screen);
  XVisualInfo visual_info = {0};
  XMatchVisualInfo(x11_server, default_screen, default_depth, TrueColor, &visual_info);
  X11_Window root_window = RootWindow(x11_server, default_screen);
  XSetWindowAttributes window_attributes = {0};
  window_attributes.colormap = XCreateColormap( x11_server,
                                                root_window,
                                                visual_info.visual, AllocNone );
  window_attributes.background_pixmap = None ;
  window_attributes.border_pixel      = 0;
  window_attributes.event_mask        = StructureNotifyMask;
  window_attributes.override_redirect = 1;
  X11_Window new_window = XCreateWindow(x11_server,
                                        root_window,
                                        out->default_window_pos.x, out->default_window_pos.y,
                                        resolution.x, resolution.y, 0,
                                        visual_info.depth,
                                        InputOutput,
                                        visual_info.visual,
                                        CWBorderPixel|CWColormap|CWEventMask,
                                        &window_attributes);
  // Use default window name if none provided, no-name windows are super annoying.
  if (title.size)
  { XStoreName(x11_server, x11_window, (char*)title.str); }
  else
  { XStoreName(x11_server, x11_window, (char*)out->window_name); }
  // Make window viewable
  XMapWindow(x11_server, x11_window);

  result->handle = (U64)new_window;
  result->window_name = push_str8_copy(gfx_lnx_arena, title);
  result->start_pos = out->default_window_pos;
  result->pos = result->start_pos;
  result->size = resolution;
  result->root_relative_depth = 0;
  result->wayland_native = 0;
  *out_handle = gfx_handle_from_window(result);
  return 0;
}
