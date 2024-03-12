// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
dasm_init(void)
{
  Arena *arena = arena_alloc();
  dasm_shared = push_array(arena, DASM_Shared, 1);
  dasm_shared->arena = arena;
  dasm_shared->entity_map.slots_count = 1024;
  dasm_shared->entity_map.slots = push_array(arena, DASM_EntitySlot, dasm_shared->entity_map.slots_count);
  dasm_shared->entity_map_stripes.count = 64;
  dasm_shared->entity_map_stripes.v = push_array(arena, DASM_Stripe, dasm_shared->entity_map_stripes.count);
  for(U64 idx = 0; idx < dasm_shared->entity_map_stripes.count; idx += 1)
  {
    dasm_shared->entity_map_stripes.v[idx].arena = arena_alloc();
    dasm_shared->entity_map_stripes.v[idx].rw_mutex = os_rw_mutex_alloc();
    dasm_shared->entity_map_stripes.v[idx].cv = os_condition_variable_alloc();
  }
  dasm_shared->u2d_ring_mutex = os_mutex_alloc();
  dasm_shared->u2d_ring_cv = os_condition_variable_alloc();
  dasm_shared->u2d_ring_size = KB(64);
  dasm_shared->u2d_ring_base = push_array_no_zero(arena, U8, dasm_shared->u2d_ring_size);
  dasm_shared->decode_thread_count = Max(1, os_logical_core_count()-1);
  dasm_shared->decode_threads = push_array(arena, OS_Handle, dasm_shared->decode_thread_count);
  for(U64 idx = 0; idx < dasm_shared->decode_thread_count; idx += 1)
  {
    dasm_shared->decode_threads[idx] = os_launch_thread(dasm_decode_thread_entry_point, (void *)idx, 0);
  }
}

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
dasm_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

////////////////////////////////
//~ rjf: Instruction Type Functions

internal void
dasm_inst_chunk_list_push(Arena *arena, DASM_InstChunkList *list, U64 cap, DASM_Inst *inst)
{
  DASM_InstChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, DASM_InstChunkNode, 1);
    node->v = push_array_no_zero(arena, DASM_Inst, cap);
    node->cap = cap;
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], inst);
  node->count += 1;
  list->inst_count += 1;
}

internal DASM_InstArray
dasm_inst_array_from_chunk_list(Arena *arena, DASM_InstChunkList *list)
{
  DASM_InstArray array = {0};
  array.count = list->inst_count;
  array.v = push_array_no_zero(arena, DASM_Inst, array.count);
  U64 idx = 0;
  for(DASM_InstChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(DASM_Inst)*n->count);
    idx += n->count;
  }
  return array;
}

internal U64
dasm_inst_array_idx_from_off__linear_scan(DASM_InstArray *array, U64 off)
{
  U64 result = 0;
  for(U64 idx = 0; idx < array->count; idx += 1)
  {
    if(array->v[idx].off == off)
    {
      result = idx;
      break;
    }
  }
  return result;
}

internal U64
dasm_inst_array_off_from_idx(DASM_InstArray *array, U64 idx)
{
  U64 off = 0;
  if(idx < array->count)
  {
    off = array->v[idx].off;
  }
  return off;
}

////////////////////////////////
//~ rjf: Disassembly Functions

#include "third_party/udis86/config.h"
#include "third_party/udis86/udis86.h"
#include "third_party/udis86/libudis86/decode.c"
#include "third_party/udis86/libudis86/itab.c"
#include "third_party/udis86/libudis86/syn-att.c"
#include "third_party/udis86/libudis86/syn-intel.c"
#include "third_party/udis86/libudis86/syn.c"
#include "third_party/udis86/libudis86/udis86.c"

