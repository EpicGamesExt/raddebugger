// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "metagen/metagen_os/core/metagen_os_core.c"

#if OS_WINDOWS
# include "metagen/metagen_os/core/win32/metagen_os_core_win32.c"
#elif OS_LINUX
# include "metagen/metagen_os/core/linux/metagen_os_core_linux.c"
#else
# error OS core layer not implemented for this operating system.
#endif
