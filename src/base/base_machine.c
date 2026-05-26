// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal MachineOpResult
machine_read_cstring_opl(Arena *arena, U64 vaddr, U64 vaddr_opl, MachineOp_MemRead *mem_read, void *mem_read_ud, String8 *cstr_out)
{
  MachineOpResult op = MachineOpResult_Fail;

  // TODO: chunk read instead one byte at a time
  U64 cursor = vaddr;
  for(;cursor < vaddr_opl; cursor += 1)
  {
    U8 byte = 0;
    op = mem_read(cursor, &byte, sizeof(byte), mem_read_ud);
    if(op != MachineOpResult_Ok) { break; }
    if(byte == 0)                { break; }
  }

  if(op == MachineOpResult_Ok)
  {
    U64 string_size = cursor - vaddr;
    if(string_size > 0)
    {
      // read string
      U8 *buffer = push_array_no_zero(arena, U8, string_size + 1);
      op = mem_read(vaddr, buffer, string_size, mem_read_ud);
      if(op == MachineOpResult_Ok)
      {
        // null-terminate the string
        buffer[string_size] = 0;
        if(cstr_out)
        {
          *cstr_out = str8(buffer, string_size);
        }
      }
      else
      {
        // rollback buffer push
        arena_pop(arena, string_size + 1);
      }
    }
    else
    {
      if(cstr_out)
      {
        *cstr_out = str8_zero();
      }
    }
  }

  return op;
}

MACHINE_OP_MEM_READ(machine_op_read_string)
{
  String8 *string = ud;
  if(addr <= string->size)
  {
    U64 off = addr;
    if(buffer_size <= string->size - off)
    {
      str8_deserial_read(*string, off, buffer, buffer_size, buffer_size);
      return MachineOpResult_Ok;
    }
  }
  return MachineOpResult_Fail;
}
