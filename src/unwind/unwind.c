// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Memory View Helpers

internal UNW_MemView
unw_memview_from_data(String8 data, U64 base_vaddr)
{
  UNW_MemView result = {0};
  result.data = data.str;
  result.addr_first = base_vaddr;
  result.addr_opl = base_vaddr + data.size;
  return result;
}

internal B32
unw_memview_read(UNW_MemView *memview, U64 addr, U64 size, void *out)
{
  B32 result = 0;
  if(memview->addr_first <= addr && addr + size <= memview->addr_opl)
  {
    MemoryCopy(out, (U8*)memview->data + addr - memview->addr_first, size);
    result = 1;
  }
  return result;
}

////////////////////////////////
//~ rjf: PE/X64 Unwind Implementation

//- rjf: helpers

internal UNW_Step
unw_pe_x64__epilog(String8 bindata, PE_BinInfo *bin, U64 base_vaddr, UNW_MemView*memview, REGS_RegBlockX64 *regs)
{
  UNW_Step result = {0};
  U64 missed_read_addr = 0;
  
  //- setup parsing context
  U64 ip_voff = regs->rip.u64 - base_vaddr;
  U64 sec_number = pe_section_num_from_voff(bindata, bin, ip_voff);
  COFF_SectionHeader *sec = coff_section_header_from_num(bindata, bin->section_array_off, sec_number);
  void* inst_base = pe_ptr_from_section_num(bindata, bin, sec_number);
  U64   inst_size = sec->vsize;
  
  //- setup parsing variables
  B32 keep_parsing = 1;
  U64 off = ip_voff - sec->voff;
  
  
  //- parsing loop
  for(;keep_parsing;)
  {
    keep_parsing = 0;
    
    U8 inst_byte = 0;
    if(off + sizeof(inst_byte) <= inst_size)
    {
      void *ptr = (U8*)inst_base + off;
      MemoryCopy(&inst_byte, ptr, sizeof(inst_byte));
    }
    off += 1;
    
    U8 rex = 0;
    if((inst_byte & 0xF0) == 0x40)
    {
      rex = inst_byte & 0xF; // rex prefix
      if(off + sizeof(inst_base) <= inst_size)
      {
        void *ptr = (U8*)inst_base + off;
        MemoryCopy(&inst_byte, ptr, sizeof(inst_byte));
      }
      off += 1;
    }
    
    switch(inst_byte)
    {
      // pop
      case 0x58:
      case 0x59:
      case 0x5A:
      case 0x5B:
      case 0x5C:
      case 0x5D:
      case 0x5E:
      case 0x5F:
      {
        U64 sp = regs->rsp.u64;
        U64 value = 0;
        if(!unw_memview_read_struct(memview, sp, &value))
        {
          missed_read_addr = sp;
          goto error_out;
        }
        
        // modify register
        PE_UnwindGprRegX64 gpr_reg = (inst_byte - 0x58) + (rex & 1)*8;
        REGS_Reg64 *reg = unw_pe_x64__gpr_reg(regs, gpr_reg);
        
        // not a final instruction
        keep_parsing = 1;
        
        // commit registers
        reg->u64 = value;
        regs->rsp.u64 = sp + 8;
      }break;
      
      // add $nnnn,%rsp 
      case 0x81:
      {
        // skip one byte (we already know what it is in this scenario)
        off += 1;
        
        // read the 4-byte immediate
        S32 imm = 0;
        if(off + sizeof(imm) < inst_size)
        {
          void *ptr = (U8*)inst_base + off;
          MemoryCopy(&imm, ptr, sizeof(imm));
        }
        off += 4;
        
        // not a final instruction
        keep_parsing = 1;
        
        // update stack pointer
        regs->rsp.u64 = (U64)(regs->rsp.u64 + imm);
      }break;
      
      // add $n,%rsp
      case 0x83:
      {
        // skip one byte (we already know what it is in this scenario)
        off += 1;
        
        // read the 1-byte immediate
        S8 imm = 0;
        if(off + sizeof(imm) < inst_size)
        {
          void *ptr = (U8*)inst_base + off;
          MemoryCopy(&imm, ptr, sizeof(imm));
        }
        off += 1;
        
        // update stack pointer
        regs->rsp.u64 = (U64)(regs->rsp.u64 + imm);
        keep_parsing = 1;
      }break;
      
      // lea imm8/imm32,$rsp
      case 0x8D:
      {
        // read source register
        U8 modrm = 0;
        if(off + sizeof(modrm) < inst_size)
        {
          void *ptr = (U8*)inst_base + off;
          MemoryCopy(&modrm, ptr, sizeof(modrm));
        }
        PE_UnwindGprRegX64 gpr_reg = (modrm & 7) + (rex & 1)*8;
        REGS_Reg64 *reg = unw_pe_x64__gpr_reg(regs, gpr_reg);
        U64 reg_value = reg->u64;
        
        // advance to the immediate
        off += 1;
        
        S32 imm = 0;
        // read 1-byte immediate
        if((modrm >> 6) == 1)
        {
          S8 imm8 = 0;
          if(off + sizeof(imm8) < inst_size)
          {
            void *ptr = (U8*)inst_base + off;
            MemoryCopy(&imm8, ptr, sizeof(imm8));
          }
          imm = imm8;
          off += 1;
        }
        
        // read 4-byte immediate
        else
        {
          if(off + sizeof(imm) < inst_size)
          {
            void *ptr = (U8*)inst_base + off;
            MemoryCopy(&imm, ptr, sizeof(imm));
          }
          off += 4;
        }
        
        regs->rsp.u64 = (U64)(reg_value + imm);
        keep_parsing = 1;
      }break;
      
      // ret $nn
      case 0xC2:
      {
        // read new ip
        U64 sp = regs->rsp.u64;
        U64 new_ip = 0;
        if(!unw_memview_read_struct(memview, sp, &new_ip))
        {
          missed_read_addr = sp;
          goto error_out;
        }
        
        // read 2-byte immediate & advance stack pointer
        U16 imm = 0;
        if(off + sizeof(imm) < inst_size)
        {
          void *ptr = (U8*)inst_base + off;
          MemoryCopy(&imm, ptr, sizeof(imm));
        }
        U64 new_sp = sp + 8 + imm;
        
        // commit registers
        regs->rip.u64 = new_ip;
        regs->rsp.u64 = new_sp;
      }break;
      
      // ret / rep; ret
      case 0xF3:
      {
        Assert(!"Hit me!");
      }
      case 0xC3:
      {
        // read new ip
        U64 sp = regs->rsp.u64;
        U64 new_ip = 0;
        if(!unw_memview_read_struct(memview, sp, &new_ip))
        {
          missed_read_addr = sp;
          goto error_out;
        }
        
        // advance stack pointer
        U64 new_sp = sp + 8;
        
        // commit registers
        regs->rip.u64 = new_ip;
        regs->rsp.u64 = new_sp;
      }break;
      
      // jmp nnnn
      case 0xE9:
      {
        Assert(!"Hit Me");
        // TODO(allen): general idea: read the immediate, move the ip, leave the sp, done
        // we don't have any cases to exercise this right now. no guess implementation!
      }break;
      
      // jmp n
      case 0xEB:
      {
        Assert(!"Hit Me");
        // TODO(allen): general idea: read the immediate, move the ip, leave the sp, done
        // we don't have any cases to exercise this right now. no guess implementation!
      }break;
    }
    
  }
  
  error_out:;
  
  if(missed_read_addr != 0)
  {
    result.dead = 1;
    result.missed_read = 1;
    result.missed_read_addr = missed_read_addr;
  }
  
  return(result);
}

