// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Listener Thread

internal void
lnx_sock_listen_thread_entry_point(void *p)
{
  ThreadNameF("lnx_sock_listen_thread");
  for(;;)
  {
    //- rjf: wait for next event
    struct epoll_event evts[1] = {0};
    int wait_result = LNX_RETRY_ON_EINTR(epoll_wait(lnx_sock_state->epoll_fd, evts, ArrayCount(evts), -1));
    
    //- rjf: unpack event data
    //
    // 0 bit64 -> just a session pointer, no connection
    // 1 bit64 -> connection pointer, which refers to its session
    //
    LNX_SOCK_Session *session = 0;
    LNX_SOCK_Connection *con = 0;
    {
      if(evts[0].data.u64 & bit64)
      {
        con = (LNX_SOCK_Connection *)(evts[0].data.u64 & ~bit64);
        session = con->session;
      }
      else
      {
        session = (LNX_SOCK_Session *)evts[0].data.u64;
      }
    }
    
    //- rjf: connection is 0 -> session's listener is ready for accept
    if(con == 0)
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
          con->recv_buffer = push_array(stripe->arena, U8, session->recv_packet_size);
        }
        con->session = session;
        con->endpoint = endpoint;
        con->protocol = SOCK_Protocol_TCP;
        con->socket = new_socket;
        con->recv_buffer_pos = 0;
        DLLPushBack(slot->first, slot->last, con);
        con_ptr_u64 = (U64)con;
      }
      
      // rjf: add socket to epoll
      struct epoll_event evt = {0};
      evt.events = EPOLLIN;
      evt.data.u64 = con_ptr_u64 | bit64;
      LNX_RETRY_ON_EINTR(epoll_ctl(lnx_sock_state->epoll_fd, EPOLL_CTL_ADD, new_socket, &evt));
    }
    
    //- rjf: connection is nonzero -> ready to read on a connection.
    else if(con != 0)
    {
      // rjf: unpack associated connection
      SOCK_Endpoint endpoint = con->endpoint;
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%session->connection_slots_count;
      LNX_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
      
      // rjf: do read
      ssize_t recv_result = LNX_RETRY_ON_EINTR(recv(con->socket, con->recv_buffer + con->recv_buffer_pos, session->recv_packet_size - con->recv_buffer_pos, MSG_DONTWAIT));
      
      // rjf: recv buffer nonzero -> increment recv buffer position
      if(recv_result > 0)
      {
        con->recv_buffer_pos += recv_result;
      }
      
      // rjf: recv buffer full -> push to user
      if(con->recv_buffer_pos >= session->recv_packet_size)
      {
        RingGuard g = guarded_ring_open(session->recv_ring);
        {
          SOCK_Protocol protocol = SOCK_Protocol_TCP;
          guarded_ring_write_struct_or_wait(&g, &endpoint, max_U64);
          guarded_ring_write_struct_or_wait(&g, &protocol, max_U64);
          guarded_ring_write_or_wait(&g, session->recv_packet_size, con->recv_buffer, max_U64);
        }
        guarded_ring_close(&g);
      }
      
      // rjf: recv buffer full -> reset recv buffer
      if(con->recv_buffer_pos >= session->recv_packet_size)
      {
        con->recv_buffer_pos = 0;
      }
      
      // rjf: error -> socket was disconnected
      if(recv_result <= 0) RWMutexScope(stripe->rw_mutex, 1)
      {
        close(con->socket);
        struct epoll_event evt = {0}; // NOTE(rjf): required on Linux pre-2.6.9
        LNX_RETRY_ON_EINTR(epoll_ctl(lnx_sock_state->epoll_fd, EPOLL_CTL_DEL, con->socket, &evt));
        DLLRemove(slot->first, slot->last, con);
        con->next = stripe->free;
        stripe->free = con;
      }
    }
    
    //- rjf: call wakeup hook
    if(session->wakeup_hook)
    {
      session->wakeup_hook();
    }
  }
}

////////////////////////////////
//~ rjf: Sender Thread

