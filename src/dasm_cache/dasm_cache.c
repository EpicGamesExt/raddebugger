// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Instruction Decoding/Disassembling Type Functions

#if !defined(ZYDIS_H)
#include "third_party/zydis/zydis.h"
#include "third_party/zydis/zydis.c"
#endif

internal DASM_Inst
dasm_inst_from_code(Arena *arena, Architecture arch, U64 vaddr, String8 code, DASM_Syntax syntax)
{
  DASM_Inst inst = {0};
  switch(arch)
  {
    default:{}break;
    
    //- rjf: x86/x64 disassembly
    case Architecture_x86:
    case Architecture_x64:
    {
      // rjf: determine zydis formatter style
      ZydisFormatterStyle style = ZYDIS_FORMATTER_STYLE_INTEL;
      switch(syntax)
      {
        default:{}break;
        case DASM_Syntax_Intel:{style = ZYDIS_FORMATTER_STYLE_INTEL;}break;
        case DASM_Syntax_ATT:  {style = ZYDIS_FORMATTER_STYLE_ATT;}break;
      }
      
      // rjf: disassemble one instruction
      ZydisDisassembledInstruction zinst = {0};
      ZyanStatus status = ZydisDisassemble(ZYDIS_MACHINE_MODE_LONG_64, vaddr, code.str, code.size, &zinst, style);
      
      // rjf: analyze
      DASM_InstFlags flags = 0;
      U64 jump_dest_vaddr = 0;
      {
        ZydisDecodedOperand *first_visible_op = (zinst.info.operand_count_visible > 0 ? &zinst.operands[0] : 0);
        ZydisDecodedOperand *first_op = (zinst.info.operand_count > 0 ? &zinst.operands[0] : 0);
        ZydisDecodedOperand *second_op = (zinst.info.operand_count > 1 ? &zinst.operands[1] : 0);
        if(first_visible_op != 0)
        {
          ZydisCalcAbsoluteAddress(&zinst.info, first_visible_op, vaddr, &jump_dest_vaddr);
        }
        if(first_op != 0 && second_op != 0 && first_op->type == ZYDIS_OPERAND_TYPE_REGISTER &&
           (first_op->reg.value == ZYDIS_REGISTER_RSP ||
            first_op->reg.value == ZYDIS_REGISTER_ESP ||
            first_op->reg.value == ZYDIS_REGISTER_SP))
        {
          flags |= DASM_InstFlag_ChangesStackPointer;
          if(second_op->type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
          {
            flags |= DASM_InstFlag_ChangesStackPointerVariably;
          }
        }
        if(zinst.info.attributes & (ZYDIS_ATTRIB_HAS_REP|
                                    ZYDIS_ATTRIB_HAS_REPE|
                                    ZYDIS_ATTRIB_HAS_REPZ|
                                    ZYDIS_ATTRIB_HAS_REPNZ|
                                    ZYDIS_ATTRIB_HAS_REPNE))
        {
          flags |= DASM_InstFlag_Repeats;
        }
        switch(zinst.info.mnemonic)
        {
          case ZYDIS_MNEMONIC_CALL:
          {
            flags |= DASM_InstFlag_Call;
          }break;
          
          case ZYDIS_MNEMONIC_JB:
          case ZYDIS_MNEMONIC_JBE:
          case ZYDIS_MNEMONIC_JCXZ:
          case ZYDIS_MNEMONIC_JECXZ:
          case ZYDIS_MNEMONIC_JKNZD:
          case ZYDIS_MNEMONIC_JKZD:
          case ZYDIS_MNEMONIC_JL:
          case ZYDIS_MNEMONIC_JLE:
          case ZYDIS_MNEMONIC_JNB:
          case ZYDIS_MNEMONIC_JNBE:
          case ZYDIS_MNEMONIC_JNL:
          case ZYDIS_MNEMONIC_JNLE:
          case ZYDIS_MNEMONIC_JNO:
          case ZYDIS_MNEMONIC_JNP:
          case ZYDIS_MNEMONIC_JNS:
          case ZYDIS_MNEMONIC_JNZ:
          case ZYDIS_MNEMONIC_JO:
          case ZYDIS_MNEMONIC_JP:
          case ZYDIS_MNEMONIC_JRCXZ:
          case ZYDIS_MNEMONIC_JS:
          case ZYDIS_MNEMONIC_JZ:
          case ZYDIS_MNEMONIC_LOOP:
          case ZYDIS_MNEMONIC_LOOPE:
          case ZYDIS_MNEMONIC_LOOPNE:
          {
            flags |= DASM_InstFlag_Branch;
          }break;
          
          case ZYDIS_MNEMONIC_JMP:
          {
            flags |= DASM_InstFlag_UnconditionalJump;
          }break;
          
          case ZYDIS_MNEMONIC_RET:
          {
            flags |= DASM_InstFlag_Return;
          }break;
          
          case ZYDIS_MNEMONIC_PUSH:
          case ZYDIS_MNEMONIC_POP:
          {
            flags |= DASM_InstFlag_ChangesStackPointer;
          }break;
          
          default:
          {
            flags |= DASM_InstFlag_NonFlow;
          }break;
        }
      }
      
      // rjf: convert
      {
        inst.flags           = flags;
        inst.size            = zinst.info.length;
        inst.string          = push_str8_copy(arena, str8_cstring(zinst.text));
        inst.jump_dest_vaddr = jump_dest_vaddr;
      }
    }break;
  }
  return inst;
}

////////////////////////////////
//~ rjf: Parameter Type Functions

internal B32
dasm_params_match(DASM_Params *a, DASM_Params *b)
{
  B32 result = (a->vaddr == b->vaddr &&
                a->arch == b->arch &&
                a->style_flags == b->style_flags &&
                a->syntax == b->syntax &&
                a->base_vaddr == b->base_vaddr &&
                di_key_match(&a->dbgi_key, &b->dbgi_key));
  return result;
}

////////////////////////////////
//~ rjf: Line Type Functions

internal void
dasm_line_chunk_list_push(Arena *arena, DASM_LineChunkList *list, U64 cap, DASM_Line *inst)
{
  DASM_LineChunkNode *node = list->last;
  if(node == 0 || node->count >= node->cap)
  {
    node = push_array(arena, DASM_LineChunkNode, 1);
    node->v = push_array_no_zero(arena, DASM_Line, cap);
    node->cap = cap;
    SLLQueuePush(list->first, list->last, node);
    list->node_count += 1;
  }
  MemoryCopyStruct(&node->v[node->count], inst);
  node->count += 1;
  list->line_count += 1;
}

internal DASM_LineArray
dasm_line_array_from_chunk_list(Arena *arena, DASM_LineChunkList *list)
{
  DASM_LineArray array = {0};
  array.count = list->line_count;
  array.v = push_array_no_zero(arena, DASM_Line, array.count);
  U64 idx = 0;
  for(DASM_LineChunkNode *n = list->first; n != 0; n = n->next)
  {
    MemoryCopy(array.v+idx, n->v, sizeof(DASM_Line)*n->count);
    idx += n->count;
  }
  return array;
}

internal U64
dasm_line_array_idx_from_code_off__linear_scan(DASM_LineArray *array, U64 off)
{
  U64 result = 0;
  for(U64 idx = 0; idx < array->count; idx += 1)
  {
    U64 next_off = (idx+1 < array->count ? array->v[idx+1].code_off : max_U64);
    if(array->v[idx].code_off <= off && off < next_off)
    {
      result = idx;
      if(!(array->v[idx].flags & DASM_LineFlag_Decorative))
      {
        break;
      }
    }
  }
  return result;
}

internal U64
dasm_line_array_code_off_from_idx(DASM_LineArray *array, U64 idx)
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
  dasm_shared->stripes_count = Min(dasm_shared->slots_count, os_get_system_info()->logical_processor_count);
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
    dasm_shared->parse_threads[idx] = os_thread_launch(dasm_parse_thread__entry_point, (void *)idx, 0);
  }
  dasm_shared->evictor_detector_thread = os_thread_launch(dasm_evictor_detector_thread__entry_point, 0, 0);
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
  U64 base_pos = arena_pos(dasm_tctx->arena);
  DASM_Scope *scope = push_array(dasm_tctx->arena, DASM_Scope, 1);
  scope->base_pos = base_pos;
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
        if(u128_match(t->hash, n->hash) && dasm_params_match(&t->params, &n->params))
        {
          ins_atomic_u64_dec_eval(&n->scope_ref_count);
          break;
        }
      }
    }
  }
  arena_pop_to(dasm_tctx->arena, scope->base_pos);
}

