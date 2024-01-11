// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include <windows.h>

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "syms_helpers/syms_internal_overrides.h"
#include "syms/syms_inc.h"
#include "syms_helpers/syms_helpers.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "syms_helpers/syms_internal_overrides.c"
#include "syms/syms_inc.c"
#include "syms_helpers/syms_helpers.c"

////////////////////////////////
// NOTE(allen): Win32 Demon Exceptions

#define DEMON_W32_EXCEPTION_BREAKPOINT               0x80000003u
#define DEMON_W32_EXCEPTION_SINGLE_STEP              0x80000004u
#define DEMON_W32_EXCEPTION_LONG_JUMP                0x80000026u
#define DEMON_W32_EXCEPTION_ACCESS_VIOLATION         0xC0000005u
#define DEMON_W32_EXCEPTION_ARRAY_BOUNDS_EXCEEDED    0xC000008Cu
#define DEMON_W32_EXCEPTION_DATA_TYPE_MISALIGNMENT   0x80000002u
#define DEMON_W32_EXCEPTION_GUARD_PAGE_VIOLATION     0x80000001u
#define DEMON_W32_EXCEPTION_FLT_DENORMAL_OPERAND     0xC000008Du
#define DEMON_W32_EXCEPTION_FLT_DEVIDE_BY_ZERO       0xC000008Eu
#define DEMON_W32_EXCEPTION_FLT_INEXACT_RESULT       0xC000008Fu
#define DEMON_W32_EXCEPTION_FLT_INVALID_OPERATION    0xC0000090u
#define DEMON_W32_EXCEPTION_FLT_OVERFLOW             0xC0000091u
#define DEMON_W32_EXCEPTION_FLT_STACK_CHECK          0xC0000092u
#define DEMON_W32_EXCEPTION_FLT_UNDERFLOW            0xC0000093u
#define DEMON_W32_EXCEPTION_INT_DIVIDE_BY_ZERO       0xC0000094u
#define DEMON_W32_EXCEPTION_INT_OVERFLOW             0xC0000095u
#define DEMON_W32_EXCEPTION_PRIVILEGED_INSTRUCTION   0xC0000096u
#define DEMON_W32_EXCEPTION_ILLEGAL_INSTRUCTION      0xC000001Du
#define DEMON_W32_EXCEPTION_IN_PAGE_ERROR            0xC0000006u
#define DEMON_W32_EXCEPTION_INVALID_DISPOSITION      0xC0000026u
#define DEMON_W32_EXCEPTION_NONCONTINUABLE           0xC0000025u
#define DEMON_W32_EXCEPTION_STACK_OVERFLOW           0xC00000FDu
#define DEMON_W32_EXCEPTION_INVALID_HANDLE           0xC0000008u
#define DEMON_W32_EXCEPTION_UNWIND_CONSOLIDATE       0x80000029u
#define DEMON_W32_EXCEPTION_DLL_NOT_FOUND            0xC0000135u
#define DEMON_W32_EXCEPTION_ORDINAL_NOT_FOUND        0xC0000138u
#define DEMON_W32_EXCEPTION_ENTRY_POINT_NOT_FOUND    0xC0000139u
#define DEMON_W32_EXCEPTION_DLL_INIT_FAILED          0xC0000142u
#define DEMON_W32_EXCEPTION_CONTROL_C_EXIT           0xC000013Au
#define DEMON_W32_EXCEPTION_FLT_MULTIPLE_FAULTS      0xC00002B4u
#define DEMON_W32_EXCEPTION_FLT_MULTIPLE_TRAPS       0xC00002B5u
#define DEMON_W32_EXCEPTION_NAT_CONSUMPTION          0xC00002C9u
#define DEMON_W32_EXCEPTION_HEAP_CORRUPTION          0xC0000374u
#define DEMON_W32_EXCEPTION_STACK_BUFFER_OVERRUN     0xC0000409u
#define DEMON_W32_EXCEPTION_INVALID_CRUNTIME_PARAM   0xC0000417u
#define DEMON_W32_EXCEPTION_ASSERT_FAILURE           0xC0000420u
#define DEMON_W32_EXCEPTION_NO_MEMORY                0xC0000017u
#define DEMON_W32_EXCEPTION_THROW                    0xE06D7363u

