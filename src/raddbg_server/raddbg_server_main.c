// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "The RAD Debugger Server"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "x64/x64_inc.h"
#include "arch/arch_inc.h"
#include "socket/socket_inc.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "pe/pe.h"
#include "elf/elf.h"
#include "gnu/gnu.h"
#include "gnu/gnu_parse.h"
#include "elf/elf_parse.h"
#include "demon/demon_inc.h"
#include "dbg_server/dbg_server.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "x64/x64_inc.c"
#include "arch/arch_inc.c"
#include "socket/socket_inc.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "elf/elf.c"
#include "gnu/gnu.c"
#include "gnu/gnu_parse.c"
#include "elf/elf_parse.c"
#include "demon/demon_inc.c"
#include "dbg_server/dbg_server.c"

////////////////////////////////
//~ rjf: Globals

typedef struct RDS_Msg RDS_Msg;
struct RDS_Msg
{
  RDS_Msg *next;
  SOCK_Endpoint endpoint;
  String8 msg_raw;
};

typedef struct RDS_Client RDS_Client;
struct RDS_Client
{
  RDS_Client *order_next;
  RDS_Client *order_prev;
  RDS_Client *hash_next;
  RDS_Client *hash_prev;
  SOCK_Endpoint endpoint;
};

typedef struct RDS_ClientSlot RDS_ClientSlot;
struct RDS_ClientSlot
{
  RDS_Client *first;
  RDS_Client *last;
};

typedef struct RDS_State RDS_State;
struct RDS_State
{
  Arena *arena;
  SOCK_Session sock_session;
  
  //- rjf: connected clients
  RDS_ClientSlot *client_slots;
  U64 client_slots_count;
  RDS_Client *first_client;
  RDS_Client *last_client;
  RDS_Client *free_client;
  
  //- rjf: client -> demon messages
  RWMutex c2d_rw_mutex;
  CondVar c2d_cv;
  Arena *c2d_arena;
  RDS_Msg *c2d_msg_first;
  RDS_Msg *c2d_msg_last;
  
  //- rjf: demon run state
  B32 demon_is_running;
};

global RDS_State *rds_state = 0;

////////////////////////////////
//~ rjf: Demon Thread

