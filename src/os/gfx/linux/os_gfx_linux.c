
global Arena* gfx_lnx_arena = NULL;
global U32 gfx_lnx_max_monitors = 6;
global OS_EventFlags gfx_lnx_modifier_state = 0;
global GFX_LinuxWindowList gfx_lnx_windows = {0};
global GFX_LinuxMonitorArray gfx_lnx_monitors = {0};
global GFX_LinuxMonitor* gfx_lnx_primary_monitor = {0};
global GFX_LinuxMonitor gfx_lnx_monitor_stub = {0};

/// Determines if wayland pathway should be used by default. We have seperate
/// pathway so recompilation isn't necesscary
global B32 gfx_lnx_wayland_preferred = 0;
global B32 gfx_lnx_wayland_disabled = 1;
/// Caps the amount of events that can be process in one collection run
global U32 gfx_lnx_event_limit = 5000;

global S32 gfx_egl_version_major = 0;
global S32 gfx_egl_version_minor = 0;
global String8 gfx_default_window_name = {0};
global GFX_LinuxContext gfx_context = {0};

global EGLContext gfx_egl_context = NULL;
global EGLDisplay gfx_egl_display = NULL;
global EGLSurface gfx_egl_draw_surface = NULL;
global EGLSurface gfx_egl_read_surface = NULL;
global S32 num_config;
global S32 gfx_egl_config[] = {
  EGL_SURFACE_TYPE,
  EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,
  EGL_OPENGL_BIT,
  EGL_NONE
};
EGLConfig gfx_egl_config_available[10];
global S32 gfx_egl_config_available_size = 0;
global S32 gfx_egl_context_config[] = {
  EGL_CONTEXT_CLIENT_VERSION,
  2,
  EGL_NONE
};


////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

/* NOTE(mallchad): The Linux graphic stack is messy and born out of this was
 * EGL, yet another khronos API. To me, this seems to solve no end user
 * problem, in fact it is actually more or less more work than using Xlib/xcb
 * and Wayland backends directly. However, they claim, EGL allows them to
 * solve backend technical problems with refresh orchestration, surface
 * resource management, graphics acceleration orchestration and the like.
 *
 * I have no idea the validity of this. And I would love to know the validty of
 * this, because the graphisc bugs on Linux are absurd and very few competent
 * people seems to know where exactly they come from. but Wayland *REQUIRES*
 * it. So be it.
 */

GFX_LinuxMonitor*
gfx_monitor_from_id(U64 id)
{
  GFX_LinuxMonitor* x_monitor = NULL;
  for (int i=0; i < gfx_lnx_monitors.head_size; ++i)
  {
    x_monitor = (gfx_lnx_monitors.data + i);
    if (x_monitor->id == id) { return x_monitor; }
  }
  return &gfx_lnx_monitor_stub;
}

GFX_LinuxMonitor*
gfx_monitor_from_handle(OS_Handle monitor)
{
  return gfx_monitor_from_id(*monitor.u64);
}
OS_Handle gfx_handle_from_monitor(GFX_LinuxMonitor* monitor)
{
  OS_Handle result = {0};
  *(result.u64) = monitor->id;
  return result;
}

GFX_LinuxWindow*
gfx_window_from_handle(OS_Handle context)
{
  return (GFX_LinuxWindow*)PtrFromInt(*context.u64);
}

OS_Handle
gfx_handle_from_window(GFX_LinuxWindow* window)
{
  OS_Handle result = {0};
  *result.u64 = IntFromPtr(window);
  return result;
}

