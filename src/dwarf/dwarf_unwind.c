// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U64 dw_based_range_read(void *base, Rng1U64 range, U64 off, U64 size, void *out) { return 0; }
internal U64 dw_based_range_read_uleb128(void *base, Rng1U64 range, U64 off, U64 *out)    { return 0; }
internal U64 dw_based_range_read_sleb128(void *base, Rng1U64 range, U64 off, S64 *out)    { return 0; }
internal U64 dw_based_range_read_length(void *base, Rng1U64 range, U64 off, U64 *out)     { return 0; }

////////////////////////////////
// x64 Unwind Function

internal DW_UnwindResult
dw_unwind_x64(String8           raw_text,
              String8           raw_eh_frame,
              String8           raw_eh_frame_hdr,
              Rng1U64           text_vrange,
              Rng1U64           eh_frame_vrange,
              Rng1U64           eh_frame_hdr_vrange,
              U64               default_image_base,
              U64               image_base,
              U64               stack_pointer,
              DW_RegsX64       *regs,
              DW_ReadMemorySig *read_memory,
              void             *read_memory_ud)
{
  // TODO: What if ELF has two sections with instructions and pointer is ecnoded relative to .text2?
  Temp scratch = scratch_begin(0, 0);
  
  DW_UnwindResult result = {0};
  
  dw_unwind_init_x64();
  
  // rebase
  U64 rebase_voff_to_vaddr = (image_base - default_image_base);
  
  // get ip register values
  U64 ip_value = regs->rip;
  U64 ip_voff  = ip_value - rebase_voff_to_vaddr;
  
  // check sections
  B32 has_needed_sections = (raw_text.size > 0 && raw_eh_frame.size > 0);
  if (!has_needed_sections) {
    result.is_invalid = 1;
  }
  
  //- get frame info range
  void    *frame_base  = raw_eh_frame.str;
  Rng1U64  frame_range = rng_1u64(0, raw_eh_frame.size);
  
  //- section vaddrs
  U64 text_base_vaddr = text_vrange.min + rebase_voff_to_vaddr;
  U64 frame_base_voff = text_vrange.min;
  U64 data_base_vaddr = eh_frame_hdr_vrange.min + rebase_voff_to_vaddr;
  
  //- find cfi records
  DW_CFIRecords cfi_recs = {0};
  if (has_needed_sections) {
    DW_EhPtrCtx ptr_ctx    = {0};
    ptr_ctx.raw_base_vaddr = frame_base_voff;
    ptr_ctx.text_vaddr     = text_base_vaddr;
    ptr_ctx.data_vaddr     = data_base_vaddr;
    ptr_ctx.func_vaddr     = 0;
    if (raw_eh_frame_hdr.size) {
      cfi_recs = dw_unwind_eh_frame_hdr_from_ip_fast_x64(raw_eh_frame, raw_eh_frame_hdr, &ptr_ctx, ip_voff);
    } else {
      cfi_recs = dw_unwind_eh_frame_cfi_from_ip_slow_x64(raw_eh_frame, &ptr_ctx, ip_voff);
    }
  }
  
  //- check cfi records
  if (!cfi_recs.valid) {
    result.is_invalid = 1;
  }
  
  //- cfi machine setup
  DW_CFIMachine machine = {0};
  if (cfi_recs.valid) {
    DW_EhPtrCtx ptr_ctx    = {0};
    ptr_ctx.raw_base_vaddr = frame_base_voff;
    ptr_ctx.text_vaddr     = text_base_vaddr;
    ptr_ctx.data_vaddr     = data_base_vaddr;
    ptr_ctx.func_vaddr     = cfi_recs.fde.ip_voff_range.min + rebase_voff_to_vaddr; // TODO: it's not super clear how to set up this member, need more test cases
    machine = dw_unwind_make_machine_x64(DW_UNWIND_X64__REG_SLOT_COUNT, &cfi_recs.cie, &ptr_ctx);
  }
  
  // initial row
  DW_CFIRow *init_row = 0;
  if (cfi_recs.valid) {
    Rng1U64    init_cfi_range = cfi_recs.cie.cfi_range;
    DW_CFIRow *row            = dw_unwind_row_alloc_x64(scratch.arena, machine.cells_per_row);
    if (dw_unwind_machine_run_to_ip_x64(frame_base, init_cfi_range, &machine, max_U64, row)) {
      init_row = row;
    }
    if (init_row == 0) {
      result.is_invalid = 1;
    }
  }
  
  // main row
  DW_CFIRow *main_row = 0;
  if (init_row != 0) {
    // upgrade machine with new equipment
    dw_unwind_machine_equip_initial_row_x64(&machine, init_row);
    dw_unwind_machine_equip_fde_ip_x64(&machine, cfi_recs.fde.ip_voff_range.min);
    
    // decode main row
    Rng1U64    main_cfi_range = cfi_recs.fde.cfi_range;
    DW_CFIRow *row            = dw_unwind_row_alloc_x64(scratch.arena, machine.cells_per_row);
    if (dw_unwind_machine_run_to_ip_x64(frame_base, main_cfi_range, &machine, ip_value, row)) {
      main_row = row;
    }
    if (main_row == 0) {
      result.is_invalid = 1;
    }
  }
  
  // apply main row to modify the registers
  if (main_row != 0) {
    result = dw_unwind_x64__apply_frame_rules(raw_eh_frame, main_row, text_base_vaddr, read_memory, read_memory_ud, stack_pointer, regs);
  }
  
  scratch_end(scratch);
  return result;
}

