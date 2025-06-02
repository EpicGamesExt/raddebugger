// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Backend Includes

#if R_OPENGL_LINUX_BACKEND == R_OPENGL_LINUX_BACKEND_GLX
# include "glx/render_opengl_linux_glx.c"
#elif R_OPENGL_LINUX_BACKEND == R_OPENGL_LINUX_BACKEND_EGL
# include "egl/render_opengl_linux_egl.c"
#else
# error Linux OpenGL backend not specified.
#endif
