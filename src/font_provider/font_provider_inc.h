// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef FONT_PROVIDER_INC_H
#define FONT_PROVIDER_INC_H

////////////////////////////////
//~ rjf: Backend Constants

#define FP_BACKEND_DWRITE 1

////////////////////////////////
//~ rjf: Decide On Backend

#if !defined(FP_BACKEND) && OS_WINDOWS
# define FP_BACKEND FP_BACKEND_DWRITE
#endif

////////////////////////////////
//~ rjf: Main Includes

#include "font_provider.h"

////////////////////////////////
//~ rjf: Backend Includes

#if LANG_CPP
# if FP_BACKEND == FP_BACKEND_DWRITE
#  include "dwrite/font_provider_dwrite.h"
# else
#  error Font provider backend not specified.
# endif
#endif

#endif // FONT_PROVIDER_INC_H
