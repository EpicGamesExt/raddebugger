// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RENDER_INC_H
#define RENDER_INC_H

////////////////////////////////
//~ rjf: Backend Constants

#define R_BACKEND_STUB 0
#define R_BACKEND_D3D11 1
#define R_BACKEND_OPENGL 2

////////////////////////////////
//~ rjf: Decide On Backend

#if !defined(R_BACKEND) && OS_WINDOWS
# define R_BACKEND R_BACKEND_D3D11
#elif !defined(R_BACKEND) && OS_LINUX
# define R_BACKEND R_BACKEND_OPENGL
#endif

////////////////////////////////
//~ rjf: Main Includes

#include "render_core.h"

////////////////////////////////
//~ rjf: Backend Includes

#if R_BACKEND == R_BACKEND_STUB
# include "stub/render_stub.h"
#elif R_BACKEND == R_BACKEND_D3D11
# include "d3d11/render_d3d11.h"
#elif R_BACKEND == R_BACKEND_OPENGL
# include "opengl/render_opengl.h"
#else
# error Renderer backend not specified.
#endif

#endif // RENDER_INC_H