internal DASM_InstChunkList
dasm_inst_chunk_list_from_arch_addr_data(Arena *arena, U64 *bytes_processed_counter, Architecture arch, U64 addr, String8 data)
{
  DASM_InstChunkList inst_list = {0};
  switch(arch)
  {
    default:{}break;
    
    //- rjf: x86/x64 decoding
    case Architecture_x64:
    case Architecture_x86:
    {
      // rjf: grab context
      struct ud udc;
      ud_init(&udc);
      ud_set_mode(&udc, bit_size_from_arch(arch));
      ud_set_pc(&udc, addr);
      ud_set_input_buffer(&udc, data.str, data.size);
      ud_set_vendor(&udc, UD_VENDOR_ANY);
      ud_set_syntax(&udc, UD_SYN_INTEL);
      
      // rjf: disassemble
      U64 byte_process_start_off = 0;
      for(U64 off = 0; off < data.size;)
      {
        // rjf: disassemble one instruction
        U64 size = ud_disassemble(&udc);
        if(size == 0)
        {
          break;
        }
        
        // rjf: analyze
        struct ud_operand *first_op = (struct ud_operand *)ud_insn_opr(&udc, 0);
        U64 rel_voff = (first_op != 0 && first_op->type == UD_OP_JIMM) ? ud_syn_rel_target(&udc, first_op) : 0;
        
        // rjf: push
        String8 string = push_str8f(arena, "%s", udc.asm_buf);
        DASM_Inst inst = {string, off, rel_voff};
        dasm_inst_chunk_list_push(arena, &inst_list, 1024, &inst);
        
        // rjf: increment
        off += size;
        if(bytes_processed_counter != 0 && (off-byte_process_start_off >= 1000))
        {
          ins_atomic_u64_add_eval(bytes_processed_counter, (off-byte_process_start_off));
          byte_process_start_off = off;
        }
      }
    }break;
  }
  return inst_list;
}

////////////////////////////////
//~ rjf: Cache Lookups

//- rjf: opening handles & correllation with module

internal DASM_Handle
dasm_handle_from_ctrl_process_range_arch(CTRL_MachineID machine, DMN_Handle process, Rng1U64 vaddr_range, Architecture arch)
{
  DASM_Handle result = {0};
  if(machine != 0 && process.u64[0] != 0)
  {
    U64 hash = dasm_hash_from_string(str8_struct(&process));
    U64 slot_idx = hash%dasm_shared->entity_map.slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->entity_map_stripes.count;
    DASM_EntitySlot *slot = &dasm_shared->entity_map.slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->entity_map_stripes.v[stripe_idx];
    OS_MutexScopeW(stripe->rw_mutex)
    {
      DASM_Entity *entity = 0;
      for(DASM_Entity *e = slot->first; e != 0; e = e->next)
      {
        if(e->machine_id == machine &&
           ctrl_handle_match(e->process, process) &&
           MemoryMatchStruct(&e->vaddr_range, &vaddr_range) &&
           e->arch == arch)
        {
          entity = e;
          break;
        }
      }
      if(entity == 0)
      {
        entity = push_array(stripe->arena, DASM_Entity, 1);
        SLLQueuePush(slot->first, slot->last, entity);
        entity->machine_id = machine;
        entity->process    = process;
        entity->vaddr_range= vaddr_range;
        entity->arch       = arch;
        entity->id         = ins_atomic_u64_inc_eval(&dasm_shared->entity_id_gen);
        entity->decode_inst_arena = arena_alloc__sized(MB(256), KB(64));
        entity->decode_string_arena = arena_alloc__sized(GB(1), KB(64));
      }
      result.u64[0] = hash;
      result.u64[1] = entity->id;
    }
  }
  return result;
}

//- rjf: asking for top-level info of a handle

internal DASM_BinaryInfo
dasm_binary_info_from_handle(Arena *arena, DASM_Handle handle)
{
  DASM_BinaryInfo info = {0};
  {
    U64 hash = handle.u64[0];
    U64 id = handle.u64[1];
    U64 slot_idx = hash%dasm_shared->entity_map.slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->entity_map_stripes.count;
    DASM_EntitySlot *slot = &dasm_shared->entity_map.slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->entity_map_stripes.v[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex)
    {
      DASM_Entity *entity = 0;
      for(DASM_Entity *e = slot->first; e != 0; e = e->next)
      {
        if(e->id == id)
        {
          entity = e;
          break;
        }
      }
      if(entity != 0)
      {
        info.machine_id       = entity->machine_id;
        info.process          = entity->process;
        info.vaddr_range      = entity->vaddr_range;
        info.bytes_processed  = ins_atomic_u64_eval(&entity->bytes_processed);
        info.bytes_to_process = ins_atomic_u64_eval(&entity->bytes_to_process);
      }
    }
  }
  return info;
}

//- rjf: asking for decoded instructions

