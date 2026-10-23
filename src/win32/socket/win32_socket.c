// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Session Thread

internal void
w32_sock_listen_thread_entry_point(void *p)
{
  ThreadNameF("w32_sock_listen_thread");
  for(;;)
  {
    //- rjf: get next completion from IOCP
    DWORD byte_count = 0;
    U64 completion_key = 0;
    OVERLAPPED *overlapped_ptr = 0;
    B32 success = GetQueuedCompletionStatus(w32_sock_state->iocp, &byte_count, &completion_key, &overlapped_ptr, INFINITE);
    
    //- rjf: completion key -> session
    W32_SOCK_Session *session = (W32_SOCK_Session *)completion_key;
    
    //- rjf: overlapped_ptr == accept overlapped? -> new connection
    if(session != 0 && overlapped_ptr == &session->tcp_accept_overlapped)
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
          con->recv_buffer_cap = session->recv_packet_size;
          con->recv_buffer = push_array(stripe->arena, U8, con->recv_buffer_cap);
        }
        con->endpoint = endpoint;
        con->protocol = SOCK_Protocol_TCP;
        con->socket = new_socket;
        con->recv_buffer_pos = 0;
        MemoryZeroStruct(&con->recv_overlapped);
        DLLPushBack(slot->first, slot->last, con);
        buf.len = con->recv_buffer_cap;
        buf.buf = (char *)con->recv_buffer;
        recv_size = &con->recv_size;
        recv_overlapped = &con->recv_overlapped;
      }
      
      // rjf: associate new socket with the global iocp
      CreateIoCompletionPort((HANDLE)new_socket, w32_sock_state->iocp, (U64)session, 0);
      
      // rjf: kick off receive
      DWORD flags = MSG_PUSH_IMMEDIATE;
      WSARecv(new_socket, &buf, 1, recv_size, &flags, recv_overlapped, 0);
      
      // rjf: create new accept socket, zero overlapped, kick off next accept
      session->tcp_accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      MemoryZeroStruct(&session->tcp_accept_overlapped);
      session->lpfnAcceptEx(session->tcp_listen_socket, session->tcp_accept_socket, session->tcp_accept_buffer, 0, sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16, &session->tcp_accept_size_out, &session->tcp_accept_overlapped);
    }
    
    //- rjf: nonzero overlapped pointer -> receive completion on existing connection
    else if(overlapped_ptr != 0)
    {
      // rjf: unpack associated connection
      W32_SOCK_Connection *con = CastFromMember(W32_SOCK_Connection, recv_overlapped, overlapped_ptr);
      SOCK_Endpoint endpoint = con->endpoint;
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%session->connection_slots_count;
      W32_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
      Stripe *stripe = stripe_from_slot_idx(&session->connection_stripes, slot_idx);
      
      // rjf: successful recv
      if(success)
      {
        // rjf: advance the recv position
        con->recv_buffer_pos += byte_count;
        con->recv_buffer_pos = Min(con->recv_buffer_pos, con->recv_buffer_cap); 
        
        // rjf: packet buffer full? -> push to user
        if(con->recv_buffer_pos >= con->recv_buffer_cap)
        {
          RingGuard g = guarded_ring_open(session->recv_ring);
          SOCK_Protocol protocol = SOCK_Protocol_TCP;
          U64 size_to_write = sizeof(endpoint) + sizeof(protocol)  + con->recv_buffer_cap;
          {
            if(guarded_ring_write_or_wait(&g, size_to_write, 0, max_U64))
            {
              guarded_ring_try_write_struct(&g, &endpoint);
              guarded_ring_try_write_struct(&g, &protocol);
              guarded_ring_try_write(&g, con->recv_buffer_cap, con->recv_buffer);
            }
          }
          guarded_ring_close(&g);
        }
        
        // rjf: packet buffer full? -> reset recv buffer
        if(con->recv_buffer_pos >= con->recv_buffer_cap)
        {
          con->recv_buffer_pos = 0;
        }
        
        // rjf: kick off next receive for this socket
        MemoryZeroStruct(&con->recv_overlapped);
        WSABUF buf = {con->recv_buffer_cap - con->recv_buffer_pos, (char *)con->recv_buffer + con->recv_buffer_pos};
        DWORD flags = MSG_PUSH_IMMEDIATE;
        WSARecv(con->socket, &buf, 1, &con->recv_size, &flags, &con->recv_overlapped, 0);
      }
      
      // rjf: no success -> connection closed
      if(!success) RWMutexScope(stripe->rw_mutex, 1)
      {
        closesocket(con->socket);
        DLLRemove(slot->first, slot->last, con);
        con->next = stripe->free;
        stripe->free = con;
      }
    }
    
    //- rjf: call session's wakeup hook
    if(session != 0 && session->wakeup_hook)
    {
      session->wakeup_hook();
    }
  }
}

