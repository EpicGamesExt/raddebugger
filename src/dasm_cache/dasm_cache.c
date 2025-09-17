// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#undef LAYER_COLOR
#define LAYER_COLOR 0xe34cd4ff

////////////////////////////////
//~ rjf: Instruction Decoding/Disassembling Type Functions

#if !defined(ZYDIS_H)
#include "third_party/zydis/zydis.h"
#include "third_party/zydis/zydis.c"
#endif

internal DASM_Inst
dasm_inst_from_code(Arena *arena, Arch arch, U64 vaddr, String8 code, DASM_Syntax syntax)
{
  DASM_Inst inst = {0};
  switch(arch)
  {
    default:{}break;
    
    //- rjf: x86/x64 disassembly
    case Arch_x86:
    case Arch_x64:
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
        if(first_visible_op != 0 && 
           (first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM8 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM32 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM64 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16_32_64 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM32_32_64 ||
            first_visible_op->encoding == ZYDIS_OPERAND_ENCODING_JIMM16_32_32))
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
//~ rjf: Control Flow Analysis

internal DASM_CtrlFlowInfo
dasm_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DASM_InstFlags exit_points_mask, Arch arch, U64 vaddr, String8 code)
{
  Temp scratch = scratch_begin(&arena, 1);
  DASM_CtrlFlowInfo info = {0};
  for(U64 offset = 0; offset < code.size;)
  {
    DASM_Inst inst = dasm_inst_from_code(scratch.arena, arch, vaddr+offset, str8_skip(code, offset), DASM_Syntax_Intel);
    U64 inst_vaddr = vaddr+offset;
    offset += inst.size;
    info.total_size += inst.size;
    if(inst.flags & exit_points_mask)
    {
      DASM_CtrlFlowPoint point = {0};
      point.inst_flags = inst.flags;
      point.vaddr = inst_vaddr;
      point.jump_dest_vaddr = inst.jump_dest_vaddr;
      DASM_CtrlFlowPointNode *node = push_array(arena, DASM_CtrlFlowPointNode, 1);
      node->v = point;
      SLLQueuePush(info.exit_points.first, info.exit_points.last, node);
      info.exit_points.count += 1;
    }
  }
  scratch_end(scratch);
  return info;
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
    dasm_shared->stripes[idx].rw_mutex = rw_mutex_alloc();
    dasm_shared->stripes[idx].cv = cond_var_alloc();
  }
  dasm_shared->u2p_ring_size = KB(64);
  dasm_shared->u2p_ring_base = push_array_no_zero(arena, U8, dasm_shared->u2p_ring_size);
  dasm_shared->u2p_ring_cv = cond_var_alloc();
  dasm_shared->u2p_ring_mutex = mutex_alloc();
  dasm_shared->evictor_detector_thread = thread_launch(dasm_evictor_detector_thread__entry_point, 0);
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
    MutexScopeR(stripe->rw_mutex)
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
  ins_atomic_u64_eval_assign(&node->last_user_clock_idx_touched, update_tick_idx());
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
    //- rjf: unpack hash
    U64 slot_idx = hash.u64[1]%dasm_shared->slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
    DASM_Slot *slot = &dasm_shared->slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
    
    //- rjf: try to get existing results
    B32 found = 0;
    MutexScopeR(stripe->rw_mutex)
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
    
    //- rjf: miss -> kick off work to fill cache
    if(!found)
    {
      B32 node_is_new = 0;
      U64 *node_working_count = 0;
      HS_Root root = {0};
      MutexScopeW(stripe->rw_mutex)
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
          // rjf: allocate node
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
          
          // rjf: fill node
          DLLPushBack(slot->first, slot->last, node);
          node->hash = hash;
          MemoryCopyStruct(&node->params, params);
          node->root = hs_root_alloc();
          // TODO(rjf): need to make this releasable - currently all exe_paths just leak
          node->params.dbgi_key = di_key_copy(stripe->arena, &node->params.dbgi_key);
          
          // rjf: gather work kickoff params
          node_is_new = 1;
          ins_atomic_u64_inc_eval(&node->working_count);
          node_working_count = &node->working_count;
          root = node->root;
        }
      }
      if(node_is_new)
      {
        dasm_u2p_enqueue_req(root, hash, params, max_U64);
        async_push_work(dasm_parse_work, .working_counter = node_working_count);
      }
    }
  }
  return info;
}

