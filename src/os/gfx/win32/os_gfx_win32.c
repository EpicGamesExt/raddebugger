// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Includes

#include <uxtheme.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#pragma comment(lib, "gdi32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "UxTheme")
#pragma comment(lib, "ole32")

////////////////////////////////
//~ rjf: Globals

global U32            w32_gfx_thread_tid = 0;
global HINSTANCE      w32_h_instance = 0;
global W32_Window *   w32_first_window = 0;
global W32_Window *   w32_last_window = 0;
global W32_Window *   w32_first_free_window = 0;
global OS_EventList   w32_event_list = {0};
global Arena *        w32_event_arena = 0;
global HCURSOR        w32_hcursor = 0;
global B32            w32_resizing = 0;
global F32            w32_default_refresh_rate = 60.f;

////////////////////////////////
//~ allen: Windows SDK Inconsistency Fixer

typedef BOOL w32_SetProcessDpiAwarenessContext_Type(void* value);
typedef UINT w32_GetDpiForWindow_Type(HWND hwnd);
#define w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((void*)-4)

global w32_GetDpiForWindow_Type *w32_GetDpiForWindow_func = 0;

////////////////////////////////
//~ rjf: Basic Helpers

internal Rng2F32
w32_base_rect_from_win32_rect(RECT rect)
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

internal OS_Handle
os_window_from_w32_window(W32_Window *window)
{
  OS_Handle handle = {(U64)window};
  return handle;
}

internal W32_Window *
w32_window_from_os_window(OS_Handle handle)
{
  W32_Window *window = (W32_Window *)handle.u64[0];
  return window;
}

