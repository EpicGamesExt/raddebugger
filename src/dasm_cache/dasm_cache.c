// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Third Party Includes

#include "third_party/udis86/config.h"
#include "third_party/udis86/udis86.h"
#include "third_party/udis86/libudis86/decode.c"
#include "third_party/udis86/libudis86/itab.c"
#include "third_party/udis86/libudis86/syn-att.c"
#include "third_party/udis86/libudis86/syn-intel.c"
#include "third_party/udis86/libudis86/syn.c"
#include "third_party/udis86/libudis86/udis86.c"

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
dasm_inst_array_idx_from_code_off__linear_scan(DASM_InstArray *array, U64 off)
{
  U64 result = 0;
  for(U64 idx = 0; idx < array->count; idx += 1)
  {
    if(array->v[idx].code_off == off)
    {
      result = idx;
      break;
    }
  }
  return result;
}

internal U64
dasm_inst_array_code_off_from_idx(DASM_InstArray *array, U64 idx)
{
  U64 off = 0;
  if(idx < array->count)
  {
    off = array->v[idx].code_off;
  }
  return off;
}

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void
dasm_init(void)
{
  Arena *arena = arena_alloc();
  dasm_shared = push_array(arena, DASM_Shared, 1);
  dasm_shared->arena = arena;
  dasm_shared->slots_count = 1024;
  dasm_shared->stripes_count = Min(dasm_shared->slots_count, os_logical_core_count());
  dasm_shared->slots = push_array(arena, DASM_Slot, dasm_shared->slots_count);
  dasm_shared->stripes = push_array(arena, DASM_Stripe, dasm_shared->stripes_count);
  for(U64 idx = 0; idx < dasm_shared->stripes_count; idx += 1)
  {
    dasm_shared->stripes[idx].arena = arena_alloc();
    dasm_shared->stripes[idx].rw_mutex = os_rw_mutex_alloc();
    dasm_shared->stripes[idx].cv = os_condition_variable_alloc();
  }
  dasm_shared->u2p_ring_size = KB(64);
  dasm_shared->u2p_ring_base = push_array_no_zero(arena, U8, dasm_shared->u2p_ring_size);
  dasm_shared->u2p_ring_cv = os_condition_variable_alloc();
  dasm_shared->u2p_ring_mutex = os_mutex_alloc();
  dasm_shared->parse_thread_count = 1;
  dasm_shared->parse_threads = push_array(arena, OS_Handle, dasm_shared->parse_thread_count);
  for(U64 idx = 0; idx < dasm_shared->parse_thread_count; idx += 1)
  {
    dasm_shared->parse_threads[idx] = os_launch_thread(dasm_parse_thread__entry_point, (void *)idx, 0);
  }
  dasm_shared->evictor_thread = os_launch_thread(dasm_evictor_thread__entry_point, 0, 0);
}

////////////////////////////////
//~ rjf: User Clock

internal void
dasm_user_clock_tick(void)
{
  ins_atomic_u64_inc_eval(&dasm_shared->user_clock_idx);
}

internal U64
dasm_user_clock_idx(void)
{
  U64 idx = ins_atomic_u64_eval(&dasm_shared->user_clock_idx);
  return idx;
}

////////////////////////////////
//~ rjf: Scoped Access

internal DASM_Scope *
dasm_scope_open(void)
{
  if(dasm_tctx == 0)
  {
    Arena *arena = arena_alloc();
    dasm_tctx = push_array(arena, DASM_TCTX, 1);
    dasm_tctx->arena = arena;
  }
  DASM_Scope *scope = dasm_tctx->free_scope;
  if(scope != 0)
  {
    SLLStackPop(dasm_tctx->free_scope);
  }
  else
  {
    scope = push_array_no_zero(dasm_tctx->arena, DASM_Scope, 1);
  }
  MemoryZeroStruct(scope);
  return scope;
}

internal void
dasm_scope_close(DASM_Scope *scope)
{
  for(DASM_Touch *t = scope->top_touch, *next = 0; t != 0; t = next)
  {
    next = t->next;
    U64 slot_idx = t->hash.u64[1]%dasm_shared->slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
    DASM_Slot *slot = &dasm_shared->slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(DASM_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(t->hash, n->hash) &&
           t->addr == n->addr &&
           t->arch == n->arch &&
           t->style_flags == n->style_flags &&
           t->syntax == n->syntax)
        {
          ins_atomic_u64_dec_eval(&n->scope_ref_count);
          break;
        }
      }
    }
    SLLStackPush(dasm_tctx->free_touch, t);
  }
  SLLStackPush(dasm_tctx->free_scope, scope);
}

