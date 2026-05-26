// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef BASE_MACHINE_H
#define BASE_MACHINE_H

typedef enum MachineOpResult
{
  MachineOpResult_Null,
  MachineOpResult_Ok,
  MachineOpResult_Fail,
  MachineOpResult_Maybe,
} MachineOpResult;

#define MACHINE_OP_REG_READ(name) MachineOpResult name(U64 reg_id, void *buffer, U64 buffer_max, void *ud)
typedef MACHINE_OP_REG_READ(MachineOp_RegRead);

#define MACHINE_OP_REG_WRITE(name) MachineOpResult name(U64 reg_id, void *value, U64 value_size, void *ud)
typedef MACHINE_OP_REG_WRITE(MachineOp_RegWrite);

#define MACHINE_OP_MEM_READ(name) MachineOpResult name(U64 addr, void *buffer, U64 buffer_size, void *ud)
typedef MACHINE_OP_MEM_READ(MachineOp_MemRead);

#define MACHINE_OP_MEM_WRITE(name) MachineOpResult name(U64 addr, void *value, U64 value_size, void *ud)
typedef MACHINE_OP_MEM_WRITE(MachineOp_MemWrite);

////////////////////////////////

internal MachineOpResult machine_read_cstring_opl(Arena *arena, U64 vaddr, U64 vaddr_opl, MachineOp_MemRead *mem_read, void *mem_read_ud, String8 *cstr_out);

#endif // BASE_MACHINE_H
