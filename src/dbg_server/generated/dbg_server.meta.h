// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef DBG_SERVER_META_H
#define DBG_SERVER_META_H

typedef enum DS_PacketKind
{
DS_PacketKind_Null,
DS_PacketKind_Msg,
DS_PacketKind_Response,
DS_PacketKind_Event,
} DS_PacketKind;

typedef enum DS_MsgKind
{
DS_MsgKind_Null,
DS_MsgKind_Handshake,
DS_MsgKind_Launch,
DS_MsgKind_Attach,
DS_MsgKind_Kill,
DS_MsgKind_Detach,
DS_MsgKind_Run,
DS_MsgKind_SingleStep,
DS_MsgKind_Halt,
DS_MsgKind_MemoryReserve,
DS_MsgKind_MemoryCommit,
DS_MsgKind_MemoryDecommit,
DS_MsgKind_MemoryRelease,
DS_MsgKind_MemoryProtect,
DS_MsgKind_MemoryRead,
DS_MsgKind_MemoryWrite,
DS_MsgKind_ThreadRegBlockRead,
DS_MsgKind_ThreadRegBlockWrite,
DS_MsgKind_ThreadGetModuleTLSVAddr,
DS_MsgKind_ListSystemProcesses,
} DS_MsgKind;

C_LINKAGE_BEGIN
extern String8 ds_name_from_packet_kind_table[4];
extern String8 ds_name_from_msg_kind_table[20];

C_LINKAGE_END

#endif // DBG_SERVER_META_H
