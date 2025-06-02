// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_LINUX_EGL_H
#define RENDER_OPENGL_LINUX_EGL_H

#define glTexImage3D glTexImage3D__static
#define glTexSubImage3D glTexSubImage3D__static
#define glActiveTexture glActiveTexture__static
#include <GL/gl.h>
#include <EGL/egl.h>
#undef glTexImage3D
#undef glTexSubImage3D
#undef glActiveTexture

typedef struct R_OGL_LNX_Window R_OGL_LNX_Window;
struct R_OGL_LNX_Window
{
  R_OGL_LNX_Window *next;
  EGLSurface *surface;
};

typedef struct R_OGL_LNX_State R_OGL_LNX_State;
struct R_OGL_LNX_State
{
  Arena *arena;
  EGLDisplay *display;
  EGLConfig config;
  EGLContext *context;
  R_OGL_LNX_Window *free_window;
};

global R_OGL_LNX_State *r_ogl_lnx_state = 0;

#endif // RENDER_OPENGL_LINUX_EGL_H
