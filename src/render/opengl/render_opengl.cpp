#include <windows.h>

////////////////////////////////
//~ dmylo: Helpers

internal R_OGL_Window *
r_ogl_window_from_handle(R_Handle handle)
{
  R_OGL_Window *window = (R_OGL_Window *)handle.u64[0];
  if(window->generation != handle.u64[1])
  {
    window = &r_ogl_window_nil;
  }
  return window;
}

internal R_Handle
r_ogl_handle_from_window(R_OGL_Window *window)
{
  R_Handle handle = {0};
  handle.u64[0] = (U64)window;
  handle.u64[1] = window->generation;
  return handle;
}

//- rjf: top-level layer initialization
r_hook void
r_init(CmdLine *cmdln)
{
  Arena *arena = arena_alloc();
  r_ogl_state = push_array(arena, R_OGL_State, 1);
  r_ogl_state->arena = arena;
  r_ogl_state->device_rw_mutex = os_rw_mutex_alloc();

  r_ogl_state->initialized = false;
  r_ogl_state->gl = {};
}

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext,
                                                    const int *attribList);
typedef BOOL wgl_choose_pixel_format_arb(HDC hdc, const int *piAttribIList,
                                         const FLOAT *pfAttribFList, UINT nMaxFormats,
                                         int *piFormats, UINT *nNumFormats);

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_TYPE_RGBA_ARB                       0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x0001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_SAMPLE_BUFFERS_ARB                  0x2041
#define WGL_SAMPLES_ARB                         0x2042

//- rjf: window setup/teardown
r_hook R_Handle
r_window_equip(OS_Handle handle)
{
  ProfBeginFunction();
  R_Handle result = {0};
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    //- rjf: allocate per-window-state
    R_OGL_Window *window = r_ogl_state->first_free_window;
    {
      if(window == 0)
      {
        window = push_array(r_ogl_state->arena, R_OGL_Window, 1);
      }
      else
      {
        U64 gen = window->generation;
        SLLStackPop(r_ogl_state->first_free_window);
        MemoryZeroStruct(window);
        window->generation = gen;
      }
      window->generation += 1;
    }

    wgl_create_context_attribs_arb *wglCreateContextAttribsARB = 0;
    wgl_choose_pixel_format_arb *wglChoosePixelFormatARB = 0;
    HDC fake_dc = 0;
    HWND fake_window = 0;
    HGLRC fake_glrc = 0;
    {
      // TODO(dmylo): error checking
      HINSTANCE instance = GetModuleHandleA(NULL);

      WNDCLASSEXA wcex = {};
      wcex.cbSize = sizeof(wcex);
      wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
      wcex.lpfnWndProc = DefWindowProcA;
      wcex.hInstance = instance;
      wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
      wcex.lpszClassName = "FakeOpenGLWindow";
      RegisterClassExA(&wcex);

      // NOTE(dmylo): Fake window shenenigans
      DWORD WindowStyle = WS_OVERLAPPEDWINDOW;
      fake_window = CreateWindowExA(0,
        "FakeOpenGLWindow", "Fake Window",      // window class, title
        WS_OVERLAPPEDWINDOW,                    // style
        CW_USEDEFAULT, CW_USEDEFAULT,           // position x, y
        1, 1,                                   // width, height
        NULL, NULL,                             // parent window, menu
        instance, NULL);           // instance, param

      HDC fake_dc = GetDC(fake_window);        // Device Context

      PIXELFORMATDESCRIPTOR desired_format = {};
      desired_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
      desired_format.nVersion = 1;
      desired_format.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
      desired_format.iPixelType = PFD_TYPE_RGBA;
      desired_format.cColorBits = 32;
      desired_format.cAlphaBits = 0;
      desired_format.cDepthBits = 32;
      desired_format.cStencilBits = 8;
      desired_format.iLayerType = PFD_MAIN_PLANE;

      int suggested_format_index = ChoosePixelFormat(fake_dc, &desired_format);

      PIXELFORMATDESCRIPTOR suggested_format;
      DescribePixelFormat(fake_dc, suggested_format_index, sizeof(PIXELFORMATDESCRIPTOR), &suggested_format);
      SetPixelFormat(fake_dc, suggested_format_index, &suggested_format);

      fake_glrc = wglCreateContext(fake_dc);
      BOOL ok = wglMakeCurrent(fake_dc, fake_glrc);

      wglCreateContextAttribsARB = (wgl_create_context_attribs_arb*)wglGetProcAddress("wglCreateContextAttribsARB");
      wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb*)wglGetProcAddress("wglChoosePixelFormatARB");
    }

    //- rjf: map os window handle -> hwnd
    HWND hwnd = {0};
    {
      W32_Window *w32_layer_window = w32_window_from_os_window(handle);
      hwnd = w32_hwnd_from_window(w32_layer_window);
    }
    HDC window_dc = GetDC(hwnd);

    const int PixelAttributes[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        // WGL_SAMPLE_BUFFERS_ARB, 0, // Number of buffers (must be 1 at time of writing)
        // WGL_SAMPLES_ARB, 1,        // Number of samples
        0
    };
    S32 NewSuggestedFormatIndex;
    U32 NumSuggestions;
    PIXELFORMATDESCRIPTOR SuggestedFormat;
    wglChoosePixelFormatARB(window_dc, PixelAttributes, 0, 1,
                                    &NewSuggestedFormatIndex, &NumSuggestions);
    // NOTE(Flame): Letting window fill the old PIXELFORMATDESCRIPTOR struct
    DescribePixelFormat(window_dc, NewSuggestedFormatIndex,
                                sizeof(PIXELFORMATDESCRIPTOR), &SuggestedFormat);
    SetPixelFormat(window_dc, NewSuggestedFormatIndex, &SuggestedFormat);

    int Attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 1,
        WGL_CONTEXT_FLAGS_ARB,
        //IMPORTANT:WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB disable stuff prior to 3.0
        /*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |*/ WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    HGLRC glrc = wglCreateContextAttribsARB(window_dc, 0, Attribs);
    bool ok = wglMakeCurrent(window_dc, glrc);

    if(!ok || glrc == NULL) {
      char buffer[256] = {0};
      raddbg_snprintf(buffer, sizeof(buffer), "OpenGL context creation failure. The process is terminating.");
      os_graphical_message(1, str8_lit("Fatal Error"), str8_cstring(buffer));
      os_exit_process(1);
    }

    if(!r_ogl_state->initialized) {
      HMODULE opengl_module = LoadLibraryA("opengl32.dll");
      for (U64 i = 0; i < ArrayCount(proc_names); i++)
      {
        GL3WglProc ptr = (GL3WglProc)wglGetProcAddress(proc_names[i]);
        if(!ptr) {
          ptr = (GL3WglProc)GetProcAddress(opengl_module, proc_names[i]);
        }
        r_ogl_state->gl.ptr[i] = ptr;
      }

      r_ogl_state->initialized = true;
    }

    // Release DC
    ReleaseDC(hwnd, window_dc);

    // Cleanup the fake window
    wglDeleteContext(fake_glrc);
    ReleaseDC(fake_window, fake_dc);
    DestroyWindow(fake_window);

    window->glrc = glrc;
    result = r_ogl_handle_from_window(window);

  }

  return result;
}