////////////////////////////////
// NOTE(allen): Win32 Demon Register API Codes

#define DEMON_W32_CTX_X86       0x00010000
#define DEMON_W32_CTX_X64       0x00100000

#define DEMON_W32_CTX_INTEL_CONTROL       0x0001
#define DEMON_W32_CTX_INTEL_INTEGER       0x0002
#define DEMON_W32_CTX_INTEL_SEGMENTS      0x0004
#define DEMON_W32_CTX_INTEL_FLOATS        0x0008
#define DEMON_W32_CTX_INTEL_DEBUG         0x0010
#define DEMON_W32_CTX_INTEL_EXTENDED      0x0020
#define DEMON_W32_CTX_INTEL_XSTATE        0x0040

#define DEMON_W32_CTX_X86_ALL (DEMON_W32_CTX_X86 | \
DEMON_W32_CTX_INTEL_CONTROL | DEMON_W32_CTX_INTEL_INTEGER | \
DEMON_W32_CTX_INTEL_SEGMENTS | DEMON_W32_CTX_INTEL_DEBUG | \
DEMON_W32_CTX_INTEL_EXTENDED)
#define DEMON_W32_CTX_X64_ALL (DEMON_W32_CTX_X64 | \
DEMON_W32_CTX_INTEL_CONTROL | DEMON_W32_CTX_INTEL_INTEGER | \
DEMON_W32_CTX_INTEL_SEGMENTS | DEMON_W32_CTX_INTEL_FLOATS | \
DEMON_W32_CTX_INTEL_DEBUG)

struct TEST_DebugEvent
{
  String8 name;
  U64 process_id;
  U64 thread_id;
  HANDLE process;
  HANDLE thread;
  U64 addr;
  DEBUG_EVENT evt;
};

struct TEST_Trap
{
  HANDLE process;
  U64 address;
};

internal U16
test_w32_real_tag_word_from_xsave(XSAVE_FORMAT *fxsave)
{
  U16 result = 0;
  U32 top = (fxsave->StatusWord >> 11) & 7;
  for (U32 fpr = 0; fpr < 8; fpr += 1){
    U32 tag = 3;
    if (fxsave->TagWord & (1 << fpr)){
      U32 st = (fpr - top)&7;
      
      SYMS_Reg80 *fp = (SYMS_Reg80*)&fxsave->FloatRegisters[st*16];
      U16 exponent = fp->sign1_exp15 & bitmask15;
      U64 integer_part  = fp->int1_frac63 >> 63;
      U64 fraction_part = fp->int1_frac63 & bitmask63;
      
      // tag: 0 - normal; 1 - zero; 2 - special
      tag = 2;
      if (exponent == 0){
        if (integer_part == 0 && fraction_part == 0){
          tag = 1;
        }
      }
      else if (exponent != bitmask15 && integer_part != 0){
        tag = 0;
      }
    }
    result |= tag << (2 * fpr);
  }
  return(result);
}

internal U16
test_w32_xsave_tag_word_from_real_tag_word(U16 ftw)
{
  U16 compact = 0;
  for (U32 fpr = 0; fpr < 8; fpr++){
    U32 tag = (ftw >> (fpr * 2)) & 3;
    if (tag != 3){
      compact |= (1 << fpr);
    }
  }
  return(compact);
}