internal void
os_graphical_init(void)
{
  // Allocate and setup basic internal stuff
  gfx_lnx_arena = arena_alloc();

  gfx_context.default_window_name = gfx_default_window_name;
  gfx_context.default_window_size = vec_2f32(500, 500);
  gfx_context.default_window_size = gfx_context.default_window_size;
  gfx_context.default_window_pos = vec_2f32(500, 500);
  gfx_context.default_window_pos = gfx_context.default_window_pos;
  gfx_default_window_name = str8_lit("raddebugger");

  ArrayAllocate(&gfx_lnx_monitors, gfx_lnx_arena, gfx_lnx_max_monitors);
  ArrayAllocate(&gfx_lnx_monitors_active, gfx_lnx_arena, gfx_lnx_max_monitors);

  B32 init_result = 0;
  if (gfx_lnx_wayland_disabled)
  { init_result = x11_graphical_init(&gfx_context); }
  else
  { init_result = wayland_graphical_init(&gfx_context); }
  Assert(init_result);

  // Initialize EGL - Windowing Agnostic
  gfx_egl_display = eglGetDisplay(gfx_context.native_server);
  // Generate a stubby window on fail
  if (gfx_egl_display == EGL_NO_DISPLAY)
  { gfx_egl_display = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY); }
  Assert(gfx_egl_display != EGL_NO_DISPLAY);

  // NOTE: EGL_TRUE (1) if success, otherwise EGL_FALSE (0) or error code
  B32 egl_initial_result = eglInitialize(gfx_egl_display, &gfx_egl_version_minor,
                                         &gfx_egl_version_major);
  B32 egl_config_result = eglChooseConfig(gfx_egl_display, gfx_egl_config, gfx_egl_config_available,
                                          10, &gfx_egl_config_available_size);
  B32 egl_bind_result = eglBindAPI(EGL_OPENGL_API);
  Assert(gfx_egl_config_available_size > 0);
  Assert(egl_initial_result == 1 && egl_config_result == 1 && egl_bind_result == 1);
  EGLConfig select_config = gfx_egl_config_available[0];
  gfx_egl_context = eglCreateContext(gfx_egl_display, select_config,
                                     EGL_NO_CONTEXT, gfx_egl_context_config );
  Assert(gfx_egl_context != EGL_NO_CONTEXT);
}


////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{

NotImplemented;}

internal String8
os_get_clipboard_text(Arena *arena)
{
  String8 result = {0};
  return result;
NotImplemented;}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  // TODO(mallchad): Figure out default window placement
  OS_Handle result = {0};
  B32 success = 0;
  if (gfx_lnx_wayland_disabled)
  { x11_window_open(&gfx_context, &result, resolution, flags, title); }
  else
  { wayland_window_open(&gfx_context, &result, resolution, flags, title); }
  GFX_LinuxWindow* window = gfx_window_from_handle(result);
  DLLPushBack(gfx_lnx_windows.first, gfx_lnx_windows.last, window);

  window->first_surface = eglCreateWindowSurface(gfx_egl_display, gfx_egl_config_available[0],
                                                 (EGLNativeWindowType)window->handle, NULL);
  return result;
}
internal void
os_window_close(OS_Handle window)
{

NotImplemented;}

internal void
os_window_first_paint(OS_Handle window)
{
  // Nothing to do on first paint yet
  NoOp;
}

