// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

//- rjf: file descriptor memory reading/writing helpers

internal U64
dmn_lnx_read(int memory_fd, Rng1U64 range, void *dst)
{
  U64 bytes_read = 0;
  U8 *ptr = (U8 *)dst;
  U8 *opl = ptr + dim_1u64(range);
  U64 cursor = range.min;
  for(;ptr < opl;)
  {
    size_t to_read = (size_t)(opl - ptr);
    ssize_t actual_read = pread(memory_fd, ptr, to_read, cursor);
    if(actual_read == -1)
    {
      break;
    }
    ptr += actual_read;
    cursor += actual_read;
    bytes_read += actual_read;
  }
  return bytes_read;
}

internal B32
dmn_lnx_write(int memory_fd, Rng1U64 range, void *src)
{
  B32 result = 1;
  U8 *ptr = (U8 *)src;
  U8 *opl = ptr + dim_1u64(range);
  U64 cursor = range.min;
  for(;ptr < opl;)
  {
    size_t to_write = (size_t)(opl - ptr);
    ssize_t actual_write = pwrite(memory_fd, ptr, to_write, cursor);
    if(actual_write == -1)
    {
      result = 0;
      break;
    }
    ptr += actual_write;
    cursor += actual_write;
  }
  return result;
}

internal String8
dmn_lnx_read_string(Arena *arena, int memory_fd, U64 base_vaddr)
{
  String8 result = {0};
  U64 string_size = 0;
  for(U64 vaddr = base_vaddr; string_size < 4096; vaddr += 1, string_size += 1)
  {
    char byte = 0;
    if(pread(memory_fd, &byte, sizeof(byte), vaddr) == 0)
    {
      break;
    }
    if(byte == '\0' || byte == '\n')
    {
      break;
    }
  }
  if(string_size != 0)
  {
    char *buf = push_array_no_zero(arena, char, string_size+1);
    pread(memory_fd, buf, string_size, base_vaddr);
    buf[string_size] = '\0';
    result = str8((U8 *)buf, string_size);
  }
  return result;
}

//- rjf: pid => info extraction

internal String8
dmn_lnx_exe_path_from_pid(Arena *arena, pid_t pid)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: get exe link path
  String8 exe_link_path = str8f(scratch.arena, "/proc/%d/exe", pid);
  
  //- rjf: read the link
  Temp restore_point = temp_begin(arena);
  B32 good = 0;
  U8 *buffer = 0;
  int readlink_result = 0;
  S64 cap = PATH_MAX;
  for(S64 r = 0; r < 4; cap *= 2, r += 1)
  {
    temp_end(restore_point);
    buffer = push_array_no_zero(arena, U8, cap);
    readlink_result = readlink((char *)exe_link_path.str, (char *)buffer, cap);
    if(readlink_result < cap)
    {
      good = 1;
      break;
    }
  }
  
  //- rjf: package result
  String8 result = {0};
  if(!good || readlink_result == -1)
  {
    temp_end(restore_point);
  }
  else
  {
    arena_pop(arena, (cap - readlink_result - 1));
    result = str8(buffer, readlink_result + 1);
  }
  
  scratch_end(scratch);
  return result;
}

internal Arch
dmn_lnx_arch_from_pid(pid_t pid)
{
  Arch result = Arch_Null;
  {
    Temp scratch = scratch_begin(0, 0);
    String8 exe_path = dmn_lnx_exe_path_from_pid(scratch.arena, pid);
    
    // rjf: unpack exe handle
    int exe_fd = -1;
    if(exe_path.size != 0)
    {
      exe_fd = open((char*)exe_path.str, O_RDONLY);
    }
    
    // rjf: unpack elf identifier
    U8 e_ident[ELF_Identifier_Max] = {0};
    B32 is_elf = 0;
    U8 elf_class = 0;
    if(exe_fd >= 0 &&
       pread(exe_fd, e_ident, sizeof(e_ident), 0) == sizeof(e_ident))
    {
      is_elf = (e_ident[ELF_Identifier_Mag0] == 0x7f && 
                e_ident[ELF_Identifier_Mag1] == 'E'  &&
                e_ident[ELF_Identifier_Mag2] == 'L'  && 
                e_ident[ELF_Identifier_Mag3] == 'F');
      elf_class = e_ident[ELF_Identifier_Class];
    }
    
    // rjf: read elf header
    ELF_Hdr64 hdr = {0};
    switch(elf_class)
    {
      case 1:
      {
        ELF_Hdr32 hdr32 = {0};
        if(pread(exe_fd, &hdr32, sizeof(hdr32), 0) == sizeof(hdr32))
        {
          hdr = elf_hdr64_from_hdr32(hdr32);
        }
      }break;
      case 2:
      {
        pread(exe_fd, &hdr, sizeof(hdr), 0);
      }break;
    }
    
    // rjf: determine arch from elf machine kind
    result = arch_from_elf_machine(hdr.e_machine);
    
    close(exe_fd);
    scratch_end(scratch);
  }
  return result;
}

internal DMN_LNX_ProcessAux
dmn_lnx_aux_from_pid(pid_t pid, Arch arch)
{
  Temp scratch = scratch_begin(0, 0);
  DMN_LNX_ProcessAux result = {0};
  
  // rjf: open aux data
  String8 auxv_path = push_str8f(scratch.arena, "/proc/%d/auxv", pid);
  int aux_fd = open((char*)auxv_path.str, O_RDONLY);
  
  // rjf: scan aux data
  if(aux_fd >= 0)
  {
    B32 addr_32bit = (arch == Arch_x86 || arch == Arch_arm32);
    for(;;)
    {
      result.filled = 1;
      
      // rjf: read next aux
      U64 type = 0;
      U64 val = 0;
      if(addr_32bit)
      {
        ELF_Auxv32 aux = {0};
        if(read(aux_fd, &aux, sizeof(aux)) != sizeof(aux))
        {
          goto brkloop;
        }
        type = aux.a_type;
        val  = aux.a_val;
      }
      else
      {
        ELF_Auxv64 aux = {0};
        if(read(aux_fd, &aux, sizeof(aux)) != sizeof(aux))
        {
          goto brkloop;
        }
        type = aux.a_type;
        val  = aux.a_val;
      }
      
      // rjf: fill result
      switch(type)
      {
        default:{}break;
        case ELF_AuxType_Null:         goto brkloop; break;
        case ELF_AuxType_Phnum:        result.phnum  = val; break;
        case ELF_AuxType_Phent:        result.phent  = val; break;
        case ELF_AuxType_Phdr:         result.phdr   = val; break;
        case ELF_AuxType_ExecFn:       result.execfn = val; break;
        case ELF_AuxType_Pagesz:       result.pagesz = val; break;
      }
    }
    brkloop:;
    close(aux_fd);
  }
  
  scratch_end(scratch);
  return result;
}

//- rjf: phdr info extraction

