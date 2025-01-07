
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

typedef enum GFX_SubPixelOrder GFX_SubPixelOrder;
enum GFX_SubPixelOrder
{
  GFX_SubPixelOrder_Stub
};

typedef enum GFX_MonitorOrientation GFX_MonitorOrientation;
enum GFX_MonitorOrientation
{
  GFX_MonitorOrientation_Stub
};

typedef struct GFX_LinuxMonitor GFX_LinuxMonitor;
struct GFX_LinuxMonitor
{
  U64 handle;
  B32 primary;
  // Offset should be zero on platforms where it doesn't make sense
  Vec2F32 offset;
  Vec2F32 offset_mid;
  Vec2F32 size_px;
  Vec2F32 size_physical_mm;
  Vec2F32 size_mid_px;
  Vec2F32 size_physical_mid_mm;
  F32 refresh_rate;
  F32 refresh_rate_target;
  GFX_SubPixelOrder subpixel_order;
  GFX_MonitorOrientation orientation;
  B32 landscape;
  B32 landscape_physical;
  B32 physical;
};

typedef struct GFX_LinuxWindow GFX_LinuxWindow;
struct GFX_LinuxWindow
{
  U64 handle;
  GFX_LinuxMonitor* monitor;
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

GFX_LinuxMonitor* gfx_monitor_from_handle(OS_Handle monitor);
OS_Handle gfx_handle_from_monitor(GFX_LinuxMonitor* monitor);
internal GFX_LinuxWindow* gfx_window_from_handle(OS_Handle context);
internal OS_Handle gfx_handle_from_window(GFX_LinuxWindow* window);

#endif /* GFX_LINUX_H */
