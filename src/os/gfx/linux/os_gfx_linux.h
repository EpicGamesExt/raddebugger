
#ifndef GFX_LINUX_H
#define GFX_LINUX_H

#include <EGL/egl.h>

typedef struct GFX_Context GFX_Context;
struct GFX_Context
{

  U8* window_name;
  Vec2S32 default_window_size;
  Vec2S32 window_size;
  Vec2S32 default_window_pos;
  Vec2S32 window_pos;

  EGLNativeDisplayType native_server;
  EGLNativeWindowType native_window;
};

#endif /* GFX_LINUX_H */