internal DMN_LNX_PhdrInfo
dmn_lnx_phdr_info_from_memory(int memory_fd, B32 is_32bit, U64 phvaddr, U64 phsize, U64 phcount)
{
  DMN_LNX_PhdrInfo result = {0};
  result.range.min = max_U64;
  
  // rjf: determine how much phdr we'll read
  U64 phdr_size_expected = (is_32bit ? sizeof(ELF_Phdr32) : sizeof(ELF_Phdr64));
  U64 phdr_stride = (phsize ? phsize : phdr_size_expected);
  U64 phdr_read_size = ClampTop(phsize, phdr_size_expected);
  
  // rjf: scan table
  U64 va = phvaddr;
  for(U64 i = 0; i < phcount; i += 1, va += phdr_stride)
  {
    // rjf: read type and range
    ELF_PType p_type = 0;
    U64 p_vaddr = 0;
    U64 p_memsz = 0;
    if(is_32bit)
    {
      ELF_Phdr32 phdr32 = {0};
      dmn_lnx_read_struct(memory_fd, va, &phdr32);
      p_type = phdr32.p_type;
      p_vaddr = phdr32.p_vaddr;
      p_memsz = phdr32.p_memsz;
    }
    else
    {
      ELF_Phdr64 phdr64 = {0};
      dmn_lnx_read_struct(memory_fd, va, &phdr64);
      p_type = phdr64.p_type;
      p_vaddr = phdr64.p_vaddr;
      p_memsz = phdr64.p_memsz;
    }
    
    // rjf: save
    switch(p_type)
    {
      default:{}break;
      case ELF_PType_Dynamic:
      {
        result.dynamic = p_vaddr;
      }break;
      case ELF_PType_Load:
      {
        U64 min = p_vaddr;
        U64 max = p_vaddr + p_memsz;
        result.range.min = Min(result.range.min, min);
        result.range.max = Max(result.range.max, max);
      }break;
    }
  }
  
  return result;
}

//- rjf: process entity => info extraction

internal DMN_LNX_ModuleInfoList
dmn_lnx_module_info_list_from_process(Arena *arena, DMN_LNX_Entity *process)
{
  Arch arch = process->arch;
  B32 is_32bit = (arch == Arch_x86 || arch == Arch_arm32);
  int memory_fd = (int)process->fd;
  
  //- rjf: pid => aux
  DMN_LNX_ProcessAux aux = dmn_lnx_aux_from_pid((pid_t)process->id, arch);
  
  //- rjf: memory => phdr info
  DMN_LNX_PhdrInfo phdr_info = dmn_lnx_phdr_info_from_memory(memory_fd, is_32bit, aux.phdr, aux.phent, aux.phnum);
  
  //- rjf: memory space & vaddr => linkmap first
  U64 first_linkmap_vaddr = 0;
  if(phdr_info.dynamic != 0)
  {
    U64 off = phdr_info.dynamic;
    for(;;)
    {
      // rjf: read next dyn entry
      ELF_Dyn64 dyn = {0};
      if(is_32bit)
      {
        ELF_Dyn32 dyn32 = {0};
        dmn_lnx_read_struct(memory_fd, off, &dyn32);
        dyn.tag = dyn32.tag;
        dyn.val = dyn32.val;
        off += sizeof(dyn32);
      }
      else
      {
        dmn_lnx_read_struct(memory_fd, off, &dyn);
        off += sizeof(dyn);
      }
      
      // rjf: break on zero
      if(dyn.tag == ELF_DynTag_Null)
      {
        break;
      }
      
      // rjf: pltgot => grab first linkmap address
      if(dyn.tag == ELF_DynTag_PltGot)
      {
        // True for x86 and x64
        //  vas[0] virtual address of .dynamic
        //  vas[2] callback for resolving function address of relocation and if successful jumps to it.
        // 
        // Code that sets up PLTGOT is in glibc/sysdeps/x86_64/dl_machine.h -> elf_machine_runtime_setup
        //
        U64 vas_off = dyn.val;
        U64 vas[3] = {0};
        dmn_lnx_read(memory_fd, r1u64(vas_off, vas_off+sizeof(vas)), vas);
        first_linkmap_vaddr = vas[1];
        break;
      }
    }
  }
  
  //- rjf: push main module
  DMN_LNX_ModuleInfoList list = {0};
  {
    U64 base_vaddr = (aux.phdr & ~(aux.pagesz-1));
    DMN_LNX_ModuleInfoNode *n = push_array(arena, DMN_LNX_ModuleInfoNode, 1);
    SLLQueuePush(list.first, list.last, n);
    list.count += 1;
    n->v.vaddr_range = shift_1u64(phdr_info.range, base_vaddr);
    n->v.name = aux.execfn;
  }
  
  //- rjf: iterate link maps
  if(first_linkmap_vaddr != 0)
  {
    U64 linkmap_vaddr = first_linkmap_vaddr;
    for(;linkmap_vaddr != 0;)
    {
      // rjf: read next linkmap entry
      ELF_LinkMap64 linkmap = {0};
      if(is_32bit)
      {
        // TODO(rjf): endianness
        ELF_LinkMap32 linkmap32 = {0};
        dmn_lnx_read_struct(memory_fd, linkmap_vaddr, &linkmap32);
        linkmap.base = linkmap32.base;
        linkmap.name = linkmap32.name;
        linkmap.ld   = linkmap32.ld;
        linkmap.next = linkmap32.next;
      }
      else
      {
        dmn_lnx_read_struct(memory_fd, linkmap_vaddr, &linkmap);
      }
      
      // rjf: push module for next link map
      if(linkmap.base != 0)
      {
        // rjf: find phdr info for this module
        U64 phvaddr = 0;
        U64 phentsize = 0;
        U64 phcount = 0;
        if(is_32bit)
        {
          ELF_Hdr32 ehdr = {0};
          dmn_lnx_read_struct(memory_fd, linkmap.base, &ehdr);
          phvaddr   = ehdr.e_phoff + linkmap.base;
          phentsize = ehdr.e_phentsize;
          phcount   = ehdr.e_phnum;
        }
        else
        {
          ELF_Hdr64 ehdr = {0};
          dmn_lnx_read_struct(memory_fd, linkmap.base, &ehdr);
          phvaddr   = ehdr.e_phoff + linkmap.base;
          phentsize = ehdr.e_phentsize;
          phcount   = ehdr.e_phnum;
        }
        
        // rjf: extract info from module's phdrs
        DMN_LNX_PhdrInfo module_phdr_info = dmn_lnx_phdr_info_from_memory(memory_fd, is_32bit, phvaddr, phentsize, phcount);
        
        // rjf: push
        DMN_LNX_ModuleInfoNode *n = push_array(arena, DMN_LNX_ModuleInfoNode, 1);
        SLLQueuePush(list.first, list.last, n);
        list.count += 1;
        n->v.vaddr_range = r1u64(linkmap.base, linkmap.base + dim_1u64(module_phdr_info.range));
        n->v.name = linkmap.name;
      }
      
      // rjf: iterate
      linkmap_vaddr = linkmap.next;
    }
  }
  
  return list;
}

////////////////////////////////
//~ rjf: Entity Functions

internal DMN_LNX_Entity *
dmn_lnx_entity_alloc(DMN_LNX_Entity *parent, DMN_LNX_EntityKind kind)
{
  DMN_LNX_Entity *entity = dmn_lnx_state->free_entity;
  if(entity != 0)
  {
    SLLStackPop(dmn_lnx_state->free_entity);
  }
  else
  {
    entity = push_array(dmn_lnx_state->entities_arena, DMN_LNX_Entity, 1);
    dmn_lnx_state->entities_count += 1;
  }
  U32 gen = entity->gen;
  MemoryCopyStruct(entity, &dmn_lnx_nil_entity);
  entity->gen += 1;
  if(parent != &dmn_lnx_nil_entity)
  {
    DLLPushBack_NPZ(&dmn_lnx_nil_entity, parent->first, parent->last, entity, next, prev);
    entity->parent = parent;
  }
  entity->kind = kind;
  return entity;
}

