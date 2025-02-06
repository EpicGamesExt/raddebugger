
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
  U64 id;
  U64 handle;
  B32 primary;
  // Offset should be zero on platforms where it doesn't make sense
  Vec2F32 offset;
  Vec2F32 offset_mid;
  Vec2F32 size_px;
  Vec2F32 size_physical_mm;
  Vec2F32 size_mid_px;
  Vec2F32 size_physical_mid_mm;
  F32 dpi;
  F32 dots_per_mm;
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
  GFX_LinuxWindow* next;
  GFX_LinuxWindow* prev;
  OS_Guid id;
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
  Vec2F32 mouse_pos;
  DenseTime mouse_timestamp;
  B32 wayland_native;
};

typedef struct GFX_LinuxWindowList GFX_LinuxWindowList;
struct GFX_LinuxWindowList
{
  GFX_LinuxWindow* first;
  GFX_LinuxWindow* last;
  U64 count;
};

DeclareArray(GFX_LinuxMonitorArray, GFX_LinuxMonitor);

// Global Data
global Arena* gfx_lnx_arena;
global U32 gfx_lnx_max_monitors;
global OS_EventFlags gfx_lnx_modifier_state;
global GFX_LinuxWindowList gfx_lnx_windows;
/// A list of any monitors known internally
global GFX_LinuxMonitorArray gfx_lnx_monitors;
/// A list of any monitors actively being used or recorded by DF or GFX
global GFX_LinuxMonitorArray gfx_lnx_monitors_active;
global GFX_LinuxMonitor* gfx_lnx_primary_monitor;
global void* gfx_lnx_icon;
global Vec2S32 gfx_lnx_icon_size;
global U32 gfx_lnx_icon_capacity;
global U32 gfx_lnx_icon_stride;

GFX_LinuxMonitor* gfx_monitor_from_handle(OS_Handle monitor);
OS_Handle gfx_handle_from_monitor(GFX_LinuxMonitor* monitor);
internal GFX_LinuxWindow* gfx_window_from_handle(OS_Handle context);
internal OS_Handle gfx_handle_from_window(GFX_LinuxWindow* window);

#endif /* GFX_LINUX_H */
