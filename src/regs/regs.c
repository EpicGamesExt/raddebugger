// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "regs/generated/regs.meta.c"

////////////////////////////////
//~ rjf: Helpers

internal U64
regs_rip_from_arch_block(Architecture arch, void *block)
{
  U64 result = 0;
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:{result = ((REGS_RegBlockX64 *)block)->rip.u64;}break;
    case Architecture_x86:{result = (U64)((REGS_RegBlockX86 *)block)->eip.u32;}break;
  }
  return result;
}

internal U64
regs_rsp_from_arch_block(Architecture arch, void *block)
{
  U64 result = 0;
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:{result = ((REGS_RegBlockX64 *)block)->rsp.u64;}break;
    case Architecture_x86:{result = (U64)((REGS_RegBlockX86 *)block)->esp.u32;}break;
  }
  return result;
}

internal void
regs_arch_block_write_rip(Architecture arch, void *block, U64 rip)
{
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:{((REGS_RegBlockX64 *)block)->rip.u64 = rip;}break;
    case Architecture_x86:{((REGS_RegBlockX86 *)block)->eip.u32 = (U32)rip;}break;
  }
}

internal void
regs_arch_block_write_rsp(Architecture arch, void *block, U64 rsp)
{
  switch(arch)
  {
    default:{}break;
    case Architecture_x64:{((REGS_RegBlockX64 *)block)->rsp.u64 = rsp;}break;
    case Architecture_x86:{((REGS_RegBlockX86 *)block)->esp.u32 = (U32)rsp;}break;
  }
}
