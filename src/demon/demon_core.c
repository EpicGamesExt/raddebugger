// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/demon.meta.c"

////////////////////////////////
//~ rjf: Basic Type Functions (Helpers, Implemented Once)

//- rjf: handles

internal DMN_Handle
dmn_handle_zero(void)
{
  DMN_Handle h = {0};
  return h;
}

internal B32
dmn_handle_match(DMN_Handle a, DMN_Handle b)
{
  return a.u32[0] == b.u32[0] && a.u32[1] == b.u32[1];
}

//- rjf: trap chunk lists

internal void
dmn_trap_chunk_list_push(Arena *arena, DMN_TrapChunkList *list, U64 cap, DMN_Trap *trap)
{
  DMN_TrapChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, DMN_TrapChunkNode, 1);
    node->cap = cap;
    node->v = push_array_no_zero(arena, DMN_Trap, node->cap);
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], trap);
  node->count += 1;
  list->trap_count += 1;
}

internal void
dmn_trap_chunk_list_concat_in_place(DMN_TrapChunkList *dst, DMN_TrapChunkList *to_push)
{
  if(dst->last == 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  else if(to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->node_count += to_push->node_count;
    dst->trap_count += to_push->trap_count;
  }
  MemoryZeroStruct(to_push);
}

internal void
dmn_trap_chunk_list_concat_shallow_copy(Arena *arena, DMN_TrapChunkList *dst, DMN_TrapChunkList *to_push)
{
  for(DMN_TrapChunkNode *src_n = to_push->first; src_n != 0; src_n = src_n->next)
  {
    DMN_TrapChunkNode *dst_n = push_array(arena, DMN_TrapChunkNode, 1);
    dst_n->v     = src_n->v;
    dst_n->cap   = src_n->cap;
    dst_n->count = src_n->count;
    SLLQueuePush(dst->first, dst->last, dst_n);
    dst->node_count += 1;
    dst->trap_count += dst_n->count;
  }
}

//- rjf: handle lists

internal void
dmn_handle_list_push(Arena *arena, DMN_HandleList *list, DMN_Handle handle)
{
  DMN_HandleNode *node = push_array(arena, DMN_HandleNode, 1);
  SLLQueuePush(list->first, list->last, node);
  node->v = handle;
  list->count += 1;
}

internal DMN_HandleArray
dmn_handle_array_from_list(Arena *arena, DMN_HandleList *list)
{
  DMN_HandleArray array = {0};
  array.count = list->count;
  array.handles = push_array_no_zero(arena, DMN_Handle, array.count);
  U64 idx = 0;
  for(DMN_HandleNode *n = list->first; n != 0; n = n->next, idx += 1)
  {
    array.handles[idx] = n->v;
  }
  return array;
}

internal DMN_HandleArray
dmn_handle_array_copy(Arena *arena, DMN_HandleArray *src)
{
  DMN_HandleArray dst = {0};
  dst.count = src->count;
  dst.handles = push_array_no_zero(arena, DMN_Handle, dst.count);
  MemoryCopy(dst.handles, src->handles, sizeof(DMN_Handle)*dst.count);
  return dst;
}

//- rjf: event list building

internal DMN_Event *
dmn_event_list_push(Arena *arena, DMN_EventList *list)
{
  DMN_EventNode *n = push_array(arena, DMN_EventNode, 1);
  SLLQueuePush(list->first, list->last, n);
  list->count += 1;
  DMN_Event *result = &n->v;
  return result;
}

////////////////////////////////
//~ rjf: Thread Reading Helper Functions (Helpers, Implemented Once)

internal U64
dmn_rip_from_thread(DMN_Handle thread)
{
  U64 result = 0;
  Temp scratch = scratch_begin(0, 0);
  {
    Arch arch = dmn_arch_from_thread(thread);
    U64 reg_block_size = regs_block_size_from_arch(arch);
    void *reg_block = push_array(scratch.arena, U8, reg_block_size);
    dmn_thread_read_reg_block(thread, reg_block);
    result = regs_rip_from_arch_block(arch, reg_block);
  }
  scratch_end(scratch);
  return result;
}

internal U64
dmn_rsp_from_thread(DMN_Handle thread)
{
  U64 result = 0;
  Temp scratch = scratch_begin(0, 0);
  {
    Arch arch = dmn_arch_from_thread(thread);
    U64 reg_block_size = regs_block_size_from_arch(arch);
    void *reg_block = push_array(scratch.arena, U8, reg_block_size);
    dmn_thread_read_reg_block(thread, reg_block);
    result = regs_rsp_from_arch_block(arch, reg_block);
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Process Reading Helper Functions (Helpers, Implemented Once)

internal String8
dmn_process_read_cstring(Arena *arena, DMN_Handle process, U64 addr)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List block_list = {0};
  for(U64 cursor = addr, stride = 256; ; cursor += stride)
  {
    U8      *raw_block = push_array_no_zero(scratch.arena, U8, stride);
    U64      read_size = dmn_process_read(process, r1u64(cursor, cursor + stride), raw_block);
    String8  block     = str8_cstring_capped(raw_block, raw_block + read_size);
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

internal String8
dmn_process_read_block(Arena *arena, DMN_Handle process, Rng1U64 vrange)
{
  String8 block = {0};
  void *raw = dmn_process_read_raw(arena, process, vrange);
  if(raw)
  {
    block = str8(raw, dim_1u64(vrange));
  }
  return block;
}

internal void *
dmn_process_read_raw(Arena *arena, DMN_Handle process, Rng1U64 vrange)
{
  Temp temp = temp_begin(arena);
  void *buffer    = push_array(arena, U8, dim_1u64(vrange));
  U64   read_size = dmn_process_read(process, vrange, buffer);
  if(read_size != dim_1u64(vrange))
  {
    buffer = 0;
    temp_end(temp);
  }
  return buffer;
}