internal B32
test_w32_read_x64_regs(HANDLE thread, SYMS_RegX64 *dst)
{
  Temp scratch = scratch_begin(0, 0);
  
  // NOTE(allen): Check available features
  U32 feature_mask = GetEnabledXStateFeatures();
  B32 avx_enabled = !!(feature_mask & XSTATE_MASK_AVX);
  
  // NOTE(allen): Setup the context
  CONTEXT *ctx = 0;
  U32 ctx_flags = DEMON_W32_CTX_X64_ALL;
  if (avx_enabled){
    ctx_flags |= DEMON_W32_CTX_INTEL_XSTATE;
  }
  DWORD size = 0;
  InitializeContext(0, ctx_flags, 0, &size);
  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
    void *ctx_memory = push_array(scratch.arena, U8, size);
    if (!InitializeContext(ctx_memory, ctx_flags, &ctx, &size)){
      ctx = 0;
    }
  }
  
  B32 avx_available = false;
  
  if (ctx != 0){
    // NOTE(allen): Finish Context Setup
    if (avx_enabled){
      SetXStateFeaturesMask(ctx, XSTATE_MASK_AVX);
    }
    
    // NOTE(allen): Determine what features are available on this particular ctx
    // TODO(allen): Experiment carefully with this nonsense.
    // Does avx_enabled = avx_available in all circumstances or not?
    DWORD64 xstate_flags = 0;
    if (GetXStateFeaturesMask(ctx, &xstate_flags)){
      if (xstate_flags & XSTATE_MASK_AVX){
        avx_available = true;
      }
    }
  }
  
  // get thread context
  HANDLE thread_handle = thread;
  if (!GetThreadContext(thread_handle, ctx)){
    ctx = 0;
  }
  
  B32 result = false;
  if (ctx != 0){
    result = true;
    
    // NOTE(allen): Convert CONTEXT -> SYMS_RegX64
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
    
    // NOTE(allen): This bit is "supposed to always be 1" I guess.
    // TODO(allen): Not sure what this is all about but I haven't investigated it yet.
    // This might be totally not necessary or something.
    dst->rflags.u64 = ctx->EFlags | 0x2;
    
    XSAVE_FORMAT *xsave = &ctx->FltSave;
    dst->fcw.u16 = xsave->ControlWord;
    dst->fsw.u16 = xsave->StatusWord;
    dst->ftw.u16 = test_w32_real_tag_word_from_xsave(xsave);
    dst->fop.u16 = xsave->ErrorOpcode;
    dst->fcs.u16 = xsave->ErrorSelector;
    dst->fds.u16 = xsave->DataSelector;
    dst->fip.u32 = xsave->ErrorOffset;
    dst->fdp.u32 = xsave->DataOffset;
    dst->mxcsr.u32 = xsave->MxCsr;
    dst->mxcsr_mask.u32 = xsave->MxCsr_Mask;
    
    M128A *float_s = xsave->FloatRegisters;
    SYMS_Reg80 *float_d = &dst->fpr0;
    for (U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1){
      MemoryCopy(float_d, float_s, sizeof(*float_d));
    }
    
    if (!avx_available){
      M128A *xmm_s = xsave->XmmRegisters;
      SYMS_Reg256 *xmm_d = &dst->ymm0;
      for (U32 n = 0; n < 16; n += 1, xmm_s += 1, xmm_d += 1){
        MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_s));
      }
    }
    
    if (avx_available){
      DWORD part0_length = 0;
      M128A *part0 = (M128A*)LocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &part0_length);
      DWORD part1_length = 0;
      M128A *part1 = (M128A*)LocateXStateFeature(ctx, XSTATE_AVX, &part1_length);
      Assert(part0_length == part1_length);
      
      DWORD count = part0_length/sizeof(part0[0]);
      count = ClampTop(count, 16);
      SYMS_Reg256 *ymm_d = &dst->ymm0;
      for (DWORD i = 0;
           i < count;
           i += 1, part0 += 1, part1 += 1, ymm_d += 1){
        // TODO(allen): Are we writing these out in the right order? Seems weird right?
        ymm_d->u64[3] = part0->Low;
        ymm_d->u64[2] = part0->High;
        ymm_d->u64[1] = part1->Low;
        ymm_d->u64[0] = part1->High;
      }
    }
    
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
test_w32_write_x64_regs(HANDLE thread, SYMS_RegX64 *src)
{
  Temp scratch = scratch_begin(0, 0);
  
  // NOTE(allen): Check available features
  U32 feature_mask = GetEnabledXStateFeatures();
  B32 avx_enabled = !!(feature_mask & XSTATE_MASK_AVX);
  
  // NOTE(allen): Setup the context
  CONTEXT *ctx = 0;
  U32 ctx_flags = DEMON_W32_CTX_X64_ALL;
  if (avx_enabled){
    ctx_flags |= DEMON_W32_CTX_INTEL_XSTATE;
  }
  DWORD size = 0;
  InitializeContext(0, ctx_flags, 0, &size);
  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
    void *ctx_memory = push_array(scratch.arena, U8, size);
    if (!InitializeContext(ctx_memory, ctx_flags, &ctx, &size)){
      ctx = 0;
    }
  }
  
  B32 avx_available = false;
  
  if (ctx != 0){
    // NOTE(allen): Finish Context Setup
    if (avx_enabled){
      SetXStateFeaturesMask(ctx, XSTATE_MASK_AVX);
    }
    
    // NOTE(allen): Determine what features are available on this particular ctx
    // TODO(allen): Experiment carefully with this nonsense.
    // Does avx_enabled = avx_available in all circumstances or not?
    DWORD64 xstate_flags = 0;
    if (GetXStateFeaturesMask(ctx, &xstate_flags)){
      if (xstate_flags & XSTATE_MASK_AVX){
        avx_available = true;
      }
    }
  }
  
  B32 result = false;
  if (ctx != 0){
    // NOTE(allen): Convert SYMS_RegX64 -> CONTEXT
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
    
    XSAVE_FORMAT *fxsave = &ctx->FltSave;
    fxsave->ControlWord = src->fcw.u16;
    fxsave->StatusWord = src->fsw.u16;
    fxsave->TagWord = test_w32_xsave_tag_word_from_real_tag_word(src->ftw.u16);
    fxsave->ErrorOpcode = src->fop.u16;
    fxsave->ErrorSelector = src->fcs.u16;
    fxsave->DataSelector = src->fds.u16;
    fxsave->ErrorOffset = src->fip.u32;
    fxsave->DataOffset = src->fdp.u32;
    
    M128A *float_d = fxsave->FloatRegisters;
    SYMS_Reg80 *float_s = &src->fpr0;
    for (U32 n = 0;
         n < 8;
         n += 1, float_s += 1, float_d += 1){
      MemoryCopy(float_d, float_s, 10);
    }
    
    if (!avx_available){
      M128A *xmm_d = fxsave->XmmRegisters;
      SYMS_Reg256 *xmm_s = &src->ymm0;
      for (U32 n = 0;
           n < 8;
           n += 1, xmm_d += 1, xmm_s += 1){
        MemoryCopy(xmm_d, xmm_s, sizeof(*xmm_d));
      }
    }
    
    if (avx_available){
      DWORD part0_length = 0;
      M128A *part0 = (M128A*)LocateXStateFeature(ctx, XSTATE_LEGACY_SSE, &part0_length);
      DWORD part1_length = 0;
      M128A *part1 = (M128A*)LocateXStateFeature(ctx, XSTATE_AVX, &part1_length);
      Assert(part0_length == part1_length);
      
      DWORD count = part0_length/sizeof(part0[0]);
      count = ClampTop(count, 16);
      SYMS_Reg256 *ymm_d = &src->ymm0;
      for (DWORD i = 0;
           i < count;
           i += 1, part0 += 1, part1 += 1, ymm_d += 1){
        // TODO(allen): Are we writing these out in the right order? Seems weird right?
        part0->Low  = ymm_d->u64[3];
        part0->High = ymm_d->u64[2];
        part1->Low  = ymm_d->u64[1];
        part1->High = ymm_d->u64[0];
      }
    }
    
    //- set thread context
    HANDLE thread_handle = thread;
    if (SetThreadContext(thread_handle, ctx)){
      result = true;
    }
  }
  
  scratch_end(scratch);
  return(result);
}

