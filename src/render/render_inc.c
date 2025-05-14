// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xc22121ff

#include "render_core.c"

#if R_BACKEND == R_BACKEND_STUB
# include "stub/render_stub.c"
#elif R_BACKEND == R_BACKEND_D3D11
# include "d3d11/render_d3d11.c"
#elif R_BACKEND == R_BACKEND_OPENGL
# include "opengl/render_opengl.c"
#else
# error Renderer backend not specified.
#endif
