
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
x11_graphical_init(GFX_Context* out)
{
  // Initialize X11
  x11_server = XOpenDisplay(NULL);
  Assert(x11_server != NULL);
  if (x11_server == NULL) { return 0; }
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
  x11_window = XCreateWindow(x11_server,
                             root_window,
                             out->window_pos.x, out->window_pos.y,
                             out->window_size.x, out->window_size.y, 0,
                             visual_info.depth,
                             InputOutput,
                             visual_info.visual,
                             CWBorderPixel|CWColormap|CWEventMask,
                             &window_attributes);

  XStoreName(x11_server, x11_window, (char*)out->window_name);
  // Make window viewable
  XMapWindow(x11_server, x11_window);

  out->native_server = (EGLNativeDisplayType)x11_server;
  out->native_window = (EGLNativeWindowType)x11_window;
  return 1;
}
