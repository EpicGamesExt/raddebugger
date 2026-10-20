// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Listener Threads

internal void
w32_sock_listener_thread_entry_point(void *p)
{
  ThreadNameF("w32_sock_listener_thread_%I64x", (U64)p);
  W32_SOCK_Session *session = (W32_SOCK_Session *)p;
  for(;;)
  {
    //- rjf: get next completion from IOCP
    DWORD byte_count = 0;
    U64 completion_key = 0;
    OVERLAPPED *overlapped_ptr = 0;
    B32 success = GetQueuedCompletionStatus(session->iocp, &byte_count, &completion_key, &overlapped_ptr, INFINITE);
    
    //- rjf: call wakeup hook
    if(session->wakeup_hook)
    {
      session->wakeup_hook();
    }
    
    //- rjf: overlapped_ptr == accept overlapped? -> new connection
    if(overlapped_ptr == &session->tcp_accept_overlapped)
    {
      // rjf: unpack new socket
      SOCKET new_socket = session->tcp_accept_socket;
      struct sockaddr_storage addr = {0};
      MemoryCopy(&addr, session->tcp_accept_buffer + sizeof(struct sockaddr_storage) + 16, sizeof(addr));
      
      // rjf: unpack socket's endpoint info
      SOCK_Endpoint endpoint = {0};
      {
        switch(addr.ss_family)
        {
          default:{}break;
          case AF_INET:
          {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)&addr;
            endpoint.port = ntohs(ipv4->sin_port);
            endpoint.address_u32[0] = ipv4->sin_addr.s_addr;
          }break;
          case AF_INET6:
          {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&addr;
            endpoint.port = ntohs(ipv6->sin6_port);
            MemoryCopy(endpoint.address_u8, ipv6->sin6_addr.u.Byte, sizeof(endpoint.address_u8));
          }break;
        }
      }
      
      // rjf: unpack endpoint
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%session->connection_slots_count;
      W32_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
      
      // rjf: store new connection
      WSABUF buf = {0};
      DWORD *recv_size = 0;
      OVERLAPPED *recv_overlapped = 0;
      RWMutexScope(stripe->rw_mutex, 1)
      {
        W32_SOCK_Connection *con = (W32_SOCK_Connection *)stripe->free;
        if(con != 0)
        {
          stripe->free = con->next;
        }
        else
        {
          con = push_array(stripe->arena, W32_SOCK_Connection, 1);
        }
        con->endpoint = endpoint;
        con->protocol = SOCK_Protocol_TCP;
        con->socket = new_socket;
        MemoryZeroStruct(&con->recv_overlapped);
        DLLPushBack(slot->first, slot->last, con);
        buf.len = sizeof(con->recv_buffer);
        buf.buf = (char *)con->recv_buffer;
        recv_size = &con->recv_size;
        recv_overlapped = &con->recv_overlapped;
      }
      
      // rjf: kick off receive
      DWORD flags = MSG_PUSH_IMMEDIATE;
      WSARecv(new_socket, &buf, 1, recv_size, &flags, recv_overlapped, 0);
      
      // rjf: create new accept socket, associate with iocp, zero overlapped, kick off next accept
      session->tcp_accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      CreateIoCompletionPort((HANDLE)session->tcp_accept_socket, session->iocp, 0, 0);
      MemoryZeroStruct(&session->tcp_accept_overlapped);
      session->lpfnAcceptEx(session->tcp_listen_socket, session->tcp_accept_socket, session->tcp_accept_buffer, 0, sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16, &session->tcp_accept_size_out, &session->tcp_accept_overlapped);
    }
    
    //- rjf: overlapped ptr anywhere else -> completion of async recv
    else
    {
      // rjf: unpack associated connection
      W32_SOCK_Connection *con = CastFromMember(W32_SOCK_Connection, recv_overlapped, overlapped_ptr);
      SOCK_Endpoint endpoint = con->endpoint;
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%session->connection_slots_count;
      W32_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
      
      // rjf: success? -> push result to user. kick off next recv
      if(success)
      {
        RingGuard g = guarded_ring_open(session->s2u_ring);
        U64 header[5] =
        {
          (U64)SOCK_Protocol_TCP,
          (U64)endpoint.port,
          endpoint.address_u64[0],
          endpoint.address_u64[1],
          (U64)byte_count,
        };
        guarded_ring_write_or_wait(&g, sizeof(header), header, max_U64);
        guarded_ring_write_or_wait(&g, byte_count, con->recv_buffer, max_U64);
        MemoryZeroStruct(&con->recv_overlapped);
        WSABUF buf = {sizeof(con->recv_buffer), (char *)con->recv_buffer};
        DWORD flags = MSG_PUSH_IMMEDIATE;
        WSARecv(con->socket, &buf, 1, &con->recv_size, &flags, &con->recv_overlapped, 0);
        guarded_ring_close(&g);
      }
      
      // rjf: no success? -> connection closed.
      else RWMutexScope(stripe->rw_mutex, 1)
      {
        closesocket(con->socket);
        DLLRemove(slot->first, slot->last, con);
        con->next = stripe->free;
        stripe->free = con;
      }
    }
  }
}

