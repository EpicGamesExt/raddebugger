// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Message Parsing

internal DS_Msg
ds_msg_from_serialized(Arena *arena, String8 data)
{
  //- rjf: unpack message
  U16 magic_maybe = 0;
  str8_deserial_read_struct(data, 0, &magic_maybe);
  
  //- rjf: parse message
  DS_Msg msg = {0};
  if(magic_maybe == DS_MSG_MAGIC)
  {
    U64 off = sizeof(magic_maybe);
    
    // rjf: read flat parts
    off += str8_deserial_read_struct(data, off, &msg.kind);
    off += str8_deserial_read_struct(data, off, &msg.flags);
    off += str8_deserial_read_struct(data, off, &msg.msg_id);
    off += str8_deserial_read_struct(data, off, &msg.vaddr_range);
    off += str8_deserial_read_struct(data, off, &msg.access_flags);
    off += str8_deserial_read_struct(data, off, &msg.entity);
    off += str8_deserial_read_struct(data, off, &msg.pid);
    
    // rjf: read command line strings
    {
      U64 cmd_line_string_count = 0;
      off += str8_deserial_read_struct(data, off, &cmd_line_string_count);
      for EachIndex(idx, cmd_line_string_count)
      {
        String8 string = {0};
        off += str8_deserial_read_struct(data, off, &string.size);
        string = str8_substr(data, r1u64(off, off+string.size));
        str8_list_push(arena, &msg.command_line, string);
      }
    }
    
    // rjf: read path
    {
      String8 string = {0};
      off += str8_deserial_read_struct(data, off, &string.size);
      string = str8_substr(data, r1u64(off, off+string.size));
      msg.path = string;
    }
    
    // rjf: read environment strings
    {
      U64 env_string_count = 0;
      off += str8_deserial_read_struct(data, off, &env_string_count);
      for EachIndex(idx, env_string_count)
      {
        String8 string = {0};
        off += str8_deserial_read_struct(data, off, &string.size);
        string = str8_substr(data, r1u64(off, off+string.size));
        str8_list_push(arena, &msg.env, string);
      }
    }
    
    // rjf: read traps
    {
      U64 trap_count = 0;
      off += str8_deserial_read_struct(data, off, &trap_count);
      for EachIndex(idx, trap_count)
      {
        DMN_Trap trap = {0};
        off += str8_deserial_read_struct(data, off, &trap.process);
        off += str8_deserial_read_struct(data, off, &trap.vaddr);
        off += str8_deserial_read_struct(data, off, &trap.id);
        off += str8_deserial_read_struct(data, off, &trap.flags);
        off += str8_deserial_read_struct(data, off, &trap.size);
        dmn_trap_chunk_list_push(arena, &msg.traps, trap_count, &trap);
      }
    }
  }
  
  return msg;
}

internal String8
ds_serialized_from_msg(Arena *arena, DS_Msg *msg)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List srl = {0};
  {
    // TODO(rjf)
  }
  scratch_end(scratch);
  String8 result = str8_list_join(arena, &srl, 0);
  return result;
}

internal DS_Response
ds_response_from_serialized(Arena *arena, String8 data)
{
  DS_Response result = {0};
  return result;
}

internal String8
ds_serialized_from_response(Arena *arena, DS_Response *response)
{
  String8 result = {0};
  return result;
}