r_hook void
r_window_unequip(OS_Handle window, R_Handle equip_handle)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Window *window = r_ogl_window_from_handle(equip_handle);
    wglDeleteContext(window->glrc);
    window->glrc = 0;
    window->generation += 1;
    SLLStackPush(r_ogl_state->first_free_window, window);
  }
  ProfEnd();
}

//- rjf: textures
r_hook R_Handle
r_tex2d_alloc(R_Tex2DKind kind, Vec2S32 size, R_Tex2DFormat format, void *data)
{
    R_Handle result = {0};

    return result;
}

r_hook void
r_tex2d_release(R_Handle texture)
{

}

r_hook R_Tex2DKind
r_kind_from_tex2d(R_Handle texture)
{
    return {};
}

r_hook Vec2S32
r_size_from_tex2d(R_Handle texture)
{
    return {};
}

r_hook R_Tex2DFormat
r_format_from_tex2d(R_Handle texture)
{
    return {};
}

r_hook void
r_fill_tex2d_region(R_Handle texture, Rng2S32 subrect, void *data)
{

}


//- rjf: buffers
r_hook R_Handle
r_buffer_alloc(R_BufferKind kind, U64 size, void *data)
{
    R_Handle result = {0};

    return result;
}

r_hook void
r_buffer_release(R_Handle buffer)
{

}


//- dmylo: frame markers
r_hook void
r_begin_frame(void)
{
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    // NOTE(dmylo): no-op
  }
}

r_hook void
r_end_frame(void)
{
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    // TODO(dmylo): free transient resources
#if 0
    for(R_D3D11_FlushBuffer *buffer = r_d3d11_state->first_buffer_to_flush; buffer != 0; buffer = buffer->next)
    {
      buffer->buffer->Release();
    }
    for(R_D3D11_Tex2D *tex = r_d3d11_state->first_to_free_tex2d, *next = 0;
        tex != 0;
        tex = next)
    {
      next = tex->next;
      tex->view->Release();
      tex->texture->Release();
      tex->generation += 1;
      SLLStackPush(r_d3d11_state->first_free_tex2d, tex);
    }
    for(R_D3D11_Buffer *buf = r_d3d11_state->first_to_free_buffer, *next = 0;
        buf != 0;
        buf = next)
    {
      next = buf->next;
      buf->buffer->Release();
      buf->generation += 1;
      SLLStackPush(r_d3d11_state->first_free_buffer, buf);
    }
    arena_clear(r_d3d11_state->buffer_flush_arena);
    r_d3d11_state->first_buffer_to_flush = r_d3d11_state->last_buffer_to_flush = 0;
    r_d3d11_state->first_to_free_tex2d  = 0;
    r_d3d11_state->first_to_free_buffer = 0;
#endif
  }
}

r_hook void
r_window_begin_frame(OS_Handle window_handle, R_Handle window_equip)
{
  ProfBeginFunction();
  OS_MutexScopeW(r_ogl_state->device_rw_mutex)
  {
    R_OGL_Window *window = r_ogl_window_from_handle(window_equip);

    //- rjf: map os window handle -> hwnd
    HWND hwnd = {0};
    {
      W32_Window *w32_layer_window = w32_window_from_os_window(window_handle);
      hwnd = w32_hwnd_from_window(w32_layer_window);
    }
    HDC window_dc = GetDC(hwnd);
    bool ok = wglMakeCurrent(window_dc, window->glrc);

    //- dmylo: get resolution
    Rng2F32 client_rect = os_client_rect_from_window(window_handle);
    Vec2S32 resolution = {(S32)(client_rect.x1 - client_rect.x0), (S32)(client_rect.y1 - client_rect.y0)};


    auto gl = &r_ogl_state->gl;
    gl->gl.Viewport(0, 0, resolution.x, resolution.y);
    gl->gl.ClearColor(0.5, 0.5, 0, 1.0);
    gl->gl.Clear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(window_dc);

    ReleaseDC(hwnd, window_dc);
  }
}

r_hook void
r_window_end_frame(OS_Handle window, R_Handle window_equip)
{

}


//- rjf: render pass submission
r_hook void
r_window_submit(OS_Handle window, R_Handle window_equip, R_PassList *passes)
{

}