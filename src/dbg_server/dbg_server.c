// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Message Parsing

internal DS_Msg
ds_msg_from_packet(Arena *arena, String8 data)
{
  //- rjf: read packet header
  DS_PacketHeader header = {0};
  {
    str8_deserial_read_struct(data, 0, &header);
  }
  
  //- rjf: get message payload part
  String8 payload = str8_substr(data, r1u64(sizeof(header), sizeof(header)+header.payload_size));
  
  //- rjf: read message
  DS_Msg msg = {0};
  if(header.magic == DS_PACKET_MAGIC && header.kind == DS_PacketKind_Msg && header.version == DS_PACKET_VERSION)
  {
    U64 off = 0;
    
    // rjf: read flat parts
    off += str8_deserial_read_struct(payload, off, &msg.kind);
    off += str8_deserial_read_struct(payload, off, &msg.flags);
    off += str8_deserial_read_struct(payload, off, &msg.msg_id);
    off += str8_deserial_read_struct(payload, off, &msg.vaddr_range);
    off += str8_deserial_read_struct(payload, off, &msg.access_flags);
    off += str8_deserial_read_struct(payload, off, &msg.entity);
    off += str8_deserial_read_struct(payload, off, &msg.pid);
    
    // rjf: read launch parameters extension
    {
      B8 has_launch_params = 0;
      off += str8_deserial_read_struct(payload, off, &has_launch_params);
      if(has_launch_params)
      {
        msg.launch_params = push_array(arena, DS_LaunchParams, 1);
        
        // rjf: read command line strings
        {
          U64 cmd_line_string_count = 0;
          off += str8_deserial_read_struct(payload, off, &cmd_line_string_count);
          for EachIndex(idx, cmd_line_string_count)
          {
            String8 string = {0};
            off += str8_deserial_read_struct(payload, off, &string.size);
            string = str8_substr(payload, r1u64(off, off+string.size));
            off += string.size;
            str8_list_push(arena, &msg.launch_params->command_line, string);
          }
        }
        
        // rjf: read path
        {
          off += str8_deserial_read_struct(payload, off, &msg.launch_params->path.size);
          msg.launch_params->path = str8_substr(payload, r1u64(off, off+msg.launch_params->path.size));
          off += msg.launch_params->path.size;
        }
        
        // rjf: read environment strings
        {
          U64 env_string_count = 0;
          off += str8_deserial_read_struct(payload, off, &env_string_count);
          for EachIndex(idx, env_string_count)
          {
            String8 string = {0};
            off += str8_deserial_read_struct(payload, off, &string.size);
            string = str8_substr(payload, r1u64(off, off+string.size));
            off += string.size;
            str8_list_push(arena, &msg.launch_params->env, string);
          }
        }
        
        // rjf: read std i/o paths
        {
          off += str8_deserial_read_struct(payload, off, &msg.launch_params->stdout_path.size);
          msg.launch_params->stdout_path = str8_substr(payload, r1u64(off, off+msg.launch_params->stdout_path.size));
          off += msg.launch_params->stdout_path.size;
          off += str8_deserial_read_struct(payload, off, &msg.launch_params->stdin_path.size);
          msg.launch_params->stdin_path = str8_substr(payload, r1u64(off, off+msg.launch_params->stdin_path.size));
          off += msg.launch_params->stdin_path.size;
          off += str8_deserial_read_struct(payload, off, &msg.launch_params->stderr_path.size);
          msg.launch_params->stderr_path = str8_substr(payload, r1u64(off, off+msg.launch_params->stderr_path.size));
          off += msg.launch_params->stderr_path.size;
        }
        
        // rjf: read bits
        off += str8_deserial_read_struct(payload, off, &msg.launch_params->debug_subprocesses);
        off += str8_deserial_read_struct(payload, off, &msg.launch_params->inherit_env);
      }
    }
    
    // rjf: read traps
    {
      U64 trap_count = 0;
      off += str8_deserial_read_struct(payload, off, &trap_count);
      for EachIndex(idx, trap_count)
      {
        DMN_Trap trap = {0};
        off += str8_deserial_read_struct(payload, off, &trap.process);
        off += str8_deserial_read_struct(payload, off, &trap.vaddr);
        off += str8_deserial_read_struct(payload, off, &trap.id);
        off += str8_deserial_read_struct(payload, off, &trap.flags);
        off += str8_deserial_read_struct(payload, off, &trap.size);
        dmn_trap_chunk_list_push(arena, &msg.traps, trap_count, &trap);
      }
    }
  }
  return msg;
}

