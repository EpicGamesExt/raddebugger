// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_SERVER_H
#define DBG_SERVER_H

////////////////////////////////
//~ rjf: Messages (Client -> Server)

#define DS_MSG_MAGIC 0x7273

typedef enum DS_MsgKind
{
  DS_MsgKind_Null,
  
  //- rjf: debuggee attaching/detaching ops
  DS_MsgKind_Launch,
  DS_MsgKind_Attach,
  DS_MsgKind_Kill,
  DS_MsgKind_Detach,
  
  //- rjf: running ops
  DS_MsgKind_Run,
  DS_MsgKind_SingleStep,
  
  //- rjf: halting
  DS_MsgKind_Halt,
  
  //- rjf: memory ops
  DS_MsgKind_MemoryReserve,
  DS_MsgKind_MemoryCommit,
  DS_MsgKind_MemoryDecommit,
  DS_MsgKind_MemoryRelease,
  DS_MsgKind_MemoryProtect,
  DS_MsgKind_MemoryRead,
  DS_MsgKind_MemoryWrite,
  
  //- rjf: thread ops
  DS_MsgKind_ThreadRegBlockRead,
  DS_MsgKind_ThreadRegBlockWrite,
  DS_MsgKind_ThreadGetModuleTLSVAddr,
  
  //- rjf: system ops
  DS_MsgKind_ListSystemProcesses,
}
DS_MsgKind;

typedef U32 DS_MsgFlags;
enum
{
  DS_MsgFlag_InheritEnv              = (1<<0),
  DS_MsgFlag_DebugSubprocesses       = (1<<1),
  DS_MsgFlag_IgnorePreviousException = (1<<2),
  DS_MsgFlag_RunEntitiesAreUnfrozen  = (1<<3),
  DS_MsgFlag_RunEntitiesAreProcesses = (1<<4),
};

typedef struct DS_Msg DS_Msg;
struct DS_Msg
{
  DS_MsgKind kind;
  DS_MsgFlags flags;
  U64 msg_id;
  
  //- rjf: general parameters
  Rng1U64 vaddr_range;
  AccessFlags access_flags;
  DMN_Handle entity;
  U64 pid;
  U64 code;
  
  //- rjf: process launching parameters
  String8List command_line;
  String8 path;
  String8List env;
  
  //- rjf: running parameters
  DMN_TrapChunkList traps;
  DMN_HandleArray run_entities;
};

////////////////////////////////
//~ rjf: Responses (Server -> Client responses to messages)

typedef struct DS_Response DS_Response;
struct DS_Response
{
  U64 msg_id;
  String8 data;
  DMN_Handle entity;
  U64 pid;
  B32 good;
  DMN_EventList events;
};

typedef struct DS_ResponseNode DS_ResponseNode;
struct DS_ResponseNode
{
  DS_ResponseNode *next;
  DS_Response v;
};

////////////////////////////////
//~ rjf: Message/Response Parsing

internal DS_Msg ds_msg_from_serialized(Arena *arena, String8 data);
internal String8 ds_serialized_from_msg(Arena *arena, DS_Msg *msg);
internal DS_Response ds_response_from_serialized(Arena *arena, String8 data);
internal String8 ds_serialized_from_response(Arena *arena, DS_Response *response);

#endif // DBG_SERVER_H
