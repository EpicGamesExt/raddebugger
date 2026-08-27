// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_SOCKET_H
#define WIN32_SOCKET_H

#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")

typedef struct W32_SOCK_Connection W32_SOCK_Connection;
struct W32_SOCK_Connection
{
  W32_SOCK_Connection *next;
  SOCK_Endpoint endpoint;
  SOCK_Protocol protocol;
  SOCKET socket;
};

typedef struct W32_SOCK_State W32_SOCK_State;
struct W32_SOCK_State
{
  Arena *arena;
  GuardedRing *u2s_ring;
  GuardedRing *s2u_ring;
  SOCKET tcp_listen_socket;
  SOCKET udp_listen_socket;
  Thread tcp_listener_thread;
  Thread udp_listener_thread;
};

global W32_SOCK_State *w32_sock_state = 0;

////////////////////////////////
//~ rjf: Listener Threads

internal void w32_sock_tcp_listener_thread_entry_point(void *p);
internal void w32_sock_udp_listener_thread_entry_point(void *p);

#endif // WIN32_SOCKET_H