internal String8
ds_packet_from_msg(Arena *arena, DS_Msg *msg)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);
  {
    // NOTE(rjf): (header wrote to beginning of `srl` at end of path)
    
    // rjf: write flat parts
    str8_serial_push_struct(scratch.arena, &srl, &msg->kind);
    str8_serial_push_struct(scratch.arena, &srl, &msg->flags);
    str8_serial_push_struct(scratch.arena, &srl, &msg->msg_id);
    str8_serial_push_struct(scratch.arena, &srl, &msg->vaddr_range);
    str8_serial_push_struct(scratch.arena, &srl, &msg->access_flags);
    str8_serial_push_struct(scratch.arena, &srl, &msg->entity);
    str8_serial_push_struct(scratch.arena, &srl, &msg->pid);
    
    // rjf: write launch parameters extension
    B8 has_launch_params = (msg->launch_params != 0);
    str8_serial_push_struct(scratch.arena, &srl, &has_launch_params);
    if(has_launch_params)
    {
      // rjf: write command line strings
      {
        str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->command_line.node_count);
        for EachNode(n, String8Node, msg->launch_params->command_line.first)
        {
          str8_serial_push_struct(scratch.arena, &srl, &n->string.size);
          str8_serial_push_string(scratch.arena, &srl, n->string);
        }
      }
      
      // rjf: write path
      {
        str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->path.size);
        str8_serial_push_string(scratch.arena, &srl, msg->launch_params->path);
      }
      
      // rjf: write environment strings
      {
        str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->env.node_count);
        for EachNode(n, String8Node, msg->launch_params->env.first)
        {
          str8_serial_push_struct(scratch.arena, &srl, &n->string.size);
          str8_serial_push_string(scratch.arena, &srl, n->string);
        }
      }
      
      // rjf: write std i/o paths
      {
        str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->stdout_path.size);
        str8_serial_push_string(scratch.arena, &srl, msg->launch_params->stdout_path);
        str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->stdin_path.size);
        str8_serial_push_string(scratch.arena, &srl, msg->launch_params->stdin_path);
        str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->stderr_path.size);
        str8_serial_push_string(scratch.arena, &srl, msg->launch_params->stderr_path);
      }
      
      // rjf: write bits
      str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->debug_subprocesses);
      str8_serial_push_struct(scratch.arena, &srl, &msg->launch_params->inherit_env);
    }
    
    // rjf: write traps
    {
      str8_serial_push_struct(scratch.arena, &srl, &msg->traps.trap_count);
      for EachNode(n, DMN_TrapChunkNode, msg->traps.first)
      {
        for EachIndex(idx, n->count)
        {
          str8_serial_push_struct(scratch.arena, &srl, &n->v[idx].process);
          str8_serial_push_struct(scratch.arena, &srl, &n->v[idx].vaddr);
          str8_serial_push_struct(scratch.arena, &srl, &n->v[idx].id);
          str8_serial_push_struct(scratch.arena, &srl, &n->v[idx].flags);
          str8_serial_push_struct(scratch.arena, &srl, &n->v[idx].size);
        }
      }
    }
    
    // rjf: push packet header (to front)
    DS_PacketHeader header =
    {
      .magic        = DS_PACKET_MAGIC,
      .kind         = DS_PacketKind_Msg,
      .version      = DS_PACKET_VERSION,
      .payload_size = (U32)srl.total_size,
    };
    str8_list_push_front(scratch.arena, &srl, str8_copy(scratch.arena, str8_struct(&header)));
  }
  String8 result = str8_serial_end(arena, &srl);
  scratch_end(scratch);
  return result;
}

