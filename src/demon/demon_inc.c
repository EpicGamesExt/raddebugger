// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "demon_core.c"
#include "demon_common.c"
#include "demon_accel.c"
#include "demon_os.c"

#if OS_WINDOWS
# include "win32/demon_os_win32.c"
#elif OS_LINUX
# include "linux/demon_os_linux.c"
#else
# error No Demon Implementation for This OS
#endif
