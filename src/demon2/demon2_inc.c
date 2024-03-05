// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "demon2_core.c"

#if OS_WINDOWS
# include "win32/demon2_core_win32.c"
#else
# error Demon layer backend not defined for this operating system.
#endif