internal void
dasm_scope_touch_node__stripe_r_guarded(DASM_Scope *scope, DASM_Node *node)
{
  DASM_Touch *touch = push_array(dasm_tctx->arena, DASM_Touch, 1);
  ins_atomic_u64_inc_eval(&node->scope_ref_count);
  ins_atomic_u64_eval_assign(&node->last_time_touched_us, os_now_microseconds());
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, dasm_user_clock_idx());
  touch->hash = node->hash;
  MemoryCopyStruct(&touch->params, &node->params);
  touch->params.dbgi_key = di_key_copy(dasm_tctx->arena, &touch->params.dbgi_key);
  SLLStackPush(scope->top_touch, touch);
}

////////////////////////////////
//~ rjf: Cache Lookups

internal DASM_Info
dasm_info_from_hash_params(DASM_Scope *scope, U128 hash, DASM_Params *params)
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
        if(u128_match(hash, n->hash) && dasm_params_match(params, &n->params))
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
          if(u128_match(hash, n->hash) && dasm_params_match(params, &n->params))
          {
            node = n;
            break;
          }
        }
        if(node == 0)
        {
          LogInfoNamedBlockF("dasm_new_node")
          {
            log_infof("hash:        [0x%I64x 0x%I64x]\n", hash.u64[0], hash.u64[1]);
            log_infof("vaddr:       0x%I64x\n", params->vaddr);
            log_infof("arch:        %S\n", string_from_architecture(params->arch));
            log_infof("style_flags: 0x%x\n", params->style_flags);
            log_infof("syntax:      %i\n",   params->syntax);
            log_infof("base_vaddr:  0x%I64x\n", params->base_vaddr);
            log_infof("dbgi_key:    [%S 0x%I64x]\n", params->dbgi_key.path, params->dbgi_key.min_timestamp);
          }
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
          MemoryCopyStruct(&node->params, params);
          // TODO(rjf): need to make this releasable - currently all exe_paths just leak
          node->params.dbgi_key = di_key_copy(stripe->arena, &node->params.dbgi_key);
          node_is_new = 1;
        }
      }
    }
    if(node_is_new)
    {
      dasm_u2p_enqueue_req(hash, params, max_U64);
    }
  }
  return info;
}

