// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_COMPOSITION_H
#define WIN32_COMPOSITION_H

////////////////////////////////
//~ Composition: synchronized window resizes
//
// The renderer writes its final pass directly into a Windows.UI.Composition drawing surface instead of a swap
// chain. BeginDraw supplies the render target and its offset; the renderer keeps neither past EndDraw.
// New surface pixels are part of a composition commit. DWM can hold a resized window frame until the commit
// that carries the resize's synchronization object.
//
// Private and Windows 11 only: user32 ordinals 2538 (LayoutCompleted), 2614
// (GetResizeDCompositionSynchronizationObject) and 2615 (EnableResizeLayoutSynchronization),
// dcomp's interop compositor (IInteropCompositorFactoryPartner slot 6) and its
// IDCompositionDesktopDevicePartner6 slot 69 (SynchronizedCommit). A hidden, zero-size ordinary Win32 child per
// window, with resize synchronization enabled on both parent and child, makes DWM issue the objects; no XAML is
// needed. Frames that follow each other are paced on the compositor clock; idle windows wait for messages.
// Each piece falls back: no composition -> the swap chain; no resize APIs -> unsynchronized commits.
//
// WM_WINDOWPOSCHANGED captures the resize token and posts a render message, returning before building the frame.
// That message draws the latest size, commits with the captured token, then calls LayoutCompleted on the same HWND.
// Like UWP XAML's size notification followed by a render tick, submission happens outside the size-change callback:
// synchronous dispatch flickered in a same-executable comparison, while posted dispatch was clean without either
// GPU-completion or composition-processing waits.
// New size changes replace an unsubmitted token; teardown closes any token still queued. Minimized windows and
// reentrant programmatic resizes retain the existing no-render completion path.
//
// Only the vtable slots used are called, by number, to keep the WinRT headers out of the build.

#include <roapi.h>
#include <winstring.h>
#pragma comment(lib, "runtimeobject")

typedef struct W32_Comp_State W32_Comp_State;
struct W32_Comp_State
{
  B32 init_tried;
  void *queue_controller;        // IDispatcherQueueController: the compositor needs a DispatcherQueue on its thread
  void *compositor;              // ICompositor
  void *graphics_device;         // ICompositionGraphicsDevice over the renderer's D3D11 device
  
  // resize synchronization
  void *device_partner;          // IDCompositionDesktopDevicePartner6
  BOOL (WINAPI *get_resize_sync_object)(HWND hwnd, HANDLE *sync);
  BOOL (WINAPI *enable_resize_sync)(HWND hwnd, BOOL enable);
  BOOL (WINAPI *layout_completed)(HWND hwnd);
  ATOM resize_child_class;
  
  // pacing
  HRESULT (WINAPI *wait_for_compositor_clock)(UINT count, const HANDLE *handles, DWORD timeout_ms);
};

global W32_Comp_State w32_comp_state = {0};

#define W32_COMP_RENDER_RESIZE_MSG (WM_APP + 0x3F2)

internal B32  w32_comp_window_equip(WM_Window window, void *d3d11_device);
internal void w32_comp_window_unequip(WM_Window window);
internal HRESULT w32_comp_window_begin_draw(WM_Window window, Vec2S32 size, REFIID iid, void **out, POINT *offset);
internal HRESULT w32_comp_window_end_draw(WM_Window window);
internal void w32_comp_commit_resize(W32_WM_Window *window, HANDLE sync);

#endif // WIN32_COMPOSITION_H
