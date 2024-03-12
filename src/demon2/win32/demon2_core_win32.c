// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
dmn_w32_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal U64
dmn_w32_hash_from_id(U64 id)
{
  return dmn_w32_hash_from_string(str8_struct(&id));
}

////////////////////////////////
//~ rjf: Entity Helpers

//- rjf: entity <-> handle

internal DMN_Handle
dmn_w32_handle_from_entity(DMN_W32_Entity *entity)
{
  U32 idx = (U32)(entity - dmn_w32_shared->entities_base);
  U32 gen = entity->gen;
  DMN_Handle handle = {idx, gen};
  return handle;
}

internal DMN_W32_Entity *
dmn_w32_entity_from_handle(DMN_Handle handle)
{
  U32 idx = handle.u32[0];
  U32 gen = handle.u32[1];
  DMN_W32_Entity *entity = dmn_w32_shared->entities_base;
  if(entity->gen != gen)
  {
    entity = &dmn_w32_entity_nil;
  }
  return entity;
}

//- rjf: entity allocation/deallocation

internal DMN_W32_Entity *
dmn_w32_entity_alloc(DMN_W32_Entity *parent, DMN_W32_EntityKind kind, U64 id)
{
  // rjf: allocate
  DMN_W32_Entity *e = dmn_w32_shared->entities_first_free;
  {
    if(e != 0)
    {
      SLLStackPop(dmn_w32_shared->entities_first_free);
    }
    else
    {
      e = push_array_no_zero(dmn_w32_shared->entities_arena, DMN_W32_Entity, 1);
      dmn_w32_shared->entities_count += 1;
    }
    U32 gen = e->gen;
    MemoryZeroStruct(e);
    e->gen = gen+1;
  }
  
  // rjf: fill
  {
    e->kind = kind;
    e->id = id;
    e->parent = parent;
    e->next = e->prev = e->first = e->last = &dmn_w32_entity_nil;
    if(parent != &dmn_w32_entity_nil)
    {
      DLLPushBack_NPZ(&dmn_w32_entity_nil, parent->first, parent->last, e, next, prev);
    }
  }
  
  // rjf: insert into id -> entity map
  if(id != 0)
  {
    U64 hash = dmn_w32_hash_from_id(id);
    U64 slot_idx = hash%dmn_w32_shared->entities_id_hash_slots_count;
    DMN_W32_EntityIDHashSlot *slot = &dmn_w32_shared->entities_id_hash_slots[slot_idx];
    DMN_W32_EntityIDHashNode *node = 0;
    for(DMN_W32_EntityIDHashNode *n = slot->first; n != 0; n = n->next)
    {
      if(n->id == id)
      {
        node = n;
        break;
      }
    }
    if(node == 0)
    {
      node = dmn_w32_shared->entities_id_hash_node_free;
      if(node != 0)
      {
        SLLStackPop(dmn_w32_shared->entities_id_hash_node_free);
      }
      else
      {
        node = push_array(dmn_w32_shared->arena, DMN_W32_EntityIDHashNode, 1);
      }
      DLLPushBack(slot->first, slot->last, node);
    }
    node->id = id;
    node->entity = e;
  }
  
  return e;
}

internal void
dmn_w32_entity_release(DMN_W32_Entity *entity)
{
  // rjf: unhook root
  if(entity->parent != &dmn_w32_entity_nil)
  {
    DLLRemove_NPZ(&dmn_w32_entity_nil, entity->parent->first, entity->parent->last, entity, next, prev);
  }
  
  // rjf: walk every entity in this tree, free each
  if(entity != &dmn_w32_entity_nil)
  {
    Temp scratch = scratch_begin(0, 0);
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      DMN_W32_Entity *e;
    };
    Task start_task = {0, entity};
    Task *first_task = &start_task;
    Task *last_task = &start_task;
    for(Task *t = first_task; t != 0; t = t->next)
    {
      for(DMN_W32_Entity *child = entity->first; child != &dmn_w32_entity_nil; child = child->next)
      {
        Task *t = push_array(scratch.arena, Task, 1);
        t->e = child;
        SLLQueuePush(first_task, last_task, t);
      }
      
      // rjf: free entity
      SLLStackPush(dmn_w32_shared->entities_first_free, t->e);
      t->e->gen += 1;
      
      // rjf: remove from id -> entity map
      if(t->e->id != 0)
      {
        U64 hash = dmn_w32_hash_from_id(t->e->id);
        U64 slot_idx = hash%dmn_w32_shared->entities_id_hash_slots_count;
        DMN_W32_EntityIDHashSlot *slot = &dmn_w32_shared->entities_id_hash_slots[slot_idx];
        DMN_W32_EntityIDHashNode *node = 0;
        for(DMN_W32_EntityIDHashNode *n = slot->first; n != 0; n = n->next)
        {
          if(n->id == t->e->id && n->entity == t->e)
          {
            DLLRemove(slot->first, slot->last, n);
            SLLStackPush(dmn_w32_shared->entities_id_hash_node_free, n);
            break;
          }
        }
      }
    }
    scratch_end(scratch);
  }
}

//- rjf: kind*id -> entity

internal DMN_W32_Entity *
dmn_w32_entity_from_kind_id(DMN_W32_EntityKind kind, U64 id)
{
  DMN_W32_Entity *result = &dmn_w32_entity_nil;
  U64 hash = dmn_w32_hash_from_id(id);
  U64 slot_idx = hash%dmn_w32_shared->entities_id_hash_slots_count;
  DMN_W32_EntityIDHashSlot *slot = &dmn_w32_shared->entities_id_hash_slots[slot_idx];
  DMN_W32_EntityIDHashNode *node = 0;
  for(DMN_W32_EntityIDHashNode *n = slot->first; n != 0; n = n->next)
  {
    if(n->entity->kind == kind && n->id == id)
    {
      node = n;
      break;
    }
  }
  if(node != 0)
  {
    result = node->entity;
  }
  return result;
}

////////////////////////////////
//~ rjf: Module Info Extraction