internal DS_Response
ds_response_from_packet(Arena *arena, String8 data)
{
  //- rjf: read packet header
  DS_PacketHeader header = {0};
  {
    str8_deserial_read_struct(data, 0, &header);
  }
  
  //- rjf: get message payload part
  String8 payload = str8_substr(data, r1u64(sizeof(header), sizeof(header)+header.payload_size));
  
  //- rjf: read response
  DS_Response result = {0};
  if(header.magic == DS_PACKET_MAGIC && header.kind == DS_PacketKind_Response && header.version == DS_PACKET_VERSION)
  {
    U64 off = 0;
    
    // rjf: read flat parts
    off += str8_deserial_read_struct(payload, off, &result.msg_id);
    off += str8_deserial_read_struct(payload, off, &result.entity);
    off += str8_deserial_read_struct(payload, off, &result.pid);
    off += str8_deserial_read_struct(payload, off, &result.good);
    
    // rjf: read data payload
    off += str8_deserial_read_struct(payload, off, &result.data.size);
    result.data.str = push_array(arena, U8, result.data.size);
    off += str8_deserial_read(payload, off, result.data.str, result.data.size, 1);
  }
  return result;
}

internal String8
ds_packet_from_response(Arena *arena, DS_Response *response)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);
  {
    // NOTE(rjf): (header wrote to beginning of `srl` at end of path)
    
    // rjf: write flat parts
    str8_serial_push_struct(scratch.arena, &srl, &response->msg_id);
    str8_serial_push_struct(scratch.arena, &srl, &response->entity);
    str8_serial_push_struct(scratch.arena, &srl, &response->pid);
    str8_serial_push_struct(scratch.arena, &srl, &response->good);
    
    // rjf: write data payload
    str8_serial_push_struct(scratch.arena, &srl, &response->data.size);
    str8_serial_push_string(scratch.arena, &srl, response->data);
    
    // rjf: push packet header (to front)
    DS_PacketHeader header =
    {
      .magic        = DS_PACKET_MAGIC,
      .kind         = DS_PacketKind_Response,
      .version      = DS_PACKET_VERSION,
      .payload_size = (U32)srl.total_size,
    };
    str8_list_push_front(scratch.arena, &srl, str8_copy(scratch.arena, str8_struct(&header)));
  }
  String8 result = str8_serial_end(arena, &srl);
  scratch_end(scratch);
  return result;
}