internal DW_UnwindResult
dw_unwind_x64__apply_frame_rules(String8           raw_eh_frame,
                                 DW_CFIRow        *row,
                                 U64               text_base_vaddr,
                                 DW_ReadMemorySig *read_memory,
                                 void             *read_memory_ud,
                                 U64               stack_pointer,
                                 DW_RegsX64       *regs)
{
  DW_UnwindResult result = {0};
  
  U64 missed_read_addr = 0;
  
  //- setup a dwarf expression machine
  DW_ExprMachineConfig dwexpr_config = {0};
  dwexpr_config.max_step_count       = 0xFFFF;
  dwexpr_config.read_memory          = read_memory;
  dwexpr_config.read_memory_ud       = read_memory_ud;
  dwexpr_config.regs                 = regs;
  dwexpr_config.text_section_base    = &text_base_vaddr;
  
  //- compute cfa
  U64 cfa = 0;
  switch (row->cfa_cell.rule) {
    case DW_CFI_CFA_Rule_RegOff: {
      // TODO: have we done anything to gaurantee reg_idx here?
      U64 reg_idx = row->cfa_cell.reg_idx;
      
      // is this a roll-over CFA?
      B32 is_roll_over_cfa = 0;
      if (reg_idx == DW_RegX64_Rsp) {
        DW_CFIRegisterRule rule = row->cells[reg_idx].rule;
        if (rule == DW_CFIRegisterRule_Undefined || rule == DW_CFIRegisterRule_SameValue) {
          is_roll_over_cfa = 1;
        }
      }
      
      // compute cfa
      if (is_roll_over_cfa) {
        cfa = stack_pointer + row->cfa_cell.offset;
      } else {
        cfa = regs->r[reg_idx] + row->cfa_cell.offset;
      }
    } break;
    
    case DW_CFI_CFA_Rule_Expr: {
      Rng1U64     expr_range = row->cfa_cell.expr;
      DW_Location location   = dw_expr__eval(0, raw_eh_frame.str, expr_range, &dwexpr_config);
      if (location.non_piece_loc.kind == DW_SimpleLocKind_Fail && location.non_piece_loc.fail_kind == DW_LocFailKind_MissingMemory) {
        missed_read_addr = location.non_piece_loc.fail_data;
        goto error_out;
      }
      if (location.non_piece_loc.kind == DW_SimpleLocKind_Address) {
        cfa = location.non_piece_loc.addr;
      }
    } break;
  }
  
  // compute registers
  {
    DW_CFICell *cell     = row->cells;
    DW_RegsX64  new_regs = {0};
    for (U64 i = 0; i < DW_UNWIND_X64__REG_SLOT_COUNT; ++i, ++cell) {
      // compute value
      U64 v = 0;
      switch (cell->rule) {
        default:
        {
          Assert(!"UNEXPECTED-RULE");
        } break;
        
        case DW_CFIRegisterRule_Undefined:
        {
          Assert(!"UNDEFINED");
        } break;
        
        case DW_CFIRegisterRule_SameValue:
        {
          v = regs->r[i];
        } break;
        
        case DW_CFIRegisterRule_Offset:
        {
          U64 addr = cfa + cell->n;
          U64 read_size = read_memory(addr, sizeof(v), &v, read_memory_ud);
          if (read_size != sizeof(v)) {
            missed_read_addr = addr;
            goto error_out;
          }
        } break;
        
        case DW_CFIRegisterRule_ValOffset:
        {
          v = cfa + cell->n;
        } break;
        
        case DW_CFIRegisterRule_Register:
        {
          v = regs->r[i];
        } break;
        
        case DW_CFIRegisterRule_Expression:
        {
          Rng1U64     expr_range = cell->expr;
          U64         addr       = 0;
          DW_Location location   = dw_expr__eval(0, raw_eh_frame.str, expr_range, &dwexpr_config);
          if (location.non_piece_loc.kind == DW_SimpleLocKind_Fail && location.non_piece_loc.fail_kind == DW_LocFailKind_MissingMemory) {
            missed_read_addr = location.non_piece_loc.fail_data;
            goto error_out;
          }
          if (location.non_piece_loc.kind == DW_SimpleLocKind_Address) {
            addr = location.non_piece_loc.addr;
          }
          U64 read_size = read_memory(addr, sizeof(v), &v, read_memory_ud);
          if (read_size != sizeof(v)) {
            missed_read_addr = addr;
            goto error_out;
          }
        } break;
        
        case DW_CFIRegisterRule_ValExpression:
        {
          Rng1U64     expr_range = cell->expr;
          DW_Location location   = dw_expr__eval(0, raw_eh_frame.str, expr_range, &dwexpr_config);
          if (location.non_piece_loc.kind == DW_SimpleLocKind_Fail && location.non_piece_loc.fail_kind == DW_LocFailKind_MissingMemory) {
            missed_read_addr = location.non_piece_loc.fail_data;
            goto error_out;
          }
          if (location.non_piece_loc.kind == DW_SimpleLocKind_Address) {
            v = location.non_piece_loc.addr;
          }
        } break;
      }
      
      // commit value to output slot
      new_regs.r[i] = v;
    }
    
    // commit all new regs
    MemoryCopy(regs, &new_regs, sizeof(new_regs));
  }
  
  //- save new stack pointer
  result.stack_pointer = cfa;
  
  error_out:;
  if (missed_read_addr) {
    result.is_invalid       = 1;
    result.missed_read      = 1;
    result.missed_read_addr = missed_read_addr;
  }
  
  return result;
}

