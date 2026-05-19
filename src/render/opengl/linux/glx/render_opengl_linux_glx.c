// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal VoidProc *
r_ogl_os_load_procedure(char *name)
{
  VoidProc *result = (VoidProc *)glXGetProcAddressARB((U8 *)name);
  return result;
}

internal void
r_ogl_os_init(CmdLine *cmdln)
{
  //- rjf: require GLX 1.3+
  int glx_version_major = 0;
  int glx_version_minor = 0;
  if(!glXQueryVersion(lnx_wm_state->display, &glx_version_major, &glx_version_minor) ||
     (glx_version_major == 1 && glx_version_minor < 3) ||
     glx_version_major < 1)
  {
    Temp scratch = scratch_begin(0, 0);
    String8 message = push_str8f(scratch.arena, "Unsupported GLX version (%i.%i, need at least 1.3)", glx_version_major, glx_version_minor);
    wm_graphical_message(1, str8_lit("Fatal Error"), message);
    abort_self(1);
    scratch_end(scratch);
  }
  
  //- rjf: get frame buffer configs
  local_persist int framebuffer_config_options[] =
  {
    GLX_X_RENDERABLE,   1,
    GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,    GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
    GLX_RED_SIZE,       8,
    GLX_GREEN_SIZE,     8,
    GLX_BLUE_SIZE,      8,
    GLX_ALPHA_SIZE,     8,
    GLX_DEPTH_SIZE,     24,
    GLX_STENCIL_SIZE,   8,
    GLX_DOUBLEBUFFER,   1,
    None
  };
  int framebuffer_configs_count = 0;
  GLXFBConfig *framebuffer_configs = glXChooseFBConfig(lnx_wm_state->display, DefaultScreen(lnx_wm_state->display), framebuffer_config_options, &framebuffer_configs_count);
  if(framebuffer_configs == 0)
  {
    wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Could not find a suitable framebuffer configuration."));
    abort_self(1);
  }
  GLXFBConfig framebuffer_config = framebuffer_configs[0];
  XFree(framebuffer_configs);

  //- extract visual/colormap from chosen fbconfig, publish to os layer
  {
    XVisualInfo *vi = glXGetVisualFromFBConfig(lnx_wm_state->display, framebuffer_config);
    if(vi == 0)
    {
      wm_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Could not get visual from GLX framebuffer config."));
      abort_self(1);
    }
    lnx_wm_state->window_visual = vi->visual;
    lnx_wm_state->window_depth = vi->depth;
    lnx_wm_state->window_colormap = XCreateColormap(lnx_wm_state->display,
                                                        XRootWindow(lnx_wm_state->display, vi->screen),
                                                        vi->visual, AllocNone);
    XFree(vi);
  }
  
  //- rjf: construct context
  {
    B32 debug_mode = cmd_line_has_flag(cmdln, str8_lit("opengl_debug"));
#if BUILD_DEBUG
    debug_mode = 1;
#endif
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((U8 *)"glXCreateContextAttribsARB");
    int context_options[] =
    {
      GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
      GLX_CONTEXT_MINOR_VERSION_ARB, 3,
      GLX_CONTEXT_FLAGS_ARB,         !!debug_mode*GLX_CONTEXT_DEBUG_BIT_ARB,
      None
    };
    r_ogl_lnx_ctx = glXCreateContextAttribsARB(lnx_wm_state->display, framebuffer_config, 0, 1, context_options);
  }
  
  glXMakeCurrent(lnx_wm_state->display, 0, r_ogl_lnx_ctx);
}

internal R_Handle
r_ogl_os_window_equip(WM_Window window)
{
  R_Handle result = {0};
  return result;
}

internal void
r_ogl_os_window_unequip(WM_Window os, R_Handle r)
{
  
}

internal void
r_ogl_os_select_window(WM_Window os, R_Handle r)
{
  LNX_WM_Window *w = (LNX_WM_Window *)os.u64[0];
  glXMakeCurrent(lnx_wm_state->display, w->window, r_ogl_lnx_ctx);
  // ensure default framebuffer writes target the back buffer; on some drivers
  // GL_DRAW_BUFFER stays GL_NONE if a context was first made current without a drawable.
  glDrawBuffer(GL_BACK);
}

internal void
r_ogl_os_window_swap(WM_Window os, R_Handle r)
{
  LNX_WM_Window *w = (LNX_WM_Window *)os.u64[0];
  glXSwapBuffers(lnx_wm_state->display, w->window);
}