internal DMN_EventList
ds_dmn_events_from_packet(Arena *arena, String8 data)
{
  //- rjf: read packet header
  DS_PacketHeader header = {0};
  {
    str8_deserial_read_struct(data, 0, &header);
  }
  
  //- rjf: get message payload part
  String8 payload = str8_substr(data, r1u64(sizeof(header), sizeof(header)+header.payload_size));
  
  //- rjf: read events
  DMN_EventList evts = {0};
  if(header.magic == DS_PACKET_MAGIC && header.kind == DS_PacketKind_Event && header.version == DS_PACKET_VERSION)
  {
    U64 off = 0;
    U64 demon_event_count = 0;
    off += str8_deserial_read_struct(payload, off, &demon_event_count);
    for EachIndex(idx, demon_event_count)
    {
      DMN_Event *evt = dmn_event_list_push(arena, &evts);
      
      // rjf: read flat parts
      off += str8_deserial_read_struct(payload, off, &evt->kind);
      off += str8_deserial_read_struct(payload, off, &evt->error_kind);
      off += str8_deserial_read_struct(payload, off, &evt->memory_kind);
      off += str8_deserial_read_struct(payload, off, &evt->exception_kind);
      off += str8_deserial_read_struct(payload, off, &evt->process);
      off += str8_deserial_read_struct(payload, off, &evt->thread);
      off += str8_deserial_read_struct(payload, off, &evt->module);
      off += str8_deserial_read_struct(payload, off, &evt->arch);
      off += str8_deserial_read_struct(payload, off, &evt->address);
      off += str8_deserial_read_struct(payload, off, &evt->size);
      off += str8_deserial_read_struct(payload, off, &evt->code);
      off += str8_deserial_read_struct(payload, off, &evt->flags);
      off += str8_deserial_read_struct(payload, off, &evt->instruction_pointer);
      off += str8_deserial_read_struct(payload, off, &evt->stack_pointer);
      off += str8_deserial_read_struct(payload, off, &evt->user_data);
      off += str8_deserial_read_struct(payload, off, &evt->exception_repeated);
      off += str8_deserial_read_struct(payload, off, &evt->tls_root_vaddr);
      
      // rjf: read string
      off += str8_deserial_read_struct(payload, off, &evt->string.size);
      evt->string.str = push_array(arena, U8, evt->string.size);
      off += str8_deserial_read(payload, off, evt->string.str, evt->string.size, 1);
      
      // rjf: read module info
      B8 has_module_info = 0;
      off += str8_deserial_read_struct(payload, off, &has_module_info);
      if(!has_module_info)
      {
        evt->module_info = &dmn_module_info_nil;
      }
      else
      {
        evt->module_info = push_array(arena, DMN_ModuleInfo, 1);
        
        // rjf: read flat parts
        off += str8_deserial_read_struct(payload, off, &evt->module_info->arch);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->vsize);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->entry_point_voff);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->debug_info_guid);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->debug_info_timestamp);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->debug_info_age);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->tls_index);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->tls_offset);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->pe_intel_pdatas_vaddr_range);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->eh_frame_header_vaddr_range);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->raddbg_info_voff_range);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->raddbg_is_attached_marker_voff);
        
        // rjf: read paths
        off += str8_deserial_read_struct(payload, off, &evt->module_info->module_path.size);
        evt->module_info->module_path.str = push_array(arena, U8, evt->module_info->module_path.size);
        off += str8_deserial_read(payload, off, evt->module_info->module_path.str, evt->module_info->module_path.size, 1);
        off += str8_deserial_read_struct(payload, off, &evt->module_info->debug_info_path.size);
        evt->module_info->debug_info_path.str = push_array(arena, U8, evt->module_info->debug_info_path.size);
        off += str8_deserial_read(payload, off, evt->module_info->debug_info_path.str, evt->module_info->debug_info_path.size, 1);
      }
    }
  }
  return evts;
}

internal String8
ds_packet_from_dmn_events(Arena *arena, DMN_EventList evts)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  str8_serial_begin(scratch.arena, &srl);
  {
    // NOTE(rjf): (header wrote to beginning of `srl` at end of path)
    
    // rjf: write events
    str8_serial_push_struct(scratch.arena, &srl, &evts.count);
    for EachNode(n, DMN_EventNode, evts.first)
    {
      // rjf: write flat parts
      str8_serial_push_struct(scratch.arena, &srl, &n->v.kind);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.error_kind);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.memory_kind);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.exception_kind);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.process);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.thread);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.module);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.arch);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.address);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.size);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.code);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.flags);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.instruction_pointer);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.stack_pointer);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.user_data);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.exception_repeated);
      str8_serial_push_struct(scratch.arena, &srl, &n->v.tls_root_vaddr);
      
      // rjf: write string
      str8_serial_push_struct(scratch.arena, &srl, &n->v.string.size);
      str8_serial_push_string(scratch.arena, &srl, n->v.string);
      
      // rjf: write module info
      B8 has_module_info = (n->v.module_info != 0 && n->v.module_info != &dmn_module_info_nil);
      str8_serial_push_struct(scratch.arena, &srl, &has_module_info);
      if(has_module_info)
      {
        // rjf: write flat parts
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->arch);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->vsize);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->entry_point_voff);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->debug_info_guid);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->debug_info_timestamp);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->debug_info_age);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->tls_index);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->tls_offset);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->pe_intel_pdatas_vaddr_range);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->eh_frame_header_vaddr_range);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->raddbg_info_voff_range);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->raddbg_is_attached_marker_voff);
        
        // rjf: write paths
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->module_path.size);
        str8_serial_push_string(scratch.arena, &srl, n->v.module_info->module_path);
        str8_serial_push_struct(scratch.arena, &srl, &n->v.module_info->debug_info_path.size);
        str8_serial_push_string(scratch.arena, &srl, n->v.module_info->debug_info_path);
      }
    }
    
    // rjf: push packet header (to front)
    DS_PacketHeader header =
    {
      .magic        = DS_PACKET_MAGIC,
      .kind         = DS_PacketKind_Event,
      .version      = DS_PACKET_VERSION,
      .payload_size = (U32)srl.total_size,
    };
    str8_list_push_front(scratch.arena, &srl, str8_copy(scratch.arena, str8_struct(&header)));
  }
  String8 result = str8_serial_end(arena, &srl);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Local Server Functions

