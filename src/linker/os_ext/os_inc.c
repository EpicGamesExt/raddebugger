// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "core/os_core.c"

#if OS_WINDOWS
//# include "core/win32/os_core_win32.c"
#elif OS_LINUX
//# include "core/linux/os_core_linux.c"
#else
# error no OS layer setup
#endif