internal DASM_Info
dasm_info_from_key_params(DASM_Scope *scope, HS_Key key, DASM_Params *params, U128 *hash_out)
{
  DASM_Info result = {0};
  for(U64 rewind_idx = 0; rewind_idx < HS_KEY_HASH_HISTORY_COUNT; rewind_idx += 1)
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
//~ rjf: Ticks

#if 0
internal void
dasm_tick(void)
{
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  DI_Scope *di_scope = di_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  
  //- rjf: gather all requests
  local_persist DASM_Request *reqs = 0;
  local_persist U64 reqs_count = 0;
  if(lane_idx() == 0) MutexScope(dasm_shared->req_mutex)
  {
    reqs_count = dasm_shared->req_count;
    reqs = push_array(scratch.arena, DASM_Request, reqs_count);
    U64 idx = 0;
    for EachNode(r, DASM_RequestNode, dasm_shared->first_req)
    {
      MemoryCopyStruct(&reqs[idx], &r->v);
      reqs[idx].params.dbgi_key = di_key_copy(scratch.arena, &reqs[idx].params.dbgi_key);
      idx += 1;
    }
    arena_clear(dasm_shared->req_arena);
    dasm_shared->first_req = dasm_shared->last_req = 0;
    dasm_shared->req_count = 0;
  }
  lane_sync();
  
  //- rjf: do requests
  Rng1U64 range = lane_range(reqs_count);
  for EachInRange(req_idx, range)
  {
    //- rjf: unpack
    DASM_Request *r = &reqs[req_idx];
    HS_Root root = r->root;
    U128 hash = r->hash;
    DASM_Params params = r->params;
    String8 data = hs_data_from_hash(hs_scope, hash);
    U64 change_gen = fs_change_gen();
    U64 slot_idx = hash.u64[1]%dasm_shared->slots_count;
    U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
    DASM_Slot *slot = &dasm_shared->slots[slot_idx];
    DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
    
    //- rjf: get dbg info
    RDI_Parsed *rdi = &rdi_parsed_nil;
    if(params.dbgi_key.path.size != 0)
    {
      rdi = di_rdi_from_key(di_scope, &params.dbgi_key, 1, 0);
    }
    
    //- rjf: data * arch * addr * dbg -> decode artifacts
    DASM_LineChunkList line_list = {0};
    String8List inst_strings = {0};
    switch(params.arch)
    {
      default:{}break;
      
      //- rjf: x86/x64 decoding
      case Arch_x64:
      case Arch_x86:
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
          if(params.style_flags & (DASM_StyleFlag_SourceFilesNames|DASM_StyleFlag_SourceLines) &&
             rdi != &rdi_parsed_nil)
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
                  HS_Key key = fs_key_from_path_range(file_normalized_full_path, r1u64(0, max_U64), 0);
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
          
          // rjf: push line
          String8 addr_part = {0};
          if(params.style_flags & DASM_StyleFlag_Addresses)
          {
            addr_part = push_str8f(scratch.arena, "%s0x%016I64x  ", rdi != &rdi_parsed_nil ? "  " : "", params.vaddr+off);
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
          if(inst.jump_dest_vaddr != 0 && rdi != &rdi_parsed_nil && params.style_flags & DASM_StyleFlag_SymbolNames)
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
    
    //- rjf: artifacts -> value bundle
    Arena *info_arena = 0;
    DASM_Info info = {0};
    {
      //- rjf: produce joined text
      Arena *text_arena = arena_alloc();
      StringJoin text_join = {0};
      text_join.sep = str8_lit("\n");
      String8 text = str8_list_join(text_arena, &inst_strings, &text_join);
      
      //- rjf: produce unique key for this disassembly's text
      HS_Key text_key = hs_key_make(root, hs_id_make(0, 0));
      
      //- rjf: submit text data to hash store
      U128 text_hash = hs_submit_data(text_key, &text_arena, text);
      
      //- rjf: produce value bundle
      info_arena = arena_alloc();
      info.text_key = text_key;
      info.lines = dasm_line_array_from_chunk_list(info_arena, &line_list);
    }
    
    //- rjf: commit results to cache
    RWMutexScope(stripe->rw_mutex, 1)
    {
      for(DASM_Node *n = slot->first; n != 0; n = n->next)
      {
        if(u128_match(n->hash, hash) && dasm_params_match(&n->params, &params))
        {
          n->info_arena = info_arena;
          MemoryCopyStruct(&n->info, &info);
          if(rdi != &rdi_parsed_nil && params.style_flags & (DASM_StyleFlag_SourceLines|DASM_StyleFlag_SourceFilesNames))
          {
            n->change_gen = change_gen;
          }
          else
          {
            n->change_gen = 0;
          }
          break;
        }
      }
    }
  }
  
  txt_scope_close(txt_scope);
  di_scope_close(di_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
}
#endif

////////////////////////////////
//~ rjf: Parse Threads

internal B32
dasm_u2p_enqueue_req(HS_Root root, U128 hash, DASM_Params *params, U64 endt_us)
{
  B32 good = 0;
  MutexScope(dasm_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = dasm_shared->u2p_ring_write_pos - dasm_shared->u2p_ring_read_pos;
    U64 available_size = dasm_shared->u2p_ring_size - unconsumed_size;
    if(available_size >= sizeof(root)+sizeof(hash)+sizeof(U64)+sizeof(Arch)+sizeof(DASM_StyleFlags)+sizeof(DASM_Syntax)+sizeof(U64)+sizeof(U64)+params->dbgi_key.path.size+sizeof(U64))
    {
      good = 1;
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &root);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &hash);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->vaddr);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->arch);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->style_flags);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->syntax);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->base_vaddr);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->dbgi_key.path.size);
      dasm_shared->u2p_ring_write_pos += ring_write(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, params->dbgi_key.path.str, params->dbgi_key.path.size);
      dasm_shared->u2p_ring_write_pos += ring_write_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_write_pos, &params->dbgi_key.min_timestamp);
      break;
    }
    if(os_now_microseconds() >= endt_us)
    {
      break;
    }
    cond_var_wait(dasm_shared->u2p_ring_cv, dasm_shared->u2p_ring_mutex, endt_us);
  }
  if(good)
  {
    cond_var_broadcast(dasm_shared->u2p_ring_cv);
  }
  return good;
}