////////////////////////////////
// Helper Functions

internal void
dw_unwind_init_x64(void)
{
  local_persist B32 did_init = 0;
  
  if (!did_init) {
    did_init = 1;
    
    // control bits tables
    dw_unwind__cfa_control_bits_kind1[DW_CFA_Nop            ] = 0x000;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_SetLoc         ] = 0x809;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_AdvanceLoc1    ] = 0x801;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_AdvanceLoc2    ] = 0x802;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_AdvanceLoc4    ] = 0x804;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_OffsetExt      ] = 0x2AA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_RestoreExt     ] = 0x20A;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_Undefined      ] = 0x20A;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_SameValue      ] = 0x20A;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_Register       ] = 0x6AA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_RememberState  ] = 0x000;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_RestoreState   ] = 0x000;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_DefCfa         ] = 0x2AA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_DefCfaRegister ] = 0x20A;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_DefCfaOffset   ] = 0x00A;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_DefCfaExpr     ] = 0x00A;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_Expr           ] = 0x2AA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_OffsetExtSf    ] = 0x2BA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_DefCfaSf       ] = 0x2BA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_DefCfaOffsetSf ] = 0x00B;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_ValOffset      ] = 0x2AA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_ValOffsetSf    ] = 0x2BA;
    dw_unwind__cfa_control_bits_kind1[DW_CFA_ValExpr        ] = 0x2AA;
    
    dw_unwind__cfa_control_bits_kind2[DW_CFA_AdvanceLoc  >> 6] = 0x800;
    dw_unwind__cfa_control_bits_kind2[DW_CFA_Offset      >> 6] = 0x10A;
    dw_unwind__cfa_control_bits_kind2[DW_CFA_Restore     >> 6] = 0x100;
  }
}

internal U64
dw_unwind_parse_pointer_x64(void *frame_base, Rng1U64 frame_range, DW_EhPtrCtx *ptr_ctx, DW_EhPtrEnc encoding, U64 off, U64 *ptr_out)
{
  // aligned offset
  U64 pointer_off = off;
  if (encoding == DW_EhPtrEnc_Aligned) {
    pointer_off = AlignPow2(off, 8); // TODO: align to 4 bytes when we parse x86 ELF binary
    encoding    = DW_EhPtrEnc_Ptr;
  }
  
  // decode pointer value
  U64 size_param        = 0;
  U64 after_pointer_off = 0;
  U64 raw_pointer       = 0;
  switch (encoding & DW_EhPtrEnc_TypeMask) {
    default:break;
    
    case DW_EhPtrEnc_Ptr   : size_param = 8; goto ufixed;
    case DW_EhPtrEnc_UData2: size_param = 2; goto ufixed;
    case DW_EhPtrEnc_UData4: size_param = 4; goto ufixed;
    case DW_EhPtrEnc_UData8: size_param = 8; goto ufixed;
    ufixed:
    {
      dw_based_range_read(frame_base, frame_range, pointer_off, size_param, &raw_pointer);
      after_pointer_off = pointer_off + size_param;
    } break;
    
    // TODO: Signed is actually just a flag that indicates this int is negavite.
    // There shouldn't be a read for Signed.
    // For instance, (DW_EhPtrEnc_UData2 | DW_EhPtrEnc_Signed) == DW_EhPtrEnc_SData etc.
    case DW_EhPtrEnc_Signed:size_param = 8; goto sfixed; 
    
    case DW_EhPtrEnc_SData2:size_param = 2; goto sfixed;
    case DW_EhPtrEnc_SData4:size_param = 4; goto sfixed;
    case DW_EhPtrEnc_SData8:size_param = 8; goto sfixed;
    sfixed:
    {
      dw_based_range_read(frame_base, frame_range, pointer_off, size_param, &raw_pointer);
      after_pointer_off = pointer_off + size_param;
      // sign extension
      U64 sign_bit = size_param*8 - 1;
      if ((raw_pointer >> sign_bit) != 0) {
        raw_pointer |= (~(1 << sign_bit)) + 1;
      }
    } break;
    
    case DW_EhPtrEnc_ULEB128:
    {
      U64 size = dw_based_range_read_uleb128(frame_base, frame_range, pointer_off, &raw_pointer);
      after_pointer_off = pointer_off + size;
    } break;
    
    case DW_EhPtrEnc_SLEB128:
    {
      U64 size = dw_based_range_read_sleb128(frame_base, frame_range, pointer_off,
                                             (S64*)&raw_pointer);
      after_pointer_off = pointer_off + size;
    } break;
  }
  
  // apply relative bases
  U64 pointer = raw_pointer;
  if (pointer != 0) {
    switch (encoding & DW_EhPtrEnc_ModifyMask) {
      case DW_EhPtrEnc_PcRel:
      {
        pointer = ptr_ctx->raw_base_vaddr + frame_range.min + off + raw_pointer;
      } break;
      case DW_EhPtrEnc_TextRel:
      {
        pointer = ptr_ctx->text_vaddr + raw_pointer;
      } break;
      case DW_EhPtrEnc_DataRel:
      {
        pointer = ptr_ctx->data_vaddr + raw_pointer;
      } break;
      case DW_EhPtrEnc_FuncRel:
      {
        Assert(!"TODO: need a sample to verify implementation");
        pointer = ptr_ctx->func_vaddr + raw_pointer;
      } break;
    }
  }
  
  // return
  *ptr_out = pointer;
  U64 result = after_pointer_off - off;
  return(result);
}

//- eh_frame parsing

