// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "font_provider.c"

#if LANG_CPP
# if FP_BACKEND == FP_BACKEND_DWRITE
#  include "dwrite/font_provider_dwrite.cpp"
# else
#  error Font provider backend not specified.
# endif
#endif