internal void
dmn_lnx_entity_release(DMN_LNX_Entity *entity)
{
  if(entity->parent != &dmn_lnx_nil_entity)
  {
    DLLRemove_NPZ(&dmn_lnx_nil_entity, entity->parent->first, entity->parent->last, entity, next, prev);
    entity->parent = &dmn_lnx_nil_entity;
  }
  {
    Temp scratch = scratch_begin(0, 0);
    DMN_LNX_EntityNode start_task = {0, entity};
    DMN_LNX_EntityNode *first_task = &start_task;
    for(DMN_LNX_EntityNode *t = first_task; t != 0; t = t->next)
    {
      SLLStackPush(dmn_lnx_state->free_entity, t->v);
      for(DMN_LNX_Entity *child = t->v->first; child != &dmn_lnx_nil_entity; child = child->next)
      {
        DMN_LNX_EntityNode *task = push_array(scratch.arena, DMN_LNX_EntityNode, 1);
        task->next = t->next;
        t->next = task;
        task->v = child;
      }
    }
    scratch_end(scratch);
  }
}

internal DMN_Handle
dmn_lnx_handle_from_entity(DMN_LNX_Entity *entity)
{
  DMN_Handle handle = {0};
  U64 index = (U64)(entity - dmn_lnx_state->entities_base);
  if(index <= 0xffffffffu)
  {
    handle.u32[0] = index;
    handle.u32[1] = entity->gen;
  }
  return handle;
}

internal DMN_LNX_Entity *
dmn_lnx_entity_from_handle(DMN_Handle handle)
{
  DMN_LNX_Entity *result = &dmn_lnx_nil_entity;
  U64 index = (U64)handle.u32[0];
  if(index < dmn_lnx_state->entities_count &&
     dmn_lnx_state->entities_base[index].gen == handle.u32[1])
  {
    result = &dmn_lnx_state->entities_base[index];
  }
  return result;
}

internal DMN_LNX_Entity *
dmn_lnx_thread_from_pid(pid_t pid)
{
  DMN_LNX_Entity *result = &dmn_lnx_nil_entity;
  if(pid != 0)
  {
    for EachIndex(idx, dmn_lnx_state->entities_count)
    {
      if(dmn_lnx_state->entities_base[idx].kind == DMN_LNX_EntityKind_Thread && (pid_t)dmn_lnx_state->entities_base[idx].id == pid)
      {
        result = &dmn_lnx_state->entities_base[idx];
        break;
      }
    }
  }
  return result;
}

internal B32
dmn_lnx_thread_read_reg_block(DMN_LNX_Entity *thread, void *reg_block)
{
  B32 result = 0;
  switch(thread->arch)
  {
    case Arch_Null:
    case Arch_COUNT:{}break;
    case Arch_x86:
    case Arch_arm64:
    case Arch_arm32:
    {NotImplemented;}break;
    
    ////////////////////////////
    //- rjf: [x64]
    //
    case Arch_x64:
    {
      REGS_RegBlockX64 *dst = (REGS_RegBlockX64 *)reg_block;
      pid_t tid = (pid_t)thread->id;
      
      //- rjf: read GPR
      B32 got_gpr = 0;
      {
        DMN_LNX_UserX64 ctx = {0};
        struct iovec iov_gpr = {0};
        iov_gpr.iov_len = sizeof(ctx);
        iov_gpr.iov_base = &ctx;
        if(ptrace(PTRACE_GETREGSET, tid, (void*)NT_PRSTATUS, &iov_gpr) != -1)
        {
          got_gpr = 1;
          DMN_LNX_UserRegsX64 *src = &ctx.regs;
          dst->rax.u64    = src->rax;
          dst->rcx.u64    = src->rcx;
          dst->rdx.u64    = src->rdx;
          dst->rbx.u64    = src->rbx;
          dst->rsp.u64    = src->rsp;
          dst->rbp.u64    = src->rbp;
          dst->rsi.u64    = src->rsi;
          dst->rdi.u64    = src->rdi;
          dst->r8.u64     = src->r8;
          dst->r9.u64     = src->r9;
          dst->r10.u64    = src->r10;
          dst->r11.u64    = src->r11;
          dst->r12.u64    = src->r12;
          dst->r13.u64    = src->r13;
          dst->r14.u64    = src->r14;
          dst->r15.u64    = src->r15;
          dst->cs.u16     = src->cs;
          dst->ds.u16     = src->ds;
          dst->es.u16     = src->es;
          dst->fs.u16     = src->fs;
          dst->gs.u16     = src->gs;
          dst->ss.u16     = src->ss;
          dst->fsbase.u64 = src->fsbase;
          dst->gsbase.u64 = src->gsbase;
          dst->rip.u64    = src->rip;
          dst->rflags.u64 = src->rflags;
        }
      }
      
      //- rjf: read FPR
      B32 got_fpr = 0;
      if(got_gpr)
      {
        Temp scratch = scratch_begin(0, 0);
        DMN_LNX_XSave *xsave = 0;
        DMN_LNX_XSaveLegacy *xsave_legacy = 0;
        
        // rjf: try xsave
        if(!xsave_legacy)
        {
          U8 xsave_buffer[KB(4)];
          struct iovec iov_xsave = {0};
          iov_xsave.iov_len = sizeof(xsave_buffer);
          iov_xsave.iov_base = xsave_buffer;
          if(ptrace(PTRACE_GETREGSET, tid, (void*)NT_X86_XSTATE, &iov_xsave) != -1)
          {
            xsave = push_array_no_zero(scratch.arena, DMN_LNX_XSave, 1);
            MemoryCopy(xsave, xsave_buffer, sizeof(*xsave));
            xsave_legacy = &xsave->legacy;
          }
        }
        
        // rjf: try fxsave
        if(!xsave_legacy)
        {
          DMN_LNX_XSaveLegacy fxsave = {0};
          struct iovec iov_fxsave = {0};
          iov_fxsave.iov_len = sizeof(fxsave);
          iov_fxsave.iov_base = &fxsave;
          if(ptrace(PTRACE_GETREGSET, tid, (void *)NT_FPREGSET, &iov_fxsave) != -1)
          {
            xsave_legacy = push_array_no_zero(scratch.arena, DMN_LNX_XSaveLegacy, 1);
            MemoryCopy(xsave_legacy, &fxsave, sizeof(*xsave_legacy));
          }
        }
        
        // rjf: fill from xsave legacy
        if(xsave_legacy)
        {
          DMN_LNX_XSaveLegacy *src = xsave_legacy;
          dst->fcw.u16 = src->fcw;
          dst->fsw.u16 = src->fsw;
          dst->ftw.u16 = src->ftw; // TODO(rjf): old: fix tag word (?)
          dst->fop.u16 = src->fop;
          dst->fip.u64 = src->b64.fip;
          // TODO(rjf): these 16-bit registers do not belong in x64
          dst->fcs.u16 = 0;
          dst->fdp.u64 = src->b64.fdp;
          dst->fds.u16 = 0;
          dst->mxcsr.u32 = src->mxcsr;
          dst->mxcsr_mask.u32 = src->mxcsr_mask;
          {
            U8 *float_s = src->st_space.u8;
            REGS_Reg80 *float_d = &dst->st0;
            for(U32 n = 0; n < 8; n += 1, float_s += 16, float_d += 1)
            {
              MemoryCopy(float_d, float_s, sizeof(*float_d));
            }
          }
          {
            U8 *xmm_s = src->xmm_space.u8;
            REGS_Reg512 *xmm_d = &dst->zmm0;
            for(U32 n = 0; n < 16; n += 1, xmm_s += 16, xmm_d += 1)
            {
              MemoryCopy(xmm_d, xmm_s, 16);
            }
          }
        }
        
        // rjf: fill from ymm registers
        // TODO(rjf): this is a lie; ymm can technically move around. study & fix.
        if(xsave)
        {
          B32 has_ymm_registers = ((xsave->header.xstate_bv & 4) != 0);
          if(has_ymm_registers)
          {
            U8 *ymm_s = (U8 *)xsave->ymmh;
            REGS_Reg512 *ymm_d = &dst->zmm0;
            for(U32 n = 0; n < 16; n += 1, ymm_s += 16, ymm_d += 1)
            {
              MemoryCopy(((U8*)ymm_d) + 16, ymm_s, 16);
            }
          }
        }
        
        got_fpr = (xsave || xsave_legacy);
        scratch_end(scratch);
      }
      
      //- rjf: read debug registers
      B32 got_debug = 0;
      if(got_fpr)
      {
        got_debug = 1;
        REGS_Reg64 *dr_d = &dst->dr0;
        for(U32 i = 0; i < 8; i += 1, dr_d += 1)
        {
          if(i != 4 && i != 5)
          {
            U64 offset = OffsetOf(DMN_LNX_UserX64, u_debugreg[i]);
            errno = 0;
            int peek_result = ptrace(PTRACE_PEEKUSER, tid, PtrFromInt(offset), 0);
            if(errno == 0)
            {
              dr_d->u64 = (U64)peek_result;
            }
            else
            {
              got_debug = 0;
            }
          }
        }
      }
      
      result = got_debug;
    }break;
  }
  return result;
}