internal B32
unw_pe_x64__voff_is_in_epilog(String8 bindata, PE_BinInfo *bin, U64 voff, PE_IntelPdata *final_pdata)
{
  // NOTE(allen): There are restrictions placed on how an epilog is allowed
  // to be formed (https://docs.microsoft.com/en-us/cpp/build/prolog-and-epilog?view=msvc-160)
  // Here we interpret machine code directly according to the rules
  // given there to determine if the code we're looking at looks like an epilog.
  
  // TODO(allen): Figure out how to verify this.
  
  //- setup parsing context
  U64 sec_number = pe_section_num_from_voff(bindata, bin, voff);
  COFF_SectionHeader *sec = coff_section_header_from_num(bindata, bin->section_array_off, sec_number);
  void* inst_base = pe_ptr_from_section_num(bindata, bin, sec_number);
  U64   inst_size = sec->vsize;
  
  //- setup parsing variables
  B32 is_epilog = 0;
  B32 keep_parsing = 1;
  U64 off = voff - sec->voff;
  
  //- check first instruction
  {
    B32 inst_read_success = 0;
    U8 inst[4];
    if (off + sizeof(inst) < inst_size){
      void *ptr = (U8*)inst_base + off;
      MemoryCopy(&inst, ptr, sizeof(inst));
      inst_read_success = 1;
    }
    
    if (!inst_read_success){
      keep_parsing = 0;
    }
    else{
      if ((inst[0] & 0xF8) == 0x48){
        switch (inst[1]){
          // add $nnnn,%rsp
          case 0x81:
          {
            if (inst[0] == 0x48 && inst[2] == 0xC4){
              off += 7;
            }
            else{
              keep_parsing = 0;
            }
          }break;
          
          // add $n,%rsp
          case 0x83:
          {
            if (inst[0] == 0x48 && inst[2] == 0xC4){
              off += 4;
            }
            else{
              keep_parsing = 0;
            }
          }break;
          
          // lea n(reg),%rsp
          case 0x8D:
          {
            if ((inst[0] & 0x06) == 0 &&
                ((inst[2] >> 3) & 0x07) == 0x04 &&
                (inst[2] & 0x07) != 0x04){
              U8 imm_size = (inst[2] >> 6);
              // 1-byte immediate
              if (imm_size == 1){
                off += 4;
              }
              // 4-byte immediate
              else if (imm_size == 2){
                off += 7;
              }
              else{
                keep_parsing = 0;
              }
            }
            else{
              keep_parsing = 0;
            }
          }break;
        }
      }
    }
  }
  
  //- parsing loop
  if (keep_parsing){
    for (;;){
      // read inst
      U8 inst_byte = 0;
      if (off + sizeof(inst_byte) < inst_size){
        void *ptr = (U8*)inst_base + off;
        MemoryCopy(&inst_byte, ptr, sizeof(inst_byte));
      }
      else{
        goto loop_break;
      }
      
      // when (... I don't know ...) rely on the next byte
      U64 check_off = off;
      U8 check_inst_byte = inst_byte;
      if ((inst_byte & 0xF0) == 0x40){
        check_off = off + 1;
        if (off + sizeof(check_inst_byte) < inst_size){
          void *ptr = (U8*)inst_base + off;
          MemoryCopy(&check_inst_byte, ptr, sizeof(check_inst_byte));
        }
        else{
          goto loop_break;
        }
      }
      
      switch (check_inst_byte){
        // pop
        case 0x58:case 0x59:case 0x5A:case 0x5B:
        case 0x5C:case 0x5D:case 0x5E:case 0x5F:
        {
          off = check_off + 1;
        }break;
        
        // ret
        case 0xC2:case 0xC3:
        { 
          is_epilog = 1;
          goto loop_break;
        }break;
        
        // jmp nnnn
        case 0xE9:
        {
          U64 imm_off = check_off + 1;
          S32 imm = 0;
          if (off + sizeof(imm) < inst_size){
            void *ptr = (U8*)inst_base + off;
            MemoryCopy(&imm, ptr, sizeof(imm));
          }
          else{
            goto loop_break;
          }
          
          U64 next_off = (U64)(imm_off + sizeof(imm) + imm);
          if (!(final_pdata->voff_first <= next_off && next_off < final_pdata->voff_one_past_last)){
            goto loop_break;
          }
          
          off = next_off;
          // TODO(allen): why isn't this just the end of the epilog?
        }break;
        
        // rep; ret (for amd64 prediction bug)
        case 0xF3:
        {
          U8 next_inst_byte = 0;
          if (off + sizeof(next_inst_byte) < inst_size){
            void *ptr = (U8*)inst_base + off;
            MemoryCopy(&next_inst_byte, ptr, sizeof(next_inst_byte));
          }
          is_epilog = (next_inst_byte == 0xC3);
          goto loop_break;
        }break;
        
        default: goto loop_break;
      }
    }
    
    loop_break:;
  }
  
  //- fill result
  B32 result = is_epilog;
  return(result);
}