internal DASM_InstArray
dasm_inst_array_from_handle(Arena *arena, DASM_Handle handle, U64 endt_us)
{
  DASM_InstArray result = {0};
  if(handle.u64[0] != 0 || handle.u64[1] != 0)
  {
    U64 hash = handle.u64[0];
    U64 id = handle.u64[1];
    U64 slot_idx = hash%dasm_shared->entity_map.slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->entity_map_stripes.count;
    DASM_EntitySlot *slot = &dasm_shared->entity_map.slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->entity_map_stripes.v[stripe_idx];
    B32 sent = 0;
    OS_MutexScopeR(stripe->rw_mutex) for(;;)
    {
      DASM_Entity *entity = 0;
      for(DASM_Entity *e = slot->first; e != 0; e = e->next)
      {
        if(e->id == id)
        {
          entity = e;
          break;
        }
      }
      U64 last_time_sent_us = 0;
      if(entity != 0)
      {
        U64 bytes_processed = ins_atomic_u64_eval(&entity->bytes_processed);
        U64 bytes_to_process = ins_atomic_u64_eval(&entity->bytes_to_process);
        last_time_sent_us = ins_atomic_u64_eval(&entity->last_time_sent_us);
        if(bytes_processed == bytes_to_process && bytes_processed != 0)
        {
          result.count = entity->decode_inst_array.count;
          result.v = push_array_no_zero(arena, DASM_Inst, result.count);
          MemoryCopy(result.v, entity->decode_inst_array.v, sizeof(DASM_Inst)*result.count);
          for(U64 idx = 0; idx < result.count; idx += 1)
          {
            result.v[idx].string = push_str8_copy(arena, result.v[idx].string);
          }
          break;
        }
      }
      if(!sent && entity != 0 && last_time_sent_us+10000 <= os_now_microseconds())
      {
        DASM_DecodeRequest req = {handle};
        sent = dasm_u2d_enqueue_request(&req, endt_us);
        ins_atomic_u64_eval_assign(&entity->last_time_sent_us, os_now_microseconds());
      }
      if(os_now_microseconds() >= endt_us)
      {
        break;
      }
      os_condition_variable_wait_rw_r(stripe->cv, stripe->rw_mutex, endt_us);
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Decode Threads

internal B32
dasm_u2d_enqueue_request(DASM_DecodeRequest *req, U64 endt_us)
{
  B32 result = 0;
  OS_MutexScope(dasm_shared->u2d_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (dasm_shared->u2d_ring_write_pos-dasm_shared->u2d_ring_read_pos);
    U64 available_size = (dasm_shared->u2d_ring_size-unconsumed_size);
    if(available_size >= sizeof(*req))
    {
      result = 1;
      dasm_shared->u2d_ring_write_pos += ring_write_struct(dasm_shared->u2d_ring_base, dasm_shared->u2d_ring_size, dasm_shared->u2d_ring_write_pos, req);
      dasm_shared->u2d_ring_write_pos += 7;
      dasm_shared->u2d_ring_write_pos -= dasm_shared->u2d_ring_write_pos%8;
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(dasm_shared->u2d_ring_cv, dasm_shared->u2d_ring_mutex, endt_us);
  }
  if(result)
  {
    os_condition_variable_broadcast(dasm_shared->u2d_ring_cv);
  }
  return result;
}

internal DASM_DecodeRequest
dasm_u2d_dequeue_request(void)
{
  DASM_DecodeRequest req = {0};
  OS_MutexScope(dasm_shared->u2d_ring_mutex) for(;;)
  {
    U64 unconsumed_size = (dasm_shared->u2d_ring_write_pos-dasm_shared->u2d_ring_read_pos);
    if(unconsumed_size >= sizeof(DASM_DecodeRequest))
    {
      dasm_shared->u2d_ring_read_pos += ring_read_struct(dasm_shared->u2d_ring_base, dasm_shared->u2d_ring_size, dasm_shared->u2d_ring_read_pos, &req);
      dasm_shared->u2d_ring_read_pos += 7;
      dasm_shared->u2d_ring_read_pos -= dasm_shared->u2d_ring_read_pos%8;
      break;
    }
    os_condition_variable_wait(dasm_shared->u2d_ring_cv, dasm_shared->u2d_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(dasm_shared->u2d_ring_cv);
  return req;
}

internal void
dasm_decode_thread_entry_point(void *p)
{
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: get next request & unpack
    DASM_DecodeRequest req = dasm_u2d_dequeue_request();
    DASM_Handle handle = req.handle;
    U64 hash = handle.u64[0];
    U64 id = handle.u64[1];
    U64 slot_idx = hash%dasm_shared->entity_map.slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->entity_map_stripes.count;
    DASM_EntitySlot *slot = &dasm_shared->entity_map.slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->entity_map_stripes.v[stripe_idx];
    
    //- rjf: request -> ctrl info
    B32 is_first_to_task = 0;
    CTRL_MachineID ctrl_machine_id = 0;
    DMN_Handle ctrl_process = {0};
    Rng1U64 vaddr_range = {0};
    Architecture arch = Architecture_Null;
    U64 *bytes_processed_counter = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      DASM_Entity *entity = 0;
      for(DASM_Entity *e = slot->first; e != 0; e = e->next)
      {
        if(e->id == id)
        {
          entity = e;
          break;
        }
      }
      if(entity != 0)
      {
        U64 initial_working_count = ins_atomic_u32_eval_cond_assign(&entity->working_count, 1, 0);
        if(initial_working_count == 0)
        {
          is_first_to_task = 1;
          ctrl_machine_id = entity->machine_id;
          ctrl_process = entity->process;
          vaddr_range = entity->vaddr_range;
          arch = entity->arch;
          bytes_processed_counter = &entity->bytes_processed;
          U64 bytes_to_process = dim_1u64(vaddr_range);
          ins_atomic_u64_eval_assign(&entity->bytes_processed, 0);
          ins_atomic_u64_eval_assign(&entity->bytes_to_process, bytes_to_process);
        }
      }
    }
    
    //- rjf: bad handle or machine id -> bad task
    B32 good_task = (is_first_to_task && ctrl_process.u64[0] != 0 && ctrl_machine_id != 0 && arch != Architecture_Null && bytes_processed_counter != 0);
    
    //- rjf: good task -> clear entity's info
    if(good_task)
    {
      OS_MutexScopeW(stripe->rw_mutex)
      {
        DASM_Entity *entity = 0;
        for(DASM_Entity *e = slot->first; e != 0; e = e->next)
        {
          if(e->id == id)
          {
            entity = e;
            break;
          }
        }
        if(entity != 0)
        {
          arena_clear(entity->decode_inst_arena);
          arena_clear(entity->decode_string_arena);
          MemoryZeroStruct(&entity->decode_inst_array);
        }
      }
    }
    
    //- rjf: good task -> read process memory & decode instructions - stop each
    // 4k and write into cache, so users can read incremental results
    if(good_task)
    {
      U64 chunk_size = KB(4);
      for(U64 off = 0; vaddr_range.min+off < vaddr_range.max; off += chunk_size)
      {
        Rng1U64 chunk_vaddr_range = r1u64(vaddr_range.min+off, vaddr_range.min+off+chunk_size);
        chunk_vaddr_range.min = ClampTop(chunk_vaddr_range.min, vaddr_range.max);
        chunk_vaddr_range.max = ClampTop(chunk_vaddr_range.max, vaddr_range.max);
        
        //- rjf: read next chunk & decode
        String8 data = {0};
        DASM_InstChunkList inst_list = {0};
        if(good_task)
        {
          data.str = push_array_no_zero(scratch.arena, U8, dim_1u64(chunk_vaddr_range));
          data.size = dmn_process_read(ctrl_process, chunk_vaddr_range, data.str);
          if(data.size != 0)
          {
            inst_list = dasm_inst_chunk_list_from_arch_addr_data(scratch.arena, bytes_processed_counter, arch, chunk_vaddr_range.min, data);
          }
        }
        
        //- rjf: write into cache
        {
          OS_MutexScopeW(stripe->rw_mutex)
          {
            DASM_Entity *entity = 0;
            for(DASM_Entity *e = slot->first; e != 0; e = e->next)
            {
              if(e->id == id)
              {
                entity = e;
                break;
              }
            }
            if(entity != 0)
            {
              DASM_Inst *new_chunk_base = push_array(entity->decode_inst_arena, DASM_Inst, inst_list.inst_count);
              U64 off = 0;
              for(DASM_InstChunkNode *node = inst_list.first; node != 0; node = node->next)
              {
                MemoryCopy(new_chunk_base+off, node->v, sizeof(DASM_Inst)*node->count);
                off += node->count;
              }
              for(U64 idx = 0; idx < inst_list.inst_count; idx += 1)
              {
                new_chunk_base[idx].string = push_str8_copy(entity->decode_string_arena, new_chunk_base[idx].string);
              }
              entity->decode_inst_array.count += inst_list.inst_count;
              if(entity->decode_inst_array.v == 0)
              {
                entity->decode_inst_array.v = new_chunk_base;
              }
            }
          }
          os_condition_variable_broadcast(stripe->cv);
        }
      }
    }
    
    //- rjf: mark task as complete
    if(good_task)
    {
      OS_MutexScopeR(stripe->rw_mutex)
      {
        DASM_Entity *entity = 0;
        for(DASM_Entity *e = slot->first; e != 0; e = e->next)
        {
          if(e->id == id)
          {
            entity = e;
            break;
          }
        }
        if(entity != 0)
        {
          U64 bytes_to_process = ins_atomic_u64_eval(&entity->bytes_to_process);
          ins_atomic_u64_eval_assign(&entity->bytes_processed, bytes_to_process);
          ins_atomic_u64_eval_assign(&entity->working_count, 0);
        }
      }
    }
    
    scratch_end(scratch);
  }
}
