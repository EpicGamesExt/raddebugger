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
  struct LNX_SOCK_Session *session;
  SOCK_Endpoint endpoint;
  SOCK_Protocol protocol;
  int socket;
  U8 *recv_buffer;
  U64 recv_buffer_pos;
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
  Arena *arena;
  SOCK_WakeupFunctionType *wakeup_hook;
  U64 recv_packet_size;
  GuardedRing *recv_ring;
  
  // rjf: connection cache
  StripeArray connection_stripes;
  U64 connection_slots_count;
  LNX_SOCK_ConnectionSlot *connection_slots;
  
  // rjf: listen socket
  int tcp_listen_socket;
};

typedef struct LNX_SOCK_State LNX_SOCK_State;
struct LNX_SOCK_State
{
  Arena *arena;
  GuardedRing *u2s_ring;
  int epoll_fd;
  Thread listen_thread;
  Thread sender_thread;
};

////////////////////////////////
//~ rjf: Globals

global LNX_SOCK_State *lnx_sock_state = 0;

////////////////////////////////
//~ rjf: Listener Thread

internal void lnx_sock_listen_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Sender Thread

internal void lnx_sock_sender_thread_entry_point(void *p);

#endif // LINUX_SOCKET_H