internal void
dw_unwind_parse_cie_x64(void *base, Rng1U64 range, DW_EhPtrCtx *ptr_ctx, U64 off, DW_CIEUnpacked *cie_out)
{
  NotImplemented;
#if 0
  MemoryZeroStruct(cie_out);
  
  // get version
  U64 version_off = off;
  U8  version     = 0;
  dw_based_range_read(base, range, version_off, 1, &version);
  
  // check version
  if (version == 1 || version == 3) {
    
    // read augmentation
    U64     augmentation_off = version_off + 1;
    String8 augmentation     = dw_based_range_read_string(base, range, augmentation_off);
    
    // read code align
    U64 code_align_factor_off  = augmentation_off + augmentation.size + 1;
    U64 code_align_factor      = 0;
    U64 code_align_factor_size = dw_based_range_read_uleb128(base, range, code_align_factor_off, &code_align_factor);
    
    // read data align
    U64 data_align_factor_off  = code_align_factor_off + code_align_factor_size;
    S64 data_align_factor      = 0;
    U64 data_align_factor_size = dw_based_range_read_sleb128(base, range, data_align_factor_off, &data_align_factor);
    
    // return address register
    U64 ret_addr_reg_off       = data_align_factor_off + data_align_factor_size;
    U64 after_ret_addr_reg_off = 0;
    U64 ret_addr_reg           = 0;
    if (version == 1) {
      dw_based_range_read(base, range, ret_addr_reg_off, 1, &ret_addr_reg);
      after_ret_addr_reg_off = ret_addr_reg_off + 1;
    } else {
      U64 ret_addr_reg_size = dw_based_range_read_uleb128(base, range, ret_addr_reg_off, &ret_addr_reg);
      after_ret_addr_reg_off = ret_addr_reg_off + ret_addr_reg_size;
    }
    
    // TODO: 
    // Handle "eh" param, it indicates presence of EH Data field.
    // On 32bit arch it is a 4-byte and on 64-bit 8-byte value.
    // Reference: https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-PDA/LSB-PDA/ehframechpt.html
    // Reference doc doesn't clarify structure for EH Data though
    
    // check for augmentation data
    U64 aug_size_off          = after_ret_addr_reg_off;
    U64 after_aug_size_off    = after_ret_addr_reg_off;
    B32 has_augmentation_size = 0;
    U64 augmentation_size     = 0;
    if (augmentation.size > 0 && augmentation.str[0] == 'z') {
      has_augmentation_size = 1;
      U64 aug_size_size = dw_based_range_read_uleb128(base, range, aug_size_off, &augmentation_size);
      after_aug_size_off += aug_size_size;
    }
    
    // read augmentation data
    U64 aug_data_off       = after_aug_size_off;
    U64 after_aug_data_off = after_aug_size_off;
    
    DW_EhPtrEnc lsda_encoding = DW_EhPtrEnc_Omit;
    U64         handler_ip    = 0;
    DW_EhPtrEnc addr_encoding = DW_EhPtrEnc_UData8;
    
    if (has_augmentation_size > 0) {
      U64 aug_data_cursor = aug_data_off;
      for (U8 *ptr = augmentation.str + 1, *opl = augmentation.str + augmentation.size; ptr < opl; ++ptr) {
        switch (*ptr) {
          case 'L': {
            dw_based_range_read_struct(base, range, aug_data_cursor, &lsda_encoding);
            aug_data_cursor += sizeof(lsda_encoding);
          } break;
          case 'P': {
            DW_EhPtrEnc handler_encoding = DW_EhPtrEnc_Omit;
            dw_based_range_read_struct(base, range, aug_data_cursor, &handler_encoding);
            
            U64 ptr_off  = aug_data_cursor + sizeof(handler_encoding);
            U64 ptr_size = dw_unwind_parse_pointer_x64(base, range, ptr_ctx, handler_encoding, ptr_off, &handler_ip);
            aug_data_cursor = ptr_off + ptr_size;
          } break;
          case 'R': {
            dw_based_range_read_struct(base, range, aug_data_cursor, &addr_encoding);
            aug_data_cursor += sizeof(addr_encoding);
          } break;
          default: {
            goto dbl_break_aug;
          } break;
        }
      }
      dbl_break_aug:;
      after_aug_data_off = aug_data_cursor;
    }
    
    // cfi range
    U64 cfi_off  = range.min + after_aug_data_off;
    U64 cfi_size = 0;
    if (range.max > cfi_off) {
      cfi_size = range.max - cfi_off;
    }
    
    // commit values to out
    cie_out->version               = version;
    cie_out->lsda_encoding         = lsda_encoding;
    cie_out->addr_encoding         = addr_encoding;
    cie_out->has_augmentation_size = has_augmentation_size;
    cie_out->augmentation_size     = augmentation_size;
    cie_out->augmentation          = augmentation;
    cie_out->code_align_factor     = code_align_factor;
    cie_out->data_align_factor     = data_align_factor;
    cie_out->ret_addr_reg          = ret_addr_reg;
    cie_out->handler_ip            = handler_ip;
    cie_out->cfi_range.min         = cfi_off;
    cie_out->cfi_range.max         = cfi_off + cfi_size;
  }
#endif
}

