// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Composition: interface IDs and the vtable slots used (from the Windows SDK's MIDL C bindings)

global const GUID w32_comp_iid_unknown               = {0x00000000, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
global const GUID w32_comp_iid_compositor            = {0xb403ca50, 0x7f8c, 0x4e83, {0x98, 0x5f, 0xcc, 0x45, 0x06, 0x00, 0x36, 0xd8}};
global const GUID w32_comp_iid_compositor_interop    = {0x25297d5c, 0x3ad4, 0x4c9c, {0xb5, 0xcf, 0xe3, 0x6a, 0x38, 0x51, 0x23, 0x30}};
global const GUID w32_comp_iid_compositor_desktop    = {0x29e691fa, 0x4567, 0x4dca, {0xb3, 0x19, 0xd0, 0xf2, 0x07, 0xeb, 0x68, 0x07}};
global const GUID w32_comp_iid_factory_partner       = {0x22118adf, 0x23f1, 0x4801, {0xbc, 0xfa, 0x66, 0xcb, 0xf4, 0x8c, 0xc5, 0x1b}};
global const GUID w32_comp_iid_device_partner6       = {0xe01eb649, 0x787e, 0x4560, {0xb3, 0x98, 0x0d, 0xe7, 0xa2, 0x06, 0x5d, 0x8b}};
global const GUID w32_comp_iid_device2               = {0x75f6468d, 0x1b8e, 0x447c, {0x9b, 0xc6, 0x75, 0xfe, 0xa8, 0x0b, 0x5b, 0x25}};
global const GUID w32_comp_iid_visual                = {0x117e202d, 0xa859, 0x4c89, {0x87, 0x3b, 0xc2, 0xaa, 0x56, 0x67, 0x88, 0xe3}};
global const GUID w32_comp_iid_composition_target    = {0xa1bea8ba, 0xd726, 0x4663, {0x81, 0x29, 0x6b, 0x5e, 0x79, 0x27, 0xff, 0xa6}};
global const GUID w32_comp_iid_composition_surface   = {0x1527540d, 0x42c7, 0x47a6, {0xa4, 0x08, 0x66, 0x8f, 0x79, 0xa9, 0x0d, 0xfb}};
global const GUID w32_comp_iid_composition_brush     = {0xab0d7608, 0x30c0, 0x40e9, {0xb5, 0x68, 0xb6, 0x0a, 0x6b, 0xd1, 0xfb, 0x46}};
global const GUID w32_comp_iid_drawing_interop       = {0xfd04e6e3, 0xfe0c, 0x4c3c, {0xab, 0x19, 0xa0, 0x76, 0x01, 0xa5, 0x76, 0xee}};
global const GUID w32_comp_iid_xaml_manager_statics  = {0x28258a12, 0x7d82, 0x505b, {0xb2, 0x10, 0x71, 0x2b, 0x04, 0xa5, 0x88, 0x82}};
global const GUID w32_comp_iid_xaml_source_native    = {0x3cbcf1bf, 0x2f76, 0x4e9c, {0x96, 0xab, 0xe8, 0x4b, 0x37, 0x97, 0x25, 0x54}};
global const GUID w32_comp_iid_target_statics        = {0x2b1af03d, 0x1ed2, 0x4b59, {0xbd, 0x00, 0x75, 0x94, 0xee, 0x92, 0x83, 0x2b}};
global const GUID w32_comp_iid_event_handler         = {0xc50898f6, 0xc536, 0x5f47, {0x85, 0x83, 0x8b, 0x2c, 0x24, 0x38, 0xa1, 0x3b}}; // EventHandler<IInspectable*>
global const GUID w32_comp_iid_agile_object          = {0x94ea2b94, 0xe9cc, 0x49e0, {0xc0, 0xff, 0xee, 0x64, 0xca, 0x8f, 0x5b, 0x90}};

// Slot 0-2 are IUnknown's, 3-5 IInspectable's on WinRT interfaces.
#define W32_COMP_SLOT_FACTORY_PARTNER_CREATE_INTEROP_COMPOSITOR 6
#define W32_COMP_SLOT_DEVICE_PARTNER6_SYNCHRONIZED_COMMIT        69
#define W32_COMP_SLOT_DEVICE_WAIT_FOR_COMMIT_COMPLETION          4
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
#define W32_COMP_SLOT_STATICS_GET                                6 // InitializeForCurrentThread
#define W32_COMP_SLOT_XAML_SOURCE_ATTACH_TO_WINDOW               3
#define W32_COMP_SLOT_XAML_SOURCE_GET_WINDOW_HANDLE              4
#define W32_COMP_SLOT_TARGET_STATICS_ADD_RENDERING               6
#define W32_COMP_SLOT_TARGET_STATICS_REMOVE_RENDERING            7

#define W32_COMP_PIXEL_FORMAT_B8G8R8A8_UNORM 87
#define W32_COMP_ALPHA_MODE_PREMULTIPLIED    1
#define W32_COMP_ALPHA_MODE_IGNORE           3

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
typedef HRESULT (STDMETHODCALLTYPE *W32_Comp_RemoveTokenFn)(void *self, S64 token);
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

internal void *
w32_comp_query(void *object, REFIID iid)
{
  void *result = 0;
  if(object != 0)
  {
    ((IUnknown *)object)->lpVtbl->QueryInterface((IUnknown *)object, iid, &result);
  }
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

internal HSTRING
w32_comp_class_name(HSTRING_HEADER *header, const WCHAR *name)
{
  HSTRING result = 0;
  WindowsCreateStringReference(name, (UINT32)lstrlenW(name), header, &result);
  return result;
}

internal void *
w32_comp_statics(const WCHAR *class_name, REFIID iid)
{
  HSTRING_HEADER header;
  void *result = 0;
  RoGetActivationFactory(w32_comp_class_name(&header, class_name), iid, &result);
  return result;
}

internal void *
w32_comp_activate(const WCHAR *class_name)
{
  HSTRING_HEADER header;
  void *result = 0;
  RoActivateInstance(w32_comp_class_name(&header, class_name), (IInspectable **)&result);
  return result;
}

//- CompositionTarget.Rendering's handler: a static COM object, as nothing else needs its lifetime.

typedef struct W32_Comp_Handler W32_Comp_Handler;
struct W32_Comp_Handler
{
  const struct W32_Comp_HandlerVtbl *vtbl;
};

struct W32_Comp_HandlerVtbl
{
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(W32_Comp_Handler *self, REFIID iid, void **out);
  ULONG (STDMETHODCALLTYPE *AddRef)(W32_Comp_Handler *self);
  ULONG (STDMETHODCALLTYPE *Release)(W32_Comp_Handler *self);
  HRESULT (STDMETHODCALLTYPE *Invoke)(W32_Comp_Handler *self, void *sender, void *args);
};

internal HRESULT STDMETHODCALLTYPE
w32_comp_handler_query_interface(W32_Comp_Handler *self, REFIID iid, void **out)
{
  HRESULT result = E_NOINTERFACE;
  *out = 0;
  if(IsEqualGUID(iid, &w32_comp_iid_unknown) || IsEqualGUID(iid, &w32_comp_iid_event_handler) || IsEqualGUID(iid, &w32_comp_iid_agile_object))
  {
    *out = self;
    result = S_OK;
  }
  return result;
}

internal ULONG STDMETHODCALLTYPE
w32_comp_handler_ref(W32_Comp_Handler *self)
{
  return 1;
}

internal HRESULT STDMETHODCALLTYPE
w32_comp_rendering_invoke(W32_Comp_Handler *self, void *sender, void *args)
{
  w32_comp_state.ticked = 1;
  return S_OK;
}

global const struct W32_Comp_HandlerVtbl w32_comp_rendering_vtbl =
{
  w32_comp_handler_query_interface,
  w32_comp_handler_ref,
  w32_comp_handler_ref,
  w32_comp_rendering_invoke,
};
global W32_Comp_Handler w32_comp_rendering_handler = {&w32_comp_rendering_vtbl};

////////////////////////////////
//~ Composition: windows

internal B32
w32_comp_window_equip(WM_Window handle, void *d3d11_device)
{
  W32_Comp_State *s = &w32_comp_state;
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  
  //- the compositor and the graphics device, at the first window
  if(!s->init_tried)
  {
    s->init_tried = 1;
    
    // a DispatcherQueue on this thread (fails harmlessly if one exists)
    W32_Comp_CreateDispatcherQueueControllerFn create_queue = (W32_Comp_CreateDispatcherQueueControllerFn)GetProcAddress(LoadLibraryW(L"CoreMessaging.dll"), "CreateDispatcherQueueController");
    if(create_queue != 0)
    {
      W32_Comp_DispatcherQueueOptions options = {sizeof(options), 2, 2};
      create_queue(options, &s->queue_controller);
    }
    
    // dcomp's interop compositor (the one UWP XAML uses), which takes synchronized commits; else a plain one
    void *factory = w32_comp_statics(L"Windows.UI.Composition.Compositor", &w32_comp_iid_factory_partner);
    if(factory != 0)
    {
      w32_comp_slot(factory, W32_COMP_SLOT_FACTORY_PARTNER_CREATE_INTEROP_COMPOSITOR, W32_Comp_CreateInteropCompositorFn)(factory, 0, 0, &w32_comp_iid_compositor, &s->compositor);
      w32_comp_release(factory);
    }
    if(s->compositor == 0)
    {
      void *compositor = w32_comp_activate(L"Windows.UI.Composition.Compositor");
      s->compositor = w32_comp_query(compositor, &w32_comp_iid_compositor);
      w32_comp_release(compositor);
    }
    
    // the graphics device that hands out the renderer's D3D11 textures
    void *compositor_interop = w32_comp_query(s->compositor, &w32_comp_iid_compositor_interop);
    if(compositor_interop != 0)
    {
      w32_comp_slot(compositor_interop, W32_COMP_SLOT_COMPOSITOR_INTEROP_CREATE_GRAPHICS_DEVICE, W32_Comp_PutGetFn)(compositor_interop, d3d11_device, &s->graphics_device);
      w32_comp_release(compositor_interop);
    }
    
    // resize synchronization
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    s->device_partner = w32_comp_query(s->compositor, &w32_comp_iid_device_partner6);
    s->commit_device = w32_comp_query(s->device_partner, &w32_comp_iid_device2);
    if(s->device_partner != 0 && s->commit_device == 0)
    {
      sh_message(1, str8_lit("Fatal Error"), str8_lit("Could not get the DirectComposition commit completion interface."));
      abort_self(1);
    }
    s->get_resize_sync_object = (BOOL (WINAPI *)(HWND, HANDLE *))GetProcAddress(user32, MAKEINTRESOURCEA(2614));
    s->enable_resize_sync = (BOOL (WINAPI *)(HWND, BOOL))GetProcAddress(user32, MAKEINTRESOURCEA(2615));
    s->layout_completed = (BOOL (WINAPI *)(HWND))GetProcAddress(user32, MAKEINTRESOURCEA(2538));
    if(s->device_partner != 0 && s->layout_completed == 0)
    {
      sh_message(1, str8_lit("Fatal Error"), str8_lit("Could not get the native window layout completion API."));
      abort_self(1);
    }
    if(s->device_partner == 0 || s->get_resize_sync_object == 0 || s->enable_resize_sync == 0 ||
       GetEnvironmentVariableW(L"RADDBG_NO_RESIZE_SYNC", 0, 0) != 0) // the control for measuring what it does
    {
      s->get_resize_sync_object = 0;
    }
    
    // pacing until XAML's ticks come
    s->wait_for_compositor_clock = (HRESULT (WINAPI *)(UINT, const HANDLE *, DWORD))GetProcAddress(LoadLibraryW(L"dcomp.dll"), "DCompositionWaitForCompositorClock");
  }
  
  //- the window's tree: target -> sprite visual, whose brush is the drawing surface
  B32 result = 0;
  if(s->graphics_device != 0 && window != 0)
  {
    void *desktop_interop = w32_comp_query(s->compositor, &w32_comp_iid_compositor_desktop);
    void *desktop_target = 0;
    void *target = 0;
    void *sprite = 0;
    void *visual = 0;
    void *drawing_surface = 0;
    void *surface = 0;
    void *surface_brush = 0;
    void *brush = 0;
    void *drawing_interop = 0;
    if(desktop_interop != 0)
    {
      w32_comp_slot(desktop_interop, W32_COMP_SLOT_COMPOSITOR_DESKTOP_CREATE_TARGET, W32_Comp_CreateTargetFn)(desktop_interop, window->hwnd, 1, &desktop_target);
    }
    target = w32_comp_query(desktop_target, &w32_comp_iid_composition_target);
    w32_comp_slot(s->compositor, W32_COMP_SLOT_COMPOSITOR_CREATE_SPRITE_VISUAL, W32_Comp_GetFn)(s->compositor, &sprite);
    visual = w32_comp_query(sprite, &w32_comp_iid_visual);
    W32_Comp_Vec2 one = {1, 1};
    if(FAILED(w32_comp_slot(s->graphics_device, W32_COMP_SLOT_GRAPHICS_DEVICE_CREATE_DRAWING_SURFACE, W32_Comp_CreateDrawingSurfaceFn)(s->graphics_device, one, W32_COMP_PIXEL_FORMAT_B8G8R8A8_UNORM, W32_COMP_ALPHA_MODE_IGNORE, &drawing_surface)))
    {
      w32_comp_slot(s->graphics_device, W32_COMP_SLOT_GRAPHICS_DEVICE_CREATE_DRAWING_SURFACE, W32_Comp_CreateDrawingSurfaceFn)(s->graphics_device, one, W32_COMP_PIXEL_FORMAT_B8G8R8A8_UNORM, W32_COMP_ALPHA_MODE_PREMULTIPLIED, &drawing_surface);
    }
    surface = w32_comp_query(drawing_surface, &w32_comp_iid_composition_surface);
    drawing_interop = w32_comp_query(drawing_surface, &w32_comp_iid_drawing_interop);
    if(surface != 0)
    {
      w32_comp_slot(s->compositor, W32_COMP_SLOT_COMPOSITOR_CREATE_SURFACE_BRUSH, W32_Comp_PutGetFn)(s->compositor, surface, &surface_brush);
    }
    brush = w32_comp_query(surface_brush, &w32_comp_iid_composition_brush);
    if(target != 0 && visual != 0 && brush != 0 && drawing_interop != 0 &&
       SUCCEEDED(w32_comp_slot(sprite, W32_COMP_SLOT_SPRITE_VISUAL_PUT_BRUSH, W32_Comp_PutFn)(sprite, brush)) &&
       SUCCEEDED(w32_comp_slot(target, W32_COMP_SLOT_TARGET_PUT_ROOT, W32_Comp_PutFn)(target, visual)))
    {
      window->comp_target = target;
      window->comp_visual = visual;
      window->comp_surface = drawing_interop;
      window->comp_size = v2s32(0, 0);
      target = visual = drawing_interop = 0;
      result = 1;
    }
    w32_comp_release(desktop_interop);
    w32_comp_release(desktop_target);
    w32_comp_release(target);
    w32_comp_release(sprite);
    w32_comp_release(visual);
    w32_comp_release(drawing_surface);
    w32_comp_release(surface);
    w32_comp_release(surface_brush);
    w32_comp_release(brush);
    w32_comp_release(drawing_interop);
  }
  return result;
}

internal void
w32_comp_window_unequip(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0 && window->comp_target != 0)
  {
    if(window->comp_island != 0 && w32_comp_state.get_resize_sync_object != 0)
    {
      w32_comp_state.enable_resize_sync(window->hwnd, 0);
    }
    w32_comp_release(window->comp_island);
    w32_comp_release(window->comp_surface);
    w32_comp_release(window->comp_visual);
    w32_comp_release(window->comp_target);
    window->comp_island = window->comp_surface = window->comp_visual = window->comp_target = 0;
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
//~ Composition: XAML island, resize synchronization, pacing

// After a window's first show (posted, so XAML's start doesn't hold back the first frame). The first island also
// starts XAML on the thread, which brings the per-frame Rendering event.
internal void
w32_comp_window_attach_island(W32_WM_Window *window)
{
  W32_Comp_State *s = &w32_comp_state;
  
  //- xaml's thread state
  if(!s->xaml_tried)
  {
    s->xaml_tried = 1;
    void *xaml_statics = w32_comp_statics(L"Windows.UI.Xaml.Hosting.WindowsXamlManager", &w32_comp_iid_xaml_manager_statics);
    if(xaml_statics != 0 &&
       SUCCEEDED(w32_comp_slot(xaml_statics, W32_COMP_SLOT_STATICS_GET, W32_Comp_GetFn)(xaml_statics, &s->xaml_manager)))
    {
      s->composition_target = w32_comp_statics(L"Windows.UI.Xaml.Media.CompositionTarget", &w32_comp_iid_target_statics);
    }
    w32_comp_release(xaml_statics);
  }
  
  //- the window's island: empty and zero-size, it only makes DWM issue resize synchronization objects
  if(s->xaml_manager != 0 && window->comp_target != 0 && window->comp_island == 0)
  {
    void *island = w32_comp_activate(L"Windows.UI.Xaml.Hosting.DesktopWindowXamlSource");
    void *native = w32_comp_query(island, &w32_comp_iid_xaml_source_native);
    HWND island_hwnd = 0;
    if(native != 0 &&
       SUCCEEDED(w32_comp_slot(native, W32_COMP_SLOT_XAML_SOURCE_ATTACH_TO_WINDOW, W32_Comp_PutFn)(native, window->hwnd)) &&
       SUCCEEDED(w32_comp_slot(native, W32_COMP_SLOT_XAML_SOURCE_GET_WINDOW_HANDLE, W32_Comp_GetFn)(native, (void **)&island_hwnd)))
    {
      SetWindowPos(island_hwnd, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE);
      if(s->get_resize_sync_object != 0)
      {
        s->enable_resize_sync(window->hwnd, 1);
      }
      window->comp_island = island;
      island = 0;
      
      // keys go to the window, not the island's child window, if it took them
      HWND focus = GetFocus();
      if(focus != 0 && (focus == island_hwnd || IsChild(island_hwnd, focus)))
      {
        SetFocus(window->hwnd);
      }
    }
    w32_comp_release(native);
    w32_comp_release(island);
  }
}

// After a size change's frame: commits it with the resize's synchronization object, tells the same window
// its layout is done, then waits for DComp to process the batch before the next resize step.
internal void
w32_comp_commit_resize(W32_WM_Window *window)
{
  W32_Comp_State *s = &w32_comp_state;
  HANDLE sync = 0;
  HRESULT error = S_OK;
  if(s->get_resize_sync_object != 0 && window->comp_island != 0)
  {
    s->get_resize_sync_object(window->hwnd, &sync);
  }
  if(sync != 0)
  {
    error = w32_comp_slot(s->device_partner, W32_COMP_SLOT_DEVICE_PARTNER6_SYNCHRONIZED_COMMIT, W32_Comp_PutFn)(s->device_partner, sync);
    CloseHandle(sync);
  }
  if(sync != 0 && SUCCEEDED(error) && !s->layout_completed(window->hwnd))
  {
    error = E_FAIL;
  }
  if(sync != 0 && SUCCEEDED(error))
  {
    // A successful synchronized commit only submits the batch. Do not let the next resize advance before
    // the composition engine has processed it. This is not an acknowledgment that the frame was displayed.
    error = w32_comp_slot(s->commit_device, W32_COMP_SLOT_DEVICE_WAIT_FOR_COMMIT_COMPLETION, W32_Comp_CallFn)(s->commit_device);
  }
  if(FAILED(error))
  {
    char buffer[256] = {0};
    raddbg_snprintf(buffer, sizeof(buffer), "DirectComposition resize commit failure (%lx). The process is terminating.", error);
    sh_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
    abort_self(1);
  }
}

// While frames follow each other, CompositionTarget.Rendering ticks once per compositor frame, as a message;
// idle, it's unsubscribed so nothing wakes the thread.
internal void
w32_comp_pace(B32 frames_follow)
{
  W32_Comp_State *s = &w32_comp_state;
  if(s->composition_target != 0 && s->ticking != frames_follow)
  {
    if(frames_follow)
    {
      s->ticking = SUCCEEDED(w32_comp_slot(s->composition_target, W32_COMP_SLOT_TARGET_STATICS_ADD_RENDERING, W32_Comp_PutGetFn)(s->composition_target, &w32_comp_rendering_handler, (void **)&s->rendering_token));
      s->ticked = 0;
    }
    else
    {
      w32_comp_slot(s->composition_target, W32_COMP_SLOT_TARGET_STATICS_REMOVE_RENDERING, W32_Comp_RemoveTokenFn)(s->composition_target, s->rendering_token);
      s->ticking = 0;
    }
  }
}
