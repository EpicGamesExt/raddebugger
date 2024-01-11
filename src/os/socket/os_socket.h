// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_SOCKET_H
#define OS_SOCKET_H

enum OS_SocketStatus{
  OS_SocketStatus_Uninitialized,
  OS_SocketStatus_Connected,
  OS_SocketStatus_GracefullyClosed,
  OS_SocketStatus_Error,
};

typedef U16 OS_SocketError;
enum{
  OS_SocketError_None,
  OS_SocketError_SocketSystemNotInitialized,
  OS_SocketError_BadPortArgument,
  OS_SocketError_BadIPArgument,
  OS_SocketError_WSAError,
};

struct OS_Socket{
  U8 memory[32];
};


////////////////////////////////
//~ NOTE(allen): Implemented Per Operating System

internal void os_socket_init(void);

internal void os_socket_listen(OS_Socket *socket, String8 port);
internal void os_socket_connect(OS_Socket *socket, String8 ip, String8 port);
internal void os_socket_close(OS_Socket *socket);

internal String8 os_socket_read(Arena *arena, OS_Socket *socket);
internal B32     os_socket_write(OS_Socket *socket, String8List list);

internal B32     os_socket_status(OS_Socket *socket, OS_SocketStatus status);
internal String8 os_socket_error_string(Arena *arena, OS_Socket *socket);
internal void    os_socket_assert_on_error(OS_Socket *socket, B32 assert_on_error);

////////////////////////////////
//~ NOTE(allen): Helpers - Portable Implementation

internal B32 os_socket_write(OS_Socket *socket, String8 data);

#endif //OS_SOCKET_H