internal void
dw_unwind_parse_fde_x64(void *base, Rng1U64 range, DW_EhPtrCtx *ptr_ctx, DW_CIEUnpacked *cie, U64 off, DW_FDEUnpacked *fde_out)
{
  // pull out pointer encoding field
  DW_EhPtrEnc ptr_enc = cie->addr_encoding;
  
  // ip first
  U64 ip_first_off  = off;
  U64 ip_first      = 0;
  U64 ip_first_size = dw_unwind_parse_pointer_x64(base, range, ptr_ctx, ptr_enc, ip_first_off, &ip_first);
  
  // ip range size
  U64 ip_range_size_off  = ip_first_off + ip_first_size;
  U64 ip_range_size      = 0;
  U64 ip_range_size_size = dw_unwind_parse_pointer_x64(base, range, ptr_ctx, ptr_enc & DW_EhPtrEnc_TypeMask, ip_range_size_off, &ip_range_size);
  
  // augmentation data
  U64 aug_data_off       = ip_range_size_off + ip_range_size_size;
  U64 after_aug_data_off = aug_data_off;
  U64 lsda_ip            = 0;
  
  if (cie->has_augmentation_size) {
    // augmentation size
    U64 augmentation_size  = 0;
    U64 aug_size_size      = dw_based_range_read_uleb128(base, range, aug_data_off, &augmentation_size);
    U64 after_aug_size_off = aug_data_off + aug_size_size;
    
    // extract lsda (only thing that can actually be in FDE's augmentation data as far as we know)
    DW_EhPtrEnc lsda_encoding = cie->lsda_encoding;
    if (lsda_encoding != DW_EhPtrEnc_Omit) {
      U64 lsda_off = after_aug_size_off;
      dw_unwind_parse_pointer_x64(base, range, ptr_ctx, lsda_encoding, lsda_off, &lsda_ip);
    }
    
    // set offset at end of augmentation data
    after_aug_data_off = after_aug_size_off + augmentation_size;
  }
  
  // cfi range
  U64 cfi_off  = range.min + after_aug_data_off;
  U64 cfi_size = 0;
  if (range.max > cfi_off) {
    cfi_size = range.max - cfi_off;
  }
  
  // commit values to out
  fde_out->ip_voff_range.min = ip_first;
  fde_out->ip_voff_range.max = ip_first + ip_range_size;
  fde_out->lsda_ip           = lsda_ip;
  fde_out->cfi_range.min     = cfi_off;
  fde_out->cfi_range.max     = cfi_off + cfi_size;
}

internal DW_CFIRecords
dw_unwind_eh_frame_cfi_from_ip_slow_x64(String8 raw_eh_frame, DW_EhPtrCtx *ptr_ctx, U64 ip_voff)
{
  Temp scratch = scratch_begin(0, 0);
  
  DW_CFIRecords result = {0};
  
  DW_CIEUnpackedNode *cie_first = 0;
  DW_CIEUnpackedNode *cie_last  = 0;
  
  U64 cursor = 0;
  for (;;) {
    // CIE/FDE size
    U64 rec_off            = cursor;
    U64 after_rec_size_off = 0;
    U64 rec_size           = 0;
    
    {
      str8_deserial_read(raw_eh_frame, rec_off, &rec_size, 4, 1);
      after_rec_size_off = 4;
      if (rec_size == max_U32) {
        str8_deserial_read(raw_eh_frame, rec_off + 4, &rec_size, 8, 1);
        after_rec_size_off = 12;
      }
    }
    
    // zero size is the end of the loop
    if (rec_size == 0) {
      break;
    }
    
    // compute end offset
    U64 rec_opl = rec_off + after_rec_size_off + rec_size;
    
    // sub-range the rest of the reads
    Rng1U64 rec_range = rng_1u64(rec_off, rec_opl);
    String8 raw_rec   = str8_substr(raw_eh_frame, rec_range);
    
    
    // discriminator
    U64 discrim_off = after_rec_size_off;
    U32 discrim     = 0;
    str8_deserial_read(raw_rec, discrim_off, &discrim, 4, 1);
    
    U64 after_discrim_off = discrim_off + 4;
    
    // CIE
    if (discrim == 0) {
      DW_CIEUnpacked cie = {0};
      dw_unwind_parse_cie_x64(raw_rec.str, rng_1u64(0, raw_rec.size), ptr_ctx, after_discrim_off, &cie);
      if (cie.version != 0) {
        DW_CIEUnpackedNode *node = push_array(scratch.arena, DW_CIEUnpackedNode, 1);
        node->cie                = cie;
        node->offset             = rec_off;
        SLLQueuePush(cie_first, cie_last, node);
      }
    }
    // FDE
    else {
      // compute cie offset
      U64 cie_offset = rec_range.min + discrim_off - discrim;
      
      // get cie node
      DW_CIEUnpackedNode *cie_node = 0;
      for (DW_CIEUnpackedNode *node = cie_first; node != 0; node = node->next) {
        if (node->offset == cie_offset) {
          cie_node = node;
          break;
        }
      }
      
      // parse fde
      DW_FDEUnpacked fde = {0};
      if (cie_node != 0) {
        dw_unwind_parse_fde_x64(raw_rec.str, rng_1u64(0,raw_rec.size), ptr_ctx, &cie_node->cie, after_discrim_off, &fde);
      }
      
      if (contains_1u64(fde.ip_voff_range, ip_voff)) {
        result.valid = 1;
        result.cie   = cie_node->cie;
        result.fde   = fde;
        break;
      }
    }
    
    // advance cursor
    cursor = rec_opl;
  }
  
  scratch_end(scratch);
  
  return(result);
}