internal void
lnx_sock_sender_thread_entry_point(void *p)
{
  ThreadNameF("lnx_sock_sender_thread");
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: wait for next send payload
    LNX_SOCK_Session *session = 0;
    SOCK_Endpoint endpoint = {0};
    SOCK_Protocol protocol = {0};
    SOCK_SendCompletionParams completion_params = {0};
    String8 data = {0};
    {
      RingGuard g = guarded_ring_open(lnx_sock_state->u2s_ring);
      {
        guarded_ring_read_struct_or_wait(&g, &session, max_U64);
        guarded_ring_read_struct_or_wait(&g, &endpoint, max_U64);
        guarded_ring_read_struct_or_wait(&g, &protocol, max_U64);
        guarded_ring_read_struct_or_wait(&g, &completion_params, max_U64);
        guarded_ring_read_struct_or_wait(&g, &data.size, max_U64);
        data.str = push_array_no_zero(scratch.arena, U8, data.size);
        guarded_ring_read_or_wait(&g, data.size, data.str, max_U64);
      }
      guarded_ring_close(&g);
    }
    
    //- rjf: unpack endpoint
    U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
    U64 slot_idx = hash%session->connection_slots_count;
    Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
    LNX_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
    
    //- rjf: endpoint -> socket
    int ep_socket = -1;
    RWMutexScope(stripe->rw_mutex, 0)
    {
      for(LNX_SOCK_Connection *c = slot->first; c != 0; c = c->next)
      {
        if(sock_endpoint_match(c->endpoint, endpoint) && c->protocol == protocol)
        {
          ep_socket = c->socket;
          break;
        }
      }
    }
    
    //- rjf: if we didn't find a socket -> create one & connect
    if(ep_socket == -1) RWMutexScope(stripe->rw_mutex, 1)
    {
      // rjf: try to get socket again, now that we have the right lock
      for(LNX_SOCK_Connection *c = slot->first; c != 0; c = c->next)
      {
        if(sock_endpoint_match(c->endpoint, endpoint) && c->protocol == protocol)
        {
          ep_socket = c->socket;
          break;
        }
      }
      
      // rjf: if we still didn't find the socket -> create
      if(ep_socket == -1)
      {
        // rjf: convert endpoint -> sockaddr
        struct sockaddr_storage endpoint_sockaddr = {0};
        int endpoint_sockaddr_size = 0;
        {
          switch(endpoint.kind)
          {
            default:{}break;
            case SOCK_EndpointKind_IPv4:
            {
              struct sockaddr_in *dst = (struct sockaddr_in *)(&endpoint_sockaddr);
              endpoint_sockaddr_size = sizeof(*dst);
              dst->sin_family = AF_INET;
              dst->sin_port = htons(endpoint.port);
              MemoryCopy(&dst->sin_addr, &endpoint.address_u32[0], sizeof(U32));
            }break;
            case SOCK_EndpointKind_IPv6:
            {
              struct sockaddr_in6 *dst = (struct sockaddr_in6 *)(&endpoint_sockaddr);
              endpoint_sockaddr_size = sizeof(*dst);
              dst->sin6_family = AF_INET6;
              dst->sin6_port = htons(endpoint.port);
              MemoryCopy(&dst->sin6_addr, &endpoint.address_u128[0], sizeof(U128));
            }break;
          }
        }
        
        // rjf: create
        int new_socket = socket(AF_INET, SOCK_STREAM, 0);
        connect(new_socket, (struct sockaddr *)&endpoint_sockaddr, endpoint_sockaddr_size);
        
        // rjf: allocate & store in cache
        LNX_SOCK_Connection *con = (LNX_SOCK_Connection *)stripe->free;
        if(con != 0)
        {
          stripe->free = con->next;
        }
        else
        {
          con = push_array(stripe->arena, LNX_SOCK_Connection, 1);
          con->recv_buffer = push_array(stripe->arena, U8, session->recv_packet_size);
        }
        con->session = session;
        con->endpoint = endpoint;
        con->protocol = protocol;
        con->socket = new_socket;
        con->recv_buffer_pos = 0;
        DLLPushBack(slot->first, slot->last, con);
        
        // rjf: hook up to epoll
        struct epoll_event evt = {0};
        evt.events = EPOLLIN;
        evt.data.u64 = (U64)con | bit64;
        LNX_RETRY_ON_EINTR(epoll_ctl(lnx_sock_state->epoll_fd, EPOLL_CTL_ADD, new_socket, &evt));
        
        // rjf: grab socket for sending
        ep_socket = con->socket;
      }
    }
    
    //- rjf: got socket -> send
    B32 send_failed = 0;
    B32 bad_connection = 0;
    if(ep_socket != -1 && send(ep_socket, (char *)data.str, data.size, MSG_NOSIGNAL) == -1)
    {
      send_failed = 1;
      bad_connection = 1;
    }
    
    //- rjf: got socket, but detected bad connection? -> connection closed
    if(ep_socket != -1 && bad_connection)
    {
      RWMutexScope(stripe->rw_mutex, 1)
      {
        for(LNX_SOCK_Connection *c = slot->first; c != 0; c = c->next)
        {
          if(sock_endpoint_match(c->endpoint, endpoint) && c->protocol == protocol)
          {
            close(c->socket);
            DLLRemove(slot->first, slot->last, c);
            c->next = stripe->free;
            stripe->free = c;
          }
        }
      }
    }
    
    //- rjf: write completion of this send into the completion ring buffer
    if(completion_params.out_ring != 0)
    {
      SOCK_SendCompletion completion = {.id = completion_params.id, .good = (ep_socket != -1 && !send_failed)};
      RingGuard g = guarded_ring_open(completion_params.out_ring);
      guarded_ring_write_struct_or_wait(&g, &completion, max_U64);
      guarded_ring_close(&g);
    }
    
    scratch_end(scratch);
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
  lnx_sock_state->u2s_ring = guarded_ring_alloc(arena, KB(256));
  lnx_sock_state->epoll_fd = epoll_create(1);
  lnx_sock_state->listen_thread = thread_launch(lnx_sock_listen_thread_entry_point, 0);
  lnx_sock_state->sender_thread = thread_launch(lnx_sock_sender_thread_entry_point, 0);
}

