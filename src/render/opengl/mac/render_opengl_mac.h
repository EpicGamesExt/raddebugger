// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_LINUX_EGL_H
#define RENDER_OPENGL_LINUX_EGL_H

#define GL_GLEXT_FUNCTION_POINTERS 1
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>

// To get proc address
#include <dlfcn.h>

global id r_ogl_mac_ctx = 0;

#endif // RENDER_OPENGL_LINUX_EGL_H