internal DASM_Info
dasm_info_from_key_params(DASM_Scope *scope, U128 key, DASM_Params *params, U128 *hash_out)
{
  DASM_Info result = {0};
  for(U64 rewind_idx = 0; rewind_idx < 2; rewind_idx += 1)
  {
    U128 hash = hs_hash_from_key(key, rewind_idx);
    result = dasm_info_from_hash_params(scope, hash, params);
    if(result.lines.count != 0)
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
dasm_u2p_enqueue_req(U128 hash, DASM_Params *params, U64 endt_us)
{
  B32 good = 0;
  OS_MutexScope(dasm_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = dasm_shared->u2p_ring_write_pos - dasm_shared->u2p_ring_read_pos;
    U64 available_size = dasm_shared->u2p_ring_size - unconsumed_size;
    if(available_size >= sizeof(hash)+sizeof(U64)+sizeof(Architecture)+sizeof(DASM_StyleFlags)+sizeof(DASM_Syntax)+sizeof(U64)+sizeof(U64)+params->dbgi_key.path.size+sizeof(U64))
    {
      good = 1;
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &hash);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->vaddr);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->arch);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->style_flags);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->syntax);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->base_vaddr);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->dbgi_key.path.size);
      dasm_shared->u2p_ring_write_pos += ring_write(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, params->dbgi_key.path.str, params->dbgi_key.path.size);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->dbgi_key.min_timestamp);
      dasm_shared->u2p_ring_write_pos += 7;
      dasm_shared->u2p_ring_write_pos -= dasm_shared->u2p_ring_write_pos%8;
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
dasm_u2p_dequeue_req(Arena *arena, U128 *hash_out, DASM_Params *params_out)
{
  OS_MutexScope(dasm_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = dasm_shared->u2p_ring_write_pos - dasm_shared->u2p_ring_read_pos;
    if(unconsumed_size >= sizeof(*hash_out)+sizeof(U64)+sizeof(Architecture)+sizeof(DASM_StyleFlags)+sizeof(DASM_Syntax)+sizeof(U64)+sizeof(U64)+sizeof(U64))
    {
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, hash_out);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->vaddr);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->arch);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->style_flags);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->syntax);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->base_vaddr);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->dbgi_key.path.size);
      params_out->dbgi_key.path.str = push_array(arena, U8, params_out->dbgi_key.path.size);
      dasm_shared->u2p_ring_read_pos += ring_read(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, params_out->dbgi_key.path.str, params_out->dbgi_key.path.size);
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, &params_out->dbgi_key.min_timestamp);
      dasm_shared->u2p_ring_read_pos += 7;
      dasm_shared->u2p_ring_read_pos -= dasm_shared->u2p_ring_read_pos%8;
      break;
    }
    os_condition_variable_wait(dasm_shared->u2p_ring_cv, dasm_shared->u2p_ring_mutex, max_U64);
  }
  os_condition_variable_broadcast(dasm_shared->u2p_ring_cv);
}

