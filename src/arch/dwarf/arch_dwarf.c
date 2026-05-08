// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal U8 *
arch_dw_from_reg_code_table_from_arch(Arch arch)
{
  U8 *result = &arch_reg_code_u8_nil;
  switch(arch)
  {
    default:{}break;
#if defined(X64_H)
    case Arch_x64:
    {
      result = dw_reg_code_from_x64_table;
    }break;
#endif
  }
  return result;
}
