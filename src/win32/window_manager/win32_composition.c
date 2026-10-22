// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Composition: interface IDs and the vtable slots used (from the Windows SDK's MIDL C bindings)

global const GUID w32_comp_iid_compositor            = {0xb403ca50, 0x7f8c, 0x4e83, {0x98, 0x5f, 0xcc, 0x45, 0x06, 0x00, 0x36, 0xd8}};
global const GUID w32_comp_iid_compositor_interop    = {0x25297d5c, 0x3ad4, 0x4c9c, {0xb5, 0xcf, 0xe3, 0x6a, 0x38, 0x51, 0x23, 0x30}};
global const GUID w32_comp_iid_compositor_desktop    = {0x29e691fa, 0x4567, 0x4dca, {0xb3, 0x19, 0xd0, 0xf2, 0x07, 0xeb, 0x68, 0x07}};
global const GUID w32_comp_iid_factory_partner       = {0x22118adf, 0x23f1, 0x4801, {0xbc, 0xfa, 0x66, 0xcb, 0xf4, 0x8c, 0xc5, 0x1b}};
global const GUID w32_comp_iid_device_partner6       = {0xe01eb649, 0x787e, 0x4560, {0xb3, 0x98, 0x0d, 0xe7, 0xa2, 0x06, 0x5d, 0x8b}};
global const GUID w32_comp_iid_visual                = {0x117e202d, 0xa859, 0x4c89, {0x87, 0x3b, 0xc2, 0xaa, 0x56, 0x67, 0x88, 0xe3}};
global const GUID w32_comp_iid_composition_target    = {0xa1bea8ba, 0xd726, 0x4663, {0x81, 0x29, 0x6b, 0x5e, 0x79, 0x27, 0xff, 0xa6}};
global const GUID w32_comp_iid_composition_surface   = {0x1527540d, 0x42c7, 0x47a6, {0xa4, 0x08, 0x66, 0x8f, 0x79, 0xa9, 0x0d, 0xfb}};
global const GUID w32_comp_iid_composition_brush     = {0xab0d7608, 0x30c0, 0x40e9, {0xb5, 0x68, 0xb6, 0x0a, 0x6b, 0xd1, 0xfb, 0x46}};
global const GUID w32_comp_iid_drawing_interop       = {0xfd04e6e3, 0xfe0c, 0x4c3c, {0xab, 0x19, 0xa0, 0x76, 0x01, 0xa5, 0x76, 0xee}};

// Slot 0-2 are IUnknown's, 3-5 IInspectable's on WinRT interfaces.
#define W32_COMP_SLOT_FACTORY_PARTNER_CREATE_INTEROP_COMPOSITOR 6
#define W32_COMP_SLOT_DEVICE_PARTNER6_SYNCHRONIZED_COMMIT        69
#define W32_COMP_SLOT_COMPOSITOR_CREATE_SPRITE_VISUAL            22
#define W32_COMP_SLOT_COMPOSITOR_CREATE_SURFACE_BRUSH            24
#define W32_COMP_SLOT_COMPOSITOR_INTEROP_CREATE_GRAPHICS_DEVICE  5
#define W32_COMP_SLOT_COMPOSITOR_DESKTOP_CREATE_TARGET           3
#define W32_COMP_SLOT_GRAPHICS_DEVICE_CREATE_DRAWING_SURFACE     6
#define W32_COMP_SLOT_VISUAL_PUT_SIZE                            36
#define W32_COMP_SLOT_SPRITE_VISUAL_PUT_BRUSH                    7
#define W32_COMP_SLOT_TARGET_PUT_ROOT                            7
#define W32_COMP_SLOT_DRAWING_INTEROP_BEGIN_DRAW                 3
#define W32_COMP_SLOT_DRAWING_INTEROP_END_DRAW                   4
#define W32_COMP_SLOT_DRAWING_INTEROP_RESIZE                     5

#define W32_COMP_PIXEL_FORMAT_B8G8R8A8_UNORM 87
#define W32_COMP_ALPHA_MODE_PREMULTIPLIED    1

typedef struct W32_Comp_Vec2 W32_Comp_Vec2;
struct W32_Comp_Vec2
{
  F32 x;
  F32 y;
};

typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_GetFn)(void *self, void **out);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_PutFn)(void *self, void *value);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_CallFn)(void *self);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_PutGetFn)(void *self, void *value, void **out);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_CreateInteropCompositorFn)(void *self, void *rendering_device, void *callback, REFIID iid, void **out);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_CreateTargetFn)(void *self, HWND hwnd, BOOL is_topmost, void **out);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_CreateDrawingSurfaceFn)(void *self, W32_Comp_Vec2 size, S32 pixel_format, S32 alpha_mode, void **out);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_PutVec2Fn)(void *self, W32_Comp_Vec2 value);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_BeginDrawFn)(void *self, const RECT *update_rect, REFIID iid, void **out, POINT *offset);
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_ResizeFn)(void *self, SIZE size);
typedef struct W32_Comp_DispatcherQueueOptions W32_Comp_DispatcherQueueOptions;
struct W32_Comp_DispatcherQueueOptions
{
  DWORD size;
  S32 thread_type;    // DQTYPE_THREAD_CURRENT = 2
  S32 apartment_type; // DQTAT_COM_STA = 2
};
typedef HRESULT (WINAPI *W32_Comp_CreateDispatcherQueueControllerFn)(W32_Comp_DispatcherQueueOptions options, void **out);

#define w32_comp_slot(object, slot, type) ((type)((*(void ***)(object))[slot]))

////////////////////////////////
//~ Composition: helpers

internal void
w32_comp_check(HRESULT error, const char *operation)
{
  if(FAILED(error))
  {
    char buffer[256] = {0};
    raddbg_snprintf(buffer, sizeof(buffer), "DirectComposition %s failure (%lx). The process is terminating.", operation, error);
    sh_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
    abort_self(1);
  }
}

internal void *
w32_comp_query(void *object, REFIID iid)
{
  void *result = 0;
  w32_comp_check(((IUnknown *)object)->lpVtbl->QueryInterface((IUnknown *)object, iid, &result), "query interface");
  return result;
}

internal void
w32_comp_release(void *object)
{
  if(object != 0)
  {
    ((IUnknown *)object)->lpVtbl->Release((IUnknown *)object);
  }
}

////////////////////////////////
//~ Composition: windows

