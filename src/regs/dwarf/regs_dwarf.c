// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal REGS_RegCode
reg_code_from_dw_reg_x64(DW_Reg reg)
{
  switch (reg) {
#define X(_SRC, _SRC_ID, _DST_ID, ...) case _SRC_ID: return REGS_RegCodeX64_##_DST_ID;
    DW_Regs_X64_XList
#undef X
  }
  return REGS_RegCodeX64_NULL;
}

internal REGS_RegCode
reg_code_from_dw_reg(Arch arch, DW_Reg reg)
{
  REGS_RegCode result = 0;
  switch (arch) {
  case Arch_Null: {} break;
  case Arch_x64: { result = reg_code_from_dw_reg_x64(reg); } break;
  case Arch_x86:
  case Arch_arm32:
  case Arch_arm64: { NotImplemented; } break;
  default: { InvalidPath; } break;
  }
  return result;
}

internal
MACHINE_OP_REG_READ(regs_read_dwarf_x64)
{
  // map DWARF register to -> the internal register
  REGS_RegBlockX64 *regs = ud;
  U64 reg_size = 0; void *reg_bytes = 0;
  switch (reg_id) {
#define X(_N, _ID, _MAP_N, ...) case _ID: { reg_size = sizeof(regs->_MAP_N); reg_bytes = &regs->_MAP_N; } break;
    DW_Regs_X64_XList
#undef X
    default: { InvalidPath; } break;
  }
  
  // copy out register value
  MachineOpResult status = MachineOpResult_Fail;
  if (reg_size > 0) {
    AssertAlways(reg_size == buffer_max);
    MemoryCopy(buffer, reg_bytes, reg_size);
    status = MachineOpResult_Ok;
  }
  
  return status;
}

internal
MACHINE_OP_REG_WRITE(regs_write_dwarf_x64)
{
  // map DWARF register to -> the internal register
  REGS_RegBlockX64 *regs = ud;
  U64 reg_size = 0; void *reg_bytes = 0;
  switch (reg_id) {
#define X(_N, _ID, _MAP_N, ...) case _ID: { reg_size = sizeof(regs->_MAP_N); reg_bytes = &regs->_MAP_N; } break;
    DW_Regs_X64_XList
#undef X
    default: { InvalidPath; } break;
  }
  
  // write value to the register
  MachineOpResult status = MachineOpResult_Fail;
  if (reg_size > 0) {
    AssertAlways(value_size <= reg_size);
    MemoryCopy(reg_bytes, value, value_size);
    status = MachineOpResult_Ok;
  }
  
  return status;
}

