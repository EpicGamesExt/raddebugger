// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_INC_H
#define OS_INC_H

#if !defined(OS_FEATURE_GRAPHICAL)
# define OS_FEATURE_GRAPHICAL 0
#endif

#if !defined(OS_GFX_STUB)
# define OS_GFX_STUB 0
#endif

#include "metagen/metagen_os/core/metagen_os_core.h"

#if OS_WINDOWS
# include "metagen/metagen_os/core/win32/metagen_os_core_win32.h"
#elif OS_LINUX
# include "metagen/metagen_os/core/linux/metagen_os_core_linux.h"
#else
# error OS core layer not implemented for this operating system.
#endif

#endif // OS_INC_H
