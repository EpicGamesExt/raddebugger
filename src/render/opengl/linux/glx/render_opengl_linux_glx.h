// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_LINUX_GLX_H
#define RENDER_OPENGL_LINUX_GLX_H

#define glTexImage3D glTexImage3D__static
#define glTexSubImage3D glTexSubImage3D__static
#define glActiveTexture glActiveTexture__static
#include <GL/gl.h>
#include <GL/glx.h>
#undef glTexImage3D
#undef glTexSubImage3D
#undef glActiveTexture

#define GLX_CONTEXT_MAJOR_VERSION_ARB          0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB          0x2092
#define GLX_CONTEXT_FLAGS_ARB                  0x2094
#define GLX_CONTEXT_DEBUG_BIT_ARB              0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

global GLXContext r_ogl_lnx_ctx = 0;

#endif // RENDER_OPENGL_LINUX_GLX_H