internal B32
dmn_lnx_thread_write_reg_block(DMN_LNX_Entity *thread, void *reg_block)
{
  B32 result = 0;
  switch(thread->arch)
  {
    case Arch_Null:
    case Arch_COUNT:{}break;
    case Arch_x86:
    case Arch_arm64:
    case Arch_arm32:
    {NotImplemented;}break;
    
    ////////////////////////////
    //- rjf: [x64]
    //
    case Arch_x64:
    {
      REGS_RegBlockX64 *src = (REGS_RegBlockX64 *)reg_block;
      pid_t tid = (pid_t)thread->id;
      
      //- rjf: write GPR
      B32 did_gpr = 0;
      {
        DMN_LNX_UserX64 dst = {0};
        dst.regs.rax       = src->rax.u64;
        dst.regs.rcx       = src->rcx.u64;
        dst.regs.rdx       = src->rdx.u64;
        dst.regs.rbx       = src->rbx.u64;
        dst.regs.rsp       = src->rsp.u64;
        dst.regs.rbp       = src->rbp.u64;
        dst.regs.rsi       = src->rsi.u64;
        dst.regs.rdi       = src->rdi.u64;
        dst.regs.r8        = src->r8.u64;
        dst.regs.r9        = src->r9.u64;
        dst.regs.r10       = src->r10.u64;
        dst.regs.r11       = src->r11.u64;
        dst.regs.r12       = src->r12.u64;
        dst.regs.r13       = src->r13.u64;
        dst.regs.r14       = src->r14.u64;
        dst.regs.r15       = src->r15.u64;
        dst.regs.cs        = src->cs.u16;
        dst.regs.ds        = src->ds.u16;
        dst.regs.es        = src->es.u16;
        dst.regs.fs        = src->fs.u16;
        dst.regs.gs        = src->gs.u16;
        dst.regs.ss        = src->ss.u16;
        dst.regs.fsbase    = src->fsbase.u64;
        dst.regs.gsbase    = src->gsbase.u64;
        dst.regs.rip       = src->rip.u64;
        dst.regs.rflags    = src->rflags.u64;
        struct iovec iov_gpr = {0};
        iov_gpr.iov_base = &dst;
        iov_gpr.iov_len = sizeof(dst);
        int gpr_result = ptrace(PTRACE_SETREGSET, tid, (void*)NT_PRSTATUS, &iov_gpr);
        did_gpr = (gpr_result != -1);
      }
      
      //- rjf: write FPR
      B32 did_fpr = 0;
      if(did_gpr)
      {
        // rjf: fill xsave structure
        DMN_LNX_XSave xsave = {0};
        {
          xsave.legacy.fcw          = src->fcw.u16;
          xsave.legacy.fsw          = src->fsw.u16;
          xsave.legacy.ftw          = src->ftw.u16;
          xsave.legacy.fop          = src->fop.u16;
          xsave.legacy.b64.fip      = src->fip.u64;
          xsave.legacy.b64.fdp      = src->fdp.u64;
          xsave.legacy.mxcsr        = src->mxcsr.u32;
          xsave.legacy.mxcsr_mask   = src->mxcsr_mask.u32;
          {
            U8 *float_d = xsave.legacy.st_space.u8;
            REGS_Reg80 *float_s = &src->st0;
            for(U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 16)
            {
              MemoryCopy(float_d, float_s, sizeof(*float_s));
            }
          }
          {
            U8 *xmm_d = xsave.legacy.xmm_space.u8;
            REGS_Reg512 *xmm_s = &src->zmm0;
            for(U32 n = 0; n < 16; n += 1, xmm_s += 1, xmm_d += 16)
            {
              MemoryCopy(xmm_d, xmm_s, 16);
            }
          }
          xsave.header.xstate_bv = 7;
          {
            // TODO(rjf): this is a lie; ymm can technically move around. study & fix.
            U8 *ymm_d = xsave.ymmh;
            REGS_Reg512 *ymm_s = &src->zmm0;
            for(U32 n = 0; n < 16; n += 1, ymm_s += 1, ymm_d += 16)
            {
              MemoryCopy(ymm_d, ((U8 *)ymm_s) + 16, 16);
            }
          }
        }
        
        // rjf: try xsave
        int xsave_result = -1;
        {
          struct iovec iov_xsave = {0};
          iov_xsave.iov_base = &xsave;
          iov_xsave.iov_len = sizeof(xsave);
          xsave_result = ptrace(PTRACE_SETREGSET, tid, (void*)NT_X86_XSTATE, &iov_xsave);
        }
        
        // rjf: try fxsave
        int fxsave_result = -1;
        if(xsave_result == -1)
        {
          struct iovec iov_fxsave = {0};
          iov_fxsave.iov_base = &xsave.legacy;
          iov_fxsave.iov_len = sizeof(xsave.legacy);
          fxsave_result = ptrace(PTRACE_SETREGSET, tid, (void*)NT_FPREGSET, &iov_fxsave);
        }
        
        // rjf: good finish requires xsave or fxsave
        did_fpr = (xsave_result != -1 || fxsave_result != -1);
      }
      
      //- rjf: write debug registers
      B32 did_dbg = 0;
      if(did_fpr)
      {
        did_dbg = 1;
        REGS_Reg64 *dr_s = &src->dr0;
        for(U32 i = 0; i < 8; i += 1, dr_s += 1)
        {
          if(i != 4 && i != 5)
          {
            U64 offset = OffsetOf(DMN_LNX_UserX64, u_debugreg[i]);
            int poke_result = ptrace(PTRACE_POKEUSER, tid, PtrFromInt(offset), dr_s->u64);
            if(poke_result == -1)
            {
              did_dbg = 0;
            }
          }
        }
      }
      
      result = (did_dbg);
    }break;
  }
  return result;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  Arena *arena = arena_alloc();
  dmn_lnx_state = push_array(arena, DMN_LNX_State, 1);
  dmn_lnx_state->arena = arena;
  dmn_lnx_state->deferred_events_arena = arena_alloc();
  dmn_lnx_state->entities_arena = arena_alloc(.reserve_size = GB(32), .commit_size = KB(64), .flags = ArenaFlag_NoChain);
  dmn_lnx_state->entities_base = push_array(dmn_lnx_state->entities_arena, DMN_LNX_Entity, 0);
  dmn_lnx_entity_alloc(&dmn_lnx_nil_entity, DMN_LNX_EntityKind_Root);
  dmn_lnx_state->access_mutex = os_mutex_alloc();
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Blocking Control Thread Operations (Implemented Per-OS)

internal DMN_CtrlCtx *
dmn_ctrl_begin(void)
{
  DMN_CtrlCtx *ctx = (DMN_CtrlCtx *)1;
  dmn_lnx_ctrl_thread = 1;
  return ctx;
}

internal void
dmn_ctrl_exclusive_access_begin(void)
{
  OS_MutexScope(dmn_lnx_state->access_mutex)
  {
    dmn_lnx_state->access_run_state = 1;
  }
}

internal void
dmn_ctrl_exclusive_access_end(void)
{
  OS_MutexScope(dmn_lnx_state->access_mutex)
  {
    dmn_lnx_state->access_run_state = 1;
  }
}

internal U32
dmn_ctrl_launch(DMN_CtrlCtx *ctx, OS_ProcessLaunchParams *params)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: unpack command line
  char **argv = 0;
  int argc = 0;
  {
    argc = (int)(params->cmd_line.node_count);
    argv = push_array(scratch.arena, char *, argc+1);
    {
      U64 idx = 0;
      for(String8Node *n = params->cmd_line.first; n != 0; n = n->next, idx += 1)
      {
        argv[idx] = (char *)push_str8_copy(scratch.arena, n->string).str;
      }
    }
  }
  
  //- rjf: unpack path
  char *path = (char *)push_str8_copy(scratch.arena, params->path).str;
  
  //- rjf: unpack environment
  char **env = 0;
  {
    env = push_array(scratch.arena, char *, params->env.node_count+1);
    {
      U64 idx = 0;
      for(String8Node *n = params->env.first; n != 0; n = n->next, idx += 1)
      {
        env[idx] = (char *)push_str8_copy(scratch.arena, n->string).str;
      }
    }
  }
  
  //- rjf: create & set up new process
  if(argv != 0 && argv[0] != 0)
  {
    pid_t pid = 0;
    int ptrace_result = 0;
    int chdir_result = 0;
    int setoptions_result = 0;
    B32 error__need_child_kill = 0;
    
    //- rjf: fork
    pid = fork();
    if(pid == -1) { goto error; }
    
    //- rjf: child process -> execute actual target
    if(pid == 0)
    {
      ptrace_result = ptrace(PTRACE_TRACEME, 0, 0, 0);
      if(ptrace_result == -1) { goto error; }
      chdir_result = chdir(path);
      if(chdir_result == -1) { goto error; }
      execve(argv[0], argv, env);
      abort();
    }
    
    //- rjf: parent process
    if(pid != 0)
    {
      //- rjf: wait for child
      int status = 0;
      pid_t wait_id = waitpid(pid, &status, __WALL);
      if(wait_id != pid)
      {
        // NOTE(rjf): we do not know what this means - needs study if this actually arises.
        goto error;
      }
      
      //- rjf: determine child launch status
      typedef enum LaunchStatus
      {
        LaunchStatus_Null,
        LaunchStatus_FailBeforePtrace,
        LaunchStatus_FailAfterPtrace,
        LaunchStatus_Success,
      }
      LaunchStatus;
      LaunchStatus launch_status = LaunchStatus_Null;
      {
        B32 wifstopped = WIFSTOPPED(status);
        int wstopsig = WSTOPSIG(status);
        if(0){}
        else if(wifstopped && wstopsig == SIGTRAP) { launch_status = LaunchStatus_Success; }
        else if(wifstopped && wstopsig != SIGTRAP) { launch_status = LaunchStatus_FailAfterPtrace; }
        else                                       { launch_status = LaunchStatus_FailBeforePtrace; }
      }
      
      //- rjf: respond to launch status appropriately
      switch(launch_status)
      {
        //- rjf: no understood handling path
        default:{}break;
        
        //- rjf: failure, after ptrace => we need to explicitly obtain the
        // result code & exit the process, otherwise it will become a zombie,
        // since it is ptrace'd.
        case LaunchStatus_FailAfterPtrace:
        {
          B32 cleanup_good = 0;
          int detach_result = ptrace(PTRACE_DETACH, pid, 0, (void*)SIGCONT);
          if(detach_result != -1)
          {
            int status_cleanup = 0;
            pid_t wait_id_cleanup = waitpid(pid, &status_cleanup, __WALL);
            if(wait_id_cleanup == pid)
            {
              cleanup_good = 1;
            }
          }
          if(cleanup_good)
          {
            // TODO(rjf): child initialization failed, but we at least cleaned it up.
          }
          else
          {
            // TODO(rjf): child initialization failed, *and* we couldn't clean it up, so we've created
            // yet-another zombie.
          }
        }break;
        
        //- rjf: successful launch
        case LaunchStatus_Success:
        {
          setoptions_result = ptrace(PTRACE_SETOPTIONS, pid, 0, PtrFromInt(DMN_LNX_PTRACE_OPTIONS));
          if(setoptions_result == -1) { error__need_child_kill = 1; goto error; }
          
          //- rjf: build initial process/thread/modules entities
          DMN_LNX_Entity *process = &dmn_lnx_nil_entity;
          DMN_LNX_Entity *main_thread = &dmn_lnx_nil_entity;
          {
            // rjf: build process
            process = dmn_lnx_entity_alloc(dmn_lnx_state->entities_base, DMN_LNX_EntityKind_Process);
            process->arch = dmn_lnx_arch_from_pid(pid);
            process->id = pid;
            process->fd = open((char*)str8f(scratch.arena, "/proc/%d/mem", pid).str, O_RDWR);
            {
              DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
              e->kind    = DMN_EventKind_CreateProcess;
              e->process = dmn_lnx_handle_from_entity(process);
              e->arch    = process->arch;
              e->code    = pid;
            }
            
            // rjf: build thread
            {
              DMN_LNX_Entity *thread = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Thread);
              thread->id = pid;
              thread->arch = process->arch;
              {
                DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
                e->kind    = DMN_EventKind_CreateThread;
                e->process = dmn_lnx_handle_from_entity(process);
                e->thread  = dmn_lnx_handle_from_entity(thread);
                e->arch    = thread->arch;
                e->code    = thread->id;
              }
              main_thread = thread;
            }
            
            // rjf: gather all process module infos
            DMN_LNX_ModuleInfoList module_infos = dmn_lnx_module_info_list_from_process(scratch.arena, process);
            for(DMN_LNX_ModuleInfoNode *n = module_infos.first; n != 0; n = n->next)
            {
              DMN_LNX_Entity *module = dmn_lnx_entity_alloc(process, DMN_LNX_EntityKind_Module);
              module->id = n->v.name;
              {
                DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
                e->kind    = DMN_EventKind_LoadModule;
                e->process = dmn_lnx_handle_from_entity(process);
                e->thread  = dmn_lnx_handle_from_entity(main_thread);
                e->module  = dmn_lnx_handle_from_entity(module);
                e->arch    = process->arch;
                e->address = n->v.vaddr_range.min;
                e->size    = dim_1u64(n->v.vaddr_range);
                e->string  = dmn_lnx_read_string(dmn_lnx_state->deferred_events_arena, process->fd, n->v.name);
              }
            }
            
            // rjf: handshake event
            {
              DMN_Event *e = dmn_event_list_push(dmn_lnx_state->deferred_events_arena, &dmn_lnx_state->deferred_events);
              e->kind    = DMN_EventKind_HandshakeComplete;
              e->process = dmn_lnx_handle_from_entity(process);
              e->thread  = dmn_lnx_handle_from_entity(main_thread);
              e->arch    = process->arch;
            }
          }
        }break;
      }
    }
    
    //- rjf: error case
    goto success;
    error:;
    {
      if(error__need_child_kill)
      {
        // TODO(rjf)
      }
    }
    
    //- rjf: success
    success:;
  }
  
  scratch_end(scratch);
  return 0;
}