internal void
w32_comp_window_equip(WM_Window handle, void *d3d11_device)
{
  W32_Comp_State *s = &w32_comp_state;
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  
  //- the compositor and the graphics device, at the first window
  if(!s->init_tried)
  {
    s->init_tried = 1;
    
    // a DispatcherQueue on this thread
    W32_Comp_CreateDispatcherQueueControllerFn create_queue = (W32_Comp_CreateDispatcherQueueControllerFn)GetProcAddress(LoadLibraryW(L"CoreMessaging.dll"), "CreateDispatcherQueueController");
    if(create_queue == 0)
    {
      w32_comp_check(HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND), "load dispatcher queue API");
    }
    W32_Comp_DispatcherQueueOptions options = {sizeof(options), 2, 2};
    w32_comp_check(create_queue(options, &s->queue_controller), "create dispatcher queue");
    
    // dcomp's interop compositor, which takes synchronized commits
    static const WCHAR compositor_name[] = L"Windows.UI.Composition.Compositor";
    HSTRING_HEADER header;
    HSTRING name = 0;
    w32_comp_check(WindowsCreateStringReference(compositor_name, ArrayCount(compositor_name) - 1, &header, &name), "name compositor class");
    void *factory = 0;
    w32_comp_check(RoGetActivationFactory(name, &w32_comp_iid_factory_partner, &factory), "get interop compositor factory");
    w32_comp_check(w32_comp_slot(factory, W32_COMP_SLOT_FACTORY_PARTNER_CREATE_INTEROP_COMPOSITOR, W32_Comp_CreateInteropCompositorFn)(factory, 0, 0, &w32_comp_iid_compositor, &s->compositor), "create interop compositor");
    w32_comp_release(factory);
    
    // the graphics device that hands out the renderer's D3D11 textures
    void *compositor_interop = w32_comp_query(s->compositor, &w32_comp_iid_compositor_interop);
    w32_comp_check(w32_comp_slot(compositor_interop, W32_COMP_SLOT_COMPOSITOR_INTEROP_CREATE_GRAPHICS_DEVICE, W32_Comp_PutGetFn)(compositor_interop, d3d11_device, &s->graphics_device), "create graphics device");
    w32_comp_release(compositor_interop);
    
    // resize synchronization
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    s->device_partner = w32_comp_query(s->compositor, &w32_comp_iid_device_partner6);
    s->get_resize_sync_object = (BOOL (WINAPI *)(HWND, HANDLE *))GetProcAddress(user32, MAKEINTRESOURCEA(2614));
    s->enable_resize_sync = (BOOL (WINAPI *)(HWND, BOOL))GetProcAddress(user32, MAKEINTRESOURCEA(2615));
    s->layout_completed = (BOOL (WINAPI *)(HWND))GetProcAddress(user32, MAKEINTRESOURCEA(2538));
    if(s->get_resize_sync_object == 0 || s->enable_resize_sync == 0 || s->layout_completed == 0)
    {
      w32_comp_check(HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND), "load resize synchronization APIs");
    }
    WNDCLASSW child_class = {0};
    child_class.lpfnWndProc = DefWindowProcW;
    child_class.hInstance = w32_wm_state->hInstance;
    child_class.lpszClassName = L"composition-resize-child";
    s->resize_child_class = RegisterClassW(&child_class);
    if(s->resize_child_class == 0)
    {
      w32_comp_check(HRESULT_FROM_WIN32(GetLastError()), "register resize synchronization child");
    }
    
    // pacing without a swap chain or XAML
    s->wait_for_compositor_clock = (HRESULT (WINAPI *)(UINT, const HANDLE *, DWORD))GetProcAddress(LoadLibraryW(L"dcomp.dll"), "DCompositionWaitForCompositorClock");
    if(s->wait_for_compositor_clock == 0)
    {
      w32_comp_check(HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND), "load compositor-clock API");
    }
  }
  
  //- the window's tree: target -> sprite visual, whose brush is the drawing surface
  if(window == 0)
  {
    w32_comp_check(E_INVALIDARG, "equip window");
  }
  void *desktop_interop = w32_comp_query(s->compositor, &w32_comp_iid_compositor_desktop);
  void *desktop_target = 0;
  w32_comp_check(w32_comp_slot(desktop_interop, W32_COMP_SLOT_COMPOSITOR_DESKTOP_CREATE_TARGET, W32_Comp_CreateTargetFn)(desktop_interop, window->hwnd, 1, &desktop_target), "create desktop target");
  void *target = w32_comp_query(desktop_target, &w32_comp_iid_composition_target);
  void *sprite = 0;
  w32_comp_check(w32_comp_slot(s->compositor, W32_COMP_SLOT_COMPOSITOR_CREATE_SPRITE_VISUAL, W32_Comp_GetFn)(s->compositor, &sprite), "create sprite visual");
  void *visual = w32_comp_query(sprite, &w32_comp_iid_visual);
  W32_Comp_Vec2 one = {1, 1};
  void *drawing_surface = 0;
  w32_comp_check(w32_comp_slot(s->graphics_device, W32_COMP_SLOT_GRAPHICS_DEVICE_CREATE_DRAWING_SURFACE, W32_Comp_CreateDrawingSurfaceFn)(s->graphics_device, one, W32_COMP_PIXEL_FORMAT_B8G8R8A8_UNORM, W32_COMP_ALPHA_MODE_PREMULTIPLIED, &drawing_surface), "create drawing surface");
  void *surface = w32_comp_query(drawing_surface, &w32_comp_iid_composition_surface);
  void *drawing_interop = w32_comp_query(drawing_surface, &w32_comp_iid_drawing_interop);
  void *surface_brush = 0;
  w32_comp_check(w32_comp_slot(s->compositor, W32_COMP_SLOT_COMPOSITOR_CREATE_SURFACE_BRUSH, W32_Comp_PutGetFn)(s->compositor, surface, &surface_brush), "create surface brush");
  void *brush = w32_comp_query(surface_brush, &w32_comp_iid_composition_brush);
  w32_comp_check(w32_comp_slot(sprite, W32_COMP_SLOT_SPRITE_VISUAL_PUT_BRUSH, W32_Comp_PutFn)(sprite, brush), "set surface brush");
  w32_comp_check(w32_comp_slot(target, W32_COMP_SLOT_TARGET_PUT_ROOT, W32_Comp_PutFn)(target, visual), "set root visual");
  window->comp_target = target;
  window->comp_visual = visual;
  window->comp_surface = drawing_interop;
  window->comp_size = v2s32(0, 0);
  window->comp_resize_child = CreateWindowExW(0, MAKEINTRESOURCEW(s->resize_child_class), L"", WS_CHILD,
                                             0, 0, 0, 0, window->hwnd, 0, w32_wm_state->hInstance, 0);
  if(window->comp_resize_child == 0)
  {
    w32_comp_check(HRESULT_FROM_WIN32(GetLastError()), "create resize synchronization child");
  }
  if(!s->enable_resize_sync(window->hwnd, 1) || !s->enable_resize_sync(window->comp_resize_child, 1))
  {
    w32_comp_check(E_FAIL, "enable resize synchronization");
  }
  w32_comp_release(desktop_interop);
  w32_comp_release(desktop_target);
  w32_comp_release(sprite);
  w32_comp_release(drawing_surface);
  w32_comp_release(surface);
  w32_comp_release(surface_brush);
  w32_comp_release(brush);
}

