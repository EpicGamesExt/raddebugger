// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_INC_H
#define OS_INC_H

#if !defined(OS_FEATURE_SOCKET)
# define OS_FEATURE_SOCKET 0
#endif

#if !defined(OS_FEATURE_GRAPHICAL)
# define OS_FEATURE_GRAPHICAL 0
#endif

#include "core/metagen_os_core.h"

#if OS_FEATURE_SOCKET
#include "socket/metagen_os_socket.h"
#endif

#if OS_FEATURE_GRAPHICAL
#include "gfx/metagen_os_gfx.h"
#endif

#if OS_WINDOWS
# include "core/win32/metagen_os_core_win32.h"
# if OS_FEATURE_SOCKET
#  include "socket/win32/metagen_os_socket_win32.h"
# endif
# if OS_FEATURE_GRAPHICAL
#  include "gfx/win32/metagen_os_gfx_win32.h"
# endif
#elif OS_LINUX
# include "core/linux/metagen_os_core_linux.h"
#else
# error no OS layer setup
#endif

#endif // OS_INC_H