internal B32
dmn_ctrl_attach(DMN_CtrlCtx *ctx, U32 pid)
{
  return 0;
}

internal B32
dmn_ctrl_kill(DMN_CtrlCtx *ctx, DMN_Handle process, U32 exit_code)
{
  B32 result = 0;
  DMN_LNX_Entity *process_entity = dmn_lnx_entity_from_handle(process);
  if(process_entity != &dmn_lnx_nil_entity &&
     kill(process_entity->id, SIGKILL) != -1)
  {
    result = 1;
  }
  return result;
}

internal B32
dmn_ctrl_detach(DMN_CtrlCtx *ctx, DMN_Handle process)
{
  B32 result = 0;
  DMN_LNX_Entity *process_entity = dmn_lnx_entity_from_handle(process);
  if(process_entity != &dmn_lnx_nil_entity &&
     ptrace(PTRACE_DETACH, process_entity->id, 0, 0) != -1)
  {
    result = 1;
  }
  return result;
}

internal DMN_EventList
dmn_ctrl_run(Arena *arena, DMN_CtrlCtx *ctx, DMN_RunCtrls *ctrls)
{
  DMN_EventList evts = {0};
  {
    Temp scratch = scratch_begin(&arena, 1);
    
    ////////////////////////////
    //- rjf: unpack controls
    //
    DMN_LNX_Entity *single_step_thread = dmn_lnx_entity_from_handle(ctrls->single_step_thread);
    
    ////////////////////////////
    //- rjf: push any deferred events
    //
    {
      for(DMN_EventNode *n = dmn_lnx_state->deferred_events.first; n != 0; n = n->next)
      {
        DMN_Event *e_src = &n->v;
        DMN_Event *e_dst = dmn_event_list_push(arena, &evts);
        MemoryCopyStruct(e_dst, e_src);
        e_dst->string = str8_copy(arena, e_dst->string);
      }
      MemoryZeroStruct(&dmn_lnx_state->deferred_events);
      arena_clear(dmn_lnx_state->deferred_events_arena);
    }
    
    ////////////////////////////
    //- rjf: no processes, no output events -> not attached
    //
    if(evts.count == 0 && dmn_lnx_state->entities_base->first == &dmn_lnx_nil_entity)
    {
      DMN_Event *e = dmn_event_list_push(arena, &evts);
      e->kind       = DMN_EventKind_Error;
      e->error_kind = DMN_ErrorKind_NotAttached;
    }
    
    ////////////////////////////
    //- rjf: determine if we need to wait for new events
    //
    B32 need_wait_on_events = (evts.count == 0);
    
    ////////////////////////////
    //- rjf: write all traps into memory
    //
    U8 *trap_swap_bytes = push_array_no_zero(scratch.arena, U8, ctrls->traps.trap_count);
    ProfScope("write all traps into memory")
    {
      U64 trap_idx = 0;
      for(DMN_TrapChunkNode *n = ctrls->traps.first; n != 0; n = n->next)
      {
        for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, trap_idx += 1)
        {
          DMN_Trap *trap = n->v+n_idx;
          if(trap->flags == 0)
          {
            trap_swap_bytes[trap_idx] = 0xCC;
            dmn_process_read(trap->process, r1u64(trap->vaddr, trap->vaddr+1), trap_swap_bytes+trap_idx);
            U8 int3 = 0xCC;
            dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr+1), &int3);
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: gather all threads which we should run
    //
    DMN_LNX_EntityNode *first_run_thread = 0;
    DMN_LNX_EntityNode *last_run_thread = 0;
    if(need_wait_on_events) ProfScope("gather all threads which we should run")
    {
      //- rjf: scan all processes
      for(DMN_LNX_Entity *process = dmn_lnx_state->entities_base->first;
          process != &dmn_lnx_nil_entity;
          process = process->next)
      {
        if(process->kind != DMN_LNX_EntityKind_Process) {continue;}
        
        //- rjf: determine if this process is frozen
        B32 process_is_frozen = 0;
        if(ctrls->run_entities_are_processes)
        {
          for(U64 idx = 0; idx < ctrls->run_entity_count; idx += 1)
          {
            if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_entity(process)))
            {
              process_is_frozen = 1;
              break;
            }
          }
        }
        
        //- rjf: scan all threads in this process
        for(DMN_LNX_Entity *thread = process->first;
            thread != &dmn_lnx_nil_entity;
            thread = thread->next)
        {
          if(thread->kind != DMN_LNX_EntityKind_Thread) {continue;}
          
          //- rjf: determine if this thread is frozen
          B32 is_frozen = 0;
          {
            // rjf: single-step? freeze if not the single-step thread.
            if(!dmn_handle_match(dmn_handle_zero(), ctrls->single_step_thread))
            {
              is_frozen = !dmn_handle_match(dmn_lnx_handle_from_entity(thread), ctrls->single_step_thread);
            }
            
            // rjf: not single-stepping? determine based on run controls freezing info
            else
            {
              if(ctrls->run_entities_are_processes)
              {
                is_frozen = process_is_frozen;
              }
              else for(U64 idx = 0; idx < ctrls->run_entity_count; idx += 1)
              {
                if(dmn_handle_match(ctrls->run_entities[idx], dmn_lnx_handle_from_entity(thread)))
                {
                  is_frozen = 1;
                  break;
                }
              }
              if(ctrls->run_entities_are_unfrozen)
              {
                is_frozen ^= 1;
              }
            }
          }
          
          //- rjf: disregard all other rules if this is the halter thread
          // TODO(rjf): halting - here is what we do on windows...
#if 0
          if(dmn_w32_shared->halter_tid == thread->id)
          {
            is_frozen = 0;
          }
#endif
          
          //- rjf: add to list
          if(!is_frozen)
          {
            DMN_LNX_EntityNode *n = push_array(scratch.arena, DMN_LNX_EntityNode, 1);
            n->v = thread;
            SLLQueuePush(first_run_thread, last_run_thread, n);
          }
        }
      }
    }
    
    ////////////////////////////
    //- rjf: resume all threads we need to run
    //
    DMN_LNX_EntityNode *first_ran_thread = 0;
    DMN_LNX_EntityNode *last_ran_thread = 0;
    for(DMN_LNX_EntityNode *n = first_run_thread; n != 0; n = n->next)
    {
      ptrace(PTRACE_CONT, (pid_t)n->v->id, 0, 0);
      DMN_LNX_EntityNode *n2 = push_array_no_zero(scratch.arena, DMN_LNX_EntityNode, 1);
      SLLQueuePush(first_ran_thread, last_ran_thread, n2);
      n2->v = n->v;
    }
    
    ////////////////////////////
    //- rjf: loop: wait for next stop, produce debug events
    //
    pid_t final_wait_pid = 0;
    if(need_wait_on_events) for(B32 done = 0; !done;)
    {
      //- rjf: wait for next event
      int status = 0;
      pid_t wait_id = waitpid(-1, &status, __WALL);
      final_wait_pid = wait_id;
      done = 1;
      
      // NOTE(rjf): siginfo hint from old code:
#if 0
      {
        switch(siginfo.si_code)
        {
          // SI_KERNEL (hit int3; 0xCC)
          case 0x80:
          {
            // TODO(rjf): breakpoint event
          }break;
          //                            +----------------------"breakpoint"
          //                            | 
          //                 v----------v----------------------"hardware breakpoint"
          // TRAP_UNK, TRAP_HWBKPT, TRAP_BRKPT, TRAP_TRACE
          case 0x5: case 0x4: case 0x1: case 0x2:
          {
            // TODO(rjf): breakpoint event (?)
          }break;
          case 0x3: case 0x0:
          {
            // TODO(rjf): do nothing(?)
          }break;
        }
      }
#endif
      
      //- rjf: unpack event
      int wifexited         = WIFEXITED(status);
      int wifsignaled       = WIFSIGNALED(status);
      int wifstopped        = WIFSTOPPED(status);
      int wstopsig          = WSTOPSIG(status);
      int ptrace_event_code = (status>>16);
      DMN_LNX_Entity *thread = dmn_lnx_thread_from_pid(wait_id);
      DMN_LNX_Entity *process = thread->parent;
      B32 thread_is_process_root = (thread->id == process->id);
      
      //- rjf: unpack thread's registers
      U64 rip = 0;
      void *regs_block = 0;
      if(thread != &dmn_lnx_nil_entity)
      {
        U64 regs_block_size = regs_block_size_from_arch(thread->arch);
        regs_block = push_array(scratch.arena, U8, regs_block_size);
        dmn_lnx_thread_read_reg_block(thread, regs_block);
        rip = regs_rip_from_arch_block(thread->arch, regs_block);
      }
      
      //- rjf: WIFEXITED(status) -> thread exit
      B32 thread_exit = 0;
      U64 exit_code = 0;
      if(wifexited)
      {
        thread_exit = 1;
      }
      
      //- rjf: WIFEXITED(status) -> thread exit w/ exit code
      else if(wifsignaled)
      {
        exit_code = WTERMSIG(status);
        thread_exit = 1;
      }
      
      //- rjf: SIGTRAP:PTRACE_EVENT_EXIT
      else if(wifstopped && wstopsig == SIGTRAP && ptrace_event_code == PTRACE_EVENT_EXIT)
      {
        // TODO(rjf): verify
        thread_exit = 1;
      }
      
      //- rjf: SIGTRAP:PTRACE_EVENT_CLONE
      else if(wifstopped && wstopsig == SIGTRAP && ptrace_event_code == PTRACE_EVENT_CLONE)
      {
        // TODO(rjf)
      }
      
      //- rjf: SIGTRAP:PTRACE_EVENT_FORK, or SIGTRAP:PTRACE_EVENT_VFORK
      else if(wifstopped && wstopsig == SIGTRAP &&
              (ptrace_event_code == PTRACE_EVENT_FORK ||
               ptrace_event_code == PTRACE_EVENT_VFORK))
      {
      }
      
      //- rjf: SIGTRAP
      else if(wifstopped && wstopsig == SIGTRAP)
      {
        // rjf: this is the single step thread => this is a single step completion
        DMN_EventKind e_kind = DMN_EventKind_Trap;
        if(thread == single_step_thread)
        {
          e_kind = DMN_EventKind_SingleStep;
        }
        
        // rjf: this matches a specified trap => breakpoint
        {
          // TODO(rjf)
        }
        
        // rjf: after breakpoint -> rollback
        if(e_kind == DMN_EventKind_Breakpoint)
        {
          // TODO(rjf)
        }
        
        // rjf: push event
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind                = e_kind;
        e->process             = dmn_lnx_handle_from_entity(process);
        e->thread              = dmn_lnx_handle_from_entity(thread);
        e->instruction_pointer = rip;
      }
      
      //- rjf: WSTOPSIG(status) is SIGSTOP
      else if(wifstopped && wstopsig == SIGSTOP)
      {
        //
        // TODO(rjf): how do we tell the following apart?:
        // - SIGSTOP All-Stop
        // - SIGSTOP Halt
        // - SIGSTOP "User"
        //
        // we are currently just assuming that, if we've queried a SIGSTOP to halt, then
        // the first one that comes back is our "dummy" sigstop. this is likely not
        // necessarily true.
        //
        if(thread->expecting_dummy_sigstop)
        {
          thread->expecting_dummy_sigstop = 0;
          done = 0;
        }
        else if(dmn_lnx_state->has_halt_injection)
        {
          DMN_Event *e = dmn_event_list_push(arena, &evts);
          e->kind    = DMN_EventKind_Halt;
          e->process = dmn_lnx_handle_from_entity(process);
          e->thread  = dmn_lnx_handle_from_entity(thread);
        }
        else
        {
          // TODO(rjf): study this case; old notes:
          //
          // a signal we don't want to mess with (except to record that it
          // happened maybe) we should "hand it back"
        }
      }
      
      //- rjf: WSTOPSIG(status) is an unrecoverable exception (unless user does something to fix state first)
      else if(wifstopped)
      {
        // TODO(rjf): possible cases:
        // SIGABRT
        // SIGFPE
        // SIGSEGV
        // SIGILL
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind                = DMN_EventKind_Exception;
        e->process             = dmn_lnx_handle_from_entity(process);
        e->thread              = dmn_lnx_handle_from_entity(thread);
        e->instruction_pointer = rip;
        e->signo               = wstopsig;
      }
      
      //- rjf: thread exit, thread is process' "root thread" -> eliminate this entire entity subtree
      if(thread_exit && thread_is_process_root)
      {
        // rjf: generate exit-thread / unload-module events
        for(DMN_LNX_Entity *child = process->first; child != &dmn_lnx_nil_entity; child = child->next)
        {
          switch(child->kind)
          {
            default:{}break;
            case DMN_LNX_EntityKind_Thread:
            {
              DMN_Event *e = dmn_event_list_push(arena, &evts);
              e->kind    = DMN_EventKind_ExitThread;
              e->process = dmn_lnx_handle_from_entity(process);
              e->thread  = dmn_lnx_handle_from_entity(child);
            }break;
            case DMN_LNX_EntityKind_Module:
            {
              DMN_Event *e = dmn_event_list_push(arena, &evts);
              e->kind    = DMN_EventKind_UnloadModule;
              e->process = dmn_lnx_handle_from_entity(process);
              e->module  = dmn_lnx_handle_from_entity(child);
              // TODO(rjf): e->string = ...;
            }break;
          }
        }
        
        // rjf: generate exit process event
        {
          DMN_Event *e = dmn_event_list_push(arena, &evts);
          e->kind    = DMN_EventKind_ExitProcess;
          e->process = dmn_lnx_handle_from_entity(process);
          e->code    = exit_code;
        }
        
        // rjf: eliminate entity tree
        dmn_lnx_entity_release(process);
      }
      
      //- rjf: thread exit, thread is *not* process root -> just exit this one thread
      if(thread_exit && !thread_is_process_root)
      {
        DMN_Event *e = dmn_event_list_push(arena, &evts);
        e->kind    = DMN_EventKind_ExitThread;
        e->process = dmn_lnx_handle_from_entity(process);
        e->thread  = dmn_lnx_handle_from_entity(thread);
        dmn_lnx_entity_release(thread);
      }
    }
    
    ////////////////////////////
    //- rjf: stop all threads
    //
    for(DMN_LNX_EntityNode *n = first_ran_thread; n != 0; n = n->next)
    {
      DMN_LNX_Entity *thread = n->v;
      pid_t thread_id = (pid_t)thread->id;
      if(thread_id != final_wait_pid)
      {
        union sigval sv = {0};
        sigqueue(thread_id, SIGSTOP, sv);
        thread->expecting_dummy_sigstop = 1;
      }
    }
    
    //////////////////////////
    //- rjf: restore original memory at trap locations
    //
    ProfScope("restore original memory at trap locations")
    {
      U64 trap_idx = 0;
      for(DMN_TrapChunkNode *n = ctrls->traps.first; n != 0; n = n->next)
      {
        for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, trap_idx += 1)
        {
          DMN_Trap *trap = n->v+n_idx;
          if(trap->flags == 0)
          {
            U8 og_byte = trap_swap_bytes[trap_idx];
            if(og_byte != 0xCC)
            {
              dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr+1), &og_byte);
            }
          }
        }
      }
    }
    
    scratch_end(scratch);
  }
  return evts;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Halting (Implemented Per-OS)

