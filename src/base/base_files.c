// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Handle Type Functions

internal File
file_zero(void)
{
  File f = {0};
  return f;
}

internal B32
file_match(File a, File b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

////////////////////////////////
//~ rjf: Filesystem Helpers (Helpers, Implemented Once)

internal String8
data_from_file_path(Arena *arena, String8 path)
{
  File file = file_open(AccessFlag_Read|AccessFlag_ShareRead, path);
  FileProperties props = properties_from_file(file);
  String8 data = string_from_file_range(arena, file, r1u64(0, props.size));
  file_close(file);
  return data;
}

internal B32
write_data_to_file_path(String8 path, String8 data)
{
  B32 good = 0;
  File file = file_open(AccessFlag_Write, path);
  if(!file_match(file, file_zero()))
  {
    U64 bytes_written = file_write(file, r1u64(0, data.size), data.str);
    good = (bytes_written == data.size);
    file_close(file);
  }
  return good;
}

internal B32
write_data_list_to_file_path(String8 path, String8List list)
{
  B32 good = 0;
  File file = file_open(AccessFlag_Write, path);
  if(!file_match(file, file_zero()))
  {
    Temp scratch = scratch_begin(0, 0);
    U64 write_buffer_size = KB(64);
    U8 *write_buffer = push_array_no_zero(scratch.arena, U8, write_buffer_size);
    U64 write_buffer_write_pos = 0;
    U64 write_buffer_read_pos = 0;
    U64 file_off = 0;
    {
      for(String8Node *n = list.first; n != 0; n = n->next)
      {
        for(U64 n_off = 0; n_off < n->string.size;)
        {
          U64 write_buffer_unconsumed_size = (write_buffer_write_pos - write_buffer_read_pos);
          U64 write_buffer_available_size = (write_buffer_size - write_buffer_unconsumed_size);
          if(write_buffer_available_size == 0)
          {
            U64 file_write_size = file_write(file, r1u64(file_off, file_off+write_buffer_size), write_buffer);
            if(file_write_size != write_buffer_size)
            {
              goto dbl_break;
            }
            file_off += write_buffer_size;
            write_buffer_read_pos += write_buffer_size;
          }
          else
          {
            U64 bytes_to_copy = Min(write_buffer_available_size, n->string.size - n_off);
            write_buffer_write_pos += ring_write(write_buffer, write_buffer_size, write_buffer_write_pos, n->string.str + n_off, bytes_to_copy);
            n_off += bytes_to_copy;
          }
        }
      }
      if(write_buffer_write_pos > write_buffer_read_pos)
      {
        U64 file_write_size = file_write(file, r1u64(file_off, file_off + (write_buffer_write_pos-write_buffer_read_pos)), write_buffer);
        file_off += file_write_size;
      }
    }
    dbl_break:;
    good = (file_off == list.total_size);
    file_close(file);
    scratch_end(scratch);
  }
  return good;
}

internal B32
append_data_to_file_path(String8 path, String8 data)
{
  B32 good = 0;
  if(data.size != 0)
  {
    File file = file_open(AccessFlag_Write|AccessFlag_Append, path);
    if(!file_match(file, file_zero()))
    {
      U64 pos = properties_from_file(file).size;
      U64 bytes_written = file_write(file, r1u64(pos, pos+data.size), data.str);
      good = (bytes_written == data.size);
      file_close(file);
    }
  }
  return good;
}

internal FileID
id_from_file_path(String8 path)
{
  File file = file_open(AccessFlag_Read|AccessFlag_ShareRead, path);
  FileID id = id_from_file(file);
  file_close(file);
  return id;
}

internal S64
file_id_compare(FileID a, FileID b)
{
  S64 cmp = MemoryCompare((void*)&a.v[0], (void*)&b.v[0], sizeof(a.v));
  return cmp;
}

internal String8
string_from_file_range(Arena *arena, File file, Rng1U64 range)
{
  U64 pre_pos = arena_pos(arena);
  String8 result;
  result.size = dim_1u64(range);
  result.str = push_array_no_zero(arena, U8, result.size);
  U64 actual_read_size = file_read(file, range, result.str);
  if(actual_read_size < result.size)
  {
    arena_pop_to(arena, pre_pos + actual_read_size);
    result.size = actual_read_size;
  }
  return result;
}

internal String8
file_read_cstring(Arena *arena, File file, U64 off)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List block_list = {0};
  for(U64 cursor = off, stride = 256;; cursor += stride)
  {
    U8      *raw_block = push_array_no_zero(scratch.arena, U8, stride);
    U64      read_size = file_read(file, r1u64(cursor, cursor + stride), raw_block);
    String8  block     = str8_cstring_capped(raw_block, raw_block+read_size);
    str8_list_push(scratch.arena, &block_list, block);
    if(read_size != stride || (block.size+1 <= read_size && block.str[block.size] == 0))
    {
      break;
    }
  }
  String8 result = str8_list_join(arena, &block_list, 0);
  scratch_end(scratch);
  return result;
}