internal REGS_Reg64*
unw_pe_x64__gpr_reg(REGS_RegBlockX64 *regs, PE_UnwindGprRegX64 unw_reg){
  static REGS_Reg64 dummy = {0};
  REGS_Reg64 *result = &dummy;
  switch (unw_reg){
    case PE_UnwindGprRegX64_RAX: result = &regs->rax; break;
    case PE_UnwindGprRegX64_RCX: result = &regs->rcx; break;
    case PE_UnwindGprRegX64_RDX: result = &regs->rdx; break;
    case PE_UnwindGprRegX64_RBX: result = &regs->rbx; break;
    case PE_UnwindGprRegX64_RSP: result = &regs->rsp; break;
    case PE_UnwindGprRegX64_RBP: result = &regs->rbp; break;
    case PE_UnwindGprRegX64_RSI: result = &regs->rsi; break;
    case PE_UnwindGprRegX64_RDI: result = &regs->rdi; break;
    case PE_UnwindGprRegX64_R8 : result = &regs->r8 ; break;
    case PE_UnwindGprRegX64_R9 : result = &regs->r9 ; break;
    case PE_UnwindGprRegX64_R10: result = &regs->r10; break;
    case PE_UnwindGprRegX64_R11: result = &regs->r11; break;
    case PE_UnwindGprRegX64_R12: result = &regs->r12; break;
    case PE_UnwindGprRegX64_R13: result = &regs->r13; break;
    case PE_UnwindGprRegX64_R14: result = &regs->r14; break;
    case PE_UnwindGprRegX64_R15: result = &regs->r15; break;
  }
  return(result);
}