internal String8
dmn_w32_full_path_from_module(Arena *arena, DMN_W32_Entity *module)
{
  Temp scratch = scratch_begin(&arena, 1);
  
  //- rjf: extract path from module
  String16 path16 = {0};
  String8 path8 = {0};
  {
    // rjf: handle -> full path
    if(module->handle != 0)
    {
      DWORD cap16 = GetFinalPathNameByHandleW(module->handle, 0, 0, VOLUME_NAME_DOS);
      U16 *buffer16 = push_array_no_zero(scratch.arena, U16, cap16);
      DWORD size16 = GetFinalPathNameByHandleW(module->handle, (WCHAR*)buffer16, cap16, VOLUME_NAME_DOS);
      path16 = str16(buffer16, size16);
    }
    
    // rjf: fallback (main module only): process -> full path
    if(path16.size == 0 && module->module.is_main)
    {
      DMN_W32_Entity *process = module->parent;
      DWORD size = KB(4);
      U16 *buf = push_array_no_zero(scratch.arena, U16, size);
      if(QueryFullProcessImageNameW(process->handle, 0, (WCHAR*)buf, &size))
      {
        path16 = str16(buf, size);
      }
    }
    
    // rjf: fallback (any module - no guarantee): address_of_name -> full path
    if(path16.size == 0 && module->module.address_of_name_pointer != 0)
    {
      DMN_W32_Entity *process = module->parent;
      U64 ptr_size = bit_size_from_arch(process->arch)/8;
      U64 name_pointer = 0;
      if(dmn_w32_process_read(process->handle, r1u64(module->module.address_of_name_pointer, module->module.address_of_name_pointer+ptr_size), &name_pointer))
      {
        if(name_pointer != 0)
        {
          if(module->module.name_is_unicode)
          {
            path16 = dmn_w32_read_memory_str16(scratch.arena, process->handle, name_pointer);
          }
          else
          {
            path8 = dmn_w32_read_memory_str(scratch.arena, process->handle, name_pointer);
          }
        }
      }
    }
  }
  
  // rjf: produce finalized result
  String8 result = {0};
  {
    if(path16.size > 0)
    {
      // rjf: skip the extended path thing if necessary
      if(path16.size >= 4 &&
         path16.str[0] == L'\\' &&
         path16.str[1] == L'\\' &&
         path16.str[2] == L'?' &&
         path16.str[3] == L'\\')
      {
        path16.size -= 4;
        path16.str += 4;
      }
      
      // rjf: convert to UTF-8
      result = str8_from_16(arena, path16);
    }
    else
    {
      // rjf: skip the extended path thing if necessary
      if (path8.size >= 4 &&
          path8.str[0] == L'\\' &&
          path8.str[1] == L'\\' &&
          path8.str[2] == L'?' &&
          path8.str[3] == L'\\')
      {
        path8.size -= 4;
        path8.str += 4;
      }
      
      // rjf: copy to output arena
      result = push_str8_copy(arena, path8);
    }
  }
  
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Win32-Level Process/Thread Reads/Writes

//- rjf: processes

internal U64
dmn_w32_process_read(HANDLE process, Rng1U64 range, void *dst)
{
  U64 bytes_read = 0;
  U8 *ptr = (U8*)dst;
  U8 *opl = ptr + dim_1u64(range);
  U64 cursor = range.min;
  for(;ptr < opl;)
  {
    SIZE_T to_read = (SIZE_T)(opl - ptr);
    SIZE_T actual_read = 0;
    if(!ReadProcessMemory(process, (LPCVOID)cursor, ptr, to_read, &actual_read))
    {
      bytes_read += actual_read;
      break;
    }
    ptr += actual_read;
    cursor += actual_read;
    bytes_read += actual_read;
  }
  return bytes_read;
}

internal B32
dmn_w32_process_write(HANDLE process, Rng1U64 range, void *src)
{
  B32 result = 1;
  U8 *ptr = (U8*)src;
  U8 *opl = ptr + dim_1u64(range);
  U64 cursor = range.min;
  for(;ptr < opl;)
  {
    SIZE_T to_write = (SIZE_T)(opl - ptr);
    SIZE_T actual_write = 0;
    if(!WriteProcessMemory(process, (LPVOID)cursor, ptr, to_write, &actual_write))
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
dmn_w32_read_memory_str(Arena *arena, HANDLE process_handle, U64 address)
{
  // TODO(rjf): @rewrite
  // 
  // OLD: this could be done better with a demon_w32_read_memory
  // that returns a read amount instead of a success/fail.
  //
  // (dmn_w32_process_read now does this, so we can switch to it)
  
  // scan piece by piece
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  U64 max_cap = 256;
  U64 cap = max_cap;
  U64 read_p = address;
  for (;;){
    U8 *block = push_array(scratch.arena, U8, cap);
    for (;cap > 0;){
      if (dmn_w32_process_read(process_handle, r1u64(read_p, read_p+cap), block)){
        break;
      }
      cap /= 2;
    }
    read_p += cap;
    
    U64 block_opl = 0;
    for (;block_opl < cap; block_opl += 1){
      if (block[block_opl] == 0){
        break;
      }
    }
    
    if (block_opl > 0){
      str8_list_push(scratch.arena, &list, str8(block, block_opl));
    }
    
    if (block_opl < cap || cap == 0){
      break;
    }
  }
  
  // assemble results
  String8 result = str8_list_join(arena, &list, 0);
  scratch_end(scratch);
  return(result);
}

internal String16
dmn_w32_read_memory_str16(Arena *arena, HANDLE process_handle, U64 address)
{
  // TODO(rjf): @rewrite
  // 
  // OLD: this could be done better with a demon_w32_read_memory
  // that returns a read amount instead of a success/fail.
  //
  // (dmn_w32_process_read now does this, so we can switch to it)
  
  // scan piece by piece
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  
  U64 max_cap = 256;
  U64 cap = max_cap;
  U64 read_p = address;
  for (;;){
    U8 *block = push_array(scratch.arena, U8, cap);
    for (;cap > 1;){
      if (dmn_w32_process_read(process_handle, r1u64(read_p, read_p+cap), block)){
        break;
      }
      cap /= 2;
    }
    read_p += cap;
    
    U16 *block16 = (U16*)block;
    (void)block16;
    U64 block_opl = 0;
    for (;block_opl < cap; block_opl += 2){
      if (*(U16*)(block + block_opl) == 0){
        break;
      }
    }
    
    if (block_opl > 0){
      str8_list_push(scratch.arena, &list, str8(block, block_opl));
    }
    
    if (block_opl < cap || cap == 0){
      break;
    }
  }
  
  // assemble results
  String8 joined = str8_list_join(arena, &list, 0);
  String16 result = {(U16*)joined.str, joined.size/2};
  scratch_end(scratch);
  return(result);
}

internal DMN_W32_ImageInfo
dmn_w32_image_info_from_process_base_vaddr(HANDLE process, U64 base_vaddr)
{
  // rjf: find PE offset
  U32 pe_offset = 0;
  {
    U64 dos_magic_off = base_vaddr;
    U16 dos_magic = 0;
    dmn_w32_process_read_struct(process, dos_magic_off, &dos_magic);
    if(dos_magic == PE_DOS_MAGIC)
    {
      U64 pe_offset_off = base_vaddr + OffsetOf(PE_DosHeader, coff_file_offset);
      dmn_w32_process_read_struct(process, pe_offset_off, &pe_offset);
    }
  }
  
  // rjf: get COFF header
  B32 got_coff_header = 0;
  U64 coff_header_off = 0;
  COFF_Header coff_header = {0};
  if(pe_offset > 0)
  {
    U64 pe_magic_off = base_vaddr + pe_offset;
    U32 pe_magic = 0;
    dmn_w32_process_read_struct(process, pe_magic_off, &pe_magic);
    if(pe_magic == PE_MAGIC)
    {
      coff_header_off = pe_magic_off + sizeof(pe_magic);
      if(dmn_w32_process_read_struct(process, coff_header_off, &coff_header))
      {
        got_coff_header = 1;
      }
    }
  }
  
  // rjf: get arch and size
  DMN_W32_ImageInfo result = zero_struct;
  if(got_coff_header)
  {
    U64 optional_size_off = 0;
    Architecture arch = Architecture_Null;
    switch(coff_header.machine)
    {
      case COFF_MachineType_X86:
      {
        arch = Architecture_x86;
        optional_size_off = OffsetOf(PE_OptionalHeader32, sizeof_image);
      }break;
      case COFF_MachineType_X64:
      {
        arch = Architecture_x64;
        optional_size_off = OffsetOf(PE_OptionalHeader32Plus, sizeof_image);
      }break;
      default:
      {}break;
    }
    if(arch != Architecture_Null)
    {
      U64 optional_off = coff_header_off + sizeof(coff_header);
      U32 size = 0;
      if(dmn_w32_process_read_struct(process, optional_off+optional_size_off, &size) >= sizeof(size))
      {
        result.arch = arch;
        result.size = size;
      }
    }
  }
  
  return result;
}

//- rjf: threads

internal U16
dmn_w32_real_tag_word_from_xsave(XSAVE_FORMAT *fxsave)
{
  U16 result = 0;
  U32 top = (fxsave->StatusWord >> 11) & 7;
  for(U32 fpr = 0; fpr < 8; fpr += 1)
  {
    U32 tag = 3;
    if(fxsave->TagWord & (1 << fpr))
    {
      U32 st = (fpr - top)&7;
      
      REGS_Reg80 *fp = (REGS_Reg80*)&fxsave->FloatRegisters[st*16];
      U16 exponent = fp->sign1_exp15 & bitmask15;
      U64 integer_part  = fp->int1_frac63 >> 63;
      U64 fraction_part = fp->int1_frac63 & bitmask63;
      
      // tag: 0 - normal; 1 - zero; 2 - special
      tag = 2;
      if(exponent == 0)
      {
        if(integer_part == 0 && fraction_part == 0)
        {
          tag = 1;
        }
      }
      else if(exponent != bitmask15 && integer_part != 0)
      {
        tag = 0;
      }
    }
    result |= tag << (2 * fpr);
  }
  return result;
}

internal U16
dmn_w32_xsave_tag_word_from_real_tag_word(U16 ftw)
{
  U16 compact = 0;
  for(U32 fpr = 0; fpr < 8; fpr++)
  {
    U32 tag = (ftw >> (fpr * 2)) & 3;
    if(tag != 3)
    {
      compact |= (1 << fpr);
    }
  }
  return compact;
}

internal B32
dmn_w32_thread_read_reg_block(Architecture arch, HANDLE thread, void *reg_block)
{
  B32 result = 0;
  switch(arch)
  {
    ////////////////////////////
    //- rjf: unimplemented win32/arch combos
    //
    case Architecture_Null:
    case Architecture_arm64:
    case Architecture_arm32:
    case Architecture_COUNT:
    {NotImplemented;}break;
    
    ////////////////////////////
    //- rjf: x86
    //
    case Architecture_x86:
    {
      REGS_RegBlockX86 *dst = (REGS_RegBlockX86 *)reg_block;
      
      //- rjf: get thread context
      WOW64_CONTEXT ctx = {0};
      ctx.ContextFlags = DMN_W32_CTX_X86_ALL;
      if(!Wow64GetThreadContext(thread, (WOW64_CONTEXT *)&ctx))
      {
        break;
      }
      result = 1;
      
      //- rjf: convert WOW64_CONTEXT -> REGS_RegBlockX86
      XSAVE_FORMAT *fxsave = (XSAVE_FORMAT *)ctx.ExtendedRegisters;
      dst->eax.u32 = ctx.Eax;
      dst->ebx.u32 = ctx.Ebx;
      dst->ecx.u32 = ctx.Ecx;
      dst->edx.u32 = ctx.Edx;
      dst->esi.u32 = ctx.Esi;
      dst->edi.u32 = ctx.Edi;
      dst->esp.u32 = ctx.Esp;
      dst->ebp.u32 = ctx.Ebp;
      dst->eip.u32 = ctx.Eip;
      dst->cs.u16 = ctx.SegCs;
      dst->ds.u16 = ctx.SegDs;
      dst->es.u16 = ctx.SegEs;
      dst->fs.u16 = ctx.SegFs;
      dst->gs.u16 = ctx.SegGs;
      dst->ss.u16 = ctx.SegSs;
      dst->dr0.u32 = ctx.Dr0;
      dst->dr1.u32 = ctx.Dr1;
      dst->dr2.u32 = ctx.Dr2;
      dst->dr3.u32 = ctx.Dr3;
      dst->dr6.u32 = ctx.Dr6;
      dst->dr7.u32 = ctx.Dr7;
      // NOTE(rjf): this bit is "supposed to always be 1", according to old info.
      // may need to be investigated.
      dst->eflags.u32 = ctx.EFlags | 0x2;
      dst->fcw.u16 = fxsave->ControlWord;
      dst->fsw.u16 = fxsave->StatusWord;
      dst->ftw.u16 = dmn_w32_real_tag_word_from_xsave(fxsave);
      dst->fop.u16 = fxsave->ErrorOpcode;
      dst->fip.u32 = fxsave->ErrorOffset;
      dst->fcs.u16 = fxsave->ErrorSelector;
      dst->fdp.u32 = fxsave->DataOffset;
      dst->fds.u16 = fxsave->DataSelector;
      dst->mxcsr.u32 = fxsave->MxCsr;
      dst->mxcsr_mask.u32 = fxsave->MxCsr_Mask;
      {
        M128A *float_s = fxsave->FloatRegisters;
        REGS_Reg80 *float_d = &dst->fpr0;
        for(U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1)
        {
          MemoryCopy(float_d, float_s, sizeof(*float_d));
        }
      }
      {
        M128A *xmm_s = fxsave->XmmRegisters;
        REGS_Reg256 *xmm_d = &dst->ymm0;
        for(U32 n = 0; n < 8; n += 1, xmm_s += 1, xmm_d += 1)
        {
          MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_s));
        }
      }
      
      //- rjf: read FS/GS base
      WOW64_LDT_ENTRY ldt = {0};
      if(Wow64GetThreadSelectorEntry(thread, ctx.SegFs, &ldt))
      {
        U32 base = (ldt.BaseLow) | (ldt.HighWord.Bytes.BaseMid << 16) | (ldt.HighWord.Bytes.BaseHi << 24);
        dst->fsbase.u32 = base;
      }
      if(Wow64GetThreadSelectorEntry(thread, ctx.SegGs, &ldt))
      {
        U32 base = (ldt.BaseLow) | (ldt.HighWord.Bytes.BaseMid << 16) | (ldt.HighWord.Bytes.BaseHi << 24);
        dst->gsbase.u32 = base;
      }
    }break;
    
    ////////////////////////////
    //- rjf: x64
    //
    case Architecture_x64:
    {
      Temp scratch = scratch_begin(0, 0);
      REGS_RegBlockX64 *dst = (REGS_RegBlockX64 *)reg_block;
      
      //- rjf: unpack info about available features
      U32 feature_mask = GetEnabledXStateFeatures();
      B32 avx_enabled = !!(feature_mask & XSTATE_MASK_AVX);
      
      //- rjf: set up context
      CONTEXT *ctx = 0;
      U32 ctx_flags = DMN_W32_CTX_X64_ALL;
      if(avx_enabled)
      {
        ctx_flags |= DMN_W32_CTX_INTEL_XSTATE;
      }
      DWORD size = 0;
      InitializeContext(0, ctx_flags, 0, &size);
      if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        void *ctx_memory = push_array(scratch.arena, U8, size);
        if(!InitializeContext(ctx_memory, ctx_flags, &ctx, &size))
        {
          ctx = 0;
        }
      }
      
      //- rjf: unpack features available on this context
      B32 avx_available = 0;
      if(ctx != 0)
      {
        if(avx_enabled)
        {
          SetXStateFeaturesMask(ctx, XSTATE_MASK_AVX);
        }
        DWORD64 xstate_flags = 0;
        if(GetXStateFeaturesMask(ctx, &xstate_flags))
        {
          avx_available = !!(xstate_flags & XSTATE_MASK_AVX);
        }
      }
      
      //- rjf: get thread context
      if(!GetThreadContext(thread, ctx))
      {
        ctx = 0;
      }
      
      //- rjf: bad context -> abort
      if(ctx == 0)
      {
        break;
      }
      result = 1;
      
      //- rjf: convert context -> REGS_RegBlockX64
      XSAVE_FORMAT *xsave = &ctx->FltSave;
      dst->rax.u64 = ctx->Rax;
      dst->rcx.u64 = ctx->Rcx;
      dst->rdx.u64 = ctx->Rdx;
      dst->rbx.u64 = ctx->Rbx;
      dst->rsp.u64 = ctx->Rsp;
      dst->rbp.u64 = ctx->Rbp;
      dst->rsi.u64 = ctx->Rsi;
      dst->rdi.u64 = ctx->Rdi;
      dst->r8.u64  = ctx->R8;
      dst->r9.u64  = ctx->R9;
      dst->r10.u64 = ctx->R10;
      dst->r11.u64 = ctx->R11;
      dst->r12.u64 = ctx->R12;
      dst->r13.u64 = ctx->R13;
      dst->r14.u64 = ctx->R14;
      dst->r15.u64 = ctx->R15;
      dst->rip.u64 = ctx->Rip;
      dst->cs.u16  = ctx->SegCs;
      dst->ds.u16  = ctx->SegDs;
      dst->es.u16  = ctx->SegEs;
      dst->fs.u16  = ctx->SegFs;
      dst->gs.u16  = ctx->SegGs;
      dst->ss.u16  = ctx->SegSs;
      dst->dr0.u32 = ctx->Dr0;
      dst->dr1.u32 = ctx->Dr1;
      dst->dr2.u32 = ctx->Dr2;
      dst->dr3.u32 = ctx->Dr3;
      dst->dr6.u32 = ctx->Dr6;
      dst->dr7.u32 = ctx->Dr7;
      // NOTE(rjf): this bit is "supposed to always be 1", according to old info.
      // may need to be investigated.
      dst->rflags.u64 = ctx->EFlags | 0x2;
      dst->fcw.u16 = xsave->ControlWord;
      dst->fsw.u16 = xsave->StatusWord;
      dst->ftw.u16 = dmn_w32_real_tag_word_from_xsave(xsave);
      dst->fop.u16 = xsave->ErrorOpcode;
      dst->fcs.u16 = xsave->ErrorSelector;
      dst->fds.u16 = xsave->DataSelector;
      dst->fip.u32 = xsave->ErrorOffset;
      dst->fdp.u32 = xsave->DataOffset;
      dst->mxcsr.u32 = xsave->MxCsr;
      dst->mxcsr_mask.u32 = xsave->MxCsr_Mask;
      {
        M128A *float_s = xsave->FloatRegisters;
        REGS_Reg80 *float_d = &dst->fpr0;
        for(U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1)
        {
          MemoryCopy(float_d, float_s, sizeof(*float_d));
        }
      }
      if(!avx_available)
      {
        M128A *xmm_s = xsave->XmmRegisters;
        REGS_Reg256 *xmm_d = &dst->ymm0;
        for(U32 n = 0; n < 16; n += 1, xmm_s += 1, xmm_d += 1)
        {
          MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_s));
        }
      }
      if(avx_available)
      {
        DWORD part0_length = 0;
        M128A *part0 = (M128A*)LocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &part0_length);
        DWORD part1_length = 0;
        M128A *part1 = (M128A*)LocateXStateFeature(ctx, XSTATE_AVX, &part1_length);
        Assert(part0_length == part1_length);
        DWORD count = part0_length/sizeof(part0[0]);
        count = ClampTop(count, 16);
        REGS_Reg256 *ymm_d = &dst->ymm0;
        for (DWORD i = 0; i < count; i += 1, part0 += 1, part1 += 1, ymm_d += 1)
        {
          // TODO(rjf): confirm ordering of writes
          ymm_d->u64[3] = part0->Low;
          ymm_d->u64[2] = part0->High;
          ymm_d->u64[1] = part1->Low;
          ymm_d->u64[0] = part1->High;
        }
      }
      
      scratch_end(scratch);
    }break;
  }
  return result;
}

