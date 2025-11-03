// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "regs/generated/regs.meta.c"

////////////////////////////////
//~ rjf: Helpers

internal REGS_RegCode
regs_reg_code_from_name(Arch arch, String8 name)
{
  String8 *name_table = regs_reg_code_string_table_from_arch(arch);
  U64 name_count = regs_reg_code_count_from_arch(arch);
  for EachIndex(i, name_count)
  {
    if(str8_match(name_table[i], name, StringMatchFlag_CaseInsensitive))
    {
      return (REGS_RegCode)i;
    }
  }
  return 0;
}

internal REGS_AliasCode
regs_alias_code_from_name(Arch arch, String8 name)
{
  String8 *alias_table = regs_alias_code_string_table_from_arch(arch);
  U64 alias_count = regs_alias_code_count_from_arch(arch);
  for EachIndex(i, alias_count)
  {
    if(str8_match(alias_table[i], name, StringMatchFlag_CaseInsensitive))
    {
      return (REGS_RegCode)i;
    }
  }
  return 0;
}

internal Rng1U64
regs_range_from_code(Arch arch, B32 is_alias, U64 reg_code)
{
  Rng1U64 range = {0};
  if(is_alias)
  {
    REGS_Slice slice = regs_alias_code_slice_table_from_arch(arch)[reg_code];
    REGS_Rng   rng   = regs_reg_code_rng_table_from_arch(arch)[reg_code];
    range = r1u64(rng.byte_off + slice.byte_off, rng.byte_off + slice.byte_off + slice.byte_size);
  }
  else
  {
    REGS_Rng rng = regs_reg_code_rng_table_from_arch(arch)[reg_code];
    range = r1u64(rng.byte_off, rng.byte_off + rng.byte_size);
  }
  return range;
}

internal U64
regs_rip_from_arch_block(Arch arch, void *block)
{
  U64 result = 0;
  if(block != 0) switch(arch)
  {
    default:{}break;
    case Arch_x64:{result = ((REGS_RegBlockX64 *)block)->rip.u64;}break;
    case Arch_x86:{result = (U64)((REGS_RegBlockX86 *)block)->eip.u32;}break;
  }
  return result;
}

internal U64
regs_rsp_from_arch_block(Arch arch, void *block)
{
  U64 result = 0;
  if(block != 0) switch(arch)
  {
    default:{}break;
    case Arch_x64:{result = ((REGS_RegBlockX64 *)block)->rsp.u64;}break;
    case Arch_x86:{result = (U64)((REGS_RegBlockX86 *)block)->esp.u32;}break;
  }
  return result;
}

internal void
regs_arch_block_write_rip(Arch arch, void *block, U64 rip)
{
  if(block != 0) switch(arch)
  {
    default:{}break;
    case Arch_x64:{((REGS_RegBlockX64 *)block)->rip.u64 = rip;}break;
    case Arch_x86:{((REGS_RegBlockX86 *)block)->eip.u32 = (U32)rip;}break;
  }
}

internal void
regs_arch_block_write_rsp(Arch arch, void *block, U64 rsp)
{
  if(block != 0) switch(arch)
  {
    default:{}break;
    case Arch_x64:{((REGS_RegBlockX64 *)block)->rsp.u64 = rsp;}break;
    case Arch_x86:{((REGS_RegBlockX86 *)block)->esp.u32 = (U32)rsp;}break;
  }
}
