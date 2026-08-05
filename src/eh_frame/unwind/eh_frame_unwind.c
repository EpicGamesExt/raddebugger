// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal UWND_StepResult
eh_uwnd_step(Arch arch, MemoryMap *memory_map, UWND_ModuleInfo *module_info, U64 tls_vaddr, void *regs, U64 *cfa_out)
{
  Temp scratch = scratch_begin(0, 0);
  B32 done = 0;
  UWND_StepResult result = {0};
  EH_UWND_ModuleUnwindInfo *unwind_info = (EH_UWND_ModuleUnwindInfo *)module_info->unwind_info;
  if(unwind_info != 0)
  {
    //- rjf: unpack context
    EH_FrameHdr *header = &unwind_info->header;
    EH_PtrCtx *ptr_ctx = &unwind_info->ptr_ctx;
    ARCH_Info *arch_info = arch_info_from_arch(arch);
    U64 pc = arch_ip_from_reg_block(arch_info, regs);
    
    //- rjf: find nearest FDE entry to the IP
    U64 fde_vaddr = 0;
    if(header->version == 1 && header->fde_count != 0)
    {
      // rjf: binary search to find FDE number
      U64 fde_num = 0;
      {
        U64 min_idx = 0;
        U64 max_idx = header->fde_count-1;
        for(;min_idx <= max_idx;)
        {
          U64 mid_idx = min_idx + (max_idx - min_idx) / 2;
          U64 mid_pc_off = mid_idx*header->entry_byte_size;
          U64 mid_pc = 0;
          eh_read_ptr(header->table, mid_pc_off, ptr_ctx->pc_vaddr + mid_pc_off, ptr_ctx, header->table_enc, &mid_pc);
          if(mid_pc > pc)
          {
            max_idx = mid_idx - 1;
          }
          else if(mid_pc < pc)
          {
            min_idx = mid_idx + 1;
          }
          else
          {
            fde_num = mid_idx + 1;
            break;
          }
        }
        if(fde_num == 0)
        {
          fde_num = min_idx > 0 ? min_idx : 1;
        }
      }
      
      // rjf: FDE number -> FDE vaddr
      if(0 < fde_num && fde_num <= header->fde_count)
      {
        U64 fde_vaddr_off = ((fde_num-1) * header->entry_byte_size) + header->field_byte_size;
        eh_read_ptr(header->table, fde_vaddr_off, ptr_ctx->pc_vaddr + fde_vaddr_off, ptr_ctx, header->table_enc, &fde_vaddr);
      }
    }
    
    //- rjf: parse info from FDE entry
    DW_CIE cie = {0};
    DW_FDE fde = {0};
    U64 cie_inst_data_off = 0;
    U64 fde_inst_data_off = 0;
    String8 cie_inst_data = {0};
    String8 fde_inst_data = {0};
    if(fde_vaddr != 0)
    {
      // rjf: determine FDE's full address range
      Rng1U64 fde_vaddr_range = {0};
      DW_Format fde_fmt = DW_Format_Null;
      {
        // rjf: read initial length data
        String8 initial_length_data = {0};
        {
          U8 *initial_length_data_buffer = push_array(scratch.arena, U8, 12);
          Rng1U64 initial_length_vaddr_range = r1u64(fde_vaddr, fde_vaddr+12);
          U64 initial_length_data_size = memory_map_read(memory_map, initial_length_vaddr_range, initial_length_data_buffer);
          initial_length_data = str8(initial_length_data_buffer, initial_length_data_size);
          if(initial_length_data_size < 4)
          {
            done = 1;
            result.status = UWND_StepStatus_FailedMemoryRead;
            result.missed_read_vaddr_range = initial_length_vaddr_range;
          }
        }
        
        // rjf: compute initial length
        U64 length = 0;
        if(!done)
        {
          dw2_read_initial_length(initial_length_data, 0, &length, &fde_fmt);
        }
        
        // rjf: determine vaddr range of all FDE data
        //
        // TODO(rjf): are we sure this is supposed to *not* advance past the 8 byte length field
        // when the initial 4 bytes are max_U32? this would mean that in 32-bit format, the length
        // *does not* include the length encoding itself, but in 64-bit format, the length *does*
        // include the length, but not the initial 4 bytes max_U32 marker. WTF?
        //
        fde_vaddr_range = r1u64(fde_vaddr, fde_vaddr + 4 + length);
      }
      
      // rjf: read FDE data
      String8 fde_data = {0};
      if(!done)
      {
        U64 fde_data_expected_size = dim_1u64(fde_vaddr_range);
        U8 *fde_data_buffer = push_array(scratch.arena, U8, fde_data_expected_size);
        U64 fde_data_size = memory_map_read(memory_map, fde_vaddr_range, fde_data_buffer);
        if(fde_data_size != fde_data_expected_size)
        {
          done = 1;
          result.status = UWND_StepStatus_FailedMemoryRead;
          result.missed_read_vaddr_range = fde_vaddr_range;
        }
        fde_data = str8(fde_data_buffer, fde_data_size);
      }
      
      // rjf: FDE data -> CIE address
      U64 cie_vaddr = 0;
      if(!done)
      {
        U64 cie_delta_off = (fde_fmt == DW_Format_32Bit ? 4 : 12);
        U64 cie_delta = 0;
        dw2_read_fmt_u64(fde_data, cie_delta_off, fde_fmt, &cie_delta);
        cie_vaddr = (fde_vaddr + cie_delta_off) - cie_delta;
      }
      
      // rjf: determine CIE's full address range
      Rng1U64 cie_vaddr_range = {0};
      DW_Format cie_fmt = DW_Format_Null;
      if(!done)
      {
        // rjf: read initial length data
        String8 initial_length_data = {0};
        {
          U8 *initial_length_data_buffer = push_array(scratch.arena, U8, 12);
          Rng1U64 initial_length_vaddr_range = r1u64(cie_vaddr, cie_vaddr+12);
          U64 initial_length_data_size = memory_map_read(memory_map, initial_length_vaddr_range, initial_length_data_buffer);
          initial_length_data = str8(initial_length_data_buffer, initial_length_data_size);
          if(initial_length_data_size < 4)
          {
            done = 1;
            result.status = UWND_StepStatus_FailedMemoryRead;
            result.missed_read_vaddr_range = initial_length_vaddr_range;
          }
        }
        
        // rjf: compute initial length
        U64 length = 0;
        if(!done)
        {
          dw2_read_initial_length(initial_length_data, 0, &length, &cie_fmt);
        }
        
        // rjf: form full address range of CIE data
        //
        // TODO(rjf): is this busted? see above note in FDE parse
        //
        cie_vaddr_range = r1u64(cie_vaddr, cie_vaddr + 4 + length);
      }
      
      // rjf: read CIE data
      String8 cie_data = {0};
      if(!done)
      {
        U64 cie_data_expected_size = dim_1u64(cie_vaddr_range);
        U8 *cie_data_buffer = push_array(scratch.arena, U8, cie_data_expected_size);
        U64 cie_data_size = memory_map_read(memory_map, cie_vaddr_range, cie_data_buffer);
        if(cie_data_size != cie_data_expected_size)
        {
          done = 1;
          result.status = UWND_StepStatus_FailedMemoryRead;
          result.missed_read_vaddr_range = cie_vaddr_range;
        }
        cie_data = str8(cie_data_buffer, cie_data_size);
      }
      
      // rjf: parse CIE data, & FDE data
      cie_inst_data_off = eh_read_cie(cie_data, 0, cie_fmt, arch, cie_vaddr_range.min, ptr_ctx, &cie);
      fde_inst_data_off = eh_read_fde(fde_data, 0, fde_fmt, arch, fde_vaddr_range.min, ptr_ctx, &cie, &fde);
      cie_inst_data = str8_skip(cie_data, cie_inst_data_off);
      fde_inst_data = str8_skip(fde_data, fde_inst_data_off);
    }
    
    //- rjf: parse CIE/FDE info, produce CFI row for this PC
    U64 reg_count = dw_reg_count_from_arch(arch);
    DW_CFIRow cfi_row = {0};
    cfi_row.reg_rules = push_array(scratch.arena, DW_UnwindRule, reg_count);
    if(!done && contains_1u64(fde.pc_range, pc))
    {
      U64 code_align_factor = cie.code_align_factor;
      S64 data_align_factor = cie.data_align_factor;
      DW_CFIRow initial_row = {0};
      initial_row.reg_rules = push_array(scratch.arena, DW_UnwindRule, reg_count);
      DW_CFIRowNode *top_row = 0;
      DW_CFIRowNode *free_row = 0;
      struct
      {
        String8 inst_data;
        U64 inst_data_off;
      }
      program_tasks[] =
      {
        {cie_inst_data, cie_inst_data_off},
        {fde_inst_data, fde_inst_data_off},
      };
      B32 computed_row = 0;
      for EachElement(program_task_idx, program_tasks)
      {
        // rjf: exit if we computed the target row
        if(computed_row)
        {
          break;
        }
        
        // rjf: run unwind instruction program
        String8 inst_data = program_tasks[program_task_idx].inst_data;
        U64 inst_data_off = program_tasks[program_task_idx].inst_data_off;
        for(U64 off = 0; off < inst_data.size;)
        {
          U64 start_off = off;
          
          // rjf: read next opcode
          DW_CFAOpCode raw_opcode = DW_CFAOpCode_Nop;
          off += str8_deserial_read_struct(inst_data, off, &raw_opcode);
          
          // rjf: unpack opcode (raw opcode can contain implicit operands)
          DW_CFAOpCode opcode = raw_opcode & ~DW_CFAOpCodeMask_OpcodeHi;
          U64 implicit_operand = 0;
          if((raw_opcode & DW_CFAOpCodeMask_OpcodeHi) != 0)
          {
            opcode = (raw_opcode & DW_CFAOpCodeMask_OpcodeHi);
            implicit_operand = raw_opcode & DW_CFAOpCodeMask_Operand;
          }
          
          // rjf: apply opcode
          U64 new_base_pc_off = cfi_row.base_pc_off;
          switch(opcode)
          {
            default:{}break;
            case DW_CFAOpCode_Nop:{}break;
            
            //- rjf: location adjustments
            case DW_CFAOpCode_SetLoc:
            {
              off += eh_read_ptr(inst_data, off, ptr_ctx->pc_vaddr + inst_data_off + off, ptr_ctx, header->table_enc, &new_base_pc_off);
            }break;
            case DW_CFAOpCode_AdvanceLoc:
            {
              new_base_pc_off += implicit_operand * code_align_factor;
            }break;
            case DW_CFAOpCode_AdvanceLoc1:
            {
              U8 delta = 0;
              off += str8_deserial_read_struct(inst_data, off, &delta);
              new_base_pc_off += delta;
            }break;
            case DW_CFAOpCode_AdvanceLoc2:
            {
              U16 delta = 0;
              off += str8_deserial_read_struct(inst_data, off, &delta);
              new_base_pc_off += delta;
            }break;
            case DW_CFAOpCode_AdvanceLoc4:
            {
              U32 delta = 0;
              off += str8_deserial_read_struct(inst_data, off, &delta);
              new_base_pc_off += delta;
            }break;
            
            //- rjf: row operations
            case DW_CFAOpCode_RememberState:
            {
              // rjf: alloc row node
              DW_CFIRowNode *n = free_row;
              if(n != 0)
              {
                SLLStackPop(free_row);
              }
              else
              {
                n = push_array(scratch.arena, DW_CFIRowNode, 1);
                n->v.reg_rules = push_array(scratch.arena, DW_UnwindRule, reg_count);
              }
              
              // rjf: copy current state
              n->v.base_pc_off = cfi_row.base_pc_off;
              n->v.cfa_rule = cfi_row.cfa_rule;
              MemoryCopy(n->v.reg_rules, cfi_row.reg_rules, sizeof(cfi_row.reg_rules[0])*reg_count);
              
              // rjf: push
              SLLStackPush(top_row, n);
            }break;
            case DW_CFAOpCode_RestoreState:
            if(top_row != 0)
            {
              // rjf: pop row
              DW_CFIRowNode *n = top_row;
              SLLStackPop(top_row);
              
              // rjf: copy to current state
              cfi_row.base_pc_off = n->v.base_pc_off;
              cfi_row.cfa_rule = n->v.cfa_rule;
              MemoryCopy(cfi_row.reg_rules, n->v.reg_rules, sizeof(cfi_row.reg_rules[0])*reg_count);
              
              // rjf: free row
              SLLStackPush(free_row, n);
            }break;
            
            //- rjf: register rules
            case DW_CFAOpCode_OffsetExt:
            {
              U64 reg = 0;
              U64 reg_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_uleb128(inst_data, off, &reg_off);
              reg_off *= data_align_factor;
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_Off;
                cfi_row.reg_rules[reg].s64 = (S64)reg_off;
              }
            }break;
            case DW_CFAOpCode_OffsetExtSf:
            {
              U64 reg = 0;
              S64 reg_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_sleb128(inst_data, off, &reg_off);
              reg_off *= data_align_factor;
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_Off;
                cfi_row.reg_rules[reg].s64 = reg_off;
              }
            }break;
            case DW_CFAOpCode_Undefined:
            {
              U64 reg = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_Undefined;
              }
            }break;
            case DW_CFAOpCode_SameValue:
            {
              U64 reg = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_SameVal;
              }
            }break;
            case DW_CFAOpCode_Register:
            {
              U64 dst_reg = 0;
              U64 src_reg = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &dst_reg);
              off += str8_deserial_read_uleb128(inst_data, off, &src_reg);
              if(dst_reg < reg_count)
              {
                cfi_row.reg_rules[dst_reg].code = DW_UnwindRuleCode_Reg;
                cfi_row.reg_rules[dst_reg].reg_code = src_reg;
              }
            }break;
            case DW_CFAOpCode_Expr:
            {
              U64 reg = 0;
              U64 expr_size = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_uleb128(inst_data, off, &expr_size);
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_Expr;
                cfi_row.reg_rules[reg].expr = str8_substr(inst_data, r1u64(off, off+expr_size));
              }
            }break;
            case DW_CFAOpCode_ValOffset:
            {
              U64 reg = 0;
              U64 val_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_uleb128(inst_data, off, &val_off);
              val_off *= data_align_factor;
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_ValOff;
                cfi_row.reg_rules[reg].s64 = (S64)val_off;
              }
            }break;
            case DW_CFAOpCode_ValOffsetSf:
            {
              U64 reg = 0;
              S64 val_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_sleb128(inst_data, off, &val_off);
              val_off *= data_align_factor;
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_ValOff;
                cfi_row.reg_rules[reg].s64 = val_off;
              }
            }break;
            case DW_CFAOpCode_ValExpr:
            {
              U64 reg = 0;
              U64 expr_size = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_uleb128(inst_data, off, &expr_size);
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_ValExpr;
                cfi_row.reg_rules[reg].expr = str8_substr(inst_data, r1u64(off, off+expr_size));
              }
            }break;
            case DW_CFAOpCode_Offset:
            {
              U64 offset = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &offset);
              offset *= data_align_factor;
              U64 reg = implicit_operand;
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg].code = DW_UnwindRuleCode_Off;
                cfi_row.reg_rules[reg].s64 = (S64)offset;
              }
            }break;
            
            //- rjf: CFA rules
            case DW_CFAOpCode_DefCfa:
            {
              U64 reg = 0;
              U64 reg_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_uleb128(inst_data, off, &reg_off);
              cfi_row.cfa_rule.code = DW_UnwindRuleCode_Reg;
              cfi_row.cfa_rule.reg_code = reg;
              cfi_row.cfa_rule.s64 = (S64)reg_off;
            }break;
            case DW_CFAOpCode_DefCfaSf:
            {
              U64 reg = 0;
              S64 reg_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              off += str8_deserial_read_sleb128(inst_data, off, &reg_off);
              cfi_row.cfa_rule.code = DW_UnwindRuleCode_Reg;
              cfi_row.cfa_rule.reg_code = reg;
              cfi_row.cfa_rule.s64 = (S64)(reg_off * data_align_factor);
            }break;
            case DW_CFAOpCode_DefCfaRegister:
            {
              U64 reg = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              cfi_row.cfa_rule.code = DW_UnwindRuleCode_Reg;
              cfi_row.cfa_rule.reg_code = reg;
            }break;
            case DW_CFAOpCode_DefCfaOffset:
            {
              U64 reg_off = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg_off);
              cfi_row.cfa_rule.s64 = (S64)reg_off;
            }break;
            case DW_CFAOpCode_DefCfaOffsetSf:
            {
              S64 reg_off = 0;
              off += str8_deserial_read_sleb128(inst_data, off, &reg_off);
              cfi_row.cfa_rule.s64 = (S64)reg_off;
            }break;
            case DW_CFAOpCode_DefCfaExpr:
            {
              U64 expr_size = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &expr_size);
              cfi_row.cfa_rule.code = DW_UnwindRuleCode_Expr;
              cfi_row.cfa_rule.expr = str8_substr(inst_data, r1u64(off, off+expr_size));
            }break;
            
            //- rjf: register state resets
            case DW_CFAOpCode_Restore:
            {
              U64 reg = implicit_operand;
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg] = initial_row.reg_rules[reg];
              }
            }break;
            case DW_CFAOpCode_RestoreExt:
            {
              U64 reg = 0;
              off += str8_deserial_read_uleb128(inst_data, off, &reg);
              if(reg < reg_count)
              {
                cfi_row.reg_rules[reg] = initial_row.reg_rules[reg];
              }
            }break;
          }
          
          // rjf: exit early if we found the CFI row for this pc
          {
            U64 pc_off = (pc - fde.pc_range.min);
            if(cfi_row.base_pc_off <= pc_off && pc_off < new_base_pc_off)
            {
              computed_row = 1;
              break;
            }
          }
          
          // rjf: commit changes to CFI row base pc
          cfi_row.base_pc_off = new_base_pc_off;
          
          // rjf: abort if we made no parsing progress
          if(off == start_off)
          {
            break;
          }
        }
        
        // rjf: on cie program -> save current state as 'initial state', to be
        // potentially restored by fde
        {
          initial_row.base_pc_off = cfi_row.base_pc_off;
          initial_row.cfa_rule = cfi_row.cfa_rule;
          MemoryCopy(initial_row.reg_rules, cfi_row.reg_rules, sizeof(initial_row.reg_rules[0])*reg_count);
        }
      }
    }
    
    //- rjf: evaluate frame base vaddr (CFA) via CFI row rule
    U64 cfa = 0;
    if(!done) switch(cfi_row.cfa_rule.code)
    {
      default:{}break;
      case DW_UnwindRuleCode_Reg:
      {
        U64 cfa_reg_val = 0;
        DW_RegCode dw_reg_code = cfi_row.cfa_rule.reg_code;
        ARCH_RegCode reg_code = arch_reg_code_from_dw(arch, dw_reg_code);
        arch_reg_block_read_range(arch_info, regs, arch_info->reg_code_rng_table[reg_code], &cfa_reg_val);
        cfa = cfa_reg_val + cfi_row.cfa_rule.s64;
      }break;
      case DW_UnwindRuleCode_Expr:
      {
        String8 expr = cfi_row.cfa_rule.expr;
        DW_EvalState eval_state = {0};
        DW_Eval eval = dw_eval(scratch.arena, cie.format, arch, memory_map, regs, 0, 0, tls_vaddr, &eval_state, expr, 1000);
        if(eval.status == DW_EvalStatus_FailedMemoryRead)
        {
          done = 1;
          result.status = UWND_StepStatus_FailedMemoryRead;
          result.missed_read_vaddr_range = eval.missed_read_vaddr_range;
        }
        else
        {
          cfa = eval.val.u512.u64[0]; 
        }
      }break;
    }
    
    //- rjf: apply CFI row rules to registers
    if(!done) for EachIndex(reg_idx, reg_count)
    {
      DW_UnwindRule *rule = &cfi_row.reg_rules[reg_idx];
      ARCH_RegCode reg_code = arch_reg_code_from_dw(arch, reg_idx);
      Rng1U16 reg_rng = arch_info->reg_code_rng_table[reg_code];
      U64 reg_size = dim_1u16(reg_rng);
      switch(rule->code)
      {
        default:
        case DW_UnwindRuleCode_SameVal:{}break;
        case DW_UnwindRuleCode_Undefined:
        {
          MemoryZero((U8 *)regs + reg_rng.min, reg_size);
        }break;
        case DW_UnwindRuleCode_Off:
        {
          Temp temp = temp_begin(scratch.arena);
          U64 addr = cfa + rule->s64;
          Rng1U64 reg_value_vaddr_range = r1u64(addr, addr+reg_size);
          U8 *reg_value_buffer = push_array(temp.arena, U8, reg_size);
          if(memory_map_read(memory_map, reg_value_vaddr_range, reg_value_buffer) != reg_size)
          {
            done = 1;
            result.status = UWND_StepStatus_FailedMemoryRead;
            result.missed_read_vaddr_range = reg_value_vaddr_range;
          }
          if(!done)
          {
            arch_reg_block_write_range(arch_info, regs, reg_rng, reg_value_buffer);
          }
          temp_end(temp);
        }break;
        case DW_UnwindRuleCode_ValOff:
        {
          U64 reg_value = cfa + rule->s64;
          Rng1U16 write_range = r1u16(reg_rng.min, Min(reg_rng.max, reg_rng.min + sizeof(reg_value)));
          arch_reg_block_write_range(arch_info, regs, write_range, &reg_value);
        }break;
        case DW_UnwindRuleCode_Reg:
        {
          Temp temp = temp_begin(scratch.arena);
          DW_RegCode src_reg_code_dw = rule->reg_code;
          ARCH_RegCode src_reg_code = arch_reg_code_from_dw(arch, src_reg_code_dw);
          Rng1U16 src_reg_rng = arch_info->reg_code_rng_table[src_reg_code];
          U64 src_reg_size = dim_1u16(src_reg_rng);
          U64 read_size = Min(src_reg_size, reg_size);
          U8 *src_reg_val_buffer = push_array(temp.arena, U8, read_size);
          arch_reg_block_read_range(arch_info, regs, src_reg_rng, src_reg_val_buffer);
          arch_reg_block_write_range(arch_info, regs, reg_rng, src_reg_val_buffer);
          temp_end(temp);
        }break;
        case DW_UnwindRuleCode_Expr:
        {
          Temp temp = temp_begin(scratch.arena);
          String8 expr = rule->expr;
          DW_EvalState eval_state = {0};
          DW_Eval eval = dw_eval(scratch.arena, cie.format, arch, memory_map, regs, 0, cfa, tls_vaddr, &eval_state, expr, 1000);
          if(eval.status == DW_EvalStatus_FailedMemoryRead)
          {
            done = 1;
            result.status = UWND_StepStatus_FailedMemoryRead;
            result.missed_read_vaddr_range = eval.missed_read_vaddr_range;
          }
          if(!done)
          {
            U64 reg_val_vaddr = eval.val.u512.u64[0];
            Rng1U64 reg_value_vaddr_range = r1u64(reg_val_vaddr, reg_val_vaddr+reg_size);
            U8 *reg_value_buffer = push_array(temp.arena, U8, reg_size);
            if(memory_map_read(memory_map, reg_value_vaddr_range, reg_value_buffer) != reg_size)
            {
              done = 1;
              result.status = UWND_StepStatus_FailedMemoryRead;
              result.missed_read_vaddr_range = reg_value_vaddr_range;
            }
            else
            {
              arch_reg_block_write_range(arch_info, regs, reg_rng, reg_value_buffer);
            }
          }
          temp_end(temp);
        }break;
        case DW_UnwindRuleCode_ValExpr:
        {
          Temp temp = temp_begin(scratch.arena);
          String8 expr = rule->expr;
          DW_EvalState eval_state = {0};
          DW_Eval eval = dw_eval(scratch.arena, cie.format, arch, memory_map, regs, 0, cfa, tls_vaddr, &eval_state, expr, 1000);
          if(eval.status == DW_EvalStatus_FailedMemoryRead)
          {
            done = 1;
            result.status = UWND_StepStatus_FailedMemoryRead;
            result.missed_read_vaddr_range = eval.missed_read_vaddr_range;
          }
          if(!done)
          {
            U512 reg_val = eval.val.u512;
            arch_reg_block_write_range(arch_info, regs, reg_rng, &reg_val);
          }
          temp_end(temp);
        }break;
        case DW_UnwindRuleCode_Architectural:
        {
          // NOTE(rjf): the DWARF spec says that this code implies:
          //
          // "The rule is defined externally to this specification by the augmenter"
          //
          // so leaving this blank until we know exactly what that means.
        }break;
      }
    }
    
    //- TODO(rjf): old code was replacing the stack pointer with the CFA.
    // this is surely incorrect, no? isn't the entire point of the CFA
    // to be *not* necessarily the stack pointer? and wouldn't the stack
    // pointer be modified via the above rules? so what are we doing
    // here?
    //
    if(!done)
    {
      arch_reg_block_write_sp(arch_info, regs, cfa);
    }
    
    //- rjf: commit new register values, if we succeeded
    if(!done)
    {
      result.status = UWND_StepStatus_Good;
    }
  }
  scratch_end(scratch);
  return result;
}