internal B32
dmn_w32_thread_write_reg_block(Architecture arch, HANDLE thread, void *reg_block)
{
  B32 result = 0;
  switch(arch)
  {
    ////////////////////////////
    //- rjf: unimplemented win32/arch combos
    //
    case Architecture_Null:
    case Architecture_arm64:
    case Architecture_arm32:
    case Architecture_COUNT:
    {NotImplemented;}break;
    
    ////////////////////////////
    //- rjf: x86
    //
    case Architecture_x86:
    {
      REGS_RegBlockX86 *src = (REGS_RegBlockX86 *)reg_block;
      
      //- rjf: convert REGS_RegBlockX86 -> WOW64_CONTEXT
      WOW64_CONTEXT ctx = {0};
      XSAVE_FORMAT *fxsave = (XSAVE_FORMAT*)ctx.ExtendedRegisters;
      ctx.ContextFlags = DMN_W32_CTX_X86_ALL;
      ctx.Eax = src->eax.u32;
      ctx.Ebx = src->ebx.u32;
      ctx.Ecx = src->ecx.u32;
      ctx.Edx = src->edx.u32;
      ctx.Esi = src->esi.u32;
      ctx.Edi = src->edi.u32;
      ctx.Esp = src->esp.u32;
      ctx.Ebp = src->ebp.u32;
      ctx.Eip = src->eip.u32;
      ctx.SegCs = src->cs.u16;
      ctx.SegDs = src->ds.u16;
      ctx.SegEs = src->es.u16;
      ctx.SegFs = src->fs.u16;
      ctx.SegGs = src->gs.u16;
      ctx.SegSs = src->ss.u16;
      ctx.Dr0 = src->dr0.u32;
      ctx.Dr1 = src->dr1.u32;
      ctx.Dr2 = src->dr2.u32;
      ctx.Dr3 = src->dr3.u32;
      ctx.Dr6 = src->dr6.u32;
      ctx.Dr7 = src->dr7.u32;
      ctx.EFlags = src->eflags.u32;
      fxsave->ControlWord = src->fcw.u16;
      fxsave->StatusWord = src->fsw.u16;
      fxsave->TagWord = dmn_w32_xsave_tag_word_from_real_tag_word(src->ftw.u16);
      fxsave->ErrorOpcode = src->fop.u16;
      fxsave->ErrorSelector = src->fcs.u16;
      fxsave->DataSelector = src->fds.u16;
      fxsave->ErrorOffset = src->fip.u32;
      fxsave->DataOffset = src->fdp.u32;
      fxsave->MxCsr = src->mxcsr.u32 & src->mxcsr_mask.u32;
      fxsave->MxCsr_Mask = src->mxcsr_mask.u32;
      {
        M128A *float_d = fxsave->FloatRegisters;
        REGS_Reg80 *float_s = &src->fpr0;
        for(U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1)
        {
          MemoryCopy(float_d, float_s, 10);
        }
      }
      {
        M128A *xmm_d = fxsave->XmmRegisters;
        REGS_Reg256 *xmm_s = &src->ymm0;
        for(U32 n = 0; n < 8; n += 1, xmm_d += 1, xmm_s += 1)
        {
          MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_d));
        }
      }
      
      //- rjf: set thread context
      B32 result = 0;
      if(Wow64SetThreadContext(thread, &ctx))
      {
        result = 1;
      }
    }break;
    
    ////////////////////////////
    //- rjf: x64
    //
    case Architecture_x64:
    {
      Temp scratch = scratch_begin(0, 0);
      REGS_RegBlockX64 *src = (REGS_RegBlockX64 *)reg_block;
      
      //- rjf: unpack info about available features
      U32 feature_mask = GetEnabledXStateFeatures();
      B32 avx_enabled = !!(feature_mask & XSTATE_MASK_AVX);
      
      //- rjf: set up context
      CONTEXT *ctx = 0;
      U32 ctx_flags = DMN_W32_CTX_X64_ALL;
      if(avx_enabled)
      {
        ctx_flags |= DMN_W32_CTX_INTEL_XSTATE;
      }
      DWORD size = 0;
      InitializeContext(0, ctx_flags, 0, &size);
      if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        void *ctx_memory = push_array(scratch.arena, U8, size);
        if(!InitializeContext(ctx_memory, ctx_flags, &ctx, &size))
        {
          ctx = 0;
        }
      }
      
      //- rjf: unpack features available on this context
      B32 avx_available = 0;
      if(ctx != 0)
      {
        if(avx_enabled)
        {
          SetXStateFeaturesMask(ctx, XSTATE_MASK_AVX);
        }
        DWORD64 xstate_flags = 0;
        if(GetXStateFeaturesMask(ctx, &xstate_flags))
        {
          avx_available = !!(xstate_flags & XSTATE_MASK_AVX);
        }
      }
      
      //- rjf: get thread context
      if(!GetThreadContext(thread, ctx))
      {
        ctx = 0;
      }
      
      //- rjf: bad context -> abort
      if(ctx == 0)
      {
        break;
      }
      
      //- rjf: convert REGS_RegBlockX64 -> CONTEXT
      XSAVE_FORMAT *fxsave = &ctx->FltSave;
      ctx->ContextFlags = ctx_flags;
      ctx->MxCsr = src->mxcsr.u32 & src->mxcsr_mask.u32;
      ctx->Rax = src->rax.u64;
      ctx->Rcx = src->rcx.u64;
      ctx->Rdx = src->rdx.u64;
      ctx->Rbx = src->rbx.u64;
      ctx->Rsp = src->rsp.u64;
      ctx->Rbp = src->rbp.u64;
      ctx->Rsi = src->rsi.u64;
      ctx->Rdi = src->rdi.u64;
      ctx->R8  = src->r8.u64;
      ctx->R9  = src->r9.u64;
      ctx->R10 = src->r10.u64;
      ctx->R11 = src->r11.u64;
      ctx->R12 = src->r12.u64;
      ctx->R13 = src->r13.u64;
      ctx->R14 = src->r14.u64;
      ctx->R15 = src->r15.u64;
      ctx->Rip = src->rip.u64;
      ctx->SegCs = src->cs.u16;
      ctx->SegDs = src->ds.u16;
      ctx->SegEs = src->es.u16;
      ctx->SegFs = src->fs.u16;
      ctx->SegGs = src->gs.u16;
      ctx->SegSs = src->ss.u16;
      ctx->Dr0 = src->dr0.u32;
      ctx->Dr1 = src->dr1.u32;
      ctx->Dr2 = src->dr2.u32;
      ctx->Dr3 = src->dr3.u32;
      ctx->Dr6 = src->dr6.u32;
      ctx->Dr7 = src->dr7.u32;
      ctx->EFlags = src->rflags.u64;
      fxsave->ControlWord = src->fcw.u16;
      fxsave->StatusWord = src->fsw.u16;
      fxsave->TagWord = dmn_w32_xsave_tag_word_from_real_tag_word(src->ftw.u16);
      fxsave->ErrorOpcode = src->fop.u16;
      fxsave->ErrorSelector = src->fcs.u16;
      fxsave->DataSelector = src->fds.u16;
      fxsave->ErrorOffset = src->fip.u32;
      fxsave->DataOffset = src->fdp.u32;
      {
        M128A *float_d = fxsave->FloatRegisters;
        REGS_Reg80 *float_s = &src->fpr0;
        for(U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1)
        {
          MemoryCopy(float_d, float_s, 10);
        }
      }
      if(!avx_available)
      {
        M128A *xmm_d = fxsave->XmmRegisters;
        REGS_Reg256 *xmm_s = &src->ymm0;
        for(U32 n = 0; n < 8; n += 1, xmm_d += 1, xmm_s += 1)
        {
          MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_d));
        }
      }
      if(avx_available)
      {
        DWORD part0_length = 0;
        M128A *part0 = (M128A*)LocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &part0_length);
        DWORD part1_length = 0;
        M128A *part1 = (M128A*)LocateXStateFeature(ctx, XSTATE_AVX, &part1_length);
        Assert(part0_length == part1_length);
        DWORD count = part0_length/sizeof(part0[0]);
        count = ClampTop(count, 16);
        REGS_Reg256 *ymm_d = &src->ymm0;
        for(DWORD i = 0; i < count; i += 1, part0 += 1, part1 += 1, ymm_d += 1)
        {
          // TODO(allen): Are we writing these out in the right order? Seems weird right?
          part0->Low  = ymm_d->u64[3];
          part0->High = ymm_d->u64[2];
          part1->Low  = ymm_d->u64[1];
          part1->High = ymm_d->u64[0];
        }
      }
      
      //- rjf: set thread context
      if(SetThreadContext(thread, ctx))
      {
        result = 1;
      }
      scratch_end(scratch);
    }break;
  }
  return result;
}