////////////////////////////////
//~ rjf: Sender Thread

internal void
w32_sock_sender_thread_entry_point(void *p)
{
  ThreadNameF("w32_sock_sender_thread");
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: wait for next send payload
    W32_SOCK_Session *session = 0;
    SOCK_Endpoint endpoint = {0};
    SOCK_Protocol protocol = SOCK_Protocol_TCP;
    SOCK_SendCompletionParams completion_params = {0};
    String8 data = {0};
    {
      RingGuard g = guarded_ring_open(w32_sock_state->u2s_ring);
      if(guarded_ring_read_struct_or_wait(&g, &session, max_U64))
      {
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
    W32_SOCK_ConnectionSlot *slot = &session->connection_slots[slot_idx];
    
    //- rjf: endpoint -> socket
    SOCKET ep_socket = -1;
    RWMutexScope(stripe->rw_mutex, 0)
    {
      for(W32_SOCK_Connection *c = slot->first; c != 0; c = c->next)
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
      for(W32_SOCK_Connection *c = slot->first; c != 0; c = c->next)
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
        SOCKET new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        connect(new_socket, (struct sockaddr *)&endpoint_sockaddr, endpoint_sockaddr_size);
        
        // rjf: allocate & store in cache
        W32_SOCK_Connection *con = (W32_SOCK_Connection *)stripe->free;
        if(con != 0)
        {
          stripe->free = con->next;
        }
        else
        {
          con = push_array(stripe->arena, W32_SOCK_Connection, 1);
          con->recv_buffer_cap = session->recv_packet_size;
          con->recv_buffer = push_array(stripe->arena, U8, con->recv_buffer_cap);
        }
        con->endpoint = endpoint;
        con->protocol = protocol;
        con->socket = new_socket;
        con->recv_buffer_pos = 0;
        MemoryZeroStruct(&con->recv_overlapped);
        DLLPushBack(slot->first, slot->last, con);
        
        // rjf: associate this socket with iocp
        CreateIoCompletionPort((HANDLE)new_socket, w32_sock_state->iocp, (U64)session, 0);
        
        // rjf: kick off receive on this socket
        WSABUF buf = {con->recv_buffer_cap, (char *)con->recv_buffer};
        DWORD flags = MSG_PUSH_IMMEDIATE;
        WSARecv(con->socket, &buf, 1, &con->recv_size, &flags, &con->recv_overlapped, 0);
        
        // rjf: grab socket for sending
        ep_socket = con->socket;
      }
    }
    
    //- rjf: got socket -> send
    B32 send_failed = 0;
    B32 bad_connection = 0;
    if(ep_socket != -1 && send(ep_socket, (char *)data.str, data.size, 0) == SOCKET_ERROR)
    {
      send_failed = 1;
      int error = WSAGetLastError();
      if(error == WSAECONNRESET)
      {
        bad_connection = 1;
      }
    }
    
    //- rjf: got socket, but detected bad connection? -> connection closed
    if(ep_socket != -1 && bad_connection)
    {
      RWMutexScope(stripe->rw_mutex, 1)
      {
        for(W32_SOCK_Connection *c = slot->first; c != 0; c = c->next)
        {
          if(sock_endpoint_match(c->endpoint, endpoint) && c->protocol == protocol)
          {
            closesocket(c->socket);
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
//~ rjf: @per_os_impl Main Layer Initialization

internal void
sock_init(void)
{
  Arena *arena = arena_alloc();
  w32_sock_state = push_array(arena, W32_SOCK_State, 1);
  w32_sock_state->arena = arena;
  w32_sock_state->u2s_ring = guarded_ring_alloc(arena, KB(256));
  w32_sock_state->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
  w32_sock_state->listen_thread = thread_launch(w32_sock_listen_thread_entry_point, 0);
  w32_sock_state->sender_thread = thread_launch(w32_sock_sender_thread_entry_point, 0);
}

////////////////////////////////
//~ rjf: @per_os_impl Sessions

internal SOCK_Session
sock_session_open(U16 port, SOCK_WakeupFunctionType *wakeup_hook, U64 recv_packet_size)
{
  //- rjf: alloc & fill basics
  Arena *arena = arena_alloc();
  W32_SOCK_Session *session = push_array(arena, W32_SOCK_Session, 1);
  session->arena = arena;
  session->wakeup_hook = wakeup_hook;
  session->recv_packet_size = recv_packet_size;
  session->recv_ring = guarded_ring_alloc(arena, KB(256));
  
  //- rjf: set up session cache
  session->connection_stripes = stripe_array_alloc(arena);
  session->connection_slots_count = 64;
  session->connection_slots = push_array(arena, W32_SOCK_ConnectionSlot, session->connection_slots_count);
  
  //- rjf: create session socket, bind to address, kick off listening & attach to session iocp
  if(port != 0)
  {
    session->tcp_listen_socket = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    DWORD ipv6only = 0;
    setsockopt(session->tcp_listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    bind(session->tcp_listen_socket, (SOCKADDR *)&server_addr, sizeof(server_addr));
    listen(session->tcp_listen_socket, SOMAXCONN);
    CreateIoCompletionPort((HANDLE)session->tcp_listen_socket, w32_sock_state->iocp, (U64)session, 0);
  }
  
  //- rjf: create initial accept socket, attach to global connection iocp
  if(port != 0)
  {
    session->tcp_accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  }
  
  //- rjf: load AcceptEx function
  if(port != 0)
  {
    DWORD dwBytes = 0;
    GUID AcceptEx_guid = WSAID_ACCEPTEX;
    WSAIoctl(session->tcp_listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &AcceptEx_guid, sizeof(AcceptEx_guid), &session->lpfnAcceptEx, sizeof(session->lpfnAcceptEx), &dwBytes, 0, 0);
  }
  
  //- rjf: kick off accept
  if(port != 0)
  {
    session->lpfnAcceptEx(session->tcp_listen_socket, session->tcp_accept_socket, session->tcp_accept_buffer, 0, sizeof(struct sockaddr_storage) + 16, sizeof(struct sockaddr_storage) + 16, &session->tcp_accept_size_out, &session->tcp_accept_overlapped);
  }
  
  //- rjf: session -> handle
  SOCK_Session result = {(U64)session};
  return result;
}

internal void
sock_session_close(SOCK_Session session)
{
  // TODO(rjf)
}

////////////////////////////////
//~ rjf: @per_os_impl Client Send / Receives

internal B32
sock_send_(SOCK_Session session, SOCK_Endpoint endpoint, SOCK_Protocol protocol, String8 data, U64 endt_us, SOCK_SendCompletionParams *completion_params)
{
  B32 result = 0;
  {
    U64 needed_size = sizeof(session) + sizeof(endpoint) + sizeof(protocol) + sizeof(*completion_params) + sizeof(data.size) + data.size;
    RingGuard g = guarded_ring_open(w32_sock_state->u2s_ring);
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
    W32_SOCK_Session *session = (W32_SOCK_Session *)session_handle.u64[0];
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
