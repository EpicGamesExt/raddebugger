// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FONT_PROVIDER_INC_H
#define FONT_PROVIDER_INC_H

////////////////////////////////
//~ rjf: Backend Constants

#define FP_BACKEND_DWRITE 1
#define FP_BACKEND_FREETYPE 2

////////////////////////////////
//~ rjf: Decide On Backend

#if !defined(FP_BACKEND) && OS_WINDOWS
# define FP_BACKEND FP_BACKEND_DWRITE
#elif !defined(FP_BACKEND) && OS_LINUX
#define FP_BACKEND FP_BACKEND_FREETYPE
#endif

////////////////////////////////
//~ rjf: Main Includes

#include "font_provider.h"

////////////////////////////////
//~ rjf: Backend Includes

#if FP_BACKEND == FP_BACKEND_DWRITE
  #include "dwrite/font_provider_dwrite.h"
#elif FP_BACKEND == FP_BACKEND_FREETYPE
  #include "freetype/font_provider_freetype.h"
#else
# error Font provider backend not specified.
#endif

#endif // FONT_PROVIDER_INC_H
