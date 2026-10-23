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
  W32_SOCK_Connection *prev;
  SOCK_Endpoint endpoint;
  SOCK_Protocol protocol;
  SOCKET socket;
  U64 recv_buffer_cap;
  U8 *recv_buffer;
  U64 recv_buffer_pos;
  DWORD recv_size;
  OVERLAPPED recv_overlapped;
};

typedef struct W32_SOCK_ConnectionSlot W32_SOCK_ConnectionSlot;
struct W32_SOCK_ConnectionSlot
{
  W32_SOCK_Connection *first;
  W32_SOCK_Connection *last;
};

typedef struct W32_SOCK_Session W32_SOCK_Session;
struct W32_SOCK_Session
{
  Arena *arena;
  SOCK_WakeupFunctionType *wakeup_hook;
  U64 recv_packet_size;
  GuardedRing *recv_ring;
  
  // rjf: connection cache
  StripeArray connection_stripes;
  U64 connection_slots_count;
  W32_SOCK_ConnectionSlot *connection_slots;
  
  // rjf: socket accepting resources
  SOCKET tcp_listen_socket;
  SOCKET tcp_accept_socket;
  OVERLAPPED tcp_accept_overlapped;
  U8 tcp_accept_buffer[4096];
  DWORD tcp_accept_size_out;
  LPFN_ACCEPTEX lpfnAcceptEx;
};

typedef struct W32_SOCK_State W32_SOCK_State;
struct W32_SOCK_State
{
  Arena *arena;
  GuardedRing *u2s_ring;
  HANDLE iocp;
  Thread listen_thread;
  Thread sender_thread;
};

global W32_SOCK_State *w32_sock_state = 0;

////////////////////////////////
//~ rjf: Listener Thread

internal void w32_sock_listen_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Sender Thread

internal void w32_sock_sender_thread_entry_point(void *p);

#endif // WIN32_SOCKET_H
