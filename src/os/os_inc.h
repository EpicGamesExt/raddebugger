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

#if !defined(OS_GFX_STUB)
# define OS_GFX_STUB 0
#endif

#include "core/os_core.h"

#if OS_FEATURE_SOCKET
#include "socket/os_socket.h"
#endif

#if OS_FEATURE_GRAPHICAL
#include "gfx/os_gfx.h"
#endif

#if OS_WINDOWS
# include "core/win32/os_core_win32.h"
# if OS_FEATURE_SOCKET
#  include "socket/win32/os_socket_win32.h"
# endif
# if OS_FEATURE_GRAPHICAL && !OS_GFX_STUB
#  include "gfx/win32/os_gfx_win32.h"
# endif
#elif OS_LINUX
# include "core/linux/os_core_linux.h"
#  if OS_FEATURE_GRAPHICAL && !OS_GFX_STUB
#    include "gfx/linux/os_gfx_linux.h"
#  endif
#else
# error no OS layer setup
#endif

#if OS_GFX_STUB
#include "gfx/stub/os_gfx_stub.h"
#endif

#endif //OS_SWITCH_H