internal void
dasm_parse_thread__entry_point(void *p)
{
  ThreadNameF("[dasm] parse thread #%I64u", (U64)p);
  for(;;)
  {
    Temp scratch = scratch_begin(0, 0);
    HS_Scope *hs_scope = hs_scope_open();
    DI_Scope *di_scope = di_scope_open();
    TXT_Scope *txt_scope = txt_scope_open();
    
    //- rjf: get next request
    U128 hash = {0};
    DASM_Params params = {0};
    dasm_u2p_dequeue_req(scratch.arena, &hash, &params);
    U64 change_gen = fs_change_gen();
    
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
        if(u128_match(n->hash, hash) && dasm_params_match(&n->params, &params))
        {
          got_task = !ins_atomic_u32_eval_cond_assign(&n->is_working, 1, 0);
          break;
        }
      }
    }
    
    //- rjf: get dbg info
    RDI_Parsed *rdi = &di_rdi_parsed_nil;
    if(got_task && params.dbgi_key.path.size != 0)
    {
      rdi = di_rdi_from_key(di_scope, &params.dbgi_key, max_U64);
    }
    
    //- rjf: hash -> data
    String8 data = {0};
    if(got_task)
    {
      data = hs_data_from_hash(hs_scope, hash);
    }
    
    //- rjf: data * arch * addr * dbg -> decode artifacts
    DASM_LineChunkList line_list = {0};
    String8List inst_strings = {0};
    if(got_task)
    {
      switch(params.arch)
      {
        default:{}break;
        
        //- rjf: x86/x64 decoding
        case Architecture_x64:
        case Architecture_x86:
        {
          // rjf: disassemble
          RDI_SourceFile *last_file = &rdi_nil_element_union.source_file;
          RDI_Line *last_line = 0;
          for(U64 off = 0; off < data.size;)
          {
            // rjf: disassemble one instruction
            DASM_Inst inst = dasm_inst_from_code(scratch.arena, params.arch, params.vaddr+off, str8_skip(data, off), params.syntax);
            if(inst.size == 0)
            {
              break;
            }
            
            // rjf: push strings derived from voff -> line info
            if(params.style_flags & (DASM_StyleFlag_SourceFilesNames|DASM_StyleFlag_SourceLines))
            {
              if(rdi != &di_rdi_parsed_nil)
              {
                U64 voff = (params.vaddr+off) - params.base_vaddr;
                U32 unit_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_UnitVMap, voff);
                RDI_Unit *unit = rdi_element_from_name_idx(rdi, Units, unit_idx);
                RDI_LineTable *line_table = rdi_element_from_name_idx(rdi, LineTables, unit->line_table_idx);
                RDI_ParsedLineTable unit_line_info = {0};
                rdi_parsed_from_line_table(rdi, line_table, &unit_line_info);
                U64 line_info_idx = rdi_line_info_idx_from_voff(&unit_line_info, voff);
                if(line_info_idx < unit_line_info.count)
                {
                  RDI_Line *line = &unit_line_info.lines[line_info_idx];
                  RDI_SourceFile *file = rdi_element_from_name_idx(rdi, SourceFiles, line->file_idx);
                  String8 file_normalized_full_path = {0};
                  file_normalized_full_path.str = rdi_string_from_idx(rdi, file->normal_full_path_string_idx, &file_normalized_full_path.size);
                  if(file != last_file)
                  {
                    if(params.style_flags & DASM_StyleFlag_SourceFilesNames &&
                       file->normal_full_path_string_idx != 0 && file_normalized_full_path.size != 0)
                    {
                      String8 inst_string = push_str8f(scratch.arena, "> %S", file_normalized_full_path);
                      DASM_Line inst = {u32_from_u64_saturate(off), DASM_LineFlag_Decorative, 0, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                                       inst_strings.total_size + inst_strings.node_count + inst_string.size)};
                      dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &inst);
                      str8_list_push(scratch.arena, &inst_strings, inst_string);
                    }
                    if(params.style_flags & DASM_StyleFlag_SourceFilesNames && file->normal_full_path_string_idx == 0)
                    {
                      String8 inst_string = str8_lit(">");
                      DASM_Line inst = {u32_from_u64_saturate(off), DASM_LineFlag_Decorative, 0, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                                       inst_strings.total_size + inst_strings.node_count + inst_string.size)};
                      dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &inst);
                      str8_list_push(scratch.arena, &inst_strings, inst_string);
                    }
                    last_file = file;
                  }
                  if(line && line != last_line && file->normal_full_path_string_idx != 0 &&
                     params.style_flags & DASM_StyleFlag_SourceLines &&
                     file_normalized_full_path.size != 0)
                  {
                    FileProperties props = os_properties_from_file_path(file_normalized_full_path);
                    if(props.modified != 0)
                    {
                      // TODO(rjf): need redirection path - this may map to a different path on the local machine,
                      // need frontend to communicate path remapping info to this layer
                      U128 key = fs_key_from_path(file_normalized_full_path);
                      TXT_LangKind lang_kind = txt_lang_kind_from_extension(file_normalized_full_path);
                      U64 endt_us = max_U64;
                      U128 hash = {0};
                      TXT_TextInfo text_info = {0};
                      for(;os_now_microseconds() <= endt_us;)
                      {
                        text_info = txt_text_info_from_key_lang(txt_scope, key, lang_kind, &hash);
                        if(!u128_match(hash, u128_zero()))
                        {
                          break;
                        }
                      }
                      if(0 < line->line_num && line->line_num < text_info.lines_count)
                      {
                        String8 data = hs_data_from_hash(hs_scope, hash);
                        String8 line_text = str8_skip_chop_whitespace(str8_substr(data, text_info.lines_ranges[line->line_num-1]));
                        if(line_text.size != 0)
                        {
                          String8 inst_string = push_str8f(scratch.arena, "> %S", line_text);
                          DASM_Line inst = {u32_from_u64_saturate(off), DASM_LineFlag_Decorative, 0, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                                           inst_strings.total_size + inst_strings.node_count + inst_string.size)};
                          dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &inst);
                          str8_list_push(scratch.arena, &inst_strings, inst_string);
                        }
                      }
                    }
                    last_line = line;
                  }
                }
              }
            }
            
            // rjf: push line
            String8 addr_part = {0};
            if(params.style_flags & DASM_StyleFlag_Addresses)
            {
              addr_part = push_str8f(scratch.arena, "%s0x%016I64x  ", rdi != &di_rdi_parsed_nil ? "  " : "", params.vaddr+off);
            }
            String8 code_bytes_part = {0};
            if(params.style_flags & DASM_StyleFlag_CodeBytes)
            {
              String8List code_bytes_strings = {0};
              str8_list_push(scratch.arena, &code_bytes_strings, str8_lit("{"));
              for(U64 byte_idx = 0; byte_idx < inst.size || byte_idx < 16; byte_idx += 1)
              {
                if(byte_idx < inst.size)
                {
                  str8_list_pushf(scratch.arena, &code_bytes_strings, "%02x%s ", (U32)data.str[off+byte_idx], byte_idx == inst.size-1 ? "}" : "");
                }
                else if(byte_idx < 8)
                {
                  str8_list_push(scratch.arena, &code_bytes_strings, str8_lit("   "));
                }
              }
              str8_list_push(scratch.arena, &code_bytes_strings, str8_lit(" "));
              code_bytes_part = str8_list_join(scratch.arena, &code_bytes_strings, 0);
            }
            String8 symbol_part = {0};
            if(inst.jump_dest_vaddr != 0 && rdi != &di_rdi_parsed_nil && params.style_flags & DASM_StyleFlag_SymbolNames)
            {
              RDI_U32 scope_idx = rdi_vmap_idx_from_section_kind_voff(rdi, RDI_SectionKind_ScopeVMap, inst.jump_dest_vaddr-params.base_vaddr);
              if(scope_idx != 0)
              {
                RDI_Scope *scope = rdi_element_from_name_idx(rdi, Scopes, scope_idx);
                RDI_U32 procedure_idx = scope->proc_idx;
                RDI_Procedure *procedure = rdi_element_from_name_idx(rdi, Procedures, procedure_idx);
                String8 procedure_name = {0};
                procedure_name.str = rdi_string_from_idx(rdi, procedure->name_string_idx, &procedure_name.size);
                if(procedure_name.size != 0)
                {
                  symbol_part = push_str8f(scratch.arena, " (%S)", procedure_name);
                }
              }
            }
            String8 inst_string = push_str8f(scratch.arena, "%S%S%S%S", addr_part, code_bytes_part, inst.string, symbol_part);
            DASM_Line line = {u32_from_u64_saturate(off), 0, inst.jump_dest_vaddr, r1u64(inst_strings.total_size + inst_strings.node_count,
                                                                                         inst_strings.total_size + inst_strings.node_count + inst_string.size)};
            dasm_line_chunk_list_push(scratch.arena, &line_list, 1024, &line);
            str8_list_push(scratch.arena, &inst_strings, inst_string);
            
            // rjf: increment
            off += inst.size;
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
          params.vaddr,
          (U64)params.arch,
          (U64)params.style_flags,
          (U64)params.syntax,
          (U64)rdi,
          0x4d534144,
        };
        text_key = hs_hash_from_data(str8((U8 *)hash_data, sizeof(hash_data)));
      }
      
      //- rjf: submit text data to hash store
      U128 text_hash = hs_submit_data(text_key, &text_arena, text);
      
      //- rjf: produce value bundle
      info_arena = arena_alloc();
      info.text_key = text_key;
      info.lines = dasm_line_array_from_chunk_list(info_arena, &line_list);
    }
    
    //- rjf: commit results to cache
    if(got_task) OS_MutexScopeW(stripe->rw_mutex)
    {
      for(DASM_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash) && dasm_params_match(&n->params, &params))
        {
          n->info_arena = info_arena;
          MemoryCopyStruct(&n->info, &info);
          if(rdi != &di_rdi_parsed_nil && params.style_flags & (DASM_StyleFlag_SourceLines|DASM_StyleFlag_SourceFilesNames))
          {
            n->change_gen = change_gen;
          }
          else
          {
            n->change_gen = 0;
          }
          ins_atomic_u32_eval_assign(&n->is_working, 0);
          ins_atomic_u64_inc_eval(&n->load_count);
          break;
        }
      }
    }
    
    txt_scope_close(txt_scope);
    di_scope_close(di_scope);
    hs_scope_close(hs_scope);
    scratch_end(scratch);
  }
}