internal U64
ds_ctrl_launch(DMN_CtrlCtx *ctrl_ctx, DS_LaunchParams *params)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: produce full stdout/stderr/stdin paths
  String8 stdout_path = path_absolute_dst_from_relative_dst_src(scratch.arena, params->stdout_path, params->path);
  String8 stdin_path  = path_absolute_dst_from_relative_dst_src(scratch.arena, params->stdin_path, params->path);
  String8 stderr_path = path_absolute_dst_from_relative_dst_src(scratch.arena, params->stderr_path, params->path);
  
  //- rjf: obtain stdout/stderr/stdin handles
  File stdout_handle = {0};
  File stderr_handle = {0};
  File stdin_handle  = {0};
  if(stdout_path.size != 0)
  {
    File f = file_open(AccessFlag_Write|AccessFlag_Read, stdout_path);
    file_close(f);
    stdout_handle = file_open(AccessFlag_Write|AccessFlag_Append|AccessFlag_ShareRead|AccessFlag_ShareWrite|AccessFlag_Inherited, params->stdout_path);
  }
  if(stderr_path.size != 0)
  {
    File f = file_open(AccessFlag_Write|AccessFlag_Read, stderr_path);
    file_close(f);
    stderr_handle = file_open(AccessFlag_Write|AccessFlag_Append|AccessFlag_ShareRead|AccessFlag_ShareWrite|AccessFlag_Inherited, params->stderr_path);
  }
  if(stdin_path.size != 0)
  {
    stdin_handle = file_open(AccessFlag_Read|AccessFlag_ShareRead|AccessFlag_ShareWrite|AccessFlag_Inherited, stdin_path);
  }
  
  //- rjf: launch
  ProcessLaunchParams dmn_launch_params = {0};
  {
    dmn_launch_params.cmd_line           = params->command_line;
    dmn_launch_params.path               = params->path;
    dmn_launch_params.env                = params->env;
    dmn_launch_params.inherit_env        = params->inherit_env;
    dmn_launch_params.debug_subprocesses = params->debug_subprocesses;
    dmn_launch_params.stdout_file        = stdout_handle;
    dmn_launch_params.stderr_file        = stderr_handle;
    dmn_launch_params.stdin_file         = stdin_handle;
  }
  U64 pid = dmn_ctrl_launch(ctrl_ctx, &dmn_launch_params);
  
  //- rjf: close stdout/stderr/stdin files
  file_close(stdout_handle);
  file_close(stderr_handle);
  file_close(stdin_handle);
  
  scratch_end(scratch);
  return pid;
}

internal B32
ds_ctrl_attach(DMN_CtrlCtx *ctrl_ctx, U32 pid)
{
  B32 result = dmn_ctrl_attach(ctrl_ctx, pid);
  return result;
}

internal B32
ds_ctrl_kill(DMN_CtrlCtx *ctrl_ctx, DMN_Handle process, U32 exit_code)
{
  B32 result = dmn_ctrl_kill(ctrl_ctx, process, exit_code);
  return result;
}

internal B32
ds_ctrl_detach(DMN_CtrlCtx *ctrl_ctx, DMN_Handle process)
{
  B32 result = dmn_ctrl_detach(ctrl_ctx, process);
  return result;
}

internal DMN_EventList
ds_ctrl_run(Arena *arena, DMN_CtrlCtx *ctrl_ctx, DMN_RunCtrls *ctrls)
{
  DMN_EventList events = dmn_ctrl_run(arena, ctrl_ctx, ctrls);
  return events;
}
