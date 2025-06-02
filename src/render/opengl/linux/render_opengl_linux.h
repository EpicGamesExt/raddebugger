// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_LINUX_H
#define RENDER_OPENGL_LINUX_H

////////////////////////////////
//~ rjf: Backend Constants

#define R_OPENGL_LINUX_BACKEND_GLX 0
#define R_OPENGL_LINUX_BACKEND_EGL 1

////////////////////////////////
//~ rjf: Decide On Backend

#if !defined(R_OPENGL_LINUX_BACKEND)
# define R_OPENGL_LINUX_BACKEND R_OPENGL_LINUX_BACKEND_EGL
#endif

////////////////////////////////
//~ rjf: Backend Includes

#if R_OPENGL_LINUX_BACKEND == R_OPENGL_LINUX_BACKEND_GLX
# include "glx/render_opengl_linux_glx.h"
#elif R_OPENGL_LINUX_BACKEND == R_OPENGL_LINUX_BACKEND_EGL
# include "egl/render_opengl_linux_egl.h"
#else
# error Linux OpenGL backend not specified.
#endif

#endif // RENDER_OPENGL_LINUX_H