internal void
dmn_halt(U64 code, U64 user_data)
{
  if(!dmn_lnx_state->has_halt_injection)
  {
    DMN_LNX_Entity *process = dmn_lnx_state->entities_base->first;
    if(process != &dmn_lnx_nil_entity)
    {
      union sigval sv = {0};
      if(sigqueue(process->id, SIGSTOP, sv) != -1)
      {
        dmn_lnx_state->has_halt_injection = 1;
        dmn_lnx_state->halt_code          = code;
        dmn_lnx_state->halt_user_data     = user_data;
      }
    }
  }
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Introspection Functions (Implemented Per-OS)

//- rjf: non-blocking-control-thread access barriers

internal B32
dmn_access_open(void)
{
  B32 result = 0;
  if(dmn_lnx_ctrl_thread)
  {
    result = 1;
  }
  else
  {
    os_mutex_take(dmn_lnx_state->access_mutex);
    result = !dmn_lnx_state->access_run_state;
  }
  return result;
}

internal void
dmn_access_close(void)
{
  if(!dmn_lnx_ctrl_thread)
  {
    os_mutex_drop(dmn_lnx_state->access_mutex);
  }
}

//- rjf: processes

internal U64
dmn_process_memory_reserve(DMN_Handle process, U64 vaddr, U64 size)
{
  return 0;
}

internal void
dmn_process_memory_commit(DMN_Handle process, U64 vaddr, U64 size)
{
}

internal void
dmn_process_memory_decommit(DMN_Handle process, U64 vaddr, U64 size)
{
}

internal void
dmn_process_memory_release(DMN_Handle process, U64 vaddr, U64 size)
{
}

internal void
dmn_process_memory_protect(DMN_Handle process, U64 vaddr, U64 size, OS_AccessFlags flags)
{
}

internal U64
dmn_process_read(DMN_Handle process, Rng1U64 range, void *dst)
{
  DMN_LNX_Entity *entity = dmn_lnx_entity_from_handle(process);
  U64 result = dmn_lnx_read(entity->fd, range, dst);
  return result;
}

internal B32
dmn_process_write(DMN_Handle process, Rng1U64 range, void *src)
{
  DMN_LNX_Entity *entity = dmn_lnx_entity_from_handle(process);
  B32 result = dmn_lnx_write(entity->fd, range, src);
  return result;
}

//- rjf: threads

internal Arch
dmn_arch_from_thread(DMN_Handle handle)
{
  DMN_LNX_Entity *thread = dmn_lnx_entity_from_handle(handle);
  return thread->arch;
}

internal U64
dmn_stack_base_vaddr_from_thread(DMN_Handle handle)
{
  return 0;
}

internal U64
dmn_tls_root_vaddr_from_thread(DMN_Handle handle)
{
  return 0;
}

internal B32
dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Entity *thread = dmn_lnx_entity_from_handle(handle);
    result = dmn_lnx_thread_read_reg_block(thread, reg_block);
  }
  return result;
}