//- rjf: remote thread injection

internal DWORD
dmn_w32_inject_thread(HANDLE process, U64 start_address)
{
  LPTHREAD_START_ROUTINE start = (LPTHREAD_START_ROUTINE)start_address;
  DWORD thread_id = 0;
  HANDLE thread = CreateRemoteThread(process, 0, 0, start, 0, 0, &thread_id);
  if(thread != 0)
  {
    CloseHandle(thread);
  }
  return thread_id;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Main Layer Initialization (Implemented Per-OS)

internal void
dmn_init(void)
{
  Arena *arena = arena_alloc();
  dmn_w32_shared = push_array(arena, DMN_W32_Shared, 1);
  dmn_w32_shared->arena = arena;
  dmn_w32_shared->detach_arena = arena_alloc();
  dmn_w32_shared->entities_arena = arena_alloc__sized(GB(8), KB(64));
  dmn_w32_shared->entities_base = dmn_w32_entity_alloc(&dmn_w32_entity_nil, DMN_W32_EntityKind_Root, 0);
  dmn_w32_shared->entities_id_hash_slots_count = 4096;
  dmn_w32_shared->entities_id_hash_slots = push_array(arena, DMN_W32_EntityIDHashSlot, dmn_w32_shared->entities_id_hash_slots_count);
  
  // rjf: load Windows 10+ GetThreadDescription API
  {
    dmn_w32_GetThreadDescription = (DMN_W32_GetThreadDescriptionFunctionType *)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "GetThreadDescription");
  }
  
  // rjf: setup environment variables
  {
    CHAR *this_proc_env = GetEnvironmentStrings();
    U64 start_idx = 0;
    for(U64 idx = 0;; idx += 1)
    {
      if(this_proc_env[idx] == 0)
      {
        if(start_idx == idx)
        {
          break;
        }
        else
        {
          String8 string = str8((U8 *)this_proc_env + start_idx, idx - start_idx);
          str8_list_push(dmn_w32_shared->arena, &dmn_w32_shared->env_strings, string);
          start_idx = idx+1;
        }
      }
    }
  }
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Running/Halting (Implemented Per-OS)

internal DMN_EventList
dmn_run(Arena *arena, DMN_RunCtrls *ctrls)
{
  DMN_EventList events = {0};
  
  //////////////////////////////
  //- rjf: determine event generation path
  //
  typedef enum DMN_W32_EventGenPath
  {
    DMN_W32_EventGenPath_Run,
    DMN_W32_EventGenPath_DetachProcesses,
  }
  DMN_W32_EventGenPath;
  DMN_W32_EventGenPath event_gen_path = DMN_W32_EventGenPath_Run;
  if(dmn_w32_shared->detach_processes.first != 0)
  {
    event_gen_path = DMN_W32_EventGenPath_DetachProcesses;
  }
  
  //////////////////////////////
  //- rjf: produce debug events
  //
  switch(event_gen_path)
  {
    ////////////////////////////
    //- rjf: produce debug events from regular running
    //
    case DMN_W32_EventGenPath_Run:
    {
      Temp scratch = scratch_begin(&arena, 1);
      
      //////////////////////////
      //- rjf: set single step bit
      //
      if(!dmn_handle_match(ctrls->single_step_thread, dmn_handle_zero()))
      {
        DMN_W32_Entity *thread = dmn_w32_entity_from_handle(ctrls->single_step_thread);
        Architecture arch = thread->arch;
        switch(arch)
        {
          //- rjf: unimplemented win32/arch combos
          case Architecture_Null:
          case Architecture_arm64:
          case Architecture_arm32:
          case Architecture_COUNT:
          {NotImplemented;}break;
          
          //- rjf: x86/64
          case Architecture_x86:
          {
            REGS_RegBlockX86 regs = {0};
            dmn_thread_read_reg_block(ctrls->single_step_thread, &regs);
            regs.eflags.u32 |= 0x100;
            dmn_thread_write_reg_block(ctrls->single_step_thread, &regs);
          }break;
          case Architecture_x64:
          {
            REGS_RegBlockX64 regs = {0};
            dmn_thread_read_reg_block(ctrls->single_step_thread, &regs);
            regs.rflags.u64 |= 0x100;
            dmn_thread_write_reg_block(ctrls->single_step_thread, &regs);
          }break;
        }
      }
      
      //////////////////////////
      //- rjf: write all traps into memory
      //
      U8 *trap_swap_bytes = push_array_no_zero(scratch.arena, U8, ctrls->traps.trap_count);
      {
        U64 trap_idx = 0;
        for(DMN_TrapChunkNode *n = ctrls->traps.first; n != 0; n = n->next)
        {
          for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, trap_idx += 1)
          {
            DMN_Trap *trap = n->v+n_idx;
            trap_swap_bytes[trap_idx] = 0xCC;
            dmn_process_read(trap->process, r1u64(trap->vaddr, trap->vaddr+1), trap_swap_bytes+trap_idx);
            U8 int3 = 0xCC;
            dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr+1), &int3);
          }
        }
      }
      
      //////////////////////////
      //- rjf: produce list of threads which will run
      //
      DMN_W32_EntityNode *first_run_thread = 0;
      DMN_W32_EntityNode *last_run_thread = 0;
      {
        //- rjf: scan all processes
        for(DMN_W32_Entity *process = dmn_w32_shared->entities_base->first;
            process != &dmn_w32_entity_nil;
            process = process->next)
        {
          if(process->kind != DMN_W32_EntityKind_Process) {continue;}
          
          //- rjf: determine if this process is frozen 
          B32 process_is_frozen = 0;
          if(ctrls->run_entities_are_processes)
          {
            for(U64 idx = 0; idx < ctrls->run_entity_count; idx += 1)
            {
              if(dmn_handle_match(ctrls->run_entities[idx], dmn_w32_handle_from_entity(process)))
              {
                process_is_frozen = 1;
                break;
              }
            }
          }
          
          //- rjf: scan all threads in this process
          for(DMN_W32_Entity *thread = process->first;
              thread != 0;
              thread = thread->next)
          {
            if(thread->kind != DMN_W32_EntityKind_Thread) {continue;}
            
            //- rjf: determine if this thread is frozen
            B32 is_frozen = 0;
            {
              // rjf: single-step? freeze if not the single-step thread.
              if(!dmn_handle_match(dmn_handle_zero(), ctrls->single_step_thread) &&
                 !dmn_handle_match(dmn_w32_handle_from_entity(thread), ctrls->single_step_thread))
              {
                is_frozen = 1;
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
                  if(dmn_handle_match(ctrls->run_entities[idx], dmn_w32_handle_from_entity(thread)))
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
            if(dmn_w32_shared->halter_tid == thread->id)
            {
              is_frozen = 0;
            }
            
            //- rjf: add to list
            if(!is_frozen)
            {
              DMN_W32_EntityNode *n = push_array(scratch.arena, DMN_W32_EntityNode, 1);
              n->v = thread;
              SLLQueuePush(first_run_thread, last_run_thread, n);
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: resume threads which will run
      //
      for(DMN_W32_EntityNode *n = first_run_thread; n != 0; n = n->next)
      {
        DMN_W32_Entity *thread = n->v;
        DWORD resume_result = ResumeThread(thread->handle);
        switch(resume_result)
        {
          case 0xffffffffu:
          {
            // TODO(rjf): error - unknown cause. need to do GetLastError, FormatMessage
          }break;
          default:
          {
            DWORD desired_counter = 0;
            DWORD current_counter = resume_result - 1;
            if(current_counter != desired_counter)
            {
              // NOTE(rjf): Warning. The user has manually suspended this thread,
              // so even though from Demon's perspective it thinks this thread
              // should run, it will not, because the user has manually called
              // SuspendThread or used CREATE_SUSPENDED or whatever.
            }
          }break;
        }
      }
      
      //////////////////////////
      //- rjf: if run threads are marked as having reported an explicit trap
      // on their last run, shift their RIPs past that trap instruction, so
      // that they may continue
      //
      for(DMN_W32_EntityNode *n = first_run_thread; n != 0; n = n->next)
      {
        DMN_W32_Entity *thread = n->v;
        if(thread->thread.last_run_reported_trap)
        {
          Temp temp = temp_begin(scratch.arena);
          U64 regs_block_size = regs_block_size_from_architecture(thread->arch);
          void *regs_block = push_array(temp.arena, U8, regs_block_size);
          B32 good = dmn_w32_thread_read_reg_block(thread->arch, thread->handle, regs_block);
          U64 pre_rip = regs_rip_from_arch_block(thread->arch, regs_block);
          if(good && pre_rip == thread->thread.last_run_reported_trap_pre_rip)
          {
            regs_arch_block_write_rip(thread->arch, regs_block, thread->thread.last_run_reported_trap_post_rip);
            dmn_w32_thread_write_reg_block(thread->arch, thread->handle, regs_block);
          }
          temp_end(temp);
          thread->thread.last_run_reported_trap = 0;
          thread->thread.last_run_reported_trap_post_rip = 0;
        }
      }
      
      //////////////////////////
      //- rjf: choose win32 resume code
      //
      DWORD resume_code = DBG_CONTINUE;
      {
        if(dmn_w32_shared->exception_not_handled && !ctrls->ignore_previous_exception)
        {
          dmn_w32_shared->exception_not_handled = 0;
          resume_code = DBG_EXCEPTION_NOT_HANDLED;
        }
      }
      
      //////////////////////////
      //- rjf: inform windows that we're resuming, run, & obtain next debug event
      //
      DEBUG_EVENT evt = {0};
      B32 evt_good = 0;
      {
        B32 resume_good = 1;
        if(dmn_w32_shared->resume_needed)
        {
          dmn_w32_shared->resume_needed = 0;
          resume_good = !!ContinueDebugEvent(dmn_w32_shared->resume_pid, dmn_w32_shared->resume_tid, resume_code);
          dmn_w32_shared->resume_needed = 0;
          dmn_w32_shared->resume_tid = 0;
          dmn_w32_shared->resume_pid = 0;
        }
        if(resume_good)
        {
          evt_good = !!WaitForDebugEvent(&evt, INFINITE);
          if(evt_good)
          {
            dmn_w32_shared->resume_needed = 1;
            dmn_w32_shared->resume_pid = evt.dwProcessId;
            dmn_w32_shared->resume_tid = evt.dwThreadId;
          }
        }
      }
      
      //////////////////////////
      //- rjf: suspend threads which ran
      //
      if(evt_good) for(DMN_W32_EntityNode *n = first_run_thread; n != 0; n = n->next)
      {
        DMN_W32_Entity *thread = n->v;
        DWORD suspend_result = SuspendThread(thread->handle);
        switch(suspend_result)
        {
          case 0xffffffffu:
          {
            // TODO(rjf): error - unknown cause. need to do do GetLastError, FormatMessage
            //
            // NOTE(rjf): this can happen when the event is EXIT_THREAD_DEBUG_EVENT
            // or EXIT_PROCESS_DEBUG_EVENT. after such an event, SuspendThread
            // gives error code 5 (access denied). this has no adverse effects, but
            // if we want to start reporting errors we should take care to avoid
            // calling SuspendThread in that case.
          }break;
          default:
          {
            DWORD desired_counter = 1;
            DWORD current_counter = suspend_result + 1;
            if(current_counter != desired_counter)
            {
              // NOTE(rjf): Warning. We've suspended to something higher than 1.
              // In this case, it means the user probably created the thread in
              // a suspended state, or they called SuspendThread.
            }
          }break;
        }
      }
      
      //////////////////////////
      //- rjf: process the new event
      //
      if(evt_good)
      {
        switch(evt.dwDebugEventCode)
        {
          //////////////////////
          //- rjf: process was created
          //
          case CREATE_PROCESS_DEBUG_EVENT:
          {
            // rjf: zero out "process pending" state
            dmn_w32_shared->new_process_pending = 0;
            
            // rjf: unpack event
            HANDLE process_handle = evt.u.CreateProcessInfo.hProcess;
            HANDLE thread_handle = evt.u.CreateProcessInfo.hThread;
            HANDLE module_handle = evt.u.CreateProcessInfo.hFile;
            U64 tls_base = (U64)evt.u.CreateProcessInfo.lpThreadLocalBase;
            U64 module_base = (U64)evt.u.CreateProcessInfo.lpBaseOfImage;
            U64 module_name_vaddr = (U64)evt.u.CreateProcessInfo.lpImageName;
            B32 module_name_is_unicode = (evt.u.CreateProcessInfo.fUnicode != 0);
            DMN_W32_ImageInfo image_info = dmn_w32_image_info_from_process_base_vaddr(process_handle, module_base);
            
            // rjf: create entities (thread/module are implied for initial - they are not reported by win32)
            DMN_W32_Entity *process = dmn_w32_entity_alloc(dmn_w32_shared->entities_base, DMN_W32_EntityKind_Process, evt.dwProcessId);
            DMN_W32_Entity *thread = dmn_w32_entity_alloc(process, DMN_W32_EntityKind_Thread, evt.dwThreadId);
            DMN_W32_Entity *module = dmn_w32_entity_alloc(process, DMN_W32_EntityKind_Module, module_base);
            {
              process->handle = process_handle;
              process->arch   = image_info.arch;
              thread->handle                   = thread_handle;
              thread->thread.thread_local_base = tls_base;
              module->handle                         = module_handle;
              module->module.vaddr_range             = r1u64(module_base, image_info.size);
              module->module.is_main                 = 1;
              module->module.address_of_name_pointer = module_name_vaddr;
              module->module.name_is_unicode         = module_name_is_unicode;
            }
            
            // rjf: put thread into suspended state, so it matches expected initial state
            SuspendThread(thread_handle);
            
            // rjf: set up per-process injected code (to run halter threads on &
            // generate debug events)
            {
              U8 injection_code[DMN_W32_INJECTED_CODE_SIZE];
              MemorySet(injection_code, 0xCC, DMN_W32_INJECTED_CODE_SIZE);
              injection_code[0] = 0xC3;
              U64 injection_size = DMN_W32_INJECTED_CODE_SIZE + sizeof(DMN_W32_InjectedBreak);
              U64 injection_address = (U64)VirtualAllocEx(process_handle, 0, injection_size, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE);
              dmn_w32_process_write(process_handle, r1u64(injection_address, injection_address+sizeof(injection_code)), injection_code);
              process->proc.injection_address = injection_address;
            }
            
            // rjf: generate events
            {
              // rjf: create process
              {
                DMN_Event *e = dmn_event_list_push(arena, &events);
                e->kind    = DMN_EventKind_CreateProcess;
                e->process = dmn_w32_handle_from_entity(process);
                e->arch    = image_info.arch;
                e->code    = evt.dwProcessId;
              }
              
              // rjf: create thread
              {
                DMN_Event *e = dmn_event_list_push(arena, &events);
                e->kind    = DMN_EventKind_CreateThread;
                e->process = dmn_w32_handle_from_entity(process);
                e->thread  = dmn_w32_handle_from_entity(thread);
                e->arch    = image_info.arch;
                e->code    = evt.dwThreadId;
              }
              
              // rjf: load module
              {
                DMN_Event *e = dmn_event_list_push(arena, &events);
                e->kind    = DMN_EventKind_LoadModule;
                e->process = dmn_w32_handle_from_entity(process);
                e->module  = dmn_w32_handle_from_entity(module);
                e->arch    = image_info.arch;
                e->address = module_base;
                e->size    = image_info.size;
                e->string  = dmn_w32_full_path_from_module(arena, module);
              }
            }
          }break;
          
          //////////////////////
          //- rjf: process exited
          //
          case EXIT_PROCESS_DEBUG_EVENT:
          {
            DMN_W32_Entity *process = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Process, evt.dwProcessId);
            
            // rjf: generate events for children
            for(DMN_W32_Entity *child = process->first; child != &dmn_w32_entity_nil; child = child->next)
            {
              switch(child->kind)
              {
                case DMN_W32_EntityKind_Thread:
                {
                  DMN_Event *e = dmn_event_list_push(arena, &events);
                  e->kind = DMN_EventKind_ExitThread;
                  e->process = dmn_w32_handle_from_entity(process);
                  e->thread = dmn_w32_handle_from_entity(child);
                }break;
                case DMN_W32_EntityKind_Module:
                {
                  DMN_Event *e = dmn_event_list_push(arena, &events);
                  e->kind = DMN_EventKind_UnloadModule;
                  e->process = dmn_w32_handle_from_entity(process);
                  e->module = dmn_w32_handle_from_entity(child);
                  e->string = dmn_w32_full_path_from_module(arena, child);
                }break;
              }
            }
            
            // rjf: generate event for process
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind = DMN_EventKind_ExitProcess;
              e->process = dmn_w32_handle_from_entity(process);
              e->code = evt.u.ExitProcess.dwExitCode;
            }
            
            // rjf: release entity storage
            dmn_w32_entity_release(process);
            
            // rjf: detach
            DebugActiveProcessStop(evt.dwProcessId);
          }break;
          
          //////////////////////
          //- rjf: thread was created
          //
          case CREATE_THREAD_DEBUG_EVENT:
          {
            DMN_W32_Entity *process = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Process, evt.dwProcessId);
            
            // rjf: create thread entity
            DMN_W32_Entity *thread = dmn_w32_entity_alloc(process, DMN_W32_EntityKind_Thread, evt.dwThreadId);
            {
              thread->handle                   = evt.u.CreateThread.hThread;
              thread->arch                     = process->arch;
              thread->thread.thread_local_base = (U64)evt.u.CreateThread.lpThreadLocalBase;
            }
            
            // rjf: suspend thread immediately upon creation, to match with expected suspension state
            DWORD sus_result = SuspendThread(thread->handle);
            (void)sus_result;
            
            // rjf: unpack thread name
            String8 thread_name = {0};
            if(dmn_w32_GetThreadDescription != 0)
            {
              WCHAR *thread_name_w = 0;
              HRESULT hr = dmn_w32_GetThreadDescription(thread->handle, &thread_name_w);
              if(SUCCEEDED(hr))
              {
                thread_name = str8_from_16(arena, str16_cstring((U16 *)thread_name_w));
                LocalFree(thread_name_w);
              }
            }
            
            // rjf: determine if this is a "halter thread" - the threads we spawn to halt processes
            B32 is_halter = (evt.dwThreadId == dmn_w32_shared->halter_tid);
            
            // rjf: generate events for non-halter threads
            if(!is_halter)
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind = DMN_EventKind_CreateThread;
              e->process = dmn_w32_handle_from_entity(process);
              e->thread  = dmn_w32_handle_from_entity(thread);
              e->arch    = thread->arch;
              e->code    = evt.dwThreadId;
              e->string  = thread_name;
            }
          }break;
          
          //////////////////////
          //- rjf: thread exited
          //
          case EXIT_THREAD_DEBUG_EVENT:
          {
            DMN_W32_Entity *thread = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Thread, evt.dwThreadId);
            DMN_W32_Entity *process = thread->parent;
            
            // rjf: determine if this is the halter thread
            B32 is_halter = (evt.dwThreadId == dmn_w32_shared->halter_tid);
            
            // rjf: generate a halt event if this thread is the halter
            if(is_halter)
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind = DMN_EventKind_Halt;
              dmn_w32_shared->halter_process = dmn_handle_zero();
              dmn_w32_shared->halter_tid = 0;
            }
            
            // rjf: if this thread is *not* the halter, then generate a regular exit-thread event
            if(!is_halter)
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind    = DMN_EventKind_ExitThread;
              e->process = dmn_w32_handle_from_entity(process);
              e->thread  = dmn_w32_handle_from_entity(thread);
              e->code    = evt.u.ExitThread.dwExitCode;
            }
            
            // rjf: release entity storage
            dmn_w32_entity_release(thread);
          }break;
          
          //////////////////////
          //- rjf: DLL was loaded
          //
          case LOAD_DLL_DEBUG_EVENT:
          {
            DMN_W32_Entity *process = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Process, evt.dwProcessId);
            
            // rjf: extract image info
            U64 module_base = (U64)evt.u.LoadDll.lpBaseOfDll;
            DMN_W32_ImageInfo image_info = dmn_w32_image_info_from_process_base_vaddr(process->handle, module_base);
            
            // rjf: create module entity
            DMN_W32_Entity *module = dmn_w32_entity_alloc(process, DMN_W32_EntityKind_Module, module_base);
            {
              module->handle                         = evt.u.LoadDll.hFile;
              module->arch                           = image_info.arch;
              module->module.vaddr_range             = r1u64(module_base, module_base+image_info.size);
              module->module.address_of_name_pointer = (U64)evt.u.LoadDll.lpImageName;
              module->module.name_is_unicode         = (evt.u.LoadDll.fUnicode != 0);
            }
            
            // rjf: generate event
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind = DMN_EventKind_LoadModule;
              e->process = dmn_w32_handle_from_entity(process);
              e->module  = dmn_w32_handle_from_entity(module);
              e->arch    = module->arch;
              e->address = module_base;
              e->size    = image_info.size;
              e->string  = dmn_w32_full_path_from_module(arena, module);
            }
          }break;
          
          //////////////////////
          //- rjf: DLL was unloaded
          //
          case UNLOAD_DLL_DEBUG_EVENT:
          {
            U64 module_base = (U64)evt.u.UnloadDll.lpBaseOfDll;
            DMN_W32_Entity *module = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Module, module_base);
            DMN_W32_Entity *process = module->parent;
            
            // rjf: generate event
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind = DMN_EventKind_UnloadModule;
              e->process = dmn_w32_handle_from_entity(process);
              e->module  = dmn_w32_handle_from_entity(module);
              e->string  = dmn_w32_full_path_from_module(arena, module);
            }
            
            // rjf: release entity storage
            dmn_w32_entity_release(module);
          }break;
          
          //////////////////////
          //- rjf: exception was hit
          //
          case EXCEPTION_DEBUG_EVENT:
          {
            // NOTE(rjf): Notes on multithreaded breakpoint events
            // (2021/11/1):
            //
            // When many threads are simultaneously running, multiple threads
            // may hit a trap "at the same time". When this happens there will be
            // multiple events in an internal queue that we cannot see. If there
            // is another event in the queue we will not see it until we call
            // ContinueDebugEvent again, in a subsequent call to demon_os_run.
            //
            // When we get a trap event, the instruction pointer stored
            // in the event will have the address of the int 3 instruction that
            // was hit. Our RIP register, however, will be one byte past that.
            // So, to get the behavior we want, we need to set the RIP register
            // back to the address of the int 3.
            //
            // To deal with the fact that we may get breakpoint events later that
            // were actually from this run what we do is:
            //
            // #1. If we get a trap event, and it corresponds to a user submitted
            //     trap, then we treat it is a breakpoint event.
            // #2. If we get a trap event, and it does NOT correspond to a user
            //     trap in this call:
            //   #A. If the actual unmodified instruction byte is NOT an int 3,
            //       then this is a queued event from a previous run that is no
            //       longer applicable and we skip it.
            //   #B. If the actual unmodified instruction is an int 3, then this
            //       becomes a trap event and we do not reset RIP.
            
            // NOTE(rjf): The exception record struct has a 32-bit version and a
            // 64-bit version. We only currently handle the 64-bit version.
            
            //- rjf: unpack
            DMN_W32_Entity *thread = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Thread, evt.dwThreadId);
            DMN_W32_Entity *process = thread->parent;
            EXCEPTION_DEBUG_INFO *edi = &evt.u.Exception;
            EXCEPTION_RECORD *exception = &edi->ExceptionRecord;
            U64 instruction_pointer = (U64)exception->ExceptionAddress;
            
            //- rjf: determine if this is the first breakpoint in a process
            // (breakpoint notifying us that the debugger is attached)
            B32 first_bp = 0;
            if(!process->proc.did_first_bp && exception->ExceptionCode == DMN_W32_EXCEPTION_BREAKPOINT)
            {
              process->proc.did_first_bp = 1;
              first_bp = 1;
            }
            
            //- rjf: determine if this exception is a trap
            B32 is_trap = (!first_bp &&
                           (exception->ExceptionCode == DMN_W32_EXCEPTION_BREAKPOINT ||
                            exception->ExceptionCode == DMN_W32_EXCEPTION_STACK_BUFFER_OVERRUN));
            
            //- rjf: check if this trap is a usage-code-specified trap or something else
            B32 hit_user_trap = 0;
            if(is_trap)
            {
              for(DMN_TrapChunkNode *n = ctrls->traps.first; n != 0; n = n->next)
              {
                for(U64 idx = 0; idx < n->count; idx += 1)
                {
                  if(dmn_handle_match(n->v[idx].process, dmn_w32_handle_from_entity(process)) && n->v[idx].vaddr == instruction_pointer)
                  {
                    hit_user_trap = 1;
                    break;
                  }
                }
              }
            }
            
            //- rjf: check if trap is explicit in the actual code memory
            B32 hit_explicit_trap = 0;
            if(is_trap && !hit_user_trap)
            {
              U8 instruction_byte = 0;
              if(dmn_w32_process_read_struct(process->handle, instruction_pointer, &instruction_byte))
              {
                hit_explicit_trap = (instruction_byte == 0xCC || instruction_byte == 0xCD);
              }
            }
            
            //- rjf: determine whether to roll back instruction pointer
            B32 should_do_rollback = (is_trap);
            
            //- rjf: roll back thread's instruction pointer
            U64 post_trap_rip = 0;
            if(should_do_rollback)
            {
              Temp temp = temp_begin(scratch.arena);
              U64 regs_block_size = regs_block_size_from_architecture(thread->arch);
              void *regs_block = push_array(scratch.arena, U8, regs_block_size);
              if(dmn_w32_thread_read_reg_block(thread->arch, thread->handle, regs_block))
              {
                post_trap_rip = regs_rip_from_arch_block(thread->arch, regs_block);
                regs_arch_block_write_rip(thread->arch, regs_block, instruction_pointer);
                dmn_w32_thread_write_reg_block(thread->arch, thread, regs_block);
              }
              temp_end(temp);
            }
            
            //- rjf: not a user trap, not an explicit trap, then it's a trap that
            // this thread hit previously but has since skipped
            B32 hit_previous_trap = (is_trap && !hit_user_trap && !hit_explicit_trap);
            
            //- rjf: determine whether to skip this event
            B32 skip_event = (hit_previous_trap);
            
            //- rjf: generate event
            if(!skip_event)
            {
              // rjf: fill top-level info
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind    = DMN_EventKind_Exception;
              e->process = dmn_w32_handle_from_entity(process);
              e->thread  = dmn_w32_handle_from_entity(thread);
              e->code    = exception->ExceptionCode;
              e->flags   = exception->ExceptionFlags;
              e->instruction_pointer = (U64)exception->ExceptionAddress;
              
              // rjf: explicit trap -> mark this thread as having reported this trap
              if(hit_explicit_trap)
              {
                thread->thread.last_run_reported_trap = 1;
                thread->thread.last_run_reported_trap_pre_rip = instruction_pointer;
                thread->thread.last_run_reported_trap_post_rip = post_trap_rip;
              }
              
              //- rjf: fill according to exception code
              switch(exception->ExceptionCode)
              {
                //- rjf: fill breakpoint event info
                case DMN_W32_EXCEPTION_BREAKPOINT:
                {
                  DMN_EventKind report_event_kind = DMN_EventKind_Trap;
                  if(first_bp)
                  {
                    report_event_kind = DMN_EventKind_HandshakeComplete;
                  }
                  else if(hit_user_trap)
                  {
                    report_event_kind = DMN_EventKind_Breakpoint;
                  }
                  e->kind = report_event_kind;
                }break;
                
                //- rjf: fill stack buffer overrun event info
                case DMN_W32_EXCEPTION_STACK_BUFFER_OVERRUN:
                {
                  e->kind = DMN_EventKind_Trap;
                }break;
                
                //- rjf: fill single-step event info
                case DMN_W32_EXCEPTION_SINGLE_STEP:
                {
                  e->kind = DMN_EventKind_SingleStep;
                }break;
                
                //- rjf: fill throw info
                case DMN_W32_EXCEPTION_THROW:
                {
                  U64 exception_sp = 0;
                  U64 exception_ip = 0;
                  if(exception->NumberParameters >= 3)
                  {
                    exception_sp = (U64)exception->ExceptionInformation[1];
                    exception_ip = (U64)exception->ExceptionInformation[2];
                  }
                  e->stack_pointer = exception_sp;
                  e->exception_kind = DMN_ExceptionKind_CppThrow;
                  e->exception_repeated = (edi->dwFirstChance == 0);
                  dmn_w32_shared->exception_not_handled = (edi->dwFirstChance != 0);
                }break;
                
                //- rjf: fill access violation info
                case DMN_W32_EXCEPTION_ACCESS_VIOLATION:
                case DMN_W32_EXCEPTION_IN_PAGE_ERROR:
                {
                  U64 exception_address = 0;
                  DMN_ExceptionKind exception_kind = DMN_ExceptionKind_Null;
                  if(exception->NumberParameters >= 2)
                  {
                    switch(exception->ExceptionInformation[0])
                    {
                      case 0: exception_kind = DMN_ExceptionKind_MemoryRead;    break;
                      case 1: exception_kind = DMN_ExceptionKind_MemoryWrite;   break;
                      case 8: exception_kind = DMN_ExceptionKind_MemoryExecute; break;
                    }
                    exception_address = exception->ExceptionInformation[1];
                  }
                  e->address = exception_address;
                  e->exception_kind = exception_kind;
                  e->exception_repeated = (edi->dwFirstChance == 0);
                  dmn_w32_shared->exception_not_handled = (edi->dwFirstChance != 0);
                }break;
                
                //- rjf: fill set-thread-name info
                case DMN_W32_EXCEPTION_SET_THREAD_NAME:
                if(exception->NumberParameters >= 2)
                {
                  U64 thread_name_address = exception->ExceptionInformation[1];
                  DMN_W32_Entity *process = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Process, evt.dwProcessId);
                  String8List thread_name_strings = {0};
                  {
                    U64 read_addr = thread_name_address;
                    U64 total_string_size = 0;
                    for(;total_string_size < KB(4);)
                    {
                      U8 *buffer = push_array_no_zero(scratch.arena, U8, 256);
                      B32 good_read = dmn_w32_process_read(process->handle, r1u64(read_addr, read_addr+256), buffer);
                      if(good_read)
                      {
                        U64 size = 256;
                        for(U64 idx = 0; idx < 256; idx += 1)
                        {
                          if(buffer[idx] == 0)
                          {
                            size = idx;
                            break;
                          }
                        }
                        String8 string_part = str8(buffer, size);
                        str8_list_push(scratch.arena, &thread_name_strings, string_part);
                        total_string_size += size;
                        read_addr += size;
                        if(size < 256)
                        {
                          break;
                        }
                      }
                    }
                  }
                  e->kind = DMN_EventKind_SetThreadName;
                  e->string = str8_list_join(arena, &thread_name_strings, 0);
                  if(exception->NumberParameters > 2)
                  {
                    e->code = exception->ExceptionInformation[2];
                  }
                }break;
                
                //- rjf: unhandled exception case
                default:
                {
                  dmn_w32_shared->exception_not_handled = (edi->dwFirstChance != 0);
                }break;
              }
            }
          }break;
          
          //////////////////////
          //- rjf: output debug string was gathered
          //
          case OUTPUT_DEBUG_STRING_EVENT:
          {
            // rjf: unpack event
            DMN_W32_Entity *process = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Process, evt.dwProcessId);
            DMN_W32_Entity *thread = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Thread, evt.dwThreadId);
            U64 string_address = (U64)evt.u.DebugString.lpDebugStringData;
            U64 string_size = (U64)evt.u.DebugString.nDebugStringLength;
            
            // rjf: read memory
            U8 *buffer = push_array_no_zero(arena, U8, string_size + 1);
            dmn_w32_process_read(process->handle, r1u64(string_address, string_address+string_size), buffer);
            buffer[string_size] = 0;
            
            // rjf: generate event
            {
              DMN_Event *e = dmn_event_list_push(arena, &events);
              e->kind = DMN_EventKind_DebugString;
              e->process = dmn_w32_handle_from_entity(process);
              e->thread = dmn_w32_handle_from_entity(thread);
              e->string = str8(buffer, string_size);
              if(string_size != 0 && buffer[string_size-1] == 0)
              {
                e->string.size -= 1;
              }
            }
          }break;
          
          //////////////////////
          //- rjf: a "rip event" - a "system debugging error".
          //
          case RIP_EVENT:
          {
            DMN_W32_Entity *process = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Process, evt.dwProcessId);
            DMN_W32_Entity *thread = dmn_w32_entity_from_kind_id(DMN_W32_EntityKind_Thread, evt.dwThreadId);
            DMN_Event *e = dmn_event_list_push(arena, &events);
            e->kind    = DMN_EventKind_Exception;
            e->process = dmn_w32_handle_from_entity(process);
            e->thread  = dmn_w32_handle_from_entity(thread);
          }break;
          
          //////////////////////
          //- rjf: default case - some kind of debugging event that we don't currently consume.
          //
          default:
          {
            NoOp;
          }break;
        }
      }
      
      //- rjf: gather new thread-names
      if(dmn_w32_GetThreadDescription != 0)
      {
        for(DMN_W32_Entity *process = dmn_w32_shared->entities_base->first;
            process != &dmn_w32_entity_nil;
            process = process->next)
        {
          if(process->kind != DMN_W32_EntityKind_Process) { continue; }
          for(DMN_W32_Entity *thread = process->first;
              thread != &dmn_w32_entity_nil;
              thread = thread->next)
          {
            if(thread->kind != DMN_W32_EntityKind_Thread) { continue; }
            if(thread->thread.last_name_hash == 0 ||
               thread->thread.name_gather_time_us+1000000 <= os_now_microseconds())
            {
              String8 name = {0};
              {
                WCHAR *thread_name_w = 0;
                HRESULT hr = dmn_w32_GetThreadDescription(thread->handle, &thread_name_w);
                if(SUCCEEDED(hr))
                {
                  name = str8_from_16(scratch.arena, str16_cstring((U16 *)thread_name_w));
                  LocalFree(thread_name_w);
                }
              }
              U64 name_hash = dmn_w32_hash_from_string(name);
              if(name.size != 0 && name_hash != thread->thread.last_name_hash)
              {
                DMN_Event *e = dmn_event_list_push(arena, &events);
                e->kind    = DMN_EventKind_SetThreadName;
                e->process = dmn_w32_handle_from_entity(process);
                e->thread  = dmn_w32_handle_from_entity(thread);
                e->string  = push_str8_copy(arena, name);
              }
              thread->thread.name_gather_time_us = os_now_microseconds();
              thread->thread.last_name_hash = name_hash;
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: restore original memory at trap locations
      //
      {
        U64 trap_idx = 0;
        for(DMN_TrapChunkNode *n = ctrls->traps.first; n != 0; n = n->next)
        {
          for(U64 n_idx = 0; n_idx < n->count; n_idx += 1, trap_idx += 1)
          {
            DMN_Trap *trap = n->v+n_idx;
            U8 og_byte = trap_swap_bytes[trap_idx];
            if(og_byte != 0xCC)
            {
              dmn_process_write(trap->process, r1u64(trap->vaddr, trap->vaddr+1), &og_byte);
            }
          }
        }
      }
      
      //////////////////////////
      //- rjf: unset single step bit
      //
      if(!dmn_handle_match(ctrls->single_step_thread, dmn_handle_zero()))
      {
        DMN_W32_Entity *thread = dmn_w32_entity_from_handle(ctrls->single_step_thread);
        Architecture arch = thread->arch;
        switch(arch)
        {
          //- rjf: unimplemented win32/arch combos
          case Architecture_Null:
          case Architecture_arm64:
          case Architecture_arm32:
          case Architecture_COUNT:
          {NotImplemented;}break;
          
          //- rjf: x86/64
          case Architecture_x86:
          {
            REGS_RegBlockX86 regs = {0};
            dmn_thread_read_reg_block(ctrls->single_step_thread, &regs);
            regs.eflags.u32 &= ~0x100;
            dmn_thread_write_reg_block(ctrls->single_step_thread, &regs);
          }break;
          case Architecture_x64:
          {
            REGS_RegBlockX64 regs = {0};
            dmn_thread_read_reg_block(ctrls->single_step_thread, &regs);
            regs.rflags.u64 &= ~0x100;
            dmn_thread_write_reg_block(ctrls->single_step_thread, &regs);
          }break;
        }
      }
      
      scratch_end(scratch);
    }break;
    
    ////////////////////////////
    //- rjf: produce debug events from queued up detached processes
    //
    case DMN_W32_EventGenPath_DetachProcesses:
    {
      for(DMN_HandleNode *n = dmn_w32_shared->detach_processes.first; n != 0; n = n->next)
      {
        DMN_W32_Entity *process = dmn_w32_entity_from_handle(n->v);
        
        // rjf: push exit thread events
        for(DMN_W32_Entity *child = process->first; child != &dmn_w32_entity_nil; child = child->next)
        {
          if(child->kind == DMN_W32_EntityKind_Thread)
          {
            DMN_Event *e = dmn_event_list_push(arena, &events);
            e->kind    = DMN_EventKind_ExitThread;
            e->process = dmn_w32_handle_from_entity(process);
            e->thread  = dmn_w32_handle_from_entity(child);
          }
        }
        
        // rjf: push unload module events
        for(DMN_W32_Entity *child = process->first; child != &dmn_w32_entity_nil; child = child->next)
        {
          if(child->kind == DMN_W32_EntityKind_Module)
          {
            DMN_Event *e = dmn_event_list_push(arena, &events);
            e->kind    = DMN_EventKind_UnloadModule;
            e->process = dmn_w32_handle_from_entity(process);
            e->module  = dmn_w32_handle_from_entity(child);
            e->string  = dmn_w32_full_path_from_module(arena, child);
          }
        }
        
        // rjf: push exit process event
        {
          DMN_Event *e = dmn_event_list_push(arena, &events);
          e->kind    = DMN_EventKind_ExitProcess;
          e->process = dmn_w32_handle_from_entity(process);
        }
        
        // rjf: free process
        dmn_w32_entity_release(process);
      }
      
      // rjf: reset queued up detached processes
      MemoryZeroStruct(&dmn_w32_shared->detach_processes);
      arena_clear(dmn_w32_shared->detach_arena);
    }break;
  }
  
  return events;
}

internal void
dmn_halt(U64 code, U64 user_data)
{
  if(!dmn_handle_match(dmn_handle_zero(), dmn_w32_shared->halter_process))
  {
    DMN_W32_Entity *process = &dmn_w32_entity_nil;
    for(DMN_W32_Entity *entity = dmn_w32_shared->entities_base->first;
        entity != &dmn_w32_entity_nil;
        entity = entity->next)
    {
      if(entity->kind == DMN_W32_EntityKind_Process)
      {
        process = entity;
        break;
      }
    }
    if(process != &dmn_w32_entity_nil)
    {
      dmn_w32_shared->halter_process = dmn_w32_handle_from_entity(process);
      DMN_W32_InjectedBreak injection = {code, user_data};
      U64 data_injection_address = process->proc.injection_address + DMN_W32_INJECTED_CODE_SIZE;
      dmn_w32_process_write_struct(process->handle, data_injection_address, &injection);
      dmn_w32_shared->halter_tid = dmn_w32_inject_thread(process->handle, process->proc.injection_address);
    }
  }
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Process Launching/Attaching/Killing/Detaching (Implemented Per-OS)

internal U32
dmn_launch_process(OS_LaunchOptions *options)
{
  Temp scratch = scratch_begin(0, 0);
  U32 result = 0;
  
  //- rjf: produce exe / arguments string
  String8 cmd = {0};
  if(options->cmd_line.first != 0)
  {
    String8List args = {0};
    String8 exe_path = options->cmd_line.first->string;
    str8_list_pushf(scratch.arena, &args, "\"%S\"", exe_path);
    for(String8Node *n = options->cmd_line.first->next; n != 0; n = n->next)
    {
      str8_list_push(scratch.arena, &args, n->string);
    }
    StringJoin join_params = {0};
    join_params.sep = str8_lit(" ");
    cmd = str8_list_join(scratch.arena, &args, &join_params);
  }
  
  //- rjf: produce environment strings
  String8 env = {0};
  {
    String8List all_opts = options->env;
    if(options->inherit_env != 0)
    {
      MemoryZeroStruct(&all_opts);
      for(String8Node *n = options->env.first; n != 0; n = n->next)
      {
        str8_list_push(scratch.arena, &all_opts, n->string);
      }
      for(String8Node *n = dmn_w32_shared->env_strings.first; n != 0; n = n->next)
      {
        str8_list_push(scratch.arena, &all_opts, n->string);
      }
    }
    StringJoin join_params2 = {0};
    join_params2.sep = str8_lit("\0");
    join_params2.post = str8_lit("\0");
    env = str8_list_join(scratch.arena, &all_opts, &join_params2);
  }
  
  //- rjf: produce utf-16 strings
  String16 cmd16 = str16_from_8(scratch.arena, cmd);
  String16 dir16 = str16_from_8(scratch.arena, options->path);
  String16 env16 = str16_from_8(scratch.arena, env);
  
  //- rjf: launch
  DWORD access_flags = CREATE_UNICODE_ENVIRONMENT|DEBUG_PROCESS;
  STARTUPINFOW startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {0};
  AllocConsole();
  if(CreateProcessW(0, (WCHAR*)cmd16.str, 0, 0, 1, access_flags, (WCHAR*)env16.str, (WCHAR*)dir16.str, &startup_info, &process_info))
  {
    // check if we are 32-bit app, and just close it immediately
    BOOL is_wow = 0;
    IsWow64Process(process_info.hProcess, &is_wow);
    if(is_wow)
    {
      MessageBox(0, "Sorry, The RAD Debugger only debugs 64-bit applications currently.", "Process error", MB_OK|MB_ICONSTOP);
      DebugActiveProcessStop(process_info.dwProcessId);
      TerminateProcess(process_info.hProcess,0xffffffff);
    }
    else
    {
      result = process_info.dwProcessId;
      dmn_w32_shared->new_process_pending = 1;
    }
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
  }
  else
  {
    MessageBox(0, "Error starting process.", "Process error", MB_OK|MB_ICONSTOP);
  }
  FreeConsole();
  
  //- rjf: eliminate all handles which have stuck around from the AllocConsole
  {
    SetStdHandle(STD_INPUT_HANDLE, 0);
    SetStdHandle(STD_OUTPUT_HANDLE, 0);
    SetStdHandle(STD_ERROR_HANDLE, 0);
  }
  
  scratch_end(scratch);
  return result;
}

internal B32
dmn_attach_process(U32 pid)
{
  B32 result = 0;
  if(DebugActiveProcess((DWORD)pid))
  {
    result = 1;
    dmn_w32_shared->new_process_pending = 1;
  }
  return result;
}

internal B32
dmn_kill_process(DMN_Handle process, U32 exit_code)
{
  B32 result = 0;
  DMN_W32_Entity *process_entity = dmn_w32_entity_from_handle(process);
  if(TerminateProcess(process_entity->handle, exit_code))
  {
    result = 1;
  }
  return result;
}

internal B32
dmn_detach_process(DMN_Handle process)
{
  B32 result = 0;
  DMN_W32_Entity *process_entity = dmn_w32_entity_from_handle(process);
  
  // rjf: resume threads
  for(DMN_W32_Entity *child = process_entity->first;
      child != &dmn_w32_entity_nil;
      child = child->next)
  {
    if(child->kind == DMN_W32_EntityKind_Thread)
    {
      DWORD resume_result = ResumeThread(child->handle);
      (void)resume_result;
    }
  }
  
  // rjf: detach
  {
    DWORD pid = (DWORD)process_entity->id;
    if(DebugActiveProcessStop(pid))
    {
      result = 1;
    }
  }
  
  // rjf: push into list of processes to generate events for later
  if(result != 0)
  {
    dmn_handle_list_push(dmn_w32_shared->detach_arena, &dmn_w32_shared->detach_processes, process);
  }
  
  return result;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks Process/Thread Reads/Writes (Implemented Per-OS)

//- rjf: processes

internal U64
dmn_process_read(DMN_Handle process, Rng1U64 range, void *dst)
{
  DMN_W32_Entity *entity = dmn_w32_entity_from_handle(process);
  U64 result = dmn_w32_process_read(entity->handle, range, dst);
  return result;
}

internal B32
dmn_process_write(DMN_Handle process, Rng1U64 range, void *src)
{
  DMN_W32_Entity *entity = dmn_w32_entity_from_handle(process);
  B32 result = dmn_w32_process_write(entity->handle, range, src);
  return result;
}

//- rjf: threads

internal Architecture
dmn_arch_from_thread(DMN_Handle handle)
{
  DMN_W32_Entity *entity = dmn_w32_entity_from_handle(handle);
  return entity->arch;
}

internal U64
dmn_stack_base_vaddr_from_thread(DMN_Handle handle)
{
  U64 result = 0;
  DMN_W32_Entity *thread = dmn_w32_entity_from_handle(handle);
  if(thread->kind == DMN_W32_EntityKind_Thread)
  {
    DMN_W32_Entity *process = thread->parent;
    U64 tlb = thread->thread.thread_local_base;
    U64 result = 0;
    switch(thread->arch)
    {
      case Architecture_Null:
      case Architecture_arm64:
      case Architecture_arm32:
      case Architecture_COUNT:
      {NotImplemented;}break;
      case Architecture_x64:
      {
        U64 stack_base_addr = tlb + 0x8;
        dmn_w32_process_read(process->handle, r1u64(stack_base_addr, stack_base_addr+8), &result);
      }break;
      case Architecture_x86:
      {
        U64 stack_base_addr = tlb + 0x4;
        dmn_w32_process_read(process->handle, r1u64(stack_base_addr, stack_base_addr+4), &result);
      }break;
    }
  }
  return result;
}

internal U64
dmn_tls_root_vaddr_from_thread(DMN_Handle handle)
{
  U64 result = 0;
  DMN_W32_Entity *entity = dmn_w32_entity_from_handle(handle);
  if(entity->kind == DMN_W32_EntityKind_Thread)
  {
    result = entity->thread.thread_local_base;
  }
  return result;
}

internal B32
dmn_thread_read_reg_block(DMN_Handle handle, void *reg_block)
{
  DMN_W32_Entity *thread = dmn_w32_entity_from_handle(handle);
  B32 result = dmn_w32_thread_read_reg_block(thread->arch, thread->handle, reg_block);
  return result;
}

internal B32
dmn_thread_write_reg_block(DMN_Handle handle, void *reg_block)
{
  DMN_W32_Entity *thread = dmn_w32_entity_from_handle(handle);
  B32 result = dmn_w32_thread_write_reg_block(thread->arch, thread->handle, reg_block);
  return result;
}

////////////////////////////////
//~ rjf: @dmn_os_hooks System Process Listing (Implemented Per-OS)

internal void
dmn_process_iter_begin(DMN_ProcessIter *iter)
{
  MemoryZeroStruct(iter);
  iter->v[0] = (U64)CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
}

internal B32
dmn_process_iter_next(Arena *arena, DMN_ProcessIter *iter, DMN_ProcessInfo *info_out)
{
  B32 result = 0;
  
  //- rjf: get the next process entry
  PROCESSENTRY32W process_entry = {sizeof(process_entry)};
  HANDLE snapshot = (HANDLE)iter->v[0];
  if(iter->v[1] == 0)
  {
    if(Process32FirstW(snapshot, &process_entry))
    {
      result = 1;
    }
  }
  else
  {
    if(Process32NextW(snapshot, &process_entry))
    {
      result = 1;
    }
  }
  
  //- rjf: increment counter
  iter->v[1] += 1;
  
  //- rjf: convert to process info
  if(result)
  {
    info_out->name = str8_from_16(arena, str16_cstring((U16*)process_entry.szExeFile));
    info_out->pid = (U32)process_entry.th32ProcessID;
  }
  
  return result;
}

internal void
dmn_process_iter_end(DMN_ProcessIter *iter)
{
  CloseHandle((HANDLE)iter->v[0]);
  MemoryZeroStruct(iter);
}
