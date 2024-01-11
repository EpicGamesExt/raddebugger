// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal void
w32_socket_set_error(W32_Socket *socket, OS_SocketError error){
  socket->error = error;
  // NOTE(allen): This flag was set earlier so that the socket would assert
  // when an error occurs. The "bug" or issue is whatever caused this error
  // not the fact the flag is set. Unless the flag wasn't supposed to be set!
  Assert(!(socket->flags & W32_SocketFlag_AssertOnError));
}

internal void
w32_socket_set_error_wsa(W32_Socket *socket, int wsa_error){
  switch (wsa_error){
    default:
    {
      socket->wsa_error = wsa_error;
      w32_socket_set_error(socket, OS_SocketError_WSAError);
    }break;
    case WSANOTINITIALISED:
    {
      w32_socket_set_error(socket, OS_SocketError_SocketSystemNotInitialized);
    }break;
  }
}

internal B32
w32_socket_read_looped(W32_Socket *w32_socket, void *buffer, U32 size){
  U32 p = 0;
  CHAR *ptr = (CHAR*)buffer;
  for (;p < size;){
    DWORD amt = (DWORD)(size - p);
    WSABUF wsabuf = {amt, ptr};
    // NOTE(allen): The flags pointer is _NOT_ optional but we can ignore it.
    // We have to zero it because it's an in/out pointer.
    DWORD ignore = 0;
    if (WSARecv(w32_socket->socket, &wsabuf, 1, &amt, &ignore, 0, 0) != 0){
      w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
      break;
    }
    if (amt == 0){
      w32_socket->flags |= W32_SocketFlag_Closed;
      break;
    }
    p += amt;
    ptr += amt;
  }
  B32 result = (p == size);
  return(result);
}

////////////////////////////////
//~ rjf: Per-OS Hook Implementations

internal void
os_socket_init(void){
  WSADATA wsaData;
  WORD vreq = MAKEWORD(2,2);
  WSAStartup(vreq, &wsaData);
}

internal void
os_socket_listen(OS_Socket *s, String8 port){
  W32_Socket *w32_socket = (W32_Socket*)s->memory;
  
  // NOTE(allen): check port string
  char port_buffer[6];
  if (port.size == 0 || port.size >= sizeof(port_buffer)){
    w32_socket_set_error(w32_socket, OS_SocketError_BadPortArgument);
    return;
  }
  MemoryCopy(port_buffer, port.str, port.size);
  port_buffer[port.size] = 0;
  
  // NOTE(allen): listen socket addrinfo
  addrinfo listen_hint = {0};
  listen_hint.ai_flags = AI_PASSIVE|AI_NUMERICSERV;
  listen_hint.ai_family = AF_UNSPEC;
  listen_hint.ai_socktype = SOCK_STREAM;
  listen_hint.ai_protocol = AF_UNSPEC;
  
  addrinfo *addr = {0};
  INT error = getaddrinfo(0, port_buffer, &listen_hint, &addr);
  if (error != 0){
    w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
    return;
  }
  
  // NOTE(allen): init listen socket
  SOCKET socket_listener = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  W32_SocketCloser listener_closer(&socket_listener);
  
  // NOTE(allen): reuseraddr
  {
    union { B32 b; char c[1]; } enable;
    enable.b = true;
    if (setsockopt(socket_listener, SOL_SOCKET, SO_REUSEADDR, enable.c, sizeof(enable)) < 0){
      w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
      return;
    }
  }
  
  // NOTE(allen): bind
  if (bind(socket_listener, addr->ai_addr, (int)addr->ai_addrlen) < 0){
    w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
    return;
  }
  
  // NOTE(allen): listen
  if (listen(socket_listener, 1) < 0){
    w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
    return;
  }
  
  // NOTE(allen): accept
  SOCKET client_socket = accept(socket_listener, 0, 0);
  W32_SocketCloser client_closer(&client_socket);
  
  // NOTE(allen): TCP_NODELAY
  {
    union { B32 b; char c[1]; } enable;
    enable.b = true;
    if (setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, enable.c, sizeof(enable)) < 0){
      w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
      return;
    }
  }
  
  // NOTE(allen): success
  w32_socket->flags |= W32_SocketFlag_Connected;
  w32_socket->socket = client_socket;
  w32_socket->error = OS_SocketError_None;
  client_closer.do_not_close();
}

internal void
os_socket_connect(OS_Socket *s, String8 ip, String8 port){
  W32_Socket *w32_socket = (W32_Socket*)s->memory;
  
  // NOTE(allen): check port string
  char port_buffer[6];
  if (port.size == 0 || port.size >= sizeof(port_buffer)){
    w32_socket_set_error(w32_socket, OS_SocketError_BadPortArgument);
    return;
  }
  MemoryCopy(port_buffer, port.str, port.size);
  port_buffer[port.size] = 0;
  
  // NOTE(allen): check ip string
  if (ip.size == 0){
    ip = str8_lit("localhost");
  }
  char ip_buffer[KB(1)];
  if (ip.size >= sizeof(ip_buffer)){
    w32_socket_set_error(w32_socket, OS_SocketError_BadIPArgument);
    return;
  }
  MemoryCopy(ip_buffer, ip.str, ip.size);
  ip_buffer[ip.size] = 0;
  
  // NOTE(allen): socket addrinfo
  addrinfo hint = {0};
  hint.ai_flags = AI_PASSIVE|AI_NUMERICSERV;
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_protocol = AF_UNSPEC;
  
  addrinfo *addr = {0};
  INT error = getaddrinfo(ip_buffer, port_buffer, &hint, &addr);
  if (error != 0){
    w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
    return;
  }
  
  // NOTE(allen): init socket
  SOCKET socket_server = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  W32_SocketCloser closer(&socket_server);
  
  // NOTE(allen): TCP_NODELAY
  {
    union { B32 b; char c[1]; } enable;
    enable.b = true;
    if (setsockopt(socket_server, IPPROTO_TCP, TCP_NODELAY, enable.c, sizeof(enable)) < 0){
      w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
      return;
    }
  }
  
  // NOTE(allen): connect
  if (connect(socket_server, addr->ai_addr, (int)addr->ai_addrlen) < 0){
    w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
    return;
  }
  
  // NOTE(allen): success
  w32_socket->flags |= W32_SocketFlag_Connected;
  w32_socket->socket = socket_server;
  w32_socket->error = OS_SocketError_None;
  closer.do_not_close();
}