internal B32
test_w32_read_memory(HANDLE process_handle, void *dst, U64 src_address, U64 size)
{
  B32 result = true;
  U8 *ptr = (U8*)dst;
  U8 *opl = ptr + size;
  U64 cursor = src_address;
  for (;ptr < opl;){
    SIZE_T to_read = (SIZE_T)(opl - ptr);
    SIZE_T actual_read = 0;
    if (!ReadProcessMemory(process_handle, (LPCVOID)cursor, ptr, to_read, &actual_read)){
      result = false;
      break;
    }
    ptr += actual_read;
    cursor += actual_read;
  }
  return(result);
}

internal B32
test_w32_write_memory(HANDLE process_handle, U64 dst_address, void *src, U64 size)
{
  B32 result = true;
  U8 *ptr = (U8*)src;
  U8 *opl = ptr + size;
  U64 cursor = dst_address;
  for (;ptr < opl;){
    SIZE_T to_write = (SIZE_T)(opl - ptr);
    SIZE_T actual_write = 0;
    if (!WriteProcessMemory(process_handle, (LPVOID)cursor, ptr, to_write, &actual_write)){
      result = false;
      break;
    }
    ptr += actual_write;
    cursor += actual_write;
  }
  return(result);
}

internal B32
test_launch_process(OS_LaunchOptions *options)
{
  B32 result = false;
  Temp scratch = scratch_begin(0, 0);
  
  StringJoin join_params = {0};
  join_params.pre = str8_lit("\"");
  join_params.sep = str8_lit("\" \"");
  join_params.post = str8_lit("\"");
  String8 cmd = str8_list_join(scratch.arena, &options->cmd_line, &join_params);
  
  StringJoin join_params2 = {0};
  join_params2.sep = str8_lit("\0");
  join_params2.post = str8_lit("\0");
  String8 env = str8_list_join(scratch.arena, &options->env, &join_params2);
  
  String16 cmd16 = str16_from_8(scratch.arena, cmd);
  String16 dir16 = str16_from_8(scratch.arena, options->path);
  String16 env16 = str16_from_8(scratch.arena, env);
  
  DWORD access_flags = PROCESS_QUERY_INFORMATION | DEBUG_PROCESS | PROCESS_VM_READ | PROCESS_VM_WRITE;
  STARTUPINFOW startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {0};
  if (CreateProcessW(0, (WCHAR*)cmd16.str, 0, 0, 0, access_flags, (WCHAR*)env16.str, (WCHAR*)dir16.str,
                     &startup_info, &process_info))
  {
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    result = true;
  }
  
  scratch_end(scratch);
  return(result);
}

