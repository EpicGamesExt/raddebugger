// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_SOCKET_H
#define WIN32_SOCKET_H

////////////////////////////////
//~ rjf: Types

typedef U16 W32_SocketFlags;
enum{
  W32_SocketFlag_Connected = (1 << 0),
  W32_SocketFlag_Closed    = (1 << 1),
  W32_SocketFlag_AssertOnError = (1 << 2),
};

struct W32_Socket{
  W32_SocketFlags flags;
  OS_SocketError error;
  int wsa_error;
  SOCKET socket;
};

struct W32_SocketCloser{
  B32 need_to_close;
  SOCKET *socket;
  W32_SocketCloser(SOCKET *s){
    this->need_to_close = true;
    this->socket = s;
  }
  ~W32_SocketCloser(){
    this->close_now();
  }
  void close_now(){
    if (this->need_to_close){
      closesocket(*this->socket);
      this->need_to_close = false;
    }
  }
  void do_not_close(){
    this->need_to_close = false;
  }
};

StaticAssert(sizeof(Member(OS_Socket, memory)) >= sizeof(W32_Socket), socket_memory_size);

////////////////////////////////
//~ rjf: Helpers

internal void w32_socket_set_error(W32_Socket *socket, OS_SocketError error);
internal void w32_socket_set_error_wsa(W32_Socket *socket, int wsa_error);
internal B32 w32_socket_read_looped(W32_Socket *w32_socket, void *buffer, U32 size);

#endif // WIN32_SOCKET_H
