// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Abstracted Architecture Functions

internal ARCH_Info *
arch_info_from_arch(Arch arch)
{
  ARCH_Info *result = &arch_info_nil;
  switch(arch)
  {
    default:{}break;
#if defined(X64_H)
    case Arch_x64:
    {
      local_persist read_only ARCH_Info info =
      {
        .reg_block_size                  = sizeof(X64_RegBlock),
        .instruction_pointer_reg_code    = X64_RegCode_rip,
        .stack_pointer_reg_code          = X64_RegCode_rsp,
        .reg_code_count                  = X64_RegCode_COUNT,
        .reg_code_rng_table              = x64_reg_code_rng_table,
        .reg_code_name_table             = x64_reg_code_name_table,
        .reg_code_base_table             = x64_reg_code_base_table,
        .reg_code_is_vector_table        = x64_reg_code_is_vector_table,
      };
      result = &info;
    }break;
#endif
  }
  return result;
}

internal ARCH_RegCode
arch_reg_code_from_name(ARCH_Info *arch_info, String8 name)
{
  ARCH_RegCode result = 0;
  for EachIndex(code, arch_info->reg_code_count)
  {
    if(str8_match(arch_info->reg_code_name_table[code], name, 0))
    {
      result = code;
      break;
    }
  }
  return result;
}

internal ARCH_RegCode
arch_reg_code_from_rdi(Arch arch, RDI_RegCode code)
{
  ARCH_RegCode result = 0;
  {
    ARCH_Info *arch_info = arch_info_from_arch(arch);
    U8 *rdi_from_reg_code_table = arch_rdi_from_reg_code_table_from_arch(arch);
    for EachIndex(c, arch_info->reg_code_count)
    {
      if(rdi_from_reg_code_table[c] == code)
      {
        result = c;
        break;
      }
    }
  }
  return result;
}

internal ARCH_RegCode
arch_reg_code_from_dw(Arch arch, DW_Reg code)
{
  ARCH_RegCode result = 0;
  {
    ARCH_Info *arch_info = arch_info_from_arch(arch);
    U8 *dw_from_reg_code_table = arch_dw_from_reg_code_table_from_arch(arch);
    for EachIndex(c, arch_info->reg_code_count)
    {
      if(dw_from_reg_code_table[c] == code)
      {
        result = c;
        break;
      }
    }
  }
  return result;
}

internal B32
arch_reg_block_read_range(ARCH_Info *arch_info, void *block, Rng1U16 range, void *dst)
{
  B32 result = 0;
  {
    Rng1U16 legal_range = r1u16(0, arch_info->reg_block_size);
    Rng1U16 try_range = range;
    Rng1U16 read_range = intersect_1u16(legal_range, try_range);
    U64 read_size = dim_1u16(read_range);
    if(read_size != 0)
    {
      MemoryCopy(dst, (U8 *)block + read_range.min, read_size);
      result = 1;
    }
  }
  return result;
}

internal B32
arch_reg_block_write_range(ARCH_Info *arch_info, void *block, Rng1U16 range, void *src)
{
  B32 result = 0;
  Rng1U16 legal_range = r1u16(0, arch_info->reg_block_size);
  Rng1U16 try_range = range;
  Rng1U16 write_range = intersect_1u16(legal_range, try_range);
  U64 write_size = dim_1u16(write_range);
  if(write_size != 0)
  {
    MemoryCopy((U8 *)block + write_range.min, src, write_size);
    result = 1;
  }
  return result;
}

internal U64
arch_ip_from_reg_block(ARCH_Info *arch_info, void *block)
{
  U64 result = 0;
  ARCH_RegCode reg_code = arch_info->instruction_pointer_reg_code;
  if(reg_code < arch_info->reg_code_count)
  {
    arch_reg_block_read_range(arch_info, block, arch_info->reg_code_rng_table[reg_code], &result);
  }
  return result;
}

internal U64
arch_sp_from_reg_block(ARCH_Info *arch_info, void *block)
{
  U64 result = 0;
  ARCH_RegCode reg_code = arch_info->stack_pointer_reg_code;
  if(reg_code < arch_info->reg_code_count)
  {
    arch_reg_block_read_range(arch_info, block, arch_info->reg_code_rng_table[reg_code], &result);
  }
  return result;
}

internal B32
arch_reg_block_write_ip(ARCH_Info *arch_info, void *block, U64 ip)
{
  B32 result = 0;
  ARCH_RegCode reg_code = arch_info->instruction_pointer_reg_code;
  if(reg_code < arch_info->reg_code_count)
  {
    result = arch_reg_block_write_range(arch_info, block, arch_info->reg_code_rng_table[reg_code], &ip);
  }
  return result;
}

internal B32
arch_reg_block_write_sp(ARCH_Info *arch_info, void *block, U64 sp)
{
  B32 result = 0;
  ARCH_RegCode reg_code = arch_info->stack_pointer_reg_code;
  if(reg_code < arch_info->reg_code_count)
  {
    result = arch_reg_block_write_range(arch_info, block, arch_info->reg_code_rng_table[reg_code], &sp);
  }
  return result;
}
