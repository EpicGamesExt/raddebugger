// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_OPENGL_WIN32_H
#define RENDER_OPENGL_WIN32_H

#include <GL/gl.h>
#pragma comment(lib, "opengl32")

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x00000001

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094

typedef BOOL WINAPI FNWGLCHOOSEPIXELFORMATARBPROC(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI FNWGLCREATECONTEXTATTRIBSARBPROC(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL WINAPI FNWGLSWAPINTERVALEXTPROC(int interval);

FNWGLCHOOSEPIXELFORMATARBPROC *wglChoosePixelFormatARB;
FNWGLCREATECONTEXTATTRIBSARBPROC *wglCreateContextAttribsARB;
FNWGLSWAPINTERVALEXTPROC *wglSwapIntervalEXT;

global HGLRC r_ogl_w32_hglrc = 0;

#endif // RENDER_OPENGL_WIN32_H