////////////////////////////////
//~ rjf: @per_os_impl Top-Level Layer Calls

internal void
sock_init(void)
{
  // NOTE(rjf): winsock2 is already initialized by the base layer for RIO function grabbing.
  
  //- rjf: set up top-level state
  Arena *arena = arena_alloc();
  w32_sock_state = push_array(arena, W32_SOCK_State, 1);
  w32_sock_state->arena = arena;
  w32_sock_state->session_rw_mutex = rw_mutex_alloc();
}

internal void
sock_async_tick(void)
{
  Temp scratch = scratch_begin(0, 0);
  
  //////////////////////////////
  //- rjf: gather send tasks
  //
  typedef struct SendTask SendTask;
  struct SendTask
  {
    SendTask *next;
    W32_SOCK_Session *session;
    SOCK_Endpoint endpoint;
    String8 data;
  };
  SendTask *first_tcp_send = 0;
  SendTask *last_tcp_send = 0;
  SendTask *first_udp_send = 0;
  SendTask *last_udp_send = 0;
  if(lane_idx() == 0)
  {
    for(;;)
    {
      B32 got_more = 0;
      RWMutexScope(w32_sock_state->session_rw_mutex, 0)
      {
        for EachNode(s, W32_SOCK_Session, w32_sock_state->first_session)
        {
          RingGuard g = guarded_ring_open(s->u2s_ring);
          {
            U64 header[5] = {0};
            if(guarded_ring_try_read(&g, sizeof(header), header))
            {
              got_more = 1;
              SOCK_Protocol protocol = (SOCK_Protocol)header[0];
              U16 port = (U16)header[1];
              SOCK_Endpoint endpoint = {0};
              endpoint.address_u64[0] = header[2];
              endpoint.address_u64[1] = header[3];
              endpoint.port = port;
              U64 data_size = header[4];
              U8 *data = push_array(scratch.arena, U8, data_size);
              guarded_ring_read_or_wait(&g, data_size, data, max_U64);
              SendTask *t = push_array(scratch.arena, SendTask, 1);
              t->session = s;
              t->endpoint = endpoint;
              t->data = str8(data, data_size);
              SLLQueuePush(first_tcp_send, last_tcp_send, t);
            }
          }
          guarded_ring_close(&g);
        }
      }
      if(!got_more)
      {
        break;
      }
    }
  }
  lane_sync();
  
  //////////////////////////////
  //- rjf: do TCP sends
  //
  for(SendTask *t = first_tcp_send; t != 0; t = t->next)
  {
    W32_SOCK_Session *session = t->session;
    
    // rjf: unpack endpoint
    U64 hash = u64_hash_from_str8(str8_struct(&t->endpoint));
    U64 slot_idx = hash%session->connection_slots_count;
    W32_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
    
    // rjf: get existing socket for this endpoint
    SOCKET ep_socket = -1;
    RWMutexScope(stripe->rw_mutex, 0)
    {
      for(W32_SOCK_Connection *c = slot->first; c != 0; c = c->next)
      {
        if(MemoryMatchStruct(&c->endpoint, &t->endpoint))
        {
          ep_socket = c->socket;
          break;
        }
      }
    }
    
    // rjf: didn't get a socket? -> open socket
    if(ep_socket == -1) RWMutexScope(stripe->rw_mutex, 1)
    {
      // rjf: try to get socket again, now that we have the write lock
      W32_SOCK_Connection *con = 0;
      for(W32_SOCK_Connection *c = slot->first; c != 0; c = c->next)
      {
        if(MemoryMatchStruct(&c->endpoint, &t->endpoint))
        {
          con = c;
          break;
        }
      }
      
      // rjf: no socket still? -> create
      if(con == 0)
      {
        // rjf: convert endpoint -> sockaddr
        struct sockaddr_storage endpoint_sockaddr = {0};
        int endpoint_sockaddr_size = 0;
        {
          switch(t->endpoint.kind)
          {
            default:{}break;
            case SOCK_EndpointKind_IPv4:
            {
              struct sockaddr_in *dst = (struct sockaddr_in *)(&endpoint_sockaddr);
              endpoint_sockaddr_size = sizeof(*dst);
              dst->sin_family = AF_INET;
              dst->sin_port = htons(t->endpoint.port);
              MemoryCopy(&dst->sin_addr, &t->endpoint.address_u32[0], sizeof(U32));
            }break;
            case SOCK_EndpointKind_IPv6:
            {
              struct sockaddr_in6 *dst = (struct sockaddr_in6 *)(&endpoint_sockaddr);
              endpoint_sockaddr_size = sizeof(*dst);
              dst->sin6_family = AF_INET6;
              dst->sin6_port = htons(t->endpoint.port);
              MemoryCopy(&dst->sin6_addr, &t->endpoint.address_u128[0], sizeof(U128));
            }break;
          }
        }
        
        // rjf: create
        SOCKET new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        connect(new_socket, (struct sockaddr *)&endpoint_sockaddr, endpoint_sockaddr_size);
        
        // rjf: store in cache
        con = (W32_SOCK_Connection *)stripe->free;
        if(con != 0)
        {
          stripe->free = con->next;
        }
        else
        {
          con = push_array(stripe->arena, W32_SOCK_Connection, 1);
        }
        con->endpoint = t->endpoint;
        con->protocol = SOCK_Protocol_TCP;
        con->socket = new_socket;
        DLLPushBack(slot->first, slot->last, con);
        
        // rjf: associate this socket with iocp
        CreateIoCompletionPort((HANDLE)new_socket, session->iocp, 0, 0);
        MemoryZeroStruct(&con->recv_overlapped);
        
        // rjf: kick off receive on this socket
        WSABUF buf = {sizeof(con->recv_buffer), (char *)con->recv_buffer};
        DWORD flags = MSG_PUSH_IMMEDIATE;
        WSARecv(con->socket, &buf, 1, &con->recv_size, &flags, &con->recv_overlapped, 0);
      }
      
      // rjf: get socket from cache
      ep_socket = con->socket;
    }
    
    // rjf: got socket? -> send
    B32 send_failed = 0;
    if(ep_socket != -1 && send(ep_socket, (char *)t->data.str, t->data.size, 0) == SOCKET_ERROR)
    {
      int error = WSAGetLastError();
      if(error == WSAECONNRESET)
      {
        send_failed = 1;
      }
    }
    
    // rjf: got a socket, but send failed? -> connection closed
    if(ep_socket != -1 && send_failed)
    {
      RWMutexScope(stripe->rw_mutex, 1)
      {
        for(W32_SOCK_Connection *c = slot->first; c != 0; c = c->next)
        {
          if(MemoryMatchStruct(&c->endpoint, &t->endpoint))
          {
            DLLRemove(slot->first, slot->last, c);
            c->next = stripe->free;
            stripe->free = c;
          }
        }
      }
    }
  }
  
  scratch_end(scratch);
}