internal U64
dw_search_eh_frame_hdr_linear_x64(String8 raw_eh_frame_hdr, DW_EhPtrCtx *ptr_ctx, U64 location)
{
  // Table contains only addresses for first instruction in a function and we cannot
  // guarantee that result is FDE that corresponds to the input location. 
  // So input location must be cheked against range from FDE header again.
  
  U64 closest_location = max_U64;
  U64 closest_address  = max_U64;
  
  U64 cursor = 0;
  
  U8 version = 0;
  cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &version);
  
  if (version == 1) {
#if 0
    DW_EhPtrCtx ptr_ctx = {0};
    // Set this to base address of .eh_frame_hdr. Entries are relative
    // to this section for some reason.
    ptr_ctx.data_vaddr = range.min;
    // If input location is VMA then set this to address of .text. 
    // Pointer parsing function will adjust "init_location" to correct VMA.
    ptr_ctx.text_vaddr = 0; 
#endif
    
    DW_EhPtrEnc eh_frame_ptr_enc = 0, fde_count_enc = 0, table_enc = 0;
    cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &eh_frame_ptr_enc);
    cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &fde_count_enc);
    cursor += str8_deserial_read_struct(raw_eh_frame_hdr, cursor, &table_enc);
    
    U64 eh_frame_ptr = 0, fde_count = 0;
    cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, eh_frame_ptr_enc, cursor, &eh_frame_ptr);
    cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, fde_count_enc, cursor, &fde_count);
    
    for (U64 fde_idx = 0; fde_idx < fde_count; ++fde_idx) {
      U64 init_location = 0, address = 0;
      cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, table_enc, cursor, &init_location);
      cursor += dw_unwind_parse_pointer_x64(raw_eh_frame_hdr.str, rng_1u64(0, raw_eh_frame_hdr.size), ptr_ctx, table_enc, cursor, &address);
      
      S64 current_delta = (S64)(location - init_location);
      S64 closest_delta = (S64)(location - closest_location);
      if (0 <= current_delta && current_delta < closest_delta) {
        closest_location = init_location;
        closest_address  = address;
      }
    }
  }
  
  // address where to find corresponding FDE, this is an absolute offset
  // into the image file.
  return closest_address;
}

internal DW_CFIRecords
dw_unwind_eh_frame_hdr_from_ip_fast_x64(String8 raw_eh_frame, String8 raw_eh_frame_hdr, DW_EhPtrCtx *ptr_ctx, U64 ip_voff)
{
  DW_CFIRecords result = {0};
  
  // find FDE offset
  void *eh_frame_hdr = raw_eh_frame.str;
  U64   fde_offset   = dw_search_eh_frame_hdr_linear_x64(raw_eh_frame_hdr, ptr_ctx, ip_voff);
  
  B32 is_fde_offset_valid = (fde_offset != max_U64);
  if (is_fde_offset_valid) {
    U64 fde_read_offset = (fde_offset - ptr_ctx->raw_base_vaddr);
    
    // read FDE size
    U64 fde_size = 0;
    fde_read_offset += dw_based_range_read_length(raw_eh_frame.str, rng_1u64(0,raw_eh_frame.size), fde_read_offset, &fde_size);
    
    // read FDE discriminator
    U32 fde_discrim = 0;
    fde_read_offset += str8_deserial_read_struct(raw_eh_frame, fde_read_offset, &fde_discrim);
    
    // compute parent CIE offset
    U64 cie_read_offset = fde_read_offset - (fde_discrim + sizeof(fde_discrim));
    
    // read CIE size
    U64 cie_size = 0;
    cie_read_offset += dw_based_range_read_length(raw_eh_frame.str, rng_1u64(0,raw_eh_frame.size), cie_read_offset, &cie_size);
    
    // read CIE discriminator
    U32 cie_discrim = max_U32;
    cie_read_offset += str8_deserial_read_struct(raw_eh_frame, cie_read_offset, &cie_discrim);
    
    B32 is_fde = (fde_discrim != 0);
    B32 is_cie = (cie_discrim == 0);
    if (is_fde && is_cie) {
      Rng1U64 cie_range = rng_1u64(0, cie_read_offset + (cie_size - sizeof(cie_discrim)));
      Rng1U64 fde_range = rng_1u64(0, fde_read_offset + (fde_size - sizeof(fde_discrim)));
      
      // parse CIE
      DW_CIEUnpacked cie = {0};
      dw_unwind_parse_cie_x64(raw_eh_frame.str, cie_range, ptr_ctx, cie_read_offset, &cie);
      
      // parse FDE
      DW_FDEUnpacked fde = {0};
      dw_unwind_parse_fde_x64(raw_eh_frame.str, fde_range, ptr_ctx, &cie, fde_read_offset, &fde);
      
      // range check instruction pointer
      if (contains_1u64(fde.ip_voff_range, ip_voff)) {
        result.valid = 1;
        result.cie   = cie;
        result.fde   = fde;
      }
    }
  }
  
  return result;
}

//- cfi machine

internal DW_CFIMachine
dw_unwind_make_machine_x64(U64 cells_per_row, DW_CIEUnpacked *cie, DW_EhPtrCtx *ptr_ctx)
{
  DW_CFIMachine result = {0};
  result.cells_per_row = cells_per_row;
  result.cie           = cie;
  result.ptr_ctx       = ptr_ctx;
  return result;
}

internal void
dw_unwind_machine_equip_initial_row_x64(DW_CFIMachine *machine, DW_CFIRow *initial_row)
{
  machine->initial_row = initial_row;
}

internal void
dw_unwind_machine_equip_fde_ip_x64(DW_CFIMachine *machine, U64 fde_ip)
{
  machine->fde_ip = fde_ip;
}

