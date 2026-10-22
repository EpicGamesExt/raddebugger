// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_SOCKET_H
#define LINUX_SOCKET_H

////////////////////////////////
//~ rjf: Includes

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

////////////////////////////////
//~ rjf: Implementation Types

typedef struct LNX_SOCK_Connection LNX_SOCK_Connection;
struct LNX_SOCK_Connection
{
  LNX_SOCK_Connection *next;
  LNX_SOCK_Connection *prev;
  SOCK_Endpoint endpoint;
  SOCK_Protocol protocol;
  int socket;
};

typedef struct LNX_SOCK_ConnectionSlot LNX_SOCK_ConnectionSlot;
struct LNX_SOCK_ConnectionSlot
{
  LNX_SOCK_Connection *first;
  LNX_SOCK_Connection *last;
};

typedef struct LNX_SOCK_Session LNX_SOCK_Session;
struct LNX_SOCK_Session
{
  LNX_SOCK_Session *next;
  LNX_SOCK_Session *prev;
  Arena *arena;
  GuardedRing *u2s_ring;
  GuardedRing *s2u_ring;
  int epoll_fd;
  SOCK_WakeupFunctionType *wakeup_hook;
  int tcp_listen_socket;
  U64 connection_slots_count;
  StripeArray connection_stripes;
  LNX_SOCK_ConnectionSlot *connection_slots;
  Thread listener_thread;
};

typedef struct LNX_SOCK_State LNX_SOCK_State;
struct LNX_SOCK_State
{
  Arena *arena;
  RWMutex session_rw_mutex;
  LNX_SOCK_Session *first_session;
  LNX_SOCK_Session *last_session;
};

////////////////////////////////
//~ rjf: Globals

global LNX_SOCK_State *lnx_sock_state = 0;

////////////////////////////////
//~ rjf: Listener Thread

internal void lnx_sock2_listener_thread_entry_point(void *p);

#endif // LINUX_SOCKET_H
