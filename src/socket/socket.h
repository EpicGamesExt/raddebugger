// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SOCKET_H
#define SOCKET_H

////////////////////////////////
//~ rjf: Socket Types

typedef enum SOCK_Protocol
{
  SOCK_Protocol_TCP,
  SOCK_Protocol_UDP,
}
SOCK_Protocol;

typedef U8 SOCK_EndpointKind;
typedef enum SOCK_EndpointKindEnum
{
  SOCK_EndpointKind_IPv4,
  SOCK_EndpointKind_IPv6,
}
SOCK_EndpointKindEnum;

typedef struct SOCK_Endpoint SOCK_Endpoint;
struct SOCK_Endpoint
{
  U16 port;
  SOCK_EndpointKind kind;
  U8 _pad_0;
  U32 _pad_1;
  union
  {
    U8 address_u8[16];
    U16 address_u16[8];
    U32 address_u32[4];
    U64 address_u64[2];
    U128 address_u128[1];
  };
};

////////////////////////////////
//~ rjf: Session Handle

typedef struct SOCK_Session SOCK_Session;
struct SOCK_Session
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Wakeup Hook Function Types

#define SOCK_WAKEUP_FUNCTION_DEF(name) void name(void)
typedef SOCK_WAKEUP_FUNCTION_DEF(SOCK_WakeupFunctionType);

////////////////////////////////
//~ rjf: Helpers

internal SOCK_Endpoint sock_endpoint_from_string_port(String8 address, U16 port);
internal SOCK_Endpoint sock_endpoint_from_string(String8 address_and_port);
internal String8 sock_string_from_endpoint(Arena *arena, SOCK_Endpoint endpoint);
internal B32 sock_endpoint_match(SOCK_Endpoint a, SOCK_Endpoint b);

////////////////////////////////
//~ rjf: @per_os_impl Top-Level Layer Calls

#if !defined(NEED_ASYNC)
# define NEED_ASYNC 1
#endif
internal void sock_init(void);
internal void sock_async_tick(void);

////////////////////////////////
//~ rjf: @per_os_impl Session Creation/Closing

internal SOCK_Session sock_session_open(U16 listener_port, SOCK_WakeupFunctionType *wakeup_hook);
internal void sock_session_close(SOCK_Session session);

////////////////////////////////
//~ rjf: @per_os_impl Sends

internal B32 sock_send(SOCK_Session session, SOCK_Protocol protocol, SOCK_Endpoint endpoint, String8 data, U64 endt_us);

////////////////////////////////
//~ rjf: @per_os_impl Receives

internal B32 sock_recv(Arena *arena, SOCK_Session session, SOCK_Protocol *protocol_out, SOCK_Endpoint *endpoint_out, String8 *data_out, U64 endt_us);

#endif // SOCKET_H
