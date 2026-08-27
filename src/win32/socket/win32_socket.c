// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Listener Threads

internal void
w32_sock_tcp_listener_thread_entry_point(void *p)
{
  ThreadNameF("w32_sock_tcp_listener_thread");
  for(;;)
  {
    SOCKET new_socket = accept(w32_sock_state->tcp_listen_socket, 0, 0);
    if(new_socket != INVALID_SOCKET)
    {
      // TODO(rjf)
    }
  }
}

internal void
w32_sock_udp_listener_thread_entry_point(void *p)
{
  ThreadNameF("w32_sock_udp_listener_thread");
  for(;;)
  {
    SOCKET new_socket = accept(w32_sock_state->udp_listen_socket, 0, 0);
    if(new_socket != INVALID_SOCKET)
    {
      // TODO(rjf)
    }
  }
}

////////////////////////////////
//~ rjf: @per_os_impl Top-Level Layer Calls

internal void
sock_init(void)
{
  // NOTE(rjf): winsock2 is already initialized by the base layer for RIO function grabbing.
  
  //- rjf: set up state
  Arena *arena = arena_alloc();
  w32_sock_state = push_array(arena, W32_SOCK_State, 1);
  w32_sock_state->arena = arena;
  w32_sock_state->u2s_ring = guarded_ring_alloc(arena, KB(256));
  w32_sock_state->s2u_ring = guarded_ring_alloc(arena, KB(256));
  
  //- rjf: create listener sockets
  w32_sock_state->tcp_listen_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  w32_sock_state->udp_listen_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  {
    DWORD ipv6only = 0;
    setsockopt(w32_sock_state->tcp_listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));
    setsockopt(w32_sock_state->udp_listen_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&ipv6only, sizeof(ipv6only));
  }
  
  //- rjf: bind listener sockets
  {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET6;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SOCKET_PORT);
    bind(w32_sock_state->tcp_listen_socket, (SOCKADDR *)&server_addr, sizeof(server_addr));
    bind(w32_sock_state->udp_listen_socket, (SOCKADDR *)&server_addr, sizeof(server_addr));
  }
  
  //- rjf: start listening
  {
    listen(w32_sock_state->tcp_listen_socket, SOMAXCONN);
    listen(w32_sock_state->udp_listen_socket, SOMAXCONN);
  }
  
  //- rjf: launch one-off listener threads to block & accept connections
  w32_sock_state->tcp_listener_thread = thread_launch(w32_sock_tcp_listener_thread_entry_point, 0);
  w32_sock_state->udp_listener_thread = thread_launch(w32_sock_udp_listener_thread_entry_point, 0);
}

internal void
sock_async_tick(void)
{
  
}

////////////////////////////////
//~ rjf: @per_os_impl Sends

internal U64
sock_send(U8 *ptr, U64 size, SOCK_Protocol *protocol_in, SOCK_Endpoint *endpoint_in, U64 endt_us)
{
  U64 result = 0;
  U64 needed_size = sizeof(*protocol_in) + sizeof(*endpoint_in) + sizeof(U64) + size;
  RingGuard guard = guarded_ring_open(w32_sock_state->u2s_ring);
  {
    void *dst = guarded_ring_push_or_wait(&guard, needed_size, endt_us);
    if(dst != 0)
    {
      MemoryCopy((U8 *)dst + 0, protocol_in, sizeof(*protocol_in));
      MemoryCopy((U8 *)dst + sizeof(*protocol_in), endpoint_in, sizeof(*endpoint_in));
      MemoryCopy((U8 *)dst + sizeof(*protocol_in) + sizeof(endpoint_in), &size, sizeof(size));
      MemoryCopy((U8 *)dst + sizeof(*protocol_in) + sizeof(endpoint_in) + sizeof(U64), ptr, size);
      result = needed_size;
    }
  }
  guarded_ring_close(&guard);
  return result;
}

////////////////////////////////
//~ rjf: @per_os_impl Receives

internal U64
sock_recv(U8 *ptr, U64 size, SOCK_Protocol *protocol_out, SOCK_Endpoint *endpoint_out, U64 endt_us)
{
  U64 received_size = 0;
  U64 header_size = sizeof(*protocol_out) + sizeof(*endpoint_out) + sizeof(U64);
  {
    RingGuard guard = guarded_ring_open(w32_sock_state->u2s_ring);
    {
      void *src = guarded_ring_pop_or_wait(&guard, header_size, endt_us);
      if(src != 0)
      {
        U64 payload_size = 0;
        MemoryCopy(protocol_out,  (U8 *)src + 0, sizeof(*protocol_out));
        MemoryCopy(endpoint_out,  (U8 *)src + sizeof(*protocol_out), sizeof(*endpoint_out));
        MemoryCopy(&payload_size, (U8 *)src + sizeof(*protocol_out) + sizeof(*endpoint_out), sizeof(U64));
        void *payload = guarded_ring_pop_or_wait(&guard, payload_size, max_U64);
        MemoryCopy(ptr, payload, payload_size);
        received_size = payload_size;
      }
    }
    guarded_ring_close(&guard);
  }
  return received_size;
}
