
#ifndef GFX_LINUX_H
#define GFX_LINUX_H

#include <EGL/egl.h>

typedef struct GFX_LinuxContext GFX_LinuxContext;
struct GFX_LinuxContext
{

  U8* window_name;
  Vec2F32 default_window_size;
  Vec2F32 window_size;
  Vec2F32 default_window_pos;
  Vec2F32 window_pos;

  EGLNativeDisplayType native_server;
  EGLNativeWindowType native_window;
};

typedef struct GFX_LinuxWindow GFX_LinuxWindow;
struct GFX_LinuxWindow
{
  U64 handle;
  String8 window_name;
  Vec2F32 start_pos;
  Vec2F32 pos;
  Vec2F32 size;
  U32 root_relative_depth;
  EGLSurface first_surface;
  EGLSurface second_surface;
  B32 wayland_native;
};
extern Arena* gfx_lnx_arena;


internal GFX_LinuxWindow* gfx_window_from_handle(OS_Handle context);

internal OS_Handle gfx_handle_from_window(GFX_LinuxWindow* window);


#endif /* GFX_LINUX_H */
