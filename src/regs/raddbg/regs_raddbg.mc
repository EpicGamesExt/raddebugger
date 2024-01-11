// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: RADDBG Converter Helper Implementation Generators

@table_gen @c_file
{
  `internal RADDBG_RegisterCode regs_raddbg_code_from_arch_reg_code(Architecture arch, REGS_RegCode code)`;
  `{`;
    `RADDBG_RegisterCode result = 0;`;
    `switch(arch)`;
    `{`;
      `default:{}break;`;
      `case Architecture_x64:`;
      `{`;
        `switch(code)`
          `{`;
          `default:{}break;`;
          @expand(REGS_RegTableX64 a) `case REGS_RegCodeX64_$(a.name):{result = RADDBG_RegisterCode_X64_$(a.name);}break;`;
          `}`;
        `}break;`;
      `case Architecture_x86:`;
      `{`;
        `switch(code)`
          `{`;
          `default:{}break;`;
          @expand(REGS_RegTableX86 a) `case REGS_RegCodeX86_$(a.name):{result = RADDBG_RegisterCode_X86_$(a.name);}break;`;
          `}`;
        `}break;`;
      `}`;
    `return result;`;
    `}`;
}

@table_gen @c_file
{
  `internal REGS_RegCode regs_reg_code_from_arch_raddbg_code(Architecture arch, RADDBG_RegisterCode code)`;
  `{`;
    `REGS_RegCode result = 0;`;
    `switch(arch)`;
    `{`;
      `default:{}break;`;
      `case Architecture_x64:`;
      `{`;
        `switch(code)`
          `{`;
          `default:{}break;`;
          @expand(REGS_RegTableX64 a) `case RADDBG_RegisterCode_X64_$(a.name):{result = REGS_RegCodeX64_$(a.name);}break;`;
          `}`;
        `}break;`;
      `case Architecture_x86:`;
      `{`;
        `switch(code)`
          `{`;
          `default:{}break;`;
          @expand(REGS_RegTableX86 a) `case RADDBG_RegisterCode_X86_$(a.name):{result = REGS_RegCodeX86_$(a.name);}break;`;
          `}`;
        `}break;`;
      `}`;
    `return result;`;
    `}`;
}
