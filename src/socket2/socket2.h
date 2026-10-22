// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SOCKET2_H
#define SOCKET2_H

////////////////////////////////
//~ rjf: Types

typedef struct SOCK2_SendCompletionParams SOCK2_SendCompletionParams;
struct SOCK2_SendCompletionParams
{
  U64 id;
  GuardedRing *out_ring;
};

typedef struct SOCK2_SendCompletion SOCK2_SendCompletion;
struct SOCK2_SendCompletion
{
  U64 id;
  B32 good;
};

typedef struct SOCK2_Session SOCK2_Session;
struct SOCK2_Session
{
  U64 u64[1];
};

////////////////////////////////
//~ rjf: Send Completion Waiting Helper

internal B32 sock2_wait_for_send_completion(GuardedRing *out_ring, SOCK2_SendCompletion *out_completion, U64 endt_us);

////////////////////////////////
//~ rjf: @per_os_impl Main Layer Initialization

internal void sock2_init(void);

////////////////////////////////
//~ rjf: @per_os_impl Sessions

internal SOCK2_Session sock2_session_open(U16 port, SOCK_WakeupFunctionType *wakeup_hook, U64 recv_packet_size);
internal void sock2_session_close(SOCK2_Session session);

////////////////////////////////
//~ rjf: @per_os_impl Sends / Receives

internal B32 sock2_send_(SOCK2_Session session, SOCK_Endpoint endpoint, SOCK_Protocol protocol, String8 data, U64 endt_us, SOCK2_SendCompletionParams *completion_params);
#define sock2_send(session, endpoint, protocol, data, endt_us, ...) sock2_send_((session), (endpoint), (protocol), (data), (endt_us), &(SOCK2_SendCompletionParams){.id = 0, __VA_ARGS__})
internal B32 sock2_recv(SOCK2_Session session, SOCK_Endpoint *endpoint_out, SOCK_Protocol *protocol_out, U8 *data_out, U64 endt_us);

#endif // SOCKET2_H
