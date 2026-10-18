// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_COMPOSITION_H
#define WIN32_COMPOSITION_H

////////////////////////////////
//~ Composition: frame-perfect window resizes
//
// The renderer draws each window into a Windows.UI.Composition drawing surface instead of a swap chain. A
// surface's new pixels are part of a composition commit, and DWM can hold a resized window frame until the commit
// that carries the resize's synchronization object; so the content of each resize step shows in the same DWM frame
// as the window's new size (no lag or gap at the right or bottom edge, nor at the left/top while dragging those).
//
// Private and Windows 11 only: user32 ordinals 2614 (GetResizeDCompositionSynchronizationObject) and 2615
// (EnableResizeLayoutSynchronization), dcomp's interop compositor (IInteropCompositorFactoryPartner slot 6) and its
// IDCompositionDesktopDevicePartner6 slot 69 (SynchronizedCommit). DWM only issues the objects to windows hosting
// UWP content, hence an empty, zero-size XAML island per window. XAML on the thread also brings
// CompositionTarget.Rendering, which paces frames that follow each other (it ticks once per compositor frame).
// Each piece falls back: no composition -> the swap chain; no island -> unsynchronized commits, paced on the
// compositor clock.
//
// Only the vtable slots used are called, by number, to keep the WinRT headers out of the build.

#include <roapi.h>
#include <winstring.h>
#pragma comment(lib, "runtimeobject")

typedef struct W32_Comp_State W32_Comp_State;
struct W32_Comp_State
{
  B32 init_tried;
  B32 xaml_tried;
  void *queue_controller;        // IDispatcherQueueController: the compositor needs a DispatcherQueue on its thread
  void *compositor;              // ICompositor
  void *graphics_device;         // ICompositionGraphicsDevice over the renderer's D3D11 device
  
  // resize synchronization
  void *device_partner;          // IDCompositionDesktopDevicePartner6
  BOOL (WINAPI *get_resize_sync_object)(HWND hwnd, HANDLE *sync);
  BOOL (WINAPI *enable_resize_sync)(HWND hwnd, BOOL enable);
  
  // xaml (the islands' thread state)
  void *xaml_manager;            // IWindowsXamlManager
  HWND core_hwnd;                // the thread's CoreWindow, which DWM also waits on during a resize
  void *resize_manager;          // ICoreWindowResizeManager
  void *composition_target;      // Windows.UI.Xaml.Media.ICompositionTargetStatics
  
  // pacing
  B32 ticking;                   // subscribed to CompositionTarget.Rendering
  B32 ticked;                    // a tick came since the last frame began
  S64 rendering_token;
  HRESULT (WINAPI *wait_for_compositor_clock)(UINT count, const HANDLE *handles, DWORD timeout_ms);
};

global W32_Comp_State w32_comp_state = {0};

#define W32_COMP_ATTACH_ISLAND_MSG (WM_APP + 0x3F1)

internal B32  w32_comp_window_equip(WM_Window window, void *d3d11_device);
internal void w32_comp_window_unequip(WM_Window window);
internal B32  w32_comp_window_begin_draw(WM_Window window, Vec2S32 size, REFIID iid, void **out, POINT *offset);
internal void w32_comp_window_end_draw(WM_Window window);
internal void w32_comp_window_attach_island(W32_WM_Window *window);
internal void w32_comp_commit_resize(W32_WM_Window *window);
internal void w32_comp_pace(B32 frames_follow);

#endif // WIN32_COMPOSITION_H
