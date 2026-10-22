// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SOCKET2_INC_H
#define SOCKET2_INC_H

#include "socket2.h"
#if OS_WINDOWS
# include "win32/socket2/win32_socket2.h"
#elif OS_LINUX
# include "linux/socket2/linux_socket2.h"
#else
# error No socket layer defined for this operating system.
#endif

#endif // SOCKET2_INC_H
