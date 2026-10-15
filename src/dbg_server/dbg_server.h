// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_SERVER_H
#define DBG_SERVER_H

////////////////////////////////
//~ rjf: Packet Headers

#define DS_PACKET_MAGIC 0x7273
#define DS_PACKET_VERSION 0x0

typedef U8 DS_PacketKind;
enum
{
  DS_PacketKind_Msg,
  DS_PacketKind_Response,
  DS_PacketKind_Event,
};

typedef struct DS_PacketHeader DS_PacketHeader;
struct DS_PacketHeader
{
  U16 magic;
  DS_PacketKind kind;
  U8 version;
  U32 payload_size;
};

////////////////////////////////
//~ rjf: Messages (Client -> Server)

typedef enum DS_MsgKind
{
  DS_MsgKind_Null,
  
  //- rjf: connection handshake
  DS_MsgKind_Handshake,
  
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
  DS_MsgFlag_IgnorePreviousException = (1<<0),
  DS_MsgFlag_RunEntitiesAreUnfrozen  = (1<<1),
  DS_MsgFlag_RunEntitiesAreProcesses = (1<<2),
};

typedef struct DS_LaunchParams DS_LaunchParams;
struct DS_LaunchParams
{
  String8List command_line;
  String8 path;
  String8List env;
  String8 stdout_path;
  String8 stdin_path;
  String8 stderr_path;
  B32 debug_subprocesses;
  B32 inherit_env;
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
  
  //- rjf: process launching parameter extension
  DS_LaunchParams *launch_params;
  
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
  DMN_Handle entity;
  U64 pid;
  B32 good;
  String8 data;
};

typedef struct DS_ResponseNode DS_ResponseNode;
struct DS_ResponseNode
{
  DS_ResponseNode *next;
  DS_Response v;
};

////////////////////////////////
//~ rjf: Message/Response Parsing

internal DS_Msg ds_msg_from_packet(Arena *arena, String8 data);
internal String8 ds_packet_from_msg(Arena *arena, DS_Msg *msg);
internal DS_Response ds_response_from_packet(Arena *arena, String8 data);
internal String8 ds_packet_from_response(Arena *arena, DS_Response *response);
internal DMN_EventList ds_dmn_events_from_packet(Arena *arena, String8 data);
internal String8 ds_packet_from_dmn_events(Arena *arena, DMN_EventList evts);

////////////////////////////////
//~ rjf: Local Server Functions

internal U64 ds_ctrl_launch(DMN_CtrlCtx *ctrl_ctx, DS_LaunchParams *params);
internal B32 ds_ctrl_attach(DMN_CtrlCtx *ctrl_ctx, U32 pid);
internal B32 ds_ctrl_kill(DMN_CtrlCtx *ctrl_ctx, DMN_Handle process, U32 exit_code);
internal B32 ds_ctrl_detach(DMN_CtrlCtx *ctrl_ctx, DMN_Handle process);
internal DMN_EventList ds_ctrl_run(Arena *arena, DMN_CtrlCtx *ctrl_ctx, DMN_RunCtrls *ctrls);

#endif // DBG_SERVER_H
