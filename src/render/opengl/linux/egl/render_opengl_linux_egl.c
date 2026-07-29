// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal VoidProc *
r_ogl_os_load_procedure(char *name)
{
  VoidProc *result = (VoidProc *)eglGetProcAddress(name);
  return result;
}

internal void
r_ogl_os_init(CmdLine *cmdln)
{
  //- rjf: set up state
  {
    Arena *arena = arena_alloc();
    r_ogl_lnx_state = push_array(arena, R_OGL_LNX_State, 1);
    r_ogl_lnx_state->arena = arena;
  }
  
  {
    EGLNativeDisplayType native_display = (lnx_wm_backend == LNX_WM_Backend_Wayland)
      ? (EGLNativeDisplayType)wl_wm_state->display
      : (EGLNativeDisplayType)lnx_wm_state->display;
    r_ogl_lnx_state->display = eglGetDisplay(native_display);
    if(r_ogl_lnx_state->display == EGL_NO_DISPLAY)
    {
      wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Failed to get EGL display."));
      abort_self(1);
    }
  }
  
  //- rjf: initialize GL version
  EGLint egl_version_major = 0;
  EGLint egl_version_minor = 0;
  if(!eglInitialize(r_ogl_lnx_state->display, &egl_version_major, &egl_version_minor))
  {
    wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't initialize EGL display."));
    abort_self(1);
  }
  if(egl_version_major < 1 || (egl_version_major == 1 && egl_version_minor < 5))
  {
    Temp scratch = scratch_begin(0, 0);
    String8 message = push_str8f(scratch.arena, "Unsupported EGL version (%i.%i, need at least 1.5)", egl_version_major, egl_version_minor);
    wm_graphical_message(1, str8_lit("Fatal Error"), message);
    abort_self(1);
    scratch_end(scratch);
  }
  
  //- rjf: pick GL API
  if(!eglBindAPI(EGL_OPENGL_API))
  {
    wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't initialize EGL API to OpenGL."));
    abort_self(1);
  }
  
  //- choose EGL config, extract matching X11 visual/colormap, publish to os layer
  {
    EGLint config_filter[] =
    {
      EGL_SURFACE_TYPE,      EGL_WINDOW_BIT|EGL_PBUFFER_BIT,
      EGL_CONFORMANT,        EGL_OPENGL_BIT,
      EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
      EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
      EGL_RED_SIZE,          8,
      EGL_GREEN_SIZE,        8,
      EGL_BLUE_SIZE,         8,
      EGL_DEPTH_SIZE,        24,
      EGL_STENCIL_SIZE,      8,
      EGL_NONE,
    };
    EGLConfig configs[256] = {0};
    EGLint configs_count = 0;
    if(!eglChooseConfig(r_ogl_lnx_state->display, config_filter, configs, ArrayCount(configs), &configs_count) || configs_count == 0)
    {
      wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't choose EGL configuration."));
      abort_self(1);
    }
    B32 found_config = 0;
    for(EGLint idx = 0; idx < configs_count && !found_config; idx += 1)
    {
      EGLint probe_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB, EGL_NONE };
      EGLSurface probe = eglCreatePbufferSurface(r_ogl_lnx_state->display, configs[idx], probe_attribs);
      if(probe == EGL_NO_SURFACE) { continue; }
      eglDestroySurface(r_ogl_lnx_state->display, probe);
      if(lnx_wm_backend == LNX_WM_Backend_X11)
      {
        EGLint native_visual_id = 0;
        if(!eglGetConfigAttrib(r_ogl_lnx_state->display, configs[idx], EGL_NATIVE_VISUAL_ID, &native_visual_id) || native_visual_id == 0)
        {
          continue;
        }
        XVisualInfo vi_template = {0};
        vi_template.visualid = native_visual_id;
        vi_template.screen = DefaultScreen(lnx_wm_state->display);
        int vi_count = 0;
        XVisualInfo *vi = XGetVisualInfo(lnx_wm_state->display, VisualIDMask|VisualScreenMask, &vi_template, &vi_count);
        if(vi == 0 || vi_count == 0) { if(vi) { XFree(vi); } continue; }
        lnx_wm_state->window_visual = vi->visual;
        lnx_wm_state->window_depth = vi->depth;
        lnx_wm_state->window_colormap = XCreateColormap(lnx_wm_state->display,
                                                        XRootWindow(lnx_wm_state->display, vi->screen),
                                                        vi->visual, AllocNone);
        XFree(vi);
      }
      r_ogl_lnx_state->config = configs[idx];
      found_config = 1;
    }
    if(!found_config)
    {
      wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't find a suitable sRGB-capable EGL config."));
      abort_self(1);
    }
  }
  
  //- rjf: construct context
  {
    B32 debug_mode = cmd_line_has_flag(cmdln, str8_lit("opengl_debug"));
#if BUILD_DEBUG
    debug_mode = 1;
#endif
    EGLint options[] =
    {
      EGL_CONTEXT_MAJOR_VERSION, 3,
      EGL_CONTEXT_MINOR_VERSION, 3,
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      debug_mode ? EGL_CONTEXT_OPENGL_DEBUG : EGL_NONE, EGL_TRUE,
      EGL_NONE,
    };
    r_ogl_lnx_state->context = eglCreateContext(r_ogl_lnx_state->display, r_ogl_lnx_state->config, EGL_NO_CONTEXT, options);
    if(r_ogl_lnx_state->context == EGL_NO_CONTEXT)
    {
      wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't create OpenGL context with EGL."));
      abort_self(1);
    }
  }
  
  //- bind context to 1x1 pbuffer so GL calls work before any window exists
  {
    EGLint pbuffer_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface init_surface = eglCreatePbufferSurface(r_ogl_lnx_state->display, r_ogl_lnx_state->config, pbuffer_attribs);
    if(init_surface == EGL_NO_SURFACE || !eglMakeCurrent(r_ogl_lnx_state->display, init_surface, init_surface, r_ogl_lnx_state->context))
    {
      eglMakeCurrent(r_ogl_lnx_state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, r_ogl_lnx_state->context);
    }
  }
}

