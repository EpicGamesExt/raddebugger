// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "demon/demon_core.c"

#if OS_WINDOWS
# include "win32/demon/win32_demon.c"
#elif OS_LINUX
# include "linux/demon/linux_demon.c"
#else
# error Demon layer backend not defined for this operating system.
#endif