internal void
dasm_scope_touch_node__stripe_r_guarded(DASM_Scope *scope, DASM_Node *node)
{
  DASM_Touch *touch = dasm_tctx->free_touch;
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  ins_atomic_u64_eval_assign(&node->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, dasm_user_clock_idx());
  if(touch != 0)
  {
    SLLStackPop(dasm_tctx->free_touch);
  }
  else
  {
    touch = push_array_no_zero(dasm_tctx->arena, DASM_Touch, 1);
  }
  MemoryZeroStruct(touch);
  touch->hash = node->hash;
  touch->addr = node->addr;
  touch->arch = node->arch;
  touch->style_flags = node->style_flags;
  touch->syntax = node->syntax;
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal DASM_Info
dasm_info_from_hash_addr_arch_style(DASM_Scope *scope, U128 hash, U64 addr, Architecture arch, DASM_StyleFlags style_flags, DASM_Syntax syntax)
{
  DASM_Info info = {0};
  if(!u128_match(hash, u128_zero()))
  {
    U64 slot_idx = hash.u64[1]%dasm_shared->slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
    DASM_Slot *slot = &dasm_shared->slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
    B32 found = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(DASM_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(hash, n->hash) &&
           addr == n->addr &&
           arch == n->arch &&
           style_flags == n->style_flags &&
           syntax == n->syntax)
        {
          MemoryCopyStruct(&info, &n->info);
          found = 1;
          dasm_scope_touch_node__stripe_r_guarded(scope, n);
          break;
        }
      }
    }
    B32 node_is_new = 0;
    if(!found)
    {
      OS_MutexScopeW(stripe->rw_mutex)
      {
        DASM_Node *node = 0;
        for(DASM_Node *n = slot->first; n != 0; n = n->next)
        {
          if(u128_match(hash, n->hash) &&
             addr == n->addr &&
             arch == n->arch &&
             style_flags == n->style_flags &&
             syntax == n->syntax)
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          node = stripe->free_node;
          if(node)
          {
            SLLStackPop(stripe->free_node);
          }
          else
          {
            node = push_array_no_zero(stripe->arena, DASM_Node, 1);
          }
          MemoryZeroStruct(node);
          DLLPushBack(slot->first, slot->last, node);
          node->hash = hash;
          node->addr = addr;
          node->arch = arch;
          node->style_flags = style_flags;
          node->syntax = syntax;
          node_is_new = 1;
        }
      }
    }
    if(node_is_new)
    {
      dasm_u2p_enqueue_req(hash, addr, arch, style_flags, syntax, max_U64);
    }
  }
  return info;
}

internal DASM_Info
dasm_info_from_key_addr_arch_style(DASM_Scope *scope, U128 key, U64 addr, Architecture arch, DASM_StyleFlags style_flags, DASM_Syntax syntax, U128 *hash_out)
{
  DASM_Info result = {0};
  for(U64 rewind_idx = 0; rewind_idx < 2; rewind_idx += 1)
  {
    U128 hash = hs_hash_from_key(key, rewind_idx);
    result = dasm_info_from_hash_addr_arch_style(scope, hash, addr, arch, style_flags, syntax);
    if(result.insts.count != 0)
    {
      if(hash_out)
      {
        *hash_out = hash;
      }
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Parse Threads

internal B32
dasm_u2p_enqueue_req(U128 hash, U64 addr, Architecture arch, DASM_StyleFlags style_flags, DASM_Syntax syntax, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(dasm_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = dasm_shared->u2p_ring_write_pos - dasm_shared->u2p_ring_read_pos;
    U64 available_size = dasm_shared->u2p_ring_size - unconsumed_size;
    if(available_size >= sizeof(hash)+sizeof(addr)+sizeof(arch))
    {
      good = 1;
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &hash);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &addr);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &arch);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &style_flags);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &syntax);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    os_condition_variable_wait(dasm_shared->u2p_ring_cv, dasm_shared->u2p_ring_mutex, endt_us);
  }
  if(good)
  {
    os_condition_variable_broadcast(dasm_shared->u2p_ring_cv);
  }
  return good;
}

internal void
dasm_u2p_dequeue_req(U128 *hash_out, U64 *addr_out, Architecture *arch_out, DASM_StyleFlags *style_flags_out, DASM_Syntax *syntax_out)
{
  OS_MutexScope(dasm_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = dasm_shared->u2p_ring_write_pos - dasm_shared->u2p_ring_read_pos;
    if(unconsumed_size >= sizeof(*hash_out)+sizeof(*addr_out)+sizeof(*arch_out))
    {
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, hash_out);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, addr_out);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, arch_out);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, style_flags_out);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, syntax_out);
      break;
    }
    os_condition_variable_wait(dasm_shared->u2p_ring_cv, dasm_shared->u2p_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(dasm_shared->u2p_ring_mutex);
}