internal R_Handle
r_ogl_os_window_equip(WM_Window window)
{
  EGLNativeWindowType native_window;
  if(lnx_wm_backend == LNX_WM_Backend_Wayland)
  {
    native_window = (EGLNativeWindowType)((WL_WM_Window *)window.u64[0])->egl_window;
  }
  else
  {
    native_window = (EGLNativeWindowType)((LNX_WM_Window *)window.u64[0])->window;
  }
  R_OGL_LNX_Window *w = r_ogl_lnx_state->free_window;
  if(w != 0)
  {
    SLLStackPop(r_ogl_lnx_state->free_window);
  }
  else
  {
    w = push_array(r_ogl_lnx_state->arena, R_OGL_LNX_Window, 1);
  }
  {
    EGLint surface_options[] =
    {
      EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
      EGL_NONE,
    };
    w->surface = eglCreateWindowSurface(r_ogl_lnx_state->display, r_ogl_lnx_state->config, native_window, surface_options);
    if(w->surface == EGL_NO_SURFACE)
    {
      EGLint surface_options_linear[] = { EGL_NONE };
      w->surface = eglCreateWindowSurface(r_ogl_lnx_state->display, r_ogl_lnx_state->config, native_window, surface_options_linear);
    }
    if(w->surface == EGL_NO_SURFACE)
    {
      wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't create EGL surface."));
      abort_self(1);
    }
  }
  R_Handle result = {(U64)w};
  return result;
}

internal void
r_ogl_os_window_unequip(WM_Window os, R_Handle r)
{
  R_OGL_LNX_Window *w = (R_OGL_LNX_Window *)r.u64[0];
  {
  }
  SLLStackPush(r_ogl_lnx_state->free_window, w);
}

internal void
r_ogl_os_select_window(WM_Window os, R_Handle r)
{
  R_OGL_Window *outer = (R_OGL_Window *)r.u64[0];
  R_OGL_LNX_Window *w_r = (R_OGL_LNX_Window *)outer->os.u64[0];
  eglMakeCurrent(r_ogl_lnx_state->display, w_r->surface, w_r->surface, r_ogl_lnx_state->context);
}

internal void
r_ogl_os_window_swap(WM_Window os, R_Handle r)
{
  R_OGL_Window *outer = (R_OGL_Window *)r.u64[0];
  R_OGL_LNX_Window *w_r = (R_OGL_LNX_Window *)outer->os.u64[0];
  eglSwapBuffers(r_ogl_lnx_state->display, w_r->surface);
  if(lnx_wm_backend == LNX_WM_Backend_X11)
  {
    lnx_window_finish_frame_sync(os);
  }
}