internal void
os_window_equip_repaint(OS_Handle window, OS_WindowRepaintFunctionType *repaint,  void *user_data)
{
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  Vec4F32 dark_magenta = vec_4f32( 0.2f, 0.f, 0.2f, 1.0f );

  eglMakeCurrent(gfx_egl_display, _window->first_surface, _window->first_surface, gfx_egl_context);
  /* glBindFramebuffer(GL_FRAMEBUFFER, 0); */
  glClearColor(dark_magenta.x, dark_magenta.y, dark_magenta.z, dark_magenta.w);
  glClear( GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
  S32 swap_result = eglSwapBuffers(gfx_egl_display, _window->first_surface);
}

internal void
os_window_focus(OS_Handle window)
{

NotImplemented;}

internal B32
os_window_is_focused(OS_Handle window)
{
  return 0;
NotImplemented;}

internal B32
os_window_is_fullscreen(OS_Handle window)
{
  return 0;
NotImplemented;}

internal void
os_window_set_fullscreen(OS_Handle window, B32 fullscreen)
{

NotImplemented;}

internal B32
os_window_is_maximized(OS_Handle window)
{
  B32 result = 0;
  if (gfx_lnx_wayland_disabled)
  { result = x11_window_is_maximized(window); }
  else
  { result = wayland_window_is_maximized(window); }
  return result;
}

internal void
os_window_set_maximized(OS_Handle window, B32 maximized)
{
  if (gfx_lnx_wayland_disabled)
  { x11_window_set_maximized(window, maximized); }
  else
  { wayland_window_set_maximized(window, maximized); }
}

internal void
os_window_minimize(OS_Handle window)
{
NotImplemented;
}

internal void
os_window_bring_to_front(OS_Handle window)
{
NotImplemented;
}

internal void
os_window_set_monitor(OS_Handle window, OS_Handle monitor)
{
  if (gfx_lnx_wayland_disabled)
  { x11_window_set_monitor(window, monitor); }
  else
  { wayland_window_set_monitor(window, monitor); }
}

internal void
os_window_clear_custom_border_data(OS_Handle handle)
{
  // NOTE(mallchad): No practical use yet4
  NoOp;
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  /* NOTE(mallchad): No standard on linux, check back here later
     https://gitlab.freedesktop.org/wayland/wayland-protocols/-/tree/main/unstable/xdg-decoration?ref_type=heads */
  NoOp;
}

internal void
os_window_push_custom_edges(OS_Handle window, F32 thickness)
{
  if (gfx_lnx_wayland_disabled)
  { x11_window_push_custom_edges(window, thickness); }
  else
  { wayland_window_push_custom_edges(window, thickness); }
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  /* NOTE(mallchad): I have no idea what this is supposed to be. I'm assuming
     it's a Windows specific API because there's no generic reasoning for this
     besides being an actual full implimentation for window setup, just not an
     OS abstraction. So I'm not adding it. */
  NoOp;
}

internal Rng2F32
os_rect_from_window(OS_Handle window)
{
  Rng2F32 result = {0};
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  result.min = _window->pos;
  result.max = add_2f32(_window->pos, _window->size);
  return result;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle window)
{
  Rng2F32 result = {0};
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  result.max = _window->size;
  return result;
}

internal F32
os_dpi_from_window(OS_Handle window)
{
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  Assert(_window->monitor->dpi > 0.001 && _window->monitor->dpi < 1000);
  return (_window->monitor->dpi);
}


////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  OS_HandleArray result = {0};
  if (gfx_lnx_wayland_disabled)
  { x11_push_monitors_array(arena, &result); }
  else
  { wayland_push_monitors_array(arena, &result); }
  return result;
}

internal OS_Handle
os_primary_monitor(void)
{
  OS_Handle result = {0};
  x11_primary_monitor(&result);
  return result;
  NotImplemented;}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  OS_Handle result = {0};
  if (gfx_lnx_wayland_disabled)
  { x11_monitor_from_window(window, &result); }
  else
  { wayland_monitor_from_window(window, &result); }
  return result;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  String8 result = {0};
  if (gfx_lnx_wayland_disabled)
  { x11_name_from_monitor(arena, monitor, &result); }
  else
  { wayland_name_from_monitor(arena, monitor, &result); }
  return result;
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  Vec2F32 result = {0};
  if (gfx_lnx_wayland_disabled)
  { x11_dim_from_monitor(monitor, &result); }
  else
  { x11_dim_from_monitor(monitor, &result); }
  return result;
}


////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

/* NOTE(mallchad): No idea what this is supposed to mean or be for it looks like
   a dead pathway in this branch anyway so I'm just going to leave it. */
internal void
os_send_wakeup_event(void)
{
  return;
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  OS_EventList result = {0};
  if (gfx_lnx_wayland_disabled)
  { x11_get_events(arena, wait, &result); }
  else
  { wayland_get_events(arena, wait, &result); }
  return result;
}

internal OS_EventFlags
os_get_event_flags(void)
{
  return gfx_lnx_modifier_state;
}

internal B32
os_key_is_down(OS_Key key)
{
  NotImplemented;
  return 0;
}

internal Vec2F32
os_mouse_from_window(OS_Handle window)
{
  GFX_LinuxWindow* _window = gfx_window_from_handle(window);
  return _window->mouse_pos;
}


////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  NotImplemented;
}


////////////////////////////////
//~ rjf: @os_hooks System Properties (Implemented Per-OS)

internal F32
os_double_click_time(void)
{
  return 0.f;
  NotImplemented;
}

internal F32
os_caret_blink_time(void)
{
  return 0.f;
  NotImplemented;
}

internal F32
os_default_refresh_rate(void)
{
  OS_Handle primary_monitor = os_primary_monitor();
  GFX_LinuxMonitor* monitor = gfx_monitor_from_handle(primary_monitor);
  return monitor->refresh_rate;
}


////////////////////////////////
//~ rjf: @os_hooks Native Messages & Panics (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  NotImplemented;
}
