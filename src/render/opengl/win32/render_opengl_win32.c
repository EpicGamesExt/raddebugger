// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal VoidProc *
r_ogl_os_load_procedure(char *name)
{
  VoidProc *p = (VoidProc*)wglGetProcAddress(name);
  if(p == (VoidProc*)1 || p == (VoidProc*)2 || p == (VoidProc*)3 || p == (VoidProc*)-1)
  {
    p = 0;
  }
  return p;
}

internal void
r_ogl_os_init(CmdLine *cmdline)
{
  //- rjf: create bootstrapping window
  HWND bootstrap_hwnd = 0;
  {
    WNDCLASSEXW wndclass = { sizeof(wndclass) };
    wndclass.lpfnWndProc = DefWindowProcW;
    wndclass.hInstance = GetModuleHandle(0);
    wndclass.lpszClassName = L"bootstrap-window";
    ATOM wndatom = RegisterClassExW(&wndclass);
    bootstrap_hwnd = CreateWindowExW(0, L"bootstrap-window", L"", 0,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     0, 0, wndclass.hInstance, 0);
  }
  
  //- rjf: grab dc
  HDC dc = GetDC(bootstrap_hwnd);
  
  //- rjf: build pixel format descriptor
  int pf = 0;
  {
    PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd)};
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pf = ChoosePixelFormat(dc, &pfd);
    BOOL describe = DescribePixelFormat(dc, pf, sizeof(pfd), &pfd);
    BOOL set_pf = SetPixelFormat(dc, pf, &pfd);
  }
  
  //- rjf: make bootstrap ctx + make current
  HGLRC bootstrap_ctx = wglCreateContext(dc);
  wglMakeCurrent(dc, bootstrap_ctx);
  
  //- rjf: load modern extensions
  wglChoosePixelFormatARB    = (FNWGLCHOOSEPIXELFORMATARBPROC*)   r_ogl_os_load_procedure("wglChoosePixelFormatARB");
  wglCreateContextAttribsARB = (FNWGLCREATECONTEXTATTRIBSARBPROC*)r_ogl_os_load_procedure("wglCreateContextAttribsARB");
  wglSwapIntervalEXT         = (FNWGLSWAPINTERVALEXTPROC*)        r_ogl_os_load_procedure("wglSwapIntervalEXT");
  
  //- rjf: set up real pixel format
  {
    int pf_attribs_i[] =
    {
      WGL_DRAW_TO_WINDOW_ARB, 1,
      WGL_SUPPORT_OPENGL_ARB, 1,
      WGL_DOUBLE_BUFFER_ARB, 1,
      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
      WGL_COLOR_BITS_ARB, 32,
      WGL_DEPTH_BITS_ARB, 24,
      WGL_STENCIL_BITS_ARB, 8,
      0
    };
    UINT num_formats = 0;
    wglChoosePixelFormatARB(dc, pf_attribs_i, 0, 1, &pf, &num_formats);
  }
  
  //- rjf: make real gl ctx
  HGLRC real_ctx = 0;
  if(pf)
  {
    B32 debug_mode = cmd_line_has_flag(cmdline, str8_lit("opengl_debug"));
#if BUILD_DEBUG
    debug_mode = 1;
#endif
    int context_attribs[] =
    {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
      WGL_CONTEXT_MINOR_VERSION_ARB, 3,
      WGL_CONTEXT_FLAGS_ARB, !!debug_mode*WGL_CONTEXT_DEBUG_BIT_ARB,
      0
    };
    real_ctx = wglCreateContextAttribsARB(dc, bootstrap_ctx, context_attribs);
    r_ogl_w32_hglrc = real_ctx;
  }
  
  //- rjf: clean up bootstrap context
  wglMakeCurrent(dc, 0);
  wglDeleteContext(bootstrap_ctx);
  wglMakeCurrent(dc, real_ctx);
  wglSwapIntervalEXT(1);
}

internal R_Handle
r_ogl_os_window_equip(OS_Handle window)
{
  //- rjf: unpack window
  OS_W32_Window *w = os_w32_window_from_handle(window);
  HWND hwnd = w->hwnd;
  HDC hdc = w->hdc;
  
  //- rjf: select in ctx
  wglMakeCurrent(hdc, r_ogl_w32_hglrc);
  
  //- rjf: setup real pixel format
  int pixel_format = 0;
  UINT num_formats = 0;
  int pf_attribs_i[] =
  {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB, 32,
    WGL_DEPTH_BITS_ARB, 24,
    WGL_STENCIL_BITS_ARB, 8,
    0
  };
  wglChoosePixelFormatARB(hdc,
                          pf_attribs_i,
                          0,
                          1,
                          &pixel_format,
                          &num_formats);
  
  // NOTE(rjf): This doesn't seem to be necessary for SetPixelFormat, we can
  // just pass 0 for it, and SetPixelFormat needs to be called here, but the
  // docs don't seem to suggest that 0 is an acceptable value, so I am just
  // filling this out with the same attribs as that for the wgl function,
  // and passing it.
  PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd)};
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 24;
  pfd.cStencilBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;
  
  //- rjf: set pixel format
  SetPixelFormat(hdc, pixel_format, &pfd);
  
  //- rjf: release hdc
  R_Handle result = {0};
  return result;
}

internal void
r_ogl_os_window_unequip(OS_Handle os, R_Handle r)
{
}

internal void
r_ogl_os_select_window(OS_Handle os, R_Handle r)
{
  OS_W32_Window *w = os_w32_window_from_handle(os);
  if(w != 0)
  {
    HWND hwnd = w->hwnd;
    HDC hdc = w->hdc;
    wglMakeCurrent(hdc, r_ogl_w32_hglrc);
  }
}

internal void
r_ogl_os_window_swap(OS_Handle os, R_Handle r)
{
  OS_W32_Window *w = os_w32_window_from_handle(os);
  if(w != 0)
  {
    HDC dc = w->hdc;
    SwapBuffers(dc);
  }
}
