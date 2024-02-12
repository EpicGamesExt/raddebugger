// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_COFF_H
#define RADDBGI_COFF_H

////////////////////////////////
//~ COFF Format Types

typedef struct COFF_Guid{
  U32 data1;
  U16 data2;
  U16 data3;
  U32 data4;
  U32 data5;
} COFF_Guid;

#define COFF_ArchXList(X)\
X(UNKNOWN, 0x0)\
X(X86, 0x14c)\
X(X64, 0x8664)\
X(ARM33, 0x1d3)\
X(ARM, 0x1c0)\
X(ARM64, 0xaa64)\
X(ARMNT, 0x1c4)\
X(EBC, 0xebc)\
X(IA64, 0x200)\
X(M32R, 0x9041)\
X(MIPS16, 0x266)\
X(MIPSFPU, 0x366)\
X(MIPSFPU16, 0x466)\
X(POWERPC, 0x1f0)\
X(POWERPCFP, 0x1f1)\
X(R4000, 0x166)\
X(RISCV32, 0x5032)\
X(RISCV64, 0x5064)\
X(RISCV128, 0x5128)\
X(SH3, 0x1a2)\
X(SH3DSP, 0x1a3)\
X(SH4, 0x1a6)\
X(SH5, 0x1a8)\
X(THUMB, 0x1c2)\
X(WCEMIPSV2, 0x169)

typedef U16 COFF_Arch;
enum{
#define X(N,c) COFF_Arch_##N = c,
  COFF_ArchXList(X)
#undef X
};

#endif //COFF_H
