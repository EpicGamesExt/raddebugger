// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "demon/demon_core.c"

#if OS_WINDOWS
# include "demon/win32/demon_core_win32.c"
#elif OS_LINUX
# include "demon/linux/demon_core_linux.c"
#else
# error Demon layer backend not defined for this operating system.
#endif
