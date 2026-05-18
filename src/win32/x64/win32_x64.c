// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
w32_x64_write_reg_block_from_thread_ctx(void *reg_block, void *thread_ctx)
{
  X64_RegBlock *dst = (X64_RegBlock *)reg_block;
  W32_X64_ThreadContext *src = (W32_X64_ThreadContext *)thread_ctx;
  
  //- rjf: convert context -> X64_RegBlock
  W32_X64_XSaveFormat *xsave = &src->FltSave;
  dst->rax = src->Rax;
  dst->rcx = src->Rcx;
  dst->rdx = src->Rdx;
  dst->rbx = src->Rbx;
  dst->rsp = src->Rsp;
  dst->rbp = src->Rbp;
  dst->rsi = src->Rsi;
  dst->rdi = src->Rdi;
  dst->r8  = src->R8;
  dst->r9  = src->R9;
  dst->r10 = src->R10;
  dst->r11 = src->R11;
  dst->r12 = src->R12;
  dst->r13 = src->R13;
  dst->r14 = src->R14;
  dst->r15 = src->R15;
  dst->rip = src->Rip;
  dst->cs  = src->SegCs;
  dst->ds  = src->SegDs;
  dst->es  = src->SegEs;
  dst->fs  = src->SegFs;
  dst->gs  = src->SegGs;
  dst->ss  = src->SegSs;
  dst->dr0 = src->Dr0;
  dst->dr1 = src->Dr1;
  dst->dr2 = src->Dr2;
  dst->dr3 = src->Dr3;
  dst->dr6 = src->Dr6;
  dst->dr7 = src->Dr7;
  // NOTE(rjf): this bit is "supposed to always be 1", according to old info.
  // may need to be investigated.
  dst->rflags = src->EFlags | 0x2;
  dst->fcw = xsave->ControlWord;
  dst->fsw = xsave->StatusWord;
  dst->ftw = xsave->TagWord;
  dst->fop = xsave->ErrorOpcode;
  MemoryCopy(&dst->fip, &xsave->ErrorOffset, sizeof(U64));
  MemoryCopy(&dst->fdp, &xsave->DataOffset, sizeof(U64));
  dst->mxcsr = xsave->MxCsr;
  dst->mxcsr_mask = xsave->MxCsr_Mask;
  {
    U128 *float_s = xsave->FloatRegisters;
    U80 *float_d = &dst->st0;
    for(U32 n = 0; n < 8; n += 1, float_s += 1, float_d += 1)
    {
      MemoryCopy(float_d, float_s, sizeof(*float_d));
    }
  }
  {
    U128 *xmm_s = xsave->XmmRegisters;
    U512 *zmm_d = &dst->zmm0;
    for(U32 n = 0; n < 16; n += 1, xmm_s += 1, zmm_d += 1)
    {
      MemoryCopy(zmm_d, xmm_s, sizeof(*xmm_s));
    }
  }
  
  // TODO(rjf): we need to determine how to do LocateXStateFeature without
  // actually running on Windows - what is that function actually looking at
  // & doing?
#if 0
  // AVX
  {
    DWORD avx_length = 0;
    U8 *avx_s = (U8 *)LocateXStateFeature(ctx, XSTATE_AVX, &avx_length);
    if(avx_length == 16 * sizeof(U128))
    {
      U512 *zmm_d = &dst->zmm0;
      for(U32 n = 0; n < 16; n += 1, avx_s += sizeof(U128), zmm_d += 1)
      {
        MemoryCopy(&zmm_d->u8[16], avx_s, sizeof(U128));
      }
    }
  }
  
  // AVX-512
  {
    // rjf: kmask
    DWORD kmask_length = 0;
    U64 *kmask_s = (U64*)LocateXStateFeature(ctx, XSTATE_AVX512_KMASK, &kmask_length);
    if(kmask_length == 8 * sizeof(U64))
    {
      U64 *kmask_d = &dst->k0;
      for(U32 n = 0; n < 8; n += 1, kmask_s += 1, kmask_d += 1)
      {
        MemoryCopy(kmask_d, kmask_s, sizeof(*kmask_s));
      }
    }
    
    // rjf: zmmh
    DWORD avx512h_length = 0;
    U8 *avx512h_s = (U8*)LocateXStateFeature(ctx, XSTATE_AVX512_ZMM_H, &avx512h_length);
    if(avx512h_length == 16 * sizeof(U256))
    {
      U512 *zmmh_d = &dst->zmm0;
      for(U32 n = 0; n < 16; n += 1, avx512h_s += sizeof(U256), zmmh_d += 1)
      {
        MemoryCopy(&zmmh_d->u8[32], avx512h_s, sizeof(U256));
      }
    }
    
    // rjf: zmm
    DWORD avx512_length = 0;
    U8 *avx512_s = (U8 *)LocateXStateFeature(ctx, XSTATE_AVX512_ZMM, &avx512_length);
    if(avx512_length == 16 * sizeof(U512))
    {
      U512 *zmm_d = &dst->zmm16;
      for(U32 n = 0; n < 16; n += 1, avx512_s += sizeof(U512), zmm_d += 1)
      {
        MemoryCopy(zmm_d, avx512_s, sizeof(U512));
      }
    }
  }
  
  // CET / Shadow Stack
  if(xstate_mask & XSTATE_MASK_CET_U)
  {
    DWORD cet_length = 0;
    XSAVE_CET_U_FORMAT *cet = LocateXStateFeature(ctx, XSTATE_CET_U, &cet_length);
    if (cet_length == sizeof(*cet))
    {
      dst->cetmsr = cet->Ia32CetUMsr;
      dst->cetssp = cet->Ia32Pl3SspMsr;
    }
  }
#endif
  return 1;
}