internal void
w32_comp_window_unequip(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0 && window->comp_target != 0)
  {
    if(window->comp_resize_sync != 0)
    {
      CloseHandle(window->comp_resize_sync);
      window->comp_resize_sync = 0;
    }
    if(window->comp_resize_queued)
    {
      Assert(w32_comp_state.pending_resizes > 0);
      --w32_comp_state.pending_resizes;
      window->comp_resize_queued = 0;
    }
    if(window->comp_resize_child != 0)
    {
      if(!w32_comp_state.enable_resize_sync(window->hwnd, 0) ||
         !w32_comp_state.enable_resize_sync(window->comp_resize_child, 0) ||
         !DestroyWindow(window->comp_resize_child))
      {
        sh_message(1, str8_lit("Fatal Error"), str8_lit("Could not remove the resize synchronization child."));
        abort_self(1);
      }
      window->comp_resize_child = 0;
    }
    w32_comp_release(window->comp_surface);
    w32_comp_release(window->comp_visual);
    w32_comp_release(window->comp_target);
    window->comp_surface = window->comp_visual = window->comp_target = 0;
  }
}

internal HRESULT
w32_comp_window_begin_draw(WM_Window handle, Vec2S32 size, REFIID iid, void **out, POINT *offset)
{
  HRESULT result = E_INVALIDARG;
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0 && window->comp_surface != 0 && size.x > 0 && size.y > 0)
  {
    // the surface and the visual change size in the same commit as the content drawn at that size
    if(window->comp_size.x != size.x || window->comp_size.y != size.y)
    {
      SIZE surface_size = {size.x, size.y};
      W32_Comp_Vec2 visual_size = {(F32)size.x, (F32)size.y};
      result = w32_comp_slot(window->comp_surface, W32_COMP_SLOT_DRAWING_INTEROP_RESIZE, W32_Comp_ResizeFn)(window->comp_surface, surface_size);
      if(FAILED(result)){return result;}
      result = w32_comp_slot(window->comp_visual, W32_COMP_SLOT_VISUAL_PUT_SIZE, W32_Comp_PutVec2Fn)(window->comp_visual, visual_size);
      if(FAILED(result)){return result;}
      window->comp_size = size;
    }
    result = w32_comp_slot(window->comp_surface, W32_COMP_SLOT_DRAWING_INTEROP_BEGIN_DRAW, W32_Comp_BeginDrawFn)(window->comp_surface, 0, iid, out, offset);
  }
  return result;
}

internal HRESULT
w32_comp_window_end_draw(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0 && window->comp_surface != 0)
  {
    return w32_comp_slot(window->comp_surface, W32_COMP_SLOT_DRAWING_INTEROP_END_DRAW, W32_Comp_CallFn)(window->comp_surface);
  }
  return E_INVALIDARG;
}

////////////////////////////////
//~ Composition: resize synchronization

// After the posted frame, commit its captured token and notify the window. Neither completion wait is needed.
internal void
w32_comp_commit_resize(W32_WM_Window *window, HANDLE sync)
{
  W32_Comp_State *s = &w32_comp_state;
  HRESULT error = w32_comp_slot(s->device_partner, W32_COMP_SLOT_DEVICE_PARTNER6_SYNCHRONIZED_COMMIT, W32_Comp_PutFn)(s->device_partner, sync);
  if(sync != 0)
  {
    CloseHandle(sync);
  }
  if(SUCCEEDED(error) && !s->layout_completed(window->hwnd))
  {
    error = E_FAIL;
  }
  if(FAILED(error))
  {
    char buffer[256] = {0};
    raddbg_snprintf(buffer, sizeof(buffer), "DirectComposition resize commit failure (%lx). The process is terminating.", error);
    sh_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
    abort_self(1);
  }
}

internal void
w32_comp_commit_frame(void)
{
  W32_Comp_State *s = &w32_comp_state;
  if(s->device_partner != 0 && s->pending_resizes == 0)
  {
    w32_comp_check(w32_comp_slot(s->device_partner, W32_COMP_SLOT_DEVICE_PARTNER6_SYNCHRONIZED_COMMIT, W32_Comp_PutFn)(s->device_partner, 0), "commit frame");
  }
}
