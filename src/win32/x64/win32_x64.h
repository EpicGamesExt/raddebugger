// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_X64_H
#define WIN32_X64_H

#pragma pack(push, 1)

typedef struct W32_X64_XSaveFormat W32_X64_XSaveFormat;
struct W32_X64_XSaveFormat
{
  U16 ControlWord;
  U16 StatusWord;
  U8 TagWord;
  U8 Reserved1;
  U16 ErrorOpcode;
  U32 ErrorOffset;
  U16 ErrorSelector;
  U16 Reserved2;
  U32 DataOffset;
  U16 DataSelector;
  U16 Reserved3;
  U32 MxCsr;
  U32 MxCsr_Mask;
  U128 FloatRegisters[8];
  U128 XmmRegisters[16];
  U8 Reserved4[96];
};

typedef struct W32_X64_ThreadContext W32_X64_ThreadContext;
struct W32_X64_ThreadContext
{
  U64 P1Home;
  U64 P2Home;
  U64 P3Home;
  U64 P4Home;
  U64 P5Home;
  U64 P6Home;
  U32 ContextFlags;
  U32 MxCsr;
  U16 SegCs;
  U16 SegDs;
  U16 SegEs;
  U16 SegFs;
  U16 SegGs;
  U16 SegSs;
  U32 EFlags;
  U64 Dr0;
  U64 Dr1;
  U64 Dr2;
  U64 Dr3;
  U64 Dr6;
  U64 Dr7;
  U64 Rax;
  U64 Rcx;
  U64 Rdx;
  U64 Rbx;
  U64 Rsp;
  U64 Rbp;
  U64 Rsi;
  U64 Rdi;
  U64 R8;
  U64 R9;
  U64 R10;
  U64 R11;
  U64 R12;
  U64 R13;
  U64 R14;
  U64 R15;
  U64 Rip;
  union
  {
    W32_X64_XSaveFormat FltSave;
    struct
    {
      U128 Header[2];
      U128 Legacy[8];
      U128 Xmm0;
      U128 Xmm1;
      U128 Xmm2;
      U128 Xmm3;
      U128 Xmm4;
      U128 Xmm5;
      U128 Xmm6;
      U128 Xmm7;
      U128 Xmm8;
      U128 Xmm9;
      U128 Xmm10;
      U128 Xmm11;
      U128 Xmm12;
      U128 Xmm13;
      U128 Xmm14;
      U128 Xmm15;
      U8 padding_0[96];
    };
  };
  U128 VectorRegister[26];
  U64 VectorControl;
  U64 DebugControl;
  U64 LastBranchToRip;
  U64 LastBranchFromRip;
  U64 LastExceptionToRip;
  U64 LastExceptionFromRip;
};

#pragma pack(pop)

internal B32 w32_x64_write_reg_block_from_thread_ctx(void *reg_block, void *thread_ctx);

#endif // WIN32_X64_H