////////////////////////////////
//~ rjf: Evictor/Detector Thread

internal void
dasm_evictor_detector_thread__entry_point(void *p)
{
  ThreadNameF("[dasm] evictor/detector thread");
  for(;;)
  {
    U64 change_gen = fs_change_gen();
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = dasm_user_clock_idx();
    U64 evict_threshold_us = 10*1000000;
    U64 retry_threshold_us =  1*1000000;
    U64 evict_threshold_user_clocks = 10;
    U64 retry_threshold_user_clocks = 10;
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
          if(n->change_gen != 0 && n->change_gen != change_gen &&
             n->last_time_requested_us+retry_threshold_us <= check_time_us &&
             n->last_user_clock_idx_requested+retry_threshold_user_clocks <= check_time_user_clocks)
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
          if(n->change_gen != 0 && n->change_gen != change_gen &&
             n->last_time_requested_us+retry_threshold_us <= check_time_us &&
             n->last_user_clock_idx_requested+retry_threshold_user_clocks <= check_time_user_clocks)
          {
            if(dasm_u2p_enqueue_req(n->hash, &n->params, max_U64))
            {
              n->last_time_requested_us = os_now_microseconds();
              n->last_user_clock_idx_requested = check_time_user_clocks;
            }
          }
        }
      }
    }
    os_sleep_milliseconds(100);
  }
}
