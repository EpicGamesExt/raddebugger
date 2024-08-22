// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "os/core/os_core.c"
#if OS_FEATURE_GRAPHICAL
# include "os/gfx/os_gfx.c"
#endif

#if OS_WINDOWS
# include "os/core/win32/os_core_win32.c"
#elif OS_LINUX
# include "os/core/linux/os_core_linux.c"
#else
# error OS core layer not implemented for this operating system.
#endif

#if OS_FEATURE_GRAPHICAL
# if OS_GFX_STUB
#  include "os/gfx/stub/os_gfx_stub.c"
# elif OS_WINDOWS
#  include "os/gfx/win32/os_gfx_win32.c"
# elif OS_LINUX
#  include "os/gfx/linux/os_gfx_linux.c"
# else
#  error OS graphical layer not implemented for this operating system.
# endif
#endif