internal void
dmn_thread__entry_point(void *p)
{
  ThreadNameF("dmn_thread");
  DMN_CtrlCtx *ctrl_ctx = dmn_ctrl_begin();
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    ////////////////////////////
    //- rjf: get next messages
    //
    RDS_Msg *msg_first = 0;
    RDS_Msg *msg_last = 0;
    RWMutexScope(rds_state->c2d_rw_mutex, 1) for(;;)
    {
      if(rds_state->c2d_msg_first != 0)
      {
        for EachNode(src, RDS_Msg, rds_state->c2d_msg_first)
        {
          RDS_Msg *dst = push_array(scratch.arena, RDS_Msg, 1);
          SLLQueuePush(msg_first, msg_last, dst);
          dst->endpoint = src->endpoint;
          dst->msg_raw = str8_copy(scratch.arena, src->msg_raw);
        }
        arena_clear(rds_state->c2d_arena);
        rds_state->c2d_msg_first = rds_state->c2d_msg_last = 0;
        break;
      }
      cond_var_wait_rw_w(rds_state->c2d_cv, rds_state->c2d_rw_mutex, max_U64);
    }
    
    ////////////////////////////
    //- rjf: do messages, push responses
    //
    for(RDS_Msg *msg_n = msg_first; msg_n != 0; msg_n = msg_n->next)
    {
      //- rjf: unpack message
      DS_Msg msg = ds_msg_from_packet(scratch.arena, msg_n->msg_raw);
      
      //- rjf: do action, get response
      DS_Response response = {0};
      DMN_EventList events = {0};
      switch(msg.kind)
      {
        default:{}break;
        
        //- rjf: launching
        case DS_MsgKind_Launch:
        {
          ProcessLaunchParams params = {0};
          {
            params.cmd_line           = msg.command_line;
            params.path               = msg.path;
            params.env                = msg.env;
            params.inherit_env        = !!(msg.flags & DS_MsgFlag_InheritEnv);
            params.debug_subprocesses = !!(msg.flags & DS_MsgFlag_DebugSubprocesses);
          }
          response.pid = dmn_ctrl_launch(ctrl_ctx, &params);
        }break;
        
        //- rjf: attaching
        case DS_MsgKind_Attach:
        {
          response.good = dmn_ctrl_attach(ctrl_ctx, msg.pid);
        }break;
        
        //- rjf: killing
        case DS_MsgKind_Kill:
        {
          response.good = dmn_ctrl_kill(ctrl_ctx, msg.entity, (U32)msg.code);
        }break;
        
        //- rjf: detaching
        case DS_MsgKind_Detach:
        {
          response.good = dmn_ctrl_detach(ctrl_ctx, msg.entity);
        }break;
        
        //- rjf: running ops
        case DS_MsgKind_Run:
        case DS_MsgKind_SingleStep:
        {
          DMN_RunCtrls ctrls = {0};
          {
            ctrls.priority_thread            = msg.entity;
            ctrls.single_step_thread         = (msg.kind == DS_MsgKind_SingleStep) ? msg.entity : dmn_handle_zero();
            ctrls.run_entities               = msg.run_entities.handles;
            ctrls.run_entity_count           = msg.run_entities.count;
            ctrls.traps                      = msg.traps;
            ctrls.ignore_previous_exception  = !!(msg.flags & DS_MsgFlag_IgnorePreviousException);
            ctrls.run_entities_are_unfrozen  = !!(msg.flags & DS_MsgFlag_RunEntitiesAreUnfrozen);
            ctrls.run_entities_are_processes = !!(msg.flags & DS_MsgFlag_RunEntitiesAreProcesses);
          };
          response.good = 1;
          events = dmn_ctrl_run(scratch.arena, ctrl_ctx, &ctrls);
        }break;
      }
      
      //- rjf: pack response & send back to the originating endpoint of the message
      {
        String8 data = ds_packet_from_response(scratch.arena, &response);
        sock_send(rds_state->sock_session, SOCK_Protocol_TCP, msg_n->endpoint, data, max_U64);
      }
      
      //- rjf: pack events & send to all connected client 
      if(events.count != 0)
      {
        String8 data = ds_serialized_from_dmn_events(scratch.arena, events);
        for(RDS_Client *c = rds_state->first_client; c != 0; c = c->order_next)
        {
          sock_send(rds_state->sock_session, SOCK_Protocol_TCP, c->endpoint, data, max_U64);
        }
      }
    }
    
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmd_line)
{
  //- rjf: get port argument
  U64 port = 7273;
  {
    U64 port_maybe = 0;
    String8 port_arg_string = cmd_line_string(cmd_line, s("port"));
    if(try_u64_from_str8_c_rules(port_arg_string, &port_maybe))
    {
      port = port_maybe;
    }
  }
  
  //- rjf: set up shared state
  Arena *arena = arena_alloc();
  rds_state = push_array(arena, RDS_State, 1);
  rds_state->arena = arena;
  rds_state->sock_session = sock_session_open((U16)port, 0);
  rds_state->c2d_rw_mutex = rw_mutex_alloc();
  rds_state->c2d_cv = cond_var_alloc();
  rds_state->c2d_arena = arena_alloc();
  
  //- rjf: launch demon thread
  thread_launch(dmn_thread__entry_point, 0);
  
  //- rjf: loop: wait for messages, issue commands to demon
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: get next message from client
    fprintf(stderr, "[waiting for message]\n");
    SOCK_Protocol protocol = SOCK_Protocol_TCP;
    SOCK_Endpoint endpoint = {0};
    String8 data = {0};
    sock_recv(scratch.arena, rds_state->sock_session, &protocol, &endpoint, &data, max_U64);
    
    //- rjf: store this client, if we don't have it
    {
      U64 hash = u64_hash_from_str8(str8_struct(&endpoint));
      U64 slot_idx = hash%rds_state->client_slots_count;
      RDS_Client *client = 0;
      for(RDS_Client *c = rds_state->client_slots[slot_idx].first; c != 0; c = c->hash_next)
      {
        if(MemoryMatchStruct(&c->endpoint, &endpoint))
        {
          client = c;
          break;
        }
      }
      if(client == 0)
      {
        client = rds_state->free_client;
        if(client != 0)
        {
          SLLStackPop_N(rds_state->free_client, order_next);
        }
        else
        {
          client = push_array(rds_state->arena, RDS_Client, 1);
        }
        DLLPushBack_NP(rds_state->client_slots[slot_idx].first, rds_state->client_slots[slot_idx].last, client, hash_next, hash_prev);
        DLLPushBack_NP(rds_state->first_client, rds_state->last_client, client, order_next, order_prev);
        client->endpoint = endpoint;
      }
    }
    
    //- rjf: get packet header
    DS_PacketHeader header = {0};
    if(data.size >= sizeof(header))
    {
      MemoryCopy(&header, data.str, sizeof(header));
    }
    B32 header_is_good = (header.magic == DS_PACKET_MAGIC && header.kind == DS_PacketKind_Msg && header.version == DS_PACKET_VERSION);
    
    //- rjf: log message info
    {
      String8 endpoint_string = sock_string_from_endpoint(scratch.arena, endpoint);
      fprintf(stderr, "[got message]\n");
      fprintf(stderr, "  sender: %.*s\n", str8_varg(endpoint_string));
      fprintf(stderr, "  magic:  0x%x\n", (int)header.magic);
    }
    
    //- rjf: try to handle this message on the main thread, before sending to
    // demon. the demon thread trades runtimes with debuggees, and so it is
    // only able to handle messages when it is not running - e.g. launching,
    // attaching, detaching, and so on. other operations, like reading/writing
    // memory/registers, listing processes, and halting, we want to just do
    // on the main thread without interrupting the demon at all.
    DS_MsgKind peek_kind = DS_MsgKind_Null;
    if(header_is_good)
    {
      String8 msg_raw = str8_skip(data, sizeof(header));
      str8_deserial_read_struct(msg_raw, 2, &peek_kind);
      if(peek_kind < DS_MsgKind_Launch || DS_MsgKind_SingleStep < peek_kind)
      {
        DS_Msg msg = ds_msg_from_packet(scratch.arena, data);
        DS_Response response = {0};
        switch(msg.kind)
        {
          default:{}break;
          case DS_MsgKind_Halt:
          {
            dmn_halt(0, 0);
          }break;
        }
      }
    }
    
    //- rjf: not handled by main thread? -> okay, send to demon thread, if it's ready.
    if(header_is_good && (DS_MsgKind_Launch <= peek_kind && peek_kind <= DS_MsgKind_SingleStep))
    {
      RWMutexScope(rds_state->c2d_rw_mutex, 1)
      {
        String8 msg_raw = str8_skip(data, sizeof(header));
        RDS_Msg *msg = push_array(rds_state->c2d_arena, RDS_Msg, 1);
        msg->endpoint = endpoint;
        msg->msg_raw = str8_copy(rds_state->c2d_arena, msg_raw);
      }
      cond_var_broadcast(rds_state->c2d_cv);
    }
    
    scratch_end(scratch);
  }
}