//- rjf: unwind step

internal UNW_Step
unw_unwind_pe_x64(String8 bindata, PE_BinInfo *bin, U64 base_vaddr, UNW_MemView *memview, REGS_RegBlockX64 *regs)
{
  UNW_Step result = {0};
  U64 missed_read_addr = 0;
  
  //- grab ip_voff (several places can use this)
  U64 ip_voff = regs->rip.u64 - base_vaddr;
  
  //- get pdata entry from current ip
  PE_IntelPdata *initial_pdata = 0;
  {
    U64 initial_pdata_off = pe_intel_pdata_off_from_voff__binary_search(bindata, bin, ip_voff);
    if(initial_pdata_off != 0)
    {
      initial_pdata = (PE_IntelPdata*)(bindata.str + initial_pdata_off);
    }
  }
  
  //- no pdata; unwind by reading stack pointer
  if(initial_pdata == 0)
  {
    // read ip from stack pointer
    U64 sp = regs->rsp.u64;
    U64 new_ip = 0;
    if(!unw_memview_read_struct(memview, sp, &new_ip))
    {
      missed_read_addr = sp;
      goto error_out;
    }
    
    // advance stack pointer
    U64 new_sp = sp + 8;
    
    // commit registers
    regs->rip.u64 = new_ip;
    regs->rsp.u64 = new_sp;
  }
  
  //- got pdata; perform unwinding with exception handling
  else
  {
    // try epilog unwind
    B32 did_epilog_unwind = 0;
    if(unw_pe_x64__voff_is_in_epilog(bindata, bin, ip_voff, initial_pdata))
    {
      result = unw_pe_x64__epilog(bindata, bin, base_vaddr, memview, regs);
      did_epilog_unwind = 1;
    }
    
    // try xdata unwind
    if(!did_epilog_unwind)
    {
      B32 did_machframe = 0;
      
      // get frame reg
      REGS_Reg64 *frame_reg = 0;
      U64 frame_off = 0;
      
      {
        U64 unwind_info_off = initial_pdata->voff_unwind_info;
        PE_UnwindInfo *unwind_info = (PE_UnwindInfo*)(pe_ptr_from_voff(bindata, bin, unwind_info_off));
        
        U32 frame_reg_id = PE_UNWIND_INFO_REG_FROM_FRAME(unwind_info->frame);
        U64 frame_off_val = PE_UNWIND_INFO_OFF_FROM_FRAME(unwind_info->frame);
        
        if (frame_reg_id != 0){
          frame_reg = unw_pe_x64__gpr_reg(regs, frame_reg_id);
          // TODO(allen): at this point if frame_reg is zero, the exe is corrupted.
        }
        frame_off = frame_off_val;
      }
      
      PE_IntelPdata *last_pdata = 0;
      PE_IntelPdata *pdata = initial_pdata;
      for (;pdata != last_pdata;)
      {
        //- rjf: unpack unwind info & codes
        U64 unwind_info_off = pdata->voff_unwind_info;
        PE_UnwindInfo *unwind_info = (PE_UnwindInfo*)(pe_ptr_from_voff(bindata, bin, unwind_info_off));
        PE_UnwindCode *unwind_codes = (PE_UnwindCode*)(unwind_info + 1);
        
        //- rjf: unpack frame base
        U64 frame_base = regs->rsp.u64;
        if(frame_reg != 0)
        {
          U64 raw_frame_base = frame_reg->u64;
          U64 adjusted_frame_base = raw_frame_base - frame_off*16;
          if(adjusted_frame_base < raw_frame_base)
          {
            frame_base = adjusted_frame_base;
          }
          else
          {
            frame_base = 0;
          }
        }
        
        //- rjf: bad unwind info -> abort
        if(unwind_info == 0)
        {
          result.dead = 1;
          goto error_out;
        }
        
        //- op code interpreter
        PE_UnwindCode *code_ptr = unwind_codes;
        PE_UnwindCode *code_opl = unwind_codes + unwind_info->codes_num;
        for(PE_UnwindCode  *next_code_ptr = 0; code_ptr < code_opl; code_ptr = next_code_ptr)
        {
          // extract op code parts
          U32 op_code = PE_UNWIND_OPCODE_FROM_FLAGS(code_ptr->flags);
          U32 op_info = PE_UNWIND_INFO_FROM_FLAGS(code_ptr->flags);
          
          // determine number of op code slots
          U32 slot_count = pe_slot_count_from_unwind_op_code(op_code);
          if(op_code == PE_UnwindOpCode_ALLOC_LARGE && op_info == 1)
          {
            slot_count += 1;
          }
          
          // check op code slot count
          if (slot_count == 0 || code_ptr + slot_count > code_opl){
            result.dead = 1;
            goto end_xdata_unwind;
          }
          
          // set next op code pointer
          next_code_ptr = code_ptr + slot_count;
          
          // interpret this op code
          U64 code_voff = pdata->voff_first + code_ptr->off_in_prolog;
          if (code_voff <= ip_voff){
            switch (op_code){
              case PE_UnwindOpCode_PUSH_NONVOL:
              {
                // read value from stack pointer
                U64 sp = regs->rsp.u64;
                U64 value = 0;
                if(!unw_memview_read_struct(memview, sp, &value))
                {
                  missed_read_addr = sp;
                  goto error_out;
                }
                
                // advance stack pointer
                U64 new_sp = sp + 8;
                
                // commit registers
                REGS_Reg64 *reg = unw_pe_x64__gpr_reg(regs, op_info);
                reg->u64 = value;
                regs->rsp.u64 = new_sp;
              }break;
              
              case PE_UnwindOpCode_ALLOC_LARGE:
              {
                // read alloc size
                U64 size = 0;
                if (op_info == 0){
                  size = code_ptr[1].u16*8;
                }
                else if (op_info == 1){
                  size = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
                }
                else{
                  result.dead = 1;
                  goto end_xdata_unwind;
                }
                
                // advance stack pointer
                U64 sp = regs->rsp.u64;
                U64 new_sp = sp + size;
                
                // advance stack pointer
                regs->rsp.u64 = new_sp;
              }break;
              
              case PE_UnwindOpCode_ALLOC_SMALL:
              {
                // advance stack pointer
                regs->rsp.u64 += op_info*8 + 8;
              }break;
              
              case PE_UnwindOpCode_SET_FPREG:
              {
                // put stack pointer back to the frame base
                regs->rsp.u64 = frame_base;
              }break;
              
              case PE_UnwindOpCode_SAVE_NONVOL:
              {
                // read value from frame base
                U64 off = code_ptr[1].u16*8;
                U64 addr = frame_base + off;
                U64 value = 0;
                if (!unw_memview_read_struct(memview, addr, &value)){
                  missed_read_addr = addr;
                  goto error_out;
                }
                
                // commit to register
                REGS_Reg64 *reg = unw_pe_x64__gpr_reg(regs, op_info);
                reg->u64 = value;
              }break;
              
              case PE_UnwindOpCode_SAVE_NONVOL_FAR:
              {
                // read value from frame base
                U64 off = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
                U64 addr = frame_base + off;
                U64 value = 0;
                if (!unw_memview_read_struct(memview, addr, &value)){
                  missed_read_addr = addr;
                  goto error_out;
                }
                
                // commit to register
                REGS_Reg64 *reg = unw_pe_x64__gpr_reg(regs, op_info);
                reg->u64 = value;
              }break;
              
              case PE_UnwindOpCode_EPILOG:
              {
                result.dead = 1;
              }break;
              
              case PE_UnwindOpCode_SPARE_CODE:
              {
                result.dead = 1;
                // Assert(!"Hit me!");
                // TODO(allen): ???
              }break;
              
              case PE_UnwindOpCode_SAVE_XMM128:
              {
                // read new register values
                U8 buf[16];
                U64 off = code_ptr[1].u16*16;
                U64 addr = frame_base + off;
                if (!unw_memview_read(memview, addr, 16, buf)){
                  missed_read_addr = addr;
                  goto error_out;
                }
                
                // commit to register
                void *xmm_reg = (&regs->ymm0) + op_info;
                MemoryCopy(xmm_reg, buf, sizeof(buf));
              }break;
              
              case PE_UnwindOpCode_SAVE_XMM128_FAR:
              {
                // read new register values
                U8 buf[16];
                U64 off = code_ptr[1].u16 + ((U32)code_ptr[2].u16 << 16);
                U64 addr = frame_base + off;
                if (!unw_memview_read(memview, addr, 16, buf)){
                  missed_read_addr = addr;
                  goto error_out;
                }
                
                // commit to register
                void *xmm_reg = (&regs->ymm0) + op_info;
                MemoryCopy(xmm_reg, buf, sizeof(buf));
              }break;
              
              case PE_UnwindOpCode_PUSH_MACHFRAME:
              {
                // NOTE(rjf): this was found by stepping through kernel code after an exception was
                // thrown, encountered in the exception_stepping_tests (after the throw) in mule_main
                
                if(op_info > 1)
                {
                  result.dead = 1;
                  goto end_xdata_unwind;
                }
                
                // read values
                U64 sp_og = regs->rsp.u64;
                U64 sp_adj = sp_og;
                if(op_info == 1)
                {
                  sp_adj += 8;
                }
                
                U64 ip_value = 0;
                if(!unw_memview_read_struct(memview, sp_adj, &ip_value))
                {
                  missed_read_addr = sp_adj;
                  goto error_out;
                }
                
                U64 sp_after_ip = sp_adj + 8;
                U16 ss_value = 0;
                if(!unw_memview_read_struct(memview, sp_after_ip, &ss_value))
                {
                  missed_read_addr = sp_after_ip;
                  goto error_out;
                }
                
                U64 sp_after_ss = sp_after_ip + 8;
                U64 rflags_value = 0;
                if(!unw_memview_read_struct(memview, sp_after_ss, &rflags_value))
                {
                  missed_read_addr = sp_after_ss;
                  goto error_out;
                }
                
                U64 sp_after_rflags = sp_after_ss + 8;
                U64 sp_value = 0;
                if(!unw_memview_read_struct(memview, sp_after_rflags, &sp_value))
                {
                  missed_read_addr = sp_after_rflags;
                  goto error_out;
                }
                
                // commit registers
                regs->rip.u64 = ip_value;
                regs->ss.u16 = ss_value;
                regs->rflags.u64 = rflags_value;
                regs->rsp.u64 = sp_value;
                
                // mark machine frame
                did_machframe = 1;
              }break;
            }
          }
        }
        
        //- iterate pdata chain
        U32 flags = PE_UNWIND_INFO_FLAGS_FROM_HDR(unwind_info->header);
        if (!(flags & PE_UnwindInfoFlag_CHAINED)){
          break;
        }
        
        U64 code_count_rounded = AlignPow2(unwind_info->codes_num, sizeof(PE_UnwindCode));
        U64 code_size = code_count_rounded*sizeof(PE_UnwindCode);
        U64 chained_pdata_off = unwind_info_off + sizeof(PE_UnwindInfo) + code_size;
        
        last_pdata = pdata;
        pdata = (PE_IntelPdata*)pe_ptr_from_voff(bindata, bin, chained_pdata_off);
      }
      
      if(!did_machframe)
      {
        U64 sp = regs->rsp.u64;
        U64 new_ip = 0;
        if(!unw_memview_read_struct(memview, sp, &new_ip))
        {
          missed_read_addr = sp;
          goto error_out;
        }
        
        // advance stack pointer
        U64 new_sp = sp + 8;
        
        // commit registers
        regs->rip.u64 = new_ip;
        regs->rsp.u64 = new_sp;
      }
      
      end_xdata_unwind:;
    }
  }
  
  error_out:;
  
  if(missed_read_addr != 0)
  {
    result.dead = 1;
    result.missed_read = 1;
    result.missed_read_addr = missed_read_addr;
  }
  
  if(!result.dead)
  {
    result.stack_pointer = regs->rsp.u64;
  }
  
  return(result);
}
