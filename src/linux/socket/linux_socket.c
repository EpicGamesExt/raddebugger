// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Listener Thread

internal void
lnx_sock_listener_thread_entry_point(void *p)
{
  ThreadNameF("lnx_sock_listener_thread_%I64x", p);
  LNX_SOCK_Session *session = (LNX_SOCK_Session *)p;
  for(;;)
  {
    //- rjf: wait for next event
    struct epoll_event evts[1] = {0};
    int wait_result = LNX_RETRY_ON_EINTR(epoll_wait(session->epoll_fd, evts, ArrayCount(evts), -1));
    
    //- rjf: call wakeup hook
    if(session->wakeup_hook)
    {
      session->wakeup_hook();
    }
    
    //- rjf: code is 0 -> listener is ready for accept
    if(evts[0].data.u64 == 0)
    {
      // rjf: accept new connection
      struct sockaddr_storage addr = {0};
      socklen_t addr_size = sizeof(addr);
      int new_socket = LNX_RETRY_ON_EINTR(accept(session->tcp_listen_socket, (struct sockaddr *)&addr, &addr_size));
      
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
            MemoryCopy(endpoint.address_u8, ipv6->sin6_addr.s6_addr, sizeof(endpoint.address_u8));
          }break;
        }
      }
      
      // rjf: unpack endpoint
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%session->connection_slots_count;
      LNX_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
      
      // rjf: store new connection
      U64 con_ptr_u64 = 0;
      RWMutexScope(stripe->rw_mutex, 1)
      {
        LNX_SOCK_Connection *con = (LNX_SOCK_Connection *)stripe->free;
        if(con != 0)
        {
          stripe->free = con->next;
        }
        else
        {
          con = push_array(stripe->arena, LNX_SOCK_Connection, 1);
        }
        con->endpoint = endpoint;
        con->protocol = SOCK_Protocol_TCP;
        con->socket = new_socket;
        DLLPushBack(slot->first, slot->last, con);
        con_ptr_u64 = (U64)con;
      }
      
      // rjf: add socket to epoll
      struct epoll_event evt = {0};
      evt.events = EPOLLIN;
      evt.data.u64 = con_ptr_u64;
      LNX_RETRY_ON_EINTR(epoll_ctl(session->epoll_fd, EPOLL_CTL_ADD, new_socket, &evt));
    }
    
    //- rjf: code is nonzero -> ready to read on a connection.
    else
    {
      // rjf: unpack associated connection
      LNX_SOCK_Connection *con = (LNX_SOCK_Connection *)evts[0].data.u64;
      SOCK_Endpoint endpoint = con->endpoint;
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%session->connection_slots_count;
      LNX_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
      
      // rjf: do read
      U8 buffer[4096] = {0};
      ssize_t recv_result = LNX_RETRY_ON_EINTR(recv(con->socket, buffer, sizeof(buffer), MSG_DONTWAIT));
      
      // rjf: bytes received -> push result to user.
      if(recv_result > 0)
      {
        RingGuard g = guarded_ring_open(session->s2u_ring);
        U64 header[5] =
        {
          (U64)SOCK_Protocol_TCP,
          (U64)endpoint.port,
          endpoint.address_u64[0],
          endpoint.address_u64[1],
          (U64)recv_result,
        };
        guarded_ring_write_or_wait(&g, sizeof(header), header, max_U64);
        guarded_ring_write_or_wait(&g, recv_result, buffer, max_U64);
        guarded_ring_close(&g);
      }
      
      // rjf: error -> socket was disconnected
      else RWMutexScope(stripe->rw_mutex, 1)
      {
        close(con->socket);
        struct epoll_event evt = {0}; // NOTE(rjf): required on Linux pre-2.6.9
        LNX_RETRY_ON_EINTR(epoll_ctl(session->epoll_fd, EPOLL_CTL_DEL, con->socket, &evt));
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
  Arena *arena = arena_alloc();
  lnx_sock_state = push_array(arena, LNX_SOCK_State, 1);
  lnx_sock_state->arena = arena;
  lnx_sock_state->session_rw_mutex = rw_mutex_alloc();
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
    LNX_SOCK_Session *session;
    SOCK_Endpoint endpoint;
    String8 data;
  };
  SendTask *first_tcp_send = 0;
  SendTask *last_tcp_send = 0;
  if(lane_idx() == 0)
  {
    for(;;)
    {
      B32 got_more = 0;
      RWMutexScope(lnx_sock_state->session_rw_mutex, 0)
      {
        for EachNode(s, LNX_SOCK_Session, lnx_sock_state->first_session)
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
    LNX_SOCK_Session *session = t->session;
    
    // rjf: unpack endpoint
    U64 hash = u64_hash_from_str8(str8_struct(&t->endpoint));
    U64 slot_idx = hash%session->connection_slots_count;
    LNX_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
    Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
    
    // rjf: get existing socket for this endpoint
    int ep_socket = -1;
    RWMutexScope(stripe->rw_mutex, 0)
    {
      for(LNX_SOCK_Connection *c = slot->first; c != 0; c = c->next)
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
      LNX_SOCK_Connection *con = 0;
      for(LNX_SOCK_Connection *c = slot->first; c != 0; c = c->next)
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
        int new_socket = socket(AF_INET, SOCK_STREAM, 0);
        connect(new_socket, (struct sockaddr *)&endpoint_sockaddr, endpoint_sockaddr_size);
        
        // rjf: store in cache
        con = (LNX_SOCK_Connection *)stripe->free;
        if(con != 0)
        {
          stripe->free = con->next;
        }
        else
        {
          con = push_array(stripe->arena, LNX_SOCK_Connection, 1);
        }
        con->endpoint = t->endpoint;
        con->protocol = SOCK_Protocol_TCP;
        con->socket = new_socket;
        DLLPushBack(slot->first, slot->last, con);
        
        // rjf: hook up to session epoll
        struct epoll_event evt = {0};
        evt.events = EPOLLIN;
        evt.data.u64 = (U64)con;
        LNX_RETRY_ON_EINTR(epoll_ctl(session->epoll_fd, EPOLL_CTL_ADD, new_socket, &evt));
      }
      
      // rjf: get socket from cache
      ep_socket = con->socket;
    }
    
    // rjf: got socket? -> send
    B32 send_failed = 0;
    if(ep_socket != -1 && send(ep_socket, t->data.str, t->data.size, MSG_NOSIGNAL) == -1)
    {
      send_failed = 1;
    }
    
    // rjf: got a socket, but send failed? -> connection closed
    if(ep_socket != -1 && send_failed)
    {
      RWMutexScope(stripe->rw_mutex, 1)
      {
        for(LNX_SOCK_Connection *c = slot->first; c != 0; c = c->next)
        {
          if(MemoryMatchStruct(&c->endpoint, &t->endpoint))
          {
            close(c->socket);
            struct epoll_event evt = {0}; // NOTE(rjf): required on Linux pre-2.6.9
            LNX_RETRY_ON_EINTR(epoll_ctl(session->epoll_fd, EPOLL_CTL_DEL, c->socket, &evt));
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
  LNX_SOCK_Session *session = push_array(arena, LNX_SOCK_Session, 1);
  session->arena = arena;
  session->u2s_ring = guarded_ring_alloc(arena, KB(256));
  session->s2u_ring = guarded_ring_alloc(arena, KB(256));
  session->epoll_fd = epoll_create(1);
  session->wakeup_hook = wakeup_hook;
  
  //- rjf: set up listener
  if(listener_port != 0)
  {
    session->tcp_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    U32 ipv6only = 0;
    setsockopt(session->tcp_listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(listener_port);
    bind(session->tcp_listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(session->tcp_listen_socket, SOMAXCONN);
    struct epoll_event evt = {0};
    evt.events = EPOLLIN;
    epoll_ctl(session->epoll_fd, EPOLL_CTL_ADD, session->tcp_listen_socket, &evt);
  }
  
  //- rjf: set up connection cache
  session->connection_slots_count = 8;
  session->connection_stripes = stripe_array_alloc(arena);
  session->connection_slots = push_array(arena, LNX_SOCK_ConnectionSlot, session->connection_slots_count);
  
  //- rjf: launch listener thread
  session->listener_thread = thread_launch(lnx_sock_listener_thread_entry_point, session);
  
  //- rjf: link into top-level storage
  RWMutexScope(lnx_sock_state->session_rw_mutex, 1)
  {
    DLLPushBack(lnx_sock_state->first_session, lnx_sock_state->last_session, session);
  }
  
  //- rjf: bundle as handle
  SOCK_Session s = {(U64)session};
  return s;
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
  LNX_SOCK_Session *s = (LNX_SOCK_Session *)session.u64[0];
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
    LNX_SOCK_Session *s = (LNX_SOCK_Session *)session.u64[0];
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