////////////////////////////////
//~ rjf: @per_os_impl Session Creation/Closing

internal SOCK_Session
sock_session_open(U16 port, SOCK_WakeupFunctionType *wakeup_hook, U64 recv_packet_size)
{
  //- rjf: set up state
  Arena *arena = arena_alloc();
  LNX_SOCK_Session *session = push_array(arena, LNX_SOCK_Session, 1);
  session->arena = arena;
  session->wakeup_hook = wakeup_hook;
  session->recv_packet_size = recv_packet_size;
  session->recv_ring = guarded_ring_alloc(arena, KB(256));
  
  //- rjf: set up connection cache
  session->connection_slots_count = 8;
  session->connection_stripes = stripe_array_alloc(arena);
  session->connection_slots = push_array(arena, LNX_SOCK_ConnectionSlot, session->connection_slots_count);
  
  //- rjf: set up listener
  if(port != 0)
  {
    session->tcp_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    U32 ipv6only = 0;
    setsockopt(session->tcp_listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    bind(session->tcp_listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(session->tcp_listen_socket, SOMAXCONN);
    struct epoll_event evt = {0};
    evt.events = EPOLLIN;
    evt.data.u64 = (U64)session;
    LNX_RETRY_ON_EINTR(epoll_ctl(lnx_sock_state->epoll_fd, EPOLL_CTL_ADD, session->tcp_listen_socket, &evt));
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
//~ rjf: @per_os_impl Sends / Receives

internal B32
sock_send_(SOCK_Session session, SOCK_Endpoint endpoint, SOCK_Protocol protocol, String8 data, U64 endt_us, SOCK_SendCompletionParams *completion_params)
{
  B32 result = 0;
  {
    U64 needed_size = sizeof(session) + sizeof(endpoint) + sizeof(protocol) + sizeof(*completion_params) + sizeof(data.size) + data.size;
    RingGuard g = guarded_ring_open(lnx_sock_state->u2s_ring);
    if(guarded_ring_write_or_wait(&g, needed_size, 0, endt_us))
    {
      result = 1;
      guarded_ring_try_write_struct(&g, &session);
      guarded_ring_try_write_struct(&g, &endpoint);
      guarded_ring_try_write_struct(&g, &protocol);
      guarded_ring_try_write_struct(&g, completion_params);
      guarded_ring_try_write_struct(&g, &data.size);
      guarded_ring_try_write(&g, data.size, data.str);
    }
    guarded_ring_close(&g);
  }
  return result;
}

internal B32
sock_recv(SOCK_Session session_handle, SOCK_Endpoint *endpoint_out, SOCK_Protocol *protocol_out, U8 *data_out, U64 endt_us)
{
  B32 result = 0;
  {
    LNX_SOCK_Session *session = (LNX_SOCK_Session *)session_handle.u64[0];
    RingGuard g = guarded_ring_open(session->recv_ring);
    {
      SOCK_Endpoint endpoint = {0};
      SOCK_Protocol protocol = SOCK_Protocol_TCP;
      U64 recv_size = sizeof(endpoint) + sizeof(protocol) + session->recv_packet_size;
      if(guarded_ring_read_or_wait(&g, recv_size, 0, endt_us))
      {
        result = 1;
        guarded_ring_try_read_struct(&g, &endpoint);
        guarded_ring_try_read_struct(&g, &protocol);
        guarded_ring_try_read(&g, session->recv_packet_size, data_out);
        if(endpoint_out != 0)
        {
          endpoint_out[0] = endpoint;
        }
        if(protocol_out != 0)
        {
          protocol_out[0] = protocol;
        }
      }
    }
    guarded_ring_close(&g);
  }
  return result;
}
