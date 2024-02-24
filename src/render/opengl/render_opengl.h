#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#pragma comment(lib, "opengl32")

#define IMGL3W_IMPL
#include "render_opengl_defines.h"

struct R_OGL_Window
{
  R_OGL_Window *next;
  U64 generation;
  HGLRC glrc;
};

struct R_OGL_State
{
  // R_OGL_Functions gl;
  bool initialized;
  ImGL3WProcs gl;

  Arena *arena;
  R_OGL_Window *first_free_window;
  OS_Handle device_rw_mutex;
};

////////////////////////////////
//~ dmylo: Globals
global R_OGL_State *r_ogl_state = 0;
global R_OGL_Window r_ogl_window_nil = {&r_ogl_window_nil};

#endif // RENDER_OPENGL_H