internal void
dasm_u2p_dequeue_req(Arena *arena, HS_Root *root_out, U128 *hash_out, DASM_Params *params_out)
{
  MutexScope(dasm_shared->u2p_ring_mutex) for(;;)
  {
    U64 unconsumed_size = dasm_shared->u2p_ring_write_pos - dasm_shared->u2p_ring_read_pos;
    if(unconsumed_size >= sizeof(*hash_out)+sizeof(U64)+sizeof(Arch)+sizeof(DASM_StyleFlags)+sizeof(DASM_Syntax)+sizeof(U64)+sizeof(U64)+sizeof(U64))
    {
      dasm_shared->u2p_ring_read_pos += ring_read_struct(dasm_shared->u2p_ring_base, dasm_shared->u2p_ring_size, dasm_shared->u2p_ring_read_pos, root_out);
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
      break;
    }
    cond_var_wait(dasm_shared->u2p_ring_cv, dasm_shared->u2p_ring_mutex, max_U64);
  }
  cond_var_broadcast(dasm_shared->u2p_ring_cv);
}

ASYNC_WORK_DEF(dasm_parse_work)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  HS_Scope *hs_scope = hs_scope_open();
  DI_Scope *di_scope = di_scope_open();
  TXT_Scope *txt_scope = txt_scope_open();
  
  //- rjf: get next request
  HS_Root root = {0};
  U128 hash = {0};
  DASM_Params params = {0};
  dasm_u2p_dequeue_req(scratch.arena, &root, &hash, &params);
  U64 change_gen = fs_change_gen();
  
  //- rjf: unpack hash
  U64 slot_idx = hash.u64[1]%dasm_shared->slots_count;
  U64 stripe_idx = slot_idx%dasm_shared->stripes_count;
  DASM_Slot *slot = &dasm_shared->slots[slot_idx];
  DASM_Stripe *stripe = &dasm_shared->stripes[stripe_idx];
  
  //- rjf: get dbg info
  RDI_Parsed *rdi = &rdi_parsed_nil;
  if(params.dbgi_key.path.size != 0)
  {
    rdi = di_rdi_from_key(di_scope, &params.dbgi_key, 1, max_U64);
  }
  
  //- rjf: hash -> data
  String8 data = hs_data_from_hash(hs_scope, hash);
  
  //- rjf: data * arch * addr * dbg -> decode artifacts
  DASM_LineChunkList line_list = {0};
  String8List inst_strings = {0};
  {
    switch(params.arch)
    {
      default:{}break;
      
      //- rjf: x86/x64 decoding
      case Arch_x64:
      case Arch_x86:
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
            if(rdi != &rdi_parsed_nil)
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
                    HS_Key key = fs_key_from_path_range(file_normalized_full_path, r1u64(0, max_U64), 0);
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
            addr_part = push_str8f(scratch.arena, "%s0x%016I64x  ", rdi != &rdi_parsed_nil ? "  " : "", params.vaddr+off);
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
          if(inst.jump_dest_vaddr != 0 && rdi != &rdi_parsed_nil && params.style_flags & DASM_StyleFlag_SymbolNames)
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
  {
    //- rjf: produce joined text
    Arena *text_arena = arena_alloc();
    StringJoin text_join = {0};
    text_join.sep = str8_lit("\n");
    String8 text = str8_list_join(text_arena, &inst_strings, &text_join);
    
    //- rjf: produce unique key for this disassembly's text
    HS_Key text_key = hs_key_make(root, hs_id_make(0, 0));
    
    //- rjf: submit text data to hash store
    U128 text_hash = hs_submit_data(text_key, &text_arena, text);
    
    //- rjf: produce value bundle
    info_arena = arena_alloc();
    info.text_key = text_key;
    info.lines = dasm_line_array_from_chunk_list(info_arena, &line_list);
  }
  
  //- rjf: commit results to cache
  MutexScopeW(stripe->rw_mutex)
  {
    for(DASM_Node *n = slot->first; n != 0; n = n->next)
    {
      if(u128_match(n->hash, hash) && dasm_params_match(&n->params, &params))
      {
        n->info_arena = info_arena;
        MemoryCopyStruct(&n->info, &info);
        if(rdi != &rdi_parsed_nil && params.style_flags & (DASM_StyleFlag_SourceLines|DASM_StyleFlag_SourceFilesNames))
        {
          n->change_gen = change_gen;
        }
        else
        {
          n->change_gen = 0;
        }
        break;
      }
    }
  }
  
  txt_scope_close(txt_scope);
  di_scope_close(di_scope);
  hs_scope_close(hs_scope);
  scratch_end(scratch);
  ProfEnd();
  return 0;
}

////////////////////////////////
//~ rjf: Evictor/Detector Thread

internal void
dasm_evictor_detector_thread__entry_point(void *p)
{
  ThreadNameF("dasm_evictor_detector_thread");
  for(;;)
  {
    U64 change_gen = fs_change_gen();
    U64 check_time_us = os_now_microseconds();
    U64 check_time_user_clocks = update_tick_idx();
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
      MutexScopeR(stripe->rw_mutex)
      {
        for(DASM_Node *n = slot->first; n != 0; n = n->next)
        {
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             ins_atomic_u64_eval(&n->working_count) == 0)
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
      if(slot_has_work) MutexScopeW(stripe->rw_mutex)
      {
        for(DASM_Node *n = slot->first, *next = 0; n != 0; n = next)
        {
          next = n->next;
          if(n->scope_ref_count == 0 &&
             n->last_time_touched_us+evict_threshold_us <= check_time_us &&
             n->last_user_clock_idx_touched+evict_threshold_user_clocks <= check_time_user_clocks &&
             ins_atomic_u64_eval(&n->working_count) == 0)
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
            if(dasm_u2p_enqueue_req(n->root, n->hash, &n->params, max_U64))
            {
              async_push_work(dasm_parse_work);
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