internal DW_CFIRow*
dw_unwind_row_alloc_x64(Arena *arena, U64 cells_per_row)
{
  DW_CFIRow *result = push_array(arena, DW_CFIRow, 1);
  result->cells     = push_array(arena, DW_CFICell, cells_per_row);
  return result;
}

internal void
dw_unwind_row_zero_x64(DW_CFIRow *row, U64 cells_per_row) {
  MemorySet(row->cells, 0, sizeof(*row->cells)*cells_per_row);
  MemoryZeroStruct(&row->cfa_cell);
}

internal void
dw_unwind_row_copy_x64(DW_CFIRow *dst, DW_CFIRow *src, U64 cells_per_row)
{
  MemoryCopy(dst->cells, src->cells, sizeof(*src->cells)*cells_per_row);
  dst->cfa_cell = src->cfa_cell;
}

internal B32
dw_unwind_machine_run_to_ip_x64(void *base, Rng1U64 range, DW_CFIMachine *machine, U64 target_ip, DW_CFIRow *row)
{
  Temp scratch = scratch_begin(0, 0);
  
  B32 result = 0;
  
  // pull out machine's equipment
  DW_CIEUnpacked *cie           = machine->cie;
  DW_EhPtrCtx    *ptr_ctx       = machine->ptr_ctx;
  U64             cells_per_row = machine->cells_per_row;
  DW_CFIRow      *initial_row   = machine->initial_row;
  
  // start with an empty stack
  DW_CFIRow *stack     = 0;
  DW_CFIRow *free_rows = 0;
  
  // initialize the row
  if (initial_row != 0) {
    dw_unwind_row_copy_x64(row, initial_row, cells_per_row);
  } else {
    dw_unwind_row_zero_x64(row, cells_per_row);
  }
  U64 table_ip = machine->fde_ip;
  
  // loop
  U64 cfi_off = 0;
  for (;;) {
    // op variables
    DW_CFA            opcode       = 0;
    U64               operand0     = 0;
    U64               operand1     = 0;
    U64               operand2     = 0;
    DW_CFAControlBits control_bits = 0;
    
    // decode opcode/operand0
    if (!dw_based_range_read(base, range, cfi_off, 1, &opcode)) {
      result = 1;
      goto done;
    }
    if ((opcode & DW_CFAMask_OpcodeHi) != 0) {
      operand0     = (opcode & DW_CFAMask_Operand);
      opcode       = (opcode & DW_CFAMask_OpcodeHi);
      control_bits = dw_unwind__cfa_control_bits_kind2[opcode >> 6];
    } else {
      if (opcode < DW_CFA_OplKind1) {
        control_bits = dw_unwind__cfa_control_bits_kind1[opcode];
      }
    }
    
    // decode operand1/operand2
    U64 decode_cursor = cfi_off + 1;
    {
      // setup loop ins/outs
      U64 o[2];
      DW_CFADecode dec[2] = {0};
      dec[0]              = (control_bits & 0xF);
      dec[1]              = ((control_bits >> 4) & 0xF);
      
      // loop
      U64 *out = o;
      for (U64 i = 0; i < 2; i += 1, out += 1) {
        DW_CFADecode d = dec[i];
        U64 o_size = 0;
        switch (d) {
          case 0: {
            *out = 0;
          } break;
          default: {
            if (d <= 8) {
              dw_based_range_read(base, range, decode_cursor, d, out);
              o_size = d;
            }
          } break;
          case DW_CFADecode_Address: {
            o_size = dw_unwind_parse_pointer_x64(base, range, ptr_ctx, cie->addr_encoding, decode_cursor, out);
          } break;
          case DW_CFADecode_ULEB128: {
            o_size = dw_based_range_read_uleb128(base, range, decode_cursor, out);
          } break;
          case DW_CFADecode_SLEB128: {
            o_size = dw_based_range_read_sleb128(base, range, decode_cursor, (S64*)out);
          } break;
        }
        decode_cursor += o_size;
      }
      
      // commit out values
      operand1 = o[0];
      operand2 = o[1];
    }
    U64 after_decode_off = decode_cursor;
    
    // register checks
    if (control_bits & DW_CFAControlBits_IsReg0) {
      if (operand0 >= cells_per_row) {
        goto done;
      }
    }
    if (control_bits & DW_CFAControlBits_IsReg1) {
      if (operand1 >= cells_per_row) {
        goto done;
      }
    }
    if (control_bits & DW_CFAControlBits_IsReg2) {
      if (operand2 >= cells_per_row) {
        goto done;
      }
    }
    
    // values for deferred work
    U64 new_table_ip = table_ip;
    
    // step
    U64 step_cursor = after_decode_off;
    switch (opcode) {
      default: goto done;
      case DW_CFA_Nop:break;
      
      //// new row/IP opcodes ////
      
      case DW_CFA_SetLoc: {
        new_table_ip = operand1;
      } break;
      case DW_CFA_AdvanceLoc: {
        new_table_ip = table_ip + operand0*cie->code_align_factor;
      } break;
      case DW_CFA_AdvanceLoc1:
      case DW_CFA_AdvanceLoc2:
      case DW_CFA_AdvanceLoc4: {
        U64 advance = operand1*cie->code_align_factor;
        new_table_ip = table_ip + advance;
      } break;
      
      //// change CFA (canonical frame address) opcodes ////
      
      case DW_CFA_DefCfa: {
        row->cfa_cell.rule = DW_CFI_CFA_Rule_RegOff;
        row->cfa_cell.reg_idx = operand1;
        row->cfa_cell.offset = operand2;
      } break;
      
      case DW_CFA_DefCfaSf: {
        row->cfa_cell.rule = DW_CFI_CFA_Rule_RegOff;
        row->cfa_cell.reg_idx = operand1;
        row->cfa_cell.offset = ((S64)operand2)*cie->data_align_factor;
      } break;
      
      case DW_CFA_DefCfaRegister: {
        // check rule
        if (row->cfa_cell.rule != DW_CFI_CFA_Rule_RegOff) {
          goto done;
        }
        // commit new cfa
        row->cfa_cell.reg_idx = operand1;
      } break;
      
      case DW_CFA_DefCfaOffset: {
        // check rule
        if (row->cfa_cell.rule != DW_CFI_CFA_Rule_RegOff) {
          goto done;
        }
        // commit new cfa
        row->cfa_cell.offset = operand1;
      } break;
      
      case DW_CFA_DefCfaOffsetSf: {
        // check rule
        if (row->cfa_cell.rule != DW_CFI_CFA_Rule_RegOff) {
          goto done;
        }
        // commit new cfa
        row->cfa_cell.offset = ((S64)operand1)*cie->data_align_factor;
      } break;
      
      case DW_CFA_DefCfaExpr: {
        // setup expr range
        U64 expr_first = range.min + after_decode_off;
        U64 expr_size  = operand1;
        step_cursor += expr_size;
        
        // commit new cfa
        row->cfa_cell.rule     = DW_CFI_CFA_Rule_Expr;
        row->cfa_cell.expr.min = expr_first;
        row->cfa_cell.expr.max = expr_first + expr_size;
      } break;
      
      
      //// change register rules ////
      
      case DW_CFA_Undefined: {
        row->cells[operand1].rule = DW_CFIRegisterRule_Undefined;
      } break;
      
      case DW_CFA_SameValue: {
        row->cells[operand1].rule = DW_CFIRegisterRule_SameValue;
      } break;
      
      case DW_CFA_Offset: {
        DW_CFICell *cell = &row->cells[operand0];
        cell->rule       = DW_CFIRegisterRule_Offset;
        cell->n          = operand1*cie->data_align_factor;
      } break;
      
      case DW_CFA_OffsetExt: {
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule       = DW_CFIRegisterRule_Offset;
        cell->n          = operand2*cie->data_align_factor;
      } break;
      
      case DW_CFA_OffsetExtSf: {
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule       = DW_CFIRegisterRule_Offset;
        cell->n          = ((S64)operand2)*cie->data_align_factor;
      } break;
      
      case DW_CFA_ValOffset: {
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule       = DW_CFIRegisterRule_ValOffset;
        cell->n          = operand2*cie->data_align_factor;
      } break;
      
      case DW_CFA_ValOffsetSf: {
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule       = DW_CFIRegisterRule_ValOffset;
        cell->n          = ((S64)operand2)*cie->data_align_factor;
      } break;
      
      case DW_CFA_Register: {
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule = DW_CFIRegisterRule_Register;
        cell->n = operand2;
      } break;
      
      case DW_CFA_Expr: {
        // setup expr range
        U64 expr_first = range.min + after_decode_off;
        U64 expr_size = operand2;
        step_cursor += expr_size;
        
        // commit new rule
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule = DW_CFIRegisterRule_Expression;
        cell->expr.min = expr_first;
        cell->expr.max = expr_first + expr_size;
      } break;
      
      case DW_CFA_ValExpr: {
        // setup expr range
        U64 expr_first = range.min + after_decode_off;
        U64 expr_size = operand2;
        step_cursor += expr_size;
        
        // commit new rule
        DW_CFICell *cell = &row->cells[operand1];
        cell->rule = DW_CFIRegisterRule_ValExpression;
        cell->expr.min = expr_first;
        cell->expr.max = expr_first + expr_size;
      } break;
      
      case DW_CFA_Restore: {
        // check initial row
        if (initial_row == 0) {
          goto done;
        }
        // commit new rule
        row->cells[operand0] = initial_row->cells[operand0];
      } break;
      
      case DW_CFA_RestoreExt: {
        // check initial row
        if (initial_row == 0) {
          goto done;
        }
        // commit new rule
        row->cells[operand1] = initial_row->cells[operand1];
      } break;
      
      
      //// row stack ////
      
      case DW_CFA_RememberState: {
        DW_CFIRow *stack_row = free_rows;
        if (stack_row != 0) {
          SLLStackPop(free_rows);
        } else {
          stack_row = dw_unwind_row_alloc_x64(scratch.arena, cells_per_row);
        }
        dw_unwind_row_copy_x64(stack_row, row, cells_per_row);
        SLLStackPush(stack, stack_row);
      } break;
      
      case DW_CFA_RestoreState: {
        if (stack != 0) {
          DW_CFIRow *stack_row = stack;
          SLLStackPop(stack);
          dw_unwind_row_copy_x64(row, stack_row, cells_per_row);
          SLLStackPush(free_rows, stack_row);
        } else {
          dw_unwind_row_zero_x64(row, cells_per_row);
        }
      } break;
    }
    
    // apply location change
    if (control_bits & DW_CFAControlBits_NewRow) {
      // new ip should always grow the ip
      if (new_table_ip <= table_ip) {
        goto done;
      }
      // stop if this encloses the target ip
      if (table_ip <= target_ip && target_ip < new_table_ip) {
        result = 1;
        goto done;
      }
      // commit new ip
      table_ip = new_table_ip;
    }
    
    // advance
    cfi_off = step_cursor;
  }
  done:;
  
  scratch_end(scratch);
  return result;
}