internal void
dasm_parse_thread__entry_point(void *p)
{
  ThreadNameF("[dasm] parse thread #%I64u", (U64)p);
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    
    //- rjf: get next request
    U128 hash = {0};
    U64 addr = 0;
    Architecture arch = Architecture_Null;
    DASM_StyleFlags style_flags = 0;
    DASM_Syntax syntax = DASM_Syntax_Intel;
    dasm_u2p_dequeue_req(&hash, &addr, &arch, &style_flags, &syntax);
    HS_Scope *hs_scope = hs_scope_open();
    
    //- rjf: unpack hash
    U64 slot_idx = hash.u64[1]%dasm_shared->slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
    DASM_Slot *slot = &dasm_shared->slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
    
    //- rjf: take task
    B32 got_task = 0;
    OS_MutexScopeR(stripe->rw_mutex)
    {
      for(DASM_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash) && n->addr == addr && n->arch == arch)
        {
          got_task = !ins_atomic_u32_eval_cond_assign(&n->is_working, 1, 0);
          break;
        }
      }
    }
    
    //- rjf: hash -> data
    String8 data = {0};
    if(got_task)
    {
      data = hs_data_from_hash(hs_scope, hash);
    }
    
    //- rjf: data * arch * addr -> decode artifacts
    DASM_InstChunkList inst_list = {0};
    String8List inst_strings = {0};
    if(got_task)
    {
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
            String8 string = push_str8f(scratch.arena, "%s", udc.asm_buf);
            DASM_Inst inst = {off, rel_voff, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                   inst_strings.total_size + inst_strings.node_count + string.size)};
            dasm_inst_chunk_list_push(scratch.arena, &inst_list, 1024, &inst);
            str8_list_push(scratch.arena, &inst_strings, string);
            
            // rjf: increment
            off += size;
          }
        }break;
      }
    }
    
    //- rjf: artifacts -> value bundle
    Arena *info_arena = 0;
    DASM_Info info = {0};
    if(got_task)
    {
      //- rjf: produce joined text
      Arena *text_arena = arena_alloc();
      StringJoin text_join = {0};
      text_join.sep = str8_lit("\n");
      String8 text = str8_list_join(text_arena, &inst_strings, &text_join);
      
      //- rjf: produce unique key for this disassembly's text
      U128 text_key = {0};
      {
        U64 hash_data[] =
        {
          hash.u64[0],
          hash.u64[1],
          addr,
          (U64)arch,
          0x4d534144,
        };
        text_key = hs_hash_from_data(str8((U8 *)hash_data, sizeof(hash_data)));
      }
      
      //- rjf: submit text data to hash store
      U128 text_hash = hs_submit_data(text_key, &text_arena, text);
      
      //- rjf: produce value bundle
      info_arena = arena_alloc();
      info.text_key = text_key;
      info.insts = dasm_inst_array_from_chunk_list(info_arena, &inst_list);
    }
    
    //- rjf: commit results to cache
    if(got_task) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(DASM_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash) &&
           addr == n->addr &&
           arch == n->arch &&
           style_flags == n->style_flags &&
           syntax == n->syntax)
        {
          n->info_arena = info_arena;
          MemoryCopyStruct(&n->info, &info);
          ins_atomic_u32_eval_assign(&n->is_working, 0);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
      }
    }
    
    hs_scope_close(hs_scope);
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Evictor Threads

internal void
dasm_evictor_thread__entry_point(void *p)
{
  ThreadNameF("[dasm] evictor thread");
  for(;;)
  {
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = dasm_user_clock_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 evict_threshold_user_clocks = 10;
    for(U64 slot_idx = 0; slot_idx < dasm_shared->slots_count; slot_idx += 1)
    {
      U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
      DASM_Slot *slot = &dasm_shared->slots[slot_idx];
      DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
      B32 slot_has_work = 0;
      OS_MutexScopeR(stripe->rw_mutex)
      {
        for(DASM_Node *n = slot->first; n != 0; n = n->next)
        {
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            slot_has_work = 1;
            break;
          }
        }
      }
      if(slot_has_work) OS_MutexScopeW(stripe->rw_mutex)
      {
        for(DASM_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             n->load_count != 0 &&
             n->is_working == 0)
          {
            DLLRemove(slot->first, slot->last, n);
            if(n->info_arena != 0)
            {
              arena_release(n->info_arena);
            }
            SLLStackPush(stripe->free_node, n);
          }
        }
      }
      os_sleep_milliseconds(5);
    }
    os_sleep_milliseconds(1000);
  }
}
