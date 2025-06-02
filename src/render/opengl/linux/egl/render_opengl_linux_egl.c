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
  
  //- rjf: get EGL display
  {
    r_ogl_lnx_state->display = eglGetDisplay((EGLNativeDisplayType)os_lnx_gfx_state->display);
    if(r_ogl_lnx_state->display == EGL_NO_DISPLAY)
    {
      os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Failed to get EGL display."));
      os_abort(1);
    }
  }
  
  //- rjf: initialize GL version
  EGLint egl_version_major = 0;
  EGLint egl_version_minor = 0;
  if(!eglInitialize(r_ogl_lnx_state->display, &egl_version_major, &egl_version_minor))
  {
    os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't initialize EGL display."));
    os_abort(1);
  }
  if(egl_version_major < 1 || (egl_version_major == 1 && egl_version_minor < 5))
  {
    Temp scratch = scratch_begin(0, 0);
    String8 message = push_str8f(scratch.arena, "Unsupported EGL version (%i.%i, need at least 1.5)", egl_version_major, egl_version_minor);
    os_graphical_message(1, str8_lit("Fatal Error"), message);
    os_abort(1);
    scratch_end(scratch);
  }
  
  //- rjf: pick GL API
  if(!eglBindAPI(EGL_OPENGL_API))
  {
    os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't initialize EGL API to OpenGL."));
    os_abort(1);
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
    r_ogl_lnx_state->context = eglCreateContext(r_ogl_lnx_state->display, 0, EGL_NO_CONTEXT, options);
    if(r_ogl_lnx_state->context == EGL_NO_CONTEXT)
    {
      os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't create OpenGL context with EGL."));
      os_abort(1);
    }
  }
  
  eglMakeCurrent(r_ogl_lnx_state->display, 0, 0, r_ogl_lnx_state->context);
  glDrawBuffer(GL_BACK);
}

internal R_Handle
r_ogl_os_window_equip(OS_Handle window)
{
  OS_LNX_Window *window_os = (OS_LNX_Window *)window.u64[0];
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
    if(r_ogl_lnx_state->config == 0)
    {
      //- rjf: get all EGL configs
      EGLConfig configs[256] = {0};
      EGLint configs_count = 0;
      {
        EGLint options[] =
        {
          EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
          EGL_CONFORMANT,        EGL_OPENGL_BIT,
          EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
          EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
          
          EGL_RED_SIZE,      8,
          EGL_GREEN_SIZE,    8,
          EGL_BLUE_SIZE,     8,
          EGL_DEPTH_SIZE,   24,
          EGL_STENCIL_SIZE,  8,
          
          EGL_NONE,
        };
        if(!eglChooseConfig(r_ogl_lnx_state->display, options, configs, ArrayCount(configs), &configs_count) || configs_count == 0)
        {
          os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't choose EGL configuration."));
          os_abort(1);
        }
      }
      
      //- rjf: actually choose the egl config
      {
        EGLint config_options[] =
        {
          EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
          EGL_NONE,
        };
        for(U32 idx = 0; idx < configs_count; idx += 1)
        {
          w->surface = eglCreateWindowSurface(r_ogl_lnx_state->display, configs[idx], window_os->window, config_options);
          if(w->surface != EGL_NO_SURFACE)
          {
            r_ogl_lnx_state->config = configs[idx];
            break;
          }
        }
        if(r_ogl_lnx_state->config == 0)
        {
          os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't find a suitable EGL configuration."));
          os_abort(1);
        }
      }
    }
    else
    {
      w->surface = eglCreateWindowSurface(r_ogl_lnx_state->display, r_ogl_lnx_state->config, window_os->window, surface_options);
    }
    if(w->surface == EGL_NO_SURFACE)
    {
      os_graphical_message(1, str8_lit("Fatal Error"), str8_lit("Couldn't create EGL surface."));
      os_abort(1);
    }
  }
  R_Handle result = {(U64)w};
  return result;
}

internal void
r_ogl_os_window_unequip(OS_Handle os, R_Handle r)
{
  R_OGL_LNX_Window *w = (R_OGL_LNX_Window *)r.u64[0];
  {
    
  }
  SLLStackPush(r_ogl_lnx_state->free_window, w);
}

internal void
r_ogl_os_select_window(OS_Handle os, R_Handle r)
{
  OS_LNX_Window *w = (OS_LNX_Window *)os.u64[0];
  R_OGL_LNX_Window *w_r = (R_OGL_LNX_Window *)r.u64[0];
  eglMakeCurrent(r_ogl_lnx_state->display, w_r->surface, w_r->surface, r_ogl_lnx_state->context);
}

internal void
r_ogl_os_window_swap(OS_Handle os, R_Handle r)
{
  OS_LNX_Window *w = (OS_LNX_Window *)os.u64[0];
  R_OGL_LNX_Window *w_r = (R_OGL_LNX_Window *)r.u64[0];
  eglSwapBuffers(r_ogl_lnx_state->display, w_r->surface);
}
