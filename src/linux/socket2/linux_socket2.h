// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_SOCKET2_H
#define LINUX_SOCKET2_H

////////////////////////////////
//~ rjf: Includes

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

////////////////////////////////
//~ rjf: Implementation Types

typedef struct LNX_SOCK2_Connection LNX_SOCK2_Connection;
struct LNX_SOCK2_Connection
{
  LNX_SOCK2_Connection *next;
  LNX_SOCK2_Connection *prev;
  struct LNX_SOCK2_Session *session;
  SOCK_Endpoint endpoint;
  SOCK_Protocol protocol;
  int socket;
  U8 *recv_buffer;
  U64 recv_buffer_pos;
};

typedef struct LNX_SOCK2_ConnectionSlot LNX_SOCK2_ConnectionSlot;
struct LNX_SOCK2_ConnectionSlot
{
  LNX_SOCK2_Connection *first;
  LNX_SOCK2_Connection *last;
};

typedef struct LNX_SOCK2_Session LNX_SOCK2_Session;
struct LNX_SOCK2_Session
{
  Arena *arena;
  SOCK_WakeupFunctionType *wakeup_hook;
  U64 recv_packet_size;
  GuardedRing *recv_ring;
  
  // rjf: connection cache
  StripeArray connection_stripes;
  U64 connection_slots_count;
  LNX_SOCK2_ConnectionSlot *connection_slots;
  
  // rjf: listen socket
  int tcp_listen_socket;
};

typedef struct LNX_SOCK2_State LNX_SOCK2_State;
struct LNX_SOCK2_State
{
  Arena *arena;
  GuardedRing *u2s_ring;
  int epoll_fd;
  Thread listen_thread;
  Thread sender_thread;
};

////////////////////////////////
//~ rjf: Globals

global LNX_SOCK2_State *lnx_sock2_state = 0;

////////////////////////////////
//~ rjf: Listener Thread

internal void lnx_sock2_listen_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Sender Thread

internal void lnx_sock2_sender_thread_entry_point(void *p);

#endif // LINUX_SOCKET2_H
