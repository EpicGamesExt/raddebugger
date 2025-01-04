
#ifndef GFX_LINUX_H
#define GFX_LINUX_H

#include <EGL/egl.h>

typedef struct GFX_LinuxContext GFX_LinuxContext;
struct GFX_LinuxContext
{

  String8 default_window_name;
  Vec2F32 default_window_size;
  Vec2F32 default_window_pos;

  EGLNativeDisplayType native_server;
  EGLNativeWindowType native_window;
};

typedef struct GFX_LinuxWindow GFX_LinuxWindow;
struct GFX_LinuxWindow
{
  U64 handle;
  String8 name;
  Vec2F32 pos;
  Vec2F32 pos_mid;
  Vec2F32 pos_target;
  Vec2F32 size;
  Vec2F32 size_target;
  F32 border_width;
  U32 root_relative_depth;
  EGLSurface first_surface;
  EGLSurface second_surface;
  B32 wayland_native;
};

// Global Data
global Arena* gfx_lnx_arena;
global U32 gfx_lnx_max_monitors;


internal GFX_LinuxWindow* gfx_window_from_handle(OS_Handle context);

internal OS_Handle gfx_handle_from_window(GFX_LinuxWindow* window);


#endif /* GFX_LINUX_H */
