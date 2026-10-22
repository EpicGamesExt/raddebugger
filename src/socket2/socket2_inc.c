// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "socket2.c"
#if OS_WINDOWS
# include "win32/socket2/win32_socket2.c"
#elif OS_LINUX
# include "linux/socket2/linux_socket2.c"
#else
# error No socket layer defined for this operating system.
#endif
