// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal void sock_init(void){}
internal SOCK_Session sock_session_open(U16 port, SOCK_WakeupFunctionType *wakeup_hook, U64 recv_packet_size){SOCK_Session s = {0}; return s;}
internal void sock_session_close(SOCK_Session session){}
internal B32 sock_send_(SOCK_Session session, SOCK_Endpoint endpoint, SOCK_Protocol protocol, String8 data, U64 endt_us, SOCK_SendCompletionParams *completion_params){return 0;}
internal B32 sock_recv(SOCK_Session session, SOCK_Endpoint *endpoint_out, SOCK_Protocol *protocol_out, U8 *data_out, U64 endt_us){return 0;}