internal B32
dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block)
{
  B32 result = 0;
  DMN_AccessScope
  {
    DMN_LNX_Entity *thread = dmn_lnx_entity_from_handle(handle);
    result = dmn_lnx_thread_write_reg_block(thread, reg_block);
  }
  return result;
}

//- rjf: system process listing

internal void
dmn_process_iter_begin(DMN_ProcessIter *iter)
{
  DIR *dir = opendir("/proc");
  MemoryZeroStruct(iter);
  iter->v[0] = IntFromPtr(dir);
}

internal B32
dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out)
{
  // rjf: scan for the next process ID in the directory
  B32 got_pid = 0;
  String8 pid_string = {0};
  {
    DIR *dir = (DIR*)PtrFromInt(iter->v[0]);
    if(dir != 0 && iter->v[1] == 0)
    {
      for(;;)
      {
        // rjf: get next entry
        struct dirent *d = readdir(dir);
        if(d == 0)
        {
          break;
        }
        
        // rjf: check file name is integer
        String8 file_name = str8_cstring((char*)d->d_name);
        B32 is_integer = str8_is_integer(file_name, 10);
        
        // rjf: break on integers (which represent processes)
        if(is_integer)
        {
          got_pid = 1;
          pid_string = file_name;
          break;
        }
      }
    }
  }
  
  // rjf: if we found a process id, map id => info
  B32 result = 0;
  if(got_pid)
  {
    pid_t pid = u64_from_str8(pid_string, 10);
    String8 name = dmn_lnx_exe_path_from_pid(arena, pid);
    if(name.size == 0)
    {
      name = str8_lit("(unknown process)");
    }
    info_out->name = name;
    info_out->pid = pid;
    result = 1;
  }
  
  return result;
}

internal void
dmn_process_iter_end(DMN_ProcessIter *iter)
{
  DIR *dir = (DIR*)PtrFromInt(iter->v[0]);
  if(dir != 0)
  {
    closedir(dir);
  }
  MemoryZeroStruct(iter);
}
