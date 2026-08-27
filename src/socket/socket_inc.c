// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "socket.c"
#if OS_WINDOWS
# include "win32/socket/win32_socket.c"
#else
# include "socket_stub.c"
#endif
