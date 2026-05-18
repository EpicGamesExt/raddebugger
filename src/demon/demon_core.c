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
    ARCH_Info *arch_info = arch_info_from_arch(arch);
    U64 reg_block_size = arch_info->reg_block_size;
    void *reg_block = push_array(scratch.arena, U8, reg_block_size);
    dmn_thread_read_reg_block(thread, reg_block);
    result = arch_ip_from_reg_block(arch_info, reg_block);
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
    ARCH_Info *arch_info = arch_info_from_arch(arch);
    U64 reg_block_size = arch_info->reg_block_size;
    void *reg_block = push_array(scratch.arena, U8, reg_block_size);
    dmn_thread_read_reg_block(thread, reg_block);
    result = arch_sp_from_reg_block(arch_info, reg_block);
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Process Reading Helper Functions (Helpers, Implemented Once)

internal String8
dmn_get_trap_inst(void)
{
#if ARCH_X64
  local_persist U8 inst[] = { 0xcc };
#else
# error "trap instruction is not defined for this arch"
#endif
  return str8_array_fixed(inst);
}

internal DMN_ActiveTrap *
dmn_set_trap(Arena *arena, DMN_Trap *trap)
{
  String8 trap_inst = dmn_get_trap_inst();
  U8 *swap_bytes = push_array(arena, U8, trap_inst.size);
  B32 good_read = dmn_process_read(trap->process, r1u64(trap->vaddr, trap->vaddr + trap_inst.size), swap_bytes);
  B32 good_write = 0;
  if(good_read)
  {
    good_write = dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr + trap_inst.size), trap_inst.str);
  }
  DMN_ActiveTrap *result = push_array(arena, DMN_ActiveTrap, 1);
  result->good = (good_read && good_write);
  result->trap = trap;
  result->swap_bytes = str8(swap_bytes, trap_inst.size);
  return result;
}
