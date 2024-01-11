// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// NOTE(allen): Include OS features for extra features and target OS

#include "core/metagen_os_core.c"

#if OS_FEATURE_SOCKET
#include "socket/metagen_os_socket.c"
#endif

#if OS_FEATURE_GRAPHICAL
#include "gfx/metagen_os_gfx.c"
#endif

#if OS_WINDOWS
# include "core/win32/metagen_os_core_win32.c"
# if OS_FEATURE_SOCKET
#  include "socket/win32/metagen_os_socket_win32.c"
# endif
# if OS_FEATURE_GRAPHICAL
#  include "gfx/win32/metagen_os_gfx_win32.c"
# endif
#elif OS_LINUX
# include "core/linux/metagen_os_core_linux.c"
#else
# error no OS layer setup
#endif
