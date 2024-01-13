// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// NOTE(allen): Include OS features for extra features and target OS

#include "core/os_core.c"

#if OS_FEATURE_SOCKET
#include "socket/os_socket.c"
#endif

#if OS_FEATURE_GRAPHICAL
#include "gfx/os_gfx.c"
#endif

#if OS_WINDOWS
# include "core/win32/os_core_win32.c"
# if OS_FEATURE_SOCKET
#  include "socket/win32/os_socket_win32.c"
# endif
# if OS_FEATURE_GRAPHICAL && !OS_GFX_STUB
#  include "gfx/win32/os_gfx_win32.c"
# endif
#elif OS_LINUX
# include "core/linux/os_core_linux.c"
#else
# error no OS layer setup
#endif

#if OS_GFX_STUB
#include "gfx/stub/os_gfx_stub.c"
#endif
