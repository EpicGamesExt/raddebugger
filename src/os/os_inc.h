// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_INC_H
#define OS_INC_H

#if !defined(OS_FEATURE_GRAPHICAL)
# define OS_FEATURE_GRAPHICAL 0
#endif

#if !defined(OS_GFX_STUB)
# define OS_GFX_STUB 0
#endif

#include "os/core/os_core.h"
#if OS_FEATURE_GRAPHICAL
# include "os/gfx/os_gfx.h"
#endif

#if OS_WINDOWS
# include "os/core/win32/os_core_win32.h"
#elif OS_LINUX
# include "os/core/linux/os_core_linux.h"
#else
# error OS core layer not implemented for this operating system.
#endif

#if OS_FEATURE_GRAPHICAL
# if OS_GFX_STUB
#  include "os/gfx/stub/os_gfx_stub.h"
# elif OS_WINDOWS
#  include "os/gfx/win32/os_gfx_win32.h"
# elif OS_LINUX
#  include "os/gfx/linux/os_gfx_linux.h"
# else
#  error OS graphical layer not implemented for this operating system.
# endif
#endif

#endif // OS_INC_H