internal void
os_socket_close(OS_Socket *socket){
  W32_Socket *w32_socket = (W32_Socket*)socket->memory;
  closesocket(w32_socket->socket);
  MemoryZeroStruct(w32_socket);
}

internal String8
os_socket_read(Arena *arena, OS_Socket *socket){
  W32_Socket *w32_socket = (W32_Socket*)socket->memory;
  String8 result = {0};
  U32 size = 0;
  if (w32_socket_read_looped(w32_socket, &size, sizeof(size))){
    Temp restore = temp_begin(arena);
    result.str = push_array_no_zero(arena, U8, size);
    if (w32_socket_read_looped(w32_socket, result.str, size)){
      result.size = size;
    }
    else{
      temp_end(restore);
      result.str = 0;
    }
  }
  return(result);
}

internal B32
os_socket_write(OS_Socket *socket, String8List list){
  U32 size = (U32)list.total_size;
  String8Node node = {0};
  str8_list_push_front(&list, &node, str8_struct(&size));
  
  W32_Socket *w32_socket = (W32_Socket*)socket->memory;
  
  WSABUF wsabuf[64];
  Assert(list.node_count <= ArrayCount(wsabuf));
  
  U64 wsabuf_count = 0;
  for (String8Node *node = list.first;
       node != 0;
       node = node->next){
    wsabuf[wsabuf_count].len = (U32)node->string.size;
    wsabuf[wsabuf_count].buf = (CHAR*)node->string.str;
    wsabuf_count += 1;
  }
  
  B32 result = false;
  DWORD amt = 0;
  if (WSASend(w32_socket->socket, wsabuf, wsabuf_count, &amt, 0, 0, 0) != 0){
    w32_socket_set_error_wsa(w32_socket, WSAGetLastError());
  }
  else if (amt == 0){
    w32_socket->flags |= W32_SocketFlag_Connected;
  }
  else{
    result = true;
  }
  
  return(result);
}

internal B32
os_socket_status(OS_Socket *socket, OS_SocketStatus status){
  W32_Socket *w32_socket = (W32_Socket*)socket;
  B32 result = false;
  switch (status){
    case OS_SocketStatus_Uninitialized:
    {
      result = (((w32_socket->flags & (W32_SocketFlag_Connected|W32_SocketFlag_Closed)) == 0) &&
                (w32_socket->error == 0));
    }break;
    
    case OS_SocketStatus_Connected:
    {
      result = (((w32_socket->flags & (W32_SocketFlag_Connected|W32_SocketFlag_Closed)) == W32_SocketFlag_Connected) &&
                (w32_socket->error == 0));
    }break;
    
    case OS_SocketStatus_GracefullyClosed:
    {
      result = (((w32_socket->flags & W32_SocketFlag_Closed) == W32_SocketFlag_Closed) && (w32_socket->error == 0));
    }break;
    
    case OS_SocketStatus_Error:
    {
      result = (w32_socket->error != 0);
    }break;
  }
  return(result);
}

internal String8
os_socket_error_string(Arena *arena, OS_Socket *socket){
  String8 result = str8_lit("no error");
  
  W32_Socket *w32_socket = (W32_Socket*)socket;
  switch (w32_socket->error){
    default:
    {
      result = str8_lit("Bad error code");
    }break;
    
    case OS_SocketError_None:break;
    
    case OS_SocketError_SocketSystemNotInitialized:
    {
      result = str8_lit("Missing call to os_socket_init");
    }break;
    
    case OS_SocketError_BadPortArgument:
    {
      result = str8_lit("Invalid port argument to socket API");
    }break;
    
    case OS_SocketError_BadIPArgument:
    {
      result = str8_lit("Invalid ip argument to socket API");
    }break;
    
    case OS_SocketError_WSAError:
    {
      DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM;
      CHAR *message = 0;
      DWORD size = FormatMessageA(flags, 0, w32_socket->wsa_error, 0, (CHAR*)&message, 0, 0);
      if (size == 0){
        result = str8_lit("Unknown WSA error");
      }
      else{
        String8 string = str8_skip_chop_whitespace(str8((U8*)message, size));
        result = push_str8_copy(arena, string);
        LocalFree(message);
      }
    }break;
  }
  
  return(result);
}

internal void
os_socket_assert_on_error(OS_Socket *socket, B32 assert_on_error){
  W32_Socket *w32_socket = (W32_Socket*)socket;
  if (assert_on_error){
    w32_socket->flags |= W32_SocketFlag_AssertOnError;
  }
  else{
    w32_socket->flags &= ~W32_SocketFlag_AssertOnError;
  }
}
