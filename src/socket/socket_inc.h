// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SOCKET_INC_H
#define SOCKET_INC_H

#include "socket.h"
#if OS_WINDOWS
# include "win32/socket/win32_socket.h"
#else
# include "socket_stub.h"
#endif

#endif // SOCKET_INC_H