////////////////////////////////
//~ rjf: @per_os_impl Session Creation/Closing

internal SOCK_Session
sock_session_open(U16 listener_port, SOCK_WakeupFunctionType *wakeup_hook)
{
  //- rjf: set up state
  Arena *arena = arena_alloc();
  W32_SOCK_Session *session = push_array(arena, W32_SOCK_Session, 1);
  session->arena = arena;
  session->u2s_ring = guarded_ring_alloc(arena, KB(256));
  session->s2u_ring = guarded_ring_alloc(arena, KB(256));
  
  //- rjf: set up IOCP
  session->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
  
  //- rjf: set up listener(s)
  if(listener_port != 0)
  {
    session->tcp_listen_socket = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    DWORD ipv6only = 0;
    setsockopt(session->tcp_listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(listener_port);
    bind(session->tcp_listen_socket, (SOCKADDR *)&server_addr, sizeof(server_addr));
    listen(session->tcp_listen_socket, SOMAXCONN);
  }
  
  //- rjf: associate listener sockets with IOCP
  {
    CreateIoCompletionPort((HANDLE)session->tcp_listen_socket, session->iocp, 0, 0);
  }
  
  //- rjf: load AcceptEx function
  DWORD dwBytes = 0;
  GUID AcceptEx_guid = WSAID_ACCEPTEX;
  WSAIoctl(session->tcp_listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &AcceptEx_guid, sizeof(AcceptEx_guid), &session->lpfnAcceptEx, sizeof(session->lpfnAcceptEx), &dwBytes, 0, 0);
  
  //- rjf: create accepting socket
  if(listener_port != 0)
  {
    session->tcp_accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  }
  
  //- rjf: associate accepting socket with IOCP
  if(listener_port != 0)
  {
    CreateIoCompletionPort((HANDLE)session->tcp_accept_socket, session->iocp, 0, 0);
  }
  
  //- rjf: kick off accept
  if(listener_port != 0)
  {
    session->lpfnAcceptEx(session->tcp_listen_socket, session->tcp_accept_socket, session->tcp_accept_buffer, 0, sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16, &session->tcp_accept_size_out, &session->tcp_accept_overlapped);
  }
  
  //- rjf: set up connection cache
  session->connection_slots_count = 8;
  session->connection_slots = push_array(arena, W32_SOCK_ConnectionSlot, session->connection_slots_count);
  session->connection_stripes = stripe_array_alloc(arena);
  
  //- rjf: set wakeup hook
  session->wakeup_hook = wakeup_hook;
  
  //- rjf: launch one-off listener threads to block & accept connections
  session->tcp_listener_thread = thread_launch(w32_sock_listener_thread_entry_point, session);
  
  //- rjf: connect to top-level state
  RWMutexScope(w32_sock_state->session_rw_mutex, 1)
  {
    DLLPushBack(w32_sock_state->first_session, w32_sock_state->last_session, session);
  }
  
  //- rjf: bundle as handle
  SOCK_Session result = {(U64)session};
  return result;
}

internal void
sock_session_close(SOCK_Session session)
{
  // TODO(rjf)
}

////////////////////////////////
//~ rjf: @per_os_impl Sends

internal B32
sock_send(SOCK_Session session, SOCK_Protocol protocol, SOCK_Endpoint endpoint, String8 data, U64 endt_us)
{
  B32 result = 0;
  W32_SOCK_Session *s = (W32_SOCK_Session *)session.u64[0];
  RingGuard guard = guarded_ring_open(s->u2s_ring);
  {
    U64 header[5] = {0};
    U64 size_cap = s->u2s_ring->ring->size - sizeof(header);
    U64 size = Min(data.size, size_cap);
    {
      header[0] = (U64)protocol;
      header[1] = (U64)endpoint.port;
      header[2] = endpoint.address_u64[0];
      header[3] = endpoint.address_u64[1];
      header[4] = size;
    }
    if(guarded_ring_write_or_wait(&guard, sizeof(header), header, endt_us))
    {
      guarded_ring_write_or_wait(&guard, size, data.str, max_U64);
      result = 1;
    }
  }
  guarded_ring_close(&guard);
  if(result)
  {
    ins_atomic_u32_eval_assign(&async_loop_again, 1);
    cond_var_broadcast(async_tick_start_cond_var);
  }
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Receives

internal B32
sock_recv(Arena *arena, SOCK_Session session, SOCK_Protocol *protocol_out, SOCK_Endpoint *endpoint_out, String8 *data_out, U64 endt_us)
{
  B32 result = 0;
  U64 header_size = sizeof(*protocol_out) + sizeof(*endpoint_out) + sizeof(U64);
  {
    W32_SOCK_Session *s = (W32_SOCK_Session *)session.u64[0];
    RingGuard guard = guarded_ring_open(s->s2u_ring);
    {
      U64 header[5] = {0};
      if(guarded_ring_read_or_wait(&guard, sizeof(header), header, endt_us))
      {
        U64 size_cap = s->s2u_ring->ring->size - sizeof(header);
        if(protocol_out != 0)
        {
          protocol_out[0] = (SOCK_Protocol)header[0];
        }
        endpoint_out->port = (U16)header[1];
        endpoint_out->address_u64[0] = header[2];
        endpoint_out->address_u64[1] = header[3];
        data_out->size = Min(size_cap, header[4]);
        data_out->str = push_array(arena, U8, data_out->size);
        guarded_ring_read_or_wait(&guard, data_out->size, data_out->str, max_U64);
        result = 1;
      }
    }
    guarded_ring_close(&guard);
  }
  return result;
}
