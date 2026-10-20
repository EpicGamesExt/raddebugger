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
  U8 recv_buffer[4096];
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
  W32_SOCK_Session *next;
  W32_SOCK_Session *prev;
  Arena *arena;
  GuardedRing *u2s_ring;
  GuardedRing *s2u_ring;
  HANDLE iocp;
  SOCKET tcp_listen_socket;
  SOCKET tcp_accept_socket;
  OVERLAPPED tcp_accept_overlapped;
  U8 tcp_accept_buffer[4096];
  DWORD tcp_accept_size_out;
  Thread tcp_listener_thread;
  U64 connection_slots_count;
  StripeArray connection_stripes;
  W32_SOCK_ConnectionSlot *connection_slots;
  LPFN_ACCEPTEX lpfnAcceptEx;
  SOCK_WakeupFunctionType *wakeup_hook;
};

typedef struct W32_SOCK_State W32_SOCK_State;
struct W32_SOCK_State
{
  Arena *arena;
  RWMutex session_rw_mutex;
  W32_SOCK_Session *first_session;
  W32_SOCK_Session *last_session;
  Mutex send_mutex;
  CondVar send_cv;
  Thread send_thread;
};

////////////////////////////////
//~ rjf: Globals

global W32_SOCK_State *w32_sock_state = 0;

////////////////////////////////
//~ rjf: Threads

internal void w32_sock_sender_thread_entry_point(void *p);
internal void w32_sock_listener_thread_entry_point(void *p);

#endif // WIN32_SOCKET_H
