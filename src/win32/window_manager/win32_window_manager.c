// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Modern Windows SDK Functions
//
// (We must dynamically link to them, since they can be missing in older SDKs)

typedef BOOL w32_SetProcessDpiAwarenessContext_Type(void* value);
typedef UINT w32_GetDpiForWindow_Type(HWND hwnd);
typedef HRESULT w32_GetDpiForMonitor_Type(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
typedef int w32_GetSystemMetricsForDpi_Type(int nIndex, UINT dpi);
#define w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((void*)-4)
global w32_GetDpiForWindow_Type *w32_GetDpiForWindow_func = 0;
global w32_GetDpiForMonitor_Type *w32_GetDpiForMonitor_func = 0;
global w32_GetSystemMetricsForDpi_Type *w32_GetSystemMetricsForDpi_func = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal Rng2F32
w32_wm_rng2f32_from_rect(RECT rect)
{
  Rng2F32 r = {0};
  r.x0 = (F32)rect.left;
  r.x1 = (F32)rect.right;
  r.y0 = (F32)rect.top;
  r.y1 = (F32)rect.bottom;
  return r;
}

////////////////////////////////
//~ rjf: Windows

internal WM_Window
w32_wm_handle_from_window(W32_WM_Window *window)
{
  WM_Window handle = {(U64)window};
  return handle;
}

internal W32_WM_Window *
w32_wm_window_from_handle(WM_Window handle)
{
  W32_WM_Window *window = (W32_WM_Window *)handle.u64[0];
  return window;
}

internal W32_WM_Window *
w32_wm_window_from_hwnd(HWND hwnd)
{
  W32_WM_Window *result = 0;
  for(W32_WM_Window *w = w32_wm_state->first_window; w; w = w->next)
  {
    if(w->hwnd == hwnd)
    {
      result = w;
      break;
    }
  }
  return result;
}

internal HWND
w32_wm_hwnd_from_window(W32_WM_Window *window)
{
  return window->hwnd;
}

internal W32_WM_Window *
w32_wm_window_alloc(void)
{
  W32_WM_Window *result = w32_wm_state->free_window;
  if(result)
  {
    SLLStackPop(w32_wm_state->free_window);
  }
  else
  {
    result = push_array_no_zero(w32_wm_state->arena, W32_WM_Window, 1);
  }
  MemoryZeroStruct(result);
  if(result)
  {
    DLLPushBack(w32_wm_state->first_window, w32_wm_state->last_window, result);
  }
  result->last_window_placement.length = sizeof(WINDOWPLACEMENT);
  return result;
}

internal void
w32_wm_window_release(W32_WM_Window *window)
{
  if(window->paint_arena != 0)
  {
    arena_release(window->paint_arena);
  }
  ReleaseDC(window->hwnd, window->hdc);
  DestroyWindow(window->hwnd);
  DLLRemove(w32_wm_state->first_window, w32_wm_state->last_window, window);
  SLLStackPush(w32_wm_state->free_window, window);
}

internal WM_Event *
w32_wm_push_event(WM_EventKind kind, W32_WM_Window *window)
{
  WM_Event *result = wm_event_list_push_new(w32_wm_event_arena, &w32_wm_event_list, kind);
  result->window = w32_wm_handle_from_window(window);
  result->modifiers = wm_get_modifiers();
  return result;
}

internal WM_Key
w32_wm_os_key_from_vkey(WPARAM vkey)
{
  local_persist B32 first = 1;
  local_persist WM_Key key_table[256];
  if (first){
    first = 0;
    MemoryZeroArray(key_table);
    
    key_table[(unsigned int)'A'] = WM_Key_A;
    key_table[(unsigned int)'B'] = WM_Key_B;
    key_table[(unsigned int)'C'] = WM_Key_C;
    key_table[(unsigned int)'D'] = WM_Key_D;
    key_table[(unsigned int)'E'] = WM_Key_E;
    key_table[(unsigned int)'F'] = WM_Key_F;
    key_table[(unsigned int)'G'] = WM_Key_G;
    key_table[(unsigned int)'H'] = WM_Key_H;
    key_table[(unsigned int)'I'] = WM_Key_I;
    key_table[(unsigned int)'J'] = WM_Key_J;
    key_table[(unsigned int)'K'] = WM_Key_K;
    key_table[(unsigned int)'L'] = WM_Key_L;
    key_table[(unsigned int)'M'] = WM_Key_M;
    key_table[(unsigned int)'N'] = WM_Key_N;
    key_table[(unsigned int)'O'] = WM_Key_O;
    key_table[(unsigned int)'P'] = WM_Key_P;
    key_table[(unsigned int)'Q'] = WM_Key_Q;
    key_table[(unsigned int)'R'] = WM_Key_R;
    key_table[(unsigned int)'S'] = WM_Key_S;
    key_table[(unsigned int)'T'] = WM_Key_T;
    key_table[(unsigned int)'U'] = WM_Key_U;
    key_table[(unsigned int)'V'] = WM_Key_V;
    key_table[(unsigned int)'W'] = WM_Key_W;
    key_table[(unsigned int)'X'] = WM_Key_X;
    key_table[(unsigned int)'Y'] = WM_Key_Y;
    key_table[(unsigned int)'Z'] = WM_Key_Z;
    
    for (U64 i = '0', j = WM_Key_0; i <= '9'; i += 1, j += 1){
      key_table[i] = (WM_Key)j;
    }
    for (U64 i = VK_NUMPAD0, j = WM_Key_0; i <= VK_NUMPAD9; i += 1, j += 1){
      key_table[i] = (WM_Key)j;
    }
    for (U64 i = VK_F1, j = WM_Key_F1; i <= VK_F24; i += 1, j += 1){
      key_table[i] = (WM_Key)j;
    }
    
    key_table[VK_SPACE]     = WM_Key_Space;
    key_table[VK_OEM_3]     = WM_Key_Tick;
    key_table[VK_OEM_MINUS] = WM_Key_Minus;
    key_table[VK_OEM_PLUS]  = WM_Key_Equal;
    key_table[VK_OEM_4]     = WM_Key_LeftBracket;
    key_table[VK_OEM_6]     = WM_Key_RightBracket;
    key_table[VK_OEM_1]     = WM_Key_Semicolon;
    key_table[VK_OEM_7]     = WM_Key_Quote;
    key_table[VK_OEM_COMMA] = WM_Key_Comma;
    key_table[VK_OEM_PERIOD]= WM_Key_Period;
    key_table[VK_OEM_2]     = WM_Key_Slash;
    key_table[VK_OEM_5]     = WM_Key_BackSlash;
    
    key_table[VK_TAB]       = WM_Key_Tab;
    key_table[VK_PAUSE]     = WM_Key_Pause;
    key_table[VK_ESCAPE]    = WM_Key_Esc;
    
    key_table[VK_UP]        = WM_Key_Up;
    key_table[VK_LEFT]      = WM_Key_Left;
    key_table[VK_DOWN]      = WM_Key_Down;
    key_table[VK_RIGHT]     = WM_Key_Right;
    
    key_table[VK_BACK]      = WM_Key_Backspace;
    key_table[VK_RETURN]    = WM_Key_Return;
    
    key_table[VK_DELETE]    = WM_Key_Delete;
    key_table[VK_INSERT]    = WM_Key_Insert;
    key_table[VK_PRIOR]     = WM_Key_PageUp;
    key_table[VK_NEXT]      = WM_Key_PageDown;
    key_table[VK_HOME]      = WM_Key_Home;
    key_table[VK_END]       = WM_Key_End;
    
    key_table[VK_CAPITAL]   = WM_Key_CapsLock;
    key_table[VK_NUMLOCK]   = WM_Key_NumLock;
    key_table[VK_SCROLL]    = WM_Key_ScrollLock;
    key_table[VK_APPS]      = WM_Key_Menu;
    
    key_table[VK_CONTROL]   = WM_Key_Ctrl;
    key_table[VK_LCONTROL]  = WM_Key_Ctrl;
    key_table[VK_RCONTROL]  = WM_Key_Ctrl;
    key_table[VK_SHIFT]     = WM_Key_Shift;
    key_table[VK_LSHIFT]    = WM_Key_Shift;
    key_table[VK_RSHIFT]    = WM_Key_Shift;
    key_table[VK_MENU]      = WM_Key_Alt;
    key_table[VK_LMENU]     = WM_Key_Alt;
    key_table[VK_RMENU]     = WM_Key_Alt;
    
    key_table[VK_DIVIDE]   = WM_Key_NumSlash;
    key_table[VK_MULTIPLY] = WM_Key_NumStar;
    key_table[VK_SUBTRACT] = WM_Key_NumMinus;
    key_table[VK_ADD]      = WM_Key_NumPlus;
    key_table[VK_DECIMAL]  = WM_Key_NumPeriod;
    
    for (U32 i = 0; i < 10; i += 1){
      key_table[VK_NUMPAD0 + i] = (WM_Key)((U64)WM_Key_Num0 + i);
    }
    
    for (U64 i = 0xDF, j = 0; i < 0xFF; i += 1, j += 1){
      key_table[i] = (WM_Key)((U64)WM_Key_Ex0 + j);
    }
  }
  
  WM_Key key = key_table[vkey&bitmask8];
  return key;
}

internal WPARAM
w32_wm_vkey_from_os_key(WM_Key key)
{
  WPARAM result = 0;
  {
    local_persist B32 initialized = 0;
    local_persist WPARAM vkey_table[WM_Key_COUNT] = {0};
    if(initialized == 0)
    {
      initialized = 1;
      vkey_table[WM_Key_Esc] = VK_ESCAPE;
      for(WM_Key key = WM_Key_F1; key <= WM_Key_F24; key = (WM_Key)(key+1))
      {
        vkey_table[key] = VK_F1+(key-WM_Key_F1);
      }
      vkey_table[WM_Key_Tick] = VK_OEM_3;
      for(WM_Key key = WM_Key_0; key <= WM_Key_9; key = (WM_Key)(key+1))
      {
        vkey_table[key] = '0'+(key-WM_Key_0);
      }
      vkey_table[WM_Key_Minus] = VK_OEM_MINUS;
      vkey_table[WM_Key_Equal] = VK_OEM_PLUS;
      vkey_table[WM_Key_Backspace] = VK_BACK;
      vkey_table[WM_Key_Tab] = VK_TAB;
      vkey_table[WM_Key_Q] = 'Q';
      vkey_table[WM_Key_W] = 'W';
      vkey_table[WM_Key_E] = 'E';
      vkey_table[WM_Key_R] = 'R';
      vkey_table[WM_Key_T] = 'T';
      vkey_table[WM_Key_Y] = 'Y';
      vkey_table[WM_Key_U] = 'U';
      vkey_table[WM_Key_I] = 'I';
      vkey_table[WM_Key_O] = 'O';
      vkey_table[WM_Key_P] = 'P';
      vkey_table[WM_Key_LeftBracket] = VK_OEM_4;
      vkey_table[WM_Key_RightBracket] = VK_OEM_6;
      vkey_table[WM_Key_BackSlash] = VK_OEM_5;
      vkey_table[WM_Key_CapsLock] = VK_CAPITAL;
      vkey_table[WM_Key_A] = 'A';
      vkey_table[WM_Key_S] = 'S';
      vkey_table[WM_Key_D] = 'D';
      vkey_table[WM_Key_F] = 'F';
      vkey_table[WM_Key_G] = 'G';
      vkey_table[WM_Key_H] = 'H';
      vkey_table[WM_Key_J] = 'J';
      vkey_table[WM_Key_K] = 'K';
      vkey_table[WM_Key_L] = 'L';
      vkey_table[WM_Key_Semicolon] = VK_OEM_1;
      vkey_table[WM_Key_Quote] = VK_OEM_7;
      vkey_table[WM_Key_Return] = VK_RETURN;
      vkey_table[WM_Key_Shift] = VK_SHIFT;
      vkey_table[WM_Key_Z] = 'Z';
      vkey_table[WM_Key_X] = 'X';
      vkey_table[WM_Key_C] = 'C';
      vkey_table[WM_Key_V] = 'V';
      vkey_table[WM_Key_B] = 'B';
      vkey_table[WM_Key_N] = 'N';
      vkey_table[WM_Key_M] = 'M';
      vkey_table[WM_Key_Comma] = VK_OEM_COMMA;
      vkey_table[WM_Key_Period] = VK_OEM_PERIOD;
      vkey_table[WM_Key_Slash] = VK_OEM_2;
      vkey_table[WM_Key_Ctrl] = VK_CONTROL;
      vkey_table[WM_Key_Alt] = VK_MENU;
      vkey_table[WM_Key_Space] = VK_SPACE;
      vkey_table[WM_Key_Menu] = VK_APPS;
      vkey_table[WM_Key_ScrollLock] = VK_SCROLL;
      vkey_table[WM_Key_Pause] = VK_PAUSE;
      vkey_table[WM_Key_Insert] = VK_INSERT;
      vkey_table[WM_Key_Home] = VK_HOME;
      vkey_table[WM_Key_PageUp] = VK_PRIOR;
      vkey_table[WM_Key_Delete] = VK_DELETE;
      vkey_table[WM_Key_End] = VK_END;
      vkey_table[WM_Key_PageDown] = VK_NEXT;
      vkey_table[WM_Key_Up] = VK_UP;
      vkey_table[WM_Key_Left] = VK_LEFT;
      vkey_table[WM_Key_Down] = VK_DOWN;
      vkey_table[WM_Key_Right] = VK_RIGHT;
      for(WM_Key key = WM_Key_Ex0; key <= WM_Key_Ex29; key = (WM_Key)(key+1))
      {
        vkey_table[key] = 0xDF + (key-WM_Key_Ex0);
      }
      vkey_table[WM_Key_NumLock] = VK_NUMLOCK;
      vkey_table[WM_Key_NumSlash] = VK_DIVIDE;
      vkey_table[WM_Key_NumStar] = VK_MULTIPLY;
      vkey_table[WM_Key_NumMinus] = VK_SUBTRACT;
      vkey_table[WM_Key_NumPlus] = VK_ADD;
      vkey_table[WM_Key_NumPeriod] = VK_DECIMAL;
      vkey_table[WM_Key_Num0] = VK_NUMPAD0;
      vkey_table[WM_Key_Num1] = VK_NUMPAD1;
      vkey_table[WM_Key_Num2] = VK_NUMPAD2;
      vkey_table[WM_Key_Num3] = VK_NUMPAD3;
      vkey_table[WM_Key_Num4] = VK_NUMPAD4;
      vkey_table[WM_Key_Num5] = VK_NUMPAD5;
      vkey_table[WM_Key_Num6] = VK_NUMPAD6;
      vkey_table[WM_Key_Num7] = VK_NUMPAD7;
      vkey_table[WM_Key_Num8] = VK_NUMPAD8;
      vkey_table[WM_Key_Num9] = VK_NUMPAD9;
      vkey_table[WM_Key_LeftMouseButton] = VK_LBUTTON;
      vkey_table[WM_Key_MiddleMouseButton] = VK_MBUTTON;
      vkey_table[WM_Key_RightMouseButton] = VK_RBUTTON;
    }
    result = vkey_table[key];
  }
  return result;
}

internal LRESULT
w32_wm_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  ProfBeginFunction();
  LRESULT result = 0;
  B32 good = 1;
  if(w32_wm_event_arena == 0)
  {
    result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
    good = 0;
  }
  if(good)
  {
    W32_WM_Window *window = w32_wm_window_from_hwnd(hwnd);
    WM_Window window_handle = w32_wm_handle_from_window(window);
    B32 release = 0;
    
    switch(uMsg)
    {
      default:
      {
        result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
      }break;
      
      case WM_ENTERSIZEMOVE:
      {
        w32_wm_resizing = 1;
      }break;
      
      case WM_EXITSIZEMOVE:
      {
        w32_wm_resizing = 0;
      }break;
      
      case WM_SIZE:
      case WM_PAINT:
      {
        PAINTSTRUCT ps = {0};
        BeginPaint(hwnd, &ps);
        update();
        EndPaint(hwnd, &ps);
        DwmFlush();
      }break;
      
      case WM_CLOSE:
      {
        w32_wm_push_event(WM_EventKind_WindowClose, window);
      }break;
      
      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
      {
        release = 1;
      } // fallthrough;
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
      {
        WM_Event *event = w32_wm_push_event(release ? WM_EventKind_Release : WM_EventKind_Press, window);
        switch (uMsg)
        {
          case WM_LBUTTONUP: case WM_LBUTTONDOWN:
          {
            event->key = WM_Key_LeftMouseButton;
          }break;
          case WM_MBUTTONUP: case WM_MBUTTONDOWN:
          {
            event->key = WM_Key_MiddleMouseButton;
          }break;
          case WM_RBUTTONUP: case WM_RBUTTONDOWN:
          {
            event->key = WM_Key_RightMouseButton;
          }break;
        }
        event->pos.x = (F32)(S16)LOWORD(lParam);
        event->pos.y = (F32)(S16)HIWORD(lParam);
        if(release)
        {
          ReleaseCapture();
        }
        else
        {
          SetCapture(hwnd);
        }
      }break;
      
      case WM_MOUSEMOVE:
      {
        WM_Event *event = w32_wm_push_event(WM_EventKind_MouseMove, window);
        event->pos.x = (F32)(S16)LOWORD(lParam);
        event->pos.y = (F32)(S16)HIWORD(lParam);
      }break;
      
      case WM_MOUSEWHEEL:
      {
        S16 wheel_delta = HIWORD(wParam);
        WM_Event *event = w32_wm_push_event(WM_EventKind_Scroll, window);
        POINT p;
        p.x = (S32)(S16)LOWORD(lParam);
        p.y = (S32)(S16)HIWORD(lParam);
        ScreenToClient(window->hwnd, &p);
        event->pos.x = (F32)p.x;
        event->pos.y = (F32)p.y;
        event->delta = v2f32(0.f, -(F32)wheel_delta);
      }break;
      
      case WM_MOUSEHWHEEL:
      {
        S16 wheel_delta = HIWORD(wParam);
        WM_Event *event = w32_wm_push_event(WM_EventKind_Scroll, window);
        POINT p;
        p.x = (S32)(S16)LOWORD(lParam);
        p.y = (S32)(S16)HIWORD(lParam);
        ScreenToClient(window->hwnd, &p);
        event->pos.x = (F32)p.x;
        event->pos.y = (F32)p.y;
        event->delta = v2f32((F32)wheel_delta, 0.f);
      }break;
      
      case WM_SYSKEYDOWN: case WM_SYSKEYUP:
      {
        if(wParam != VK_MENU && (wParam < VK_F1 || VK_F24 < wParam || wParam == VK_F4))
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      } // fallthrough;
      case WM_KEYDOWN: case WM_KEYUP:
      {
        B32 was_down = (lParam & bit31);
        B32 is_down  = !(lParam & bit32);
        
        B32 is_repeat = 0;
        if(!is_down)
        {
          release = 1;
        }
        else if(was_down)
        {
          is_repeat = 1;
        }
        
        B32 right_sided = 0;
        if ((lParam & bit25) &&
            (wParam == VK_CONTROL || wParam == VK_RCONTROL ||
             wParam == VK_MENU || wParam == VK_RMENU ||
             wParam == VK_SHIFT || wParam == VK_RSHIFT))
        {
          right_sided = 1;
        }
        
        WM_Event *event = w32_wm_push_event(release ? WM_EventKind_Release : WM_EventKind_Press, window);
        event->key = w32_wm_os_key_from_vkey(wParam);
        event->repeat_count = lParam & bitmask16;
        event->is_repeat = is_repeat;
        event->right_sided = right_sided;
        if(event->key == WM_Key_Alt   && event->modifiers & WM_Modifier_Alt)   { event->modifiers &= ~WM_Modifier_Alt; }
        if(event->key == WM_Key_Ctrl  && event->modifiers & WM_Modifier_Ctrl)  { event->modifiers &= ~WM_Modifier_Ctrl; }
        if(event->key == WM_Key_Shift && event->modifiers & WM_Modifier_Shift) { event->modifiers &= ~WM_Modifier_Shift; }
      }break;
      
      case WM_SYSCHAR:
      {
        WORD vk_code = LOWORD(wParam);
        if(vk_code == VK_SPACE)
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        else
        {
          result = 0;
        }
      }break;
      
      case WM_CHAR:
      {
        U32 character = wParam;
        if(character >= 32 && character != 127)
        {
          WM_Event *event = w32_wm_push_event(WM_EventKind_Text, window);
          if(lParam & bit29)
          {
            event->modifiers |= WM_Modifier_Alt;
          }
          event->character = character;
        }
      }break;
      
      case WM_KILLFOCUS:
      {
        w32_wm_push_event(WM_EventKind_WindowLoseFocus, window);
        ReleaseCapture();
      }break;
      
      case WM_SETCURSOR:
      {
        Rng2F32 window_rect = wm_client_rect_from_window(window_handle);
        Vec2F32 mouse = wm_mouse_from_window(window_handle);
        B32 on_border = 0;
        DWORD window_style = window ? GetWindowLong(window->hwnd, GWL_STYLE) : 0;
        B32 is_fullscreen = !(window_style & WS_OVERLAPPEDWINDOW);
        if(window != 0 && window->custom_border && !is_fullscreen)
        {
          B32 on_border_x = (mouse.x <= window->custom_border_edge_thickness || window_rect.x1-window->custom_border_edge_thickness <= mouse.x);
          B32 on_border_y = (mouse.y <= window->custom_border_edge_thickness || window_rect.y1-window->custom_border_edge_thickness <= mouse.y);
          on_border = on_border_x || on_border_y;
        }
        if(!w32_wm_resizing && !on_border && contains_2f32(window_rect, mouse))
        {
          SetCursor(w32_wm_state->hCursor);
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      
      case WM_DPICHANGED:
      {
        F32 new_dpi = (F32)(wParam & 0xffff);
        RECT suggested_new_rect = *(RECT *)lParam;
        window->dpi = new_dpi;
        SetWindowPos(window->hwnd, 0,
                     suggested_new_rect.left,
                     suggested_new_rect.top,
                     suggested_new_rect.right - suggested_new_rect.left,
                     suggested_new_rect.bottom - suggested_new_rect.top,
                     0);
      }break;
      
      //- rjf: [file drop]
      case WM_DROPFILES:
      {
        HDROP drop = (HDROP)wParam;
        POINT drop_pt = {0};
        DragQueryPoint(drop, &drop_pt);
        U64 num_files_dropped = DragQueryFile(drop, 0xffffffff, 0, 0);
        WM_Event *event = w32_wm_push_event(WM_EventKind_FileDrop, window);
        event->pos = v2f32((F32)drop_pt.x, (F32)drop_pt.y);
        for(U64 idx = 0; idx < num_files_dropped; idx += 1)
        {
          U64 name_size = DragQueryFile(drop, idx, 0, 0) + 1;
          U8 *name_ptr = push_array(w32_wm_event_arena, U8, name_size);
          DragQueryFile(drop, idx, (char *)name_ptr, name_size);
          String8 path_string = str8(name_ptr, name_size - 1);
          String8 path_string__normalized = path_normalized_from_string(w32_wm_event_arena, path_string);
          str8_list_push(w32_wm_event_arena, &event->strings, path_string__normalized);
        }
        DragFinish(drop);
      }break;
      
      //- rjf: [custom border]
      case WM_NCPAINT:
      {
        if(w32_wm_new_window_custom_border || (window != 0 && window->custom_border && !window->custom_border_composition_enabled))
        {
          result = 0;
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      case WM_DWMCOMPOSITIONCHANGED:
      {
        if(window != 0 && window->custom_border)
        {
          BOOL enabled = 0;
          DwmIsCompositionEnabled(&enabled);
          window->custom_border_composition_enabled = enabled;
          if(enabled)
          {
            MARGINS m = { 0, 0, 1, 0 };
            DwmExtendFrameIntoClientArea(hwnd, &m);
            DWORD dwmncrp_enabled = DWMNCRP_ENABLED;
            DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &enabled, sizeof(dwmncrp_enabled));
          }
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      case WM_WINDOWPOSCHANGED:
      {
        result = 0;
      }break;
      case WM_NCUAHDRAWCAPTION:
      case WM_NCUAHDRAWFRAME:
      {
        // NOTE(rjf): undocumented messages for drawing themed window borders.
        if(w32_wm_new_window_custom_border || (window != 0 && window->custom_border))
        {
          result = 0;
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      case WM_SETICON:
      case WM_SETTEXT:
      {
        if(w32_wm_new_window_custom_border || (window && window->custom_border && !window->custom_border_composition_enabled))
        {
          // NOTE(rjf):
          // https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/
          LONG_PTR old_style = GetWindowLongPtrW(hwnd, GWL_STYLE);
          SetWindowLongPtrW(hwnd, GWL_STYLE, old_style & ~WS_VISIBLE);
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
          SetWindowLongPtrW(hwnd, GWL_STYLE, old_style);
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      
      //- rjf: [custom border] activation - without this `result`, stuff flickers.
      case WM_NCACTIVATE:
      {
        if(!w32_wm_new_window_custom_border && (window == 0 || window->custom_border == 0))
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, -1);
        }
      }break;
      
      //- rjf: [custom border] client/window size calculation
      case WM_NCCALCSIZE:
      {
        if(w32_wm_new_window_custom_border || (window && window->custom_border))
        {
          F32 dpi = w32_GetDpiForWindow_func ? (F32)w32_GetDpiForWindow_func(hwnd) : 96.f;
          S32 frame_x = w32_GetSystemMetricsForDpi_func ? w32_GetSystemMetricsForDpi_func(SM_CXFRAME, dpi) : GetSystemMetrics(SM_CXFRAME);
          S32 frame_y = w32_GetSystemMetricsForDpi_func ? w32_GetSystemMetricsForDpi_func(SM_CYFRAME, dpi) : GetSystemMetrics(SM_CYFRAME);
          S32 padding = w32_GetSystemMetricsForDpi_func ? w32_GetSystemMetricsForDpi_func(SM_CXPADDEDBORDER, dpi) : GetSystemMetrics(SM_CXPADDEDBORDER);
          DWORD window_style = GetWindowLong(hwnd, GWL_STYLE);
          B32 is_fullscreen = !(window_style & WS_OVERLAPPEDWINDOW);
          if(!is_fullscreen)
          {
            RECT* rect = wParam == 0 ? (RECT*)lParam : ((NCCALCSIZE_PARAMS*)lParam)->rgrc;
            rect->right  -= frame_x + padding;
            rect->left   += frame_x + padding;
            rect->bottom -= frame_y + padding;
            
            if(IsMaximized(hwnd))
            {
              rect->top += frame_y + padding;
              // If we do not do this hidden taskbar can not be unhidden on mouse hover
              // Unfortunately it can create an ugly bottom border when maximized...
              rect->bottom -= 1; 
            }
          }
        }
        else
        {
          result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
      }break;
      
      //- rjf: [custom border] client/window hit testing (mapping mouse -> action)
      case WM_NCHITTEST:
      {
        DWORD window_style = window ? GetWindowLong(window->hwnd, GWL_STYLE) : 0;
        B32 is_fullscreen = !(window_style & WS_OVERLAPPEDWINDOW);
        if(window == 0 || window->custom_border == 0 || is_fullscreen)
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        else
        {
          B32 is_default_handled = 0;
          
          // Let the default procedure handle resizing areas
          result = DefWindowProc(hwnd, uMsg, wParam, lParam);
          switch (result)
          {
            case HTNOWHERE:
            case HTRIGHT:
            case HTLEFT:
            case HTTOPLEFT:
            case HTTOPRIGHT:
            case HTBOTTOMRIGHT:
            case HTBOTTOM:
            case HTBOTTOMLEFT:
            {
              is_default_handled = 1;
            } break;
          }
          
          if (!is_default_handled)
          {
            POINT pos_monitor;
            pos_monitor.x = GET_X_LPARAM(lParam);
            pos_monitor.y = GET_Y_LPARAM(lParam);
            POINT pos_client = pos_monitor;
            ScreenToClient(hwnd, &pos_client);
            
            // Adjustments happening in NCCALCSIZE are messing with the detection
            // of the top hit area so manually checking that.
            F32 dpi = w32_GetDpiForWindow_func ? (F32)w32_GetDpiForWindow_func(hwnd) : 96.f;
            S32 frame_y = w32_GetSystemMetricsForDpi_func ? w32_GetSystemMetricsForDpi_func(SM_CYFRAME, dpi) : GetSystemMetrics(SM_CYFRAME);
            // NOTE(rjf): it seems incorrect to apply this padding here...
            // S32 padding = w32_GetSystemMetricsForDpi_func ? w32_GetSystemMetricsForDpi_func(SM_CXPADDEDBORDER, dpi) : GetSystemMetrics(SM_CXPADDEDBORDER);
            
            B32 is_over_top_resize = pos_client.y >= 0 && pos_client.y < frame_y; // + padding;
            B32 is_over_title_bar  = pos_client.y >= 0 && pos_client.y < window->custom_border_title_thickness;
            
            //- rjf: check against title bar client areas
            B32 is_over_title_bar_client_area = 0;
            for(W32_WM_TitleBarClientArea *area = window->first_title_bar_client_area;
                area != 0;
                area = area->next)
            {
              Rng2F32 rect = area->rect;
              if(rect.x0 <= pos_client.x && pos_client.x < rect.x1 &&
                 rect.y0 <= pos_client.y && pos_client.y < rect.y1)
              {
                is_over_title_bar_client_area = 1;
                break;
              }
            }
            
            if (IsMaximized(hwnd))
            {
              if (is_over_title_bar_client_area)
              {
                result = HTCLIENT;
              }
              else if (is_over_title_bar)
              {
                result = HTCAPTION;
              }
              else 
              {
                result = HTCLIENT;
              }
            }
            else
            {
              //Swap the first two conditions to choose if hovering the top border
              //should prioritize resize or title bar buttons.
              if (is_over_title_bar_client_area)
              {
                result = HTCLIENT;
              }
              else if (is_over_top_resize)
              {
                result = HTTOP;
              }
              else if (is_over_title_bar)
              {
                result = HTCAPTION;
              }
              else {
                result = HTCLIENT;
              }
            }
          }
        }
      }break;
    }
  }
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: Monitors

internal BOOL
w32_wm_monitor_gather_enum_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM bundle_ptr)
{
  W32_WM_MonitorGatherBundle *bundle = (W32_WM_MonitorGatherBundle *)bundle_ptr;
  WM_Monitor m = {(U64)monitor};
  W32_WM_MonitorGatherNode *n = push_array(bundle->arena, W32_WM_MonitorGatherNode, 1);
  n->v = m;
  SLLQueuePush(bundle->first_monitor, bundle->last_monitor, n);
  bundle->monitor_count += 1;
  return 1;
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
wm_init(void)
{
  //- rjf: set up base shared state
  Arena *arena = arena_alloc();
  w32_wm_state = push_array(arena, W32_WM_State, 1);
  w32_wm_state->arena = arena;
  w32_wm_state->gfx_thread_tid = (U32)GetCurrentThreadId();
  w32_wm_state->hInstance = GetModuleHandle(0);
  
  //- rjf: set dpi awareness
  w32_SetProcessDpiAwarenessContext_Type *SetProcessDpiAwarenessContext_func = 0;
  HMODULE module = LoadLibraryA("user32.dll");
  if(module != 0)
  {
    SetProcessDpiAwarenessContext_func =
    (w32_SetProcessDpiAwarenessContext_Type*)GetProcAddress(module, "SetProcessDpiAwarenessContext");
    w32_GetDpiForWindow_func =
    (w32_GetDpiForWindow_Type*)GetProcAddress(module, "GetDpiForWindow");
    w32_GetDpiForMonitor_func = (w32_GetDpiForMonitor_Type *)GetProcAddress(module, "GetDpiForMonitor");
    w32_GetSystemMetricsForDpi_func = (w32_GetSystemMetricsForDpi_Type *)GetProcAddress(module, "GetSystemMetricsForDpi");
    FreeLibrary(module);
  }
  if(SetProcessDpiAwarenessContext_func != 0)
  {
    SetProcessDpiAwarenessContext_func(w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }
  else
  {
    HMODULE shcore = LoadLibraryA("shcore.dll");
    if(shcore)
    {
      typedef HRESULT (WINAPI* SetProcessDpiAwareness_t)(int);
      SetProcessDpiAwareness_t SetProcessDpiAwareness = (void*)GetProcAddress(shcore, "SetProcessDpiAwareness");
      if(SetProcessDpiAwareness)
      {
        SetProcessDpiAwareness(2);
      }
      FreeLibrary(shcore);
    }
    SetProcessDPIAware();
  }
  
  //- rjf: register graphical-window class
  {
    WNDCLASSEXW wndclass = {sizeof(wndclass)};
    wndclass.lpfnWndProc = w32_wm_wnd_proc;
    wndclass.hInstance = w32_wm_state->hInstance;
    wndclass.lpszClassName = L"graphical-window";
    wndclass.hCursor = LoadCursorA(0, IDC_ARROW);
    wndclass.hIcon = LoadIcon(w32_wm_state->hInstance, MAKEINTRESOURCE(1));
    wndclass.style = CS_VREDRAW|CS_HREDRAW;
    ATOM wndatom = RegisterClassExW(&wndclass);
    (void)wndatom;
  }
  
  //- rjf: grab graphics system info
  {
    w32_wm_state->gfx_info.double_click_time = GetDoubleClickTime()/1000.f;
    w32_wm_state->gfx_info.caret_blink_time = GetCaretBlinkTime()/1000.f;
    DEVMODEW devmodew = {0};
    if(EnumDisplaySettingsW(0, ENUM_CURRENT_SETTINGS, &devmodew))
    {
      w32_wm_state->gfx_info.default_refresh_rate = (F32)devmodew.dmDisplayFrequency;
    }
  }
  
  //- rjf: set initial cursor
  wm_set_cursor(WM_Cursor_Pointer);
  
  //- rjf: fill vkey -> WM_Key table
  {
    w32_wm_state->key_from_vkey_table[(unsigned int)'A'] = WM_Key_A;
    w32_wm_state->key_from_vkey_table[(unsigned int)'B'] = WM_Key_B;
    w32_wm_state->key_from_vkey_table[(unsigned int)'C'] = WM_Key_C;
    w32_wm_state->key_from_vkey_table[(unsigned int)'D'] = WM_Key_D;
    w32_wm_state->key_from_vkey_table[(unsigned int)'E'] = WM_Key_E;
    w32_wm_state->key_from_vkey_table[(unsigned int)'F'] = WM_Key_F;
    w32_wm_state->key_from_vkey_table[(unsigned int)'G'] = WM_Key_G;
    w32_wm_state->key_from_vkey_table[(unsigned int)'H'] = WM_Key_H;
    w32_wm_state->key_from_vkey_table[(unsigned int)'I'] = WM_Key_I;
    w32_wm_state->key_from_vkey_table[(unsigned int)'J'] = WM_Key_J;
    w32_wm_state->key_from_vkey_table[(unsigned int)'K'] = WM_Key_K;
    w32_wm_state->key_from_vkey_table[(unsigned int)'L'] = WM_Key_L;
    w32_wm_state->key_from_vkey_table[(unsigned int)'M'] = WM_Key_M;
    w32_wm_state->key_from_vkey_table[(unsigned int)'N'] = WM_Key_N;
    w32_wm_state->key_from_vkey_table[(unsigned int)'O'] = WM_Key_O;
    w32_wm_state->key_from_vkey_table[(unsigned int)'P'] = WM_Key_P;
    w32_wm_state->key_from_vkey_table[(unsigned int)'Q'] = WM_Key_Q;
    w32_wm_state->key_from_vkey_table[(unsigned int)'R'] = WM_Key_R;
    w32_wm_state->key_from_vkey_table[(unsigned int)'S'] = WM_Key_S;
    w32_wm_state->key_from_vkey_table[(unsigned int)'T'] = WM_Key_T;
    w32_wm_state->key_from_vkey_table[(unsigned int)'U'] = WM_Key_U;
    w32_wm_state->key_from_vkey_table[(unsigned int)'V'] = WM_Key_V;
    w32_wm_state->key_from_vkey_table[(unsigned int)'W'] = WM_Key_W;
    w32_wm_state->key_from_vkey_table[(unsigned int)'X'] = WM_Key_X;
    w32_wm_state->key_from_vkey_table[(unsigned int)'Y'] = WM_Key_Y;
    w32_wm_state->key_from_vkey_table[(unsigned int)'Z'] = WM_Key_Z;
    
    for(U64 i = '0', j = WM_Key_0; i <= '9'; i += 1, j += 1)
    {
      w32_wm_state->key_from_vkey_table[i] = (WM_Key)j;
    }
    for(U64 i = VK_NUMPAD0, j = WM_Key_0; i <= VK_NUMPAD9; i += 1, j += 1)
    {
      w32_wm_state->key_from_vkey_table[i] = (WM_Key)j;
    }
    for(U64 i = VK_F1, j = WM_Key_F1; i <= VK_F24; i += 1, j += 1)
    {
      w32_wm_state->key_from_vkey_table[i] = (WM_Key)j;
    }
    
    w32_wm_state->key_from_vkey_table[VK_SPACE]     = WM_Key_Space;
    w32_wm_state->key_from_vkey_table[VK_OEM_3]     = WM_Key_Tick;
    w32_wm_state->key_from_vkey_table[VK_OEM_MINUS] = WM_Key_Minus;
    w32_wm_state->key_from_vkey_table[VK_OEM_PLUS]  = WM_Key_Equal;
    w32_wm_state->key_from_vkey_table[VK_OEM_4]     = WM_Key_LeftBracket;
    w32_wm_state->key_from_vkey_table[VK_OEM_6]     = WM_Key_RightBracket;
    w32_wm_state->key_from_vkey_table[VK_OEM_1]     = WM_Key_Semicolon;
    w32_wm_state->key_from_vkey_table[VK_OEM_7]     = WM_Key_Quote;
    w32_wm_state->key_from_vkey_table[VK_OEM_COMMA] = WM_Key_Comma;
    w32_wm_state->key_from_vkey_table[VK_OEM_PERIOD]= WM_Key_Period;
    w32_wm_state->key_from_vkey_table[VK_OEM_2]     = WM_Key_Slash;
    w32_wm_state->key_from_vkey_table[VK_OEM_5]     = WM_Key_BackSlash;
    
    w32_wm_state->key_from_vkey_table[VK_TAB]       = WM_Key_Tab;
    w32_wm_state->key_from_vkey_table[VK_PAUSE]     = WM_Key_Pause;
    w32_wm_state->key_from_vkey_table[VK_ESCAPE]    = WM_Key_Esc;
    
    w32_wm_state->key_from_vkey_table[VK_UP]        = WM_Key_Up;
    w32_wm_state->key_from_vkey_table[VK_LEFT]      = WM_Key_Left;
    w32_wm_state->key_from_vkey_table[VK_DOWN]      = WM_Key_Down;
    w32_wm_state->key_from_vkey_table[VK_RIGHT]     = WM_Key_Right;
    
    w32_wm_state->key_from_vkey_table[VK_BACK]      = WM_Key_Backspace;
    w32_wm_state->key_from_vkey_table[VK_RETURN]    = WM_Key_Return;
    
    w32_wm_state->key_from_vkey_table[VK_DELETE]    = WM_Key_Delete;
    w32_wm_state->key_from_vkey_table[VK_INSERT]    = WM_Key_Insert;
    w32_wm_state->key_from_vkey_table[VK_PRIOR]     = WM_Key_PageUp;
    w32_wm_state->key_from_vkey_table[VK_NEXT]      = WM_Key_PageDown;
    w32_wm_state->key_from_vkey_table[VK_HOME]      = WM_Key_Home;
    w32_wm_state->key_from_vkey_table[VK_END]       = WM_Key_End;
    
    w32_wm_state->key_from_vkey_table[VK_CAPITAL]   = WM_Key_CapsLock;
    w32_wm_state->key_from_vkey_table[VK_NUMLOCK]   = WM_Key_NumLock;
    w32_wm_state->key_from_vkey_table[VK_SCROLL]    = WM_Key_ScrollLock;
    w32_wm_state->key_from_vkey_table[VK_APPS]      = WM_Key_Menu;
    
    w32_wm_state->key_from_vkey_table[VK_CONTROL]   = WM_Key_Ctrl;
    w32_wm_state->key_from_vkey_table[VK_LCONTROL]  = WM_Key_Ctrl;
    w32_wm_state->key_from_vkey_table[VK_RCONTROL]  = WM_Key_Ctrl;
    w32_wm_state->key_from_vkey_table[VK_SHIFT]     = WM_Key_Shift;
    w32_wm_state->key_from_vkey_table[VK_LSHIFT]    = WM_Key_Shift;
    w32_wm_state->key_from_vkey_table[VK_RSHIFT]    = WM_Key_Shift;
    w32_wm_state->key_from_vkey_table[VK_MENU]      = WM_Key_Alt;
    w32_wm_state->key_from_vkey_table[VK_LMENU]     = WM_Key_Alt;
    w32_wm_state->key_from_vkey_table[VK_RMENU]     = WM_Key_Alt;
    
    w32_wm_state->key_from_vkey_table[VK_DIVIDE]   = WM_Key_NumSlash;
    w32_wm_state->key_from_vkey_table[VK_MULTIPLY] = WM_Key_NumStar;
    w32_wm_state->key_from_vkey_table[VK_SUBTRACT] = WM_Key_NumMinus;
    w32_wm_state->key_from_vkey_table[VK_ADD]      = WM_Key_NumPlus;
    w32_wm_state->key_from_vkey_table[VK_DECIMAL]  = WM_Key_NumPeriod;
    
    for(U32 i = 0; i < 10; i += 1)
    {
      w32_wm_state->key_from_vkey_table[VK_NUMPAD0 + i] = (WM_Key)((U64)WM_Key_Num0 + i);
    }
    
    for(U64 i = 0xDF, j = 0; i < 0xFF; i += 1, j += 1)
    {
      w32_wm_state->key_from_vkey_table[i] = (WM_Key)((U64)WM_Key_Ex0 + j);
    }
  }
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal WM_SystemInfo *
wm_get_system_info(void)
{
  return &w32_wm_state->gfx_info;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
wm_set_clipboard_text(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  if(OpenClipboard(0))
  {
    EmptyClipboard();
    String16 string16 = str16_from_8(scratch.arena, string);
    HANDLE string16_copy_handle = GlobalAlloc(GMEM_MOVEABLE, (string16.size+1)*sizeof(string16.str[0]));
    if(string16_copy_handle)
    {
      U16 *copy_buffer = (U16 *)GlobalLock(string16_copy_handle);
      MemoryCopy(copy_buffer, string16.str, string16.size*sizeof(string16.str[0]));
      copy_buffer[string16.size] = 0;
      GlobalUnlock(string16_copy_handle);
      SetClipboardData(CF_UNICODETEXT, string16_copy_handle);
    }
    CloseClipboard();
  }
  scratch_end(scratch);
}

internal String8
wm_get_clipboard_text(Arena *arena)
{
  String8 result = {0};
  if(IsClipboardFormatAvailable(CF_UNICODETEXT) &&
     OpenClipboard(0))
  {
    HANDLE data_handle = GetClipboardData(CF_UNICODETEXT);
    if(data_handle)
    {
      U16 *buffer = (U16 *)GlobalLock(data_handle);
      if(buffer)
      {
        U64 size = cstring16_length(buffer);
        String16 string16 = str16(buffer, size);
        result = str8_from_16(arena, string16);
        GlobalUnlock(data_handle);
      }
    }
    CloseClipboard();
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal WM_Window
wm_window_open(Rng2F32 rect, WM_WindowFlags flags, String8 title)
{
  B32 custom_border = !!(flags & WM_WindowFlag_CustomBorder);
  B32 use_default_position = !!(flags & WM_WindowFlag_UseDefaultPosition);
  Vec2F32 pos = rect.p0;
  Vec2F32 dim = dim_2f32(rect);
  
  //- rjf: make hwnd
  HWND hwnd = 0;
  {
    Temp scratch = scratch_begin(0, 0);
    String16 title16 = str16_from_8(scratch.arena, title);
    w32_wm_new_window_custom_border = custom_border;
    DWORD style_flags = WS_EX_APPWINDOW;
#if defined(R_BACKEND) && R_BACKEND != R_BACKEND_OPENGL
    style_flags |= WS_EX_NOREDIRECTIONBITMAP;
#endif
    hwnd = CreateWindowExW(style_flags,
                           L"graphical-window",
                           (WCHAR*)title16.str,
                           WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
                           use_default_position ? CW_USEDEFAULT : (S32)pos.x,
                           use_default_position ? CW_USEDEFAULT : (S32)pos.y,
                           (S32)dim.x,
                           (S32)dim.y,
                           0, 0,
                           w32_wm_state->hInstance,
                           0);
    DragAcceptFiles(hwnd, 1);
    w32_wm_new_window_custom_border = 0;
    scratch_end(scratch);
  }
  
  //- rjf- make/fill window
  W32_WM_Window *window = w32_wm_window_alloc();
  {
    window->hwnd = hwnd;
    window->hdc = GetDC(hwnd);
    if(w32_GetDpiForWindow_func != 0)
    {
      window->dpi = (F32)w32_GetDpiForWindow_func(hwnd);
    }
    else
    {
      window->dpi = 96.f;
    }
  }
  
  //- rjf: early detection of composition
  {
    BOOL enabled = 0;
    DwmIsCompositionEnabled(&enabled);
    window->custom_border_composition_enabled = enabled;
  }
  
  //- rjf: custom border
  if(flags & WM_WindowFlag_CustomBorder)
  {
    window->custom_border = 1;
    window->paint_arena = arena_alloc();
  }
  
  //- rjf: convert to handle + return
  WM_Window result = w32_wm_handle_from_window(window);
  return result;
}

internal void
wm_window_close(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  w32_wm_window_release(window);
}

internal void
wm_window_set_title(WM_Window handle, String8 title)
{
  Temp scratch = scratch_begin(0, 0);
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  String16 title16 = str16_from_8(scratch.arena, title);
  SetWindowTextW(window->hwnd, (WCHAR *)title16.str);
  scratch_end(scratch);
}

internal void
wm_window_first_paint(WM_Window window_handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(window_handle);
  window->first_paint_done = 1;
  ShowWindow(window->hwnd, SW_SHOW);
  if(window->maximized)
  {
    ShowWindow(window->hwnd, SW_MAXIMIZE);
  }
}

internal void
wm_window_focus(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  SetForegroundWindow(window->hwnd);
  SetFocus(window->hwnd);
}

internal B32
wm_window_is_focused(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  HWND active_hwnd = GetForegroundWindow();
  return active_hwnd == window->hwnd;
}

internal B32
wm_window_is_fullscreen(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  DWORD window_style = GetWindowLong(window->hwnd, GWL_STYLE);
  return !(window_style & WS_OVERLAPPEDWINDOW);
}

internal void
wm_window_set_fullscreen(WM_Window handle, B32 fullscreen)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  DWORD window_style = GetWindowLong(window->hwnd, GWL_STYLE);
  B32 is_fullscreen_already = wm_window_is_fullscreen(handle);
  if(fullscreen)
  {
    if(!is_fullscreen_already)
    {
      GetWindowPlacement(window->hwnd, &window->last_window_placement);
    }
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    if(GetMonitorInfo(MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
    {
      SetWindowLong(window->hwnd, GWL_STYLE, window_style & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(window->hwnd, HWND_TOP,
                   monitor_info.rcMonitor.left,
                   monitor_info.rcMonitor.top,
                   monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                   monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  }
  else
  {
    SetWindowLong(window->hwnd, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(window->hwnd, &window->last_window_placement);
    SetWindowPos(window->hwnd, 0, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

internal B32
wm_window_is_maximized(WM_Window handle)
{
  B32 result = 0;
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window)
  {
    result = !!(IsZoomed(window->hwnd));
  }
  return result;
}

internal void
wm_window_set_maximized(WM_Window handle, B32 maximized)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0)
  {
    if(window->first_paint_done)
    {
      switch(maximized)
      {
        default:
        case 0:{ShowWindow(window->hwnd, SW_RESTORE);}break;
        case 1:{ShowWindow(window->hwnd, SW_MAXIMIZE);}break;
      }
    }
    else
    {
      window->maximized = maximized;
    }
  }
}

internal B32
wm_window_is_minimized(WM_Window handle)
{
  B32 result = 0;
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window)
  {
    result = !!(IsIconic(window->hwnd));
  }
  return result;
}

internal void
wm_window_set_minimized(WM_Window handle, B32 minimized)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0 && minimized != wm_window_is_minimized(handle))
  {
    switch(minimized)
    {
      default:
      case 0:{ShowWindow(window->hwnd, SW_RESTORE);}break;
      case 1:{ShowWindow(window->hwnd, SW_MINIMIZE);}break;
    }
  }
}

internal void
wm_window_bring_to_front(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0)
  {
    BringWindowToTop(window->hwnd);
  }
}

internal void
wm_window_set_monitor(WM_Window window_handle, WM_Monitor monitor)
{
  W32_WM_Window *window = w32_wm_window_from_handle(window_handle);
  HMONITOR hmonitor = (HMONITOR)monitor.u64[0];
  {
    MONITORINFOEXW info;
    info.cbSize = sizeof(MONITORINFOEXW);
    if(GetMonitorInfoW(hmonitor, (MONITORINFO *)&info))
    {
      Rng2F32 existing_rect = wm_rect_from_window(window_handle);
      Vec2F32 window_size = dim_2f32(existing_rect);
      SetWindowPos(window->hwnd, HWND_TOP,
                   (info.rcWork.left + info.rcWork.right)/2 - window_size.x/2,
                   (info.rcWork.top + info.rcWork.bottom)/2 - window_size.y/2,
                   window_size.x,
                   window_size.y, 0);
    }
  }
}

internal void
wm_window_clear_custom_border_data(WM_Window handle)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window->custom_border)
  {
    arena_clear(window->paint_arena);
    window->first_title_bar_client_area = window->last_title_bar_client_area = 0;
    window->custom_border_title_thickness = 0;
    window->custom_border_edge_thickness = 0;
  }
}

internal void
wm_window_push_custom_title_bar(WM_Window handle, F32 thickness)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  window->custom_border_title_thickness = thickness;
}

internal void
wm_window_push_custom_edges(WM_Window handle, F32 thickness)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  window->custom_border_edge_thickness = thickness;
}

internal void
wm_window_push_custom_title_bar_client_area(WM_Window handle, Rng2F32 rect)
{
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window->custom_border)
  {
    W32_WM_TitleBarClientArea *area = push_array(window->paint_arena, W32_WM_TitleBarClientArea, 1);
    if(area != 0)
    {
      area->rect = rect;
      SLLQueuePush(window->first_title_bar_client_area, window->last_title_bar_client_area, area);
    }
  }
}

internal Rng2F32
wm_rect_from_window(WM_Window handle)
{
  Rng2F32 r = {0};
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window)
  {
    RECT rect = {0};
    GetWindowRect(w32_wm_hwnd_from_window(window), &rect);
    r = w32_wm_rng2f32_from_rect(rect);
  }
  return r;
}

internal Rng2F32
wm_client_rect_from_window(WM_Window handle)
{
  Rng2F32 r = {0};
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window)
  {
    RECT rect = {0};
    GetClientRect(w32_wm_hwnd_from_window(window), &rect);
    r = w32_wm_rng2f32_from_rect(rect);
  }
  return r;
}

internal F32
wm_dpi_from_window(WM_Window handle)
{
  F32 result = 96.f;
  W32_WM_Window *window = w32_wm_window_from_handle(handle);
  if(window != 0)
  {
    result = window->dpi;
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks External Windows (Implemented Per-OS)

internal WM_ExtWindow
wm_focused_external_window(void)
{
  HWND hwnd = GetForegroundWindow();
  WM_ExtWindow result = {(U64)hwnd};
  return result;
}

internal void
wm_focus_external_window(WM_ExtWindow handle)
{
  HWND hwnd = (HWND)handle.u64[0];
  if(hwnd != 0)
  {
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
  }
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal WM_MonitorArray
wm_push_monitors_array(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  W32_WM_MonitorGatherBundle bundle = {arena};
  EnumDisplayMonitors(0, 0, w32_wm_monitor_gather_enum_proc, (LPARAM)&bundle);
  WM_MonitorArray array = {0};
  array.count = bundle.monitor_count;
  array.v = push_array(arena, WM_Monitor, array.count);
  {
    U64 idx = 0;
    for EachNode(n, W32_WM_MonitorGatherNode, bundle.first_monitor)
    {
      array.v[idx] = n->v;
      idx += 1;
    }
  }
  scratch_end(scratch);
  return array;
}

internal WM_Monitor
wm_primary_monitor(void)
{
  POINT zero_pt = {0, 0};
  HMONITOR monitor = MonitorFromPoint(zero_pt, MONITOR_DEFAULTTOPRIMARY);
  WM_Monitor result = {(U64)monitor};
  return result;
}

internal WM_Monitor
wm_monitor_from_window(WM_Window window)
{
  W32_WM_Window *w = w32_wm_window_from_handle(window);
  HMONITOR handle = MonitorFromWindow(w->hwnd, MONITOR_DEFAULTTOPRIMARY);
  WM_Monitor result = {(U64)handle};
  return result;
}

internal String8
wm_name_from_monitor(Arena *arena, WM_Monitor monitor)
{
  String8 result = {0};
  HMONITOR monitor_handle = (HMONITOR)monitor.u64[0];
  MONITORINFOEXW info;
  info.cbSize = sizeof(MONITORINFOEXW);
  if(GetMonitorInfoW(monitor_handle, (MONITORINFO *)&info))
  {
    String16 result16 = str16_cstring((U16 *)info.szDevice);
    result = str8_from_16(arena, result16);
  }
  return result;
}

internal Vec2F32
wm_dim_from_monitor(WM_Monitor monitor)
{
  Vec2F32 result = {0};
  HMONITOR monitor_handle = (HMONITOR)monitor.u64[0];
  MONITORINFO info = {0};
  info.cbSize = sizeof(MONITORINFO);
  if(GetMonitorInfoW(monitor_handle, &info))
  {
    result.x = info.rcWork.right - info.rcWork.left;
    result.y = info.rcWork.bottom - info.rcWork.top;
  }
  return result;
}

internal F32
wm_dpi_from_monitor(WM_Monitor monitor)
{
  F32 result = 96.f;
  HMONITOR monitor_handle = (HMONITOR)monitor.u64[0];
  if(w32_GetDpiForMonitor_func != 0)
  {
    UINT dpi_x = 0;
    UINT dpi_y = 0;
    HRESULT hr = w32_GetDpiForMonitor_func(monitor_handle, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
    result = (F32)dpi_x;
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
wm_send_wakeup_event(void)
{
  PostThreadMessageA(w32_wm_state->gfx_thread_tid, 0x401, 0, 0);
}

internal WM_EventList
wm_get_events(Arena *arena, B32 wait)
{
  w32_wm_event_arena = arena;
  MemoryZeroStruct(&w32_wm_event_list);
  MSG msg = {0};
  if(!wait || GetMessage(&msg, 0, 0, 0))
  {
    B32 first_wait = wait;
    for(;first_wait || PeekMessage(&msg, 0, 0, 0, PM_REMOVE); first_wait = 0)
    {
      DispatchMessage(&msg);
      TranslateMessage(&msg);
      if(msg.message == WM_QUIT)
      {
        w32_wm_push_event(WM_EventKind_WindowClose, 0);
      }
    }
  }
  return w32_wm_event_list;
}

internal WM_Modifiers
wm_get_modifiers(void)
{
  WM_Modifiers modifiers = 0;
  if(GetKeyState(VK_CONTROL) & 0x8000)
  {
    modifiers |= WM_Modifier_Ctrl;
  }
  if(GetKeyState(VK_SHIFT) & 0x8000)
  {
    modifiers |= WM_Modifier_Shift;
  }
  if(GetKeyState(VK_MENU) & 0x8000)
  {
    modifiers |= WM_Modifier_Alt;
  }
  return modifiers;
}

internal B32
wm_key_is_down(WM_Key key)
{
  B32 down = 0;
  WPARAM vkey = w32_wm_vkey_from_os_key(key);
  if(GetKeyState(vkey) & 0x8000)
  {
    down = 1;
  }
  return down;
}

internal Vec2F32
wm_mouse_from_window(WM_Window handle)
{
  ProfBeginFunction();
  Vec2F32 v = {0};
  POINT p;
  if(GetCursorPos(&p))
  {
    W32_WM_Window *window = w32_wm_window_from_handle(handle);
    ScreenToClient(window->hwnd, &p);
    v.x = (F32)p.x;
    v.y = (F32)p.y;
  }
  ProfEnd();
  return v;
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
wm_set_cursor(WM_Cursor cursor)
{
  B32 valid_cursor = 1;
  HCURSOR hcursor = 0;
  switch(cursor)
  {
    default: {valid_cursor = 0;}break;
#define Win32CursorXList(X) \
X(Pointer, IDC_ARROW) \
X(IBar, IDC_IBEAM) \
X(LeftRight, IDC_SIZEWE) \
X(UpDown, IDC_SIZENS) \
X(DownRight, IDC_SIZENWSE) \
X(UpRight, IDC_SIZENESW) \
X(UpDownLeftRight, IDC_SIZEALL) \
X(HandPoint, IDC_HAND)\
X(Disabled, IDC_NO)
#define CursorCase(E,R) case WM_Cursor_##E:{ \
local_persist HCURSOR curs = 0; \
if (curs == 0){ curs = LoadCursor(NULL, R); } \
hcursor = curs; }break;
    Win32CursorXList(CursorCase)
#undef CursorCase
#undef Win32CursorXList
  }
  if(valid_cursor && !w32_wm_resizing)
  {
    if(hcursor != w32_wm_state->hCursor)
    {
      PostMessage(0, WM_SETCURSOR, 0, 0);
      POINT p = {0};
      GetCursorPos(&p);
      SetCursorPos(p.x, p.y);
    }
    w32_wm_state->hCursor = hcursor;
  }
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
wm_graphical_message(B32 error, String8 title, String8 message)
{
  Temp scratch = scratch_begin(0, 0);
  String16 title16 = str16_from_8(scratch.arena, title);
  String16 message16 = str16_from_8(scratch.arena, message);
  MessageBoxW(0, (WCHAR *)message16.str, (WCHAR *)title16.str, MB_OK|(!!error*MB_ICONERROR));
  scratch_end(scratch);
}

internal String8
wm_graphical_pick_file(Arena *arena, String8 initial_path)
{
  String8 result = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    U64 buffer_size = 4096;
    U16 *buffer = push_array(scratch.arena, U16, buffer_size);
    OPENFILENAMEW params = {sizeof(params)};
    {
      params.lpstrFile = (WCHAR *)buffer;
      params.nMaxFile = buffer_size;
      params.lpstrInitialDir = (WCHAR *)str16_from_8(scratch.arena, initial_path).str;
    }
    if(GetOpenFileNameW(&params))
    {
      result = str8_from_16(arena, str16_cstring((U16 *)buffer));
    }
    scratch_end(scratch);
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
wm_show_in_filesystem_ui(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);
  for(U64 idx = 0; idx < path_copy.size; idx += 1)
  {
    if(path_copy.str[idx] == '/')
    {
      path_copy.str[idx] = '\\';
    }
  }
  String16 path16 = str16_from_8(scratch.arena, path_copy);
  SFGAOF flags = 0;
  PIDLIST_ABSOLUTE list = 0;
  if(path16.size != 0 && SUCCEEDED(SHParseDisplayName(path16.str, 0, &list, 0, &flags)))
  {
    HRESULT hr = SHOpenFolderAndSelectItems(list, 0, 0, 0);
    CoTaskMemFree(list);
    (void)hr;
  }
  scratch_end(scratch);
}

internal void
wm_open_in_browser(String8 url)
{
  Temp scratch = scratch_begin(0, 0);
  String16 url16 = str16_from_8(scratch.arena, url);
  ShellExecuteW(0, L"open", (WCHAR *)url16.str, 0, 0, SW_SHOWNORMAL);
  scratch_end(scratch);
}