global HANDLE g_process = 0;
global DWORD g_process_id = 0;
global HANDLE g_thread1 = 0;
global DWORD g_thread1_id = 0;
global HANDLE g_thread2 = 0;
global DWORD g_thread2_id = 0;

internal TEST_DebugEvent
test_run_process(HANDLE step_thread, HANDLE suspend_thread, U64 traps_count, TEST_Trap *traps)
{
  Temp scratch = scratch_begin(0, 0);
  TEST_DebugEvent result = {0};
  
  //- rjf: freeze thread
  if(suspend_thread)
  {
    DWORD result = SuspendThread(suspend_thread);
    DWORD error = GetLastError();
    int x = 0;
  }
  
  //- rjf: write traps
  U8 *trap_swap_bytes = push_array_no_zero(scratch.arena, U8, traps_count);
  {
    TEST_Trap *trap = traps;
    for(U64 i = 0; i < traps_count; i += 1, trap += 1)
    {
      if(test_w32_read_memory(trap->process, trap_swap_bytes + i, trap->address, 1))
      {
        U8 int3 = 0xCC;
        test_w32_write_memory(trap->process, trap->address, &int3, 1);
      }
      else
      {
        trap_swap_bytes[i] = 0xCC;
      }
    }
  }
  
  //- rjf: set single step bit
  if(step_thread != 0)
  {
    SYMS_RegX64 regs = {0};
    test_w32_read_x64_regs(step_thread, &regs);
    regs.rflags.u64 |= 0x100;
    test_w32_write_x64_regs(step_thread, &regs);
  }
  
  //- rjf: continue
  local_persist B32 need_resume = 0;
  local_persist DWORD resume_pid = 0;
  local_persist DWORD resume_tid = 0;
  
  if(need_resume)
  {
    need_resume = 0;
    ContinueDebugEvent(resume_pid, resume_tid, DBG_CONTINUE);
  }
  
  //- rjf: get event
  DEBUG_EVENT evt = {0};
  if(WaitForDebugEvent(&evt, INFINITE))
  {
    need_resume = 1;
    resume_pid = evt.dwProcessId;
    resume_tid = evt.dwThreadId;
    result.evt = evt;
    
    switch(evt.dwDebugEventCode)
    {
      default:break;
      
      case CREATE_PROCESS_DEBUG_EVENT:
      {
        result.name = str8_lit("create process");
        result.process_id = evt.dwProcessId;
        result.process = evt.u.CreateProcessInfo.hProcess;
        result.thread_id = evt.dwThreadId;
        result.thread = evt.u.CreateProcessInfo.hThread;
        if(g_process == 0)
        {
          g_process = result.process;
          g_process_id = result.process_id;
        }
        if(g_thread1 == 0)
        {
          g_thread1 = result.thread;
          g_thread1_id = result.thread_id;
        }
      }break;
      
      case EXIT_PROCESS_DEBUG_EVENT:
      {
        result.name = str8_lit("exit process");
        result.process_id = evt.dwProcessId;
      }break;
      
      case CREATE_THREAD_DEBUG_EVENT:
      {
        result.name = str8_lit("create thread");
        result.thread_id = evt.dwThreadId;
        result.thread = evt.u.CreateThread.hThread;
        g_thread2 = result.thread;
        g_thread2_id = result.thread_id;
      }break;
      
      case EXIT_THREAD_DEBUG_EVENT:
      {
        result.name = str8_lit("exit thread");
        result.thread_id = evt.dwThreadId;
      }break;
      
      case LOAD_DLL_DEBUG_EVENT:
      {
        result.name = str8_lit("load dll");
      }break;
      
      case UNLOAD_DLL_DEBUG_EVENT:
      {
        result.name = str8_lit("unload dll");
      }break;
      
      case EXCEPTION_DEBUG_EVENT:
      {
        result.name = str8_lit("exception");
        
        EXCEPTION_DEBUG_INFO *edi = &evt.u.Exception;
        EXCEPTION_RECORD *exception = &edi->ExceptionRecord;
        
        switch(exception->ExceptionCode)
        {
          case DEMON_W32_EXCEPTION_BREAKPOINT:
          {
            result.name = str8_lit("breakpoint");
            result.addr = (U64)exception->ExceptionAddress;
            
            local_persist B32 did_first_bp = 0;
            if(did_first_bp != 0)
            {
              HANDLE thread = evt.dwThreadId == g_thread1_id ? g_thread1 : g_thread2;
              SYMS_RegX64 regs = {0};
              test_w32_read_x64_regs(thread, &regs);
              regs.rip.u64 = result.addr;
              test_w32_write_x64_regs(thread, &regs);
            }
            did_first_bp = 1;
            
          }break;
          
          case DEMON_W32_EXCEPTION_SINGLE_STEP:
          {
            result.name = str8_lit("single_step");
          }break;
          
          case DEMON_W32_EXCEPTION_THROW:
          {
            result.name = str8_lit("exception throw");
          }break;
          
          case DEMON_W32_EXCEPTION_ACCESS_VIOLATION:
          case DEMON_W32_EXCEPTION_IN_PAGE_ERROR:
          {
            result.name = str8_lit("exception access violation");
          }break;
          
          default:
          {
          }break;
        }
        
      }break;
      
      case OUTPUT_DEBUG_STRING_EVENT:
      {
        Temp scratch = scratch_begin(0, 0);
        result.name = str8_lit("output debug string");
        
        U64 string_address = (U64)evt.u.DebugString.lpDebugStringData;
        U64 string_size = (U64)evt.u.DebugString.nDebugStringLength;
        
        // TODO(allen): is the string in UTF-8 or UTF-16?
        
        U8 *buffer = push_array_no_zero(scratch.arena, U8, string_size + 1);
        test_w32_read_memory(g_process, buffer, string_address, string_size);
        buffer[string_size] = 0;
        
        printf("%s\n", buffer);
        scratch_end(scratch);
      }break;
      
      case RIP_EVENT:
      {
        result.name = str8_lit("rip event");
      }break;
      
    }
  }
  
  //- rjf: set single step bit
  if(step_thread != 0)
  {
    SYMS_RegX64 regs = {0};
    test_w32_read_x64_regs(step_thread, &regs);
    regs.rflags.u64 &= ~0x100;
    test_w32_write_x64_regs(step_thread, &regs);
  }
  
  //- rjf: unset traps
  {
    TEST_Trap *trap = traps;
    for(U64 i = 0; i < traps_count; i += 1, trap += 1)
    {
      U8 og_byte = trap_swap_bytes[i];
      if(og_byte != 0xCC)
      {
        test_w32_write_memory(trap->process, trap->address, &og_byte, 1);
      }
    }
  }
  
  //- rjf: check for more events
  for(int i = 0; i < 100; i += 1)
  {
    DEBUG_EVENT evt = {0};
    if(WaitForDebugEvent(&evt, 0))
    {
      int x = 0;
    }
  }
  
  //- rjf: resume thread
  if(suspend_thread)
  {
    ResumeThread(suspend_thread);
  }
  
  scratch_end(scratch);
  return result;
}