internal W32_Window *
w32_window_from_hwnd(HWND hwnd)
{
  W32_Window *result = 0;
  for(W32_Window *w = w32_first_window; w; w = w->next)
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
w32_hwnd_from_window(W32_Window *window)
{
  return window->hwnd;
}

internal W32_Window *
w32_allocate_window(void)
{
  W32_Window *result = w32_first_free_window;
  if(result == 0)
  {
    result = push_array(w32_perm_arena, W32_Window, 1);
  }
  else
  {
    w32_first_free_window = w32_first_free_window->next;
    MemoryZeroStruct(result);
  }
  if(result)
  {
    DLLPushBack(w32_first_window, w32_last_window, result);
  }
  result->last_window_placement.length = sizeof(WINDOWPLACEMENT);
  return result;
}

internal void
w32_free_window(W32_Window *window)
{
  if(window->paint_arena != 0)
  {
    arena_release(window->paint_arena);
  }
  DestroyWindow(window->hwnd);
  DLLRemove(w32_first_window, w32_last_window, window);
  window->next = w32_first_free_window;
  w32_first_free_window = window;
}

internal OS_Event *
w32_push_event(OS_EventKind kind, W32_Window *window)
{
  OS_Event *result = push_array(w32_event_arena, OS_Event, 1);
  DLLPushBack(w32_event_list.first, w32_event_list.last, result);
  result->timestamp_us = os_now_microseconds();
  result->kind = kind;
  result->window = os_window_from_w32_window(window);
  result->flags = os_get_event_flags();
  w32_event_list.count += 1;
  return(result);
}

internal OS_Key
w32_os_key_from_vkey(WPARAM vkey)
{
  local_persist B32 first = 1;
  local_persist OS_Key key_table[256];
  if (first){
    first = 0;
    MemoryZeroArray(key_table);
    
    key_table[(unsigned int)'A'] = OS_Key_A;
    key_table[(unsigned int)'B'] = OS_Key_B;
    key_table[(unsigned int)'C'] = OS_Key_C;
    key_table[(unsigned int)'D'] = OS_Key_D;
    key_table[(unsigned int)'E'] = OS_Key_E;
    key_table[(unsigned int)'F'] = OS_Key_F;
    key_table[(unsigned int)'G'] = OS_Key_G;
    key_table[(unsigned int)'H'] = OS_Key_H;
    key_table[(unsigned int)'I'] = OS_Key_I;
    key_table[(unsigned int)'J'] = OS_Key_J;
    key_table[(unsigned int)'K'] = OS_Key_K;
    key_table[(unsigned int)'L'] = OS_Key_L;
    key_table[(unsigned int)'M'] = OS_Key_M;
    key_table[(unsigned int)'N'] = OS_Key_N;
    key_table[(unsigned int)'O'] = OS_Key_O;
    key_table[(unsigned int)'P'] = OS_Key_P;
    key_table[(unsigned int)'Q'] = OS_Key_Q;
    key_table[(unsigned int)'R'] = OS_Key_R;
    key_table[(unsigned int)'S'] = OS_Key_S;
    key_table[(unsigned int)'T'] = OS_Key_T;
    key_table[(unsigned int)'U'] = OS_Key_U;
    key_table[(unsigned int)'V'] = OS_Key_V;
    key_table[(unsigned int)'W'] = OS_Key_W;
    key_table[(unsigned int)'X'] = OS_Key_X;
    key_table[(unsigned int)'Y'] = OS_Key_Y;
    key_table[(unsigned int)'Z'] = OS_Key_Z;
    
    for (U64 i = '0', j = OS_Key_0; i <= '9'; i += 1, j += 1){
      key_table[i] = (OS_Key)j;
    }
    for (U64 i = VK_NUMPAD0, j = OS_Key_0; i <= VK_NUMPAD9; i += 1, j += 1){
      key_table[i] = (OS_Key)j;
    }
    for (U64 i = VK_F1, j = OS_Key_F1; i <= VK_F24; i += 1, j += 1){
      key_table[i] = (OS_Key)j;
    }
    
    key_table[VK_SPACE]     = OS_Key_Space;
    key_table[VK_OEM_3]     = OS_Key_Tick;
    key_table[VK_OEM_MINUS] = OS_Key_Minus;
    key_table[VK_OEM_PLUS]  = OS_Key_Equal;
    key_table[VK_OEM_4]     = OS_Key_LeftBracket;
    key_table[VK_OEM_6]     = OS_Key_RightBracket;
    key_table[VK_OEM_1]     = OS_Key_Semicolon;
    key_table[VK_OEM_7]     = OS_Key_Quote;
    key_table[VK_OEM_COMMA] = OS_Key_Comma;
    key_table[VK_OEM_PERIOD]= OS_Key_Period;
    key_table[VK_OEM_2]     = OS_Key_Slash;
    key_table[VK_OEM_5]     = OS_Key_BackSlash;
    
    key_table[VK_TAB]       = OS_Key_Tab;
    key_table[VK_PAUSE]     = OS_Key_Pause;
    key_table[VK_ESCAPE]    = OS_Key_Esc;
    
    key_table[VK_UP]        = OS_Key_Up;
    key_table[VK_LEFT]      = OS_Key_Left;
    key_table[VK_DOWN]      = OS_Key_Down;
    key_table[VK_RIGHT]     = OS_Key_Right;
    
    key_table[VK_BACK]      = OS_Key_Backspace;
    key_table[VK_RETURN]    = OS_Key_Return;
    
    key_table[VK_DELETE]    = OS_Key_Delete;
    key_table[VK_INSERT]    = OS_Key_Insert;
    key_table[VK_PRIOR]     = OS_Key_PageUp;
    key_table[VK_NEXT]      = OS_Key_PageDown;
    key_table[VK_HOME]      = OS_Key_Home;
    key_table[VK_END]       = OS_Key_End;
    
    key_table[VK_CAPITAL]   = OS_Key_CapsLock;
    key_table[VK_NUMLOCK]   = OS_Key_NumLock;
    key_table[VK_SCROLL]    = OS_Key_ScrollLock;
    key_table[VK_APPS]      = OS_Key_Menu;
    
    key_table[VK_CONTROL]   = OS_Key_Ctrl;
    key_table[VK_LCONTROL]  = OS_Key_Ctrl;
    key_table[VK_RCONTROL]  = OS_Key_Ctrl;
    key_table[VK_SHIFT]     = OS_Key_Shift;
    key_table[VK_LSHIFT]    = OS_Key_Shift;
    key_table[VK_RSHIFT]    = OS_Key_Shift;
    key_table[VK_MENU]      = OS_Key_Alt;
    key_table[VK_LMENU]     = OS_Key_Alt;
    key_table[VK_RMENU]     = OS_Key_Alt;
    
    key_table[VK_DIVIDE]   = OS_Key_NumSlash;
    key_table[VK_MULTIPLY] = OS_Key_NumStar;
    key_table[VK_SUBTRACT] = OS_Key_NumMinus;
    key_table[VK_ADD]      = OS_Key_NumPlus;
    key_table[VK_DECIMAL]  = OS_Key_NumPeriod;
    
    for (U32 i = 0; i < 10; i += 1){
      key_table[VK_NUMPAD0 + i] = (OS_Key)((U64)OS_Key_Num0 + i);
    }
    
    for (U64 i = 0xDF, j = 0; i < 0xFF; i += 1, j += 1){
      key_table[i] = (OS_Key)((U64)OS_Key_Ex0 + j);
    }
  }
  
  OS_Key key = key_table[vkey&bitmask8];
  return(key);
}

internal WPARAM
w32_vkey_from_os_key(OS_Key key)
{
  WPARAM result = 0;
  {
    local_persist B32 initialized = 0;
    local_persist WPARAM vkey_table[OS_Key_COUNT] = {0};
    if(initialized == 0)
    {
      initialized = 1;
      vkey_table[OS_Key_Esc] = VK_ESCAPE;
      for(OS_Key key = OS_Key_F1; key <= OS_Key_F24; key = (OS_Key)(key+1))
      {
        vkey_table[key] = VK_F1+(key-OS_Key_F1);
      }
      vkey_table[OS_Key_Tick] = VK_OEM_3;
      for(OS_Key key = OS_Key_0; key <= OS_Key_9; key = (OS_Key)(key+1))
      {
        vkey_table[key] = '0'+(key-OS_Key_0);
      }
      vkey_table[OS_Key_Minus] = VK_OEM_MINUS;
      vkey_table[OS_Key_Equal] = VK_OEM_PLUS;
      vkey_table[OS_Key_Backspace] = VK_BACK;
      vkey_table[OS_Key_Tab] = VK_TAB;
      vkey_table[OS_Key_Q] = 'Q';
      vkey_table[OS_Key_W] = 'W';
      vkey_table[OS_Key_E] = 'E';
      vkey_table[OS_Key_R] = 'R';
      vkey_table[OS_Key_T] = 'T';
      vkey_table[OS_Key_Y] = 'Y';
      vkey_table[OS_Key_U] = 'U';
      vkey_table[OS_Key_I] = 'I';
      vkey_table[OS_Key_O] = 'O';
      vkey_table[OS_Key_P] = 'P';
      vkey_table[OS_Key_LeftBracket] = VK_OEM_4;
      vkey_table[OS_Key_RightBracket] = VK_OEM_6;
      vkey_table[OS_Key_BackSlash] = VK_OEM_5;
      vkey_table[OS_Key_CapsLock] = VK_CAPITAL;
      vkey_table[OS_Key_A] = 'A';
      vkey_table[OS_Key_S] = 'S';
      vkey_table[OS_Key_D] = 'D';
      vkey_table[OS_Key_F] = 'F';
      vkey_table[OS_Key_G] = 'G';
      vkey_table[OS_Key_H] = 'H';
      vkey_table[OS_Key_J] = 'J';
      vkey_table[OS_Key_K] = 'K';
      vkey_table[OS_Key_L] = 'L';
      vkey_table[OS_Key_Semicolon] = VK_OEM_1;
      vkey_table[OS_Key_Quote] = VK_OEM_7;
      vkey_table[OS_Key_Return] = VK_RETURN;
      vkey_table[OS_Key_Shift] = VK_SHIFT;
      vkey_table[OS_Key_Z] = 'Z';
      vkey_table[OS_Key_X] = 'X';
      vkey_table[OS_Key_C] = 'C';
      vkey_table[OS_Key_V] = 'V';
      vkey_table[OS_Key_B] = 'B';
      vkey_table[OS_Key_N] = 'N';
      vkey_table[OS_Key_M] = 'M';
      vkey_table[OS_Key_Comma] = VK_OEM_COMMA;
      vkey_table[OS_Key_Period] = VK_OEM_PERIOD;
      vkey_table[OS_Key_Slash] = VK_OEM_2;
      vkey_table[OS_Key_Ctrl] = VK_CONTROL;
      vkey_table[OS_Key_Alt] = VK_MENU;
      vkey_table[OS_Key_Space] = VK_SPACE;
      vkey_table[OS_Key_Menu] = VK_APPS;
      vkey_table[OS_Key_ScrollLock] = VK_SCROLL;
      vkey_table[OS_Key_Pause] = VK_PAUSE;
      vkey_table[OS_Key_Insert] = VK_INSERT;
      vkey_table[OS_Key_Home] = VK_HOME;
      vkey_table[OS_Key_PageUp] = VK_PRIOR;
      vkey_table[OS_Key_Delete] = VK_DELETE;
      vkey_table[OS_Key_End] = VK_END;
      vkey_table[OS_Key_PageDown] = VK_NEXT;
      vkey_table[OS_Key_Up] = VK_UP;
      vkey_table[OS_Key_Left] = VK_LEFT;
      vkey_table[OS_Key_Down] = VK_DOWN;
      vkey_table[OS_Key_Right] = VK_RIGHT;
      for(OS_Key key = OS_Key_Ex0; key <= OS_Key_Ex29; key = (OS_Key)(key+1))
      {
        vkey_table[key] = 0xDF + (key-OS_Key_Ex0);
      }
      vkey_table[OS_Key_NumLock] = VK_NUMLOCK;
      vkey_table[OS_Key_NumSlash] = VK_DIVIDE;
      vkey_table[OS_Key_NumStar] = VK_MULTIPLY;
      vkey_table[OS_Key_NumMinus] = VK_SUBTRACT;
      vkey_table[OS_Key_NumPlus] = VK_ADD;
      vkey_table[OS_Key_NumPeriod] = VK_DECIMAL;
      vkey_table[OS_Key_Num0] = VK_NUMPAD0;
      vkey_table[OS_Key_Num1] = VK_NUMPAD1;
      vkey_table[OS_Key_Num2] = VK_NUMPAD2;
      vkey_table[OS_Key_Num3] = VK_NUMPAD3;
      vkey_table[OS_Key_Num4] = VK_NUMPAD4;
      vkey_table[OS_Key_Num5] = VK_NUMPAD5;
      vkey_table[OS_Key_Num6] = VK_NUMPAD6;
      vkey_table[OS_Key_Num7] = VK_NUMPAD7;
      vkey_table[OS_Key_Num8] = VK_NUMPAD8;
      vkey_table[OS_Key_Num9] = VK_NUMPAD9;
    }
    result = vkey_table[key];
  }
  return result;
}

internal LRESULT
w32_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  ProfBeginFunction();
  LRESULT result = 0;
  
  B32 good = 1;
  if(w32_event_arena == 0)
  {
    result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
    good = 0;
  }
  
  if(good)
  {
    W32_Window *window = w32_window_from_hwnd(hwnd);
    OS_Handle window_handle = os_window_from_w32_window(window);
    B32 release = 0;
    
    switch(uMsg)
    {
      default:
      {
        result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
      }break;
      
      case WM_ENTERSIZEMOVE:
      {
        w32_resizing = 1;
      }break;
      
      case WM_EXITSIZEMOVE:
      {
        w32_resizing = 0;
      }break;
      
      case WM_SIZE:
      case WM_PAINT:
      {
        if(window->repaint != 0)
        {
          PAINTSTRUCT ps = {0};
          BeginPaint(hwnd, &ps);
          window->repaint(os_window_from_w32_window(window), window->repaint_user_data);
          EndPaint(hwnd, &ps);
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      
      case WM_CLOSE:
      {
        w32_push_event(OS_EventKind_WindowClose, window);
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
        OS_Event *event = w32_push_event(release ? OS_EventKind_Release : OS_EventKind_Press, window);
        switch (uMsg)
        {
          case WM_LBUTTONUP: case WM_LBUTTONDOWN:
          {
            event->key = OS_Key_LeftMouseButton;
          }break;
          case WM_MBUTTONUP: case WM_MBUTTONDOWN:
          {
            event->key = OS_Key_MiddleMouseButton;
          }break;
          case WM_RBUTTONUP: case WM_RBUTTONDOWN:
          {
            event->key = OS_Key_RightMouseButton;
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
        OS_Event *event = w32_push_event(OS_EventKind_MouseMove, window);
        event->pos.x = (F32)(S16)LOWORD(lParam);
        event->pos.y = (F32)(S16)HIWORD(lParam);
      }break;
      
      case WM_MOUSEWHEEL:
      {
        S16 wheel_delta = HIWORD(wParam);
        OS_Event *event = w32_push_event(OS_EventKind_Scroll, window);
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
        OS_Event *event = w32_push_event(OS_EventKind_Scroll, window);
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
        
        OS_Event *event = w32_push_event(release ? OS_EventKind_Release : OS_EventKind_Press, window);
        event->key = w32_os_key_from_vkey(wParam);
        event->repeat_count = lParam & bitmask16;
        event->is_repeat = is_repeat;
        event->right_sided = right_sided;
        if(event->key == OS_Key_Alt   && event->flags & OS_EventFlag_Alt)   { event->flags &= ~OS_EventFlag_Alt; }
        if(event->key == OS_Key_Ctrl  && event->flags & OS_EventFlag_Ctrl)  { event->flags &= ~OS_EventFlag_Ctrl; }
        if(event->key == OS_Key_Shift && event->flags & OS_EventFlag_Shift) { event->flags &= ~OS_EventFlag_Shift; }
      }break;
      
      case WM_SYSCHAR:
      {
        result = 0;
      }break;
      
      case WM_CHAR:
      {
        U32 character = wParam;
        if(character >= 32 && character != 127)
        {
          OS_Event *event = w32_push_event(OS_EventKind_Text, window);
          if(lParam & bit29)
          {
            event->flags |= OS_EventFlag_Alt;
          }
          event->character = character;
        }
      }break;
      
      case WM_KILLFOCUS:
      {
        w32_push_event(OS_EventKind_WindowLoseFocus, window);
        ReleaseCapture();
      }break;
      
      case WM_SETCURSOR:
      {
        Rng2F32 window_rect = os_client_rect_from_window(window_handle);
        Vec2F32 mouse = os_mouse_from_window(window_handle);
        B32 on_border = 0;
        DWORD window_style = window ? GetWindowLong(window->hwnd, GWL_STYLE) : 0;
        B32 is_fullscreen = !(window_style & WS_OVERLAPPEDWINDOW);
        if(window != 0 && window->custom_border && !is_fullscreen)
        {
          B32 on_border_x = (mouse.x <= window->custom_border_edge_thickness || window_rect.x1-window->custom_border_edge_thickness <= mouse.x);
          B32 on_border_y = (mouse.y <= window->custom_border_edge_thickness || window_rect.y1-window->custom_border_edge_thickness <= mouse.y);
          on_border = on_border_x || on_border_y;
        }
        if(!w32_resizing && !on_border &&
           contains_2f32(window_rect, mouse))
        {
          SetCursor(w32_hcursor);
        }
        else
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
      }break;
      
      case WM_DPICHANGED:
      {
        F32 new_dpi = (F32)(wParam & 0xffff);
        window->dpi = new_dpi;
      }break;
      
      //- rjf: [custom border]
      case WM_NCPAINT:
      {
        if(window != 0 && window->custom_border && !window->custom_border_composition_enabled)
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
        if(window != 0 && window->custom_border)
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
        if(window && window->custom_border && !window->custom_border_composition_enabled)
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
        if(window == 0 || window->custom_border == 0)
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
      if(window != 0)
      {
        if(window->custom_border == 0)
        {
          result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        else
        {
          MARGINS m = {0, 0, 0, 0};
          RECT *r = (RECT *)lParam;
          DWORD window_style = window ? GetWindowLong(window->hwnd, GWL_STYLE) : 0;
          B32 is_fullscreen = !(window_style & WS_OVERLAPPEDWINDOW);
          if(IsZoomed(hwnd) && !is_fullscreen)
          {
            int x_push_in = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            int y_push_in = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            r->left   += x_push_in;
            r->top    += y_push_in;
            r->bottom -= x_push_in;
            r->right  -= y_push_in;
            m.cxLeftWidth = m.cxRightWidth = x_push_in;
            m.cyTopHeight = m.cyBottomHeight = y_push_in;
          }
          DwmExtendFrameIntoClientArea(hwnd, &m);
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
          POINT pos_monitor;
          pos_monitor.x = GET_X_LPARAM(lParam);
          pos_monitor.y = GET_Y_LPARAM(lParam);
          POINT pos_client = pos_monitor;
          ScreenToClient(hwnd, &pos_client);
          
          //- rjf: check against window boundaries
          RECT frame_rect;
          GetWindowRect(hwnd, &frame_rect);
          B32 is_over_window = (frame_rect.left <= pos_monitor.x && pos_monitor.x < frame_rect.right &&
                                frame_rect.top <= pos_monitor.y && pos_monitor.y < frame_rect.bottom);
          
          //- rjf: check against borders
          B32 is_over_left   = 0;
          B32 is_over_right  = 0;
          B32 is_over_top    = 0;
          B32 is_over_bottom = 0;
          {
            RECT rect;
            GetClientRect(hwnd, &rect);
            if(!IsZoomed(hwnd))
            {
              if(rect.left <= pos_client.x && pos_client.x < rect.left + window->custom_border_edge_thickness)
              {
                is_over_left = 1;
              }
              if(rect.right - window->custom_border_edge_thickness <= pos_client.x && pos_client.x < rect.right)
              {
                is_over_right = 1;
              }
              if(rect.bottom - window->custom_border_edge_thickness <= pos_client.y && pos_client.y < rect.bottom)
              {
                is_over_bottom = 1;
              }
              if(rect.top <= pos_client.y && pos_client.y < rect.top + window->custom_border_edge_thickness)
              {
                is_over_top = 1;
              }
            }
          }
          
          //- rjf: check against title bar
          B32 is_over_title_bar = 0;
          {
            RECT rect;
            GetClientRect(hwnd, &rect);
            is_over_title_bar = (rect.left <= pos_client.x && pos_client.x < rect.right &&
                                 rect.top <= pos_client.y && pos_client.y < rect.top + window->custom_border_title_thickness);
          }
          
          //- rjf: check against title bar client areas
          B32 is_over_title_bar_client_area = 0;
          for(W32_TitleBarClientArea *area = window->first_title_bar_client_area;
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
          
          //- rjf: resolve hovering to result
          result = HTNOWHERE;
          if(is_over_window)
          {
            // rjf: default to client area
            result = HTCLIENT;
            
            // rjf: title bar
            if(is_over_title_bar)
            {
              result = HTCAPTION;
            }
            
            // rjf: normal edges
            if(is_over_left)   { result = HTLEFT; }
            if(is_over_right)  { result = HTRIGHT; }
            if(is_over_top)    { result = HTTOP; }
            if(is_over_bottom) { result = HTBOTTOM; }
            
            // rjf: corners
            if(is_over_left  && is_over_top)    { result = HTTOPLEFT; }
            if(is_over_left  && is_over_bottom) { result = HTBOTTOMLEFT; }
            if(is_over_right && is_over_top)    { result = HTTOPRIGHT; }
            if(is_over_right && is_over_bottom) { result = HTBOTTOMRIGHT; }
            
            // rjf: title bar client area
            if(is_over_title_bar_client_area)
            {
              result = HTCLIENT;
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
w32_monitor_gather_enum_proc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM bundle_ptr)
{
  W32_MonitorGatherBundle *bundle = (W32_MonitorGatherBundle *)bundle_ptr;
  OS_Handle handle = {(U64)monitor};
  os_handle_list_push(bundle->arena, bundle->list, handle);
  return 1;
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_graphical_init(void)
{
  //- rjf: grab TID of thread which is doing graphics
  w32_gfx_thread_tid = (U32)GetCurrentThreadId();
  
  //- rjf: grab hinstance
  w32_h_instance = GetModuleHandle(0);
  
  //- rjf: set dpi awareness
  w32_SetProcessDpiAwarenessContext_Type *SetProcessDpiAwarenessContext_func = 0;
  HMODULE module = LoadLibraryA("user32.dll");
  if(module != 0)
  {
    SetProcessDpiAwarenessContext_func =
    (w32_SetProcessDpiAwarenessContext_Type*)GetProcAddress(module, "SetProcessDpiAwarenessContext");
    w32_GetDpiForWindow_func =
    (w32_GetDpiForWindow_Type*)GetProcAddress(module, "GetDpiForWindow");
    FreeLibrary(module);
  }
  if(SetProcessDpiAwarenessContext_func != 0)
  {
    SetProcessDpiAwarenessContext_func(w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }
  
  //- rjf: register graphical-window class
  {
    WNDCLASSEXW wndclass = {sizeof(wndclass)};
    wndclass.lpfnWndProc = w32_wnd_proc;
    wndclass.hInstance = w32_h_instance;
    wndclass.lpszClassName = L"graphical-window";
    wndclass.hCursor = LoadCursorA(0, IDC_ARROW);
    wndclass.hIcon = LoadIcon(w32_h_instance, MAKEINTRESOURCE(1));
    wndclass.style = CS_VREDRAW|CS_HREDRAW;
    ATOM wndatom = RegisterClassExW(&wndclass);
    (void)wndatom;
  }
  
  //- rjf: grab refresh rate
  {
    DEVMODEW devmodew = {0};
    if(EnumDisplaySettingsW(0, ENUM_CURRENT_SETTINGS, &devmodew))
    {
      w32_default_refresh_rate = (F32)devmodew.dmDisplayFrequency;
    }
  }
  
  //- rjf: set initial cursor
  os_set_cursor(OS_Cursor_Pointer);
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{
  if(OpenClipboard(0))
  {
    EmptyClipboard();
    HANDLE string_copy_handle = GlobalAlloc(GMEM_MOVEABLE, string.size+1);
    if(string_copy_handle)
    {
      U8 *copy_buffer = (U8 *)GlobalLock(string_copy_handle);
      MemoryCopy(copy_buffer, string.str, string.size);
      copy_buffer[string.size] = 0;
      GlobalUnlock(string_copy_handle);
      SetClipboardData(CF_TEXT, string_copy_handle);
    }
    CloseClipboard();
  }
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  String8 result = {0};
  if(IsClipboardFormatAvailable(CF_TEXT) &&
     OpenClipboard(0))
  {
    HANDLE data_handle = GetClipboardData(CF_TEXT);
    if(data_handle)
    {
      U8 *buffer = (U8 *)GlobalLock(data_handle);
      if(buffer)
      {
        U64 size = cstring8_length(buffer);
        result = push_str8_copy(arena, str8(buffer, size));
        GlobalUnlock(data_handle);
      }
    }
    CloseClipboard();
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  //- rjf: make hwnd
  HWND hwnd = 0;
  {
    Temp scratch = scratch_begin(0, 0);
    String16 title16 = str16_from_8(scratch.arena, title);
    hwnd = CreateWindowExW(WS_EX_APPWINDOW,
                           L"graphical-window",
                           (WCHAR*)title16.str,
                           WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           (int)resolution.x,
                           (int)resolution.y,
                           0, 0,
                           w32_h_instance,
                           0);
    scratch_end(scratch);
  }
  
  //- rjf- make/fill window
  W32_Window *window = w32_allocate_window();
  {
    window->hwnd = hwnd;
    if (w32_GetDpiForWindow_func != 0){
      window->dpi = (F32)w32_GetDpiForWindow_func(hwnd);
    }
    else{
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
  if(flags & OS_WindowFlag_CustomBorder)
  {
    window->custom_border = 1;
    window->paint_arena = arena_alloc();
  }
  
  //- rjf: convert to handle + return
  OS_Handle result = os_window_from_w32_window(window);
  return result;
}

internal void
os_window_close(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  w32_free_window(window);
}

internal void
os_window_first_paint(OS_Handle window_handle)
{
  W32_Window *window = w32_window_from_os_window(window_handle);
  window->first_paint_done = 1;
  ShowWindow(window->hwnd, SW_SHOW);
  if(window->maximized)
  {
    ShowWindow(window->hwnd, SW_MAXIMIZE);
  }
}

internal void
os_window_equip_repaint(OS_Handle handle, OS_WindowRepaintFunctionType *repaint, void *user_data)
{
  W32_Window *window = w32_window_from_os_window(handle);
  window->repaint = repaint;
  window->repaint_user_data = user_data;
}

internal void
os_window_focus(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  SetForegroundWindow(window->hwnd);
  SetFocus(window->hwnd);
}

internal B32
os_window_is_focused(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  HWND active_hwnd = GetActiveWindow();
  return active_hwnd == window->hwnd;
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  DWORD window_style = GetWindowLong(window->hwnd, GWL_STYLE);
  return !(window_style & WS_OVERLAPPEDWINDOW);
}

internal void
os_window_set_fullscreen(OS_Handle handle, B32 fullscreen)
{
  W32_Window *window = w32_window_from_os_window(handle);
  OS_WindowRepaintFunctionType *repaint = window->repaint;
  window->repaint = 0;
  DWORD window_style = GetWindowLong(window->hwnd, GWL_STYLE);
  B32 is_fullscreen_already = os_window_is_fullscreen(handle);
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
  window->repaint = repaint;
}

internal B32
os_window_is_maximized(OS_Handle handle)
{
  B32 result = 0;
  W32_Window *window = w32_window_from_os_window(handle);
  if(window)
  {
    result = !!(IsZoomed(window->hwnd));
  }
  return result;
}

internal void
os_window_set_maximized(OS_Handle handle, B32 maximized)
{
  W32_Window *window = w32_window_from_os_window(handle);
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

internal void
os_window_minimize(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  if(window != 0)
  {
    ShowWindow(window->hwnd, SW_MINIMIZE);
  }
}

internal void
os_window_bring_to_front(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  if(window != 0)
  {
    BringWindowToTop(window->hwnd);
  }
}

internal void
os_window_set_monitor(OS_Handle window_handle, OS_Handle monitor)
{
  W32_Window *window = w32_window_from_os_window(window_handle);
  HMONITOR hmonitor = (HMONITOR)monitor.u64[0];
  {
    MONITORINFOEXW info;
    info.cbSize = sizeof(MONITORINFOEXW);
    if(GetMonitorInfoW(hmonitor, (MONITORINFO *)&info))
    {
      Rng2F32 existing_rect = os_rect_from_window(window_handle);
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
os_window_clear_custom_border_data(OS_Handle handle)
{
  W32_Window *window = w32_window_from_os_window(handle);
  if(window->custom_border)
  {
    arena_clear(window->paint_arena);
    window->first_title_bar_client_area = window->last_title_bar_client_area = 0;
    window->custom_border_title_thickness = 0;
    window->custom_border_edge_thickness = 0;
  }
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  W32_Window *window = w32_window_from_os_window(handle);
  window->custom_border_title_thickness = thickness;
}

internal void
os_window_push_custom_edges(OS_Handle handle, F32 thickness)
{
  W32_Window *window = w32_window_from_os_window(handle);
  window->custom_border_edge_thickness = thickness;
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  W32_Window *window = w32_window_from_os_window(handle);
  if(window->custom_border)
  {
    W32_TitleBarClientArea *area = push_array(window->paint_arena, W32_TitleBarClientArea, 1);
    if(area != 0)
    {
      area->rect = rect;
      SLLQueuePush(window->first_title_bar_client_area, window->last_title_bar_client_area, area);
    }
  }
}

internal Rng2F32
os_rect_from_window(OS_Handle handle)
{
  Rng2F32 r = {0};
  W32_Window *window = w32_window_from_os_window(handle);
  if(window)
  {
    RECT rect = {0};
    GetWindowRect(w32_hwnd_from_window(window), &rect);
    r = w32_base_rect_from_win32_rect(rect);
  }
  return r;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle handle)
{
  Rng2F32 r = {0};
  W32_Window *window = w32_window_from_os_window(handle);
  if(window)
  {
    RECT rect = {0};
    GetClientRect(w32_hwnd_from_window(window), &rect);
    r = w32_base_rect_from_win32_rect(rect);
  }
  return r;
}

internal F32
os_dpi_from_window(OS_Handle handle)
{
  F32 result = 96.f;
  W32_Window *window = w32_window_from_os_window(handle);
  if(window != 0)
  {
    result = window->dpi;
  }
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  Temp scratch = scratch_begin(&arena, 1);
  OS_HandleList list = {0};
  {
    W32_MonitorGatherBundle bundle = {arena, &list};
    EnumDisplayMonitors(0, 0, w32_monitor_gather_enum_proc, (LPARAM)&bundle);
  }
  OS_HandleArray array = os_handle_array_from_list(arena, &list);
  scratch_end(scratch);
  return array;
}

internal OS_Handle
os_primary_monitor(void)
{
  POINT zero_pt = {0, 0};
  HMONITOR monitor = MonitorFromPoint(zero_pt, MONITOR_DEFAULTTOPRIMARY);
  OS_Handle result = {(U64)monitor};
  return result;
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  W32_Window *w = w32_window_from_os_window(window);
  HMONITOR handle = MonitorFromWindow(w->hwnd, MONITOR_DEFAULTTOPRIMARY);
  OS_Handle result = {(U64)handle};
  return result;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
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
os_dim_from_monitor(OS_Handle monitor)
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

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
os_send_wakeup_event(void)
{
  PostThreadMessageA(w32_gfx_thread_tid, 0x401, 0, 0);
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  w32_event_arena = arena;
  MemoryZeroStruct(&w32_event_list);
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
        w32_push_event(OS_EventKind_WindowClose, 0);
      }
    }
  }
  return w32_event_list;
}

internal OS_EventFlags
os_get_event_flags(void)
{
  OS_EventFlags flags = 0;
  if(GetKeyState(VK_CONTROL) & 0x8000)
  {
    flags |= OS_EventFlag_Ctrl;
  }
  if(GetKeyState(VK_SHIFT) & 0x8000)
  {
    flags |= OS_EventFlag_Shift;
  }
  if(GetKeyState(VK_MENU) & 0x8000)
  {
    flags |= OS_EventFlag_Alt;
  }
  return(flags);
}

internal B32
os_key_is_down(OS_Key key)
{
  B32 result = 0;
  {
    WPARAM vkey_code = w32_vkey_from_os_key(key);
    SHORT state = GetAsyncKeyState(vkey_code);
    result = !!(state & (0x8000));
  }
  return result;
}

internal Vec2F32
os_mouse_from_window(OS_Handle handle)
{
  ProfBeginFunction();
  Vec2F32 v = {0};
  POINT p;
  if(GetCursorPos(&p))
  {
    W32_Window *window = w32_window_from_os_window(handle);
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
os_set_cursor(OS_Cursor cursor)
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
#define CursorCase(E,R) case OS_Cursor_##E:{ \
local_persist HCURSOR curs = 0; \
if (curs == 0){ curs = LoadCursor(NULL, R); } \
hcursor = curs; }break;
    Win32CursorXList(CursorCase)
#undef CursorCase
#undef Win32CursorXList
  }
  
  if(valid_cursor && !w32_resizing)
  {
    if(hcursor != w32_hcursor)
    {
      PostMessage(0, WM_SETCURSOR, 0, 0);
      POINT p = {0};
      GetCursorPos(&p);
      SetCursorPos(p.x, p.y);
    }
    w32_hcursor = hcursor;
  }
}

////////////////////////////////
//~ rjf: @os_hooks System Properties (Implemented Per-OS)

internal F32
os_double_click_time(void)
{
  UINT time_milliseconds = GetDoubleClickTime();
  return time_milliseconds / 1000.f;
}

internal F32
os_caret_blink_time(void)
{
  UINT time_milliseconds = GetCaretBlinkTime();
  return time_milliseconds / 1000.f;
}

internal F32
os_default_refresh_rate(void)
{
  return w32_default_refresh_rate;
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  Temp scratch = scratch_begin(0, 0);
  String16 title16 = str16_from_8(scratch.arena, title);
  String16 message16 = str16_from_8(scratch.arena, message);
  MessageBoxW(0, (WCHAR *)message16.str, (WCHAR *)title16.str, MB_OK|(!!error*MB_ICONERROR));
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
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