int
main(int argument_count, char **arguments)
{
  os_init(argument_count, arguments);
  Arena *arena = arena_alloc();
  
  U64 before_loop_stop_vaddr = 0x0000000140001089;
  U64 inner_loop_stop_vaddr = 0x0000000140001098;
  
  // rjf: launch
  {
    OS_LaunchOptions opts = {0};
    opts.path = os_get_path(arena, OS_SystemPath_Current);
    str8_list_push(arena, &opts.cmd_line, str8_lit("R:\\projects\\debugger\\build\\mule_loop_threads_win32.exe"));
    B32 launch_good = test_launch_process(&opts);
    int x = 0;
  }
  
  // rjf: get process/thread handles
  HANDLE process = 0;
  HANDLE thread1 = 0;
  U64 thread1_id = 0;
  {
    for(TEST_DebugEvent evt = {0};;)
    {
      evt = test_run_process(0, 0, 0, 0);
      if(evt.process)
      {
        process = evt.process;
      }
      if(evt.thread)
      {
        thread1 = evt.thread;
        thread1_id = evt.thread_id;
      }
      if(process != 0 && thread1 != 0)
      {
        break;
      }
    }
  }
  
  // rjf: get first breakpoint
  {
    for(TEST_DebugEvent evt = {0};;)
    {
      evt = test_run_process(0, 0, 0, 0);
      if(evt.evt.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
      {
        break;
      }
    }
  }
  
  //- rjf: run until 2nd thread starts up
  HANDLE thread2 = 0;
  U64 thread2_id = 0;
  {
    TEST_Trap traps[] =
    {
      {process, before_loop_stop_vaddr},
    };
    int trap_count = 0; //ArrayCount(traps);
    for(TEST_DebugEvent evt = {0};;)
    {
      evt = test_run_process(0, 0, trap_count, traps);
      if(str8_match(evt.name, str8_lit("breakpoint"), 0))
      {
        trap_count = 0;
      }
      if(evt.thread)
      {
        thread2 = evt.thread;
        thread2_id = evt.thread_id;
        break;
      }
    }
  }
  
  //- rjf: wait for bps
  {
    Temp scratch = scratch_begin(0, 0);
    TEST_Trap traps[] =
    {
      {process, 0x0000000140001098},
      {process, 0x00000001400010fb},
      {process, 0x00000001400010bc},
      {process, 0x00000001400010d7},
    };
    
    for(int i = 0;; i += 1)
    {
      TEST_DebugEvent evt = test_run_process(0, 0, 1, &traps[i % ArrayCount(traps)]);
      
      // rjf: check regs
      {
        U64 rip = 0;
        SYMS_RegX64 *regs = push_array(scratch.arena, SYMS_RegX64, 1);
        if(evt.evt.dwThreadId == g_thread1_id && test_w32_read_x64_regs(thread1, regs))
        {
          rip = regs->rip.u64;
        }
        if(evt.evt.dwThreadId == g_thread2_id && test_w32_read_x64_regs(thread2, regs))
        {
          rip = regs->rip.u64;
        }
        
        for(int i = 0; i < ArrayCount(traps); i += 1)
        {
          if(traps[i].address == rip)
          {
            printf("WRONG BP! 0x%I64x\n", rip);
            break;
          }
        }
      }
      
      if(str8_match(evt.name, str8_lit("breakpoint"), 0))
      {
        HANDLE step = 0;
        HANDLE suspend = 0;
        step = evt.evt.dwThreadId == thread2_id ? thread2 : thread1;
        suspend = step == thread2 ? thread1 : thread2;
        evt = test_run_process(step, suspend, 0, 0);
      }
    }
    scratch_end(scratch);
  }
  
  return 0;
}
